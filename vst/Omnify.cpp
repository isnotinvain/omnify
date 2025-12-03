
#include "Omnify.h"

#include "PluginProcessor.h"

Omnify::Omnify(juce::AudioProcessorValueTreeState* params) : parameters(params) {
    chordModeParam =
        dynamic_cast<juce::AudioParameterChoice*>(parameters->getParameter(ParamIDs::CHORD_MODE));
    jassert(chordModeParam != nullptr);

    strumGateTimeParam = dynamic_cast<juce::AudioParameterFloat*>(
        parameters->getParameter(ParamIDs::STRUM_GATE_TIME));
    jassert(strumGateTimeParam != nullptr);

    strumCooldownParam = dynamic_cast<juce::AudioParameterFloat*>(
        parameters->getParameter(ParamIDs::STRUM_COOLDOWN));
    jassert(strumCooldownParam != nullptr);
}

void Omnify::prepareToPlay(double sampleRate) {
    currentSampleRate = sampleRate;

    // Clear all state since sample rate may have changed
    outstandingNotes.clear();
    currentChord = std::nullopt;
    lastRootNote = -1;
    totalSamplesProcessed = 0;
    futureEvents = {};
    lastStrumZone = -1;
    lastStrumSampleTime = -1;
}

int Omnify::clampNote(int note) {
    if (note > 127) {
        note = (note % 12) + 108;  // Clamp to highest octave
    }
    return note;
}

void Omnify::clearOutstandingNotes(juce::MidiBuffer& midiBuffer, int time) {
    // loop through outstanding notes and convert them to note off messages
    for (const auto& noteOn : outstandingNotes) {
        juce::MidiMessage noteOff =
            juce::MidiMessage::noteOff(noteOn.getChannel(), noteOn.getNoteNumber());
        midiBuffer.addEvent(noteOff, time);
    }
    outstandingNotes.clear();
}

void Omnify::addNoteOn(juce::MidiBuffer& midiBuffer, const juce::MidiMessage& noteOn, int time) {
    outstandingNotes.push_back(noteOn);
    midiBuffer.addEvent(noteOn, time);
}

void Omnify::process(juce::MidiBuffer& midiMessages, int numSamples) {
    juce::MidiBuffer processedMidi;

    // first off, fire any scheduled events that fall in this window

    // Calculate the time range this MIDI buffer contains
    int64_t bufferStartSample = totalSamplesProcessed;
    int64_t bufferEndSample = totalSamplesProcessed + numSamples;

    // Process scheduled events that fall within this buffer's time window
    while (!futureEvents.empty()) {
        const auto& nextEvent = futureEvents.top();
        if (nextEvent.globalSampleTime < bufferEndSample) {
            // Calculate the relative position within this buffer
            int relativeTime = static_cast<int>(nextEvent.globalSampleTime - bufferStartSample);
            processedMidi.addEvent(nextEvent.message, relativeTime);
            futureEvents.pop();
        } else {
            break;  // No more events in this time window
        }
    }

    // Get the current chord mode from the choice parameter
    auto chordModeIndex = static_cast<size_t>(chordModeParam->getIndex());

    for (const auto metadata : midiMessages) {
        auto message = metadata.getMessage();
        const auto time = metadata.samplePosition;

        if (message.isNoteOn()) {
            clearOutstandingNotes(processedMidi, time);

            this->lastRootNote = message.getNoteNumber();
            this->currentChord = &CHORD_MODES.at(chordModeIndex);

            for (int offset : currentChord.value()->offsets) {
                auto newNote = message;  // copy!
                newNote.setNoteNumber(clampNote(this->lastRootNote + offset));
                addNoteOn(processedMidi, newNote, time);
            }
        } else if (message.isNoteOff()) {
            if (message.getNoteNumber() == this->lastRootNote) {
                clearOutstandingNotes(processedMidi, time);
                this->currentChord = std::nullopt;
            }
            processedMidi.addEvent(message, time);
        }

        if (message.isController() && message.getControllerNumber() == 1 && this->currentChord) {
            // We Strummin'

            // each zone is 13/128ths in size (plus some floor-ing from integer division)
            auto strumPlateZone = message.getControllerValue() * 13 / 128;

            // Calculate current time in global samples
            int64_t currentTime = totalSamplesProcessed + time;

            // Cooldown period: convert from parameter (ms) to samples
            float cooldownMs = strumCooldownParam->get();
            int64_t cooldownSamples =
                static_cast<int64_t>((cooldownMs / 1000.0) * currentSampleRate);

            // Determine if we should trigger a note:
            // 1. Zone changed (always trigger)
            // 2. Same zone but cooldown period has elapsed
            bool zoneChanged = (strumPlateZone != lastStrumZone);
            bool cooldownElapsed = (currentTime - lastStrumSampleTime) >= cooldownSamples;

            if (zoneChanged || cooldownElapsed) {
                int rootNoteClass = lastRootNote % 12;

                int noteToPlay = ARPS.at(this->currentChord.value()->chordType)
                                     .at(rootNoteClass)
                                     .at(static_cast<size_t>(strumPlateZone));

                // now, what if we're already playing that note as part of the held chord? We need
                // to pick something else!

                for (auto note : outstandingNotes) {
                    if (note.getNoteNumber() == noteToPlay) {
                        noteToPlay = noteToPlay + 12;
                        break;
                    }
                }

                auto onMessage = outstandingNotes.at(0);  // we want this for its velocity data
                onMessage.setNoteNumber(noteToPlay);
                processedMidi.addEvent(onMessage, time);

                // Schedule the note-off event in the future
                juce::MidiMessage noteOff =
                    juce::MidiMessage::noteOff(onMessage.getChannel(), onMessage.getNoteNumber());

                ScheduledMidiEvent scheduledNoteOff;
                scheduledNoteOff.message = noteOff;
                // Convert gate time from milliseconds to samples
                float gateTimeMs = strumGateTimeParam->get();
                int64_t gateTimeSamples =
                    static_cast<int64_t>((gateTimeMs / 1000.0) * currentSampleRate);
                scheduledNoteOff.globalSampleTime = totalSamplesProcessed + time + gateTimeSamples;
                futureEvents.push(scheduledNoteOff);

                // Update strum state
                lastStrumZone = strumPlateZone;
                lastStrumSampleTime = currentTime;
            }
        }
    }

    midiMessages.swapWith(processedMidi);
    totalSamplesProcessed += numSamples;
}
