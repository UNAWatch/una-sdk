# App Config JSON

## Revision History

| Revision | Date of Changes | Matter of Change | Note | Editor |
|----------|-----------------|------------------|------|--------|
| 1.00    | 7.08.2025      | Creating        |      | Denys Saienko |

## Output Package

The watch application package is generated using the UNA SDK. It is provided as a ZIP archive accompanied by a GPG signature, which ensures the authenticity and integrity of the entire archive and its contents. This mechanism can be used for content validation and to verify the identity of the application provider — for example, to confirm whether an application is UNA-verified.

### Package Content

The package includes the following files:

- A binary application file (`*.uapp`)
- A JSON file with application metadata and configuration (`config.json`)
- An icon image (`icon.png`) in PNG format
- A folder (`previews`) containing application preview images (multiple PNG screenshots)
- A folder (`assets`) with subdirectories (`icons` and `previews`):
  - `assets/icons`: Icons of the application and widgets
  - `assets/previews`: Application preview images (several screenshots in PNG format)
- A signature file (`SIGNATURE.asc`)

### Config File

The `config.json` configuration file provides general metadata about the application and defines the structure, capabilities, and custom metrics supported by the watch app.

This file is intended solely for defining supported features and displaying them in the mobile application. It should not contain runtime information or settings for the watch and should not be uploaded to the watch during application installation.

All information needed by the watch is also embedded in the `*.uapp` file.

Simplest `config.json` for [Files](Tutorials/Files/ARCHITECTURE.md) Tutorial

```json
{
  "type": [
    "activity"
  ],
  "name": "Files Tutorial",
  "icon": "Resources/icon_60x60.png",
  "binary": "Files_0.1.3-16-ed77913-dirty.uapp",
  "appVersion": "0.1.3",
  "minSdkVersion": "0.1.3",
  "requiredHardware": [],
  "stravaExport" : false,
  "id": "03AD5A741E38A35F",
  "supportsLaps": false,
  "supportsDistance": false,
  "supportsTrack": false,
  "supportsHeartbeat": false,
  "supportsElevation": false,
  "supportsStep": false,
  "supportsSpeed": "none",
  "customMeasures": []
}
```

Watch application config example:

```json
{
  "type": [
    "activity", // Type of the app: activity, clockface, glance, or utility
    "glance" // The app can combine several types
  ],
  "name": "Running", // Name of the activity
  "icon": "assets/icons/running.png", // Relative path to the activity icon
  "binary": "running.uapp", // Relative path to the app binary file
  "previews": "assets/previews/", // Relative path to the previews images
  "appVersion": "1.0.3", // Current version of the app
  "minSdkVersion": "3.0.0", // Minimum supported SDK version required to run the app
  "requiredHardware": [ // List of hardware features required for the app to function
    "GPS",
    "ACCELEROMETER",
    "HR"
  ],
  "description": "Running is a real-time tracking app for your runs. It displays current speed, pace, distance, and heart rate. Designed for athletes and casual runners alike, this app helps monitor and improve your training performance.",
  "supportsLaps": true, // Whether lap counting is supported
  "supportsDistance": true, // Whether distance tracking is supported
  "supportsTrack": true, // Whether GPS track/map is supported
  "supportsHeartbeat": true, // Whether heart rate tracking is supported
  "supportsElevation": true, // Whether elevation data is supported
  "supportsStep": true, // Whether step counter data is supported
  "supportsSpeed": "pace", // Type of speed metric: "none", "speed", or "pace"
  "customMeasures": [ // List of additional metrics shown during or after the activity
    {
      "id": "calories", // Unique identifier of the measure
      "title": "Calories Burned", // Display title for the user
      "icon": "assets/icons/calories.png", // Icon shown in the UI for this measure
      "unitMetric": "kcal", // Unit in the metric system
      "unitImperial": "kcal", // Unit in the imperial system
      "unitScalingFactor": 1.0, // Scaling factor to convert from metric to imperial
      "isTimeBased": true, // True if the value changes over time (e.g., one per second)
      "visualisation": "line-chart", // Visualization type: "line-chart", "number", or "gauge"
      "preview": true, // Whether this measure should be shown in the activity preview
      "previewAggregation": "average" // Aggregation type for preview if time-based: "average", "min", "max"
    },
    {
      "id": "distance_to_goal",
      "title": "Distance to Goal",
      "icon": "assets/icons/distance_goal.png",
      "unitMetric": "km", // Example: kilometers
      "unitImperial": "mi", // Example: miles
      "unitScalingFactor": 0.621371, // Converts kilometers to miles
      "isTimeBased": false, // Only one value per activity
      "visualisation": "gauge",
      "preview": true,
      "previewAggregation": "average"
    },
    {
      "id": "training_load",
      "title": "Training Load",
      "icon": "assets/icons/load.png",
      "unitMetric": "pts",
      "unitImperial": "pts",
      "unitScalingFactor": 1.0,
      "isTimeBased": false,
      "visualisation": "number",
      "preview": true
    },
    {
      "id": "cadence",
      "title": "Running cadence",
      "icon": "assets/icons/cadence.png",
      "unitMetric": "s/min",
      "unitImperial": "s/min",
      "unitScalingFactor": 1.0,
      "isTimeBased": true,
      "visualisation": "line-chart",
      "preview": true,
      "previewAggregation": "average"
    },
    {
      "id": "speed_max",
      "title": "Maximum speed",
      "icon": "assets/icons/speedmax.png",
      "unitMetric": "km/h", // Example: kilometers
      "unitImperial": "mi/h", // Example: miles
      "unitScalingFactor": 0.621371, // Converts kilometers to miles
      "isTimeBased": false,
      "visualisation": "number",
      "preview": false,
      "previewAggregation": "average"
    },
    {
      "id": "hr",
      "title": "Heart rate",
      "icon": "assets/icons/hr.png",
      "unitMetric": "b/min",
      "unitImperial": "b/min",
      "unitScalingFactor": 1.0,
      "isTimeBased": true,
      "visualisation": "line-chart",
      "preview": false,
      "previewAggregation": "average"
    }
  ],
  "stravaExport": true // Whether the activity can be exported to Strava
}
```

## Activity Data Format

Correspondence of metrics in the config file to data in the CBOR activity report.

### General Format

General format for describing metrics for non-time-based values:

```
"metric_id": [value_in_lap_0, value_in_lap_1, ... value_in_lap_n]
```

If the app does not support splitting the track into laps, then there will be only one value in the array:

```
"metric_id": [value]
```

General format for describing metrics for time-based values:

```
"metric_id": [[time_0, value_0], [time_1, value_1], ... [time_n, value_n]]
```

Where:

- `time_x` - Time in seconds since the start of the session activity (not lap!)
- `value_x` - Value at time `time_x`

### Predefined Metrics

#### supportsLaps

For the "supportsLaps" metric, the start of each lap is indicated in seconds from the start of the session.

```
"lap_start_time": [0, 900, 1800, 2700] // 4 laps
```

#### supportsDistance

The "supportsDistance" metric specifies the distance of each lap.

```
"distance": [200, 203, 206, 209]
```

#### supportsTrack

The "supportsTrack" metric is time-based, it specifies the time in seconds since the start of the session activity (first value), latitude and longitude (second and third values).

```
"track": [
  [ // first point
    0, // time
    52.22977066040039, // latitude
    21.01177978515625 // longitude
  ],
  [ // second point
    10,
    52.329769134521484,
    21.111780166625977
  ],
  ...
  [ // last point
    3590,
    88.12976837158203,
    56.911781311035156
  ]
]
```

#### supportsHeartbeat

The "supportsHeartbeat" metric indicates the average heart rate of each lap.

```
"heartbeat": [80, 81, 82, 83]
```

#### supportsElevation

The "supportsElevation" metric shows the change in elevation on each lap.

```
"elevation": [1, 4, 7, 10]
```

#### supportsStep

The "supportsStep" metric indicates the number of steps in each lap.

```
"step": [500, 520, 540, 580]
```

#### supportsSpeed

The "supportsSpeed" metric shows the average speed for each lap.

```
"speed": [20, 21, 22, 23]
```

### Custom Metrics

#### Custom Non-Time-Based Metric

See General format.

Example for 4 laps:

```
"custom_metric_id": [value_0, value_1, value_2, value_3]
```

Example for 1 lap or if "supportsLaps": false:

```
"custom_metric_id": [value]
```

#### Custom Time-Based Metric

See General format.

Example for 5 points:

```
"custom_metric_id": [[time_0, value_0], [time_1, value_1], [time_2, value_2], [time_3, value_3], [time_4, value_4]]
```

## Activity Data File Example

Example of track activity data saved in a CBOR file that matches the config above. Total track time – 1 hour. 4 laps of 15 minutes each.

```json
{
  "start_time": 1752871238,
  "duration": 3600,
  "lap_start_time": [0, 900, 1800, 2700],
  "distance": [200, 203, 206, 209],
  "track": [
    [0, 52.22977066040039, 21.01177978515625],
    [10, 52.329769134521484, 21.111780166625977],
    [20, 52.429771423339844, 21.211780548095703],
    ...
    [3590, 88.12976837158203, 56.911781311035156]
  ],
  "speed": [20, 21, 22, 23],
  "step": [500, 520, 540, 560],
  "heartbeat": [80, 81, 82, 83],
  "elevation": [1, 4, 7, 10],
  "calories": [
    [0, 40],
    [60, 41],
    [120, 42],
    ...
    [3540, 44]
  ],
  "training_load": [30, 37, 44, 51],
  "cadence": [
    [0, 60],
    [10, 61],
    [720, 62],
    ...
    [3540, 63]
  ],
  "speed_max": [22, 23, 24, 25],
  "hr": [
    [0, 80],
    [10, 81],
    [20, 82],
    ...
    [3590, 439]
  ]
}
```
