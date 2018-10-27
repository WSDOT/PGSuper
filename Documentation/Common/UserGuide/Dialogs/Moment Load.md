Moment Load {#ug_dialogs_moment_load}
==============================================
Define a concentrated moment load. Moment loads can only be applied at the ends of girders. This is a useful load case for modeling restraint moments when required by LRFD 5.12.3.3.2 (*pre-2017: 5.14.1.4.2*).

Item | Description
----|-----
Load Case | Select the Load Case that the load is to be applied to. The available options are DC, DW and LL+IM as defined in the LRFD Specifications. LL+IM can only be applied in Event 5: Final with Live Load.
Event | Select the construction event when the load is first applied. In PGSuper all user-defined loads are permanent and their results are accumulated through all subsequent stages.
Span/Girder | Select the span and girder that the load is to be applied to. You can select "All Spans" or "All girders" to apply the load to more than one girder.
Location | Select the location along the girder where the load is to be applied. The load can be applied at the start or end of the girder.
Magnitude | Enter the magnitude of the load. Positive moments are counter clockwise 
Description | Enter a description for this load

