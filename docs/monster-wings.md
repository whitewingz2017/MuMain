# Monster wings (2.5 wings)

Four wings between the second and third tier, crafted at the Chaos Goblin. The server side is OpenMU's
`MonsterWings` initializer and `MonsterWingsCrafting` (crafting number 56).

| Wing | Item | Material | Classes | Size | Defense | Damage increase | Absorption |
|---|---|---|---|---|---|---|---|
| Cloak of Death | 12:262 | Death King's Bone (14:176) | Dark Lord, Rage Fighter | 2x3 | 27 | 21% + 1%/level | 13% + 2%/level |
| Wings of Chaos | 12:263 | Hell Maine's Leather (14:177) | Blade Knight, Magic Gladiator | 4x3 | 46 | 33% + 1%/level | 30% + 2%/level |
| Wings of Power | 12:264 | Dark Phoenix Flame (14:178) | Soul Master, Magic Gladiator, Bloody Summoner | 4x3 | 37 | 35% + 1%/level | 29% + 2%/level |
| Wings of Life | 12:265 | Death Beam Knight's Soul (14:179) | Muse Elf | 4x3 | 37 | 35% + 1%/level | 29% + 2%/level |

All require level 290. Defense grows like the first wings (`defenseBonus1stWings`, profile `monsterWing` in
`Data/Items/Stats/ItemStats.json`). They are second tier wings in the item data, so they get the second wing
additional options (HP, mana, ignore defense) and the second wing handling of the mix.

## Recipe

One second wing (12:3-6, 12:42) or cape (12:49, 13:30), a Jewel of Chaos, a Jewel of Creation and one material.
The material decides the wing. Rate: second wing price / 9,000,000 or cape price / 500,000, at most 60%;
100,000 zen per percent. Talismans are not accepted. `Mix.bmd` has no such recipe, so `CMixRecipeMgr::LoadData`
adds it to the Chaos Goblin recipes (named "Create Wings").

## Drops

Hell Maine (309), Dark Phoenix (77) and Death Beam Knight (63) drop their material. OpenMU has no Death King,
so Death King's Bone drops from Lost Tower monsters of level 60 and up.

## Assets

`Data/Custom/MonsterWings/`: the wing models are Mudream's `cape_of_death`, `chaoswing`, `magic_wing` and
`flamewing` (BMD v15 and DTX textures converted to BMD v10 and OZT/OZJ); the materials come from 6.Main.
