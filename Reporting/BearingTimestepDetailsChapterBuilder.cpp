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
   if (pLossParams->GetLossMethod() != PrestressLossCriteria::LossMethodType::TIME_STEP)
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

   GET_IFACE2(pBroker, IIntervals, pIntervals);
   GET_IFACE2(pBroker, IBearingDesignParameters, pBearingDesignParameters);
   GET_IFACE2(pBroker, IPointOfInterest, pPOI);

   const pgsPointOfInterest poi = pPOI->GetPierPointOfInterest(girderKey, rptLocation.PierIdx);
   IntervalIndexType erectSegmentIntervalIdx = pIntervals->GetErectSegmentInterval(poi.GetSegmentKey());
   IntervalIndexType rptIntervalIdx = pBTSDRptSpec->GetInterval();
   IntervalIndexType nIntervals = pIntervals->GetIntervalCount();
   IntervalIndexType firstIntervalIdx = (rptIntervalIdx == INVALID_INDEX ? erectSegmentIntervalIdx : rptIntervalIdx);
   IntervalIndexType lastIntervalIdx = (rptIntervalIdx == INVALID_INDEX ? nIntervals - 1 : rptIntervalIdx);


   SHEARDEFORMATIONDETAILS details;
   pBearingDesignParameters->GetBearingTableParameters(girderKey, &details);
   pBearingDesignParameters->GetTimeDependentShearDeformation(girderKey, &details);


   for (auto intervalIdx = erectSegmentIntervalIdx + 1; intervalIdx <= lastIntervalIdx; intervalIdx++)
   {
       for (auto&  bearing: details.brg_details)
       {
            for (auto& detail_interval : bearing.timestep_details)
            {
                if (detail_interval.interval == intervalIdx)
                {
                    if (pBTSDRptSpec->ReportAtAllLocations())
                    {
                        *pPara << CTimeStepShearDeformationTable().BuildTimeStepShearDeformationTable(pBroker, bearing.reactionLocation, poi, &detail_interval);
                    }
                    else
                    {
                        if (bearing.reactionLocation == rptLocation)
                        {
                            *pPara << CTimeStepShearDeformationTable().BuildTimeStepShearDeformationTable(pBroker, bearing.reactionLocation, poi, &detail_interval);
                        }
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

