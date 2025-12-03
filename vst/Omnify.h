#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <optional>
#include <queue>

#include "Chords.h"

struct ScheduledMidiEvent {
    int64_t globalSampleTime;
    juce::MidiMessage message;

    bool operator>(const ScheduledMidiEvent& other) const {
        return globalSampleTime > other.globalSampleTime;  // Min-heap (earliest first)
    }
};

class Omnify {
   public:
    Omnify(juce::AudioProcessorValueTreeState* parameters);
    ~Omnify() = default;

    void prepareToPlay(double sampleRate);
    void process(juce::MidiBuffer& midiMessages, int numSamples);

    static int clampNote(int note);

    void clearOutstandingNotes(juce::MidiBuffer& midiBuffer, int time);

    void addNoteOn(juce::MidiBuffer& midiBuffer, const juce::MidiMessage& noteOn, int time);

   private:
    juce::AudioProcessorValueTreeState* parameters;
    juce::AudioParameterChoice* chordModeParam;
    juce::AudioParameterFloat* strumGateTimeParam;
    juce::AudioParameterFloat* strumCooldownParam;
    int lastRootNote = -1;
    std::vector<juce::MidiMessage> outstandingNotes;
    std::optional<const Chord*> currentChord = std::nullopt;

    std::priority_queue<ScheduledMidiEvent, std::vector<ScheduledMidiEvent>,
                        std::greater<ScheduledMidiEvent>>
        futureEvents;
    int64_t totalSamplesProcessed = 0;
    double currentSampleRate;

    // Strum detection state
    int lastStrumZone = -1;
    int64_t lastStrumSampleTime = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Omnify)
};
