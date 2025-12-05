#include "PluginProcessor.h"

#include "PluginEditor.h"

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout OmnifyAudioProcessor::createParameterLayout() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Chord Voicing Style parameter
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        ParamIDs::CHORD_VOICING_STYLE,
        "Chord Voicing Style",
        ChordVoicingStyles::choices,
        0));

    // Strum Voicing Style parameter
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        ParamIDs::STRUM_VOICING_STYLE,
        "Strum Voicing Style",
        StrumVoicingStyles::choices,
        0));

    return layout;
}

//==============================================================================
OmnifyAudioProcessor::OmnifyAudioProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "PARAMETERS", createParameterLayout()) {}

OmnifyAudioProcessor::~OmnifyAudioProcessor() = default;

//==============================================================================
const juce::String OmnifyAudioProcessor::getName() const { return JucePlugin_Name; }

bool OmnifyAudioProcessor::acceptsMidi() const {
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool OmnifyAudioProcessor::producesMidi() const {
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool OmnifyAudioProcessor::isMidiEffect() const {
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double OmnifyAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int OmnifyAudioProcessor::getNumPrograms() {
    return 1;
}

int OmnifyAudioProcessor::getCurrentProgram() { return 0; }

void OmnifyAudioProcessor::setCurrentProgram(int index) { juce::ignoreUnused(index); }

const juce::String OmnifyAudioProcessor::getProgramName(int index) {
    juce::ignoreUnused(index);
    return {};
}

void OmnifyAudioProcessor::changeProgramName(int index, const juce::String& newName) {
    juce::ignoreUnused(index, newName);
}

//==============================================================================
void OmnifyAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    juce::ignoreUnused(sampleRate, samplesPerBlock);
}

void OmnifyAudioProcessor::releaseResources() {}

void OmnifyAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                        juce::MidiBuffer& midiMessages) {
    juce::ignoreUnused(buffer, midiMessages);
    // Pass-through: do nothing to the audio
}

//==============================================================================
juce::AudioProcessorEditor* OmnifyAudioProcessor::createEditor() {
    return new OmnifyAudioProcessorEditor(*this);
}

//==============================================================================
void OmnifyAudioProcessor::getStateInformation(juce::MemoryBlock& destData) {
    auto state = parameters.copyState();
    if (auto xmlState = state.createXml()) {
        copyXmlToBinary(*xmlState, destData);
    }
}

void OmnifyAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
    if (auto xmlState = getXmlFromBinary(data, sizeInBytes)) {
        if (xmlState->hasTagName(parameters.state.getType())) {
            parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
        }
    }
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new OmnifyAudioProcessor(); }
