General {#ug_library_dialogs_girder_general}
==============================================
The General Tab defines the basic girder type and the specific dimensions of the girder cross section.

Item | Description
----|-----
Girder Type | Select from the drop down list, the general type of girder cross section.
Girder Dimensions | Each of the girder dimensions are defined on the accompanying schematic. 
[View Section at Ends...] | Displays the basic girder section at the left end of the girder including the location of prestressing strands
[View Section at Mid-Span...] | Displays the basic girder section at the mid-point of the girder including the location of prestressing strands

Notes for Girders with End Blocks
-----------------------------
Solid end blocks can be added to I-Beams, U Beams, Box Beams, Decked Slab Beams, and Voided Slabs. The geometry of the end block is considered for dead load and all beam structural properties. End block geometry is not shown when the girder section is displayed by pressing [View Section at Ends...] or [View Section at Mid-Span...]

Notes for Box Girders
---------------------
Shear key loads can be automatically generated for adjacently-spaced box girders. The shear key will fill the gap between girders from the specified shear key depth to the tops of the beams and will have the same unit weight as the slab concrete. The load is applied in a load case called "Shear Key" applied in Bridge Site stage 1 into the DC combination. Results will only be displayed if a shear key is present. Structural stiffness properties of the shear key are not considered in any analyses.

> NOTE: PGSuper was written for bridges composed of typical prestressed girders. A clever user might be tempted to use these dimensions to create unique section shapes like Inverted T's, parallelograms, or even triangles. **Do not do this!** The program has not been tested for these types of shapes and may generate incorrect results.

