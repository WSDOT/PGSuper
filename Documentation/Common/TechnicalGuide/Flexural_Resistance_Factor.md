Flexural Resistance Factor {#tg_flexural_resistance_factor}
======================================
A generalized version of the flexure resistance factor defined in AASHTO LRFD 5.5.4.2 is used in this software. The resistance factor is a linear interpolation between the resistance factors for compression controlled and tension controlled sections. A linear interpolation is also made for partial prestress ratio between reinforced concrete and prestressed sections.

This article illustrates the derivation of the generalized resistance factor for flexure equation.

[et_image]:et.png
[dt_image]:dt.png
[phi_trc_image]:phi_trc.png
[phi_tps_image]:phi_tps.png
[phi_c_image]:phi_c.png
[phi_t_image]:phi_t.png

Given:
<table border=0>
<tr><td>![et_image]</td> 
    <td> = Net tensile strain in extreme tension steel at nominal resistance</td></tr>
<tr><td>![dt_image]</td>
    <td>= Distance from extreme compression fiber to the centroid of extreme tension steel</td></tr>
<tr><td>![phi_trc_image]</td>
    <td>= Resistance factor for a reinforced concrete tension controlled section</td></tr>
<tr><td>![phi_tps_image]</td>
    <td>= Resistance factor for a prestressed tension controlled section</td></tr>
<tr><td>![phi_c_image]</td>
    <td>= Resistance factor for a compression controlled section</td></tr>
<tr><td>![phi_t_image]</td>
    <td>= Resistance factor for a partial prestressed tension controlled section</td></tr>
<tr><td>PPR</td>
    <td>= Partial prestress ratio</td></tr>
</table>
 
![](ResistanceFactorGraph.png)

The net tensile strain is

![](net_tensile_strain.png)(Eq. 1)

The resistance factor for a partially prestressed tension controlled section is

![](PPR.png)(Eq. 2)

From the graph above, the resistance factor can be computed as

![](Eqn1.png)(Eq. 3)

Substituting Eq. 1 into Eq. 3 we get

![](Eqn2.png)(Eq. 4)

Simplify

![](Eqn3.png)(Eq. 5)

Substituting Eq. 2 into Eq. 5 gives

![](Eqn4.png)(Eq. 6)

