# Third-Party Licenses

The MIT license in [`LICENSE`](LICENSE) covers **UNA Watch Ltd's own source code**
in this repository.

The third-party components vendored under `ThirdParty/` are **licensed separately
under their own terms and are NOT covered by this repository's MIT license**. Each
component retains its own copyright notices and license file; consult those before
redistributing.

| Component | Location | License |
|-----------|----------|---------|
| TouchGFX | `ThirdParty/touchgfx/` | STMicroelectronics **SLA0048** — see [`ThirdParty/touchgfx/LICENSE.txt`](ThirdParty/touchgfx/LICENSE.txt). Use and redistribution are permitted **only in connection with a microcontroller or microprocessor manufactured by or for STMicroelectronics**, and the component **must not be made subject to any open-source license terms**. TouchGFX itself bundles further third-party code (e.g. the Anti-Grain Geometry rasterizer and, for the simulator, SDL2) under their own respective licenses. |
| coreJSON | `ThirdParty/coreJSON/` | **MIT** — see [`ThirdParty/coreJSON/LICENSE`](ThirdParty/coreJSON/LICENSE). |
| tinycbor | `ThirdParty/tinycbor_version/` | **MIT** — Copyright (C) Intel Corporation; `SPDX-License-Identifier: MIT` (see the header files in that directory). |

> Note: because SLA0048 forbids subjecting TouchGFX to open-source terms, the
> repository's MIT license is scoped to UNA Watch Ltd's own code only and does not
> extend to anything under `ThirdParty/`.
