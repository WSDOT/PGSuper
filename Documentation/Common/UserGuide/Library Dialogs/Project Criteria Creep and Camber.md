Creep and Camber {#ug_library_dialogs_project_criteria_creep_and_camber}
==============================================
Define project criteria for creep and camber.

> NOTE: These parameters do not apply to time-step analysis

Item | Description
-----|---------------
Time from stressing to prestress transfer | Enter the time from stressing of the pretension strands to imparting the force onto the girder. This time is used to compute initial relaxation losses.
Time from prestress transfer until temporary strand removal/diaphragm casting | Enter the range of times when temporary strands will be removed and diaphragms cast
Time from prestress transfer until deck casting or application of superimposed dead loads on noncomposite girders | Enter the range of time for this loading condition
Total Creep Duration | Enter the total creep duration. Only used for bridges without composite decks.
Variability between upper and lower bound camber | Enter the amount of camber variability that is to be expected.

> NOTE: Creep is assumed to stop once the deck becomes composite with the girder

> NOTE: There is a natural variability in camber. PGSuper predicts an upper bound camber. The camber variability factor reduces the predicted camber by a percentage to estimate a lower bound value.

Curing of Precast Concrete
-----------------------------
Used for estimating losses for project criteria based on AASHTO LRFD 3rd Edition with 2004 interims and earlier.

Assumed Excess Camber and Haunch
--------------------------------
> NOTE: These parameters do not apply to PGSplice, where the assumed excess camber is always zero.

#### Allowable tolerance between assumed and computed excess camber ####
Enter a tolerance that will be used to compare the assumed excess camber as described above, to the computed excess camber. The specification check will fail if difference in the assumed and predicted excess camber is not within tolerance.

A failed specification check indicates that the slab haunch dead load and/or section properties used in design and analysis are not consistent with the predicted shape of the actual slab haunch. This can result in an inaccurate analysis.

Use a reasonable value for the tolerance (i.e., a value near zero will make it practically impossible to pass the specification check).

#### Haunch Dead Load ####
Select the method for computing the haunch dead load. 

Method | Description
-------|------------
Zero excess camber | The top of the girder is assumed to be straight (zero camber) for purposes of computing the haunch dead load.
Excess camber is defined by a parabola fitting the Slab Offset and Assumed Excess Camber dimensions | The top of the girder is assumed to follow a parabolic curve defined by the slab offsets at the ends of the girder, and the user-input Assumed Excess Camber dimension. A specification check is performed to insure that the assumed excess camber is within tolerance of the computed excess camber.

> TIP: Refer to the Slab Haunch loading section of @ref tg_structural_analysis_models for detailed information about how the slab offset and assumed excess camber dimensions are used for computing the haunch dead load.

#### Composite Section Properties ####
Select the method used to determine the effects of haunch on composite section properties. Properties include those used when computing section stiffness, stresses, moment capacities for positive moment, shear capacities and live load distribution factors.

> NOTE: The actual slab haunch, computed using roadway surface geometry and girder camber, is alway used when computing negative moment capacity.

Method | Description
-------|------------
Ignore haunch depth | The haunch is ignored and the slab lies directly on the top of the girder.
Constant haunch depth equal to the Fillet value | The haunch depth is constant along the girder and equal to the Fillet dimension.
Variable haunch depth... | Haunch depth varies along the girder, and is defined by a parabola fitting the Slab Offset and Assumed Excess Camber dimensions. The top of the girder is assumed to follow a parabolic curve defined by the roadway geometry, slab offsets at the ends of the girder, and the user-input Assumed Excess Camber value. A specification check is performed to insure that the assumed excess camber is within tolerance of the computed excess camber.

> TIP: Refer to @ref tg_section_properties in the Technical Guide for detailed information about haunch depth is used when computing composite section properties.

> TIP: Refer to the @ref technical_guide for more information about how camber is computed.
