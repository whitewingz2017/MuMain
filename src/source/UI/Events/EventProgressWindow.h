#pragma once
#include "UI/Core/WindowObject.h"
#include "UI/RmlBridge/RmlThemedView.h"
#include <vector>

namespace mu::ui::window
{
class CManager;
// Blood Castle / Devil Square objective panel (event_progress.rml): shown while the hero is in
// either event map, fed by GameLogic/Events/EventProgress. Takes no pointer events.
class CEventProgressWindow : public CObject
{
public:
    ~CEventProgressWindow() override;
    void Create(CManager* manager);
    bool Update() override;
    bool Render() override { return true; }
    bool UpdateMouseEvent() override { return true; }
    bool UpdateKeyEvent() override { return true; }
    float GetLayerDepth() override { return 1.2f; }

    struct Objective
    {
        Rml::String label, countdown, state;
        bool operator==(const Objective&) const = default;
    };
    struct Model
    {
        Rml::String name, time, notice, points, rank;
        bool scores = false;
        std::vector<Objective> objectives;
    };

private:
    void Bind(Rml::DataModelConstructor& constructor, Model& model);
    CManager* m_manager = nullptr;
    unsigned long long m_lastSync = 0;
    UI::RmlBridge::ThemedView<Model> m_view{"event_progress",
        [this](Rml::DataModelConstructor& c, Model& m) { Bind(c, m); },
        {{"Data/Interface/RmlUi/event_progress.rml"}},
        {.stacking = UI::RmlBridge::ThemedStacking::Back}};
};
}
