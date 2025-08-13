///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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

#include <EAF/Agent.h>
#include "CLSID.h"
#include "PsForceEng.h"
#include "Designer2.h"
#include "LoadRater.h"
#include "MomentCapacityEngineer.h"
#include "ShearCapacityEngineer.h"
#include "TransferLengthEngineer.h"
#include "DevelopmentLengthEngineer.h"
#include "BearingDesignEngineer.h"
#include "PrincipalWebStressEngineer.h"

#include <IFace\MomentCapacity.h>
#include <IFace\ShearCapacity.h>
#include <IFace\Constructability.h>
#include <IFace\DistFactorEngineer.h>
#include <IFace\RatingSpecification.h>
#include <IFace\CrackedSection.h>
#include <IFace\PrincipalWebStress.h>


#include <PgsExt\PoiKey.h>
#include <PsgLib\Keys.h>

#include <map>

#if defined _USE_MULTITHREADING
#include <PgsExt\ThreadManager.h>
#endif

class PrestressWithLiveLoadSubKey
{
public:
   PrestressWithLiveLoadSubKey()
   {
      m_LimitState = pgsTypes::StrengthI;
      m_Strand = pgsTypes::Straight;
      m_VehicleIdx = INVALID_INDEX;
   }

   PrestressWithLiveLoadSubKey(pgsTypes::StrandType strand,pgsTypes::LimitState limitState,VehicleIndexType vehicleIdx) :
      m_Strand(strand), m_LimitState(limitState), m_VehicleIdx(vehicleIdx)
      {
      }

   PrestressWithLiveLoadSubKey(const PrestressWithLiveLoadSubKey& rOther)
   {
      m_Strand = rOther.m_Strand;
      m_LimitState = rOther.m_LimitState;
      m_VehicleIdx = rOther.m_VehicleIdx;
   }

   bool operator<(const PrestressWithLiveLoadSubKey& rOther) const
   {
      if ( m_Strand < rOther.m_Strand ) 
      {
         return true;
      }

      if ( rOther.m_Strand < m_Strand ) 
      {
         return false;
      }

      if ( m_LimitState < rOther.m_LimitState )
      {
         return true;
      }

      if (rOther.m_LimitState < m_LimitState)
      {
         return false;
      }

      if (m_VehicleIdx < rOther.m_VehicleIdx)
      {
         return true;
      }

      return false;
   }

private:
   pgsTypes::LimitState m_LimitState;
   pgsTypes::StrandType m_Strand;
   VehicleIndexType m_VehicleIdx;
};

typedef TPoiKey<PrestressWithLiveLoadSubKey> PrestressWithLiveLoadPoiKey;


/////////////////////////////////////////////////////////////////////////////
// CEngAgentImp
class CEngAgentImp : public WBFL::EAF::Agent,
   public ILosses,
   public IPretensionForce,
   public IPosttensionForce,
   public ILiveLoadDistributionFactors,
   public IMomentCapacity,
   public IShearCapacity,
   public IPrincipalWebStress,
   public IGirderHaunch,
   public IBearingDesignParameters,
   public IFabricationOptimization,
   public IArtifact,
   public IBridgeDescriptionEventSink,
   public ISpecificationEventSink,
   public IRatingSpecificationEventSink,
   public ILoadModifiersEventSink,
   public IEnvironmentEventSink,
   public ILossParametersEventSink,
   public ICrackedSection
{
public:
   CEngAgentImp();

   virtual ~CEngAgentImp()
   {
   }

#if defined _USE_MULTITHREADING
   CThreadManager m_ThreadManager;
#endif

   // callback id's
   StatusCallbackIDType m_scidUnknown;
   StatusCallbackIDType m_scidRefinedAnalysis;
   StatusCallbackIDType m_scidBridgeDescriptionError;
   StatusCallbackIDType m_scidLldfWarning;

// IAgent
public:
   std::_tstring GetName() const override { return _T("EngAgent"); }
   bool RegisterInterfaces() override;
   bool Init() override;
   bool Reset() override;
   bool ShutDown() override;
   CLSID GetCLSID() const override;

// ILosses
public:
   const LOSSDETAILS* GetLossDetails(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx) const override;
   CString GetRestrainingLoadName(IntervalIndexType intervalIdx,int loadType) const override;
   Float64 GetElasticShortening(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType) const override;
   void ReportLosses(const CGirderKey& girderKey,rptChapter* pChapter,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) const override;
   void ReportFinalLosses(const CGirderKey& girderKey,rptChapter* pChapter,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) const override;

   Float64 GetElasticShortening(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,const GDRCONFIG& config) const override;
   const LOSSDETAILS* GetLossDetails(const pgsPointOfInterest& poi,const GDRCONFIG& config,IntervalIndexType intervalIdx) const override;
   void ClearDesignLosses() override;

   Float64 GetEffectivePrestressLoss(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime,const GDRCONFIG* pConfig = nullptr) const override;
   Float64 GetEffectivePrestressLossWithLiveLoad(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,pgsTypes::LimitState limitState, VehicleIndexType vehicleIdx, bool bIncludeElasticEffects, bool bApplyElasticGainReduction,const GDRCONFIG* pConfig=nullptr) const override;

   Float64 GetTimeDependentLosses(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime,const GDRCONFIG* pConfig = nullptr) const override;

   Float64 GetInstantaneousEffects(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime,const GDRCONFIG* pConfig = nullptr) const override;
   Float64 GetInstantaneousEffectsWithLiveLoad(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,pgsTypes::LimitState limitState,VehicleIndexType vehicleIdx = INVALID_INDEX,const GDRCONFIG* pConfig = nullptr) const override;

   Float64 GetGirderTendonFrictionLoss(const pgsPointOfInterest& poi, DuctIndexType ductIdx) const override;
   Float64 GetSegmentTendonFrictionLoss(const pgsPointOfInterest& poi, DuctIndexType ductIdx) const override;
   Float64 GetGirderTendonAnchorSetZoneLength(const CGirderKey& girderKey, DuctIndexType ductIdx, pgsTypes::MemberEndType endType) const override;
   Float64 GetSegmentTendonAnchorSetZoneLength(const CSegmentKey& segmentKey, DuctIndexType ductIdx, pgsTypes::MemberEndType endType) const override;
   Float64 GetGirderTendonAnchorSetLoss(const pgsPointOfInterest& poi, DuctIndexType ductIdx) const override;
   Float64 GetSegmentTendonAnchorSetLoss(const pgsPointOfInterest& poi, DuctIndexType ductIdx) const override;
   Float64 GetGirderTendonElongation(const CGirderKey& girderKey, DuctIndexType ductIdx, pgsTypes::MemberEndType endType) const override;
   Float64 GetSegmentTendonElongation(const CSegmentKey& segmentKey, DuctIndexType ductIdx, pgsTypes::MemberEndType endType) const override;
   Float64 GetGirderTendonAverageFrictionLoss(const CGirderKey& girderKey, DuctIndexType ductIdx) const override;
   Float64 GetGirderTendonAverageAnchorSetLoss(const CGirderKey& girderKey, DuctIndexType ductIdx) const override;
   Float64 GetSegmentTendonAverageFrictionLoss(const CSegmentKey& segmentKey, DuctIndexType ductIdx) const override;
   Float64 GetSegmentTendonAverageAnchorSetLoss(const CSegmentKey& segmentKey, DuctIndexType ductIdx) const override;

   bool AreElasticGainsApplicable() const override;
   bool IsDeckShrinkageApplicable() const override;

   bool LossesIncludeInitialRelaxation() const override;


// IPretensionForce
public:
   Float64 GetPjackMax(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType,StrandIndexType nStrands) const override;
   Float64 GetPjackMax(const CSegmentKey& segmentKey,const WBFL::Materials::PsStrand& strand,StrandIndexType nStrands) const override;

   Float64 GetTransferLength(const CSegmentKey& segmentKey, pgsTypes::StrandType strandType, pgsTypes::TransferLengthType xferType, const GDRCONFIG* pConfig = nullptr) const override;
   std::shared_ptr<const pgsTransferLength> GetTransferLengthDetails(const CSegmentKey& segmentKey, pgsTypes::StrandType strandType, pgsTypes::TransferLengthType xferType, const GDRCONFIG* pConfig = nullptr) const override;
   Float64 GetTransferLengthAdjustment(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType, pgsTypes::TransferLengthType xferType,const GDRCONFIG* pConfig=nullptr) const override;
   Float64 GetTransferLengthAdjustment(const pgsPointOfInterest& poi, pgsTypes::StrandType strandType, pgsTypes::TransferLengthType xferType, StrandIndexType strandIdx, const GDRCONFIG* pConfig = nullptr) const override;
   void ReportTransferLengthDetails(const CSegmentKey& segmentKey, pgsTypes::TransferLengthType xferType, rptChapter* pChapter) const override;

   Float64 GetDevelopmentLength(const pgsPointOfInterest& poi, pgsTypes::StrandType strandType, bool bDebonded, const GDRCONFIG* pConfig=nullptr) const override;
   std::shared_ptr<const pgsDevelopmentLength> GetDevelopmentLengthDetails(const pgsPointOfInterest& poi, pgsTypes::StrandType strandType, bool bDebonded,const GDRCONFIG* pConfig=nullptr) const override;
   void ReportDevelopmentLengthDetails(const CSegmentKey& segmentKey, rptChapter* pChapter) const override;
   Float64 GetDevelopmentLengthAdjustment(const pgsPointOfInterest& poi,StrandIndexType strandIdx,pgsTypes::StrandType strandType, bool bDebonded, const GDRCONFIG* pConfig=nullptr) const override;
   Float64 GetDevelopmentLengthAdjustment(const pgsPointOfInterest& poi,StrandIndexType strandIdx,pgsTypes::StrandType strandType,Float64 fps, Float64 fpe,const GDRCONFIG* pConfig=nullptr) const override;

   Float64 GetHoldDownForce(const CSegmentKey& segmentKey,HoldDownCriteria::Type holdDownForceType = HoldDownCriteria::Type::Total,Float64* pSlope=nullptr,pgsPointOfInterest* pPoi=nullptr,const GDRCONFIG* pConfig=nullptr) const override;

   Float64 GetHorizHarpedStrandForce(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime,const GDRCONFIG* pConfig = nullptr) const override;

   Float64 GetVertHarpedStrandForce(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime,const GDRCONFIG* pConfig = nullptr) const override;

   Float64 GetPrestressForcePerStrand(const pgsPointOfInterest& poi, pgsTypes::StrandType strandType, IntervalIndexType intervalIdx, pgsTypes::IntervalTimeType intervalTime, const GDRCONFIG* pConfig = nullptr) const override;
   Float64 GetPrestressForcePerStrand(const pgsPointOfInterest& poi, pgsTypes::StrandType strandType, IntervalIndexType intervalIdx, pgsTypes::IntervalTimeType intervalTime, bool bIncludeElasticEffects, const GDRCONFIG* pConfig = nullptr) const override;

   Float64 GetPrestressForce(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime, pgsTypes::TransferLengthType xferLengthType,const GDRCONFIG* pConfig = nullptr) const override;
   Float64 GetPrestressForce(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime,bool bIncludeElasticEffects, pgsTypes::TransferLengthType xferLengthType, const GDRCONFIG* pConfig = nullptr) const override;
   Float64 GetEffectivePrestress(const pgsPointOfInterest& poi, pgsTypes::StrandType strandType, IntervalIndexType intervalIdx, pgsTypes::IntervalTimeType intervalTime, const GDRCONFIG* pConfig = nullptr) const override;
   Float64 GetEffectivePrestress(const pgsPointOfInterest& poi, pgsTypes::StrandType strandType, IntervalIndexType intervalIdx, pgsTypes::IntervalTimeType intervalTime, bool bIncludeElasticEffects, const GDRCONFIG* pConfig = nullptr) const override;

   Float64 GetPrestressForceWithLiveLoad(const pgsPointOfInterest& poi, pgsTypes::StrandType strandType, pgsTypes::LimitState limitState, VehicleIndexType vehicleIndex = INVALID_INDEX, const GDRCONFIG* pConfig = nullptr) const override;
   Float64 GetPrestressForceWithLiveLoad(const pgsPointOfInterest& poi, pgsTypes::StrandType strandType, pgsTypes::LimitState limitState, bool bIncludeElasticEffects, VehicleIndexType vehicleIndex = INVALID_INDEX, const GDRCONFIG* pConfig = nullptr) const override;
   Float64 GetEffectivePrestressWithLiveLoad(const pgsPointOfInterest& poi, pgsTypes::StrandType strandType, pgsTypes::LimitState limitState, VehicleIndexType vehicleIndex = INVALID_INDEX, const GDRCONFIG* pConfig = nullptr) const override;
   Float64 GetEffectivePrestressWithLiveLoad(const pgsPointOfInterest& poi, pgsTypes::StrandType strandType, pgsTypes::LimitState limitState, bool bIncludeElasticEffects, bool bApplyElasticGainReduction, VehicleIndexType vehicleIndex = INVALID_INDEX, const GDRCONFIG* pConfig = nullptr) const override;

   void GetEccentricityEnvelope(const pgsPointOfInterest& rpoi,const GDRCONFIG& config, Float64* pLowerBound, Float64* pUpperBound) const override;

   // non virtual
   pgsEccEnvelope GetEccentricityEnvelope(const pgsPointOfInterest& rpoi,const GDRCONFIG& config) const;

// IPosttensionForce
public:
   Float64 GetGirderTendonPjackMax(const CGirderKey& girderKey,StrandIndexType nStrands) const override;
   Float64 GetGirderTendonPjackMax(const CGirderKey& girderKey,const WBFL::Materials::PsStrand& strand,StrandIndexType nStrands) const override;
   Float64 GetSegmentTendonPjackMax(const CSegmentKey& segmentKey, StrandIndexType nStrands) const override;
   Float64 GetSegmentTendonPjackMax(const CSegmentKey& segmentKey, const WBFL::Materials::PsStrand& strand, StrandIndexType nStrands) const override;

   Float64 GetGirderTendonInitialForce(const pgsPointOfInterest& poi,DuctIndexType ductIdx,bool bIncludeAnchorSet) const override;
   Float64 GetGirderTendonInitialStress(const pgsPointOfInterest& poi,DuctIndexType ductIdx,bool bIncludeAnchorSet) const override;
   Float64 GetGirderTendonAverageInitialForce(const CGirderKey& girderKey,DuctIndexType ductIdx) const override;
   Float64 GetGirderTendonAverageInitialStress(const CGirderKey& girderKey,DuctIndexType ductIdx) const override;

   Float64 GetSegmentTendonInitialForce(const pgsPointOfInterest& poi, DuctIndexType ductIdx, bool bIncludeAnchorSet) const override;
   Float64 GetSegmentTendonInitialStress(const pgsPointOfInterest& poi, DuctIndexType ductIdx, bool bIncludeAnchorSet) const override;
   Float64 GetSegmentTendonAverageInitialForce(const CSegmentKey& segmentKey, DuctIndexType ductIdx) const override;
   Float64 GetSegmentTendonAverageInitialStress(const CSegmentKey& segmentKey, DuctIndexType ductIdx) const override;

   Float64 GetGirderTendonForce(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType time,DuctIndexType ductIdx,bool bIncludeMinLiveLoad,bool bIncludeMaxLiveLoad, pgsTypes::LimitState limitState, VehicleIndexType vehicleIdx) const override;
   Float64 GetGirderTendonStress(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType time,DuctIndexType ductIdx,bool bIncludeMinLiveLoad,bool bIncludeMaxLiveLoad, pgsTypes::LimitState limitState, VehicleIndexType vehicleIdx) const override;
   Float64 GetGirderTendonVerticalForce(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime,DuctIndexType ductIdx) const override;

   Float64 GetSegmentTendonForce(const pgsPointOfInterest& poi, IntervalIndexType intervalIdx, pgsTypes::IntervalTimeType time, DuctIndexType ductIdx, bool bIncludeMinLiveLoad, bool bIncludeMaxLiveLoad, pgsTypes::LimitState limitState, VehicleIndexType vehicleIdx) const override;
   Float64 GetSegmentTendonStress(const pgsPointOfInterest& poi, IntervalIndexType intervalIdx, pgsTypes::IntervalTimeType time, DuctIndexType ductIdx, bool bIncludeMinLiveLoad, bool bIncludeMaxLiveLoad, pgsTypes::LimitState limitState, VehicleIndexType vehicleIdx) const override;
   Float64 GetSegmentTendonVerticalForce(const pgsPointOfInterest& poi, IntervalIndexType intervalIdx, pgsTypes::IntervalTimeType intervalTime, DuctIndexType ductIdx) const override;


// ILiveLoadDistributionFactors
public:
   Int32 VerifyDistributionFactorRequirements(const pgsPointOfInterest& poi) const override;
   void TestRangeOfApplicability(const CSpanKey& spanKey) const override;
   Float64 GetMomentDistFactor(const CSpanKey& spanKey,pgsTypes::LimitState limitState, const GDRCONFIG* pConfig = nullptr) const override;
   Float64 GetNegMomentDistFactor(const CSpanKey& spanKey,pgsTypes::LimitState limitState, const GDRCONFIG* pConfig = nullptr) const override;
   Float64 GetNegMomentDistFactorAtPier(PierIndexType pierIdx,GirderIndexType gdrIdx,pgsTypes::LimitState limitState,pgsTypes::PierFaceType pierFace, const GDRCONFIG* pConfig = nullptr) const override;
   Float64 GetShearDistFactor(const CSpanKey& spanKey,pgsTypes::LimitState limitState, const GDRCONFIG* pConfig = nullptr) const override;
   Float64 GetSkewCorrectionFactorForMoment(const CSpanKey& spanKey,pgsTypes::LimitState ls) const override;
   Float64 GetSkewCorrectionFactorForShear(const CSpanKey& spanKey,pgsTypes::LimitState ls) const override;
   void GetNegMomentDistFactorPoints(const CSpanKey& spanKey,Float64* dfPoints,IndexType* nPoints) const override;
   void GetDistributionFactors(const pgsPointOfInterest& poi,pgsTypes::LimitState limitState,Float64* pM,Float64* nM,Float64* V,const GDRCONFIG* pConfig = nullptr) const override;
   void ReportDistributionFactors(const CGirderKey& girderKey,rptChapter* pChapter,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) const override;
   void ReportReactionDistributionFactors(const CGirderKey& girderKey, rptChapter* pChapter, bool bSubHeading) const override;
   bool Run1250Tests(const CSpanKey& spanKey,pgsTypes::LimitState limitState,LPCTSTR pid,LPCTSTR bridgeId,std::_tofstream& resultsFile, std::_tofstream& poiFile) const override;
   Uint32 GetNumberOfDesignLanes(SpanIndexType spanIdx) const override;
   Uint32 GetNumberOfDesignLanesEx(SpanIndexType spanIdx,Float64* pDistToSection,Float64* pCurbToCurb) const override;
   bool GetDFResultsEx(const CSpanKey& spanKey,pgsTypes::LimitState limitState,
               Float64* gpM, Float64* gpM1, Float64* gpM2,  // pos moment
               Float64* gnM, Float64* gnM1, Float64* gnM2,  // neg moment, ahead face
               Float64* gV,  Float64* gV1,  Float64* gV2) const;   // shear
   Float64 GetDeflectionDistFactor(const CSpanKey& spanKey) const override;
   Float64 GetDeflectionDistFactorEx(const CSpanKey& spanKey, Float64* pMPF, Uint32* pnLanes, GirderIndexType* pnGirders) const override;

// IMomentCapacity
public:
   Float64 GetMomentCapacity(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,bool bPositiveMoment) const override;
   std::vector<Float64> GetMomentCapacity(IntervalIndexType intervalIdx,const PoiList& vPoi,bool bPositiveMoment) const override;
   const MOMENTCAPACITYDETAILS* GetMomentCapacityDetails(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, bool bPositiveMoment, const GDRCONFIG* pConfig = nullptr) const override;
   std::vector<const MOMENTCAPACITYDETAILS*> GetMomentCapacityDetails(IntervalIndexType intervalIdx, const PoiList& vPoi, bool bPositiveMoment, const GDRCONFIG* pConfig = nullptr) const override;
   Float64 GetCrackingMoment(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,bool bPositiveMoment) const override;
   const CRACKINGMOMENTDETAILS* GetCrackingMomentDetails(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,bool bPositiveMoment) const override;
   void GetCrackingMomentDetails(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,const GDRCONFIG& config,bool bPositiveMoment,CRACKINGMOMENTDETAILS* pcmd) const override;
   Float64 GetMinMomentCapacity(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,bool bPositiveMoment) const override;
   const MINMOMENTCAPDETAILS* GetMinMomentCapacityDetails(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,bool bPositiveMoment) const override;
   void GetMinMomentCapacityDetails(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,const GDRCONFIG& config,bool bPositiveMoment,MINMOMENTCAPDETAILS* pmmcd) const override;
   std::vector<const MINMOMENTCAPDETAILS*> GetMinMomentCapacityDetails(IntervalIndexType intervalIdx,const PoiList& vPoi,bool bPositiveMoment) const override;
   std::vector<const CRACKINGMOMENTDETAILS*> GetCrackingMomentDetails(IntervalIndexType intervalIdx,const PoiList& vPoi,bool bPositiveMoment) const override;
   std::vector<Float64> GetCrackingMoment(IntervalIndexType intervalIdx,const PoiList& vPoi,bool bPositiveMoment) const override;
   std::vector<Float64> GetMinMomentCapacity(IntervalIndexType intervalIdx,const PoiList& vPoi,bool bPositiveMoment) const override;

// IShearCapacity
public:
   pgsTypes::FaceType GetFlexuralTensionSide(pgsTypes::LimitState limitState,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) const override;
   Float64 GetShearCapacity(pgsTypes::LimitState limitState, IntervalIndexType intervalIdx,const pgsPointOfInterest& poi, const GDRCONFIG* pConfig = nullptr) const override;
   std::vector<Float64> GetShearCapacity(pgsTypes::LimitState limitState, IntervalIndexType intervalIdx,const PoiList& vPoi) const override;
   void GetShearCapacityDetails(pgsTypes::LimitState limitState, IntervalIndexType intervalIdx,const pgsPointOfInterest& poi, const GDRCONFIG* pConfig,SHEARCAPACITYDETAILS* pmcd) const override;
   void GetRawShearCapacityDetails(pgsTypes::LimitState limitState, IntervalIndexType intervalIdx,const pgsPointOfInterest& poi, const GDRCONFIG* pConfig,SHEARCAPACITYDETAILS* pmcd) const override;
   Float64 GetFpc(const pgsPointOfInterest& poi, const GDRCONFIG* pConfig = nullptr) const override;
   void GetFpcDetails(const pgsPointOfInterest& poi, const GDRCONFIG* pConfig, FPCDETAILS* pmcd) const override;


   ZoneIndexType GetCriticalSectionZoneIndex(pgsTypes::LimitState limitState,const pgsPointOfInterest& poi) const override;
   void GetCriticalSectionZoneBoundary(pgsTypes::LimitState limitState,const CGirderKey& girderKeyegmentKey,ZoneIndexType csZoneIdx,Float64* pStart,Float64* pEnd) const override;

   std::vector<Float64> GetCriticalSections(pgsTypes::LimitState limitState,const CGirderKey& girderKey, const GDRCONFIG* pConfig = nullptr) const override;
   const std::vector<CRITSECTDETAILS>& GetCriticalSectionDetails(pgsTypes::LimitState limitState,const CGirderKey& girderKey,const GDRCONFIG* pConfig) const override;

   std::vector<SHEARCAPACITYDETAILS> GetShearCapacityDetails(pgsTypes::LimitState limitState, IntervalIndexType intervalIdx,const PoiList& vPoi) const override;
   void ClearDesignCriticalSections() const override;

// IPrincipalWebStress
public:
   void GetPrincipalWebStressPointsOfInterest(const CSegmentKey& segmentKey, IntervalIndexType interval, PoiList* pPoiList) const override;
   const PRINCIPALSTRESSINWEBDETAILS* GetPrincipalWebStressDetails(const pgsPointOfInterest& poi) const override;
   const std::vector<TimeStepCombinedPrincipalWebStressDetailsAtWebSection>* GetTimeStepPrincipalWebStressDetails(const pgsPointOfInterest& poi, IntervalIndexType interval) const override;

// IGirderHaunch
public:
   Float64 GetRequiredSlabOffset(const CSegmentKey& segmentKey) const override;
   const SLABOFFSETDETAILS& GetSlabOffsetDetails(const CSegmentKey& segmentKey) const override;
   Float64 GetSectionGirderOrientationEffect(const pgsPointOfInterest& poi) const override;


// IBearingDesignParameters
 public:
   void GetBearingParameters(CGirderKey girderKey, BEARINGPARAMETERS* pDetails) const override;

   void GetBearingDesignProperties(DESIGNPROPERTIES* pDetails) const override;

   void GetBearingRotationDetails(pgsTypes::AnalysisType analysisType, const pgsPointOfInterest& poi, const ReactionLocation& reactionLocation, 
       CGirderKey girderKey, bool bIncludeImpact, bool bIncludeLLDF, bool isFlexural, ROTATIONDETAILS* pDetails) const override;

   void GetBearingReactionDetails(const ReactionLocation& reactionLocation,
       CGirderKey girderKey, pgsTypes::AnalysisType analysisType, bool bIncludeImpact, bool bIncludeLLDF, REACTIONDETAILS* pDetails) const override;

   void GetThermalExpansionDetails(BEARINGSHEARDEFORMATIONDETAILS* bearing) const override;

   Float64 GetDistanceToPointOfFixity(const pgsPointOfInterest& poi, SHEARDEFORMATIONDETAILS* pDetails) const override;

   void GetTimeDependentShearDeformation(CGirderKey girderKey, SHEARDEFORMATIONDETAILS* pDetails) const override;

   PoiList GetBearingPoiList(const CGirderKey girderKey, SHEARDEFORMATIONDETAILS* pDetails) const override;

private:
    void GetLongitudinalPointOfFixity(const CGirderKey& girderKey, BEARINGPARAMETERS* pDetails) const;

    std::array<Float64, 2> GetTimeDependentComponentShearDeformation(Float64 loss, BEARINGSHEARDEFORMATIONDETAILS* bearing) const;

    void SetBearingDesignData(const CBearingData2& brgData, const ReactionLocation& reactionLocation, bool bFlexural,
        WBFL::EngTools::Bearing* pBearing, WBFL::EngTools::BearingLoads* pLoads) override;

// IFabricationOptimization
public:
   bool GetFabricationOptimizationDetails(const CSegmentKey& segmentKey,FABRICATIONOPTIMIZATIONDETAILS* pDetails) const override;

// IArtifact
public:
   const pgsGirderArtifact* GetGirderArtifact(const CGirderKey& girderKey) const override;
   const pgsSegmentArtifact* GetSegmentArtifact(const CSegmentKey& segmentKey) const override;
   std::shared_ptr<const WBFL::Stability::LiftingCheckArtifact> GetLiftingCheckArtifact(const CSegmentKey& segmentKey) const override;
   std::shared_ptr<const pgsHaulingAnalysisArtifact> GetHaulingAnalysisArtifact(const CSegmentKey& segmentKey) const override;
   const pgsGirderDesignArtifact* CreateDesignArtifact(const CGirderKey& girderKey, bool bDesignFlexure, arSlabOffsetDesignType haunchDesignType, arConcreteDesignType concreteDesignType, arShearDesignType shearDesignType) const override;
   const pgsGirderDesignArtifact* GetDesignArtifact(const CGirderKey& girderKey) const override;
   std::shared_ptr<WBFL::Stability::LiftingCheckArtifact> CreateLiftingCheckArtifact(const CSegmentKey& segmentKey,Float64 supportLoc) const override;
   std::shared_ptr<pgsHaulingAnalysisArtifact> CreateHaulingAnalysisArtifact(const CSegmentKey& segmentKey,Float64 leftSupportLoc,Float64 rightSupportLoc) const override;
   const pgsRatingArtifact* GetRatingArtifact(const CGirderKey& girderKey,pgsTypes::LoadRatingType ratingType,VehicleIndexType vehicleIdx) const override;
   std::shared_ptr<const pgsISummaryRatingArtifact> GetSummaryRatingArtifact(const std::vector<CGirderKey>& girderKeys,pgsTypes::LoadRatingType ratingType,VehicleIndexType vehicleIdx) const override;

// ICrackedSection
public:
   Float64 GetIcr(const pgsPointOfInterest& poi,bool bPositiveMoment) const override;
   const CRACKEDSECTIONDETAILS* GetCrackedSectionDetails(const pgsPointOfInterest& poi, bool bPositiveMoment) const override;
   std::vector<const CRACKEDSECTIONDETAILS*> GetCrackedSectionDetails(const PoiList& vPoi,bool bPositiveMoment) const override;

// IBridgeDescriptionEventSink
public:
   HRESULT OnBridgeChanged(CBridgeChangedHint* pHint) override;
   HRESULT OnGirderFamilyChanged() override;
   HRESULT OnGirderChanged(const CGirderKey& girderKey,Uint32 lHint) override;
   HRESULT OnLiveLoadChanged() override;
   HRESULT OnLiveLoadNameChanged(LPCTSTR strOldName,LPCTSTR strNewName) override;
   HRESULT OnConstructionLoadChanged() override;

// ISpecificationEventSink
public:
   HRESULT OnSpecificationChanged() override;
   HRESULT OnAnalysisTypeChanged() override;

// IRatingSpecificationEventSink
public:
   HRESULT OnRatingSpecificationChanged() override;

// ILoadModifiersEventSink
public:
   HRESULT OnLoadModifiersChanged() override;

// IEnvironmentEventSink
public:
   HRESULT OnExposureConditionChanged() override;
   HRESULT OnClimateConditionChanged() override;
   HRESULT OnRelHumidityChanged() override;

// ILossParametersEventSink
public:
   HRESULT OnLossParametersChanged() override;

private:
   EAF_DECLARE_AGENT_DATA;

   mutable std::map<CGirderKey,pgsGirderDesignArtifact> m_DesignArtifacts;

   struct RatingArtifactKey
   {
      RatingArtifactKey(const CGirderKey& girderKey,VehicleIndexType vehicleIdx)
      { GirderKey = girderKey; VehicleIdx = vehicleIdx; }

      CGirderKey GirderKey;
      VehicleIndexType VehicleIdx;

      bool operator<(const RatingArtifactKey& other) const
      {
         if( GirderKey < other.GirderKey )
            return true;

         if( other.GirderKey < GirderKey)
            return false;

         if( VehicleIdx < other.VehicleIdx )
            return true;

         return false;
      }

   };
   mutable std::array<std::map<RatingArtifactKey, pgsRatingArtifact>, pgsTypes::lrLoadRatingTypeCount> m_RatingArtifacts; // pgsTypes::LoadRatingType enum as key

   mutable std::map<PrestressPoiKey,Float64> m_PsForce; // cache of prestress forces
   mutable std::map<PrestressWithLiveLoadPoiKey,Float64> m_PsForceWithLiveLoad; // cache of prestress forces including live load

   std::unique_ptr<pgsTransferLengthEngineer> m_TransferLengthEngineer;
   std::unique_ptr<pgsDevelopmentLengthEngineer> m_DevelopmentLengthEngineer;
   std::unique_ptr<pgsPsForceEng> m_PsForceEngineer;
   std::unique_ptr<pgsDesigner2>  m_Designer;
   std::unique_ptr<pgsLoadRater>  m_LoadRater;
   std::unique_ptr<pgsShearCapacityEngineer> m_ShearCapEngineer;
   std::unique_ptr<pgsBearingDesignEngineer> m_BearingEngineer;
   
   mutable bool m_bAreDistFactorEngineersValidated;
   mutable std::unique_ptr<PGS::Beams::DistFactorEngineer> m_pDistFactorEngineer; // assigned a polymorphic object during validation (must be mutable for delayed assignment)

   static UINT DeleteMomentCapacityEngineer(LPVOID pParam);
   std::unique_ptr<pgsMomentCapacityEngineer> m_pMomentCapacityEngineer;

   static UINT DeletePrincipalWebStressEngineer(LPVOID pParam);
   std::unique_ptr<pgsPrincipalWebStressEngineer> m_pPrincipalWebStressEngineer;


   // Shear Capacity
   using ShearCapacityContainer = std::map<PoiIDType, SHEARCAPACITYDETAILS>;
   mutable std::array<ShearCapacityContainer, 9> m_ShearCapacity; // use the LimitStateToShearIndex method to map limit state to array index
   const SHEARCAPACITYDETAILS* ValidateShearCapacity(pgsTypes::LimitState limitState, IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) const;
   void InvalidateShearCapacity();
   mutable std::map<PoiIDType,FPCDETAILS> m_Fpc;
   const FPCDETAILS* ValidateFpc(const pgsPointOfInterest& poi) const;
   void InvalidateFpc();

   std::vector<Float64> GetCriticalSectionFromDetails(const std::vector<CRITSECTDETAILS>& csDetails) const;

   // critical section for shear (a segment can have N critical sections)
   // critical sections are listed left to right along a segment
   mutable std::array<std::map<CGirderKey,std::vector<CRITSECTDETAILS>>, 9> m_CritSectionDetails; // use the LimitStateToShearIndex method to map limit state to array index
   mutable std::array<std::map<CGirderKey,std::vector<CRITSECTDETAILS>>, 9> m_DesignCritSectionDetails; // use the LimitStateToShearIndex method to map limit state to array index
   const std::vector<CRITSECTDETAILS>& ValidateShearCritSection(pgsTypes::LimitState limitState,const CGirderKey& girderKey, const GDRCONFIG* pConfig = nullptr) const;
   std::vector<CRITSECTDETAILS> CalculateShearCritSection(pgsTypes::LimitState limitState,const CGirderKey& girderKey, const GDRCONFIG* pConfig = nullptr) const;
   void InvalidateShearCritSection();

   mutable std::map<CSegmentKey,SLABOFFSETDETAILS> m_SlabOffsetDetails;

   //ROTATIONDETAILS m_staticRotationDetails;

   // Lifting and hauling analysis artifact cache for ad-hoc analysis (typically during design)
   mutable std::map<CSegmentKey, std::map<Float64,std::shared_ptr<WBFL::Stability::LiftingCheckArtifact>,Float64_less> > m_LiftingArtifacts;
   mutable std::map<CSegmentKey, std::map<Float64,std::shared_ptr<pgsHaulingAnalysisArtifact>,Float64_less> > m_HaulingArtifacts;

   // Event Sink Cookies
   IDType m_BridgeDescCookie;
   IDType m_SpecificationCookie;
   IDType m_RatingSpecificationCookie;
   IDType m_LoadModifiersCookie;
   IDType m_EnvironmentCookie;
   IDType m_LossParametersCookie;

   void InvalidateAll();
   void InvalidateHaunch();
   void InvalidateLosses();
   void ValidateLiveLoadDistributionFactors(const CGirderKey& girderKey) const;
   void InvalidateLiveLoadDistributionFactors();
   void ValidateArtifacts(const CGirderKey& girderKey) const;
   void ValidateRatingArtifacts(const CGirderKey& girderKey,pgsTypes::LoadRatingType ratingType,VehicleIndexType vehicleIdx) const;
   void InvalidateArtifacts();
   void InvalidateRatingArtifacts();

   const LOSSDETAILS* FindLosses(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx) const;
   const pgsRatingArtifact* FindRatingArtifact(const CGirderKey& girderKey, pgsTypes::LoadRatingType ratingType, VehicleIndexType vehicleIdx) const;

   DECLARE_LOGFILE;

   Int32 CheckCurvatureRequirements(const pgsPointOfInterest& poi) const;
   Int32 CheckGirderStiffnessRequirements(const pgsPointOfInterest& poi) const;
   Int32 CheckParallelGirderRequirements(const pgsPointOfInterest& poi) const;

   //Float64 GetPrestressForcePerStrand(const pgsPointOfInterest& poi, pgsTypes::StrandType strandType, IntervalIndexType intervalIdx, pgsTypes::IntervalTimeType intervalTime, bool bIncludeElasticEffects, const GDRCONFIG* pConfig) const;
};
