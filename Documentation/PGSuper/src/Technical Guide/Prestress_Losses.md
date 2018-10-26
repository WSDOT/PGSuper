Prestress Losses (General) {#tg_prestress_losses}
======================================
The methods for predicting prestress losses have changed several times since the first edition of the LRFD Bridge Design Specifications. PGSuper supports the loss prediction methods, including refined and approximate methods, for all editions of the LRFD specifications. Some of the loss methods do not provide clear guidance for computing losses at intermediate steps such as at the time the girder is transported, or when temporary strands are removed. The information given below details how PGSuper computes the effective prestress at the various times considered during analysis.

[eq1]:Eq1.png
[eq2]:Eq2.png
[eq3]:Eq3.png
[eq4]:Eq4.png
[eq5]:Eq5.png
[eq6]:Eq6.png
[eq7]:Eq7.png
[eq8]:Eq8.png
[eq9]:Eq9.png
[eq10]:Eq10.png
[eq11]:Eq11.png
[eq12]:Eq12.png
[eq13]:Eq13.png
[eq14]:Eq14.png
[eq15]:Eq15.png
[eq16]:Eq16.png
[eq17]:Eq17.png
[eq18]:Eq18.png
[eq19]:Eq19.png
[eq20]:Eq20.png
[eq21]:Eq21.png
[eq22]:Eq22.png
[eq23]:Eq23.png
[eq24]:Eq24.png
[eq25]:Eq25.png
[eq26]:Eq26.png
[eq27]:Eq27.png
[eq28]:Eq28.png



No Temporary Strands
-------------------
<table>
<tr><th>Time</th><th>Permanent Strands</th></tr>
<tr><td>At jacking</td>
    <td>![eq1]</td></tr>
<tr><td>Before Prestress Transfer</td>
    <td>![eq2]</td></tr>
<tr><td>After Prestress Transfer</td>
    <td>![eq3]</td></tr>
<tr><td>At Lifting</td>
    <td>![eq3]</td></tr>
<tr><td>At Shipping</td>
    <td>![eq4] 
         <br> Refined Method (LRFD 3rd Edition 2004 and earlier) <br>  ![eq5] 
         <br> Refined Method (LRFD 3rd Edition 2005 and later) <br>   ![eq6] 
         <br> Approximate Method <br>  ![eq7]    </td></tr>
<tr><td>After Deck Placement</td>
    <td>![eq8] 
        <br> Refined Method (LRFD 3rd Edition 2004 and earlier) <br>  ![eq9] 
        <br> Refined Method (LRFD 3rd Edition 2005 and later) <br>  ![eq10] 
        <br> Approximate Method <br>  ![eq11]</td></tr>
<tr><td>After All Losses</td>
    <td>![eq12] 
       <br> Refined Method (LRFD 3rd Edition 2004 and earlier) <br>  ![eq13] 
       <br> Refined Method (LRFD 3rd Edition 2005 and later) <br>  ![eq14] 
       <br> Approximate Method <br>  ![eq13]
    </td></tr>
</table>

Pretensioned Temporary Strands
---------------------------------
<table>
<tr><th>Time</th><th>Permanent Strands</th><th>Temporary Strands</th></tr>
<tr>
<td>At jacking</td>
<td>![eq1]</td>
<td>![eq1]</td>
</tr>
<tr>
<td>Before Prestress Transfer</td>
<td>![eq2]</td>
<td>![eq2]</td>
</tr>
<tr>
<td>After Prestress Transfer</td>
<td>![eq3]</td>
<td>![eq3]</td>
</tr>
<tr>
<td>At Lifting</td>
<td>![eq3]</td>
<td>![eq3]</td>
</tr>
<tr>
<td>At Shipping</td>
<td>
![eq4]
<br>Refined Method (LRFD 3rd Edition 2004 and earlier)<br> ![eq5]
<br>Refined Method (LRFD 3rd Edition 2005 and later)<br> ![eq6]
<br>Approximate method<br> ![eq7]
</td>
<td>
![eq4]
<br>Refined Method (LRFD 3rd Edition 2004 and earlier)<br> ![eq5]
<br>Refined Method (LRFD 3rd Edition 2005 and later)<br> ![eq6]
<br>Approximate method<br> ![eq7]
</td>
</tr>
<tr>
<td>Before Temporary Strand Removal</td>
<td>Same as at shipping. It is conservative to assume that the girders are shipped, erected, braced, and the temporary strands are removed all in the same day.</td>
<td>Same as at shipping. It is conservative to assume that the girders are shipped, erected, braced, and the temporary strands are removed all in the same day.</td>
</tr>
<tr>
<td>After Temporary Strand Removal</td>
<td>![eq16]</td>
<td></td>
</tr>
<tr>
<td>After Deck Placement</td>
<td>![eq17]</td>
<td></td>
</tr>
<tr>
<td>After all losses</td>
<td>
![eq4]
<br>Refined Method (LRFD 3rd Edition 2004 and earlier)<br> ![eq5]
<br>Refined Method (LRFD 3rd Edition 2005 and later)<br> ![eq6]
<br>Approximate method<br> ![eq7]
</td>
<td>
</td>
</tr>
</table>

Temporary Strands Post-Tensioned Before Lifting
-------------------------------------------------
<table>
<tr><th>Time</th><th>Permanent Strands</th><th>Temporary Strands</th></tr>
<tr>
<td>At jacking</td>
<td>![eq1]</td>
<td></td>
</tr>
<tr>
<td>Before Prestress Transfer</td>
<td>![eq2]</td>
<td></td>
</tr>
<tr>
<td>After Prestress Transfer</td>
<td>![eq3]</td>
<td></td>
</tr>
<tr>
<td>After Temporary Strand Installation</td>
<td>![eq18]</td>
<td>![eq19]</td>
</tr>
<tr>
<td>At Lifting</td>
<td>![eq18]</td>
<td>![eq19]</td>
</tr>
<tr>
<td>At Shipping</td>
<td>
![eq20]
<br>Refined Method (LRFD 3rd Edition 2004 and earlier)<br> ![eq5]
<br>Refined Method (LRFD 3rd Edition 2005 and later)<br> ![eq6]
<br>Approximate method<br> ![eq7]
</td>
<td>
![eq21]
<br>Refined Method (LRFD 3rd Edition 2004 and earlier)<br> ![eq5]
<br>Refined Method (LRFD 3rd Edition 2005 and later)<br> ![eq6]
<br>Approximate method<br> ![eq7]
</td>
</tr>
<tr>
<td>Before Temporary Strand Removal</td>
<td>Same as at shipping. It is conservative to assume that the girders are shipped, erected, braced, and the temporary strands are removed all in the same day.</td>
<td>Same as at shipping. It is conservative to assume that the girders are shipped, erected, braced, and the temporary strands are removed all in the same day.</td>
</tr>
<tr>
<td>After Temporary Strand Removal</td>
<td>![eq22]</td>
<td></td>
</tr>
<tr>
<td>After Deck Placement</td>
<td>![eq23]</td>
<td></td>
</tr>
<tr>
<td>After all losses</td>
<td>
![eq4]
<br>Refined Method (LRFD 3rd Edition 2004 and earlier)<br> ![eq5]
<br>Refined Method (LRFD 3rd Edition 2005 and later)<br> ![eq6]
<br>Approximate method<br> ![eq7]
</td>
<td>
</td>
</tr>
</table>

Temporary Strands Post-Tensioned After Lifting
-----------------------------------------------
<table>
<tr><th>Time</th><th>Permanent Strands</th><th>Temporary Strands</th></tr>
<tr>
<td>At jacking</td>
<td>![eq1]</td>
<td></td>
</tr>
<tr>
<td>Before Prestress Transfer</td>
<td>![eq2]</td>
<td></td>
</tr>
<tr>
<td>After Prestress Transfer</td>
<td>![eq3]</td>
<td></td>
</tr>
<tr>
<td>At Lifting</td>
<td>![eq3]</td>
<td></td>
</tr>
<tr>
<td>After Temporary Strand Installation</td>
<td>![eq18]</td>
<td>![eq19]</td>
</tr>
<tr>
<td>At Shipping</td>
<td>
![eq20]
<br>Refined Method (LRFD 3rd Edition 2004 and earlier)<br> ![eq5]
<br>Refined Method (LRFD 3rd Edition 2005 and later)<br> ![eq6]
<br>Approximate method<br> ![eq7]
</td>
<td>
![eq21]
<br>Refined Method (LRFD 3rd Edition 2004 and earlier)<br> ![eq5]
<br>Refined Method (LRFD 3rd Edition 2005 and later)<br> ![eq6]
<br>Approximate method<br> ![eq7]
</td>
</tr>
<tr>
<td>Before Temporary Strand Removal</td>
<td>Same as at shipping. It is conservative to assume that the girders are shipped, erected, braced, and the temporary strands are removed all in the same day.</td>
<td>Same as at shipping. It is conservative to assume that the girders are shipped, erected, braced, and the temporary strands are removed all in the same day.</td>
</tr>
<tr>
<td>After Temporary Strand Removal</td>
<td>![eq22]</td>
<td></td>
</tr>
<tr>
<td>After Deck Placement</td>
<td>![eq23]</td>
<td></td>
</tr>
<tr>
<td>After All Losses</td>
<td>
![eq24]
<br>Refined Method (LRFD 3rd Edition 2004 and earlier)<br> ![eq13]
<br>Refined Method (LRFD 3rd Edition 2005 and later)<br> ![eq14]
<br>Approximate method<br> ![eq13]
</td>
<td></td>
</tr>
</table>

Temporary Strands Post-Tensioned Before Shipping
-------------------------------------------------
<table>
<tr><th>Time</th><th>Permanent Strands</th><th>Temporary Strands</th></tr>
<tr>
<td>At jacking</td>
<td>![eq1]</td>
<td></td>
</tr>
<tr>
<td>Before Prestress Transfer</td>
<td>![eq2]</td>
<td></td>
</tr>
<tr>
<td>After Prestress Transfer</td>
<td>![eq3]</td>
<td></td>
</tr>
<tr>
<td>At Lifting</td>
<td>![eq3]</td>
<td></td>
</tr>
<tr>
<td>After Temporary Strand Installation</td>
<td>
![eq20]
<br>Refined Method (LRFD 3rd Edition 2004 and earlier)<br> ![eq5]
<br>Refined Method (LRFD 3rd Edition 2005 and later)<br> ![eq6]
<br>Approximate method<br> ![eq7]
</td>
<td>![eq19]</td>
</tr>
<tr>
<td>At Shipping</td>
<td>![eq20]</td>
<td>![eq19]</td>
</tr>
<tr>
<td>Before Temporary Strand Removal</td>
<td>
![eq25]
<br>Refined Method (LRFD 3rd Edition 2004 and earlier)<br> ![eq9]
<br>Refined Method (LRFD 3rd Edition 2005 and later)<br> ![eq10]
<br>Approximate method<br> ![eq11]
</td>
<td>![eq19]</td>
</tr>
<tr>
<td>After Temporary Strand Removal</td>
<td>![eq26]</td>
<td></td>
</tr>
<tr>
<td>After Deck Placement</td>
<td>![eq27]</td>
<td></td>
</tr>
<tr>
<td>After All Losses</td>
<td>
![eq28]
<br>Refined Method (LRFD 3rd Edition 2004 and earlier)<br> ![eq13]
<br>Refined Method (LRFD 3rd Edition 2005 and later)<br> ![eq14]
<br>Approximate method<br> ![eq13]
</td>
<td></td>
</tr>
</table>

Notation
----------

<table border=0>
<tr>
<td>![](N1.png)</td>
<td>= Effective prestress</td>
</tr>
<tr>
<td>![](N2.png)</td>
<td>= Stress in prestress strands at jacking</td>
</tr>
<tr>
<td>![](N3.png)</td>
<td>= Relaxation loss between stressing and prestress transfer</td>
</tr>
<tr>
<td>![](N4.png)</td>
<td>= Total long term relaxation loss (LRFD 3rd Edition 2004 and earlier)<br>Relaxation loss between release and deck placement (LRFD 3rd Edition 2005 and later)</td>
</tr>
<tr>
<td>![](N5.png)</td>
<td>= Relaxation loss between release and shipping</td>
</tr>
<tr>
<td>![](N6.png)</td>
<td>= Relaxation loss between deck placement and final</td>
</tr>
<tr>
<td>![](N7.png)</td>
<td>= Loss due to elastic shortening</td>
</tr>
<tr>
<td>![](N8.png)</td>
<td>= Time dependent losses at time of shipping</td>
</tr>
<tr>
<td>![](N9.png)</td>
<td>= Total shrinkage loss (LRFD 3rd Edition 2004 and earlier)<br>Shrinkage loss between release and deck placement (LRFD 3rd Edition 2005 and later)</td>
</tr>
<tr>
<td>![](N10.png)</td>
<td>= Total creep loss (LRFD 3rd Edition 2004 and earlier)<br>Creep loss between release and deck placement (LRFD 3rd Edition 2005 and later)</td>
</tr>
<tr>
<td>![](N11.png)</td>
<td>= Total time dependent loss</td>
</tr>
<tr>
<td>![](N12.png)</td>
<td>= Creep loss between deck placment an final</td>
</tr>
<tr>
<td>![](N13.png)</td>
<td>= Shrinkage loss between deck placment an final</td>
</tr>
<tr>
<td>![](N14.png)</td>
<td>= Loss due to deck shrinkage</td>
</tr>
<tr>
<td>![](N15.png)</td>
<td>= elastic gain due to deck placement</td>
</tr>
<tr>
<td>![](N16.png)</td>
<td>= shrinkage loss between release and shipping</td>
</tr>
<tr>
<td>![](N17.png)</td>
<td>= creep loss between release and shipping</td>
</tr>
<tr>
<td>![](N18.png)</td>
<td>= time dependent losses between release and deck placement</td>
</tr>
<tr>
<td>![](N19.png)</td>
<td>= time dependent losses between deck placement and final</td>
</tr>
<tr>
<td>![](N20.png)</td>
<td>= loss due to friction</td>
</tr>
<tr>
<td>![](N21.png)</td>
<td>= loss due to anchor set</td>
</tr>
<tr>
<td>![](N22.png)</td>
<td>= change in stress in permanent strands due to installation of the temporary strands</td>
</tr>
<tr>
<td>![](N23.png)</td>
<td>= average change in stress in the temporary strands due to installation of subsequent temporary strands</td>
</tr>
<tr>
<td>![](N24.png)</td>
<td>= change in stress in permanent strands due to removal of the temporary strands</td>
</tr>
<tr>
<td>![](N25.png)</td>
<td>= fraction of final loss used at an intermediate time</td>
</tr>


</table>