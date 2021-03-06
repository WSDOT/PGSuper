General {#ug_dialogs_span_description_general}
==============================================
Define general information about the span.

Span Length
------------

Item | Description
-----|-------------
Span Length | Enter the span length measured along the alignment between the abutment/pier lines at either end of the span.

The length of the span is changed by moving all the up-station piers.

Slab Offset ("A" Dimension)
---------------------------
Define the slab offset for the span.

Item | Description
-----|------------
Type | Click on the combo box to define the slab offset for the entire bridge or only for this span.
Start | Slab offset at the start of the span
End | Slab offset at the end of the span

If the slab offset is defined by span and you click the combo box to change the slab offset to be a single value for the entire bridge: the slab offset defined for this span will be applied to the entire bridge. If the start and end slab offsets are different, you will be prompted to select one value.

Assumed Excess Camber
---------------------
Enter the excess camber that is used to define the parabolic haunch depth used for computing composite section properties and/or haunch dead load. Note that this option is only available if the parabolic option is selected for computing composite section properties or haunch loading in the current Project Criteria library entry.

Refer to  the Slab Haunch loading section of @ref tg_structural_analysis_models for more information. Refer to @ref tg_section_properties in the Technical Guide for detailed information about haunch depth is used when computing composite section properties.
