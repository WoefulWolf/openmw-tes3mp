#ifndef OPENMW_PROCESSOROBJECTATTACH_HPP
#define OPENMW_PROCESSOROBJECTATTACH_HPP

#include "BaseObjectProcessor.hpp"

namespace mwmp
{
    class ProcessorObjectAttach final: public BaseObjectProcessor
    {
    public:
        ProcessorObjectAttach()
        {
            BPP_INIT(ID_OBJECT_ATTACH)
        }

        virtual void Do(ObjectPacket &packet, ObjectList &objectList)
        {
            BaseObjectProcessor::Do(packet, objectList);

            //objectList.attachObjects(ptrCellStore);
        }

    };
}

#endif //OPENMW_PROCESSOROBJECTATTACH_HPP
