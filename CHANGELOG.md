# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.1.0] - 2026-02-19

### Added

- CI/CD pipeline with GitHub Actions for Linux and Windows.
- Code coverage support with gcov/gcovr (source files only).

### Changed

- Test step in CI now runs the test binary directly instead of using CTest.

## [0.0.9] - 2026-02-04

### Added

- `Disc::readSectors()` method for batch sector reading.
- `Disc::leadOutLba()` to retrieve the lead-out position.
- `Disc::findTrack()` to look up a track by LBA.

### Fixed

- Sector reads near track boundaries returning incorrect data due to off-by-one in file offset calculation.

## [0.0.8] - 2026-01-22

### Added

- `Track::isAudio()` and `Track::isData()` convenience methods.
- `Track::endLba()` computed accessor.
- `SectorData` struct with raw 2352-byte sector storage.

### Changed

- `Disc::readSector()` now returns `Result<SectorData>` instead of raw byte vectors.

## [0.0.7] - 2026-01-10

### Added

- `Disc` class with full disc image I/O via `Disc::fromCue()`.
- Sector reading by LBA and MSF address.
- Disc-level metadata accessors: `title()`, `performer()`, `catalog()`.
- Unit tests for `Disc` (`testDisc.cpp`).

### Changed

- `Track` constructor now takes file index and byte offset for I/O support.

## [0.0.6] - 2025-12-28

### Added

- `Track` class encapsulating track number, mode, sector size, LBA range, pregap/postgap, indices, and metadata.
- `sectorSizeForMode()` utility to retrieve the sector size for a given `TrackMode`.

### Fixed

- `CueParser` not correctly handling tracks with no explicit INDEX 00.

## [0.0.5] - 2025-12-18

### Added

- PREGAP and POSTGAP directive parsing in `CueParser`.
- ISRC, TITLE, PERFORMER, SONGWRITER metadata parsing for tracks.
- `TrackFlag` enum with `DCP`, `CH4`, `PRE`, `SCMS` flags.
- FLAGS directive parsing.

### Changed

- `CueTrack` struct now holds optional pregap, postgap, flags, and metadata fields.

## [0.0.4] - 2025-12-12

### Added

- Multi-file CUE sheet support (multiple FILE directives).
- Multi-track parsing with correct index association.
- Test data files: `multiFile.cue`, `multiTrack.cue`.

### Fixed

- Parser failing on quoted filenames containing spaces.

## [0.0.3] - 2025-12-08

### Added

- `CueParser::parseFile()` for parsing CUE sheets from disk.
- `CueParser::parseString()` for parsing CUE sheets from strings.
- `CueSheet`, `CueFile`, `CueTrack`, `CueIndex` data structures.
- `FileType` enum: Binary, Motorola, Aiff, Wave, MP3.
- `TrackMode` enum: Audio, CDG, Mode1/2 variants, CDI variants.
- Basic unit tests for the CUE parser (`testCueParser.cpp`).
- Test data file: `singleTrack.cue`.

## [0.0.2] - 2025-12-04

### Added

- `MSF` struct with minute/second/frame fields.
- LBA conversion: `MSF::toLba()`, `MSF::fromLba()`, `MSF::toPhysicalMsf()`.
- `MSF::parse()` for parsing "MM:SS:FF" strings.
- `MSF::toString()` for formatting.
- Comparison operators on `MSF`.
- Unit tests for MSF (`testMsf.cpp`).

## [0.0.1] - 2025-12-01

### Added

- Initial project scaffolding with CMake and vcpkg.
- `Error` and `Result<T>` types for error handling.
- `ErrorCode` enum covering parse, I/O, and disc errors.
- spdlog dependency for logging.
- Google Test dependency for unit testing.
- Project README and LICENSE.
