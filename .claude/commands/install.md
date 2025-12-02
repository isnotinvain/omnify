---
description: Install a package using uv
---

Install the package using `uv add`.

**IMPORTANT**: Use `uv add` to install dependencies. Do NOT manually edit `pyproject.toml` to add dependencies unless there's a very specific need to do so.

Examples:
- `uv add package-name` - Add to main dependencies
- `uv add --dev package-name` - Add to dev dependencies
- `uv add --group groupname package-name` - Add to a dependency group
- `uv sync` - Sync environment with pyproject.toml (removes unused packages)
