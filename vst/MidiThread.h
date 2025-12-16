#pragma once

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_core/juce_core.h>

#include <atomic>
#include <mutex>
#include <optional>

class MidiMessageScheduler;
class Omnify;

class MidiThread : private juce::Thread {
   public:
    MidiThread(Omnify& omnify, MidiMessageScheduler& scheduler, const juce::String& outputPortName);
    ~MidiThread() override;

    MidiThread(const MidiThread&) = delete;
    MidiThread& operator=(const MidiThread&) = delete;

    void start();
    void stop();
    bool isRunning() const { return running.load(); }

    void setInputDevice(std::optional<juce::String> deviceId);

   private:
    void run() override;
    bool openMidiInput(const juce::String& deviceId);
    void closeMidiInput();
    bool openMidiOutput();
    void closeMidiOutput();

    Omnify& omnify;
    MidiMessageScheduler& scheduler;

    std::atomic<bool> running{false};

    std::unique_ptr<juce::MidiInput> midiInput;
    std::unique_ptr<juce::MidiOutput> midiOutput;
    juce::MidiMessageCollector midiCollector;

    std::optional<juce::String> deviceId;
    mutable std::mutex deviceMutex;

    juce::String outputPortName;

    static constexpr int POLL_INTERVAL_MS = 1;
};
