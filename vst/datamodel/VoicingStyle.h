#pragma once

#include <functional>
#include <json.hpp>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "ChordQuality.h"

enum class VoicingFor { Chord, Strum };

template <VoicingFor T>
class VoicingStyle;

template <VoicingFor T>
using VoicingStyleFactory = std::function<std::shared_ptr<VoicingStyle<T>>(const nlohmann::json&)>;

template <VoicingFor T>
struct VoicingStyleEntry {
    std::shared_ptr<VoicingStyle<T>> style;
    VoicingStyleFactory<T> from_json;
};

// Abstract base class for voicing styles.
template <VoicingFor T>
class VoicingStyle {
   public:
    virtual ~VoicingStyle() = default;
    virtual std::string displayName() const = 0;
    virtual std::vector<int> constructChord(ChordQuality quality, int root) const = 0;
    virtual void to_json(nlohmann::json& j) const = 0;
};

// Registry of available voicing styles - one per plugin instance
template <VoicingFor T>
class VoicingStyleRegistry {
   public:
    VoicingStyleRegistry() = default;

    void registerStyle(const std::string& typeName, std::shared_ptr<VoicingStyle<T>> style, VoicingStyleFactory<T> factory) {
        registry[typeName] = VoicingStyleEntry<T>{std::move(style), std::move(factory)};
    }

    std::shared_ptr<VoicingStyle<T>> from_json(const nlohmann::json& j) const {
        if (!j.contains("type")) {
            throw std::runtime_error("VoicingStyle JSON missing 'type' field");
        }

        std::string typeName = j.at("type").get<std::string>();
        auto it = registry.find(typeName);
        if (it == registry.end()) {
            throw std::runtime_error("Unknown VoicingStyle type: " + typeName);
        }

        return it->second.from_json(j);
    }

    std::map<std::string, VoicingStyleEntry<T>>& getRegistry() { return registry; }

   private:
    std::map<std::string, VoicingStyleEntry<T>> registry;
};
