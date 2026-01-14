# Una-Watch SDK Quick Start

Welcome to Una-Watch development! This guide will get you running your first app in under 10 minutes. Una-Watch apps are unique - they run as pure machine code directly in the watch's MCU memory, using position-independent execution and shared libc libraries.

## What Makes Una-Watch Special

Unlike traditional smartwatch platforms, Una-Watch apps are:
- **Pure Machine Code**: Compiled ELF binaries executing directly in MCU memory
- **Position-Independent**: Apps abstracted from kernel using PIC (Position-Independent Code)
- **Shared libc**: Memory-efficient library sharing across all apps
- **Dual-Process**: Service (background) + GUI (interface) process architecture

## Prerequisites

- Linux/macOS/Windows development machine
- STM32CubeIDE (recommended) or command-line tools
- USB cable for device flashing

## One-Command Setup

```bash
# Clone the SDK and set up development environment
git clone https://github.com/una-watch/una-sdk.git
cd una-sdk
./setup-dev-environment.sh
```

This script will:
- Install required build tools
- Set up cross-compilation toolchain
- Configure simulator environment
- Download sample apps

## Your First App: "Hello World" Clock

### Step 1: Create App Structure

```bash
# Create new app from template
./create-app.sh hello-clock --type clockface

cd apps/hello-clock
```

### Step 2: Examine the Code

The app consists of two main files:

**Service.hpp** (background logic):
```cpp
class Service : public SDK::Interface::IApp::Callback {
private:
    SDK::Kernel& kernel;
public:
    Service(SDK::Kernel& k) : kernel(k) {}

    void run() {
        while (true) {
            // Get current time from system
            uint32_t now = kernel.sys.getTimeMs();

            // Send time update to GUI process
            sendTimeUpdate(now);

            // Sleep for 1 second
            kernel.sys.delay(1000);
        }
    }
};
```

**Gui.hpp** (user interface):
```cpp
class Gui : public TouchGFX::Application {
public:
    void updateTime(const TimeInfo& time) {
        // Update display with current time
        hourHand.setRotation(time.hour * 30);
        minuteHand.setRotation(time.minute * 6);
        secondHand.setRotation(time.second * 6);
    }
};
```

### Step 3: Build the App

```bash
# Build for simulator (fast development)
make simulator

# Or build for hardware
make release
```

### Step 4: Test in Simulator

```bash
# Launch simulator
./simulator hello-clock.uapp

# The simulator shows your watch app running in a desktop window
# Interact with virtual buttons and touch screen
```

### Step 5: Deploy to Device

```bash
# Connect your Una-Watch via USB
# Flash the app
make flash

# Your clock app is now running on the watch!
```

## Understanding the Magic

### Pure Machine Code Execution

Your app isn't interpreted or virtualized - it's compiled to native ARM Cortex-M instructions that run directly on the MCU. This provides native performance and minimal overhead.

```cpp
// High-performance direct buffer access
void updateDisplay(const uint8_t* frameBuffer) {
    // Write frame buffer directly via SDK interface
    kernel.app.writeFrameBuffer(frameBuffer);
}
```

### Position-Independent Code (PIC)

Apps use PIC to remain kernel-abstracted:

```cpp
// Function calls use relative addressing
void drawPixel(int x, int y) {
    // No absolute memory addresses - all relative
    lcdBuffer[y * WIDTH + x] = currentColor;
}
```

### Shared libc Architecture

All apps share the same libc implementation for memory efficiency:

```cpp
// Apps can use standard C++ features
#include <string>
#include <vector>

std::string formatTime(const TimeInfo& t) {
    return std::to_string(t.hour) + ":" + std::to_string(t.minute);
}
```

## Next Steps

🎉 **Congratulations!** You have a working Una-Watch app.

### What to Explore Next

1. **Add Sensors**: Integrate heart rate or accelerometer data
2. **Custom UI**: Design beautiful watch faces with TouchGFX
3. **Notifications**: Handle phone notifications and glances
4. **Data Persistence**: Store user preferences and app data
5. **BLE Communication**: Connect with companion apps

### Learning Resources

- **[Getting Started Guide](getting-started.md)**: Complete development setup
- **[API Reference](api-reference.rst)**: Comprehensive SDK documentation
- **[SDK Overview](sdk-overview.md)**: Core concepts and tools
- **[Community Forum](https://forum.una-watch.dev)**: Ask questions and share apps

### Need Help?

- **Discord**: Real-time chat with developers
- **GitHub Issues**: Bug reports and feature requests
- **Documentation Issues**: Found a problem? [Edit this page](https://github.com/una-watch/docs/edit/main/quick-start.md)

---

**Time to complete**: 10 minutes
**Skills learned**: App creation, building, testing, deployment
**Platform concepts**: PIC execution, shared libc, dual-process architecture