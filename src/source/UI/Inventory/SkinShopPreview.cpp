#include "stdafx.h"
#include "UI/Inventory/SkinShopPreview.h"
#include "GameLogic/Cosmetics/CosmeticArmor.h"
#include "Render/Models/CosmeticArmor.h"
#include <RmlUi/Core/Element.h>
#include <cmath>

namespace UI::Inventory
{
namespace
{
constexpr float PreviewZoom = 1.1f;
constexpr float WingPreviewZoom = 0.8f;
constexpr float MountPreviewZoom = 0.9f;
constexpr float PreviewAngle = 90.f;
constexpr float DefaultPetScale = 0.5f;
constexpr float PetAnimationSpeed = 0.3f;
constexpr float PetSideOffset = 70.f;

}

SkinShopPreview::SkinShopPreview() : m_petAction(PetActionRound::Make())
{
    m_viewer.Init(0);
    m_viewer.SetStudioMode(true);
    m_viewer.SetAutoupdatePlayer(false);
    m_viewer.SetZoom(PreviewZoom);
    m_viewer.SetAngle(PreviewAngle);
    m_viewer.SetCompanionDrawer([this](CHARACTER& character) { DrawCompanion(character); });
    Select(-1);
}

SkinShopPreview::~SkinShopPreview()
{
    Disable();
    Render::Models::CosmeticArmor::SetPreviewCharacter(nullptr, -1);
}

void SkinShopPreview::Select(int type)
{
    m_type = type;
    m_pet.reset();
    m_viewer.CopyUnequippedPlayer();
    m_viewer.SetZoom(GameLogic::Cosmetics::IsFenrirMount(type) ? MountPreviewZoom
        : GameLogic::Cosmetics::ArmorPart(type) == 7 ? WingPreviewZoom : PreviewZoom);
    auto* character = m_viewer.GetPhotoChar();
    const int pet = GameLogic::Cosmetics::ArmorPart(type) == 8 ? type : -1;
    if (GameLogic::Cosmetics::IsFenrirMount(type)) m_viewer.SetFenrirPreview(type);
    else if (GameLogic::Cosmetics::IsCosmeticPet(pet) && character->Helper.Type != MODEL_HORN_OF_FENRIR)
        m_viewer.ClearHelper();
    const int part = GameLogic::Cosmetics::ArmorPart(type);
    if (part == 5 || part == 6)
        character->Weapon[part - 5].Type = MODEL_ITEM + GameLogic::Cosmetics::EquipmentVisualItem(type);
    else if (part == 7) character->Wing.Type = MODEL_ITEM + type;
    m_viewer.SetAnimation(AT_STAND1);
    Render::Models::CosmeticArmor::SetPreviewCharacter(character, type);
    if (!GameLogic::Cosmetics::IsCosmeticPet(pet)) return;
    m_pet = PetObject::Make();
    m_pet->Create(pet, MODEL_ITEM + pet, character->Object.Position, character, 0, 0);
    const auto* imported = GameLogic::Cosmetics::FindFinalMuSkin(pet);
    m_pet->SetScale(imported ? imported->scale : DefaultPetScale);
    for (int action = 0; action < PetObject::eAction_End; ++action)
        m_pet->SetActions(static_cast<PetObject::ActionType>(action), m_petAction, PetAnimationSpeed);
    m_pet->GetObject()->Alpha = 1.f;
}

void SkinShopPreview::Sync(Rml::Element* image)
{
    if (!image)
    {
        Disable();
        return;
    }
    const auto size = image->GetBox().GetSize(Rml::BoxArea::Content);
    if (size.x < 1 || size.y < 1) return;
    auto& target = m_viewer.Target();
    target.SetEnabled(true);
    target.Resize(static_cast<std::uint32_t>(std::lround(size.x)), static_cast<std::uint32_t>(std::lround(size.y)));
    const auto& source = target.Source();
    image->SetProperty("visibility", source.empty() ? "hidden" : "visible");
    if (!source.empty() && image->GetAttribute<Rml::String>("src", "") != source)
        image->SetAttribute("src", source);
}

void SkinShopPreview::DrawCompanion(CHARACTER& character)
{
    if (!m_pet) return;
    m_pet->Update(true);
    // Keep the companion beside the character so every pet fits the preview pane.
    auto* object = m_pet->GetObject();
    VectorCopy(character.Object.Position, object->Position);
    object->Position[1] += PetSideOffset;
    object->Position[2] += GameLogic::Cosmetics::CosmeticPetHeight(object->Type - MODEL_ITEM);
    object->Angle[2] = character.Object.Angle[2];
    if (!GameLogic::Cosmetics::IsFloatingPet(object->Type - MODEL_ITEM)) object->CurrentAction = 0;
    m_pet->Render(true);
}

void SkinShopPreview::SetLevel(int level)
{
    Render::Models::CosmeticArmor::SetPreviewCharacter(m_viewer.GetPhotoChar(), m_type, level);
}

void SkinShopPreview::Disable() { m_viewer.Target().SetEnabled(false); }
void SkinShopPreview::Turn(float degrees) { m_viewer.TurnBy(degrees); }
}
