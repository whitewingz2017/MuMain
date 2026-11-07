"""Keep upgrade glow off the transparent effect meshes of cosmetic skins."""
import json
from pathlib import Path
import unittest

DATA = Path(__file__).resolve().parents[1] / "src/bin/Data"
RENDER_TEXTURE = 2


class SkinGlowMeshesTest(unittest.TestCase):
    def test_upgrade_passes_exclude_effect_meshes(self):
        checked = 0
        for path in (DATA / "Items/Models").glob("Group*.json"):
            for model in json.loads(path.read_text(encoding="utf-8-sig"))["models"]:
                if "/Custom/Skins/" not in model.get("file", ""):
                    continue
                material = model.get("material", {})
                layers = material.get("layers", [])
                solid = {layer["mesh"] for layer in layers if layer["flags"] == RENDER_TEXTURE}
                effects = {layer["mesh"] for layer in layers} - solid
                effects.update(material.get("additiveMeshes", []))
                effects.update(animation["mesh"] for animation in material.get("animations", []))
                if not effects:
                    continue
                checked += 1
                with self.subTest(group=path.name, number=model["number"]):
                    glow = model.get("glow", {})
                    if glow.get("level") == 0:
                        continue
                    for field in ("meshes", "shineMeshes"):
                        selected = set(glow.get(field, []))
                        self.assertTrue(selected, field)
                        self.assertTrue(selected.isdisjoint(effects), field)
                        if layers:
                            self.assertEqual(selected, solid, field)
                    self.assertFalse(glow.get("excellent", True))
        self.assertGreater(checked, 0)


if __name__ == "__main__":
    unittest.main()
