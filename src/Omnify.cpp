#include "Omnify.h"

#include <juce_core/juce_core.h>

#include <algorithm>
#include <unordered_set>

namespace {
constexpr int STRUM_ZONE_COUNT = 13;
constexpr int STRUM_DEAD_ZONE_SIZE = 2;

// NOTE / IDEA:
// use the inversions for up/down, but use root move direction as signal for up/down, eg C3 -> G3 means up, C3 -> G2 means down -- even for cases
// where in the octave that doesn't happen

// Returns zone index (0-12), or -1 if in a dead zone
// Layout: [zone0][dead][zone1][dead]...[zone11][dead][zone12] (no dead after last zone)
int getStrumZone(int ccValue) {
    constexpr int deadZonesCount = STRUM_ZONE_COUNT - 1;
    constexpr int zoneSize = (128 - deadZonesCount * STRUM_DEAD_ZONE_SIZE) / STRUM_ZONE_COUNT;
    constexpr int unitSize = zoneSize + STRUM_DEAD_ZONE_SIZE;
    constexpr int lastZoneStart = (STRUM_ZONE_COUNT - 1) * unitSize;

    // Last zone has no dead zone after it
    if (ccValue >= lastZoneStart) {
        return STRUM_ZONE_COUNT - 1;
    }

    int zone = ccValue / unitSize;
    int positionInUnit = ccValue % unitSize;
    if (positionInUnit < zoneSize) {
        return zone;
    }

    return -1;  // dead zone
}
}  // namespace

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

void Omnify::setSampleRate(double sr) { sampleRate = sr; }

std::vector<juce::MidiMessage> Omnify::handle(const juce::MidiMessage& msg, int64_t currentSample) {
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
    if (auto r = handleStrum(msg, *s, currentSample)) {
        return *r;
    }
    return {msg};
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
        enqueuedChordQuality.store(*quality, std::memory_order_relaxed);
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

    auto quality = enqueuedChordQuality.load(std::memory_order_relaxed);
    currentChord = Chord{quality, msg.getNoteNumber()};
    currentRoot.store(msg.getNoteNumber(), std::memory_order_relaxed);
    lastPlayedChord = currentChord;
    lastVelocity = msg.getVelocity();

    std::unordered_set<int> clampedNotes;

    std::vector<int> chord;

    switch (s.voicingModifier) {
        case VoicingModifier::NONE:
            chord = s.chordVoicingStyle->constructChord(currentChord->quality, currentChord->root);
            break;
        case VoicingModifier::FIXED:
            chord = s.chordVoicingStyle->constructChord(currentChord->quality, 60 + (currentChord->root % 12));
            break;
        case VoicingModifier::SMOOTH:
            auto normalizedRoot = 60 + (currentChord->root % 12);
            auto middleOctaveNotes = s.chordVoicingStyle->constructChord(currentChord->quality, normalizedRoot);
            std::vector<int> offsets;
            offsets.reserve(middleOctaveNotes.size());
            for (int x : middleOctaveNotes) {
                offsets.push_back(x - normalizedRoot);
            }
            chord = smooth(offsets, currentChord->root);
            break;
    }

    ChordNotes newChordNotes;
    for (int note : chord) {
        int clamped = clampNote(note);
        if (clampedNotes.contains(clamped)) {
            continue;
        }
        clampedNotes.insert(clamped);

        events.push_back(juce::MidiMessage::noteOn(s.chordChannel, clamped, msg.getVelocity()));

        if (newChordNotes.count < ChordNotes::MAX_NOTES) {
            newChordNotes.notes[newChordNotes.count].note = static_cast<int8_t>(clamped);
            newChordNotes.notes[newChordNotes.count].channel = static_cast<int8_t>(s.chordChannel);
            newChordNotes.count++;
        }
    }
    chordNotes.store(newChordNotes, std::memory_order_relaxed);

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

std::optional<std::vector<juce::MidiMessage>> Omnify::handleStrum(const juce::MidiMessage& msg, const OmnifySettings& s, int64_t currentSample) {
    if (!(msg.isController() && msg.getControllerNumber() == s.strumPlateCC)) {
        return std::nullopt;
    }

    const Chord* chordToStrum = nullptr;
    if (currentChord) {
        chordToStrum = &*currentChord;
    } else if (lastPlayedChord) {
        chordToStrum = &*lastPlayedChord;
    } else {
        return std::vector<juce::MidiMessage>{};
    }

    auto cooldownSamples = static_cast<int64_t>((realtimeParams->strumCooldownMs.load() / 1000.0) * sampleRate);
    bool cooldownReady = currentSample >= lastStrumSample + cooldownSamples;

    int strumPlateZone = getStrumZone(msg.getControllerValue());
    if (strumPlateZone < 0) {
        return std::vector<juce::MidiMessage>{};  // in dead zone
    }

    if (lastStrumZone != strumPlateZone || cooldownReady) {
        auto rootToUse = (chordToStrum->root % 12) + 60;
        auto strumChord = s.strumVoicingStyle->constructChord(chordToStrum->quality, rootToUse);
        int noteToPlay = strumChord[static_cast<size_t>(strumPlateZone)];

        auto noteOn = juce::MidiMessage::noteOn(s.strumChannel, noteToPlay, lastVelocity);

        scheduler.schedule(juce::MidiMessage::noteOff(s.strumChannel, noteToPlay), currentSample,
                           static_cast<double>(realtimeParams->strumGateTimeMs.load()));

        lastStrumSample = currentSample;
        lastStrumZone = strumPlateZone;

        return std::vector<juce::MidiMessage>{noteOn};
    }

    return std::vector<juce::MidiMessage>{};
}

std::vector<juce::MidiMessage> Omnify::stopNotesOfCurrentChord() {
    currentChord = std::nullopt;
    currentRoot.store(-1, std::memory_order_relaxed);

    auto notes = chordNotes.load(std::memory_order_relaxed);
    std::vector<juce::MidiMessage> events;
    events.reserve(notes.count);
    for (uint8_t i = 0; i < notes.count; i++) {
        events.push_back(juce::MidiMessage::noteOff(notes.notes[i].channel, notes.notes[i].note));
    }
    chordNotes.store(ChordNotes{}, std::memory_order_relaxed);
    return events;
}

int Omnify::clampNote(int note) { return std::clamp(note, 0, 127); }

std::vector<int> Omnify::smooth(std::vector<int> offsets, int root) {
    std::sort(offsets.begin(), offsets.end());
    auto octave = root / 12;
    std::vector<int> notes;
    notes.reserve(offsets.size());

    std::vector<int> inversionOffsets(offsets.size(), 0);

    switch (octave) {
        case 2:
            inversionOffsets[inversionOffsets.size() - 3] = -12;
            inversionOffsets[inversionOffsets.size() - 2] = -12;
            inversionOffsets[inversionOffsets.size() - 1] = -12;
            break;
        case 3:
            inversionOffsets[inversionOffsets.size() - 2] = -12;
            inversionOffsets[inversionOffsets.size() - 1] = -12;
            break;
        case 4:
            inversionOffsets[inversionOffsets.size() - 1] = -12;
            break;
        // case 5: middle octave, do nothing
        case 6:
            inversionOffsets[0] = 12;
            break;
        case 7:
            inversionOffsets[0] = 12;
            inversionOffsets[1] = 12;
            break;
        case 8:
            inversionOffsets[0] = 12;
            inversionOffsets[1] = 12;
            inversionOffsets[2] = 12;
        default:
            break;
    }

    for (size_t i = 0; i < offsets.size(); i++) {
        notes.push_back(60 + (root % 12) + offsets[i] + inversionOffsets[i]);
    }

    return notes;
}