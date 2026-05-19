/**
 ******************************************************************************
 * @file    GonfigurationSimulator.hpp
 * @date    28-March-2026
 * @author  Vlad Andriyash
 * @brief   Gonfiguration Simulator file
 *
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __CONFIG_SIMULATOR_HPP
#define __CONFIG_SIMULATOR_HPP

#include <iostream>

//GPS Sensor
#define GSP_SIM_ENABLE               1  // 0 - Disable
#define GSP_SIM_SPEED_MIN            15 // km/h
#define GPS_SIM_SPEED_BASE           20 // km/h
#define GPS_SIM_SPEED_MAX            25 // km/h
#define GPS_SIM_TIME_SEACH_SATELLITE 4 // seconds

//HeatRate Sensor
#define HEAT_RATE_SIM_ENABLE        1 // 0 - Disable
#define HEAT_RATE_SIM_MIN_HR        120 // Max - 255
#define HEAT_RATE_SIM_MAX_HR        180 // Max - 255
#define HEAT_RATE_SIM_TYPE_TRAINING 1// 0 - Cycling, 1 - Hiking, 2 - Running

// Pressure Sensor
#define PRESSURE_SIM_ENABLE       1 // 0 - Disable
#define PRESSURE_SIM_PRESS_VALLUE 1020.2

// Battery Level Sensor
#define BATT_LEVEL_SIM_ENABLE      1 // 0 - Disable
#define BATT_LEVEL_SIM_START_VALUE 100 // 10 - 100%
#define BATT_LEVEL_SIM_STEP_VALUE  0.2 //percent 

// IMU Writs Sensor
#define IMU_WRIST_SIM_ENABLE           1 // 0 - Disable
#define IMU_WRIST_SIM_WRIST_DETECT_KEY '5' // char type 

// IMU StepCounter Sensor
#define IMU_STEP_COUNTER_SIM_ENABLE         1 // 0 - Disable
#define IMU_STEP_COUNTER_SIM_STRIDE_LENGTH  1 //meters per step. Walking 0.65–0.75 m, hiking 0.55–0.70 m, running 1.0–1.4 m

#endif /* __CONFIG_SIMULATOR_HPP */