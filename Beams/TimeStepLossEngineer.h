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

#include <IFace\PsLossEngineer.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Bridge.h>
#include <IFace\Intervals.h>
#include <IFace\Project.h>
#include <IFace\PrestressForce.h>
#include <IFace/Limits.h>
#include <EAF\EAFDisplayUnits.h>

#include <Beams/Interfaces.h>

#include <PgsExt\PoiKey.h>
#include <PgsExt\PTData.h>

#include <Math\LinearFunction.h>

#include <Plugins\CLSID.h>

typedef std::map<pgsPointOfInterest,LOSSDETAILS> SectionLossContainer;

/////////////////////////////////////////////////////////////////////////////
// CTimeStepLossEngineer
class ATL_NO_VTABLE CTimeStepLossEngineer : 
   public CComObjectRootEx<CComSingleThreadModel>,
   public CComCoClass<CTimeStepLossEngineer, &CLSID_TimeStepLossEngineer>,
   public IPsLossEngineer,
   public IInitialize
{
public:
	CTimeStepLossEngineer()
	{
	}

   HRESULT FinalConstruct();
   void FinalRelease();

DECLARE_REGISTRY_RESOURCEID(IDR_TIMESTEPLOSSENGINEER)

BEGIN_COM_MAP(CTimeStepLossEngineer)
   COM_INTERFACE_ENTRY(IPsLossEngineer)
   COM_INTERFACE_ENTRY(IInitialize)
END_COM_MAP()

   // IInitialize
public:
   virtual void SetBroker(IBroker* pBroker, StatusGroupIDType statusGroupID) override;

// IPsLossEngineer
public:
   virtual const LOSSDETAILS* GetLosses(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx) override;
   virtual const LOSSDETAILS* GetLosses(const pgsPointOfInterest& poi,const GDRCONFIG& config,IntervalIndexType intervalIdx) override;
   virtual void ClearDesignLosses() override;
   virtual void BuildReport(const CGirderKey& girderKey,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits) override;
   virtual void ReportFinalLosses(const CGirderKey& girderKey,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits) override;
   virtual const ANCHORSETDETAILS* GetGirderTendonAnchorSetDetails(const CGirderKey& girderKey,DuctIndexType ductIdx) override;
   virtual Float64 GetGirderTendonElongation(const CGirderKey& girderKey,DuctIndexType ductIdx,pgsTypes::MemberEndType endType) override;
   virtual void GetGirderTendonAverageFrictionAndAnchorSetLoss(const CGirderKey& girderKey,DuctIndexType ductIdx,Float64* pfpF,Float64* pfpA) override;
   virtual const ANCHORSETDETAILS* GetSegmentTendonAnchorSetDetails(const CSegmentKey& segmentKey, DuctIndexType ductIdx) override;
   virtual Float64 GetSegmentTendonElongation(const CSegmentKey& segmentKey, DuctIndexType ductIdx, pgsTypes::MemberEndType endType) override;
   virtual void GetSegmentTendonAverageFrictionAndAnchorSetLoss(const CSegmentKey& segmentKey, DuctIndexType ductIdx, Float64* pfpF, Float64* pfpA) override;

private:
   pgsTypes::BridgeAnalysisType m_Bat;

   StatusCallbackIDType m_scidProjectCriteria;

   // This are interfaces that are used over and over and over
   // Get them once so we don't have to call GET_IFACE so many times
   CComPtr<IProgress>          m_pProgress;
   CComPtr<IBridgeDescription> m_pBridgeDesc;
   CComPtr<IBridge>            m_pBridge;
   CComPtr<IStrandGeometry>    m_pStrandGeom;
   CComPtr<IGirderTendonGeometry>    m_pGirderTendonGeometry;
   CComPtr<ISegmentTendonGeometry>   m_pSegmentTendonGeometry;
   CComPtr<IIntervals>         m_pIntervals;
   CComPtr<ISectionProperties> m_pSectProp;
   CComPtr<IGirder>            m_pGirder;
   CComPtr<IMaterials>         m_pMaterials;
   CComPtr<IPretensionForce>   m_pPSForce;
   CComPtr<IPosttensionForce>  m_pPTForce;
   CComPtr<ILossParameters>    m_pLossParams;
   CComPtr<IPointOfInterest>   m_pPoi;
   CComPtr<ILongRebarGeometry> m_pRebarGeom;
   CComPtr<IProductLoads>      m_pProductLoads;
   CComPtr<IProductForces>     m_pProductForces;
   CComPtr<ICombinedForces>    m_pCombinedForces;
   CComPtr<IExternalLoading>   m_pExternalLoading;
   CComPtr<IEAFDisplayUnits>   m_pDisplayUnits;
   CComPtr<ILosses>            m_pLosses;
   CComPtr<IDuctLimits>        m_pDuctLimits;



   // keeps track of the strand types we are analyzing
   // no need to analyze types of strands that have a strand count of zero
   std::map<CSegmentKey,std::vector<pgsTypes::StrandType>> m_StrandTypes; 
   void InitializeStrandTypes(const CSegmentKey& segmentKey);
   const std::vector<pgsTypes::StrandType>& GetStrandTypes(const CSegmentKey& segmentKey);

   IBroker* m_pBroker; // must be a weak reference
   StatusGroupIDType m_StatusGroupID;

   struct LOSSES
   {
      std::vector<ANCHORSETDETAILS> GirderAnchorSet; // one for each duct
      std::map<CSegmentKey, std::vector<ANCHORSETDETAILS>> SegmentAnchorSet;
      SectionLossContainer SectionLosses;
   };

   std::map<CGirderKey,LOSSES> m_Losses;

   std::map<CGirderTendonKey,std::pair<Float64,Float64>> m_GirderTendonAvgFrictionAndAnchorSetLoss; // first in pair is friction, second is anchor set loss
   std::map<CGirderTendonKey, std::pair<Float64, Float64>> m_GirderTendonElongation; // first in pair is left end, second is right end
   
   std::map<CSegmentTendonKey, std::pair<Float64, Float64>> m_SegmentTendonAvgFrictionAndAnchorSetLoss; // first in pair is friction, second is anchor set loss
   std::map<CSegmentTendonKey, std::pair<Float64, Float64>> m_SegmentTendonElongation; // first in pair is left end, second is right end

   // computes losses for the specified girder for all intervals upto and including endAnalysisIntervalIdx
   void ComputeLosses(const CGirderKey& girderKey,IntervalIndexType endAnalysisIntervalIdx);
   void ComputeLosses(GirderIndexType girderLineIdx,IntervalIndexType endAnalysisIntervalIdx,std::vector<LOSSES*>* pvpLosses);

   void ComputeFrictionLosses(const CGirderKey& girderKey,LOSSES* pLosses);
   void ComputeFrictionLosses(const CPrecastSegmentData* pSegment, LOSSES* pLosses);
   void ComputeAnchorSetLosses(const CGirderKey& girderKey,LOSSES* pLosses);
   void ComputeAnchorSetLosses(const CPrecastSegmentData* pSegment, LOSSES* pLosses);

   void ComputeSectionLosses(GirderIndexType girderLineIdx,IntervalIndexType endAnalysisIntervalIdx,std::vector<LOSSES*>* pvpLosses);
   void InitializeTimeStepAnalysis(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,LOSSDETAILS& details);
   void AnalyzeInitialStrains(IntervalIndexType intervalIdx,const CGirderKey& girderKey,LOSSES* pLosses);
   void FinalizeTimeStepAnalysis(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,LOSSDETAILS& details);

   void ComputeAnchorSetLosses(const CPTData* pPTData,const CDuctData* pDuctData,DuctIndexType ductIdx,pgsTypes::MemberEndType endType,LOSSES* pLosses,Float64 Lg,SectionLossContainer::iterator& frMinIter,Float64* pdfpA,Float64* pdfpS,Float64* pXSet);
   void BoundAnchorSet(const CPTData* pPTData,const CDuctData* pDuctData,DuctIndexType ductIdx,pgsTypes::MemberEndType endType,Float64 Dset,LOSSES* pLosses,Float64 fpj,Float64 Lg,SectionLossContainer::iterator& frMinIter,Float64* pXsetMin,Float64* pDsetMin,Float64* pdfpATMin,Float64* pdfpSMin,Float64* pXsetMax,Float64* pDsetMax,Float64* pdfpATMax,Float64* pdfpSMax);
   Float64 EvaluateAnchorSet(const CPTData* pPTData,const CDuctData* pDuctData,DuctIndexType ductIdx,pgsTypes::MemberEndType endType,LOSSES* pLosses,Float64 fpj,Float64 Lg,SectionLossContainer::iterator& frMinIter,Float64 Xset,Float64* pdfpAT,Float64* pdfpS);

   void ComputeAnchorSetLosses(const CSegmentPTData* pPTData, const CSegmentDuctData* pDuctData, DuctIndexType ductIdx, pgsTypes::MemberEndType endType, LOSSES* pLosses, Float64 Ls, SectionLossContainer::iterator& frMinIter, Float64* pdfpA, Float64* pdfpS, Float64* pXSet);
   void BoundAnchorSet(const CSegmentPTData* pPTData, const CSegmentDuctData* pDuctData, DuctIndexType ductIdx, pgsTypes::MemberEndType endType, Float64 Dset, LOSSES* pLosses, Float64 fpj, Float64 Ls, SectionLossContainer::iterator& frMinIter, Float64* pXsetMin, Float64* pDsetMin, Float64* pdfpATMin, Float64* pdfpSMin, Float64* pXsetMax, Float64* pDsetMax, Float64* pdfpATMax, Float64* pdfpSMax);
   Float64 EvaluateAnchorSet(const CSegmentPTData* pPTData, const CSegmentDuctData* pDuctData, DuctIndexType ductIdx, pgsTypes::MemberEndType endType, LOSSES* pLosses, Float64 fpj, Float64 Ls, SectionLossContainer::iterator& frMinIter, Float64 Xset, Float64* pdfpAT, Float64* pdfpS);

   LOSSDETAILS* GetLossDetails(LOSSES* pLosses,const pgsPointOfInterest& poi);
   std::vector<pgsTypes::ProductForceType> GetApplicableProductLoads(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,bool bExternalForcesOnly=false);

   void GetAnalysisLocations(const CGirderKey& girderKey, PoiList* pPoiList);
   void GetAnalysisLocations(const CSegmentKey& segmentKey, PoiList* pPoiList);

   void ComputePrincipalStressInWeb(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi, pgsTypes::ProductForceType pfType, DuctIndexType nSegmentDucts, DuctIndexType nGirderDucts,TIME_STEP_DETAILS& tsDetails, const TIME_STEP_DETAILS* pPrevTsDetails);

   ISpecification::PrincipalWebStressCheckType m_PrincipalTensileStressCheckType;
   Float64 m_DuctDiameterNearnessFactor;


   CSegmentKey m_SegmentKey; // segment for which we are currently computing deflections
};
