from typing import Annotated, Literal

from pydantic import BaseModel, PrivateAttr

from daemomnify.chord_quality import ChordQuality
from daemomnify.chord_voicings.chord_voicing_style import ChordVoicingStyle
from daemomnify.vst_params import VSTString


class ChordFile(BaseModel):
    name: str
    description: str
    is_offset_file: bool
    chords: dict[ChordQuality, dict[int, list[int]]]


class FileStyle(ChordVoicingStyle):
    """
    Loads chord notes or offsets from a json file
    """

    type: Literal["FileStyle"] = "FileStyle"
    path: Annotated[str, VSTString(label="Chord File Path")]

    @classmethod
    def vst_label(cls) -> str:
        return "File"

    _data: ChordFile | None = PrivateAttr(default=None)

    @staticmethod
    def _load_chord_file(path: str) -> ChordFile:
        with open(path) as f:
            return ChordFile.model_validate_json(f.read())

    def _get_data(self) -> ChordFile:
        if self._data is None:
            self._data = self._load_chord_file(self.path)
        return self._data

    def construct_chord(self, quality: ChordQuality, root: int) -> list[int]:
        data = self._get_data()
        lookup = data.chords[quality]
        note_class = root % 12
        offsets_or_notes = lookup[note_class]

        if data.is_offset_file:
            notes = [root + x for x in offsets_or_notes]
        else:
            notes = offsets_or_notes
        return notes
