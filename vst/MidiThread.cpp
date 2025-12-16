#include "MidiThread.h"

#include <juce_core/juce_core.h>

#include <climits>
#include <utility>

#include "MidiMessageScheduler.h"
#include "Omnify.h"

MidiThread::MidiThread(Omnify& omnify, MidiMessageScheduler& scheduler, juce::String outputPortName)
    : juce::Thread("MidiThread"), omnify(omnify), scheduler(scheduler), outputPortName(std::move(outputPortName)) {}

MidiThread::~MidiThread() { stop(); }

void MidiThread::start() {
    if (isThreadRunning()) {
        return;
    }
    startThread();
}

void MidiThread::stop() {
    stopThread(1000);
    closeMidiInput();
    closeMidiOutput();
}

void MidiThread::setInputDevice(std::optional<juce::String> newDeviceId) {
    std::scoped_lock lock(deviceMutex);
    deviceId = std::move(newDeviceId);
}

void MidiThread::run() {
    while (!threadShouldExit()) {
        // Ensure output port is open
        if (!midiOutput) {
            if (!openMidiOutput()) {
                wait(RETRY_INTERVAL_MS);
                continue;
            }
        }

        // Check if input device needs to change
        std::optional<juce::String> desiredDeviceId;
        {
            std::scoped_lock lock(deviceMutex);
            desiredDeviceId = deviceId;
        }

        std::optional<juce::String> currentDeviceId = midiInput ? std::make_optional(midiInput->getIdentifier()) : std::nullopt;

        if (desiredDeviceId != currentDeviceId) {
            closeMidiInput();
            if (desiredDeviceId) {
                openMidiInput(*desiredDeviceId);
            }
        }

        // Process incoming MIDI messages
        if (midiInput && midiOutput) {
            juce::MidiBuffer buffer;
            midiCollector.removeNextBlockOfMessages(buffer, INT_MAX);

            for (const auto metadata : buffer) {
                try {
                    auto toSend = omnify.handle(metadata.getMessage());
                    for (const auto& m : toSend) {
                        midiOutput->sendMessageNow(m);
                    }
                } catch (const std::exception& e) {
                    DBG("MidiThread: exception in handle(): " << e.what());
                }
            }
        }

        // Send any scheduled messages whose time has arrived
        if (midiOutput) {
            double now = juce::Time::getMillisecondCounterHiRes();
            scheduler.sendOverdueMessages(now, *midiOutput);
        }

        wait(POLL_INTERVAL_MS);
    }
}

bool MidiThread::openMidiInput(const juce::String& inputDeviceId) {
    double now = juce::Time::getMillisecondCounterHiRes();
    if (now < lastInputOpenAttemptMs + RETRY_INTERVAL_MS) {
        return false;
    }
    lastInputOpenAttemptMs = now;

    auto devices = juce::MidiInput::getAvailableDevices();
    for (const auto& device : devices) {
        if (device.identifier == inputDeviceId) {
            midiInput = juce::MidiInput::openDevice(device.identifier, &midiCollector);
            if (midiInput) {
                midiCollector.reset(44100.0);
                midiInput->start();
                return true;
            }
        }
    }
    return false;
}

void MidiThread::closeMidiInput() {
    if (midiInput) {
        midiInput->stop();
        midiInput.reset();
    }
}

bool MidiThread::openMidiOutput() {
    midiOutput = juce::MidiOutput::createNewDevice(outputPortName);
    return midiOutput != nullptr;
}

void MidiThread::closeMidiOutput() { midiOutput.reset(); }
