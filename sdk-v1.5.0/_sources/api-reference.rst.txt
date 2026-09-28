API Reference
===========================

This section provides detailed documentation for the UNA Watch SDK core interfaces, message system, and sensor layer.

.. contents:: Table of Contents
   :local:
   :depth: 2

Core SDK Interfaces
-------------------

The following interfaces provide the primary abstraction layer between the application and the watch kernel.

.. doxygenclass:: SDK::Interface::IKernel
   :project: SDK
   :members:

.. doxygenclass:: SDK::Interface::ISystem
   :project: SDK
   :members:

.. doxygenclass:: SDK::Interface::ILogger
   :project: SDK
   :members:

.. doxygenclass:: SDK::Interface::IFileSystem
   :project: SDK
   :members:

.. doxygenclass:: SDK::Interface::IAppMemory
   :project: SDK
   :members:

Application Communication (IPC)
-------------------------------

These interfaces and classes manage inter-process communication between the Service and GUI processes.

.. doxygenclass:: SDK::Interface::IAppComm
   :project: SDK
   :members:

.. doxygenclass:: SDK::MessageBase
   :project: SDK
   :members:

.. doxygenclass:: SDK::MessageGuard
   :project: SDK
   :members:

Sensor Layer
------------

The Sensor Layer provides type-safe access to hardware sensors and data parsers.

.. doxygenclass:: SDK::Interface::ISensorManager
   :project: SDK
   :members:

.. doxygenclass:: SDK::Sensor::Connection
   :project: SDK
   :members:

### Data Parsers

.. doxygenclass:: SDK::SensorDataParser::Accelerometer
   :project: SDK
   :members:

.. doxygenclass:: SDK::SensorDataParser::HeartRate
   :project: SDK
   :members:

.. doxygenclass:: SDK::SensorDataParser::GpsLocation
   :project: SDK
   :members:

.. doxygenclass:: SDK::SensorDataParser::BatteryLevel
   :project: SDK
   :members:

UI Framework
------------

Interfaces for building user interfaces using TouchGFX or LVGL, or the lightweight
Glance system.

For detailed information on the TouchGFX port implementation, see :doc:`TouchGFX-Port-Architecture`;
for the LVGL port, see :doc:`Tutorials/RunLVGL/ARCHITECTURE`.

.. doxygenclass:: SDK::Interface::IGlance
   :project: SDK
   :members:

.. doxygenclass:: SDK::GuiCommandProcessor
   :project: SDK
   :members:

``SDK::TouchGFXCommandProcessor`` is an alias of ``SDK::GuiCommandProcessor``,
kept for existing applications.

LVGL Widgets
~~~~~~~~~~~~

Drawing helpers and the widgets the UNA activity apps share, built from LVGL
primitives (``SDK/GUI/LVGL/``). Fonts and images are the app's: a widget that
draws text or an icon takes it as a ``const lv_font_t*`` or ``const lv_image_dsc_t*``.

.. doxygennamespace:: SDK::LVGL::Draw
   :project: SDK
   :members:

.. doxygenclass:: SDK::LVGL::Buttons
   :project: SDK
   :members:

.. doxygenclass:: SDK::LVGL::Title
   :project: SDK
   :members:

.. doxygenclass:: SDK::LVGL::ScrollIndicator
   :project: SDK
   :members:

.. doxygenclass:: SDK::LVGL::SensorStatusRow
   :project: SDK
   :members:

.. doxygenclass:: SDK::LVGL::Battery
   :project: SDK
   :members:

.. doxygenclass:: SDK::LVGL::TimerRing
   :project: SDK
   :members:

.. doxygenclass:: SDK::LVGL::Toggle
   :project: SDK
   :members:

.. doxygenclass:: SDK::LVGL::WheelMenu
   :project: SDK
   :members:

Utilities
---------

Common utility classes and helper functions.

.. doxygenclass:: CircularBuffer
   :project: SDK
   :members:

.. doxygenclass:: SDK::SwTimer
   :project: SDK
   :members:

.. doxygenclass:: CborStreamReader
   :project: SDK
   :members:

.. doxygenclass:: SDK::JsonStreamReader
   :project: SDK
   :members:
