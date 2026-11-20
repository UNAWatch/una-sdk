# Third-party notices

Rally Slate original code is provided under MIT. The service loop and simulator bootstrap derive from UNA Watch SDK HelloWorld at d71d0dc3bad7d47282fcf5a4c53a3f0f2031afb1; preserve licenses/UNA-SDK-MIT.txt. Modifications and new GUI/core were prepared by Codex under brodyga45's direction; no human review is claimed.

The SDK is fetched as a dependency. Its own code is MIT; its separately licensed third-party components are not automatically MIT. This app uses LVGL rather than TouchGFX, and does not link the TouchGFX GUI libraries. LVGL at 85aa60d18b3d5e5588d7b247abf90198f07c8a63 uses MIT; preserve licenses/LVGL-MIT.txt. The built-in Montserrat font uses SIL Open Font License; preserve licenses/Montserrat-OFL.txt. Original seven-segment drawing and launcher icons contain no downloaded image or external font.

GNU ARM compiler, newlib and Python packaging dependencies retain their own licenses. Source licensing is not a claim of official UNA endorsement, device validation, contest acceptance or award.

For the measured GNU ARM13.3.1 firmware build, the toolchain headers identify newlib4.4.0. Linker maps contain GNU compiler/C++ and C-library archive members (some records may be discarded by section collection). To preserve runtime notices with binary distributions, the package includes GCC GPLv3 with Runtime Library Exception3.1 and the full newlib4.4.0 notice collection. These texts do not change the MIT licence on the original app source.

Runtime notice sources:

- GCC13.3.0: https://github.com/gcc-mirror/gcc/blob/releases/gcc-13.3.0/COPYING3 and https://github.com/gcc-mirror/gcc/blob/releases/gcc-13.3.0/COPYING.RUNTIME
- newlib4.4.0: https://github.com/mirror/newlib-cygwin/blob/newlib-4.4.0/COPYING.NEWLIB

The three SDK/LVGL/font notices are also visible in the app. Full compiler and C-runtime notice texts accompany the ZIP and source under licenses/.
