import json
from abc import ABC, abstractmethod
from dataclasses import dataclass
from enum import Enum


def _load_chord_offsets(path):
    with open(path) as f:
        data = json.load(f)
        result = {}
        for name, chord_map in data.items():
            result[name] = {}
            for note_str, offsets in chord_map.items():
                result[name][int(note_str)] = offsets
        return result


# _chord_data = _load_chord_offsets(path=Path(__file__).parent / "chord_offsets.json")


@dataclass(frozen=True)
class ChordQualityData:
    nice_name: str
    json_file_key: str


class ChordQuality(Enum):
    MAJOR = ChordQualityData("Major", "MAJOR")
    MINOR = ChordQualityData("Minor", "MINOR")
    DOM_7 = ChordQualityData("Dominant 7th", "DOMINANT_7")


ChordQuality.all = list(ChordQuality)
ChordQuality.chord_region_size = 128 / len(ChordQuality.all)


class ChordBuilder(ABC):
    """
    Interface for building a chord given the root note of the chord.
    Some builders will return notes below the root, that's fine, think of
    the root as the "anchor" of the chord. Builders can choose to use fixed octaves
    or be relative to the root.
    """

    @abstractmethod
    def notes(self, root: int) -> list[int]: ...


class VoicingStyle(ABC):
    """
    Interface for a chord voicing style.
    Maps each chord quality to a ChordBuilder.
    """

    @abstractmethod
    def builders(self) -> dict[ChordQuality, ChordBuilder]: ...


class FixedOffsetBuilder(ChordBuilder):
    """
    offsets are semitones from the root
    add_sub is a mutable container for whether the user has toggled a bass root note
    """

    def __init__(self, offsets: list[int], add_sub: list[bool]):
        self.offsets = offsets
        # list of single element, used as a mutable container (or None)
        self.add_sub = add_sub

    def notes(self, root: int) -> list[int]:
        n = [root + x for x in self.offsets]
        if self.add_sub is not None and self.add_sub[0]:
            n.append(root - 12)
        return n


class LookupBuilder(ChordBuilder):
    """
    lookup is dict[note_class, offsets]
    offsets are semitones from the root
    """

    def __init__(self, lookup: dict[int, list[int]]):
        self.lookup = lookup

    def notes(self, root: int) -> list[int]:
        note_class = root % 12
        return self.lookup[note_class]


class RootPositionVoicingStyle(VoicingStyle):
    """
    Makes plain root position chords
    """

    def __init__(self, add_sub_ref: list[bool]):
        self.add_sub = add_sub_ref
        self._builders = {
            ChordQuality.MAJOR: FixedOffsetBuilder(offsets=[0, 4, 7], add_sub=self.add_sub),
            ChordQuality.MINOR: FixedOffsetBuilder(offsets=[0, 3, 7], add_sub=self.add_sub),
            ChordQuality.DOM_7: FixedOffsetBuilder(offsets=[0, 3, 7, 10], add_sub=self.add_sub),
        }

    def builders(self) -> dict[ChordQuality, ChordBuilder]:
        return self._builders


class FileVoicingStyle(VoicingStyle):
    """
    Loads chord offset voicings from a json file
    """

    def __init__(self, path):
        # dict[json_file_key, dict[note_class, list[offsets]]]
        self.data = _load_chord_offsets(path)
        self._builders = {}
        for quality in ChordQuality.all:
            self._builders[quality] = LookupBuilder(self.data[quality.value.json_file_key])

    def builders(self) -> dict[ChordQuality, ChordBuilder]:
        return self._builders


class PlainAscendingStrumStyle(VoicingStyle):
    """
    Makes plain ascending strum sequences
    """

    @staticmethod
    def make_strum_from_triad(offsets):
        res = []
        for shift in (-12, 0, 12, 24):
            for o in offsets:
                res.append(shift + o)

        # 13th element is one more octave up
        res.append(36)

    @staticmethod
    def make_strum_from_tetrad(offsets):
        """
        just drops the 5th
        """
        root, third, fifth, seventh = offsets
        return PlainAscendingStrumStyle.make_strum_from_triad([root, third, seventh])

    @staticmethod
    def make_strum_from_chord_offsets(offsets):
        match len(offsets):
            case 3:
                return PlainAscendingStrumStyle.make_strum_from_triad(offsets)
            case 4:
                return PlainAscendingStrumStyle.make_strum_from_tetrad(offsets)
            case _:
                raise ValueError("offsets should be length 3 or 4")

    def __init__(self):
        self._builders = {}
        root_style = RootPositionVoicingStyle(None)
        for quality, builder in root_style.builders().items():
            strum_offsets = PlainAscendingStrumStyle.make_strum_from_chord_offsets(builder.offsets)
            self._builders[quality] = FixedOffsetBuilder(strum_offsets, None)

    def builders(self) -> dict[ChordQuality, ChordBuilder]:
        return self._builders


class OmnichordStrumBuilder(ChordBuilder):
    def __init__(self, triad: list[int]):
        self.triad = triad

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

    def notes(self, root: int) -> list[int]:
        res = []
        root_octave_start = OmnichordStrumBuilder.find_lowest_f_sharp(root)
        for o in (-12, 0, 12, 24, 36):
            this_octave_start = root_octave_start + o
            for n in self.triad:
                res.append(OmnichordStrumBuilder.clamp(this_octave_start, n))

        return res[0:13]  # we only need 13 notes not 15


class OmnichordStrumStyle(VoicingStyle):
    """
    Mimics the omnichord strum voicing algorithm
    """

    def __init__(self):
        self._builders = {}
        root_style = RootPositionVoicingStyle(None)
        for quality, builder in root_style.builders().items():
            match builder.offsets:
                case 3:
                    triad = builder.offsets
                case 4:
                    root, third, fifth, seventh = builder.offsets
                    # drop 5th
                    triad = [root, third, seventh]
                case _:
                    raise ValueError("offsets should be length 3 or 4")

            self._builders[quality] = OmnichordStrumBuilder(triad)

    def builders(self) -> dict[ChordQuality, ChordBuilder]:
        return self._builders
