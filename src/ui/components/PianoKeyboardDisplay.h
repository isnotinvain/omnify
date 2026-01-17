#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>

class PianoKeyboardDisplay : public juce::Component {
   public:
    PianoKeyboardDisplay() = default;

    void paint(juce::Graphics& g) override;

    void setActiveNotes(const std::array<bool, 128>& notes);
    void setHighlightColour(juce::Colour colour) { highlightColour = colour; }
    void setWhiteKeyColour(juce::Colour colour) { whiteKeyColour = colour; }
    void setBlackKeyColour(juce::Colour colour) { blackKeyColour = colour; }
    void setKeyRange(int startNote, int endNote);

   private:
    static constexpr int DEFAULT_START_NOTE = 48;  // C3
    static constexpr int DEFAULT_END_NOTE = 84;    // C6

    int startNote = DEFAULT_START_NOTE;
    int endNote = DEFAULT_END_NOTE;
    std::array<bool, 128> activeNotes{};
    juce::Colour highlightColour{juce::Colours::orange};
    juce::Colour whiteKeyColour{juce::Colours::white};
    juce::Colour blackKeyColour{juce::Colours::black};

    static bool isBlackKey(int note);
    int countWhiteKeys() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PianoKeyboardDisplay)
};
