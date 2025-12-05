#include "PluginProcessor.h"

#include "PluginEditor.h"

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout OmnifyAudioProcessor::createParameterLayout() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Gain parameter (0.0 to 2.0, default 1.0) - for UI demo, does nothing
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::GAIN,
        "Gain",
        juce::NormalisableRange<float>(0.0f, 2.0f, 0.01f),
        1.0f));

    // Mix parameter (0.0 to 1.0, default 1.0) - for UI demo, does nothing
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::MIX,
        "Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        1.0f));

    // Bypass parameter (on/off, default off) - for UI demo, does nothing
    layout.add(std::make_unique<juce::AudioParameterBool>(
        ParamIDs::BYPASS,
        "Bypass",
        false));

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
