#include "PluginProcessor.h"

#include "Chords.h"

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout OmnifyAudioProcessor::createParameterLayout() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Create a StringArray of chord mode names for the choice parameter
    juce::StringArray chordModeNames;
    for (const auto& chord : CHORD_MODES) {
        chordModeNames.add(chord.name);
    }

    // Use a single AudioParameterChoice for mutually exclusive chord modes
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        ParamIDs::CHORD_MODE,  // param ID (stable string)
        "Chord Mode",          // name shown in host
        chordModeNames,        // list of choices
        0                      // default index (Major)
        ));

    // Add strum gate time parameter (in milliseconds)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::STRUM_GATE_TIME,                            // param ID
        "Strum Gate Time",                                    // name shown in host
        juce::NormalisableRange<float>(1.0f, 5000.0f, 1.0f),  // range: 1ms to 5000ms, step 1ms
        500.0f                                                // 500ms
        ));

    // Add strum cooldown parameter (in milliseconds)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::STRUM_COOLDOWN,                             // param ID
        "Strum Cooldown",                                     // name shown in host
        juce::NormalisableRange<float>(0.0f, 1000.0f, 1.0f),  // range: 0ms to 100ms, step 1ms
        300.0f                                                // default: 300ms
        ));

    return layout;
}

//==============================================================================
OmnifyAudioProcessor::OmnifyAudioProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "PARAMETERS", createParameterLayout()),
      omnify(&parameters) {}

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
    return 1;  // Only one program per plugin
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
    juce::ignoreUnused(samplesPerBlock);
    omnify.prepareToPlay(sampleRate);
}

void OmnifyAudioProcessor::releaseResources() {}

void OmnifyAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                        juce::MidiBuffer& midiMessages) {
    buffer.clear();
    // TODO is getNumSamples different from samplesPerBlock?
    omnify.process(midiMessages, buffer.getNumSamples());
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
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new OmnifyAudioProcessor(); }
