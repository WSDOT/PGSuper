User Guide {#tuser_guide}
============
> Note that the exporter is only available from within the PGSuper application (i.e., a PGSuper file must be loaded within BridgeLink in order to access the CAD Exporter). 

Start the TxDOT CAD Exporter from PGSuper's main menu by selecting "File | Export | TxDOT CAD Data". This will bring up the "Export TxDDOT CAD Data" dialog. From here, select the girders to be included in analysis and the format that the data is to be exported in. There are three file format options:

* Excel Spreadsheet - PGSuper data and results are output into a pre-defined TxDOT Excel template that is installed along with PGSuper (see the Administrator Guide for more information). Once the spreadsheet is created it can then be brought into MicroStation using the Axiom Microsoft Office Importer. This is the fastest and most reliable way to get tables into MicroStation. Note that Microsoft Office 2007 or later must be installed in order to use this option.


* CSV (Comma Separated Value) File - The same data values that are exported into Excel are exported in to a text file in which column fields are separated by SEMICOLONS (;'s). Semicolons are used in order maintain the integrity of the strings used in non-standard strand definitions (strand definitions are separated by commas). Data lines from the CSV file can then be pasted into Excel and parsed using the "Text to Columns" feature in Excel.


* Legacy Text File Format - This is the text file format used prior to Version 5.0 of PGSuper. Note that this format will not be maintained and will likely be removed in a future version of the software.

Once girder(s) and format are selected, the program will ask for a file where the data will be stored. Note that the export will fail if the file is open in another application.


> Tip: The TxDOT CAD Exporter is installed with both the TxDOT and WSDOT versions of BridgeLink, and is enabled by default. Users can disable and enable the Exporter via BridgeLink's "File | Manage | PGSuper Plugins and Extensions" command which is available from the main program menu.