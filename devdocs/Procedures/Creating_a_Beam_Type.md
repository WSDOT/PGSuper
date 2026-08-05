Creating a Beam Type {#creating_a_beam_type}
================================================

# Purpose
This procedure describes how to add a new precast beam cross-section type (I-Beam, U-Beam, Box
Beam, Voided Slab, or an entirely new shape category) to PGSuper/PGSplice. This is the most
involved of the extension mechanisms (see \ref extensibility "Extensibility") - a beam type isn't
just UI or data plumbing, it's a structural engineering component: it must provide cross-section
geometry, LRFD live-load distribution factor calculations, prestress loss calculations, dimension
persistence, and the images used throughout the UI and reports. The core beam types that ship with
PGSuper/PGSplice (`PGSuper\Beams`, built as `PGSuperBeams.dll`) are themselves ordinary
implementations of this same extensibility mechanism, and are the best reference for a complete,
real implementation.

# The two-tier model: families and factories
Beam types are organized in two tiers, both plain C++ abstract classes deriving from
`WBFL::EAF::ComponentObject` (declared in `Include\IFace\BeamFamily.h` and
`Include\IFace\BeamFactory.h`):

- **`PGS::Beams::BeamFamily`** - a general classification of beam, such as "I-Beam", "U-Beam", or
  "Voided Slab". A family doesn't describe a cross section itself; it's a named collection of one
  or more factories.
- **`PGS::Beams::BeamFactory`** - one specific, named cross-section shape within a family (e.g.
  "Precast I-Beam" and "Nebraska NU Girder" are two different factories within the I-Beam family).
  This is the interface that actually does the structural/geometric work.

Each `BeamFamily` reports its own component category (`GetCATID()`), and that category is where
*all* of that family's member factories are registered - unlike the other extension mechanisms,
which share one CATID per role, beam families each get their own dedicated factory category. For
example, the I-Beam family's factories (`Precast I-Beam`, `Tapered I-Beam`, `Nebraska NU Girder`,
...) are all registered under `CATID_WFBeamFactory`, while the Box Beam family's factories are
registered under the separate `CATID_BoxBeamFactory`. All of these per-family CATIDs are declared
in `Include\Plugins\BeamFactoryCATID.h`; the families themselves register under the broader
`CATID_PGSuperBeamFamily` / `CATID_PGSpliceBeamFamily` (`Include\PGSuperCatCom.h` /
`Include\PGSpliceCatCom.h`).

# Two ways to extend
- **Add a new shape to an existing family** - implement a new `BeamFactory` and register it under
  that family's existing CATID (e.g. a new I-beam-like shape under `CATID_WFBeamFactory`). No new
  `BeamFamily` is needed.
- **Add an entirely new family** - implement a `BeamFactory` (or several) *and* a `BeamFamily` to
  group them, with its own CLSID and its own dedicated factory CATID. `PGS::Beams::BeamFamilyImpl`
  (`Include\Beams\Helper.h`) is the recommended base class for the family: it implements all of
  `BeamFamily`'s discovery/creation logic for you, so a concrete family only needs to override two
  one-line accessors and call `Init()`:
  ~~~
  class CMyBeamFamily : public PGS::Beams::BeamFamilyImpl
  {
  public:
     CMyBeamFamily() { Init(); }
  protected:
     const CLSID& GetCLSID() const override { return CLSID_MyBeamFamily; }
     const CATID& GetCATID() const override { return CATID_MyBeamFactory; } // your new, dedicated CATID
  };
  ~~~
  (this is exactly the pattern every core family in `PGSuper\Beams\BeamFamilyImpl.h` follows).

# The BeamFactory interface
`PGS::Beams::BeamFactory` (`Include\IFace\BeamFactory.h`) is a large interface - a beam factory is
responsible for everything the application needs to work with a cross section. In broad groups:

- **Geometry** - `CreateGirderSection`, `CreateSegment`/`ConfigureSegment`, `CreateSegmentShape`,
  `GetSegmentHeight`, `LayoutSectionChangePointsOfInterest`.
- **Structural analysis integration** - `CreateDistFactorEngineer` (LRFD live-load distribution
  factors), `CreatePsLossEngineer` (prestress losses), `CreateStrandMover` (harped strand
  relocation).
- **Dimensions** - `GetDimensionNames`/`GetDimensionUnits`/`GetDefaultDimensions`,
  `ValidateDimensions`, `SaveSectionDimensions`/`LoadSectionDimensions`.
- **UI integration** - `GetImage`/`GetIcon`/`GetImageResourceName` and the various schematic-image
  accessors used in the girder library entry dialog and in reports.
- **Compatibility rules** - which deck types, beam spacings, work point locations, girder
  orientations, diaphragm types, and top-width configurations this shape supports.
- **Publisher attribution** - `GetPublisher()`/`GetPublisherContactInformation()`, shown to the user
  if there's an error creating the factory. This is a strong signal the interface was designed with
  third-party beam publishers in mind from the start.

`PGS::Beams::BeamFactorySingleton<T>` is a required mixin - beam factories are singletons, created
once via `T::GetInstance()` rather than per-use, since they hold no per-document state (see
`INIT_BEAM_FACTORY_SINGLETON` in `Include\IFace\BeamFactory.h`).

`PGS::Beams::SplicedBeamFactory` extends `BeamFactory` with the additional queries needed for
spliced-girder construction (variable-depth sections, end blocks, web thickening) - implement this
instead of plain `BeamFactory` if your shape supports being built up from segments joined by
closure joints.

# Version compatibility
Two optional interfaces exist specifically because a beam factory's data outlives any one version
of the application:

- **`PGS::Beams::BeamFactoryCLSIDTranslator`** - PGSuper's own beam factory CLSIDs were renumbered
  once, between the 2.x and 3.x releases; PGSuper translates its own old CLSIDs internally, but a
  third-party publisher needs their own translator if they ever renumber a CLSID, so old files keep
  resolving to the right factory. Register your translator under
  `CATID_BeamFactoryCLSIDTranslator` and it will be discovered and consulted automatically.
- **`PGS::Beams::BeamFactoryCompatibility`** - implement this if data that used to live in your
  factory's own dimension set is later moved into the bridge model proper (this has happened to
  core beam types too - see the comment on `CompatibilityData` in `Include\IFace\BeamFactory.h`).
  Your factory keeps loading the old value as before, and `UpdateBridgeModel` is called so you can
  push it into its new home.

# Registration
Beam types use the same manifest mechanism as every other extension - see
\ref component_manifests "Component Manifests" for the full explanation. A real excerpt (from the
manifest that registers the core beam types in `PGSuperBeams.dll`) shows both tiers:
~~~
// The family, registered under the shared beam-family category
WBFL::EAF::ComponentManager::GetInstance().RegisterComponent(
   _T("I-Beam"), CLSID_WFBeamFamily, _T("PGSuperBeams.dll"), CATID_PGSuperBeamFamily);

// Its member factories, registered under that family's own dedicated category
WBFL::EAF::ComponentManager::GetInstance().RegisterComponent(
   _T("Precast I-Beam"), CLSID_WFBeamFactory, _T("PGSuperBeams.dll"), CATID_WFBeamFactory);
WBFL::EAF::ComponentManager::GetInstance().RegisterComponent(
   _T("Nebraska NU Girder"), CLSID_NUBeamFactory, _T("PGSuperBeams.dll"), CATID_WFBeamFactory);
~~~
In your DLL's object map, factories are registered as singletons while families are not:
~~~
EAF_BEGIN_OBJECT_MAP(ObjectMap)
   EAF_OBJECT_ENTRY_SINGLETON(CLSID_MyBeamFactory, CMyBeamFactory)
   EAF_OBJECT_ENTRY(CLSID_MyBeamFamily, CMyBeamFamily)
EAF_END_OBJECT_MAP()
~~~

# Step-by-step
-# Decide whether you're adding a shape to an existing family or creating a new family.
-# Implement `BeamFactory` (or `SplicedBeamFactory`) for your shape, and mix in
   `BeamFactorySingleton<T>`.
-# If creating a new family, implement it by deriving from `BeamFamilyImpl` as shown above, with
   its own CLSID and its own new, dedicated factory CATID.
-# Pick CLSIDs (GUIDGEN - don't reuse an existing beam type's) and, for a new family, a new CATID.
-# Add your classes to your DLL's object map (`EAF_OBJECT_ENTRY_SINGLETON` for the factory,
   `EAF_OBJECT_ENTRY` for a new family).
-# Register everything in a manifest - the family under `CATID_PGSuperBeamFamily`/
   `CATID_PGSpliceBeamFamily`, each factory under its family's dedicated CATID.
-# If you ever need to renumber a CLSID after shipping, implement and register a
   `BeamFactoryCLSIDTranslator` so existing files keep working.
