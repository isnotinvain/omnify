from enum import Enum
from pathlib import Path
from typing import Annotated, Literal

from pydantic import BaseModel, Field

from daemomnify import chord_quality, vst_params
from daemomnify.chord_voicings.file_style import FileStyle
from daemomnify.chord_voicings.omni84_style import Omni84Style
from daemomnify.chord_voicings.omnichord_strum_style import OmnichordStrumStyle
from daemomnify.chord_voicings.plain_ascending_strum_style import PlainAscendingStrumStyle
from daemomnify.chord_voicings.root_position_style import (
    RootPositionStyle,
)


class ButtonPerChordQuality(BaseModel):
    type: Literal["ButtonPerChordQuality"] = "ButtonPerChordQuality"
    notes: dict[int, chord_quality.ChordQuality]
    ccs: dict[int, chord_quality.ChordQuality]

    def handles_message(self, msg):
        if msg.type == "note_on" and msg.note in self.notes:
            return self.notes[msg.note]
        if msg.type == "control_change" and msg.control in self.ccs:
            return self.ccs[msg.control]
        return None


class CCRangePerChordQuality(BaseModel):
    type: Literal["CCRangePerChordQuality"] = "CCRangePerChordQuality"
    cc: int

    def handles_message(self, msg):
        if msg.type == "control_change" and msg.control == self.cc:
            chord_index = min(int(msg.value / chord_quality.ChordQuality.chord_region_size), len(chord_quality.ChordQuality.all) - 1)
            return chord_quality.ChordQuality.all[chord_index]
        return None


ChordQualitySelectionStyle = Annotated[
    ButtonPerChordQuality | CCRangePerChordQuality,
    Field(discriminator="type"),
]


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


MidiButton = Annotated[
    MidiNoteButton | MidiCCButton,
    Field(discriminator="type"),
]


ChordStyleConfig = Annotated[
    RootPositionStyle | FileStyle | Omni84Style,
    Field(discriminator="type"),
]
StrumStyleConfig = Annotated[
    PlainAscendingStrumStyle | OmnichordStrumStyle,
    Field(discriminator="type"),
]


class AdditionalSettings(BaseModel):
    """Complex settings that are serialized as a JSON blob."""
    chord_voicing_style: ChordStyleConfig
    strum_voicing_style: StrumStyleConfig
    chord_quality_selection_style: ChordQualitySelectionStyle
    latch_toggle_button: MidiButton
    stop_button: MidiButton


class DaemomnifySettings(BaseModel):
    # Scalar VST parameters
    midi_device_name: Annotated[str, vst_params.VSTString(label="MIDI Device")]
    chord_channel: Annotated[int, vst_params.VSTIntChoice(min=1, max=16, default=1, label="Chord Channel")]
    strum_channel: Annotated[int, vst_params.VSTIntChoice(min=1, max=16, default=2, label="Strum Channel")]
    strum_cooldown_secs: Annotated[float, vst_params.VSTFloat(min=0.0, max=5.0, label="Strum Cooldown (sec)")]
    strum_gate_time_secs: Annotated[float, vst_params.VSTFloat(min=0.0, max=5.0, label="Strum Gate Time (sec)")]
    strum_plate_cc: Annotated[int, vst_params.VSTInt(min=0, max=127, label="Strum Plate CC")]

    # JSON blob containing complex/nested settings
    additional_settings: Annotated[AdditionalSettings, vst_params.VSTJsonBlob()]

    # TODO: just make a set of these in the constructor
    def is_note_control_note(self, note: int) -> bool:
        match self.additional_settings.chord_quality_selection_style:
            case ButtonPerChordQuality() as m:
                if note in m.notes:
                    return True
            case _:
                pass
        match self.additional_settings.latch_toggle_button:
            case MidiNoteButton() as b:
                if b.note == note:
                    return True
            case _:
                pass
        match self.additional_settings.stop_button:
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
    strum_cooldown_secs=0.3,
    strum_gate_time_secs=0.5,
    strum_plate_cc=1,
    additional_settings=AdditionalSettings(
        chord_voicing_style=RootPositionStyle(),
        strum_voicing_style=PlainAscendingStrumStyle(),
        chord_quality_selection_style=ButtonPerChordQuality(
            notes={
                0: chord_quality.ChordQuality.MAJOR,
                1: chord_quality.ChordQuality.MINOR,
                2: chord_quality.ChordQuality.DOM_7,
            },
            ccs={},
        ),
        latch_toggle_button=MidiCCButton(cc=102, is_toggle=True),
        stop_button=MidiCCButton(cc=103, is_toggle=False),
    ),
)

if __name__ == "__main__":
    print("Writing default settings file...")
    save_settings(DEFAULT_SETTINGS)
    print("Done!")
    print("Reading...")
    print(load_settings())
