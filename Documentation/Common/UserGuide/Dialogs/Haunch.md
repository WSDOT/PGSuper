Haunch Geometry {#ug_dialogs_haunch}
==============================================
The haunch geometry for the entire bridge can be defined in this window.

Fillet
------
Enter the depth of the slab fillet. A single Fillet value is defined for the whole bridge.

The Fillet is the least allowable haunch depth along a girder, and is used by PGSuper to compute the required “A” dimension during a specification check and design. Refer to @ref tg_slab_offset section of the Technical Manual for more information.

Haunch Shape
------------
Select the shape of the slab haunch. This setting is for graphical presentation purposes only

Slab Offset ("A" Dimension)
---------------------------
Use the drop down list to select the method of defining the slab offset.  Note that a single Slab Offset can be defined for the whole bridge, per bearing line, or per girder.

Enter the slab offset dimensions. Refer to @ref tg_slab_offset section of the Technical Manual for more information.

Assumed Excess Camber
--------------------------
Enter the excess camber that is used to define the parabolic haunch depth used for computing the slab haunch dead load. **Note that this option is only available if the parabolic haunch load option is selected in the current Project Criteria library entry. Also, it is not applicable for spliced girder bridges**.

Refer to  the Slab Haunch loading section of @ref tg_structural_analysis_models for more information.