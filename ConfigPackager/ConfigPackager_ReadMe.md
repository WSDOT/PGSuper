ConfigPackager
==============
ConfigPackager.exe builds the files a configuration server publishes for PGSuper, PGSplice and TOGA:
configuration packages, and ConfigurationPackages.json, the list of configurations. It was called
MakePgz.exe in earlier versions. It no longer needs md5deep.exe.

See "Configuration Servers" in the PGSuper Administrator Guide for the full publishing instructions.

Commands
--------
Create a package (a standard zip file). Also writes Package.zip.md5 for older program versions:

    ConfigPackager Package.zip Library.lbr RootTemplateFolder TemplateExtension [TemplateExtension2]

    TemplateExtension is PGT for PGSuper, SPT for PGSplice, and "TOGT PGS" for TOGA.

List or uncompress a package (.zip or .pgz):

    ConfigPackager Package.zip /L
    ConfigPackager Package.zip /U

Add a package to ConfigurationPackages.json, with its MD5 (the file is created if needed):

    ConfigPackager /AddPackage ConfigurationPackages.json Program "Configuration" ProgramVersion Package.zip [/Url=address] [/WebLink=address]

Create ConfigurationPackages.json from <Program>Packages.ini files:

    ConfigPackager /Convert ConfigurationPackages.json PGSuperPackages.ini [PGSplicePackages.ini ...] [/Base=address] [/WebLink=address] [/Zip]

    /Base    package addresses below this address become relative to the JSON file
    /WebLink documentation address to use for every configuration
    /Zip     package addresses ending in .pgz end in .zip instead (rename the files when you publish them)

Fill in the MD5 of every package stored with the JSON file:

    ConfigPackager /UpdateMd5 ConfigurationPackages.json

Check a JSON file before publishing it (exit code 0 when there are no problems):

    ConfigPackager /Check ConfigurationPackages.json

Write a checksum list for a "Local or Network File System" server:

    ConfigPackager /Md5List Library.lbr Library.lbr.md5
    ConfigPackager /Md5List TemplateFolder TemplateFolder\WorkgroupTemplates.md5

Source
------
ConfigurationManifest.h/.cpp (reading and writing ConfigurationPackages.json, reading legacy INI files,
version selection and MD5) is shared with PgsExt, which compiles it into the programs. The zip code
(zip.cpp, unzip.cpp, UnzipPgz.cpp) is shared the same way. Tests are in PGSuperUnitTests.
