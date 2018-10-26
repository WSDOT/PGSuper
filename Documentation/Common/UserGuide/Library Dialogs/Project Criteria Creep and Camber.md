Creep and Camber {#ug_library_dialogs_project_criteria_creep_and_camber}
==============================================
Define project criteria for creep and camber.

> NOTE: These parameters do not apply to time-step analysis

Item | Description
-----|---------------
Time from stressing to prestress transfer | Enter the time from stressing of the pretension strands to imparting the force onto the girder. This time is used to compute initial relaxation losses.
Time from prestress transfer until temporary strand removal/diaphragm casting | Enter the range of times when temporary strands will be removed and diaphragms cast
Time from prestress transfer until slab casting or application of superimposed dead loads on noncomposite girders | Enter the range of time for this loading condition
Total Creep Duration | Enter the total creep duration. Only used for bridges without composite decks.
Variability between upper and lower bound camber | Enter the amount of camber variability that is to be expected.

> NOTE: Creep is assumed to stop once the deck becomes composite with the girder

> NOTE: There is a natural variability in camber. PGSuper predicts an upper bound camber. The camber variability factor reduces the predicted camber by a percentage to estimate a lower bound value.

Curing of Precast Concrete
-----------------------------
Used for estimating losses for project criteria based on AASHTO LRFD 3rd Edition with 2004 interims and earlier.
