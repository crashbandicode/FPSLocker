# FPSLocker fork

This tree is a fork of [FPSLocker](https://github.com/masagrator/FPSLocker) **3.5.0**, commit `0931909324a917b8cd0eea58587c170075327d94`. The original overlay is by masagrator.

The program is distributed under the **GNU General Public License, version 2**. The license text in the repository root, `LICENSE`, is the upstream file and is not modified. Third-party components keep their own upstream licenses and copyright notices, including libtesla, asmjit, and rapidyaml/c4core.

Branch `monitor-preferences` adds per-monitor storage for a manually selected external refresh rate and for the display-settings warning. It does not change the original author's copyright or relicense the overlay.

The companion SaltyNX tree for this branch is fork `2.1.3-monitor.1`. Its upstream base is commit `cf2adb7b8facdc34e3cbf9ec6ff7686f67575048`: release 2.1.3 plus the NVN stack reduction. An earlier 2.1.2 base is superseded. The display INI keys are unchanged. SaltyNX itself is not part of this repository.

## Operator test — 2026-10-27

The operator tested the installed **FPSLocker 3.5.0-monitor.1 / SaltyNX 2.1.3-monitor.1** pair and reported: **"seems to work great."** The deployed setup is an Erista Switch 1 running Smash Ultimate 13.0.5, using a 120-Hz external display and 60-FPS game target with the existing normal-speed refresh-clock plugin retained.

Tested source commits: FPSLocker `cc9eb5a0c3ccc6caabd5d8763bff9e61d31338d1` and SaltyNX `6431462d5213cc3f5b4e99937acf8c90576ed088`. Host sanitizer tests and isolated Switch builds passed before installation. The seven installed runtime files matched release hashes; 68 protected files were unchanged and the SD card was safely unmounted.

This records successful operator testing on that setup. Individual results for every alternate monitor/resolution/link scenario and measured latency/frametimes were not supplied.

Monitor persistence uses `SaltySD/plugins/FPSLocker/ExtDisplays/<EDID CRC32>.ini`. After a successful manual rate change, FPSLocker saves `preferredRefreshRate` for 1920×1080 or `preferredRefreshRate720p` for 1280×720. Explicit Accept saves `displayDisclaimerAcknowledged=True` for that monitor. With Docked Display Sync off, the matching SaltyNX recognizes the same monitor on reconnect and restores a valid saved rate after the link stabilizes.
