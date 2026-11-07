# Rank titles

A player with a rank title shows a badge above their name and an aura
around their body. The server decides who has which rank (VIP, purchased
tiers, ...); the client only draws what each rank looks like, from
`Data/Custom/RankTitles/RankTitles.json`. Every player in view shows their
rank, the hero included.
Equipped badges remain visible without hovering the player or displaying a name
or chat bubble. A confirmed shop refresh also restores the local badge.

Rank 0 is no title. A rank the file doesn't have shows nothing.

## The file

```json
{
  "formatVersion": 1,
  "ranks": [
    {
      "rank": 1,
      "badge": { "image": "Data/Custom/RankHornor/TwinMuVip.tga", "width": 75, "height": 40 },
      "aura": [
        { "type": "groundSeal", "sprite": "magic", "color": [1, 0.65, 0.2], "scale": 1.2, "spin": 20 }
      ]
    }
  ]
}
```

The file is read once, when the player first enters the game world; restart
the client after changing it. Mistakes are listed in `MuError.log` with
where they are (`ranks[2].aura[1].spin: is out of range`): a wrong field
keeps its default, a layer with an unknown `type`, `sprite` or `particle`
is left out, and the rest of the file still works.

### Badge

| Field | What it does |
|---|---|
| `image` | The badge image, relative to the client folder. Name it `.tga`: the game reads the packed `.OZT` of that name. Any size works. |
| `width`, `height` | Its size over the name, in interface units (the 640×480 layout). |
| `offsetX`, `offsetY` | Moves it from the centre above the name box. |
| `animation` | Optional looping badge above the character. Set `image` to a packed frame sheet, with `columns`, `rows`, `frames` and `frameMs` for its grid, frame count and milliseconds per frame. Frames play left to right and top to bottom. The badge's main image is the shop thumbnail and fallback. |

### Badge effects

Static badge images support an animated effect behind the badge. The supplied
presets are `BadgeGlow.tga`, `BadgeFire.tga`, and `BadgeLightning.tga` in
`Data/Custom/RankHornor/`. Each preset has 16 frames on a 4-by-4 sheet.
For example, add this inside a title's `badge` object:

```json
"effect": {
  "image": "Data/Custom/RankHornor/BadgeLightning.tga",
  "columns": 4, "rows": 4, "frames": 16, "frameMs": 65,
  "scale": 1.25, "opacity": 0.7
}
```

Change the image to choose a preset. `scale` controls its size relative to the
badge (0.5–3), `opacity` controls its strength (0–1), and `frameMs` controls
speed. Set opacity to 0 or remove `effect` to disable it. These settings affect
the equipped title above the head. Restart the client after editing the file
and distribute the same catalog and assets with PC and mobile.
The nine GIF badges use the supplied 180-by-45 artwork at a compact 64-by-16
display size. The equipped rank sits above the GM mark and the visible name,
guild, alliance, and chat box.
When no name box is displayed, a GM's rank anchors directly above the GM badge
instead of keeping the additional clearance used above an ordinary character's head.

### Aura layers

`aura` is a list of up to 16 layers, drawn in order. Combine them freely:
two `beams` layers with the same `count`, `period` and `spin` but different
`width` give a wide dim glow with a bright core.

| `type` | What it draws | Fields it reads |
|---|---|---|
| `halo` | A camera-facing disc above the head, drawn before the character so armour and wings cover its centre | texture, `color`, `size`, `height`, `depth`, `spin`, `pulse`, `blend` |
| `groundSeal` | A texture laid on the ground under the feet | texture, `color`, `scale`, `spin`, `pulse`, `blend` |
| `risingRings` | Rings that rise and widen, fading in and out | `color`, `count`, `radius`, `spread`, `width`, `height`, `rise`, `period`, `pulse`, `blend` |
| `beams` | Vertical streaks that shoot up around the body | `color`, `count`, `radius`, `width`, `length`, `height`, `rise`, `period`, `spin`, `pulse`, `blend` |
| `orbit` | Sprites circling the body | texture, `color`, `count`, `radius`, `height`, `bob`, `size`, `spin`, `pulse`, `blend` |
| `particles` | The game's particles, spawned around the body | `particle`, `subType`, `when`, `color`, `count`, `chance`, `radius`, `height`, `rise`, `scale` |

| Field | Meaning | Default |
|---|---|---|
| `sprite` | A texture by its name in `Data/Effects/SpriteTypes.json` (`magic`, `light`, `shiny2`, `flare`, ...) | — |
| `image` | An image of your own instead of a sprite, relative to the client folder | — |
| `particle` | A particle by its name in `Data/Effects/ParticleTypes.json` | — |
| `subType` | The particle's variant; what it does depends on the particle (`light` 1 fades out, `shiny` 0 sparkles) | 0 |
| `when` | `"always"`, or `"moving"` for a trail while the player walks | always |
| `color` | `[red, green, blue]`, 0 to 1 | `[1, 1, 1]` |
| `blend` | `"add"` brightens, `"subtract"` darkens (a dark core behind a bright halo) | add |
| `size` | Diameter of a halo or orbit sprite | 150 |
| `scale` | Size of a ground seal or particle | 1 |
| `height` | Height above the feet where the layer starts | 0 |
| `depth` | How far a halo sits behind the player, seen from the camera | 0 |
| `radius` | Distance from the body: rings, beams, orbit, where particles spawn | 70 |
| `spread` | How much rings widen while they rise | 0 |
| `rise` | How high rings and beams travel; height range of particles | 200 |
| `length` | Length of a beam | 60 |
| `width` | Width of a ring or beam | 2 |
| `bob` | Up and down swing of orbiting sprites | 0 |
| `count` | Rings, beams, orbiting sprites, or particles per frame (1 to 32) | 4 |
| `spin` | Turning speed in degrees per second; negative turns the other way | 0 |
| `period` | Milliseconds one ring or beam takes to rise | 2000 |
| `pulse` | Milliseconds of one breath of brightness (down to 35%); 0 is steady | 0 |
| `chance` | Chance a particle spawns each frame, 0 to 1 | 1 |

Everything scales with the character's size and fades with it. Auras are
hidden by cloaking, and when "render all effects" is off.

## Rank shop

Players buy rank titles for WCoinC and choose the one they show. Open it from
the window menu's **Rank Shop** row, or the **RANK** button of the modern
theme's top bar.

- **Shop** lists every title of `RankTitles.json`, in the file's order, with
  the server's price; owned titles say so, and a title the server doesn't
  sell shows `-`. Select one and press **Buy**. Titles are permanent.
- **My Ranks** lists the owned titles. **Equip** shows the selected one over
  your name and around your character; **Unequip** shows none. The title
  shown is marked with a green border.

The server decides prices, the balance and what a player owns. A title the
server sells but `RankTitles.json` doesn't have is not listed.
If the shop shows **WCoinC: Unavailable**, refresh it. Buying needs a current
shop response from the game server.

On OpenMU, the **Rank Titles** feature reads the account's WCoinC balance and
applies pending coin grants before purchases. The seventeen titles initially cost
100 WCoinC each, matching the skin shop default. Configure each title's price
in the server admin panel. Ownership is permanent per character.

Existing servers need the **RankTitles** database migration and the **TwinMU
Rank Titles** configuration update, followed by a game-server restart.
Servers with rank titles already installed can apply version 3 of **TwinMU
Rank Titles** to add the animated titles. Rebuild the PC and mobile clients
with the updated assets to display their animations.

The client catalog uses rank IDs 1 for VIP, 2 for Fighter, 3 for Legend,
4 for TwinMU, 5 for Slayer, 6 for Hunter, 7 for PvP King, and 8 for Top Tier.
The animated ranks are 9 Destroyer, 11 Hokage, 12 Hero, 13 Godlike, 14 Elite,
15 Beta Tester, 16 Soul Reaper, 17 Playboy, and 18 Immortal.
Rank 10 (TwinMU III) has been removed from the catalog.
The badge images are in `Data/Custom/RankHornor/`.

## Server packet

The server sends each player's rank to everyone who sees them: when the
player enters someone's view, and to everyone in view when the rank
changes. Rank 0 takes the title away.

`C2` with head code `FD`, sub code `07`, followed by UTF-8 JSON:

```
C2 <size hi> <size lo> FD 07 {"Ranks":[{"Id":1234,"Name":"Kevin","Rank":3}]}
```

| Field | Value |
|---|---|
| `Id` | The player's object id, as the viewport packets carry it (0 to 32767) |
| `Name` | The character name; an id the client has for a different name is ignored |
| `Rank` | 0 to 65535 |

`Ranks` holds up to 400 entries, so one packet can carry the whole view.

### Shop: C1/C2 FD 08

The client asks, the server answers every request with the whole shop:

```
C1 0F FD 08 <operation> <rank: 2 bytes> <price: 4 bytes> <request id: 4 bytes>
```

All numbers little-endian. `operation`:

| Value | Request | `rank` | `price` |
|---|---|---|---|
| 0 | Load the shop | 0 | 0 |
| 1 | Buy a title | the title | the price the shop showed; refuse it if it changed |
| 2 | Show a title | an owned title, or 0 for none | 0 |

The answer, `C2 <size hi> <size lo> FD 08` followed by UTF-8 JSON:

```json
{
  "RequestId": 7,
  "Status": "Ready",
  "Balance": 1200,
  "Active": 3,
  "Ranks": [
    { "Rank": 1, "Price": 250, "Owned": true, "Name": "Godlike" },
    { "Rank": 2, "Price": 500, "Owned": false }
  ]
}
```

| Field | Value |
|---|---|
| `RequestId` | The request's id; the client ignores an answer to any other |
| `Status` | `"Ready"`, or a message the shop shows (`"Not enough WCoinC"`) |
| `Balance` | The player's WCoinC |
| `Active` | The title the player shows, 0 for none |
| `Ranks` | Every title for sale: `Rank`, `Price`, `Owned`, and an optional `Name` (shown when the title has no badge image) |

After a purchase or a change of the shown title, also send the `FD 07` rank
update to everyone who sees the player, the player included. The client
gives up on a request the server hasn't answered in 8 seconds.

