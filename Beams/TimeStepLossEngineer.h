///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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
#include <EAF\EAFDisplayUnits.h>

#include <PgsExt\PoiKey.h>
#include <PgsExt\PTData.h>

#include <Math\LinFunc2d.h>

#include <Plugins\CLSID.h>

class ComparePoi
{
public:
   bool operator()(const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2) const
   {
      if ( poi1.AtSamePlace(poi2) )
      {
         if ( poi1.GetID() == INVALID_ID || poi2.GetID() == INVALID_ID )
         {
            // never match an invalid ID
            return false;
         }

         return poi1.GetID() < poi2.GetID();
      }
      else
      {
         return poi1 < poi2;
      }
   }
};

typedef std::map<pgsPointOfInterest,LOSSDETAILS,ComparePoi> SectionLossContainer;

/////////////////////////////////////////////////////////////////////////////
// CTimeStepLossEngineer
class ATL_NO_VTABLE CTimeStepLossEngineer : 
   public CComObjectRootEx<CComSingleThreadModel>,
   public CComCoClass<CTimeStepLossEngineer, &CLSID_TimeStepLossEngineer>,
   public IPsLossEngineer
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
END_COM_MAP()

public:
   virtual void SetBroker(IBroker* pBroker,StatusGroupIDType statusGroupID);
   virtual const LOSSDETAILS* GetLosses(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx);
   virtual const LOSSDETAILS* GetLosses(const pgsPointOfInterest& poi,const GDRCONFIG& config,IntervalIndexType intervalIdx);
   virtual void ClearDesignLosses();
   virtual void BuildReport(const CGirderKey& girderKey,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits);
   virtual void ReportFinalLosses(const CGirderKey& girderKey,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits);
   virtual const ANCHORSETDETAILS* GetAnchorSetDetails(const CGirderKey& girderKey,DuctIndexType ductIdx);
   virtual Float64 GetElongation(const CGirderKey& girderKey,DuctIndexType ductIdx,pgsTypes::MemberEndType endType);
   virtual void GetAverageFrictionAndAnchorSetLoss(const CGirderKey& girderKey,DuctIndexType ductIdx,Float64* pfpF,Float64* pfpA);

private:
   pgsTypes::BridgeAnalysisType m_Bat;

   std::_tstring m_strLoadingName[3]; // contains the loading names for the artifical restraint loads

   StatusCallbackIDType m_scidProjectCriteria;

   // This are interfaces that are used over and over and over
   // Get them once so we don't have to call GET_IFACE so many times
   CComPtr<IProgress>          m_pProgress;
   CComPtr<IBridgeDescription> m_pBridgeDesc;
   CComPtr<IBridge>            m_pBridge;
   CComPtr<IStrandGeometry>    m_pStrandGeom;
   CComPtr<ITendonGeometry>    m_pTendonGeom;
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


   std::map<CSegmentKey,std::vector<pgsTypes::StrandType>> m_StrandTypes; // keeps track of the strand types we are analyzing
   // no need to analyze types of strands that have a strand count of zero
   void InitializeStrandTypes(const CSegmentKey& segmentKey);
   const std::vector<pgsTypes::StrandType>& GetStrandTypes(const CSegmentKey& segmentKey);

   IBroker* m_pBroker; // must be a weak reference
   StatusGroupIDType m_StatusGroupID;

   struct LOSSES
   {
      std::vector<ANCHORSETDETAILS> AnchorSet; // one for each duct
      SectionLossContainer SectionLosses;
   };

   std::map<CGirderKey,LOSSES> m_Losses;

   std::map<CTendonKey,std::pair<Float64,Float64>> m_AvgFrictionAndAnchorSetLoss; // first is friction, second is anchor set loss
   std::map<CTendonKey,std::pair<Float64,Float64>> m_Elongation; // first in pair is left end, second is right end

   void ComputeLosses(const CGirderKey& girderKey,IntervalIndexType endAnalysisIntervalIdx);
   void ComputeLosses(const CGirderKey& girderKey,IntervalIndexType endAnalysisIntervalIdx,LOSSES* pLosses);
   void ComputeFrictionLosses(const CGirderKey& girderKey,LOSSES* pLosses);
   void ComputeAnchorSetLosses(const CGirderKey& girderKey,LOSSES* pLosses);
   void ComputeSectionLosses(const CGirderKey& girderKey,IntervalIndexType endAnalysisIntervalIdx,LOSSES* pLosses);
   void InitializeTimeStepAnalysis(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,LOSSDETAILS& details);
   void AnalyzeInitialStrains(IntervalIndexType intervalIdx,const CGirderKey& girderKey,LOSSES* pLosses);
   void FinalizeTimeStepAnalysis(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,LOSSDETAILS& details);

   void ComputeAnchorSetLosses(const CPTData* pPTData,const CDuctData* pDuctData,DuctIndexType ductIdx,pgsTypes::MemberEndType endType,LOSSES* pLosses,Float64 Lg,SectionLossContainer::iterator& frMinIter,Float64* pdfpA,Float64* pdfpS,Float64* pXSet);
   void BoundAnchorSet(const CPTData* pPTData,const CDuctData* pDuctData,DuctIndexType ductIdx,pgsTypes::MemberEndType endType,Float64 Dset,LOSSES* pLosses,Float64 fpj,Float64 Lg,SectionLossContainer::iterator& frMinIter,Float64* pXsetMin,Float64* pDsetMin,Float64* pdfpATMin,Float64* pdfpSMin,Float64* pXsetMax,Float64* pDsetMax,Float64* pdfpATMax,Float64* pdfpSMax);
   Float64 EvaluateAnchorSet(const CPTData* pPTData,const CDuctData* pDuctData,DuctIndexType ductIdx,pgsTypes::MemberEndType endType,LOSSES* pLosses,Float64 fpj,Float64 Lg,SectionLossContainer::iterator& frMinIter,Float64 Xset,Float64* pdfpAT,Float64* pdfpS);
   LOSSDETAILS* GetLossDetails(LOSSES* pLosses,const pgsPointOfInterest& poi);
   std::vector<ProductForceType> GetApplicableProductLoads(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,bool bExternalForcesOnly=false);

   int GetProductForceCount();

   CSegmentKey m_SegmentKey; // segment for which we are currently computing deflections
};
