# HypeClip AI

**Never miss a moment.** Automatic gaming highlight detection & broadcast-style
instant replay for OBS Studio on Windows.

HypeClip AI listens to your mic and game audio (and, optionally, your screen),
scores the excitement of every moment in real time, and automatically saves a clip
through OBS's native Replay Buffer when something hype happens — then can cut to a
slow-motion instant replay, all with **zero configuration**.

> **Status:** this repository is a complete, buildable engineering scaffold. The
> audio highlight engine, scoring/confidence brain, replay-buffer + clip pipeline,
> metadata, storage, and Qt dock UI are implemented in full. The optional GPU
> computer-vision detectors and the cinematic replay shader effects ship as wired
> interfaces with framework code + documented stubs (they need ML assets / tuning).
> The audio path works on its own out of the box. See [`docs/SPEC.md`](docs/SPEC.md).

## Features

- Automatic hype detection from **mic voice** (shouting, screaming, laughter, vocal
  spikes) and **game audio** (sustained combat, victory stingers) — training-free,
  self-calibrating.
- **Multi-layer confidence engine**: audio · voice · vision · event history · momentum,
  with cross-modal agreement bonuses and single-signal damping (one cough is ignored).
- **Momentum system** that escalates Kill → Kill Streak → Multi-Kill → Ace.
- Native **Replay Buffer** integration: auto-start, auto-save, configurable
  20 s-before / 10 s-after clips.
- **Auto titles & tags**, file organisation by Game/Date, JSON index.
- **Instant Replay Director**: plugin-owned scene, slow-motion ramp, style presets
  (Esports / Hype / Cinematic / Streamer / Retro), auto-return to live.
- **Best-of Top 5/10/25** reel playlist at stream end.
- Modern dark **dock UI**: live hype meter, clip-worthiness gauge, highlights list,
  Beginner/Creator settings.
- Robust **failsafes**: anti-spam spacing, duplicate guard, no replay loops, storage
  retention + size cap.

## Build (Windows x64)

Uses the standard [`obs-plugintemplate`](https://github.com/obsproject/obs-plugintemplate)
flow. With OBS deps + Qt6 available:

```powershell
cmake --preset windows-x64
cmake --build --preset windows-x64 --config Release
```

Then run `installer/installer.iss` through Inno Setup, or copy
`hypeclip-ai.dll` + the `data` folder into your OBS plugin directory.

> Before a release build, copy the upstream `obs-plugintemplate` `cmake/` tree over
> `cmake/common/bootstrap.cmake` (the shim only exists so host test builds configure
> without the full template).

## Run the unit tests (any platform, no OBS needed)

```bash
cmake -S . -B build-test -DHYPECLIP_BUILD_TESTS=ON
cmake --build build-test
ctest --test-dir build-test --output-on-failure
```

The tests cover feature extraction, voice-spike detection, momentum escalation,
confidence fusion and metadata generation. (Verified: 19/19 checks pass.)

## Layout

See [`docs/SPEC.md`](docs/SPEC.md) §12 for the full source map and architecture.

## License

GPL-2.0 (matching OBS Studio). See headers.
