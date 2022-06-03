Deflections {#tg_deflections}
======================================
Deflections are computed using linear elastic analysis and superposition of results.

Unrecoverable Deflections
-------------------------
Pretensioned girders are not perfectly straight elements when erected. They are shaped with permanent unrecoverable deformations caused by prestressing forces, creep, shrinkage, relaxation, girder hardening (increase in elastic modulus during storage), and precamber.

These unrecoverable deformations must be accounted for when girders are erected into the final bridge structure. Girder self-weight deflections at erection are often computed by hand as \f$ \Delta = \frac{5wL^4}{384E_cI}\f$. This computation does not take into considerations deflections that occur prior to erection and the change in the concrete elastic modulus with time.

A girder is initially loaded by self-weight at release. The deflection is \f$ \Delta = \frac{5wL^4}{384E_{ci}I}\f$. Over time, the elastic modulus increases from \f$E_{ci}\f$ to \f$E_c\f$. When the support conditions change due to lifting, hauling, and erection, the dead load moment, curvature, and deflection change as well. These changes are related to the elastic modulus at the time of the change. Moving the support points inwards partially unloads the girder. Since the elastic modulus has increased, some of the initial deflection is not elastically recovered. This unrecoverable deflection prior to erection must be accounted for in the final structure.

Changes in support locations between storage and erection induce changes in dead load deflections that are superimposed on the unrecoverable girder dead load deflection. As an example, consider a girder that is supported 5 ft inboard from its ends during storage causing a dead load deflection for this configuration. During storage the concrete ages and the modulus of elasticity increases. When the girder is moved to its erected configuration to different support locations, say 1 ft inboard from the ends, some but not all of the initial dead load deflection is recovered. This additional deflection in the erected configuration due to the change in support location is superimposed with the unrecoverable deflection, resulting in the total dead load deflection in the erected configuration.

The change in deflection may be small for the example above, however the effect of unrecoverable deflection can magnify when lifting and hauling support locations are taken into consideration. For very long girders, support locations can be 10%-15% of the girder length inboard from the ends.

PGSuper computes deflections at erection for a straight beam and then adds the unrecoverable deflections for easier validation with hand calculations. After erection results are presented with and without the unrecoverable deflection.


