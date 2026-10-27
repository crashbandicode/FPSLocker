# FPSLocker fork

This tree is a fork of [FPSLocker](https://github.com/masagrator/FPSLocker) **3.5.0**, commit `0931909324a917b8cd0eea58587c170075327d94`. The original overlay is by masagrator.

The program is distributed under the **GNU General Public License, version 2**. The license text in the repository root, `LICENSE`, is the upstream file and is not modified. Third-party components keep their own upstream licenses and copyright notices, including libtesla, asmjit, and rapidyaml/c4core.

Branch `monitor-preferences` adds per-monitor storage for a manually selected external refresh rate and for the display-settings warning. It does not change the original author's copyright or relicense the overlay.

The companion SaltyNX tree for this branch is fork `2.1.3-monitor.1`. Its upstream base is commit `cf2adb7b8facdc34e3cbf9ec6ff7686f67575048`: release 2.1.3 plus the NVN stack reduction. An earlier 2.1.2 base is superseded. The display INI keys are unchanged. SaltyNX itself is not part of this repository.
