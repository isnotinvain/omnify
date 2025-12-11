#include "PluginProcessor.h"

#include "BinaryData.h"
#include "ui/GuiItems.h"
#include "ui/LcarsLookAndFeel.h"

//==============================================================================
OmnifyAudioProcessor::OmnifyAudioProcessor()
    : foleys::MagicProcessor(BusesProperties()
                                 .withInput("Input", juce::AudioChannelSet::stereo(), true)
                                 .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "PARAMETERS", GeneratedParams::createParameterLayout(params)),
      additionalSettings(GeneratedAdditionalSettings::Settings::defaults()) {
    FOLEYS_SET_SOURCE_PATH(__FILE__);

    // Update parameter map after APVTS is created so Foleys can see the parameters
    magicState.updateParameterMap();

    // Load additional settings from ValueTree if present
    loadAdditionalSettingsFromValueTree();

    // Load the GUI layout - from file in debug mode for hot reload, from binary data in release
#if JUCE_DEBUG
    auto xmlFile = juce::File(__FILE__).getParentDirectory().getChildFile("Resources/magic.xml");
    if (xmlFile.existsAsFile()) {
        magicState.setGuiValueTree(xmlFile);
    } else {
        magicState.setGuiValueTree(BinaryData::magic_xml, BinaryData::magic_xmlSize);
    }
#else
    magicState.setGuiValueTree(BinaryData::magic_xml, BinaryData::magic_xmlSize);
#endif
}

//==============================================================================
void OmnifyAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    juce::ignoreUnused(sampleRate, samplesPerBlock);
}

void OmnifyAudioProcessor::releaseResources() {}

void OmnifyAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                        juce::MidiBuffer& midiMessages) {
    juce::ignoreUnused(buffer);
    MidiLearnComponent::broadcastMidi(midiMessages);
}

//==============================================================================
void OmnifyAudioProcessor::initialiseBuilder(foleys::MagicGUIBuilder& builder) {
    builder.registerJUCEFactories();
    builder.registerJUCELookAndFeels();

    auto lcarsLookAndFeel = std::make_unique<LcarsLookAndFeel>();
    lcarsLookAndFeel->setBuilder(&builder);
    builder.registerLookAndFeel("Lcars", std::move(lcarsLookAndFeel));

    builder.registerFactory("OmnifyMidiLearn", &MidiLearnItem::factory);
    builder.registerFactory("ChordQualitySelector", &ChordQualitySelectorItem::factory);
    builder.registerFactory("VariantSelector", &VariantSelectorItem::factory);
    builder.registerFactory("LcarsSettings", &LcarsSettingsItem::factory);
}

//==============================================================================
void OmnifyAudioProcessor::setAdditionalSettings(const GeneratedAdditionalSettings::Settings& settings) {
    additionalSettings = settings;
    saveAdditionalSettingsToValueTree();
}

void OmnifyAudioProcessor::loadAdditionalSettingsFromValueTree() {
    auto jsonString = parameters.state.getProperty(ADDITIONAL_SETTINGS_KEY, "").toString();
    if (jsonString.isNotEmpty()) {
        try {
            additionalSettings = GeneratedAdditionalSettings::fromJson(jsonString.toStdString());
        } catch (...) {
            // If parsing fails, keep defaults
            additionalSettings = GeneratedAdditionalSettings::Settings::defaults();
        }
    }
}

void OmnifyAudioProcessor::saveAdditionalSettingsToValueTree() {
    auto jsonString = GeneratedAdditionalSettings::toJson(additionalSettings);
    parameters.state.setProperty(ADDITIONAL_SETTINGS_KEY, juce::String(jsonString), nullptr);
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new OmnifyAudioProcessor(); }