# SonosESP: Art + Polling Flow Reference

## What This Document Is

A precise reference for two critical user flows and all the subsystems they touch.
Created to understand why **CMD_NEXT works** but **CMD_PLAY_QUEUE_ITEM gets stuck** with art and DMA issues.

---

## Current State (as of 2026-03-21)

### What Works
- CMD_NEXT: art loads reliably
- CMD_PLAY_QUEUE_ITEM: art loads reliably (DMA threshold lowered + deadlock fixed)
- Art LRU cache: cache hits work at any DMA level (no TCP)
- Lyrics: `art_download_in_progress` flag prevents TLS race with art download
- WiFi screen: phone-style layout, SSID deduplication, spinner feedback, 30s timeout

### Previously Broken — Now Fixed
- **DMA depletion / art stuck in wait loop**: `ART_MIN_FREE_DMA` lowered from 40KB → 8KB. Structural steady-state DMA floor is ~19-26KB (16 TIME_WAIT PCBs × ~6KB); the old 40KB threshold was architecturally unreachable.
- **DMA wait deadlock**: `art_download_in_progress=false` during DMA wait let polling fire SOAPs → new PCBs created as fast as old expired → DMA stayed flat. Fixed: flag=true before DMA wait loop; brief warm windows (flag=false for 300ms) every 3s prevent SDIO clock-gate.
- **Pre-GET burst crash** (`ART_MIN_DMA_PRE_BURST=36000`): TCP slow-start cwnd 10-14 MSS → server burst up to 20KB before window-update ACK arrives. Pre-GET DMA check gates this.
- **SO_LINGER unavailable**: `CONFIG_LWIP_SO_LINGER` not compiled in pre-built framework — `lwip_setsockopt` returns -1 silently. Mitigation: `ART_MIN_DMA_PRE_BURST` guard + 5ms inter-chunk reads.

### Remaining Open
- **Session DMA depletion floor**: WiFi dynamic RX buffers (CONFIG_ESP_WIFI_DYNAMIC_RX_BUFFER_NUM=32 × 1.6KB) appear to not fully release after heavy downloads. Not fixable without recompiling framework. Mitigated by lowered `ART_MIN_FREE_DMA` + pre-burst guard.

---

## Architecture Overview

### Tasks

| Task Name | Priority | Core | Stack | Purpose |
|-----------|----------|------|-------|---------|
| `mainAppTask` | 1 | 1 | 16KB (SRAM) | LVGL render, UI updates, watchdog |
| `loopTask` | 1 | 1 | 8KB (SRAM) | Idle (just vTaskDelay) |
| `albumArtTask` | 0 | 0 | 20KB (PSRAM) | Download + decode art |
| `SonosNet` | 2 | 0 | 6KB | Process command queue (Next/Pause/Seek/etc.) |
| `SonosPoll` | 3 | 0 | 6KB | Polling GetPositionInfo every 300ms |
| `lyricsTask` | 1 | 0 | 8KB | Fetch HTTPS lyrics from lrclib.net |
| `clockBgTask` | 1 | 0 | 8KB | Clock screensaver background + weather |

### Mutexes

| Mutex | Taken By | Purpose |
|-------|----------|---------|
| `network_mutex` | art task, lyrics task, all sendSOAP calls | Serialize ALL WiFi TCP operations |
| `deviceMutex` | SonosNet, SonosPoll | Protect SonosDevice struct |
| `art_mutex` | art task, main UI, requestAlbumArt() | Protect pending_art_url / art_ready / last_art_url |

### Key Global Variables (`ui_globals.cpp` / `ui_common.h`)

```
volatile bool          art_download_in_progress  // MAIN GATE: suppresses pollingTask
volatile unsigned long last_art_download_end_ms   // Set after ALL art/clock downloads
volatile unsigned long last_transient_500_ms       // Set when Sonos returns HTTP 500
volatile unsigned long last_track_change_ms        // Set in requestAlbumArt() (disabled: SDIO_TRACK_CHANGE_SETTLE_MS=0)
volatile unsigned long last_network_end_ms         // Set after every network op
volatile unsigned long last_https_end_ms           // Set after every HTTPS op
volatile unsigned long last_queue_fetch_time       // Set after updateQueue() completes

String  pending_art_url    // URL that art task should download next
String  last_art_url       // URL of the art currently displayed
bool    art_ready          // true when scaled art is in art_buffer and ready to display
bool    art_show_placeholder // true when placeholder should show (no art, or error)
bool    art_abort_download   // Signal art task to abort mid-download (track changed)
bool    art_suppress_source_change // Used by CMD_PLAY_QUEUE_ITEM settle window
```

---

## Flow 1: CMD_NEXT (Press "Next" Button)

### Step-by-Step

```
[mainAppTask]  ev_next() callback fires
               → sonos.next()
               → 400ms debounce check
               → xQueueSend(commandQueue, CMD_NEXT)

[SonosNet]     dequeue CMD_NEXT
               art_download_in_progress = TRUE   ← suppress polling NOW
               sendSOAP("AVTransport", "Next")   ← acquires network_mutex
                  last_network_end_ms = millis()
               vTaskDelay(200ms)                  ← allow Sonos device to settle
               updateTrackInfo()                  ← GetPositionInfo SOAP
                  → onSonosUpdate() callback fires
                  → pending_art_url = new_track_art_url
                  last_network_end_ms = millis()

[mainAppTask]  updateUI() → requestAlbumArt(pending_art_url)
               → pending_art_url stored in art_mutex block
               → last_track_change_ms = millis()

[albumArtTask] loop: pending_art_url != last_art_url → download path
               → art_download_in_progress = FALSE  ← allow polling during sdioPreWait
               → sdioPreWait() (queue-poll cooldown only, ~0-3000ms)
               → art_download_in_progress = TRUE   ← DMA wait starts
               → DMA wait loop (up to 165s)
               → acquire network_mutex
               → HTTP download
               → decode + scale
               → http.end() + drain
               → last_art_download_end_ms = millis()
               → release network_mutex
               → art_ready = true (main UI renders on next cycle)
```

### TCP Connections Created (TIME_WAIT PCBs)

| SOAP/Request | TCP teardown | TIME_WAIT? |
|---|---|---|
| Next SOAP | FIN-close (SO_LINGER fails silently) | YES, ~6KB DMA, 120s |
| GetPositionInfo SOAP | FIN-close | YES, ~6KB DMA, 120s |
| Art HTTP download | FIN-close (art stays open during decode now) | Passive close → NO (server does TIME_WAIT) |

**CMD_NEXT creates 2 new TIME_WAIT PCBs ≈ 12KB DMA consumed.**

---

## Flow 2: CMD_PLAY_QUEUE_ITEM (Tap Queue Track)

### Step-by-Step

```
[mainAppTask]  ev_queue_item(trackNum) callback fires
               → sonos.playQueueItem(trackNum)
               → xQueueSend(commandQueue, CMD_PLAY_QUEUE_ITEM + trackNum)
               → lv_screen_load(scr_main)

[SonosNet]     dequeue CMD_PLAY_QUEUE_ITEM
               art_download_in_progress = TRUE   ← suppress polling NOW
               sendSOAP("AVTransport", "Seek", "<Target>N</Target>")  ← TIME_WAIT PCB #1
                  last_network_end_ms = millis()
               vTaskDelay(100ms)
               sendSOAP("AVTransport", "Play")   ← TIME_WAIT PCB #2
                  last_network_end_ms = millis()
               deviceMutex: dev->isPlaying = true
               vTaskDelay(200ms)
               updateTrackInfo()                 ← GetPositionInfo SOAP, TIME_WAIT PCB #3
                  → onSonosUpdate() → pending_art_url = new_track_art_url
                  last_network_end_ms = millis()

[mainAppTask]  updateUI() → requestAlbumArt(pending_art_url)
               → same as CMD_NEXT from here

[albumArtTask] same as CMD_NEXT from here
```

### TCP Connections Created (TIME_WAIT PCBs)

| SOAP/Request | TCP teardown | TIME_WAIT? |
|---|---|---|
| Seek SOAP | FIN-close | YES, ~6KB DMA, 120s |
| Play SOAP | FIN-close | YES, ~6KB DMA, 120s |
| GetPositionInfo SOAP | FIN-close | YES, ~6KB DMA, 120s |
| Art HTTP download | Passive close (new) | NO (server's TIME_WAIT) |

**CMD_PLAY_QUEUE_ITEM creates 3 new TIME_WAIT PCBs ≈ 18KB DMA consumed.**

If DMA was already at 20KB before CMD_PLAY_QUEUE_ITEM fires, adding 18KB = -DMA. This is the stuck state.

---

## Why CMD_NEXT Works But CMD_PLAY_QUEUE_ITEM Gets Stuck

**CMD_NEXT typically fires when DMA is still healthy** (user presses Next at the start of a session, DMA ~90KB). 2 new PCBs = 12KB consumed, leaving 78KB → well above ART_MIN_FREE_DMA (8KB, lowered from 40KB) → art downloads fine.

**CMD_PLAY_QUEUE_ITEM typically fires AFTER the user has been using the app** for a while:
- Many SOAPs have accumulated: 16 TIME_WAIT PCBs × 6KB = 96KB consumed
- DMA is already at 19-24KB before CMD fires
- 3 more SOAPs from CMD = +18KB needed but DMA is already low
- Art enters DMA wait loop at 15-19KB
- DMA stuck at ~24KB (16 PCBs permanently cycling through socket pool via tcp_kill_timewait)
- **ART_MIN_FREE_DMA=40KB is unreachable at steady state when all 16 sockets are TIME_WAIT**

**The steady-state DMA floor = boot_DMA - (max_sockets × avg_pcb_dma_cost)**
= 115KB - 16 × ~6KB = **~19KB**

This is a structural problem. The system can never maintain 40KB free DMA at steady state because the SOAP polling alone fills all 16 socket slots with Time_Wait PCBs.

---

## Polling Task Guards (SonosPoll Task)

All guards are checked every loop cycle before firing any SOAP:

```
pollingTaskFunction() loop:
│
├─ [EARLY EXIT GUARD] ──────────────────────────────────────────────────────
│   if (art_download_in_progress) → skip all SOAPs, sleep 300ms, continue
│   if (last_art_download_end_ms < 1000ms ago) → skip, sleep 300ms, continue
│   if (last_track_change_ms < SDIO_TRACK_CHANGE_SETTLE_MS ago) → skip, sleep 1000ms (disabled: =0)
│   if (last_transient_500_ms < 3000ms ago) → skip, sleep 1000ms
│
├─ updateTrackInfo() ── ALWAYS (GetPositionInfo SOAP) ─────────────────────
│   → onSonosUpdate() may set pending_art_url
│
├─ [POST-SOAP-1 GUARD] ─────────────────────────────────────────────────────
│   if (pending_art_url != last_art_url) → tick++, sleep 300ms, continue
│   if (last_transient_500_ms < 3000ms ago) → tick++, sleep 300ms, continue
│
├─ updatePlaybackState() ── MANDATORY (GetTransportInfo SOAP) ─────────────
│
├─ [MID-CYCLE GUARD] ───────────────────────────────────────────────────────
│   if (art_download_in_progress || !pending_art_url.isEmpty()) → tick++, continue
│
├─ OPTIONAL SOAPs (all gated by mid-cycle guard passing):
│   ├─ updateVolume() every 5 cycles
│   ├─ updateTransportSettings() every 10 cycles
│   ├─ updateMediaInfo() every 50 cycles (radio only)
│   └─ updateQueue() every 100 cycles (non-radio only)
│       └─ if (last_art_download_end_ms < 3000ms) → SKIP
│       └─ after success: vTaskDelay(1000ms) drain
│
└─ vTaskDelay(300ms) ─── base interval
```

---

## Art Task Main Loop (albumArtTask)

```
albumArtTask() infinite loop:
│
├─ Check WiFi connected → if not: vTaskDelay(1000ms), continue
│
├─ [NO-URL PATH] ───────────────────────────────────────────────────────────
│   Read pending_art_url under art_mutex
│   if url == "":
│       if millis() - last_art_download_end_ms >= 1000ms (or ==0):
│           art_download_in_progress = FALSE  ← ONLY place flag is cleared
│       vTaskDelay(100ms); continue
│
├─ [URL PRESENT PATH] ──────────────────────────────────────────────────────
│   Check LRU cache → if hit: display cached art, continue
│   Check art_abort_download → if set: clear, continue
│   art_download_in_progress = FALSE  ← allow polling during sdioPreWait
│   sdioPreWait("ART", SDIO_WAIT_QUEUE_POLL, &abort_flag, &shutdown_flag)
│   → waits up to 3000ms if last_queue_fetch_time < 3s ago
│   → returns false if abort/shutdown fired → continue
│
├─ [DMA WAIT LOOP] ─────────────────────────────────────────────────────────
│   art_download_in_progress = TRUE  ← suppress polling NOW (set BEFORE wait)
│   for outer in 0..50:  (max 165s total)
│       for inner in 0..30:  (3s per outer)
│           fdma = heap_caps_get_free_size(DMA)
│           if fdma >= ART_MIN_FREE_DMA (8KB): dma_ok=true; break
│           vTaskDelay(100ms)
│       if !dma_ok:
│           art_download_in_progress = FALSE; vTaskDelay(300ms)  ← SDIO warm
│           art_download_in_progress = TRUE
│   if !dma_ok after 50 loops:
│       if fdma < 16384: Serial.printf("DMA critically low — skipping"); continue
│       else: proceed cautiously
│
│   [PRE-GET DMA BURST CHECK] — immediately before http.GET()
│       if fdma < ART_MIN_DMA_PRE_BURST (36000): abort (TCP slow-start max burst ~21KB
│           would crash dl-start); sets last_art_download_end_ms to rate-limit retries
│
├─ [ACQUIRE MUTEX + DOWNLOAD] ──────────────────────────────────────────────
│   xSemaphoreTake(network_mutex, 10s)
│   check abort → if set: release mutex, continue
│   inside-mutex cooldown rechecks (short TCP drain times, NOT full sdioPreWait times)
│   artPreConnectHTTP() → raw socket → SO_RCVBUF=8192 → connect → wrap in WiFiClient
│   http.begin(preClient, url)
│   http.GET()
│   if code==200: chunked read into jpgBuf (PSRAM), 5ms between chunks
│   [SUCCESS PATH]:
│       artLogMem("post-close")  ← TCP still open (passive close design)
│       decode JPEG/PNG → 420×420 scale → extract dominant color → update LVGL
│       http.end()  ← passive close: server FIN already arrived during decode
│       vTaskDelay(SDIO_TCP_CLOSE_MS = 200ms)
│       last_network_end_ms = last_art_download_end_ms = millis()
│       xSemaphoreGive(network_mutex)
│   [FAILURE PATH]:
│       http.end() + cleanup
│       last_network_end_ms = last_art_download_end_ms = millis()
│       xSemaphoreGive(network_mutex)
│
└─ vTaskDelay(100ms) ─── check interval
```

---

## The DMA Problem: Root Cause Analysis

### Confirmed Facts

1. Each TCP connection (SOAP or HTTP) creates a TIME_WAIT PCB holding **~6-8.7KB DMA** for 120 seconds
2. `SO_LINGER{1,0}` to force RST-close: **not compiled into pioarduino pre-built framework** — `lwip_setsockopt` returns -1 silently
3. `CONFIG_LWIP_TCP_MSL=60000ms` (60s), so TIME_WAIT = 2×60s = 120s — cannot be changed (pre-built)
4. `CONFIG_LWIP_MAX_SOCKETS=16` — at any time, max 16 PCBs exist
5. `tcp_kill_timewait()` auto-recycles oldest TIME_WAIT slot when pool is full

### Steady-State DMA Consumption

At polling rate 300ms per cycle, 2 SOAPs per cycle:
- 16 socket slots filled with TIME_WAIT within ~2.4 seconds of startup
- Each PCB holds ~6KB DMA (lwIP TCP receive buffer + PCB struct)
- **Steady state: 16 × 6KB = 96KB permanently consumed**
- **Boot DMA: ~115KB**
- **Available for downloads: 115 - 96 = ~19KB**

### Why ART_MIN_FREE_DMA=40KB Was Unreachable (Historical — threshold now 8KB)

The threshold was set at 40KB = 17KB TCP pre-connect + 23KB dl-start headroom. Correct calculation, but unreachable at steady state because the 16 TIME_WAIT PCBs are always consuming the headroom.

### What Needs to Change

**Option A: Reduce per-PCB DMA cost**
- Reduce `CONFIG_TCP_WND` (TCP receive window) → smaller PCB → less DMA per PCB
- Cannot change (pre-built framework)

**Option B: Force RST-close on SOAP connections**
- `SO_LINGER` not compiled in
- Alternative: `lwip_abort()` raw API or `shutdown(SHUT_RDWR)` then `close()`
- Risky: may not work reliably without SO_LINGER support

**Option C: HTTP keep-alive for SOAPs**
- Sonos device does NOT support HTTP/1.1 keep-alive
- Already confirmed: "Fresh HTTPClient per request — Sonos's embedded HTTP server does not support HTTP/1.1 keep-alive"

**Option D: Lower ART_MIN_FREE_DMA** ✅ DONE
- Lowered to 8KB. Pre-burst safety now handled by `ART_MIN_DMA_PRE_BURST=36000` check immediately before HTTP GET (accounts for TCP slow-start max burst of 14×1460=20KB).
- `ART_TCP_RCVBUF_DL_SAFETY=12000` as belt-and-suspenders at dl-start.

**Option E: Brief wait after http.getString() before http.end() in sendSOAP**
- If Sonos server sends FIN immediately after response body, a 1-5ms wait lets FIN arrive
- http.end() called in CLOSE_WAIT state → passive close → no TIME_WAIT on P4 side
- Server enters TIME_WAIT (server's RAM, not our DMA)
- **Most promising fix. Needs testing.**

**Option F: Reboot after extended DMA wait**
- If DMA stays below threshold for >30s, soft-reboot
- Extreme but guaranteed to restore DMA to boot level
- Bad UX but prevents permanent art failure

---

## Summary: What's Happening When Queue Play Gets Stuck

1. User has been using the app for ~2 minutes
2. SOAPs have depleted DMA to ~19-26KB (steady state)
3. User taps queue item
4. CMD_PLAY_QUEUE_ITEM fires 3 SOAPs: Seek + Play + GetPositionInfo
5. Each SOAP gets its tcp_kill_timewait() slot — DMA stays at ~19KB (recycled)
6. Art task enters DMA wait loop: 19KB < 40KB threshold
7. DMA wait loop: 3s inner wait + 300ms warm window, repeat up to 50×
8. During warm window: 2 SOAPs fire (warm cycle), which GET killed by tcp_kill_timewait() = DMA stays flat
9. DMA NEVER reaches 40KB — loop runs for the full 165 seconds
10. After 165s: fdma=24KB. NEW HARD-SKIP at 35KB fires → skip download → continue loop
11. Next iteration: DMA still 24KB → DMA wait loop again immediately
12. Art NEVER loads

**This is why it appears "stuck" — it IS stuck, legitimately, waiting for DMA that can never recover.**
