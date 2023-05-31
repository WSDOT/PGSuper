General {#ug_dialogs_pier_description_general}
==============================================
The general properties of piers and abutments are described on this tab. Piers and abutments are described by an arbitrary reference line called the Pier Line and the Abutment Line, respectively. This line is completely described by its location along the horizontal alignment and its orientation. The Abutment/Pier Line can be located at the Back of Pavement Seat, Centerline of Bearing, or any other line of your choosing. It is best to choose the reference line based on the information given on the layout sheet of your bridge plans.

Abutment/Pier Line
-------------------
Define the location and orientation of the Abutment/Pier Line

Item | Description
-----|-----------
Station | Enter the location of the reference line as a station along the horizontal alignment. If the station is changed from a previously defined value, use the drop down list to specify how the bridge is to be adjusted when this pier is moved from one station to another.
Orientation | Enter the orientation of the reference line. The orientation may be entered as the keyword "NORMAL", a bearing, or a skew angle.
Erection Event | Select the event when this pier/abutment is erected. (PGSplice only)

Haunch Geometry
---------------
Define the geometry of the slab haunch

> A complete description of vertical bridge geometry, haunch design, and haunch input methods can be found at @ref tg_vertical_geometry

Different options are shown depending on the selected haunch input method

###"A" and Assumed Excess Camber Method (PGSuper Only)###

Item | Description
-----|-------------
Slab Offset ("A" Dimension) | Enter the slab offset dimension for the pier.
Assumed Excess Camber <sup>+</sup>  | Enter the excess camber that is used to define the parabolic haunch depth used for computing composite section properties and/or haunch dead load. Note that this option is only available if the parabolic option is selected for computing composite section properties or haunch loading in the current Project Criteria library entry.

###Explicit Haunch Depth input method###

Item | Description
-----|-------------
Haunch Depth | Haunch depth is shown when applicable

###Edit Haunch Description....###
Click on this button to bring up the @ref ug_dialogs_haunch
