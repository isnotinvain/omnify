from enum import Enum
from pathlib import Path
from typing import Annotated, Literal

from pydantic import BaseModel, Field, PrivateAttr, field_serializer, field_validator

from daemomnify import chords


def _serialize_chord_quality_dict(value: dict[int, chords.ChordQuality]) -> dict[int, str]:
    return {k: v.name for k, v in value.items()}


def _deserialize_chord_quality_dict(value: dict[int, str | chords.ChordQuality]) -> dict[int, chords.ChordQuality]:
    return {k: v if isinstance(v, chords.ChordQuality) else chords.ChordQuality[v] for k, v in value.items()}


class NotePerChordQuality(BaseModel):
    type: Literal["NotePerChordQuality"] = "NotePerChordQuality"
    note_mapping: dict[int, chords.ChordQuality]

    @field_serializer("note_mapping")
    def serialize_note_mapping(self, value: dict[int, chords.ChordQuality]) -> dict[int, str]:
        return _serialize_chord_quality_dict(value)

    @field_validator("note_mapping", mode="before")
    @classmethod
    def deserialize_note_mapping(cls, value: dict[int, str]) -> dict[int, chords.ChordQuality]:
        return _deserialize_chord_quality_dict(value)

    def handles_message(self, msg):
        if msg.type == "note_on" and msg.note in self.note_mapping:
            return self.note_mapping[msg.note]
        return None


class CCPerChordQuality(BaseModel):
    type: Literal["CCPerChordQuality"] = "CCPerChordQuality"
    cc_mapping: dict[int, chords.ChordQuality]

    @field_serializer("cc_mapping")
    def serialize_cc_mapping(self, value: dict[int, chords.ChordQuality]) -> dict[int, str]:
        return _serialize_chord_quality_dict(value)

    @field_validator("cc_mapping", mode="before")
    @classmethod
    def deserialize_cc_mapping(cls, value: dict[int, str]) -> dict[int, chords.ChordQuality]:
        return _deserialize_chord_quality_dict(value)

    def handles_message(self, msg):
        if msg.type == "control_change" and msg.control in self.cc_mapping:
            return self.cc_mapping[msg.control]
        return None


class CCRangePerChordQuality(BaseModel):
    type: Literal["CCRangePerChordQuality"] = "CCRangePerChordQuality"
    cc: int

    def handles_message(self, msg):
        if msg.type == "control_change" and msg.control == self.cc:
            chord_index = min(int(msg.value / chords.ChordQuality.chord_region_size), len(chords.ChordQuality.all) - 1)
            return chords.ChordQuality.all[chord_index]
        return None


ChordQualitySelectionStyle = Annotated[NotePerChordQuality | CCPerChordQuality | CCRangePerChordQuality, Field(discriminator="type")]


class ButtonAction(Enum):
    FLIP = 0
    ON = 1
    OFF = 2


class MidiNoteButton(BaseModel):
    type: Literal["MidiNoteButton"] = "MidiNoteButton"
    note: int

    def handles_message(self, msg):
        if msg.type == "note_on" and msg.note == self.note:
            return ButtonAction.FLIP
        return None


class MidiCCButton(BaseModel):
    type: Literal["MidiCCButton"] = "MidiCCButton"
    cc: int
    is_toggle: bool

    def handles_message(self, msg):
        if msg.type == "control_change" and msg.control == self.cc:
            if self.is_toggle:
                return ButtonAction.ON if msg.value > 64 else ButtonAction.OFF
            else:
                return ButtonAction.FLIP
        return None


MidiButton = Annotated[MidiNoteButton | MidiCCButton, Field(discriminator="type")]


# Chord voicing style configs (for sustained chord output)
class RootPositionChordStyleConfig(BaseModel):
    type: Literal["RootPositionChordStyle"] = "RootPositionChordStyle"
    _cached: chords.ChordVoicingStyle | None = PrivateAttr(default=None)

    def build(self, settings: DaemomnifySettings) -> chords.ChordVoicingStyle:
        return chords.RootPositionStyle(settings)

    def get(self, settings: DaemomnifySettings) -> chords.ChordVoicingStyle:
        if self._cached is None:
            self._cached = self.build(settings)
        return self._cached


class FileChordStyleConfig(BaseModel):
    type: Literal["FileChordStyle"] = "FileChordStyle"
    path: str
    _cached: chords.ChordVoicingStyle | None = PrivateAttr(default=None)

    def build(self, settings: DaemomnifySettings) -> chords.ChordVoicingStyle:
        return chords.FileStyle(settings, self.path)

    def get(self, settings: DaemomnifySettings) -> chords.ChordVoicingStyle:
        if self._cached is None:
            self._cached = self.build(settings)
        return self._cached


ChordStyleConfig = Annotated[RootPositionChordStyleConfig | FileChordStyleConfig, Field(discriminator="type")]


# Strum voicing style configs (for strum plate output)
class PlainAscendingStrumStyleConfig(BaseModel):
    type: Literal["PlainAscendingStrumStyle"] = "PlainAscendingStrumStyle"
    _cached: chords.ChordVoicingStyle | None = PrivateAttr(default=None)

    def build(self, settings: DaemomnifySettings) -> chords.ChordVoicingStyle:
        return chords.PlainAscendingStrumStyle(settings)

    def get(self, settings: DaemomnifySettings) -> chords.ChordVoicingStyle:
        if self._cached is None:
            self._cached = self.build(settings)
        return self._cached


class OmnichordStrumStyleConfig(BaseModel):
    type: Literal["OmnichordStrumStyle"] = "OmnichordStrumStyle"
    _cached: chords.ChordVoicingStyle | None = PrivateAttr(default=None)

    def build(self, settings: DaemomnifySettings) -> chords.ChordVoicingStyle:
        return chords.OmnichordStrumStyle(settings)

    def get(self, settings: DaemomnifySettings) -> chords.ChordVoicingStyle:
        if self._cached is None:
            self._cached = self.build(settings)
        return self._cached


StrumStyleConfig = Annotated[PlainAscendingStrumStyleConfig | OmnichordStrumStyleConfig, Field(discriminator="type")]


class DaemomnifySettings(BaseModel):
    midi_device_name: str
    chord_voicing_style: ChordStyleConfig
    chord_channel: int
    strum_channel: int
    strum_voicing_style: StrumStyleConfig
    strum_cooldown_secs: float
    strum_gate_time_secs: float
    chord_quality_selection_style: ChordQualitySelectionStyle
    strum_plate_cc: int
    latch_toggle_button: MidiButton
    stop_button: MidiButton

    # TODO: just make a set of these in the constructor
    def is_note_control_note(self, note: int) -> bool:
        match self.chord_quality_selection_style:
            case NotePerChordQuality() as m:
                if note in m.note_mapping:
                    return True
            case _:
                pass
        match self.latch_toggle_button:
            case MidiNoteButton() as b:
                if b.note == note:
                    return True
            case _:
                pass
        match self.stop_button:
            case MidiNoteButton() as b:
                if b.note == note:
                    return True
            case _:
                pass
        return False


def load_settings(path: Path = Path("daemomnify_settings.json")) -> DaemomnifySettings | None:
    try:
        json_str = path.read_text()
        return DaemomnifySettings.model_validate_json(json_str)
    except FileNotFoundError:
        print(f"I don't see a settings file at: {path}...")
        return None


def save_settings(settings: DaemomnifySettings, path: Path = Path("daemomnify_settings.json")) -> None:
    path.write_text(settings.model_dump_json(indent=2))


DEFAULT_SETTINGS: DaemomnifySettings = DaemomnifySettings(
    midi_device_name="Launchkey Mini MK3 MIDI Port",
    chord_voicing_style=RootPositionChordStyleConfig(),
    chord_channel=1,
    strum_channel=2,
    strum_voicing_style=OmnichordStrumStyleConfig(),
    strum_cooldown_secs=0.3,  # TODO: use cc
    strum_gate_time_secs=0.5,  # TODO: use cc
    chord_quality_selection_style=NotePerChordQuality(
        note_mapping={
            24: chords.ChordQuality.MAJOR,
            25: chords.ChordQuality.MINOR,
            26: chords.ChordQuality.DOM_7,
        }
    ),
    strum_plate_cc=1,
    latch_toggle_button=MidiCCButton(cc=102, is_toggle=True),
    stop_button=MidiCCButton(cc=103, is_toggle=False),
)

if __name__ == "__main__":
    print("Writing default settings file...")
    save_settings(DEFAULT_SETTINGS)
    print("Done!")
    print("Reading...")
    print(load_settings())
