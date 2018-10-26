Time-Dependent Material Models {#tg_time_dependent_material_models}
======================================
Time-dependent material properties are computed using the AASHTO LRFD, ACI 209R-92, and CEB-FIP 1990 models.

AASHTO LRFD
--------------------

### Compressive Strength ###
AASHTO does not define a time-dependent model for concrete compressive strength. The ACI 209R-92 model is used.

### Modulus of Elasticity ###
Modulus of elasticity is defined by a modified version of LRFD Equation 5.4.2.4-1. See @ref tg_concrete_properties for details.

### Shrinkage Strain ###
Shrinkage strain is defined by a modified version of LRFD Equation 5.4.2.3.3-1. See @ref tg_concrete_properties for details.

### Creep Coefficient ###
Creep coefficient is defined by a modified version of LRFD Equation 5.4.2.3.2-1. See @ref tg_concrete_properties for details.

### Relaxation of Prestressing Steel ###
AASHTO does not define an intrinsic model for strand relaxation. The ACI 209R-92 model is used.

ACI 209R-92
-------------
### Compressive Strength ###
Compressive strength is computed by ACI 209R-92 Equation 2-1.
![ACI 209R-92 Eqn. 2-1](ACI_fc.png)

From ACI 209R-92 Table 2.2.1

Type of Curing | Cement Type | a | <span style="font-family:symbol">b</span>
---------------|-------------|---|-------------------------------------------
Moist | I | 4.0 | 0.85
Moist | III | 2.3 | 0.92
Steam | I | 1.0 | 0.95
Steam | III | 0.7 | 0.98

### Modulus of Elasticity ###
Modulus of elasticity is computed by ACI 209R-92 Equation 2-5.
![ACI 209R-92 Eqn. 2-5](ACI_Ec.png)

for g<sub>ct</sub> = 33.0, 

![](ACI_Ec_simplified.png)

### Shrinkage Strain ###
Shrinkage strain is computed by ACI 209R-92 Equations 2-7, 2-9, 2-10, 2-15, 2-16, and 2-22. Here we present the shrinkage strain as an ultimate shrinkage value multiplied by modification factors.
![](ACI_Shrinkage.png)

#### Time Modification Factor ####
![](ACI_Shrinkage_Time_Factor.png)
f = 35 for moist cured concrete (ACI Eqn. 2-9) and 55 for steam cured concrete (ACI Eqn. 2-10). <br>
t = shrinkage duration

#### Curing Modification Factor ####
<span style="font-family:symbol">g</span><sub>cp</sub> = 1.0 for steam curing.

From ACI 209R-92 Table 2.5.3

Moist Curing Duration (days) | <span style="font-family:symbol">g</span><sub>cp</sub>
-----------------------------|------------------------
1 | 1.2
3 | 1.1
7 | 1.0
14 | 0.93
28 | 0.86
90 | 0.75

<span style="font-family:symbol">g</span><sub>cp</sub> is linearly interpolated for other curing durations.

#### Relative Humidity Modification Factor ####
![ACI 209R-92 Eqn 2-15 and 2-16](ACI_Shrinkage_Humidity_Factor.png)
RH = Average Ambient Relative Humidity

#### Volume to Surface Ratio Modification Factor ####
![ACI 209R-92 Eqn. 2-22](ACI_Shrinkage_VS_Factor.png)
v/s = volume to surface ratio

### Creep Coefficient ###
Creep coefficient is computed by ACI 209R-92 Equations 2-8, 2-11, 2-12, 2-14, and 2-21. Here we present the creep coeficient as an ultimate coefficient value multiplied by modification factors.

![](ACI_Creep.png)

#### Time Modification Factor ####
![ACI 209R-92 Eqn. 2-8](ACI_Creep_Time_Factor.png)
t = time after loading (days)

#### Loading Age Modification Factor ####
![ACI 209R-92 Eqn. 2-11 and 2-12](ACI_Creep_Loading_Age_Factor.png)

t<sub>la</sub> is the age of the concrete at the time of loading (days)

#### Relative Humidity Modification Factor ####
![ACI 209R-92 Eqn. 2-14](ACI_Creep_Humidity_Factor.png)
RH = Average Ambient Relative Humidity

#### Volume to Surface Ratio Modification Factor ####
![ACI 209R-92 Eqn. 2-21](ACI_Creep_VS_Factor.png)
v/s = volume to surface ratio

### Relaxation of Prestressing Steel ###
Relaxation is computed from the equations given in ACI 209R-92 Table 3.7.1.

[aci_relaxation_sr]: ACI_Relaxation_SR.png
[aci_relaxation_lr]: ACI_Relaxation_LR.png

<table>
<tr><th>Strand Type</th><th>Relaxation</th></tr>
<tr>
<td>Stress Relieved</td>
<td>![aci_relaxation_sr]</td>
</tr>
<tr>
<td>Low Relaxation</td>
<td>![aci_relaxation_lr]</td>
</tr>
</table>

CEB-FIP 1990
-------------

### Compressive Strength ###
The concrete compressive strength is computed by CEB-FIP 1990 Eqn. 2.1-53.
![CEB-FIP 1990 Eqn. 2.1-53 and 2.1-54](CEBFIP_fc.png)

Cement Type | s
------------|---
Rapid Hardening High Strength (RS) | 0.20
Normal Hardening (N) | 0.25
Rapid Hardening (R) | 0.25
Slowly Hardening (SL) | 0.38

### Modulus of Elasticity ###
The modulus of elasticity is computed by CEB-FIP 1990 Eqn. 2.1-57.
![CEB-FIP 1990 Eqn. 2.1-57 and 2.1-58](CEBFIP_Ec.png)

### Shrinkage Strain ###
Shrinkage strain is computed by CEB-FIP 1990 Equation 2.1-74.
![CEB-FIP 1990 Eqn. 2.1-74](CEBFIP_Shrinkage.png)
![CEB-FIP 1990 Eqn. 2.1-75](CEBFIP_2_1_75.png)
![CEB-FIP 1990 Eqn. 2.1-76](CEBFIP_2_1_76.png)

Cement Type | <span style="font-family:symbol">b</span><sub>sc</sub>
------------|---------------------------------------------------------
Rapid Hardening High Strength (RS) | 8
Normal Hardening (N)  | 5
Rapid Hardening (R) | 5
Slowly Hardening (SL) | 4


![CEB-FIP 1990 Eqn. 2.1-77](CEBFIP_2_1_77.png)
![CEB-FIP 1990 Eqn. 2.1-78](CEBFIP_2_1_78.png)
![CEB-FIP 1990 Eqn. 2.1-79](CEBFIP_2_1_79.png)
![CEB-FIP 1990 Eqn. 2.1-69](CEBFIP_2_1_69.png)


### Creep Coefficient ###
Creep coefficient is computed by CEB-FIP 1990 Equation 2.1-64.

![CEB-FIP 1990 Eqn. 2.1-64](CEBFIP_Creep.png)
![CEB-FIP 1990 Eqn. 2.1-65](CEBFIP_2_1_65.png)
![CEB-FIP 1990 Eqn. 2.1-66](CEBFIP_2_1_66.png)
![CEB-FIP 1990 Eqn. 2.1-67](CEBFIP_2_1_67.png)
![CEB-FIP 1990 Eqn. 2.1-68](CEBFIP_2_1_68.png)
![CEB-FIP 1990 Eqn. 2.1-69](CEBFIP_2_1_69.png)
![CEB-FIP 1990 Eqn. 2.1-70](CEBFIP_2_1_70.png)
![CEB-FIP 1990 Eqn. 2.1-71](CEBFIP_2_1_71.png)


### Relaxation of Prestresing Steel ###
Relaxation is defined in CEB-FIP 2.3.4.5. For design purposes, the relxation lossses in CEB-FIP Figure 2.3.3 can be used. The curves in this figure have been fit into the following format
![](CEBFIP_Relaxation.png)
![](CEBFIP_Relaxation_P.png)
![](CEBFIP_Relaxation_k.png)
