# HypeClip AI — Engineering Specification

**Tagline:** *Never miss a moment.*
**Target:** OBS Studio 30.x / 31.x · Windows 10/11 · x64 · MSVC + Qt6
**Version:** 1.0.0

---

## 1. What this document is (and an honest scope note)

This is the engineering specification and architecture for **HypeClip AI**, an OBS
Studio plugin that automatically detects exciting gameplay moments and turns them
into clips and broadcast-style instant replays, with zero configuration required.

The accompanying source tree is a **complete, coherent, buildable scaffold**: every
module, interface, data type, build file, installer and test harness exists and is
wired together. The pieces that genuinely require shipped ML assets or weeks of
per-game tuning — the GPU computer-vision detectors and the cinematic replay shader
effects — are implemented as **clean interfaces with working framework code and
documented stubs**, not trained models. They are designed so the plugin is fully
functional on the **audio-only path out of the box**, and so enabling vision can
never destabilise that path. This is called out explicitly wherever it applies.

---

## 2. Design principles

1. **Reliability first.** Nothing the plugin does may drop a frame or interrupt a
   stream. The audio render callback only downmixes and pushes to a lock-free ring
   buffer; all analysis is on separate threads. Vision runs at a low cadence on its
   own worker and is off by default.
2. **Works out of the box.** Beginner mode ships sane defaults, auto-picks the mic
   and desktop-audio sources, and auto-starts the Replay Buffer. No AI training.
3. **Training-free accuracy.** Detectors use adaptive baselines that calibrate to
   the user's mic/room/game mix automatically, so "LET'S GOOOO" stands out from
   calm talking for anyone, on any setup.
4. **Cross-modal confidence.** A single gunshot or single cough is ignored. Real
   moments are corroborated across modalities (combat audio + vocal spike) and
   escalating momentum.
5. **Lightweight.** Time-domain features + a tiny Goertzel band bank — no FFT
   library, no per-frame allocation. Target < 2% average CPU, < 500 MB RAM.
6. **Modular & future-proof.** Detectors publish `Contribution`s onto an `EventBus`;
   new detectors (additional AI models, new modalities) plug in without touching the
   scoring brain.

---

## 3. High-level architecture

```
                    ┌────────────────────── OBS Studio process ──────────────────────┐
                    │                                                                 │
  Mic source ──▶ AudioCapture(Mic) ─┐         (OBS audio thread: downmix + ring push) │
 Game source ──▶ AudioCapture(Game)─┤                                                 │
                    │               ▼  analysis threads (per capture)                 │
                    │      FeatureExtractor → VoiceExcitement / GameAudio detectors    │
                    │               │ Contribution                                    │
  Program video ─▶ VisionAnalyzer ──┤ (optional, GPU worker, low fps)                 │
                    │               ▼                                                  │
                    │          ┌────────────┐  publish(Contribution)                  │
                    │          │  EventBus   │◀───────────────────────────────────────│
                    │          └────┬───────┘                                          │
                    │               ▼                                                  │
                    │        ┌──────────────┐  layers: audio/voice/vision/history/mom │
                    │        │ ScoreEngine   │── MomentumTracker + ConfidenceEngine    │
                    │        └──────┬───────┘                                          │
                    │  publish(HighlightEvent) when fused confidence ≥ threshold       │
                    │               ▼                                                  │
                    │        ┌──────────────┐   requestSave()    ┌──────────────────┐  │
                    │        │  ClipManager  │──────────────────▶│ ReplayBufferCtrl  │  │
                    │        └──────┬───────┘◀───saved path──────└──────────────────┘  │
                    │   title+tag → organise file → persist index                      │
                    │               │ publishClip(ClipRecord)                          │
                    │     ┌─────────┼────────────────┬───────────────────────┐         │
                    │     ▼         ▼                ▼                        ▼         │
                    │ StorageMgr  Qt Dock UI   InstantReplayDirector   (best-of reel)   │
                    └─────────────────────────────────────────────────────────────────┘
```

All inter-module communication is through value types (`Contribution`,
`HighlightEvent`, `ClipRecord`) on a thread-safe `EventBus`, so any module can be
unit-tested or replaced in isolation.

---

## 4. Module breakdown

### 4.1 Core (`src/core`)
| File | Responsibility |
|---|---|
| `Types.hpp` | POD value types + enums (`TriggerSource`, `HighlightType`, `Contribution`, `HighlightEvent`, `ClipRecord`). No OBS dependency → host-testable. |
| `Config` | Thread-safe settings, backed by an OBS `config_t`. Beginner defaults. |
| `EventBus` | Thread-safe pub/sub for contributions, highlights and clips. |
| `PipelineController` | Owns and wires every subsystem; `startup/shutdown/reconfigure`. |
| `Log.hpp` | `blog()` wrapper with stderr fallback for host builds. |

### 4.2 Audio (`src/audio`)
| File | Responsibility |
|---|---|
| `AudioCapture` | Taps an OBS source via `obs_source_add_audio_capture_callback`, downmixes to mono on the audio thread, pushes to a lock-free `RingBuffer`, and runs an analysis thread (≈20 ms windows). One instance for mic, one for game. |
| `AudioFeatures` | Per-frame features: RMS, peak, crest factor, zero-crossing rate, spectral flux, and a 3-band Goertzel bank (low / voice / high). O(N), no FFT. |
| `VoiceExcitementDetector` | Adaptive-baseline mic analysis → shouting, screaming, laughter, sustained celebration, vocal spikes. |
| `GameAudioDetector` | Adaptive-baseline game-audio analysis → sustained combat (transient runs), victory/round stingers. Ignores single transients. |

**Why Goertzel, not FFT:** we only need a handful of band energies, and the Goertzel
algorithm computes a single DFT bin in O(N) with no buffers or plan setup — perfect
for a real-time budget. The whole audio analysis path is allocation-free after warm-up.

### 4.3 Scoring (`src/scoring`)
| File | Responsibility |
|---|---|
| `ScoreEngine` | Sliding 4 s window of contributions, recency-weighted per-layer aggregation, threshold decision, failsafes, emits `HighlightEvent`. The brain. |
| `MomentumTracker` | Leaky-integrator momentum with a clustering amplifier; escalates `Kill → KillStreak → MultiKill → Ace`. |
| `ConfidenceEngine` | Fuses the 5 layers (audio, voice, vision, history, momentum). Cross-modal **agreement bonus**, single-modality **damping**, momentum multiplier. Output 0–100. |

**Scoring model (matches the requested rubric).** Detector base scores are tuned so
that, after fusion: a lone mic scream ≈ 45 but damped to ~27 if nothing corroborates
it; gunfight + sustained yelling crosses 70; an Ace or team wipe with reactions and a
victory sting saturates toward 100. The default clip threshold is **70**.

### 4.4 Replay & clips (`src/replay`)
| File | Responsibility |
|---|---|
| `ReplayBufferController` | Native `obs_frontend_replay_buffer_*`: detect/auto-start, save, retrieve last replay path, surface it via callback. |
| `ClipManager` | Pairs async replay saves with pending highlights (FIFO), titles + tags, organises files into `Game/Date/`, persists a JSON index, publishes `ClipRecord`, hands major events to the director. |
| `InstantReplayDirector` | Flagship: builds a plugin-owned "HypeClip Instant Replay" scene (ffmpeg media source + caption), applies a slow-mo speed ramp + style preset, cuts to it, and returns to live on `media_ended`. |

### 4.5 Metadata (`src/metadata`)
`AutoTitler` ("Triple Kill - 8:14 PM", "Insane Clutch - Valorant") and `AutoTagger`
(game, type, confidence bucket, modality tags, `instant-replay`). Pure logic, fully
unit-tested.

### 4.6 Storage (`src/storage`)
`StorageManager`: day-based retention, total-size flood guard (evict oldest), and
end-of-stream **Best-of Top 5/10/25** ranked by confidence, written as an `ffconcat`
playlist (optional `ffmpeg -f concat` render documented).

### 4.7 Vision (`src/vision`, optional, GPU)
`VisionAnalyzer` + `GameProfile`. Low-cadence (≈5 fps) GPU-readback worker, never on
the render thread. Game-agnostic heuristics (kill-feed churn, victory bursts) plus
data-driven per-game profiles (regions + optional ONNX model) for Valorant, Apex,
CS2, Fortnite, CoD, Overwatch 2, League, Marvel Rivals. **Status:** framework wired;
GPU stage-surface readback and ONNX/DirectML inference are the Windows-only modules to
finish — until present the loop idles, so enabling vision is always safe.

### 4.8 UI (`src/ui`, Qt6)
Dark, native-styled dockable `QTabWidget`: Dashboard (live hype meter + clip-worthiness
gauge + replay-buffer banner + clips-today), Highlights (live list, double-click to
open), Settings (Beginner/Creator, threshold, pre/post, sources, replay style, vision,
commentary, retention). Registered via `obs_frontend_add_dock_by_id` (persistent).

---

## 5. Data flow (clip lifecycle)

1. Audio threads emit `Contribution`s (scored, with energy + label).
2. `ScoreEngine` ingests them, updates momentum, aggregates recency-weighted layer
   scores, and fuses confidence each tick.
3. When `confidence ≥ threshold` **and** failsafes pass, it publishes a
   `HighlightEvent` (with `majorEvent` flag for replay-worthy moments).
4. `ClipManager` ensures the Replay Buffer is active, queues the event, and calls
   `requestSave()`.
5. OBS fires `REPLAY_BUFFER_SAVED`; `ReplayBufferController` reads the path and calls
   back into `ClipManager`, which pairs it with the oldest pending event.
6. `ClipManager` builds title + tags, moves the file into `Game/Date/`, persists the
   index, and publishes a `ClipRecord`.
7. `StorageManager` records it for best-of; the **UI** appends it live; if
   `majorEvent && enableInstantReplay`, the **InstantReplayDirector** plays it.

---

## 6. OBS integration points

| Need | OBS API |
|---|---|
| Module entry | `OBS_DECLARE_MODULE`, `obs_module_load/unload`, locale macros |
| Lifecycle timing | `obs_frontend_add_event_callback` → `FINISHED_LOADING` / `EXIT` / `*_STOPPED` |
| Audio tap | `obs_source_add_audio_capture_callback`, `obs_get_output_source` (channels), `audio_output_get_sample_rate/channels` |
| Replay buffer | `obs_frontend_replay_buffer_active/start/save`, `obs_frontend_get_last_replay` |
| Instant replay | `obs_scene_create`, `obs_source_create("ffmpeg_source"/text)`, `obs_frontend_set_current_scene`, `media_ended` signal, `obs_queue_task(OBS_TASK_UI,…)` |
| Vision frame tap | `obs_add_main_render_callback`, `gs_stage_texture` + `gs_stagesurface_map` |
| Dock | `obs_frontend_add_dock_by_id` (OBS 30+) |
| Source enumeration | `obs_enum_sources`, `obs_source_get_output_flags & OBS_SOURCE_AUDIO` |

Thread safety: the audio callback is real-time and does no allocation; all frontend
mutations are marshalled to the UI thread with `obs_queue_task`.

---

## 7. Performance plan

- **Budget:** < 2% average CPU, < 500 MB RAM, zero added frame drops.
- **Audio:** ~20 ms windows × 2 streams = ~100 analyses/s. Each is O(960) time-domain
  plus ~10 Goertzel evaluations. Allocation-free steady state. Worker sleeps 5 ms when
  the ring is empty.
- **Lock-free hand-off:** SPSC `RingBuffer` (power-of-two, masked) means the audio
  thread never blocks on the analysis thread.
- **Vision:** GPU downscale to 480-wide, analyse ≤5 fps on a worker; render-thread tap
  only issues a non-blocking stage copy. Off by default.
- **UI:** dashboard polls at ~8 Hz; meters are cheap custom paints.
- **Validation:** ship a `perf` harness that runs the audio path on recorded WAVs and
  reports per-window microseconds; CI fails if p99 regresses.

---

## 8. Failsafe systems

| Risk | Mitigation |
|---|---|
| False triggers | Cross-modal damping (single modality × 0.6), adaptive baselines, transient-*run* requirement for combat. |
| Spam clipping | `minClipGapMs` spacing in `ScoreEngine::passesFailsafes`, overridable only by a clearly bigger, different-type moment. |
| Duplicate clips | Same-type back-to-back guard within 2× gap. |
| Replay loops | `InstantReplayDirector` `compare_exchange` guard — never stacks replays. |
| Storage flooding | Retention by age + total-size cap eviction. |
| Pending-queue growth | `ClipManager` bounds the pending queue to 8. |
| Replay buffer off | Auto-start + UI banner + per-highlight `ensureActive()`. |
| Muted mic | Audio callback drops muted frames. |
| Audio desync / corruption | We never touch the encoder; clips come straight from OBS's own replay buffer. |

---

## 9. Build, dependencies, deployment

- **Build system:** CMake ≥ 3.28 following the official `obs-plugintemplate` layout
  (`buildspec.json`, `cmake/common`). `find_package(libobs / obs-frontend-api / Qt6)`.
- **Dependencies:** libobs, obs-frontend-api, Qt6 (Widgets/Core/Gui). Optional vision
  build adds ONNX Runtime (DirectML EP) — guarded by `HYPECLIP_ENABLE_VISION`.
- **Bootstrap:** copy the upstream template `cmake/` tree over `cmake/common/bootstrap.cmake`
  before producing release artifacts (the shim lets host/CI test builds configure
  without the full template).
- **CI:** GitHub Actions matrix (windows-2022) → configure/build/package; a separate
  host job builds `HYPECLIP_BUILD_TESTS=ON` and runs `ctest`.
- **Installer:** `installer/installer.iss` (Inno Setup) drops `hypeclip-ai.dll` and the
  `data/` folder into the user's OBS plugin directory and writes nothing else.
- **Packaging:** the template's `cpack`/`Package.ps1` produces a zip; the Inno script
  wraps it in a friendly Windows installer with a "Launch OBS" finish option.

---

## 10. Testing framework

- **Unit (host, no OBS):** `test/` builds the core/audio/scoring/metadata sources with
  `HYPECLIP_BUILD_TESTS` and exercises: feature extraction on synthesised tones/noise,
  voice-spike detection vs. baseline, momentum escalation, confidence fusion
  (single-modality damping & cross-modal bonus), and title/tag generation. A tiny
  assertion harness keeps it dependency-free; swap in doctest/Catch2 freely.
- **Integration (OBS):** scripted scenes + canned WAV/MKV inputs validate replay-buffer
  save→organise→index→UI and the instant-replay scene round-trip.
- **Performance:** the `perf` target measures per-window analysis time and memory.

---

## 11. Bonus / future expansion (roadmap)

Designed-for, not all shipped in v1:

- **Vertical export** (TikTok / YouTube Shorts): a post-processor that crops to 9:16
  around a facecam/action region and burns captions; hooks off `ClipRecord`.
- **AI highlight reels:** the best-of playlist + ffmpeg render is the v1 seed; v2 adds
  beat-synced transitions.
- **Facecam reaction emphasis & VTuber model reaction detection:** reuse the vision
  worker on the facecam source region.
- **Crowd-hype audio simulation & dynamic replay cameras:** OverlayEffect modules in
  the director.
- **Integrations:** Discord clip share (webhook on `ClipRecord`), Twitch markers
  (`CreateStreamMarker` on highlight), Stream Deck / Elgato actions (manual clip,
  toggle armed), multi-PC streaming (NDI-aware source selection).
- **Heat-map timeline of stream excitement & real-time hype meter:** the meter exists;
  the heat-map is a `StorageManager` aggregation over the session's confidence track.
- **Additional AI models:** any new detector just publishes `Contribution`s — the
  scoring brain needs no changes.

---

## 12. Source map

```
HypeClipAI/
├─ CMakeLists.txt          buildspec.json        README.md
├─ cmake/common/bootstrap.cmake
├─ data/locale/en-US.ini
├─ installer/installer.iss
├─ docs/SPEC.md            (this file)
├─ test/CMakeLists.txt     test/test_main.cpp
└─ src/
   ├─ plugin-main.cpp
   ├─ core/      Types.hpp Config.* EventBus.* PipelineController.* Log.hpp
   ├─ util/      RingBuffer.hpp
   ├─ audio/     AudioCapture.* AudioFeatures.* VoiceExcitementDetector.* GameAudioDetector.*
   ├─ scoring/   ScoreEngine.* MomentumTracker.* ConfidenceEngine.*
   ├─ replay/    ReplayBufferController.* ClipManager.* InstantReplayDirector.*
   ├─ metadata/  AutoTitler.* AutoTagger.*
   ├─ storage/   StorageManager.*
   ├─ vision/    VisionAnalyzer.* GameProfile.*
   └─ ui/        HypeClipDock.* DashboardTab.* HighlightsTab.* SettingsTab.* HypeMeter.*
```
