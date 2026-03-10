Longitudinal Reinforcement {#ug_library_dialogs_girder_longitudinal_reinforcement}
==============================================
This tab allows you to define the longitudinal reinforcement layout seed data for a girder. 

> Note that longitudinal rebar data from the library is copied to all girders when a girder type is first selected while editing the bridge model. The data can be subsequently overridden in the girder editor. Users can also re-copy the data from the library.

Refer to the @ref tg_longitudinal_reinforcement section in the @ref technical_guide for a discussion of how, and when, longitudinal rebars are used in analysis.

General
---------------

Item | Description
----------|------------
Mild Steel Material | Select the appropriate mild steel for longitudinal rebar

Rows of Longitudinal Mild Steel
----------------------------------
PGSuper allows you to describe an unlimited number of horizontal rows of longitudinal reinforcement. Each row can be full-length or a partial-length segment along the girder length. Rows of reinforcement must lie within the girder cross section. The following data is used to describe rows of longitudinal reinforcement.

Item | Description
-------|-------------
Measured From | Defines the datum for locating the start point of the reinforcing bar. Segments that extend beyond the end of the girder are truncated at the end of the girder. Full Length and Mid-Girder-End segments grow and shrink as the girder length changes. ![](LongRebarOptions.gif)
Distance from End | Defines the location of the start point of the reinforcing bar.
Bar Length | Defines the length of the bar
Girder Face | Rebar rows may be measured either from the top or bottom face of the girder.
Cover | Enter the distance between the specified (top or bottom) girder face to the outside nominal diameter for the specified bar.
Bar Size | Select from the available standard bar sizes that may be used for longitudinal rebar.
\# of Bars | Specify the number of bars to be placed in the row.
Spacing | Enter the center-to-center rebar spacing.
[Insert Row] | Inserts a new row of reinforcing at the current location in the grid
[Delete Row] | Deletes the selected row of reinforcement
[Append Row] | Appends a row of reinforcement at the bottom of the girder

> Note that cover and minimum bar spacing requirements are not checked by the program. It is your responsibility to insure that bars do not overlap or violate pertinent rebar detailing requirements.


> All cells in the grid must be filled in. Blank cells are considered incomplete data.  

Check For Equality Between Library and Project Longitudinal Reinforcement Input
---------------------------------------------
Some agencies, such as WSDOT and TxDOT, have standardized input for longitudinal reinforcement. If this is the case, it is useful to generate a warning in output reports if project data does not exactly match library data. Enable the checkbox if a warning is desired.