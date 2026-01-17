#pragma once

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include <functional>
#include <memory>

#include "MidiMessageScheduler.h"
#include "Omnify.h"
#include "OmnifyLogger.h"
#include "ui/LcarsLookAndFeel.h"
#include "ui/components/MidiLearnComponent.h"

//==============================================================================
class OmnifyAudioProcessor : public juce::AudioProcessor,
                             private juce::AudioProcessorValueTreeState::Listener,
                             private juce::AsyncUpdater {
   public:
    OmnifyAudioProcessor();
    ~OmnifyAudioProcessor() override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    //==============================================================================
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    //==============================================================================
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    //==============================================================================
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    std::shared_ptr<OmnifySettings> getSettings() const { return std::atomic_load(&omnifySettings); }
    void modifySettings(std::function<void(OmnifySettings&)> mutator);

    juce::AudioProcessorValueTreeState& getAPVTS() { return parameters; }

    // Thread-safe getters for UI display (delegates to Omnify)
    ChordQuality getDisplayChordQuality() const { return omnify->getEnqueuedChordQuality(); }
    ChordNotes getDisplayChordNotes() const { return omnify->getChordNotes(); }
    int getDisplayCurrentRoot() const { return omnify->getCurrentRoot(); }  // -1 if no chord

   private:
    juce::ValueTree stateTree{"OmnifyState"};
    static constexpr const char* SETTINGS_JSON_KEY = "settings_v2";

    juce::AudioProcessorValueTreeState parameters;
    juce::AudioParameterFloat* strumGateTimeParam = nullptr;
    juce::AudioParameterFloat* strumCooldownParam = nullptr;

    void parameterChanged(const juce::String& parameterID, float newValue) override;
    void handleAsyncUpdate() override;
    void applySettingsFromJson(const juce::String& jsonString);
    void loadSettingsFromValueTree();
    void saveSettingsToValueTree();

    std::unique_ptr<MidiMessageScheduler> midiScheduler;
    std::shared_ptr<RealtimeParams> realtimeParams;
    std::shared_ptr<OmnifySettings> omnifySettings;
    std::unique_ptr<Omnify> omnify;

    std::unique_ptr<juce::MidiInput> midiInput;
    std::shared_ptr<juce::MidiOutput> midiOutput;
    juce::MidiMessageCollector inputCollector;
    double sampleRate = 44100.0;
    int64_t currentSamplePosition = 0;
    void reconcileDevices();

    juce::SharedResourcePointer<OmnifyLogger> logger;

    LcarsLookAndFeel lcarsLookAndFeel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OmnifyAudioProcessor)
};
