#include "MidiButton.h"

MidiButton::MidiButton(int note, int cc, bool ccIsToggle) : note(note), cc(cc), ccIsToggle(ccIsToggle) {}

MidiButton MidiButton::fromNote(int noteNum) { return MidiButton{noteNum, -1, false}; }

MidiButton MidiButton::fromCC(int ccNum, bool toggle) { return MidiButton{-1, ccNum, toggle}; }

std::optional<ButtonAction> MidiButton::handle(const juce::MidiMessage& msg) const {
    // velocity == 0 means note off in some devices
    if (msg.isNoteOn() && msg.getNoteNumber() == note && msg.getVelocity() != 0) {
        return ButtonAction::FLIP;
    }

    if (msg.isController() && msg.getControllerNumber() == cc) {
        if (ccIsToggle) {
            return msg.getControllerValue() > 63 ? ButtonAction::ON : ButtonAction::OFF;
        } else {
            return ButtonAction::FLIP;
        }
    }

    return std::nullopt;
}
