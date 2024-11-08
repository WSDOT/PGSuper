///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2024  Washington State Department of Transportation
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

// EngAgentImp.h : Declaration of the CEngAgentImp

#ifndef __ENGAGENT_H_
#define __ENGAGENT_H_

#include "CLSID.h"
#include "resource.h"       // main symbols
#include "PsForceEng.h"
#include "Designer2.h"
#include "LoadRater.h"
#include "MomentCapacityEngineer.h"
#include "ShearCapacityEngineer.h"
#include "TransferLengthEngineer.h"
#include "DevelopmentLengthEngineer.h"
#include "BearingDesignEngineer.h"
#include "PrincipalWebStressEngineer.h"
#include <IFace\DistFactorEngineer.h>
#include <IFace\RatingSpecification.h>
#include <IFace\CrackedSection.h>
#include <IFace\PrincipalWebStress.h>
#include <EAF\EAFInterfaceCache.h>

#include <PgsExt\PoiKey.h>
#include <PgsExt\Keys.h>

#include <map>

#if defined _USE_MULTITHREADING
#include <PgsExt\ThreadManager.h>
#endif
#include <Reporting/ReactionInterfaceAdapters.h>

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
class ATL_NO_VTABLE CEngAgentImp : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CEngAgentImp, &CLSID_EngAgent>,
	public IConnectionPointContainerImpl<CEngAgentImp>,
	public IAgentEx,
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

DECLARE_REGISTRY_RESOURCEID(IDR_ENGAGENT)
DECLARE_NOT_AGGREGATABLE(CEngAgentImp)

BEGIN_COM_MAP(CEngAgentImp)
   COM_INTERFACE_ENTRY(IAgent)
   COM_INTERFACE_ENTRY(IAgentEx)
   COM_INTERFACE_ENTRY(ILosses)
   COM_INTERFACE_ENTRY(IPretensionForce)
   COM_INTERFACE_ENTRY(IPosttensionForce)
   COM_INTERFACE_ENTRY(ILiveLoadDistributionFactors)
   COM_INTERFACE_ENTRY(IMomentCapacity)
   COM_INTERFACE_ENTRY(IShearCapacity)
   COM_INTERFACE_ENTRY(IPrincipalWebStress)
   COM_INTERFACE_ENTRY(IGirderHaunch)
   COM_INTERFACE_ENTRY(IBearingDesignParameters)
   COM_INTERFACE_ENTRY(IFabricationOptimization)
   COM_INTERFACE_ENTRY(IArtifact)
   COM_INTERFACE_ENTRY(IBridgeDescriptionEventSink)
   COM_INTERFACE_ENTRY(ISpecificationEventSink)
   COM_INTERFACE_ENTRY(IRatingSpecificationEventSink)
   COM_INTERFACE_ENTRY(ILoadModifiersEventSink)
   COM_INTERFACE_ENTRY(IEnvironmentEventSink)
   COM_INTERFACE_ENTRY(ILossParametersEventSink)
   COM_INTERFACE_ENTRY(ICrackedSection)
   COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
END_COM_MAP()

BEGIN_CONNECTION_POINT_MAP(CEngAgentImp)
END_CONNECTION_POINT_MAP()

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
	STDMETHOD(SetBroker)(IBroker* pBroker) override;
	STDMETHOD(RegInterfaces)() override;
	STDMETHOD(Init)() override;
	STDMETHOD(Reset)() override;
	STDMETHOD(ShutDown)() override;
   STDMETHOD(Init2)() override;
   STDMETHOD(GetClassID)(CLSID* pCLSID) override;

// ILosses
public:
   virtual const LOSSDETAILS* GetLossDetails(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx) const override;
   virtual CString GetRestrainingLoadName(IntervalIndexType intervalIdx,int loadType) const override;
	virtual Float64 GetElasticShortening(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType) const override;
   virtual void ReportLosses(const CGirderKey& girderKey,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits) const override;
   virtual void ReportFinalLosses(const CGirderKey& girderKey,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits) const override;

   virtual Float64 GetElasticShortening(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,const GDRCONFIG& config) const override;
   virtual const LOSSDETAILS* GetLossDetails(const pgsPointOfInterest& poi,const GDRCONFIG& config,IntervalIndexType intervalIdx) const override;
   virtual void ClearDesignLosses() override;

   virtual Float64 GetEffectivePrestressLoss(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime,const GDRCONFIG* pConfig = nullptr) const override;
   virtual Float64 GetEffectivePrestressLossWithLiveLoad(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,pgsTypes::LimitState limitState, VehicleIndexType vehicleIdx, bool bIncludeElasticEffects, bool bApplyElasticGainReduction,const GDRCONFIG* pConfig=nullptr) const override;

   virtual Float64 GetTimeDependentLosses(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime,const GDRCONFIG* pConfig = nullptr) const override;

   virtual Float64 GetInstantaneousEffects(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime,const GDRCONFIG* pConfig = nullptr) const override;
   virtual Float64 GetInstantaneousEffectsWithLiveLoad(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,pgsTypes::LimitState limitState,VehicleIndexType vehicleIdx = INVALID_INDEX,const GDRCONFIG* pConfig = nullptr) const override;

   virtual Float64 GetGirderTendonFrictionLoss(const pgsPointOfInterest& poi, DuctIndexType ductIdx) const override;
   virtual Float64 GetSegmentTendonFrictionLoss(const pgsPointOfInterest& poi, DuctIndexType ductIdx) const override;
   virtual Float64 GetGirderTendonAnchorSetZoneLength(const CGirderKey& girderKey, DuctIndexType ductIdx, pgsTypes::MemberEndType endType) const override;
   virtual Float64 GetSegmentTendonAnchorSetZoneLength(const CSegmentKey& segmentKey, DuctIndexType ductIdx, pgsTypes::MemberEndType endType) const override;
   virtual Float64 GetGirderTendonAnchorSetLoss(const pgsPointOfInterest& poi, DuctIndexType ductIdx) const override;
   virtual Float64 GetSegmentTendonAnchorSetLoss(const pgsPointOfInterest& poi, DuctIndexType ductIdx) const override;
   virtual Float64 GetGirderTendonElongation(const CGirderKey& girderKey, DuctIndexType ductIdx, pgsTypes::MemberEndType endType) const override;
   virtual Float64 GetSegmentTendonElongation(const CSegmentKey& segmentKey, DuctIndexType ductIdx, pgsTypes::MemberEndType endType) const override;
   virtual Float64 GetGirderTendonAverageFrictionLoss(const CGirderKey& girderKey, DuctIndexType ductIdx) const override;
   virtual Float64 GetGirderTendonAverageAnchorSetLoss(const CGirderKey& girderKey, DuctIndexType ductIdx) const override;
   virtual Float64 GetSegmentTendonAverageFrictionLoss(const CSegmentKey& segmentKey, DuctIndexType ductIdx) const override;
   virtual Float64 GetSegmentTendonAverageAnchorSetLoss(const CSegmentKey& segmentKey, DuctIndexType ductIdx) const override;

   virtual bool AreElasticGainsApplicable() const override;
   virtual bool IsDeckShrinkageApplicable() const override;

   virtual bool LossesIncludeInitialRelaxation() const override;


// IPretensionForce
public:
   virtual Float64 GetPjackMax(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType,StrandIndexType nStrands) const override;
   virtual Float64 GetPjackMax(const CSegmentKey& segmentKey,const WBFL::Materials::PsStrand& strand,StrandIndexType nStrands) const override;

   virtual Float64 GetTransferLength(const CSegmentKey& segmentKey, pgsTypes::StrandType strandType, pgsTypes::TransferLengthType xferType, const GDRCONFIG* pConfig = nullptr) const override;
   virtual const std::shared_ptr<pgsTransferLength> GetTransferLengthDetails(const CSegmentKey& segmentKey, pgsTypes::StrandType strandType, pgsTypes::TransferLengthType xferType, const GDRCONFIG* pConfig = nullptr) const override;
   virtual Float64 GetTransferLengthAdjustment(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType, pgsTypes::TransferLengthType xferType,const GDRCONFIG* pConfig=nullptr) const override;
   virtual Float64 GetTransferLengthAdjustment(const pgsPointOfInterest& poi, pgsTypes::StrandType strandType, pgsTypes::TransferLengthType xferType, StrandIndexType strandIdx, const GDRCONFIG* pConfig = nullptr) const override;
   virtual void ReportTransferLengthDetails(const CSegmentKey& segmentKey, pgsTypes::TransferLengthType xferType, rptChapter* pChapter) const override;

   virtual Float64 GetDevelopmentLength(const pgsPointOfInterest& poi, pgsTypes::StrandType strandType, bool bDebonded, const GDRCONFIG* pConfig=nullptr) const override;
   virtual const std::shared_ptr<pgsDevelopmentLength> GetDevelopmentLengthDetails(const pgsPointOfInterest& poi, pgsTypes::StrandType strandType, bool bDebonded,const GDRCONFIG* pConfig=nullptr) const override;
   virtual void ReportDevelopmentLengthDetails(const CSegmentKey& segmentKey, rptChapter* pChapter) const override;
   virtual Float64 GetDevelopmentLengthAdjustment(const pgsPointOfInterest& poi,StrandIndexType strandIdx,pgsTypes::StrandType strandType, bool bDebonded, const GDRCONFIG* pConfig=nullptr) const override;
   virtual Float64 GetDevelopmentLengthAdjustment(const pgsPointOfInterest& poi,StrandIndexType strandIdx,pgsTypes::StrandType strandType,Float64 fps, Float64 fpe,const GDRCONFIG* pConfig=nullptr) const override;

   virtual Float64 GetHoldDownForce(const CSegmentKey& segmentKey,HoldDownCriteria::Type holdDownForceType = HoldDownCriteria::Type::Total,Float64* pSlope=nullptr,pgsPointOfInterest* pPoi=nullptr,const GDRCONFIG* pConfig=nullptr) const override;

   virtual Float64 GetHorizHarpedStrandForce(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime,const GDRCONFIG* pConfig = nullptr) const override;

   virtual Float64 GetVertHarpedStrandForce(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime,const GDRCONFIG* pConfig = nullptr) const override;

   virtual Float64 GetPrestressForcePerStrand(const pgsPointOfInterest& poi, pgsTypes::StrandType strandType, IntervalIndexType intervalIdx, pgsTypes::IntervalTimeType intervalTime, const GDRCONFIG* pConfig = nullptr) const override;
   virtual Float64 GetPrestressForcePerStrand(const pgsPointOfInterest& poi, pgsTypes::StrandType strandType, IntervalIndexType intervalIdx, pgsTypes::IntervalTimeType intervalTime, bool bIncludeElasticEffects, const GDRCONFIG* pConfig = nullptr) const override;

   virtual Float64 GetPrestressForce(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime, pgsTypes::TransferLengthType xferLengthType,const GDRCONFIG* pConfig = nullptr) const override;
   virtual Float64 GetPrestressForce(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime,bool bIncludeElasticEffects, pgsTypes::TransferLengthType xferLengthType, const GDRCONFIG* pConfig = nullptr) const override;
   virtual Float64 GetEffectivePrestress(const pgsPointOfInterest& poi, pgsTypes::StrandType strandType, IntervalIndexType intervalIdx, pgsTypes::IntervalTimeType intervalTime, const GDRCONFIG* pConfig = nullptr) const override;
   virtual Float64 GetEffectivePrestress(const pgsPointOfInterest& poi, pgsTypes::StrandType strandType, IntervalIndexType intervalIdx, pgsTypes::IntervalTimeType intervalTime, bool bIncludeElasticEffects, const GDRCONFIG* pConfig = nullptr) const override;

   virtual Float64 GetPrestressForceWithLiveLoad(const pgsPointOfInterest& poi, pgsTypes::StrandType strandType, pgsTypes::LimitState limitState, VehicleIndexType vehicleIndex = INVALID_INDEX, const GDRCONFIG* pConfig = nullptr) const override;
   virtual Float64 GetPrestressForceWithLiveLoad(const pgsPointOfInterest& poi, pgsTypes::StrandType strandType, pgsTypes::LimitState limitState, bool bIncludeElasticEffects, VehicleIndexType vehicleIndex = INVALID_INDEX, const GDRCONFIG* pConfig = nullptr) const override;
   virtual Float64 GetEffectivePrestressWithLiveLoad(const pgsPointOfInterest& poi, pgsTypes::StrandType strandType, pgsTypes::LimitState limitState, VehicleIndexType vehicleIndex = INVALID_INDEX, const GDRCONFIG* pConfig = nullptr) const override;
   virtual Float64 GetEffectivePrestressWithLiveLoad(const pgsPointOfInterest& poi, pgsTypes::StrandType strandType, pgsTypes::LimitState limitState, bool bIncludeElasticEffects, bool bApplyElasticGainReduction, VehicleIndexType vehicleIndex = INVALID_INDEX, const GDRCONFIG* pConfig = nullptr) const override;

   virtual void GetEccentricityEnvelope(const pgsPointOfInterest& rpoi,const GDRCONFIG& config, Float64* pLowerBound, Float64* pUpperBound) const override;

   // non virtual
   pgsEccEnvelope GetEccentricityEnvelope(const pgsPointOfInterest& rpoi,const GDRCONFIG& config) const;

// IPosttensionForce
public:
   virtual Float64 GetGirderTendonPjackMax(const CGirderKey& girderKey,StrandIndexType nStrands) const override;
   virtual Float64 GetGirderTendonPjackMax(const CGirderKey& girderKey,const WBFL::Materials::PsStrand& strand,StrandIndexType nStrands) const override;
   virtual Float64 GetSegmentTendonPjackMax(const CSegmentKey& segmentKey, StrandIndexType nStrands) const override;
   virtual Float64 GetSegmentTendonPjackMax(const CSegmentKey& segmentKey, const WBFL::Materials::PsStrand& strand, StrandIndexType nStrands) const override;

   virtual Float64 GetGirderTendonInitialForce(const pgsPointOfInterest& poi,DuctIndexType ductIdx,bool bIncludeAnchorSet) const override;
   virtual Float64 GetGirderTendonInitialStress(const pgsPointOfInterest& poi,DuctIndexType ductIdx,bool bIncludeAnchorSet) const override;
   virtual Float64 GetGirderTendonAverageInitialForce(const CGirderKey& girderKey,DuctIndexType ductIdx) const override;
   virtual Float64 GetGirderTendonAverageInitialStress(const CGirderKey& girderKey,DuctIndexType ductIdx) const override;

   virtual Float64 GetSegmentTendonInitialForce(const pgsPointOfInterest& poi, DuctIndexType ductIdx, bool bIncludeAnchorSet) const override;
   virtual Float64 GetSegmentTendonInitialStress(const pgsPointOfInterest& poi, DuctIndexType ductIdx, bool bIncludeAnchorSet) const override;
   virtual Float64 GetSegmentTendonAverageInitialForce(const CSegmentKey& segmentKey, DuctIndexType ductIdx) const override;
   virtual Float64 GetSegmentTendonAverageInitialStress(const CSegmentKey& segmentKey, DuctIndexType ductIdx) const override;

   virtual Float64 GetGirderTendonForce(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType time,DuctIndexType ductIdx,bool bIncludeMinLiveLoad,bool bIncludeMaxLiveLoad, pgsTypes::LimitState limitState, VehicleIndexType vehicleIdx) const override;
   virtual Float64 GetGirderTendonStress(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType time,DuctIndexType ductIdx,bool bIncludeMinLiveLoad,bool bIncludeMaxLiveLoad, pgsTypes::LimitState limitState, VehicleIndexType vehicleIdx) const override;
   virtual Float64 GetGirderTendonVerticalForce(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime,DuctIndexType ductIdx) const override;

   virtual Float64 GetSegmentTendonForce(const pgsPointOfInterest& poi, IntervalIndexType intervalIdx, pgsTypes::IntervalTimeType time, DuctIndexType ductIdx, bool bIncludeMinLiveLoad, bool bIncludeMaxLiveLoad, pgsTypes::LimitState limitState, VehicleIndexType vehicleIdx) const override;
   virtual Float64 GetSegmentTendonStress(const pgsPointOfInterest& poi, IntervalIndexType intervalIdx, pgsTypes::IntervalTimeType time, DuctIndexType ductIdx, bool bIncludeMinLiveLoad, bool bIncludeMaxLiveLoad, pgsTypes::LimitState limitState, VehicleIndexType vehicleIdx) const override;
   virtual Float64 GetSegmentTendonVerticalForce(const pgsPointOfInterest& poi, IntervalIndexType intervalIdx, pgsTypes::IntervalTimeType intervalTime, DuctIndexType ductIdx) const override;


// ILiveLoadDistributionFactors
public:
   virtual Int32 VerifyDistributionFactorRequirements(const pgsPointOfInterest& poi) const override;
   virtual void TestRangeOfApplicability(const CSpanKey& spanKey) const override;
   virtual Float64 GetMomentDistFactor(const CSpanKey& spanKey,pgsTypes::LimitState limitState, const GDRCONFIG* pConfig = nullptr) const override;
   virtual Float64 GetNegMomentDistFactor(const CSpanKey& spanKey,pgsTypes::LimitState limitState, const GDRCONFIG* pConfig = nullptr) const override;
   virtual Float64 GetNegMomentDistFactorAtPier(PierIndexType pierIdx,GirderIndexType gdrIdx,pgsTypes::LimitState limitState,pgsTypes::PierFaceType pierFace, const GDRCONFIG* pConfig = nullptr) const override;
   virtual Float64 GetShearDistFactor(const CSpanKey& spanKey,pgsTypes::LimitState limitState, const GDRCONFIG* pConfig = nullptr) const override;
   virtual Float64 GetSkewCorrectionFactorForMoment(const CSpanKey& spanKey,pgsTypes::LimitState ls) const override;
   virtual Float64 GetSkewCorrectionFactorForShear(const CSpanKey& spanKey,pgsTypes::LimitState ls) const override;
   virtual void GetNegMomentDistFactorPoints(const CSpanKey& spanKey,Float64* dfPoints,IndexType* nPoints) const override;
   virtual void GetDistributionFactors(const pgsPointOfInterest& poi,pgsTypes::LimitState limitState,Float64* pM,Float64* nM,Float64* V,const GDRCONFIG* pConfig = nullptr) const override;
   virtual void ReportDistributionFactors(const CGirderKey& girderKey,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits) const override;
   virtual void ReportReactionDistributionFactors(const CGirderKey& girderKey, rptChapter* pChapter, bool bSubHeading) const override;
   virtual bool Run1250Tests(const CSpanKey& spanKey,pgsTypes::LimitState limitState,LPCTSTR pid,LPCTSTR bridgeId,std::_tofstream& resultsFile, std::_tofstream& poiFile) const override;
   virtual Uint32 GetNumberOfDesignLanes(SpanIndexType spanIdx) const override;
   virtual Uint32 GetNumberOfDesignLanesEx(SpanIndexType spanIdx,Float64* pDistToSection,Float64* pCurbToCurb) const override;
   virtual bool GetDFResultsEx(const CSpanKey& spanKey,pgsTypes::LimitState limitState,
                       Float64* gpM, Float64* gpM1, Float64* gpM2,  // pos moment
                       Float64* gnM, Float64* gnM1, Float64* gnM2,  // neg moment, ahead face
                       Float64* gV,  Float64* gV1,  Float64* gV2) const;   // shear
   virtual Float64 GetDeflectionDistFactor(const CSpanKey& spanKey) const override;
   virtual Float64 GetDeflectionDistFactorEx(const CSpanKey& spanKey, Float64* pMPF, Uint32* pnLanes, GirderIndexType* pnGirders) const override;

// IMomentCapacity
public:
   virtual Float64 GetMomentCapacity(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,bool bPositiveMoment) const override;
   virtual std::vector<Float64> GetMomentCapacity(IntervalIndexType intervalIdx,const PoiList& vPoi,bool bPositiveMoment) const override;
   virtual const MOMENTCAPACITYDETAILS* GetMomentCapacityDetails(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, bool bPositiveMoment, const GDRCONFIG* pConfig = nullptr) const override;
   virtual std::vector<const MOMENTCAPACITYDETAILS*> GetMomentCapacityDetails(IntervalIndexType intervalIdx, const PoiList& vPoi, bool bPositiveMoment, const GDRCONFIG* pConfig = nullptr) const override;
   virtual Float64 GetCrackingMoment(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,bool bPositiveMoment) const override;
   virtual const CRACKINGMOMENTDETAILS* GetCrackingMomentDetails(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,bool bPositiveMoment) const override;
   virtual void GetCrackingMomentDetails(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,const GDRCONFIG& config,bool bPositiveMoment,CRACKINGMOMENTDETAILS* pcmd) const override;
   virtual Float64 GetMinMomentCapacity(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,bool bPositiveMoment) const override;
   virtual const MINMOMENTCAPDETAILS* GetMinMomentCapacityDetails(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,bool bPositiveMoment) const override;
   virtual void GetMinMomentCapacityDetails(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,const GDRCONFIG& config,bool bPositiveMoment,MINMOMENTCAPDETAILS* pmmcd) const override;
   virtual std::vector<const MINMOMENTCAPDETAILS*> GetMinMomentCapacityDetails(IntervalIndexType intervalIdx,const PoiList& vPoi,bool bPositiveMoment) const override;
   virtual std::vector<const CRACKINGMOMENTDETAILS*> GetCrackingMomentDetails(IntervalIndexType intervalIdx,const PoiList& vPoi,bool bPositiveMoment) const override;
   virtual std::vector<Float64> GetCrackingMoment(IntervalIndexType intervalIdx,const PoiList& vPoi,bool bPositiveMoment) const override;
   virtual std::vector<Float64> GetMinMomentCapacity(IntervalIndexType intervalIdx,const PoiList& vPoi,bool bPositiveMoment) const override;

// IShearCapacity
public:
   virtual pgsTypes::FaceType GetFlexuralTensionSide(pgsTypes::LimitState limitState,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) const override;
   virtual Float64 GetShearCapacity(pgsTypes::LimitState limitState, IntervalIndexType intervalIdx,const pgsPointOfInterest& poi, const GDRCONFIG* pConfig = nullptr) const override;
   virtual std::vector<Float64> GetShearCapacity(pgsTypes::LimitState limitState, IntervalIndexType intervalIdx,const PoiList& vPoi) const override;
   virtual void GetShearCapacityDetails(pgsTypes::LimitState limitState, IntervalIndexType intervalIdx,const pgsPointOfInterest& poi, const GDRCONFIG* pConfig,SHEARCAPACITYDETAILS* pmcd) const override;
   virtual void GetRawShearCapacityDetails(pgsTypes::LimitState limitState, IntervalIndexType intervalIdx,const pgsPointOfInterest& poi, const GDRCONFIG* pConfig,SHEARCAPACITYDETAILS* pmcd) const override;
   virtual Float64 GetFpc(const pgsPointOfInterest& poi, const GDRCONFIG* pConfig = nullptr) const override;
   virtual void GetFpcDetails(const pgsPointOfInterest& poi, const GDRCONFIG* pConfig, FPCDETAILS* pmcd) const override;


   virtual ZoneIndexType GetCriticalSectionZoneIndex(pgsTypes::LimitState limitState,const pgsPointOfInterest& poi) const override;
   virtual void GetCriticalSectionZoneBoundary(pgsTypes::LimitState limitState,const CGirderKey& girderKeyegmentKey,ZoneIndexType csZoneIdx,Float64* pStart,Float64* pEnd) const override;

   virtual std::vector<Float64> GetCriticalSections(pgsTypes::LimitState limitState,const CGirderKey& girderKey, const GDRCONFIG* pConfig = nullptr) const override;
   virtual const std::vector<CRITSECTDETAILS>& GetCriticalSectionDetails(pgsTypes::LimitState limitState,const CGirderKey& girderKey,const GDRCONFIG* pConfig) const override;

   virtual std::vector<SHEARCAPACITYDETAILS> GetShearCapacityDetails(pgsTypes::LimitState limitState, IntervalIndexType intervalIdx,const PoiList& vPoi) const override;
   virtual void ClearDesignCriticalSections() const override;

// IPrincipalWebStress
public:
   virtual void GetPrincipalWebStressPointsOfInterest(const CSegmentKey& segmentKey, IntervalIndexType interval, PoiList* pPoiList) const override;
   virtual const PRINCIPALSTRESSINWEBDETAILS* GetPrincipalWebStressDetails(const pgsPointOfInterest& poi) const override;
   virtual const std::vector<TimeStepCombinedPrincipalWebStressDetailsAtWebSection>* GetTimeStepPrincipalWebStressDetails(const pgsPointOfInterest& poi, IntervalIndexType interval) const override;

// IGirderHaunch
public:
   virtual Float64 GetRequiredSlabOffset(const CSegmentKey& segmentKey) const override;
   virtual const SLABOFFSETDETAILS& GetSlabOffsetDetails(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetSectionGirderOrientationEffect(const pgsPointOfInterest& poi) const override;


// IBearingDesignParameters
 public:
   void GetBearingParameters(CGirderKey girderKey, BEARINGPARAMETERS* pDetails) const override;

   void GetLongitudinalPointOfFixity(const CGirderKey& girderKey, BEARINGPARAMETERS* pDetails) const override;

   void GetBearingDesignProperties(DESIGNPROPERTIES* pDetails) const override;

   void GetBearingRotationDetails(pgsTypes::AnalysisType analysisType, const pgsPointOfInterest& poi, const ReactionLocation& reactionLocation, 
       CGirderKey girderKey, bool bIncludeImpact, bool bIncludeLLDF, bool isFlexural, ROTATIONDETAILS* pDetails) const override;

   void GetBearingReactionDetails(const ReactionLocation& reactionLocation,
       CGirderKey girderKey, pgsTypes::AnalysisType analysisType, bool bIncludeImpact, bool bIncludeLLDF, REACTIONDETAILS* pDetails) const override;

   void GetThermalExpansionDetails(CGirderKey girderKey, BEARINGSHEARDEFORMATIONDETAILS* bearing) const override;

   Float64 GetDistanceToPointOfFixity(const pgsPointOfInterest& poi, SHEARDEFORMATIONDETAILS* pDetails) const override;

   std::array<Float64,2> GetTimeDependentComponentShearDeformation(Float64 loss, BEARINGSHEARDEFORMATIONDETAILS* bearing) const override;

   void GetTimeDependentShearDeformation(CGirderKey girderKey, SHEARDEFORMATIONDETAILS* pDetails) const override;

// IFabricationOptimization
public:
   virtual bool GetFabricationOptimizationDetails(const CSegmentKey& segmentKey,FABRICATIONOPTIMIZATIONDETAILS* pDetails) const override;

// IArtifact
public:
   virtual const pgsGirderArtifact* GetGirderArtifact(const CGirderKey& girderKey) const override;
   virtual const pgsSegmentArtifact* GetSegmentArtifact(const CSegmentKey& segmentKey) const override;
   virtual const WBFL::Stability::LiftingCheckArtifact* GetLiftingCheckArtifact(const CSegmentKey& segmentKey) const override;
   virtual const pgsHaulingAnalysisArtifact* GetHaulingAnalysisArtifact(const CSegmentKey& segmentKey) const override;
   virtual const pgsGirderDesignArtifact* CreateDesignArtifact(const CGirderKey& girderKey, bool bDesignFlexure, arSlabOffsetDesignType haunchDesignType, arConcreteDesignType concreteDesignType, arShearDesignType shearDesignType) const override;
   virtual const pgsGirderDesignArtifact* GetDesignArtifact(const CGirderKey& girderKey) const override;
   virtual void CreateLiftingCheckArtifact(const CSegmentKey& segmentKey,Float64 supportLoc,WBFL::Stability::LiftingCheckArtifact* pArtifact) const override;
   virtual const pgsHaulingAnalysisArtifact* CreateHaulingAnalysisArtifact(const CSegmentKey& segmentKey,Float64 leftSupportLoc,Float64 rightSupportLoc) const override;
   virtual const pgsRatingArtifact* GetRatingArtifact(const CGirderKey& girderKey,pgsTypes::LoadRatingType ratingType,VehicleIndexType vehicleIdx) const override;
   virtual std::shared_ptr<const pgsISummaryRatingArtifact> GetSummaryRatingArtifact(const std::vector<CGirderKey>& girderKeys,pgsTypes::LoadRatingType ratingType,VehicleIndexType vehicleIdx) const override;

// ICrackedSection
public:
   virtual Float64 GetIcr(const pgsPointOfInterest& poi,bool bPositiveMoment) const override;
   virtual const CRACKEDSECTIONDETAILS* GetCrackedSectionDetails(const pgsPointOfInterest& poi, bool bPositiveMoment) const override;
   virtual std::vector<const CRACKEDSECTIONDETAILS*> GetCrackedSectionDetails(const PoiList& vPoi,bool bPositiveMoment) const override;

// IBridgeDescriptionEventSink
public:
   virtual HRESULT OnBridgeChanged(CBridgeChangedHint* pHint) override;
   virtual HRESULT OnGirderFamilyChanged() override;
   virtual HRESULT OnGirderChanged(const CGirderKey& girderKey,Uint32 lHint) override;
   virtual HRESULT OnLiveLoadChanged() override;
   virtual HRESULT OnLiveLoadNameChanged(LPCTSTR strOldName,LPCTSTR strNewName) override;
   virtual HRESULT OnConstructionLoadChanged() override;

// ISpecificationEventSink
public:
   virtual HRESULT OnSpecificationChanged() override;
   virtual HRESULT OnAnalysisTypeChanged() override;

// IRatingSpecificationEventSink
public:
   virtual HRESULT OnRatingSpecificationChanged() override;

// ILoadModifiersEventSink
public:
   virtual HRESULT OnLoadModifiersChanged() override;

// IEnvironmentEventSink
public:
   virtual HRESULT OnExposureConditionChanged() override;
   virtual HRESULT OnClimateConditionChanged() override;
   virtual HRESULT OnRelHumidityChanged() override;

// ILossParametersEventSink
public:
   virtual HRESULT OnLossParametersChanged() override;

private:
   DECLARE_EAF_AGENT_DATA;

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

   pgsTransferLengthEngineer m_TransferLengthEngineer;
   pgsDevelopmentLengthEngineer m_DevelopmentLengthEngineer;

   pgsPsForceEng             m_PsForceEngineer;
   pgsDesigner2              m_Designer;
   pgsLoadRater              m_LoadRater;
   pgsShearCapacityEngineer  m_ShearCapEngineer;
   pgsBearingDesignEngineer  m_BearingEngineer;
   
   mutable bool m_bAreDistFactorEngineersValidated;
   mutable CComPtr<IDistFactorEngineer> m_pDistFactorEngineer; // assigned a polymorphic object during validation (must be mutable for delayed assignment)

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
   mutable std::map<CSegmentKey, std::map<Float64,WBFL::Stability::LiftingCheckArtifact,Float64_less> > m_LiftingArtifacts;
   mutable std::map<CSegmentKey, std::map<Float64,std::shared_ptr<pgsHaulingAnalysisArtifact>,Float64_less> > m_HaulingArtifacts;

   // Event Sink Cookies
   DWORD m_dwBridgeDescCookie;
   DWORD m_dwSpecificationCookie;
   DWORD m_dwRatingSpecificationCookie;
   DWORD m_dwLoadModifiersCookie;
   DWORD m_dwEnvironmentCookie;
   DWORD m_dwLossParametersCookie;

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

#endif //__ENGAGENT_H_
