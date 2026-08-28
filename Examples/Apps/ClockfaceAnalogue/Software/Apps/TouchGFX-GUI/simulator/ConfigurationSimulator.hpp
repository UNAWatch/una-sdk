/**
 ******************************************************************************
 * @file    ConfigurationSimulator.hpp
 * @date    27-August-2026
 * @author  Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief   What the simulated sensor layer feeds this app.
 *
 * The face subscribes to one sensor, so only the battery is enabled here; the
 * rest are switched off to keep the simulator from producing data nobody
 * reads. Every macro still has to be defined -- the shared simulator wires all
 * of them into its parameter setters regardless of the enable flags.
 ******************************************************************************
 */

#ifndef CONFIG_SIMULATOR_HPP
#define CONFIG_SIMULATOR_HPP

// GPS sensor
#define GSP_SIM_ENABLE               0
#define GSP_SIM_SPEED_MIN            0
#define GPS_SIM_SPEED_BASE           0
#define GPS_SIM_SPEED_MAX            0
#define GPS_SIM_TIME_SEACH_SATELLITE 0

// Heart-rate sensor
#define HEAT_RATE_SIM_ENABLE        0
#define HEAT_RATE_SIM_MIN_HR        0
#define HEAT_RATE_SIM_MAX_HR        0
#define HEAT_RATE_SIM_TYPE_TRAINING 0

// Pressure sensor
#define PRESSURE_SIM_ENABLE       0
#define PRESSURE_SIM_PRESS_VALLUE 0.0f

// Battery level sensor.
//
// Starting just inside the top band and falling 0.2 %/s crosses into the next
// one about twenty seconds in, which is what makes the indicator observably
// follow the sensor rather than merely show something on boot.
#define BATT_LEVEL_SIM_ENABLE      1
#define BATT_LEVEL_SIM_START_VALUE 78
#define BATT_LEVEL_SIM_STEP_VALUE  0.2f

// IMU wrist-detect sensor
#define IMU_WRIST_SIM_ENABLE           0
#define IMU_WRIST_SIM_WRIST_DETECT_KEY '5'

// IMU step counter
#define IMU_STEP_COUNTER_SIM_ENABLE        0
#define IMU_STEP_COUNTER_SIM_STRIDE_LENGTH 0.0f

#endif // CONFIG_SIMULATOR_HPP
