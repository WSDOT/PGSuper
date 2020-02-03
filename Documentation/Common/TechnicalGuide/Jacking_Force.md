Maximum Jacking Force {#tg_pjack}
======================================
This section describes how the maximum jacking forces for prestressing strands is computed.

The jacking force is computed as ![](Pjack.png#left) where <i>f<sub>pj</sub></i> is the jacking stress and <i>A<sub>ps</sub></i> is the total area of prestressing strand.

Jacking Stress - 1st Edition
------------------------------
The jacking stress is prescribed in Article 5.9.2.2 (*pre-2017: 5.9.3*) of the 1st Edition of the LRFD specification. The jacking stresses are ![](fpj_SR_1994.png#left) for Stress Relieved Strands and ![](fpj_LR_1994.png#left) for Low Relaxation Strands.

Jacking Stress - 1st Edition with 1996 and 1997 Interim Provisions to 3rd Edition 2004
------------------------------------------------------------------------------
The 1st Edition with 1996 and 1997 interim provisions change the method by which the allowable jacking stress is to be computed. Article 5.9.3 specifics the strand stress immediately prior to prestress transfer as ![](fpt_LR_1997.png#left) for Stress Relieved Strands and ![](fpt_SR_1997.png#left) for Low Relaxation Strands.

The jacking stress is ![](fpj_1997.png#left)

Substituting, we get ![](fpj_1997a.png#left)

> NOTE: From now on, we will only deal with the stress relieved strands. The formulation for low relaxation strands is similar.

From LRFD 5.9.3.4.2c (*pre-2017:5.9.5.4.2c*), ![](deltaFpR1.png#left)

Substituting and re-arranging, we get ![](SolveForFpj.png#left)
 
This equation is solved for <i>f<sub>pj</sub></i>

Jacking Stress - 3rd Edition 2004 with 2005 Interim Provisions and Later
------------------------------------------------------------------------------
AASHTO removed the computation of relaxation loss prior to prestress transfer with the 2005 interim provisions. The jacking stresses are ![](fpj_SR_2005.png#left) for Stress Relieved Strands and ![](fpj_LR_2005.png#left) for Low Relaxation Strands.

WSDOT retained the computation of relaxation loss prior to prestress transfer. The method for compute the maximum jacking stress is the same as described above however WSDOT uses the term ![](deltaFpR0.png#left) in the equations.