#pragma once

#include "UI/Social/PhotoViewer.h"
#include "GameLogic/Pets/w_BasePet.h"
#include "GameLogic/Pets/w_PetActionRound.h"

namespace Rml { class Element; }

namespace UI::Inventory
{
class SkinShopPreview
{
public:
    SkinShopPreview();
    ~SkinShopPreview();
    void Select(int type);
    void Sync(Rml::Element* image);
    void Disable();
    void Turn(float degrees);
    void SetLevel(int level);

private:
    void DrawCompanion(CHARACTER& character);
    int m_type = -1;
    CUIPhotoViewer m_viewer;
    PetActionRoundPtr m_petAction;
    PetObjectPtr m_pet;
};
}
