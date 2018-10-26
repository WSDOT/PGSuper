///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
//                        Bridge and Structures Office
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the Alternate Route Open Source License as 
// published by the Washington State Department of Transportation, 
// Bridge and Structures Office.
//
// This program is distributed in the hope that it will be useful, but 
// distribution is AS IS, WITHOUT ANY WARRANTY; without even the implied 
// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See 
// the Alternate Route Open Source License for more details.
//
// You should have received a copy of the Alternate Route Open Source 
// License along with this program; if not, write to the Washington 
// State Department of Transportation, Bridge and Structures Office, 
// P.O. Box  47340, Olympia, WA 98503, USA or e-mail 
// Bridge_Support@wsdot.wa.gov
///////////////////////////////////////////////////////////////////////

#ifndef INCLUDED_IFACE_PROJECT_H_
#define INCLUDED_IFACE_PROJECT_H_

/*****************************************************************************
COPYRIGHT
   Copyright © 1997-1998
   Washington State Department Of Transportation
   All Rights Reserved
*****************************************************************************/

// SYSTEM INCLUDES
//
#include <WbflTypes.h>

// PROJECT INCLUDES
//
#include <psglib\librarymanager.h>
#include <Lrfd\ILiveLoadDistributionFactor.h>

#include <Material\Material.h>

#include <EAF\EAFProjectLog.h> // IEAFProjectLog was moved... do the include here so other files don't have to change

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//
class CShearData;
class CLongitudinalRebarData;
class CGirderData;
class CPointLoadData;
class CDistributedLoadData;
class CMomentLoadData;
class CLoadFactors;

class CBridgeDescription;
class CDeckDescription;
class CPierData;
class CSpanData;
class CGirderSpacing;
class CGirderTypes;
class CGirderMaterial;

interface IStructuredLoad;
interface IPrestressingStrand;
interface IStation;
interface IDirection;

// MISCELLANEOUS
//
/// integer conversions for LLDF ROA values - so we decouple pgsuper from WBFL changes
inline long GetIntForLldfAction(LldfRangeOfApplicabilityAction action)
{
   if (action == roaEnforce)
   {
      return 0;
   }
   else if (action == roaIgnore)
   {
      return 1;
   }
   else if (action == roaIgnoreUseLeverRule)
   {
      return 2;
   }
   else
   {
      ATLASSERT(0); // something new added to wbfl?
      return 0;
   }
}

inline LldfRangeOfApplicabilityAction GetLldfActionForInt(long iaction)
{
   if (iaction == 0)
   {
      return roaEnforce;
   }
   else if (iaction == 1)
   {
      return roaIgnore;
   }
   else if (iaction == 2)
   {
      return roaIgnoreUseLeverRule;
   }
   else
   {
      ATLASSERT(0); // something got hosed?
      return roaEnforce;
   }
}

/*****************************************************************************
INTERFACE
   IProjectProperties

   Interface to edit project properties.

DESCRIPTION
   Interface to edit project properties.
*****************************************************************************/
// {59D50425-265C-11D2-8EB0-006097DF3C68}
DEFINE_GUID(IID_IProjectProperties,
0x59D50425, 0x265C, 0x11D2, 0x8E, 0xB0, 0x00, 0x60, 0x97, 0xDF, 0x3C, 0x68);
interface IProjectProperties : IUnknown
{
   virtual std::string GetBridgeName() const = 0;
   virtual void SetBridgeName(const std::string& name) = 0;
   virtual std::string GetBridgeId() const = 0;
   virtual void SetBridgeId(const std::string& bid) = 0;
   virtual std::string GetJobNumber() const = 0;
   virtual void SetJobNumber(const std::string& jid) = 0;
   virtual std::string GetEngineer() const = 0;
   virtual void SetEngineer(const std::string& eng) = 0;
   virtual std::string GetCompany() const = 0;
   virtual void SetCompany(const std::string& company) = 0;
   virtual std::string GetComments() const = 0;
   virtual void SetComments(const std::string& comments) = 0;

   // Enables/Disables the update mechanism.  If the update mechanism is
   // disable, change notifications aren't fired to the event sinks.
   // When enabled, a notification is fired if pending updates exist.
   virtual void EnableUpdate(bool bEnable) = 0;
   virtual bool IsUpdatedEnabled() = 0;
   virtual bool AreUpdatesPending() = 0;
};

/*****************************************************************************
INTERFACE
   IProjectPropertiesEventSink

   Callback interface for project properties.

DESCRIPTION
   Callback interface for project properties.
*****************************************************************************/
// {FD1DA96A-D57C-11d2-88F9-006097C68A9C}
DEFINE_GUID(IID_IProjectPropertiesEventSink, 
0xfd1da96a, 0xd57c, 0x11d2, 0x88, 0xf9, 0x0, 0x60, 0x97, 0xc6, 0x8a, 0x9c);
interface IProjectPropertiesEventSink : IUnknown
{
   virtual HRESULT OnProjectPropertiesChanged() = 0;
};

/*****************************************************************************
INTERFACE
   IEnvironment

   Interface to environmental settings.

DESCRIPTION
   Interface to environmental settings.
*****************************************************************************/
// {880AE100-2F4C-11d2-8D11-94FA07C10000}
DEFINE_GUID(IID_IEnvironment,
0x880AE100, 0x2F4C, 0x11d2, 0x8D, 0x11, 0x94, 0xFA, 0x07, 0xC1, 0x00, 0x00);
typedef enum enumExposureCondition { expNormal, expSevere } enumExposureCondition;
interface IEnvironment : IUnknown
{
   virtual enumExposureCondition GetExposureCondition() const = 0;
	virtual void SetExposureCondition(enumExposureCondition newVal) = 0;
	virtual Float64 GetRelHumidity() const = 0;
	virtual void SetRelHumidity(Float64 newVal) = 0;
};

/*****************************************************************************
INTERFACE
   IEnvironmentEventSink

   Callback interface for enviroment events

DESCRIPTION
   Callback interface for enviroment events
*****************************************************************************/
// {DBA24DC0-2F4D-11d2-8D11-94FA07C10000}
DEFINE_GUID(IID_IEnvironmentEventSink,
0xDBA24DC0, 0x2F4D, 0x11d2, 0x8D, 0x11, 0x94, 0xFA, 0x07, 0xC1, 0x00, 0x00);
interface IEnvironmentEventSink : IUnknown
{
   virtual HRESULT OnExposureConditionChanged() = 0;
   virtual HRESULT OnRelHumidityChanged() = 0;
};

/*****************************************************************************
INTERFACE
   IRoadway

   Interface for describing the roadway geometry

DESCRIPTION
   Interface for manipulating the alignment.
*****************************************************************************/
#define ALIGN_STRAIGHT 0
#define ALIGN_CURVE    1
#define PROFILE_STRAIGHT 0
#define PROFILE_VCURVE   1

struct HorzCurveData
{
   double PIStation;
   double FwdTangent;
   double Radius;
   double EntrySpiral;
   double ExitSpiral;

   bool bFwdTangent; // if true, FwdTangent is the bearing of the forward tangent otherwise it is a delta angle

   bool operator==(const HorzCurveData& other) const
   {
      return (PIStation == other.PIStation) && 
             (FwdTangent == other.FwdTangent) && 
             (Radius == other.Radius) && 
             (EntrySpiral == other.EntrySpiral) && 
             (ExitSpiral == other.EntrySpiral) &&
             (bFwdTangent == other.bFwdTangent);
   }
};

struct AlignmentData2
{
   double Direction;
   std::vector<HorzCurveData> HorzCurves;

   bool operator==(const AlignmentData2& other) const
   {
      if ( Direction != other.Direction )
         return false;

      if ( HorzCurves != other.HorzCurves )
         return false;

      // NOTE: We had to change this to an IsZero test because
      //       with very large numbers, IsEqual would return true even
      //       though the weren't equal (IsEqual uses delta/max which produces a very small value)
      if ( !IsZero(RefStation-other.RefStation) )
         return false;

      if ( !IsZero(xRefPoint-other.xRefPoint) )
         return false;

      if ( !IsZero(yRefPoint-other.yRefPoint) )
         return false;

      return true;
   }

   bool operator!=(const AlignmentData2& other) const
   {
      return !operator==(other);
   }

   // alignment reference point
   double RefStation;
   double xRefPoint;
   double yRefPoint;
};

struct VertCurveData
{
   double PVIStation;
   double ExitGrade;
   double L1;
   double L2;

   bool operator==(const VertCurveData& other) const
   {
      return (PVIStation == other.PVIStation) && 
             (ExitGrade == other.ExitGrade) && 
             (L1 == other.L1) && 
             (L2 == other.L2);
   }
};

struct ProfileData2
{
   double Station;
   double Elevation;
   double Grade;
   std::vector<VertCurveData> VertCurves;

   bool operator==(const ProfileData2& other) const
   {
      if ( Station != other.Station)
         return false;

      if ( Elevation != other.Elevation)
         return false;

      if ( Grade != other.Grade)
         return false;

      if ( VertCurves != other.VertCurves )
         return false;

      return true;
   }

   bool operator!=(const ProfileData2& other) const
   {
      return !operator==(other);
   }
};

struct CrownData2
{
   double Station;
   double Left;
   double Right;
   double CrownPointOffset;

   bool operator==(const CrownData2& other) const
   {
      return (Station == other.Station) &&
             (Left == other.Left) &&
             (Right == other.Right) &&
             (CrownPointOffset == other.CrownPointOffset);
   }
};

struct RoadwaySectionData
{
   std::vector<CrownData2> Superelevations;

   bool operator==(const RoadwaySectionData& other) const
   {
      return Superelevations == other.Superelevations;
   }

   bool operator!=(const RoadwaySectionData& other) const
   {
      return !operator==(other);
   }
};

// {77D1CD00-2F90-11d2-8D11-94FA07C10000}
DEFINE_GUID(IID_IRoadwayData,
0x77D1CD00, 0x2F90, 0x11d2, 0x8D, 0x11, 0x94, 0xFA, 0x07, 0xC1, 0x00, 0x00);
interface IRoadwayData : IUnknown
{
   virtual void SetAlignmentData2(const AlignmentData2& data) = 0;
   virtual AlignmentData2 GetAlignmentData2() const = 0;

   virtual void SetProfileData2(const ProfileData2& data) = 0;
   virtual ProfileData2 GetProfileData2() const = 0;

   virtual void SetRoadwaySectionData(const RoadwaySectionData& data) = 0;
   virtual RoadwaySectionData GetRoadwaySectionData() const = 0;
};

/*****************************************************************************
INTERFACE
   IGirderData

   Interface to manipulating the prestressing input

DESCRIPTION
   Interface to manipulating the prestressing input
*****************************************************************************/
// {61D8C8F0-58B9-11d2-8ED3-006097DF3C68}
DEFINE_GUID(IID_IGirderData, 
0x61d8c8f0, 0x58b9, 0x11d2, 0x8e, 0xd3, 0x0, 0x60, 0x97, 0xdf, 0x3c, 0x68);
interface IGirderData : IUnknown
{
   virtual const matPsStrand* GetStrandMaterial(SpanIndexType span,GirderIndexType gdr,pgsTypes::StrandType type) const = 0;
   virtual void SetStrandMaterial(SpanIndexType span,GirderIndexType gdr,pgsTypes::StrandType type,const matPsStrand* pmat)=0;
   virtual CGirderData GetGirderData(SpanIndexType span,GirderIndexType gdr) const = 0;
   virtual bool SetGirderData(const CGirderData& data,SpanIndexType span,GirderIndexType gdr) = 0;
   virtual const CGirderMaterial* GetGirderMaterial(SpanIndexType span,GirderIndexType gdr) const = 0;
   virtual void SetGirderMaterial(SpanIndexType span,GirderIndexType gdr,const CGirderMaterial& material) = 0;
};

/*****************************************************************************
INTERFACE
   IShear

   Interface to manipulating the shear input

DESCRIPTION
   Interface to manipulating the shear input
*****************************************************************************/
// {7A716040-8BD4-11d2-9D99-00609710E6CE}
DEFINE_GUID(IID_IShear, 
0x7a716040, 0x8bd4, 0x11d2, 0x9d, 0x99, 0x0, 0x60, 0x97, 0x10, 0xe6, 0xce);
interface IShear : IUnknown
{
   virtual std::string GetStirrupMaterial(SpanIndexType span,GirderIndexType gdr) const = 0;
   virtual void SetStirrupMaterial(SpanIndexType span,GirderIndexType gdr,const char* matName)=0;
   virtual CShearData GetShearData(SpanIndexType span,GirderIndexType gdr) const = 0;
   virtual bool SetShearData(const CShearData& data,SpanIndexType span,GirderIndexType gdr) = 0;
};

/*****************************************************************************
INTERFACE
   ILongitudinalRebar

   Interface to manipulating the longitudinal rebar input

DESCRIPTION
   Interface to manipulating the longitudinal rebar input
*****************************************************************************/
// {E87AC679-4753-4b18-81A1-3C6858226F87}
DEFINE_GUID(IID_ILongitudinalRebar, 
0xe87ac679, 0x4753, 0x4b18, 0x81, 0xa1, 0x3c, 0x68, 0x58, 0x22, 0x6f, 0x87);
interface ILongitudinalRebar : IUnknown
{
   virtual std::string GetLongitudinalRebarMaterial(SpanIndexType span,GirderIndexType gdr) const = 0;
   virtual void SetLongitudinalRebarMaterial(SpanIndexType span,GirderIndexType gdr,const char* matName)=0;
   virtual CLongitudinalRebarData GetLongitudinalRebarData(SpanIndexType span,GirderIndexType gdr) const = 0;
   virtual bool SetLongitudinalRebarData(const CLongitudinalRebarData& data,SpanIndexType span,GirderIndexType gdr) = 0;
};


/*****************************************************************************
INTERFACE
   ISpecification

   Interface to manipulating the specification input

DESCRIPTION
   Interface to manipulating the specification input
*****************************************************************************/
// {B72842B0-5BB1-11d2-8ED7-006097DF3C68}
DEFINE_GUID(IID_ISpecification, 
0xb72842b0, 0x5bb1, 0x11d2, 0x8e, 0xd7, 0x0, 0x60, 0x97, 0xdf, 0x3c, 0x68);
interface ISpecification : IUnknown
{
   virtual std::string GetSpecification() const = 0;
   virtual void SetSpecification(const std::string& spec) = 0;

   virtual void GetTrafficBarrierDistribution(GirderIndexType* pNGirders,pgsTypes::TrafficBarrierDistribution* pDistType) = 0;

   virtual Uint16 GetMomentCapacityMethod() = 0;

   virtual void SetAnalysisType(pgsTypes::AnalysisType analysisType) = 0;
   virtual pgsTypes::AnalysisType GetAnalysisType() = 0;

   virtual arDesignOptions GetDesignOptions(SpanIndexType spanIdx,GirderIndexType gdrIdx) = 0;

   virtual bool IsSlabOffsetDesignEnabled() = 0; // global setting from library

   virtual pgsTypes::OverlayLoadDistributionType GetOverlayLoadDistributionType() = 0;

   virtual std::string GetRatingSpecification() const = 0;
   virtual void SetRatingSpecification(const std::string& spec) = 0;
};

/*****************************************************************************
INTERFACE
   ISpecificationEventSink

   Callback interface for specification input change events.

DESCRIPTION
   Callback interface for specification input change events.
*****************************************************************************/
// {B72842B1-5BB1-11d2-8ED7-006097DF3C68}
DEFINE_GUID(IID_ISpecificationEventSink, 
0xb72842b1, 0x5bb1, 0x11d2, 0x8e, 0xd7, 0x0, 0x60, 0x97, 0xdf, 0x3c, 0x68);
interface ISpecificationEventSink : IUnknown
{
   virtual HRESULT OnSpecificationChanged() = 0;
   virtual HRESULT OnAnalysisTypeChanged() = 0;
};

/*****************************************************************************
INTERFACE
   ILibraryNames

   Interface to obtain list of names of library entries.

DESCRIPTION
   Interface to obtain list of names of library entries.
*****************************************************************************/
// {5A3A28C0-480F-11d2-8EC7-006097DF3C68}
DEFINE_GUID(IID_ILibraryNames,
0x5A3A28C0, 0x480F, 0x11d2, 0x8E, 0xC7, 0x00, 0x60, 0x97, 0xDF, 0x3C, 0x68);
interface ILibraryNames : IUnknown
{
   virtual void EnumGdrConnectionNames( std::vector<std::string>* pNames ) const = 0;
   virtual void EnumGirderNames( std::vector<std::string>* pNames ) const = 0;
   virtual void EnumGirderNames( const char* strGirderFamily, std::vector<std::string>* pNames ) const = 0;
   virtual void EnumConcreteNames( std::vector<std::string>* pNames ) const = 0;
   virtual void EnumDiaphragmNames( std::vector<std::string>* pNames ) const = 0;
   virtual void EnumTrafficBarrierNames( std::vector<std::string>* pNames ) const = 0;
   virtual void EnumSpecNames( std::vector<std::string>* pNames) const = 0;
   virtual void EnumLiveLoadNames( std::vector<std::string>* pNames) const = 0;

   virtual void EnumGirderFamilyNames( std::vector<std::string>* pNames ) = 0;
   virtual void GetBeamFactory(const std::string& strBeamFamily,const std::string& strBeamName,IBeamFactory** ppFactory) = 0;

   virtual void EnumRatingCriteriaNames( std::vector<std::string>* pNames) const = 0;
};

/*****************************************************************************
INTERFACE
   ILibrary

   Interface for retrieving library entries.

DESCRIPTION
   Interface for retrieving library entries.
*****************************************************************************/
// {34172AE0-3781-11d2-8EC1-006097DF3C68}
DEFINE_GUID(IID_ILibrary,
0x34172AE0, 0x3781, 0x11d2, 0x8E, 0xC1, 0x00, 0x60, 0x97, 0xDF, 0x3C, 0x68);
interface ILibrary : IUnknown
{
   virtual void SetLibraryManager(psgLibraryManager* pNewLibMgr)=0; 
   virtual psgLibraryManager* GetLibraryManager()=0; 
   virtual const ConnectionLibraryEntry* GetConnectionEntry(const char* lpszName ) const = 0;
   virtual const GirderLibraryEntry* GetGirderEntry( const char* lpszName ) const = 0;
   virtual const ConcreteLibraryEntry* GetConcreteEntry( const char* lpszName ) const = 0;
   virtual const DiaphragmLayoutEntry* GetDiaphragmEntry( const char* lpszName ) const = 0;
   virtual const TrafficBarrierEntry* GetTrafficBarrierEntry( const char* lpszName ) const = 0;
   virtual const SpecLibraryEntry* GetSpecEntry( const char* lpszName ) const = 0;
   virtual const LiveLoadLibraryEntry* GetLiveLoadEntry( const char* lpszName ) const = 0;
   virtual ConcreteLibrary&        GetConcreteLibrary() = 0;
   virtual ConnectionLibrary&      GetConnectionLibrary() = 0;
   virtual GirderLibrary&          GetGirderLibrary() = 0;
   virtual DiaphragmLayoutLibrary& GetDiaphragmLayoutLibrary() = 0;
   virtual TrafficBarrierLibrary&  GetTrafficBarrierLibrary() = 0;
   virtual SpecLibrary*            GetSpecLibrary() = 0;
   virtual LiveLoadLibrary*        GetLiveLoadLibrary() = 0;

   virtual std::vector<libEntryUsageRecord> GetLibraryUsageRecords() const = 0;
   virtual void GetMasterLibraryInfo(std::string& strPublisher,std::string& strMasterLib,sysTime& time) const = 0;

   virtual const RatingLibrary* GetRatingLibrary() const = 0;
   virtual RatingLibrary* GetRatingLibrary() = 0;
   virtual const RatingLibraryEntry* GetRatingEntry( const char* lpszName ) const = 0;
};


/*****************************************************************************
INTERFACE
   IBridgeDescriptionEventSink

   Callback interface for bridge description events

DESCRIPTION
   Callback interface for bridge description events
*****************************************************************************/
#define GCH_PRESTRESSING_CONFIGURATION 0x0001
#define GCH_STRAND_MATERIAL            0x0002
#define GCH_STIRRUPS                   0x0004
#define GCH_LONGITUDINAL_REBAR         0x0008
#define GCH_LIFTING_CONFIGURATION      0x0010
#define GCH_SHIPPING_CONFIGURATION     0x0020
#define GCH_LOADING_ADDED              0x0040
#define GCH_LOADING_REMOVED            0x0080
#define GCH_LOADING_CHANGED            0x0100
#define GCH_CONCRETE                   0x0200

// {6132E890-719D-11d2-8EF1-006097DF3C68}
DEFINE_GUID(IID_IBridgeDescriptionEventSink, 
0x6132e890, 0x719d, 0x11d2, 0x8e, 0xf1, 0x0, 0x60, 0x97, 0xdf, 0x3c, 0x68);
interface IBridgeDescriptionEventSink : IUnknown
{
   virtual HRESULT OnBridgeChanged() = 0;
   virtual HRESULT OnGirderFamilyChanged() = 0;
   virtual HRESULT OnGirderChanged(SpanIndexType span,GirderIndexType gdr,Uint32 lHint) = 0;
   virtual HRESULT OnLiveLoadChanged() = 0;
   virtual HRESULT OnLiveLoadNameChanged(const char* strOldName,const char* strNewName) = 0;
   virtual HRESULT OnConstructionLoadChanged() = 0;
};


/*****************************************************************************
INTERFACE
   ILoadModifiers

   Interface to manipulating the load modifiers

DESCRIPTION
   Interface to manipulating the load modifiers
*****************************************************************************/
// {2C1A3E70-727E-11d2-8EF2-006097DF3C68}
DEFINE_GUID(IID_ILoadModifiers, 
0x2c1a3e70, 0x727e, 0x11d2, 0x8e, 0xf2, 0x0, 0x60, 0x97, 0xdf, 0x3c, 0x68);
interface ILoadModifiers : IUnknown
{
   enum Level { Low, Normal, High };
   virtual void SetDuctilityFactor(Level level,Float64 value)  = 0;
   virtual void SetImportanceFactor(Level level,Float64 value) = 0;
   virtual void SetRedundancyFactor(Level level,Float64 value) = 0;

   virtual Float64 GetDuctilityFactor() = 0;
   virtual Float64 GetImportanceFactor() = 0;
   virtual Float64 GetRedundancyFactor() = 0;

   virtual Level GetDuctilityLevel() = 0;
   virtual Level GetImportanceLevel() = 0;
   virtual Level GetRedundancyLevel() = 0;
};

/*****************************************************************************
INTERFACE
   ILoadModifiersEventSink

   Callback interface for LoadModifiers events

DESCRIPTION
   Callback interface for LoadModifiers events
*****************************************************************************/
// {2C1A3E71-727E-11d2-8EF2-006097DF3C68}
DEFINE_GUID(IID_ILoadModifiersEventSink, 
0x2c1a3e71, 0x727e, 0x11d2, 0x8e, 0xf2, 0x0, 0x60, 0x97, 0xdf, 0x3c, 0x68);
interface ILoadModifiersEventSink : IUnknown
{
   virtual HRESULT OnLoadModifiersChanged() = 0;
};

/*****************************************************************************
INTERFACE ILibraryConflictGuiEventSink

   Interface to support a gui for dealing with library entry conflicts

DESCRIPTION
   Interface to support a gui for dealing with library entry conflicts
*****************************************************************************/
enum LibConflictResult {RenameEntry, ReplaceEntry};

// {BCE5B018-2149-11d3-AD79-00105A9AF985}
DEFINE_GUID(IID_ILibraryConflictEventSink, 
0xbce5b018, 0x2149, 0x11d3, 0xad, 0x79, 0x0, 0x10, 0x5a, 0x9a, 0xf9, 0x85);
interface ILibraryConflictEventSink : IUnknown
{
   virtual HRESULT OnLibraryConflictResolved() = 0;
};

/*****************************************************************************
INTERFACE IImportProjectLibrary

   Interface to support importation of library entries from an exising project

DESCRIPTION
   Interface to support importation of library entries from an exising project
*****************************************************************************/
// {916B4250-3EFD-11d3-AD94-00105A9AF985}
DEFINE_GUID(IID_IImportProjectLibrary, 
0x916b4250, 0x3efd, 0x11d3, 0xad, 0x94, 0x0, 0x10, 0x5a, 0x9a, 0xf9, 0x85);
interface IImportProjectLibrary : IUnknown
{
   virtual bool ImportProjectLibraries(IStructuredLoad* pLoad)=0;
};

/*****************************************************************************
INTERFACE
   IUserDefinedLoadData

DESCRIPTION
   Interface for editing user defined loads.
*****************************************************************************/
// {24B25C88-1551-4025-ADAD-DF69CB18ADCA}
DEFINE_GUID(IID_IUserDefinedLoadData, 
0x24b25c88, 0x1551, 0x4025, 0xad, 0xad, 0xdf, 0x69, 0xcb, 0x18, 0xad, 0xca);
interface IUserDefinedLoadData : IUnknown
{
   // point loads
   virtual CollectionIndexType GetPointLoadCount() const = 0;
   // add point load and return current count
   virtual CollectionIndexType AddPointLoad(const CPointLoadData& pld)= 0;
   virtual const CPointLoadData& GetPointLoad(CollectionIndexType idx) const = 0;
   virtual void UpdatePointLoad(CollectionIndexType idx, const CPointLoadData& pld) = 0;
   virtual void DeletePointLoad(CollectionIndexType idx) = 0;

   // distributed loads
   virtual CollectionIndexType GetDistributedLoadCount() const = 0;
   // add distributed load and return current count
   virtual CollectionIndexType AddDistributedLoad(const CDistributedLoadData& pld)= 0;
   virtual const CDistributedLoadData& GetDistributedLoad(CollectionIndexType idx) const = 0;
   virtual void UpdateDistributedLoad(CollectionIndexType idx, const CDistributedLoadData& pld) = 0;
   virtual void DeleteDistributedLoad(CollectionIndexType idx) = 0;

   // moment loads
   virtual CollectionIndexType GetMomentLoadCount() const = 0;
   // add moment load and return current count
   virtual CollectionIndexType AddMomentLoad(const CMomentLoadData& pld)= 0;
   virtual const CMomentLoadData& GetMomentLoad(CollectionIndexType idx) const = 0;
   virtual void UpdateMomentLoad(CollectionIndexType idx, const CMomentLoadData& pld) = 0;
   virtual void DeleteMomentLoad(CollectionIndexType idx) = 0;

   // construction loads
   virtual void SetConstructionLoad(Float64 load) = 0;
   virtual Float64 GetConstructionLoad() const = 0;
};

/*****************************************************************************
INTERFACE
   IEvents

   Interface to control events

DESCRIPTION
   Interface to control events
*****************************************************************************/
// {58B12CA6-A09D-4208-BE4A-30B5B4338513}
DEFINE_GUID(IID_IEvents, 
0x58b12ca6, 0xa09d, 0x4208, 0xbe, 0x4a, 0x30, 0xb5, 0xb4, 0x33, 0x85, 0x13);
interface IEvents : IUnknown
{
   virtual void HoldEvents() = 0;
   virtual void FirePendingEvents() = 0;
   virtual void CancelPendingEvents() = 0;
};

/*****************************************************************************
INTERFACE
   IUIEvents

   Interface to control events in the user interface

DESCRIPTION
   Interface to control events in the user interface
*****************************************************************************/
DEFINE_GUID(IID_IUIEvents, 
0x74a056fd, 0xccfd, 0x496f, 0xb3, 0x12, 0xd2, 0x2e, 0x22, 0xe6, 0xb7, 0x73);
interface IUIEvents : IUnknown
{
   virtual void HoldEvents(bool bHold=true) = 0;
   virtual void FirePendingEvents() = 0;
};

/*****************************************************************************
INTERFACE
   ILimits

   Interface to get information about limits

DESCRIPTION
   Interface to get information about limits
*****************************************************************************/
// {DB41F235-1ED8-4024-B7AB-AD053330CD26}
DEFINE_GUID(IID_ILimits, 
0xdb41f235, 0x1ed8, 0x4024, 0xb7, 0xab, 0xad, 0x5, 0x33, 0x30, 0xcd, 0x26);
interface ILimits : IUnknown
{
   virtual double GetMaxSlabFc() = 0;
   virtual double GetMaxGirderFci() = 0;
   virtual double GetMaxGirderFc() = 0;
   virtual double GetMaxConcreteUnitWeight() = 0;
   virtual double GetMaxConcreteAggSize() = 0;
};

/*****************************************************************************
INTERFACE
   ILimits

   Interface to get information about limits

DESCRIPTION
   Interface to get information about limits
*****************************************************************************/
// {797998B4-FE8C-4ad1-AE9E-167A193CC296}
DEFINE_GUID(IID_ILimits2, 
0x797998b4, 0xfe8c, 0x4ad1, 0xae, 0x9e, 0x16, 0x7a, 0x19, 0x3c, 0xc2, 0x96);
interface ILimits2 : IUnknown
{
   virtual double GetMaxSlabFc(pgsTypes::ConcreteType concType) = 0;
   virtual double GetMaxGirderFci(pgsTypes::ConcreteType concType) = 0;
   virtual double GetMaxGirderFc(pgsTypes::ConcreteType concType) = 0;
   virtual double GetMaxConcreteUnitWeight(pgsTypes::ConcreteType concType) = 0;
   virtual double GetMaxConcreteAggSize(pgsTypes::ConcreteType concType) = 0;
};

/*****************************************************************************
INTERFACE
   ILoadFactors

   Interface for getting information about user defined load factors

DESCRIPTION
   Interface for getting information about user defined load factors
*****************************************************************************/
// {769DC2AE-D6DE-45c3-A612-84F8A0534CBE}
DEFINE_GUID(IID_ILoadFactors, 
0x769dc2ae, 0xd6de, 0x45c3, 0xa6, 0x12, 0x84, 0xf8, 0xa0, 0x53, 0x4c, 0xbe);
interface ILoadFactors : IUnknown
{
   virtual const CLoadFactors* GetLoadFactors() const = 0;
   virtual void SetLoadFactors(const CLoadFactors& loadFactors) = 0;
};

/*****************************************************************************
INTERFACE IILiveLoads

   Interface to support selection of live load entries and options

DESCRIPTION
   Interface to support selection of live load entries and options
*****************************************************************************/
// {D286FE13-3818-4805-B4B2-0CF4E056B37E}
DEFINE_GUID(IID_ILiveLoads, 
0xd286fe13, 0x3818, 0x4805, 0xb4, 0xb2, 0xc, 0xf4, 0xe0, 0x56, 0xb3, 0x7e);
interface ILiveLoads : IUnknown
{
   virtual bool IsLiveLoadDefined(pgsTypes::LiveLoadType llType) = 0;
   virtual bool IsPedestianLoadEnabled(pgsTypes::LiveLoadType llType) = 0;
   virtual void EnablePedestianLoad(pgsTypes::LiveLoadType llType,bool bEnable) = 0;
   virtual std::vector<std::string> GetLiveLoadNames(pgsTypes::LiveLoadType llType) = 0;
   virtual void SetLiveLoadNames(pgsTypes::LiveLoadType llType,const std::vector<std::string>& names) = 0;
   virtual double GetTruckImpact(pgsTypes::LiveLoadType llType) = 0;
   virtual void SetTruckImpact(pgsTypes::LiveLoadType llType,double impact) = 0;
   virtual double GetLaneImpact(pgsTypes::LiveLoadType llType) = 0;
   virtual void SetLaneImpact(pgsTypes::LiveLoadType llType,double impact) = 0;
   virtual void SetLldfRangeOfApplicabilityAction(LldfRangeOfApplicabilityAction action) = 0;
   virtual LldfRangeOfApplicabilityAction GetLldfRangeOfApplicabilityAction() = 0;
   virtual std::string GetLLDFSpecialActionText()=0; // get common string for ignore roa case
   virtual bool IgnoreLLDFRangeOfApplicability()=0; // true if action is to ignore ROA
};

// {483673C2-9F4E-40ec-9DC2-6B36B0D34498}
DEFINE_GUID(IID_IBridgeDescription, 
0x483673c2, 0x9f4e, 0x40ec, 0x9d, 0xc2, 0x6b, 0x36, 0xb0, 0xd3, 0x44, 0x98);
interface IBridgeDescription : IUnknown
{
   virtual const CBridgeDescription* GetBridgeDescription() = 0;
   virtual void SetBridgeDescription(const CBridgeDescription& desc) = 0;

   virtual const CDeckDescription* GetDeckDescription() = 0;
   virtual void SetDeckDescription(const CDeckDescription& deck) = 0;

   virtual const CSpanData* GetSpan(SpanIndexType spanIdx) = 0;
   virtual void SetSpan(SpanIndexType spanIdx,const CSpanData& spanData) = 0;

   virtual const CPierData* GetPier(PierIndexType pierIdx) = 0;
   virtual void SetPier(PierIndexType pierIdx,const CPierData& PierData) = 0;

   virtual void SetSpanLength(SpanIndexType spanIdx,double newLength) = 0;
   virtual void MovePier(PierIndexType pierIdx,double newStation,pgsTypes::MovePierOption moveOption) = 0;

   virtual void SetMeasurementType(PierIndexType pierIdx,pgsTypes::PierFaceType pierFace,pgsTypes::MeasurementType mt) = 0;
   virtual void SetMeasurementLocation(PierIndexType pierIdx,pgsTypes::PierFaceType pierFace,pgsTypes::MeasurementLocation ml) = 0;
   virtual void SetGirderSpacing(PierIndexType pierIdx,pgsTypes::PierFaceType face,const CGirderSpacing& spacing) = 0;
   virtual void SetGirderSpacingAtStartOfSpan(SpanIndexType spanIdx,const CGirderSpacing& spacing) = 0;
   virtual void SetGirderSpacingAtEndOfSpan(SpanIndexType spanIdx,const CGirderSpacing& spacing) = 0;
   virtual void UseSameGirderSpacingAtBothEndsOfSpan(SpanIndexType spanIdx,bool bUseSame) = 0;
   virtual void SetGirderTypes(SpanIndexType spanIdx,const CGirderTypes& girderTypes) = 0;
   virtual void SetGirderName( SpanIndexType spanIdx, GirderIndexType gdrIdx, const char* strGirderName) = 0;
   virtual void SetGirderCount(SpanIndexType spanIdx,GirderIndexType nGirders) = 0;
   virtual void SetBoundaryCondition(PierIndexType pierIdx,pgsTypes::PierConnectionType connectionType) = 0;

   virtual void DeletePier(PierIndexType pierIdx,pgsTypes::PierFaceType faceForSpan) = 0;
   virtual void InsertSpan(PierIndexType refPierIdx,pgsTypes::PierFaceType pierFace,Float64 spanLength,const CSpanData* pSpanData=NULL,const CPierData* pPierData=NULL) = 0;

   virtual void SetLiveLoadDistributionFactorMethod(pgsTypes::DistributionFactorMethod method) = 0;
   virtual pgsTypes::DistributionFactorMethod GetLiveLoadDistributionFactorMethod() = 0;

   virtual void UseSameNumberOfGirdersInAllSpans(bool bUseSame) = 0;
   virtual bool UseSameNumberOfGirdersInAllSpans() = 0;
   virtual void SetGirderCount(GirderIndexType nGirders) = 0; // used for the entire bridge

   virtual void UseSameGirderForEntireBridge(bool bSame) = 0;
   virtual bool UseSameGirderForEntireBridge() = 0;
   virtual void SetGirderName(const char* strGirderName) = 0; // sets the name of the girder that is used for the entire bridge

   virtual void SetGirderSpacingType(pgsTypes::SupportedBeamSpacing sbs) = 0;
   virtual pgsTypes::SupportedBeamSpacing GetGirderSpacingType() = 0;
   virtual void SetGirderSpacing(double spacing) = 0; // used for the entire bridge
   virtual void SetMeasurementType(pgsTypes::MeasurementType mt) = 0;
   virtual pgsTypes::MeasurementType GetMeasurementType() = 0;
   virtual void SetMeasurementLocation(pgsTypes::MeasurementLocation ml) = 0;
   virtual pgsTypes::MeasurementLocation GetMeasurementLocation() = 0;

   // slab offset
   // chnages slab offset type to be sotBridge
   virtual void SetSlabOffset( Float64 slabOffset) = 0;
   // changes slab offset type to sotSpan
   virtual void SetSlabOffset( SpanIndexType spanIdx, Float64 start, Float64 end) = 0;
   // sets slab offset per girder... sets the slab offset type to sotGirder
   virtual void SetSlabOffset( SpanIndexType spanIdx, GirderIndexType gdrIdx, Float64 start, Float64 end) = 0;
   virtual pgsTypes::SlabOffsetType GetSlabOffsetType() = 0;
   virtual void GetSlabOffset( SpanIndexType spanIdx, GirderIndexType gdrIdx, Float64* pStart, Float64* pEnd) = 0;
};

#endif // INCLUDED_IFACE_PROJECT_H_
