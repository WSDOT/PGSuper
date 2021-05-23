Grade 300 7-wire Prestress Strand {#tg_grade_300}
======================================
Designers and owners are constantly looking for ways to extend span lengths. Grade 300 prestressing strand is a fairly new product and provides another tool in the engineer's toolbox. Grade 300 strand can provide 10% more precompression force compared to Grade 270 for the same number and size of strands.

The AASHTO LRFD Bridge Design Specifications are limited to Grade 270 strand (see LRFD 5.4.4.1). **For this reason, support for Grade 300 strand is considered to be experimental. Please take the necessary time to understand the material, it's properties, how it performs, and how it's accounted for in this software**.

## Mechanical Properties ##
Research conducted by Carroll, et. al. investigated the use of Grade 300 prestressing strand in pretensioned, prestressed concrete beams. The research concluded current AASHTO LRFD provisions provide acceptable estimates of transfer length, flexural bond length, flexural strength, and relaxation losses of pretensioned, prestressed concrete members containing Grade 300 prestressing strand. The AASHTO LRFD provisions are therefore applied to Grade 300 strand without modification in this software. The reseach suggests the modulus of elasticity of Grade 300 strand my be somewhat higher than the design value provided in AASHTO LRFD. This software uses the standard design modulus of elasticity as defined by AASHTO.


[Carrol, 2017, "The use of Grade 300 prestressing strand in pretensioned, prestressed concrete beams", PCI Journal 62(1): 49-65](https://doi.org/10.15554/pcij62.1-01)

## Power Formula ##
\ref tg_moment_capacity analysis uses the strain compatibility method. Industry accepted material models for Grade 300 strand have not yet been adopted. The properties of Grade 300 prestressing strand was investigated by Loflin. This study included an experimentally derived stress-strain model using the power formula for three Grade 300 strand samples. The power formula is

\f[ 
f_{ps} = \epsilon_{ps} \left[ A + \frac{B}{ (1+(C \epsilon_{ps})^D)^{\frac{1}{D}}} \right] \leq f_{pu}
\f]

Until industry accept values for coefficients A, B, C, and D are adopted, the average of the coefficients determined by the three strand tests conducted by Loflin are used in this software. See Table 4.19 of Loflin.

\f[
A = 263, B = 33811, C = 120.2, D = 5.347
\f]

[Loflin, Bryan, 2008, "Bond and Material Properties of Grade 270 and Grade 300 Prestressing Strand", Masters Thesis, Virginia Polytechnic Institute and State University](http://hdl.handle.net/10919/33838)
