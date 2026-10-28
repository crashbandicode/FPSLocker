# FPSLocker

Set game FPS and display refresh rates on Nintendo Switch. This fork also remembers each monitor's refresh-rate preferences and display-warning acceptance.

**Matched releases:** [FPSLocker 3.5.0-monitor.1](https://git.syntaxbyexample.com/intpa/FPSLocker/releases/tag/3.5.0-monitor.1) + [SaltyNX 2.1.3-monitor.1](https://git.syntaxbyexample.com/intpa/SaltyNX/releases/tag/2.1.3-monitor.1).

**Hardware tested:** the operator reported successful use on an Erista Switch 1 / Smash Ultimate 13.0.5 setup at 120 Hz / 60 FPS on 2026-10-27. [Test record and fork provenance](docs/FORK.md).

## Setup

| Component | Requirement |
| --- | --- |
| Custom firmware | [Atmosphère](https://github.com/Atmosphere-NX/Atmosphere/releases) |
| Game/display hooks | Matching SaltyNX release above |
| Overlay menu | [Ultrahand](https://github.com/ppkantorski/Ultrahand-Overlay/releases) / Tesla environment |
| Performance | Adequate clocks for the chosen FPS; an overclocking toolset may be needed, especially docked |
| Docked display support | [sys-dock](https://git.syntaxbyexample.com/intpa/sys-dock) for OLED 120-Hz support and horizontal-line fixes; follow its configuration guide |

Copy `FPSLocker.ovl` to `sdmc:/switch/.overlays/`. Install the matching SaltyNX runtime and reboot after updating it. Keep existing settings, exclusions and plugins. For a fresh environment, see the [upstream setup guide](https://gist.github.com/masagrator/65fcbd5ad09243399268d145aaab899b).

> [!WARNING]
> Combining FPSLocker with 60-FPS cheats/mods can cause compatibility issues and crashes. Use one FPS-control method.

## Usage

### Change game FPS

1. Start a supported game and open FPSLocker. Confirm **NX-FPS plugin is running**.
2. Choose an FPS target. If the game needs a patch, connect to the internet and use **Advanced settings**:

   ```text
   Check/download config
             ↓
   Convert config to patch file
             ↓
   Restart game → Set FPS target
   ```

   Configs must match the game version and be available in [FPSLocker Warehouse](https://github.com/masagrator/FPSLocker-Warehouse).
3. If NVN reports **Set/Active/Available buffers: 2/2/3**, select **Set Buffering → Triple (force)**. Follow any restart prompt.

| Control | Handheld | Docked |
| --- | --- | --- |
| FPS target | 15–60 FPS, in steps of 5 | 15–60 FPS, plus enabled monitor rates above 60, up to 120 |
| Manual refresh rate | 40–60 Hz; OLED: 45–60 Hz | Allowed rates for the current display/link, up to 120 Hz |
| Display Sync | While a game runs, chooses a matching refresh or supported multiple. Hides manual Hz controls while enabled. | Same behavior, controlled separately |

Higher targets still depend on game performance and engine limits. [Full controls, patch errors and buffering guidance](docs/USAGE.md).

### Remember a monitor's refresh rate

1. Open **Display settings** and choose **Accept** when prompted.
2. Turn **Docked Display Sync off** for an independent display rate, such as a 120-Hz display with a 60-FPS game target.
3. Use **Change refresh rate** to select a supported rate. Configure **Docked Settings → Allowed refresh rates** if needed.

A successful selection saves the monitor's EDID profile, with separate 1080p and 720p values. On reconnect or resume, SaltyNX restores a valid saved rate once the link stabilizes. A new monitor starts with its own settings and warning; handheld acceptance is separate. Display Sync takes priority when enabled.

### Find an option

| Menu | Use it for |
| --- | --- |
| **Advanced settings** | Game patches, buffering, Window Sync Wait and halting an unfocused game |
| **Display settings** | Refresh rate, Display Sync, handheld HOME-menu behavior and display tests |
| **Docked Settings** | Monitor allowlists, under/overclock wizards, link information and docked policies |
| **Games list** *(no game running)* | Delete saved settings or patches for one game or **All** |
| **Force English language** *(no game running)* | Override the UI language; an overlay update resets this choice |

Languages: English, German, French, Russian, Brazilian Portuguese, Simplified Chinese and Traditional Chinese.

## Reference

- [Usage reference](docs/USAGE.md): every control, configuration paths and handheld/docked display notes.
- [Game compatibility](docs/COMPATIBILITY.md): Window Sync Wait and double-to-triple-buffer results.
- [Fork and hardware test record](docs/FORK.md): exact versions, source commits and test scope.
- Host checks: `make -C tests test` (C++23; Linux file-I/O fault tests).

## Credits and license

Fork of [masagrator/FPSLocker](https://github.com/masagrator/FPSLocker) 3.5.0, under the original [GNU GPL v2 license](LICENSE).

<details>
<summary>Upstream contributors and translations</summary>

Thanks to:
- ~WerWolv for creating Tesla environment
- ~cucholix + ~Monked for tests
- ~CTCaer for info about Samsung OLED panels
- ~NaGa for tests on Retro Remake SUPER5 display and providing a Switch OLED unit
  - and backers: Jorge Conceição, zany tofu, Lei Feng, brandon foster, AlM, Alex Haley, Stefano Bigio, Le Duc, Sylvixor x
- Anonymous contributor who found how to unlock the full refresh-rate range for Switch OLED while docked

Translations:
- German: ~Lightos_
- French: ~ganonlebucher
- Russian: ~usagi, ~redraz
- Brazilian Portuguese: ~Fl4sh
- Simplified Chinese: ~Soneoy, ~Tone Darkwell
- Traditional Chinese: [david082321](https://github.com/david082321)

Upstream notes that Claude was used to generate the host tester and add Aarch32 support to its asmjit fork.

</details>
