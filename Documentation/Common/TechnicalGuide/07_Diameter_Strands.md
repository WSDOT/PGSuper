0.7" Diameter Strands {#tg_07_strands}
======================================
The AASHTO LRFD Bridge Design Specifications (as of 9th Edition) do not support 0.7" diameter strands. Significant research (NCHRP Project 12-109) has been accomplished on the implementation of 0.7" diameter strands in precast, prestressed concrete bridge girders. The research report has not yet been published.

PGSuper/PGSplice have basic support for 0.7" diameter strands. These larger strands are treated the same as all the other strand sizes. 

AASHTO T-10 committee is currently working on agenda item 223 (WAI 223) which will bring 0.7" diameter strands into the 10th Edition of the LRFD Specifications. WAI 223 is proposing revisions to confinement reinforcement detailing, adding a horizontal transverse tension tie in the bottom flange of I-Beams, and revised debonding requirements. These requirements have not been fully vetted at this time and they have not been incorporated into this software.

Some fabricators are resistent to using 0.7" strands due to their high stiffness. Local fabricators should be consulted before specifying 0.7" diameter strands.

One design option is to use 0.7" diameter straight strands and 0.6" diameter harped strands. PGSuper/PGSplice accounts for the difference in strand area when computing the location of the resultant prestress force and total strand eccentricity.