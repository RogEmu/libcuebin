# libcuebin

A C++20 library for parsing CUE sheet files and reading raw CD sectors. Designed for PSX emulation with lazy I/O -- BIN files are not preloaded into memory.

## Features

- Full CUE sheet parsing with all standard directives (FILE, TRACK, INDEX, PREGAP, POSTGAP, FLAGS, CATALOG, ISRC, TITLE, PERFORMER, SONGWRITER, REM, CDTEXTFILE)
- All track modes: AUDIO, CDG, MODE1/2048, MODE1/2352, MODE2/2336, MODE2/2352, CDI/2336, CDI/2352
- All file types: BINARY, MOTOROLA, AIFF, WAVE, MP3
- Lazy file handle management -- files are opened on first sector read
- Thread-safe sector reads (per-file mutex for concurrent CDDA playback)
- No exceptions in the public API -- all errors returned via `Result<T>`
- MSF (minute/second/frame) time type with constexpr LBA conversion

## Building

### Prerequisites

- C++20 compiler (GCC 10+, Clang 12+, MSVC 2019+)
- CMake 3.21+
- vcpkg (included as a git submodule)

### Setup

```bash
git clone --recursive <repo-url>
cd libcuebin
./vcpkg/bootstrap-vcpkg.sh   # or bootstrap-vcpkg.bat on Windows
```

### Build

```bash
cmake --preset default
cmake --build build
```

### Run Tests

```bash
ctest --test-dir build
```

## Usage

### Loading a CUE file

```cpp
#include "libcuebin/libcuebin.hpp"

auto result = cuebin::Disc::fromCue("game.cue");
if (!result) {
    // Handle error
    std::cerr << result.error().message << std::endl;
    return;
}

auto& disc = *result;
```

### Querying tracks

```cpp
std::cout << "Tracks: " << disc.trackCount() << std::endl;

for (const auto& track : disc.tracks()) {
    std::cout << "Track " << (int)track.number()
              << " - " << (track.isAudio() ? "Audio" : "Data")
              << " - " << track.lengthSectors() << " sectors"
              << std::endl;
}
```

### Reading sectors

```cpp
// Read by LBA
auto sector = disc.readSector(0);
if (sector) {
    // sector->data is std::array<uint8_t, 2352>
    // sector->mode indicates the track mode
}

// Read by MSF address
auto sector2 = disc.readSector(cuebin::MSF(0, 2, 0));

// Read multiple sectors
auto sectors = disc.readSectors(100, 16);
```

### Disc metadata

```cpp
if (auto title = disc.title()) {
    std::cout << "Title: " << *title << std::endl;
}
if (auto performer = disc.performer()) {
    std::cout << "Performer: " << *performer << std::endl;
}
```

### MSF conversions

```cpp
using cuebin::MSF;

constexpr auto msf = MSF(1, 2, 3);
constexpr int32_t lba = msf.toLba();             // 4653
constexpr auto back = MSF::fromLba(4653);         // 01:02:03
constexpr auto phys = MSF::toPhysicalMsf(0);      // 00:02:00 (adds 150-frame pregap)

auto parsed = MSF::parse("01:02:03");  // Returns Result<MSF>
std::string str = msf.toString();      // "01:02:03"
```

## Architecture

The library is split into two layers:

1. **CUE Parsing Layer** (`CueParser`, `CueSheet`) -- Parses CUE text into pure-data structures. No file I/O beyond reading the CUE text itself.

2. **Disc Abstraction Layer** (`Disc`, `Track`, `SectorData`) -- Wraps parsed CUE data, resolves LBA positions, manages lazy file handles, and provides the emulator-facing API.

## Error Handling

All public API methods return `Result<T>` instead of throwing exceptions. Check `.ok()` or use `operator bool()` before accessing the value:

```cpp
auto result = cuebin::Disc::fromCue("game.cue");
if (!result) {
    auto& err = result.error();
    // err.code    -- ErrorCode enum
    // err.message -- human-readable description
}
```

## License

MIT
