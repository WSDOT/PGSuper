///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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

#pragma once

#include <WbflTypes.h>
#include <psglib\librarymanager.h>
#include <Lrfd\ILiveLoadDistributionFactor.h>

#include <PgsExt\Keys.h>

#include <Materials/Materials.h>

#include <EAF\EAFProjectLog.h> // IEAFProjectLog was moved... do the include here so other files don't have to change

class CShearData;
class CLongitudinalRebarData;
class CGirderData;
class CHandlingData;
class CHandlingData;

class CPointLoadData;
class CDistributedLoadData;
class CMomentLoadData;
class CLoadFactors;

class CBridgeDescription2;
class CDeckDescription2;
class CPierData2;
class CSpanData2;
class CGirderSpacing2;
class CGirderMaterial;
class CTemporarySupportData;
class CSplicedGirderData;
class CPrecastSegmentData;
class CClosureJointData;
class CGirderGroupData;
class CPTData;
class CTimelineManager;
class CTimelineEvent;
class CStrandData;
class CHandlingData;
class CBearingData2;
class CSegmentPTData;

class CBridgeChangedHint;

interface IStructuredLoad;
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
      ATLASSERT(false); // something new added to wbfl?
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
      ATLASSERT(false); // something got hosed?
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
   virtual LPCTSTR GetBridgeName() const = 0;
   virtual void SetBridgeName(LPCTSTR name) = 0;
   virtual LPCTSTR GetBridgeID() const = 0;
   virtual void SetBridgeID(LPCTSTR bid) = 0;
   virtual LPCTSTR GetJobNumber() const = 0;
   virtual void SetJobNumber(LPCTSTR jid) = 0;
   virtual LPCTSTR GetEngineer() const = 0;
   virtual void SetEngineer(LPCTSTR eng) = 0;
   virtual LPCTSTR GetCompany() const = 0;
   virtual void SetCompany(LPCTSTR company) = 0;
   virtual LPCTSTR GetComments() const = 0;
   virtual void SetComments(LPCTSTR comments) = 0;
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

struct CompoundCurveData
{
   Float64 PIStation;
   Float64 FwdTangent;
   Float64 Radius;
   Float64 EntrySpiral;
   Float64 ExitSpiral;

   bool bFwdTangent; // if true, FwdTangent is the bearing of the forward tangent otherwise it is a delta angle

   bool operator==(const CompoundCurveData& other) const
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
   std::_tstring Name{ _T("") };
   Float64 Direction{ 0.0 };
   // alignment reference point
   Float64 RefStation{ 0.0 };
   Float64 xRefPoint{ 0.0 };
   Float64 yRefPoint{ 0.0 };
   std::vector<CompoundCurveData> CompoundCurves;

   bool operator==(const AlignmentData2& other) const
   {
      if ( Name != other.Name)
      {
         return false;
      }

      if (Direction != other.Direction)
      {
         return false;
      }

      if ( CompoundCurves != other.CompoundCurves )
      {
         return false;
      }

      // NOTE: We had to change this to an IsZero test because
      //       with very large numbers, IsEqual would return true even
      //       though the weren't equal (IsEqual uses delta/max which produces a very small value)
      if ( !IsZero(RefStation-other.RefStation) )
      {
         return false;
      }

      if ( !IsZero(xRefPoint-other.xRefPoint) )
      {
         return false;
      }

      if ( !IsZero(yRefPoint-other.yRefPoint) )
      {
         return false;
      }

      return true;
   }

   bool operator!=(const AlignmentData2& other) const
   {
      return !operator==(other);
   }
};

struct VertCurveData
{
   Float64 PVIStation{ 0.0 };
   Float64 ExitGrade{ 0.0 };
   Float64 L1{ 0.0 };
   Float64 L2{ 0.0 };

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
   Float64 Station{ 0.0 };
   Float64 Elevation{ 0.0 };
   Float64 Grade{ 0.0 };
   std::vector<VertCurveData> VertCurves;

   bool operator==(const ProfileData2& other) const
   {
      if ( Station != other.Station)
      {
         return false;
      }

      if ( Elevation != other.Elevation)
      {
         return false;
      }

      if ( Grade != other.Grade)
      {
         return false;
      }

      if ( VertCurves != other.VertCurves )
      {
         return false;
      }

      return true;
   }

   bool operator!=(const ProfileData2& other) const
   {
      return !operator==(other);
   }
};

struct RoadwaySegmentData
{
   Float64 Length{ 0.0 };
   Float64 Slope{ 0.0 };

   bool operator==(const RoadwaySegmentData& other) const
   {
      if (Length != other.Length)
      {
         return false;
      }

      if (Slope != other.Slope)
      {
         return false;
      }

      return true;
   }


};

struct RoadwaySectionTemplate
{
   Float64 Station{ 0.0 };
   Float64 LeftSlope{ 0.0 };
   Float64 RightSlope{ 0.0 };
   std::vector<RoadwaySegmentData> SegmentDataVec; // only used if number of slope segments > 2

   bool operator==(const RoadwaySectionTemplate& other) const
   {
      return (Station == other.Station) &&
             (LeftSlope == other.LeftSlope) &&
             (RightSlope == other.RightSlope) &&
             (SegmentDataVec == other.SegmentDataVec);
   }
};

struct RoadwaySectionData
{
   enum SlopeMeasure {RelativeToAlignmentPoint,FromLeftEdge};
   SlopeMeasure slopeMeasure{ RelativeToAlignmentPoint };
   IndexType NumberOfSegmentsPerSection{ 2 }; // Always have at least 2 outer infinite segments. Inner segments add to this
   IndexType AlignmentPointIdx{ 0 };   // Not zero based. No ridge points at ends of outer segments. For 2 segment case, this value is 1.
   IndexType ProfileGradePointIdx{ 0 }; // Not zero based. No ridge points at ends of outer segments. For 2 segment case, this value is 1. If PGL and Alignment are the same, this is equal to AlignmentRidgePointIdx

   std::vector<RoadwaySectionTemplate> RoadwaySectionTemplates;

   bool operator==(const RoadwaySectionData& other) const
   {
      if (slopeMeasure != other.slopeMeasure)
         return false;

      if (NumberOfSegmentsPerSection != other.NumberOfSegmentsPerSection)
         return false;

      if (AlignmentPointIdx != other.AlignmentPointIdx)
         return false;

      if (ProfileGradePointIdx != other.ProfileGradePointIdx)
         return false;

      return RoadwaySectionTemplates == other.RoadwaySectionTemplates;
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
   virtual const AlignmentData2& GetAlignmentData2() const = 0;

   virtual void SetProfileData2(const ProfileData2& data) = 0;
   virtual const ProfileData2& GetProfileData2() const = 0;

   virtual void SetRoadwaySectionData(const RoadwaySectionData& data) = 0;
   virtual const RoadwaySectionData& GetRoadwaySectionData() const = 0;
};

/*****************************************************************************
INTERFACE
   ISegmentData

   Interface to manipulating the prestressing input

DESCRIPTION
   Interface to manipulating the prestressing input
*****************************************************************************/
// {D660F3C1-328E-4ef2-924B-DBB8B8D4DB6F}
DEFINE_GUID(IID_ISegmentData, 
0xd660f3c1, 0x328e, 0x4ef2, 0x92, 0x4b, 0xdb, 0xb8, 0xb8, 0xd4, 0xdb, 0x6f);
interface ISegmentData : IUnknown
{
   virtual const WBFL::Materials::PsStrand* GetStrandMaterial(const CSegmentKey& segmentKey,pgsTypes::StrandType type) const = 0;
   virtual void SetStrandMaterial(const CSegmentKey& segmentKey,pgsTypes::StrandType type,const WBFL::Materials::PsStrand* pmat) = 0;

   virtual const CGirderMaterial* GetSegmentMaterial(const CSegmentKey& segmentKey) const = 0;
   virtual void SetSegmentMaterial(const CSegmentKey& segmentKey,const CGirderMaterial& material) = 0;

   virtual const CStrandData* GetStrandData(const CSegmentKey& segmentKey) const = 0;
   virtual void SetStrandData(const CSegmentKey& segmentKey,const CStrandData& strands) = 0;

   virtual const CSegmentPTData* GetSegmentPTData(const CSegmentKey& segmentKey) const = 0;
   virtual void SetSegmentPTData(const CSegmentKey& segmentKey,const CSegmentPTData& strands) = 0;

   virtual const CHandlingData* GetHandlingData(const CSegmentKey& segmentKey) const = 0;
   virtual void SetHandlingData(const CSegmentKey& segmentKey,const CHandlingData& handling) = 0;
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
   virtual std::_tstring GetSegmentStirrupMaterial(const CSegmentKey& segmentKey) const = 0;
   virtual void GetSegmentStirrupMaterial(const CSegmentKey& segmentKey,WBFL::Materials::Rebar::Type& type,WBFL::Materials::Rebar::Grade& grade) const = 0;
   virtual void SetSegmentStirrupMaterial(const CSegmentKey& segmentKey,WBFL::Materials::Rebar::Type type,WBFL::Materials::Rebar::Grade grade) = 0;
   virtual const CShearData2* GetSegmentShearData(const CSegmentKey& segmentKey) const = 0;
   virtual void SetSegmentShearData(const CSegmentKey& segmentKey,const CShearData2& data) = 0;

   virtual std::_tstring GetClosureJointStirrupMaterial(const CClosureKey& closureKey) const = 0;
   virtual void GetClosureJointStirrupMaterial(const CClosureKey& closureKey,WBFL::Materials::Rebar::Type& type,WBFL::Materials::Rebar::Grade& grade) const = 0;
   virtual void SetClosureJointStirrupMaterial(const CClosureKey& closureKey,WBFL::Materials::Rebar::Type type,WBFL::Materials::Rebar::Grade grade) = 0;
   virtual const CShearData2* GetClosureJointShearData(const CClosureKey& closureKey) const = 0;
   virtual void SetClosureJointShearData(const CClosureKey& closureKey,const CShearData2& data) = 0;
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
   virtual std::_tstring GetSegmentLongitudinalRebarMaterial(const CSegmentKey& segmentKey) const = 0;
   virtual void GetSegmentLongitudinalRebarMaterial(const CSegmentKey& segmentKey,WBFL::Materials::Rebar::Type& type,WBFL::Materials::Rebar::Grade& grade) const = 0;
   virtual void SetSegmentLongitudinalRebarMaterial(const CSegmentKey& segmentKey,WBFL::Materials::Rebar::Type type,WBFL::Materials::Rebar::Grade grade) = 0;
   virtual const CLongitudinalRebarData* GetSegmentLongitudinalRebarData(const CSegmentKey& segmentKey) const = 0;
   virtual void SetSegmentLongitudinalRebarData(const CSegmentKey& segmentKey,const CLongitudinalRebarData& data) = 0;

   virtual std::_tstring GetClosureJointLongitudinalRebarMaterial(const CClosureKey& closureKey) const = 0;
   virtual void GetClosureJointLongitudinalRebarMaterial(const CClosureKey& closureKey,WBFL::Materials::Rebar::Type& type,WBFL::Materials::Rebar::Grade& grade) const = 0;
   virtual void SetClosureJointLongitudinalRebarMaterial(const CClosureKey& closureKey,WBFL::Materials::Rebar::Type type,WBFL::Materials::Rebar::Grade grade) = 0;
   virtual const CLongitudinalRebarData* GetClosureJointLongitudinalRebarData(const CClosureKey& closureKey) const = 0;
   virtual void SetClosureJointLongitudinalRebarData(const CClosureKey& closureKey,const CLongitudinalRebarData& data) = 0;
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
   virtual std::_tstring GetSpecification() const = 0;
   virtual void SetSpecification(const std::_tstring& spec) = 0;

   virtual void GetTrafficBarrierDistribution(GirderIndexType* pNGirders,pgsTypes::TrafficBarrierDistribution* pDistType) const = 0;

   virtual Uint16 GetMomentCapacityMethod() const = 0;

   virtual void SetAnalysisType(pgsTypes::AnalysisType analysisType) = 0;
   virtual pgsTypes::AnalysisType GetAnalysisType() const = 0;

   virtual std::vector<arDesignOptions> GetDesignOptions(const CGirderKey& girderKey) const = 0;

   virtual bool IsSlabOffsetDesignEnabled() const = 0; // global setting from library

   virtual pgsTypes::OverlayLoadDistributionType GetOverlayLoadDistributionType() const = 0;

   // Tolerance value is only used if HaunchLoadComputationType==hlcDetailedAnalysis && slab offset input
   virtual pgsTypes::HaunchLoadComputationType GetHaunchLoadComputationType() const = 0;
   virtual Float64 GetCamberTolerance() const = 0;
   virtual Float64 GetHaunchLoadCamberFactor() const = 0;

   virtual std::_tstring GetRatingSpecification() const = 0;
   virtual void SetRatingSpecification(const std::_tstring& spec) = 0;

   // Assumed excess camber is used for parabolic variation of haunch load and composite props. These functions see if it
   // is applicable at all or for either
   virtual bool IsAssumedExcessCamberInputEnabled(bool considerDeckType=true) const = 0; // Depends on library and deck type
   virtual bool IsAssumedExcessCamberForLoad() const = 0; 
   virtual bool IsAssumedExcessCamberForSectProps() const = 0; 

   // Rounding method for required slab offset value
   virtual void GetRequiredSlabOffsetRoundingParameters(pgsTypes::SlabOffsetRoundingMethod* pMethod, Float64* pTolerance) const = 0;

   virtual void GetTaperedSolePlateRequirements(bool* pbCheckTaperedSolePlate, Float64* pTaperedSolePlateThreshold) const = 0;

   // Method and applicabiity for Principal Web stress check are based on several requirements
   typedef enum PrincipalWebStressCheckType { pwcNotApplicable, pwcAASHTOMethod, pwcNCHRPMethod, pwcNCHRPTimeStepMethod } PrincipalWebStressCheckType;

   virtual PrincipalWebStressCheckType GetPrincipalWebStressCheckType(const CSegmentKey& segmentKey) const = 0;

   virtual lrfdVersionMgr::Version GetSpecificationType() const = 0;
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
   virtual void EnumGdrConnectionNames( std::vector<std::_tstring>* pNames ) const = 0;
   virtual void EnumGirderNames( std::vector<std::_tstring>* pNames ) const = 0;
   virtual void EnumGirderNames( LPCTSTR strGirderFamily, std::vector<std::_tstring>* pNames ) const = 0;
   virtual void EnumConcreteNames( std::vector<std::_tstring>* pNames ) const = 0;
   virtual void EnumDiaphragmNames( std::vector<std::_tstring>* pNames ) const = 0;
   virtual void EnumTrafficBarrierNames( std::vector<std::_tstring>* pNames ) const = 0;
   virtual void EnumSpecNames( std::vector<std::_tstring>* pNames) const = 0;
   virtual void EnumLiveLoadNames( std::vector<std::_tstring>* pNames) const = 0;
   virtual void EnumDuctNames( std::vector<std::_tstring>* pNames) const = 0;
   virtual void EnumHaulTruckNames( std::vector<std::_tstring>* pNames) const = 0;

   virtual void EnumGirderFamilyNames( std::vector<std::_tstring>* pNames ) const = 0;
   virtual void GetBeamFactory(const std::_tstring& strBeamFamily,const std::_tstring& strBeamName,IBeamFactory** ppFactory) = 0;

   virtual void EnumRatingCriteriaNames( std::vector<std::_tstring>* pNames) const = 0;
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
   virtual void SetLibraryManager(psgLibraryManager* pNewLibMgr) = 0; 
   virtual psgLibraryManager* GetLibraryManager() = 0;
   virtual const psgLibraryManager* GetLibraryManager() const = 0;
   virtual const ConnectionLibraryEntry* GetConnectionEntry(LPCTSTR lpszName ) const = 0;
   virtual const GirderLibraryEntry* GetGirderEntry( LPCTSTR lpszName ) const = 0;
   virtual const ConcreteLibraryEntry* GetConcreteEntry( LPCTSTR lpszName ) const = 0;
   virtual const DiaphragmLayoutEntry* GetDiaphragmEntry( LPCTSTR lpszName ) const = 0;
   virtual const TrafficBarrierEntry* GetTrafficBarrierEntry( LPCTSTR lpszName ) const = 0;
   virtual const SpecLibraryEntry* GetSpecEntry( LPCTSTR lpszName ) const = 0;
   virtual const LiveLoadLibraryEntry* GetLiveLoadEntry( LPCTSTR lpszName ) const = 0;
   virtual const DuctLibraryEntry* GetDuctEntry( LPCTSTR lpszName ) const = 0;
   virtual const HaulTruckLibraryEntry* GetHaulTruckEntry(LPCTSTR lpszName) const = 0;
   virtual ConcreteLibrary&        GetConcreteLibrary() = 0;
   virtual ConnectionLibrary&      GetConnectionLibrary() = 0;
   virtual GirderLibrary&          GetGirderLibrary() = 0;
   virtual DiaphragmLayoutLibrary& GetDiaphragmLayoutLibrary() = 0;
   virtual TrafficBarrierLibrary&  GetTrafficBarrierLibrary() = 0;
   virtual SpecLibrary*            GetSpecLibrary() = 0;
   virtual LiveLoadLibrary*        GetLiveLoadLibrary() = 0;
   virtual DuctLibrary*            GetDuctLibrary() = 0;
   virtual HaulTruckLibrary*       GetHaulTruckLibrary() = 0;

   virtual std::vector<libEntryUsageRecord> GetLibraryUsageRecords() const = 0;
   virtual void GetMasterLibraryInfo(std::_tstring& strServer, std::_tstring& strConfiguration, std::_tstring& strMasterLib,WBFL::System::Time& time) const = 0;

   virtual const RatingLibrary* GetRatingLibrary() const = 0;
   virtual RatingLibrary* GetRatingLibrary() = 0;
   virtual const RatingLibraryEntry* GetRatingEntry( LPCTSTR lpszName ) const = 0;
};


/*****************************************************************************
INTERFACE
   IBridgeDescriptionEventSink

   Callback interface for bridge description events

DESCRIPTION
   Callback interface for bridge description events
*****************************************************************************/
#define GCH_PRESTRESSING_CONFIGURATION   0x0001
#define GCH_STRAND_MATERIAL              0x0002
#define GCH_STIRRUPS                     0x0004
#define GCH_LONGITUDINAL_REBAR           0x0008
#define GCH_LIFTING_CONFIGURATION        0x0010
#define GCH_SHIPPING_CONFIGURATION       0x0020
#define GCH_LOADING_ADDED                0x0040
#define GCH_LOADING_REMOVED              0x0080
#define GCH_LOADING_CHANGED              0x0100
#define GCH_CONCRETE                     0x0200
#define GCH_POSTTENSIONING_CONFIGURATION 0x0400


class CBridgeChangedHint
{
public:
   // Used when a span is added or removed... 
   PierIndexType PierIdx; // Reference pier where the span is added or removed
   pgsTypes::PierFaceType PierFace; // Pier face where the span is added or removed
   bool bAdded; // true if a span was added, false if removed
};

// {6132E890-719D-11d2-8EF1-006097DF3C68}
DEFINE_GUID(IID_IBridgeDescriptionEventSink, 
0x6132e890, 0x719d, 0x11d2, 0x8e, 0xf1, 0x0, 0x60, 0x97, 0xdf, 0x3c, 0x68);
interface IBridgeDescriptionEventSink : IUnknown
{
   virtual HRESULT OnBridgeChanged(CBridgeChangedHint* pHint) = 0;
   virtual HRESULT OnGirderFamilyChanged() = 0;
   virtual HRESULT OnGirderChanged(const CGirderKey& girderKey,Uint32 lHint) = 0;
   virtual HRESULT OnLiveLoadChanged() = 0;
   virtual HRESULT OnLiveLoadNameChanged(LPCTSTR strOldName,LPCTSTR strNewName) = 0;
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

   virtual Float64 GetDuctilityFactor() const = 0;
   virtual Float64 GetImportanceFactor() const = 0;
   virtual Float64 GetRedundancyFactor() const = 0;

   virtual Level GetDuctilityLevel() const = 0;
   virtual Level GetImportanceLevel() const = 0;
   virtual Level GetRedundancyLevel() const = 0;
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
   virtual bool ImportProjectLibraries(IStructuredLoad* pLoad) = 0;
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
   virtual bool HasUserDC(const CGirderKey& girderKey) const = 0;
   virtual bool HasUserDW(const CGirderKey& girderKey) const = 0;
   virtual bool HasUserLLIM(const CGirderKey& girderKey) const = 0;

   // point loads
   virtual CollectionIndexType GetPointLoadCount() const = 0;
   // add point load and return current count
   virtual CollectionIndexType AddPointLoad(EventIDType eventID,const CPointLoadData& pld) = 0;
   virtual const CPointLoadData* GetPointLoad(CollectionIndexType idx) const = 0;
   virtual const CPointLoadData* FindPointLoad(LoadIDType loadID) const = 0;
   virtual EventIndexType GetPointLoadEventIndex(LoadIDType loadID) const = 0;
   virtual EventIDType GetPointLoadEventID(LoadIDType loadID) const = 0;
   virtual void UpdatePointLoad(CollectionIndexType idx, EventIDType eventID, const CPointLoadData& pld) = 0;
   virtual void UpdatePointLoadByID(LoadIDType loadID, EventIDType eventID, const CPointLoadData& pld) = 0;
   virtual void DeletePointLoad(CollectionIndexType idx) = 0;
   virtual void DeletePointLoadByID(LoadIDType loadID) = 0;
   virtual std::vector<CPointLoadData> GetPointLoads(const CSpanKey& spanKey) const = 0;

   // distributed loads
   virtual CollectionIndexType GetDistributedLoadCount() const = 0;
   // add distributed load and return current count
   virtual CollectionIndexType AddDistributedLoad(EventIDType eventID,const CDistributedLoadData& pld) = 0;
   virtual const CDistributedLoadData* GetDistributedLoad(CollectionIndexType idx) const = 0;
   virtual const CDistributedLoadData* FindDistributedLoad(LoadIDType loadID) const = 0;
   virtual EventIndexType GetDistributedLoadEventIndex(LoadIDType loadID) const = 0;
   virtual EventIDType GetDistributedLoadEventID(LoadIDType loadID) const = 0;
   virtual void UpdateDistributedLoad(CollectionIndexType idx, EventIDType eventID, const CDistributedLoadData& pld) = 0;
   virtual void UpdateDistributedLoadByID(LoadIDType loadID, EventIDType eventID, const CDistributedLoadData& pld) = 0;
   virtual void DeleteDistributedLoad(CollectionIndexType idx) = 0;
   virtual void DeleteDistributedLoadByID(LoadIDType loadID) = 0;
   virtual std::vector<CDistributedLoadData> GetDistributedLoads(const CSpanKey& spanKey) const = 0;

   // moment loads
   virtual CollectionIndexType GetMomentLoadCount() const = 0;
   // add moment load and return current count
   virtual CollectionIndexType AddMomentLoad(EventIDType eventID,const CMomentLoadData& pld) = 0;
   virtual const CMomentLoadData* GetMomentLoad(CollectionIndexType idx) const = 0;
   virtual const CMomentLoadData* FindMomentLoad(LoadIDType loadID) const = 0;
   virtual EventIndexType GetMomentLoadEventIndex(LoadIDType loadID) const = 0;
   virtual EventIDType GetMomentLoadEventID(LoadIDType loadID) const = 0;
   virtual void UpdateMomentLoad(CollectionIndexType idx, EventIDType eventID, const CMomentLoadData& pld) = 0;
   virtual void UpdateMomentLoadByID(LoadIDType loadID, EventIDType eventID, const CMomentLoadData& pld) = 0;
   virtual void DeleteMomentLoad(CollectionIndexType idx) = 0;
   virtual void DeleteMomentLoadByID(LoadIDType loadID) = 0;
   virtual std::vector<CMomentLoadData> GetMomentLoads(const CSpanKey& spanKey) const = 0;

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

//////////////////////////////////////////////////////////////
// Simple exception-safe class for holding and releasing I events
//
template<class T> class CIEventsHolderT
{
public:
   CIEventsHolderT(T* pIEvents):
   m_pIEvents(pIEvents)
   {
      m_pIEvents->HoldEvents();
   }

   ~CIEventsHolderT() noexcept(false)
   {
      // exceptions downstream from this call can throw so we have to set 
      // the exception spec to permit it, otherwise the default is to call terminate()
      m_pIEvents->FirePendingEvents(); 
   }

private:
   CComPtr<T> m_pIEvents;
};

typedef CIEventsHolderT<IEvents> CIEventsHolder;

/*****************************************************************************
INTERFACE
   IEventsSink

   Interface to control events

DESCRIPTION
   Interface to control events
*****************************************************************************/
// {0538457E-9446-4a35-8B60-1263F4156763}
DEFINE_GUID(IID_IEventsSink, 
0x538457e, 0x9446, 0x4a35, 0x8b, 0x60, 0x12, 0x63, 0xf4, 0x15, 0x67, 0x63);
interface IEventsSink : IUnknown
{
   virtual HRESULT OnHoldEvents() = 0;
   virtual HRESULT OnFirePendingEvents() = 0;
   virtual HRESULT OnCancelPendingEvents() = 0;
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
   virtual void CancelPendingEvents() = 0;
   virtual void FireEvent(CView* pSender = nullptr,LPARAM lHint = 0,std::shared_ptr<CObject> pHint = nullptr) = 0;
};
typedef CIEventsHolderT<IUIEvents> CUIEventsHolder;

/*****************************************************************************
INTERFACE
   ILimits

   Interface to get information about limits

DESCRIPTION
   Interface to get information about limits
*****************************************************************************/
// {797998B4-FE8C-4ad1-AE9E-167A193CC296}
DEFINE_GUID(IID_ILimits, 
0x797998b4, 0xfe8c, 0x4ad1, 0xae, 0x9e, 0x16, 0x7a, 0x19, 0x3c, 0xc2, 0x96);
interface ILimits : IUnknown
{
   virtual Float64 GetMaxSlabFc(pgsTypes::ConcreteType concType) const = 0;
   virtual Float64 GetMaxSegmentFci(pgsTypes::ConcreteType concType) const = 0;
   virtual Float64 GetMaxSegmentFc(pgsTypes::ConcreteType concType) const = 0;
   virtual Float64 GetMaxClosureFci(pgsTypes::ConcreteType concType) const = 0;
   virtual Float64 GetMaxClosureFc(pgsTypes::ConcreteType concType) const = 0;
   virtual Float64 GetMaxConcreteUnitWeight(pgsTypes::ConcreteType concType) const = 0;
   virtual Float64 GetMaxConcreteAggSize(pgsTypes::ConcreteType concType) const = 0;
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
   enum PedestrianLoadApplicationType {PedDontApply, PedConcurrentWithVehicular, PedEnvelopeWithVehicular};

   virtual bool IsLiveLoadDefined(pgsTypes::LiveLoadType llType) const = 0;
   virtual PedestrianLoadApplicationType GetPedestrianLoadApplication(pgsTypes::LiveLoadType llType) const = 0;
   // SetPedestrianLoadApplication function only applicable to lltDesign, lltPermit, lltFatigue
   virtual void SetPedestrianLoadApplication(pgsTypes::LiveLoadType llType, PedestrianLoadApplicationType PedLoad) = 0;
   virtual std::vector<std::_tstring> GetLiveLoadNames(pgsTypes::LiveLoadType llType) const = 0;
   virtual void SetLiveLoadNames(pgsTypes::LiveLoadType llType,const std::vector<std::_tstring>& names) = 0;
   virtual Float64 GetTruckImpact(pgsTypes::LiveLoadType llType) const = 0;
   virtual void SetTruckImpact(pgsTypes::LiveLoadType llType,Float64 impact) = 0;
   virtual Float64 GetLaneImpact(pgsTypes::LiveLoadType llType) const = 0;
   virtual void SetLaneImpact(pgsTypes::LiveLoadType llType,Float64 impact) = 0;
   virtual void SetLldfRangeOfApplicabilityAction(LldfRangeOfApplicabilityAction action) = 0;
   virtual LldfRangeOfApplicabilityAction GetLldfRangeOfApplicabilityAction() const = 0;
   virtual std::_tstring GetLLDFSpecialActionText() const = 0; // get common string for ignore roa case
   virtual bool IgnoreLLDFRangeOfApplicability() const = 0; // true if action is to ignore ROA
};

// {483673C2-9F4E-40ec-9DC2-6B36B0D34498}
DEFINE_GUID(IID_IBridgeDescription, 
0x483673c2, 0x9f4e, 0x40ec, 0x9d, 0xc2, 0x6b, 0x36, 0xb0, 0xd3, 0x44, 0x98);
interface IBridgeDescription : IUnknown
{
   virtual const CBridgeDescription2* GetBridgeDescription() const = 0;
   virtual void SetBridgeDescription(const CBridgeDescription2& desc) = 0;

   virtual const CDeckDescription2* GetDeckDescription() const = 0;
   virtual void SetDeckDescription(const CDeckDescription2& deck) = 0;

   virtual SpanIndexType GetSpanCount() const = 0;
   virtual const CSpanData2* GetSpan(SpanIndexType spanIdx) const = 0;
   virtual void SetSpan(SpanIndexType spanIdx,const CSpanData2& spanData) = 0;

   virtual PierIndexType GetPierCount() const = 0;
   virtual const CPierData2* GetPier(PierIndexType pierIdx) const = 0;
   virtual const CPierData2* FindPier(PierIDType pierID) const = 0;
   virtual void SetPierByIndex(PierIndexType pierIdx,const CPierData2& PierData) = 0;
   virtual void SetPierByID(PierIDType pierID,const CPierData2& PierData) = 0;

   virtual SupportIndexType GetTemporarySupportCount() const = 0;
   virtual const CTemporarySupportData* GetTemporarySupport(SupportIndexType tsIdx) const = 0;
   virtual const CTemporarySupportData* FindTemporarySupport(SupportIDType tsID) const = 0;
   virtual void SetTemporarySupportByIndex(SupportIndexType tsIdx,const CTemporarySupportData& tsData) = 0;
   virtual void SetTemporarySupportByID(SupportIDType tsID,const CTemporarySupportData& tsData) = 0;

   virtual GroupIndexType GetGirderGroupCount() const = 0;
   virtual const CGirderGroupData* GetGirderGroup(GroupIndexType grpIdx) const = 0;
   virtual void SetGirderGroup(GroupIndexType grpIdx,const CGirderGroupData& girderGroup) = 0;

   virtual const CSplicedGirderData* GetGirder(const CGirderKey& girderKey) const = 0;
   virtual const CSplicedGirderData* FindGirder(GirderIDType gdrID) const = 0;
   virtual void SetGirder(const CGirderKey& girderKey,const CSplicedGirderData& splicedGirder) = 0;

   virtual const CPTData* GetPostTensioning(const CGirderKey& girderKey) const = 0;
   virtual void SetPostTensioning(const CGirderKey& girderKey,const CPTData& ptData) = 0;

   virtual const CPrecastSegmentData* GetPrecastSegmentData(const CSegmentKey& segmentKey) const = 0;
   virtual void SetPrecastSegmentData(const CSegmentKey& segmentKey,const CPrecastSegmentData& segment) = 0;

   virtual const CClosureJointData* GetClosureJointData(const CClosureKey& closureKey) const = 0;
   virtual void SetClosureJointData(const CClosureKey& closureKey,const CClosureJointData& closure) = 0;

   virtual void SetSpanLength(SpanIndexType spanIdx,Float64 newLength) = 0;
   virtual void MovePier(PierIndexType pierIdx,Float64 newStation,pgsTypes::MovePierOption moveOption) = 0;
   virtual SupportIndexType MoveTemporarySupport(SupportIndexType tsIdx,Float64 newStation) = 0;

   virtual void SetMeasurementType(PierIndexType pierIdx,pgsTypes::PierFaceType pierFace,pgsTypes::MeasurementType mt) = 0;
   virtual void SetMeasurementLocation(PierIndexType pierIdx,pgsTypes::PierFaceType pierFace,pgsTypes::MeasurementLocation ml) = 0;
   virtual void SetGirderSpacing(PierIndexType pierIdx,pgsTypes::PierFaceType face,const CGirderSpacing2& spacing) = 0;
   virtual void SetGirderSpacingAtStartOfGroup(GroupIndexType grpIdx,const CGirderSpacing2& spacing) = 0;
   virtual void SetGirderSpacingAtEndOfGroup(GroupIndexType grpIdx,const CGirderSpacing2& spacing) = 0;
   virtual void SetGirderName(const CGirderKey& girderKey, LPCTSTR strGirderName) = 0;
   virtual void SetGirderCount(GroupIndexType grpIdx,GirderIndexType nGirders) = 0;
   virtual void SetBoundaryCondition(PierIndexType pierIdx,pgsTypes::BoundaryConditionType connectionType) = 0;
   virtual void SetBoundaryCondition(PierIndexType pierIdx,pgsTypes::PierSegmentConnectionType connectionType,EventIndexType castClosureEventIdx) = 0;
   virtual void SetBoundaryCondition(SupportIndexType tsIdx, pgsTypes::TemporarySupportType supportType) = 0;
   virtual void SetBoundaryCondition(SupportIndexType tsIdx, pgsTypes::TempSupportSegmentConnectionType connectionType, EventIndexType castClosureEventIdx) = 0;

   virtual void DeletePier(PierIndexType pierIdx,pgsTypes::PierFaceType faceForSpan) = 0;
   virtual void InsertSpan(PierIndexType refPierIdx,pgsTypes::PierFaceType pierFace,Float64 spanLength,const CSpanData2* pSpanData,const CPierData2* pPierData,bool bCreateNewGroup,EventIndexType eventIdx) = 0;

   virtual void InsertTemporarySupport(CTemporarySupportData* pTSData,EventIndexType erectionEventIdx,EventIndexType removalEventIdx,EventIndexType castClosureJointEventIdx) = 0;
   virtual void DeleteTemporarySupportByIndex(SupportIndexType tsIdx) = 0;
   virtual void DeleteTemporarySupportByID(SupportIDType tsID) = 0;

   virtual void SetLiveLoadDistributionFactorMethod(pgsTypes::DistributionFactorMethod method) = 0;
   virtual pgsTypes::DistributionFactorMethod GetLiveLoadDistributionFactorMethod() const = 0;

   virtual void UseSameNumberOfGirdersInAllGroups(bool bUseSame) = 0;
   virtual bool UseSameNumberOfGirdersInAllGroups() const = 0;
   virtual void SetGirderCount(GirderIndexType nGirders) = 0; // used for the entire bridge

   virtual void UseSameGirderForEntireBridge(bool bSame) = 0;
   virtual bool UseSameGirderForEntireBridge() const = 0;
   virtual void SetGirderName(LPCTSTR strGirderName) = 0; // sets the name of the girder that is used in all spans

   virtual void SetGirderSpacingType(pgsTypes::SupportedBeamSpacing sbs) = 0;
   virtual pgsTypes::SupportedBeamSpacing GetGirderSpacingType() const = 0;
   virtual void SetGirderSpacing(Float64 spacing) = 0; // used for the entire bridge
   virtual void SetMeasurementType(pgsTypes::MeasurementType mt) = 0;
   virtual pgsTypes::MeasurementType GetMeasurementType() const = 0;
   virtual void SetMeasurementLocation(pgsTypes::MeasurementLocation ml) = 0;
   virtual pgsTypes::MeasurementLocation GetMeasurementLocation() const = 0;

   virtual void SetWearingSurfaceType(pgsTypes::WearingSurfaceType wearingSurfaceType) = 0;

   /////////// Haunch Input //////////////
   // Define how haunch is input. This can be older Slab Offset dimension method or newer hidHaunchDirectly and hidHaunchPlusSlabDirectly methods
   virtual void SetHaunchInputDepthType(pgsTypes::HaunchInputDepthType type) = 0;
   virtual pgsTypes::HaunchInputDepthType GetHaunchInputDepthType() const = 0;

   // slab offset (valid only when HaunchInputDepthType == pgsTypes::hidACamber)
   // changes slab offset type to be sotBridge
   virtual void SetSlabOffsetType(pgsTypes::SlabOffsetType offsetType) = 0;
   virtual void SetSlabOffset( Float64 slabOffset) = 0;
   virtual pgsTypes::SlabOffsetType GetSlabOffsetType() const = 0;
   // changes slab offset type to sotBearingLine at permanent piers.
   virtual void SetSlabOffset(SupportIndexType supportIdx, pgsTypes::PierFaceType face, Float64 offset) = 0;
   virtual void SetSlabOffset(SupportIndexType supportIdx, Float64 backSlabOffset,Float64 aheadSlabOffset) = 0;
   virtual Float64 GetSlabOffset(SupportIndexType supportIdx, pgsTypes::PierFaceType face) const = 0;
   virtual void GetSlabOffset(SupportIndexType supportIdx, Float64* pBackSlabOffset, Float64* pAheadSlabOffset) const = 0;
   // sets slab offset per girder ... sets the slab offset type to sotSegment
   virtual void SetSlabOffset(const CSegmentKey& segmentKey, pgsTypes::MemberEndType end, Float64 offset) = 0;
   virtual void SetSlabOffset(const CSegmentKey& segmentKey, Float64 startSlabOffset,Float64 endSlabOffset) = 0;
   virtual Float64 GetSlabOffset(const CSegmentKey& segmentKey, pgsTypes::MemberEndType end) const = 0;
   virtual void GetSlabOffset(const CSegmentKey& segmentKey, Float64* pStartSlabOffset,Float64* pEndSlabOffset) const = 0;

   // fillet is used for both slab offset and direct haunch input
   virtual void SetFillet( Float64 Fillet) = 0;
   virtual Float64 GetFillet() const = 0;

   // Assumed Excess Camber
   virtual void SetAssumedExcessCamberType(pgsTypes::AssumedExcessCamberType cType) = 0;
   virtual pgsTypes::AssumedExcessCamberType GetAssumedExcessCamberType() const = 0;
   // changes AssumedExcessCamber type to be aecBridge
   virtual void SetAssumedExcessCamber( Float64 assumedExcessCamber) = 0;
   // changes AssumedExcessCamber type to fttPier
   virtual void SetAssumedExcessCamber(SpanIndexType spanIdx, Float64 assumedExcessCamber) = 0;
   // sets AssumedExcessCamber per girder ... sets the AssumedExcessCamber type to aecGirder
   virtual void SetAssumedExcessCamber( SpanIndexType spanIdx, GirderIndexType gdrIdx, Float64 assumedExcessCamber) = 0;
   virtual Float64 GetAssumedExcessCamber( SpanIndexType spanIdx, GirderIndexType gdrIdx) const = 0;

   // Direct input of haunch depths (valid only when HaunchInputDepthType == pgsTypes::hidHaunchDirectly or hidHaunchPlusSlabDirectly)
   virtual void SetHaunchInputLocationType(pgsTypes::HaunchInputLocationType type) = 0;
   virtual pgsTypes::HaunchInputLocationType GetHaunchInputLocationType() const = 0;
   virtual void SetHaunchLayoutType(pgsTypes::HaunchLayoutType type) = 0;
   virtual pgsTypes::HaunchLayoutType GetHaunchLayoutType() const = 0;
   virtual void SetHaunchInputDistributionType(pgsTypes::HaunchInputDistributionType type) = 0;
   virtual pgsTypes::HaunchInputDistributionType GetHaunchInputDistributionType() const = 0;
   // Set haunch depths for entire bridge (segments or spans depending on HaunchLayoutType) (HaunchInputLocationType must be preset to hilSame4Bridge)
   virtual void SetDirectHaunchDepths4Bridge(const std::vector<Float64>& haunchDepths) = 0;
   // Method valid only when HaunchLayoutType==hltAlongSpans && HaunchInputLocationType==hilSame4AllGirders
   virtual void SetDirectHaunchDepthsPerSpan(SpanIndexType spanIdx,const std::vector<Float64>& haunchDepths) = 0;
   // Method valid only when HaunchLayoutType==hltAlongSpans && HaunchInputLocationType==hilPerEach
   virtual void SetDirectHaunchDepthsPerSpan(SpanIndexType spanIdx,GirderIndexType gdrIdx,const std::vector<Float64>& haunchDepths) = 0;
   // Method valid only when HaunchLayoutType==hltAlongSegments && HaunchInputLocationType==hilSame4AllGirders
   virtual void SetDirectHaunchDepthsPerSegment(GroupIndexType group,SegmentIndexType SegmentIdx,const std::vector<Float64>& haunchDepths) = 0;
   // Method valid only when HaunchLayoutType==hltAlongSegments && HaunchInputLocationType==hilPerEach
   virtual void SetDirectHaunchDepthsPerSegment(GroupIndexType group,GirderIndexType gdrIdx,SegmentIndexType SegmentIdx,const std::vector<Float64>& haunchDepths) = 0;
   // Method valid only when HaunchLayoutType==hltAlongSpans
   virtual std::vector<Float64> GetDirectHaunchDepthsPerSpan(SpanIndexType spanIdx,GirderIndexType gdrIdx) = 0;
   // Method valid only when HaunchLayoutType==hltAlongSegments
   virtual std::vector<Float64> GetDirectHaunchDepthsPerSegment(GroupIndexType group,GirderIndexType gdrIdx, SegmentIndexType SegmentIdx) = 0;

   // Returns a vector of valid connection types
   virtual std::vector<pgsTypes::BoundaryConditionType> GetBoundaryConditionTypes(PierIndexType pierIdx) const = 0;
   virtual std::vector<pgsTypes::PierSegmentConnectionType> GetPierSegmentConnectionTypes(PierIndexType pierIdx) const = 0;

   // timeline management
   virtual const CTimelineManager* GetTimelineManager() const = 0;
   virtual void SetTimelineManager(const CTimelineManager& timelineMgr) = 0;

   virtual EventIndexType AddTimelineEvent(const CTimelineEvent& timelineEvent) = 0;
   virtual EventIndexType GetEventCount() const = 0;
   virtual const CTimelineEvent* GetEventByIndex(EventIndexType eventIdx) const = 0;
   virtual const CTimelineEvent* GetEventByID(EventIDType eventID) const = 0;
   virtual void SetEventByIndex(EventIndexType eventIdx,const CTimelineEvent& timelineEvent) = 0;
   virtual void SetEventByID(EventIDType eventID,const CTimelineEvent& timelineEvent) = 0;

   virtual EventIndexType GetPierErectionEvent(PierIndexType pierIdx) const = 0;
   virtual void SetPierErectionEventByIndex(PierIndexType pierIdx,EventIndexType eventIdx) = 0;
   virtual void SetPierErectionEventByID(PierIndexType pierIdx,EventIDType eventID) = 0;
   virtual void SetTempSupportEventsByIndex(SupportIndexType tsIdx,EventIndexType erectEventIdx,EventIndexType removeEventIdx) = 0;
   virtual void SetTempSupportEventsByID(SupportIDType tsID,EventIndexType erectEventIdx,EventIndexType removeEventIdx) = 0;

   virtual void SetSegmentConstructionEventByIndex(const CSegmentKey& segmentKey,EventIndexType eventIdx) = 0;
   virtual void SetSegmentConstructionEventByID(const CSegmentKey& segmentKey,EventIDType eventID) = 0;
   virtual EventIndexType GetSegmentConstructionEventIndex(const CSegmentKey& segmentKey) const = 0;
   virtual EventIDType GetSegmentConstructionEventID(const CSegmentKey& segmentKey) const = 0;

   virtual void SetSegmentErectionEventByIndex(const CSegmentKey& segmentKey,EventIndexType eventIdx) = 0;
   virtual void SetSegmentErectionEventByID(const CSegmentKey& segmentKey,EventIDType eventID) = 0;
   virtual EventIndexType GetSegmentErectionEventIndex(const CSegmentKey& segmentKey) const = 0;
   virtual EventIDType GetSegmentErectionEventID(const CSegmentKey& segmentKey) const = 0;

   virtual void SetSegmentEventsByIndex(const CSegmentKey& segmentKey,EventIndexType constructionEventIdx,EventIndexType erectionEventIdx) = 0;
   virtual void SetSegmentEventsByID(const CSegmentKey& segmentKey,EventIDType constructionEventID,EventIDType erectionEventID) = 0;
   virtual void GetSegmentEventsByIndex(const CSegmentKey& segmentKey,EventIndexType* constructionEventIdx,EventIndexType* erectionEventIdx) const = 0;
   virtual void GetSegmentEventsByID(const CSegmentKey& segmentKey,EventIDType* constructionEventID,EventIDType* erectionEventID) const = 0;

   virtual EventIndexType GetCastClosureJointEventIndex(GroupIndexType grpIdx,CollectionIndexType closureIdx) const = 0;
   virtual EventIDType GetCastClosureJointEventID(GroupIndexType grpIdx,CollectionIndexType closureIdx) const = 0;
   virtual void SetCastClosureJointEventByIndex(GroupIndexType grpIdx,CollectionIndexType closureIdx,EventIndexType eventIdx) = 0;
   virtual void SetCastClosureJointEventByID(GroupIndexType grpIdx,CollectionIndexType closureIdx,EventIDType eventID) = 0;

   virtual EventIndexType GetStressTendonEventIndex(const CGirderKey& girderKey,DuctIndexType ductIdx) const = 0;
   virtual EventIDType GetStressTendonEventID(const CGirderKey& girderKey,DuctIndexType ductIdx) const = 0;
   virtual void SetStressTendonEventByIndex(const CGirderKey& girderKey,DuctIndexType ductIdx,EventIndexType eventIdx) = 0;
   virtual void SetStressTendonEventByID(const CGirderKey& girderKey,DuctIndexType ductIdx,EventIDType eventID) = 0;

   virtual EventIndexType GetCastLongitudinalJointEventIndex() const = 0;
   virtual EventIDType GetCastLongitudinalJointEventID() const = 0;
   virtual int SetCastLongitudinalJointEventByIndex(EventIndexType eventIdx, bool bAdjustTimeline) = 0;
   virtual int SetCastLongitudinalJointEventByID(EventIDType eventID, bool bAdjustTimeline) = 0;

   virtual EventIndexType GetCastDeckEventIndex() const = 0;
   virtual EventIDType GetCastDeckEventID() const = 0;
   virtual int SetCastDeckEventByIndex(EventIndexType eventIdx,bool bAdjustTimeline) = 0;
   virtual int SetCastDeckEventByID(EventIDType eventID,bool bAdjustTimeline) = 0;

   virtual EventIndexType GetIntermediateDiaphragmsLoadEventIndex() const = 0;
   virtual EventIDType GetIntermediateDiaphragmsLoadEventID() const = 0;
   virtual void SetIntermediateDiaphragmsLoadEventByIndex(EventIndexType eventIdx) = 0;
   virtual void SetIntermediateDiaphragmsLoadEventByID(EventIDType eventID) = 0;

   virtual EventIndexType GetRailingSystemLoadEventIndex() const = 0;
   virtual EventIDType GetRailingSystemLoadEventID() const = 0;
   virtual void SetRailingSystemLoadEventByIndex(EventIndexType eventIdx) = 0;
   virtual void SetRailingSystemLoadEventByID(EventIDType eventID) = 0;

   virtual EventIndexType GetOverlayLoadEventIndex() const = 0;
   virtual EventIDType GetOverlayLoadEventID() const = 0;
   virtual void SetOverlayLoadEventByIndex(EventIndexType eventIdx) = 0;
   virtual void SetOverlayLoadEventByID(EventIDType eventID) = 0;

   virtual EventIndexType GetLiveLoadEventIndex() const = 0;
   virtual EventIDType GetLiveLoadEventID() const = 0;
   virtual void SetLiveLoadEventByIndex(EventIndexType eventIdx) = 0;
   virtual void SetLiveLoadEventByID(EventIDType eventID) = 0;

   virtual GroupIDType GetGroupID(GroupIndexType groupIdx) const = 0;
   virtual GirderIDType GetGirderID(const CGirderKey& girderKey) const = 0;
   virtual SegmentIDType GetSegmentID(const CSegmentKey& segmentKey) const = 0;

   virtual void SetBearingType(pgsTypes::BearingType offsetType) = 0;
   virtual pgsTypes::BearingType GetBearingType() const = 0;
   virtual void SetBearingData(const CBearingData2* pBearingData) = 0;
   virtual void SetBearingData(GroupIndexType grpIdx, PierIndexType pierIdx, pgsTypes::PierFaceType face, const CBearingData2* pBearingData) = 0;
   virtual void SetBearingData(GroupIndexType grpIdx, PierIndexType pierIdx, pgsTypes::PierFaceType face, GirderIndexType gdrIdx, const CBearingData2* pBearingData) = 0;
   virtual const CBearingData2* GetBearingData(PierIDType pierID, pgsTypes::PierFaceType face, GirderIndexType gdrIdx) const = 0;

   virtual void SetConnectionGeometry(PierIndexType pierIdx, pgsTypes::PierFaceType face, 
                                      Float64 endDist, ConnectionLibraryEntry::EndDistanceMeasurementType endDistMeasure,
                                      Float64 bearingOffset, ConnectionLibraryEntry::BearingOffsetMeasurementType bearingOffsetMeasurementType) = 0;
   virtual void GetConnectionGeometry(PierIndexType pierIdx, pgsTypes::PierFaceType face, 
                                      Float64* endDist, ConnectionLibraryEntry::EndDistanceMeasurementType* endDistMeasure,
                                      Float64* bearingOffset, ConnectionLibraryEntry::BearingOffsetMeasurementType* bearingOffsetMeasurementType) const = 0;

   virtual void SetPierDiaphragmData(PierIndexType pierIdx, pgsTypes::PierFaceType face,
                                    Float64 height, Float64 width, ConnectionLibraryEntry::DiaphragmLoadType loadType, Float64 loadLocation) = 0;
   virtual void GetPierDiaphragmData(PierIndexType pierIdx, pgsTypes::PierFaceType face,
                                    Float64* pHeight, Float64* pWidth, ConnectionLibraryEntry::DiaphragmLoadType* pLoadType, Float64* pLoadLocation) const = 0;

   virtual bool IsCompatibleGirder(const CGirderKey& girderKey, LPCTSTR lpszGirderName) const = 0;
   virtual bool AreGirdersCompatible(GroupIndexType groupIdx) const = 0;
   virtual bool AreGirdersCompatible(const std::vector<std::_tstring>& vGirderNames) const = 0;
   virtual bool AreGirdersCompatible(const CBridgeDescription2& bridgeDescription, const std::vector<std::_tstring>& vGirderNames) const = 0;
};

/*****************************************************************************
INTERFACE
   IEventMap

   Interface for managing event names.

DESCRIPTION
   Interface for managing event names. Provides information about event names
   and sequences

*****************************************************************************/
// {B1728315-80C4-4174-A587-CE5EF80C5E15}
DEFINE_GUID(IID_IEventMap, 
0xb1728315, 0x80c4, 0x4174, 0xa5, 0x87, 0xce, 0x5e, 0xf8, 0xc, 0x5e, 0x15);
interface IEventMap : IUnknown
{
   virtual CComBSTR GetEventName(EventIndexType eventIdx) const = 0;  
   virtual EventIndexType GetEventIndex(CComBSTR bstrEvent) const = 0;
   virtual EventIndexType GetEventCount() const = 0;
};

/*****************************************************************************
INTERFACE
   IEffectiveFlangeWidth

DESCRIPTION
   Interface for controlling how effective flange widths are computed.
*****************************************************************************/
// {1EAF4313-36A3-434d-801A-0458447A9B49}
DEFINE_GUID(IID_IEffectiveFlangeWidth, 
0x1eaf4313, 0x36a3, 0x434d, 0x80, 0x1a, 0x4, 0x58, 0x44, 0x7a, 0x9b, 0x49);
interface IEffectiveFlangeWidth : IUnknown
{
   virtual bool IgnoreEffectiveFlangeWidthLimits() const = 0;
   virtual void IgnoreEffectiveFlangeWidthLimits(bool bIgnore) = 0;
};

/*****************************************************************************
INTERFACE
   ILossParameters

DESCRIPTION
   Interface for getting information about the prestress loss method and
   related parameters. The information returned from this interface describe
   how losses are to be computed, not the actual losses.
*****************************************************************************/
// {AAF586AA-D06E-446b-9EB2-CA916427AD9E}
DEFINE_GUID(IID_ILossParameters, 
0xaaf586aa, 0xd06e, 0x446b, 0x9e, 0xb2, 0xca, 0x91, 0x64, 0x27, 0xad, 0x9e);
interface ILossParameters : IUnknown
{
   // Returns a string that describes the method used to compute prestress losses
   // This string is intended to be used in reports
   virtual std::_tstring GetLossMethodDescription() const = 0;

   // Returns the method for computing prestress losses
   virtual pgsTypes::LossMethod GetLossMethod() const = 0;

   // Returns the time-dependent model type
   virtual pgsTypes::TimeDependentModel GetTimeDependentModel() const = 0;

   // Indicates if time dependent effects are ignored during time-step analysis.
   // This setting only applies to time-step analysis. If ignored, the time-step
   // analysis results are restricted to an elastic response.
   virtual void IgnoreCreepEffects(bool bIgnore) = 0;
   virtual bool IgnoreCreepEffects() const = 0;
   virtual void IgnoreShrinkageEffects(bool bIgnore) = 0;
   virtual bool IgnoreShrinkageEffects() const = 0;
   virtual void IgnoreRelaxationEffects(bool bIgnore) = 0;
   virtual bool IgnoreRelaxationEffects() const = 0;
   virtual void IgnoreTimeDependentEffects(bool bIgnoreCreep,bool bIgnoreShrinkage,bool bIgnoreRelaxation) = 0;

   // Set/Get the parameters for computing initial losses in post-tension tendons
   virtual void SetTendonPostTensionParameters(Float64 Dset,Float64 wobble,Float64 friction) = 0;
   virtual void GetTendonPostTensionParameters(Float64* Dset,Float64* wobble,Float64* friction) const = 0;

   // Set/Get the parameters for computing initial losses in post-tension temporary strands
   virtual void SetTemporaryStrandPostTensionParameters(Float64 Dset,Float64 wobble,Float64 friction) = 0;
   virtual void GetTemporaryStrandPostTensionParameters(Float64* Dset,Float64* wobble,Float64* friction) const = 0;

   // Indicates if the method of computing prestress losses is ignored
   // and general lump sum losses are used. If general lump sum losses
   // are used, use the methods below to get the loss values.
   virtual void UseGeneralLumpSumLosses(bool bLumpSum) = 0;
   virtual bool UseGeneralLumpSumLosses() const = 0;

   // Set/Get general lump sum losses

   //------------------------------------------------------------------------
   // Returns the losses before prestress xfer for a lump sum method
   virtual Float64 GetBeforeXferLosses() const = 0;
   virtual void SetBeforeXferLosses(Float64 loss) = 0;

   //------------------------------------------------------------------------
   // Returns the losses after prestress xfer for a lump sum method
   virtual Float64 GetAfterXferLosses() const = 0;
   virtual void SetAfterXferLosses(Float64 loss) = 0;

   virtual Float64 GetLiftingLosses() const = 0;
   virtual void SetLiftingLosses(Float64 loss) = 0;

   //------------------------------------------------------------------------
   // Returns the shipping losses for a lump sum method or for a method
   // that does not support computing losses at shipping.
   virtual Float64 GetShippingLosses() const = 0;
   virtual void SetShippingLosses(Float64 loss) = 0;

   //------------------------------------------------------------------------
   virtual Float64 GetBeforeTempStrandRemovalLosses() const = 0;
   virtual void SetBeforeTempStrandRemovalLosses(Float64 loss) = 0;

   //------------------------------------------------------------------------
   virtual Float64 GetAfterTempStrandRemovalLosses() const = 0;
   virtual void SetAfterTempStrandRemovalLosses(Float64 loss) = 0;

   //------------------------------------------------------------------------
   virtual Float64 GetAfterDeckPlacementLosses() const = 0;
   virtual void SetAfterDeckPlacementLosses(Float64 loss) = 0;

   //------------------------------------------------------------------------
   virtual Float64 GetAfterSIDLLosses() const = 0;
   virtual void SetAfterSIDLLosses(Float64 loss) = 0;

   //------------------------------------------------------------------------
   // Returns the final losses for a lump sum method
   virtual Float64 GetFinalLosses() const = 0;
   virtual void SetFinalLosses(Float64 loss) = 0;
};

/*****************************************************************************
INTERFACE
   ILossParametersEventSink

   Callback interface for Loss Parameters events

DESCRIPTION
   Callback interface for Parameters events
*****************************************************************************/
// {E677C320-9E35-4ce2-9FA6-083E99F87742}
DEFINE_GUID(IID_ILossParametersEventSink, 
0xe677c320, 0x9e35, 0x4ce2, 0x9f, 0xa6, 0x8, 0x3e, 0x99, 0xf8, 0x77, 0x42);
interface ILossParametersEventSink : IUnknown
{
   virtual HRESULT OnLossParametersChanged() = 0;
};


/*****************************************************************************
INTERFACE
   IValidate
*****************************************************************************/
#define VALIDATE_SUCCESS 0
#define VALIDATE_INVALID 1 // the string is bad (nullptr or blank)
#define VALIDATE_SKEW_ANGLE 2

// {C3D02F95-D861-483d-8A41-11FC3A16D77F}
DEFINE_GUID(IID_IValidate, 
0xc3d02f95, 0xd861, 0x483d, 0x8a, 0x41, 0x11, 0xfc, 0x3a, 0x16, 0xd7, 0x7f);
interface IValidate: IUnknown
{
   virtual UINT Orientation(LPCTSTR lpszOrientation) = 0;
};
