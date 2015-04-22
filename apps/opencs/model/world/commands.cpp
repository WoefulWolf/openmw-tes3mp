#include "commands.hpp"

#include <QAbstractItemModel>

#include "idtable.hpp"
#include "idtree.hpp"
#include <components/misc/stringops.hpp>
#include "nestedtablewrapper.hpp"

CSMWorld::ModifyCommand::ModifyCommand (QAbstractItemModel& model, const QModelIndex& index,
                                        const QVariant& new_, QUndoCommand* parent)
: QUndoCommand (parent), mModel (model), mIndex (index), mNew (new_)
{
    setText ("Modify " + mModel.headerData (mIndex.column(), Qt::Horizontal, Qt::DisplayRole).toString());
}

void CSMWorld::ModifyCommand::redo()
{
    mOld = mModel.data (mIndex, Qt::EditRole);
    mModel.setData (mIndex, mNew);
}

void CSMWorld::ModifyCommand::undo()
{
    mModel.setData (mIndex, mOld);
}


void CSMWorld::CreateCommand::applyModifications()
{
    for (std::map<int, QVariant>::const_iterator iter (mValues.begin()); iter!=mValues.end(); ++iter)
        mModel.setData (mModel.getModelIndex (mId, iter->first), iter->second);
}

CSMWorld::CreateCommand::CreateCommand (IdTable& model, const std::string& id, QUndoCommand* parent)
: QUndoCommand (parent), mModel (model), mId (id), mType (UniversalId::Type_None)
{
    setText (("Create record " + id).c_str());
}

void CSMWorld::CreateCommand::addValue (int column, const QVariant& value)
{
    mValues[column] = value;
}

void CSMWorld::CreateCommand::setType (UniversalId::Type type)
{
    mType = type;
}

void CSMWorld::CreateCommand::redo()
{
    mModel.addRecord (mId, mType);
    applyModifications();
}

void CSMWorld::CreateCommand::undo()
{
    mModel.removeRow (mModel.getModelIndex (mId, 0).row());
}

CSMWorld::RevertCommand::RevertCommand (IdTable& model, const std::string& id, QUndoCommand* parent)
: QUndoCommand (parent), mModel (model), mId (id), mOld (0)
{
    setText (("Revert record " + id).c_str());

    mOld = model.getRecord (id).clone();
}

CSMWorld::RevertCommand::~RevertCommand()
{
    delete mOld;
}

void CSMWorld::RevertCommand::redo()
{
    int column = mModel.findColumnIndex (Columns::ColumnId_Modification);

    QModelIndex index = mModel.getModelIndex (mId, column);
    RecordBase::State state = static_cast<RecordBase::State> (mModel.data (index).toInt());

    if (state==RecordBase::State_ModifiedOnly)
    {
        mModel.removeRows (index.row(), 1);
    }
    else
    {
        mModel.setData (index, static_cast<int> (RecordBase::State_BaseOnly));
    }
}

void CSMWorld::RevertCommand::undo()
{
    mModel.setRecord (mId, *mOld);
}

CSMWorld::DeleteCommand::DeleteCommand (IdTable& model, const std::string& id, QUndoCommand* parent)
: QUndoCommand (parent), mModel (model), mId (id), mOld (0)
{
    setText (("Delete record " + id).c_str());

    mOld = model.getRecord (id).clone();
}

CSMWorld::DeleteCommand::~DeleteCommand()
{
    delete mOld;
}

void CSMWorld::DeleteCommand::redo()
{
    int column = mModel.findColumnIndex (Columns::ColumnId_Modification);

    QModelIndex index = mModel.getModelIndex (mId, column);
    RecordBase::State state = static_cast<RecordBase::State> (mModel.data (index).toInt());

    if (state==RecordBase::State_ModifiedOnly)
    {
        mModel.removeRows (index.row(), 1);
    }
    else
    {
        mModel.setData (index, static_cast<int> (RecordBase::State_Deleted));
    }
}

void CSMWorld::DeleteCommand::undo()
{
    mModel.setRecord (mId, *mOld);
}


CSMWorld::ReorderRowsCommand::ReorderRowsCommand (IdTable& model, int baseIndex,
        const std::vector<int>& newOrder)
: mModel (model), mBaseIndex (baseIndex), mNewOrder (newOrder)
{}

void CSMWorld::ReorderRowsCommand::redo()
{
    mModel.reorderRows (mBaseIndex, mNewOrder);
}

void CSMWorld::ReorderRowsCommand::undo()
{
    int size = static_cast<int> (mNewOrder.size());
    std::vector<int> reverse (size);

    for (int i=0; i< size; ++i)
        reverse.at (mNewOrder[i]) = i;

    mModel.reorderRows (mBaseIndex, reverse);
}

CSMWorld::CloneCommand::CloneCommand (CSMWorld::IdTable& model,
                                      const std::string& idOrigin,
                                      const std::string& idDestination,
                                      const CSMWorld::UniversalId::Type type,
                                      QUndoCommand* parent)
: CreateCommand (model, idDestination, parent), mIdOrigin (idOrigin)
{
    setType (type);
    setText ( ("Clone record " + idOrigin + " to the " + idDestination).c_str());
}

void CSMWorld::CloneCommand::redo()
{
    mModel.cloneRecord (mIdOrigin, mId, mType);
    applyModifications();
}

void CSMWorld::CloneCommand::undo()
{
    mModel.removeRow (mModel.getModelIndex (mId, 0).row());
}

CSMWorld::DeleteNestedCommand::DeleteNestedCommand (IdTree& model,
                                                    const std::string& id,
                                                    int nestedRow,
                                                    int parentColumn,
                                                    QUndoCommand* parent) :
    mId(id),
    mModel(model),
    mParentColumn(parentColumn),
    QUndoCommand(parent),
    mNestedRow(nestedRow),
    NestedTableStoring(model, id, parentColumn)
{
    std::string title =
        model.headerData(parentColumn, Qt::Horizontal, Qt::DisplayRole).toString().toUtf8().constData();
    setText (("Delete row in " + title + " sub-table of " + mId).c_str());
}

void CSMWorld::DeleteNestedCommand::redo()
{
    const QModelIndex& parentIndex = mModel.getModelIndex(mId, mParentColumn);

    mModel.removeRows (mNestedRow, 1, parentIndex);
}


void CSMWorld::DeleteNestedCommand::undo()
{
    const QModelIndex& parentIndex = mModel.getModelIndex(mId, mParentColumn);

    mModel.setNestedTable(parentIndex, getOld());
}

CSMWorld::AddNestedCommand::AddNestedCommand(IdTree& model, const std::string& id, int nestedRow, int parentColumn, QUndoCommand* parent)
    : mModel(model),
      mId(id),
      mNewRow(nestedRow),
      mParentColumn(parentColumn),
      QUndoCommand(parent),
      NestedTableStoring(model, id, parentColumn)
{
    std::string title =
        model.headerData(parentColumn, Qt::Horizontal, Qt::DisplayRole).toString().toUtf8().constData();
    setText (("Add row in " + title + " sub-table of " + mId).c_str());
}

void CSMWorld::AddNestedCommand::redo()
{
    const QModelIndex& parentIndex = mModel.getModelIndex(mId, mParentColumn);

    mModel.addNestedRow (parentIndex, mNewRow);
}

void CSMWorld::AddNestedCommand::undo()
{
    const QModelIndex& parentIndex = mModel.getModelIndex(mId, mParentColumn);

    mModel.setNestedTable(parentIndex, getOld());
}

CSMWorld::NestedTableStoring::NestedTableStoring(const IdTree& model, const std::string& id, int parentColumn)
    : mOld(model.nestedTable(model.getModelIndex(id, parentColumn))) {}

CSMWorld::NestedTableStoring::~NestedTableStoring()
{
    delete mOld;
}

const CSMWorld::NestedTableWrapperBase& CSMWorld::NestedTableStoring::getOld() const
{
    return *mOld;
}

// Current interface does not allow adding a non-blank row, so we're forced to modify
// the whole record.
CSMWorld::ModifyPathgridCommand::ModifyPathgridCommand(IdTree& model,
    const std::string& id, int parentColumn, NestedTableWrapperBase* newRecord, QUndoCommand* parent)
    : mModel(model), mId(id), mParentColumn(parentColumn), mRecord(newRecord)
    , QUndoCommand(parent), NestedTableStoring(model, id, parentColumn)
{
    setText (("Modify Pathgrid record " + mId).c_str()); // FIXME: better description
}

void CSMWorld::ModifyPathgridCommand::redo()
{
    const QModelIndex& parentIndex = mModel.getModelIndex(mId, mParentColumn);

    mModel.setNestedTable(parentIndex, *mRecord);
}

void CSMWorld::ModifyPathgridCommand::undo()
{
    const QModelIndex& parentIndex = mModel.getModelIndex(mId, mParentColumn);

    mModel.setNestedTable(parentIndex, getOld());

    // FIXME: needs to tell the cell to redraw, possibly using signals
}
