#include "stdafx.h"
#include "UI/Inventory/CursorItemLayer.h"

#include <RmlUi/Core/ElementDocument.h>

#include "UI/Inventory/InventoryCtrl.h"
#include "UI/Inventory/ItemCameraTarget.h"
#include "UI/Inventory/MuunInventoryWindow.h"
#include "UI/RmlBridge/RmlDocumentVisibility.h"
#include "UI/RmlBridge/RmlThemedView.h"

namespace
{
using mu::ui::window::CInventoryCtrl;
using mu::ui::window::CPickedItem;
using mu::ui::window::g_pMuunInventoryWindow;

bool ShownMuunItem()
{
    return g_pMuunInventoryWindow && g_pMuunInventoryWindow->IsDraggingItem();
}

CPickedItem* ShownPickedItem()
{
    CPickedItem* picked = CInventoryCtrl::GetPickedItem();
    return picked && picked->IsVisible() && picked->GetItem() ? picked : nullptr;
}

struct CursorItemLayer
{
    UI::RmlBridge::ThemedView<> view{{{"Data/Interface/RmlUi/cursor_item.rml"}},
                                     {.stacking = UI::RmlBridge::ThemedStacking::Front}};
    UI::Items::ItemCameraTarget target{[](const Rml::Vector2f&, const Rml::Vector2f&)
                                       {
                                           if (CPickedItem* picked = ShownPickedItem())
                                               picked->Render3D();
                                           else if (ShownMuunItem())
                                               g_pMuunInventoryWindow->RenderDraggedItem();
                                       }};
};

// Outlives static destruction order: released explicitly while RmlUi is still up.
CursorItemLayer* g_Layer = nullptr;
} // namespace

void UI::Items::SyncCursorItemLayer()
{
    if (g_Layer == nullptr)
        g_Layer = new CursorItemLayer();
    g_Layer->view.Ensure();
    Rml::ElementDocument* document = g_Layer->view.Document();
    const bool shown = ShownPickedItem() != nullptr || ShownMuunItem();
    UI::RmlBridge::SyncDocumentVisibilityInFront(document, shown);
    g_Layer->target.Sync(document ? document->GetElementById("cursor_item") : nullptr, shown);
}

void UI::Items::ReleaseCursorItemLayer()
{
    if (g_Layer == nullptr)
        return;
    g_Layer->target.Disable();
    g_Layer->view.Release();
}
