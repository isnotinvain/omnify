import json
from dataclasses import dataclass
from enum import Enum
from pathlib import Path


def _load_arps_data(path):
    with open(path) as f:
        data = json.load(f)
        result = {}
        for name, arp_map in data.items():
            result[name] = {}
            for note_str, arp in arp_map.items():
                result[name][int(note_str)] = arp
        return result


def _load_chord_data(path):
    with open(path) as f:
        data = json.load(f)
        result = {}
        for name, chord_map in data.items():
            result[name] = {}
            for note_str, offsets in chord_map.items():
                result[name][int(note_str)] = offsets
        return result


_arps_data = _load_arps_data(path=Path(__file__).parent / "arps.json")
_chord_data = _load_chord_data(path=Path(__file__).parent / "chord_offsets.json")


@dataclass(frozen=True)
class ChordData:
    nice_name: str
    offsets: list[int]
    arp: dict[int, list[int]]


class Chord(Enum):
    MAJOR = ChordData("Major", _chord_data["MAJOR"], _arps_data["MAJOR"])
    MINOR = ChordData("Minor", _chord_data["MINOR"], _arps_data["MINOR"])
    DOM_7 = ChordData("Dominant 7th", _chord_data["DOMINANT_7"], _arps_data["DOMINANT_7"])

    def __repr__(self):
        return self.name


# Static members for Chord enum
Chord.chord_list = list(Chord)
Chord.chord_region_size = 128 / len(Chord.chord_list)
