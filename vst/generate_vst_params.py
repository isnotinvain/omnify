#!/usr/bin/env python3
"""
Generate C++ parameter definitions and settings structs from DaemomnifySettings pydantic model.

This script introspects the DaemomnifySettings class and generates:
- GeneratedParams.h/cpp: Scalar VST parameter definitions
- GeneratedAdditionalSettings.h/cpp: C++ structs + JSON serialization for the JSON blob

It uses VSTParam annotations from vst_params.py to determine how fields
should be exposed as VST parameters vs included in the JSON blob.
"""

from pathlib import Path
from typing import Annotated, Union, get_args, get_origin

from pydantic import BaseModel

from daemomnify import vst_params
from daemomnify.chord_quality import ChordQuality
from daemomnify.settings import AdditionalSettings, DaemomnifySettings


def snake_to_pascal(name: str) -> str:
    """Convert snake_case to PascalCase."""
    return "".join(x.title() for x in name.split("_"))


def get_vst_annotation(annotation) -> vst_params.VSTParam | None:
    """Extract VSTParam from an Annotated type's metadata."""
    if get_origin(annotation) is not Annotated:
        return None

    for arg in get_args(annotation):
        if isinstance(arg, vst_params.VSTParam):
            return arg
    return None


def get_base_type(annotation):
    """Get the base type from an Annotated type."""
    if get_origin(annotation) is Annotated:
        return get_args(annotation)[0]
    return annotation


def get_union_types(annotation) -> list[type] | None:
    """Extract types from a Union type."""
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
# Scalar Parameters Generation (GeneratedParams.h/cpp)
# =============================================================================


def collect_scalar_params(model_class: type[BaseModel]) -> list[dict]:
    """Collect scalar VST parameters (non-JSON-blob fields)."""
    params = []

    for field_name, field_info in model_class.model_fields.items():
        annotation = model_class.__annotations__[field_name]
        vst_param = get_vst_annotation(annotation)

        # Skip JSON blob fields and explicitly skipped fields
        if isinstance(vst_param, (vst_params.VSTSkip, vst_params.VSTJsonBlob)):
            continue

        if isinstance(vst_param, vst_params.VSTInt):
            params.append({
                "field_name": field_name,
                "param_id": field_name,
                "label": vst_param.label or field_name.replace("_", " ").title(),
                "type": "int",
                "min": vst_param.min,
                "max": vst_param.max,
                "default": vst_param.default,
            })
        elif isinstance(vst_param, vst_params.VSTFloat):
            params.append({
                "field_name": field_name,
                "param_id": field_name,
                "label": vst_param.label or field_name.replace("_", " ").title(),
                "type": "float",
                "min": vst_param.min,
                "max": vst_param.max,
                "default": vst_param.default,
            })
        elif isinstance(vst_param, vst_params.VSTBool):
            params.append({
                "field_name": field_name,
                "param_id": field_name,
                "label": vst_param.label or field_name.replace("_", " ").title(),
                "type": "bool",
                "default": vst_param.default,
            })
        elif isinstance(vst_param, vst_params.VSTIntChoice):
            choices = [str(i) for i in range(vst_param.min, vst_param.max + 1)]
            params.append({
                "field_name": field_name,
                "param_id": field_name,
                "label": vst_param.label or field_name.replace("_", " ").title(),
                "type": "int_choice",
                "choices": choices,
                "namespace": snake_to_pascal(field_name) + "Choices",
                "default": vst_param.default - vst_param.min,
            })
        elif isinstance(vst_param, vst_params.VSTString):
            params.append({
                "field_name": field_name,
                "param_id": field_name,
                "label": vst_param.label or field_name.replace("_", " ").title(),
                "type": "string",
                "default": vst_param.default,
            })

    return params


def generate_params_header(params: list[dict]) -> str:
    """Generate the GeneratedParams.h file content."""
    lines = [
        "// AUTO-GENERATED FILE - DO NOT EDIT",
        "// Generated by vst/generate_vst_params.py",
        "#pragma once",
        "",
        "#include <juce_audio_processors/juce_audio_processors.h>",
        "",
        "namespace GeneratedParams {",
        "",
    ]

    # Generate choice arrays for int_choice params
    for p in params:
        if p["type"] == "int_choice":
            lines.append(f"namespace {p['namespace']} {{")
            choices_str = ", ".join(f'"{c}"' for c in p["choices"])
            lines.append(f"inline const juce::StringArray choices = {{{choices_str}}};")
            lines.append(f"}}  // namespace {p['namespace']}")
            lines.append("")

    # Parameter IDs
    lines.append("namespace ParamIDs {")
    for p in params:
        const_name = p["param_id"].upper()
        lines.append(f'inline constexpr const char* {const_name} = "{p["param_id"]}";')
    lines.append("}  // namespace ParamIDs")
    lines.append("")

    # Params struct
    lines.append("struct Params {")
    type_map = {
        "choice": "juce::AudioParameterChoice",
        "int_choice": "juce::AudioParameterChoice",
        "int": "juce::AudioParameterInt",
        "float": "juce::AudioParameterFloat",
        "bool": "juce::AudioParameterBool",
        "string": "juce::AudioParameterChoice",  # Use choice for string (device name)
    }
    for p in params:
        cpp_type = type_map[p["type"]]
        lines.append(f"    {cpp_type}* {p['field_name']} = nullptr;")
    lines.append("};")
    lines.append("")

    lines.extend([
        "juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout(Params& params);",
        "",
        "}  // namespace GeneratedParams",
        "",
    ])

    return "\n".join(lines)


def generate_params_cpp(params: list[dict]) -> str:
    """Generate the GeneratedParams.cpp file content."""
    lines = [
        "// AUTO-GENERATED FILE - DO NOT EDIT",
        "// Generated by vst/generate_vst_params.py",
        '#include "GeneratedParams.h"',
        "",
        "namespace GeneratedParams {",
        "",
        "juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout(Params& params) {",
        "    juce::AudioProcessorValueTreeState::ParameterLayout layout;",
        "",
    ]

    for i, p in enumerate(params):
        var = f"p{i}"
        param_id = p["param_id"]

        if p["type"] == "int_choice":
            lines.append(
                f"    auto {var} = std::make_unique<juce::AudioParameterChoice>(\n"
                f'        juce::ParameterID("{param_id}", 1),\n'
                f'        "{p["label"]}",\n'
                f"        {p['namespace']}::choices,\n"
                f"        {p['default']});"
            )
        elif p["type"] == "int":
            lines.append(
                f"    auto {var} = std::make_unique<juce::AudioParameterInt>(\n"
                f'        juce::ParameterID("{param_id}", 1),\n'
                f'        "{p["label"]}",\n'
                f"        {p['min']}, {p['max']}, {p.get('default', p['min'])});"
            )
        elif p["type"] == "float":
            default = p.get("default", p["min"])
            lines.append(
                f"    auto {var} = std::make_unique<juce::AudioParameterFloat>(\n"
                f'        juce::ParameterID("{param_id}", 1),\n'
                f'        "{p["label"]}",\n'
                f"        {p['min']}F, {p['max']}F, {default}F);"
            )
        elif p["type"] == "bool":
            default = "true" if p.get("default") else "false"
            lines.append(
                f"    auto {var} = std::make_unique<juce::AudioParameterBool>(\n"
                f'        juce::ParameterID("{param_id}", 1),\n'
                f'        "{p["label"]}",\n'
                f"        {default});"
            )
        elif p["type"] == "string":
            # For string params (like midi_device_name), use a choice with placeholder
            # The actual choices will be populated at runtime
            lines.append(
                f"    auto {var} = std::make_unique<juce::AudioParameterChoice>(\n"
                f'        juce::ParameterID("{param_id}", 1),\n'
                f'        "{p["label"]}",\n'
                f'        juce::StringArray{{"(none)"}},\n'
                f"        0);"
            )

        lines.append(f"    params.{p['field_name']} = {var}.get();")
        lines.append(f"    layout.add(std::move({var}));")
        lines.append("")

    lines.extend([
        "    return layout;",
        "}",
        "",
        "}  // namespace GeneratedParams",
        "",
    ])

    return "\n".join(lines)


# =============================================================================
# JSON Blob C++ Code Generation (GeneratedAdditionalSettings.h/cpp)
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
        base_type = get_base_type(annotation)
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
        elif hasattr(base_type, "model_fields"):
            # Nested model
            collect_variant_types(base_type, collected, variant_signatures)


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


def generate_additional_settings_header(additional_settings_class: type[BaseModel]) -> str:
    """Generate GeneratedAdditionalSettings.h"""
    lines = [
        "// AUTO-GENERATED FILE - DO NOT EDIT",
        "// Generated by vst/generate_vst_params.py",
        "#pragma once",
        "",
        "#include <string>",
        "#include <variant>",
        "#include <map>",
        "#include <vector>",
        "#include <json.hpp>",
        "",
        "namespace GeneratedAdditionalSettings {",
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

    # Collect all types from AdditionalSettings
    collected = {}
    variant_signatures = {}
    collect_variant_types(additional_settings_class, collected, variant_signatures)

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

    # Main Settings struct
    lines.append("struct Settings {")
    for field_name in additional_settings_class.model_fields:
        annotation = additional_settings_class.__annotations__[field_name]
        base_type = get_base_type(annotation)
        union_types = get_union_types(annotation)

        if union_types:
            cpp_type = collected[field_name]["variant_name"]
        else:
            cpp_type = python_type_to_cpp(base_type, field_name)

        if cpp_type:
            lines.append(f"    {cpp_type} {field_name};")
    lines.append("")
    lines.append("    static Settings defaults();")
    lines.append("};")
    lines.append("")

    # Function declarations
    lines.extend([
        "// JSON serialization",
        "std::string toJson(const Settings& settings);",
        "Settings fromJson(const std::string& json);",
        "bool isValidJson(const std::string& json);",
        "",
        "// nlohmann JSON conversion declarations",
        "void to_json(nlohmann::json& j, const Settings& s);",
        "void from_json(const nlohmann::json& j, Settings& s);",
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
        "}  // namespace GeneratedAdditionalSettings",
        "",
    ])

    return "\n".join(lines)


def generate_additional_settings_cpp(additional_settings_class: type[BaseModel]) -> str:
    """Generate GeneratedAdditionalSettings.cpp"""
    lines = [
        "// AUTO-GENERATED FILE - DO NOT EDIT",
        "// Generated by vst/generate_vst_params.py",
        '#include "GeneratedAdditionalSettings.h"',
        "",
        "namespace GeneratedAdditionalSettings {",
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
    collect_variant_types(additional_settings_class, collected, variant_signatures)

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

    # Settings to_json/from_json
    lines.append("void to_json(nlohmann::json& j, const Settings& s) {")
    for field_name in additional_settings_class.model_fields:
        lines.append(f'    j["{field_name}"] = s.{field_name};')
    lines.append("}")
    lines.append("")

    lines.append("void from_json(const nlohmann::json& j, Settings& s) {")
    for field_name in additional_settings_class.model_fields:
        lines.append(f'    j.at("{field_name}").get_to(s.{field_name});')
    lines.append("}")
    lines.append("")

    # Public API functions
    lines.append("std::string toJson(const Settings& settings) {")
    lines.append("    nlohmann::json j = settings;")
    lines.append("    return j.dump();")
    lines.append("}")
    lines.append("")

    lines.append("Settings fromJson(const std::string& json) {")
    lines.append("    auto j = nlohmann::json::parse(json);")
    lines.append("    return j.get<Settings>();")
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
    lines.append("Settings Settings::defaults() {")
    lines.append("    return Settings{")

    # Import default values from Python
    from daemomnify.settings import DEFAULT_SETTINGS
    additional = DEFAULT_SETTINGS.additional_settings

    for field_name in additional_settings_class.model_fields:
        field_value = getattr(additional, field_name)
        cpp_init = python_value_to_cpp_init(field_value)
        lines.append(f"        .{field_name} = {cpp_init},")

    lines.append("    };")
    lines.append("}")
    lines.append("")

    lines.extend([
        "}  // namespace GeneratedAdditionalSettings",
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

    print("Collecting scalar parameters from DaemomnifySettings...")
    scalar_params = collect_scalar_params(DaemomnifySettings)
    print(f"  Found {len(scalar_params)} scalar parameters:")
    for p in scalar_params:
        print(f"    - {p['field_name']}: {p['type']}")

    # Generate scalar params files
    params_header = generate_params_header(scalar_params)
    params_cpp = generate_params_cpp(scalar_params)

    params_header_path = output_dir / "GeneratedParams.h"
    params_cpp_path = output_dir / "GeneratedParams.cpp"

    params_header_path.write_text(params_header)
    params_cpp_path.write_text(params_cpp)

    print(f"\nGenerated {params_header_path}")
    print(f"Generated {params_cpp_path}")

    # Generate additional settings files
    print("\nGenerating AdditionalSettings C++ code...")
    additional_header = generate_additional_settings_header(AdditionalSettings)
    additional_cpp = generate_additional_settings_cpp(AdditionalSettings)

    additional_header_path = output_dir / "GeneratedAdditionalSettings.h"
    additional_cpp_path = output_dir / "GeneratedAdditionalSettings.cpp"

    additional_header_path.write_text(additional_header)
    additional_cpp_path.write_text(additional_cpp)

    print(f"Generated {additional_header_path}")
    print(f"Generated {additional_cpp_path}")


if __name__ == "__main__":
    main()
