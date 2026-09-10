/**
 ******************************************************************************
 * @file    ConfigurationSimulator.hpp
 * @brief   Which simulated sensors the Smile face's simulator brings up.
 ******************************************************************************
 *
 * Every macro still has to be defined -- the shared simulator wires all of
 * them into its parameter setters regardless of the enable flags.
 *
 * @note Two of the four readings the face shows cannot be simulated. The
 *       simulated pedometer serves STEP_COUNTER (since boot), not
 *       STEP_COUNTER_DAILY, and there is no simulated ACTIVITY_TIME_DAILY
 *       sensor at all, so the step and active-minute rows stay on their
 *       placeholders here. Both are exercised on the watch instead.
 */

#ifndef CONFIG_SIMULATOR_HPP
#define CONFIG_SIMULATOR_HPP

// GPS sensor
#define GSP_SIM_ENABLE               0      // The face draws no position
#define GSP_SIM_SPEED_MIN            0      // km/h
#define GPS_SIM_SPEED_BASE           0      // km/h
#define GPS_SIM_SPEED_MAX            0      // km/h
#define GPS_SIM_TIME_SEACH_SATELLITE 0      // seconds

// Heart-rate sensor. A resting band, so the row shows a plausible rate and
// moves often enough to prove the path without ever looking like exercise.
#define HEAT_RATE_SIM_ENABLE        1
#define HEAT_RATE_SIM_MIN_HR        60      // max 255
#define HEAT_RATE_SIM_MAX_HR        90      // max 255
#define HEAT_RATE_SIM_TYPE_TRAINING 0       // 0 = Cycling, 1 = Hiking, 2 = Running

// Pressure sensor
#define PRESSURE_SIM_ENABLE       0
#define PRESSURE_SIM_PRESS_VALLUE 0.0f

// Battery level sensor. Starting just inside the top band and falling 0.2 %/s
// crosses into the next one about twenty seconds in, which is what shows the
// indicator swapping artwork rather than merely having some.
#define BATT_LEVEL_SIM_ENABLE      1
#define BATT_LEVEL_SIM_START_VALUE 78       // 10-100 %
#define BATT_LEVEL_SIM_STEP_VALUE  0.2f     // percent per second, falling

// IMU wrist-detect sensor
#define IMU_WRIST_SIM_ENABLE           0
#define IMU_WRIST_SIM_WRIST_DETECT_KEY '5'

// IMU step counter. Off: it serves STEP_COUNTER, which this face does not
// subscribe to, so enabling it would cost a driver and change nothing.
#define IMU_STEP_COUNTER_SIM_ENABLE        0
#define IMU_STEP_COUNTER_SIM_STRIDE_LENGTH 0.0f  // metres per step

// IMU running cadence sensor
#define IMU_RUNNING_CADENCE_SIM_ENABLE 0

#endif // CONFIG_SIMULATOR_HPP
