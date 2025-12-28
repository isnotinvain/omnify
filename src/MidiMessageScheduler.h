#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>

#include <cstdint>
#include <queue>
#include <vector>

struct ScheduledMidiMessage {
    int64_t sendAtSample;
    juce::MidiMessage message;

    bool operator>(const ScheduledMidiMessage& other) const { return sendAtSample > other.sendAtSample; }
};

class MidiMessageScheduler {
   public:
    MidiMessageScheduler() = default;

    void setSampleRate(double sampleRate);

    void schedule(const juce::MidiMessage& msg, int64_t currentSample, double delayMs);

    void collectOverdueMessages(int64_t blockStartSample, int64_t blockEndSample, juce::MidiBuffer& buffer);

    void clear();

    bool isEmpty() const { return queue.empty(); }

    size_t size() const { return queue.size(); }

   private:
    double sampleRate = 44100.0;
    std::priority_queue<ScheduledMidiMessage, std::vector<ScheduledMidiMessage>, std::greater<ScheduledMidiMessage>> queue;
};
