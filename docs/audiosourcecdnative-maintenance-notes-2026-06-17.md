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
