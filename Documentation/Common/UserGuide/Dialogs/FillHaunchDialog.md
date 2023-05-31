Fill Haunch Input with Computed Values {#ug_dialogs_fill_haunch}
==============================================

Select Girder to be Modified
-------------
> Girderline is pre-selected prior to opening this dialog by parent window.

Select the span(s) or segment(s) that are to be modified by this dialog.

There are two diffent fill options:

Compute Haunch Depths....
--------------------
Design a haunch new layout based on an analysis based on current bridge data. This will attempt to find an optimal haunch layout for the selected girder based on current haunch data.

> Note that design of the haunch is an iterative process. The design algorithm will compute a better design if the current data is closer to the optimal design. It may require multiple runs to converge on a satisfactory solution.

A different girderline other than that to be modified can be selected if desired.

Design haunch depths can be distributed at 1/10th points or 1/4 points along spans or segments.

> Tip: Select 1/10th points for a more accurate design. You can always change the haunch distribution later in the haunch editing dialog.

Add a Scalar Value to the Current Haunch Design
---------------------------------------------
Enter a value to be added to all current haunch depths for the selected girder. This number can be positive or negative.

> Tip: This feature is useful for shrinking the overall haunch depth when fine-tuning a successful design