#pragma once

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include <functional>
#include <memory>

#include "MidiMessageScheduler.h"
#include "MidiThread.h"
#include "Omnify.h"
#include "OmnifyLogger.h"
#include "datamodel/ChordQuality.h"
#include "datamodel/VoicingStyle.h"
#include "ui/components/MidiLearnComponent.h"

//==============================================================================
class OmnifyAudioProcessor : public juce::AudioProcessor,
                             private juce::AudioProcessorValueTreeState::Listener,
                             private juce::MidiInputCallback {
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
    bool producesMidi() const override { return false; }
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
    void setMidiInputDevice(const juce::String& deviceName);

    juce::AudioProcessorValueTreeState& getAPVTS() { return parameters; }
    juce::ValueTree& getStateTree() { return stateTree; }  // TODO: remove once UI uses callbacks

    const VoicingStyleRegistry<VoicingFor::Chord>& getChordVoicingRegistry() const { return chordVoicingRegistry; }
    const VoicingStyleRegistry<VoicingFor::Strum>& getStrumVoicingRegistry() const { return strumVoicingRegistry; }

   private:
    juce::ValueTree stateTree{"OmnifyState"};
    static constexpr const char* SETTINGS_JSON_KEY = "settings_v1";

    juce::AudioProcessorValueTreeState parameters;
    juce::AudioParameterFloat* strumGateTimeParam = nullptr;
    juce::AudioParameterFloat* strumCooldownParam = nullptr;

    VoicingStyleRegistry<VoicingFor::Chord> chordVoicingRegistry;
    VoicingStyleRegistry<VoicingFor::Strum> strumVoicingRegistry;
    void initVoicingRegistries();

    void parameterChanged(const juce::String& parameterID, float newValue) override;
    void applySettingsFromJson(const juce::String& jsonString);
    void loadSettingsFromValueTree();
    void saveSettingsToValueTree();
    void loadDefaultSettings();

    std::unique_ptr<MidiMessageScheduler> midiScheduler;
    std::shared_ptr<RealtimeParams> realtimeParams;
    std::shared_ptr<OmnifySettings> omnifySettings;
    std::unique_ptr<Omnify> omnify;
    std::unique_ptr<MidiThread> midiThread;

    // Direct MIDI input for MIDI Learn (bypasses DAW routing)
    std::unique_ptr<juce::MidiInput> midiLearnInput;
    void openMidiLearnInput(const juce::String& deviceName);
    void closeMidiLearnInput();
    void handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& message) override;

    juce::SharedResourcePointer<OmnifyLogger> logger;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OmnifyAudioProcessor)
};
