import sys
from pathlib import Path
from typing import Literal

from pydantic import PrivateAttr

from daemomnify.chord_quality import ChordQuality
from daemomnify.chord_voicings.chord_voicing_style import ChordVoicingStyle
from daemomnify.chord_voicings.file_style import ChordFile


def get_bundled_data_dir() -> Path:
    """Get path to bundled chord voicing data files.

    Works in both development (running from source) and production
    (PyInstaller bundle).
    """
    if getattr(sys, "frozen", False):
        # Running in PyInstaller bundle - files are extracted to _MEIPASS
        return Path(sys._MEIPASS) / "chord_voicing_files"
    else:
        # Development - files are relative to this module
        return Path(__file__).parent / "chord_voicing_files"


# Registry of bundled chord voicing files (filenames only)
BUNDLED_CHORD_VOICING_FILES: list[str] = [
    "om_108_chord_voicing_offsets.json",
    "om_108_chord_voicings.json",
]


def get_bundled_file_info() -> list[tuple[str, str]]:
    """Get (filename, display_name) pairs for all bundled files.

    Reads each file to extract its 'name' field for display.
    Used by code generation to populate UI dropdowns.
    """
    result = []
    data_dir = get_bundled_data_dir()
    for filename in BUNDLED_CHORD_VOICING_FILES:
        path = data_dir / filename
        if path.exists():
            with open(path) as f:
                data = ChordFile.model_validate_json(f.read())
                result.append((filename, data.name))
    return result


class BundledFileStyle(ChordVoicingStyle):
    """Loads chord voicings from a bundled JSON file.

    Unlike FileStyle (which takes an arbitrary filesystem path),
    BundledFileStyle references files that are bundled with the application.
    The filename must be one of the files in BUNDLED_CHORD_VOICING_FILES.
    """

    type: Literal["BundledFileStyle"] = "BundledFileStyle"
    filename: str  # Just the filename, e.g. "om_108_chord_voicing_offsets.json"

    _data: ChordFile | None = PrivateAttr(default=None)

    def _get_full_path(self) -> Path:
        return get_bundled_data_dir() / self.filename

    def _get_data(self) -> ChordFile:
        if self._data is None:
            with open(self._get_full_path()) as f:
                self._data = ChordFile.model_validate_json(f.read())
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
