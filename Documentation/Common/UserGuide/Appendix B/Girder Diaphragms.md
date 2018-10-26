Diaphragms {#appendix_b_girder_diaphragms}
==============================================
PGSuper allows you to describe the rules to define the number of intermediate diaphragms used for given span lengths. An unlimited number of rules may be specified. Diaphragms are modeled for all rules that are applicable to a specific girder.

The loads due to intermediate diaphragms are included in the dead load analysis. The concrete for intermediate diaphragms is assumed to be the same as that used for the slab.

Intermediate Diaphragm Layout Rules
-----------------------------------
The diaphragm layout rules are listed in the grid. Use the [Add], [Edit], and [Delete] buttons to manage the rules. Double-clicking the mouse on a row will open the rule for editing. 

> Tip: It is wise to give the rules names that describe what the rule does for a given span range.


Example Diaphragm Layout Rules
------------------------------
* Span length between 0 and 40 feet - No intermediate diaphragms
* 40 to 80 feet - 1 diaphragm at mid-span
* 80 to 120 feet - 2 diaphragms at third points
* greater than 120 feet (input as 120 to 99999 feet) - 3 diaphragms at quarter points

<br>

* @subpage appendix_b_diaphragm_layout_rules
