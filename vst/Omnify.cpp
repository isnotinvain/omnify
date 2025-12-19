#include "Omnify.h"

#include <juce_core/juce_core.h>

#include <algorithm>
#include <unordered_set>

Omnify::Omnify(MidiMessageScheduler& scheduler, std::shared_ptr<OmnifySettings> settings, std::shared_ptr<RealtimeParams> realtimeParams)
    : scheduler(scheduler), realtimeParams(std::move(realtimeParams)) {
    updateSettings(std::move(settings), true);
}

void Omnify::updateSettings(std::shared_ptr<OmnifySettings> newSettings, bool includeRealtime) {
    if (includeRealtime) {
        realtimeParams->strumGateTimeMs.store(newSettings->strumGateTimeMs);
        realtimeParams->strumCooldownMs.store(newSettings->strumCooldownMs);
    }
    std::atomic_store(&settings, std::move(newSettings));
}

void Omnify::syncRealtimeSettings() {
    auto s = std::atomic_load(&settings);
    s->strumGateTimeMs = realtimeParams->strumGateTimeMs.load();
    s->strumCooldownMs = realtimeParams->strumCooldownMs.load();
}

std::vector<juce::MidiMessage> Omnify::handle(const juce::MidiMessage& msg) {
    auto s = std::atomic_load(&settings);
    if (auto r = handleChordQualityChange(msg, *s)) {
        return *r;
    }
    if (auto r = handleStopButton(msg, *s)) {
        return *r;
    }
    if (auto r = handleLatchButton(msg, *s)) {
        return *r;
    }
    if (auto r = handleChordNoteOn(msg, *s)) {
        return *r;
    }
    if (auto r = handleChordNoteOff(msg, *s)) {
        return *r;
    }
    if (auto r = handleStrum(msg, *s)) {
        return *r;
    }
    return {};
}

std::optional<std::vector<juce::MidiMessage>> Omnify::handleChordQualityChange(const juce::MidiMessage& msg, const OmnifySettings& s) {
    std::optional<ChordQuality> quality;

    std::visit(
        [&](auto&& style) {
            using T = std::decay_t<decltype(style)>;
            if constexpr (std::is_same_v<T, ButtonPerChordQuality>) {
                if (msg.isNoteOn() && msg.getVelocity() > 0) {
                    auto it = style.notes.find(msg.getNoteNumber());
                    if (it != style.notes.end()) {
                        quality = it->second;
                    }
                } else if (msg.isController()) {
                    auto it = style.ccs.find(msg.getControllerNumber());
                    if (it != style.ccs.end() && msg.getControllerValue() > 63) {
                        quality = it->second;
                    }
                }
            } else if constexpr (std::is_same_v<T, CCRangePerChordQuality>) {
                if (msg.isController() && msg.getControllerNumber() == style.cc) {
                    int idx = (msg.getControllerValue() * 9) / 128;
                    quality = ALL_CHORD_QUALITIES[static_cast<size_t>(idx)];
                }
            }
        },
        s.chordQualitySelectionStyle.value);

    if (quality) {
        enqueuedChordQuality = *quality;
        return std::vector<juce::MidiMessage>{};
    }
    return std::nullopt;
}

std::optional<std::vector<juce::MidiMessage>> Omnify::handleStopButton(const juce::MidiMessage& msg, const OmnifySettings& s) {
    if (s.stopButton.handle(msg)) {
        return stopNotesOfCurrentChord();
    }
    return std::nullopt;
}

std::optional<std::vector<juce::MidiMessage>> Omnify::handleLatchButton(const juce::MidiMessage& msg, const OmnifySettings& s) {
    auto action = s.latchButton.handle(msg);
    if (!action) {
        return std::nullopt;
    }

    switch (*action) {
        case ButtonAction::ON:
            latch = true;
            break;
        case ButtonAction::OFF:
            latch = false;
            break;
        case ButtonAction::FLIP:
            latch = !latch;
            break;
    }

    if (!latch) {
        return stopNotesOfCurrentChord();
    }
    return std::vector<juce::MidiMessage>{};
}

std::optional<std::vector<juce::MidiMessage>> Omnify::handleChordNoteOn(const juce::MidiMessage& msg, const OmnifySettings& s) {
    if (!msg.isNoteOn() || msg.getVelocity() == 0) {
        return std::nullopt;
    }

    auto events = stopNotesOfCurrentChord();

    currentChord = Chord{enqueuedChordQuality, msg.getNoteNumber()};

    std::unordered_set<int> clampedNotes;
    auto chord = s.chordVoicingStyle->constructChord(currentChord->quality, currentChord->root);

    for (int note : chord) {
        int clamped = clampNote(note);
        if (clampedNotes.contains(clamped)) {
            continue;
        }
        clampedNotes.insert(clamped);

        auto on = juce::MidiMessage::noteOn(s.chordChannel, clamped, msg.getVelocity());
        events.push_back(on);
        noteOnEventsOfCurrentChord.push_back(on);
    }

    return events;
}

std::optional<std::vector<juce::MidiMessage>> Omnify::handleChordNoteOff(const juce::MidiMessage& msg, const OmnifySettings& s) {
    bool isNoteOff = msg.isNoteOff() || (msg.isNoteOn() && msg.getVelocity() == 0);
    if (!isNoteOff) {
        return std::nullopt;
    }

    if (currentChord && currentChord->root == msg.getNoteNumber() && !latch) {
        return stopNotesOfCurrentChord();
    }

    return std::nullopt;
}

std::optional<std::vector<juce::MidiMessage>> Omnify::handleStrum(const juce::MidiMessage& msg, const OmnifySettings& s) {
    if (!(msg.isController() && msg.getControllerNumber() == s.strumPlateCC)) {
        return std::nullopt;
    }

    if (!currentChord) {
        // TODO: Should strums be allowed if no chord is playing?
        // TODO: (use previous chord?)
        return std::vector<juce::MidiMessage>{};
    }

    double now = juce::Time::getMillisecondCounterHiRes();
    bool cooldownReady = now >= lastStrumTimeMs + realtimeParams->strumCooldownMs.load();

    int strumPlateZone = (msg.getControllerValue() * 13) / 128;

    if (lastStrumZone != strumPlateZone || cooldownReady) {
        auto velocity = noteOnEventsOfCurrentChord[0].getVelocity();

        auto strumChord = s.strumVoicingStyle->constructChord(currentChord->quality, currentChord->root);
        int noteToPlay = strumChord[static_cast<size_t>(strumPlateZone)];

        auto noteOn = juce::MidiMessage::noteOn(s.strumChannel, noteToPlay, velocity);

        scheduler.schedule(juce::MidiMessage::noteOff(s.strumChannel, noteToPlay), now, static_cast<double>(realtimeParams->strumGateTimeMs.load()));

        lastStrumTimeMs = now;
        lastStrumZone = strumPlateZone;

        return std::vector<juce::MidiMessage>{noteOn};
    }

    return std::vector<juce::MidiMessage>{};
}

std::vector<juce::MidiMessage> Omnify::stopNotesOfCurrentChord() {
    currentChord = std::nullopt;

    std::vector<juce::MidiMessage> events;
    events.reserve(noteOnEventsOfCurrentChord.size());
    for (const auto& noteOn : noteOnEventsOfCurrentChord) {
        events.push_back(juce::MidiMessage::noteOff(noteOn.getChannel(), noteOn.getNoteNumber()));
    }
    noteOnEventsOfCurrentChord.clear();
    return events;
}

int Omnify::clampNote(int note) { return std::clamp(note, 0, 127); }
