/**
 ******************************************************************************
 * @file    ConfigurationSimulator.hpp
 * @brief   Simulated sensor settings for the Images LVGL PC simulator.
 *
 * Read by the SDK's simulated sensor layer (SDK/Libs/Source/Simulator).
 * Images uses no sensors, so the values only need to exist.
 ******************************************************************************
 */

#ifndef CONFIG_SIMULATOR_HPP
#define CONFIG_SIMULATOR_HPP

#include <iostream>

// GPS sensor
#define GSP_SIM_ENABLE               1  // 0 - Disable
#define GSP_SIM_SPEED_MIN            15 // km/h
#define GPS_SIM_SPEED_BASE           20 // km/h
#define GPS_SIM_SPEED_MAX            25 // km/h
#define GPS_SIM_TIME_SEACH_SATELLITE 4  // seconds until the first fix

// Heart rate sensor
#define HEAT_RATE_SIM_ENABLE        1   // 0 - Disable
#define HEAT_RATE_SIM_MIN_HR        120 // Max - 255
#define HEAT_RATE_SIM_MAX_HR        180 // Max - 255
#define HEAT_RATE_SIM_TYPE_TRAINING 2   // 0 - Cycling, 1 - Hiking, 2 - Running

// Pressure sensor
#define PRESSURE_SIM_ENABLE       1 // 0 - Disable
#define PRESSURE_SIM_PRESS_VALLUE 1020.2

// Battery level sensor
#define BATT_LEVEL_SIM_ENABLE      1   // 0 - Disable
#define BATT_LEVEL_SIM_START_VALUE 100 // 10 - 100%
#define BATT_LEVEL_SIM_STEP_VALUE  0.2 // percent

// IMU wrist sensor: press this key to raise a wrist-motion event
#define IMU_WRIST_SIM_ENABLE           1   // 0 - Disable
#define IMU_WRIST_SIM_WRIST_DETECT_KEY '5' // char type

// IMU step counter
#define IMU_STEP_COUNTER_SIM_ENABLE         1 // 0 - Disable
#define IMU_STEP_COUNTER_SIM_STRIDE_LENGTH  1 // metres per step

#endif /* CONFIG_SIMULATOR_HPP */
