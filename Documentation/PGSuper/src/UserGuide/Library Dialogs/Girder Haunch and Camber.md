Haunch and Camber {#ug_library_dialogs_girder_haunch_and_camber}
==============================================

Haunch Design Parameters
-----------------------
These parameters control how the slab haunch is designed and evaluated in specification checks.

Item | Description
-----|-------------
Minimum Fillet | Enter the minimum permissible depth from the bottom of the slab to the top of the girder.
Minimum Required Haunch Depth at Bearing Centerlines | When checked, the haunch depth at bearing centerlines is compared to the input value.
Excessive Haunch Depth Warning Tolerance | Defines the tolerance for issuing a warning that the haunch depth is too deep. A deep haunch depth is not a structural problem, but you may be wasting material.

> TIP: Refer to @ref tg_slab_offset in the @ref technical_guide for more information about how the slab offset and fillet dimensions are used for slab offset design.

Deflection Multipliers for Computing Camber
-----------------------------------------
Enter deflection multipliers for computing camber. See @ref tg_camber in the @ref technical_guide for specific information about how camber is computed.

Item | Description
-----|-------------
Erection Camber | Multiplier on the camber at erection. This camber includes the dead load, prestress, and creep during storage deflections.
Creep | Multiplier on all other creep deflections
Diaphragm + Shear Key + Construction | Multiplier on deflections caused by diaphragm, shear key, and construction loads
Deck Panel | Multiplier on deflections caused by deck panel dead loads
Slab + User1 | Multiplier on deflections caused by the cast slab and user defined loads applied to the noncomposite girder section
Haunch | Multiplier on the slab haunch dead load
Barrier + Sidewalk + Overlay + User2 | Multiplier on deflections caused by the traffic barrier, sidewalk, overlay, and user defined loads applied to the composite girder section
