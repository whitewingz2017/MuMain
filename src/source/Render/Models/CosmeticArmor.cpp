#include "stdafx.h"
#include "Render/Models/CosmeticArmor.h"
#include "Render/Models/CosmeticPreviewRules.h"
#include "Render/Models/ScopedPartOverride.h"
#include "GameLogic/Cosmetics/CosmeticArmor.h"
#include "GameLogic/Cosmetics/CosmeticAnimationRules.h"
#include "Data/GameData/ItemData/ItemModelSlots.h"
#include "Render/Models/ZzzBMD.h"
#include "Render/Effects/ZzzEffect.h"
#include "Engine/Object/ZzzObject.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Character/CharacterManager.h"
#include "World/MapInfra/MapManager.h"
#include <array>

namespace Render::Models::CosmeticArmor
{
namespace
{
CHARACTER* previewCharacter = nullptr;
int previewLevel = 0;
std::array<int, GameLogic::Cosmetics::SkinSlotCount> previewSkins{};
int VisibleItem(CHARACTER* character, int part)
{
    if (part < 0 || part >= GameLogic::Cosmetics::SkinSlotCount) return -1;
    return character == previewCharacter ? previewSkins[part] : GameLogic::Cosmetics::VisualItem(part);
}

// Transformation rings swap the body model but keep the linked weapons and wings, so equipment skins still apply.
bool EquipmentSkinsVisible(const CHARACTER* character, const OBJECT* object, bool preview)
{
    if (!preview && (character != Hero || !GameLogic::Cosmetics::Ready() || gMapManager.InChaosCastle())) return false;
    return character->Change || (object->Type == MODEL_PLAYER && object->SubType == 0);
}

void OverrideEquipment(PART_t& part, int skin, int level)
{
    part.Type = MODEL_ITEM + GameLogic::Cosmetics::EquipmentVisualItem(skin);
    part.Level = level;
    part.ExcellentFlags = 0;
    part.AncientDiscriminator = 0;
    part.m_byNumCloth = 0;
    part.m_pCloth[0] = nullptr;
    part.m_pCloth[1] = nullptr;
}
}

void SetPreviewCharacter(CHARACTER* character, int itemType, int level)
{
    previewCharacter = character;
    previewLevel = std::clamp(level, 0, 15);
    previewSkins = PreviewItems(itemType);
}

int WeaponModel(CHARACTER* character, int hand)
{
    if (!character || hand < 0 || hand >= 2) return -1;
    const bool preview = previewCharacter && character == previewCharacter;
    if (EquipmentSkinsVisible(character, &character->Object, preview))
    {
        constexpr int FirstWeaponPart = 5;
        const int skin = VisibleItem(character, FirstWeaponPart + hand);
        if (skin >= 0) return MODEL_ITEM + GameLogic::Cosmetics::EquipmentVisualItem(skin);
    }
    return character->Weapon[hand].Type;
}

int SkillWeaponModel(const OBJECT* owner)
{
    if (!owner) return -1;
    // Equipment refreshes can outlive the cast's cached weapon; resolve the local skin at draw time.
    if (Hero && owner == &Hero->Object) return WeaponModel(Hero, 0);
    return Data::Items::ToModelSlot(owner->Weapon);
}

bool Render(CHARACTER* character, int selection)
{
    const bool preview = previewCharacter && character == previewCharacter;
    if ((!preview && (character != Hero || !GameLogic::Cosmetics::Ready())) || character->Change
        || character->Object.Type != MODEL_PLAYER || character->Object.SubType != 0 || (!preview && gMapManager.InChaosCastle()))
        return false;
    const int baseClass = gCharacterManager.GetBaseClass(character->Class);
    const auto visibleArmor = [baseClass, character, preview](int part)
    {
        const int skin = VisibleItem(character, part);
        return !preview && GameLogic::Cosmetics::IsFemaleArmor(skin) && baseClass != CLASS_ELF && baseClass != CLASS_SUMMONER ? -1 : skin;
    };
    bool hasSkin = false;
    for (int part = 0; part < GameLogic::Cosmetics::ArmorPartCount; ++part)
        hasSkin = hasSkin || visibleArmor(part) >= 0;
    if (!hasSkin) return false;
    for (int body = 0; body < MAX_BODYPART; ++body)
    {
        if (body == BODYPART_HEAD && visibleArmor(0) >= 0) continue;
        const int skin = visibleArmor(body - BODYPART_HELM);
        PART_t* part = &character->BodyPart[body];
        const int model = skin >= 0 ? MODEL_ITEM + skin : part->Type;
        if (model < 0 || ::Models[model].NumMeshs <= 0) continue;
        RenderPartObject(&character->Object, model, skin >= 0 ? nullptr : part, character->Light,
            character->Object.Alpha, skin >= 0 ? (preview ? previewLevel : 0) : part->Level,
            skin >= 0 ? 0 : part->ExcellentFlags, skin >= 0 ? 0 : part->AncientDiscriminator,
            false, false, true, selection);
    }
    return true;
}

void WithEquipment(CHARACTER* character, OBJECT* object, int selection,
    void (*draw)(CHARACTER*, OBJECT*, int))
{
    ScopedPartOverride<PART_t> right(character->Weapon[0]);
    ScopedPartOverride<PART_t> left(character->Weapon[1]);
    ScopedPartOverride<PART_t> wing(character->Wing);
    ScopedPartOverride<PART_t>* restore[] = {&right, &left, &wing};
    const bool preview = character == previewCharacter;
    bool wingSkin = GameLogic::Cosmetics::ArmorPart(character->Wing.Type - MODEL_ITEM) == 7;
    if (EquipmentSkinsVisible(character, object, preview))
    {
        PART_t* slots[] = {&character->Weapon[0], &character->Weapon[1], &character->Wing};
        for (int part = 5; part <= 7; ++part)
        {
            const int skin = VisibleItem(character, part);
            if (skin < 0) continue;
            restore[part - 5]->Save();
            OverrideEquipment(*slots[part - 5], skin, preview ? previewLevel : 0);
            wingSkin = wingSkin || part == 7;
        }
    }
    if (!wingSkin) character->CosmeticWingClock.Reset();
    draw(character, object, selection);
}

void Release()
{
    GameLogic::Cosmetics::Reset();
}
}
