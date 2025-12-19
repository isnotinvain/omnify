#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

#include <json.hpp>
#include <optional>

// FLIP means x = !x, ON / OFF ignore prior state
enum class ButtonAction { FLIP, ON, OFF };

// Trigger an event based on a midi note or cc signal
class MidiButton {
   public:
    int note = -1;
    int cc = -1;

    // for cc pads that send a high signal for on and low signal for off
    // they are nice because the keyboard will often keep them highlighted while "on"
    bool ccIsToggle = false;

    MidiButton() = default;
    MidiButton(int note, int cc, bool ccIsToggle);

    static MidiButton fromNote(int noteNum);
    static MidiButton fromCC(int ccNum, bool toggle = false);

    std::optional<ButtonAction> handle(const juce::MidiMessage& msg) const;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(MidiButton, note, cc, ccIsToggle)
};