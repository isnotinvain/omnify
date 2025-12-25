#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>
#include <string>

#include "../LcarsColors.h"

class FromFileView : public juce::Component {
   public:
    FromFileView();

    void setPath(const std::string& path);
    std::function<void(const std::string& newPath)> onPathChanged;

    void resized() override;
    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& event) override;

   private:
    juce::Label pathLabel;
    juce::Rectangle<int> browseButtonBounds;
    std::unique_ptr<juce::FileChooser> fileChooser;

    void launchFileBrowser();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FromFileView)
};
