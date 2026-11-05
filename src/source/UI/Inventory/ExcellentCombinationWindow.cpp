#include "stdafx.h"
#include "UI/Inventory/ExcellentCombinationWindow.h"
#include "GameLogic/Items/ExcellentCombination.h"
#include "UI/Core/WindowManager.h"
#include "UI/Core/WindowSystem.h"
#include "UI/Inventory/InventoryCtrl.h"
#include "UI/RmlBridge/RmlPointer.h"
#include "Core/Input/KeyState.h"
#include "Engine/Object/ZzzInventory.h"
#include "Core/Text/GroupedNumbers.h"
#include "Core/Utilities/StringUtils.h"
#include "Render/Items/ScopedItemEffects.h"
#include <RmlUi/Core.h>
namespace mu::ui::window {
namespace Combination=GameLogic::Items::ExcellentCombination;
CExcellentCombinationWindow* g_pExcellentCombinationWindow=nullptr;
CExcellentCombinationWindow::~CExcellentCombinationWindow() { Release(); }
void CExcellentCombinationWindow::Create(CManager* manager) {
    m_manager=manager;
    for(int i=0;i<3;++i) { m_targets[i]=std::make_unique<UI::Items::ItemCameraTarget>([this,i](const auto& p,const auto& s){DrawItem(i,p,s);}); m_targets[i]->SetContinuous(false); }
    m_view.Ensure(); manager->AddUIObj(INTERFACE_EXCELLENT_COMBINATION,this); Show(false);
}
void CExcellentCombinationWindow::Release() {
    Show(false); m_view.Release(); for(auto& target:m_targets) target.reset();
    if(m_manager) m_manager->RemoveUIObj(this); m_manager=nullptr;
}
void CExcellentCombinationWindow::Bind(Rml::DataModelConstructor& c,Model& m) {
    c.Bind("title",&m.title);c.Bind("has_title",&m.hasTitle);c.Bind("positioned",&m.positioned);c.Bind("root_x",&m.rootX);c.Bind("root_y",&m.rootY);
    c.Bind("status",&m.status);c.Bind("cost",&m.cost);c.Bind("can_combine",&m.canCombine);
    c.BindEventCallback("combination_close",[this](auto,auto&,const auto&){Show(false);});
    c.BindEventCallback("combination_inspect",[this](auto,auto&,const Rml::VariantList& a){if(a.size()==1) m_inspected=a[0].Get<int>(-1);});
    c.BindEventCallback("combination_execute",[this](auto,auto&,const auto&){if(m_view.GetModel().canCombine) Combination::Request(true,m_slots[0],m_slots[1]);});
}
void CExcellentCombinationWindow::Open() {
    CInventoryCtrl::BackupPickedItem(); Combination::Clear(); m_slots={-1,-1};m_present.fill(false);m_inspected=-1;
    g_pNewUISystem->Show(INTERFACE_INVENTORY);m_view.Ensure();
    m_view.GetModel().status="Drag items from your bag onto the two input slots."; m_view.MarkDirty("status");Show(true);
}
void CExcellentCombinationWindow::Show(bool visible) {
    CObject::Show(visible); if(visible && m_view.Document()) m_view.Document()->Show(); else m_view.Hide();
    if(!visible) { for(auto& target:m_targets) if(target) target->Disable(); Combination::Clear();m_present.fill(false); }
}
void CExcellentCombinationWindow::Select(int slot,int inventorySlot,ITEM* item) {
    if(slot<0 || slot>1 || Combination::Get().pending) return;
    Combination::Clear();m_present[2]=false;
    if(item && (item->Type<0 || item->Type>=MAX_ITEM || ItemAttribute[item->Type].m_byItemSlot>11 || !(item->ExcellentFlags&63) || m_slots[1-slot]==inventorySlot)) {
        m_view.GetModel().status="Select two different Excellent equipment items.";m_view.MarkDirty("status");return;
    }
    m_slots[slot]=inventorySlot;m_present[slot]=item!=nullptr;
    if(item) m_items[slot]=*item;
    for(auto& target:m_targets) target->Invalidate();
    if(m_present[0] && m_present[1]) Combination::Request(false,m_slots[0],m_slots[1]);
}
bool CExcellentCombinationWindow::Update() {
    Combination::Update(); if(!IsVisible()) return true;
    if(SceneFlag!=MAIN_SCENE || !g_pNewUISystem->IsVisible(INTERFACE_INVENTORY)) {Show(false);return true;}
    const auto& state=Combination::Get();
    if(m_revision!=state.revision) {
        m_revision=state.revision;
        if(state.success) {m_present[1]=false;m_slots[1]=-1;auto* item=g_pMyInventory->FindItem(m_slots[0]);if(item){m_items[0]=m_items[2]=*item;m_items[2].ExcellentFlags=state.excellentFlags;m_present[0]=m_present[2]=true;} }
        else if(state.valid) {m_items[2]=m_items[0];m_present[2]=true;}
        else m_present[2]=false;
        auto& m=m_view.GetModel();m.canCombine=state.valid && !state.pending;
        if(!state.status.empty())m.status=state.status;
        m.cost=StringUtils::WideToNarrow(Core::Text::GroupedNumbers(L"Required Zen: "+std::to_wstring(state.cost)).c_str());
        for(const char* field:{"can_combine","status","cost"})m_view.MarkDirty(field);
        for(auto& target:m_targets)target->Invalidate();
    }
    if(!state.pending && !state.success) for(int i=0;i<2;++i) if(m_present[i]) {
        auto* current=g_pMyInventory->FindItem(m_slots[i]);
        if(!current || current->Key!=m_items[i].Key || current->ExcellentFlags!=m_items[i].ExcellentFlags || current->Type!=m_items[i].Type) {Select(i,-1,nullptr);break;}
    }
    return true;
}
bool CExcellentCombinationWindow::UpdateMouseEvent() {
    if(!IsVisible() || !m_view.Document())return true;
    auto* doc=m_view.Document();if(IsPress(VK_LBUTTON))m_inspected=-1; if(!UI::RmlBridge::IsPointerWithin(doc->GetElementById("panel")))return true;
    auto* picked=CInventoryCtrl::GetPickedItem();
    if(picked && IsRelease(VK_LBUTTON) && picked->GetSourceStorageType()==STORAGE_TYPE::INVENTORY) {
        for(int i=0;i<2;++i)if(UI::RmlBridge::IsPointerWithin(doc->GetElementById("combination_slot_"+std::to_string(i))))Select(i,picked->GetSourceLinealPos(),picked->GetItem());
        CInventoryCtrl::BackupPickedItem();
    }
    return false;
}
bool CExcellentCombinationWindow::UpdateKeyEvent(){if(!IsVisible())return true;if(IsPress(VK_ESCAPE))Show(false);return false;}
void CExcellentCombinationWindow::DrawItem(int slot,const Rml::Vector2f& p,const Rml::Vector2f& s){
    if(!m_present[slot])return;const auto& item=m_items[slot];const Render::Items::ScopedItemEffects effects(false);
    RenderItem3DWithHover(p.x,p.y,s.x,s.y,item.Type,item.Level,item.ExcellentFlags,item.AncientDiscriminator,false);
}
bool CExcellentCombinationWindow::Render(){
    auto* doc=m_view.Document();if(!doc)return true;
    for(int i=0;i<3;++i){auto* image=doc->GetElementById("combination_item_"+std::to_string(i));m_targets[i]->Sync(image,IsVisible()&&m_present[i]);
        if(IsVisible() && m_present[i] && !CInventoryCtrl::GetPickedItem() && (m_inspected==i || UI::RmlBridge::IsPointerWithin(doc->GetElementById("combination_slot_"+std::to_string(i)))))RenderItemInfoAtPx(MouseX,MouseY,&m_items[i],false);
    }return true;
}
}
