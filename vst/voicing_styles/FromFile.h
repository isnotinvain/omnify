#pragma once

#include <filesystem>
#include <fstream>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "../ResourcesPath.h"
#include "../datamodel/VoicingStyle.h"

struct ChordFile {
    std::string name;
    std::string description;
    bool isOffsetFile;
    std::map<ChordQuality, std::map<int, std::vector<int>>> chords;
};

// JSON has string keys, so we parse quality names and root numbers from strings
inline void from_json(const nlohmann::json& j, ChordFile& cf) {
    j.at("name").get_to(cf.name);
    j.at("description").get_to(cf.description);
    j.at("isOffsetFile").get_to(cf.isOffsetFile);

    for (const auto& [qualityName, rootMap] : j.at("chords").items()) {
        ChordQuality quality = chordQualityFromName(qualityName);
        for (const auto& [rootStr, notes] : rootMap.items()) {
            int root = std::stoi(rootStr);
            cf.chords[quality][root] = notes.get<std::vector<int>>();
        }
    }
}

template <VoicingFor T>
class FromFile : public VoicingStyle<T> {
   public:
    explicit FromFile(std::string filePath) : path(std::move(filePath)) {}

    std::string displayName() const override { return "File"; }

    std::vector<int> constructChord(ChordQuality quality, int root) const override {
        const ChordFile& chordData = getData();
        int noteClass = root % 12;
        const auto& offsetsOrNotes = chordData.chords.at(quality).at(noteClass);

        if (chordData.isOffsetFile) {
            std::vector<int> notes;
            notes.reserve(offsetsOrNotes.size());
            for (int offset : offsetsOrNotes) {
                notes.push_back(root + offset);
            }
            return notes;
        }
        return offsetsOrNotes;
    }

    void to_json(nlohmann::json& j) const override { j = nlohmann::json{{"type", "FromFile"}, {"path", path}}; }

    static std::shared_ptr<VoicingStyle<T>> from_json(const nlohmann::json& j) {
        return std::make_shared<FromFile<T>>(j.at("path").get<std::string>());
    }

    const std::string& getPath() const { return path; }

    void setPath(const std::string& newPath) {
        path = newPath;
        data.reset();  // Clear cached data so it reloads
    }

   private:
    std::string path;
    mutable std::optional<ChordFile> data;

    std::string resolvedPath() const {
        std::filesystem::path p(path);
        if (p.empty() || p.is_absolute()) {
            return path;
        }
        // relative path, resolve against resources base
        return (std::filesystem::path(getResourcesBasePath()) / p).string();
    }

    const ChordFile& getData() const {
        if (!data) {
            std::ifstream file(resolvedPath());
            nlohmann::json j;
            file >> j;
            data = j.get<ChordFile>();
        }
        return *data;
    }
};
