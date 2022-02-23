Deflections {#tg_deflections}
======================================

Mapping of Permanent Girder Deformations from Storage to Bridge Model Deflections
======================================

Girder Segments are not perfectly straight elements when placed at erection. They are shaped with permanent unrecoverable deformations caused by prestressing forces, creep, shrinkage, relaxation, girder hardening (increase in modulus of elasticity during storage), and precamber.

These unrecoverable deformations must be accounted for when segments are placed into PGSuper’s flat bridge analysis model (aka, the LBAM). The flat analysis model assumes that girders are initially perfectly straight with zero applied external forces prior to loading. Girders are placed on simple supports at the time of erection, so the second assumption is correct,
but unrecoverable deflections must be summed with flat model deflections when computing total deflections in the bridge. 

***Unrecoverable Deflections Due to Change in Modulus During Storage***

Changes in support locations between storage and erection induce changes in dead load deflections that are superimposed on the unrecoverable girder dead load deflection. For example: a segment may be supported 5 ft inboard from its ends during storage causing a dead load deflection for this configuration. During storage the concrete ages and the modulus of elasticity increases. When the segment is moved to its erected configuration potentially on different support locations: some, but not all of the initial dead load deflection is recovered. E.g., the erected configuration may be supported 1 ft inboard from the end. This additional deflection in the erected configuration due to the change in support location is superimposed with the unrecoverable deflection, resulting in the total dead load deflection in the erected configuration.

