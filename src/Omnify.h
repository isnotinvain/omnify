#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

#include <atomic>
#include <memory>
#include <optional>
#include <vector>

#include "MidiMessageScheduler.h"
#include "datamodel/ChordQuality.h"
#include "datamodel/MidiButton.h"
#include "datamodel/OmnifySettings.h"

struct RealtimeParams {
    std::atomic<int> strumGateTimeMs{500};
    std::atomic<int> strumCooldownMs{300};
};

class Omnify {
   public:
    Omnify(MidiMessageScheduler& scheduler, std::shared_ptr<OmnifySettings> settings, std::shared_ptr<RealtimeParams> realtimeParams);

    void setSampleRate(double sr);
    std::vector<juce::MidiMessage> handle(const juce::MidiMessage& msg, int64_t currentSample);

    void updateSettings(std::shared_ptr<OmnifySettings> newSettings, bool includeRealtime = false);
    void syncRealtimeSettings();

   private:
    MidiMessageScheduler& scheduler;
    std::shared_ptr<OmnifySettings> settings;  // use std::atomic_load/store for thread safety
    std::shared_ptr<RealtimeParams> realtimeParams;
    double sampleRate = 44100.0;

    // State
    ChordQuality enqueuedChordQuality = ChordQuality::MAJOR;
    std::optional<Chord> currentChord;
    std::optional<Chord> lastPlayedChord;
    juce::uint8 lastVelocity = 100;
    std::vector<juce::MidiMessage> noteOnEventsOfCurrentChord;
    int64_t lastStrumSample = 0;
    std::optional<int> lastStrumZone;
    bool latch = false;

    std::optional<std::vector<juce::MidiMessage>> handleChordQualityChange(const juce::MidiMessage& msg, const OmnifySettings& s);
    std::optional<std::vector<juce::MidiMessage>> handleStopButton(const juce::MidiMessage& msg, const OmnifySettings& s);
    std::optional<std::vector<juce::MidiMessage>> handleLatchButton(const juce::MidiMessage& msg, const OmnifySettings& s);
    std::optional<std::vector<juce::MidiMessage>> handleChordNoteOn(const juce::MidiMessage& msg, const OmnifySettings& s);
    std::optional<std::vector<juce::MidiMessage>> handleChordNoteOff(const juce::MidiMessage& msg, const OmnifySettings& s);
    std::optional<std::vector<juce::MidiMessage>> handleStrum(const juce::MidiMessage& msg, const OmnifySettings& s, int64_t currentSample);

    std::vector<juce::MidiMessage> stopNotesOfCurrentChord();
    static int clampNote(int note);
    static std::vector<int> smooth(std::vector<int> offsets, int root);
};
