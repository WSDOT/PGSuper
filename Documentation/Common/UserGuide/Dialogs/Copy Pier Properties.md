Copy Pier Properties {#ug_dialogs_copy_pier_properties}
==============================================
Often Piers in a structure may be detailed identically, or to share some select properties. Copy Pier Properties provides a convenient method for copying all, or selected, properties from one Pier to another pier in the structure.

Copy From
--------------------
Select the source Pier who's properties you wish to copy

Copy To
--------------------
Select the target Pier (or All Piers) To which the properties will be copied. 

Select Properties to be Copied
------------------------------
Click the check boxes to select which properties are to be copied. 

Note that individual properties are disabled when "All" is selected. You must click the "All" check box in order to select individual items again. The types of properties that can be selected are shown below. 

Property | Description
---------|-------------
All Properties | Copy all pier input properties listed below, including Pier type, from the selected From Pier
Connection Properties | Copy Pier connection geometry and boundary conditions. Note that boundary conditions will not be copied in PGSplice, or if the selected "From" pier's boundary condition is not compatible with the target.
Diaphragm Properties | Copy diaphragm dimensions and loading data
Pier Model Properties | Copy the model type (Idealized or Physical), and all dimensions, column layout, and material properties if source is a Physical pier.

> NOTE: Haunch properties ("A" dimension) on piers are not copied are reported on this dialog. This can be done from the Edit->Haunch dialog.

Comparison Report
-----------------
This report provides an input echo of permanent pier properties. Values on the selected "To" pier (highlighted in yellow) are compared all other piers in the bridge. The "Same as From" column in the report indicates whether a pier's input data is the same as the selected "From" pier.

Buttons
-------

Control Buttons | Description
----------------| -----------
[Copy Now] | Press this button to copy the properties. 
[Close] | Close this window without copying properties
[Help] | Opens this help topic.

