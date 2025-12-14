#!/usr/bin/env python3
"""
Generate C++ settings structs from DaemomnifySettings pydantic model.

This script introspects the DaemomnifySettings class and generates:
- GeneratedSettings.h/cpp: C++ structs + JSON serialization for all settings

The generated code uses nlohmann::json for JSON marshalling.
"""

from pathlib import Path
from typing import Annotated, Union, get_args, get_origin

from pydantic import BaseModel

from daemomnify.chord_quality import ChordQuality
from daemomnify.settings import DaemomnifySettings


def snake_to_pascal(name: str) -> str:
    """Convert snake_case to PascalCase."""
    return "".join(x.title() for x in name.split("_"))


def get_base_type(annotation):
    """Get the base type from an Annotated type."""
    if get_origin(annotation) is Annotated:
        return get_args(annotation)[0]
    return annotation


def get_union_types(annotation) -> list[type] | None:
    """Extract types from a Union type, unwrapping Annotated if needed."""
    # Unwrap Annotated first
    base = get_base_type(annotation)
    origin = get_origin(base)

    if origin is Union:
        return list(get_args(base))
    return None


def get_discriminator_value(model_class: type) -> str | None:
    """Get the discriminator 'type' field default value from a model class."""
    if hasattr(model_class, "model_fields") and "type" in model_class.model_fields:
        type_field = model_class.model_fields["type"]
        if type_field.default:
            return type_field.default
    return None


# =============================================================================
# JSON Blob C++ Code Generation (GeneratedSettings.h/cpp)
# =============================================================================


def get_variant_signature(union_types: list[type]) -> str:
    """Get a unique signature for a variant based on its component types."""
    type_names = sorted(ut.__name__ for ut in union_types if get_discriminator_value(ut))
    return "|".join(type_names)


def collect_variant_types(model_class: type[BaseModel], collected: dict, variant_signatures: dict) -> None:
    """Recursively collect all variant types and their structures.

    variant_signatures maps signature -> variant_name to avoid duplicates.
    """
    for field_name, field_info in model_class.model_fields.items():
        if field_name == "type":
            continue

        annotation = model_class.__annotations__[field_name]
        union_types = get_union_types(annotation)

        if union_types:
            # Check if we've already seen this exact variant composition
            sig = get_variant_signature(union_types)

            if sig in variant_signatures:
                # Reuse the existing variant name
                collected[field_name] = {
                    "variant_name": variant_signatures[sig],
                    "variants": collected[next(k for k, v in collected.items() if v.get("variant_name") == variant_signatures[sig])]["variants"],
                    "is_alias": True,  # Mark as alias, don't generate separate to_json/from_json
                }
            else:
                # This is a discriminated union - create new variant
                variant_name = snake_to_pascal(field_name)
                variants = []
                for ut in union_types:
                    disc_value = get_discriminator_value(ut)
                    if disc_value:
                        struct_name = ut.__name__
                        variants.append({
                            "struct_name": struct_name,
                            "type_value": disc_value,
                            "fields": collect_struct_fields(ut),
                        })
                        # Recursively collect nested types
                        collect_variant_types(ut, collected, variant_signatures)

                collected[field_name] = {
                    "variant_name": variant_name,
                    "variants": variants,
                    "is_alias": False,
                }
                variant_signatures[sig] = variant_name
        elif hasattr(annotation, "model_fields"):
            # Nested model
            collect_variant_types(annotation, collected, variant_signatures)


def collect_struct_fields(model_class: type[BaseModel]) -> list[dict]:
    """Collect the serializable fields of a model class."""
    fields = []
    for field_name, field_info in model_class.model_fields.items():
        if field_name == "type":
            continue

        annotation = model_class.__annotations__[field_name]
        base_type = get_base_type(annotation)

        # Determine C++ type
        cpp_type = python_type_to_cpp(base_type, field_name)
        if cpp_type:
            fields.append({
                "name": field_name,
                "cpp_type": cpp_type,
                "python_type": base_type,
            })

    return fields


def python_type_to_cpp(py_type, field_name: str) -> str | None:
    """Convert Python type to C++ type string."""
    if py_type is int:
        return "int"
    elif py_type is float:
        return "double"
    elif py_type is bool:
        return "bool"
    elif py_type is str:
        return "std::string"
    elif hasattr(py_type, "__origin__"):
        origin = get_origin(py_type)
        args = get_args(py_type)
        if origin is dict:
            key_type = python_type_to_cpp(args[0], field_name) if args else "int"
            val_type = python_type_to_cpp(args[1], field_name) if len(args) > 1 else "int"
            return f"std::map<{key_type}, {val_type}>"
        elif origin is list:
            elem_type = python_type_to_cpp(args[0], field_name) if args else "int"
            return f"std::vector<{elem_type}>"
    elif py_type is ChordQuality or (hasattr(py_type, "__name__") and py_type.__name__ == "ChordQuality"):
        return "ChordQuality"

    return None


def generate_settings_header(settings_class: type[BaseModel]) -> str:
    """Generate GeneratedSettings.h"""
    lines = [
        "// AUTO-GENERATED FILE - DO NOT EDIT",
        "// Generated by vst/generate_settings.py",
        "#pragma once",
        "",
        "#include <string>",
        "#include <variant>",
        "#include <map>",
        "#include <vector>",
        "#include <json.hpp>",
        "",
        "namespace GeneratedSettings {",
        "",
    ]

    # ChordQuality enum
    lines.append("// ChordQuality enum (mirrors Python)")
    lines.append("enum class ChordQuality {")
    for i, q in enumerate(ChordQuality):
        comma = "," if i < len(ChordQuality) - 1 else ""
        lines.append(f"    {q.name}{comma}")
    lines.append("};")
    lines.append("")

    # ChordQualities helper namespace (for UI components)
    lines.append("// ChordQualities helper namespace for UI")
    lines.append("namespace ChordQualities {")
    lines.append(f"inline constexpr int NUM_QUALITIES = {len(ChordQuality)};")
    names = [q.value.nice_name for q in ChordQuality]
    names_str = ", ".join(f'"{name}"' for name in names)
    lines.append(f"inline const char* NAMES[NUM_QUALITIES] = {{{names_str}}};")
    enum_names = [q.name for q in ChordQuality]
    enum_names_str = ", ".join(f'"{name}"' for name in enum_names)
    lines.append(f"inline const char* ENUM_NAMES[NUM_QUALITIES] = {{{enum_names_str}}};")
    lines.append("}  // namespace ChordQualities")
    lines.append("")

    # Collect all variant types from DaemomnifySettings
    collected = {}
    variant_signatures = {}
    collect_variant_types(settings_class, collected, variant_signatures)

    # Generate struct definitions for each variant
    all_structs = []
    for field_name, info in collected.items():
        for variant in info["variants"]:
            all_structs.append(variant)

    # Sort and deduplicate
    seen = set()
    unique_structs = []
    for s in all_structs:
        if s["struct_name"] not in seen:
            seen.add(s["struct_name"])
            unique_structs.append(s)

    # Generate structs
    for struct in unique_structs:
        lines.append(f"struct {struct['struct_name']} {{")
        lines.append(f'    static constexpr const char* TYPE = "{struct["type_value"]}";')
        for field in struct["fields"]:
            lines.append(f"    {field['cpp_type']} {field['name']};")
        lines.append("};")
        lines.append("")

    # Generate variant typedefs (skip aliases - they'll use the same type)
    seen_variant_names = set()
    for field_name, info in collected.items():
        typedef_name = info["variant_name"]
        if typedef_name in seen_variant_names:
            continue
        seen_variant_names.add(typedef_name)
        variant_types = ", ".join(v["struct_name"] for v in info["variants"])
        lines.append(f"using {typedef_name} = std::variant<{variant_types}>;")
    lines.append("")

    # Main DaemomnifySettings struct (all settings flat)
    lines.append("struct DaemomnifySettings {")
    for field_name in settings_class.model_fields:
        annotation = settings_class.__annotations__[field_name]
        union_types = get_union_types(annotation)

        if union_types:
            cpp_type = collected[field_name]["variant_name"]
        else:
            base_type = get_base_type(annotation)
            cpp_type = python_type_to_cpp(base_type, field_name)

        if cpp_type:
            lines.append(f"    {cpp_type} {field_name};")
    lines.append("")
    lines.append("    static DaemomnifySettings defaults();")
    lines.append("};")
    lines.append("")

    # Function declarations
    lines.extend([
        "// JSON serialization",
        "std::string toJson(const DaemomnifySettings& settings);",
        "DaemomnifySettings fromJson(const std::string& json);",
        "bool isValidJson(const std::string& json);",
        "",
        "// nlohmann JSON conversion declarations",
        "void to_json(nlohmann::json& j, const DaemomnifySettings& s);",
        "void from_json(const nlohmann::json& j, DaemomnifySettings& s);",
        "",
    ])

    # JSON conversion for each variant type
    for struct in unique_structs:
        lines.append(f"void to_json(nlohmann::json& j, const {struct['struct_name']}& s);")
        lines.append(f"void from_json(const nlohmann::json& j, {struct['struct_name']}& s);")
    lines.append("")

    # JSON conversion for variant types (skip duplicates)
    seen_variant_names = set()
    for field_name, info in collected.items():
        typedef_name = info["variant_name"]
        if typedef_name in seen_variant_names:
            continue
        seen_variant_names.add(typedef_name)
        lines.append(f"void to_json(nlohmann::json& j, const {typedef_name}& v);")
        lines.append(f"void from_json(const nlohmann::json& j, {typedef_name}& v);")
    lines.append("")

    lines.extend([
        "}  // namespace GeneratedSettings",
        "",
    ])

    return "\n".join(lines)


def generate_settings_cpp(settings_class: type[BaseModel]) -> str:
    """Generate GeneratedSettings.cpp"""
    lines = [
        "// AUTO-GENERATED FILE - DO NOT EDIT",
        "// Generated by vst/generate_settings.py",
        '#include "GeneratedSettings.h"',
        "",
        "namespace GeneratedSettings {",
        "",
    ]

    # ChordQuality JSON conversion
    lines.append("// ChordQuality JSON conversion")
    lines.append("NLOHMANN_JSON_SERIALIZE_ENUM(ChordQuality, {")
    for q in ChordQuality:
        lines.append(f'    {{ChordQuality::{q.name}, "{q.name}"}},')
    lines.append("})")
    lines.append("")

    # Collect types
    collected = {}
    variant_signatures = {}
    collect_variant_types(settings_class, collected, variant_signatures)

    all_structs = []
    for field_name, info in collected.items():
        for variant in info["variants"]:
            all_structs.append(variant)

    seen = set()
    unique_structs = []
    for s in all_structs:
        if s["struct_name"] not in seen:
            seen.add(s["struct_name"])
            unique_structs.append(s)

    # Generate to_json/from_json for each struct
    for struct in unique_structs:
        struct_name = struct["struct_name"]
        has_fields = len(struct["fields"]) > 0

        # to_json - use [[maybe_unused]] for structs with no fields
        if has_fields:
            lines.append(f"void to_json(nlohmann::json& j, const {struct_name}& s) {{")
        else:
            lines.append(f"void to_json(nlohmann::json& j, [[maybe_unused]] const {struct_name}& s) {{")
        lines.append(f'    j["type"] = {struct_name}::TYPE;')
        for field in struct["fields"]:
            lines.append(f'    j["{field["name"]}"] = s.{field["name"]};')
        lines.append("}")
        lines.append("")

        # from_json - use [[maybe_unused]] for structs with no fields
        if has_fields:
            lines.append(f"void from_json(const nlohmann::json& j, {struct_name}& s) {{")
        else:
            lines.append(f"void from_json([[maybe_unused]] const nlohmann::json& j, [[maybe_unused]] {struct_name}& s) {{")
        for field in struct["fields"]:
            # Special handling for std::map<int, ...> - JSON has string keys
            if field["cpp_type"].startswith("std::map<int,"):
                value_type = field["cpp_type"].split(",", 1)[1].strip().rstrip(">")
                lines.append(f'    for (auto& [key, val] : j.at("{field["name"]}").items()) {{')
                lines.append(f'        s.{field["name"]}[std::stoi(key)] = val.get<{value_type}>();')
                lines.append("    }")
            else:
                lines.append(f'    j.at("{field["name"]}").get_to(s.{field["name"]});')
        lines.append("}")
        lines.append("")

    # Generate to_json/from_json for variant types (skip duplicates)
    seen_variant_names = set()
    for field_name, info in collected.items():
        typedef_name = info["variant_name"]
        if typedef_name in seen_variant_names:
            continue
        seen_variant_names.add(typedef_name)
        variants = info["variants"]

        # to_json for variant
        lines.append(f"void to_json(nlohmann::json& j, const {typedef_name}& v) {{")
        lines.append("    std::visit([&j](const auto& val) {")
        lines.append("        j = val;")
        lines.append("    }, v);")
        lines.append("}")
        lines.append("")

        # from_json for variant
        lines.append(f"void from_json(const nlohmann::json& j, {typedef_name}& v) {{")
        lines.append('    std::string type = j.at("type").get<std::string>();')
        for i, variant in enumerate(variants):
            keyword = "if" if i == 0 else "} else if"
            lines.append(f'    {keyword} (type == {variant["struct_name"]}::TYPE) {{')
            lines.append(f'        v = j.get<{variant["struct_name"]}>();')
        lines.append("    } else {")
        lines.append('        throw std::runtime_error("Unknown type: " + type);')
        lines.append("    }")
        lines.append("}")
        lines.append("")

    # DaemomnifySettings to_json/from_json
    lines.append("void to_json(nlohmann::json& j, const DaemomnifySettings& s) {")
    for field_name in settings_class.model_fields:
        lines.append(f'    j["{field_name}"] = s.{field_name};')
    lines.append("}")
    lines.append("")

    lines.append("void from_json(const nlohmann::json& j, DaemomnifySettings& s) {")
    for field_name in settings_class.model_fields:
        lines.append(f'    j.at("{field_name}").get_to(s.{field_name});')
    lines.append("}")
    lines.append("")

    # Public API functions
    lines.append("std::string toJson(const DaemomnifySettings& settings) {")
    lines.append("    nlohmann::json j = settings;")
    lines.append("    return j.dump();")
    lines.append("}")
    lines.append("")

    lines.append("DaemomnifySettings fromJson(const std::string& json) {")
    lines.append("    auto j = nlohmann::json::parse(json);")
    lines.append("    return j.get<DaemomnifySettings>();")
    lines.append("}")
    lines.append("")

    lines.append("bool isValidJson(const std::string& json) {")
    lines.append("    try {")
    lines.append("        fromJson(json);")
    lines.append("        return true;")
    lines.append("    } catch (...) {")
    lines.append("        return false;")
    lines.append("    }")
    lines.append("}")
    lines.append("")

    # defaults() function
    lines.append("DaemomnifySettings DaemomnifySettings::defaults() {")
    lines.append("    return DaemomnifySettings{")

    # Import default values from Python
    from daemomnify.settings import DEFAULT_SETTINGS
    settings = DEFAULT_SETTINGS

    for field_name in settings_class.model_fields:
        field_value = getattr(settings, field_name)
        cpp_init = python_value_to_cpp_init(field_value)
        lines.append(f"        .{field_name} = {cpp_init},")

    lines.append("    };")
    lines.append("}")
    lines.append("")

    lines.extend([
        "}  // namespace GeneratedSettings",
        "",
    ])

    return "\n".join(lines)


def python_value_to_cpp_init(value) -> str:
    """Convert a Python value to C++ initializer syntax."""
    # Check for ChordQuality first (before model_dump which serializes it to string)
    if isinstance(value, ChordQuality):
        return f"ChordQuality::{value.name}"

    if hasattr(value, "model_dump"):
        # It's a Pydantic model - iterate over actual fields, not serialized values
        class_name = value.__class__.__name__

        fields = []
        for fname in value.__class__.model_fields:
            if fname == "type":
                continue
            fval = getattr(value, fname)
            cpp_val = python_value_to_cpp_init(fval)
            fields.append(f".{fname} = {cpp_val}")

        if fields:
            return f"{class_name}{{{', '.join(fields)}}}"
        else:
            return f"{class_name}{{}}"
    elif isinstance(value, dict):
        items = []
        for k, v in value.items():
            cpp_key = python_value_to_cpp_init(k)
            cpp_val = python_value_to_cpp_init(v)
            items.append(f"{{{cpp_key}, {cpp_val}}}")
        return "{" + ", ".join(items) + "}"
    elif isinstance(value, bool):
        return "true" if value else "false"
    elif isinstance(value, int):
        return str(value)
    elif isinstance(value, float):
        return f"{value}"
    elif isinstance(value, str):
        return f'"{value}"'
    elif hasattr(value, "name"):
        # Likely an enum
        return f"ChordQuality::{value.name}"
    else:
        return "{}"


def main():
    output_dir = Path(__file__).parent

    print("Generating DaemomnifySettings C++ code...")
    settings_header = generate_settings_header(DaemomnifySettings)
    settings_cpp = generate_settings_cpp(DaemomnifySettings)

    settings_header_path = output_dir / "GeneratedSettings.h"
    settings_cpp_path = output_dir / "GeneratedSettings.cpp"

    settings_header_path.write_text(settings_header)
    settings_cpp_path.write_text(settings_cpp)

    print(f"Generated {settings_header_path}")
    print(f"Generated {settings_cpp_path}")


if __name__ == "__main__":
    main()
