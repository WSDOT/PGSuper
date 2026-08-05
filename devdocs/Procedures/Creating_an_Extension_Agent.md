Creating an Extension Agent {#creating_an_extension_agent}
============================================================

# Purpose
This procedure describes how to build an Extension Agent for PGSuper/PGSplice, using the
`ExtensionAgentExample` project (`PGSuper\ExtensionAgentExample`) as a worked example. An Extension
Agent is an optional agent in the \ref architecture "Agent/Broker architecture" - it has the same
base as a required core agent, but is loaded on demand and typically integrates with the UI,
reporting, and graphing subsystems. Additionally an Extension Agent can perform engineering calculations to extend the functional capabilities of the program. See
\ref extensibility "Extensibility" for how this compares to a Project Importer, Data Importer, or
Data Exporter, and \ref adding_an_interface_to_an_agent for the general mechanics of adding a
broker-registered interface to any agent.

# The agent class
Every agent - core or extension - derives from `WBFL::EAF::Agent` (`Include\EAF\Agent.h`) and
overrides `GetName()`, `RegisterInterfaces()`, `Init()`, `Reset()`, `ShutDown()`, and `GetCLSID()`.
What makes an agent an *extension* agent is not a special interface, but (a) its CATID
registration (see Registration below) and (b) which optional "integration" mixins it implements.
`CExampleExtensionAgent` (`ExampleExtensionAgent.h`) implements all four:

~~~
class CExampleExtensionAgent : public CCmdTarget, // must be listed first - see Warning C4407
   public WBFL::EAF::Agent,
   public WBFL::EAF::IAgentPersist,             // persist data into the project file
   public WBFL::EAF::IAgentUIIntegration,       // add menus/toolbars/views/tabs
   public WBFL::EAF::IAgentReportingIntegration,// add or modify reports
   public WBFL::EAF::IAgentGraphingIntegration, // add graphs
   public IEditBridgeCallback, public IEditPierCallback, /* ...one per dialog it extends... */
   public IExtendUIEventSink,
   public WBFL::EAF::ICommandCallback
{ ... };
~~~
`CCmdTarget` must come first in the inheritance list (see the warning comment in the header itself)
because of how MFC message-map dispatch resolves the vtable.

# Capabilities

## Property-page tabs
Declared in `Include\IFace\ExtendUI.h`. For each dialog you want to extend there's an
`IEditXxxCallback` interface (e.g. `IEditPierCallback`, `IEditGirderCallback`) with a
`CreatePropertyPage(...)` and an `OnOK(...)` method, plus PGSuper-only and PGSplice-only
registration hubs, `IExtendPGSuperUI`/`IExtendPGSpliceUI` (both extend the common `IExtendUI`).
Register your callback in `RegisterInterfaces()`/an equivalent setup method:
~~~
GET_IFACE(IExtendPGSuperUI, pExtendPGSuperUI);
pExtendPGSuperUI->RegisterEditPierCallback(this, nullptr);
~~~
(`ExampleExtensionAgent.cpp`, `RegisterUIExtensions()`). Every `IEditXxxCallback` also derives from
`IExtensionPageCallback`, whose `GetPropertyPagePosition()` lets you say where your tab goes
(`AtStart()`/`AtEnd()`/`Before(name)`/`After(name)`/`AtIndex(n)`) - the default is `AtEnd()`, so
existing extension agents that don't override it keep today's append-only behavior.
`ExtensionAgentExample` demonstrates two shapes of page: `CEditPierPage` (`EditPierPage.h`) reads
live data through the `IEditPierData*` its constructor is given, while `CExtensionPage`
(`ExtensionPage.h`) is a deliberately simpler, self-contained page used by every other callback.

## Menu and toolbar commands
`Include\EAF\EAFUIIntegration.h` declares `IEAFMainMenu` (get the main menu / build a context menu)
and `IEAFToolbars` (create/get/destroy a toolbar). Commands you add are routed back to your agent
through `WBFL::EAF::ICommandCallback::OnCommandMessage`, handled with an ordinary MFC message map:
~~~
GET_IFACE(IEAFMainMenu, pMainMenu);
m_MyMenu = pMainMenu->GetMainMenu()->CreatePopupMenu(...);
...
BEGIN_MESSAGE_MAP(CExampleExtensionAgent, CCmdTarget)
   ON_COMMAND(ID_COMMAND1, &CExampleExtensionAgent::OnCommand1)
END_MESSAGE_MAP()
~~~
(`ExampleExtensionAgent::CreateMenus()`/`CreateToolBar()`). This example only shows adding a new
top-level menu and a new toolbar - for splicing commands into an *existing* menu, adding buttons to
an existing toolbar, accelerator keys, and the full API, see
\ref WBFL_EAF_UIIntegration "UI Integration" in the WBFL EAF devdocs.

## Views
`IEAFViewRegistrar::RegisterView` registers a `CView`/`CFrameWnd` pair the same way PGSuper's own
report and graph views are registered. `ExampleExtensionAgent::RegisterViews()` registers `CMyView`
(`MyView.h`), which holds a `std::shared_ptr<ICommandCallback>` back to the agent for routing its
own commands. See \ref WBFL_EAF_UIIntegration "UI Integration" in the WBFL EAF devdocs for the full
`IEAFViewRegistrar` API and the related `IEAFViewController`.

## Reports

### Adding a new report
`Include\EAF\EAFReportManager.h`'s `IEAFReportManager::AddReportBuilder` registers a brand new
report. `ExampleExtensionAgent::RegisterReports()` builds a
`WBFL::Reporting::ReportBuilder("Extension Agent Report")` with a `CMyReportSpecificationBuilder`
(`MyReportSpecificationBuilder.h`) and a `CMyChapterBuilder` (`MyChapterBuilder.h`).

### Modifying an existing report
To add or remove a chapter on a report *another* agent defined - rather than building your own -
look the report up by name with `IEAFReportManager::GetReportBuilder`, then call
`WBFL::Reporting::ReportBuilder::InsertChapterBuilder`/`RemoveChapterBuilder` on the result:
~~~
GET_IFACE(IEAFReportManager, pReportMgr);
auto pReportBuilder = pReportMgr->GetReportBuilder(_T("Some Existing Report"));
pReportBuilder->InsertChapterBuilder(std::make_shared<CMyChapterBuilder>(), _T("SomeExistingChapterKey"));
~~~
`ExtensionAgentExample` doesn't exercise this path - it only adds chapters to its own brand-new
report via `AddChapterBuilder` - but the API is there for extending someone else's report.

See \ref WBFL_EAF_Reporting "Reporting" in the WBFL EAF devdocs for the full `IEAFReportManager` API,
including `ReportBuilder::Hidden()` versus `RemoveReportBuilder` for hiding a report without fully
unregistering it.

## Graphs
`Include\EAF\EAFGraphManager.h`'s `IEAFGraphManager::AddGraphBuilder` registers a brand new graph.
`ExampleExtensionAgent::RegisterGraphs()` adds three `CTestGraphBuilder*` instances
(`TestGraphBuilder.h`). See \ref WBFL_EAF_Graphing "Graphing" in the WBFL EAF devdocs for the full
`IEAFGraphManager` API.

## Persisting data
Implement `IAgentPersist::Load`/`Save` (`Include\EAF\Agent.h`) using the native
`WBFL::System::IStructuredLoad`/`IStructuredSave` interfaces:
~~~
WBFL::EAF::Broker::LoadResult CExampleExtensionAgent::Load(WBFL::System::IStructuredLoad* pStrLoad)
{
   if (!pStrLoad->BeginUnit(_T("ExampleExtensionAgent")))
      return WBFL::EAF::Broker::LoadResult::Error;

   std::_tstring answer;
   if (!pStrLoad->Property(_T("SampleData"), &answer))
      return WBFL::EAF::Broker::LoadResult::Error;
   m_Answer = answer.c_str();

   if (!pStrLoad->EndUnit())
      return WBFL::EAF::Broker::LoadResult::Error;

   return WBFL::EAF::Broker::LoadResult::Success;
}

bool CExampleExtensionAgent::Save(WBFL::System::IStructuredSave* pStrSave)
{
   pStrSave->BeginUnit(_T("ExampleExtensionAgent"), 1.0);
   pStrSave->Property(_T("SampleData"), m_Answer);
   pStrSave->EndUnit();
   return true;
}
~~~
(`ExampleExtensionAgent.cpp`). Each agent's data is wrapped by the broker in its own `"Agent"` unit
tagged with the agent's CLSID (`Broker::SaveAgentData`, `WBFL\EAF\Broker.cpp`) - this is what makes
the "data is retained when opened without the extension agent installed" guarantee work, by
capturing and replaying the raw unit of any agent that isn't currently loaded. See
\ref WBFL_EAF_DataPersistence "Data Persistence" in the WBFL EAF devdocs for the full
`IStructuredLoad`/`IStructuredSave` API and exactly how that passthrough works.

## Documentation and Help
An extension agent can integrate its own help content with PGSuper/PGSplice's documentation system
by implementing `WBFL::EAF::IAgentDocumentationIntegration` (`Include\EAF\Agent.h`) - see
\ref WBFL_EAF_Documentation "Documentation and Help" in the WBFL EAF devdocs for the interface, the
`.dm` doc-map file format, and the `PluginAppDocumentationImpl` helper. This is a different interface
from `PGS::IPluginDocumentation` (`Include\Plugins\PGSuperIEPlugin.h`), which is the equivalent for
Project Importer/Data Importer/Data Exporter plug-ins rather than Extension Agents.

## Command Line Processing
An extension agent - like any agent - can handle its own command-line switches by implementing
`WBFL::EAF::IEAFProcessCommandLine` (`Include\EAF\EAFUIIntegration.h`). See
\ref WBFL_EAF_CommandLine "Command Line Processing" in the WBFL EAF devdocs for the interface and
how it's discovered (a plain RTTI probe over every loaded agent, not a broker interface lookup, so
it's available to core agents and extension agents alike).

# Registration
Extension agents use the same manifest mechanism as importers/exporters - see
\ref component_manifests "Component Manifests" for the full explanation of what a manifest is, how
to build your own, and how it's discovered at startup - but register under a different CATID pair,
declared in `Include\PGSuperCatCom.h` / `Include\PGSpliceCatCom.h`:
~~~
CATID_PGSuperExtensionAgent
CATID_PGSpliceExtensionAgent
~~~
`ExtensionAgentExample` itself is registered under *both* categories at once in
`Examples.Manifest.PGSuper` (shown in full in \ref component_manifests "Component Manifests"),
since the same DLL supports both PGSuper and PGSplice:
~~~
WBFL::EAF::ComponentManager::GetInstance().RegisterComponent(
   _T("Example Extension Agent"), CLSID_ExampleExtensionAgent,
   _T("ExtensionAgentExample.dll"),
   { CATID_PGSuperExtensionAgent, CATID_PGSpliceExtensionAgent });
~~~
At startup, `CEAFBrokerDocument::LoadAgents()` (`WBFL\EAF\EAFBrokerDocument.cpp`) loads required
agents first, then calls `Broker::LoadExtensionAgents(GetExtensionAgentCategoryID())` - unlike
required agents, a failure to load an extension agent doesn't stop the application, it just prompts
to disable that agent. Users can enable/disable installed extension agents from the **Manage
Plugins and Extensions** dialog (`PGSuperAppPlugin\PGSuperPluginApp.cpp`).

# Step-by-step
-# Pick a CLSID for your agent class (GUIDGEN - don't reuse the example's).
-# Derive your agent class from `WBFL::EAF::Agent` plus whichever `IAgentXxxIntegration` mixins you
   need, and whichever `IEditXxxCallback`/other interfaces you need, following
   `CExampleExtensionAgent`'s shape.
-# Implement `GetName()`, `RegisterInterfaces()`, `Init()`, `Reset()`, `ShutDown()`, `GetCLSID()`,
   plus whichever integration methods (`IntegrateWithUI`, `IntegrateWithReporting`,
   `IntegrateWithGraphing`) and `Load`/`Save` you need.
-# Add your class to your DLL's `EAF_BEGIN_OBJECT_MAP`/`EAF_OBJECT_ENTRY`/`EAF_END_OBJECT_MAP` (see
   `ExtensionAgentExample\dllmain.cpp`).
-# Register your CLSID under `CATID_PGSuperExtensionAgent` and/or `CATID_PGSpliceExtensionAgent` in
   a manifest, following the `PGSuper_Examples()` pattern above.
-# Build, install the manifest and DLL alongside the application, and enable your agent from the
   Manage Plugins and Extensions dialog.
