# Overview: Alarm App

This tutorial walks you through building your first UNA Watch app using the Alarm example. For SDK setup and build system reference, see [SDK Setup and Build Overview](sdk-setup.md).


Our example app demonstrates a complete alarm clock application with the following features:

- Set multiple alarms with custom times
- Configure repeat patterns (daily, weekdays, weekends, specific days)
- Choose alarm effects (beep, vibration, or both)
- Snooze functionality with configurable delay
- Persistent storage of alarm settings
- Touch-based navigation through alarm list
- Time setting interface with wheel-based input

The Alarm app showcases the dual-process architecture with a service component handling time monitoring and alarm triggering, and a GUI component providing the user interface.

## Prerequisites

Before starting, ensure you have:

- UNA SDK cloned/set up ([SDK Setup](sdk-setup.md))
- CMake 3.21+
- **ST ARM GCC Toolchain (CRITICAL)**: CubeIDE/CubeCLT version only (system `gcc-arm-none-eabi` incompatible). See [sdk-setup.md#toolchain-setup](sdk-setup.md#toolchain-setup).
- TouchGFX Designer (optional, GUI mods)
- `UNA_SDK` env var set to SDK root

## Project Setup

### Step 1: Set Environment Variables

**CRITICAL: Verify ST ARM GCC** (see [sdk-setup.md#toolchain-setup]):
```
arm-none-eabi-gcc --version  # Must show ST version (e.g., 14.3.1+st.2), NOT system 13.2
```

Export `UNA_SDK`:
```bash
export UNA_SDK=/path/to/una-sdk
```
**Persist**: Add to `~/.zshrc` & `source ~/.zshrc` (Linux) or PowerShell profile (Win).

### Step 2: Copy the Alarm Template

Create a new app directory and copy the Alarm example:

```bash
# Create your app directory
mkdir -p MyAlarmApp
cd MyAlarmApp

# Copy the Alarm app structure
cp -r $UNA_SDK/SDK/Examples/Apps/Alarm/* .
```

### Step 3: Customize CMakeLists.txt

#### Edit `Software/Apps/Alarm-CMake/CMakeLists.txt` to match your app:

```cmake
# App configuration
set(APP_NAME "MyAlarm")
set(APP_TYPE "Activity")
set(DEV_ID "UNA")
set(APP_ID "YOUR_UNIQUE_APP_ID")  # Generate a unique 16-character hex ID
set(RESOURCES_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../Resources")
set(OUTPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../Output")
set(LIBS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../Libs")
set(TOUCHGFX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../TouchGFX-GUI")
```

#### Generate a unique APP_ID using:
```bash
python3 -c "import hashlib; print(hashlib.md5(b'MyAlarm').hexdigest().upper()[:16])"
```

#### Choose App Type

Una-Watch supports four app types:

| Type | Use Case | Lifecycle | Note |
|------|----------|-----------|---------|
| **Activity** | Full-featured apps (Running, Hiking) | Always running | Running Glance(background) + GUI |
| **Utility** | Tools (calculator, settings) | On-demand | Not supported yet |
| **Glance** | Notifications (weather, alerts) | Background | Background periodick tasks for statistics, etc. |
| **Clockface** | System Interface | System-level | Not supported yet |

### Step 4: Build the App

Navigate to the CMake directory and build:

```bash
cd Software/Apps/Alarm-CMake

# Configure
cmake -S . -B build

# Build
cmake --build build

# The .uapp file will be generated in ../../../Output/
```

## App Architecture Overview

The Alarm app follows the UNA SDK's dual-process model:

### Service Process (Background)
- Monitors current time via system clock
- Checks for active alarms every minute
- Triggers alarm notifications (beep/vibration)
- Manages snooze functionality
- Persists alarm settings to flash storage

### GUI Process (User Interface)
- Displays alarm list with navigation
- Provides time setting interface
- Shows alarm configuration options
- Handles user input via touch and buttons

### Key Components

- **AlarmManager**: Core business logic for alarm operations
- **AppTypes**: Data structures for alarm configuration
- **TouchGFX GUI**: Screens for user interaction
- **Persistent Storage**: JSON-based alarm storage

## Step-by-Step Implementation Analysis

### Phase 1: Understanding the Service Layer

#### Alarm Data Structure

The alarm configuration is defined in `Libs/Header/AppTypes.hpp`:

```cpp
namespace AppType {
struct Alarm {
    bool on;                ///< Alarm active state
    uint8_t timeHours;      ///< Alarm hour (0-23)
    uint8_t timeMinutes;    ///< Alarm minute (0-59)
    Repeat repeat;          ///< Repetition pattern
    Effect effect;          ///< Notification effect

    enum Repeat {
        REPEAT_NO,           // One-time alarm
        REPEAT_EVERY_DAY,    // Daily
        REPEAT_WEEK_DAYS,    // Monday-Friday
        REPEAT_WEEKENDS,     // Saturday-Sunday
        REPEAT_MONDAY, REPEAT_TUESDAY, REPEAT_WEDNESDAY,
        REPEAT_THURSDAY, REPEAT_FRIDAY, REPEAT_SATURDAY, REPEAT_SUNDAY,
        REPEAT_COUNT
    };

    enum Effect {
        EFFECT_BEEP_AND_VIBRO,  // Both sound and vibration
        EFFECT_VIBRO,           // Vibration only
        EFFECT_BEEP,            // Sound only
        EFFECT_COUNT
    };
};
}
```

#### Alarm Manager Logic

The `AlarmManager` class in `Libs/Header/AlarmManager.hpp` handles:

**Time Monitoring:**
```cpp
uint32_t execute(const std::tm& tmNow) {
    checkAlarms(tmNow.tm_hour, tmNow.tm_min, tmNow.tm_wday, tmNow);
    return 60000; // Check again in 1 minute
}
```

**Alarm Checking:**
```cpp
void checkAlarms(uint8_t currentHour, uint8_t currentMinute, uint8_t currentDay, const std::tm& tmNow) {
    for (const auto& alarm : mAlarms) {
        if (alarm.on && isAlarmDueToday(alarm, currentDay) &&
            alarm.timeHours == currentHour && alarm.timeMinutes == currentMinute) {

            // Trigger alarm
            onAlarm(alarm);

            // Handle snooze logic
            if (shouldSnooze(alarm)) {
                addSnoozedAlarm(alarm, tmNow);
            }
        }
    }
}
```

**Persistence:**
```cpp
bool saveAlarmList(const std::vector<AppType::Alarm>& list) {
    // Serialize to JSON and save to file system
    char buffer[1024];
    uint32_t jsonSize = createJSON(list, buffer, sizeof(buffer));

    auto file = mKernel.fs.file(skFilePath);
    file->write(buffer, jsonSize);
    return true;
}
```

### Phase 2: GUI Implementation Analysis

#### Screen Flow

The app has several screens managed by TouchGFX:

1. **Main Screen**: Alarm list with navigation
2. **Set Time Screen**: Time configuration with hour/minute wheels
3. **Action Screen**: Alarm settings (repeat, effect, on/off)
4. **Alarm Screen**: Active alarm display with snooze/dismiss

#### Main Screen Logic

In `gui/src/main_screen/MainView.cpp`:

```cpp
void MainView::updateAlarmList(const std::vector<AppType::Alarm>& list) {
    pList = &list;
    mAlarmId = 0;
    show();
}

void MainView::handleKeyEvent(uint8_t key) {
    if (key == Gui::Config::Button::L1) {
        // Next alarm
        mAlarmId = (mAlarmId + 1) % (pList->size() + 1);
        show();
    }
    if (key == Gui::Config::Button::R1) {
        if (mAlarmId == pList->size()) {
            // Create new alarm
            application().gotoSetTimeScreenNoTransition();
        } else {
            // Edit existing alarm
            application().gotoActionScreenNoTransition();
        }
    }
}
```

#### Time Setting Interface

The time setting uses custom wheel containers for intuitive input:

```cpp
// In SetTimeView.cpp
void SetTimeView::handleKeyEvent(uint8_t key) {
    if (key == Gui::Config::Button::R1) {
        // Save time and proceed to action screen
        uint8_t hours = timeWheel.getHours();
        uint8_t minutes = timeWheel.getMinutes();
        presenter->setAlarmTime(hours, minutes);
        application().gotoActionScreenNoTransition();
    }
}
```

### Phase 3: Inter-Process Communication

#### Service to GUI Messages

When an alarm triggers, the service sends a message to the GUI:

```cpp
// In AlarmManager
void AlarmManager::onAlarm(const AppType::Alarm& alarm) {
    // Notify GUI process
    if (mObserver) {
        mObserver->onAlarm(alarm);
    }

    // Send IPC message for GUI activation
    // (Implementation depends on specific IPC mechanism)
}
```

#### GUI to Service Commands

The GUI sends commands to modify alarm settings:

```cpp
// In MainPresenter
void MainPresenter::saveAlarm(size_t id, const AppType::Alarm& alarm) {
    // Send message to service to update alarm list
    model->saveAlarm(id, alarm);
}
```

## Feature Analysis

### Alarm Scheduling Logic

The app supports flexible scheduling:

- **One-time alarms**: Trigger once and disable automatically
- **Daily alarms**: Repeat every day at the same time
- **Weekly patterns**: Weekdays, weekends, or specific days
- **Snooze functionality**: 5-minute delay with up to 5 snoozes

### Notification Effects

Three effect options provide user choice:

- **Beep + Vibration**: Full attention-grabbing alert
- **Vibration only**: Silent but tactile notification
- **Beep only**: Audio notification for noisy environments

### Persistent Storage

Alarms are stored as JSON for human-readable configuration:

```json
[
  {
    "on": true,
    "timeHours": 7,
    "timeMinutes": 30,
    "repeat": "every_day",
    "effect": "beep_vibro"
  }
]
```

## Modifying TouchGFX GUI

### Using TouchGFX Designer

1. **Open the Project**:
   ```
   Open TouchGFX Designer
   File > Open > Select AlarmGUI.touchgfx
   ```

2. **Visual Design**:
   - Modify screen layouts in the Screens tab
   - Add new widgets from the Widgets panel
   - Configure interactions in the Interactions tab
   - Import images and fonts in the Assets tab

3. **Generate Code**:
   ```
   Click "Generate Code" button
   This updates generated/ directory with new base classes
   ```

### Custom Code Implementation

**Avoid editing generated files**. Instead, modify custom files:

- **View Classes** (`gui/src/screen_name/screenview.cpp`):
  ```cpp
  void AlarmView::handleKeyEvent(uint8_t key) {
      // Custom button handling logic
      if (key == CUSTOM_BUTTON) {
          // Your custom action
          presenter->customAction();
      }

      // Call base implementation for default behavior
      AlarmViewBase::handleKeyEvent(key);
  }
  ```

- **Presenter Classes** (`gui/src/screen_name/screenpresenter.cpp`):
  ```cpp
  void AlarmPresenter::activate() {
      // Custom activation logic
      view.updateDisplay(model->getAlarmData());
  }
  ```

- **Model Updates** (`gui/src/model/model.cpp`):
  ```cpp
  void Model::saveAlarm(const AppType::Alarm& alarm) {
      // Send to service process via IPC
      // Update local state
  }
  ```

### Adding New Screens

1. **Create in Designer**:
   - Add new screen in TouchGFX Designer
   - Configure transitions and interactions

2. **Implement Custom Logic**:
   ```cpp
   // In gui/include/gui/new_screen/NewView.hpp
   class NewView : public NewViewBase {
   public:
       void setupScreen();
       void handleKeyEvent(uint8_t key);
   };
   ```

3. **Update Application**:
   ```cpp
   // In FrontendApplication.cpp
   void FrontendApplication::gotoNewScreen() {
       transition<NewView>();
   }
   ```

## Building and Testing

### Build Process

```bash
# Clean build
rm -rf Apps/Alarm-CMake/build
cmake -S Apps/Alarm-CMake -B Apps/Alarm-CMake/build
cmake --build Apps/Alarm-CMake/build

# Check output
ls -la Output/
# Should contain MyAlarm.uapp
```

### Simulator Testing

The TouchGFX GUI includes a simulator for desktop testing:

```bash
# From TouchGFX-GUI directory
cd Apps/TouchGFX-GUI/simulator/gcc
make
./simulator
```

### Hardware Deployment

1. **Build Release**:
   ```bash
   cmake --build Apps/Alarm-CMake/build --config Release
   ```

2. **Install via USB/BLE**:
   - Connect watch to development machine
   - Use companion app or SDK tools to install `MyAlarm.uapp`

### Debugging

- **Service Logs**: Check kernel logs for service process output
- **GUI Simulator**: Test UI interactions on desktop
- **Hardware Testing**: Use debugger for real-time inspection

## Troubleshooting

### Common Issues

**Build Failures**:
- Verify `UNA_SDK` environment variable is set
- Check CMake version (3.21+ required)
- Ensure all dependencies are installed

**TouchGFX Issues**:
- Regenerate code after Designer changes
- Check TouchGFX installation and version compatibility
- Verify assets are properly imported

**Runtime Problems**:
- Check memory usage doesn't exceed limits
- Verify IPC messages are properly formatted
- Test on hardware early in development

### Performance Optimization

- **Memory**: Monitor heap usage in service and GUI processes
- **Power**: Use efficient timing for background checks
- **Storage**: Minimize JSON size for alarm persistence

## Key Technical Concepts

### Dual-Process Architecture
- Service handles time-critical operations
- GUI provides responsive user interface
- IPC enables safe communication between processes

### MVP Pattern in TouchGFX
- **Model**: Data management and business logic
- **View**: UI rendering and user input
- **Presenter**: Coordination between Model and View

### Memory-Constrained Development
- 256KB RAM limit for Activity apps
- Efficient data structures and algorithms
- Careful resource management

### Event-Driven Design
- Timer-based alarm checking
- Message-based inter-process communication
- Touch event handling in GUI

## Next Steps

With the Alarm app as your foundation, explore:

1. **Enhanced Features**: Add alarm labels, gradual volume increase, weather integration
2. **UI Improvements**: Custom animations, themes, accessibility features
3. **Advanced Scheduling**: Sunrise/sunset times, location-based alarms
4. **Integration**: Calendar sync, smart home connectivity
5. **New Apps**: Create stopwatch, timer, or reminder applications

## Resources

- [SDK Setup](sdk-setup.md) - Environment and build system
- [API Reference](api-reference.rst) - Complete SDK documentation
- [Platform Overview](platform-overview.md) - Hardware capabilities
- [TouchGFX Documentation](https://support.touchgfx.com/) - GUI framework guide
