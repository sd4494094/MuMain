"""Binary regression checks for explicit NPC placement maintenance."""
import importlib.util
from pathlib import Path
import struct
import unittest

spec = importlib.util.spec_from_file_location("minimap", Path(__file__).resolve().parents[1]/"tools/update_minimap_npcs.py")
minimap = importlib.util.module_from_spec(spec)
spec.loader.exec_module(minimap)


def record(kind, x, y, name):
    return struct.pack("<B3xiii100s", kind, x, y, 0, name.encode())


def fixture():
    rows = [record(1, 130, 90, "幻影导师"), bytes(116), record(2, 121, 232, "地下城入口"), record(1, 59, 129, "商人")]
    rows += [bytes(116)]*(100-len(rows))
    payload = b"".join(minimap.xor_record(row) for row in rows)+bytes(range(45))
    return payload+struct.pack("<I", minimap.checksum(payload))


class MinimapTests(unittest.TestCase):
    def test_relocates_every_instance_and_preserves_portal_after_hole(self):
        original = fixture()
        placements = [{"name":"幻影导师","points":[{"x":130,"y":128},{"x":174,"y":129}]},
                      {"name":"洗点师·艾琳","points":[{"x":130,"y":133}]}]
        result = minimap.update(original, placements)
        rows = minimap.decode(result)
        original_rows = minimap.decode(original)
        self.assertEqual(rows[0], original_rows[2])
        self.assertEqual(rows[1], original_rows[3])
        self.assertEqual(result[-49:-4], original[-49:-4])
        self.assertEqual([row[0] for row in rows[:6]], [2,1,1,1,1,0])
        self.assertEqual(struct.unpack_from("<ii", rows[4], 4), (130,133))
        self.assertEqual(minimap.update(result, placements), result)

    def test_rejects_corruption(self):
        damaged=bytearray(fixture())
        damaged[-1] ^= 1
        with self.assertRaises(ValueError):
            minimap.update(damaged, [])

    def test_rejects_invalid_or_ambiguous_placements(self):
        for groups in [
            [{"name":"NPC","points":[{"x":256,"y":1}]}],
            [{"name":"NPC","points":[{"x":1.5,"y":1}]}],
            [{"name":"中"*34,"points":[]}],
            [{"name":"NPC","points":[]},{"name":"NPC","points":[]}],
            [{"name":"NPC","points":[{"x":1,"y":1},{"x":1,"y":1}]}],
            [{"name":"NPC","points":[{"x":i,"y":1} for i in range(100)]}],
        ]:
            with self.subTest(groups=groups), self.assertRaises(ValueError):
                minimap.update(fixture(), groups)


if __name__ == "__main__":
    unittest.main()
