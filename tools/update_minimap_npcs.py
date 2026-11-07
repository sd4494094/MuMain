"""Update explicit NPC placements in a licensed MU minimap without changing portals.

Input is the original BMD plus JSON [{"name": "...", "points": [{"x": 1, "y": 2}]}].
Use a separate output file, back up the installed resource, and verify against server spawns.
"""
import argparse
import json
from pathlib import Path
import struct

CAPACITY = 100
RECORD_SIZE = 116
TAIL_SIZE = 45
CHECKSUM_KEY = 0x2BC1


def checksum(data):
    result = CHECKSUM_KEY << 9
    for offset in range(0, len(data) - 3, 4):
        value = struct.unpack_from("<I", data, offset)[0]
        result = result ^ value if (offset // 4 + CHECKSUM_KEY) % 2 == 0 else (result + value) & 0xFFFFFFFF
        if offset % 16 == 0:
            result ^= ((CHECKSUM_KEY + result) & 0xFFFFFFFF) >> ((offset // 4) % 8 + 1)
    return result


def xor_record(data):
    return bytes(value ^ (0xFC, 0xCF, 0xAB)[index % 3] for index, value in enumerate(data))


def decode(data):
    if len(data) != CAPACITY * RECORD_SIZE + TAIL_SIZE + 4:
        raise ValueError("Unexpected minimap file size")
    if checksum(data[:-4]) != struct.unpack_from("<I", data, len(data)-4)[0]:
        raise ValueError("Invalid minimap checksum")
    return [xor_record(data[i*RECORD_SIZE:(i+1)*RECORD_SIZE]) for i in range(CAPACITY)]


def update(data, placements):
    records = decode(data)
    replacements = {}
    for group in placements:
        name = group["name"]
        encoded = name.encode("utf-8")
        if not encoded or len(encoded) >= 100 or "\0" in name or name in replacements:
            raise ValueError("NPC names must be unique and fit the UTF-8 field")
        replacements[name] = []
        for point in group["points"]:
            x, y = point["x"], point["y"]
            if type(x) is not int or type(y) is not int or not (0 <= x < 256 and 0 <= y < 256):
                raise ValueError("NPC coordinates must be integer tiles from 0 to 255")
            record = struct.pack("<B3xiii100s", 1, x, y, 0, encoded)
            if record in replacements[name]:
                raise ValueError("Duplicate NPC placement")
            replacements[name].append(record)
    kept = []
    for record in records:
        if record[0] == 0:
            continue
        # Only replace the requested NPC names. Portals and other record kinds remain byte exact.
        name = record[16:].split(b"\0", 1)[0].decode("utf-8") if record[0] == 1 else None
        if record[0] != 1 or name not in replacements:
            kept.append(record)
    result = kept + [record for group in replacements.values() for record in group]
    if len(result) > CAPACITY:
        raise ValueError("Minimap capacity exceeded")
    result += [bytes(RECORD_SIZE)] * (CAPACITY-len(result))
    # Compact holes: the game stops reading visible markers at the first Kind == 0.
    payload = b"".join(xor_record(record) for record in result) + data[-TAIL_SIZE-4:-4]
    return payload + struct.pack("<I", checksum(payload))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", required=True, type=Path)
    parser.add_argument("--placements", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    if args.input.resolve() == args.output.resolve() or args.output.exists():
        parser.error("Choose a new output path; originals must stay intact")
    result = update(args.input.read_bytes(), json.loads(args.placements.read_text(encoding="utf-8")))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(result)
    print("Wrote validated NPC minimap:", args.output)


if __name__ == "__main__":
    main()
