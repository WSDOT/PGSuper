Lightweight Concrete {#lightweight_concrete}
======================================
Lightweight concrete was added to PGSuper Version 2.4. The features are compatible back to LRFD 1st Edition 1994. Lightweight concrete is defined by LRFD 5.2 as *concrete containing lightweight aggregate and having and air-dry unit weight not exceeding 0.120 kcf*. 

Input of a unit weight is not enough to fully define lightweight concrete. Lightweight concrete can be classified as *all lightweight* and *sand lightweight*. All-lightweight concrete contains lightweight sand and course aggregate. Sand lightweight contains normal weight sand and lightweight course aggregate.

AASHTO defines normal weight concrete as having a unit weight between 0.135 and 0.155 kcf. This leaves unclassified types with unit weights between 0.120 and 0.135 kcf and greater than 0.155 kcf. PGSuper permits concrete definitions to use any unit weight. You decide how PGSuper will treat your concrete definitions by classifying the concrete as Normal Weight, Sand Lightweight, and All Lightweight. PGSuper will select the applicable equations from the AASHTO LRFD based on your concrete classification.

> NOTE: AASHTO LRFD 7th Edition, 2016 Interim revisions change the classification of concrete to simply normal weight and lightweight.

The material properties of lightweight concrete vary greatly with the aggregate source and mix design. Modulus of elasticity, creep, and shrinkage are significant material properties that generally need to account for aggregate source. Concrete material definitions include aggregate correction factors for averaging and bounding. See [Concrete Properties](@ref concrete_properties) for more information.
