import json
from dataclasses import dataclass
from enum import Enum
from typing import ClassVar, Literal

from pydantic import BaseModel, PrivateAttr
from pydantic_core import core_schema


@dataclass(frozen=True)
class ChordQualityData:
    nice_name: str
    offsets: tuple[int, ...]


class ChordQuality(Enum):
    MAJOR = ChordQualityData("Major", (0, 4, 7))
    MINOR = ChordQualityData("Minor", (0, 3, 7))
    DOM_7 = ChordQualityData("Dominant 7th", (0, 4, 7, 10))

    @classmethod
    def __get_pydantic_core_schema__(cls, source_type, handler):
        return core_schema.no_info_plain_validator_function(
            lambda v: v if isinstance(v, cls) else cls[v],
            serialization=core_schema.plain_serializer_function_ser_schema(lambda v: v.name),
        )


# TODO: maybe this should filter away multiples of 7? Maybe it never even comes up
def drop_fifth(offsets: tuple[int, ...]) -> tuple[int, ...]:
    """Converts a tetrad to a triad by dropping the 5th (index 2)."""
    if len(offsets) == 3:
        return offsets
    root, third, _fifth, seventh = offsets
    return (root, third, seventh)


ChordQuality.all = list(ChordQuality)
ChordQuality.chord_region_size = 128 / len(ChordQuality.all)


class ChordVoicingStyle(BaseModel):
    """
    Interface for constructing a chord given a quality and root note.
    Some styles will return notes below the root, that's fine, think of
    the root as the "anchor" of the chord. Styles can choose to use fixed octaves
    or be relative to the root.
    """

    def construct_chord(self, quality: ChordQuality, root: int) -> list[int]:
        raise NotImplementedError


class RootPositionStyle(ChordVoicingStyle):
    """
    Makes plain root position chords
    """

    type: Literal["RootPositionChordStyle"] = "RootPositionChordStyle"

    def construct_chord(self, quality: ChordQuality, root: int) -> list[int]:
        offsets = quality.value.offsets
        return [root + x for x in offsets]


class Omni84Style(ChordVoicingStyle):
    """
    Just outputs a single root note, in the octave corresponding to the chord quality using omni-84s octave layout
    Makes it easy to play omnify with omni-84 samples
    """

    type: Literal["Omni84Style"] = "Omni84Style"

    OCTAVE_BEGIN_MAP: ClassVar[dict[ChordQuality, int]] = {ChordQuality.MAJOR: 36, ChordQuality.MINOR: 48, ChordQuality.DOM_7: 60}

    def construct_chord(self, quality: ChordQuality, root: int) -> list[int]:
        pitch_class = root % 12
        octave_begin = self.OCTAVE_BEGIN_MAP[quality]
        return [octave_begin + pitch_class]


class FileStyle(ChordVoicingStyle):
    """
    Loads chord offset voicings from a json file
    """

    type: Literal["FileChordStyle"] = "FileChordStyle"
    path: str
    _data: dict | None = PrivateAttr(default=None)

    @staticmethod
    def _load_chord_offsets(path):
        with open(path) as f:
            data = json.load(f)
            result = {}
            for name, chord_map in data.items():
                result[name] = {}
                for note_str, offsets in chord_map.items():
                    result[name][int(note_str)] = offsets
            return result

    def _get_data(self):
        if self._data is None:
            self._data = self._load_chord_offsets(self.path)
        return self._data

    def construct_chord(self, quality: ChordQuality, root: int) -> list[int]:
        lookup = self._get_data()[quality.name]
        note_class = root % 12
        return lookup[note_class]


class PlainAscendingStrumStyle(ChordVoicingStyle):
    """
    Makes plain ascending strum sequences
    """

    type: Literal["PlainAscendingStrumStyle"] = "PlainAscendingStrumStyle"

    @staticmethod
    def make_strum_from_triad(offsets):
        res = []
        for shift in (-12, 0, 12, 24):
            for o in offsets:
                res.append(shift + o)
        # 13th element is one more octave up
        res.append(36)
        return res

    def construct_chord(self, quality: ChordQuality, root: int) -> list[int]:
        triad = drop_fifth(quality.value.offsets)
        strum_offsets = self.make_strum_from_triad(triad)
        return [root + x for x in strum_offsets]


class OmnichordStrumStyle(ChordVoicingStyle):
    """
    Mimics the omnichord strum voicing algorithm
    """

    type: Literal["OmnichordStrumStyle"] = "OmnichordStrumStyle"

    @staticmethod
    def clamp(lowest_f_sharp: int, n: int) -> int:
        pitch_class = n % 12
        distance_from_f_sharp = pitch_class - 6
        if distance_from_f_sharp < 0:
            distance_from_f_sharp += 12
        return lowest_f_sharp + distance_from_f_sharp

    @staticmethod
    def find_lowest_f_sharp(root: int) -> int:
        root_shifted = max(0, root - 6)
        root_octave = root_shifted // 12
        lowest_f_sharp = (root_octave * 12) + 6
        return lowest_f_sharp

    def construct_chord(self, quality: ChordQuality, root: int) -> list[int]:
        triad = drop_fifth(quality.value.offsets)
        res = []
        root_octave_start = self.find_lowest_f_sharp(root)
        for o in (-12, 0, 12, 24, 36):
            this_octave_start = root_octave_start + o
            for offset in triad:
                note = root + offset
                res.append(self.clamp(this_octave_start, note))

        return res[0:13]  # we only need 13 notes not 15
