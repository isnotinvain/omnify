#include "Omnify.h"

#include <juce_core/juce_core.h>

#include <algorithm>
#include <unordered_set>

namespace {
constexpr int STRUM_ZONE_COUNT = 13;
constexpr int STRUM_DEAD_ZONE_SIZE = 2;

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
        case VoicingModifier::SMOOTH: {
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
        case VoicingModifier::DYNAMIC: {
            if (previousChordNotes.empty()) {
                chord = s.chordVoicingStyle->constructChord(currentChord->quality, currentChord->root);
                break;
            }

            auto voicingNotes = s.chordVoicingStyle->constructChord(currentChord->quality, 60 + (currentChord->root % 12));
            std::vector<int> pitchClasses;
            pitchClasses.reserve(voicingNotes.size());
            for (int note : voicingNotes) {
                pitchClasses.push_back(note % 12);
            }
            chord = dynamicSmooth(pitchClasses, previousChordNotes);
            break;
        }
    }

    previousChordNotes = chord;

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

    std::vector<juce::MidiMessage> events;
    events.reserve(noteOnEventsOfCurrentChord.size());
    for (const auto& noteOn : noteOnEventsOfCurrentChord) {
        events.push_back(juce::MidiMessage::noteOff(noteOn.getChannel(), noteOn.getNoteNumber()));
    }
    noteOnEventsOfCurrentChord.clear();
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

namespace {

int nearestInstanceOfPitchClass(int pitchClass, int target) {
    int offset = ((pitchClass - target) % 12 + 12) % 12;
    int above = target + offset;
    int below = above - 12;
    return (std::abs(target - below) <= std::abs(target - above)) ? below : above;
}

std::vector<int> placeNearCentroid(const std::vector<int>& pitchClasses, int centroid) {
    std::vector<int> placed;
    placed.reserve(pitchClasses.size());
    for (int pc : pitchClasses) {
        placed.push_back(nearestInstanceOfPitchClass(pc, centroid));
    }
    return placed;
}

int assignmentCost(const std::vector<int>& newNotes, const std::vector<int>& prevNotes, const std::vector<size_t>& assignment) {
    int cost = 0;
    for (size_t i = 0; i < assignment.size(); i++) {
        cost += std::abs(newNotes[i] - prevNotes[assignment[i]]);
    }
    return cost;
}

std::pair<std::vector<size_t>, int> bestAssignment(const std::vector<int>& newNotes, const std::vector<int>& prevNotes) {
    std::vector<size_t> perm(prevNotes.size());
    std::iota(perm.begin(), perm.end(), 0);

    std::vector<size_t> bestPerm = perm;
    int bestCost = INT_MAX;

    do {
        int cost = assignmentCost(newNotes, prevNotes, perm);
        if (cost < bestCost) {
            bestCost = cost;
            bestPerm = perm;
        }
    } while (std::next_permutation(perm.begin(), perm.end()));

    return {bestPerm, bestCost};
}

}  // namespace

std::vector<int> Omnify::dynamicSmooth(const std::vector<int>& newPitchClasses, const std::vector<int>& previousNotes) {
    jassert(!previousNotes.empty());

    int centroid = 0;
    for (int n : previousNotes) {
        centroid += n;
    }
    centroid /= static_cast<int>(previousNotes.size());

    size_t newSize = newPitchClasses.size();
    size_t prevSize = previousNotes.size();

    if (newSize == prevSize) {
        std::vector<int> newNotes = placeNearCentroid(newPitchClasses, centroid);
        auto [assignment, cost] = bestAssignment(newNotes, previousNotes);
        std::vector<int> result(newSize);
        for (size_t i = 0; i < newSize; i++) {
            int targetPrev = previousNotes[assignment[i]];
            result[i] = nearestInstanceOfPitchClass(newPitchClasses[i], targetPrev);
        }
        return result;

    } else if (newSize > prevSize) {
        // More new notes than previous (e.g., triad -> 7th chord)
        // Try each new note as "odd one out", scored by centroid distance
        int bestCost = INT_MAX;
        std::vector<int> bestResult;

        for (size_t oddOut = 0; oddOut < newSize; oddOut++) {
            std::vector<int> subset;
            for (size_t i = 0; i < newSize; i++) {
                if (i != oddOut) {
                    subset.push_back(newPitchClasses[i]);
                }
            }

            std::vector<int> subsetPlaced = placeNearCentroid(subset, centroid);
            auto [assignment, assignCost] = bestAssignment(subsetPlaced, previousNotes);

            int oddPlaced = nearestInstanceOfPitchClass(newPitchClasses[oddOut], centroid);
            int oddCost = std::abs(oddPlaced - centroid);
            int totalCost = assignCost + oddCost;

            if (totalCost < bestCost) {
                bestCost = totalCost;
                bestResult.clear();
                bestResult.reserve(newSize);
                size_t subIdx = 0;
                for (size_t i = 0; i < newSize; i++) {
                    if (i == oddOut) {
                        bestResult.push_back(oddPlaced);
                    } else {
                        int targetPrev = previousNotes[assignment[subIdx]];
                        bestResult.push_back(nearestInstanceOfPitchClass(subset[subIdx], targetPrev));
                        subIdx++;
                    }
                }
            }
        }
        return bestResult;

    } else {
        // Fewer new notes than previous (e.g., 7th chord -> triad)
        // Try each previous note as "orphan", pick lowest assignment cost
        int bestCost = INT_MAX;
        std::vector<int> bestResult;

        for (size_t orphan = 0; orphan < prevSize; orphan++) {
            std::vector<int> prevSubset;
            for (size_t i = 0; i < prevSize; i++) {
                if (i != orphan) {
                    prevSubset.push_back(previousNotes[i]);
                }
            }

            std::vector<int> newPlaced = placeNearCentroid(newPitchClasses, centroid);
            auto [assignment, assignCost] = bestAssignment(newPlaced, prevSubset);

            if (assignCost < bestCost) {
                bestCost = assignCost;
                bestResult.clear();
                bestResult.reserve(newSize);
                for (size_t i = 0; i < newSize; i++) {
                    int targetPrev = prevSubset[assignment[i]];
                    bestResult.push_back(nearestInstanceOfPitchClass(newPitchClasses[i], targetPrev));
                }
            }
        }
        return bestResult;
    }
}