Maximum Jacking Force {#tg_pjack}
======================================
This section describes how the maximum jacking forces for prestressing strands is computed.

The jacking force is computed as \f$ P_{jack} = f_{pj} A_{ps} \f$ where \f$ f_{pj} \f$ is the jacking stress and \f$ A_{ps} \f$ is the area of prestressing strand.

Jacking Stress - 1st Edition
------------------------------
The jacking stress is prescribed in Article 5.9.3 of the 1st Edition of the LRFD specification. The limiting stress at jacking is \f$ f_{pj} = 0.72 f_{pu} \f$ for stress relieved strands and \f$ f_{pj} = 0.78 f_{pu} \f$ for low relaxation strands for pretensioning. For post-tensioning, the limiting stress at jacking is \f$ f_{pj} = 0.76 f_{pu} \f$ for stress relieved strands and \f$ f_{pj} = 0.80 f_{pu} \f$ for low relaxation strands.

Jacking Stress - 1st Edition with 1996 and 1997 Interim Provisions to 3rd Edition 2004
------------------------------------------------------------------------------
The 1996 interims to the LRFD 1st Edition changed the limiting stress in prestressing strands. Article 5.9.3 limits the strand stress immediately prior to prestress transfer \f$ (f_{pt} + \Delta f_{pES}) \f$ to \f$ 0.70 f_{pu} \f$ for stress relieved strands and \f$ 0.75 f_{pu} \f$ for low relaxation strands.

The jacking stress is computed as \f$ f_{pj} = f_{pt} + \Delta f_{pES} + \Delta f_{pR1} \f$ where LRFD 5.9.5.4.2c gives \f$ \Delta f_{pR1} = \frac{log(24.0 t)}{K} \left[ \frac{f_{pj}}{f_{py}} - 0.55 \right] f_{pj} \f$  with \f$ K = 10.0 \f$ for stress relieved strands and \f$ K = 40.0 \f$ for low relaxation strands.

Substituting the stress limits for low relaxation strand and re-arranging
\f[
\left[ \frac{log(24.0 t)}{K}\right]\frac{f^2_{pj}}{f_{py}}- \left[1 + 0.55\left(\frac{log(24.0 t)}{K} \right) \right] + 0.75 f_{pu} = 0
\f]

This equation is solved for \f$ f_{pj} \f$. Note that the last term is \f$ 0.70 f_{pu} \f$ for stress relieved strands.

Jacking Stress - 3rd Edition 2004 with 2005 Interim Provisions and Later
------------------------------------------------------------------------------
AASHTO removed the computation of relaxation loss prior to prestress transfer with the 2005 interim provisions. The limiting stress immediately before transfer is \f$ 0.70 f_{pu} \f$ for stress relieved strands and \f$ 0.75 f_{pu} \f$ for low relaxation strands. The jacking stress is taken to be equal to these values unless the method for computing prestress losses is per the WSDOT Bridge Design Manual, in which case the relaxation prior to prestress transfer \f$ \Delta f_{pR1} \f$ is add to the stress limit immediately prior to transfer.

> NOTE: When AASHTO updated the prestress loss method with the 2005 interim provisions \f$ \Delta f_{pR1} \f$ was repurposed. WSDOT adopted the notation \f$ \Delta f_{pR0} \f$ for the relaxation loss occuring between jacking and transfer.
