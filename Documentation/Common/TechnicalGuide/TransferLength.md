Transfer Length {#tg_transfer_length}
======================================
Transfer length is the length over which pretensioning force is transferred to the concrete section by bond and friction in a pretension member. The reduced prestress force over the transfer length reduces the precompession affect on the girder section (See LRFD 5.9.4.3.1). Several parameters related to shear are also affected (See LRFD C5.7.2.2).

Transfer length is defined as a multiple of strand diameter in LRFD 5.9.4.3.1. Because different size strands can be modeled in a girder there can be different transfer lengths to consider. The stress in the prestressing strand is assumed to vary linearly from zero at the point where bonding commences to the effective stress at the end of the transfer length. Generally the stress in the same in all strands however different size strands have different transfer lengths and the resulting force is transfer over different lengths. For this reason the stress in the strands must be treated separately for each size used.

The stress in the prestressing strand is computed as:

\f[
f_{pe} = \frac{(f_{pj} - losses + gains) \sum{\zeta A_{ps}}}{\sum{A_{ps}}}
\f]

where <br>
\f$ \zeta \f$ = transfer length reduction factor.

Debonded strands complicates the transfer length reduction factor because the area of reinforcement changes along the length of the girder. Consider a section within the transfer length for a debonded strand. The transfer length reduction factor is 1.0 for all fully transfered strands before the section and something less than 1.0 for the debonded strands that are starting to transfer their force. A single transfer length reduction factor is needed to reduce the stress that accounts for both the fully transferred and partially transferred precompression force. The transfer length reduction factor is computed as:

\f[
\zeta = \frac{\zeta_f A_{psf} + \zeta_p A_{psp}}{A_{ps}}
\f]

where <br>
\f$ \zeta \f$ = transfer length reduction factor <br>
\f$ \zeta_f \f$ = transfer length reduction factor for the fully transferred precompression force = 1.0 <br>
\f$ \zeta_p \f$ = transfer length reduction factor for the partially transferred precompression force \f$ = \frac{x_s - x_d}{l_t}\f$ <br>
\f$ A_{psf} \f$ = area of strand that for the fully transferred precompression force <br>
\f$ A_{psp} \f$ = area of strand that for the partially transferred precompression force \f$ = A_{ps} - A_{psf}\f$ <br>
\f$ A_{ps} \f$ = total area of strand at the section under consideration <br>
\f$ x_s \f$ = location of section under considerion <br>
\f$ x_d \f$ = location where bonding commences <br>
\f$ l_t \f$ = prestress transfer length <br>

The transfer length reduction factor is listed in the Effective Prestress Force table in the Details Report. The area of prestressing strands is shown graphically in the Girder Properties graph. The graph shows the basic area of strand (number of strands times area of one strand) for each strand type and the effective area of permanent strands. The effective area of permanent strands plot shows the effect of the transfer length reduction factor.




