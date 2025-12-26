#include "Daemomnify.h"

#include <juce_core/juce_core.h>

#include <climits>
#include <utility>

#include "MidiMessageScheduler.h"
#include "Omnify.h"

Daemomnify::Daemomnify(Omnify& omnify, MidiMessageScheduler& scheduler, juce::String outputPortName)
    : juce::Thread("Daemomnify"), omnify(omnify), scheduler(scheduler), outputPortName(std::move(outputPortName)) {}

Daemomnify::~Daemomnify() { stop(); }

void Daemomnify::start() {
    if (isThreadRunning()) {
        return;
    }
    startThread();
}

void Daemomnify::stop() {
    stopThread(1000);
    closeMidiInput();
    closeMidiOutput();
}

void Daemomnify::setInputDevice(std::optional<juce::String> newDeviceId) {
    std::scoped_lock lock(deviceMutex);
    deviceId = std::move(newDeviceId);
}

void Daemomnify::run() {
    while (!threadShouldExit()) {
        // Process incoming MIDI messages
        {
            std::scoped_lock lock(deviceMutex);
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
                        DBG("Daemomnify: exception in handle(): " << e.what());
                    }
                }
            }

            // Send any scheduled messages whose time has arrived
            if (midiOutput) {
                double now = juce::Time::getMillisecondCounterHiRes();
                scheduler.sendOverdueMessages(now, *midiOutput);
            }
        }

        wait(POLL_INTERVAL_MS);
    }
}

void Daemomnify::checkDevices() {
    // Ensure output port is open
    if (!midiOutput) {
        openMidiOutput();
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
}

bool Daemomnify::openMidiInput(const juce::String& inputDeviceId) {
    double now = juce::Time::getMillisecondCounterHiRes();
    if (now < lastInputOpenAttemptMs + RETRY_INTERVAL_MS) {
        return false;
    }
    lastInputOpenAttemptMs = now;

    std::scoped_lock lock(deviceMutex);
    midiInput = juce::MidiInput::openDevice(inputDeviceId, &midiCollector);
    if (midiInput) {
        midiCollector.reset(44100.0);
        midiInput->start();
        return true;
    }
    return false;
}

void Daemomnify::closeMidiInput() {
    std::scoped_lock lock(deviceMutex);
    if (midiInput) {
        midiInput->stop();
        midiInput.reset();
    }
}

bool Daemomnify::openMidiOutput() {
    std::scoped_lock lock(deviceMutex);
    midiOutput = juce::MidiOutput::createNewDevice(outputPortName);
    return midiOutput != nullptr;
}

void Daemomnify::closeMidiOutput() {
    std::scoped_lock lock(deviceMutex);
    midiOutput.reset();
}
