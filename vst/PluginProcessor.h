#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

//==============================================================================
namespace ParamIDs {
constexpr const char* CHORD_VOICING_STYLE = "chordVoicingStyle";
constexpr const char* STRUM_VOICING_STYLE = "strumVoicingStyle";
}

namespace ChordVoicingStyles {
const juce::StringArray choices = {"RootPositionStyle", "Omni84Style", "FileStyle"};
}

namespace StrumVoicingStyles {
const juce::StringArray choices = {"OmnichordStrumStyle", "PlainAscendingStrumStyle"};
}

//==============================================================================
class OmnifyAudioProcessor : public juce::AudioProcessor {
   public:
    //==============================================================================
    OmnifyAudioProcessor();
    ~OmnifyAudioProcessor() override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    void processBlock(juce::AudioBuffer<float>& buffer,
                      juce::MidiBuffer& midiMessages) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    //==============================================================================
    juce::AudioProcessorValueTreeState& getParameters() { return parameters; }

   private:
    //==============================================================================
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::AudioProcessorValueTreeState parameters;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OmnifyAudioProcessor)
};
