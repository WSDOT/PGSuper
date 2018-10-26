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
#include <map>

/////////////////////////////////////////////////////////////////////////////
// CEngAgentImp
class ATL_NO_VTABLE CEngAgentImp : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CEngAgentImp, &CLSID_EngAgent>,
	public IConnectionPointContainerImpl<CEngAgentImp>,
	public IAgentEx,
   public ILosses,
   public IPrestressForce,
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
   COM_INTERFACE_ENTRY(IPrestressForce)
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
   virtual Float64 GetBeforeXferLosses(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType);
   virtual Float64 GetAfterXferLosses(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType);
   virtual Float64 GetLiftingLosses(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType);
   virtual Float64 GetShippingLosses(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType);
   virtual Float64 GetAfterTemporaryStrandInstallationLosses(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType);
   virtual Float64 GetBeforeTemporaryStrandRemovalLosses(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType);
   virtual Float64 GetAfterTemporaryStrandRemovalLosses(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType);
   virtual Float64 GetDeckPlacementLosses(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType);
   virtual Float64 GetSIDLLosses(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType);
   virtual Float64 GetFinal(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType);
   virtual LOSSDETAILS GetLossDetails(const pgsPointOfInterest& poi);
   virtual void ReportLosses(SpanIndexType span,GirderIndexType gdr,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits);
   virtual void ReportFinalLosses(SpanIndexType span,GirderIndexType gdr,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits);

   virtual Float64 GetElasticShortening(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,const GDRCONFIG& config);
   virtual Float64 GetBeforeXferLosses(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,const GDRCONFIG& config);
   virtual Float64 GetAfterXferLosses(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,const GDRCONFIG& config);
   virtual Float64 GetLiftingLosses(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,const GDRCONFIG& config);
   virtual Float64 GetShippingLosses(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,const GDRCONFIG& config);
   virtual Float64 GetAfterTemporaryStrandInstallationLosses(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,const GDRCONFIG& config);
   virtual Float64 GetBeforeTemporaryStrandRemovalLosses(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,const GDRCONFIG& config);
   virtual Float64 GetAfterTemporaryStrandRemovalLosses(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,const GDRCONFIG& config);
   virtual Float64 GetDeckPlacementLosses(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,const GDRCONFIG& config);
   virtual Float64 GetSIDLLosses(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,const GDRCONFIG& config);
   virtual Float64 GetFinal(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,const GDRCONFIG& config);
   virtual LOSSDETAILS GetLossDetails(const pgsPointOfInterest& poi,const GDRCONFIG& config);
   virtual void ClearDesignLosses();

// IPrestressForce
public:
   virtual Float64 GetPjackMax(SpanIndexType span,GirderIndexType gdr,pgsTypes::StrandType strandType,StrandIndexType nStrands);
   virtual Float64 GetPjackMax(SpanIndexType span,GirderIndexType gdr,const matPsStrand& strand,StrandIndexType nStrands);

   virtual Float64 GetXferLength(SpanIndexType span,GirderIndexType gdr,pgsTypes::StrandType strandType);
   virtual Float64 GetDevLength(const pgsPointOfInterest& poi,bool bDebonded);
   virtual STRANDDEVLENGTHDETAILS GetDevLengthDetails(const pgsPointOfInterest& poi,bool bDebonded);
   virtual Float64 GetStrandBondFactor(const pgsPointOfInterest& poi,StrandIndexType strandIdx,pgsTypes::StrandType strandType);
   virtual Float64 GetStrandBondFactor(const pgsPointOfInterest& poi,const GDRCONFIG& config,StrandIndexType strandIdx,pgsTypes::StrandType strandType);
   virtual Float64 GetStrandBondFactor(const pgsPointOfInterest& poi,StrandIndexType strandIdx,pgsTypes::StrandType strandType,Float64 fps,Float64 fpe);
   virtual Float64 GetStrandBondFactor(const pgsPointOfInterest& poi,const GDRCONFIG& config,StrandIndexType strandIdx,pgsTypes::StrandType strandType,Float64 fps,Float64 fpe);

   virtual Float64 GetHoldDownForce(SpanIndexType span,GirderIndexType gdr);
   virtual Float64 GetHoldDownForce(SpanIndexType span,GirderIndexType gdr,const GDRCONFIG& config);

   virtual Float64 GetPrestressForce(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,pgsTypes::LossStage lossStage);
   virtual Float64 GetPrestressForce(const pgsPointOfInterest& poi,const GDRCONFIG& config,pgsTypes::StrandType strandType,pgsTypes::LossStage lossStage);

   virtual Float64 GetPrestressForcePerStrand(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,pgsTypes::LossStage lossStage);
   virtual Float64 GetPrestressForcePerStrand(const pgsPointOfInterest& poi,const GDRCONFIG& config,pgsTypes::StrandType strandType,pgsTypes::LossStage lossStage);

   virtual Float64 GetHorizHarpedStrandForce(const pgsPointOfInterest& poi,pgsTypes::LossStage lossStage);
   virtual Float64 GetHorizHarpedStrandForce(const pgsPointOfInterest& poi,const GDRCONFIG& config,pgsTypes::LossStage lossStage);

   virtual Float64 GetVertHarpedStrandForce(const pgsPointOfInterest& poi,pgsTypes::LossStage lossStage);
   virtual Float64 GetVertHarpedStrandForce(const pgsPointOfInterest& poi,const GDRCONFIG& config,pgsTypes::LossStage lossStage);

   virtual Float64 GetStrandForce(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,pgsTypes::LossStage lossStage);
   virtual Float64 GetStrandForce(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,const GDRCONFIG& config,pgsTypes::LossStage lossStage);

   virtual Float64 GetStrandStress(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,pgsTypes::LossStage lossStage);
   virtual Float64 GetStrandStress(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,const GDRCONFIG& config,pgsTypes::LossStage lossStage);


// ILiveLoadDistributionFactors
public:
   virtual void VerifyDistributionFactorRequirements(const pgsPointOfInterest& poi);
   virtual Float64 GetMomentDistFactor(SpanIndexType span,GirderIndexType gdr,pgsTypes::LimitState ls);
   virtual Float64 GetNegMomentDistFactor(SpanIndexType span,GirderIndexType gdr,pgsTypes::LimitState ls);
   virtual Float64 GetNegMomentDistFactorAtPier(PierIndexType pier,GirderIndexType gdr,pgsTypes::LimitState ls,pgsTypes::PierFaceType pierFace);
   virtual Float64 GetShearDistFactor(SpanIndexType span,GirderIndexType gdr,pgsTypes::LimitState ls);
   virtual Float64 GetReactionDistFactor(PierIndexType pier,GirderIndexType gdr,pgsTypes::LimitState ls);
   virtual Float64 GetMomentDistFactor(SpanIndexType span,GirderIndexType gdr,pgsTypes::LimitState ls,Float64 fcgdr);
   virtual Float64 GetNegMomentDistFactor(SpanIndexType span,GirderIndexType gdr,pgsTypes::LimitState ls,Float64 fcgdr);
   virtual Float64 GetNegMomentDistFactorAtPier(PierIndexType pier,GirderIndexType gd,pgsTypes::LimitState lsr,pgsTypes::PierFaceType pierFace,Float64 fcgdr);
   virtual Float64 GetShearDistFactor(SpanIndexType span,GirderIndexType gdr,pgsTypes::LimitState ls,Float64 fcgdr);
   virtual Float64 GetReactionDistFactor(PierIndexType pier,GirderIndexType gdr,pgsTypes::LimitState ls,Float64 fcgdr);
   virtual void GetNegMomentDistFactorPoints(SpanIndexType span,GirderIndexType gdr,double* dfPoints,Uint32* nPoints);
   virtual void GetDistributionFactors(const pgsPointOfInterest& poi,pgsTypes::LimitState ls,double* pM,double* nM,double* V);
   virtual void GetDistributionFactors(const pgsPointOfInterest& poi,pgsTypes::LimitState ls,double fcgdr,double* pM,double* nM,double* V);
   virtual void ReportDistributionFactors(SpanIndexType span,GirderIndexType gdr,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits);
   virtual bool Run1250Tests(SpanIndexType span,GirderIndexType gdr,pgsTypes::LimitState ls,LPCTSTR pid,LPCTSTR bridgeId,std::_tofstream& resultsFile, std::_tofstream& poiFile);
   virtual Uint32 GetNumberOfDesignLanes(SpanIndexType span);
   virtual Uint32 GetNumberOfDesignLanesEx(SpanIndexType span,Float64* pDistToSection,Float64* pCurbToCurb);
   virtual bool GetDFResultsEx(SpanIndexType span,GirderIndexType gdr,pgsTypes::LimitState ls,
                       Float64* gpM, Float64* gpM1, Float64* gpM2,  // pos moment
                       Float64* gnM, Float64* gnM1, Float64* gnM2,  // neg moment, ahead face
                       Float64* gV,  Float64* gV1,  Float64* gV2,   // shear
                       Float64* gR,  Float64* gR1,  Float64* gR2 ); // reaction

// IMomentCapacity
public:
   virtual Float64 GetMomentCapacity(pgsTypes::Stage stage,const pgsPointOfInterest& poi,bool bPositiveMoment);
   virtual std::vector<Float64> GetMomentCapacity(pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,bool bPositiveMoment);
   virtual void GetMomentCapacityDetails(pgsTypes::Stage stage,const pgsPointOfInterest& poi,bool bPositiveMoment,MOMENTCAPACITYDETAILS* pmcd);
   virtual void GetMomentCapacityDetails(pgsTypes::Stage stage,const pgsPointOfInterest& poi,const GDRCONFIG& config,bool bPositiveMoment,MOMENTCAPACITYDETAILS* pmcd);
   virtual Float64 GetCrackingMoment(pgsTypes::Stage stage,const pgsPointOfInterest& poi,bool bPositiveMoment);
   virtual void GetCrackingMomentDetails(pgsTypes::Stage stage,const pgsPointOfInterest& poi,bool bPositiveMoment,CRACKINGMOMENTDETAILS* pcmd);
   virtual void GetCrackingMomentDetails(pgsTypes::Stage stage,const pgsPointOfInterest& poi,const GDRCONFIG& config,bool bPositiveMoment,CRACKINGMOMENTDETAILS* pcmd);
   virtual Float64 GetMinMomentCapacity(pgsTypes::Stage stage,const pgsPointOfInterest& poi,bool bPositiveMoment);
   virtual void GetMinMomentCapacityDetails(pgsTypes::Stage stage,const pgsPointOfInterest& poi,bool bPositiveMoment,MINMOMENTCAPDETAILS* pmmcd);
   virtual void GetMinMomentCapacityDetails(pgsTypes::Stage stage,const pgsPointOfInterest& poi,const GDRCONFIG& config,bool bPositiveMoment,MINMOMENTCAPDETAILS* pmmcd);
   virtual std::vector<MOMENTCAPACITYDETAILS> GetMomentCapacityDetails(pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,bool bPositiveMoment);
   virtual std::vector<MINMOMENTCAPDETAILS> GetMinMomentCapacityDetails(pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,bool bPositiveMoment);
   virtual std::vector<CRACKINGMOMENTDETAILS> GetCrackingMomentDetails(pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,bool bPositiveMoment);
   virtual std::vector<Float64> GetCrackingMoment(pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,bool bPositiveMoment);
   virtual std::vector<Float64> GetMinMomentCapacity(pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,bool bPositiveMoment);

// IShearCapacity
public:
   virtual Float64 GetShearCapacity(pgsTypes::LimitState ls, pgsTypes::Stage stage,const pgsPointOfInterest& poi);
   virtual std::vector<Float64> GetShearCapacity(pgsTypes::LimitState ls, pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi);
   virtual void GetShearCapacityDetails(pgsTypes::LimitState ls, pgsTypes::Stage stage,const pgsPointOfInterest& poi,SHEARCAPACITYDETAILS* pmcd);
   virtual void GetRawShearCapacityDetails(pgsTypes::LimitState ls, pgsTypes::Stage stage,const pgsPointOfInterest& poi,SHEARCAPACITYDETAILS* pmcd);
   virtual Float64 GetFpc(const pgsPointOfInterest& poi);
   virtual void GetFpcDetails(const pgsPointOfInterest& poi, FPCDETAILS* pmcd);
   virtual Float64 GetShearCapacity(pgsTypes::LimitState ls, pgsTypes::Stage stage,const pgsPointOfInterest& poi,const GDRCONFIG& config);
   virtual void GetShearCapacityDetails(pgsTypes::LimitState ls, pgsTypes::Stage stage,const pgsPointOfInterest& poi,const GDRCONFIG& config,SHEARCAPACITYDETAILS* pmcd);
   virtual void GetRawShearCapacityDetails(pgsTypes::LimitState ls, pgsTypes::Stage stage,const pgsPointOfInterest& poi,const GDRCONFIG& config,SHEARCAPACITYDETAILS* pmcd);
   virtual Float64 GetFpc(const pgsPointOfInterest& poi,const GDRCONFIG& config);
   virtual void GetFpcDetails(const pgsPointOfInterest& poi, const GDRCONFIG& config,FPCDETAILS* pmcd);
   virtual void GetCriticalSection(pgsTypes::LimitState limitState,SpanIndexType span,GirderIndexType gdr,Float64* pLeft,Float64* pRight);
   virtual void GetCriticalSection(pgsTypes::LimitState limitState,SpanIndexType span,GirderIndexType gdr,const GDRCONFIG& config,Float64* pLeft,Float64* pRight);
   virtual void GetCriticalSectionDetails(pgsTypes::LimitState limitState,SpanIndexType span,GirderIndexType gdr,CRITSECTDETAILS* pDetails);
   virtual void GetCriticalSectionDetails(pgsTypes::LimitState limitState,SpanIndexType span,GirderIndexType gdr,const GDRCONFIG& config,CRITSECTDETAILS* pDetails);
   virtual std::vector<SHEARCAPACITYDETAILS> GetShearCapacityDetails(pgsTypes::LimitState ls, pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi);

// IGirderHaunch
public:
   virtual Float64 GetRequiredSlabOffset(SpanIndexType span,GirderIndexType gdr);
   virtual void GetHaunchDetails(SpanIndexType span,GirderIndexType gdr,HAUNCHDETAILS* pDetails);

// IFabricationOptimization
public:
   virtual void GetFabricationOptimizationDetails(SpanIndexType span,GirderIndexType gdr,FABRICATIONOPTIMIZATIONDETAILS* pDetails);

// IArtifact
public:
   virtual const pgsGirderArtifact* GetArtifact(SpanIndexType span,GirderIndexType gdr);
   virtual const pgsDesignArtifact* CreateDesignArtifact(SpanIndexType span,GirderIndexType gdr,arDesignOptions design);
   virtual const pgsDesignArtifact* GetDesignArtifact(SpanIndexType span,GirderIndexType gdr);
   virtual void CreateLiftingAnalysisArtifact(SpanIndexType span,GirderIndexType gdr,Float64 supportLoc,pgsLiftingAnalysisArtifact* pArtifact);
   virtual void CreateHaulingAnalysisArtifact(SpanIndexType span,GirderIndexType gdr,Float64 leftSupportLoc,Float64 rightSupportLoc,pgsHaulingAnalysisArtifact* pArtifact);
   virtual const pgsRatingArtifact* GetRatingArtifact(GirderIndexType gdrLineIdx,pgsTypes::LoadRatingType ratingType,VehicleIndexType vehicleIndex);

// ICrackedSection
public:
   virtual void GetCrackedSectionDetails(const pgsPointOfInterest& poi,bool bPositiveMoment,CRACKEDSECTIONDETAILS* pCSD);
   virtual Float64 GetIcr(const pgsPointOfInterest& poi,bool bPositiveMoment);
   virtual std::vector<CRACKEDSECTIONDETAILS> GetCrackedSectionDetails(const std::vector<pgsPointOfInterest>& vPoi,bool bPositiveMoment);

// IBridgeDescriptionEventSink
public:
   virtual HRESULT OnBridgeChanged();
   virtual HRESULT OnGirderFamilyChanged();
   virtual HRESULT OnGirderChanged(SpanIndexType span,GirderIndexType gdr,Uint32 lHint);
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

   // Losses are cached for two different cases:
   // 1) This data structure caches losses for the current project data
   std::map<PoiKey,LOSSDETAILS> m_PsLosses;

   // 2) This data structure is for design cases. It caches the most recently
   //    computed losses
   class DesignLosses
   {
      struct Losses
      {
         GDRCONFIG m_Config;
         LOSSDETAILS m_Details;
      };

      std::map<PoiKey,Losses> m_Losses;
      
      public:
      DesignLosses()
      {
         Invalidate();
      }

      void Invalidate()
      {
         m_Losses.clear();
         m_Losses = std::map<PoiKey,Losses>();
      }

      bool GetFromCache(const pgsPointOfInterest& poi, const GDRCONFIG& config, LOSSDETAILS* pLosses)
      {
         PoiKey key(poi.GetID(),poi);
         std::map<PoiKey,Losses>::iterator found;
         found = m_Losses.find( key );
         if ( found == m_Losses.end() )
           return false;

    
         Losses& losses = (*found).second;

         if ( config == losses.m_Config )
         {
            *pLosses = losses.m_Details;
            return true;
         }
         else
         {
            // the one that was found doesn't match for this POI, so remove it because
            // we have new values for this POI
            m_Losses.erase(found);
            return false;
         }
      }

      void SaveToCache(const pgsPointOfInterest& poi, const GDRCONFIG& config, const LOSSDETAILS& losses)
      {
         // don't cache if this is a temporary poi
         if ( poi.GetID() < 0 )
         {
            //ATLASSERT(false);
            return; 
         }

         Losses l;
         l.m_Config = config;
         l.m_Details = losses;

         PoiKey key(poi.GetID(),poi);
         std::pair<std::map<PoiKey,Losses>::iterator,bool> result = m_Losses.insert( std::make_pair(key,l) );
         ATLASSERT( result.second == true );
      }
   };

   DesignLosses m_DesignLosses;

   std::map<SpanGirderHashType,pgsGirderArtifact> m_CheckArtifacts;
   std::map<SpanGirderHashType,pgsDesignArtifact> m_DesignArtifacts;

   struct RatingArtifactKey
   {
      RatingArtifactKey(GirderIndexType gdrLineIdx,VehicleIndexType vehIdx)
      { GirderLineIdx = gdrLineIdx; VehicleIdx = vehIdx; }

      GirderIndexType GirderLineIdx;
      VehicleIndexType VehicleIdx;

      bool operator<(const RatingArtifactKey& other) const
      {
         if( GirderLineIdx < other.GirderLineIdx )
            return true;

         if( other.GirderLineIdx < GirderLineIdx)
            return false;

         if( VehicleIdx < other.VehicleIdx )
            return true;

         return false;
      }

   };
   std::map<RatingArtifactKey,pgsRatingArtifact> m_RatingArtifacts[6]; // pgsTypes::LoadRatingType enum as key

   std::map<PrestressPoiKey,double> m_PsForce;

   pgsPsForceEng             m_PsForceEngineer;
   pgsDesigner2              m_Designer;
   pgsLoadRater              m_LoadRater;
   pgsMomentCapacityEngineer m_MomentCapEngineer;
   pgsShearCapacityEngineer  m_ShearCapEngineer;
   CComPtr<IDistFactorEngineer> m_pDistFactorEngineer;

   pgsPointOfInterest GetEquivalentPointOfInterest(pgsTypes::Stage stage,const pgsPointOfInterest& poi);

   void BuildLosses(const pgsPointOfInterest& poi,LOSSDETAILS* pDetails);
   void BuildLosses(const pgsPointOfInterest& poi,const GDRCONFIG& config,LOSSDETAILS* pDetails);

   // index 0 = Moment Type (Positive = 0, Negative = 1)
   typedef std::map<PoiKey,MOMENTCAPACITYDETAILS> MomentCapacityDetailsContainer;
   const MOMENTCAPACITYDETAILS* ValidateMomentCapacity(pgsTypes::Stage stage,const pgsPointOfInterest& poi,bool bPositiveMoment);
   const MOMENTCAPACITYDETAILS* ValidateMomentCapacity(pgsTypes::Stage stage,const pgsPointOfInterest& poi,const GDRCONFIG& config,bool bPositiveMoment);
   MOMENTCAPACITYDETAILS ComputeMomentCapacity(pgsTypes::Stage stage,const pgsPointOfInterest& poi,bool bPositiveMoment);
   MOMENTCAPACITYDETAILS ComputeMomentCapacity(pgsTypes::Stage stage,const pgsPointOfInterest& poi,const GDRCONFIG& config,bool bPositiveMoment);
   const MOMENTCAPACITYDETAILS* GetCachedMomentCapacity(pgsTypes::Stage stage,const pgsPointOfInterest& poi,bool bPositiveMoment);
   const MOMENTCAPACITYDETAILS* GetCachedMomentCapacity(pgsTypes::Stage stage,const pgsPointOfInterest& poi,const GDRCONFIG& config,bool bPositiveMoment);
   const MOMENTCAPACITYDETAILS* GetCachedMomentCapacity(pgsTypes::Stage stage,const pgsPointOfInterest& poi,bool bPositiveMoment,const MomentCapacityDetailsContainer& container);
   const MOMENTCAPACITYDETAILS* StoreMomentCapacityDetails(pgsTypes::Stage stage,const pgsPointOfInterest& poi,bool bPositiveMoment,const MOMENTCAPACITYDETAILS& mcd,MomentCapacityDetailsContainer& container);

   MomentCapacityDetailsContainer m_NonCompositeMomentCapacity[2];
   MomentCapacityDetailsContainer m_CompositeMomentCapacity[2];
   void InvalidateMomentCapacity();

   std::map<PoiKey,CRACKINGMOMENTDETAILS> m_NonCompositeCrackingMoment[2];
   std::map<PoiKey,CRACKINGMOMENTDETAILS> m_CompositeCrackingMoment[2];
   const CRACKINGMOMENTDETAILS* ValidateCrackingMoments(pgsTypes::Stage stage,const pgsPointOfInterest& poi,bool bPositiveMoment);

   // index 0 = Moment Type (Positive = 0, Negative = 1)
   std::map<PoiKey,MINMOMENTCAPDETAILS> m_NonCompositeMinMomentCapacity[2];
   std::map<PoiKey,MINMOMENTCAPDETAILS> m_CompositeMinMomentCapacity[2];
   const MINMOMENTCAPDETAILS* ValidateMinMomentCapacity(pgsTypes::Stage stage,const pgsPointOfInterest& poi,bool bPositiveMoment);

   typedef std::map<PoiKey,CRACKEDSECTIONDETAILS> CrackedSectionDetailsContainer;
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
   std::map<PoiKey,SHEARCAPACITYDETAILS> m_ShearCapacity[8]; // use the LimitStateToShearIndex method to map limit state to array index
   const SHEARCAPACITYDETAILS* ValidateShearCapacity(pgsTypes::LimitState ls, pgsTypes::Stage stage,const pgsPointOfInterest& poi);
   void InvalidateShearCapacity();
   std::map<PoiKey,FPCDETAILS> m_Fpc;
   const FPCDETAILS* ValidateFpc(const pgsPointOfInterest& poi);
   void InvalidateFpc();

   void GetCriticalSectionFromDetails(const CRITSECTDETAILS& details,Float64* pLeft,Float64* pRight);

   // critical section for shear
   std::map<SpanGirderHashType,CRITSECTDETAILS> m_CritSectionDetails[8]; // use the LimitStateToShearIndex method to map limit state to array index
   const CRITSECTDETAILS* ValidateShearCritSection(pgsTypes::LimitState limitState,SpanIndexType span, GirderIndexType gdr);
   void CalculateShearCritSection(pgsTypes::LimitState limitState,SpanIndexType span,GirderIndexType gdr,CRITSECTDETAILS* pDetails);
   void CalculateShearCritSection(pgsTypes::LimitState limitState,SpanIndexType span,GirderIndexType gdr,const GDRCONFIG& config,CRITSECTDETAILS* pDetails);
   void CalculateShearCritSection(pgsTypes::LimitState limitState,SpanIndexType span,GirderIndexType gdr,bool bUseConfig,const GDRCONFIG& config,CRITSECTDETAILS* pDetails);
   void InvalidateShearCritSection();

   std::map<SpanGirderHashType,HAUNCHDETAILS> m_HaunchDetails;

   // Lifting analysis artifacts
   std::map<SpanGirderHashType, std::map<Float64,pgsLiftingAnalysisArtifact,Float64_less> > m_LiftingArtifacts;
   std::map<SpanGirderHashType, std::map<Float64,pgsHaulingAnalysisArtifact,Float64_less> > m_HaulingArtifacts;

   // Event Sink Cookies
   DWORD m_dwBridgeDescCookie;
   DWORD m_dwSpecificationCookie;
   DWORD m_dwRatingSpecificationCookie;
   DWORD m_dwLoadModifiersCookie;
   DWORD m_dwEnvironmentCookie;

   void InvalidateAll();
   void ValidateLosses(const pgsPointOfInterest& poi);
   void InvalidateHaunch();
   void InvalidateLosses();
   void ValidateLiveLoadDistributionFactors(SpanIndexType span,GirderIndexType gdr);
   void InvalidateLiveLoadDistributionFactors();
   void ValidateArtifacts(SpanIndexType span,GirderIndexType gdr);
   void ValidateRatingArtifacts(GirderIndexType gdrLineIdx,pgsTypes::LoadRatingType ratingType,VehicleIndexType vehicleIndex);
   void InvalidateArtifacts();
   void InvalidateRatingArtifacts();

   LOSSDETAILS* FindLosses(const pgsPointOfInterest& poi);
   pgsGirderArtifact* FindArtifact(SpanIndexType span,GirderIndexType gdr);
   pgsRatingArtifact* FindRatingArtifact(GirderIndexType gdrLineIdx,pgsTypes::LoadRatingType ratingType,VehicleIndexType vehicleIndex);

   DECLARE_LOGFILE;

   void CheckCurvatureRequirements(const pgsPointOfInterest& poi);
   void CheckGirderStiffnessRequirements(const pgsPointOfInterest& poi);
   void CheckParallelGirderRequirements(const pgsPointOfInterest& poi);
};

#endif //__ENGAGENT_H_
