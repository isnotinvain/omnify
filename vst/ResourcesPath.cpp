#include "ResourcesPath.h"

#include <juce_core/juce_core.h>

#include <stdexcept>

#if JUCE_MAC
#include <dlfcn.h>
#elif JUCE_WINDOWS
#include <windows.h>
#elif JUCE_LINUX
#include <dlfcn.h>
#endif

std::string getResourcesBasePath() {
#if JUCE_MAC
    Dl_info dlInfo;
    static int dummy = 0;
    if (dladdr(&dummy, &dlInfo) && dlInfo.dli_fname) {
        // dli_fname gives us the path to the .so/.dylib inside MacOS/
        // Go up to Contents, then into Resources
        auto pluginBundle = juce::File(dlInfo.dli_fname).getParentDirectory().getParentDirectory().getParentDirectory();
        return pluginBundle.getChildFile("Contents/Resources").getFullPathName().toStdString();
    }
    throw std::runtime_error("Failed to determine plugin path via dladdr");
#elif JUCE_WINDOWS
    HMODULE hModule = nullptr;
    static int dummy = 0;
    if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCSTR>(&dummy),
                           &hModule)) {
        char path[MAX_PATH];
        if (GetModuleFileNameA(hModule, path, MAX_PATH)) {
            // path is something like C:\...\Plugin.vst3\Contents\x86_64-win\Plugin.vst3
            // Go up to Contents, then into Resources
            auto pluginFile = juce::File(path);
            auto contentsDir = pluginFile.getParentDirectory().getParentDirectory();
            return contentsDir.getChildFile("Resources").getFullPathName().toStdString();
        }
    }
    throw std::runtime_error("Failed to determine plugin path via GetModuleHandleEx");
#elif JUCE_LINUX
    Dl_info dlInfo;
    static int dummy = 0;
    if (dladdr(&dummy, &dlInfo) && dlInfo.dli_fname) {
        // dli_fname gives us path to the .so inside x86_64-linux/
        // Go up to Contents, then into Resources
        auto pluginFile = juce::File(dlInfo.dli_fname);
        auto contentsDir = pluginFile.getParentDirectory().getParentDirectory();
        return contentsDir.getChildFile("Resources").getFullPathName().toStdString();
    }
    throw std::runtime_error("Failed to determine plugin path via dladdr");
#else
    throw std::runtime_error("getResourcesBasePath not implemented for this platform");
#endif
}
