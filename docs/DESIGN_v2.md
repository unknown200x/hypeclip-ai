# HypeClip AI — v2 Redesign (User Control · Transparency · Performance)

This pass refactors the plugin around five priorities — **user control, reliability,
performance, transparency, streamer experience** — addressing the 12 requested
priority areas. Below: the settings architecture, clip database schema, OBS
integration flow, the rule engine, the UI layout, and how each priority is met.

---

## 1. Architecture overview

```
 Master switch (Config.masterEnabled)
        │  OFF  ──────────────► everything stopped, no threads, idle only
        ▼  ON
 PipelineController  ── creates one AudioCapture per selected source ──┐
   honors every FeatureFlag                                            │
        │                                                              ▼
   AudioCapture(mic/game) ─► detectors ─► MetricsHub (live snapshot) ──┤
   VisionAnalyzer (opt) ────────────────► MetricsHub                   │
        │ Contributions (EventBus)                                     │
        ▼                                                              │
   ScoreEngine ── AI ConfidenceEngine ──┐                              │
              ── RuleEngine (IF/THEN)  ──┼─► decision + ScoreReason     │
              ── SessionLearner (FP)   ──┘     │ HighlightEvent          │
                                               ▼                         │
   ClipManager ─► ReplayBufferController (native or custom) ─► clip file │
        │  title+tags, clips.json (DB), publishClip                      │
        ├─► InstantReplayDirector (clean INSTANT REPLAY, user scene)     │
        └─► StorageManager (retention, best-of)                          │
                                                                         │
   UI Dock (Qt) reads MetricsHub + Config + clips ◄──────────────────────┘
```

The **MetricsHub** is the single source of truth: detectors write to it, the meters,
rule engine and AI scorer read from it. This is what makes the UI live and the
"why did this clip happen" transparency possible.

---

## 2. Settings architecture (persisted via OBS `config_t`, `hypeclip.ini`)

| Group | Keys | Purpose |
|---|---|---|
| `general` | `master_enabled`, `mode` | Priority #1 master switch; Beginner/Creator |
| `features` | `auto_clip`, `instant_replay`, `visual`, `audio`, `mic`, `game_audio`, `ai_scoring`, `eos_highlights`, `social_export` | Priority #1 independent toggles |
| `scoring` | `threshold`, `min_gap_ms`, `fp_learning` | AI threshold, anti-spam, Priority #9 |
| `replay` | `mode`, `pre`, `post`, `duration_ms`, `enter_ms`, `return_ms`, `show_label`, `scene`, `enter_tr`, `return_tr` | Priority #2/#3/#4 |
| `mic` | `voice_act`, `excitement`, `peak`, `scream`, `laughter`, `reaction` | Priority #5 thresholds |
| `game` | `sensitivity`, `event_sens`, `spike`, `clustering`, `cooldown_ms` | Priority #6 tuning |
| `audio` | `mic_sources`, `game_sources` (newline-joined) | Priority #5/#6 multi-device |
| `rules` | `data` (serialized) | Priority #7 rule set |
| `storage` | `retention_days`, `clip_dir` | cleanup + location |

Rules serialize compactly as one line per rule:
`name|enabled|join|action|cooldownMs|metric,op,value;metric,op,value;…`

---

## 3. Clip database schema (`<clipDir>/clips.json`)

One JSON object per clip (written by `ClipManager::persistIndex`). Designed so it
can be lifted into SQLite unchanged (column == key):

| Field | Type | Notes |
|---|---|---|
| `id` | string | unique (creation epoch ms) |
| `title` | string | auto-generated |
| `file` | string | absolute path |
| `game` | string | detected/blank |
| `type` | string | Kill/Ace/Victory/Reaction/… |
| `confidence` | int | 0–100 final score |
| `time` | int64 | epoch ms |
| `duration` | int | seconds (0 = native RB length) |
| `favorite` | bool | Priority #10 |
| `rule` | string | firing rule name ("" = AI) |
| `audio`,`voice`,`vision`,`final` | int | Priority #8 score breakdown |

Suggested SQLite DDL (future migration):
```sql
CREATE TABLE clips(
  id TEXT PRIMARY KEY, title TEXT, file TEXT, game TEXT, type TEXT,
  confidence INT, time INTEGER, duration INT, favorite INT,
  rule TEXT, audio INT, voice INT, vision INT, final INT);
CREATE INDEX idx_clips_time ON clips(time);
CREATE INDEX idx_clips_fav  ON clips(favorite);
```
"Best of Day/Week/Month" = `SELECT … WHERE time >= ? ORDER BY confidence DESC LIMIT N`.

---

## 4. OBS integration flow

| Action | OBS API |
|---|---|
| Audio tap (per source) | `obs_source_add_audio_capture_callback`, `obs_get_output_source` |
| Replay save | `obs_frontend_replay_buffer_active/start/save`, `obs_frontend_get_last_replay` |
| Replay scene | `obs_frontend_get_scene_names`, `obs_frontend_set_current_scene` |
| Transitions | `obs_frontend_get_transitions`, `obs_frontend_set_current_transition`, `obs_frontend_set_transition_duration` |
| Minimal replay scene | `obs_scene_create` + `ffmpeg_source` + a single centered `text_*` source |
| Dock | `obs_frontend_add_dock_by_id` |
| Source enumeration | `obs_enum_sources` + `OBS_SOURCE_AUDIO` |

All scene/transition mutations are marshalled to the UI thread (`obs_queue_task`);
the audio callback only downmixes + pushes (never blocks the stream).

---

## 5. Rule engine (Priority #7)

`Rule = { name, enabled, join(ALL/ANY), [conditions], action(Clip/Replay), cooldownMs }`
`Condition = { metric, comparator(>,>=,<,<=), value }`

Metrics: Mic Excitement/Volume/Peak, Scream, Laughter, Reaction, Game Spike/
Intensity/Volume, Vision Score, Kill Feed, Momentum, AI Confidence.

`RuleEngine::evaluate(snapshot, now)` returns the first satisfied, off-cooldown rule.
A satisfied rule **wins over** the AI scorer (explicit user intent). `bestProgress`
drives the Clip-Worthiness meter even before a rule trips. With **zero rules**, the
AI scoring engine decides (if enabled).

## 6. Transparency (Priority #8) & false positives (Priority #9)

Every `HighlightEvent` carries a `ScoreReason` (audio/voice/vision/momentum/final +
firing rule + plain-English explanation), persisted to the DB and shown in the
Highlights tab. The `SessionLearner` adapts a per-session noise floor and rejects
short, isolated, single-modality transients (coughs, pops, keyboard, pings, music
peaks) unless corroborated or sustained.

---

## 7. UI (Priority #12 — professional, dark, OBS-native)

Dock = header + `QTabWidget`:

- **Dashboard** — master switch, 9 feature toggles, live Hype / Clip-Worthiness /
  Mic meters, numeric scores, replay-buffer banner.
- **Audio** — checkable mic + game source lists, 6 mic threshold sliders, 4 game
  tuning sliders + cooldown, live level/score readouts.
- **Rules** — visual IF/THEN editor (list + condition table + action), example presets.
- **Replay** — native-vs-custom length, scene picker, enter/return transition +
  duration pickers, hold duration, single-label toggle.
- **Highlights** — timeline with reason, confidence, Open/Favorite/Export/Delete.
- **Settings** — mode, AI threshold, spacing, retention, FP-learning, clip folder, reset.

All controls write straight to `Config` and call `PipelineController::reconfigure()`
for instant hot-apply.

---

## 8. Performance (Priority #11)

- Master OFF / feature OFF ⇒ the corresponding threads are not even created.
- Audio: ~20 ms windows, time-domain + Goertzel (no FFT), allocation-free steady
  state, lock-free SPSC hand-off; analysis sleeps when the ring is empty.
- Vision: ≤5 fps on its own worker, off by default; render tap only does a
  non-blocking stage copy.
- UI: meters poll `MetricsHub` at ~8–10 Hz (cheap mutex + a few painted bars).
- Targets: <1% idle, <2% monitoring CPU, no added frame drops, no leaks (RAII
  + single owned scene/sources released on shutdown).

---

## 9. Bonus features (Priority list) — status

Implemented/wired: live Hype Meter, live Clip-Worthiness, favorites, end-of-stream
best-of (Top N), per-game folders, transparency dashboard, false-positive learning.
Architected with toggles/interfaces (need external SDKs/assets to finish): Stream
Deck, Discord auto-share, Twitch markers, TikTok/Shorts vertical export, facecam/
VTuber reaction emphasis, multi-PC, excitement heatmap, Best-of Day/Week/Month
(DB query above). `features.autoSocialExport` gates the export pipeline when added.
