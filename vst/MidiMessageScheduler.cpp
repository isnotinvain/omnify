#include "MidiMessageScheduler.h"

void MidiMessageScheduler::schedule(const juce::MidiMessage& msg, double currentTimeMs, double delayMs) {
    queue.push(ScheduledMidiMessage{currentTimeMs + delayMs, msg});
}

void MidiMessageScheduler::sendOverdueMessages(double currentTimeMs, juce::MidiOutput& output) {
    while (!queue.empty() && queue.top().sendTimeMs <= currentTimeMs) {
        output.sendMessageNow(queue.top().message);
        queue.pop();
    }
}

void MidiMessageScheduler::clear() { queue = {}; }
