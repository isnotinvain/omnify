#include "MidiMessageScheduler.h"

void MidiMessageScheduler::setSampleRate(double sr) { sampleRate = sr; }

void MidiMessageScheduler::schedule(const juce::MidiMessage& msg, int64_t currentSample, double delayMs) {
    int64_t delaySamples = static_cast<int64_t>((delayMs / 1000.0) * sampleRate);
    queue.push(ScheduledMidiMessage{.sendAtSample = currentSample + delaySamples, .message = msg});
}

void MidiMessageScheduler::collectOverdueMessages(int64_t blockStartSample, int64_t blockEndSample, juce::MidiBuffer& buffer) {
    while (!queue.empty() && queue.top().sendAtSample <= blockEndSample) {
        int samplePosition = static_cast<int>(queue.top().sendAtSample - blockStartSample);
        if (samplePosition < 0) samplePosition = 0;
        buffer.addEvent(queue.top().message, samplePosition);
        queue.pop();
    }
}

void MidiMessageScheduler::clear() { queue = {}; }
