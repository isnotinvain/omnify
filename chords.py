import json
from dataclasses import dataclass
from enum import Enum
from pathlib import Path


# Load arps data from JSON
def _load_arps_data():
    arps_path = Path(__file__).parent / "arps.json"
    with open(arps_path) as f:
        data = json.load(f)
        result = {}
        for name, arp_map in data.items():
            result[name] = {}
            for note_str, arp in arp_map.items():
                result[name][int(note_str)] = arp
        return result


_arps_data = _load_arps_data()


@dataclass(frozen=True)
class ChordData:
    nice_name: str
    offsets: list[int]
    arp: dict[int, list[int]]


class Chord(Enum):
    MAJOR = ChordData("Major", [0, 4, 7], _arps_data["MAJOR"])
    MINOR = ChordData("Minor", [0, 3, 7], _arps_data["MINOR"])
    DOM_7 = ChordData("Dominant 7th", [0, 4, 7, 10], _arps_data["DOMINANT_7"])

    def __repr__(self):
        return self.name
