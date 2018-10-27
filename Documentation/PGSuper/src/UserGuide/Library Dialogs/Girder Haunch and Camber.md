Haunch and Camber {#ug_library_dialogs_girder_haunch_and_camber}
==============================================

Haunch Design Parameters
-----------------------
These parameters control how the slab haunch is designed and evaluated in specification checks.

Item | Description
-----|-------------
Standard Minimum Fillet | Enter the minimum permissible depth from the bottom of the slab to the top of the girder. Note that this value is only applicable to spread spaced girders. For adjacent giders, the minimum fillet is always assumed to be zero.
Minimum Required Haunch Depth at Bearing Centerlines | When checked, the haunch depth at bearing centerlines is compared to the input value.
Excessive Haunch Depth Warning Tolerance | Defines the tolerance for issuing a warning that the haunch depth is too deep. A deep haunch is not a structural problem, but may be be a sign of wasted material.

> TIP: Refer to @ref tg_slab_offset in the @ref technical_guide for more information about how the slab offset and fillet dimensions are used for slab offset design.

> TIP: Refer to @ref tg_structural_analysis_models in the @ref technical_guide for more information about how the haunch load is applied.


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

Precamber
------------
The precamber limit identifies the maximum precamber that can be built into this type of girder. The input precamber is evaluated against this criteria. Input precamber that is not within +/- of the precamber limit will be flagged as a specification check failure.

Item | Description
-----|-------------
Precamber Limit | Enter the precamber limit in the form L/n. Example L/80.

> NOTE: Work with your local fabricators to estabilish reasonable precamber limits. Set this value to 0 if girders cannot be precambered.