Debonding {#ug_dialogs_girder_details_debonding}
==============================================
Define strand debonding and extensions.

Item | Description
-----|--------------
Strand Grid | This grid lists all of the strands modeled in the girder. Debonding and strand extension information is defined in this grid. See below for more information.
Symmetrical Debonding | When checked, debonding is the same at both ends of the girder.

### Strand Grid ###
The strand grid lists all of the strands modeled in the girder. Debonding and strand extension information is defined in this grid. 

Item | Description
-----|-----------
Strand # | Identifies a pair of strands, one on either side of the girder centerline. Refer to the girder image for strand location.
Debond | Check this box to model a strand as debonded
Debond Length Both Ends | Used for Symmetrical Debonding. Enter the debond length at both ends of the girder
Debond Length Left End | Enter the debond length at the left end of the girder
Debond Length Right End | Enter the debond length at the right end of the girder
Extend Left | Check this box to model a strand as extended at the left end of the girder
Extend Right | Check this box to model a strand as extended at the right end of the girder

> NOTE: Strands in rows without a check box in the Debond column cannot be debonded because they were not designated as "debond-able" in the girder definition.

> NOTE: Extended strands are considered to be fully developed along their entire length at the strength limit states.

> NOTE: Extended strands are only available if the "Allow Strand Extensions" option is enabled in the Project Criteria.
