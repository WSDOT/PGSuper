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

#include "StdAfx.h"
#include <Reporting\BearingTimeStepDetailsChapterBuilder.h>
#include <Reporting\BearingTimeStepDetailsReportSpecification.h>
#include <Reporting\ReactionInterfaceAdapters.h>

#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\PointOfInterest.h>
#include <IFace\PrestressForce.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Intervals.h>
#include <IFace\ReportOptions.h>

#include <WBFLGenericBridgeTools.h>

#include <PgsExt\TimelineEvent.h>
#include <IFace/BearingDesignParameters.h>

#include <string>
#include <Reporting/BearingTimeStepShearDeformationTable.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define DELTA_P    symbol(DELTA) << _T("P")
#define DELTA_M    symbol(DELTA) << _T("M")
#define DELTA_V    symbol(DELTA) << _T("V")
#define DELTA_Pk   symbol(DELTA) << Sub2(_T("P"),_T("k"))
#define DELTA_Mk   symbol(DELTA) << Sub2(_T("M"),_T("k"))
#define DELTA_E    symbol(DELTA) << Sub2(symbol(epsilon),_T("cr"))
#define DELTA_R    symbol(DELTA) << Sub2(symbol(varphi),_T("cr"))
#define DELTA_FR   symbol(DELTA) << RPT_STRESS(_T("r"))
#define DELTA_ER   symbol(DELTA) << Sub2(symbol(epsilon),_T("r"))
#define DELTA_ESH  symbol(DELTA) << Sub2(symbol(epsilon),_T("sh"))
#define CREEP(_a_,_b_) symbol(psi) << _T("(") << _a_ << _T(",") << _b_ << _T(")")
#define CREEP_tb_ti_ti  CREEP(Sub2(_T("t"),_T("b")) << _T(" - ") << Sub2(_T("t"),_T("i")),Sub2(_T("t"),_T("i"))) // Y(tb-ti,ti)
#define CREEP_te_ti_ti  CREEP(Sub2(_T("t"),_T("e")) << _T(" - ") << Sub2(_T("t"),_T("i")),Sub2(_T("t"),_T("i"))) // Y(te-ti,ti)
#define CREEP_tb_ti  CREEP(Sub2(_T("t"),_T("b")),Sub2(_T("t"),_T("i"))) // Y(tb,ti)
#define CREEP_te_ti  CREEP(Sub2(_T("t"),_T("e")),Sub2(_T("t"),_T("i"))) // Y(te,ti)
#define CREEP_tb_tla CREEP(Sub2(_T("t"),_T("b")),Sub2(_T("t"),_T("la")))
#define CREEP_te_tla CREEP(Sub2(_T("t"),_T("e")),Sub2(_T("t"),_T("la")))
#define CREEP_tb_to  CREEP(Sub2(_T("t"),_T("b")),Sub2(_T("t"),_T("o")))
#define CREEP_te_to  CREEP(Sub2(_T("t"),_T("e")),Sub2(_T("t"),_T("o")))

/****************************************************************************
CLASS
   CBearingTimeStepDetailsChapterBuilder
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CBearingTimeStepDetailsChapterBuilder::CBearingTimeStepDetailsChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================


LPCTSTR CBearingTimeStepDetailsChapterBuilder::GetName() const
{
   return TEXT("Bearing Time-Dependent Shear Deformations");
}

rptChapter* CBearingTimeStepDetailsChapterBuilder::Build(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,Uint16 level) const
{
   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);
   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   auto pBTSDRptSpec = std::dynamic_pointer_cast<const CBearingTimeStepDetailsReportSpecification>(pRptSpec);

   CComPtr<IBroker> pBroker;
   pBTSDRptSpec->GetBroker(&pBroker);
   const ReactionLocation& rptLocation(pBTSDRptSpec->GetReactionLocation());
   const CGirderKey& girderKey(rptLocation.GirderKey);


   GET_IFACE2(pBroker, ILossParameters, pLossParams);
   if ( pLossParams->GetLossMethod() != PrestressLossCriteria::LossMethodType::TIME_STEP )
   {
      *pPara << color(Red) << _T("Time Step analysis results not available.") << color(Black) << rptNewLine;
      return pChapter;
   }

   *pPara << rptNewLine;
   *pPara << _T("Incremental ") << symbol(epsilon) << _T(" = longitudinal shear strain from time-dependent effects occuring during this interval.") << rptNewLine;
   *pPara << _T("cumulative ") << symbol(epsilon) << _T(" = longitudinal shear strain from time-dependent effects occurring in all intervals up to and including interval.") << rptNewLine;
   *pPara << Sub2(symbol(delta), _T("d")) << _T(" = longitudinal distance between current and previous POI.") << rptNewLine;
   *pPara << Sub2(symbol(epsilon), _T("avg")) << _T(" = average cumulative longitudinal strain at current POI using the midpoint rule.") << rptNewLine;
   *pPara << rptNewLine;


   GET_IFACE2(pBroker, IBridge, pBridge);

   GET_IFACE2(pBroker, IIntervals, pIntervals);
   IntervalIndexType lastIntervalIdx = pIntervals->GetIntervalCount() - 1;

   GET_IFACE2(pBroker, IPointOfInterest, pPOI);

   SHEARDEFORMATIONDETAILS details;
   GET_IFACE2(pBroker, IBearingDesignParameters, pBearingDesignParameters);
   pBearingDesignParameters->GetBearingTableParameters(girderKey, &details);


   PoiList vPoi;
   std::vector<CGirderKey> vGirderKeys;
   pBridge->GetGirderline(girderKey.girderIndex, details.startGroup, details.endGroup, &vGirderKeys);
   for (const auto& thisGirderKey : vGirderKeys)
   {
        PierIndexType startPierIdx = pBridge->GetGirderGroupStartPier(thisGirderKey.groupIndex);
        PierIndexType endPierIdx = pBridge->GetGirderGroupEndPier(thisGirderKey.groupIndex);
        for (PierIndexType pierIdx = startPierIdx; pierIdx <= endPierIdx; pierIdx++)
        {
            if (pierIdx == startPierIdx)
            {
                CSegmentKey segmentKey(thisGirderKey, 0);
                PoiList segPoi;
                pPOI->GetPointsOfInterest(segmentKey, POI_0L | POI_ERECTED_SEGMENT, &segPoi);
                vPoi.push_back(segPoi.front());
            }
            else if (pierIdx == endPierIdx)
            {
                SegmentIndexType nSegments = pBridge->GetSegmentCount(thisGirderKey);
                CSegmentKey segmentKey(thisGirderKey, nSegments - 1);
                PoiList segPoi;
                pPOI->GetPointsOfInterest(segmentKey, POI_10L | POI_ERECTED_SEGMENT, &segPoi);
                vPoi.push_back(segPoi.front());
            }
            else
            {
                Float64 Xgp;
                VERIFY(pBridge->GetPierLocation(thisGirderKey, pierIdx, &Xgp));
                pgsPointOfInterest poi = pPOI->ConvertGirderPathCoordinateToPoi(thisGirderKey, Xgp);
                vPoi.push_back(poi);
            }
        }
   }

   GET_IFACE2(pBroker, IBearingDesign, pBearingDesign);
   IntervalIndexType lastCompositeDeckIntervalIdx = pIntervals->GetLastCompositeDeckInterval();
   std::unique_ptr<IProductReactionAdapter> pForces(std::make_unique<BearingDesignProductReactionAdapter>(pBearingDesign, lastCompositeDeckIntervalIdx, girderKey));

   ReactionLocationIter iter = pForces->GetReactionLocations(pBridge);
   iter.First();
   PierIndexType startPierIdx = (iter.IsDone() ? INVALID_INDEX : iter.CurrentItem().PierIdx);


  

   // Use iterator to walk locations
   for (iter.First(); !iter.IsDone(); iter.Next())
   {

       const ReactionLocation& reactionLocation(iter.CurrentItem());
       const CGirderKey& thisGirderKey(reactionLocation.GirderKey);

       const pgsPointOfInterest& poi = vPoi[reactionLocation.PierIdx - startPierIdx];  ///// use in chapter builder


       // bearing time-dependent effects begin at the erect segment interval
       const CSegmentKey& segmentKey(poi.GetSegmentKey());
       GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
       const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(segmentKey);
       IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
       IntervalIndexType erectSegmentIntervalIdx = pIntervals->GetErectSegmentInterval(poi.GetSegmentKey());
       IntervalIndexType lastIntervalIdx = pIntervals->GetIntervalCount() - 1;

       
       GroupIndexType nGroups = pBridge->GetGirderGroupCount();
       GroupIndexType firstGroupIdx = (segmentKey.groupIndex == ALL_GROUPS ? 0 : segmentKey.groupIndex);
       GroupIndexType lastGroupIdx = (segmentKey.groupIndex == ALL_GROUPS ? nGroups - 1 : firstGroupIdx);


       for (IntervalIndexType intervalIdx = erectSegmentIntervalIdx + 1; intervalIdx <= lastIntervalIdx; intervalIdx++)
       {
           if (pIntervals->GetDuration(intervalIdx) != 0)
           {
               if (pBTSDRptSpec->ReportAtAllLocations())
               {
                   *pPara << CTimeStepShearDeformationTable().BuildTimeStepShearDeformationTable(pBroker, reactionLocation, intervalIdx);
               }
               else
               {
                   if (reactionLocation.PierIdx == pBTSDRptSpec->GetReactionLocation().PierIdx && reactionLocation.Face == pBTSDRptSpec->GetReactionLocation().Face)
                   {
                       *pPara << CTimeStepShearDeformationTable().BuildTimeStepShearDeformationTable(pBroker, reactionLocation, intervalIdx);
                   }
               }
           }
       }
   }


   return pChapter;
}

std::unique_ptr<WBFL::Reporting::ChapterBuilder> CBearingTimeStepDetailsChapterBuilder::Clone() const
{
   return std::make_unique<CBearingTimeStepDetailsChapterBuilder>();
}

