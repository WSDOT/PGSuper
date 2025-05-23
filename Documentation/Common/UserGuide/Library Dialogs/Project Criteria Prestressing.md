Prestressing {#ug_library_dialogs_project_criteria_prestressing}
==============================================
Define project criteria for allowable stresses in the prestressing strands. Values for both stress relieved and low relaxation strands must be specified.

Stress Limits for Prestressing (LRFD 5.9.2.2 (*pre-2017: 5.9.3*))
-------------------------------------------

Item | Description
-----|--------------
Check at Jacking | If selected strands stresses are checked at the time of jacking. Enter a fraction of the ultimate strand stress (fpu) that is allowable.
Immediately Prior to Transfer | If selected strands stresses are checked immediately prior to transfer. Enter a fraction of the ultimate strand stress (fpu) that is allowable.
Check After Transfer | If checked, strand stresses are checked immediately after transfer.  Enter a fraction of the ultimate strand stress (fpu) that is allowable.
After All Losses | Entry the fraction of the strand yield stress that is allowable in the final condition.


Stress Limits for Post-Tensioning (LRFD 5.9.2.2 (*pre-2017: 5.9.3*))
------------------------------------------------
Check at Jacking | If selected strands stresses are checked at the time of jacking. Enter a fraction of the ultimate strand stress (fpu) that is allowable.
Check prior to seating | If selected strands stresses are checked immediately prior to seating. Enter a fraction of the ultimate strand stress (fpu) that is allowable.
At anchorages and couplers immediately after anchor set | Enter the fraction of the ultimate strand stress (fpu) that is allowed
Elsewhere along length of member away from anchorages at anchor set | Enter the fraction of the ultimate strand stress (fpu) that is allowed
After All Losses | Entry the fraction of the strand yield stress that is allowable in the final condition.

Pretensioned Strand Options
---------------------------

Item | Description
----|------------
Compute the transfer length of prestressing strands using the following method: | Select a method for computing prestress transfer length. Options are per LRFD specifications and "Zero Length". See discussion below.

Transfer length is defined as "the length over which the pretensioning force is transferred to the concrete by bond and friction in a pretensioned member". LRFD 5.9.4.3.1 (*pre-2017: 5.11.4.1*) defines the transfer length as 60 strand diameters. PGSuper accounts for reduced prestress forces in the transfer zone using linear interpolation as described in the Specifications.

However, some agencies may wish to ignore the effect of prestress transfer (zero transfer length) and assume that the strand is fully effective right to the debond location. Note that this option may, or may not, be conservative depending on the limit state or behavior under consideration.  If selected, the "Zero Transfer" option will approximate this condition by setting the transfer length to 0.1 inches. A true "zero transfer length" cannot be used as it will create "jumps" in the stress due to prestressing. That is, on unbonded side of a debond section the stress due to prestressing will be zero and on the other side it will "jump" to some value - mathematically this is a discontinuous function and wreaks havoc with the design and analysis algorithms..

> NOTE: The zero transfer option will affect all pretensioned designs, including debonded and harped designs.


Size of Ducts (LRFD 5.4.6.2)
----------------------------
These parameters allow you to modify the requirements of LRFD 5.4.6.2.

> NOTE: The permissible duct size was increased to 0.54 times the least gross concrete thickness at the duct in LRFD 9th Edition. As stated in LRFD 9th Edition, C5.4.6.2, the research forming the basis for this requirement was limited to duct stacks and does not apply to bundled ducts. PGSplice will treat all ducts as duct stacks.
