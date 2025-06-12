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

#include <Beams/BeamsExp.h>
#include <IFace\PsLossEngineer.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Bridge.h>
#include <IFace\Intervals.h>
#include <IFace\Project.h>
#include <IFace\PrestressForce.h>
#include <IFace/Limits.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace/PointOfInterest.h>

#include <Beams/Interfaces.h>

#include <PgsExt\PoiKey.h>

#include <PsgLib\PTData.h>

#include <Math\LinearFunction.h>

#include <Plugins\CLSID.h>

namespace PGS
{
   namespace Beams
   {
      using SectionLossContainer = std::map<pgsPointOfInterest, LOSSDETAILS>;

      class BEAMSCLASS TimeStepLossEngineer : public PsLossEngineerBase
      {
      public:
         TimeStepLossEngineer(std::weak_ptr<WBFL::EAF::Broker> pBroker, StatusGroupIDType statusGroupID);

      // PsLossEngineerBase
      public:
         const LOSSDETAILS* GetLosses(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx) override;
         const LOSSDETAILS* GetLosses(const pgsPointOfInterest& poi,const GDRCONFIG& config,IntervalIndexType intervalIdx) override;
         void ClearDesignLosses() override;
         void BuildReport(const CGirderKey& girderKey,rptChapter* pChapter,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) override;
         void ReportFinalLosses(const CGirderKey& girderKey,rptChapter* pChapter,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) override;
         const ANCHORSETDETAILS* GetGirderTendonAnchorSetDetails(const CGirderKey& girderKey,DuctIndexType ductIdx) override;
         Float64 GetGirderTendonElongation(const CGirderKey& girderKey,DuctIndexType ductIdx,pgsTypes::MemberEndType endType) override;
         void GetGirderTendonAverageFrictionAndAnchorSetLoss(const CGirderKey& girderKey,DuctIndexType ductIdx,Float64* pfpF,Float64* pfpA) override;
         const ANCHORSETDETAILS* GetSegmentTendonAnchorSetDetails(const CSegmentKey& segmentKey, DuctIndexType ductIdx) override;
         Float64 GetSegmentTendonElongation(const CSegmentKey& segmentKey, DuctIndexType ductIdx, pgsTypes::MemberEndType endType) override;
         void GetSegmentTendonAverageFrictionAndAnchorSetLoss(const CSegmentKey& segmentKey, DuctIndexType ductIdx, Float64* pfpF, Float64* pfpA) override;

      private:
         pgsTypes::BridgeAnalysisType m_Bat;

         StatusCallbackIDType m_scidProjectCriteria;

         // This are interfaces that are used over and over and over
         // Get them once so we don't have to call GET_IFACE so many times.
         // Generally, it is bad to hold onto shared_ptr for Agent/Broker,
         // however, in this case, the scope of these pointers is controled in the
         // ComputeLosses function. That is, the are held as shared_ptr for
         // only for the duration of the function call.
         std::shared_ptr<IEAFProgress>          m_pProgress;
         std::shared_ptr<IBridgeDescription> m_pBridgeDesc;
         std::shared_ptr<IBridge>            m_pBridge;
         std::shared_ptr<IStrandGeometry>    m_pStrandGeom;
         std::shared_ptr<IGirderTendonGeometry>    m_pGirderTendonGeometry;
         std::shared_ptr<ISegmentTendonGeometry>   m_pSegmentTendonGeometry;
         std::shared_ptr<IIntervals>         m_pIntervals;
         std::shared_ptr<ISectionProperties> m_pSectProp;
         std::shared_ptr<IGirder>            m_pGirder;
         std::shared_ptr<IMaterials>         m_pMaterials;
         std::shared_ptr<IPretensionForce>   m_pPSForce;
         std::shared_ptr<IPosttensionForce>  m_pPTForce;
         std::shared_ptr<ILossParameters>    m_pLossParams;
         std::shared_ptr<IPointOfInterest>   m_pPoi;
         std::shared_ptr<ILongRebarGeometry> m_pRebarGeom;
         std::shared_ptr<IProductLoads>      m_pProductLoads;
         std::shared_ptr<IProductForces>     m_pProductForces;
         std::shared_ptr<ICombinedForces>    m_pCombinedForces;
         std::shared_ptr<IExternalLoading>   m_pExternalLoading;
         std::shared_ptr<IEAFDisplayUnits>   m_pDisplayUnits;
         std::shared_ptr<ILosses>            m_pLosses;
         std::shared_ptr<IDuctLimits>        m_pDuctLimits;



         // keeps track of the strand types we are analyzing
         // no need to analyze types of strands that have a strand count of zero
         std::map<CSegmentKey,std::vector<pgsTypes::StrandType>> m_StrandTypes; 
         void InitializeStrandTypes(const CSegmentKey& segmentKey);
         const std::vector<pgsTypes::StrandType>& GetStrandTypes(const CSegmentKey& segmentKey);

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
   };
};
