PGSuper Template Bridge {#template_bridge}
===========================================
As mentioned in the [Technical Guide](@ref technical_guide), the first step when TOGA builds a PGSuper analysis model is to load the PGSuper Template file AKA, the Template Bridge. This file is named *TogaTemplate.pgs* and it resides in the TOGA configuration folder. 

After the Template Bridge is loaded TOGA modifies the model extensively, however many settings are not changed. Therefore, you can modify the template bridge and some model settings and modifications will "stick" into the analysis. The best way to determine which settings are not modified by TOGA is to look at the TOGA source code. If you are not up to that, you might try modifying the template bridge then opening a .toga file and then exporting the PGSuper model. If your modifications stuck, you are probably OK.

> TIP: Modifying the template bridge is not for the weak of heart and should probably be left up to programmers. Your mileage may vary.