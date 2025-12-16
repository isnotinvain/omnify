#pragma once

#include <functional>
#include <json.hpp>
#include <map>
#include <memory>
#include <optional>
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

// Registry of available voicing styles - one per plugin instance.
// Styles are registered once at startup and never modified after.
template <VoicingFor T>
class VoicingStyleRegistry {
   public:
    VoicingStyleRegistry() = default;

    void registerStyle(const std::string& typeName, std::shared_ptr<VoicingStyle<T>> style, VoicingStyleFactory<T> factory) {
        styleToTypeName[style.get()] = typeName;
        registry[typeName] = VoicingStyleEntry<T>{std::move(style), std::move(factory)};
    }

    std::shared_ptr<VoicingStyle<T>> from_json(const nlohmann::json& j) {
        if (!j.contains("type")) {
            throw std::runtime_error("VoicingStyle JSON missing 'type' field");
        }

        std::string typeName = j.at("type").get<std::string>();
        auto it = registry.find(typeName);
        if (it == registry.end()) {
            throw std::runtime_error("Unknown VoicingStyle type: " + typeName);
        }

        // Replace canonical instance with loaded one
        auto loaded = it->second.from_json(j);
        styleToTypeName.erase(it->second.style.get());
        it->second.style = loaded;
        styleToTypeName[loaded.get()] = typeName;
        return loaded;
    }

    std::optional<std::string> getTypeName(const VoicingStyle<T>* style) const {
        auto it = styleToTypeName.find(style);
        if (it != styleToTypeName.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    std::map<std::string, VoicingStyleEntry<T>>& getRegistry() { return registry; }
    const std::map<std::string, VoicingStyleEntry<T>>& getRegistry() const { return registry; }

   private:
    std::map<std::string, VoicingStyleEntry<T>> registry;
    std::map<const VoicingStyle<T>*, std::string> styleToTypeName;
};
