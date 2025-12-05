#!/usr/bin/env python3
"""
Generate C++ parameter definitions from DaemomnifySettings pydantic model.

This script introspects the DaemomnifySettings class and generates:
- GeneratedParams.h: Parameter IDs, choice arrays
- GeneratedParams.cpp: createParameterLayout() implementation

It uses VSTParam annotations from vst_params.py to determine how fields
should be exposed as VST parameters.
"""

from pathlib import Path
from typing import Annotated, Union, get_args, get_origin

from pydantic import BaseModel

from daemomnify import vst_params
from daemomnify.chord_quality import ChordQuality
from daemomnify.settings import DaemomnifySettings


def snake_to_camel(name: str) -> str:
    """Convert snake_case to camelCase."""
    components = name.split("_")
    return components[0] + "".join(x.title() for x in components[1:])


def snake_to_pascal(name: str) -> str:
    """Convert snake_case to PascalCase."""
    return "".join(x.title() for x in name.split("_"))


def snake_to_screaming(name: str) -> str:
    """Convert snake_case to SCREAMING_SNAKE_CASE."""
    return name.upper()


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


def get_variant_label(model_class: type) -> str | None:
    """Get the vst_label from a model class if present."""
    if hasattr(model_class, "vst_label"):
        return model_class.vst_label()
    return None


def get_discriminated_union_info(annotation) -> tuple[list[str], list[str], list[type]] | None:
    """Get (type_values, labels, types) from a discriminated union."""
    union_types = get_union_types(annotation)
    if not union_types:
        return None

    type_values = []  # The actual discriminator values (used for param names)
    labels = []  # The display labels
    valid_types = []
    for t in union_types:
        if hasattr(t, "model_fields") and "type" in t.model_fields:
            type_field = t.model_fields["type"]
            if type_field.default:
                type_values.append(type_field.default)
                # Use VSTVariant label if present, otherwise fall back to type value
                variant_label = get_variant_label(t)
                labels.append(variant_label if variant_label else type_field.default)
                valid_types.append(t)

    return (type_values, labels, valid_types) if type_values else None


def collect_params(
    model_class: type[BaseModel],
    prefix: str = "",
    parent_variant: str | None = None,
) -> list[dict]:
    """
    Recursively collect all VST parameters from a pydantic model.

    Returns a list of parameter definitions.
    """
    params = []

    for field_name, field_info in model_class.model_fields.items():
        # Skip the 'type' discriminator field
        if field_name == "type":
            continue

        annotation = model_class.__annotations__[field_name]
        vst_param = get_vst_annotation(annotation)

        # Build the full parameter name
        if prefix:
            full_name = f"{prefix}_{field_name}"
        else:
            full_name = field_name

        # Skip if marked with VSTSkip
        if isinstance(vst_param, vst_params.VSTSkip):
            continue

        # Check if this is a discriminated union (choice with nested types)
        union_info = get_discriminated_union_info(annotation)
        if union_info:
            type_values, labels, variant_types = union_info

            # Add the type selector parameter
            label = vst_param.label if isinstance(vst_param, vst_params.VSTChoice) and vst_param.label else field_name.replace("_", " ").title()
            params.append(
                {
                    "name": full_name,
                    "param_id": snake_to_camel(full_name),
                    "label": label,
                    "type": "choice",
                    "choices": labels,  # Use labels for display
                    "values": type_values,  # Programmatic values for serialization
                    "namespace": snake_to_pascal(full_name) + "Choices",
                }
            )

            # Recursively collect params from each variant
            for variant_type, type_value in zip(variant_types, type_values):
                variant_params = collect_params(
                    variant_type,
                    prefix=f"{full_name}_{type_value}",  # Use type_value for param names
                    parent_variant=type_value,
                )
                params.extend(variant_params)

            continue

        # Handle simple annotated types
        if isinstance(vst_param, vst_params.VSTInt):
            params.append(
                {
                    "name": full_name,
                    "param_id": snake_to_camel(full_name),
                    "label": vst_param.label or field_name.replace("_", " ").title(),
                    "type": "int",
                    "min": vst_param.min,
                    "max": vst_param.max,
                    "default": vst_param.default,
                    "parent_variant": parent_variant,
                }
            )
        elif isinstance(vst_param, vst_params.VSTFloat):
            params.append(
                {
                    "name": full_name,
                    "param_id": snake_to_camel(full_name),
                    "label": vst_param.label or field_name.replace("_", " ").title(),
                    "type": "float",
                    "min": vst_param.min,
                    "max": vst_param.max,
                    "default": vst_param.default,
                    "parent_variant": parent_variant,
                }
            )
        elif isinstance(vst_param, vst_params.VSTBool):
            params.append(
                {
                    "name": full_name,
                    "param_id": snake_to_camel(full_name),
                    "label": vst_param.label or field_name.replace("_", " ").title(),
                    "type": "bool",
                    "default": vst_param.default,
                    "parent_variant": parent_variant,
                }
            )
        elif isinstance(vst_param, vst_params.VSTIntChoice):
            # Generate choices as strings from min to max
            choices = [str(i) for i in range(vst_param.min, vst_param.max + 1)]
            params.append(
                {
                    "name": full_name,
                    "param_id": snake_to_camel(full_name),
                    "label": vst_param.label or field_name.replace("_", " ").title(),
                    "type": "int_choice",
                    "choices": choices,
                    "namespace": snake_to_pascal(full_name) + "Choices",
                    "default": vst_param.default - vst_param.min,  # Convert to index
                    "parent_variant": parent_variant,
                }
            )
        elif isinstance(vst_param, vst_params.VSTString):
            # For now, skip strings as JUCE doesn't have a native string parameter
            # Could implement as a special case later
            print(f"  Warning: Skipping string field {full_name} (not supported yet)")
        elif isinstance(vst_param, vst_params.VSTChordQualityMap):
            # Expand to one int parameter per chord quality
            for quality in ChordQuality:
                quality_name = quality.name.lower()
                param_name = f"{full_name}_{quality_name}"
                label_prefix = vst_param.label_prefix or field_name.replace("_", " ").title()
                params.append(
                    {
                        "name": param_name,
                        "param_id": snake_to_camel(param_name),
                        "label": f"{label_prefix} {quality.name.replace('_', ' ').title()}",
                        "type": "int",
                        "min": vst_param.min,
                        "max": vst_param.max,
                        "default": 0,
                        "parent_variant": parent_variant,
                    }
                )

    return params


def generate_header(params: list[dict]) -> str:
    """Generate the .h file content."""
    lines = [
        "// AUTO-GENERATED FILE - DO NOT EDIT",
        "// Generated by generate_vst_params.py from DaemomnifySettings",
        "#pragma once",
        "",
        "#include <juce_audio_processors/juce_audio_processors.h>",
        "",
        "namespace GeneratedParams {",
        "",
        "// Parameter IDs",
        "namespace ParamIDs {",
    ]

    for p in params:
        lines.append(f'constexpr const char* {snake_to_screaming(p["name"])} = "{p["param_id"]}";')

    lines.extend(["", "} // namespace ParamIDs", ""])

    # Generate choice arrays
    for p in params:
        if p["type"] == "choice":
            lines.append(f"namespace {p['namespace']} {{")
            labels_str = ", ".join(f'"{c}"' for c in p["choices"])
            lines.append(f"inline const juce::StringArray labels = {{{labels_str}}};")
            if "values" in p:
                values_str = ", ".join(f'"{v}"' for v in p["values"])
                lines.append(f"inline const juce::StringArray values = {{{values_str}}};")
            lines.append(f"}} // namespace {p['namespace']}")
            lines.append("")
        elif p["type"] == "int_choice":
            lines.append(f"namespace {p['namespace']} {{")
            choices_str = ", ".join(f'"{c}"' for c in p["choices"])
            lines.append(f"inline const juce::StringArray choices = {{{choices_str}}};")
            lines.append(f"}} // namespace {p['namespace']}")
            lines.append("")

    lines.extend(
        [
            "// Create the parameter layout",
            "juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();",
            "",
            "} // namespace GeneratedParams",
            "",
        ]
    )

    return "\n".join(lines)


def generate_cpp(params: list[dict]) -> str:
    """Generate the .cpp file content."""
    lines = [
        "// AUTO-GENERATED FILE - DO NOT EDIT",
        "// Generated by generate_vst_params.py from DaemomnifySettings",
        '#include "GeneratedParams.h"',
        "",
        "namespace GeneratedParams {",
        "",
        "juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() {",
        "    juce::AudioProcessorValueTreeState::ParameterLayout layout;",
        "",
    ]

    for p in params:
        variant_comment = f" (when {p['parent_variant']})" if p.get("parent_variant") else ""

        if p["type"] == "choice":
            lines.append(f"    // {p['label']}")
            lines.append("    layout.add(std::make_unique<juce::AudioParameterChoice>(")
            lines.append(f"        juce::ParameterID(ParamIDs::{snake_to_screaming(p['name'])}, 1),")
            lines.append(f'        "{p["label"]}",')
            lines.append(f"        {p['namespace']}::labels,")
            lines.append("        0));")
            lines.append("")
        elif p["type"] == "int_choice":
            lines.append(f"    // {p['label']}{variant_comment}")
            lines.append("    layout.add(std::make_unique<juce::AudioParameterChoice>(")
            lines.append(f"        juce::ParameterID(ParamIDs::{snake_to_screaming(p['name'])}, 1),")
            lines.append(f'        "{p["label"]}",')
            lines.append(f"        {p['namespace']}::choices,")
            lines.append(f"        {p['default']}));")
            lines.append("")
        elif p["type"] == "int":
            lines.append(f"    // {p['label']}{variant_comment}")
            lines.append("    layout.add(std::make_unique<juce::AudioParameterInt>(")
            lines.append(f"        juce::ParameterID(ParamIDs::{snake_to_screaming(p['name'])}, 1),")
            lines.append(f'        "{p["label"]}",')
            lines.append(f"        {p['min']}, {p['max']}, {p.get('default', p['min'])}));")
            lines.append("")
        elif p["type"] == "float":
            lines.append(f"    // {p['label']}{variant_comment}")
            lines.append("    layout.add(std::make_unique<juce::AudioParameterFloat>(")
            lines.append(f"        juce::ParameterID(ParamIDs::{snake_to_screaming(p['name'])}, 1),")
            lines.append(f'        "{p["label"]}",')
            default = p.get("default", p["min"])
            lines.append(f"        {p['min']}f, {p['max']}f, {default}f));")
            lines.append("")
        elif p["type"] == "bool":
            lines.append(f"    // {p['label']}{variant_comment}")
            lines.append("    layout.add(std::make_unique<juce::AudioParameterBool>(")
            lines.append(f"        juce::ParameterID(ParamIDs::{snake_to_screaming(p['name'])}, 1),")
            lines.append(f'        "{p["label"]}",')
            default = "true" if p.get("default") else "false"
            lines.append(f"        {default}));")
            lines.append("")

    lines.extend(
        [
            "    return layout;",
            "}",
            "",
            "} // namespace GeneratedParams",
            "",
        ]
    )

    return "\n".join(lines)


def main():
    output_dir = Path(__file__).parent

    print("Collecting parameters from DaemomnifySettings...")
    params = collect_params(DaemomnifySettings)

    print(f"\nFound {len(params)} parameters:")
    for p in params:
        variant_info = f" (variant: {p['parent_variant']})" if p.get("parent_variant") else ""
        print(f"  {p['name']}: {p['type']}{variant_info}")

    # Generate files
    header_content = generate_header(params)
    cpp_content = generate_cpp(params)

    header_path = output_dir / "GeneratedParams.h"
    cpp_path = output_dir / "GeneratedParams.cpp"

    header_path.write_text(header_content)
    cpp_path.write_text(cpp_content)

    print(f"\nGenerated {header_path}")
    print(f"Generated {cpp_path}")
    print(f"\nTotal parameters: {len(params)}")


if __name__ == "__main__":
    main()
