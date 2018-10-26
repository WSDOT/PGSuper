Layout {#ug_dialogs_pier_description_layout}
==============================================
Select the pier model type.

An idealized pier model is defined by a simple idealized boundary condition as defined on the @ref ug_dialogs_pier_description_connections page.

A physical pier model is defined by the physical attributes of the pier including its material properties, cross beam, and column arrangement.

Material Properties
-------------------
Define the material properties of the cross beam and columns.

Item | Description
-----|--------------
f'c | Concrete strength
Ec | When checked, enter the modulus of elasticity of the concrete. Otherwise, the modulus is computed from the AASHTO equation
[More Properties...] | Edit additional concrete properties. Pre-defined concrete can be retrieved from the library using this option.

Pier Cap Dimensions
--------------------
Define the dimensions of the pier cab or cross beam.

Enter the dimensions is the various fields as defined by the schematic show on this tab.

Column Layout
--------------
Define the layout of the columns.

Item | Description
-----|-------------
Base of column | Define how the base of column is located.
Longit. Base Fixity | Define the fixity of the base of the column for longitudinal plane frame analysis.
Column Grid | Define the columns (see below for more information)
Locate the columns | Use these controls to transversely locate the columns (see below for more information)

### Column Grid ###
Define the columns in this grid

Item | Description
-----|-----------
Height | Height of the column
Bottom Elevation | Bottom of column elevation
Transv. Base Fixity | Define the fixity of the base of the column for transverse plane frame analysis. This analysis is not performed by PGSuper, however this information is used by the XBRate cross beam load rating application
Shape | Select the cross sectional shape of the column
Diameter/Width | Enter the diameter of a circular column or the width (dimension in the plane of the pier) of a rectangular column
Depth | Enter the depth (dimension normal to the plane of the pier) of a rectangular column
S | Enter the center to center column spacing

### Location of Columns ###
The columns must be located transversely in the plane of the pier.

Item | Description
-----|----------
Column | select a column to locate.
Offset | Enter the offset of the selected column from the selected datum
Datum | Select the datum from where the column location is measured.
