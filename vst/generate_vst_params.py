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
    path: list[str] | None = None,
    group_name: str | None = None,
    group_label: str | None = None,
) -> dict:
    """
    Recursively collect all VST parameters from a pydantic model.

    Returns a hierarchical structure:
    {
        "group_name": str | None,  # None for root
        "group_label": str | None,
        "params": [...],  # Direct parameters in this group
        "children": [...]  # Child groups (for discriminated unions)
    }
    """
    if path is None:
        path = []

    result = {
        "group_name": group_name,
        "group_label": group_label,
        "params": [],
        "children": [],
    }

    for field_name, field_info in model_class.model_fields.items():
        # Skip the 'type' discriminator field
        if field_name == "type":
            continue

        annotation = model_class.__annotations__[field_name]
        vst_param = get_vst_annotation(annotation)

        # Build the path for this parameter
        current_path = path + [field_name]

        # Skip if marked with VSTSkip
        if isinstance(vst_param, vst_params.VSTSkip):
            continue

        # Check if this is a discriminated union (choice with nested types)
        union_info = get_discriminated_union_info(annotation)
        if union_info:
            type_values, labels, variant_types = union_info

            # Create a group for this discriminated union
            union_label = vst_param.label if isinstance(vst_param, vst_params.VSTChoice) and vst_param.label else field_name.replace("_", " ").title()
            union_group = {
                "group_name": field_name,
                "group_label": union_label,
                "params": [],
                "children": [],
            }

            # Add the type selector parameter to this group
            union_group["params"].append(
                {
                    "field_name": field_name,
                    "param_id": ".".join(current_path),
                    "label": "Type",  # Shorter label since group provides context
                    "type": "choice",
                    "choices": labels,
                    "values": type_values,
                    "namespace": snake_to_pascal(field_name) + "Choices",
                }
            )

            # Recursively collect params from each variant as child groups
            for variant_type, type_value, label in zip(variant_types, type_values, labels):
                variant_group = collect_params(
                    variant_type,
                    path=current_path + [type_value],
                    group_name=type_value,
                    group_label=label,
                )
                # Only add if the variant has params
                if variant_group["params"] or variant_group["children"]:
                    union_group["children"].append(variant_group)

            result["children"].append(union_group)
            continue

        # Handle simple annotated types - add directly to params
        if isinstance(vst_param, vst_params.VSTInt):
            result["params"].append(
                {
                    "field_name": field_name,
                    "param_id": ".".join(current_path),
                    "label": vst_param.label or field_name.replace("_", " ").title(),
                    "type": "int",
                    "min": vst_param.min,
                    "max": vst_param.max,
                    "default": vst_param.default,
                }
            )
        elif isinstance(vst_param, vst_params.VSTFloat):
            result["params"].append(
                {
                    "field_name": field_name,
                    "param_id": ".".join(current_path),
                    "label": vst_param.label or field_name.replace("_", " ").title(),
                    "type": "float",
                    "min": vst_param.min,
                    "max": vst_param.max,
                    "default": vst_param.default,
                }
            )
        elif isinstance(vst_param, vst_params.VSTBool):
            result["params"].append(
                {
                    "field_name": field_name,
                    "param_id": ".".join(current_path),
                    "label": vst_param.label or field_name.replace("_", " ").title(),
                    "type": "bool",
                    "default": vst_param.default,
                }
            )
        elif isinstance(vst_param, vst_params.VSTIntChoice):
            choices = [str(i) for i in range(vst_param.min, vst_param.max + 1)]
            result["params"].append(
                {
                    "field_name": field_name,
                    "param_id": ".".join(current_path),
                    "label": vst_param.label or field_name.replace("_", " ").title(),
                    "type": "int_choice",
                    "choices": choices,
                    "namespace": snake_to_pascal(field_name) + "Choices",
                    "default": vst_param.default - vst_param.min,
                }
            )
        elif isinstance(vst_param, vst_params.VSTString):
            print(f"  Warning: Skipping string field {'.'.join(current_path)} (not supported yet)")
        elif isinstance(vst_param, vst_params.VSTChordQualityMap):
            # Expand to one int parameter per chord quality
            for quality in ChordQuality:
                quality_name = quality.name.lower()
                label_prefix = vst_param.label_prefix or field_name.replace("_", " ").title()
                result["params"].append(
                    {
                        "field_name": f"{field_name}_{quality_name}",
                        "param_id": ".".join(current_path + [quality_name]),
                        "label": f"{label_prefix} {quality.name.replace('_', ' ').title()}",
                        "type": "int",
                        "min": vst_param.min,
                        "max": vst_param.max,
                        "default": 0,
                    }
                )

    return result


def flatten_params(tree: dict) -> list[dict]:
    """Flatten the hierarchical tree into a list of all params (for header generation)."""
    params = list(tree["params"])
    for child in tree["children"]:
        params.extend(flatten_params(child))
    return params


def generate_params_struct(tree: dict, indent: str = "") -> list[str]:
    """Generate a nested struct for cached parameter pointers."""
    lines = []

    type_map = {
        "choice": "juce::AudioParameterChoice",
        "int_choice": "juce::AudioParameterChoice",
        "int": "juce::AudioParameterInt",
        "float": "juce::AudioParameterFloat",
        "bool": "juce::AudioParameterBool",
    }

    # Generate fields for direct params
    for p in tree["params"]:
        cpp_type = type_map[p["type"]]
        lines.append(f"{indent}{cpp_type}* {p['field_name']} = nullptr;")

    # Generate nested structs for children
    for child in tree["children"]:
        struct_name = snake_to_pascal(child["group_name"])
        lines.append(f"{indent}struct {struct_name}Params {{")
        lines.extend(generate_params_struct(child, indent + "    "))
        lines.append(f"{indent}}} {child['group_name']};")

    return lines




def generate_chord_qualities_namespace() -> list[str]:
    """Generate the ChordQualities namespace with count and names from ChordQuality enum."""
    lines = [
        "// Chord quality definitions from Python ChordQuality enum",
        "namespace ChordQualities {",
        f"inline constexpr int NUM_QUALITIES = {len(ChordQuality)};",
    ]

    # Generate nice names array
    names = [q.value.nice_name for q in ChordQuality]
    names_str = ", ".join(f'"{name}"' for name in names)
    lines.append(f"inline const char* NAMES[NUM_QUALITIES] = {{{names_str}}};")

    # Generate enum names array (for param IDs etc)
    enum_names = [q.name for q in ChordQuality]
    enum_names_str = ", ".join(f'"{name}"' for name in enum_names)
    lines.append(f"inline const char* ENUM_NAMES[NUM_QUALITIES] = {{{enum_names_str}}};")

    lines.append("}  // namespace ChordQualities")
    lines.append("")

    return lines


def generate_header(tree: dict, params: list[dict]) -> str:
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
    ]

    # Add chord qualities namespace
    lines.extend(generate_chord_qualities_namespace())

    # Generate choice arrays
    for p in params:
        if p["type"] == "choice":
            lines.append(f"namespace {p['namespace']} {{")
            labels_str = ", ".join(f'"{c}"' for c in p["choices"])
            lines.append(f"inline const juce::StringArray labels = {{{labels_str}}};")
            if "values" in p:
                values_str = ", ".join(f'"{v}"' for v in p["values"])
                lines.append(f"inline const juce::StringArray values = {{{values_str}}};")
            lines.append(f"}}  // namespace {p['namespace']}")
            lines.append("")
        elif p["type"] == "int_choice":
            lines.append(f"namespace {p['namespace']} {{")
            choices_str = ", ".join(f'"{c}"' for c in p["choices"])
            lines.append(f"inline const juce::StringArray choices = {{{choices_str}}};")
            lines.append(f"}}  // namespace {p['namespace']}")
            lines.append("")

    # Generate Params struct
    lines.append("// Cached parameter pointers")
    lines.append("struct Params {")
    lines.extend(generate_params_struct(tree, "    "))
    lines.append("};")
    lines.append("")

    lines.extend(
        [
            "// Create the parameter layout and populate the Params struct",
            "juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout(Params& params);",
            "",
            "}  // namespace GeneratedParams",
            "",
        ]
    )

    return "\n".join(lines)


def generate_param_creation(p: dict, var_name: str, indent: str = "    ") -> list[str]:
    """Generate code to create a parameter, assign its pointer, and return lines."""
    lines = []
    param_id = p["param_id"]

    if p["type"] == "choice":
        lines.append(
            f"{indent}auto {var_name} = std::make_unique<juce::AudioParameterChoice>(\n"
            f'{indent}    juce::ParameterID("{param_id}", 1),\n'
            f'{indent}    "{p["label"]}",\n'
            f"{indent}    {p['namespace']}::labels,\n"
            f"{indent}    0);"
        )
    elif p["type"] == "int_choice":
        lines.append(
            f"{indent}auto {var_name} = std::make_unique<juce::AudioParameterChoice>(\n"
            f'{indent}    juce::ParameterID("{param_id}", 1),\n'
            f'{indent}    "{p["label"]}",\n'
            f"{indent}    {p['namespace']}::choices,\n"
            f"{indent}    {p['default']});"
        )
    elif p["type"] == "int":
        lines.append(
            f"{indent}auto {var_name} = std::make_unique<juce::AudioParameterInt>(\n"
            f'{indent}    juce::ParameterID("{param_id}", 1),\n'
            f'{indent}    "{p["label"]}",\n'
            f"{indent}    {p['min']}, {p['max']}, {p.get('default', p['min'])});"
        )
    elif p["type"] == "float":
        default = p.get("default", p["min"])
        lines.append(
            f"{indent}auto {var_name} = std::make_unique<juce::AudioParameterFloat>(\n"
            f'{indent}    juce::ParameterID("{param_id}", 1),\n'
            f'{indent}    "{p["label"]}",\n'
            f"{indent}    {p['min']}F, {p['max']}F, {default}F);"
        )
    elif p["type"] == "bool":
        default = "true" if p.get("default") else "false"
        lines.append(
            f"{indent}auto {var_name} = std::make_unique<juce::AudioParameterBool>(\n"
            f'{indent}    juce::ParameterID("{param_id}", 1),\n'
            f'{indent}    "{p["label"]}",\n'
            f"{indent}    {default});"
        )

    return lines


def generate_group_code(
    tree: dict, var_name: str, params_path: str, param_counter: list[int], indent: str = "    "
) -> list[str]:
    """Recursively generate code to build a parameter group and assign pointers."""
    lines = []

    group_id = tree["group_name"] if tree["group_name"] else "root"
    group_label = tree["group_label"] or "Parameters"

    lines.append(
        f'{indent}auto {var_name} = std::make_unique<juce::AudioProcessorParameterGroup>("{group_id}", "{group_label}", "|");'
    )

    # Add direct parameters
    for p in tree["params"]:
        pvar = f"p{param_counter[0]}"
        param_counter[0] += 1
        lines.extend(generate_param_creation(p, pvar, indent))
        lines.append(f"{indent}{params_path}.{p['field_name']} = {pvar}.get();")
        lines.append(f"{indent}{var_name}->addChild(std::move({pvar}));")

    # Recursively add child groups
    for i, child in enumerate(tree["children"]):
        child_var = f"{var_name}_child{i}"
        child_params_path = f"{params_path}.{child['group_name']}"
        lines.append("")
        lines.extend(generate_group_code(child, child_var, child_params_path, param_counter, indent))
        lines.append(f"{indent}{var_name}->addChild(std::move({child_var}));")

    return lines


def generate_cpp(tree: dict) -> str:
    """Generate the .cpp file content with hierarchical parameter groups."""
    lines = [
        "// AUTO-GENERATED FILE - DO NOT EDIT",
        "// Generated by generate_vst_params.py from DaemomnifySettings",
        '#include "GeneratedParams.h"',
        "",
        "namespace GeneratedParams {",
        "",
        "juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout(Params& params) {",
        "    juce::AudioProcessorValueTreeState::ParameterLayout layout;",
        "",
    ]

    param_counter = [0]  # Use list to allow mutation in nested calls

    # Add root-level params directly
    for p in tree["params"]:
        pvar = f"p{param_counter[0]}"
        param_counter[0] += 1
        lines.extend(generate_param_creation(p, pvar, "    "))
        lines.append(f"    params.{p['field_name']} = {pvar}.get();")
        lines.append(f"    layout.add(std::move({pvar}));")
        lines.append("")

    # Add child groups
    for i, child in enumerate(tree["children"]):
        var_name = f"group{i}"
        params_path = f"params.{child['group_name']}"
        lines.append("")
        lines.extend(generate_group_code(child, var_name, params_path, param_counter, "    "))
        lines.append(f"    layout.add(std::move({var_name}));")
        lines.append("")

    lines.extend(
        [
            "",
            "    return layout;",
            "}",
            "",
            "}  // namespace GeneratedParams",
            "",
        ]
    )

    return "\n".join(lines)


def print_tree(tree: dict, indent: int = 0) -> None:
    """Pretty print the parameter tree."""
    prefix = "  " * indent
    if tree["group_name"]:
        print(f"{prefix}[{tree['group_label']}] ({tree['group_name']})")
    else:
        print(f"{prefix}(root)")

    for p in tree["params"]:
        print(f"{prefix}  - {p['param_id']}: {p['type']}")

    for child in tree["children"]:
        print_tree(child, indent + 1)


def main():
    output_dir = Path(__file__).parent

    print("Collecting parameters from DaemomnifySettings...")
    tree = collect_params(DaemomnifySettings)

    print("\nParameter hierarchy:")
    print_tree(tree)

    # Flatten for header generation (needs choice arrays)
    flat_params = flatten_params(tree)
    print(f"\nTotal parameters: {len(flat_params)}")

    # Generate files
    header_content = generate_header(tree, flat_params)
    cpp_content = generate_cpp(tree)

    header_path = output_dir / "GeneratedParams.h"
    cpp_path = output_dir / "GeneratedParams.cpp"

    header_path.write_text(header_content)
    cpp_path.write_text(cpp_content)

    print(f"\nGenerated {header_path}")
    print(f"Generated {cpp_path}")


if __name__ == "__main__":
    main()
