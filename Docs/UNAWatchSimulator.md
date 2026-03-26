# UNA-Watch Simulator

The simulator application works only on Windows OS. The UNA Simulator works with TouchGFX Designer and Visual Studio.

## Getting Started
### Need Components
1. Download and install [TouchGFX Designer](https://www.st.com/en/development-tools/touchgfxdesigner.html#get-software).
2. If you want to work with UNA Simulator in Visual Studio, then:
   - Download and install [Visual Studio](https://visualstudio.microsoft.com).
   - In the Installing window, where you select the required packages, go to **Workloads → Desktop & Mobile** and choose **Desktop development with C++**.
   - In **Installation Details**, for **Desktop development with C++** select **MSVC v143**, then click **Install**..
### Install UNA_SDK variable
1. Run  **RunAddVariable.bat**, located in:  
   `una-sdk\Utilities\Scripts\add-sdk-variable`
2. Enter **1** to install/update the **UNA_SDK** environment variable.
### Start Simulator
#### TouchGFX Designer
1. TouchGFX Designer does not support **Debug mode**. If you want to debug the project, use Visual Studio.
2. Open `<name>.touchgfx`, located in:  
   `<app_name>\Software\Apps\TouchGFX-GUI`
3. When opening the project for the first time, click **Generate Code (F4)**. This only needs to be done once.
4. Click **Run Simulator (F5)**.
#### Visual Studio
1. This step only needs to be done **once**. Open the application project in TouchGFX Designer and click **Generate Code (F4)**.
2. Go to:  
   `<app_name>\Software\Apps\TouchGFX-GUI\simulator\msvs`
3. Open the **Application.vcxproj** file.
4. Start debugging by pressing **F5**.
### Simulation 
#### Buttons
1. Mechanical buttons in the UNA Simulator are controlled using **keyboard keys**.
2. You can see the mapping in the **Terminal Log**, which shows which number keys correspond to the buttons on the UNA watch.
   ```cpp
   ---------------------------------------------------
   |   For Simulation Button use keybaord Keys.      |
   |       Keys Keybaord:                            |
   |       1   L1,                                   |
   |       2   L2,                                   |
   |       3   R1,                                   |
   |       4   R2                                    |
   |                  /---------\                    |
   |                 /           \                   |
   | BUTTON UP   L1 |             | R1 BUTTON SELECT |
   |                |     UNA     |                  |
   |                |    WATCH    |                  |
   | BUTTON DOWN L2 |             | R2 BUTTON BACK   |
   |                 \           /                   |
   |                  \---------/                    |
   ---------------------------------------------------
   ```
#### Buzzer
1. Buzzer actions are printed in the terminal as log messages. Example:
   ```cpp
   I- Mock.Buzzer::play::39               : [0] time=150 ms, level=3
   I- Mock.Buzzer::play::39               : [1] time=100 ms, level=0
   I- Mock.Buzzer::play::39               : [2] time=150 ms, level=3
   ```
2. Explanation:
   - **[]** — note array index
   - **time** — duration in milliseconds
   - **level** — sound level (1–3), 0 means no sound 
#### Backlight
1. Backlight actions are printed in the terminal as log messages.
   ```cpp
   I- Mock.Backlight::on::22              : on backlight, timeout = 5000
   I- Mock.Backlight::off::30             : off backlight
   ```
2. There are two types of log messages:
   - turning the backlight on with a timeout in milliseconds
   - turning the backlight off
#### Vibro
1. ibro actions are printed in the terminal as log messages.
   ```cpp
   I- Mock.Vibro::play::31                : play(melody[1])
   I- Mock.Vibro::play::34                :   [0] effect=1, loop=0, pause=0
   ```
2. Explanation:
   - **[]** — note array index
   - **effect** — vibration effect ID
   - **loop** — number of repetitions
   - **pause** — pause between effects
#### Sensors
In the file **ConfigurationSimulator.hpp**, you can enable/disable sensor simulation and configure parameters.
Location: 
`<app-name>\Software\Libs\Header`
##### GPS
- Simulates **speed, altitude, and distance**.
- The simulated user moves around a **stadium track**.
- Includes a timer for **satellite acquisition**.
- Can simulate **GPS signal loss**.
- Adds **noise to latitude/longitude**.
     
Configuration options:
   - enable/disable sensor
   - minimum and maximum speed
   - satellite search time
  ```cpp
   //GPS Sensor
   #define GSP_SIM_ENABLE               1  // 0 - Disable
   #define GSP_SIM_SPEED_MIN            20 // km/h
   #define GPS_SIM_SPEED_BASE           25 // km/h
   #define GPS_SIM_SPEED_MAX            30 // km/h
   #define GPS_SIM_TIME_SEACH_SATELLITE 7 // seconds
  ```
##### Heart Rate
Simulates:
   - **HR** (Heart Rate)
   - **AHR** (Average Heart Rate)
   - **RHR** (Resting Heart Rate)
   - **trust level**
     
Options:
   - enable/disable sensor
   - minimum and maximum heart rate
   - training type (**Cycling / Hiking / Running**)
  ```cpp
   //HeatRate Sensor
   #define HEAT_RATE_SIM_ENABLE        1 // 0 - Disable
   #define HEAT_RATE_SIM_MIN_HR        50
   #define HEAT_RATE_SIM_MAX_HR        140
   #define HEAT_RATE_SIM_TYPE_TRAINING 0// 0 - Cycling, 1 - Hiking, 2 - Running

  ```
##### Battery Level
Simulates battery voltage drop.

Options:
   - enable/disable sensor
   - initial value
   - decrease step
  ```cpp
   // Battery Level Sensor
   #define BATT_LEVEL_SIM_ENABLE      1 // 0 - Disable
   #define BATT_LEVEL_SIM_START_VALUE 100 // 10 - 100%
   #define BATT_LEVEL_SIM_STEP_VALUE  0.1 //percent 
  ```
##### IMU
Simulates **wrist detection**.

A wrist detection event activates the **backlight for 5 seconds**.

Options:
   - enable/disable sensor
   - change the key used to trigger the event

To simulate a wrist detection event, press **key 5**.
  ```cpp
   // IMU Sensor
   #define IMU_SIM_ENABLE           1 // 0 - Disable
   #define IMU_SIM_WRIST_DETECT_KEY 5 
  ```
### Include Header & Source file
#### TouchGFX Designer
1. Open **Application.vcxproj** in a text editor:
     `<app_name>\Software\Apps\TouchGFX-GUI\`
2. Add header paths to **ADDITIONAL_INCLUDE_PATHS**.
3. Add source file paths to **ADDITIONAL_SOURCES_UNA**.
#### Visual Studio
1. Open **MakeFile** in text editor:
  `<app-name>\Software\Apps\TouchGFX-GUI\simulator\gcc`
2. Add header paths to **ClInclude**.
3. Add source file paths to **ClCompile**.
### Transfer Application
If you move the application to another location, you need to update the **TouchGFX library path**.
1. Open `<name>.touchgfx` in text editor:
    `<app_name>\Software\Apps\TouchGFX-GUI`
2. Find **"TouchGfxPath":** and update the relative path to
   `<una_sdk>/ThirdParty/touchgfx.`
Example:
   ```cpp
    "SelectedStartupLanguage": "GB",
    "TouchGfxPath": "../../../../../../ThirdParty/touchgfx",
    "UIPath": ".",
   ```
4. If you move the **una_sdk** folder, you must update U**NA_SDK** environment variable.
   See: [Install UNA_SDK variable](#install-una-sdk-varible) 
