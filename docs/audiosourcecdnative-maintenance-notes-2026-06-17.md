# audiosourcecdnative maintenance notes (2026-06-17)

## Scope of this session
This session focused on native CD playback behavior in audiosourcecdnative, especially startup reliability, underrun behavior, UI responsiveness during transport changes, eject handling, and a residual post-transition micro-snippet issue.

## Environment and operational constraints
- Target environment was Linux on a low-memory DietPi/Raspberry Pi setup.
- Build constraint: use serial `make` only. Parallel builds (notably `make -j4`) can hit OOM.
- CD playback path depends on launching through `start.sh` (virtual environment active) when Python CD integration is relevant.

## User-visible issues observed
- Initial symptom: starting playback produced no audio.
- Frequent underruns and drive spin-up/spin-down behavior during playback.
- Track time/progress reporting was incorrect on following tracks.
- Stop/Next/Previous and some seek operations could freeze UI.
- No clear loading feedback while seek/rebuffer occurred.
- Stop behavior expectation: return to start of current track (not forced disc position 0).
- Eject UX issues: missing "EJECTING...", occasional "EJECT FAILED", and blocking UI during eject prep.
- Persistent issue after many fixes: very small stale snippet heard on stop/seek/track change.

## Major implementation decisions
- Keep source-specific behavior in audiosourcecdnative and transport internals in CDNativePlaybackEngine.
- Treat discontinuities (stop/seek/track jump/eject prep) as non-gapless transitions and reset transport explicitly.
- Prefer non-blocking control paths to avoid UI stalls:
  - Reader stop can be requested without synchronous wait in hot paths.
  - Transport startup is retried asynchronously while buffering.
- Add explicit buffering signal path so UI can render LOADING state for CD transitions.
- Keep deferred slider seek behavior CD-only in coordinator/view wiring.

## Changes that materially improved behavior

### 1) QIODevice semantics for pull-mode sink startup/underrun
Files:
- src/audiosourcecdnative/cdpcmiodevice.h
- src/audiosourcecdnative/cdpcmiodevice.cpp

What changed:
- Implemented `seek(0)` as a tolerated no-op for live stream restarts.
- Overrode `atEnd()` to always return false for live stream semantics.
- Implemented `bytesAvailable()` backed by ring buffer size.
- Added queued `notifyReadyRead()` helper and invoked it after writes.
- Added short bounded wait in `readData()` to reduce immediate underrun churn.

Why it matters:
- Prevents sink from misclassifying temporary underrun as end-of-stream/idle and improves startup consistency.

### 2) Transport startup/prefill and non-blocking sequencing
Files:
- src/audiosourcecdnative/cdnativeplaybackengine.h
- src/audiosourcecdnative/cdnativeplaybackengine.cpp

What changed:
- Added pre-roll/target buffering thresholds before activating sink.
- Added `startTransportWhenReady()` retry loop with timer-based backoff.
- Added stream generation invalidation to discard stale reader data paths logically.
- Added non-blocking `stopReaderLoop(false)` option for latency-sensitive transitions.

Why it matters:
- Reduces audible instability and UI blocking during discontinuity transitions.

### 3) Position/time model correctness
Files:
- src/audiosourcecdnative/audiosourcecdnative.cpp
- src/audiosourcecdnative/cdnativeplaybackengine.cpp

What changed:
- Engine position now derives from sink processed time (`processedUSecs`) anchored at transition start.
- Source layer maps absolute disc position to track-relative UI position/duration.

Why it matters:
- Fixes drift/mismatch where subsequent track timing was wrong from user perspective.

### 4) Eject flow hardening and async prep
Files:
- src/audiosourcecdnative/audiosourcecdnative.h
- src/audiosourcecdnative/audiosourcecdnative.cpp
- src/audiosourcecdnative/cdnativeplaybackengine.h
- src/audiosourcecdnative/cdnativeplaybackengine.cpp
- src/audiosourcecdnative/cdnativediscservice.cpp

What changed:
- Added explicit eject preparation flow in engine with completion signal.
- UI/source now displays EJECTING state and finalizes on completion callback.
- Added no-disc guard before eject attempt.
- Disc service eject sequence now attempts unlock and stop before eject ioctl.

Why it matters:
- Reduces eject failures, avoids blocking the UI thread during transport teardown, improves user feedback.

### 5) CD-only deferred seek UX
Files:
- src/view-player/playerview.h
- src/view-player/playerview.cpp
- src/audiosource-coordinator/audiosourcecoordinator.cpp

What changed:
- Added deferred seek mode in player view (preview while dragging, commit on release).
- Enabled deferred mode only when active source label is CD.

Why it matters:
- Avoids churn from repeated seek commands during drag on high-latency CD transport.

### 6) Output-side transition muting around sink restart
File:
- src/audiosourcecdnative/cdnativeplaybackengine.cpp

What changed:
- On discontinuity entry points, mark transition mute pending.
- New sink starts at volume 0 during transition.
- Delayed unmute (120 ms) after transport restart.

Why it matters:
- Mitigation targeted at residual stale snippet likely queued in or around backend output path.

## Changes that did not solve the residual snippet
- Input-side sector discard after discontinuity did not produce measurable improvement.
- That path has been removed to keep code simpler and avoid false confidence.

Current status of this issue:
- Residual micro-snippet may still be backend queue/timing related around sink restart.
- Existing mitigation is transition mute + delayed unmute.

## Key gotchas for future maintenance
- `QMediaPlayer` top-level source is not representative of backend flush details; inspect backend behavior or validate empirically.
- Non-blocking reader stop can leave a short window where old data is still in flight; generation checks and post-stop ring clears are important.
- Do not assume input discard solves output queue artifacts.
- Keep UI signaling (`bufferingChanged`) symmetric: always clear pending buffering state on early exits/cancel paths.
- Any change to seek/stop semantics should be tested against:
  - stop-to-current-track-start expectation
  - next/previous responsiveness
  - slider drag behavior in CD mode
  - eject sequencing while playing and while paused

## Practical validation checklist
1. Build with serial `make`.
2. Run via `start.sh` when CD path is involved.
3. Verify startup audio begins reliably.
4. Verify no UI freeze on stop/next/previous/seek.
5. Verify LOADING message appears during buffering transitions and clears afterward.
6. Verify stop returns to start of current track.
7. Verify eject path shows EJECTING and avoids false EJECT FAILED in normal case.
8. Listen for residual snippet on stop/seek/track change.

## Suggested next technical step (if snippet persists)
- Replace fixed-time unmute with buffer-gated unmute:
  - Keep sink muted through transition.
  - Unmute only after a minimum quantity of fresh post-discontinuity PCM has been confirmed queued.
  - Optionally ramp volume up over a short window (for example 100 to 200 ms) to avoid edge clicks.

## Files most central to this behavior
- src/audiosourcecdnative/cdnativeplaybackengine.cpp
- src/audiosourcecdnative/cdnativeplaybackengine.h
- src/audiosourcecdnative/cdpcmiodevice.cpp
- src/audiosourcecdnative/cdpcmiodevice.h
- src/audiosourcecdnative/audiosourcecdnative.cpp
- src/audiosourcecdnative/cdnativediscservice.cpp
- src/view-player/playerview.cpp
- src/audiosource-coordinator/audiosourcecoordinator.cpp

---

## Session update (2026-06-19)

### Scope of this session
Follow-up work on native CD playback and closely related UI/coordinator behavior: end-of-disc handling, source-switch metadata, track navigation, shuffle, position display during jumps, eject UX when stopped, and volume/balance slider responsiveness.

### Issues fixed

#### 1) End-of-disc did not stop or reset
**Symptom:** When the disc finished (repeat off), playback stayed in the playing state with no audio; position remained at the end of the last track.

**Root cause:** `CDNativePlaybackEngine::updatePositionTick()` stopped internal transport on disc end but never notified the source layer or reset to track 1.

**Fix:**
- Added `finishNaturalPlayback()` and `playbackFinished` signal in the engine.
- On natural disc end (repeat off): reset to disc start (track 1, position 0), emit `activeTrackChanged(0)` and `positionChanged(0)`.
- `AudioSourceCDNative` connects `playbackFinished` to emit `StoppedState` and stop the spectrum.
- Manual stop semantics unchanged: still resets to **start of current track**.
- Repeat-on still loops the disc inside the engine.

Files: `cdnativeplaybackengine.h/.cpp`, `audiosourcecdnative.cpp`

#### 2) Stale file metadata when switching to CD source
**Symptom:** Switching from file playback to CD left the previous track info on screen.

**Root cause:** `AudioSourceCDNative::activate()` called `pollDisc()` but did not refresh display state when a disc was already loaded or absent (poll only reacts to insert/eject transitions).

**Fix:** After `pollDisc()`, `activate()` now refreshes UI state:
- Disc loaded: force metadata refresh (`m_lastTrackIndex = -1`), emit position 0 and track metadata.
- No disc: emit `"NO DISC"` metadata with cleared position/duration.

Files: `audiosourcecdnative.cpp`

#### 3) Next on last track repeated instead of wrapping
**Symptom:** Pressing Next on the last track stayed on that track.

**Root cause:** `CDNativePlaybackEngine::next()` used `qMin(currentIndex + 1, size - 1)`.

**Fix:** Sequential and shuffle Next both wrap to the first entry (track index 0, or first shuffle-order entry) when on the last track/order position.

Files: `cdnativeplaybackengine.cpp`

#### 4) Shuffle did not work during playback
**Symptom:** With shuffle enabled, tracks still advanced linearly; early broken implementation also stopped playback before track end and caused seek glitches on resume.

**Root cause:** Shuffle only affected manual Next/Previous via `m_playOrder`. The reader still streamed the full disc linearly. A first fix attempt used metadata `durationMs` for track-end detection, which MusicBrainz can override to a shorter value than the disc's LBA length, causing premature stops and transport restart loops.

**Fix:**
- **Shuffle off:** unchanged gapless linear disc read.
- **Shuffle on:** reader stops at current track's last LBA; non-gapless jump to next shuffle-order track via `advanceToNextShuffleTrack()`.
- Track boundaries and position math use **LBA-derived duration** (`lengthLba * 1000 / 75`), not metadata `durationMs`.
- Advance triggers when reader is finished **and** playback position reached track end (or buffer drained after progress).
- `setShuffleEnabled()` restarts transport when toggled mid-playback so read bounds update immediately.
- `currentTrackIndex()` returns `m_currentLogicalIndex` in shuffle mode.

Files: `cdnativeplaybackengine.h/.cpp`

#### 5) Progress bar flicker on track jump
**Symptom:** Next/shuffle jumps briefly showed random progress/time before correcting once playback started.

**Root cause:** Engine uses LBA-based absolute timeline; source layer converted to track-relative position using metadata `durationMs`. Signal order also emitted `positionChanged` before `activeTrackChanged`.

**Fix:**
- Source layer `trackStartMs` / `resolveTrackIndexForAbsolutePosition` now use LBA-based `trackPlaybackDurationMs()`.
- Position mapping uses `m_engine->currentTrackIndex()` as authoritative track index.
- Engine emits `activeTrackChanged` before `positionChanged` on seeks and shuffle advances.

Files: `audiosourcecdnative.cpp`, `cdnativeplaybackengine.cpp`

#### 6) "EJECTING..." not shown when ejecting while stopped
**Symptom:** Eject while stopped froze the UI briefly, then jumped to `"NO DISC"` without showing `"EJECTING..."`. Worked when playing.

**Root cause:** With no reader running, eject prep completed synchronously and `m_discService.eject()` blocked the UI thread before the message could repaint.

**Fix:**
- `finalizeEject()` runs blocking drive eject in `QtConcurrent`; completion handled in `completeEject()` on the main thread.
- `finishEjectPreparationWhenReady()` defers `ejectPreparationFinished` by one event-loop tick.
- Guard against duplicate eject while async eject is in flight.

Files: `audiosourcecdnative.h/.cpp`, `cdnativeplaybackengine.cpp`

### Related changes outside audiosourcecdnative (same session)

#### Volume/balance slider choppiness
**Symptom:** Dragging volume/balance sliders caused choppy UI and audio.

**Root cause:** Every slider step called synchronous ALSA mixer I/O on the UI thread.

**Fix in `SystemAudioControl`:**
- Throttled async ALSA apply (~32 ms) on a worker thread.
- `QMutex` serializes all mixer access (apply + poll + shutdown).
- Volume/balance snapshots passed into worker to avoid torn reads.

Files: `src/shared/systemaudiocontrol.h/.cpp`

#### Volume/balance overlay during drag
**Symptom:** Debounced overlay messages (150 ms) appeared briefly and seemingly at random during drag.

**Fix:** Drag-aware overlay in coordinator + player view:
- Drag start: `setPersistentMessage()` (no auto-hide timer).
- During drag: update text live.
- Drag end: normal 500 ms timed message, then restore track info.
- New signals: `volumeDragStarted/Finished`, `balanceDragStarted/Finished`.

Files: `playerview.h/.cpp`, `audiosourcecoordinator.h/.cpp`

### Updated gotchas
- Use **LBA-derived duration** for all playback timing in the engine and for absolute→relative position mapping in the source layer. Metadata `durationMs` is for display only and may differ from disc length.
- In shuffle mode, do not infer active track from absolute position alone; use `m_currentLogicalIndex`.
- End-of-disc (repeat off) resets to **track 1**; manual stop resets to **current track start**. Do not conflate the two.
- Eject must stay async through drive `ioctl`; do not call blocking eject on the UI thread even when the reader is already stopped.
- `startTransportWhenReady()` accepts partial preroll when the reader has finished but could not fill the full threshold (near track end).

### Updated validation checklist
1. Build with serial `make`.
2. Run via `start.sh` for runtime CD testing.
3. Play disc to end (repeat off): stops, track 1, position 0, spectrum off.
4. Play again after disc end starts from track 1.
5. Repeat on loops disc without fake playing state.
6. Switch file → CD: shows CD track info or `"NO DISC"` immediately.
7. Next on last track wraps to track 1.
8. Shuffle: tracks play in random order, each plays fully, no early stop or seek loop glitches.
9. Next/shuffle jump: progress bar stays at 0 (no flicker) until playback advances.
10. Eject while stopped and while playing: `"EJECTING..."` visible, UI responsive.
11. Volume/balance drag: smooth UI/audio; overlay stays visible and updates during drag.

