///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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
#include <PgsExt\PoiKey.h>

#include <PgsExt\PTData.h>

#include <Math\LinFunc2d.h>

// {26275720-66E8-40f6-A4C3-79404FB64968}
DEFINE_GUID(CLSID_TimeStepLossEngineer, 
0x26275720, 0x66e8, 0x40f6, 0xa4, 0xc3, 0x79, 0x40, 0x4f, 0xb6, 0x49, 0x68);

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

BEGIN_COM_MAP(CTimeStepLossEngineer)
   COM_INTERFACE_ENTRY(IPsLossEngineer)
END_COM_MAP()

public:
   virtual void SetBroker(IBroker* pBroker,StatusGroupIDType statusGroupID);
   virtual const LOSSDETAILS* GetLosses(const pgsPointOfInterest& poi);
   virtual const LOSSDETAILS* GetLosses(const pgsPointOfInterest& poi,const GDRCONFIG& config);
   virtual void ClearDesignLosses();
   virtual void BuildReport(const CGirderKey& girderKey,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits);
   virtual void ReportFinalLosses(const CGirderKey& girderKey,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits);
   virtual const ANCHORSETDETAILS* GetAnchorSetDetails(const CGirderKey& girderKey,DuctIndexType ductIdx);
   virtual Float64 GetElongation(const CGirderKey& girderKey,DuctIndexType ductIdx,pgsTypes::MemberEndType endType);

private:
   IBroker* m_pBroker;
   StatusGroupIDType m_StatusGroupID;

   struct LOSSES
   {
      std::vector<ANCHORSETDETAILS> AnchorSet;
      std::map<pgsPointOfInterest,LOSSDETAILS> SectionLosses;
   };

   std::map<CGirderKey,LOSSES> m_Losses;


   void ComputeLosses(const CGirderKey& girderKey);
   void ComputeFrictionLosses(const CGirderKey& girderKey,LOSSES* pLosses);
   void ComputeAnchorSetLosses(const CGirderKey& girderKey,LOSSES* pLosses);
   void ComputeSectionLosses(const CGirderKey& girderKey,LOSSES* pLosses);
   void InitializeTimeStepAnalysis(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,LOSSDETAILS& details);
   void AnalyzeInitialStrains(IntervalIndexType intervalIdx,const CGirderKey& girderKey,LOSSES* pLosses);
   void FinalizeTimeStepAnalysis(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,LOSSDETAILS& details);
   void ComputeDeflections(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,LOSSES* pLosses);
   void ComputeIncrementalDeflections(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,ProductForceType pfType,const std::vector<pgsPointOfInterest>& vAllPoi,const std::vector<Float64>& unitLoadMomentsPreviousInterval,const std::vector<Float64>& unitLoadMomentsThisInterval,LOSSES* pLosses);
   void ComputeIncrementalDeflections(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,ProductForceType pfType,const std::vector<pgsPointOfInterest>& vAllPoi,const std::vector<Float64>& unitLoadMoments,LOSSES* pLosses);

   void ComputeAnchorSetLosses(const CPTData* pPTData,const CDuctData* pDuctData,DuctIndexType ductIdx,pgsTypes::MemberEndType endType,LOSSES* pLosses,Float64 Lg,std::map<pgsPointOfInterest,LOSSDETAILS>::iterator& frMinIter,Float64* pdfpA,Float64* pdfpS,Float64* pXSet);
   void BoundAnchorSet(const CPTData* pPTData,const CDuctData* pDuctData,DuctIndexType ductIdx,pgsTypes::MemberEndType endType,Float64 Dset,LOSSES* pLosses,Float64 fpj,Float64 Lg,std::map<pgsPointOfInterest,LOSSDETAILS>::iterator& frMinIter,Float64* pXsetMin,Float64* pDsetMin,Float64* pdfpATMin,Float64* pdfpSMin,Float64* pXsetMax,Float64* pDsetMax,Float64* pdfpATMax,Float64* pdfpSMax);
   Float64 EvaluateAnchorSet(const CPTData* pPTData,const CDuctData* pDuctData,DuctIndexType ductIdx,pgsTypes::MemberEndType endType,LOSSES* pLosses,Float64 fpj,Float64 Lg,std::map<pgsPointOfInterest,LOSSDETAILS>::iterator& frMinIter,Float64 Xset,Float64* pdfpAT,Float64* pdfpS);
   LOSSDETAILS* GetLossDetails(LOSSES* pLosses,const pgsPointOfInterest& poi);
   std::vector<ProductForceType> GetApplicableProductLoads(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,bool bExternalForcesOnly=false);
   void MakeClosureJointAdjustment(IntervalIndexType intervalIdx,const pgsPointOfInterest& leftPoi,const pgsPointOfInterest& closurePoi,const pgsPointOfInterest& rightPoi,LOSSES* pLosses);


   void InitializeDeflectionCalculations();
   void InitializeErectionAdjustments(IntervalIndexType intervalIdx,const CSegmentKey& segmentKey,LOSSES* pLosses);
   Float64 GetErectionAdjustment(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,ProductForceType pfType);

   CSegmentKey m_SegmentKey; // segment for which we are currently computing deflections
   mathLinFunc2d m_ErectionAdjustment[3]; // adjusts to inelastic deformations due to rigid body movement during erection
                                          // access the arrary with 0 = creep, 1 = shrinkage, 2 = relaxation
};
