# daemomnify
A daemon that "omnify"'s any midi source, allowing you to play any midi instrument (virtual or not) like it was an omnichord,
strumming on the mod touch plate or modwheel of your keyboard and using pads to select chord quality and notes to select the root of chords.
Outputs separate midi channels for chords vs strums.

WIP, has many configurable input / output modes, and the plan is to wrap this all up in a VST as well.

## Setup

Requires Python 3.14+ and [uv](https://docs.astral.sh/uv/).

```bash
# Clone the repo
git clone https://github.com/isnotinvain/daemomnify.git
cd daemomnify

# Install dependencies
uv sync

# Install the package in editable mode
uv pip install -e .
```

## Running

```bash
uv run python -m daemomnify
```

---

[daemomnify](https://github.com/isnotinvain/daemomnify) © 2025 by [Alexander Levenson](https://www.isnotinvain.com/) is licensed under [CC BY-NC-SA 4.0](https://creativecommons.org/licenses/by-nc-sa/4.0/) ![CC](https://mirrors.creativecommons.org/presskit/icons/cc.svg) ![BY](https://mirrors.creativecommons.org/presskit/icons/by.svg) ![NC](https://mirrors.creativecommons.org/presskit/icons/nc.svg) ![SA](https://mirrors.creativecommons.org/presskit/icons/sa.svg)
