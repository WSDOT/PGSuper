///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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

#include "resource.h"       // main symbols
#include "PsForceEng.h"
#include "Designer2.h"
#include "LoadRater.h"
#include "MomentCapacityEngineer.h"
#include "ShearCapacityEngineer.h"
#include <IFace\DistFactorEngineer.h>
#include <IFace\RatingSpecification.h>
#include <IFace\CrackedSection.h>
#include <EAF\EAFInterfaceCache.h>

#include <PgsExt\PoiKey.h>
#include <PgsExt\Keys.h>
#include <map>


class PrestressWithLiveLoadSubKey
{
public:
   PrestressWithLiveLoadSubKey()
   {
      m_LimitState = pgsTypes::StrengthI;
      m_Strand = pgsTypes::Straight;
   }

   PrestressWithLiveLoadSubKey(pgsTypes::StrandType strand,pgsTypes::LimitState limitState) :
      m_Strand(strand), m_LimitState(limitState)
      {
      }

   PrestressWithLiveLoadSubKey(const PrestressWithLiveLoadSubKey& rOther)
   {
      m_Strand = rOther.m_Strand;
      m_LimitState = rOther.m_LimitState;
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

      return false;
   }

private:
   pgsTypes::LimitState m_LimitState;
   pgsTypes::StrandType m_Strand;
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
   public IGirderHaunch,
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
   COM_INTERFACE_ENTRY(IGirderHaunch)
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

   // callback id's
   StatusCallbackIDType m_scidUnknown;
   StatusCallbackIDType m_scidRefinedAnalysis;
   StatusCallbackIDType m_scidBridgeDescriptionError;
   StatusCallbackIDType m_scidLldfWarning;

// IAgent
public:
	STDMETHOD(SetBroker)(IBroker* pBroker);
	STDMETHOD(RegInterfaces)();
	STDMETHOD(Init)();
	STDMETHOD(Reset)();
	STDMETHOD(ShutDown)();
   STDMETHOD(Init2)();
   STDMETHOD(GetClassID)(CLSID* pCLSID);

// ILosses
public:
   virtual const LOSSDETAILS* GetLossDetails(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx);
   virtual CString GetRestrainingLoadName(IntervalIndexType intervalIdx,int loadType);
	virtual Float64 GetElasticShortening(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType);
   virtual void ReportLosses(const CGirderKey& girderKey,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits);
   virtual void ReportFinalLosses(const CGirderKey& girderKey,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits);

   virtual Float64 GetElasticShortening(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,const GDRCONFIG& config);
   virtual const LOSSDETAILS* GetLossDetails(const pgsPointOfInterest& poi,const GDRCONFIG& config,IntervalIndexType intervalIdx);
   virtual void ClearDesignLosses();

   virtual Float64 GetEffectivePrestressLoss(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime);
   virtual Float64 GetEffectivePrestressLoss(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime,const GDRCONFIG& config);
   virtual Float64 GetEffectivePrestressLossWithLiveLoad(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,pgsTypes::LimitState limitState);
   virtual Float64 GetEffectivePrestressLossWithLiveLoad(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,pgsTypes::LimitState limitState,const GDRCONFIG& config);

   virtual Float64 GetTimeDependentLosses(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime);
   virtual Float64 GetTimeDependentLosses(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime,const GDRCONFIG& config);

   virtual Float64 GetInstantaneousEffects(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime);
   virtual Float64 GetInstantaneousEffects(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime,const GDRCONFIG& config);
   virtual Float64 GetInstantaneousEffectsWithLiveLoad(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,pgsTypes::LimitState limitState);
   virtual Float64 GetInstantaneousEffectsWithLiveLoad(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,pgsTypes::LimitState limitState,const GDRCONFIG& config);

   virtual Float64 GetFrictionLoss(const pgsPointOfInterest& poi,DuctIndexType ductIdx);
   virtual Float64 GetAnchorSetZoneLength(const CGirderKey& girderKey,DuctIndexType ductIdx,pgsTypes::MemberEndType endType);
   virtual Float64 GetAnchorSetLoss(const pgsPointOfInterest& poi,DuctIndexType ductIdx);
   virtual Float64 GetElongation(const CGirderKey& girderKey,DuctIndexType ductIdx,pgsTypes::MemberEndType endType);
   virtual Float64 GetAverageFrictionLoss(const CGirderKey& girderKey,DuctIndexType ductIdx);
   virtual Float64 GetAverageAnchorSetLoss(const CGirderKey& girderKey,DuctIndexType ductIdx);

   virtual bool AreElasticGainsApplicable();
   virtual bool IsDeckShrinkageApplicable();


// IPretensionForce
public:
   virtual Float64 GetPjackMax(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType,StrandIndexType nStrands);
   virtual Float64 GetPjackMax(const CSegmentKey& segmentKey,const matPsStrand& strand,StrandIndexType nStrands);

   virtual Float64 GetXferLength(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType);
   virtual Float64 GetXferLengthAdjustment(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType);
   virtual Float64 GetDevLength(const pgsPointOfInterest& poi,bool bDebonded);
   virtual STRANDDEVLENGTHDETAILS GetDevLengthDetails(const pgsPointOfInterest& poi,bool bDebonded);
   virtual STRANDDEVLENGTHDETAILS GetDevLengthDetails(const pgsPointOfInterest& poi,const GDRCONFIG& config,bool bDebonded);
   virtual Float64 GetStrandBondFactor(const pgsPointOfInterest& poi,StrandIndexType strandIdx,pgsTypes::StrandType strandType);
   virtual Float64 GetStrandBondFactor(const pgsPointOfInterest& poi,const GDRCONFIG& config,StrandIndexType strandIdx,pgsTypes::StrandType strandType);
   virtual Float64 GetStrandBondFactor(const pgsPointOfInterest& poi,StrandIndexType strandIdx,pgsTypes::StrandType strandType,Float64 fps,Float64 fpe);
   virtual Float64 GetStrandBondFactor(const pgsPointOfInterest& poi,const GDRCONFIG& config,StrandIndexType strandIdx,pgsTypes::StrandType strandType,Float64 fps,Float64 fpe);

   virtual Float64 GetHoldDownForce(const CSegmentKey& segmentKey);
   virtual Float64 GetHoldDownForce(const CSegmentKey& segmentKey,const GDRCONFIG& config);

   virtual Float64 GetHorizHarpedStrandForce(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime);
   virtual Float64 GetHorizHarpedStrandForce(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime,const GDRCONFIG& config);

   virtual Float64 GetVertHarpedStrandForce(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime);
   virtual Float64 GetVertHarpedStrandForce(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime,const GDRCONFIG& config);

   virtual Float64 GetPrestressForce(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime);
   virtual Float64 GetPrestressForce(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime,const GDRCONFIG& config);
   virtual Float64 GetPrestressForce(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime,bool bIncludeElasticEffects);
   virtual Float64 GetPrestressForcePerStrand(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime);
   virtual Float64 GetPrestressForcePerStrand(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime,const GDRCONFIG& config);
   virtual Float64 GetEffectivePrestress(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime);
   virtual Float64 GetEffectivePrestress(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime,const GDRCONFIG& config);

   virtual Float64 GetPrestressForceWithLiveLoad(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,pgsTypes::LimitState limitState);
   virtual Float64 GetPrestressForceWithLiveLoad(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,pgsTypes::LimitState limitState,const GDRCONFIG& config);
   virtual Float64 GetEffectivePrestressWithLiveLoad(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,pgsTypes::LimitState limitState);
   virtual Float64 GetEffectivePrestressWithLiveLoad(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,pgsTypes::LimitState limitState,const GDRCONFIG& config);

   virtual void GetEccentricityEnvelope(const pgsPointOfInterest& rpoi,const GDRCONFIG& config, Float64* pLowerBound, Float64* pUpperBound);

   // non virtual
   pgsEccEnvelope GetEccentricityEnvelope(const pgsPointOfInterest& rpoi,const GDRCONFIG& config);

// IPosttensionForce
public:
   virtual Float64 GetPjackMax(const CGirderKey& girderKey,StrandIndexType nStrands);
   virtual Float64 GetPjackMax(const CGirderKey& girderKey,const matPsStrand& strand,StrandIndexType nStrands);
   virtual Float64 GetInitialTendonForce(const pgsPointOfInterest& poi,DuctIndexType ductIdx,bool bIncludeAnchorSet);
   virtual Float64 GetInitialTendonStress(const pgsPointOfInterest& poi,DuctIndexType ductIdx,bool bIncludeAnchorSet);
   virtual Float64 GetAverageInitialTendonForce(const CGirderKey& girderKey,DuctIndexType ductIdx);
   virtual Float64 GetAverageInitialTendonStress(const CGirderKey& girderKey,DuctIndexType ductIdx);
   virtual Float64 GetTendonForce(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType time,DuctIndexType ductIdx,bool bIncludeMinLiveLoad,bool bIncludeMaxLiveLoad);
   virtual Float64 GetTendonStress(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType time,DuctIndexType ductIdx,bool bIncludeMinLiveLoad,bool bIncludeMaxLiveLoad);
   virtual Float64 GetVerticalTendonForce(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime,DuctIndexType ductIdx);


// ILiveLoadDistributionFactors
public:
   virtual void VerifyDistributionFactorRequirements(const pgsPointOfInterest& poi);
   virtual Float64 GetMomentDistFactor(const CSpanKey& spanKey,pgsTypes::LimitState limitState);
   virtual Float64 GetNegMomentDistFactor(const CSpanKey& spanKey,pgsTypes::LimitState limitState);
   virtual Float64 GetNegMomentDistFactorAtPier(PierIndexType pierIdx,GirderIndexType gdrIdx,pgsTypes::LimitState limitState,pgsTypes::PierFaceType pierFace);
   virtual Float64 GetShearDistFactor(const CSpanKey& spanKey,pgsTypes::LimitState limitState);
   virtual Float64 GetReactionDistFactor(PierIndexType pierIdx,GirderIndexType gdrIdx,pgsTypes::LimitState limitState);
   virtual Float64 GetMomentDistFactor(const CSpanKey& spanKey,pgsTypes::LimitState limitState,Float64 fcgdr);
   virtual Float64 GetNegMomentDistFactor(const CSpanKey& spanKey,pgsTypes::LimitState limitState,Float64 fcgdr);
   virtual Float64 GetNegMomentDistFactorAtPier(PierIndexType pierIdx,GirderIndexType gdrIdx,pgsTypes::LimitState lsr,pgsTypes::PierFaceType pierFace,Float64 fcgdr);
   virtual Float64 GetShearDistFactor(const CSpanKey& spanKey,pgsTypes::LimitState limitState,Float64 fcgdr);
   virtual Float64 GetReactionDistFactor(PierIndexType pierIdx,GirderIndexType gdrIdx,pgsTypes::LimitState limitState,Float64 fcgdr);
   virtual Float64 GetSkewCorrectionFactorForMoment(const CSpanKey& spanKey,pgsTypes::LimitState ls);
   virtual Float64 GetSkewCorrectionFactorForShear(const CSpanKey& spanKey,pgsTypes::LimitState ls);
   virtual void GetNegMomentDistFactorPoints(const CSpanKey& spanKey,Float64* dfPoints,IndexType* nPoints);
   virtual void GetDistributionFactors(const pgsPointOfInterest& poi,pgsTypes::LimitState limitState,Float64* pM,Float64* nM,Float64* V);
   virtual void GetDistributionFactors(const pgsPointOfInterest& poi,pgsTypes::LimitState limitState,Float64 fcgdr,Float64* pM,Float64* nM,Float64* V);
   virtual void ReportDistributionFactors(const CGirderKey& girderKey,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits);
   virtual bool Run1250Tests(const CSpanKey& spanKey,pgsTypes::LimitState limitState,LPCTSTR pid,LPCTSTR bridgeId,std::_tofstream& resultsFile, std::_tofstream& poiFile);
   virtual Uint32 GetNumberOfDesignLanes(SpanIndexType spanIdx);
   virtual Uint32 GetNumberOfDesignLanesEx(SpanIndexType spanIdx,Float64* pDistToSection,Float64* pCurbToCurb);
   virtual bool GetDFResultsEx(const CSpanKey& spanKey,pgsTypes::LimitState limitState,
                       Float64* gpM, Float64* gpM1, Float64* gpM2,  // pos moment
                       Float64* gnM, Float64* gnM1, Float64* gnM2,  // neg moment, ahead face
                       Float64* gV,  Float64* gV1,  Float64* gV2,   // shear
                       Float64* gR,  Float64* gR1,  Float64* gR2 ); // reaction
   virtual Float64 GetDeflectionDistFactor(const CSpanKey& spanKey);
   virtual void TestRangeOfApplicability(const CSpanKey& spanKey);

// IMomentCapacity
public:
   virtual Float64 GetMomentCapacity(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,bool bPositiveMoment);
   virtual std::vector<Float64> GetMomentCapacity(IntervalIndexType intervalIdx,const std::vector<pgsPointOfInterest>& vPoi,bool bPositiveMoment);
   virtual void GetMomentCapacityDetails(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,bool bPositiveMoment,MOMENTCAPACITYDETAILS* pmcd);
   virtual void GetMomentCapacityDetails(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,const GDRCONFIG& config,bool bPositiveMoment,MOMENTCAPACITYDETAILS* pmcd);
   virtual Float64 GetCrackingMoment(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,bool bPositiveMoment);
   virtual void GetCrackingMomentDetails(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,bool bPositiveMoment,CRACKINGMOMENTDETAILS* pcmd);
   virtual void GetCrackingMomentDetails(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,const GDRCONFIG& config,bool bPositiveMoment,CRACKINGMOMENTDETAILS* pcmd);
   virtual Float64 GetMinMomentCapacity(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,bool bPositiveMoment);
   virtual void GetMinMomentCapacityDetails(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,bool bPositiveMoment,MINMOMENTCAPDETAILS* pmmcd);
   virtual void GetMinMomentCapacityDetails(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,const GDRCONFIG& config,bool bPositiveMoment,MINMOMENTCAPDETAILS* pmmcd);
   virtual std::vector<MOMENTCAPACITYDETAILS> GetMomentCapacityDetails(IntervalIndexType intervalIdx,const std::vector<pgsPointOfInterest>& vPoi,bool bPositiveMoment);
   virtual std::vector<MINMOMENTCAPDETAILS> GetMinMomentCapacityDetails(IntervalIndexType intervalIdx,const std::vector<pgsPointOfInterest>& vPoi,bool bPositiveMoment);
   virtual std::vector<CRACKINGMOMENTDETAILS> GetCrackingMomentDetails(IntervalIndexType intervalIdx,const std::vector<pgsPointOfInterest>& vPoi,bool bPositiveMoment);
   virtual std::vector<Float64> GetCrackingMoment(IntervalIndexType intervalIdx,const std::vector<pgsPointOfInterest>& vPoi,bool bPositiveMoment);
   virtual std::vector<Float64> GetMinMomentCapacity(IntervalIndexType intervalIdx,const std::vector<pgsPointOfInterest>& vPoi,bool bPositiveMoment);

// IShearCapacity
public:
   virtual pgsTypes::FaceType GetFlexuralTensionSide(pgsTypes::LimitState limitState,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi);
   virtual Float64 GetShearCapacity(pgsTypes::LimitState limitState, IntervalIndexType intervalIdx,const pgsPointOfInterest& poi);
   virtual std::vector<Float64> GetShearCapacity(pgsTypes::LimitState limitState, IntervalIndexType intervalIdx,const std::vector<pgsPointOfInterest>& vPoi);
   virtual void GetShearCapacityDetails(pgsTypes::LimitState limitState, IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,SHEARCAPACITYDETAILS* pmcd);
   virtual void GetRawShearCapacityDetails(pgsTypes::LimitState limitState, IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,SHEARCAPACITYDETAILS* pmcd);
   virtual Float64 GetFpc(const pgsPointOfInterest& poi);
   virtual void GetFpcDetails(const pgsPointOfInterest& poi, FPCDETAILS* pmcd);
   virtual Float64 GetShearCapacity(pgsTypes::LimitState limitState, IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,const GDRCONFIG& config);
   virtual void GetShearCapacityDetails(pgsTypes::LimitState limitState, IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,const GDRCONFIG& config,SHEARCAPACITYDETAILS* pmcd);
   virtual void GetRawShearCapacityDetails(pgsTypes::LimitState limitState, IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,const GDRCONFIG& config,SHEARCAPACITYDETAILS* pmcd);
   virtual Float64 GetFpc(const pgsPointOfInterest& poi,const GDRCONFIG& config);
   virtual void GetFpcDetails(const pgsPointOfInterest& poi, const GDRCONFIG& config,FPCDETAILS* pmcd);
   virtual ZoneIndexType GetCriticalSectionZoneIndex(pgsTypes::LimitState limitState,const pgsPointOfInterest& poi);
   virtual void GetCriticalSectionZoneBoundary(pgsTypes::LimitState limitState,const CGirderKey& girderKeyegmentKey,ZoneIndexType csZoneIdx,Float64* pStart,Float64* pEnd);
   virtual std::vector<Float64> GetCriticalSections(pgsTypes::LimitState limitState,const CGirderKey& girderKey);
   virtual std::vector<Float64> GetCriticalSections(pgsTypes::LimitState limitState,const CGirderKey& girderKey,const GDRCONFIG& config);
   virtual const std::vector<CRITSECTDETAILS>& GetCriticalSectionDetails(pgsTypes::LimitState limitState,const CGirderKey& girderKey);
   virtual std::vector<CRITSECTDETAILS> GetCriticalSectionDetails(pgsTypes::LimitState limitState,const CGirderKey& girderKey,const GDRCONFIG& config);
   virtual std::vector<SHEARCAPACITYDETAILS> GetShearCapacityDetails(pgsTypes::LimitState limitState, IntervalIndexType intervalIdx,const std::vector<pgsPointOfInterest>& vPoi);
   virtual void ClearDesignCriticalSections();

// IGirderHaunch
public:
   virtual Float64 GetRequiredSlabOffset(const CSpanKey& spanKey);
   virtual void GetHaunchDetails(const CSpanKey& spanKey,HAUNCHDETAILS* pDetails);
   virtual Float64 GetSectionGirderOrientationEffect(const pgsPointOfInterest& poi);

// IFabricationOptimization
public:
   virtual void GetFabricationOptimizationDetails(const CSegmentKey& segmentKey,FABRICATIONOPTIMIZATIONDETAILS* pDetails);

// IArtifact
public:
   virtual const pgsGirderArtifact* GetGirderArtifact(const CGirderKey& girderKey);
   virtual const pgsSegmentArtifact* GetSegmentArtifact(const CSegmentKey& segmentKey);
   virtual const stbLiftingCheckArtifact* GetLiftingCheckArtifact(const CSegmentKey& segmentKey);
   virtual const pgsHaulingAnalysisArtifact* GetHaulingAnalysisArtifact(const CSegmentKey& segmentKey);
   virtual const pgsGirderDesignArtifact* CreateDesignArtifact(const CGirderKey& girderKey,const std::vector<arDesignOptions>& design);
   virtual const pgsGirderDesignArtifact* GetDesignArtifact(const CGirderKey& girderKey);
   virtual void CreateLiftingCheckArtifact(const CSegmentKey& segmentKey,Float64 supportLoc,stbLiftingCheckArtifact* pArtifact);
   virtual const pgsHaulingAnalysisArtifact* CreateHaulingAnalysisArtifact(const CSegmentKey& segmentKey,Float64 leftSupportLoc,Float64 rightSupportLoc);
   virtual const pgsRatingArtifact* GetRatingArtifact(const CGirderKey& girderKey,pgsTypes::LoadRatingType ratingType,VehicleIndexType vehicleIdx);

// ICrackedSection
public:
   virtual void GetCrackedSectionDetails(const pgsPointOfInterest& poi,bool bPositiveMoment,CRACKEDSECTIONDETAILS* pCSD);
   virtual Float64 GetIcr(const pgsPointOfInterest& poi,bool bPositiveMoment);
   virtual std::vector<CRACKEDSECTIONDETAILS> GetCrackedSectionDetails(const std::vector<pgsPointOfInterest>& vPoi,bool bPositiveMoment);

// IBridgeDescriptionEventSink
public:
   virtual HRESULT OnBridgeChanged(CBridgeChangedHint* pHint);
   virtual HRESULT OnGirderFamilyChanged();
   virtual HRESULT OnGirderChanged(const CGirderKey& girderKey,Uint32 lHint);
   virtual HRESULT OnLiveLoadChanged();
   virtual HRESULT OnLiveLoadNameChanged(LPCTSTR strOldName,LPCTSTR strNewName);
   virtual HRESULT OnConstructionLoadChanged();

// ISpecificationEventSink
public:
   virtual HRESULT OnSpecificationChanged();
   virtual HRESULT OnAnalysisTypeChanged();

// IRatingSpecificationEventSink
public:
   virtual HRESULT OnRatingSpecificationChanged();

// ILoadModifiersEventSink
public:
   virtual HRESULT OnLoadModifiersChanged();

// IEnvironmentEventSink
public:
   virtual HRESULT OnExposureConditionChanged();
   virtual HRESULT OnRelHumidityChanged();

// ILossParametersEventSink
public:
   virtual HRESULT OnLossParametersChanged();

private:
   DECLARE_EAF_AGENT_DATA;

   std::map<CGirderKey,pgsGirderDesignArtifact> m_DesignArtifacts;

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
   std::map<RatingArtifactKey,pgsRatingArtifact> m_RatingArtifacts[6]; // pgsTypes::LoadRatingType enum as key

   std::map<PrestressPoiKey,Float64> m_PsForce; // cache of prestress forces
   std::map<PrestressWithLiveLoadPoiKey,Float64> m_PsForceWithLiveLoad; // cache of prestress forces including live load

   pgsPsForceEng             m_PsForceEngineer;
   pgsDesigner2              m_Designer;
   pgsLoadRater              m_LoadRater;
   pgsMomentCapacityEngineer m_MomentCapEngineer;
   pgsShearCapacityEngineer  m_ShearCapEngineer;
   CComPtr<IDistFactorEngineer> m_pDistFactorEngineer;
   bool                         m_bAreDistFactorEngineersValidated;

   pgsPointOfInterest GetEquivalentPointOfInterest(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi);

   // Moment capacity can be computed for every interval, however we are only supporting
   // it being computed at the interval when the deck becomes composite
   // index 0 = Moment Type (Positive = 0, Negative = 1)
   typedef std::map<PoiIDKey,MOMENTCAPACITYDETAILS> MomentCapacityDetailsContainer;
   const MOMENTCAPACITYDETAILS* ValidateMomentCapacity(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,bool bPositiveMoment);
   const MOMENTCAPACITYDETAILS* ValidateMomentCapacity(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,const GDRCONFIG& config,bool bPositiveMoment);
   MOMENTCAPACITYDETAILS ComputeMomentCapacity(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,bool bPositiveMoment);
   MOMENTCAPACITYDETAILS ComputeMomentCapacity(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,const GDRCONFIG& config,bool bPositiveMoment);
   const MOMENTCAPACITYDETAILS* GetCachedMomentCapacity(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,bool bPositiveMoment);
   const MOMENTCAPACITYDETAILS* GetCachedMomentCapacity(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,const GDRCONFIG& config,bool bPositiveMoment);
   const MOMENTCAPACITYDETAILS* GetCachedMomentCapacity(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,bool bPositiveMoment,const MomentCapacityDetailsContainer& container);
   const MOMENTCAPACITYDETAILS* StoreMomentCapacityDetails(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,bool bPositiveMoment,const MOMENTCAPACITYDETAILS& mcd,MomentCapacityDetailsContainer& container);

   MomentCapacityDetailsContainer m_NonCompositeMomentCapacity[2];
   MomentCapacityDetailsContainer m_CompositeMomentCapacity[2];
   void InvalidateMomentCapacity();

   std::map<PoiIDKey,CRACKINGMOMENTDETAILS> m_NonCompositeCrackingMoment[2];
   std::map<PoiIDKey,CRACKINGMOMENTDETAILS> m_CompositeCrackingMoment[2];
   const CRACKINGMOMENTDETAILS* ValidateCrackingMoments(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,bool bPositiveMoment);

   // index 0 = Moment Type (Positive = 0, Negative = 1)
   std::map<PoiIDKey,MINMOMENTCAPDETAILS> m_NonCompositeMinMomentCapacity[2];
   std::map<PoiIDKey,MINMOMENTCAPDETAILS> m_CompositeMinMomentCapacity[2];
   MINMOMENTCAPDETAILS ValidateMinMomentCapacity(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,bool bPositiveMoment);

   typedef std::map<PoiIDKey,CRACKEDSECTIONDETAILS> CrackedSectionDetailsContainer;
   CrackedSectionDetailsContainer m_CrackedSectionDetails[2]; // 0 = positive moment, 1 = negative moment
   const CRACKEDSECTIONDETAILS* ValidateCrackedSectionDetails(const pgsPointOfInterest& poi,bool bPositiveMoment);
   void InvalidateCrackedSectionDetails();

   // Temporary Moment Capacity (idx 0 = positive moment, idx 1 = negative moment)
   // This is a cache of moment capacities computed based on a supplied GdrConfig and not the
   // current state of input.
   GDRCONFIG m_TempGirderConfig;
   MomentCapacityDetailsContainer m_TempNonCompositeMomentCapacity[2];
   MomentCapacityDetailsContainer m_TempCompositeMomentCapacity[2];

   // Shear Capacity
   std::map<PoiIDKey,SHEARCAPACITYDETAILS> m_ShearCapacity[8]; // use the LimitStateToShearIndex method to map limit state to array index
   const SHEARCAPACITYDETAILS* ValidateShearCapacity(pgsTypes::LimitState limitState, IntervalIndexType intervalIdx,const pgsPointOfInterest& poi);
   void InvalidateShearCapacity();
   std::map<PoiIDKey,FPCDETAILS> m_Fpc;
   const FPCDETAILS* ValidateFpc(const pgsPointOfInterest& poi);
   void InvalidateFpc();

   std::vector<Float64> GetCriticalSectionFromDetails(const std::vector<CRITSECTDETAILS>& csDetails);

   // critical section for shear (a segment can have N critical sections)
   // critical sections are listed left to right along a segment
   std::map<CGirderKey,std::vector<CRITSECTDETAILS>> m_CritSectionDetails[8]; // use the LimitStateToShearIndex method to map limit state to array index
   std::map<CGirderKey,std::vector<CRITSECTDETAILS>> m_DesignCritSectionDetails[8]; // use the LimitStateToShearIndex method to map limit state to array index
   const std::vector<CRITSECTDETAILS>& ValidateShearCritSection(pgsTypes::LimitState limitState,const CGirderKey& girderKey);
   const std::vector<CRITSECTDETAILS>& ValidateShearCritSection(pgsTypes::LimitState limitState,const CGirderKey& girderKey,const GDRCONFIG& config);
   std::vector<CRITSECTDETAILS> CalculateShearCritSection(pgsTypes::LimitState limitState,const CGirderKey& girderKey);
   std::vector<CRITSECTDETAILS> CalculateShearCritSection(pgsTypes::LimitState limitState,const CGirderKey& girderKey,const GDRCONFIG& config);
   std::vector<CRITSECTDETAILS> CalculateShearCritSection(pgsTypes::LimitState limitState,const CGirderKey& girderKey,bool bUseConfig,const GDRCONFIG& config);
   void InvalidateShearCritSection();

   std::map<CSpanKey,HAUNCHDETAILS> m_HaunchDetails;

   // Lifting and hauling analysis artifact cache for ad-hoc analysis (typically during design)
   std::map<CSegmentKey, std::map<Float64,stbLiftingCheckArtifact,Float64_less> > m_LiftingArtifacts;
   std::map<CSegmentKey, std::map<Float64,boost::shared_ptr<pgsHaulingAnalysisArtifact>,Float64_less> > m_HaulingArtifacts;

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
   void ValidateLiveLoadDistributionFactors(const CGirderKey& girderKey);
   void InvalidateLiveLoadDistributionFactors();
   void ValidateArtifacts(const CGirderKey& girderKey);
   void ValidateRatingArtifacts(const CGirderKey& girderKey,pgsTypes::LoadRatingType ratingType,VehicleIndexType vehicleIdx);
   void InvalidateArtifacts();
   void InvalidateRatingArtifacts();

   const LOSSDETAILS* FindLosses(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx);
   pgsRatingArtifact* FindRatingArtifact(const CGirderKey& girderKey,pgsTypes::LoadRatingType ratingType,VehicleIndexType vehicleIdx);

   DECLARE_LOGFILE;

   void CheckCurvatureRequirements(const pgsPointOfInterest& poi);
   void CheckGirderStiffnessRequirements(const pgsPointOfInterest& poi);
   void CheckParallelGirderRequirements(const pgsPointOfInterest& poi);
};

#endif //__ENGAGENT_H_
