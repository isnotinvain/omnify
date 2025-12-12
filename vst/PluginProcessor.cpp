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
    chordVoicingFilePathValue.removeListener(this);
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
    builder.registerFactory("FilePicker", &FilePickerItem::factory);
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
    chordVoicingFilePathValue.referTo(magicState.getValueTree().getPropertyAsValue("filepath_chord_voicing_file", nullptr));

    chordVoicingStyleValue.addListener(this);
    strumVoicingStyleValue.addListener(this);
    chordVoicingFilePathValue.addListener(this);
}

void OmnifyAudioProcessor::valueChanged(juce::Value& value) {
    if (value.refersToSameSourceAs(chordVoicingStyleValue)) {
        int index = static_cast<int>(chordVoicingStyleValue.getValue());
        // When switching to FileStyle, restore path from ValueTree (it persists there even when variant changes)
        switch (index) {
            case 0:
                additionalSettings.chord_voicing_style = GeneratedAdditionalSettings::RootPositionStyle{};
                break;
            case 1: {
                GeneratedAdditionalSettings::FileStyle fs;
                // Restore path from ValueTree property (persists across variant switches)
                fs.path = magicState.getValueTree()
                              .getProperty("filepath_chord_voicing_file", "")
                              .toString()
                              .toStdString();
                additionalSettings.chord_voicing_style = fs;
                break;
            }
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
    } else if (value.refersToSameSourceAs(chordVoicingFilePathValue)) {
        auto path = chordVoicingFilePathValue.getValue().toString().toStdString();
        DBG("valueChanged(filepath): path = " << path);
        // Update path in FileStyle if that's the current variant
        if (auto* fileStyle = std::get_if<GeneratedAdditionalSettings::FileStyle>(&additionalSettings.chord_voicing_style)) {
            fileStyle->path = path;
            saveAdditionalSettingsToValueTree();
            DBG("valueChanged(filepath): saved to additionalSettings");
        } else {
            DBG("valueChanged(filepath): FileStyle not active, path not saved to additionalSettings");
        }
    }
}

void OmnifyAudioProcessor::pushVariantIndexesToValueTree() {
    // Write variant indexes so UI can restore them on rebuild
    magicState.getValueTree().setProperty("variant_chord_voicing_style",
                                          static_cast<int>(additionalSettings.chord_voicing_style.index()), nullptr);
    magicState.getValueTree().setProperty("variant_strum_voicing_style",
                                          static_cast<int>(additionalSettings.strum_voicing_style.index()), nullptr);

    // Write file path from FileStyle if that's the active variant
    if (auto* fileStyle = std::get_if<GeneratedAdditionalSettings::FileStyle>(&additionalSettings.chord_voicing_style)) {
        DBG("pushVariantIndexesToValueTree: FileStyle path = " << fileStyle->path);
        magicState.getValueTree().setProperty("filepath_chord_voicing_file",
                                              juce::String(fileStyle->path), nullptr);
    }
}

void OmnifyAudioProcessor::loadAdditionalSettingsFromValueTree() {
    auto jsonString = magicState.getValueTree().getProperty(ADDITIONAL_SETTINGS_KEY, "").toString();
    DBG("loadAdditionalSettingsFromValueTree: JSON = " << jsonString);
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
