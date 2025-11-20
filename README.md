# frotzl - Frotz Launcher

Simple game launcher for Frotz that scans the local `games` directory and launches `sfrotz` when a game is selected.

## Usage

```bash
# Launch in windowed mode
./frotzl

# Launch with on-screen keyboard enabled (-k flag passed to sfrotz)
./frotzl -k

# Launch in fullscreen mode (-F flag passed to sfrotz)
./frotzl -F

# Launch with both keyboard and fullscreen
./frotzl -k -F
```

## Requirements

- SDL2
- SDL2_ttf
- DejaVu Sans Mono font (or similar system font)
- `sfrotz` binary in the same directory
- `games/` directory containing Z-machine game files

## Supported Game Formats

- .z3, .z4, .z5, .z8 (Z-machine story files)
- .zblorb, .zlb (Blorb files)
- .dat (Generic data files)

## Controls

- **UP/DOWN**: Navigate game list
- **ENTER/SPACE**: Launch selected game
- **ESC**: Quit launcher

## Building

```bash
make
```

## Notes

- The `-k` flag enables the on-screen keyboard in sfrotz
- The `-F` flag launches games in fullscreen mode
- Launches sfrotz from the current directory (./sfrotz)
- Flags can be combined (e.g., `-k -F`)
