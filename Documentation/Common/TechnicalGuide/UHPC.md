Ultra High Performance Concrete (UHPC) {#tg_uhpc}
======================================
> **USE THESE EXPERIMENTAL FEATURES WITH EXTREME CAUTION**

This section describes experimental features to support Ultra High Performance Concrete (UHPC). UHPC is a relatively new material for precast, prestressed concrete bridge products. This material has been generally used for connections between adjacent precast members. Significant effort is underway in the precast industry to bring UHPC into the mainstream for precast bridge girder products.

> **USE THESE EXPERIMENTAL FEATURES WITH EXTREME CAUTION**

The features described in this section are experimental. They are subject to change and may be removed at any time. They may be incomplete, inaccurate, and incompatable with guides, specifications, and research reports that have been published or may be published in the future.

The AASHTO LRFD Bridge Design Specifications are applicable for bridges with specified concrete strength up to 15 ksi . UHPC is a high strength material with specified concrete strength in the range of 18-22 ksi. The equations, procedures, recommendations, and other related information from the AASHTO LRFD Bridge Design Specifications are applied to UHPC concrete members. The applicability of the AASHTO LRFD Bridge Design Specifications to UHPC concrete members has not been verified or validated. 

> **USE THESE EXPERIMENTAL FEATURES WITH EXTREME CAUTION**

There are many potential benefits of UHPC precast, pretensioned concrete bridge components. UHPC can lead to smaller, lighter, and more efficient girders. Experimental girders made from UHPC have thinner webs and flanges, greatly reducing weight. Web and flange buckling are typically not a concern for traditional precast girders. Web and flange buckling, and other behavior related to thin members, **are not evaluated**. **Additional analysis beyond the scope of this program may be required.**

> **USE THESE EXPERIMENTAL FEATURES WITH EXTREME CAUTION**

Because of fibers in UHPC, some tension capacity can be relied upon for shear strength. In some cases, UHPC girders can be constructed without shear stirrups. This translates to significant savings in labor and materials during girder fabrication.

> **USE THESE EXPERIMENTAL FEATURES WITH EXTREME CAUTION**

## General Concrete Properties
Concrete parameters are used in the AASHTO equations as if the UHPC concrete was modeled as normal weight concrete. You may want to adjust any or all of the following parameters, and possibly others, to modify how the AASHTO equations are evaluated.

Equation                    | Property              | Possible Modification                                        |
----------------------------|-----------------------|--------------------------------------------------------------|
5.4.2.3.2-1                 | Creep                 | The creep coefficient may be modified through the use of averaging and bounding factors applied to the AASHTO equations
5.4.2.3.2-3 and 5.4.2.3.3-2 | Humidity Factor       | UHPC tends to be impervious. Creep and shrinkage properties are generally not effected by humidity. A ficticous relative humidity can be defined to approximate approprate humidity factors for creep and shrinkage.
5.4.2.3.3-1                 | Shrinkage             | Shrinkage strain may be modified through the use of averaging and bounding factors applied to the AASHTO equations
5.4.2.4-1                   | Modulus of Elasticity | A modulus of elasticity approprate for UHPC may be defined by modifing the AASHTO equation with averaging and bounding factors, or an appropreate modulus of elasticity can be computed by other means and input into the program

## Transfer and Development Length
UHPC concrete has excellent bond characteristics. The transfer length for bare and epoxy coated prestressing strand in UHPC is taken to be 20 strand diameters. For development length, <span style="font-family:Symbol">k</span>, in LRFD Equation 5.9.4.3.2-1 is taken to be 0.3 for UHPC.

The development length for standard reinforcing bars are not adjusted for UHPC.

## Shear Capacity
Several modifications are made to shear capacity analyais for UHPC concrete.

1. <span style="font-family:Symbol">b</span> is always computed by LRFD Equation 5.7.3.4.2-1 and is limited to a maximum value of 4.8
2. <span style="font-family:Symbol">q</span> is limited to a minimum value of 29 degrees
3. LRFD Equation 5.7.3.3-1 for nominal shear capacity is modfied to 
![](.\Vn_UHPC.png) 
where 
![](.\Vf.png)

## Splitting Resistance
Fibers in UHPC contribute to splitting resistance. The splitting resistance defined by LRFD Equation 5.9.4.4.1-1 (*pre-2017: Eqn. 5.10.10.1-1*) is modifed to include the fiber contribution. Splitting resistance is taken to be: ![](.\Pr_UHPC.png)

## Girder Dimensions
AASHTO LRFD 5.12.3.2.2 (*pre-2017 5.14.1.2.2*) limits the thickness of the various parts of a precast concrete girders. Webs of girders made from UHPC are generally much thinner than webs of girders constructed with conventional concrete. Evaluation of web thickness is not performed for UHPC girders.