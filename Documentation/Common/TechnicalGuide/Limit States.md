Limit States {#tg_limit_states}
======================================

General
-------
Limit states are defined by AASHTO LRFD 3.4.1. The load combination elements are summarized in the table below.

Load Combination | Contributing Forces
-----------------|---------------------
DC | Girder, diaphragms, slab, slab haunch, railing system (sidewalk and barriers), user defined DC loads
DW | Overlay, user defined DW loads
LL+IM | Vehicular live load, pedestrian live load, user defined LL+IM loads
CR | Creep and relaxation due to internal restraint forces and secondary forces.
SH | Shrinkage due to internal restraind forces and secondary forces
PS | Secondary forces from post-tensioning for strength limit states and total prestress forces for service limit states

Fatigue Limit State
--------------------
Prior to the LRFD 4th Edition with 2009 interims, LRFD 5.9.4.2.1 required a Service I compression stress evaluation for live load and one-half the sum of effective prestress and permanent loads. This requirement effectively changed the DC load factor in the Service I limit state from 1.0 to 0.5. To avoid confusion, we added a new combination named Service IA. The Service IA limit state combination was used to evaluate the compression stress limitation.

AASHTO LRFD 4th Edition, 2009 interim provisions moved the previously mentioned compression stress limitation from LRFD 5.9.4.2.1 to LRFD 5.5.3.1. The limit state combination used to compute the compression stress was changed from Service I using half permanent loads (which we called Service IA) to Fatigue I.

For all intents and purposes, the Service IA and Fatigue I limit state are the same.
