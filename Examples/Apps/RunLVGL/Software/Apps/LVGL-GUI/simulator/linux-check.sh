#!/usr/bin/env bash
# Build the RunLVGL PC simulator on Linux and regenerate its assets, the way a
# developer on Debian/Ubuntu would. Run from anywhere inside the SDK checkout:
#
#   UNA_SDK=/path/to/sdk ./linux-check.sh
#
# or in a throwaway container from the SDK root:
#
#   docker run --rm -v "$PWD:/sdk" -w /sdk ubuntu:24.04 \
#       bash Examples/Apps/RunLVGL/Software/Apps/LVGL-GUI/simulator/linux-check.sh
#
# Inside a container the script installs what it needs (apt + npm) itself.
set -euo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
export UNA_SDK="${UNA_SDK:-$(cd "$here/../../../../../../.." && pwd)}"
echo "UNA_SDK=$UNA_SDK"

if [ "${EUID:-$(id -u)}" -eq 0 ] && command -v apt-get >/dev/null; then
    export DEBIAN_FRONTEND=noninteractive
    # security.ubuntu.com outages hang apt for good; the main archive carries
    # the same pocket (same workaround as the SDK's CI workflows).
    find /etc/apt -type f \( -name '*.list' -o -name '*.sources' \) \
        -exec sed -i 's#security\.ubuntu\.com/ubuntu#archive.ubuntu.com/ubuntu#g' {} +
    apt-get update -qq
    apt-get install -y -qq --no-install-recommends \
        build-essential cmake ninja-build libsdl2-dev git ca-certificates gdb \
        python3 python3-png python3-lz4 nodejs npm >/dev/null
fi

echo "=== simulator: configure + build (gcc, Ninja) ==="
cmake -S "$here" -B "$here/build-linux" -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build "$here/build-linux" -j"$(nproc)"
ls -l "$here/build/bin/RunLVGLSimulator"

echo "=== simulator: headless smoke run (3 s) ==="
( cd "$here/build/bin" && SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy timeout 3s ./RunLVGLSimulator \
    || [ $? -eq 124 ] ) | tail -n 12

echo "=== assets: regenerate and compare with the committed files ==="
# A plain content comparison rather than git status: inside a container the
# checkout's .git may point at a path that only exists on the host.
snapshot="$(mktemp -d)"
cp -r "$here/../assets/fonts" "$here/../assets/images" "$snapshot/"
( cd "$here/../assets" && python3 gen_assets.py >/dev/null )
if diff -rq "$snapshot/fonts" "$here/../assets/fonts" && diff -rq "$snapshot/images" "$here/../assets/images"; then
    echo "generator output matches the committed assets"
else
    echo "generator output DIFFERS from the committed assets (see above)"
    exit 1
fi
rm -rf "$snapshot"
