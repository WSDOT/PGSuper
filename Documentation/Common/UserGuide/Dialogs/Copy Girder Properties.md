Copy Girder Properties {#ug_dialogs_copy_girder_properties}
==============================================
It is common practice for precast girder bridge designs to have all girders detailed identically, or to share some identical select properties. Copy Girder Properties provides a convenient method for copying all, or selected, properties from one girder to other girders in the structure.

Copy Properties From
--------------------
Select the source girder who's properties you wish to copy

Copy Properties To
--------------------
Select the target girder(s) To which the properties will be copied. Target girders can be selected with either of two options. Note that in PGSplice, you can only copy within the same group.

Option | Description
-------|------------
Single Girder, Span, Girderline or All | Using the drop down lists to select  the target girders
Multiple Girders (PGSuper only) |  Press [Select Girders] to select several target girders

Select Properties to be Copied
------------------------------
Click the check boxes to select which properties are to be copied. 

Note that individual properties are disabled when "All" is selected. You must click the "All" check box in order to select individual items again.

The types of properties that can be selected are shown below. Prestressing and longitudinal rebar properties cannot be copied between girders of different types.

Property | Description
---------|---------------
All Girder Properties | Copy all input properties, including girder type, from the selected From girder
Concrete Material Properties | Copy girder concrete material properties
Prestressing | Copy both pretensioned and post-tensioning, if present. Prestressing strand information (quantities, locations, jacking forces, debonding, etc.) will be copied. For bridges using multiple types of girders, prestressing information can only be copied between girders of the same type.
Longitudinal Reinforcement | Copy longitudinal reinforcement
Transverse Reinforcement | Copy transverse shear reinforcement
Temporary Conditions | Copy location of support locations and release and storage, lifting loops, and transportation support points

Comparison Report
-----------------
This report provides an input echo of properties for all girders in the bridge. Values on the selected "To" girder (highlighted in yellow) are compared all other girders in the bridge. The "Same as From" column in the report indicates whether a girder's input data is the same as the selected "From" girder.

Buttons
----------
Button | Description
----------------| -----------
[Copy Now] | Press this button to copy the properties. 
[Close] | Close this window without copying properties
[Help] | Opens this help topic.
[Print] | Print the Comparison Report

