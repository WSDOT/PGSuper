Component Manifests {#component_manifests}
=============================================

# Why manifests exist
PGSuper/PGSplice do not use the Windows registry or classic COM self-registration
(`DllRegisterServer`, `.rgs` scripts, Component Category Manager) to discover Project Importers,
Data Importers, Data Exporters, Extension Agents, or even the application's own core Agents.
Registering with the Windows registry's Component Categories requires elevated (administrator)
privileges, which WSDOT's IT Division considers an unacceptable security requirement to place on
developers just to enable a plugin. Instead, all of this is registered in a small, human-readable,
file-based **manifest** - registration information that anyone can create or edit without admin
rights. This is the `WBFL::EAF::ComponentManager` mechanism described in general terms in
`WBFL\EAF\devdocs\ComponentObjectModel.md`; this page covers it specifically as it applies to
building a Project Importer, Data Importer, Data Exporter, or Extension Agent (see
\ref extensibility "Extensibility").

# What a manifest is
A manifest is a JSON file listing one or more components: each component's friendly name, CLSID,
the DLL that implements it, and the component category (or categories - see below) it belongs to.
Here is a real manifest, `Examples.Manifest.PGSuper`, that registers the example plug-ins covered by
\ref creating_a_project_importer "Creating a Project Importer",
\ref creating_a_data_plugin "Creating a Data Plugin", and
\ref creating_an_extension_agent "Creating an Extension Agent":

~~~
{
    "Component": {
        "Name": "Example Extension Agent",
        "CLSID": "{C75182FE-7DAC-47A1-B87C-8BFA0F3C5D35}",
        "dll": "ExtensionAgentExample.dll",
        "Categories": {
            "CATID": "{4B5AD09A-E616-468E-8F84-4F9D68E7B6CA}",
            "CATID": "{5875EE83-78FE-4181-834A-C979FAD24222}"
        }
    },
    "Component": {
        "Name": "PGSuper Example Data Importer",
        "CLSID": "{8D7B7110-D77E-4393-B28D-8A75586BE29B}",
        "dll": "IEPluginExample.dll",
        "Categories": {
            "CATID": "{BD3B6F1E-7826-478B-99C0-A946C12C89CF}"
        }
    }
}
~~~
Note the Extension Agent entry belongs to *two* categories at once (`CATID_PGSuperExtensionAgent`
and `CATID_PGSpliceExtensionAgent`) since one DLL supports both applications, while each
importer/exporter entry belongs to exactly one category - the specific role it plays.

# Registration has two independent halves
Getting a component discovered and creatable takes two separate pieces of registration, and it's
easy to do one and forget the other:

1. **Inside your DLL** - a CLSID -> C++ class mapping, so the framework can actually construct your
   class once it decides to. This is the `EAF_BEGIN_OBJECT_MAP`/`EAF_OBJECT_ENTRY`/`EAF_END_OBJECT_MAP`
   block built at `InitInstance()` time (see `IEPluginExample.cpp` or
   `ExtensionAgentExample\dllmain.cpp` for worked examples). Without this, the manifest entry points
   at a CLSID your DLL doesn't know how to create.
2. **In a manifest file next to your DLL** - which CLSID(s) exist, which DLL implements them, and
   which category (role) each one belongs to. Without this, the host never learns your DLL exists
   at all, no matter how correct your object map is.

# Building your own manifest
A manifest can be hand-written as a plain JSON file matching the structure above - there's nothing
magic about it, and for a single-component manifest that's often the fastest path. The more
convenient approach for anything beyond a one-off, though, is what WSDOT itself does: write a small
developer utility application whose only job is to build your manifest(s) programmatically by
calling `WBFL::EAF::ComponentManager` and saving the result. `BridgeLink\BridgeLinkManifestBuilder`
is exactly this - a standalone utility, not part of any shipping product, that WSDOT runs to
regenerate every manifest it distributes. A third-party developer would write their own equivalent
utility, following the same pattern:

~~~
#include <EAF\ComponentManager.h>

WBFL::EAF::ComponentManager::GetInstance().RegisterComponent(
   _T("My Company's Data Importer"),   // friendly name shown in the Manage Plugins dialog
   CLSID_MyDataImporter,               // your own CLSID - generate with GUIDGEN, don't reuse an example's
   _T("MyDataImporter.dll"),           // DLL name only, or a fully qualified path - never a relative path
   CATID_PGSuperDataImporter);         // the role this component plays

WBFL::EAF::ComponentManager::GetInstance().Save(_T("MyCompany.Manifest.PGSuper"));
~~~
Building a small utility this way instead of hand-editing JSON pays off as soon as you have more
than one component, more than one target application, or want the manifest regenerated
automatically as part of your build - `RegisterComponent`/`Save` guarantee well-formed output and a
single source of truth for your CLSIDs, categories, and DLL names.

# Naming and placement
Manifest files must be named `<Type>.Manifest.<Application>`, where `Type` is any descriptive label
for the set of components in the file (`Application`, `Extensions`, `Examples`, your company name -
whatever groups your registrations sensibly) and `Application` is the target application's name
(`PGSuper`, `PGSplice`, `BridgeLink`, `BEToolbox`, ...) - the same name used to form the
`CATID_<Application>*` identifiers you're registering against. Ship your manifest file next to your
DLL.

# How manifests are discovered at startup
`WBFL::EAF::ComponentManager::Load` searches for every file matching `*.Manifest.<Application>`,
starting with the application's own folder (where the `.exe` lives), then every directory listed in
the `<APPLICATION>_PATH` environment variable (semicolon-separated, `<APPLICATION>` again being the
target application's name). Every manifest found is merged into the same `ComponentManager`
instance, which is why a third party can add a manifest of their own without touching or replacing
anything WSDOT ships - your manifest is simply one more file the search finds. Once all manifests
are loaded, `CEAFBrokerDocument::LoadAgents()` and the Project Importer / Data Importer / Data
Exporter plugin managers ask `ComponentManager::GetComponents(catid)` for everything registered
under the category they care about, and instantiate each one on demand.
