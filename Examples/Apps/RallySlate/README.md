# Rally Slate

Offline casual-round A/B scoring using all four watch buttons. L1/L2 add points, R1 undoes the latest rally (including a winning point), and R2 opens the menu. Targets11/15/21 use a two-point lead and hard cap30. New rounds and target changes require confirmation. A/B labels accompany the colors. There is no player identity, network, sensor or model component.

Two validated save records alternate writes. Recovery from one damaged record is visible; two damaged records require an explicit new round. Write failures and unsaved exit are shown. These are local simulator checks, not a crash-atomic hardware guarantee or an official sport rules implementation.

Build with UNA_SDK set to the repository root, an ARM GNU toolchain on PATH, and the SDK packer dependencies installed:

```sh
cmake -S Software/Apps/RallySlate-CMake -B build -G Ninja
cmake --build build
```

Run the existing core checks independently of the SDK:

```sh
c++ -std=c++17 -Wall -Wextra test_score.cpp -o /tmp/rally-core-check
/tmp/rally-core-check
```

The .uapp is written to build/. The default ID is a development ID; a portal release must be rebuilt with that portal's generated APP_ID. This proposal does not establish community-store publication or contest entry.

The standalone development source, desktop simulator, original core tests, screenshots and measured validation scope are at https://github.com/brodyga45/refund-pact/tree/codex/rally-slate-candidate . The app was prepared by a coding agent under brodyga45's direction. No human review or actual-watch installation is claimed. A general watch scorekeeper is an existing idea; catalog-wide originality is unproven. Original code is MIT; SDK/LVGL/Montserrat notices are preserved under licenses/ and the in-app Licenses menu.
