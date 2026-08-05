Creating a Data Plugin {#creating_a_data_plugin}
===================================================

# Purpose
This procedure describes how to build a Data Importer or Data Exporter plugin for PGSuper/PGSplice,
using the `IEPluginExample` project (`PGSuper\IEPluginExample`) as a worked example. Unlike a
Project Importer (see \ref creating_a_project_importer "Creating a Project Importer"), a data
plugin is a simple, on-demand operation against the *currently open* project - it does not
participate in document creation. See \ref extensibility "Extensibility" for how these roles
compare to the others.

# The interfaces
Both roles are plain C++ abstract classes declared in `namespace PGS` in
`Include\Plugins\PGSuperIEPlugin.h`:
~~~
namespace PGS
{
   class IDataImporter { ... };   // imports into the current project
   class IDataExporter { ... };   // exports from the current project
};
~~~
A single DLL can implement both roles - `IEPluginExample.dll` implements both, alongside its
Project Importers. Each concrete class derives from `WBFL::EAF::ComponentObject` (the base for every
component in this component model) and from the role interface itself:
~~~
class CPGSuperDataImporter : public WBFL::EAF::ComponentObject,
                              public PGS::IDataImporter
{ ... };
~~~
`IDataImporter::Import` and `IDataExporter::Export` read or write the *currently open* project - the
example importer changes the roadway alignment (`PGSuperDataImporter.cpp`), and the example exporter
writes pier and girder data to a text file (`PGSuperDataExporter.cpp`). Both receive a
`std::shared_ptr<WBFL::EAF::Broker>`, which is how they reach the host application's agents (the
interfaces under `Include\IFace`) to read or write project data - the broker handed to them is
always for a project the user already has open and fully loaded, so unlike a Project Importer there
is no document-creation sequence to understand first.

# Registering a new data plugin

## 1. Pick your CLSID and CATID
Each concrete class needs its own CLSID (use GUIDGEN, don't reuse the example's). The CATID
identifies which role the class plays:
~~~
CATID_PGSuperDataImporter / CATID_PGSpliceDataImporter
CATID_PGSuperDataExporter / CATID_PGSpliceDataExporter
~~~
(all declared in `PGSuperIEPlugin.h`).

## 2. Add your class to the DLL's object map
`IEPluginExample.cpp` builds a `WBFL::EAF::ComponentModule` and a CLSID -> C++ class map:
~~~
WBFL::EAF::ComponentModule _Module;
EAF_BEGIN_OBJECT_MAP(ObjectMap)
   EAF_OBJECT_ENTRY(CLSID_PGSuperDataImporter, CPGSuperDataImporter)
   EAF_OBJECT_ENTRY(CLSID_PGSuperDataExporter, CPGSuperDataExporter)
EAF_END_OBJECT_MAP()
~~~
`CIEPluginExampleApp::InitInstance()` calls `_Module.Init(ObjectMap)`. This is not classic COM
self-registration (there's no `.idl`/`.rgs` in the project, and no `DllGetClassObject`) - it's a
lightweight in-DLL factory that the framework calls through `boost::dll`
(see `Include\EAF\ComponentModule.h`).

## 3. Register the component in a manifest
Instead of the Windows registry, components are registered in a JSON manifest file next to your
DLL - see \ref component_manifests "Component Manifests" for the full explanation, including how to
build one and how it's discovered at startup.

## 4. Confirm the host can find it
At startup, the host application enumerates all registered components under a given CATID and
instantiates each one on demand. For Data Importers/Exporters this happens in
`PGSuperAppPlugin\PGSuperPluginMgr.h` (`GetImporterCATID()`/`GetExporterCATID()`). The **Manage
Plugins and Extensions** dialog (`PGSuperAppPlugin\PGSuperPluginApp.cpp`) lets the user see and
enable/disable everything registered under `CATID_PGSuperDataImporter`,
`CATID_PGSuperDataExporter`, and `CATID_PGSuperExtensionAgent`.

# Optional: integrating your own help content
A data plugin can additionally implement `PGS::IPluginDocumentation` (also in `PGSuperIEPlugin.h`)
to hook its own documentation into the host's help system - `IEPluginExample` does not implement
this, but the interface exists for plugins that ship their own help content.
