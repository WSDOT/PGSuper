The Templates Text Files {#template_files}
===========================================
When you click on *File > New* to open the New Project window, click on the TOGA project type and you are presented with a list of girder types (e.g., Tx70) for the new project. You might wonder - where does this list come from?

Further inspection will show several files with *.togt extensions in the TOGA configuration folder created during installation. The names of these files may look suspiciously like the girder names in the list - that's because they are exactly this. To add a new girder type called XYZ to TOGA, simply add a new file to the configuration folder named XYZ.togt. The contents of the file is defined below.

Format of TOGA Templates Files
==============================
A TOGA template file is a simple comma-delimited text file installed in the TOGA configuration folder. The name of the file, minus the .togt extension is the text that will be displayed in the New Project window. The format of the contents of the text file is as follows:
   
     GirderEntryName,EndConnection,StartConnection,SpecVersion,FolderIconFileName
   
Where:
Parameter | Description
----------|------------
GirderEntryName | Name of the girder entry to be used from the TxDOT standard library
EndConnection and StartConnection | The names of the TxDOT standard Connection library entries to be used at the start and end of the bridge model.
SpecVersion | Name of the Project Criteria library entry to be used with this template (e.g., "TxDOT 2015")
FolderIconFileName | The name of the containing folder where this template will shown when displaying this template in the "File -> New" dialog. This is also the name of the  icon (*.ico) file file used when displaying the folder. Do not include ".ico" in the name

> NOTE: Names above are case sensitive and must match exactly with the associated library entry or file name. An error will occur if the library entry does not exist in the current TOGA configuration. Do not use blank spaces unless there is a blank space in the library entry name.

How The Template File is Used
=============================
As mentioned in the previous section, the first step TOGA takes to build its internal PGSuper model is to load the PGSuper Template Bridge. Shortly thereafter, TOGA will set the girder type, girder end connections, and project criteria to those specified in the .togt template file. TOGA will then modify the bridge using the settings defined in the TOGA input.

