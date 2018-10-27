Lateral Deflections {#tg_lateral_deflections}
======================================
Lateral deflections occur in precast-prestressed concrete girders due:
1. Installation of girder in a rotated orientation so that the top of the girder is parallel to the roadway cross slope
2. Asymmetry of the girder section

> NOTE: Lateral deflection of girders only occurs prior to the girder becoming connected to adjacent girders forming the bridge cross section. That is, there is no additional lateral deflection once the girder is no longer an independent structural component.

Installation with Rotated Orientation
---------------------------------------
Many girder types, such as slabs, boxes, tubs (U-beams), and shallow decked bulb tee girders, are installed with a rotated orientation that aligns the top of the girder with the roadway cross slope. When installed, the rotation of the girder introduces a lateral component of load that causes lateral deflection. These are typically small deflections, however for long spans and girders without significant lateral stiffness these deflections can become sizable. The girder section shown below is installed with a rotated orientation. The load component w<sub>x</sub> causes lateral deflection.

![](Lateral_Deflection_Images\Lateral_Component_Dead_Load.png)

![](Lateral_Deflection_Images\Tilted_Girder_Lateral_Deflection.png)

Asymmetric Girder Sections
-------------------------
Some girder sections, such as the decked bulb tee, are constructed with the top flange sloped to match roadway cross slope. This results in an asymmetric section. For this case, there are two sources of lateral deflection.
1. Lateral deflection due to asymmetry
2. Lateral deflection due to lateral eccentricity of the precompression force.

The girder section shown below is asymmetric. The principle axes are not aligned with the vertical and horizontal axes and the product of inertia, I<sub>xy</sub>, is non-zero. This results in a component of lateral deflection due to the vertical loads. Additionally, the centroid of the prestressing force (red/white target) is eccentric with respect to the center of mass of the section (blue/white target). This results in lateral deflection due to prestressing as well as a vertical deflection component due to the asymmetry of the section. 

![](Lateral_Deflection_Images\Asymmetric_Section.png)

The total deflection is the sum of direct and indirect deflections. Direct deflections are those deflection that result from direct loading. Indirect deflections are those deflections that result due to the asymmetry of the section.

The figure below shows an arbitrary asymmetric section with moments M<sub>x</sub> and M<sub>y</sub> about the x and y axes.

![](Lateral_Deflection_Images\Arbitrary_Asymmetric_Section.png)

Let <span style="font-family:symbol">f</span><sub>x</sub> and <span style="font-family:symbol">f</span><sub>y</sub> be the curvatures about the x and y axes, respectively.

Equation 1 relates moments and curvatures.

![](Lateral_Deflection_Images\Lateral_Deflection_Eq_1.png)(Eq. 1)

Solving Equation 1 for curvature gives,

![](Lateral_Deflection_Images\Lateral_Deflection_Eq_2.png)(Eq. 2)

From classical Bernoulli beam theory, curvature is equal to the second derivate of deflection, therefore

![](Lateral_Deflection_Images\Lateral_Deflection_Eq_3.png)(Eq. 3)
![](Lateral_Deflection_Images\Lateral_Deflection_Eq_4.png)(Eq. 4)

Assuming stresses are in the linear elastic range, the principle of superposition is applicable. The following two cases are independent and the summation of the deflection results is the total deflection.

### Case 1 - Mx != 0, My = 0 ###
M<sub>x</sub> is caused by gravity loads (self-weight of girder) and prestressing that is eccentric with respect to the x-axis. Substituting M<sub>y</sub> = 0 into Equations 3 and 4 yields
![](Lateral_Deflection_Images\Lateral_Deflection_Eq_5.png)(Eq. 5)
![](Lateral_Deflection_Images\Lateral_Deflection_Eq_6.png)(Eq. 6)

The deflections in the x and y directions are found by integrating Equations 5 and 6 two times.

![](Lateral_Deflection_Images\Lateral_Deflection_Eq_7.png)(Eq. 7)
![](Lateral_Deflection_Images\Lateral_Deflection_Eq_8.png)(Eq. 8)

Equation 7 gives the direct vertical deflection.

Re-arranging Equations 7 and 8 gives

![](Lateral_Deflection_Images\Lateral_Deflection_Eq_9.png)(Eq. 9)
![](Lateral_Deflection_Images\Lateral_Deflection_Eq_10.png)(Eq. 10)

Setting Equation 9 equal to Equation 10 and simplifying gives

![](Lateral_Deflection_Images\Lateral_Deflection_Eq_11.png)(Eq. 11)

Equation 11 gives the indirect lateral deflection that results due to the asymmetry of the section.

### Case 2 - Mx = 0, My != 0 ###
M<sub>y</sub> is caused by prestressing that is eccentric with respect to the y-axis. Substituing M<sub>x</sub> = 0 into Equations 3 and 4 yields

![](Lateral_Deflection_Images\Lateral_Deflection_Eq_12.png)(Eq. 12)
![](Lateral_Deflection_Images\Lateral_Deflection_Eq_13.png)(Eq. 13)

The deflections in the x and y directions are found by integrating Equations 12 and 13 two times.

![](Lateral_Deflection_Images\Lateral_Deflection_Eq_14.png)(Eq. 14)
![](Lateral_Deflection_Images\Lateral_Deflection_Eq_15.png)(Eq. 15)

Equation 15 gives the direct lateral deflection.

Re-arranging Equations 14 and 15 gives

![](Lateral_Deflection_Images\Lateral_Deflection_Eq_16.png)(Eq. 16)
![](Lateral_Deflection_Images\Lateral_Deflection_Eq_17.png)(Eq. 17)

Setting Equation 16 equal to Equation 17 and simplifying gives

![](Lateral_Deflection_Images\Lateral_Deflection_Eq_18.png)(Eq. 18)

Equation 18 gives the indirect vertical deflection that results due to the asymmetry of the section.

Superimposing the deflections from Case 1 and Case 2 gives the total deflection

![](Lateral_Deflection_Images\Lateral_Deflection_Eq_19.png)(Eq. 19)
![](Lateral_Deflection_Images\Lateral_Deflection_Eq_20.png)(Eq. 20)


