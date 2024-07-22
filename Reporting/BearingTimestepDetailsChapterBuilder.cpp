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
#include <Reporting\TimeStepDetailsReportSpecification.h>
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

   auto pGdrRptSpec = std::dynamic_pointer_cast<const CGirderReportSpecification>(pRptSpec);
   auto pGdrLineRptSpec = std::dynamic_pointer_cast<const CGirderLineReportSpecification>(pRptSpec);

   CComPtr<IBroker> pBroker;
   CGirderKey girderKey;

   if (pGdrRptSpec)
   {
       pGdrRptSpec->GetBroker(&pBroker);
       girderKey = pGdrRptSpec->GetGirderKey();
   }
   else
   {
       pGdrLineRptSpec->GetBroker(&pBroker);
       girderKey = pGdrLineRptSpec->GetGirderKey();
   }

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

   // get poi where pier Reactions occur
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

   ReactionTableType tableType = BearingReactionsTable;


   GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);
   INIT_UV_PROTOTYPE(rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), false);
   INIT_UV_PROTOTYPE(rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false);
   INIT_UV_PROTOTYPE(rptLength4UnitValue, I, pDisplayUnits->GetMomentOfInertiaUnit(), false);
   INIT_UV_PROTOTYPE(rptLength2UnitValue, A, pDisplayUnits->GetAreaUnit(), false);
   INIT_UV_PROTOTYPE(rptLengthUnitValue, deflection, pDisplayUnits->GetDeflectionUnit(), false);
   INIT_UV_PROTOTYPE(rptTemperatureUnitValue, temperature, pDisplayUnits->GetTemperatureUnit(), false);



   // Build table
   INIT_UV_PROTOTYPE(rptForceUnitValue, reactu, pDisplayUnits->GetShearUnit(), false);

   // TRICKY:
   // Use the adapter class to get the reaction response functions we need and to iterate piers
   ReactionUnitValueTool reaction(tableType, reactu);

   GET_IFACE2(pBroker, IBearingDesignParameters, pBearing);
   SHEARDEFORMATIONDETAILS sf_details;


   // Use iterator to walk locations
   for (iter.First(); !iter.IsDone(); iter.Next())
   {



       const ReactionLocation& reactionLocation(iter.CurrentItem());
       const CGirderKey& thisGirderKey(reactionLocation.GirderKey);

       ReactionDecider reactionDecider(BearingReactionsTable, reactionLocation, thisGirderKey, pBridge, pIntervals);

       const pgsPointOfInterest& poi = vPoi[reactionLocation.PierIdx - startPierIdx];

       

       // bearing time-dependent effects begin at the erect segment interval
       const CSegmentKey& segmentKey(poi.GetSegmentKey());
       GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
       const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(segmentKey);
       IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
       IntervalIndexType erectSegmentIntervalIdx = pIntervals->GetErectSegmentInterval(poi.GetSegmentKey()) + 1;
       IntervalIndexType lastIntervalIdx = pIntervals->GetIntervalCount() - 1;

       Float64 L = pBearing->GetDistanceToPointOfFixity(poi, &details);
       GroupIndexType nGroups = pBridge->GetGirderGroupCount();
       GroupIndexType firstGroupIdx = (segmentKey.groupIndex == ALL_GROUPS ? 0 : segmentKey.groupIndex);
       GroupIndexType lastGroupIdx = (segmentKey.groupIndex == ALL_GROUPS ? nGroups - 1 : firstGroupIdx);

       GET_IFACE2(pBroker, IProductLoads, pProductLoads);

       for (IntervalIndexType intervalIdx = erectSegmentIntervalIdx; intervalIdx <= lastIntervalIdx; intervalIdx++)
       {
 //          if (pIntervals->GetDuration(intervalIdx) != 0)

           {
               ColumnIndexType nCols = 16;
               CString label{ reactionLocation.PierLabel.c_str() };
               label += _T(" - ");
               label += _T("Interval ");
               label += std::to_string(intervalIdx).c_str();
               label += _T(" (Previous Interval: ");
               label += std::to_string(intervalIdx-1).c_str();
               label += _T(" - ");
               label += pIntervals->GetDescription(intervalIdx - 1).c_str();
               label += _T(")");

               rptRcTable* p_table = rptStyleManager::CreateDefaultTable(nCols, label);

               p_table->SetNumberOfHeaderRows(2);

               p_table->SetRowSpan(0, 0, 2);
               (*p_table)(0, 0) << COLHDR(_T("Location"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
               p_table->SetRowSpan(0, 1, 1);

               p_table->SetColumnSpan(0, 1, 5);
               (*p_table)(0, 1) << pProductLoads->GetProductLoadName(pgsTypes::pftCreep);
               (*p_table)(1, 1) << _T("Incremental") << rptNewLine << symbol(epsilon) << Super2(_T("x10"), _T("6"));
               (*p_table)(1, 2) << _T("Cumulative") << rptNewLine << symbol(epsilon) << Super2(_T("x10"), _T("6"));
               (*p_table)(1, 3) << COLHDR(Sub2(symbol(delta), _T("d")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
               (*p_table)(1, 4) << Sub2(symbol(epsilon), _T("avg")) << Super2(_T("x10"), _T("6"));
               (*p_table)(1, 5) << COLHDR(Sub2(symbol(DELTA), _T("s")) << Super2(_T("x10"), _T("3")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());

               p_table->SetColumnSpan(0, 6, 5);
               (*p_table)(0, 6) << pProductLoads->GetProductLoadName(pgsTypes::pftShrinkage);
               (*p_table)(1, 6) << _T("Incremental") << rptNewLine << symbol(epsilon) << Super2(_T("x10"), _T("6"));
               (*p_table)(1, 7) << _T("Cumulative") << rptNewLine << symbol(epsilon) << Super2(_T("x10"), _T("6"));
               (*p_table)(1, 8) << COLHDR(Sub2(symbol(delta), _T("d")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
               (*p_table)(1, 9) << Sub2(symbol(epsilon), _T("avg")) << Super2(_T("x10"), _T("6"));
               (*p_table)(1, 10) << COLHDR(Sub2(symbol(DELTA), _T("s")) << Super2(_T("x10"), _T("3")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());

               p_table->SetColumnSpan(0, 11, 5);
               (*p_table)(0, 11) << pProductLoads->GetProductLoadName(pgsTypes::pftRelaxation);
               (*p_table)(1, 11) << _T("Incremental") << rptNewLine << symbol(epsilon) << Super2(_T("x10"), _T("6"));
               (*p_table)(1, 12) << _T("Cumulative") << rptNewLine << symbol(epsilon) << Super2(_T("x10"), _T("36"));
               (*p_table)(1, 13) << COLHDR(Sub2(symbol(delta), _T("d")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
               (*p_table)(1, 14) << Sub2(symbol(epsilon), _T("avg")) << Super2(_T("x10"), _T("6"));
               (*p_table)(1, 15) << COLHDR(Sub2(symbol(DELTA), _T("s")) << Super2(_T("x10"), _T("3")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());


               PoiList vPoi;
               GET_IFACE2(pBroker, IPointOfInterest, pIPoi);

               if (pIPoi->ConvertPoiToGirderlineCoordinate(poi) < pIPoi->ConvertPoiToGirderlineCoordinate(details.poi_fixity))
               {
                   pIPoi->GetPointsOfInterestInRange(0, poi, L, &vPoi);
               }
               else
               {
                   pIPoi->GetPointsOfInterestInRange(L, poi, 0, &vPoi);
               }

               vPoi.erase(std::remove_if(vPoi.begin(), vPoi.end(), [](const pgsPointOfInterest& i) {
                   return i.HasAttribute(POI_BOUNDARY_PIER);
                   }), vPoi.end());



               pgsPointOfInterest p0, p1;

               RowIndexType row = p_table->GetNumberOfHeaderRows();


               for (IndexType idx = 1, nPoi = vPoi.size(); idx < nPoi; idx++)
               {

                   p0 = vPoi[idx - 1];
                   p1 = vPoi[idx];

                   (*p_table)(row, 0) << location.SetValue(POI_SPAN, p1);


                   std::vector<pgsTypes::ProductForceType> td_types{
                       pgsTypes::ProductForceType::pftCreep, pgsTypes::ProductForceType::pftGirder, pgsTypes::ProductForceType::pftRelaxation};


                   for (IndexType ty = 0, nTypes = td_types.size(); ty < nTypes; ty++)
                   {
                       TIMEDEPENDENTSHEARDEFORMATIONPARAMETERS sf_params;
                       pBearing->GetBearingTimeDependentShearDeformationParameters(poi, intervalIdx, p0, p1, td_types[ty], &sf_params);


                       (*p_table)(row, ty * 5 + 1) << (idx == 1 ? _T("N/A") : std::to_wstring(sf_params.inc_strain_bot_girder1 * 1E6));
                       (*p_table)(row, ty * 5 + 2) << (idx == 1 ? _T("N/A") : std::to_wstring(sf_params.cum_strain_bot_girder1 * 1E6));
                       if (idx == 1)
                       {
                           (*p_table)(row, ty * 5 + 3) << _T("N/A");
                       }
                       else
                       {
                           (*p_table)(row, ty * 5 + 3) << deflection.SetValue(sf_params.delta_d);
                       }
                       (*p_table)(row, ty * 5 + 4) << (idx == 1 ? _T("N/A") : std::to_wstring(sf_params.average_cumulative_strain * 1E6));
                       if (idx == 1)
                       {
                           (*p_table)(row, ty * 5 + 5) << _T("N/A");
                       }
                       else
                       {
                           (*p_table)(row, ty * 5 + 5) << deflection.SetValue(sf_params.inc_shear_def * 1E3);
                       }
                       
                   }

                   row++;

               }

               pBearing->GetBearingTotalTimeDependentShearDeformation(poi, intervalIdx, &sf_details);


               
               p_table->SetColumnStyle(0, rptStyleManager::GetTableCellStyle(CJ_LEFT));
               p_table->SetStripeRowColumnStyle(0, rptStyleManager::GetTableCellStyle(CJ_LEFT));
               (*p_table)(row++, 0) << symbol(SIGMA) << _T(" Incrmental ") << Sub2(symbol(DELTA),_T("s"));
               (*p_table)(row--, 0) << symbol(SIGMA) << _T(" Cumulative ") << Sub2(symbol(DELTA), _T("s"));

               INIT_UV_PROTOTYPE(rptLengthUnitValue, deflection, pDisplayUnits->GetDeflectionUnit(), true);

               p_table->SetColumnSpan(row, 1, 5);
               (*p_table)(row++, 1) << deflection.SetValue(sf_details.incremental_creep) << rptNewLine;
               p_table->SetColumnSpan(row, 1, 5);
               (*p_table)(row--, 1) << deflection.SetValue(sf_details.cumulative_creep) << rptNewLine;
               p_table->SetColumnSpan(row, 6, 5);
               (*p_table)(row++, 6) << deflection.SetValue(sf_details.incremental_shrinkage) << rptNewLine;
               p_table->SetColumnSpan(row, 6, 5);
               (*p_table)(row--, 6) << deflection.SetValue(sf_details.cumulative_shrinkage) << rptNewLine;
               p_table->SetColumnSpan(row, 11, 5);
               (*p_table)(row++, 11) << deflection.SetValue(sf_details.incremental_relaxation) << rptNewLine;
               p_table->SetColumnSpan(row, 11, 5);
               (*p_table)(row--, 11) << deflection.SetValue(sf_details.cumulative_relaxation) << rptNewLine;

               *pPara << p_table << rptNewLine;
           }

       }    

   }


   return pChapter;
}

std::unique_ptr<WBFL::Reporting::ChapterBuilder> CBearingTimeStepDetailsChapterBuilder::Clone() const
{
   return std::make_unique<CBearingTimeStepDetailsChapterBuilder>();
}

