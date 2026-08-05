Extensibility {#extensibility}
==============================

# Overview
PGSuper and PGSplice can be extended by third-party developers without modifying or relinking
against the applications themselves. There are five extension mechanisms, each suited to a
different kind of integration:

- **Project Importer** - an application extension that creates a *new* project by importing
  information from an external source. Runs at "New Project" time and appears as an entry in the
  New Project template gallery.
- **Data Importer** - a plugin that imports data into the *currently open* project. Runs on demand
  from a File > Import menu command.
- **Data Exporter** - a plugin that exports data from the *currently open* project. Runs on demand
  from a File > Export menu command.
- **Extension Agent** - an agent in the \ref architecture "Agent/Broker architecture" that can
  extend the capabilities of the program more deeply and more persistently than an importer or
  exporter. An extension agent can add tabs (and thus custom data) to many dialogs, can add new
  commands to the menus and toolbars, can add new views, can add new reports and graphs, can modify
  reports defined by other agents (add and remove chapters), and can persist its own data into the
  PGSuper/PGSplice data file. Extension Agent persisted data is retained when the data file is
  opened on an installation that does not have the extension agent installed.
- **Beam Type** - adds a new precast beam cross-section shape, or an entirely new family of shapes,
  to the girder types PGSuper/PGSplice can model. Unlike the other four mechanisms, this is a
  structural engineering component, not just data/UI plumbing - it supplies cross-section geometry,
  LRFD distribution factor and prestress loss calculations, and the UI images used to present the
  shape.

Both `IEPluginExample` and `ExtensionAgentExample` (siblings of this `devdocs` folder, under
`PGSuper\`) are working, buildable example projects. `IEPluginExample` covers Project Importer,
Data Importer, and Data Exporter; `ExtensionAgentExample` covers Extension Agent. The core beam
types (`PGSuper\Beams`, built as `PGSuperBeams.dll`) are themselves ordinary implementations of the
Beam Type mechanism and serve as its reference.

# Licensing: interfaces vs. implementations
The interfaces a plugin or extension agent implements against - `PGS::IProjectImporter`,
`PGS::IDataImporter`, `PGS::IDataExporter`, `WBFL::EAF::Agent` and its mixins, the `IEditXxxCallback`
family, and the rest - are declared in headers licensed under the Alternate Route Library Open
Source License (ARLOSL). That license covers the *interface declarations only*. A developer's
*implementation* behind one of these interfaces - the concrete class in their own DLL - can be
licensed however that developer chooses, including as closed-source, proprietary code. This is a
deliberate design choice, not an oversight: exposing extension points as plain C++ abstract classes
(rather than requiring third parties to statically link ARLOSL-covered implementation code) is what
makes it possible for third parties to build proprietary plug-ins for PGSuper and PGSplice at all.
See the file header comment in `Include\Plugins\PGSuperIEPlugin.h` for the canonical statement of
this policy; the same policy applies to the Extension Agent interfaces.

# Choosing a mechanism
- Building an entire new project from an external data source (e.g. a bridge design tool's native
  file format)? Use a **Project Importer**. This is the most involved of the three plugin roles -
  it doesn't just run on demand, it effectively backs an MFC document-template object and
  participates in document creation through the New Project dialog (see
  \ref creating_a_project_importer "Creating a Project Importer").
- Merging or applying external data into a project the user already has open, on demand? Use a
  **Data Importer** (or **Data Exporter** for the reverse direction) - a much simpler, self-contained
  operation than a Project Importer (see \ref creating_a_data_plugin "Creating a Data Plugin").
- Need an ongoing presence in the application - your own dialog tabs, toolbar commands, views,
  reports/graphs, or data that has to round-trip through the project file? Use an **Extension
  Agent**.
- Adding a new precast girder shape, or a shape that doesn't fit any of the existing families? Use
  a **Beam Type** (see \ref creating_a_beam_type "Creating a Beam Type"). This is the most capable
  and most involved of the five mechanisms, and the only one that's a structural engineering
  component rather than data/UI plumbing.

# Comparison

Mechanism | Purpose | Key interface(s) | PGSuper CATID | PGSplice CATID | Example project
----------|---------|-------------------|----------------|-----------------|----------------
Project Importer | Creates a brand new project from an external source | `PGS::IProjectImporter` | `CATID_PGSuperProjectImporter` | `CATID_PGSpliceProjectImporter` | `IEPluginExample`
Data Importer | Imports data into the currently open project | `PGS::IDataImporter` | `CATID_PGSuperDataImporter` | `CATID_PGSpliceDataImporter` | `IEPluginExample`
Data Exporter | Exports data from the currently open project | `PGS::IDataExporter` | `CATID_PGSuperDataExporter` | `CATID_PGSpliceDataExporter` | `IEPluginExample`
Extension Agent | Deep, persistent UI/data/report integration | `WBFL::EAF::Agent` + `IAgent*Integration` mixins | `CATID_PGSuperExtensionAgent` | `CATID_PGSpliceExtensionAgent` | `ExtensionAgentExample`
Beam Type | Adds a new precast girder cross-section shape or family | `PGS::Beams::BeamFactory` (+ `PGS::Beams::BeamFamily` for a new family) | `CATID_PGSuperBeamFamily` + a per-family factory CATID | `CATID_PGSpliceBeamFamily` + a per-family factory CATID | `PGSuperBeams`

All of these `CATID_*` values are declared in `Include\Plugins\PGSuperIEPlugin.h` (importer/exporter
roles), `Include\PGSuperCatCom.h` / `Include\PGSpliceCatCom.h` (extension agent and beam family
roles), and `Include\Plugins\BeamFactoryCATID.h` (one dedicated category per beam family, for its
member factories). None of this uses the Windows registry or classic COM self-registration - every
one of these five mechanisms is registered in a file-based **manifest** instead, so building and
distributing a plug-in doesn't require administrative privileges. See \ref component_manifests
"Component Manifests" for what a manifest is, how to build your own, and how it's discovered at
startup - this is shared machinery underlying all five extension mechanisms and is worth
understanding before building any of them.

# Building an extension

@subpage component_manifests

@subpage creating_a_project_importer

@subpage creating_a_data_plugin

@subpage creating_an_extension_agent

@subpage creating_a_beam_type
