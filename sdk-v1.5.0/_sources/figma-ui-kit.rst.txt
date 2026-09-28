Figma UI Resource Pack
=======================

The UNA Watch UI Resource Pack is the design source behind the watch GUI, shipped as a single Figma file: ``Docs/Templates/Figma-UI-Kit/UNA-Watch-UI-Resource-Pack.fig``.

It contains the watch mockups, the round screen frame with the four button labels in place, brand assets, and the UI elements the built-in apps are drawn from. Designing an app screen on top of these keeps a third-party app visually consistent with the pre-installed ones — same screen geometry, same button semantics, same visual language.

:download:`Download the resource pack <Templates/Figma-UI-Kit/UNA-Watch-UI-Resource-Pack.fig>`

Importing the pack
-------------------

1. Open Figma (a free account is enough — no paid tier or plugin is required).
2. Go to the **File browser** and choose **Import**, or drag the ``.fig`` file onto an open Figma project.
3. Duplicate the frame you want to start from and lay out your own screen inside it.

From Figma to the app
----------------------

The watch renders what the TouchGFX Designer generates, so a Figma screen is a layout reference, not an asset that ships as-is. Two rules keep the hand-off cheap:

* **Draw in Figma, rebuild in Designer.** Text, boxes, lines, and arcs are cheaper and sharper as TouchGFX primitives than as exported bitmaps, and they can be moved or recolored at runtime. Export a bitmap only for artwork that primitives cannot express.
* **Export bitmaps at 1x, PNG.** Keep the pixel size the screen actually uses; the color format is picked when the image is imported into Designer. See :doc:`Tutorials/Images/ARCHITECTURE` for how images are added to an app.

Ready-made TouchGFX containers that already implement several of these UI patterns in code are documented in :doc:`touchgfx-widgets`.
