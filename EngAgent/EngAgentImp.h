///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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
#include <PgsExt\SegmentKey.h>
#include <map>

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
	virtual Float64 GetElasticShortening(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType);
   virtual const LOSSDETAILS* GetLossDetails(const pgsPointOfInterest& poi);
   virtual void ReportLosses(const CGirderKey& girderKey,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits);
   virtual void ReportFinalLosses(const CGirderKey& girderKey,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits);

   virtual Float64 GetElasticShortening(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,const GDRCONFIG& config);
   virtual const LOSSDETAILS* GetLossDetails(const pgsPointOfInterest& poi,const GDRCONFIG& config);
   virtual void ClearDesignLosses();

   virtual Float64 GetPrestressLoss(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime);
   virtual Float64 GetPrestressLoss(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,const GDRCONFIG& config,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime);

   virtual Float64 GetPrestressLossWithLiveLoad(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType);

   virtual Float64 GetFrictionLoss(const pgsPointOfInterest& poi,DuctIndexType ductIdx);
   virtual Float64 GetAnchorSetZoneLength(const CGirderKey& girderKey,DuctIndexType ductIdx,pgsTypes::MemberEndType endType);
   virtual Float64 GetAnchorSetLoss(const pgsPointOfInterest& poi,DuctIndexType ductIdx);

// IPretensionForce
public:
   virtual Float64 GetPjackMax(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType,StrandIndexType nStrands);
   virtual Float64 GetPjackMax(const CSegmentKey& segmentKey,const matPsStrand& strand,StrandIndexType nStrands);

   virtual Float64 GetXferLength(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType);
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
   virtual Float64 GetHorizHarpedStrandForce(const pgsPointOfInterest& poi,const GDRCONFIG& config,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime);

   virtual Float64 GetVertHarpedStrandForce(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime);
   virtual Float64 GetVertHarpedStrandForce(const pgsPointOfInterest& poi,const GDRCONFIG& config,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime);

   virtual Float64 GetPrestressForce(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime);
   virtual Float64 GetPrestressForce(const pgsPointOfInterest& poi,const GDRCONFIG& config,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime);

   virtual Float64 GetPrestressForcePerStrand(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime);
   virtual Float64 GetPrestressForcePerStrand(const pgsPointOfInterest& poi,const GDRCONFIG& config,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime);

   virtual Float64 GetEffectivePrestress(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime);
   virtual Float64 GetEffectivePrestress(const pgsPointOfInterest& poi,const GDRCONFIG& config,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime);

   virtual Float64 GetPrestressForceWithLiveLoad(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType);
   virtual Float64 GetEffectivePrestressWithLiveLoad(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType);

// IPosttensionForce
public:
   virtual Float64 GetPjackMax(const CGirderKey& girderKey,StrandIndexType nStrands);
   virtual Float64 GetPjackMax(const CGirderKey& girderKey,const matPsStrand& strand,StrandIndexType nStrands);
   virtual Float64 GetTendonForce(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType time,DuctIndexType ductIdx);
   virtual Float64 GetTendonStress(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType time,DuctIndexType ductIdx);


// ILiveLoadDistributionFactors
public:
   virtual void VerifyDistributionFactorRequirements(const pgsPointOfInterest& poi);
   virtual Float64 GetMomentDistFactor(SpanIndexType spanIdx,GirderIndexType gdrIdx,pgsTypes::LimitState ls);
   virtual Float64 GetNegMomentDistFactor(SpanIndexType spanIdx,GirderIndexType gdrIdx,pgsTypes::LimitState ls);
   virtual Float64 GetNegMomentDistFactorAtPier(PierIndexType pierIdx,GirderIndexType gdrIdx,pgsTypes::LimitState ls,pgsTypes::PierFaceType pierFace);
   virtual Float64 GetShearDistFactor(SpanIndexType spanIdx,GirderIndexType gdrIdx,pgsTypes::LimitState ls);
   virtual Float64 GetReactionDistFactor(PierIndexType pierIdx,GirderIndexType gdrIdx,pgsTypes::LimitState ls);
   virtual Float64 GetMomentDistFactor(SpanIndexType spanIdx,GirderIndexType gdrIdx,pgsTypes::LimitState ls,Float64 fcgdr);
   virtual Float64 GetNegMomentDistFactor(SpanIndexType spanIdx,GirderIndexType gdrIdx,pgsTypes::LimitState ls,Float64 fcgdr);
   virtual Float64 GetNegMomentDistFactorAtPier(PierIndexType pierIdx,GirderIndexType gdrIdx,pgsTypes::LimitState lsr,pgsTypes::PierFaceType pierFace,Float64 fcgdr);
   virtual Float64 GetShearDistFactor(SpanIndexType spanIdx,GirderIndexType gdrIdx,pgsTypes::LimitState ls,Float64 fcgdr);
   virtual Float64 GetReactionDistFactor(PierIndexType pierIdx,GirderIndexType gdrIdx,pgsTypes::LimitState ls,Float64 fcgdr);
   virtual void GetNegMomentDistFactorPoints(SpanIndexType spanIdx,GirderIndexType gdrIdx,Float64* dfPoints,Uint32* nPoints);
   virtual void GetDistributionFactors(const pgsPointOfInterest& poi,pgsTypes::LimitState ls,Float64* pM,Float64* nM,Float64* V);
   virtual void GetDistributionFactors(const pgsPointOfInterest& poi,pgsTypes::LimitState ls,Float64 fcgdr,Float64* pM,Float64* nM,Float64* V);
   virtual void ReportDistributionFactors(const CGirderKey& girderKey,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits);
   virtual bool Run1250Tests(SpanIndexType spanIdx,GirderIndexType gdrIdx,pgsTypes::LimitState ls,LPCTSTR pid,LPCTSTR bridgeId,std::_tofstream& resultsFile, std::_tofstream& poiFile);
   virtual Uint32 GetNumberOfDesignLanes(SpanIndexType spanIdx);
   virtual Uint32 GetNumberOfDesignLanesEx(SpanIndexType spanIdx,Float64* pDistToSection,Float64* pCurbToCurb);
   virtual bool GetDFResultsEx(SpanIndexType spanIdx,GirderIndexType gdrIdx,pgsTypes::LimitState ls,
                       Float64* gpM, Float64* gpM1, Float64* gpM2,  // pos moment
                       Float64* gnM, Float64* gnM1, Float64* gnM2,  // neg moment, ahead face
                       Float64* gV,  Float64* gV1,  Float64* gV2,   // shear
                       Float64* gR,  Float64* gR1,  Float64* gR2 ); // reaction
   virtual Float64 GetDeflectionDistFactor(SpanIndexType spanIdx,GirderIndexType gdrIdx);

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
   virtual Float64 GetShearCapacity(pgsTypes::LimitState ls, IntervalIndexType intervalIdx,const pgsPointOfInterest& poi);
   virtual std::vector<Float64> GetShearCapacity(pgsTypes::LimitState ls, IntervalIndexType intervalIdx,const std::vector<pgsPointOfInterest>& vPoi);
   virtual void GetShearCapacityDetails(pgsTypes::LimitState ls, IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,SHEARCAPACITYDETAILS* pmcd);
   virtual void GetRawShearCapacityDetails(pgsTypes::LimitState ls, IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,SHEARCAPACITYDETAILS* pmcd);
   virtual Float64 GetFpc(const pgsPointOfInterest& poi);
   virtual void GetFpcDetails(const pgsPointOfInterest& poi, FPCDETAILS* pmcd);
   virtual Float64 GetShearCapacity(pgsTypes::LimitState ls, IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,const GDRCONFIG& config);
   virtual void GetShearCapacityDetails(pgsTypes::LimitState ls, IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,const GDRCONFIG& config,SHEARCAPACITYDETAILS* pmcd);
   virtual void GetRawShearCapacityDetails(pgsTypes::LimitState ls, IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,const GDRCONFIG& config,SHEARCAPACITYDETAILS* pmcd);
   virtual Float64 GetFpc(const pgsPointOfInterest& poi,const GDRCONFIG& config);
   virtual void GetFpcDetails(const pgsPointOfInterest& poi, const GDRCONFIG& config,FPCDETAILS* pmcd);
   virtual std::vector<Float64> GetCriticalSections(pgsTypes::LimitState limitState,const CGirderKey& girderKey);
   virtual std::vector<Float64> GetCriticalSections(pgsTypes::LimitState limitState,const CGirderKey& girderKey,const GDRCONFIG& config);
   virtual const std::vector<CRITSECTDETAILS>& GetCriticalSectionDetails(pgsTypes::LimitState limitState,const CGirderKey& girderKey);
   virtual std::vector<CRITSECTDETAILS> GetCriticalSectionDetails(pgsTypes::LimitState limitState,const CGirderKey& girderKey,const GDRCONFIG& config);
   virtual std::vector<SHEARCAPACITYDETAILS> GetShearCapacityDetails(pgsTypes::LimitState ls, IntervalIndexType intervalIdx,const std::vector<pgsPointOfInterest>& vPoi);

// IGirderHaunch
public:
   virtual Float64 GetRequiredSlabOffset(const CSegmentKey& segmentKey);
   virtual void GetHaunchDetails(const CSegmentKey& segmentKey,HAUNCHDETAILS* pDetails);

// IFabricationOptimization
public:
   virtual void GetFabricationOptimizationDetails(const CSegmentKey& segmentKey,FABRICATIONOPTIMIZATIONDETAILS* pDetails);

// IArtifact
public:
   virtual const pgsGirderArtifact* GetGirderArtifact(const CGirderKey& girderKey);
   virtual const pgsSegmentArtifact* GetSegmentArtifact(const CSegmentKey& segmentKey);
   virtual const pgsClosurePourArtifact* GetClosurePourArtifact(const CSegmentKey& segmentKey);
   virtual const pgsDesignArtifact* CreateDesignArtifact(const CGirderKey& girderKey,arDesignOptions design);
   virtual const pgsDesignArtifact* GetDesignArtifact(const CGirderKey& girderKey);
   virtual void CreateLiftingAnalysisArtifact(const CSegmentKey& segmentKey,Float64 supportLoc,pgsLiftingAnalysisArtifact* pArtifact);
   virtual void CreateHaulingAnalysisArtifact(const CSegmentKey& segmentKey,Float64 leftSupportLoc,Float64 rightSupportLoc,pgsHaulingAnalysisArtifact* pArtifact);
   virtual const pgsRatingArtifact* GetRatingArtifact(const CGirderKey& girderKey,pgsTypes::LoadRatingType ratingType,VehicleIndexType vehicleIndex);

// ICrackedSection
public:
   virtual void GetCrackedSectionDetails(const pgsPointOfInterest& poi,bool bPositiveMoment,CRACKEDSECTIONDETAILS* pCSD);
   virtual Float64 GetIcr(const pgsPointOfInterest& poi,bool bPositiveMoment);
   virtual std::vector<CRACKEDSECTIONDETAILS> GetCrackedSectionDetails(const std::vector<pgsPointOfInterest>& vPoi,bool bPositiveMoment);

// IBridgeDescriptionEventSink
public:
   virtual HRESULT OnBridgeChanged();
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

private:
   DECLARE_AGENT_DATA;

   std::map<CGirderKey,pgsGirderArtifact> m_CheckArtifacts;
   std::map<CGirderKey,pgsDesignArtifact> m_DesignArtifacts;

   struct RatingArtifactKey
   {
      RatingArtifactKey(const CGirderKey& girderKey,VehicleIndexType vehIdx)
      { GirderKey = girderKey; VehicleIdx = vehIdx; }

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

#pragma Reminder("UPDATE: need to cache prestress force") // I broke this when converting to interval based loss model
   //std::map<PoiPrestressKey,Float64> m_PsForce;

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
   const MINMOMENTCAPDETAILS* ValidateMinMomentCapacity(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,bool bPositiveMoment);

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
   const SHEARCAPACITYDETAILS* ValidateShearCapacity(pgsTypes::LimitState ls, IntervalIndexType intervalIdx,const pgsPointOfInterest& poi);
   void InvalidateShearCapacity();
   std::map<PoiIDKey,FPCDETAILS> m_Fpc;
   const FPCDETAILS* ValidateFpc(const pgsPointOfInterest& poi);
   void InvalidateFpc();

   std::vector<Float64> GetCriticalSectionFromDetails(const std::vector<CRITSECTDETAILS>& csDetails);

   // critical section for shear (a segment can have N critical sections)
   // critical sections are listed left to right along a segment
   std::map<CGirderKey,std::vector<CRITSECTDETAILS>> m_CritSectionDetails[8]; // use the LimitStateToShearIndex method to map limit state to array index
   const std::vector<CRITSECTDETAILS>& ValidateShearCritSection(pgsTypes::LimitState limitState,const CGirderKey& girderKey);
   std::vector<CRITSECTDETAILS> CalculateShearCritSection(pgsTypes::LimitState limitState,const CGirderKey& girderKey);
   std::vector<CRITSECTDETAILS> CalculateShearCritSection(pgsTypes::LimitState limitState,const CGirderKey& girderKey,const GDRCONFIG& config);
   std::vector<CRITSECTDETAILS> CalculateShearCritSection(pgsTypes::LimitState limitState,const CGirderKey& girderKey,bool bUseConfig,const GDRCONFIG& config);
   ZoneIndexType GetCriticalSectionZoneIndex(pgsTypes::LimitState limitState,const pgsPointOfInterest& poi);
   void InvalidateShearCritSection();

   std::map<CSegmentKey,HAUNCHDETAILS> m_HaunchDetails;

   // Lifting analysis artifacts
   std::map<CSegmentKey, std::map<Float64,pgsLiftingAnalysisArtifact,Float64_less> > m_LiftingArtifacts;
   std::map<CSegmentKey, std::map<Float64,pgsHaulingAnalysisArtifact,Float64_less> > m_HaulingArtifacts;

   // Event Sink Cookies
   DWORD m_dwBridgeDescCookie;
   DWORD m_dwSpecificationCookie;
   DWORD m_dwRatingSpecificationCookie;
   DWORD m_dwLoadModifiersCookie;
   DWORD m_dwEnvironmentCookie;

   void InvalidateAll();
   void InvalidateHaunch();
   void InvalidateLosses();
   void ValidateLiveLoadDistributionFactors(const CGirderKey& girderKey);
   void InvalidateLiveLoadDistributionFactors();
   void ValidateArtifacts(const CGirderKey& girderKey);
   void ValidateRatingArtifacts(const CGirderKey& girderKey,pgsTypes::LoadRatingType ratingType,VehicleIndexType vehicleIndex);
   void InvalidateArtifacts();
   void InvalidateRatingArtifacts();

   const LOSSDETAILS* FindLosses(const pgsPointOfInterest& poi);
   pgsGirderArtifact* FindArtifact(const CGirderKey& girderKey);
   pgsRatingArtifact* FindRatingArtifact(const CGirderKey& girderKey,pgsTypes::LoadRatingType ratingType,VehicleIndexType vehicleIndex);

   DECLARE_LOGFILE;

   void CheckCurvatureRequirements(const pgsPointOfInterest& poi);
   void CheckGirderStiffnessRequirements(const pgsPointOfInterest& poi);
   void CheckParallelGirderRequirements(const pgsPointOfInterest& poi);
};

#endif //__ENGAGENT_H_
