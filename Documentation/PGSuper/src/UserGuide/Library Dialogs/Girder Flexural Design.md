Flexural Design {#ug_library_dialogs_girder_flexural_design}
==============================================
Parameters that control the flexural design of this girder type are defined on this tab. Parameters include debonding limits, location of debonding sections, and the strategies that are to be used for service limit state design.

Debonding Strand Limits (5.9.4.3.3 (*pre-2017: 5.11.4.3*))
----------------------------------
These parameters are used to check and design debonded sections to ensure that local stress concentrations at debond locations are excessive

Item | Description
------|---------
Maximum total debonded strands | Enter this value as a percentage of the total number of permanent strands
Maximum debonded strands per row | Enter this value as a percentage of the number of permanent strands in a row
Maximum debonded strands per section | These values limit the number of strands that can be debonded at a given longitudinal location. Enter both a total number and a percentage of the total number of debonded strands. The number of debonded strands will be limited to the greater of the two values.

Criteria for Debond Distances
-----------------------------
These parameters relate to the location of sections where debonding begins and terminates along the length of precast elements.

Item | Description
------|------------
Maximum Debond Length | The maximum distance from the end of a precast element to the farthest debonded section (at mid-length) cannot exceed lesser of: <ul><li>half the length of the precast element minus the strand development length computed per LRFD 5.9.4.3.3 (*pre-2017: 5.11.4.3*)</li><li>a specified percentage of the overall length of the precast element</li><li>a specified length</li></ul>. This value is used by the automated design algorithm only
Minimum distance between debond sections | The minimum distance between adjacent debond sections. As a rule of thumb, this length should typically be longer than the prestress transfer length. This criteria is evaluated by the spec checker and will result in a spec check fail if violated.
Default Debond Length | This value is used for design, and as an initial value in the debonding grid. The value cannot be less than the minimum length specified above.

Prestressing Optimization Strategies for the Automated Girder Designer
-----------------------------------------------------------------------
Use the controls in this group to direct PGSuper's prestress Girder Designer which strategies to use for the girder type in question. Strategies are attempted from top to bottom in the order shown on the dialog until a successful design is found, or all strategies are exhausted. 

Refer to Advanced Flexural Design - Multiple Prestress Optimization Strategies in the User Guide for more information
