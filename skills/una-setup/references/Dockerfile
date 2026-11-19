# Build environment for UNA Watch apps: the Arm GNU Toolchain, CMake, Ninja and
# Python, plus SDL2 for the LVGL and glance simulators. Works on x86-64 and arm64
# hosts (Apple Silicon included).
#
# From the SDK root (the build context is the packer's folder, for its
# requirements.txt):
#
#   docker build -t una-build -f skills/una-setup/references/Dockerfile Utilities/Scripts/app_packer
#   docker run --rm -it --user "$(id -u):$(id -g)" -e HOME=/tmp \
#     -v "<una-sdk>:/sdk" -v "<your-app>:/app" una-build
#
# --user and HOME are for a Linux host, so the build output belongs to you.
#
# The container sets UNA_SDK=/sdk.
FROM ubuntu:24.04

ARG ARM_GNU_VERSION=14.3.rel1

RUN apt-get update \
 && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
      ca-certificates curl xz-utils git cmake ninja-build make zip \
      python3 python3-pip python3-venv build-essential libsdl2-dev \
 && rm -rf /var/lib/apt/lists/*

RUN case "$(uname -m)" in \
      x86_64)  host=x86_64 ;; \
      aarch64) host=aarch64 ;; \
      *) echo "unsupported host $(uname -m)"; exit 1 ;; \
    esac \
 && curl -fsSL -o /tmp/arm.tar.xz \
      "https://developer.arm.com/-/media/Files/downloads/gnu/${ARM_GNU_VERSION}/binrel/arm-gnu-toolchain-${ARM_GNU_VERSION}-${host}-arm-none-eabi.tar.xz" \
 && mkdir -p /opt/arm-gnu \
 && tar xf /tmp/arm.tar.xz -C /opt/arm-gnu --strip-components=1 \
 && rm /tmp/arm.tar.xz

# The packer's Python dependencies, in a venv so pip leaves the system Python alone.
COPY requirements.txt /tmp/requirements.txt
RUN python3 -m venv /opt/venv \
 && /opt/venv/bin/pip install --no-cache-dir -r /tmp/requirements.txt

ENV PATH=/opt/arm-gnu/bin:/opt/venv/bin:$PATH \
    UNA_SDK=/sdk

# A mounted checkout belongs to another user; let git read it for the version.
RUN git config --system --add safe.directory '*'

WORKDIR /app
CMD ["bash"]
