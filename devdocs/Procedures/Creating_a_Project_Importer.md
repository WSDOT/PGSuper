Creating a Project Importer {#creating_a_project_importer}
=============================================================

# Purpose
This procedure describes how to build a Project Importer plug-in for PGSuper/PGSplice. A Project
Importer is a substantially more involved mechanism than a Data Importer/Exporter (see
\ref creating_a_data_plugin "Creating a Data Plugin") because it isn't just a menu command - it is
effectively a plug-in that backs an MFC document-template object, and it participates directly in
document creation through the New Project dialog. See \ref extensibility "Extensibility" for how
this role compares to the others.

# The interface
`PGS::IProjectImporter` is declared in `Include\Plugins\PGSuperIEPlugin.h`:
~~~
namespace PGS
{
   class IProjectImporter
   {
   public:
      virtual CString GetItemText() const = 0;
      virtual HRESULT Import(std::shared_ptr<WBFL::EAF::Broker> pBroker) = 0;
      virtual HICON GetIcon() const = 0;
      virtual CLSID GetCLSID() const = 0;
      virtual CString GetTemplateFilePath() const = 0;
   };
};
~~~
A concrete implementation derives from `WBFL::EAF::ComponentObject` (the base for every component in
this component model) and `PGS::IProjectImporter`, the same shape as the toy implementations in
`PGSuper\IEPluginExample` (`CPGSuperProjectImporter`, `CPGSpliceProjectImporter`):
~~~
class CMyProjectImporter : public WBFL::EAF::ComponentObject,
                            public PGS::IProjectImporter
{ ... };
~~~

# How a Project Importer becomes part of "New Project"
A Project Importer is not called directly by application code the way an `IDataImporter` is invoked
from a menu command - it is wrapped into a `CEAFTemplateItem` and appears in the New Project dialog
alongside ordinary file-based (`.pgt`) templates. This wiring happens in several layers, all under
`PGSuper\PGSuperAppPlugin`:

- **`CEAFTemplateItem`** (`Include\EAF\EAFTemplateGroup.h`) is one entry in the New dialog's icon
  list - a name, an icon, an optional template file path, and a back-pointer to the
  `CEAFDocTemplate` that will build the document. `CPGSProjectImporterTemplateItem` (declared at the
  bottom of `PGSuperIEPlugin.h`) is a thin subclass that additionally holds
  `std::shared_ptr<PGS::IProjectImporter> m_Importer`.
- **`CEAFTemplateGroup`** (same header) is a tree node in the New dialog's left-hand list, holding
  child items and/or child groups - this is how importer-backed items and ordinary file templates
  end up grouped together under one application node.
- **`CEAFDocTemplate`** (`Include\EAF\EAFDocTemplate.h`) subclasses MFC's `CMultiDocTemplate` and
  owns one `CEAFTemplateGroup`. `PGSuperAppPlugin\PGSImportPluginDocTemplateBase.h/.cpp` declares the
  doc template subclass used specifically for project importers - its `GetProjectImporterManager()`
  lazily discovers every registered `IProjectImporter` (via
  `PGSuperAppPlugin\PGSProjectImporterMgrBase.h/.cpp`'s `LoadImporters()`, which asks
  `WBFL::EAF::ComponentManager` for every component under `CATID_PGSuperProjectImporter` /
  `CATID_PGSpliceProjectImporter` - see \ref component_manifests "Component Manifests") and, the
  first time it's called, wraps each one:
  ~~~
  auto strText = importer->GetItemText();
  HICON hIcon  = importer->GetIcon();
  auto path    = importer->GetTemplateFilePath();
  auto template_item = new CPGSProjectImporterTemplateItem(this, strText, path, hIcon, importer);
  m_TemplateGroup.AddItem(template_item);
  ~~~
  `GetItemText()`, `GetIcon()`, and `GetTemplateFilePath()` are each read exactly once here, not
  live by the dialog on every open.
- `PGSuperAppPlugin\PGSProjectImporterPluginAppBase.cpp`'s `CreateDocTemplates()` is what triggers
  this discovery and registers the resulting doc template with `CEAFDocManager::AddDocTemplate()`.
  If there are zero enabled importers, the doc template is never registered at all - the group
  simply doesn't appear in the New Project dialog.

# `GetTemplateFilePath()`: template-backed vs. from-scratch
This single virtual method controls how the new document is seeded *before* `Import()` runs, and is
the most consequential decision in a Project Importer's design:

- **Empty string** - the document starts bare. `CEAFDocTemplate::DoOpenDocumentFile` calls
  `pDocument->OnNewDocument()`, which stands up a broker and loads every agent (core agents, then
  extension agents via `Broker::LoadExtensionAgents`), but loads no project data. `Import()` alone is
  responsible for building an entire bridge model from nothing - this is what the toy
  `IEPluginExample` implementation does.
- **Path to a `.pgt` file** - `DoOpenDocumentFile` instead calls
  `pDocument->OnNewDocumentFromTemplate(path)`, which opens that file's structured storage and
  deserializes it into the already-live broker/agents - the same file format used for an ordinary
  "create from template" document, with whatever defaults, units, spec, and library references the
  template author baked into it. `Import()` then layers external data on top of that starting point
  instead of building everything itself.

Since this is an ordinary virtual method rather than a static manifest property, a real
implementation can compute the path dynamically (e.g. choose among several bundled templates based
on the external source's contents) - though neither PGSuper implementation shipped with this
codebase does that.

# The full call sequence
1. The user picks the importer's item in the New Project dialog (`CNewProjectDlg`, populated from
   the `CEAFTemplateGroup` tree built above).
2. `CEAFDocManager::OnFileNew()` calls `pTemplate->SetTemplateItem(item)`, then
   `pTemplate->OpenDocumentFile(nullptr, FALSE, TRUE)`.
3. `CEAFDocTemplate::OpenDocumentFile()` constructs the document (e.g. `CPGSuperDoc`) and its MDI
   frame, calls `OnCreateInitialize()`, then calls the virtual `DoOpenDocumentFile()`.
4. `CPGSImportPluginDocTemplateBase::DoOpenDocumentFile()` first calls the **base class**
   `CEAFDocTemplate::DoOpenDocumentFile()` - this is the `OnNewDocument()`/`OnNewDocumentFromTemplate()`
   branch described above. By the time that call returns, the document has a fully operational
   broker with every agent already loaded and initialized (and, if template-backed, the template's
   data already deserialized into those agents). Only then does it hold UI events
   (`IEvents::HoldEvents()`) and call `pTemplateItem->m_Importer->Import(broker)`.
5. On success, `InitialUpdateFrame()` creates and attaches the views, then `OnCreateFinalize()`
   releases the held UI events - so `Import()`'s changes are never rendered piecemeal, only after
   the whole document is ready to display.
6. If `Import()` returns a failed `HRESULT`, `DoOpenDocumentFile()` returns `FALSE`; the half-built
   document and frame are destroyed and the New Project attempt simply aborts - there is no
   partially-created project left visible. It's the importer's own responsibility to give the user
   useful diagnostics before returning failure.

**The key takeaway**: `Import()` never bootstraps the broker itself, and is never responsible for
loading agents. It always receives a fully operational broker - core agents, extension agents, and
(optionally) a pre-loaded template's data already in place - and its only job is to populate or
adjust project data using the same public interfaces the UI itself uses (`GET_IFACE2` calls into
`IRoadwayData`, `IBridgeDescription`, `ILibrary`, and so on under `Include\IFace`), exactly the way a
Data Importer does, just at document-creation time instead of against an already-open project. The
resulting document is an ordinary `CPGSuperDoc`/`CPGSpliceDoc` - importing doesn't produce a special
document type.

# Registering a new Project Importer
1. Pick a CLSID for your class (GUIDGEN - don't reuse an example's). Your CATID is
   `CATID_PGSuperProjectImporter` or `CATID_PGSpliceProjectImporter` (both declared in
   `PGSuperIEPlugin.h`).
2. Add your class to your DLL's `EAF_BEGIN_OBJECT_MAP`/`EAF_OBJECT_ENTRY`/`EAF_END_OBJECT_MAP` block
   (see `IEPluginExample.cpp` for a worked example) so the framework can construct your class by
   CLSID.
3. Register your CLSID under your CATID in a manifest file - see
   \ref component_manifests "Component Manifests" for the full explanation.
4. Decide whether your importer is template-backed or from-scratch, and implement
   `GetTemplateFilePath()` accordingly.
5. Implement `Import()` against the broker's public interfaces, following the sequence above -
   remember it will already have a live broker and (if template-backed) pre-loaded data to build on.

# Optional: integrating your own help content
A Project Importer can additionally implement `PGS::IPluginDocumentation` (also in
`PGSuperIEPlugin.h`) to hook its own documentation into the host's help system.
