"""Check Tigris equipment, textures and the player skeleton on PC and mobile."""
import json
from pathlib import Path
import struct
import unittest

ROOT = Path(__file__).resolve().parents[1]
DATA = ROOT / "src/bin/Data"


def model_contents(path):
    raw = path.read_bytes()
    if raw[:4] == b"BMD\x0c":
        key = bytes.fromhex("D1 73 52 F6 D2 9A CB 27 3E AF 59 31 37 B3 E7 A2")
        assert struct.unpack_from("<I", raw, 4)[0] == len(raw) - 8
        previous = 0x5E
        data = bytearray()
        for index, value in enumerate(raw[8:]):
            data.append(((value ^ key[index % len(key)]) - previous) & 255)
            previous = (value + 0x3D) & 255
    else:
        assert raw[:4] == b"BMD\x0a", path
        data = raw[4:]
    meshes, bones, actions = struct.unpack_from("<3h", data, 32)
    offset = 38
    nodes, textures = set(), []
    for _ in range(meshes):
        vertices, normals, coords, triangles, _ = struct.unpack_from("<5h", data, offset)
        offset += 10
        for count, stride in [(vertices, 16), (normals, 20)]:
            nodes.update(struct.unpack_from("<h", data, offset + i * stride)[0] for i in range(count))
            offset += count * stride
        offset += coords * 8 + triangles * 64
        textures.append(data[offset:offset + 32].split(b"\0")[0].decode("ascii"))
        offset += 32
    keys = []
    for _ in range(actions):
        count, locked = struct.unpack_from("<h?", data, offset)
        keys.append(count)
        offset += 3 + (count * 12 if locked else 0)
    names = []
    for _ in range(bones):
        dummy = data[offset]
        offset += 1
        names.append("" if dummy else data[offset:offset + 32].split(b"\0")[0].decode("ascii"))
        if not dummy:
            offset += 34 + sum(keys) * 24
    assert offset == len(data), path
    return nodes, textures, names


def bone_name(name):
    for old, new in [(" Leg1", " Calf"), (" Leg", " Thigh"), (" Arm2", " Forearm"),
                     (" Arm1", " UpperArm"), (" Arm", " Clavicle")]:
        if name.endswith(old):
            return name[:-len(old)] + new
    return name


class TigrisItemsTest(unittest.TestCase):
    def test_items_keep_rage_fighter_stats_and_unique_numbers(self):
        for group in [0, 7, 8, 9, 11]:
            entries = json.loads(next((DATA / "Items").glob(f"Group{group:02}_*.json")).read_text(encoding="utf-8-sig"))["items"]
            numbers = [item["number"] for item in entries]
            self.assertEqual(len(numbers), len(set(numbers)))
            added = next(item for item in entries if item["number"] == (52 if group == 0 else 146))
            source = next(item for item in entries if item["number"] == (35 if group == 0 else 73))
            self.assertEqual(added["classRequirements"], {"rageFighter": 1})
            self.assertEqual(added["requirements"], {"level": 380, "strength": 101, "dexterity": 51}
                             if group == 0 else {"level": 380, "strength": 97})
            self.assertEqual({k: v for k, v in added.items() if k not in ("number", "name", "requirements")},
                             {k: v for k, v in source.items() if k not in ("number", "name", "requirements")})

    def test_assets_have_textures_and_matching_worn_bones(self):
        player = model_contents(DATA / "Player/Player.bmd")[2]
        checked = 0
        for path in (DATA / "Items/Models").glob("Group*.json"):
            for model in json.loads(path.read_text(encoding="utf-8-sig"))["models"]:
                if "/Custom/Tigris/" not in model.get("file", ""):
                    continue
                checked += 1
                file = ROOT / "src/bin" / model["file"]
                nodes, textures, bones = model_contents(file)
                self.assertNotIn("renderStyle", model)
                for texture in textures:
                    packed = Path(texture).with_suffix(".ozj" if texture.lower().endswith(".jpg") else ".ozt")
                    self.assertTrue((file.parent / packed).is_file(), texture)
                if file.name != "Sword37.bmd":
                    for node in nodes:
                        self.assertEqual(bone_name(bones[node]), bone_name(player[node]), file.name)
                effects = set(model.get("material", {}).get("additiveMeshes", []))
                for field in ("meshes", "shineMeshes"):
                    self.assertTrue(effects.isdisjoint(model.get("glow", {}).get(field, [])))
        self.assertEqual(checked, 7)


if __name__ == "__main__":
    unittest.main()
