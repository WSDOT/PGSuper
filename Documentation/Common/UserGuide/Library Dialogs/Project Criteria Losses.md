Losses {#ug_library_dialogs_project_criteria_losses}
==============================================
Use this tab to select the method for calculating prestress losses.

Loss Method
-----------

Select method for computing losses. Using the drop down list, select the method that will be used for computing losses.

The available loss methods are:
* Refined Estimate per LRFD 5.9.3.4 (*pre-2017: 5.9.5.4*)
* Refined Estimate per WSDOT Bridge Design Manual
	- This is the same as the Refined Estimate per LRFD 5.9.3.4 except it also includes prestress loss due to relaxation prior to prestress transfer
* Refined Estimate per TxDOT Bridge Design Manual
* Refined Estimate per TxDOT Research Report 0-6374-2
* Approximate Lump Sum per LRFD 5.9.3.3 (*pre-2017: 5.9.5.3*)
* Approximate Lump Sum per WSDOT Bridge Design Manual
	- This is the same as the Approximate Lump Sum per LRFD 5.9.3.3 except it also includes prestress loss due to relaxation prior to prestress transfer
* Time Step Method

The LRFD specification does not explicitly provide a method of estimating losses at the time of shipping. For refined estimates (LRFD 5.9.3.4 (*pre-2017: 5.9.5.4*)), for LRFD 2005 and later, input the day after release when shipping is assumed to take place. If there is a range of days when shipping can occur, input the earliest time for a conservative estimate.

For approximate methods (LRFD 5.9.3.3 (*pre-2017: 5.9.5.3*)) the shipping losses can be described as a lump sum value or a percentage of the final losses. Use the drop down list to select the method for shipping losses and enter an appropriate value. 

For the Time-Step Method, select the time dependent concrete model. Choose from AASHTO LRFD, ACI 209R-92 or CEB-FIB 1990.

For refined estimates (LRFD 5.9.3.4 (*pre-2017: 5.9.5.4*)), for LRFD 2005 and later, select the method for computing relaxation losses.

For refined estimates using the methods adapted in the TxDOT Research Report 0-6374-3:  Use the drop down list to select the method for shipping losses and enter an appropriate value. Also, select the method used to compute fcgp, the concrete stress at the center of gravity of prestressing tendons at transfer, which is used for the computation of elastic shortening and creep loss. For more information, refer to the Technical Guide.

Elastic Gains
--------------
When gross section properties are used for stress analysis, elastic gains are be computed and added to the effective prestress force to approximate the transformed section analysis that was used when developing the prestress loss method (See NCHRP Report 496). For refined estimates (LRFD 5.9.3.4 (*pre-2017: 5.9.5.4*)), for LRFD 2005 and later, define the percentage of the various external loads that will contribute to the elastic gains.

Gains due to deck shrinkage must also include increased stresses at the top and bottom of the girder. Deck shrinkage is computed as defined in LRFD 5.9.3.4.3d (*pre-2017: 5.9.5.4.3d*). The shrinkage strain is scaled by the percentage defined on this tab. The stresses in the girders is computed using equation 5.9.3.4.3d-2 with St and Sb being substituted for epc/Ic.
