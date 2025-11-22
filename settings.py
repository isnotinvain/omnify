from pathlib import Path
from typing import Annotated, Literal

from pydantic import BaseModel, Field, field_serializer, field_validator

from chords import Chord


def _serialize_chord_dict(value: dict[int, Chord]) -> dict[int, str]:
    return {k: v.name for k, v in value.items()}


def _deserialize_chord_dict(value: dict[int, str | Chord]) -> dict[int, Chord]:
    return {k: v if isinstance(v, Chord) else Chord[v] for k, v in value.items()}


class NotePerChordMode(BaseModel):
    type: Literal["NotePerChordMode"] = "NotePerChordMode"
    note_mapping: dict[int, Chord]

    @field_serializer("note_mapping")
    def serialize_note_mapping(self, value: dict[int, Chord]) -> dict[int, str]:
        return _serialize_chord_dict(value)

    @field_validator("note_mapping", mode="before")
    @classmethod
    def deserialize_note_mapping(cls, value: dict[int, str]) -> dict[int, Chord]:
        return _deserialize_chord_dict(value)


class CCPerChordMode(BaseModel):
    type: Literal["CCPerChordMode"] = "CCPerChordMode"
    cc_mapping: dict[int, Chord]

    @field_serializer("cc_mapping")
    def serialize_cc_mapping(self, value: dict[int, Chord]) -> dict[int, str]:
        return _serialize_chord_dict(value)

    @field_validator("cc_mapping", mode="before")
    @classmethod
    def deserialize_cc_mapping(cls, value: dict[int, str]) -> dict[int, Chord]:
        return _deserialize_chord_dict(value)


class CCRangePerChordMode(BaseModel):
    type: Literal["CCRangePerChordMode"] = "CCRangePerChordMode"
    cc: int


ChordModeMidiMapStyle = Annotated[NotePerChordMode | CCPerChordMode | CCRangePerChordMode, Field(discriminator="type")]


class MidiNoteButton(BaseModel):
    type: Literal["MidiNoteButton"] = "MidiNoteButton"
    note: int


class MidiCCButton(BaseModel):
    type: Literal["MidiCCButton"] = "MidiCCButton"
    cc: int
    is_toggle: bool


MidiButton = Annotated[MidiNoteButton | MidiCCButton, Field(discriminator="type")]


class DaemomnifySettings(BaseModel):
    midi_device_name: str
    chord_channel: int
    strum_channel: int
    strum_cooldown_secs: float
    strum_gate_time_secs: float
    chord_midi_map_style: ChordModeMidiMapStyle
    strum_plate_cc: int
    latch_toggle_button: MidiButton
    stop_button: MidiButton

    # TODO: just make a set of these in the constructor
    def is_note_control_note(self, note: int) -> bool:
        match self.chord_midi_map_style:
            case NotePerChordMode() as m:
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
    chord_channel=1,
    strum_channel=2,
    strum_cooldown_secs=0.3,  # TODO: use cc
    strum_gate_time_secs=0.5,  # TODO: use cc
    chord_midi_map_style=NotePerChordMode(note_mapping={24: Chord.MAJOR, 25: Chord.MINOR, 26: Chord.DOM_7}),
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
