Section Properties {#tg_section_properties}
======================================
PGSuper ignores the deck haunch and fillets when calculating composite section properties.

![](CompositeProperties.gif)

The reason this is done is two-fold. This provides the least-stiff section so it is conservative for computing stresses and deflections, and it makes structural modeling easy because it we have a prismatic section. Also, if camber in the field comes out too high, we may not actually have the slab pad we designed for at mid-span. 


