# Alarm - Wearable Alarm Clock Application

## Overview

The Alarm app is a comprehensive alarm clock application designed for wearable devices, specifically targeting time-based notifications and scheduling. It provides users with the ability to set multiple alarms with flexible scheduling options, various notification effects, and persistent storage of alarm configurations.

The application follows a modular architecture with separate service and GUI components, communicating through a custom message system using the UNA SDK's kernel infrastructure. It supports features like one-time and recurring alarms, snooze functionality, multiple notification effects (beep, vibration, or both), and a TouchGFX-based user interface with intuitive time-setting wheels and navigation.

Key features include:
- Multiple alarm scheduling with flexible repeat patterns
- Time setting interface with wheel-based input
- Alarm effects (beep, vibration, or both)
- Snooze functionality with configurable delays
- Persistent storage of alarm settings in JSON format
- Touch-based navigation and configuration

## Architecture

The Alarm app follows a client-server architecture pattern where the service component handles all backend logic, time monitoring, and alarm triggering, while the GUI component manages user interaction and display. Communication between these components occurs through a message-based system using the UNA SDK's kernel infrastructure.

### High-Level Components

1. **Service Layer**: Core business logic, time monitoring, alarm processing
2. **GUI Layer**: TouchGFX-based user interface, screen management
3. **SDK Integration**: Kernel, file system, messaging
4. **Data Persistence**: JSON-based alarm storage

### Component Interaction

```
[Hardware Clock] <-> [Time Monitoring] <-> [Service]
       ^                    ^                ^
       |                    |                |
[File System] <-- [Persistence] --> [Alarm Manager]
       ^                    ^                ^
       |                    |                |
[Kernel Messages] <-- [Message System] --> [GUI]
```

The service runs as a separate process/thread, continuously monitoring time and checking for active alarms. The GUI runs on the TouchGFX framework, handling user input and displaying alarm information received from the service.

## Service Backend

The service backend is implemented in `Service.hpp` and `Service.cpp`, providing the core functionality for alarm management and time monitoring.

### Core Classes and Structures

#### Service Class

The main service class inherits from `AlarmManager::AlarmCallback` to receive alarm events and notifications.

```cpp
class Service : public AlarmManager::AlarmCallback {
public:
    Service(SDK::Kernel &kernel);
    virtual ~Service();
    void run();
private:
    // Implementation details
};
```

#### Key Data Structures

**Alarm Data Structure:**
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

**Snoozed Alarm Structure:**
```cpp
struct SnoozedAlarm {
    AppType::Alarm info;                    ///< Original alarm info
    uint8_t snoozeCount = kMaxSnoozeCount;  ///< Count of remaining snoozes
    uint8_t nextTriggerHour = 0;            ///< Next hour for the alarm trigger
    uint8_t nextTriggerMinute = 0;          ///< Next minute for the alarm trigger
};
```

### Alarm Management

The service manages alarm operations through the `AlarmManager` class, which handles time monitoring, alarm checking, and persistence.

#### Time Monitoring and Alarm Checking

The alarm checking process runs every minute during active operation:

```cpp
uint32_t AlarmManager::execute(const std::tm& tmNow) {
    checkAlarms(static_cast<uint8_t>(tmNow.tm_hour),
        static_cast<uint8_t>(tmNow.tm_min),
        static_cast<uint8_t>(tmNow.tm_wday), tmNow);

    // Calculate time until next minute (when next alarms can trigger)
    uint32_t nextCheckMs = (60 - tmNow.tm_sec) * 1000;
    return nextCheckMs;
}
```

**Alarm Checking Logic:**
```cpp
void AlarmManager::checkAlarms(uint8_t currentHour, uint8_t currentMinute, uint8_t currentDay, const std::tm& tmNow) {
    // Check active alarms from mAlarms (only enabled ones)
    for (auto& alarm : mAlarms) {
        if (!alarm.on) continue;  // Skip disabled alarms

        if (alarm.timeHours == currentHour && alarm.timeMinutes == currentMinute &&
            isAlarmDueToday(alarm, currentDay)) {
            if (!isSnoozed(alarm)) {
                if (mObserver) {
                    mObserver->onAlarm(alarm);
                }
                addSnoozedAlarm(alarm, tmNow);

                // Immediately disable one-time alarms after triggering
                if (alarm.repeat == AppType::Alarm::REPEAT_NO) {
                    auto alarmIt = std::find(mAlarms.begin(), mAlarms.end(), alarm);
                    if (alarmIt != mAlarms.end() && alarmIt->on) {
                        alarmIt->on = false;
                    }
                }
            }
        }
    }

    // Check snoozed alarms and remove completed ones
    auto it = mSnoozedAlarms.begin();
    while (it != mSnoozedAlarms.end()) {
        if (it->snoozeCount > 0 &&
            it->nextTriggerHour == currentHour &&
            it->nextTriggerMinute == currentMinute) {
            it->snoozeCount--;

            if (it->snoozeCount > 0) {
                if (mObserver) {
                    mObserver->onAlarm(it->info);
                }
                updateSnoozedTriggerTime(*it, tmNow);
                ++it;
            } else {
                // Snooze exhausted - remove from snoozed list
                if (mObserver) {
                    mObserver->onAlarm(it->info);
                }
                it = mSnoozedAlarms.erase(it);  // Remove from snoozed list
                // Note: One-time alarms are already disabled when first triggered
            }
        } else {
            ++it;
        }
    }

    if (needSave) {
        saveTofile(mAlarms);
        if (mObserver) {
            mObserver->onListChanged(mAlarms);
        }
    }
}
```

#### Snooze Management

The app implements snooze functionality with configurable delays (5 minutes by default, maximum 5 snoozes):

```cpp
void AlarmManager::addSnoozedAlarm(const AppType::Alarm& alarm, const std::tm& tmNow) {
    SnoozedAlarm snoozed;
    snoozed.info = alarm;
    snoozed.snoozeCount = kMaxSnoozeCount;
    updateSnoozedTriggerTime(snoozed, tmNow);

    mSnoozedAlarms.push_back(std::move(snoozed));
}

void AlarmManager::updateSnoozedTriggerTime(SnoozedAlarm& snoozed, const std::tm& tmNow) {
    // Calculate next trigger time by adding snooze minutes to current time
    uint16_t totalMinutes = static_cast<uint16_t>(tmNow.tm_hour * 60 + tmNow.tm_min + kSnoozedTimeMinutes);

    // Handle day overflow (wrap around at 24:00)
    if (totalMinutes >= 24 * 60) {
        totalMinutes -= 24 * 60;
    }

    snoozed.nextTriggerHour = static_cast<uint8_t>(totalMinutes / 60);
    snoozed.nextTriggerMinute = static_cast<uint8_t>(totalMinutes % 60);
}
```

### Data Processing Pipeline

The data processing pipeline handles alarm configuration updates and persistence:

#### 1. Alarm List Management

The service maintains a vector of alarms and provides methods for CRUD operations:

```cpp
const std::vector<AppType::Alarm>& AlarmManager::getAlarmList() {
    return mAlarms;
}

bool AlarmManager::saveAlarmList(const std::vector<AppType::Alarm>& list) {
    mAlarms = list;

    // Remove obsolete snoozed alarms when alarm list changes
    removeObsoleteSnoozedAlarms();

    saveTofile(mAlarms);

    if (mObserver) {
        mObserver->onListChanged(mAlarms);
    }

    return true;
}
```

#### 2. JSON Serialization

Alarm data is persisted using JSON format for human-readable storage:

```cpp
uint32_t AlarmManager::createJSON(const std::vector<AppType::Alarm>& alarms, char* buff, uint32_t buffSize) {
    SDK::JsonStreamWriter writer{ buff, buffSize };

    // Start root object
    writer.startMap();

    // Start "alarms" array
    {
        SDK::JsonStreamWriter::KeyedArrayScope alarmsArray{ writer, "alarms", alarms.size() };

        // Add each alarm to the array
        for (const auto& alarm : alarms) {
            SDK::JsonStreamWriter::MapScope alarmObj{ writer };

            writer.add("on", alarm.on);
            writer.add("time_h", alarm.timeHours);
            writer.add("time_m", alarm.timeMinutes);

            // Add repeat field with string value
            if (alarm.repeat < AppType::Alarm::REPEAT_COUNT) {
                writer.add("repeat", kRepeatJsonKeyValue[alarm.repeat].data());
            }

            // Add effect field with string value
            if (alarm.effect < AppType::Alarm::EFFECT_COUNT) {
                writer.add("effect", kEffectJsonKeyValue[alarm.effect].data());
            }
        }
    }

    // End root object
    writer.endMap();

    return strlen(buff);
}
```

#### 3. File System Integration

The app uses the UNA SDK's file system abstraction for persistent storage:

```cpp
bool AlarmManager::saveTofile(const std::vector<AppType::Alarm>& alarms) {
    // Create file object
    auto file = mKernel.fs.file(skFilePath);
    if (!file) {
        return false;
    }

    // Generate JSON
    size_t len = createJSON(alarms, mBuffer, sizeof(mBuffer));
    if (len > 0) {
        if (file->open(true, true)) {  // Create/truncate file
            size_t bw = 0;
            if (file->write(mBuffer, len, bw) && bw == len) {
                file->close();
                return true;
            }
            file->close();
        }
    }
    return false;
}
```

### Activity State Management

The service maintains alarm state and handles lifecycle events:

- **Active Monitoring**: Continuously checks time against alarm schedules
- **Snooze State**: Tracks snoozed alarms with remaining counts
- **Persistence**: Saves alarm list on changes
- **Cleanup**: Removes obsolete snoozed alarms when configurations change

### Settings and Persistence

Settings are stored in JSON format and include:
- Alarm on/off state
- Time configuration (hours, minutes)
- Repeat patterns
- Effect preferences

Activity data is persisted for configuration restoration across app restarts.

## GUI Implementation

The GUI is built using TouchGFX framework, providing a rich, animated interface for the alarm app.

### Model-View-Presenter Pattern

The GUI follows MVP architecture:

- **Model**: `Model.hpp/cpp` - Data management and service communication
- **View**: Various view classes (MainView, SetTimeView, etc.) - UI rendering
- **Presenter**: Presenter classes - Logic binding model and view

### Key GUI Components

#### Model Class

The Model class serves as the central data hub:

```cpp
class Model : public touchgfx::UIEventListener,
              public SDK::Interface::IGuiLifeCycleCallback,
              public SDK::Interface::ICustomMessageHandler {
public:
    void bind(ModelListener *listener);
    void tick();
    void handleKeyEvent(uint8_t c);
    // Alarm management methods
    void saveAlarm(size_t id, const AppType::Alarm& alarm);
    void deleteAlarm(size_t id);
    // ... additional methods
};
```

Key responsibilities:
- Lifecycle management (onStart, onResume, onSuspend, onStop)
- Message handling from service
- Alarm list management
- Time setting coordination

#### View Classes

**MainView**: Alarm list display with navigation
- Shows list of configured alarms
- Navigation between alarms using L1/L2 buttons
- R1 button for editing/creating alarms

**SetTimeView**: Time configuration screen
- Hour and minute wheel selectors
- Intuitive touch-based time setting

**ActionView**: Alarm configuration screen
- Toggle on/off state
- Select repeat patterns
- Choose notification effects

**AlarmView**: Active alarm display
- Large time display
- Snooze and dismiss options

### Message Handling System

The Model implements `ICustomMessageHandler` to receive asynchronous updates from the service:

```cpp
bool Model::customMessageHandler(SDK::MessageBase *msg) {
    switch (msg->getType()) {
        case CustomMessage::ALARM_LIST: {
            auto *cmsg = static_cast<CustomMessage::AlarmList*>(msg);
            mAlarmList = cmsg->list;
            modelListener->onAlarmListChanged();
        } break;

        case CustomMessage::ACTIVATED_ALARM: {
            auto *cmsg = static_cast<CustomMessage::ActivatedAlarm*>(msg);
            mActiveAlarm = cmsg->alarm;
            modelListener->onAlarmActivated(mActiveAlarm);
        } break;

        default:
            break;
    }
    return true;
}
```

### Screen Navigation and State Management

The app uses TouchGFX's screen management system with custom transitions:

**Screen Flow**:
```
Start Screen -> Main Screen -> Set Time Screen -> Action Screen -> Saved Screen
      |              |              |
      |              v              v
      |         Alarm Screen    Deleted Screen
      v
   (Exit App)
```

**Transition Types**:
- **No-transition calls**: Instant updates for data refreshes
- **Slide transitions**: Smooth animated screen changes

### Custom Containers Implementation

The app uses custom containers for reusable UI components:

**TimeWheel Container**:
```cpp
class TimeWheel : public TimeWheelBase {
public:
    void setHours(uint8_t hours);
    void setMinutes(uint8_t minutes);
    uint8_t getHours() const;
    uint8_t getMinutes() const;
};
```

**MenuWheel Container**:
```cpp
class MenuWheel : public MenuWheelBase {
public:
    void setItems(const std::vector<std::string>& items);
    void setSelectedIndex(uint8_t index);
    uint8_t getSelectedIndex() const;
};
```

### Input Handling Architecture

User input is processed through a hierarchical system:

1. **Hardware Events**: Button presses detected by TouchGFX HAL
2. **Key Event Processing**: Model::handleKeyEvent() for global actions
3. **Screen-Specific Handling**: View classes handle context-specific input

**Button Mapping**:
- **L1**: Previous item/navigation left
- **L2**: Next item/navigation right
- **R1**: Primary action (select/edit)
- **R2**: Secondary action (snooze)

### Data Formatting and Units

The GUI handles time formatting and display:

**Time Formatting**:
```cpp
std::string formatTime(uint8_t hours, uint8_t minutes) {
    char buffer[6];
    sprintf(buffer, "%02d:%02d", hours, minutes);
    return std::string(buffer);
}
```

**Alarm Display**:
- 12-hour or 24-hour format based on system settings
- AM/PM indicators for 12-hour mode
- Repeat pattern indicators
- Effect type visualization

## Build and Setup

The Alarm app uses CMake for cross-platform builds targeting embedded hardware and simulation.

### Build System Overview

**Primary Build File**: `CMakeLists.txt` in `Alarm-CMake/`

```cmake
# App configuration
set(APP_NAME "Alarm")
set(APP_USER_NAME "Alarm")
set(APP_TYPE "Activity")
set(DEV_ID "UNA")
set(APP_ID "123456789ABCDEF0")
set(RESOURCES_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../Resources")
set(OUTPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../Output")
set(LIBS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../Libs")

# Includes all build steps and required tools
include($ENV{UNA_SDK}/cmake/una-app.cmake)
include($ENV{UNA_SDK}/cmake/una-sdk.cmake)
include(${LIBS_PATH}/libs.cmake)
```

### Build Targets

**Service Build**:
```cmake
set(SERVICE_SOURCES
    ${LIBS_SOURCES}
    ${UNA_SDK_SOURCES_COMMON}
    ${UNA_SDK_SOURCES_APPSYSTEM}
    ${UNA_SDK_SOURCES_JSON}
)
set(SERVICE_INCLUDE_DIRS
    ${LIBS_INCLUDE_DIRS}
    ${UNA_SDK_INCLUDE_DIRS_COMMON}
    ${UNA_SDK_INCLUDE_DIRS_SERVICE}
)
una_app_build_service(${APP_NAME}Service.elf)
```

**GUI Build**:
```cmake
if(DEFINED TOUCHGFX_PATH)
    set(GUI_SOURCES
        ${TOUCHGFX_SOURCES}
        ${UNA_SDK_SOURCES_COMMON}
        ${UNA_SDK_SOURCES_GUI}
    )
    set(GUI_INCLUDE_DIRS
        ${LIBS_INCLUDE_DIRS}
        ${UNA_SDK_INCLUDE_DIRS_COMMON}
        ${UNA_SDK_INCLUDE_DIRS_GUI}
        ${TOUCHGFX_INCLUDE_DIRS}
    )
    una_app_build_gui(${APP_NAME}GUI.elf)
endif()
```

**Complete App**:
```cmake
una_app_build_app()
```

### Dependencies

**SDK Components**:
- UNA SDK common, service, and GUI sources
- File system interfaces
- Kernel and messaging systems

**External Libraries**:
- TouchGFX framework
- Custom app libraries (AlarmManager, etc.)

### Development Setup

**Prerequisites**:
- UNA SDK environment
- CMake 3.21+
- TouchGFX Designer
- Cross-compilation toolchain

**Build Commands**:
```bash
mkdir build
cd build
cmake -G "Unix Makefiles" ..
make
```

### Simulator Build

TouchGFX provides simulator builds for PC development:
- Visual Studio project for Windows
- Makefile for Linux
- Includes mock hardware interfaces

## Conclusion

The Alarm app demonstrates a robust implementation of an alarm clock application on wearable devices. Its modular architecture separates concerns effectively between service logic and user interface, enabling reliable time monitoring and alarm management.

Key architectural strengths include:
- **Separation of Concerns**: Clear division between service and GUI
- **Message-Based Communication**: Loose coupling between components
- **Flexible Scheduling**: Support for various repeat patterns
- **Rich UI Framework**: TouchGFX provides professional user experience
- **Data Persistence**: JSON-based storage ensures configuration survival

The implementation showcases advanced C++ patterns, real-time systems programming, and embedded GUI development. The app successfully integrates time monitoring, user interaction, and persistent storage while maintaining a clean separation of concerns.
