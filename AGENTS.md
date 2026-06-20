# AGENTS

This repository contains Linamp, a Qt 6 music player for Linux with file playback, native CD playback, and a retro Winamp-style UI.

## Project Focus

- Keep changes small and localized. Prefer fixing the owning module instead of layering new glue around it.
- Preserve the current stack: Qt 6 Widgets and Multimedia in C++, with Python used only for BT and Spotify sources unless the task explicitly requires broader Python work.
- Do not invent new build, test, or release workflows. Follow the commands and constraints already documented in this repository.

## Repository Layout

- `src/audiosource-base/`: common audio source interfaces and spectrum capture support.
- `src/audiosource-coordinator/`: coordinates which audio source is active.
- `src/audiosourcefile/`: file playback and metadata handling.
- `src/audiosourcecdnative/`: native C++ CD playback logic.
- `src/audiosourcepython/`: embedded Python bridge used by BT and Spotify sources.
- `src/view-player/`: playback controls, spectrum, and player-facing widgets.
- `src/view-playlist/`: playlist, file browser, and playlist parsing.
- `src/view-basewindow/`: desktop and embedded window shells.
- `src/view-menu/`: menu UI.
- `src/shared/`: shared utilities such as scaling, FFT, sliders, and system audio control.
- `python/`: Python requirements and supporting code for BT and Spotify playback.
- `styles/` and `assets/`: QSS styling and UI assets.

## Build And Run

Use the repository root for all commands.

```bash
cmake CMakeLists.txt
./setup.sh
make
./start.sh
```

Notes:

- `./setup.sh` prepares the Python environment needed for BT and Spotify playback.
- Run the app with `./start.sh` when BT or Spotify functionality matters. Running the binary directly outside the virtual environment can crash those sources.
- Qt Creator is supported, but if you need BT or Spotify playback there, configure it to launch `start.sh` instead of the built executable directly.

## Architecture Notes

- The `AudioSource` abstraction is the primary extension point for playback backends. Keep source-specific behavior inside the relevant source implementation.
- `AudioSourceCoordinator` owns source switching behavior. If a change affects how playback sources are activated, routed, or synchronized, start there.
- UI behavior should stay in the `view-*` modules. Shared non-UI helpers belong in `src/shared/`.
- Desktop and embedded behavior are separated in the base window layer. Respect the existing split instead of adding scattered platform checks.

## Change Workflow For Agents

- Read the nearest owning implementation before editing. Avoid broad repo sweeps when a local code path is enough.
- Prefer minimal edits that preserve existing public APIs unless the task requires an API change.
- Update documentation only when behavior, setup, or workflow actually changes.

## Validation

- For behavior changes, prefer the narrowest practical validation first.
- In this repository, the default validation path is:

```bash
make
./start.sh
```

- If a task touches Qt/C++ code and the user wants a review or a pre-commit check, use the available `qt-cpp-review` skill.
- There is no dedicated automated test suite documented in this repository. Do not claim test coverage that was not run.

## Known Constraints

- Mouse interaction in the file browser and playlist has a known limitation; click-and-hold may be required on non-touch systems.
- CD playback uses the native C++ backend and requires libcdio-paranoia and libdiscid system packages described in `README.md`.
- Packaging exists through the Debian files and README instructions, but release packaging should stay out of scope unless the task explicitly asks for it.