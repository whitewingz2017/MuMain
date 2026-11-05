#pragma once
#include "UI/Core/WindowObject.h"
#include "UI/RmlBridge/RmlThemedView.h"
#include <vector>

namespace mu::ui::window
{
class CManager;
class CCombatStatisticsWindow : public CObject
{
public:
    ~CCombatStatisticsWindow() override;
    void Create(CManager* manager);
    void Show(bool visible) override;
    bool Update() override;
    bool Render() override { return true; }
    bool UpdateMouseEvent() override;
    bool UpdateKeyEvent() override;
    float GetLayerDepth() override { return 5.2f; }
    Rml::ElementDocument* GetFillDocument() const override { return m_view.Document(); }
    Rml::ElementDocument* GetPlacedDocument() const override { return m_view.Document(); }
private:
    struct Model { std::vector<Rml::String> statistics; };
    void Bind(Rml::DataModelConstructor& constructor, Model& model);
    CManager* m_manager = nullptr;
    unsigned long long m_lastRequest = 0;
    UI::RmlBridge::ThemedView<Model> m_view{"combat_statistics",
        [this](Rml::DataModelConstructor& c, Model& m) { Bind(c, m); },
        {{"Data/Interface/RmlUi/combat_statistics.rml"}}};
};
}
