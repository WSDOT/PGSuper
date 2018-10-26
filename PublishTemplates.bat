REM - Script to publish templates and master library to WSDOT ftp server

SET TARGET=\\wsdot\resources\Topics\Publish\FTP\Data\public\Bridge\Software\PGSuper\Version_2.3.0

REM '''''''''''''''''''''''''''''''''''''''''''''''''''''
REM 'WSDOT Resources
REM '''''''''''''''''''''''''''''''''''''''''''''''''''''
copy /Y WSDOT.lbr %TARGET%\WSDOT.lbr
md5deep -q %TARGET%\WSDOT.lbr > %TARGET%\WSDOT.lbr.md5

del %TARGET%\WSDOT_Templates\W-Girders\*.pgt
del %TARGET%\WSDOT_Templates\U-Girders\*.pgt
del %TARGET%\WSDOT_Templates\WF-Girders\*.pgt
del %TARGET%\WSDOT_Templates\WBT-Girders\*.pgt
del %TARGET%\WSDOT_Templates\Deck_Bulb_Tees\*.pgt
del %TARGET%\WSDOT_Templates\MultiWeb\*.pgt
del %TARGET%\WSDOT_Templates\Slabs\*.pgt

copy /Y Templates\WSDOT\W-Girders\*.pgt       	%TARGET%\WSDOT_Templates\W-Girders\
copy /Y Templates\WSDOT\U-Girders\*.pgt       	%TARGET%\WSDOT_Templates\U-Girders\
copy /Y Templates\WSDOT\WF-Girders\*.pgt    	%TARGET%\WSDOT_Templates\WF-Girders\
copy /Y Templates\WSDOT\WBT-Girders\*.pgt   	%TARGET%\WSDOT_Templates\WBT-Girders\
copy /Y Templates\WSDOT\Deck_Bulb_Tees\*.pgt  	%TARGET%\WSDOT_Templates\Deck_Bulb_Tees\
copy /Y Templates\WSDOT\MultiWeb\*.pgt      	%TARGET%\WSDOT_Templates\MultiWeb\
copy /Y Templates\WSDOT\Slabs\*.pgt       	%TARGET%\WSDOT_Templates\Slabs\

del %TARGET%\WSDOT_Templates\WorkgroupTemplates.md5
md5deep -q -r %TARGET%\WSDOT_Templates > WorkgroupTemplates.md5
copy /Y WorkgroupTemplates.md5 %TARGET%\WSDOT_Templates\WorkgroupTemplates.md5

REM '''''''''''''''''''''''''''''''''''''''''''''''''''''
REM 'AASHTO Resources
REM '''''''''''''''''''''''''''''''''''''''''''''''''''''
copy /Y AASHTO.lbr %TARGET%\AASHTO.lbr
md5deep -q %TARGET%\AASHTO.lbr > %TARGET%\AASHTO.lbr.md5

del %TARGET%\AASHTO_Templates\Box_Beams\*.pgt
del %TARGET%\AASHTO_Templates\Bulb_Tees\*.pgt
del %TARGET%\AASHTO_Templates\Deck_Bulb_Tees\*.pgt
del %TARGET%\AASHTO_Templates\I-Beams\*.pgt
del %TARGET%\AASHTO_Templates\Slabs\*.pgt

copy /Y Templates\AASHTO\Box_Beams\*.pgt 	      	%TARGET%\AASHTO_Templates\Box_Beams\
copy /Y Templates\AASHTO\Bulb_Tees\*.pgt       		%TARGET%\AASHTO_Templates\Bulb_Tees\
copy /Y Templates\AASHTO\Deck_Bulb_Tees\*.pgt       	%TARGET%\AASHTO_Templates\Deck_Bulb_Tees\
copy /Y Templates\AASHTO\I-Beams\*.pgt       		%TARGET%\AASHTO_Templates\I-Beams\
copy /Y Templates\AASHTO\Slabs\*.pgt       		%TARGET%\AASHTO_Templates\Slabs\


del %TARGET%\AASHTO_Templates\WorkgroupTemplates.md5
md5deep -q -r %TARGET%\AASHTO_Templates > WorkgroupTemplates.md5
copy /Y WorkgroupTemplates.md5 %TARGET%\AASHTO_Templates\WorkgroupTemplates.md5

REM  - final cleanup
del WorkgroupTemplates.md5
