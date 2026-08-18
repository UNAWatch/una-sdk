/**
 ******************************************************************************
 * @file    ConfigurationSimulator.hpp
 * @brief   Which simulated sensors the host simulator provides to this app.
 * @details The sensor-layer simulator includes this header from the app, so
 *          every app that links it must supply one. Waypoint subscribes to
 *          GPS_LOCATION and nothing else, so only the GPS simulator is enabled;
 *          the rest stay defined but switched off, because the sensor sources
 *          are still compiled and read these values.
 *
 *          The simulated GPS walks a track at the speeds below after pretending
 *          to search for satellites, which is what makes the distance and
 *          bearing on screen change while you watch.
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef CONFIG_SIMULATOR_HPP
#define CONFIG_SIMULATOR_HPP

#include <iostream>

// GPS Sensor -- the one this app needs. Note the spelling of GSP_SIM_ENABLE:
// that is the name the SDK's simulator sources test, typo included.
#define GSP_SIM_ENABLE               1  // 0 - Disable
#define GSP_SIM_SPEED_MIN            4  // km/h
#define GPS_SIM_SPEED_BASE           5  // km/h
#define GPS_SIM_SPEED_MAX            6  // km/h
#define GPS_SIM_TIME_SEACH_SATELLITE 3  // seconds

// HeatRate Sensor
#define HEAT_RATE_SIM_ENABLE        0 // 0 - Disable
#define HEAT_RATE_SIM_MIN_HR        120 // Max - 255
#define HEAT_RATE_SIM_MAX_HR        180 // Max - 255
#define HEAT_RATE_SIM_TYPE_TRAINING 1// 0 - Cycling, 1 - Hiking, 2 - Running

// Pressure Sensor
#define PRESSURE_SIM_ENABLE       0 // 0 - Disable
#define PRESSURE_SIM_PRESS_VALLUE 1020.2

// Battery Level Sensor
#define BATT_LEVEL_SIM_ENABLE      0 // 0 - Disable
#define BATT_LEVEL_SIM_START_VALUE 100 // 10 - 100%
#define BATT_LEVEL_SIM_STEP_VALUE  0.2 //percent

// IMU Writs Sensor
#define IMU_WRIST_SIM_ENABLE           0 // 0 - Disable
#define IMU_WRIST_SIM_WRIST_DETECT_KEY '5' // char type

// IMU StepCounter Sensor
#define IMU_STEP_COUNTER_SIM_ENABLE         0 // 0 - Disable
#define IMU_STEP_COUNTER_SIM_STRIDE_LENGTH  1 //meters per step. Walking 0.65-0.75 m, hiking 0.55-0.70 m, running 1.0-1.4 m

// IMU Running Cadence Sensor
#define IMU_RUNNING_CADENCE_SIM_ENABLE      0 // 0 - Disable

#endif /* CONFIG_SIMULATOR_HPP */
