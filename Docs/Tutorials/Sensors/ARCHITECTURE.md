(tutorials/images/architecture)=
# Sensors Tutorial

In this tutorial we need to talk through how to subscribe to any of the
sensors to get a live view of the sensor on the screen.
Changing the frequency of the update is also explained e.g. update
every x seconds

## List of sensors:

• Live HR
• Live location (Lat/Long), turn gps onn/off
• Live elevation
• No BLE calibration at the moment but we should explain this is required for it to work properly
• Accelerometer G readings
    • In X, Y, and Z direction
• Step counter
• Floors
• Compass direction from magnetometer between 0°
• RTC time (just in UTC if that’s the format its in?)

## Implementation Details:
* This app is copied from Sensors
* Sensors subscriptions at maximum frequency 
* New messages structure to be added for each sensor with timestamps
* gui have single screen, batt state at top, and multiline text with each sensors dat, aL1 and L2 will switch verbosity of this multiline sensors, e.g, switch between which sensor data add to this text
* At bottom there is stats text field: service and gui cpu time in percents, bytes/sec of tx/rx messeges, tx/rx messeges/sec 
