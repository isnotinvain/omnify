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

    setupValueListeners();
}

OmnifyAudioProcessor::~OmnifyAudioProcessor() {
    chordVoicingStyleValue.removeListener(this);
    strumVoicingStyleValue.removeListener(this);
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
void OmnifyAudioProcessor::postSetStateInformation() {
    loadAdditionalSettingsFromValueTree();
    pushVariantIndexesToValueTree();
}

//==============================================================================
void OmnifyAudioProcessor::setupValueListeners() {
    chordVoicingStyleValue.referTo(magicState.getValueTree().getPropertyAsValue("variant_chord_voicing_style", nullptr));
    strumVoicingStyleValue.referTo(magicState.getValueTree().getPropertyAsValue("variant_strum_voicing_style", nullptr));

    chordVoicingStyleValue.addListener(this);
    strumVoicingStyleValue.addListener(this);
}

void OmnifyAudioProcessor::valueChanged(juce::Value& value) {
    if (value.refersToSameSourceAs(chordVoicingStyleValue)) {
        int index = static_cast<int>(chordVoicingStyleValue.getValue());
        switch (index) {
            case 0:
                additionalSettings.chord_voicing_style = GeneratedAdditionalSettings::RootPositionStyle{};
                break;
            case 1:
                additionalSettings.chord_voicing_style = GeneratedAdditionalSettings::FileStyle{};
                break;
            case 2:
                additionalSettings.chord_voicing_style = GeneratedAdditionalSettings::Omni84Style{};
                break;
        }
        saveAdditionalSettingsToValueTree();
    } else if (value.refersToSameSourceAs(strumVoicingStyleValue)) {
        int index = static_cast<int>(strumVoicingStyleValue.getValue());
        switch (index) {
            case 0:
                additionalSettings.strum_voicing_style = GeneratedAdditionalSettings::PlainAscendingStrumStyle{};
                break;
            case 1:
                additionalSettings.strum_voicing_style = GeneratedAdditionalSettings::OmnichordStrumStyle{};
                break;
        }
        saveAdditionalSettingsToValueTree();
    }
}

void OmnifyAudioProcessor::pushVariantIndexesToValueTree() {
    // Write variant indexes so UI can restore them on rebuild
    magicState.getValueTree().setProperty("variant_chord_voicing_style",
                                          static_cast<int>(additionalSettings.chord_voicing_style.index()), nullptr);
    magicState.getValueTree().setProperty("variant_strum_voicing_style",
                                          static_cast<int>(additionalSettings.strum_voicing_style.index()), nullptr);
}

void OmnifyAudioProcessor::loadAdditionalSettingsFromValueTree() {
    auto jsonString = magicState.getValueTree().getProperty(ADDITIONAL_SETTINGS_KEY, "").toString();
    if (jsonString.isNotEmpty()) {
        try {
            additionalSettings = GeneratedAdditionalSettings::fromJson(jsonString.toStdString());
        } catch (...) {
            additionalSettings = GeneratedAdditionalSettings::Settings::defaults();
        }
    }
}

void OmnifyAudioProcessor::saveAdditionalSettingsToValueTree() {
    auto jsonString = GeneratedAdditionalSettings::toJson(additionalSettings);
    magicState.getValueTree().setProperty(ADDITIONAL_SETTINGS_KEY, juce::String(jsonString), nullptr);
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new OmnifyAudioProcessor(); }
