General {#ug_dialogs_bridge_description_general}
==============================================
Define the general layout of the bridge.

Item | Description 
-----|--------------
Bridge Line Offset | Enter the offset from the alignment to the bridge line. 
Girder Family | Select a general type of girder from the drop down list
Girder Type | Select a specific type of girder. Check the box below the list if this girder type is to be used for the entire bridge.
Number of Girders |  Use the up/down arrows to select the number of girders. Check the box below if this number of girders is to be used in all spans.
Spacing Type | Select the spacing type to be used
Girder Spacing |  Enter the girder spacing, located the girders transversely, and select the measurement datum. See @ref ug_bridge_modeling in the @ref user_guide and @ref tg_girder_spacing in the @ref technical_guide for additional details.
Girder Connectivity | For adjacent girders, select the transverse connectivity. The girder connectivity influences the live load distribution factor calculations.
Girder Orientation | Select the girder orientation. Some girders are not aligned with their webs plumb. Girders can be oriented such that their top flanges parallel the roadway surface
Deck Type | Select the type of deck to be used

A description of the bridge, along with the assumed cross section type for live load distribution factor calculations, is given.

Bridge Line
-----------
The Bridge Line is an arbitrary line that parallels the alignment. This line can represent anything, though it is most commonly used for the centerline of the bridge. Many of the bridge geometry parameters, such as transverse location of girders and the edge of deck, can be located by an offset from the Alignment or the Bridge Line. 

More Information
----------------
The general layout of the bridge defined in this window is for a simple, uniform bridge with the same type of girder in all spans and the same girder spacing in all spans. More complex bridges can be defined. Each girder can be a different type, though all girders must be in the same family. That is, for example, you can use several different types of slab girders in a span, but you cannot mix slab girders with other girder types. Each span can also have a unique number of girders and unique girder spacing. See the following for additional information:

* @ref user_guide - @ref ug_bridge_modeling

