#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>

#include <queue>
#include <vector>

struct ScheduledMidiMessage {
    double sendTimeMs;
    juce::MidiMessage message;

    // Comparison for priority queue (min-heap: earliest time first)
    bool operator>(const ScheduledMidiMessage& other) const { return sendTimeMs > other.sendTimeMs; }
};

/*
 * Schedules MIDI messages for delayed delivery.
 *
 * All times are in milliseconds.
 * Use juce::Time::getMillisecondCounterHiRes() for currentTimeMs.
 */
class MidiMessageScheduler {
   public:
    MidiMessageScheduler() = default;

    void schedule(const juce::MidiMessage& msg, double currentTimeMs, double delayMs);

    void sendOverdueMessages(double currentTimeMs, juce::MidiOutput& output);

    void clear();

    bool isEmpty() const { return queue.empty(); }

    size_t size() const { return queue.size(); }

   private:
    std::priority_queue<ScheduledMidiMessage, std::vector<ScheduledMidiMessage>, std::greater<ScheduledMidiMessage>> queue;
};
