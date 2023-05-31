Haunch Geometry Dialog {#ug_dialogs_haunch}
==============================================
The haunch geometry for the entire bridge is defined in this window. 

> A complete description of vertical bridge geometry, haunch design, and haunch input methods can be found at @ref tg_vertical_geometry

Fillet
------
Enter the Fillet dimension

Haunch Shape
-----------
The haunch shape can have filleted or square corners. This value is used only for graphical display of the haunch.

Haunch Description Method
==========================
Select one of the three methods: 

-# ***Explicit Haunch Depths*** - Enter haunch depths at equal-spaced locations along spans or segments
-# ***Explicit Haunch+Slab Depths*** - Enter haunch plus slab depths at equal-spaced locations along spans or segments
-# ***"A" and Assumed Excess Camber*** - Enter Slab Offset and Assumed Excess Camber values.

Explicit Haunch Depths, and Explicit Haunch+Slab Depths Methods
---------------------------------------------
These two methods are identical except that the slab depth is included in the second method.

- Haunch depths can be distributed along Spans or Segments (PGSplice only)
- Distribution is at even spacings along the member between ends
- Depths can be entered Unformly, Linearly Between Ends, Parabollically, at 1/4 Points, or at 1/10th points

Both methods can define the haunch the same for every member in the bridge, same in each span or segment, or unique for every span or segment

> Tip: An automated design algorithm can be used to design Explicit haunch depths. The option is available from the "Fill Haunch Depth Input with Computed Values" option on the @ref ug_finished_elevations_graph window.

"A" (Slab Offset) and Assumed Excess Camber Method (PGSuper Only)
--------------------------------
Enter the Slab Offset dimensions.

Enter the excess camber that is used to define the parabolic haunch depth used for computing composite section properties and/or haunch dead load. Note that this option is only available if the parabolic option is selected for computing composite section properties or haunch loading in the current Project Criteria library entry.

Slab Offset input can define the haunch the same for all members in the bridge, same in each span, or unique for every span.

> TIP: Refer to @ref tg_slab_offset and the Slab Haunch loading section of @ref tg_structural_analysis_models for more information about how the slab offset and fillet dimensions are used for slab offset design,  haunch loading, and composite section properties.

> TIP: Drag the mouse to select multiple grid cells. Then the **Ctrl-C** and **Ctrl-V** keyboard commands can be used to copy values from or paste values to selected cells. Cells can be copy/pasted to and from Excel. Note that selected grid size and shapes must be compatible.