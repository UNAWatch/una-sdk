TouchGFX Widgets
=================

The UNA SDK ships a set of ready-made TouchGFX custom containers that cover the most common UI patterns for watch applications. Each widget is distributed as a ``.tpkg`` file — a standard TouchGFX Custom Container Package (ZIP archive) that can be imported directly into TouchGFX Designer.

Using the pre-built widgets
----------------------------

1. Open TouchGFX Designer.
2. Go to **Edit → Import → Custom Containers**.
3. Select the desired ``.tpkg`` file from ``Docs/Templates/TouchGFX-Widgets/``.

Exporting your own widgets
---------------------------

Any custom container in an existing project can be exported and reused in a new one. This is the recommended way to transfer app-specific widgets between projects or share them with other developers.

To export a container from TouchGFX Designer:

1. Open the project that contains the widget.
2. In the **Custom Containers** panel, right-click the container you want to export.
3. Select **Export** — Designer creates a ``.tpkg`` file with all required source files, assets, and fonts bundled inside.
4. Import the resulting ``.tpkg`` into any other project using the steps above.

You can modify the exported container freely: edit the C++ implementation, adjust the Designer layout, change assets, or extend the class before importing.

.. toctree::
   :maxdepth: 4
   :caption: 🎨 TouchGFX Widgets

   Templates/TouchGFX-Widgets/docs/Battery
   Templates/TouchGFX-Widgets/docs/Buttons
   Templates/TouchGFX-Widgets/docs/GpsIndicator
   Templates/TouchGFX-Widgets/docs/HeartRateZone
   Templates/TouchGFX-Widgets/docs/InfoCarousel
   Templates/TouchGFX-Widgets/docs/MainMenu
   Templates/TouchGFX-Widgets/docs/Map
   Templates/TouchGFX-Widgets/docs/PauseIndicator
   Templates/TouchGFX-Widgets/docs/ScrollIndicator
   Templates/TouchGFX-Widgets/docs/Texts
   Templates/TouchGFX-Widgets/docs/Title
   Templates/TouchGFX-Widgets/docs/Toggle
