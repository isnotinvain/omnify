#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_events/juce_events.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <atomic>
#include <functional>

enum class MidiLearnedType { None, Note, CC };

struct MidiLearnedValue {
    MidiLearnedType type = MidiLearnedType::None;
    int value = -1;  // Note number or CC number
};

class MidiLearnComponent : public juce::Component, private juce::AsyncUpdater {
   public:
    MidiLearnComponent();
    ~MidiLearnComponent() override;

    static void broadcastMidi(const juce::MidiBuffer& buffer);

    void setCaption(const juce::String& caption);
    void setLearnedValue(MidiLearnedValue val);
    MidiLearnedValue getLearnedValue() const;

    void processNextMidiBuffer(const juce::MidiBuffer& buffer);

    std::function<void(MidiLearnedValue)> onValueChanged;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;
    bool keyPressed(const juce::KeyPress& key) override;

   private:
    void handleAsyncUpdate() override;
    static juce::String noteNumberToName(int noteNumber);
    juce::String getDisplayText() const;
    void startLearning();
    void stopLearning();

    static inline std::atomic<MidiLearnComponent*> currentlyLearning{nullptr};

    juce::String caption{"Learn"};
    std::atomic<MidiLearnedType> learnedType{MidiLearnedType::None};
    std::atomic<int> learnedValue{-1};
    std::atomic<bool> isLearning{false};

    juce::Rectangle<int> boxBounds;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiLearnComponent)
};