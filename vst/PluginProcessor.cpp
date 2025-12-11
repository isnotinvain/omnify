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
juce::AudioProcessorEditor* OmnifyAudioProcessor::createEditor() {
    auto* editor = foleys::MagicProcessor::createEditor();
    auto* magicEditor = dynamic_cast<foleys::MagicPluginEditor*>(editor);
    if (magicEditor != nullptr) {
        wireUpSettingsBindings(magicEditor->getGUIBuilder());
    }
    return editor;
}

void OmnifyAudioProcessor::wireUpSettingsBindings(foleys::MagicGUIBuilder& builder) {
    // Wire up latch_toggle_button
    if (auto* latchItem = dynamic_cast<MidiLearnItem*>(builder.findGuiItemWithId("latch_toggle_button"))) {
        auto& midiLearn = latchItem->getMidiLearnComponent();
        // Load initial value from settings
        std::visit(
            [&midiLearn](auto&& btn) {
                using T = std::decay_t<decltype(btn)>;
                if constexpr (std::is_same_v<T, GeneratedAdditionalSettings::MidiNoteButton>) {
                    midiLearn.setLearnedValue({MidiLearnedType::Note, btn.note});
                } else if constexpr (std::is_same_v<T, GeneratedAdditionalSettings::MidiCCButton>) {
                    midiLearn.setLearnedValue({MidiLearnedType::CC, btn.cc});
                }
            },
            additionalSettings.latch_toggle_button);
        // Set callback for when user changes value
        midiLearn.onValueChanged = [this](MidiLearnedValue val) {
            if (val.type == MidiLearnedType::Note) {
                additionalSettings.latch_toggle_button =
                    GeneratedAdditionalSettings::MidiNoteButton{.note = val.value};
            } else if (val.type == MidiLearnedType::CC) {
                additionalSettings.latch_toggle_button =
                    GeneratedAdditionalSettings::MidiCCButton{.cc = val.value, .is_toggle = true};
            }
            saveAdditionalSettingsToValueTree();
        };
    }

    // Wire up stop_button
    if (auto* stopItem = dynamic_cast<MidiLearnItem*>(builder.findGuiItemWithId("stop_button"))) {
        auto& midiLearn = stopItem->getMidiLearnComponent();
        // Load initial value from settings
        std::visit(
            [&midiLearn](auto&& btn) {
                using T = std::decay_t<decltype(btn)>;
                if constexpr (std::is_same_v<T, GeneratedAdditionalSettings::MidiNoteButton>) {
                    midiLearn.setLearnedValue({MidiLearnedType::Note, btn.note});
                } else if constexpr (std::is_same_v<T, GeneratedAdditionalSettings::MidiCCButton>) {
                    midiLearn.setLearnedValue({MidiLearnedType::CC, btn.cc});
                }
            },
            additionalSettings.stop_button);
        // Set callback for when user changes value
        midiLearn.onValueChanged = [this](MidiLearnedValue val) {
            if (val.type == MidiLearnedType::Note) {
                additionalSettings.stop_button =
                    GeneratedAdditionalSettings::MidiNoteButton{.note = val.value};
            } else if (val.type == MidiLearnedType::CC) {
                additionalSettings.stop_button =
                    GeneratedAdditionalSettings::MidiCCButton{.cc = val.value, .is_toggle = false};
            }
            saveAdditionalSettingsToValueTree();
        };
    }

    // Wire up chord_quality_cc (for CCRangePerChordQuality mode)
    if (auto* ccItem = dynamic_cast<MidiLearnItem*>(builder.findGuiItemWithId("chord_quality_cc"))) {
        auto& midiLearn = ccItem->getMidiLearnComponent();
        // Load initial value if we're in CCRangePerChordQuality mode
        if (auto* ccRange =
                std::get_if<GeneratedAdditionalSettings::CCRangePerChordQuality>(
                    &additionalSettings.chord_quality_selection_style)) {
            midiLearn.setLearnedValue({MidiLearnedType::CC, ccRange->cc});
        }
        // Set callback for when user changes value
        midiLearn.onValueChanged = [this](MidiLearnedValue val) {
            if (val.type == MidiLearnedType::CC) {
                additionalSettings.chord_quality_selection_style =
                    GeneratedAdditionalSettings::CCRangePerChordQuality{.cc = val.value};
                saveAdditionalSettingsToValueTree();
            }
        };
    }

    // Wire up chord_quality_selector (for NotePerChordQuality mode)
    if (auto* selector = dynamic_cast<ChordQualitySelectorItem*>(
            builder.findGuiItemWithId("chord_quality_selector"))) {
        // Load initial values if we're in NotePerChordQuality mode
        if (auto* noteStyle = std::get_if<GeneratedAdditionalSettings::NotePerChordQuality>(
                &additionalSettings.chord_quality_selection_style)) {
            for (const auto& [note, quality] : noteStyle->note_mapping) {
                auto qualityIndex = static_cast<size_t>(quality);
                selector->setLearnerValue(qualityIndex, {MidiLearnedType::Note, note});
            }
        }
        // Set callbacks for each quality
        for (size_t i = 0; i < ChordQualitySelectorItem::NUM_QUALITIES; ++i) {
            selector->setOnValueChanged(i, [this, i](MidiLearnedValue val) {
                // Get or create NotePerChordQuality
                auto* noteStyle = std::get_if<GeneratedAdditionalSettings::NotePerChordQuality>(
                    &additionalSettings.chord_quality_selection_style);
                if (noteStyle == nullptr) {
                    additionalSettings.chord_quality_selection_style =
                        GeneratedAdditionalSettings::NotePerChordQuality{};
                    noteStyle = std::get_if<GeneratedAdditionalSettings::NotePerChordQuality>(
                        &additionalSettings.chord_quality_selection_style);
                }
                auto quality = static_cast<GeneratedAdditionalSettings::ChordQuality>(i);
                // Remove old mapping for this quality
                for (auto it = noteStyle->note_mapping.begin();
                     it != noteStyle->note_mapping.end();) {
                    if (it->second == quality) {
                        it = noteStyle->note_mapping.erase(it);
                    } else {
                        ++it;
                    }
                }
                // Add new mapping if valid
                if (val.type == MidiLearnedType::Note && val.value >= 0) {
                    noteStyle->note_mapping[val.value] = quality;
                }
                saveAdditionalSettingsToValueTree();
            });
        }
    }
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