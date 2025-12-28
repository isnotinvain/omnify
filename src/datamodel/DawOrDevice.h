#pragma once

#include <json.hpp>
#include <string>
#include <variant>

struct Daw {};

struct Device {
    std::string name;
    bool operator==(const Device& other) const { return name == other.name; }
};

using DawOrDevice = std::variant<Daw, Device>;

inline bool isDaw(const DawOrDevice& v) { return std::holds_alternative<Daw>(v); }
inline bool isDevice(const DawOrDevice& v) { return std::holds_alternative<Device>(v); }
inline const std::string& getDeviceName(const DawOrDevice& v) { return std::get<Device>(v).name; }

inline void to_json(nlohmann::json& j, const DawOrDevice& v) {
    if (isDaw(v)) {
        j = nlohmann::json{{"type", "daw"}};
    } else {
        j = nlohmann::json{{"type", "device"}, {"name", getDeviceName(v)}};
    }
}

inline void from_json(const nlohmann::json& j, DawOrDevice& v) {
    std::string type = j.at("type").get<std::string>();
    if (type == "daw") {
        v = Daw{};
    } else {
        v = Device{j.at("name").get<std::string>()};
    }
}
