Configuration Servers - The Details {#configuration_server_files}
============
Configuration Servers are more of a concept than what we traditionally think of as a server. Examples of the servers we traditionally think of are FTP, HTTP, PHP, SQL, and the like. All of these servers use dedicated hardware running specialized software. The Configuration Servers we refer to are basically locations where some files can be stored and accessed by users. 

Our Configuration Servers can use a local or network file system on your computer, an FTP server or an HTTP server to store and provide access to Configurations.

In this topic we will describe how to setup configuration servers.

Local and Network File System based Configuration Servers
----------------------------------------------------------
Configuration Servers based on a file system are the easiest to setup, however the accessibility is limited to a LAN/WAN. Generally, you do not have your file system shared on the Internet for everyone to use (although it may work on DropBox or Google Drive, we have not tested this). File system based configuration servers are useful when developing configurations or when working with a limited group.

To set up the server:
1. Determine the location on your file system where you would like the server to reside. Let's say G:\\MyTOGA\\.
2. Copy your Master Library to G:\\MyTOGA\\. The Master Library is a file you create with the PGS Library Editor application. The file has a .LBR extension.
3. Create a folder for template files, let's say G:\\MyTOGA\\MyTemplates\\
3. Copy your TOGA Project Templates (*.togt), icon files (*.ico) and TogaTemplate.pgs to G:\\MyTOGA\\MyTemplates. 
4. Let everyone know how to access your configuration.

> NOTE: Local and Network File System based servers do not use publishing instructions or configuration documentation.

See @ref configurations in the @ref adminstrator_guide to learn how to configure TOGA to use a file system based configuration. You will want to provide your users with the following information that is to be entered in the Configuration Server Definition window.

Item | Value
-----|----------
Name | This can be any name. Specify one, or let your users decide
Server Type | Local or Network File System
Master Library File | The full path to your Master Library file. (e.g. G:\\MyTOGA\\MasterLibrary.lbr)
Path to Template Files | The full path to the folder containing your Project Templates. (e.g. G:\\MyTOGA\\MyTemplates\\). The TOGA Project Templates folder contains all of your templates (*.togt), icon files (*.ico) and TogaTemplate.pgs

FTP and HTTP based Configuration Servers
----------------------------------------
We have a little more work to do before setting up an FTP or HTTP based configuration server. 

### Configuration Documentation ###
Let's start by writing the optional configuration documentation. This is simply an HTML page that contains information about your configuration. You can create any web page you want, but the documentation generally should contain the following information:
* Description
* Publishing Date
* Warnings and Limitations
* Support contact information
* Revision log

The configuration documentation must be placed on an HTTP or FTP server where users can access it. The location of the configuration documentation is recorded in the publishing instructions as described below. The @ref dialogs_configure window has a hyperlink that, when pressed, will display your configuration documentation.

![](ConfigureWindow.png)

### Building the Configuration Package ###
The Master Library and Project Templates must be placed into a Configuration Package file (*.pgz) that will be stored on the configuration server. The configuration package allows the software to download a single file when installing a configuration, rather than downloading the Master Library file and additional files for each Project Template. TOGA validates the file contents using an mD5 checksum to insure that the file contents have not been corrupted during download.

The MakePGZ.exe utility program was installed with BrigeLink. It can be found in the BridgeLink application folder. MakePGZ creates the Configuration Package.

To make a configuration package, use the command:
   
     MakePGZ.exe pgz_file master_library_file project_template_folder TOGT PGS

where

Parameter | Description
----------|------------
pgz_file | The name of the TOGA Configuration Package (PGZ file) that will be created
master_library_file | The name of your Master Library file
project_template_folder | The name of the folder containing your project templates (*.togt files), icon files (*.ico) and TogaTemplate.pgs. 
TOGT PGS | The file extension for your templates and pgs file (use TOGT PGS literally)

Example:

    C:\Program Files\WSDOT\BridgeLink\MakePGZ.exe C:\MyFolder\MyState.pgz  G:\MyTOGA\MasterLibrary.lbr  G:\MyTOGA\MyTemplates\ TOGT PGS

This command will generate two files: C:\\MyFolder\\MyState.pgz, and C:\\MyFolder\\MyState.pgz.mD5. Both of these files should be published onto your server.
	
> Note: The template folder is a single folder containing all of your *.togt files, *.ico files, and TogaTemplate.pgs. This is unlike how a multi-folder structure is used in PGSuper or PGSplice templates. Also, note that MakePGZ.exe is dumb: it does not verify that all of the files necessary to run TOGA are present.

### Publishing Instructions ###
Next, we have to create the publishing instructions file. Publishing instructions are provided in a specially-named text file called TOGAPackages.ini. The content of the file provides information about the configurations you are publishing. 

Here is an example TOGAPackages.ini file for an FTP server:

    [WSDOT]
    WebLink="http://www.wsdot.wa.gov/eesc/bridge/software/TOGA/LibraryInfo.html"
    Format="pgz"
    Version_2.9.1_PgzFiles="ftp://ftp.wsdot.wa.gov/public/bridge/Software/TOGA/Version_2.9.1/WSDOT.pgz"
    Version_3.0.0_PgzFiles="ftp://ftp.wsdot.wa.gov/public/bridge/Software/TOGA/Version_3.0.0/WSDOT.pgz"

    [AASHTO]
    WebLink="http://www.wsdot.wa.gov/eesc/bridge/software/TOGA/LibraryInfo.html"
    Format="pgz"
    Version_2.9.1_PgzFiles="ftp://ftp.wsdot.wa.gov/public/bridge/Software/TOGA/Version_2.9.1/AASHTO.pgz"
    Version_3.0.0_PgzFiles="ftp://ftp.wsdot.wa.gov/public/bridge/Software/TOGA/Version_3.0.0/AASHTO.pgz"

Here is an example TOGAPackages.ini file for a file system server:

	[TxDOT]
	Format="pgz"
	WebLink="http://ftp.dot.state.tx.us/pub/txdot-info/brg/TOGA/TxDOT_TOGA_LibraryInfo.htm"
	Version_2.9.1_PgzFiles="C:\Shared\TogaServers\txdot.pgz" 
	Version_3.0.0_PgzFiles="C:\Shared\TogaServers\txdot30.pgz" 

Instructions for a configuration begin with the configuration name in square brackets. For the FTP example, publishing instructions are for two configurations; WSDOT and AASHTO.

Following the configuration name are the publishing instructions for the configuration. The instructions are a keyword-value pair.

Keyword | Description
--------|-----------
WebLink | URL of the configuration documentation. Omit if configuration documentation is not used.
Format  | Format of the configuration package. "pgz" is the only valid value at this time
Version_x.y.z_PgzFiles | Location of the configuration package. Substitute the application version number for x.y.z. In the examples above, configuration packages are available for TOGA version 2.9.1 and 3.0.0

When installing a configuration, TOGA will look for the configuration package with the highest version number that is not greater than its own version number. If you are use TOGA Version 2.9.5, the configuration package for version 2.9.1 will be installed.

> NOTE: All configuration packages in an .ini file must be of the same type (e.g., links in an .ini file on an FTP server must all be FTP links). The server type must match the type defined in the @ref configuration_server_definition.

Use NOTEPAD.EXE or any other plain text editor to create your publishing instructions. Be sure to save your publishing instructions with the name TOGAPackages.ini

### Publishing your Configuration ###
Your configuration is now ready to be published on the configuration server and accessed by engineers all around the world.

To publish your configuration:
1. Open Windows Explorer or an ftp client to the location on your HTTP server where the configuration documentation will be stored. Copy the documentation onto the server
2. Open Windows Explorer or an ftp client to the location on the server where you will be storing the publishing instructions and configuration packages. Copy the publishing instructions (TOGAPackages.ini) and the configuration packages (the .PGZ and .PGZ.mD5 files).
3. Let everyone know how to access your configuration.

See @ref configurations to learn how to configure TOGA to use an FTP or HTTP based configuration. You will want to provide your users with the following information that is to be entered in the PGSuper Configuration Server Definition window.

Item | Value
-----|----------
Name | This can be any name. Specify one or let your users decide
Server Type | The server type you are using, either Internet FTP Server, Internet HTTP Server, or file system.
Server Address (URL) | The fully qualified URL for your server. This is the location where the INI file is stored. (e.g. http://www.mycompany.com/PGSuper/ConfigurationServer/)
