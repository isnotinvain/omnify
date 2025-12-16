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

   private:
    juce::Label pathLabel;
    juce::TextButton browseButton{"Browse..."};
    std::unique_ptr<juce::FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FromFileView)
};
