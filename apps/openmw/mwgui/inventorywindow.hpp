#ifndef MGUI_Inventory_H
#define MGUI_Inventory_H

#include "container.hpp"
namespace MWWorld
{
    class Environment;
}

namespace MyGUI
{
  class Gui;
  class Widget;
}

namespace MWGui
{
    class WindowManager;
}


namespace MWGui
{
    class InventoryWindow : public MWGui::ContainerWindow
    {
        public:
            InventoryWindow(WindowManager& parWindowManager,MWWorld::Environment& environment,DragAndDrop* dragAndDrop);

            void openInventory();
    };
}
#endif // Inventory_H
