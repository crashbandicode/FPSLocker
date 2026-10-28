# Usage reference

[Quick start](../README.md#usage) · [Game compatibility](COMPATIBILITY.md) · [Fork and test record](FORK.md)

## Game controls and status

With a supported game running, **NX-FPS plugin is running** confirms that the game hooks are active.

| Field or control | Meaning |
| --- | --- |
| **Interval Mode** | NVN/EGL vsync interval: 1–4; unset `0` behaves like `1`. At 60 Hz, interval `2` limits presentation to at most 30 FPS. |
| **Custom FPS Target** | Requested FPS cap. An engine's own limit may require a game/version-specific patch to exceed 30 FPS. |
| **FPS** | Frames counted during the last second; use it to check the achieved rate. |
| **Patch file doesn't exist.** | FPSLocker has identified a required patch that is missing. Download a matching config and convert it; one may not be available for that game/version. |
| **Increase/Decrease FPS target** | Handheld: adjust by 5, from 15 to 60 FPS. |
| **Change FPS target** | Docked: 15–60 FPS in steps of 5, plus enabled monitor rates of 70 Hz or above that fit the display's reported maximum. The extra choices use the current 720p/1080p allowlist and can reach 120 FPS. |
| **Disable custom FPS target** | Clear the current mode's custom cap; with Display Sync on for that mode, also return to 60 Hz. Restore the desired interval through the FPS controls first: disabling the cap does not infer the game's original interval. |

Higher FPS requires sufficient CPU/GPU/memory performance. Docked 60 FPS is not guaranteed even with substantial overclocking.

Targets are saved per game, with separate handheld and docked values, in `SaltySD/plugins/FPSLocker/<TITLEID>.dat`.

## Advanced settings

### Game patches

Configs come from [FPSLocker Warehouse](https://github.com/masagrator/FPSLocker-Warehouse) and must match the game and build. Supported YAML size is at most 32 KiB. The conversion/deletion actions require a valid matching config.

| Action | Behavior |
| --- | --- |
| **Check/download config file** | Download and compare the matching config. A changed config removes the old config/patch; convert the new config again. |
| **Convert config to patch file** | Create the patch from a matching config. It takes effect on the next game launch. |
| **Delete patch file** | Remove the patch so it will not load on the next game launch. |

Patches are stored at `SaltySD/plugins/FPSLocker/patches/<TITLEID>/<BUILDID>.bin`, with uppercase IDs. Download error `0x312` means GitHub returned an unexpected file; other download errors can indicate a connection or server problem.

### Buffering and Window Sync Wait

| Option | Availability | Behavior |
| --- | --- | --- |
| **Window Sync Wait** | NVN games using double buffering | Choose **Enabled**, **Semi-Enabled** or **Disabled**. Disabled can bypass double-buffer vsync in compatible games, but can cause graphical glitches or crashes. See [known compatible games](COMPATIBILITY.md#sync-wait). |
| **Set Buffering** | NVN | Choose double/triple buffering, or quadruple when four buffers are available. **(force)** variants override a count explicitly requested by the game. Some changes apply only at the next game launch; follow the restart prompt. |
| **Set Buffering** | Vulkan | Choose double or triple buffering; restart the game for the change to take effect. |
| **Halt unfocused game** | Advanced settings | Suspend a game when it loses focus, including games that would otherwise keep running in HOME. |

Double buffering can help frame pacing/input lag in games already holding a steady 30 or 60 FPS. With uneven performance, it can make drops much worse. NVN games reporting `0/2/2` may fail to boot or crash later when forced to triple buffering; consult the [compatibility tables](COMPATIBILITY.md#double-to-triple-buffer-forcing).

**Set / Active / Available buffers** means:

| Value | Meaning |
| --- | --- |
| **Set** | Count requested through `nvnWindowSetNumActiveTextures`; `0` means the game does not use it. A request below **Available** may need the **(force)** setting to avoid reverting to the game's choice. |
| **Active** | Buffers currently used by the game. |
| **Available** | Buffers provided to NVN; this is the capacity the game can use. |

For `2/2/3`, select **Triple (force)** to use the third available buffer.

## Display settings

Changing display Hz and setting a game's FPS cap are separate operations. **Handheld Display Sync** and **Docked Display Sync** link them when enabled: while a game runs, SaltyNX chooses a supported/allowed refresh rate matching the target or a multiple. If no match is available, it uses the configured fallback. Manual refresh controls are hidden while Display Sync is enabled.

| Option | Availability | Purpose |
| --- | --- | --- |
| **Increase/Decrease refresh rate** | Handheld, Display Sync off | Set 40–60 Hz on LCD/Retro Remake panels, or 45–60 Hz on OLED. |
| **Change refresh rate** | Docked, Display Sync off | Select an allowed rate supported by the current monitor/link. Successful changes save that monitor's preference. |
| **60 Hz in HOME Menu** | Handheld Display Sync on | Return to 60 Hz in HOME while a game is running. The docked equivalent is under **Docked Settings → Additional settings**. |
| **Retro Remake Mode** | Lite reporting `InnoLux 2J055IA-27A (Rev B1)` | Enable only if the panel is a first-revision SUPER5, which spoofs this ID. Other Retro Remake displays are detected automatically. |
| **Frameskip tester** | Handheld or docked | Check whether the panel really displays every refresh. A panel can accept 50 Hz while still displaying at 60 Hz; follow the on-screen instructions. |

### Docked Settings

Monitor configuration tools require docked mode at 720p or 1080p and are unavailable on Switch Lite. Close the game before running either wizard; **B** cancels a wizard and restores the previous allowlist.

| Item | Purpose |
| --- | --- |
| **myDP link rate** | Report `RBR`, `HBR`, `HBR2`, `HBR3` or `n/d`, with a link-capacity indicator. See [link limits](#docked-display-notes). |
| **Config ID** | Identify the current monitor's file in `SaltySD/plugins/FPSLocker/ExtDisplays/`. |
| **Allowed refresh rates** | Edit this monitor's 720p/1080p allowlist: 40/45/50/55 Hz and supported table rates from 70–120 Hz. Defaults enable 50 and 60 Hz; 60 Hz is fixed. Press **B** to save and leave. |
| **Display underclock wizard** | Test 40–55 Hz. Confirm a working mode within 15 seconds; otherwise the wizard advances. Review results in **Allowed refresh rates**. |
| **Display overclock wizard** | Available when the reported maximum is at least 70 Hz. Test 70/72/75/80/90/95/100/110/120 Hz up to that maximum; confirmation timeout is 10 seconds. Review the resulting allowlist. |

**Additional settings**:

| Option | Default | Effect |
| --- | --- | --- |
| **Allow patches to force 60 Hz** | On | Allow patches to use 60 Hz for frame pacing, such as 30-FPS cutscenes. A forced change pauses the game for four seconds. Off applies only the FPS lock, without that rate change or delay. |
| **Use lowest refresh rate for unmatched FPS targets** | Off | Use the lowest enabled rate when the target has no refresh-rate match; otherwise fall back to 60 Hz. |
| **60 Hz in HOME Menu** | Off | With Docked Display Sync enabled, use 60 Hz in HOME for this monitor while a game is running. |

### Saved monitor profiles and warning acceptance

The current monitor is identified by the CRC32 of its complete EDID structure. FPSLocker stores its profile at `SaltySD/plugins/FPSLocker/ExtDisplays/<EDID CRC32>.ini`, under `[Common]`.

| Key | Saved value |
| --- | --- |
| `preferredRefreshRate` | Successful manual selection at 1920×1080. |
| `preferredRefreshRate720p` | Independent selection at 1280×720. |
| `displayDisclaimerAcknowledged=True` | Explicit acceptance of this monitor's display warning. |

With Docked Display Sync off, the matching SaltyNX restores a valid preference after a stable reconnect, resolution change or resume. Display Sync takes priority when enabled. Missing, invalid, disallowed or unsupported preferences retain the default 60-Hz behavior. A new EDID gets its own profile; settings are not copied between monitors or resolutions. Selecting a rate does not toggle Display Sync or change a game's FPS target.

The internal display stores its separate warning acknowledgment in `SaltySD/plugins/FPSLocker/display.ini`. Accept persists it; Decline/Back do not. A failed docked identity query does not reuse handheld acceptance. Saving failures remain visible in the menu.

Profile writes preserve names, comments, unknown keys/sections, allowlists, policies and the other resolution's values. `[Common]` stays first for older readers. Both components use a 16-KiB INI limit; unreadable/oversized originals are preserved, as is the previous file if replacement fails.

### Handheld display notes

- **OLED:** gamma correction after a rate change is approximate. Small color differences have been observed, notably at 45 Hz / 60% brightness. The supported minimum is 45 Hz; 40 Hz misbehaves.
- **LCD/Retro Remake:** the supported range is 40–60 Hz. One report described flicker at 40 Hz on `InnoLux P062CCA-AZ2`; other users of that panel did not see it. Upstream reported no immediately visible LCD issues at 70 Hz but image glitches from 75 Hz; FPSLocker keeps the handheld ceiling at 60 Hz. OLED misbehaves above 60 Hz.
- **Retro Remake transitions:** panels need time to settle, so changes are delayed. Switching too quickly can produce a black screen; sleep or a restart can recover it.
- With **Handheld Display Sync off**, a custom handheld rate is not restored after sleep. This differs from the fork's saved external-monitor preferences.
- Lower refresh rates also slow OS/Tesla animations.

Upstream disclaimer: “I am not taking any responsibility for damages occurring from changing the refresh rate.” The display warning asks you to accept responsibility before changing rates; acceptance is remembered separately for the internal display and each external monitor.

### Docked display notes

- The external rate cap is 120 Hz. Available modes still depend on the monitor, dock and trained link.
- A two-lane HBR link has a roughly 180-MHz pixel-clock limit, allowing about 75 Hz at 1080p. If 75 Hz is the limit, 60 Hz may be needed for working audio. HBR2 training can fail depending on the Switch/dock; forcing retraining is blocked by HOS resetting the signal.
- OLED's software PCIe bandwidth cap also leads to HBR limits; dock lane count matters, and original docks use two active DP lanes. [sys-dock](https://git.syntaxbyexample.com/intpa/sys-dock) can override those limitations and address horizontal-line glitches.
- HOS applets can become unstable at 100 Hz or above, including crashes when a game opens user selection. Some games have their own issues; *Batman: The Enemy Within* was reported to crash when closed at higher refresh rates.

## When no game is running

| Menu | Behavior |
| --- | --- |
| **Games list** | List up to 32 installed games plus **All**. Delete saved settings or generated patches for the selected scope. |
| **Display settings** | Use the display controls described above. |
| **Force English language** | Override the UI language by modifying the overlay executable. Updating the overlay resets it. |
