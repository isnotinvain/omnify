#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "../LcarsColors.h"

// A file picker with a label showing the path and a browse button.
// Path is truncated from the left if too long.
class FilePicker : public juce::Component, private juce::Value::Listener {
   public:
    FilePicker();
    ~FilePicker() override;

    void setFileFilter(const juce::String& filter);
    void bindToValue(juce::Value& value);

    void resized() override;

   private:
    // Simple label with rounded border and path truncation from the left
    class RoundedLabel : public juce::Label {
       public:
        void paint(juce::Graphics& g) override {
            auto bounds = getLocalBounds().toFloat().reduced(1.0f);
            g.setColour(findColour(outlineColourId));
            g.drawRoundedRectangle(bounds, 4.0f, 2.0f);

            // Draw text - truncate from left if too long
            g.setColour(findColour(textColourId));
            auto font = getLookAndFeel().getLabelFont(*this);
            g.setFont(font);

            auto textBounds = getLocalBounds().reduced(8, 0);
            auto text = getText();

            // Measure text width using GlyphArrangement
            juce::GlyphArrangement glyphs;
            glyphs.addLineOfText(font, text, 0, 0);
            auto textWidth = glyphs.getBoundingBox(0, -1, false).getWidth();

            if (textWidth > textBounds.getWidth() && text.isNotEmpty()) {
                // Truncate from the left, show "..." prefix
                juce::String truncated = "..." + text;
                while (truncated.length() > 4) {
                    glyphs.clear();
                    glyphs.addLineOfText(font, truncated, 0, 0);
                    if (glyphs.getBoundingBox(0, -1, false).getWidth() <= textBounds.getWidth())
                        break;
                    truncated = "..." + truncated.substring(4);  // Remove char after "..."
                }
                text = truncated;
            }

            g.drawText(text, textBounds, juce::Justification::centredLeft, false);
        }
    };

    void openFileChooser();
    void valueChanged(juce::Value& value) override;

    RoundedLabel pathLabel;
    juce::TextButton browseButton{"Browse"};
    std::unique_ptr<juce::FileChooser> fileChooser;
    juce::String fileFilter = "*";
    juce::Value boundValue;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FilePicker)
};
