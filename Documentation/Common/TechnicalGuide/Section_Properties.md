Section Properties {#section_properties}
======================================
PGSuper ignores the slab haunch and fillets when calculating composite section properties.

![](CompositeProperties.gif)

The reason this is done is two-fold. This provides the least-stiff section so it is conservative for computing stresses and deflections, and it makes structural modeling easy because it we have a prismatic section. Also, if camber in the field comes out too high, we may not actually have the slab pad we designed for at mid-span. 

Dead loads are based on the full slab pad depth (slab offset or "A" dimension) project across the entire girder. If camber comes out lower than expected, the girder design will still be adequate for the increased pad depth required to achieve the design roadway profile.

