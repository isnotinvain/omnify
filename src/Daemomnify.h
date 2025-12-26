#pragma once

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_core/juce_core.h>

#include <mutex>
#include <optional>

class MidiMessageScheduler;
class Omnify;

class Daemomnify : private juce::Thread {
   public:
    Daemomnify(Omnify& omnify, MidiMessageScheduler& scheduler, juce::String outputPortName);
    ~Daemomnify() override;

    Daemomnify(const Daemomnify&) = delete;
    Daemomnify& operator=(const Daemomnify&) = delete;

    void start();
    void stop();
    bool isRunning() const { return isThreadRunning(); }

    void setInputDevice(std::optional<juce::String> deviceId);

    // Called from message thread by PluginProcessor timer
    void checkDevices();

   private:
    void run() override;
    bool openMidiInput(const juce::String& deviceId);
    void closeMidiInput();
    bool openMidiOutput();
    void closeMidiOutput();

    Omnify& omnify;
    MidiMessageScheduler& scheduler;

    std::unique_ptr<juce::MidiInput> midiInput;
    std::unique_ptr<juce::MidiOutput> midiOutput;
    juce::MidiMessageCollector midiCollector;

    std::optional<juce::String> deviceId;
    mutable std::mutex deviceMutex;
    double lastInputOpenAttemptMs = 0;

    juce::String outputPortName;

    static constexpr int POLL_INTERVAL_MS = 1;
    static constexpr int RETRY_INTERVAL_MS = 500;
};
