Library Conflict {#ug_dialogs_library_conflict}
==============================================
PGSuper and PGSplice project files are completely self-contained. Even though your bridge model references entries in a library, copies are those entries are kept in the project file. This allows you to open your project on a computer that doesn't have the PGSuper or PGSplice Configuration originally used or share your project with a colleague on the other side of the world that doesn't have access to your PGSuper/PGSplice Configuration. 

If your library in your configuration is changed, by perhaps your PGSuper/PGSplice Administrator, or if your colleague should happen to have a library entry in his configuration that has the same name, but different values than one of the entries in your project, the software need guidance for resolving the conflict.

We leave it up to you to resolve the conflict. At this point you have two choices:

Choice | Description
-------|--------------
[Rename Entry] | Use this option to rename the entry in your project file that is in conflict. All references to this library entry will be updated to refer to the renamed entry. By selection this option, all data remain unchanged (except for the entry's name)
[Overwrite] | Use this option to overwrite the library entry in your project file with the one from the current configuration. This updates your project data.

The differences list will describe what is different between the conflicting library entries. The difference list may contain specific data values or a general description of the difference.s

Remarks
--------
There are several situations where you will encounter a Library Entry Conflict. Two typical scenarios are described below.

* You are currently working on a precast girder design. The last time you worked with your project file everything was fine. This time, you open the project and you get a Library Entry Conflict. For some reason your PGSuper/PGSplice Configuration was updated (maybe your office practice changed or there was an error in the library data). If you want to use the latest configuration, press [Overwrite]. If you want to keep your project the way it was the last time you were working on it, press [Rename Entry].
* You retrieve a project file from a backup. The project file is several years old. When you open the project file you get a Library Entry Conflict. Depending on your reason for revisiting this old project, you may or may not what to update the data. If you want to analyze the data as it was when the project was archived, press [Rename Entry]. If you are using the old project as a template for a new project, you might want to press [Overwrite] to update the old library entries so they match the current configuration.
