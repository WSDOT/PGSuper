///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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
#include <Reporting\TimeStepDetailsChapterBuilder.h>
#include <Reporting\TimeStepDetailsReportSpecification.h>

#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\PointOfInterest.h>
#include <IFace\PrestressForce.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Intervals.h>
#include <IFace\ReportOptions.h>

#include <WBFLGenericBridgeTools.h>

#include <PgsExt\TimelineEvent.h>

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
#define DELTA_E    symbol(DELTA) << symbol(epsilon)
#define DELTA_R    symbol(DELTA) << symbol(varphi)
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
   CTimeStepDetailsChapterBuilder
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CTimeStepDetailsChapterBuilder::CTimeStepDetailsChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CTimeStepDetailsChapterBuilder::GetName() const
{
   return TEXT("Time Step Details");
}

rptChapter* CTimeStepDetailsChapterBuilder::Build(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,Uint16 level) const
{
   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);
   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   auto pTSDRptSpec = std::dynamic_pointer_cast<const CTimeStepDetailsReportSpecification>(pRptSpec);

   CComPtr<IBroker> pBroker;
   pTSDRptSpec->GetBroker(&pBroker);

   GET_IFACE2(pBroker, ILossParameters, pLossParams);
   if ( pLossParams->GetLossMethod() != PrestressLossCriteria::LossMethodType::TIME_STEP )
   {
      *pPara << color(Red) << _T("Time Step analysis results not available.") << color(Black) << rptNewLine;
      return pChapter;
   }

   const pgsPointOfInterest& rptPoi(pTSDRptSpec->GetPointOfInterest());
   const CSegmentKey& segmentKey(rptPoi.GetSegmentKey());
   const CGirderKey& girderKey(segmentKey);
   PoiList vPoi;
   if ( pTSDRptSpec->ReportAtAllLocations() )
   {
      GET_IFACE2(pBroker,IPointOfInterest,pPoi);
      pPoi->GetPointsOfInterest(CSegmentKey(ALL_GROUPS,girderKey.girderIndex,ALL_SEGMENTS), &vPoi);
      vPoi.erase(std::unique(vPoi.begin(), vPoi.end(), [](const pgsPointOfInterest& poi1, const pgsPointOfInterest& poi2) {return IsEqual(poi1.GetDistFromStart(), poi2.GetDistFromStart());}), vPoi.end());
   }
   else
   {
      vPoi.push_back(rptPoi);
   }

   IntervalIndexType rptIntervalIdx = pTSDRptSpec->GetInterval();

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker,ILosses,pLosses);
   GET_IFACE2(pBroker,IIntervals,pIntervals);
   GET_IFACE2(pBroker, ISegmentTendonGeometry, pSegmentTendonGeometry);
   GET_IFACE2(pBroker, IGirderTendonGeometry, pGirderTendonGeometry);
   GET_IFACE2(pBroker, IBridge, pBridge);
   GET_IFACE2(pBroker, IMaterials, pMaterials);
   GET_IFACE2(pBroker,IProductLoads,pProductLoads);

   bool bHasDeck = IsStructuralDeck(pBridge->GetDeckType());

   DuctIndexType nSegmentDucts = pSegmentTendonGeometry->GetDuctCount(segmentKey);
   DuctIndexType nGirderDucts = pGirderTendonGeometry->GetDuctCount(girderKey);

   INIT_UV_PROTOTYPE(rptPointOfInterest,    location,   pDisplayUnits->GetSpanLengthUnit(),      true);

   std::_tstring strImagePath(rptStyleManager::GetImagePath());

   GET_IFACE2(pBroker,IReportOptions,pReportOptions);
   location.IncludeSpanAndGirder(pReportOptions->IncludeSpanAndGirder4Pois(girderKey));

   // reporting for a specific poi... list poi at top of report
   if ( !pTSDRptSpec->ReportAtAllLocations() )
   {
      pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
      *pChapter << pPara;

      CString strLocation;
      rptReportContent& rcLocation = location.SetValue(POI_SPAN,rptPoi);
#if defined _DEBUG || defined _BETA_VERSION
      strLocation.Format(_T("Point Of Interest: ID = %d %s"),rptPoi.GetID(),location.AsString().c_str());
#else
      strLocation.Format(_T("%s"),location.AsString().c_str());
#endif
      strLocation.Replace(_T("<sub>"),_T(""));
      strLocation.Replace(_T("</sub>"),_T(""));

      *pPara << rcLocation << rptNewLine;
      pPara->SetName(strLocation);
      *pPara << rptNewLine;
   }

   // reporting for a specific interval... list interval name at top of report
   if ( rptIntervalIdx != INVALID_INDEX )
   {
      pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
      *pChapter << pPara;

      CString str;
      str.Format(_T("Interval %d : %s"),LABEL_INTERVAL(rptIntervalIdx),pIntervals->GetDescription(rptIntervalIdx).c_str());
      *pPara << str << rptNewLine;
      pPara->SetName(str);
   }


   IntervalIndexType nIntervals = pIntervals->GetIntervalCount();
   IntervalIndexType firstIntervalIdx = (rptIntervalIdx == INVALID_INDEX ? 0 : rptIntervalIdx);
   IntervalIndexType lastIntervalIdx = (rptIntervalIdx == INVALID_INDEX ? nIntervals - 1 : rptIntervalIdx);

   if (vPoi.size() == 1)
   {
      pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
      *pChapter << pPara;
      pPara->SetName(_T("Creep Details"));
      *pPara << pPara->GetName() << rptNewLine;

      ReportCreepDetails(pChapter, pBroker, vPoi.front(), firstIntervalIdx, lastIntervalIdx, pDisplayUnits);

      pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
      *pChapter << pPara;
      pPara->SetName(_T("Shrinkage Details"));
      *pPara << pPara->GetName() << rptNewLine;

      ReportShrinkageDetails(pChapter, pBroker, vPoi.front(), firstIntervalIdx, lastIntervalIdx, pDisplayUnits);

      pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
      *pChapter << pPara;
      pPara->SetName(_T("Relaxation Details"));
      *pPara << pPara->GetName() << rptNewLine;

      ReportStrandRelaxationDetails(pChapter, pBroker, vPoi.front(), firstIntervalIdx, lastIntervalIdx, pDisplayUnits);

      if (0 < nSegmentDucts)
      {
         ReportSegmentTendonRelaxationDetails(pChapter, pBroker, vPoi.front(), firstIntervalIdx, lastIntervalIdx, pDisplayUnits);
      }

      if (0 < nGirderDucts)
      {
         ReportGirderTendonRelaxationDetails(pChapter, pBroker, vPoi.front(), firstIntervalIdx, lastIntervalIdx, pDisplayUnits);
      }
   }
   else
   {
      pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
      *pChapter << pPara;
      pPara->SetName(_T("Creep, Shrinkage and Relaxation Details"));
      *pPara << pPara->GetName() << rptNewLine;
      pPara = new rptParagraph;
      *pChapter << pPara;
      *pPara << _T("Creep, shrinkage and relaxation details for multiple cross sections results in a very large report. Create a Time Step Details report for a specific section to see the calculation details at that section.") << rptNewLine;
   }

   for ( IntervalIndexType intervalIdx = firstIntervalIdx; intervalIdx <= lastIntervalIdx; intervalIdx++ )
   {
      CGirderKey prevGirderKey;
      std::vector<pgsTypes::ProductForceType> vLoads;
      for (const pgsPointOfInterest& poi : vPoi)
      {
         const CSegmentKey& segmentKey(poi.GetSegmentKey());
         if ( pTSDRptSpec->ReportAtAllLocations() )
         {
            pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
            *pChapter << pPara;

            CString strLocation;
            rptReportContent& rcLocation = location.SetValue(POI_SPAN,poi);
#if defined _DEBUG || defined _BETA_VERSION
            strLocation.Format(_T("Point Of Interest: ID = %d %s"),poi.GetID(),location.AsString().c_str());
#else
            strLocation.Format(_T("%s"),location.AsString().c_str());
#endif
            strLocation.Replace(_T("<sub>"),_T(""));
            strLocation.Replace(_T("</sub>"),_T(""));
            pPara->SetName(strLocation);
            *pPara << rcLocation << rptNewLine;
            *pPara << rptNewLine;
         }

         // Heading
         if ( rptIntervalIdx == INVALID_INDEX )
         {
            pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
            *pChapter << pPara;

            CString str;
            str.Format(_T("Interval %d : %s"),LABEL_INTERVAL(intervalIdx),pIntervals->GetDescription(intervalIdx).c_str());
            *pPara << str << rptNewLine;
            pPara->SetName(str);
         }

         if ( !prevGirderKey.IsEqual(segmentKey) )
         {
            // we only want to do this when the girder changes
            vLoads = pProductLoads->GetProductForcesForGirder(poi.GetSegmentKey());
         }

         const LOSSDETAILS* pDetails = pLosses->GetLossDetails(poi,intervalIdx);
         const TIME_STEP_DETAILS& tsDetails(pDetails->TimeStepDetails[intervalIdx]);

         // Interval Time Parameters
         pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
         (*pChapter) << pPara;
         (*pPara) << _T("Interval Details") << rptNewLine;
         pPara = new rptParagraph;
         *pChapter << pPara;
         rptRcTable* pIntervalTable = BuildIntervalTable(tsDetails,pIntervals,pDisplayUnits);
         *pPara << pIntervalTable << rptNewLine;
         *pPara << rptNewLine;

         // Concrete Parameters
         pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
         (*pChapter) << pPara;
         (*pPara) << _T("Concrete Details") << rptNewLine;
         pPara = new rptParagraph;
         *pChapter << pPara;
         rptRcTable* pConcreteTable = BuildConcreteTable(tsDetails, segmentKey, pMaterials, pDisplayUnits);
         *pPara << pConcreteTable << rptNewLine;
         *pPara << rptNewLine;

         // Properties
         pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
         (*pChapter) << pPara;
         (*pPara) << _T("Component Net Section Properties") << rptNewLine;
         pPara = new rptParagraph;
         (*pChapter) << pPara;
         rptRcTable* pComponentPropertiesTable = BuildComponentPropertiesTable(tsDetails,bHasDeck,pDisplayUnits);
         *pPara << pComponentPropertiesTable << rptNewLine;
         *pPara << Sub2(_T("Y"),_T("k")) << _T(" is measured positive upwards from the top of girder.") << rptNewLine;
         *pPara << rptNewLine;

         pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
         (*pChapter) << pPara;
         (*pPara) << _T("Composite Transformed Section Properties using Age Adjusted Modulus") << rptNewLine;
         pPara = new rptParagraph;
         (*pChapter) << pPara;
         (*pPara) << rptRcImage(strImagePath + _T("TransformedProperties.png")) << rptNewLine;
         rptRcTable* pSectionPropertiesTable = BuildSectionPropertiesTable(tsDetails, pDisplayUnits);
         *pPara << pSectionPropertiesTable << rptNewLine;
         *pPara << Sub2(_T("Y"),_T("tr")) << _T(" is measured positive upwards from the top of girder (at the girder/deck interface).") << rptNewLine;
         *pPara << rptNewLine;

         Float64 duration = pIntervals->GetDuration(intervalIdx);
         if (0 < duration)
         {
            // Unrestrained creep deformation in concrete parts due to loads applied in previous intervals
            pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
            (*pChapter) << pPara;
            (*pPara) << _T("Unrestrained creep deformation of concrete components due to loads applied in previous intervals") << rptNewLine;
            pPara = new rptParagraph;
            (*pChapter) << pPara;
            (*pPara) << rptRcImage(strImagePath + _T("FreeCreep_Axial.png")) << rptNewLine;
            (*pPara) << rptRcImage(strImagePath + _T("FreeCreep_Curvature.png")) << rptNewLine;
            rptRcTable* pFreeCreepTable = BuildFreeCreepDeformationTable(tsDetails, bHasDeck, pDisplayUnits);
            *pPara << pFreeCreepTable << rptNewLine;
            *pPara << rptNewLine;

            // Unrestrained shrinkage during this interval
            pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
            (*pChapter) << pPara;
            (*pPara) << _T("Unrestrained shrinkage deformation of concrete components") << rptNewLine;
            pPara = new rptParagraph;
            (*pChapter) << pPara;
            *pPara << _T("Girder: ") << DELTA_ESH << Super2(_T("x10"), _T("6")) << _T(" = ") << tsDetails.Girder.Shrinkage.esi * 1E6 << rptNewLine;

            if (bHasDeck)
            {
               *pPara << _T("Deck: ") << DELTA_ESH << Super2(_T("x10"), _T("6")) << _T(" = ") << tsDetails.Deck.Shrinkage.esi * 1E6 << rptNewLine;
            }

            *pPara << rptNewLine;

            // Unrestrained strand relaxation
            pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
            (*pChapter) << pPara;
            (*pPara) << _T("Apparent unrestrained deformation of strands due to relaxation") << rptNewLine;
            pPara = new rptParagraph;
            (*pChapter) << pPara;
            (*pPara) << rptRcImage(strImagePath + _T("ApparentRelaxationStrain.png")) << rptNewLine;
            rptRcTable* pStrandRelaxationTable = BuildStrandRelaxationTable(tsDetails, pDisplayUnits);
            (*pPara) << pStrandRelaxationTable << rptNewLine;
            (*pPara) << rptNewLine;

            // Unrestrained tendon relaxation
            if (0 < nSegmentDucts)
            {
               pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
               (*pChapter) << pPara;
               (*pPara) << _T("Apparent unrestrained deformation of segment tendons due to relaxation") << rptNewLine;
               pPara = new rptParagraph;
               (*pChapter) << pPara;
               (*pPara) << rptRcImage(strImagePath + _T("ApparentRelaxationStrain.png")) << rptNewLine;
               rptRcTable* pTendonRelaxationTable = BuildSegmentTendonRelaxationTable(tsDetails, pDisplayUnits);
               (*pPara) << pTendonRelaxationTable << rptNewLine;
               (*pPara) << rptNewLine;
            }

            if (0 < nGirderDucts)
            {
               pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
               (*pChapter) << pPara;
               (*pPara) << _T("Apparent unrestrained deformation of girder tendons due to relaxation") << rptNewLine;
               pPara = new rptParagraph;
               (*pChapter) << pPara;
               (*pPara) << rptRcImage(strImagePath + _T("ApparentRelaxationStrain.png")) << rptNewLine;
               rptRcTable* pTendonRelaxationTable = BuildGirderTendonRelaxationTable(tsDetails, pDisplayUnits);
               (*pPara) << pTendonRelaxationTable << rptNewLine;
               (*pPara) << rptNewLine;
            }

            // Forces required to fully restrain each component
            pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
            (*pChapter) << pPara;
            (*pPara) << _T("Component Restraining Forces") << rptNewLine;
            pPara = new rptParagraph;
            (*pChapter) << pPara;
            (*pPara) << rptRcImage(strImagePath + _T("IncrementalRestrainingForce_Axial.png")) << rptNewLine;
            (*pPara) << rptRcImage(strImagePath + _T("IncrementalRestrainingForce_Moment.png")) << rptNewLine;
            rptRcTable* pComponentRestrainingForcesTable = BuildComponentRestrainingForceTable(tsDetails, bHasDeck, pDisplayUnits);
            (*pPara) << pComponentRestrainingForcesTable << rptNewLine;
            (*pPara) << rptNewLine;


            // Forces required to fully restrain each component
            pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
            (*pChapter) << pPara;
            (*pPara) << _T("Section Restraining Forces") << rptNewLine;
            pPara = new rptParagraph;
            (*pChapter) << pPara;
            (*pPara) << rptRcImage(strImagePath + _T("TotalRestrainingForce_Axial.png")) << rptNewLine;
            (*pPara) << rptRcImage(strImagePath + _T("TotalRestrainingForce_Moment.png")) << rptNewLine;
            rptRcTable* pSectionRestrainingForcesTable = BuildSectionRestrainingForceTable(tsDetails, pDisplayUnits);
            (*pPara) << pSectionRestrainingForcesTable << rptNewLine;
            (*pPara) << rptNewLine;

            // Deformations required to fully restrain each component
            pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
            (*pChapter) << pPara;
            (*pPara) << _T("Section Restraining Deformations") << rptNewLine;
            pPara = new rptParagraph;
            (*pChapter) << pPara;
            (*pPara) << rptRcImage(strImagePath + _T("TotalRestrainingDeformation_Axial.png")) << rptNewLine;
            (*pPara) << rptRcImage(strImagePath + _T("TotalRestrainingDeformation_Curvature.png")) << rptNewLine;
            rptRcTable* pSectionRestrainingDeformationTable = BuildSectionRestrainingDeformationTable(tsDetails, pDisplayUnits);
            (*pPara) << pSectionRestrainingDeformationTable << rptNewLine;
            (*pPara) << _T("Section restraining deformations are computed at multiple sections along the girder. It is assumed that deformations vary linerally between sections. The girder is analyzed for these deformations. The resulting section forces are listed in the Restrained Section Forces table below.") << rptNewLine;
            (*pPara) << rptNewLine;

            // Forces due to the external restraints of the strutural system
            pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
            (*pChapter) << pPara;
            (*pPara) << _T("Restrained Section Forces") << rptNewLine;
            pPara = new rptParagraph;
            (*pChapter) << pPara;
            rptRcTable* pRestrainedSectionForcesTable = BuildRestrainedSectionForceTable(tsDetails, pDisplayUnits);
            (*pPara) << pRestrainedSectionForcesTable << rptNewLine;
            (*pPara) << _T("The restrained section forces are the secondary forces due to the structural system restraining the deformations due to creep, shrinkage, and relaxation.") << rptNewLine;
            *pPara << rptNewLine;

            // Restrained component forces
            pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
            (*pChapter) << pPara;
            (*pPara) << _T("Restrained Component Forces") << rptNewLine;
            pPara = new rptParagraph;
            (*pChapter) << pPara;
            (*pPara) << rptRcImage(strImagePath + _T("RestrainedComponentForces_Axial.png")) << rptNewLine;
            (*pPara) << rptRcImage(strImagePath + _T("RestrainedComponentForces_Moment.png")) << rptNewLine;
            rptRcTable* pRestrainedComponentForcesTable = BuildRestrainedComponentForceTable(tsDetails, bHasDeck, pDisplayUnits);
            (*pPara) << pRestrainedComponentForcesTable << rptNewLine;
            *pPara << rptNewLine;
         }
         else
         {
            // Incremental Forces due to loads applied during this interval
            pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
            (*pChapter) << pPara;
            (*pPara) << _T("Incremental forces due to external loads and restrained component forces during this interval.") << rptNewLine;

            pPara = new rptParagraph;
            (*pChapter) << pPara;
            (*pPara) << rptRcImage(strImagePath + _T("IncrementalForce_Axial.png")) << rptNewLine;
            (*pPara) << rptRcImage(strImagePath + _T("IncrementalForce_Moment.png")) << rptNewLine;

            rptRcTable* pIncForcesTable = BuildIncrementalForceTable(pBroker, vLoads, tsDetails, bHasDeck, pDisplayUnits);
            (*pPara) << pIncForcesTable << rptNewLine;
            *pPara << rptNewLine;
         }

         // Incremental stresses due to loads applied during this interval
         pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
         (*pChapter) << pPara;
         (*pPara) << _T("Incremental component stress") << rptNewLine;
         pPara = new rptParagraph;
         (*pChapter) << pPara;
         (*pPara) << rptRcImage(strImagePath + _T("IncrementalComponentStress.png")) << rptNewLine;
         rptRcTable* pIncStressTable = BuildIncrementalStressTable(pBroker,vLoads,tsDetails, bHasDeck, pDisplayUnits);
         (*pPara) << pIncStressTable << rptNewLine;
         (*pPara) << _T("Change in stress in strands and tendons are the prestress losses during this interval.") << rptNewLine;
         *pPara << rptNewLine;

         prevGirderKey = CGirderKey(poi.GetSegmentKey());

         // start a new page for the next poi
         *pPara << rptNewPage;
      } // next POI
   } // next interval

   if ( rptIntervalIdx == INVALID_INDEX && vPoi.size() == 1)
   {
      pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
      *pChapter << pPara;
      pPara->SetName(_T("Girder Stress Summary"));
      *pPara << pPara->GetName() << rptNewLine;

      pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
      (*pChapter) << pPara;
      (*pPara) << _T("Incremental girder stress summary") << rptNewLine;
      pPara = new rptParagraph;
      (*pChapter) << pPara;
      rptRcTable* pIncrementalSummaryTable = BuildConcreteStressSummaryTable(pBroker,vPoi.front(),rtIncremental,true/*girder*/,pDisplayUnits);
      (*pPara) << pIncrementalSummaryTable << rptNewLine;

      pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
      (*pChapter) << pPara;
      (*pPara) << _T("Cumulative girder stress summary") << rptNewLine;
      pPara = new rptParagraph;
      (*pChapter) << pPara;
      rptRcTable* pCumulativeSummaryTable = BuildConcreteStressSummaryTable(pBroker,vPoi.front(),rtCumulative,true/*girder*/,pDisplayUnits);
      (*pPara) << pCumulativeSummaryTable << rptNewLine;

      GET_IFACE2_NOCHECK(pBroker,IPrecompressedTensileZone,pPrecompressedTensileZone);
      if (bHasDeck && pPrecompressedTensileZone->IsDeckPrecompressed(vPoi.front().get().GetSegmentKey()) )
      {
         pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
         *pChapter << pPara;
         pPara->SetName(_T("Deck Stress Summary"));
         *pPara << pPara->GetName() << rptNewLine;

         pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
         (*pChapter) << pPara;
         (*pPara) << _T("Incremental deck stress summary") << rptNewLine;
         pPara = new rptParagraph;
         (*pChapter) << pPara;
         rptRcTable* pIncrementalSummaryTable = BuildConcreteStressSummaryTable(pBroker,vPoi.front(),rtIncremental,false/*deck*/,pDisplayUnits);
         (*pPara) << pIncrementalSummaryTable << rptNewLine;

         pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
         (*pChapter) << pPara;
         (*pPara) << _T("Cumulative deck stress summary") << rptNewLine;
         pPara = new rptParagraph;
         (*pChapter) << pPara;
         rptRcTable* pCumulativeSummaryTable = BuildConcreteStressSummaryTable(pBroker,vPoi.front(),rtCumulative,false/*deck*/,pDisplayUnits);
         (*pPara) << pCumulativeSummaryTable << rptNewLine;
      }
   }

   return pChapter;
}

std::unique_ptr<WBFL::Reporting::ChapterBuilder> CTimeStepDetailsChapterBuilder::Clone() const
{
   return std::make_unique<CTimeStepDetailsChapterBuilder>();
}

rptRcTable* CTimeStepDetailsChapterBuilder::BuildIntervalTable(const TIME_STEP_DETAILS& tsDetails,IIntervals* pIntervals,IEAFDisplayUnits* pDisplayUnits) const
{
   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(4);
   (*pTable)(0,0) << _T("Start") << rptNewLine << _T("(day)");
   (*pTable)(0,1) << _T("Middle") << rptNewLine << _T("(day)");
   (*pTable)(0,2) << _T("End") << rptNewLine << _T("(day)");
   (*pTable)(0,3) << _T("Duration") << rptNewLine << _T("(day)");

   Float64 start    = pIntervals->GetTime(tsDetails.intervalIdx,pgsTypes::Start);
   Float64 middle   = pIntervals->GetTime(tsDetails.intervalIdx,pgsTypes::Middle);
   Float64 end      = pIntervals->GetTime(tsDetails.intervalIdx,pgsTypes::End);
   Float64 duration = pIntervals->GetDuration(tsDetails.intervalIdx);

   (*pTable)(1,0) << start;
   (*pTable)(1,1) << middle;
   (*pTable)(1,2) << end;
   (*pTable)(1,3) << duration;

   return pTable;
}

rptRcTable* CTimeStepDetailsChapterBuilder::BuildConcreteTable(const TIME_STEP_DETAILS& tsDetails, const CSegmentKey& segmentKey,IMaterials* pMaterials, IEAFDisplayUnits* pDisplayUnits) const
{
   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(13);
   pTable->SetColumnStyle(0, rptStyleManager::GetTableCellStyle(CJ_LEFT));
   pTable->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle(CJ_LEFT));
   pTable->SetNumberOfHeaderRows(2);

   pTable->SetRowSpan(0, 0, 2);
   pTable->SetRowSpan(1, 0, SKIP_CELL);
   (*pTable)(0, 0) << _T("Component");

   pTable->SetColumnSpan(0, 1, 3);
   (*pTable)(0, 1) << _T("Start");

   pTable->SetColumnSpan(0, 4, 6);
   (*pTable)(0, 4) << _T("Middle");

   pTable->SetColumnSpan(0, 10, 3);
   (*pTable)(0, 10) << _T("End");

   ColumnIndexType col = 1;
   // start
   (*pTable)(1, col++) << _T("Age") << rptNewLine << _T("(days)");
   (*pTable)(1, col++) << COLHDR(Sub2(_T("f"), _T("c")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
   (*pTable)(1, col++) << COLHDR(Sub2(_T("E"), _T("c")), rptStressUnitTag, pDisplayUnits->GetModEUnit());
   // middle
   (*pTable)(1, col++) << _T("Age") << rptNewLine << _T("(days)");
   (*pTable)(1, col++) << COLHDR(Sub2(_T("f"), _T("c")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
   (*pTable)(1, col++) << COLHDR(Sub2(_T("E"), _T("c")), rptStressUnitTag, pDisplayUnits->GetModEUnit());
   (*pTable)(1, col++) << symbol(chi) << _T("(") << Sub2(_T("i"),_T("m")) << _T(")");
   (*pTable)(1, col++) << CREEP(Sub2(_T("i"), _T("e")), Sub2(_T("i"), _T("m")));
   (*pTable)(1, col++) << COLHDR(Sub2(_T("E"), _T("ce")), rptStressUnitTag, pDisplayUnits->GetModEUnit());
   // end
   (*pTable)(1, col++) << _T("Age") << rptNewLine << _T("(days)");
   (*pTable)(1, col++) << COLHDR(Sub2(_T("f"), _T("c")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
   (*pTable)(1, col++) << COLHDR(Sub2(_T("E"), _T("c")), rptStressUnitTag, pDisplayUnits->GetModEUnit());

   INIT_UV_PROTOTYPE(rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), false);
   INIT_UV_PROTOTYPE(rptStressUnitValue, modE, pDisplayUnits->GetModEUnit(), false);

   RowIndexType row = pTable->GetNumberOfHeaderRows();
   col = 0;
   (*pTable)(row, col++) << _T("Girder");
   for (int t = 0; t < 3; t++)
   {
      pgsTypes::IntervalTimeType timeType = (pgsTypes::IntervalTimeType)(t);
      Float64 age = pMaterials->GetSegmentConcreteAge(segmentKey, tsDetails.intervalIdx, timeType);
      Float64 fc = pMaterials->GetSegmentFc(segmentKey, tsDetails.intervalIdx, timeType);
      Float64 Ec = pMaterials->GetSegmentEc(segmentKey, tsDetails.intervalIdx, timeType);

      (*pTable)(row, col++) << age;
      (*pTable)(row, col++) << stress.SetValue(fc);
      (*pTable)(row, col++) << modE.SetValue(Ec);

      if (timeType == pgsTypes::Middle)
      {
         Float64 X = pMaterials->GetSegmentAgingCoefficient(segmentKey, tsDetails.intervalIdx);
         Float64 Y = pMaterials->GetSegmentCreepCoefficient(segmentKey, tsDetails.intervalIdx, pgsTypes::Middle, tsDetails.intervalIdx, pgsTypes::End);
         Float64 Ece = pMaterials->GetSegmentAgeAdjustedEc(segmentKey, tsDetails.intervalIdx);

         (*pTable)(row, col++) << X;
         (*pTable)(row, col++) << Y;
         (*pTable)(row, col++) << modE.SetValue(Ece);
      }
   }
   row++;


   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
   EventIndexType castDeckEventIdx = pIBridgeDesc->GetCastDeckEventIndex();
   const auto* pEvent = pIBridgeDesc->GetEventByIndex(castDeckEventIdx);
   const auto& castDeckActivity = pEvent->GetCastDeckActivity();
   IndexType nRegions = castDeckActivity.GetCastingRegionCount();
   for (IndexType regionIdx = 0; regionIdx < nRegions; regionIdx++)
   {
      col = 0;
      (*pTable)(row, col++) << _T("Deck Region ") << LABEL_INDEX(regionIdx);
      for (int t = 0; t < 3; t++)
      {
         pgsTypes::IntervalTimeType timeType = (pgsTypes::IntervalTimeType)(t);
         Float64 age = pMaterials->GetDeckConcreteAge(regionIdx, tsDetails.intervalIdx, timeType);
         Float64 fc = pMaterials->GetDeckFc(regionIdx, tsDetails.intervalIdx, timeType);
         Float64 Ec = pMaterials->GetDeckEc(regionIdx, tsDetails.intervalIdx, timeType);

         (*pTable)(row, col++) << age;
         (*pTable)(row, col++) << stress.SetValue(fc);
         (*pTable)(row, col++) << modE.SetValue(Ec);

         if (timeType == pgsTypes::Middle)
         {
            Float64 X = pMaterials->GetDeckAgingCoefficient(regionIdx, tsDetails.intervalIdx);
            Float64 Y = pMaterials->GetDeckCreepCoefficient(regionIdx, tsDetails.intervalIdx, pgsTypes::Middle, tsDetails.intervalIdx, pgsTypes::End);
            Float64 Ece = pMaterials->GetDeckAgeAdjustedEc(regionIdx, tsDetails.intervalIdx);

            (*pTable)(row, col++) << X;
            (*pTable)(row, col++) << Y;
            (*pTable)(row, col++) << modE.SetValue(Ece);
         }
      }
      row++;
   }

   return pTable;
}

rptRcTable* CTimeStepDetailsChapterBuilder::BuildComponentPropertiesTable(const TIME_STEP_DETAILS& tsDetails,bool bHasDeck,IEAFDisplayUnits* pDisplayUnits) const
{
   INIT_UV_PROTOTYPE(rptLengthUnitValue,    ecc,        pDisplayUnits->GetComponentDimUnit(),    false);
   INIT_UV_PROTOTYPE(rptLengthUnitValue,    height,     pDisplayUnits->GetComponentDimUnit(),    false);
   INIT_UV_PROTOTYPE(rptLength2UnitValue,   area,       pDisplayUnits->GetAreaUnit(),            false);
   INIT_UV_PROTOTYPE(rptLength4UnitValue,   momI,       pDisplayUnits->GetMomentOfInertiaUnit(), false);
   INIT_UV_PROTOTYPE(rptStressUnitValue,    modE,       pDisplayUnits->GetModEUnit(),            false);

   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(6);
   pTable->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CJ_LEFT));
   pTable->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CJ_LEFT));

   RowIndexType rowIdx = 0;
   ColumnIndexType colIdx = 0;
   (*pTable)(rowIdx,colIdx++) << _T("Component");
   (*pTable)(rowIdx,colIdx++) << COLHDR(Sub2(_T("E"),_T("k")),rptStressUnitTag,pDisplayUnits->GetModEUnit());
   (*pTable)(rowIdx,colIdx++) << COLHDR(Sub2(_T("A"),_T("k")),rptLength2UnitTag,pDisplayUnits->GetAreaUnit());
   (*pTable)(rowIdx,colIdx++) << COLHDR(Sub2(_T("I"),_T("k")),rptLength4UnitTag,pDisplayUnits->GetMomentOfInertiaUnit());
   (*pTable)(rowIdx,colIdx++) << COLHDR(Sub2(_T("Y"),_T("k")),rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit());
   (*pTable)(rowIdx,colIdx++) << COLHDR(_T("H"),rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit());

   rowIdx = pTable->GetNumberOfHeaderRows();
   colIdx = 0;
   (*pTable)(rowIdx,colIdx++) << _T("Girder");
   (*pTable)(rowIdx,colIdx++) << modE.SetValue(tsDetails.Girder.Ea);
   (*pTable)(rowIdx,colIdx++) << area.SetValue(tsDetails.Girder.An);
   (*pTable)(rowIdx,colIdx++) << momI.SetValue(tsDetails.Girder.In);
   (*pTable)(rowIdx,colIdx++) << ecc.SetValue(tsDetails.Girder.Yn);
   (*pTable)(rowIdx,colIdx++) << height.SetValue(tsDetails.Girder.H);

   IndexType nRebar = tsDetails.GirderRebar.size();
   for ( IndexType i = 0; i < nRebar; i++ )
   {
      const TIME_STEP_REBAR& tsRebar = tsDetails.GirderRebar[i];

      rowIdx++;
      colIdx = 0;
      (*pTable)(rowIdx,colIdx++) << _T("Girder Rebar ") << LABEL_INDEX(i);
      (*pTable)(rowIdx,colIdx++) << modE.SetValue(tsRebar.E);
      (*pTable)(rowIdx,colIdx++) << area.SetValue(tsRebar.As);
      (*pTable)(rowIdx,colIdx++) << _T("-");
      (*pTable)(rowIdx,colIdx++) << ecc.SetValue(tsRebar.Ys);
      (*pTable)(rowIdx,colIdx++) << _T("-");
   }

   for ( int i = 0; i < 3; i++ )
   {
      rowIdx++;
      colIdx = 0;
      pgsTypes::StrandType strandType = (pgsTypes::StrandType)i;
      if ( strandType == pgsTypes::Straight )
      {
         (*pTable)(rowIdx,colIdx++) << _T("Straight Strands");
      }
      else if ( strandType == pgsTypes::Harped )
      {
         (*pTable)(rowIdx,colIdx++) << _T("Harped Strands");
      }
      else if ( strandType == pgsTypes::Temporary )
      {
         (*pTable)(rowIdx,colIdx++) << _T("Temporary Strands");
      }

      (*pTable)(rowIdx,colIdx++) << modE.SetValue(tsDetails.Strands[strandType].E);
      (*pTable)(rowIdx,colIdx++) << area.SetValue(tsDetails.Strands[strandType].As);
      (*pTable)(rowIdx,colIdx++) << _T("-");
      (*pTable)(rowIdx,colIdx++) << ecc.SetValue(tsDetails.Strands[strandType].Ys);
      (*pTable)(rowIdx,colIdx++) << _T("-");
   }

   if (bHasDeck)
   {
      rowIdx++;
      colIdx = 0;
      (*pTable)(rowIdx, colIdx++) << _T("Deck");
      (*pTable)(rowIdx, colIdx++) << modE.SetValue(tsDetails.Deck.Ea);
      (*pTable)(rowIdx, colIdx++) << area.SetValue(tsDetails.Deck.An);
      (*pTable)(rowIdx, colIdx++) << momI.SetValue(tsDetails.Deck.In);
      (*pTable)(rowIdx, colIdx++) << ecc.SetValue(tsDetails.Deck.Yn);
      (*pTable)(rowIdx, colIdx++) << height.SetValue(tsDetails.Deck.H);

      for (int i = 0; i < 2; i++)
      {
         pgsTypes::DeckRebarMatType matType = (pgsTypes::DeckRebarMatType)i;
         for (int j = 0; j < 2; j++)
         {
            rowIdx++;
            colIdx = 0;
            pgsTypes::DeckRebarBarType barType = (pgsTypes::DeckRebarBarType)j;
            if (matType == pgsTypes::drmTop)
            {
               if (barType == pgsTypes::drbIndividual)
               {
                  (*pTable)(rowIdx, colIdx++) << _T("Deck Top Mat Individual Rebar");
               }
               else
               {
                  (*pTable)(rowIdx, colIdx++) << _T("Deck Top Mat Lump Sum Rebar");
               }
            }
            else
            {
               if (barType == pgsTypes::drbIndividual)
               {
                  (*pTable)(rowIdx, colIdx++) << _T("Deck Bottom Mat Individual Rebar");
               }
               else
               {
                  (*pTable)(rowIdx, colIdx++) << _T("Deck Bottom Mat Lump Sum Rebar");
               }
            }

            (*pTable)(rowIdx, colIdx++) << modE.SetValue(tsDetails.DeckRebar[matType][barType].E);
            (*pTable)(rowIdx, colIdx++) << area.SetValue(tsDetails.DeckRebar[matType][barType].As);
            (*pTable)(rowIdx, colIdx++) << _T("-");
            (*pTable)(rowIdx, colIdx++) << ecc.SetValue(tsDetails.DeckRebar[matType][barType].Ys);
            (*pTable)(rowIdx, colIdx++) << _T("-");
         }
      }
   }

   DuctIndexType nTendons = tsDetails.SegmentTendons.size();
   for (DuctIndexType tendonIdx = 0; tendonIdx < nTendons; tendonIdx++)
   {
      const TIME_STEP_STRAND& tsTendon = tsDetails.SegmentTendons[tendonIdx];

      rowIdx++;
      colIdx = 0;
      (*pTable)(rowIdx, colIdx++) << _T("Segment Tendon ") << LABEL_DUCT(tendonIdx);
      (*pTable)(rowIdx, colIdx++) << modE.SetValue(tsTendon.E);
      (*pTable)(rowIdx, colIdx++) << area.SetValue(tsTendon.As);
      (*pTable)(rowIdx, colIdx++) << _T("-");
      (*pTable)(rowIdx, colIdx++) << ecc.SetValue(tsTendon.Ys);
      (*pTable)(rowIdx, colIdx++) << _T("-");
   }

   nTendons = tsDetails.GirderTendons.size();
   for ( DuctIndexType tendonIdx = 0; tendonIdx < nTendons; tendonIdx++ )
   {
      const TIME_STEP_STRAND& tsTendon = tsDetails.GirderTendons[tendonIdx];

      rowIdx++;
      colIdx = 0;
      (*pTable)(rowIdx,colIdx++) << _T("Girder Tendon ") << LABEL_DUCT(tendonIdx);
      (*pTable)(rowIdx,colIdx++) << modE.SetValue(tsTendon.E);
      (*pTable)(rowIdx,colIdx++) << area.SetValue(tsTendon.As);
      (*pTable)(rowIdx,colIdx++) << _T("-");
      (*pTable)(rowIdx,colIdx++) << ecc.SetValue(tsTendon.Ys);
      (*pTable)(rowIdx,colIdx++) << _T("-");
   }

   return pTable;
}

rptRcTable* CTimeStepDetailsChapterBuilder::BuildSectionPropertiesTable(const TIME_STEP_DETAILS& tsDetails,IEAFDisplayUnits* pDisplayUnits) const
{
   INIT_UV_PROTOTYPE(rptLengthUnitValue,    ecc,        pDisplayUnits->GetComponentDimUnit(),    false);
   INIT_UV_PROTOTYPE(rptLengthUnitValue,    height,     pDisplayUnits->GetComponentDimUnit(),    false);
   INIT_UV_PROTOTYPE(rptLength2UnitValue,   area,       pDisplayUnits->GetAreaUnit(),            false);
   INIT_UV_PROTOTYPE(rptLength4UnitValue,   momI,       pDisplayUnits->GetMomentOfInertiaUnit(), false);
   INIT_UV_PROTOTYPE(rptStressUnitValue,    modE,       pDisplayUnits->GetModEUnit(),            false);

   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(6);
   pTable->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CJ_LEFT));
   pTable->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CJ_LEFT));

   RowIndexType rowIdx = 0;
   ColumnIndexType colIdx = 0;
   (*pTable)(rowIdx,colIdx++) << _T("Component");
   (*pTable)(rowIdx,colIdx++) << COLHDR(Sub2(_T("E"),_T("tr")),rptStressUnitTag,pDisplayUnits->GetModEUnit());
   (*pTable)(rowIdx,colIdx++) << COLHDR(Sub2(_T("A"),_T("tr")),rptLength2UnitTag,pDisplayUnits->GetAreaUnit());
   (*pTable)(rowIdx,colIdx++) << COLHDR(Sub2(_T("I"),_T("tr")),rptLength4UnitTag,pDisplayUnits->GetMomentOfInertiaUnit());
   (*pTable)(rowIdx,colIdx++) << COLHDR(Sub2(_T("Y"),_T("tr")),rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit());
   (*pTable)(rowIdx,colIdx++) << COLHDR(_T("H"),rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit());


   rowIdx = pTable->GetNumberOfHeaderRows();
   colIdx = 0;
   (*pTable)(rowIdx,colIdx++) << _T("Composite Section");
   (*pTable)(rowIdx,colIdx++) << modE.SetValue(tsDetails.Girder.Ea);
   (*pTable)(rowIdx,colIdx++) << area.SetValue(tsDetails.Atr);
   (*pTable)(rowIdx,colIdx++) << momI.SetValue(tsDetails.Itr);
   (*pTable)(rowIdx,colIdx++) << ecc.SetValue(tsDetails.Ytr);
   (*pTable)(rowIdx,colIdx++) << height.SetValue(tsDetails.Girder.H + tsDetails.Deck.H);

   return pTable;
}

rptRcTable* CTimeStepDetailsChapterBuilder::BuildFreeCreepDeformationTable(const TIME_STEP_DETAILS& tsDetails, bool bHasDeck, IEAFDisplayUnits* pDisplayUnits) const
{
   INIT_UV_PROTOTYPE(rptLength2UnitValue,   area,       pDisplayUnits->GetAreaUnit(),            true);
   INIT_UV_PROTOTYPE(rptLength4UnitValue,   momI,       pDisplayUnits->GetMomentOfInertiaUnit(), true);
   INIT_UV_PROTOTYPE(rptStressUnitValue,    modE,       pDisplayUnits->GetModEUnit(),            true);
   INIT_UV_PROTOTYPE(rptForceUnitValue,     force,      pDisplayUnits->GetGeneralForceUnit(),    true);
   INIT_UV_PROTOTYPE(rptMomentUnitValue,    moment,     pDisplayUnits->GetSmallMomentUnit(),     true);
   INIT_UV_PROTOTYPE(rptPerLengthUnitValue, curvature,  pDisplayUnits->GetCurvatureUnit(),       false);

   ColumnIndexType nCols = 3;
   if (bHasDeck)
   {
      nCols += 2;
   }

   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(nCols);
   pTable->SetNumberOfHeaderRows(2);

   RowIndexType rowIdx = 0;
   ColumnIndexType colIdx = 0;
   pTable->SetRowSpan(rowIdx,colIdx,2);
   (*pTable)(rowIdx,colIdx++) << _T("Loading") << rptNewLine << _T("Interval");

   pTable->SetColumnSpan(rowIdx,colIdx,2);
   (*pTable)(rowIdx,colIdx) << _T("Girder");
   colIdx += 2;

   if (bHasDeck)
   {
      pTable->SetColumnSpan(rowIdx, colIdx, 2);
      (*pTable)(rowIdx, colIdx) << _T("Deck");
      colIdx += 2;
   }

   rowIdx++;
   colIdx = 1;
   (*pTable)(rowIdx,colIdx++) << DELTA_E << Super2(_T("x10"), _T("6"));
   (*pTable)(rowIdx,colIdx++) << COLHDR(DELTA_R << Super2(_T("x10"), _T("6")),rptPerLengthUnitTag,pDisplayUnits->GetCurvatureUnit());

   if (bHasDeck)
   {
      (*pTable)(rowIdx, colIdx++) << DELTA_E << Super2(_T("x10"), _T("6"));
      (*pTable)(rowIdx, colIdx++) << COLHDR(DELTA_R << Super2(_T("x10"), _T("6")), rptPerLengthUnitTag, pDisplayUnits->GetCurvatureUnit());
   }

   std::vector<TIME_STEP_CONCRETE::CREEP_STRAIN>::const_iterator girder_strain_iter(tsDetails.Girder.ec.begin());
   std::vector<TIME_STEP_CONCRETE::CREEP_STRAIN>::const_iterator girder_strain_iter_end(tsDetails.Girder.ec.end());
   std::vector<TIME_STEP_CONCRETE::CREEP_CURVATURE>::const_iterator girder_curvature_iter(tsDetails.Girder.rc.begin());
   std::vector<TIME_STEP_CONCRETE::CREEP_CURVATURE>::const_iterator girder_curvature_iter_end(tsDetails.Girder.rc.end());

   std::vector<TIME_STEP_CONCRETE::CREEP_STRAIN>::const_iterator deck_strain_iter(tsDetails.Deck.ec.begin());
   std::vector<TIME_STEP_CONCRETE::CREEP_STRAIN>::const_iterator deck_strain_iter_end(tsDetails.Deck.ec.end());
   std::vector<TIME_STEP_CONCRETE::CREEP_CURVATURE>::const_iterator deck_curvature_iter(tsDetails.Deck.rc.begin());
   std::vector<TIME_STEP_CONCRETE::CREEP_CURVATURE>::const_iterator deck_curvature_iter_end(tsDetails.Deck.rc.end());

   rowIdx = pTable->GetNumberOfHeaderRows();
   IntervalIndexType interval_of_load_application = 0;
   for ( ; girder_strain_iter != girder_strain_iter_end; girder_strain_iter++, girder_curvature_iter++, deck_strain_iter++, deck_curvature_iter++, interval_of_load_application++, rowIdx++ )
   {
      colIdx = 0;
      const TIME_STEP_CONCRETE::CREEP_STRAIN& girder_creep_strain(*girder_strain_iter);
      const TIME_STEP_CONCRETE::CREEP_CURVATURE& girder_creep_curvature(*girder_curvature_iter);

      const TIME_STEP_CONCRETE::CREEP_STRAIN& deck_creep_strain(*deck_strain_iter);
      const TIME_STEP_CONCRETE::CREEP_CURVATURE& deck_creep_curvature(*deck_curvature_iter);

      (*pTable)(rowIdx,colIdx++) << LABEL_INTERVAL(interval_of_load_application);
      (*pTable)(rowIdx,colIdx++) << _T("(") << force.SetValue(girder_creep_strain.P) << _T("/(") << area.SetValue(girder_creep_strain.A) << _T(")(") << modE.SetValue(girder_creep_strain.E) << _T("))[(") << girder_creep_strain.Xe << _T(")(") << girder_creep_strain.Ce << _T(") - (") << girder_creep_strain.Xs << _T(")(") << girder_creep_strain.Cs << _T(")]") << Super2(_T("x10"), _T("6")) << _T(" = ") << girder_creep_strain.e*1E6;
      (*pTable)(rowIdx,colIdx++) << _T("(") << moment.SetValue(girder_creep_curvature.M) << _T("/(") << momI.SetValue(girder_creep_curvature.I) << _T(")(") << modE.SetValue(girder_creep_curvature.E) << _T("))[(") << girder_creep_curvature.Xe << _T(")(") << girder_creep_curvature.Ce << _T(") - (") << girder_creep_curvature.Xs << _T(")(") << girder_creep_curvature.Cs << _T(")]") << Super2(_T("x10"), _T("6")) << _T(" = ") << curvature.SetValue(girder_creep_curvature.r*1E6);

      if (bHasDeck)
      {
         (*pTable)(rowIdx, colIdx++) << _T("(") << force.SetValue(deck_creep_strain.P) << _T("/(") << area.SetValue(deck_creep_strain.A) << _T(")(") << modE.SetValue(deck_creep_strain.E) << _T("))[(") << deck_creep_strain.Xe << _T(")(") << deck_creep_strain.Ce << _T(") - (") << deck_creep_strain.Xs << _T(")(") << deck_creep_strain.Cs << _T(")]") << Super2(_T("x10"), _T("6")) << _T(" = ") << deck_creep_strain.e*1E6;
         (*pTable)(rowIdx, colIdx++) << _T("(") << moment.SetValue(deck_creep_curvature.M) << _T("/(") << momI.SetValue(deck_creep_curvature.I) << _T(")(") << modE.SetValue(deck_creep_curvature.E) << _T("))[(") << deck_creep_curvature.Xe << _T(")(") << deck_creep_curvature.Ce << _T(") - (") << deck_creep_curvature.Xs << _T(")(") << deck_creep_curvature.Cs << _T(")]") << Super2(_T("x10"), _T("6")) << _T(" = ") << curvature.SetValue(deck_creep_curvature.r*1E6);
      }
   }

   colIdx = 0;
   (*pTable)(rowIdx,colIdx++) << _T("Total");
   (*pTable)(rowIdx,colIdx++) << tsDetails.Girder.eci*1E6;
   (*pTable)(rowIdx,colIdx++) << curvature.SetValue(tsDetails.Girder.rci*1E6);
   if (bHasDeck)
   {
      (*pTable)(rowIdx, colIdx++) << tsDetails.Deck.eci*1E6;
      (*pTable)(rowIdx, colIdx++) << curvature.SetValue(tsDetails.Deck.rci*1E6);
   }

   return pTable;
}

rptRcTable* CTimeStepDetailsChapterBuilder::BuildStrandRelaxationTable(const TIME_STEP_DETAILS& tsDetails,IEAFDisplayUnits* pDisplayUnits) const
{
   INIT_UV_PROTOTYPE(rptStressUnitValue,    stress,     pDisplayUnits->GetStressUnit(),          false);

   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(3);
   RowIndexType rowIdx = 0;
   ColumnIndexType colIdx = 0;

   (*pTable)(rowIdx,colIdx++) << _T("Strand");
   (*pTable)(rowIdx,colIdx++) << COLHDR(DELTA_FR,rptStressUnitTag,pDisplayUnits->GetStressUnit());
   (*pTable)(rowIdx,colIdx++) << DELTA_ER << Super2(_T("x10"), _T("6"));

   rowIdx = pTable->GetNumberOfHeaderRows();
   for ( int i = 0; i < 3; i++, rowIdx++ )
   {
      colIdx = 0;
      pgsTypes::StrandType strandType = (pgsTypes::StrandType)i;
      if ( strandType == pgsTypes::Straight )
      {
         (*pTable)(rowIdx,colIdx++) << _T("Straight");
      }
      else if ( strandType == pgsTypes::Harped )
      {
         (*pTable)(rowIdx,colIdx++) << _T("Harped");
      }
      else if ( strandType == pgsTypes::Temporary )
      {
         (*pTable)(rowIdx,colIdx++) << _T("Temporary");
      }

      (*pTable)(rowIdx,colIdx++) << stress.SetValue(tsDetails.Strands[strandType].Relaxation.fr);
      (*pTable)(rowIdx,colIdx++) << tsDetails.Strands[strandType].er*1E6;
   }

   return pTable;
}

rptRcTable* CTimeStepDetailsChapterBuilder::BuildSegmentTendonRelaxationTable(const TIME_STEP_DETAILS& tsDetails,IEAFDisplayUnits* pDisplayUnits) const
{
   INIT_UV_PROTOTYPE(rptStressUnitValue,    stress,     pDisplayUnits->GetStressUnit(),          false);

   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(3);
   RowIndexType rowIdx = 0;
   ColumnIndexType colIdx = 0;

   (*pTable)(rowIdx,colIdx++) << _T("Segment Tendon");
   (*pTable)(rowIdx,colIdx++) << COLHDR(DELTA_FR,rptStressUnitTag,pDisplayUnits->GetStressUnit());
   (*pTable)(rowIdx,colIdx++) << DELTA_ER << Super2(_T("x10"), _T("6"));

   rowIdx = pTable->GetNumberOfHeaderRows();
   DuctIndexType nTendons = tsDetails.SegmentTendons.size();
   for ( DuctIndexType tendonIdx = 0; tendonIdx < nTendons; tendonIdx++, rowIdx++ )
   {
      colIdx = 0;
      (*pTable)(rowIdx,colIdx++) << LABEL_DUCT(tendonIdx);
      (*pTable)(rowIdx,colIdx++) << stress.SetValue(tsDetails.SegmentTendons[tendonIdx].Relaxation.fr);
      (*pTable)(rowIdx,colIdx++) << tsDetails.SegmentTendons[tendonIdx].er*1E6;
   }

   return pTable;
}

rptRcTable* CTimeStepDetailsChapterBuilder::BuildGirderTendonRelaxationTable(const TIME_STEP_DETAILS& tsDetails, IEAFDisplayUnits* pDisplayUnits) const
{
   INIT_UV_PROTOTYPE(rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), false);

   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(3);
   RowIndexType rowIdx = 0;
   ColumnIndexType colIdx = 0;

   (*pTable)(rowIdx, colIdx++) << _T("Girder Tendon");
   (*pTable)(rowIdx, colIdx++) << COLHDR(DELTA_FR, rptStressUnitTag, pDisplayUnits->GetStressUnit());
   (*pTable)(rowIdx, colIdx++) << DELTA_ER << Super2(_T("x10"), _T("6"));

   rowIdx = pTable->GetNumberOfHeaderRows();
   DuctIndexType nTendons = tsDetails.GirderTendons.size();
   for (DuctIndexType tendonIdx = 0; tendonIdx < nTendons; tendonIdx++, rowIdx++)
   {
      colIdx = 0;
      (*pTable)(rowIdx, colIdx++) << LABEL_DUCT(tendonIdx);
      (*pTable)(rowIdx, colIdx++) << stress.SetValue(tsDetails.GirderTendons[tendonIdx].Relaxation.fr);
      (*pTable)(rowIdx, colIdx++) << tsDetails.GirderTendons[tendonIdx].er*1E6;
   }

   return pTable;
}

rptRcTable* CTimeStepDetailsChapterBuilder::BuildComponentRestrainingForceTable(const TIME_STEP_DETAILS& tsDetails, bool bHasDeck, IEAFDisplayUnits* pDisplayUnits) const
{
   INIT_UV_PROTOTYPE(rptStressUnitValue,    stress,     pDisplayUnits->GetStressUnit(),          true);
   INIT_UV_PROTOTYPE(rptLength2UnitValue,   area,       pDisplayUnits->GetAreaUnit(),            true);
   INIT_UV_PROTOTYPE(rptLength4UnitValue,   momI,       pDisplayUnits->GetMomentOfInertiaUnit(), true);
   INIT_UV_PROTOTYPE(rptStressUnitValue,    modE,       pDisplayUnits->GetModEUnit(),            true);
   INIT_UV_PROTOTYPE(rptForceUnitValue,     force,      pDisplayUnits->GetGeneralForceUnit(),    true);
   INIT_UV_PROTOTYPE(rptMomentUnitValue,    moment,     pDisplayUnits->GetSmallMomentUnit(),     true);
   INIT_UV_PROTOTYPE(rptPerLengthUnitValue, curvature,  pDisplayUnits->GetCurvatureUnit(),       true);

   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(7);
   RowIndexType rowIdx = 0;
   ColumnIndexType colIdx = 0;

   pTable->SetNumberOfHeaderRows(2);
   pTable->SetRowSpan(rowIdx,colIdx,2);
   (*pTable)(rowIdx,colIdx++) << _T("Component");

   pTable->SetColumnSpan(rowIdx,colIdx,2);
   (*pTable)(rowIdx,colIdx) << _T("Creep");
   colIdx += 2;

   pTable->SetColumnSpan(rowIdx,colIdx,2);
   (*pTable)(rowIdx,colIdx) << _T("Shrinkage");
   colIdx += 2;

   pTable->SetColumnSpan(rowIdx,colIdx,2);
   (*pTable)(rowIdx,colIdx) << _T("Relaxation");
   colIdx += 2;
   
   rowIdx++;
   colIdx = 1;
   (*pTable)(rowIdx,colIdx++) << COLHDR(symbol(DELTA) << Sub2(_T("P"),_T("r")),rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());
   (*pTable)(rowIdx,colIdx++) << COLHDR(symbol(DELTA) << Sub2(_T("M"),_T("r")),rptMomentUnitTag,pDisplayUnits->GetSmallMomentUnit());
   (*pTable)(rowIdx,colIdx++) << COLHDR(symbol(DELTA) << Sub2(_T("P"),_T("r")),rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());
   (*pTable)(rowIdx,colIdx++) << COLHDR(symbol(DELTA) << Sub2(_T("M"),_T("r")),rptMomentUnitTag,pDisplayUnits->GetSmallMomentUnit());
   (*pTable)(rowIdx,colIdx++) << COLHDR(symbol(DELTA) << Sub2(_T("P"),_T("r")),rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());
   (*pTable)(rowIdx,colIdx++) << COLHDR(symbol(DELTA) << Sub2(_T("M"),_T("r")),rptMomentUnitTag,pDisplayUnits->GetSmallMomentUnit());

   rowIdx = pTable->GetNumberOfHeaderRows();
   colIdx = 0;
   (*pTable)(rowIdx,colIdx++) << _T("Girder");
   (*pTable)(rowIdx,colIdx++) << _T("-(") << tsDetails.Girder.eci*1e6 << _T(")(") << Super2(_T("10"),_T("-6")) << _T(")(") << modE.SetValue(tsDetails.Girder.E) << _T(")(") << area.SetValue(tsDetails.Girder.An) << _T(") = ") << force.SetValue(tsDetails.Girder.PrCreep);
   (*pTable)(rowIdx,colIdx++) << _T("-(") << curvature.SetValue(tsDetails.Girder.rci*1e6) << _T(")(") << Super2(_T("10"), _T("-6")) << _T(")(") << modE.SetValue(tsDetails.Girder.E) << _T(")(") << momI.SetValue(tsDetails.Girder.In) << _T(") = ") << moment.SetValue(tsDetails.Girder.MrCreep);
   (*pTable)(rowIdx,colIdx++) << _T("-(") << tsDetails.Girder.Shrinkage.esi*1e6 << _T(")(") << Super2(_T("10"), _T("-6")) << _T(")(") << modE.SetValue(tsDetails.Girder.E) << _T(")(") << area.SetValue(tsDetails.Girder.An) << _T(") = ") << force.SetValue(tsDetails.Girder.PrShrinkage);
   (*pTable)(rowIdx,colIdx++) << _T("");
   (*pTable)(rowIdx,colIdx++) << _T("");
   (*pTable)(rowIdx,colIdx++) << _T("");

   if (bHasDeck)
   {
      rowIdx++;
      colIdx = 0;
      (*pTable)(rowIdx, colIdx++) << _T("Deck");
      (*pTable)(rowIdx, colIdx++) << _T("-(") << tsDetails.Deck.eci*1e6 << _T(")(") << Super2(_T("10"), _T("-6")) << _T(")(") << modE.SetValue(tsDetails.Deck.E) << _T(")(") << area.SetValue(tsDetails.Deck.An) << _T(") = ") << force.SetValue(tsDetails.Deck.PrCreep);
      (*pTable)(rowIdx, colIdx++) << _T("-(") << curvature.SetValue(tsDetails.Deck.rci*1e6) << _T(")(") << Super2(_T("10"), _T("-6")) << _T(")(") << modE.SetValue(tsDetails.Deck.E) << _T(")(") << momI.SetValue(tsDetails.Deck.In) << _T(") = ") << moment.SetValue(tsDetails.Deck.MrCreep);
      (*pTable)(rowIdx, colIdx++) << _T("-(") << tsDetails.Deck.Shrinkage.esi*1e6 << _T(")(") << Super2(_T("10"), _T("-6")) << _T(")(") << modE.SetValue(tsDetails.Deck.E) << _T(")(") << area.SetValue(tsDetails.Deck.An) << _T(") = ") << force.SetValue(tsDetails.Deck.PrShrinkage);
      (*pTable)(rowIdx, colIdx++) << _T("");
      (*pTable)(rowIdx, colIdx++) << _T("");
      (*pTable)(rowIdx, colIdx++) << _T("");
   }

   for ( int i = 0; i < 3; i++ )
   {
      rowIdx++;
      colIdx = 0;
      pgsTypes::StrandType strandType = (pgsTypes::StrandType)i;
      if ( strandType == pgsTypes::Straight )
      {
         (*pTable)(rowIdx,colIdx++) << _T("Straight Strands");
      }
      else if ( strandType == pgsTypes::Harped )
      {
         (*pTable)(rowIdx,colIdx++) << _T("Harped Strands");
      }
      else if ( strandType == pgsTypes::Temporary )
      {
         (*pTable)(rowIdx,colIdx++) << _T("Temporary Strands");
      }

      (*pTable)(rowIdx,colIdx++) << _T("");
      (*pTable)(rowIdx,colIdx++) << _T("");
      (*pTable)(rowIdx,colIdx++) << _T("");
      (*pTable)(rowIdx,colIdx++) << _T("");
      (*pTable)(rowIdx,colIdx++) << _T("-(") << tsDetails.Strands[strandType].er*1e6 << _T(")(") << Super2(_T("10"), _T("-6")) << _T(")(") << modE.SetValue(tsDetails.Strands[strandType].E) << _T(")(") << area.SetValue(tsDetails.Strands[strandType].As) << _T(") = ") << force.SetValue(tsDetails.Strands[strandType].PrRelaxation);
      (*pTable)(rowIdx,colIdx++) << _T("");
   }

   DuctIndexType nTendons = tsDetails.SegmentTendons.size();
   for (DuctIndexType tendonIdx = 0; tendonIdx < nTendons; tendonIdx++)
   {
      rowIdx++;
      colIdx = 0;
      (*pTable)(rowIdx, colIdx++) << _T("Segment Tendon ") << LABEL_DUCT(tendonIdx);
      (*pTable)(rowIdx, colIdx++) << _T("");
      (*pTable)(rowIdx, colIdx++) << _T("");
      (*pTable)(rowIdx, colIdx++) << _T("");
      (*pTable)(rowIdx, colIdx++) << _T("");
      (*pTable)(rowIdx, colIdx++) << _T("-(") << tsDetails.SegmentTendons[tendonIdx].er*1e6 << _T(")(") << Super2(_T("10"), _T("-6")) << _T(")(") << modE.SetValue(tsDetails.SegmentTendons[tendonIdx].E) << _T(")(") << area.SetValue(tsDetails.SegmentTendons[tendonIdx].As) << _T(") = ") << force.SetValue(tsDetails.SegmentTendons[tendonIdx].PrRelaxation);
      (*pTable)(rowIdx, colIdx++) << _T("");
   }

   nTendons = tsDetails.GirderTendons.size();
   for ( DuctIndexType tendonIdx = 0; tendonIdx < nTendons; tendonIdx++ )
   {
      rowIdx++;
      colIdx = 0;
      (*pTable)(rowIdx,colIdx++) << _T("Girder Tendon ") << LABEL_DUCT(tendonIdx);
      (*pTable)(rowIdx,colIdx++) << _T("");
      (*pTable)(rowIdx,colIdx++) << _T("");
      (*pTable)(rowIdx,colIdx++) << _T("");
      (*pTable)(rowIdx,colIdx++) << _T("");
      (*pTable)(rowIdx,colIdx++) << _T("-(") << tsDetails.GirderTendons[tendonIdx].er*1e6 << _T(")(") << Super2(_T("10"), _T("-6")) << _T(")(") << modE.SetValue(tsDetails.GirderTendons[tendonIdx].E) << _T(")(") << area.SetValue(tsDetails.GirderTendons[tendonIdx].As) << _T(") = ") << force.SetValue(tsDetails.GirderTendons[tendonIdx].PrRelaxation);
      (*pTable)(rowIdx,colIdx++) << _T("");
   }

   return pTable;
}

rptRcTable* CTimeStepDetailsChapterBuilder::BuildSectionRestrainingForceTable(const TIME_STEP_DETAILS& tsDetails,IEAFDisplayUnits* pDisplayUnits) const
{
   INIT_UV_PROTOTYPE(rptForceUnitValue,     force,      pDisplayUnits->GetGeneralForceUnit(),    false);
   INIT_UV_PROTOTYPE(rptMomentUnitValue,    moment,     pDisplayUnits->GetSmallMomentUnit(),     false);

   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(7);
   RowIndexType rowIdx = 0;
   ColumnIndexType colIdx = 0;

   pTable->SetNumberOfHeaderRows(2);
   pTable->SetRowSpan(rowIdx,colIdx,2);
   (*pTable)(rowIdx,colIdx++) << _T("Component");

   pTable->SetColumnSpan(rowIdx,colIdx,2);
   (*pTable)(rowIdx,colIdx) << _T("Creep");
   colIdx += 2;

   pTable->SetColumnSpan(rowIdx,colIdx,2);
   (*pTable)(rowIdx,colIdx) << _T("Shrinkage");
   colIdx += 2;

   pTable->SetColumnSpan(rowIdx,colIdx,2);
   (*pTable)(rowIdx,colIdx) << _T("Relaxation");
   colIdx += 2;
   
   rowIdx++;
   colIdx = 1;
   (*pTable)(rowIdx,colIdx++) << COLHDR(Sub2(_T("P"),_T("r")),rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());
   (*pTable)(rowIdx,colIdx++) << COLHDR(Sub2(_T("M"),_T("r")),rptMomentUnitTag,pDisplayUnits->GetSmallMomentUnit());
   (*pTable)(rowIdx,colIdx++) << COLHDR(Sub2(_T("P"),_T("r")),rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());
   (*pTable)(rowIdx,colIdx++) << COLHDR(Sub2(_T("M"),_T("r")),rptMomentUnitTag,pDisplayUnits->GetSmallMomentUnit());
   (*pTable)(rowIdx,colIdx++) << COLHDR(Sub2(_T("P"),_T("r")),rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());
   (*pTable)(rowIdx,colIdx++) << COLHDR(Sub2(_T("M"),_T("r")),rptMomentUnitTag,pDisplayUnits->GetSmallMomentUnit());

   rowIdx = pTable->GetNumberOfHeaderRows();
   colIdx = 0;
   (*pTable)(rowIdx,colIdx++) << _T("Composite Section");
   (*pTable)(rowIdx,colIdx++) << force.SetValue(tsDetails.Pr[TIMESTEP_CR]);
   (*pTable)(rowIdx,colIdx++) << moment.SetValue(tsDetails.Mr[TIMESTEP_CR]);
   (*pTable)(rowIdx,colIdx++) << force.SetValue(tsDetails.Pr[TIMESTEP_SH]);
   (*pTable)(rowIdx,colIdx++) << moment.SetValue(tsDetails.Mr[TIMESTEP_SH]);
   (*pTable)(rowIdx,colIdx++) << force.SetValue(tsDetails.Pr[TIMESTEP_RE]);
   (*pTable)(rowIdx,colIdx++) << moment.SetValue(tsDetails.Mr[TIMESTEP_RE]);

   return pTable;
}

rptRcTable* CTimeStepDetailsChapterBuilder::BuildSectionRestrainingDeformationTable(const TIME_STEP_DETAILS& tsDetails,IEAFDisplayUnits* pDisplayUnits) const
{
   INIT_UV_PROTOTYPE(rptPerLengthUnitValue, curvature,  pDisplayUnits->GetCurvatureUnit(),       false);

   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(7);
   RowIndexType rowIdx = 0;
   ColumnIndexType colIdx = 0;

   pTable->SetNumberOfHeaderRows(2);
   pTable->SetRowSpan(rowIdx,colIdx,2);
   (*pTable)(rowIdx,colIdx++) << _T("Component");

   pTable->SetColumnSpan(rowIdx,colIdx,2);
   (*pTable)(rowIdx,colIdx) << _T("Creep");
   colIdx += 2;

   pTable->SetColumnSpan(rowIdx,colIdx,2);
   (*pTable)(rowIdx,colIdx) << _T("Shrinkage");
   colIdx += 2;

   pTable->SetColumnSpan(rowIdx,colIdx,2);
   (*pTable)(rowIdx,colIdx) << _T("Relaxation");
   colIdx += 2;
   
   rowIdx++;
   colIdx = 1;
   (*pTable)(rowIdx,colIdx++) << Sub2(symbol(epsilon),_T("r")) << Super2(_T("x10"), _T("6"));
   (*pTable)(rowIdx,colIdx++) << COLHDR(Sub2(symbol(varphi),_T("r")) << Super2(_T("x10"), _T("6")),rptPerLengthUnitTag,pDisplayUnits->GetCurvatureUnit());
   (*pTable)(rowIdx,colIdx++) << Sub2(symbol(epsilon),_T("r")) << Super2(_T("x10"), _T("6"));
   (*pTable)(rowIdx,colIdx++) << COLHDR(Sub2(symbol(varphi),_T("r")) << Super2(_T("x10"), _T("6")),rptPerLengthUnitTag,pDisplayUnits->GetCurvatureUnit());
   (*pTable)(rowIdx,colIdx++) << Sub2(symbol(epsilon),_T("r")) << Super2(_T("x10"), _T("6"));
   (*pTable)(rowIdx,colIdx++) << COLHDR(Sub2(symbol(varphi),_T("r")) << Super2(_T("x10"), _T("6")),rptPerLengthUnitTag,pDisplayUnits->GetCurvatureUnit());

   rowIdx = pTable->GetNumberOfHeaderRows();
   colIdx = 0;
   (*pTable)(rowIdx,colIdx++) << _T("Composite Section");
   (*pTable)(rowIdx,colIdx++) << tsDetails.e[TIMESTEP_CR]*1E6;
   (*pTable)(rowIdx,colIdx++) << curvature.SetValue(tsDetails.r[TIMESTEP_CR]*1E6);
   (*pTable)(rowIdx,colIdx++) << tsDetails.e[TIMESTEP_SH]*1E6;
   (*pTable)(rowIdx,colIdx++) << curvature.SetValue(tsDetails.r[TIMESTEP_SH]*1E6);
   (*pTable)(rowIdx,colIdx++) << tsDetails.e[TIMESTEP_RE]*1E6;
   (*pTable)(rowIdx,colIdx++) << curvature.SetValue(tsDetails.r[TIMESTEP_RE]*1E6);

   return pTable;
}

rptRcTable* CTimeStepDetailsChapterBuilder::BuildRestrainedSectionForceTable(const TIME_STEP_DETAILS& tsDetails,IEAFDisplayUnits* pDisplayUnits) const
{
   INIT_UV_PROTOTYPE(rptForceUnitValue,     force,      pDisplayUnits->GetGeneralForceUnit(),    false);
   INIT_UV_PROTOTYPE(rptMomentUnitValue,    moment,     pDisplayUnits->GetSmallMomentUnit(),     false);

   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(10);
   RowIndexType rowIdx = 0;
   ColumnIndexType colIdx = 0;

   pTable->SetNumberOfHeaderRows(2);
   pTable->SetRowSpan(rowIdx,colIdx,2);
   (*pTable)(rowIdx,colIdx++) << _T("Component");

   pTable->SetColumnSpan(rowIdx,colIdx,3);
   (*pTable)(rowIdx,colIdx) << _T("Creep");
   colIdx += 3;

   pTable->SetColumnSpan(rowIdx,colIdx,3);
   (*pTable)(rowIdx,colIdx) << _T("Shrinkage");
   colIdx += 3;

   pTable->SetColumnSpan(rowIdx,colIdx,3);
   (*pTable)(rowIdx,colIdx) << _T("Relaxation");
   colIdx += 3;
   
   rowIdx++;
   colIdx = 1;
   (*pTable)(rowIdx,colIdx++) << COLHDR(Overline(_T("P")) << Sub(_T("r")),rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());
   (*pTable)(rowIdx,colIdx++) << COLHDR(Overline(_T("M")) << Sub(_T("r")),rptMomentUnitTag,pDisplayUnits->GetSmallMomentUnit());
   (*pTable)(rowIdx,colIdx++) << COLHDR(Overline(_T("V")) << Sub(_T("r")),rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());
   (*pTable)(rowIdx,colIdx++) << COLHDR(Overline(_T("P")) << Sub(_T("r")),rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());
   (*pTable)(rowIdx,colIdx++) << COLHDR(Overline(_T("M")) << Sub(_T("r")),rptMomentUnitTag,pDisplayUnits->GetSmallMomentUnit());
   (*pTable)(rowIdx,colIdx++) << COLHDR(Overline(_T("V")) << Sub(_T("r")),rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());
   (*pTable)(rowIdx,colIdx++) << COLHDR(Overline(_T("P")) << Sub(_T("r")),rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());
   (*pTable)(rowIdx,colIdx++) << COLHDR(Overline(_T("M")) << Sub(_T("r")),rptMomentUnitTag,pDisplayUnits->GetSmallMomentUnit());
   (*pTable)(rowIdx,colIdx++) << COLHDR(Overline(_T("V")) << Sub(_T("r")),rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());


   rowIdx = pTable->GetNumberOfHeaderRows();
   colIdx = 0;
   (*pTable)(rowIdx,colIdx++) << _T("Composite Section");
   (*pTable)(rowIdx,colIdx++) <<  force.SetValue(tsDetails.dPi[pgsTypes::pftCreep]);
   (*pTable)(rowIdx,colIdx++) << moment.SetValue(tsDetails.dMi[pgsTypes::pftCreep]);
   (*pTable)(rowIdx,colIdx++) <<  force.SetValue(tsDetails.dVi[pgsTypes::pftCreep]);
   (*pTable)(rowIdx,colIdx++) <<  force.SetValue(tsDetails.dPi[pgsTypes::pftShrinkage]);
   (*pTable)(rowIdx,colIdx++) << moment.SetValue(tsDetails.dMi[pgsTypes::pftShrinkage]);
   (*pTable)(rowIdx,colIdx++) <<  force.SetValue(tsDetails.dVi[pgsTypes::pftShrinkage]);
   (*pTable)(rowIdx,colIdx++) <<  force.SetValue(tsDetails.dPi[pgsTypes::pftRelaxation]);
   (*pTable)(rowIdx,colIdx++) << moment.SetValue(tsDetails.dMi[pgsTypes::pftRelaxation]);
   (*pTable)(rowIdx,colIdx++) <<  force.SetValue(tsDetails.dVi[pgsTypes::pftRelaxation]);

   return pTable;
}

rptRcTable* CTimeStepDetailsChapterBuilder::BuildRestrainedComponentForceTable(const TIME_STEP_DETAILS& tsDetails, bool bHasDeck, IEAFDisplayUnits* pDisplayUnits) const
{
   INIT_UV_PROTOTYPE(rptForceUnitValue,     force,      pDisplayUnits->GetGeneralForceUnit(),    false);
   INIT_UV_PROTOTYPE(rptMomentUnitValue,    moment,     pDisplayUnits->GetSmallMomentUnit(),     false);

   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(7);
   RowIndexType rowIdx = 0;
   ColumnIndexType colIdx = 0;

   pTable->SetNumberOfHeaderRows(2);
   pTable->SetRowSpan(rowIdx, colIdx, 2);
   (*pTable)(rowIdx, colIdx++) << _T("Component");

   pTable->SetColumnSpan(rowIdx, colIdx, 2);
   (*pTable)(rowIdx, colIdx) << _T("Creep");
   colIdx += 2;

   pTable->SetColumnSpan(rowIdx, colIdx, 2);
   (*pTable)(rowIdx, colIdx) << _T("Shrinkage");
   colIdx += 2;

   pTable->SetColumnSpan(rowIdx, colIdx, 2);
   (*pTable)(rowIdx, colIdx) << _T("Relaxation");
   colIdx += 2;

   rowIdx++;
   colIdx = 1;
   (*pTable)(rowIdx,colIdx++) << COLHDR(symbol(DELTA) << _T("P") << Sub(_T("k")),rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());
   (*pTable)(rowIdx,colIdx++) << COLHDR(symbol(DELTA) << _T("M") << Sub(_T("k")),rptMomentUnitTag,pDisplayUnits->GetSmallMomentUnit());
   (*pTable)(rowIdx,colIdx++) << COLHDR(symbol(DELTA) << _T("P") << Sub(_T("k")),rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());
   (*pTable)(rowIdx,colIdx++) << COLHDR(symbol(DELTA) << _T("M") << Sub(_T("k")),rptMomentUnitTag,pDisplayUnits->GetSmallMomentUnit());
   (*pTable)(rowIdx,colIdx++) << COLHDR(symbol(DELTA) << _T("P") << Sub(_T("k")),rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());
   (*pTable)(rowIdx,colIdx++) << COLHDR(symbol(DELTA) << _T("M") << Sub(_T("k")),rptMomentUnitTag,pDisplayUnits->GetSmallMomentUnit());


   rowIdx = pTable->GetNumberOfHeaderRows();
   colIdx = 0;
   (*pTable)(rowIdx,colIdx++) << _T("Girder");
   (*pTable)(rowIdx,colIdx++) << force.SetValue(tsDetails.Girder.dPi[pgsTypes::pftCreep]);
   (*pTable)(rowIdx,colIdx++) << moment.SetValue(tsDetails.Girder.dMi[pgsTypes::pftCreep]);
   (*pTable)(rowIdx,colIdx++) << force.SetValue(tsDetails.Girder.dPi[pgsTypes::pftShrinkage]);
   (*pTable)(rowIdx,colIdx++) << moment.SetValue(tsDetails.Girder.dMi[pgsTypes::pftShrinkage]);
   (*pTable)(rowIdx,colIdx++) << force.SetValue(tsDetails.Girder.dPi[pgsTypes::pftRelaxation]);
   (*pTable)(rowIdx,colIdx++) << moment.SetValue(tsDetails.Girder.dMi[pgsTypes::pftRelaxation]);

   IndexType nRebar = tsDetails.GirderRebar.size();
   for ( IndexType i = 0; i < nRebar; i++ )
   {
      const TIME_STEP_REBAR& tsRebar = tsDetails.GirderRebar[i];

      rowIdx++;
      colIdx = 0;
      (*pTable)(rowIdx,colIdx++) << _T("Girder Rebar ") << LABEL_INDEX(i);
      (*pTable)(rowIdx,colIdx++) << force.SetValue(tsRebar.dPi[pgsTypes::pftCreep]);
      (*pTable)(rowIdx,colIdx++) << _T("");;
      (*pTable)(rowIdx,colIdx++) << force.SetValue(tsRebar.dPi[pgsTypes::pftShrinkage]);
      (*pTable)(rowIdx,colIdx++) << _T("");
      (*pTable)(rowIdx,colIdx++) << force.SetValue(tsRebar.dPi[pgsTypes::pftRelaxation]);
      (*pTable)(rowIdx,colIdx++) << _T("");
   }

   for ( int i = 0; i < 3; i++ )
   {
      rowIdx++;
      colIdx = 0;
      pgsTypes::StrandType strandType = (pgsTypes::StrandType)i;
      if ( strandType == pgsTypes::Straight )
      {
         (*pTable)(rowIdx,colIdx++) << _T("Straight Strands");
      }
      else if ( strandType == pgsTypes::Harped )
      {
         (*pTable)(rowIdx,colIdx++) << _T("Harped Strands");
      }
      else if ( strandType == pgsTypes::Temporary )
      {
         (*pTable)(rowIdx,colIdx++) << _T("Temporary Strands");
      }

      (*pTable)(rowIdx,colIdx++) << force.SetValue(tsDetails.Strands[strandType].dPi[pgsTypes::pftCreep]);
      (*pTable)(rowIdx,colIdx++) << _T("");
      (*pTable)(rowIdx,colIdx++) << force.SetValue(tsDetails.Strands[strandType].dPi[pgsTypes::pftShrinkage]);
      (*pTable)(rowIdx,colIdx++) << _T("");
      (*pTable)(rowIdx,colIdx++) << force.SetValue(tsDetails.Strands[strandType].dPi[pgsTypes::pftRelaxation]);
      (*pTable)(rowIdx,colIdx++) << _T("");
   }

   if (bHasDeck)
   {
      rowIdx++;
      colIdx = 0;
      (*pTable)(rowIdx, colIdx++) << _T("Deck");
      (*pTable)(rowIdx, colIdx++) << force.SetValue(tsDetails.Deck.dPi[pgsTypes::pftCreep]);
      (*pTable)(rowIdx, colIdx++) << moment.SetValue(tsDetails.Deck.dMi[pgsTypes::pftCreep]);
      (*pTable)(rowIdx, colIdx++) << force.SetValue(tsDetails.Deck.dPi[pgsTypes::pftShrinkage]);
      (*pTable)(rowIdx, colIdx++) << moment.SetValue(tsDetails.Deck.dMi[pgsTypes::pftShrinkage]);
      (*pTable)(rowIdx, colIdx++) << force.SetValue(tsDetails.Deck.dPi[pgsTypes::pftRelaxation]);
      (*pTable)(rowIdx, colIdx++) << moment.SetValue(tsDetails.Deck.dMi[pgsTypes::pftRelaxation]);

      for (int i = 0; i < 2; i++)
      {
         pgsTypes::DeckRebarMatType matType = (pgsTypes::DeckRebarMatType)i;
         for (int j = 0; j < 2; j++)
         {
            rowIdx++;
            colIdx = 0;
            pgsTypes::DeckRebarBarType barType = (pgsTypes::DeckRebarBarType)j;
            if (matType == pgsTypes::drmTop)
            {
               if (barType == pgsTypes::drbIndividual)
               {
                  (*pTable)(rowIdx, colIdx++) << _T("Deck Top Mat Individual Rebar");
               }
               else
               {
                  (*pTable)(rowIdx, colIdx++) << _T("Deck Top Mat Lump Sum Rebar");
               }
            }
            else
            {
               if (barType == pgsTypes::drbIndividual)
               {
                  (*pTable)(rowIdx, colIdx++) << _T("Deck Bottom Mat Individual Rebar");
               }
               else
               {
                  (*pTable)(rowIdx, colIdx++) << _T("Deck Bottom Mat Lump Sum Rebar");
               }
            }

            (*pTable)(rowIdx, colIdx++) << force.SetValue(tsDetails.DeckRebar[matType][barType].dPi[pgsTypes::pftCreep]);
            (*pTable)(rowIdx, colIdx++) << _T("");
            (*pTable)(rowIdx, colIdx++) << force.SetValue(tsDetails.DeckRebar[matType][barType].dPi[pgsTypes::pftShrinkage]);
            (*pTable)(rowIdx, colIdx++) << _T("");
            (*pTable)(rowIdx, colIdx++) << force.SetValue(tsDetails.DeckRebar[matType][barType].dPi[pgsTypes::pftRelaxation]);
            (*pTable)(rowIdx, colIdx++) << _T("");
         }
      }
   }

   DuctIndexType nTendons = tsDetails.SegmentTendons.size();
   for (DuctIndexType tendonIdx = 0; tendonIdx < nTendons; tendonIdx++)
   {
      rowIdx++;
      colIdx = 0;
      (*pTable)(rowIdx, colIdx++) << _T("Segment Tendon ") << LABEL_DUCT(tendonIdx);
      (*pTable)(rowIdx, colIdx++) << force.SetValue(tsDetails.SegmentTendons[tendonIdx].dPi[pgsTypes::pftCreep]);
      (*pTable)(rowIdx, colIdx++) << _T("");
      (*pTable)(rowIdx, colIdx++) << force.SetValue(tsDetails.SegmentTendons[tendonIdx].dPi[pgsTypes::pftShrinkage]);
      (*pTable)(rowIdx, colIdx++) << _T("");
      (*pTable)(rowIdx, colIdx++) << force.SetValue(tsDetails.SegmentTendons[tendonIdx].dPi[pgsTypes::pftRelaxation]);
      (*pTable)(rowIdx, colIdx++) << _T("");
   }

   nTendons = tsDetails.GirderTendons.size();
   for ( DuctIndexType tendonIdx = 0; tendonIdx < nTendons; tendonIdx++ )
   {
      rowIdx++;
      colIdx = 0;
      (*pTable)(rowIdx,colIdx++) << _T("Girder Tendon ") << LABEL_DUCT(tendonIdx);
      (*pTable)(rowIdx,colIdx++) << force.SetValue(tsDetails.GirderTendons[tendonIdx].dPi[pgsTypes::pftCreep]);
      (*pTable)(rowIdx,colIdx++) << _T("");
      (*pTable)(rowIdx,colIdx++) << force.SetValue(tsDetails.GirderTendons[tendonIdx].dPi[pgsTypes::pftShrinkage]);
      (*pTable)(rowIdx,colIdx++) << _T("");
      (*pTable)(rowIdx,colIdx++) << force.SetValue(tsDetails.GirderTendons[tendonIdx].dPi[pgsTypes::pftRelaxation]);
      (*pTable)(rowIdx,colIdx++) << _T("");
   }

   return pTable;
}

rptRcTable* CTimeStepDetailsChapterBuilder::BuildIncrementalForceTable(IBroker* pBroker, const std::vector<pgsTypes::ProductForceType>& vLoads, const TIME_STEP_DETAILS& tsDetails, bool bHasDeck, IEAFDisplayUnits* pDisplayUnits) const
{
   GET_IFACE2(pBroker, IProductLoads, pProductLoads);

   INIT_UV_PROTOTYPE(rptForceUnitValue, force, pDisplayUnits->GetGeneralForceUnit(), false);
   INIT_UV_PROTOTYPE(rptMomentUnitValue, moment, pDisplayUnits->GetSmallMomentUnit(), false);

   IndexType nLoads = vLoads.size();
   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(nLoads + 4);
   pTable->SetColumnStyle(0, rptStyleManager::GetTableCellStyle(CJ_LEFT));
   pTable->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle(CJ_LEFT));
   pTable->SetNumberOfHeaderRows(2);

   // label loading types across the top row
   RowIndexType rowIdx = 0;
   ColumnIndexType colIdx = 0;
   pTable->SetRowSpan(rowIdx, colIdx, 2);
   (*pTable)(rowIdx, colIdx++) << _T("Component");

   pTable->SetRowSpan(rowIdx, colIdx, 2);
   (*pTable)(rowIdx, colIdx++) << _T("");

   pTable->SetColumnSpan(rowIdx, colIdx, nLoads);
   (*pTable)(rowIdx, colIdx) << _T("Loading");
   colIdx += nLoads;

   colIdx = 2;
   for (IndexType i = 0; i < nLoads; i++)
   {
      pgsTypes::ProductForceType pfType = vLoads[i];
      (*pTable)(rowIdx + 1, colIdx++) << pProductLoads->GetProductLoadName(pfType);
   }

   pTable->SetRowSpan(rowIdx, colIdx, 2);
   (*pTable)(rowIdx, colIdx++) << _T("Incremental") << rptNewLine << _T("Total");

   pTable->SetRowSpan(rowIdx, colIdx, 2);
   (*pTable)(rowIdx, colIdx++) << _T("Cumulative") << rptNewLine << _T("Total");

   rowIdx++;

   // Label the rows in column 0
   rowIdx++;
   colIdx = 0;
   (*pTable)(rowIdx, colIdx++) << _T("Composite");
   (*pTable)(rowIdx, colIdx)   << DELTA_P << _T("(") << rptForceUnitTag(&pDisplayUnits->GetGeneralForceUnit().UnitOfMeasure) << _T(")") << rptNewLine;
   (*pTable)(rowIdx, colIdx)   << DELTA_M << _T("(") << rptMomentUnitTag(&pDisplayUnits->GetSmallMomentUnit().UnitOfMeasure) << _T(")") << rptNewLine;
   (*pTable)(rowIdx, colIdx++) << DELTA_V << _T("(") << rptForceUnitTag(&pDisplayUnits->GetGeneralForceUnit().UnitOfMeasure) << _T(")");

   rowIdx++;
   colIdx = 0;
   (*pTable)(rowIdx, colIdx++) << _T("Girder");
   (*pTable)(rowIdx, colIdx) << DELTA_Pk << _T("(") << rptForceUnitTag(&pDisplayUnits->GetGeneralForceUnit().UnitOfMeasure) << _T(")") << rptNewLine;
   (*pTable)(rowIdx, colIdx++) << DELTA_Mk << _T("(") << rptMomentUnitTag(&pDisplayUnits->GetSmallMomentUnit().UnitOfMeasure) << _T(")");

   IndexType nRebar = tsDetails.GirderRebar.size();
   for (IndexType i = 0; i < nRebar; i++)
   {
      const TIME_STEP_REBAR& tsRebar = tsDetails.GirderRebar[i];

      rowIdx++;
      colIdx = 0;
      (*pTable)(rowIdx, colIdx++) << _T("Girder Rebar ") << (i + 1);
      (*pTable)(rowIdx, colIdx++) << DELTA_Pk << _T("(") << rptForceUnitTag(&pDisplayUnits->GetGeneralForceUnit().UnitOfMeasure) << _T(")");
   }

   for (int i = 0; i < 3; i++)
   {
      rowIdx++;
      colIdx = 0;
      pgsTypes::StrandType strandType = (pgsTypes::StrandType)i;
      if (strandType == pgsTypes::Straight)
      {
         (*pTable)(rowIdx, colIdx++) << _T("Straight Strands");
      }
      else if (strandType == pgsTypes::Harped)
      {
         (*pTable)(rowIdx, colIdx++) << _T("Harped Strands");
      }
      else if (strandType == pgsTypes::Temporary)
      {
         (*pTable)(rowIdx, colIdx++) << _T("Temporary Strands");
      }

      (*pTable)(rowIdx, colIdx++) << DELTA_Pk << _T("(") << rptForceUnitTag(&pDisplayUnits->GetGeneralForceUnit().UnitOfMeasure) << _T(")");
   }

   if (bHasDeck)
   {
      rowIdx++;
      colIdx = 0;
      (*pTable)(rowIdx, colIdx++) << _T("Deck");
      (*pTable)(rowIdx, colIdx) << DELTA_Pk << _T("(") << rptForceUnitTag(&pDisplayUnits->GetGeneralForceUnit().UnitOfMeasure) << _T(")") << rptNewLine;
      (*pTable)(rowIdx, colIdx++) << DELTA_Mk << _T("(") << rptMomentUnitTag(&pDisplayUnits->GetSmallMomentUnit().UnitOfMeasure) << _T(")");

      for (int i = 0; i < 2; i++)
      {
         pgsTypes::DeckRebarMatType matType = (pgsTypes::DeckRebarMatType)i;
         for (int j = 0; j < 2; j++)
         {
            rowIdx++;
            colIdx = 0;
            pgsTypes::DeckRebarBarType barType = (pgsTypes::DeckRebarBarType)j;
            if (matType == pgsTypes::drmTop)
            {
               if (barType == pgsTypes::drbIndividual)
               {
                  (*pTable)(rowIdx, colIdx++) << _T("Deck Top Mat Individual Rebar");
               }
               else
               {
                  (*pTable)(rowIdx, colIdx++) << _T("Deck Top Mat Lump Sum Rebar");
               }
            }
            else
            {
               if (barType == pgsTypes::drbIndividual)
               {
                  (*pTable)(rowIdx, colIdx++) << _T("Deck Bottom Mat Individual Rebar");
               }
               else
               {
                  (*pTable)(rowIdx, colIdx++) << _T("Deck Bottom Mat Lump Sum Rebar");
               }
            }

            (*pTable)(rowIdx, colIdx++) << DELTA_Pk << _T("(") << rptForceUnitTag(&pDisplayUnits->GetGeneralForceUnit().UnitOfMeasure) << _T(")");
         }
      }
   }

   DuctIndexType nTendons = tsDetails.SegmentTendons.size();
   for (DuctIndexType tendonIdx = 0; tendonIdx < nTendons; tendonIdx++)
   {
      const TIME_STEP_STRAND& tsTendon = tsDetails.SegmentTendons[tendonIdx];

      rowIdx++;
      colIdx = 0;
      (*pTable)(rowIdx, colIdx++) << _T("Segment Tendon ") << LABEL_DUCT(tendonIdx);
      (*pTable)(rowIdx, colIdx++) << DELTA_Pk << _T("(") << rptForceUnitTag(&pDisplayUnits->GetGeneralForceUnit().UnitOfMeasure) << _T(")");
   }

   nTendons = tsDetails.GirderTendons.size();
   for (DuctIndexType tendonIdx = 0; tendonIdx < nTendons; tendonIdx++)
   {
      const TIME_STEP_STRAND& tsTendon = tsDetails.GirderTendons[tendonIdx];

      rowIdx++;
      colIdx = 0;
      (*pTable)(rowIdx, colIdx++) << _T("Girder Tendon ") << LABEL_DUCT(tendonIdx);
      (*pTable)(rowIdx, colIdx++) << DELTA_Pk << _T("(") << rptForceUnitTag(&pDisplayUnits->GetGeneralForceUnit().UnitOfMeasure) << _T(")");
   }

   // fill the table
   colIdx = 2;
   for (IndexType i = 0; i < nLoads; i++, colIdx++)
   {
      rowIdx = pTable->GetNumberOfHeaderRows();

      pgsTypes::ProductForceType pfType = vLoads[i];

      // Composite
      (*pTable)(rowIdx, colIdx) << force.SetValue(tsDetails.dPi[pfType]) << rptNewLine;
      (*pTable)(rowIdx, colIdx) << moment.SetValue(tsDetails.dMi[pfType]) << rptNewLine;
      (*pTable)(rowIdx, colIdx) << force.SetValue(tsDetails.dVi[pfType]);
      rowIdx++;

      // Girder
      (*pTable)(rowIdx, colIdx) << force.SetValue(tsDetails.Girder.dPi[pfType]) << rptNewLine;
      (*pTable)(rowIdx, colIdx) << moment.SetValue(tsDetails.Girder.dMi[pfType]);
      rowIdx++;

      // Girder Rebar
      for (const auto& tsRebar : tsDetails.GirderRebar)
      {
         (*pTable)(rowIdx++, colIdx) << force.SetValue(tsRebar.dPi[pfType]);
      }

      // Strands
      for (int i = 0; i < 3; i++)
      {
         pgsTypes::StrandType strandType = (pgsTypes::StrandType)i;
         (*pTable)(rowIdx++, colIdx) << force.SetValue(tsDetails.Strands[strandType].dPi[pfType]);
      }

      if (bHasDeck)
      {
         // Deck
         (*pTable)(rowIdx, colIdx) << force.SetValue(tsDetails.Deck.dPi[pfType]) << rptNewLine;
         (*pTable)(rowIdx, colIdx) << moment.SetValue(tsDetails.Deck.dMi[pfType]);
         rowIdx++;

         // Deck Rebar
         for (int i = 0; i < 2; i++)
         {
            pgsTypes::DeckRebarMatType matType = (pgsTypes::DeckRebarMatType)i;
            for (int j = 0; j < 2; j++)
            {
               pgsTypes::DeckRebarBarType barType = (pgsTypes::DeckRebarBarType)j;
               (*pTable)(rowIdx++, colIdx) << force.SetValue(tsDetails.DeckRebar[matType][barType].dPi[pfType]);
            }
         }
      }

      // Segment Tendons
      for (const auto& tsTendon : tsDetails.SegmentTendons)
      {
         (*pTable)(rowIdx++, colIdx) << force.SetValue(tsTendon.dPi[pfType]);
      }

      // Girder Tendons
      for (const auto& tsTendon : tsDetails.GirderTendons)
      {
         (*pTable)(rowIdx++, colIdx) << force.SetValue(tsTendon.dPi[pfType]);
      }
   } // next loading

   // Incremental Totals
   rowIdx = pTable->GetNumberOfHeaderRows();

   // Composite
   (*pTable)(rowIdx, colIdx) << force.SetValue(tsDetails.dP) << rptNewLine;
   (*pTable)(rowIdx, colIdx) << moment.SetValue(tsDetails.dM) << rptNewLine;
   (*pTable)(rowIdx, colIdx) << force.SetValue(tsDetails.dV);
   rowIdx++;

   // Girder
   (*pTable)(rowIdx, colIdx) << force.SetValue(tsDetails.Girder.dP) << rptNewLine;
   (*pTable)(rowIdx, colIdx) << moment.SetValue(tsDetails.Girder.dM);
   rowIdx++;

   // Girder Rebar
   for (const auto& tsRebar : tsDetails.GirderRebar)
   {
      (*pTable)(rowIdx++, colIdx) << force.SetValue(tsRebar.dP);
   }

   // Strands
   for (int i = 0; i < 3; i++)
   {
      pgsTypes::StrandType strandType = (pgsTypes::StrandType)i;
      (*pTable)(rowIdx++, colIdx) << force.SetValue(tsDetails.Strands[strandType].dP);
   }

   if (bHasDeck)
   {
      // Deck
      (*pTable)(rowIdx, colIdx) << force.SetValue(tsDetails.Deck.dP) << rptNewLine;
      (*pTable)(rowIdx, colIdx) << moment.SetValue(tsDetails.Deck.dM);
      rowIdx++;

      // Deck Rebar
      for (int i = 0; i < 2; i++)
      {
         pgsTypes::DeckRebarMatType matType = (pgsTypes::DeckRebarMatType)i;
         for (int j = 0; j < 2; j++)
         {
            pgsTypes::DeckRebarBarType barType = (pgsTypes::DeckRebarBarType)j;
            (*pTable)(rowIdx++, colIdx) << force.SetValue(tsDetails.DeckRebar[matType][barType].dP);
         }
      }
   }

   // Segment Tendons
   for (const auto& tsTendon : tsDetails.SegmentTendons)
   {
      (*pTable)(rowIdx++, colIdx) << force.SetValue(tsTendon.dP);
   }

   // Girder Tendons
   for (const auto& tsTendon : tsDetails.GirderTendons)
   {
      (*pTable)(rowIdx++, colIdx) << force.SetValue(tsTendon.dP);
   }


   // Cumulatve Totals
   colIdx++;
   rowIdx = pTable->GetNumberOfHeaderRows();

   // Composite
   (*pTable)(rowIdx, colIdx) << force.SetValue(tsDetails.P) << rptNewLine;
   (*pTable)(rowIdx, colIdx) << moment.SetValue(tsDetails.M)<< rptNewLine;
   (*pTable)(rowIdx, colIdx) << force.SetValue(tsDetails.V);
   rowIdx++;

   // Girder
   (*pTable)(rowIdx, colIdx) << force.SetValue(tsDetails.Girder.P) << rptNewLine;
   (*pTable)(rowIdx, colIdx) << moment.SetValue(tsDetails.Girder.M);
   rowIdx++;

   // Girder Rebar
   for (const auto& tsRebar : tsDetails.GirderRebar)
   {
      (*pTable)(rowIdx++, colIdx) << force.SetValue(tsRebar.P);
   }

   // Strands
   for (int i = 0; i < 3; i++)
   {
      pgsTypes::StrandType strandType = (pgsTypes::StrandType)i;
      (*pTable)(rowIdx++, colIdx) << force.SetValue(tsDetails.Strands[strandType].P);
   }

   if (bHasDeck)
   {
      // Deck
      (*pTable)(rowIdx, colIdx) << force.SetValue(tsDetails.Deck.P) << rptNewLine;
      (*pTable)(rowIdx, colIdx) << moment.SetValue(tsDetails.Deck.M);
      rowIdx++;

      // Deck Rebar
      for (int i = 0; i < 2; i++)
      {
         pgsTypes::DeckRebarMatType matType = (pgsTypes::DeckRebarMatType)i;
         for (int j = 0; j < 2; j++)
         {
            pgsTypes::DeckRebarBarType barType = (pgsTypes::DeckRebarBarType)j;
            (*pTable)(rowIdx++, colIdx) << force.SetValue(tsDetails.DeckRebar[matType][barType].P);
         }
      }
   }

   // Segment Tendons
   for (const auto& tsTendon : tsDetails.SegmentTendons)
   {
      (*pTable)(rowIdx++, colIdx) << force.SetValue(tsTendon.P);
   }

   // Girder Tendons
   for (const auto& tsTendon : tsDetails.GirderTendons)
   {
      (*pTable)(rowIdx++, colIdx) << force.SetValue(tsTendon.P);
   }

   return pTable;
}

rptRcTable* CTimeStepDetailsChapterBuilder::BuildIncrementalStressTable(IBroker* pBroker,const std::vector<pgsTypes::ProductForceType>& vLoads,const TIME_STEP_DETAILS& tsDetails, bool bHasDeck, IEAFDisplayUnits* pDisplayUnits) const
{
   GET_IFACE2(pBroker,IProductLoads,pProductLoads);

   INIT_UV_PROTOTYPE(rptStressUnitValue,     stress,      pDisplayUnits->GetStressUnit(),    false);

   IndexType nLoads = vLoads.size();
   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(nLoads+3+3);
   pTable->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CJ_LEFT));
   pTable->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CJ_LEFT));
   pTable->SetNumberOfHeaderRows(2);

   // label loading types across the top row
   RowIndexType rowIdx = 0;
   ColumnIndexType colIdx = 0;
   pTable->SetRowSpan(rowIdx,colIdx,2);
   (*pTable)(rowIdx,colIdx++) << _T("Component");

   pTable->SetColumnSpan(rowIdx,colIdx,nLoads);
   (*pTable)(rowIdx,colIdx) << _T("Loading");
   colIdx += nLoads;
   
   rowIdx++;
   colIdx = 1;
   for ( IndexType i = 0; i < nLoads; i++ )
   {
      pgsTypes::ProductForceType pfType = vLoads[i];
      (*pTable)(rowIdx,colIdx++) << COLHDR(pProductLoads->GetProductLoadName(pfType),rptStressUnitTag,pDisplayUnits->GetStressUnit());
   }

   rowIdx = 0;
   pTable->SetRowSpan(rowIdx,colIdx,2);
   (*pTable)(rowIdx,colIdx++) << COLHDR(_T("Incremental") << rptNewLine << _T("Total"),rptStressUnitTag,pDisplayUnits->GetStressUnit());

   pTable->SetRowSpan(rowIdx,colIdx,2);
   (*pTable)(rowIdx,colIdx++) << COLHDR(_T("Cumulative") << rptNewLine << _T("Total"),rptStressUnitTag,pDisplayUnits->GetStressUnit());

   pTable->SetColumnSpan(rowIdx, colIdx, 3);
   (*pTable)(rowIdx, colIdx) << _T("Cumulative Losses");
   rowIdx++;
   (*pTable)(rowIdx, colIdx++) << COLHDR(pProductLoads->GetProductLoadName(pgsTypes::pftCreep), rptStressUnitTag, pDisplayUnits->GetStressUnit());
   (*pTable)(rowIdx, colIdx++) << COLHDR(pProductLoads->GetProductLoadName(pgsTypes::pftShrinkage), rptStressUnitTag, pDisplayUnits->GetStressUnit());
   (*pTable)(rowIdx, colIdx++) << COLHDR(pProductLoads->GetProductLoadName(pgsTypes::pftRelaxation), rptStressUnitTag, pDisplayUnits->GetStressUnit());


   // Label the rows in column 0
   rowIdx = pTable->GetNumberOfHeaderRows();
   colIdx = 0;
   (*pTable)(rowIdx++,colIdx) << _T("Top Girder");
   (*pTable)(rowIdx++,colIdx) << _T("Bottom Girder");

   for ( int i = 0; i < 3; i++ )
   {
      pgsTypes::StrandType strandType = (pgsTypes::StrandType)i;
      if ( strandType == pgsTypes::Straight )
      {
         (*pTable)(rowIdx++,colIdx) << _T("Straight Strands");
      }
      else if ( strandType == pgsTypes::Harped )
      {
         (*pTable)(rowIdx++,colIdx) << _T("Harped Strands");
      }
      else if ( strandType == pgsTypes::Temporary )
      {
         (*pTable)(rowIdx++,colIdx) << _T("Temporary Strands");
      }
   }

   if (bHasDeck)
   {
      (*pTable)(rowIdx++, colIdx) << _T("Top Deck");
      (*pTable)(rowIdx++, colIdx) << _T("Bottom Deck");
   }

   DuctIndexType nTendons = tsDetails.SegmentTendons.size();
   for (DuctIndexType tendonIdx = 0; tendonIdx < nTendons; tendonIdx++)
   {
      const TIME_STEP_STRAND& tsTendon = tsDetails.SegmentTendons[tendonIdx];
      (*pTable)(rowIdx++, colIdx) << _T("Segment Tendon ") << LABEL_DUCT(tendonIdx);
   }

   nTendons = tsDetails.GirderTendons.size();
   for ( DuctIndexType tendonIdx = 0; tendonIdx < nTendons; tendonIdx++ )
   {
      const TIME_STEP_STRAND& tsTendon = tsDetails.GirderTendons[tendonIdx];
      (*pTable)(rowIdx++,colIdx) << _T("Girder Tendon ") << LABEL_DUCT(tendonIdx);
   }

   // fill the table
   std::array<Float64, 2> f_top_girder = {0,0};
   std::array<Float64, 2> f_bot_girder = {0,0};
   std::array<Float64, 2> f_top_deck   = {0,0};
   std::array<Float64, 2> f_bot_deck   = {0,0};
   colIdx = 1;
   for ( IndexType i = 0; i < nLoads; i++, colIdx++ )
   {
      rowIdx = pTable->GetNumberOfHeaderRows();

      pgsTypes::ProductForceType pfType = vLoads[i];

      // Girder
      f_top_girder[rtIncremental] += tsDetails.Girder.f[pgsTypes::TopFace   ][pfType][rtIncremental];
      f_bot_girder[rtIncremental] += tsDetails.Girder.f[pgsTypes::BottomFace][pfType][rtIncremental];
      f_top_girder[rtCumulative]  += tsDetails.Girder.f[pgsTypes::TopFace   ][pfType][rtCumulative];
      f_bot_girder[rtCumulative]  += tsDetails.Girder.f[pgsTypes::BottomFace][pfType][rtCumulative];
      (*pTable)(rowIdx++,colIdx) << stress.SetValue(tsDetails.Girder.f[pgsTypes::TopFace   ][pfType][rtIncremental]);
      (*pTable)(rowIdx++,colIdx) << stress.SetValue(tsDetails.Girder.f[pgsTypes::BottomFace][pfType][rtIncremental]);

      // Strands
      for ( int i = 0; i < 3; i++ )
      {
         pgsTypes::StrandType strandType = (pgsTypes::StrandType)i;
         (*pTable)(rowIdx++,colIdx) << stress.SetValue(tsDetails.Strands[strandType].dfpei[pfType]);
      }

      if (bHasDeck)
      {
         // Deck
         f_top_deck[rtIncremental] += tsDetails.Deck.f[pgsTypes::TopFace][pfType][rtIncremental];
         f_bot_deck[rtIncremental] += tsDetails.Deck.f[pgsTypes::BottomFace][pfType][rtIncremental];
         f_top_deck[rtCumulative] += tsDetails.Deck.f[pgsTypes::TopFace][pfType][rtCumulative];
         f_bot_deck[rtCumulative] += tsDetails.Deck.f[pgsTypes::BottomFace][pfType][rtCumulative];
         (*pTable)(rowIdx++, colIdx) << stress.SetValue(tsDetails.Deck.f[pgsTypes::TopFace][pfType][rtIncremental]);
         (*pTable)(rowIdx++, colIdx) << stress.SetValue(tsDetails.Deck.f[pgsTypes::BottomFace][pfType][rtIncremental]);
      }

      // Segment Tendons
      for (const auto& tsTendon : tsDetails.SegmentTendons)
      {
         (*pTable)(rowIdx++, colIdx) << stress.SetValue(tsTendon.dfpei[pfType]);
      }

      // Girder Tendons
      for (const auto& tsTendon : tsDetails.GirderTendons)
      {
         (*pTable)(rowIdx++, colIdx) << stress.SetValue(tsTendon.dfpei[pfType]);
      }
   } // next loading

   // Incremental Totals
   rowIdx = pTable->GetNumberOfHeaderRows();

   // Girder
   (*pTable)(rowIdx++,colIdx) << stress.SetValue(f_top_girder[rtIncremental]);
   (*pTable)(rowIdx++,colIdx) << stress.SetValue(f_bot_girder[rtIncremental]);

   // Strands
   for ( int i = 0; i < 3; i++ )
   {
      pgsTypes::StrandType strandType = (pgsTypes::StrandType)i;
      (*pTable)(rowIdx++,colIdx) << stress.SetValue(tsDetails.Strands[strandType].dfpe);
   }

   if (bHasDeck)
   {
      // Deck
      (*pTable)(rowIdx++, colIdx) << stress.SetValue(f_top_deck[rtIncremental]);
      (*pTable)(rowIdx++, colIdx) << stress.SetValue(f_bot_deck[rtIncremental]);
   }

   // Segment Tendons
   for (const auto& tsTendon : tsDetails.SegmentTendons)
   {
      (*pTable)(rowIdx++, colIdx) << stress.SetValue(tsTendon.dfpe);
   }

   // Girder Tendons
   for (const auto& tsTendon : tsDetails.GirderTendons)
   {
      (*pTable)(rowIdx++, colIdx) << stress.SetValue(tsTendon.dfpe);
   }

   // Cumulative Totals
   colIdx++;
   rowIdx = pTable->GetNumberOfHeaderRows();

   // Girder
   (*pTable)(rowIdx++,colIdx) << stress.SetValue(f_top_girder[rtCumulative]);
   (*pTable)(rowIdx++,colIdx) << stress.SetValue(f_bot_girder[rtCumulative]);

   // Strands
   for ( int i = 0; i < 3; i++ )
   {
      pgsTypes::StrandType strandType = (pgsTypes::StrandType)i;
      (*pTable)(rowIdx++,colIdx) << stress.SetValue(tsDetails.Strands[strandType].fpe);
   }

   if (bHasDeck)
   {
      // Deck
      (*pTable)(rowIdx++, colIdx) << stress.SetValue(f_top_deck[rtCumulative]);
      (*pTable)(rowIdx++, colIdx) << stress.SetValue(f_bot_deck[rtCumulative]);
   }

   // Segment Tendons
   for (const auto& tsTendon : tsDetails.SegmentTendons)
   {
      (*pTable)(rowIdx++, colIdx) << stress.SetValue(tsTendon.fpe);
   }

   // Girder Tendons
   for (const auto& tsTendon : tsDetails.GirderTendons)
   {
      (*pTable)(rowIdx++, colIdx) << stress.SetValue(tsTendon.fpe);
   }


   // Cumulative Time-Dependent Effects
   colIdx++;
   rowIdx = pTable->GetNumberOfHeaderRows();

   // Girder
   (*pTable)(rowIdx, colIdx) << _T(""); //stress.SetValue(f_top_girder[rtCumulative]);
   (*pTable)(rowIdx, colIdx+1) << _T(""); //stress.SetValue(f_top_girder[rtCumulative]);
   (*pTable)(rowIdx, colIdx+2) << _T(""); //stress.SetValue(f_top_girder[rtCumulative]);
   rowIdx++;

   (*pTable)(rowIdx, colIdx) << _T(""); //stress.SetValue(f_bot_girder[rtCumulative]);
   (*pTable)(rowIdx, colIdx+1) << _T(""); //stress.SetValue(f_bot_girder[rtCumulative]);
   (*pTable)(rowIdx, colIdx+2) << _T(""); //stress.SetValue(f_bot_girder[rtCumulative]);
   rowIdx++;

   // Strands
   for (int i = 0; i < 3; i++)
   {
      pgsTypes::StrandType strandType = (pgsTypes::StrandType)i;
      (*pTable)(rowIdx, colIdx) << stress.SetValue(tsDetails.Strands[strandType].fpei[pgsTypes::pftCreep]);
      (*pTable)(rowIdx, colIdx+1) << stress.SetValue(tsDetails.Strands[strandType].fpei[pgsTypes::pftShrinkage]);
      (*pTable)(rowIdx, colIdx+2) << stress.SetValue(tsDetails.Strands[strandType].fpei[pgsTypes::pftRelaxation]);
      rowIdx++;
   }

   if (bHasDeck)
   {
      // Deck
      (*pTable)(rowIdx, colIdx) << _T(""); //stress.SetValue(f_top_deck[rtCumulative]);
      (*pTable)(rowIdx, colIdx+1) << _T(""); //stress.SetValue(f_top_deck[rtCumulative]);
      (*pTable)(rowIdx, colIdx+2) << _T(""); //stress.SetValue(f_top_deck[rtCumulative]);
      rowIdx++;

      (*pTable)(rowIdx, colIdx) << _T(""); //stress.SetValue(f_bot_deck[rtCumulative]);
      (*pTable)(rowIdx, colIdx+1) << _T(""); //stress.SetValue(f_bot_deck[rtCumulative]);
      (*pTable)(rowIdx, colIdx+2) << _T(""); //stress.SetValue(f_bot_deck[rtCumulative]);
      rowIdx++;
   }

   // Segment Tendons
   for (const auto& tsTendon : tsDetails.SegmentTendons)
   {
      (*pTable)(rowIdx, colIdx) << stress.SetValue(tsTendon.fpei[pgsTypes::pftCreep]);
      (*pTable)(rowIdx, colIdx+1) << stress.SetValue(tsTendon.fpei[pgsTypes::pftShrinkage]);
      (*pTable)(rowIdx, colIdx+2) << stress.SetValue(tsTendon.fpei[pgsTypes::pftRelaxation]);
      rowIdx++;
   }

   // Girder Tendons
   for (const auto& tsTendon : tsDetails.GirderTendons)
   {
      (*pTable)(rowIdx, colIdx) << stress.SetValue(tsTendon.fpei[pgsTypes::pftCreep]);
      (*pTable)(rowIdx, colIdx+1) << stress.SetValue(tsTendon.fpei[pgsTypes::pftShrinkage]);
      (*pTable)(rowIdx, colIdx+2) << stress.SetValue(tsTendon.fpei[pgsTypes::pftRelaxation]);
      rowIdx++;
   }

   return pTable;
}

rptRcTable* CTimeStepDetailsChapterBuilder::BuildConcreteStressSummaryTable(IBroker* pBroker,const pgsPointOfInterest& poi,ResultsType resultsType,bool bGirder,IEAFDisplayUnits* pDisplayUnits) const
{
   INIT_UV_PROTOTYPE(rptStressUnitValue,     stress,      pDisplayUnits->GetStressUnit(),    false);

   GET_IFACE2(pBroker,IProductLoads,pProductLoads);
   std::vector<pgsTypes::ProductForceType> vLoads = pProductLoads->GetProductForcesForGirder(poi.GetSegmentKey());

   IndexType nLoads = vLoads.size();
   ColumnIndexType nColumns = nLoads + 2;
   if ( resultsType == rtCumulative )
   {
      nColumns++;
   }
   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(nColumns);
   pTable->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CJ_LEFT));
   pTable->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CJ_LEFT));
   pTable->SetNumberOfHeaderRows(2);

   // label loading types across the top row
   RowIndexType rowIdx = 0;
   ColumnIndexType colIdx = 0;
   pTable->SetRowSpan(rowIdx,colIdx,2);
   (*pTable)(rowIdx,colIdx++) << _T("Interval");

   pTable->SetRowSpan(rowIdx,colIdx,2);
   (*pTable)(rowIdx,colIdx++) << _T("Stress");

   pTable->SetColumnSpan(rowIdx,colIdx,nLoads);
   (*pTable)(rowIdx,colIdx) << _T("Loading");
   colIdx += nLoads;
   
   rowIdx++;
   colIdx = 2;
   for ( IndexType i = 0; i < nLoads; i++ )
   {
      pgsTypes::ProductForceType pfType = vLoads[i];
      (*pTable)(rowIdx,colIdx++) << COLHDR(pProductLoads->GetProductLoadName(pfType),rptStressUnitTag,pDisplayUnits->GetStressUnit());
   }

   if ( resultsType == rtCumulative )
   {
      rowIdx = 0;
      pTable->SetRowSpan(rowIdx,colIdx,2);
      (*pTable)(rowIdx,colIdx) << _T("Total");
   }

   rowIdx = pTable->GetNumberOfHeaderRows();

   GET_IFACE2(pBroker,ILosses,pLosses);
   GET_IFACE2(pBroker,IIntervals,pIntervals);
   GET_IFACE2(pBroker, IPointOfInterest, pPoi);
   IndexType deckCastingRegionIdx = pPoi->GetDeckCastingRegion(poi);
   ATLASSERT(deckCastingRegionIdx != INVALID_INDEX);
   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval(deckCastingRegionIdx);
   IntervalIndexType nIntervals = pIntervals->GetIntervalCount();
   IntervalIndexType firstIntervalIdx = (bGirder ? 0 : compositeDeckIntervalIdx);
   for ( IntervalIndexType intervalIdx = firstIntervalIdx; intervalIdx < nIntervals; intervalIdx++, rowIdx += 2 )
   {
      colIdx = 0;

      pTable->SetRowSpan(rowIdx,colIdx,2);
      (*pTable)(rowIdx,colIdx++) << LABEL_INTERVAL(intervalIdx);

      (*pTable)(rowIdx,colIdx) << _T("Top, ") << RPT_FTOP;
      (*pTable)(rowIdx+1,colIdx) << _T("Bottom, ") << RPT_FBOT;
      colIdx++;

      const LOSSDETAILS* pDetails = pLosses->GetLossDetails(poi,intervalIdx);
      const TIME_STEP_DETAILS& tsDetails(pDetails->TimeStepDetails[intervalIdx]);

      const TIME_STEP_CONCRETE* pConcrete = (bGirder ? &tsDetails.Girder : &tsDetails.Deck);

      Float64 fTop = 0;
      Float64 fBot = 0;

      for ( IndexType i = 0; i < nLoads; i++, colIdx++)
      {
         pgsTypes::ProductForceType pfType = vLoads[i];

         fTop += pConcrete->f[pgsTypes::TopFace   ][pfType][resultsType];
         fBot += pConcrete->f[pgsTypes::BottomFace][pfType][resultsType];

         (*pTable)(rowIdx,  colIdx) << stress.SetValue(pConcrete->f[pgsTypes::TopFace   ][pfType][resultsType]);
         (*pTable)(rowIdx+1,colIdx) << stress.SetValue(pConcrete->f[pgsTypes::BottomFace][pfType][resultsType]);
      } // next loading

      if ( resultsType == rtCumulative)
      {
         (*pTable)(rowIdx,  colIdx) << stress.SetValue(fTop);
         (*pTable)(rowIdx+1,colIdx) << stress.SetValue(fBot);
      }
   }

   if ( resultsType == rtIncremental )
   {
      const LOSSDETAILS* pDetails = pLosses->GetLossDetails(poi,nIntervals-1);
      const TIME_STEP_DETAILS& tsDetails(pDetails->TimeStepDetails[nIntervals-1]);
      const TIME_STEP_CONCRETE* pConcrete = (bGirder ? &tsDetails.Girder : &tsDetails.Deck);

      colIdx = 0;

      pTable->SetRowSpan(rowIdx,colIdx,2);
      (*pTable)(rowIdx,colIdx++) << _T("Total");

      (*pTable)(rowIdx,colIdx) << _T("Top, ") << RPT_FTOP;
      (*pTable)(rowIdx+1,colIdx) << _T("Bottom, ") << RPT_FBOT;
      colIdx++;

      for ( IndexType i = 0; i < nLoads; i++, colIdx++)
      {
         pgsTypes::ProductForceType pfType = vLoads[i];

         (*pTable)(rowIdx,  colIdx) << stress.SetValue(pConcrete->f[pgsTypes::TopFace   ][pfType][rtCumulative]);
         (*pTable)(rowIdx+1,colIdx) << stress.SetValue(pConcrete->f[pgsTypes::BottomFace][pfType][rtCumulative]);
      } // next loading
   }

   return pTable;
}

void CTimeStepDetailsChapterBuilder::ReportCreepDetails(rptChapter* pChapter,IBroker* pBroker,const pgsPointOfInterest& poi,IntervalIndexType firstIntervalIdx,IntervalIndexType lastIntervalIdx,IEAFDisplayUnits* pDisplayUnits) const
{
   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
   (*pChapter) << pPara;
   (*pPara) << _T("Creep coefficient details") << rptNewLine;
   pPara = new rptParagraph;
   (*pChapter) << pPara;


   GET_IFACE2(pBroker,ILossParameters,pLossParams);
   if ( pLossParams->IgnoreCreepEffects() )
   {
      (*pPara) << _T("Creep effects were ignored") << rptNewLine;
      return;
   }

   if (firstIntervalIdx == lastIntervalIdx)
   {
      (*pPara) << _T("There are no loads applied in this interval or the duration of this interval is zero.  No creep coefficients to report.") << rptNewLine;
      return;
   }

   GET_IFACE2(pBroker,ILosses,pLosses);
   GET_IFACE2(pBroker,IIntervals,pIntervals);
   GET_IFACE2(pBroker,IPointOfInterest,pPoi);
   GET_IFACE2(pBroker,IMaterials,pMaterials);
   GET_IFACE2(pBroker,ISectionProperties,pSectProps);
   GET_IFACE2(pBroker,IEnvironment,pEnv);
   GET_IFACE2(pBroker,IBridge,pBridge);

   INIT_UV_PROTOTYPE(rptLengthUnitValue, vs,     pDisplayUnits->GetComponentDimUnit(),    true);
   INIT_UV_PROTOTYPE(rptLengthUnitValue, ecc,    pDisplayUnits->GetComponentDimUnit(),    false);
   INIT_UV_PROTOTYPE(rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(),          false);

   std::_tstring strImagePath(rptStyleManager::GetImagePath());

   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   CClosureKey closureKey;
   bool bIsInClosure = pPoi->IsInClosureJoint(poi,&closureKey);

   IndexType deckCastingRegionIdx = pPoi->GetDeckCastingRegion(poi);
   ATLASSERT(deckCastingRegionIdx != INVALID_INDEX);

   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval(deckCastingRegionIdx);

   ColumnIndexType nColumns = 7; // This Interval (i), Previous Intervals (j), tj, tb, te, ..., Y(ib,jm), Y(ie,jm)

   PrestressLossCriteria::TimeDependentConcreteModelType model = pLossParams->GetTimeDependentModel();
   if ( model == PrestressLossCriteria::TimeDependentConcreteModelType::AASHTO )
   {
      if ( WBFL::LRFD::BDSManager::GetEdition() < WBFL::LRFD::BDSManager::Edition::ThirdEditionWith2005Interims )
      {
         nColumns += 4;
         (*pPara) << rptRcImage(strImagePath + _T("AASHTO_Creep_Before_2005.png")) << rptNewLine;
      }
      else if ( WBFL::LRFD::BDSManager::Edition::SeventhEditionWith2015Interims <= WBFL::LRFD::BDSManager::GetEdition() )
      {
         nColumns += 6;
         (*pPara) << rptRcImage(strImagePath + _T("AASHTO_Creep_2015.png")) << rptNewLine;
      }
      else
      {
         nColumns += 6;
         (*pPara) << rptRcImage(strImagePath + _T("AASHTO_Creep_2005.png")) << rptNewLine;
      }
   }
   else if ( model == PrestressLossCriteria::TimeDependentConcreteModelType::ACI209 )
   {
      nColumns += 6;
      (*pPara) << rptRcImage(strImagePath + _T("ACI209_Creep.png")) << rptNewLine;
   }
   else if ( model == PrestressLossCriteria::TimeDependentConcreteModelType::CEBFIP )
   {
      nColumns += 8;
      (*pPara) << rptRcImage(strImagePath + _T("CEBFIP1990_Creep.png")) << rptNewLine;
   }
   else
   {
      ATLASSERT(false);
   }

   (*pPara) << Sub2(_T("t"),_T("b")) << _T(" = age of concrete at beginning of interval") << rptNewLine;
   (*pPara) << Sub2(_T("t"),_T("e")) << _T(" = age of concrete at end of interval") << rptNewLine;
   if ( model == PrestressLossCriteria::TimeDependentConcreteModelType::AASHTO )
   {
      (*pPara) << Sub2(_T("t"),_T("i")) << _T(" = age of concrete at the middle of loading interval") << rptNewLine;
      (*pPara) << _T("H = ") << pEnv->GetRelHumidity() << _T(" %") << rptNewLine;
   }
   else if ( model == PrestressLossCriteria::TimeDependentConcreteModelType::ACI209 )
   {
      (*pPara) << Sub2(_T("t"),_T("la")) << _T(" = age of concrete at the middle of loading interval") << rptNewLine;
      (*pPara) << _T("RH = ") << pEnv->GetRelHumidity() << _T(" %") << rptNewLine;
   }
   else if ( model == PrestressLossCriteria::TimeDependentConcreteModelType::CEBFIP )
   {
      (*pPara) << Sub2(_T("t"),_T("o")) << _T(" = age of concrete at the middle of loading interval") << rptNewLine;
      (*pPara) << _T("RH = ") << pEnv->GetRelHumidity() << _T(" %") << rptNewLine;
   }
   else
   {
      ATLASSERT(false);
   }

   int nParts = (IsNonstructuralDeck(pBridge->GetDeckType()) ? 1 : 2);
   for ( int i = 0; i < nParts; i++ )
   {
      rptParagraph* pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
      (*pChapter) << pPara;
      if ( i == 0 )
      {
         (*pPara) << _T("Girder") << rptNewLine;
      }
      else
      {
         (*pPara) << _T("Deck") << rptNewLine;
      }
      pPara = new rptParagraph;
      (*pChapter) << pPara;

      Float64 V, S;
      if ( i == 0 )
      {
         if ( bIsInClosure )
         {
            pSectProps->GetClosureJointVolumeAndSurfaceArea(closureKey, &V, &S);
         }
         else
         {
            pSectProps->GetSegmentVolumeAndSurfaceArea(segmentKey, &V, &S);
         }
      }
      else
      {
         pSectProps->GetDeckVolumeAndSurfaceArea(&V, &S);
      }
      *pPara << _T("V/S = ") << vs.SetValue(V/S) << rptNewLine;
      
      if ( model == PrestressLossCriteria::TimeDependentConcreteModelType::ACI209 )
      {
         if ( i == 0 )
         {
            if ( bIsInClosure )
            {
               *pPara << _T("Curing: ") << WBFL::Materials::ConcreteBase::GetCuringType(pMaterials->GetClosureJointConcrete(closureKey)->GetCuringType()) << rptNewLine;
            }
            else
            {
               *pPara << _T("Curing: ") << WBFL::Materials::ConcreteBase::GetCuringType(pMaterials->GetSegmentConcrete(segmentKey)->GetCuringType()) << rptNewLine;
            }
         }
         else
         {
            *pPara << _T("Curing: ") << WBFL::Materials::ConcreteBase::GetCuringType(pMaterials->GetDeckConcrete(deckCastingRegionIdx)->GetCuringType()) << rptNewLine;
         }
      }

      {
         ColumnIndexType nCol = nColumns;
         if (model == PrestressLossCriteria::TimeDependentConcreteModelType::AASHTO)
            nCol -= 4;
         else if (model == PrestressLossCriteria::TimeDependentConcreteModelType::ACI209)
            nCol -= 5;
         else if (model == PrestressLossCriteria::TimeDependentConcreteModelType::CEBFIP)
            nCol -= 4;
         else
            ATLASSERT(false);

         *pPara << _T("Creep coefficients used to compute age adjusted modulus") << rptNewLine;
         rptRcTable* pTable = rptStyleManager::CreateDefaultTable(nCol);
         *pPara << pTable << rptNewLine;

         RowIndexType rowIdx = 0;
         ColumnIndexType colIdx = 0;
         (*pTable)(rowIdx, colIdx++) << _T("Interval");
         (*pTable)(rowIdx, colIdx++) << Sub2(_T("t"), _T("e")) << rptNewLine << _T("(day)");

         if (model == PrestressLossCriteria::TimeDependentConcreteModelType::AASHTO)
         {
            (*pTable)(rowIdx, colIdx++) << Sub2(_T("t"), _T("i")) << rptNewLine << _T("(day)");
            if (WBFL::LRFD::BDSManager::GetEdition() < WBFL::LRFD::BDSManager::Edition::ThirdEditionWith2005Interims)
            {
               (*pTable)(rowIdx, colIdx++) << COLHDR(RPT_FC, rptStressUnitTag, pDisplayUnits->GetStressUnit());
               (*pTable)(rowIdx, colIdx++) << Sub2(_T("k"), _T("c")) << _T("(") << Sub2(_T("t"), _T("b")) << _T(")");
               (*pTable)(rowIdx, colIdx++) << Sub2(_T("k"), _T("c")) << _T("(") << Sub2(_T("t"), _T("e")) << _T(")");
               (*pTable)(rowIdx, colIdx++) << Sub2(_T("k"), _T("f"));
            }
            else if (WBFL::LRFD::BDSManager::Edition::SeventhEditionWith2015Interims <= WBFL::LRFD::BDSManager::GetEdition())
            {
               (*pTable)(rowIdx, colIdx++) << COLHDR(RPT_FCI, rptStressUnitTag, pDisplayUnits->GetStressUnit());
               (*pTable)(rowIdx, colIdx++) << Sub2(_T("k"), _T("vs"));
               (*pTable)(rowIdx, colIdx++) << Sub2(_T("k"), _T("hc"));
               (*pTable)(rowIdx, colIdx++) << Sub2(_T("k"), _T("f"));
               (*pTable)(rowIdx, colIdx++) << Sub2(_T("k"), _T("td")) << _T("(") << Sub2(_T("t"), _T("e")) << _T(" - ") << Sub2(_T("t"), _T("i")) << _T(")");
            }
            else
            {
               (*pTable)(rowIdx, colIdx++) << COLHDR(RPT_FCI, rptStressUnitTag, pDisplayUnits->GetStressUnit());
               (*pTable)(rowIdx, colIdx++) << Sub2(_T("k"), _T("vs"));
               (*pTable)(rowIdx, colIdx++) << Sub2(_T("k"), _T("hc"));
               (*pTable)(rowIdx, colIdx++) << Sub2(_T("k"), _T("f"));
               (*pTable)(rowIdx, colIdx++) << Sub2(_T("k"), _T("td")) << _T("(") << Sub2(_T("t"), _T("e")) << _T(" - ") << Sub2(_T("t"), _T("i")) << _T(")");
            }
         }
         else if (model == PrestressLossCriteria::TimeDependentConcreteModelType::ACI209)
         {
            (*pTable)(rowIdx, colIdx++) << Sub2(_T("t"), _T("la")) << rptNewLine << _T("(day)");
            (*pTable)(rowIdx, colIdx++) << Sub2(symbol(gamma), _T("t")) << _T("(") << Sub2(_T("t"), _T("m")) << _T(")");
            (*pTable)(rowIdx, colIdx++) << Sub2(symbol(gamma), _T("la")) << _T("(") << Sub2(_T("t"), _T("m")) << _T(")");
            (*pTable)(rowIdx, colIdx++) << Sub2(symbol(gamma), symbol(lambda));
            (*pTable)(rowIdx, colIdx++) << Sub2(symbol(gamma), _T("vs"));
         }
         else if (model == PrestressLossCriteria::TimeDependentConcreteModelType::CEBFIP)
         {
            (*pTable)(rowIdx, colIdx++) << Sub2(_T("t"), _T("o")) << rptNewLine << _T("(day)");
            (*pTable)(rowIdx, colIdx++) << COLHDR(_T("h"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
            (*pTable)(rowIdx, colIdx++) << Sub2(symbol(beta), _T("H"));
            (*pTable)(rowIdx, colIdx++) << Sub2(symbol(beta), _T("c")) << _T("(") << Sub2(_T("t"), _T("m")) << _T(")");
            (*pTable)(rowIdx, colIdx++) << symbol(beta) << _T("(") << Sub2(_T("t"), _T("o")) << _T(")");
            (*pTable)(rowIdx, colIdx++) << symbol(beta) << _T("((") << RPT_FC << Sub2(_T(")"), _T("28")) << _T(")");
            (*pTable)(rowIdx, colIdx++) << Sub2(symbol(psi), _T("RH"));
            (*pTable)(rowIdx, colIdx++) << Sub2(symbol(psi), _T("o"));
         }
         else
         {
            ATLASSERT(false);
         }

         if (model == PrestressLossCriteria::TimeDependentConcreteModelType::AASHTO)
         {
            if (WBFL::LRFD::BDSManager::GetEdition() < WBFL::LRFD::BDSManager::Edition::ThirdEditionWith2005Interims)
            {
               (*pTable)(rowIdx, colIdx++) << CREEP_te_ti;
            }
            else
            {
               (*pTable)(rowIdx, colIdx++) << CREEP_te_ti_ti;
            }
         }
         else if (model == PrestressLossCriteria::TimeDependentConcreteModelType::ACI209)
         {
            (*pTable)(rowIdx, colIdx++) << CREEP_te_tla;
         }
         else if (model == PrestressLossCriteria::TimeDependentConcreteModelType::CEBFIP)
         {
            (*pTable)(rowIdx, colIdx++) << CREEP_te_to;
         }
         else
         {
            ATLASSERT(false);
         }


         rowIdx = pTable->GetNumberOfHeaderRows();

         if (i == 0 && firstIntervalIdx < releaseIntervalIdx)
         {
            // we are reporting for the segment, make the first interval
            // when the segment first gets loaded (ie., at release)
            firstIntervalIdx = releaseIntervalIdx;
         }
         else if (i == 1 && firstIntervalIdx < compositeDeckIntervalIdx)
         {
            // we reporting for the deck, make the first interval
            // when the deck becomes composite (values are just 0 before this)
            firstIntervalIdx = compositeDeckIntervalIdx;
         }


         for (IntervalIndexType intervalIdx = firstIntervalIdx; intervalIdx <= lastIntervalIdx; intervalIdx++)
         {
            if (firstIntervalIdx != lastIntervalIdx && (intervalIdx < releaseIntervalIdx || ::IsZero(pIntervals->GetDuration(intervalIdx))))
            {
               continue;
            }

            std::shared_ptr<WBFL::Materials::ConcreteBaseCreepDetails> pCreep;
            if (i == 0)
            {
               if (bIsInClosure)
               {
                  pCreep = pMaterials->GetClosureJointCreepCoefficientDetails(closureKey, intervalIdx, pgsTypes::Middle, intervalIdx, pgsTypes::End);
               }
               else
               {
                  pCreep = pMaterials->GetSegmentCreepCoefficientDetails(segmentKey, intervalIdx, pgsTypes::Middle, intervalIdx, pgsTypes::End);
               }
            }
            else
            {
               pCreep = pMaterials->GetDeckCreepCoefficientDetails(deckCastingRegionIdx, intervalIdx, pgsTypes::Middle, intervalIdx, pgsTypes::End);
            }

            colIdx = 0;

            Float64 endAge;
            if (i == 0)
            {
               if (bIsInClosure)
               {
                  endAge = pMaterials->GetClosureJointConcreteAge(closureKey, intervalIdx, pgsTypes::End);
               }
               else
               {
                  endAge = pMaterials->GetSegmentConcreteAge(segmentKey, intervalIdx, pgsTypes::End);
               }
            }
            else
            {
               endAge = pMaterials->GetDeckConcreteAge(deckCastingRegionIdx, intervalIdx, pgsTypes::End);
            }

            colIdx = 0;
            (*pTable)(rowIdx, colIdx++) << LABEL_INTERVAL(intervalIdx);
            (*pTable)(rowIdx, colIdx++) << endAge;

            ATLASSERT(::IsEqual(endAge, pCreep->age));

            (*pTable)(rowIdx, colIdx++) << pCreep->age_at_loading;

            if (model == PrestressLossCriteria::TimeDependentConcreteModelType::AASHTO)
            {
               auto pDetails = std::dynamic_pointer_cast<const WBFL::LRFD::LRFDTimeDependentConcreteCreepDetails>(pCreep);
               if (WBFL::LRFD::BDSManager::GetEdition() < WBFL::LRFD::BDSManager::Edition::ThirdEditionWith2005Interims)
               {
                  (*pTable)(rowIdx, colIdx++) << stress.SetValue(pDetails->fci);
                  (*pTable)(rowIdx, colIdx++) << pDetails->kc;
                  (*pTable)(rowIdx, colIdx++) << pDetails->kf;
               }
               else if (WBFL::LRFD::BDSManager::Edition::SeventhEditionWith2015Interims <= WBFL::LRFD::BDSManager::GetEdition())
               {
                  (*pTable)(rowIdx, colIdx++) << stress.SetValue(pDetails->fci);
                  (*pTable)(rowIdx, colIdx++) << pDetails->kvs;
                  (*pTable)(rowIdx, colIdx++) << pDetails->khc;
                  (*pTable)(rowIdx, colIdx++) << pDetails->kf;
                  (*pTable)(rowIdx, colIdx++) << pDetails->ktd;
               }
               else
               {
                  (*pTable)(rowIdx, colIdx++) << stress.SetValue(pDetails->fci);
                  (*pTable)(rowIdx, colIdx++) << pDetails->kvs;
                  (*pTable)(rowIdx, colIdx++) << pDetails->khc;
                  (*pTable)(rowIdx, colIdx++) << pDetails->kf;
                  (*pTable)(rowIdx, colIdx++) << pDetails->ktd;
               }
            }
            else if (model == PrestressLossCriteria::TimeDependentConcreteModelType::ACI209)
            {
               auto pDetails = std::dynamic_pointer_cast<const WBFL::Materials::ACI209ConcreteCreepDetails>(pCreep);
               (*pTable)(rowIdx, colIdx++) << pDetails->time_factor;
               (*pTable)(rowIdx, colIdx++) << pDetails->loading_age_factor;
               (*pTable)(rowIdx, colIdx++) << pDetails->humidity_factor;
               (*pTable)(rowIdx, colIdx++) << pDetails->vs_factor;
            }
            else if (model == PrestressLossCriteria::TimeDependentConcreteModelType::CEBFIP)
            {
               auto pDetails = std::dynamic_pointer_cast<const WBFL::Materials::CEBFIPConcreteCreepDetails>(pCreep);
               (*pTable)(rowIdx, colIdx++) << ecc.SetValue(pDetails->h);
               (*pTable)(rowIdx, colIdx++) << pDetails->Bh;
               (*pTable)(rowIdx, colIdx++) << pDetails->Bc;
               (*pTable)(rowIdx, colIdx++) << pDetails->Bt;
               (*pTable)(rowIdx, colIdx++) << pDetails->Bfc;
               (*pTable)(rowIdx, colIdx++) << pDetails->Yrh;
               (*pTable)(rowIdx, colIdx++) << pDetails->Yo;
            }
            else
            {
               ATLASSERT(false);
            }

            (*pTable)(rowIdx, colIdx++) << pCreep->Ct;

            rowIdx++;
         } // next interval
      } // end of scope

      {
         *pPara << _T("Creep coefficients used to compute unrestrained creep deformations") << rptNewLine;
      
         rptRcTable* pTable = rptStyleManager::CreateDefaultTable(nColumns);
         *pPara << pTable << rptNewLine;

         RowIndexType rowIdx = 0;
         ColumnIndexType colIdx = 0;
         (*pTable)(rowIdx, colIdx++) << _T("Interval");
         (*pTable)(rowIdx, colIdx++) << Sub2(_T("t"), _T("b")) << rptNewLine << _T("(day)");
         (*pTable)(rowIdx, colIdx++) << Sub2(_T("t"), _T("e")) << rptNewLine << _T("(day)");
         (*pTable)(rowIdx, colIdx++) << _T("Loading") << rptNewLine << _T("Interval");

         if (model == PrestressLossCriteria::TimeDependentConcreteModelType::AASHTO)
         {
            (*pTable)(rowIdx, colIdx++) << Sub2(_T("t"), _T("i")) << rptNewLine << _T("(day)");
            if (WBFL::LRFD::BDSManager::GetEdition() < WBFL::LRFD::BDSManager::Edition::ThirdEditionWith2005Interims)
            {
               (*pTable)(rowIdx, colIdx++) << COLHDR(RPT_FC, rptStressUnitTag, pDisplayUnits->GetStressUnit());
               (*pTable)(rowIdx, colIdx++) << Sub2(_T("k"), _T("c")) << _T("(") << Sub2(_T("t"), _T("b")) << _T(")");
               (*pTable)(rowIdx, colIdx++) << Sub2(_T("k"), _T("c")) << _T("(") << Sub2(_T("t"), _T("e")) << _T(")");
               (*pTable)(rowIdx, colIdx++) << Sub2(_T("k"), _T("f"));
            }
            else if (WBFL::LRFD::BDSManager::Edition::SeventhEditionWith2015Interims <= WBFL::LRFD::BDSManager::GetEdition())
            {
               (*pTable)(rowIdx, colIdx++) << COLHDR(RPT_FCI, rptStressUnitTag, pDisplayUnits->GetStressUnit());
               (*pTable)(rowIdx, colIdx++) << Sub2(_T("k"), _T("vs"));
               (*pTable)(rowIdx, colIdx++) << Sub2(_T("k"), _T("hc"));
               (*pTable)(rowIdx, colIdx++) << Sub2(_T("k"), _T("f"));
               (*pTable)(rowIdx, colIdx++) << Sub2(_T("k"), _T("td")) << _T("(") << Sub2(_T("t"), _T("b")) << _T(" - ") << Sub2(_T("t"), _T("i")) << _T(")");
               (*pTable)(rowIdx, colIdx++) << Sub2(_T("k"), _T("td")) << _T("(") << Sub2(_T("t"), _T("e")) << _T(" - ") << Sub2(_T("t"), _T("i")) << _T(")");
            }
            else
            {
               (*pTable)(rowIdx, colIdx++) << COLHDR(RPT_FCI, rptStressUnitTag, pDisplayUnits->GetStressUnit());
               (*pTable)(rowIdx, colIdx++) << Sub2(_T("k"), _T("vs"));
               (*pTable)(rowIdx, colIdx++) << Sub2(_T("k"), _T("hc"));
               (*pTable)(rowIdx, colIdx++) << Sub2(_T("k"), _T("f"));
               (*pTable)(rowIdx, colIdx++) << Sub2(_T("k"), _T("td")) << _T("(") << Sub2(_T("t"), _T("b")) << _T(" - ") << Sub2(_T("t"), _T("i")) << _T(")");
               (*pTable)(rowIdx, colIdx++) << Sub2(_T("k"), _T("td")) << _T("(") << Sub2(_T("t"), _T("e")) << _T(" - ") << Sub2(_T("t"), _T("i")) << _T(")");
            }
         }
         else if (model == PrestressLossCriteria::TimeDependentConcreteModelType::ACI209)
         {
            (*pTable)(rowIdx, colIdx++) << Sub2(_T("t"), _T("la")) << rptNewLine << _T("(day)");
            (*pTable)(rowIdx, colIdx++) << Sub2(symbol(gamma), _T("t")) << _T("(") << Sub2(_T("t"), _T("b")) << _T(")");
            (*pTable)(rowIdx, colIdx++) << Sub2(symbol(gamma), _T("t")) << _T("(") << Sub2(_T("t"), _T("e")) << _T(")");
            (*pTable)(rowIdx, colIdx++) << Sub2(symbol(gamma), _T("la")) << _T("(") << Sub2(_T("t"), _T("b")) << _T(")");
            (*pTable)(rowIdx, colIdx++) << Sub2(symbol(gamma), _T("la")) << _T("(") << Sub2(_T("t"), _T("e")) << _T(")");
            (*pTable)(rowIdx, colIdx++) << Sub2(symbol(gamma), symbol(lambda));
            (*pTable)(rowIdx, colIdx++) << Sub2(symbol(gamma), _T("vs"));
         }
         else if (model == PrestressLossCriteria::TimeDependentConcreteModelType::CEBFIP)
         {
            (*pTable)(rowIdx, colIdx++) << Sub2(_T("t"), _T("o")) << rptNewLine << _T("(day)");
            (*pTable)(rowIdx, colIdx++) << COLHDR(_T("h"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
            (*pTable)(rowIdx, colIdx++) << Sub2(symbol(beta), _T("H"));
            (*pTable)(rowIdx, colIdx++) << Sub2(symbol(beta), _T("c")) << _T("(") << Sub2(_T("t"), _T("b")) << _T(")");
            (*pTable)(rowIdx, colIdx++) << Sub2(symbol(beta), _T("c")) << _T("(") << Sub2(_T("t"), _T("e")) << _T(")");
            (*pTable)(rowIdx, colIdx++) << symbol(beta) << _T("(") << Sub2(_T("t"), _T("o")) << _T(")");
            (*pTable)(rowIdx, colIdx++) << symbol(beta) << _T("((") << RPT_FC << Sub2(_T(")"), _T("28")) << _T(")");
            (*pTable)(rowIdx, colIdx++) << Sub2(symbol(psi), _T("RH"));
            (*pTable)(rowIdx, colIdx++) << Sub2(symbol(psi), _T("o"));
         }
         else
         {
            ATLASSERT(false);
         }

         if (model == PrestressLossCriteria::TimeDependentConcreteModelType::AASHTO)
         {
            if (WBFL::LRFD::BDSManager::GetEdition() < WBFL::LRFD::BDSManager::Edition::ThirdEditionWith2005Interims)
            {
               (*pTable)(rowIdx, colIdx++) << CREEP_tb_ti;
               (*pTable)(rowIdx, colIdx++) << CREEP_te_ti;
            }
            else
            {
               (*pTable)(rowIdx, colIdx++) << CREEP_tb_ti_ti;
               (*pTable)(rowIdx, colIdx++) << CREEP_te_ti_ti;
            }
         }
         else if (model == PrestressLossCriteria::TimeDependentConcreteModelType::ACI209)
         {
            (*pTable)(rowIdx, colIdx++) << CREEP_tb_tla;
            (*pTable)(rowIdx, colIdx++) << CREEP_te_tla;
         }
         else if (model == PrestressLossCriteria::TimeDependentConcreteModelType::CEBFIP)
         {
            (*pTable)(rowIdx, colIdx++) << CREEP_tb_to;
            (*pTable)(rowIdx, colIdx++) << CREEP_te_to;
         }
         else
         {
            ATLASSERT(false);
         }


         rowIdx = pTable->GetNumberOfHeaderRows();

         if (i == 0 && firstIntervalIdx < releaseIntervalIdx)
         {
            // we are reporting for the segment, make the first interval
            // when the segment first gets loaded (ie., at release)
            firstIntervalIdx = releaseIntervalIdx;
         }
         else if (i == 1 && firstIntervalIdx < compositeDeckIntervalIdx)
         {
            // we reporting for the deck, make the first interval
            // when the deck becomes composite (values are just 0 before this)
            firstIntervalIdx = compositeDeckIntervalIdx;
         }


         for (IntervalIndexType intervalIdx = firstIntervalIdx; intervalIdx <= lastIntervalIdx; intervalIdx++)
         {
            if (firstIntervalIdx != lastIntervalIdx && (intervalIdx < releaseIntervalIdx || ::IsZero(pIntervals->GetDuration(intervalIdx))))
            {
               continue;
            }

            const LOSSDETAILS* pDetails = pLosses->GetLossDetails(poi, intervalIdx);
            const TIME_STEP_DETAILS& tsDetails(pDetails->TimeStepDetails[intervalIdx]);
            const TIME_STEP_CONCRETE* pConcrete;
            if (i == 0)
            {
               pConcrete = &tsDetails.Girder;
            }
            else
            {
               pConcrete = &tsDetails.Deck;
            }

            colIdx = 0;
            pTable->SetRowSpan(rowIdx, colIdx++, intervalIdx - firstIntervalIdx); // interval
            pTable->SetRowSpan(rowIdx, colIdx++, intervalIdx - firstIntervalIdx); // t_ib
            pTable->SetRowSpan(rowIdx, colIdx++, intervalIdx - firstIntervalIdx); // t_ie

            Float64 startAge, endAge;
            if (i == 0)
            {
               if (bIsInClosure)
               {
                  startAge = pMaterials->GetClosureJointConcreteAge(closureKey, intervalIdx, pgsTypes::Start);
                  endAge = pMaterials->GetClosureJointConcreteAge(closureKey, intervalIdx, pgsTypes::End);
               }
               else
               {
                  startAge = pMaterials->GetSegmentConcreteAge(segmentKey, intervalIdx, pgsTypes::Start);
                  endAge = pMaterials->GetSegmentConcreteAge(segmentKey, intervalIdx, pgsTypes::End);
               }
            }
            else
            {
               startAge = pMaterials->GetDeckConcreteAge(deckCastingRegionIdx, intervalIdx, pgsTypes::Start);
               endAge = pMaterials->GetDeckConcreteAge(deckCastingRegionIdx, intervalIdx, pgsTypes::End);
            }

            for (IntervalIndexType prevIntervalIdx = firstIntervalIdx; prevIntervalIdx < intervalIdx; prevIntervalIdx++)
            {
               colIdx = 0;

               if (prevIntervalIdx == firstIntervalIdx)
               {
                  (*pTable)(rowIdx, colIdx++) << LABEL_INTERVAL(intervalIdx);
                  (*pTable)(rowIdx, colIdx++) << startAge;
                  (*pTable)(rowIdx, colIdx++) << endAge;
               }
               else
               {
                  colIdx += 3;
               }

               ATLASSERT(::IsEqual(startAge, pConcrete->Creep[prevIntervalIdx].pStartDetails->age));
               ATLASSERT(::IsEqual(endAge, pConcrete->Creep[prevIntervalIdx].pEndDetails->age));

               (*pTable)(rowIdx, colIdx++) << LABEL_INTERVAL(prevIntervalIdx);
               (*pTable)(rowIdx, colIdx++) << pConcrete->Creep[prevIntervalIdx].pStartDetails->age_at_loading;

               if (model == PrestressLossCriteria::TimeDependentConcreteModelType::AASHTO)
               {
                  const std::shared_ptr<WBFL::LRFD::LRFDTimeDependentConcreteCreepDetails> pStartDetails = std::dynamic_pointer_cast<WBFL::LRFD::LRFDTimeDependentConcreteCreepDetails>(pConcrete->Creep[prevIntervalIdx].pStartDetails);
                  const std::shared_ptr<WBFL::LRFD::LRFDTimeDependentConcreteCreepDetails> pEndDetails   = std::dynamic_pointer_cast<WBFL::LRFD::LRFDTimeDependentConcreteCreepDetails>(pConcrete->Creep[prevIntervalIdx].pEndDetails);
                  if (WBFL::LRFD::BDSManager::GetEdition() < WBFL::LRFD::BDSManager::Edition::ThirdEditionWith2005Interims)
                  {
                     (*pTable)(rowIdx, colIdx++) << stress.SetValue(pStartDetails->fci);
                     (*pTable)(rowIdx, colIdx++) << pStartDetails->kc;
                     (*pTable)(rowIdx, colIdx++) << pEndDetails->kc;
                     (*pTable)(rowIdx, colIdx++) << pStartDetails->kf;
                  }
                  else if (WBFL::LRFD::BDSManager::Edition::SeventhEditionWith2015Interims <= WBFL::LRFD::BDSManager::GetEdition())
                  {
                     (*pTable)(rowIdx, colIdx++) << stress.SetValue(pStartDetails->fci);
                     (*pTable)(rowIdx, colIdx++) << pStartDetails->kvs;
                     (*pTable)(rowIdx, colIdx++) << pStartDetails->khc;
                     (*pTable)(rowIdx, colIdx++) << pStartDetails->kf;
                     (*pTable)(rowIdx, colIdx++) << pStartDetails->ktd;
                     (*pTable)(rowIdx, colIdx++) << pEndDetails->ktd;
                  }
                  else
                  {
                     (*pTable)(rowIdx, colIdx++) << stress.SetValue(pStartDetails->fci);
                     (*pTable)(rowIdx, colIdx++) << pStartDetails->kvs;
                     (*pTable)(rowIdx, colIdx++) << pStartDetails->khc;
                     (*pTable)(rowIdx, colIdx++) << pStartDetails->kf;
                     (*pTable)(rowIdx, colIdx++) << pStartDetails->ktd;
                     (*pTable)(rowIdx, colIdx++) << pEndDetails->ktd;
                  }
               }
               else if (model == PrestressLossCriteria::TimeDependentConcreteModelType::ACI209)
               {
                  const std::shared_ptr<WBFL::Materials::ACI209ConcreteCreepDetails> pStartDetails = std::dynamic_pointer_cast<WBFL::Materials::ACI209ConcreteCreepDetails>(pConcrete->Creep[prevIntervalIdx].pStartDetails);
                  const std::shared_ptr<WBFL::Materials::ACI209ConcreteCreepDetails> pEndDetails   = std::dynamic_pointer_cast<WBFL::Materials::ACI209ConcreteCreepDetails>(pConcrete->Creep[prevIntervalIdx].pEndDetails);
                  (*pTable)(rowIdx, colIdx++) << pStartDetails->time_factor;
                  (*pTable)(rowIdx, colIdx++) << pEndDetails->time_factor;
                  (*pTable)(rowIdx, colIdx++) << pStartDetails->loading_age_factor;
                  (*pTable)(rowIdx, colIdx++) << pEndDetails->loading_age_factor;
                  (*pTable)(rowIdx, colIdx++) << pStartDetails->humidity_factor;
                  (*pTable)(rowIdx, colIdx++) << pStartDetails->vs_factor;
               }
               else if (model == PrestressLossCriteria::TimeDependentConcreteModelType::CEBFIP)
               {
                  const std::shared_ptr<WBFL::Materials::CEBFIPConcreteCreepDetails> pStartDetails = std::dynamic_pointer_cast<WBFL::Materials::CEBFIPConcreteCreepDetails>(pConcrete->Creep[prevIntervalIdx].pStartDetails);
                  const std::shared_ptr<WBFL::Materials::CEBFIPConcreteCreepDetails> pEndDetails   = std::dynamic_pointer_cast<WBFL::Materials::CEBFIPConcreteCreepDetails>(pConcrete->Creep[prevIntervalIdx].pEndDetails);
                  (*pTable)(rowIdx, colIdx++) << ecc.SetValue(pStartDetails->h);
                  (*pTable)(rowIdx, colIdx++) << pStartDetails->Bh;
                  (*pTable)(rowIdx, colIdx++) << pStartDetails->Bc;
                  (*pTable)(rowIdx, colIdx++) << pEndDetails->Bc;
                  (*pTable)(rowIdx, colIdx++) << pStartDetails->Bt;
                  (*pTable)(rowIdx, colIdx++) << pStartDetails->Bfc;
                  (*pTable)(rowIdx, colIdx++) << pStartDetails->Yrh;
                  (*pTable)(rowIdx, colIdx++) << pStartDetails->Yo;
               }
               else
               {
                  ATLASSERT(false);
               }

               (*pTable)(rowIdx, colIdx++) << pConcrete->ec[prevIntervalIdx].Cs;
               (*pTable)(rowIdx, colIdx++) << pConcrete->ec[prevIntervalIdx].Ce;

               rowIdx++;
            } // next previous interval
         } // next interval
      } // end of scope
   } // next element type (girder,deck)
}

void CTimeStepDetailsChapterBuilder::ReportShrinkageDetails(rptChapter* pChapter,IBroker* pBroker,const pgsPointOfInterest& poi,IntervalIndexType firstIntervalIdx,IntervalIndexType lastIntervalIdx,IEAFDisplayUnits* pDisplayUnits) const
{
   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
   (*pChapter) << pPara;
   (*pPara) << _T("Concrete shrinkage details") << rptNewLine;
   pPara = new rptParagraph;
   (*pChapter) << pPara;

   GET_IFACE2(pBroker,ILossParameters,pLossParams);
   if ( pLossParams->IgnoreShrinkageEffects() )
   {
      (*pPara) << _T("Shrinkage effects were ignored") << rptNewLine;
      return;
   }

   GET_IFACE2_NOCHECK(pBroker,ILosses,pLosses);
   GET_IFACE2(pBroker,IIntervals,pIntervals);
   GET_IFACE2(pBroker,IPointOfInterest,pPoi);
   GET_IFACE2(pBroker,IMaterials,pMaterials);
   GET_IFACE2(pBroker,ISectionProperties,pSectProps);
   GET_IFACE2(pBroker,IEnvironment,pEnv);
   GET_IFACE2(pBroker,IBridge,pBridge);

   INIT_UV_PROTOTYPE(rptLengthUnitValue, vs,     pDisplayUnits->GetComponentDimUnit(),    true);
   INIT_UV_PROTOTYPE(rptLengthUnitValue, ecc,    pDisplayUnits->GetComponentDimUnit(),    false);
   INIT_UV_PROTOTYPE(rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(),          true);

   std::_tstring strImagePath(rptStyleManager::GetImagePath());

   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   CClosureKey closureKey;
   bool bIsInClosure = pPoi->IsInClosureJoint(poi,&closureKey);

   IndexType deckCastingRegionIdx = pPoi->GetDeckCastingRegion(poi);
   ATLASSERT(deckCastingRegionIdx != INVALID_INDEX);

   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval(deckCastingRegionIdx);

   ColumnIndexType nColumns = 6; // Interval, tb, te, ..... , esh(tb), esh(te), Desh

   PrestressLossCriteria::TimeDependentConcreteModelType model = pLossParams->GetTimeDependentModel();
   if ( model == PrestressLossCriteria::TimeDependentConcreteModelType::AASHTO )
   {
      if ( WBFL::LRFD::BDSManager::GetEdition() < WBFL::LRFD::BDSManager::Edition::ThirdEditionWith2005Interims )
      {
         nColumns += 3;
         (*pPara) << rptRcImage(strImagePath + _T("AASHTO_Shrinkage_Before_2005.png")) << rptNewLine;
      }
      else if ( WBFL::LRFD::BDSManager::Edition::SeventhEditionWith2015Interims <= WBFL::LRFD::BDSManager::GetEdition() )
      {
         nColumns += 5;
         (*pPara) << rptRcImage(strImagePath + _T("AASHTO_Shrinkage_2015.png")) << rptNewLine;
      }
      else
      {
         nColumns += 5;
         (*pPara) << rptRcImage(strImagePath + _T("AASHTO_Shrinkage_2005.png")) << rptNewLine;
      }
   }
   else if ( model == PrestressLossCriteria::TimeDependentConcreteModelType::ACI209 )
   {
      nColumns += 6;
      (*pPara) << rptRcImage(strImagePath + _T("ACI209_Shrinkage.png")) << rptNewLine;
   }
   else if ( model == PrestressLossCriteria::TimeDependentConcreteModelType::CEBFIP )
   {
      nColumns += 8;
      (*pPara) << rptRcImage(strImagePath + _T("CEBFIP1990_Shrinkage.png")) << rptNewLine;
   }
   else
   {
      ATLASSERT(false);
   }

   (*pPara) << Sub2(_T("t"),_T("b")) << _T(" = Duration of shrinkage to the beginning of the interval") << rptNewLine;
   (*pPara) << Sub2(_T("t"),_T("e")) << _T(" = Duration of shrinkage to the end of the interval") << rptNewLine;
   if ( model == PrestressLossCriteria::TimeDependentConcreteModelType::AASHTO )
   {
      (*pPara) << _T("H = ") << pEnv->GetRelHumidity() << _T(" %") << rptNewLine;
   }
   else if ( model == PrestressLossCriteria::TimeDependentConcreteModelType::ACI209 )
   {
      (*pPara) << _T("RH = ") << pEnv->GetRelHumidity() << _T(" %") << rptNewLine;
   }
   else if ( model == PrestressLossCriteria::TimeDependentConcreteModelType::CEBFIP )
   {
      (*pPara) << _T("RH = ") << pEnv->GetRelHumidity() << _T(" %") << rptNewLine;
   }
   else
   {
      ATLASSERT(false);
   }

   int nParts = (IsNonstructuralDeck(pBridge->GetDeckType()) ? 1 : 2);
   for ( int i = 0; i < nParts; i++ )
   {
      rptParagraph* pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
      (*pChapter) << pPara;
      if ( i == 0 )
      {
         (*pPara) << _T("Girder") << rptNewLine;
      }
      else
      {
         (*pPara) << _T("Deck") << rptNewLine;
      }
      pPara = new rptParagraph;
      (*pChapter) << pPara;

      Float64 V, S;
      if ( i == 0 )
      {
         if ( bIsInClosure )
         {
            pSectProps->GetClosureJointVolumeAndSurfaceArea(closureKey, &V, &S);
         }
         else
         {
            pSectProps->GetSegmentVolumeAndSurfaceArea(segmentKey, &V, &S);
         }
      }
      else
      {
         pSectProps->GetDeckVolumeAndSurfaceArea(&V, &S);
      }
      *pPara << _T("V/S = ") << vs.SetValue(V/S) << rptNewLine;
      
      if ( (model == PrestressLossCriteria::TimeDependentConcreteModelType::AASHTO && WBFL::LRFD::BDSManager::GetEdition() < WBFL::LRFD::BDSManager::Edition::ThirdEditionWith2005Interims) || model == PrestressLossCriteria::TimeDependentConcreteModelType::ACI209 )
      {
         if ( i == 0 )
         {
            if ( bIsInClosure )
            {
               *pPara << _T("Curing: ") << WBFL::Materials::ConcreteBase::GetCuringType(pMaterials->GetClosureJointConcrete(closureKey)->GetCuringType()) << rptNewLine;
            }
            else
            {
               *pPara << _T("Curing: ") << WBFL::Materials::ConcreteBase::GetCuringType(pMaterials->GetSegmentConcrete(segmentKey)->GetCuringType()) << rptNewLine;
            }
         }
         else
         {
            *pPara << _T("Curing: ") << WBFL::Materials::ConcreteBase::GetCuringType(pMaterials->GetDeckConcrete(deckCastingRegionIdx)->GetCuringType()) << rptNewLine;
         }
      }

      if ( model == PrestressLossCriteria::TimeDependentConcreteModelType::AASHTO && WBFL::LRFD::BDSManager::Edition::ThirdEditionWith2005Interims <= WBFL::LRFD::BDSManager::GetEdition() )
      {
         if ( i == 0 )
         {
            if ( bIsInClosure )
            {
               IntervalIndexType compositeClosureIntervalIdx = pIntervals->GetCompositeClosureJointInterval(closureKey);
               Float64 fci = pMaterials->GetClosureJointFc(closureKey,compositeClosureIntervalIdx,pgsTypes::Start);
               (*pPara) << RPT_FCI << _T(" = ") << stress.SetValue(fci) << rptNewLine;
            }
            else
            {
               IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
               Float64 fci = pMaterials->GetSegmentFc(segmentKey,releaseIntervalIdx,pgsTypes::Start);
               (*pPara) << RPT_FCI << _T(" = ") << stress.SetValue(fci) << rptNewLine;
            }
         }
         else
         {
            Float64 fci = pMaterials->GetDeckFc(deckCastingRegionIdx,compositeDeckIntervalIdx,pgsTypes::Start);
            (*pPara) << RPT_FCI << _T(" = ") << stress.SetValue(fci) << rptNewLine;
         }
      }
      else if ( model == PrestressLossCriteria::TimeDependentConcreteModelType::CEBFIP )
      {
         if ( i == 0 )
         {
            if ( bIsInClosure )
            {
               (*pPara) << _T("(") << RPT_FC << Sub2(_T(")"),_T("28")) << _T(" = ") << stress.SetValue(pMaterials->GetClosureJointFc28(closureKey));
            }
            else
            {
               (*pPara) << _T("(") << RPT_FC << Sub2(_T(")"),_T("28")) << _T(" = ") << stress.SetValue(pMaterials->GetSegmentFc28(segmentKey));
            }
         }
         else
         {
            (*pPara) << _T("(") << RPT_FC << Sub2(_T(")"),_T("28")) << _T(" = ") << stress.SetValue(pMaterials->GetDeckFc28());
         }
      }

      rptRcTable* pTable = rptStyleManager::CreateDefaultTable(nColumns);
      *pPara << pTable << rptNewLine;

      RowIndexType rowIdx = 0;
      ColumnIndexType colIdx = 0;
      (*pTable)(rowIdx,colIdx++) << _T("Interval");
      (*pTable)(rowIdx,colIdx++) << Sub2(_T("t"),_T("b")) << rptNewLine << _T("(day)");
      (*pTable)(rowIdx,colIdx++) << Sub2(_T("t"),_T("e")) << rptNewLine << _T("(day)");

      if ( model == PrestressLossCriteria::TimeDependentConcreteModelType::AASHTO )
      {
         if ( WBFL::LRFD::BDSManager::GetEdition() < WBFL::LRFD::BDSManager::Edition::ThirdEditionWith2005Interims )
         {
            (*pTable)(rowIdx,colIdx++) << Sub2(_T("k"),_T("s")) << _T("(") << Sub2(_T("t"),_T("b")) << _T(")");
            (*pTable)(rowIdx,colIdx++) << Sub2(_T("k"),_T("s")) << _T("(") << Sub2(_T("t"),_T("e")) << _T(")");
            (*pTable)(rowIdx,colIdx++) << Sub2(_T("k"),_T("h"));
         }
         else if ( WBFL::LRFD::BDSManager::Edition::SeventhEditionWith2015Interims <= WBFL::LRFD::BDSManager::GetEdition() )
         {
            (*pTable)(rowIdx,colIdx++) << Sub2(_T("k"),_T("vs"));
            (*pTable)(rowIdx,colIdx++) << Sub2(_T("k"),_T("hs"));
            (*pTable)(rowIdx,colIdx++) << Sub2(_T("k"),_T("f"));
            (*pTable)(rowIdx,colIdx++) << Sub2(_T("k"),_T("td")) << _T("(") << Sub2(_T("t"),_T("b")) << _T(")");
            (*pTable)(rowIdx,colIdx++) << Sub2(_T("k"),_T("td")) << _T("(") << Sub2(_T("t"),_T("e")) << _T(")");
         }
         else
         {
            (*pTable)(rowIdx,colIdx++) << Sub2(_T("k"),_T("vs"));
            (*pTable)(rowIdx,colIdx++) << Sub2(_T("k"),_T("hs"));
            (*pTable)(rowIdx,colIdx++) << Sub2(_T("k"),_T("f"));
            (*pTable)(rowIdx,colIdx++) << Sub2(_T("k"),_T("td")) << _T("(") << Sub2(_T("t"),_T("b")) << _T(")");
            (*pTable)(rowIdx,colIdx++) << Sub2(_T("k"),_T("td")) << _T("(") << Sub2(_T("t"),_T("e")) << _T(")");
         }
      }
      else if ( model == PrestressLossCriteria::TimeDependentConcreteModelType::ACI209 )
      {
         (*pTable)(rowIdx,colIdx++) << _T("f");
         (*pTable)(rowIdx,colIdx++) << Sub2(symbol(gamma),_T("t")) << _T("(") << Sub2(_T("t"),_T("b")) << _T(")");
         (*pTable)(rowIdx,colIdx++) << Sub2(symbol(gamma),_T("t")) << _T("(") << Sub2(_T("t"),_T("e")) << _T(")");
         (*pTable)(rowIdx,colIdx++) << Sub2(symbol(gamma),symbol(lambda));
         (*pTable)(rowIdx,colIdx++) << Sub2(symbol(gamma),_T("cp"));
         (*pTable)(rowIdx,colIdx++) << Sub2(symbol(gamma),_T("vs"));
      }
      else if ( model == PrestressLossCriteria::TimeDependentConcreteModelType::CEBFIP )
      {
         (*pTable)(rowIdx,colIdx++) << COLHDR(_T("h"),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
         (*pTable)(rowIdx,colIdx++) << Sub2(symbol(beta),_T("sc"));
         (*pTable)(rowIdx,colIdx++) << Sub2(symbol(beta),_T("sRH"));
         (*pTable)(rowIdx,colIdx++) << Sub2(symbol(beta),_T("RH"));
         (*pTable)(rowIdx,colIdx++) << Sub2(symbol(beta),_T("s")) << _T("(") << Sub2(_T("t"),_T("b")) << _T(")");
         (*pTable)(rowIdx,colIdx++) << Sub2(symbol(beta),_T("s")) << _T("(") << Sub2(_T("t"),_T("e")) << _T(")");
         (*pTable)(rowIdx,colIdx++) << Sub2(symbol(epsilon),_T("s")) << _T("((") << RPT_FC << Sub2(_T(")"),_T("28")) << _T(")") << Super2(_T("x10"),_T("6"));
         (*pTable)(rowIdx,colIdx++) << Sub2(symbol(epsilon),_T("cso")) << Super2(_T("x10"),_T("6"));
      }
      else
      {
         ATLASSERT(false);
      }

      (*pTable)(rowIdx,colIdx++) << Sub2(symbol(epsilon),_T("sh")) << _T("(") << Sub2(_T("t"),_T("b")) << _T(")") << Super2(_T("x10"),_T("6"));
      (*pTable)(rowIdx,colIdx++) << Sub2(symbol(epsilon),_T("sh")) << _T("(") << Sub2(_T("t"),_T("e")) << _T(")") << Super2(_T("x10"),_T("6"));
      (*pTable)(rowIdx,colIdx++) << DELTA_ESH << Super2(_T("x10"),_T("6"));


      rowIdx = pTable->GetNumberOfHeaderRows();

      if ( i == 1 && firstIntervalIdx < compositeDeckIntervalIdx )
      {
         // we reporting for the deck, make the first interval
         // when the deck becomes composite (values are just 0 before this)
         firstIntervalIdx = compositeDeckIntervalIdx;
      }

      for ( IntervalIndexType intervalIdx = firstIntervalIdx; intervalIdx <= lastIntervalIdx; intervalIdx++ )
      {
         if ( intervalIdx < releaseIntervalIdx || ::IsZero(pIntervals->GetDuration(intervalIdx)) )
         {
            continue;
         }

         const LOSSDETAILS* pDetails = pLosses->GetLossDetails(poi,intervalIdx);
         const TIME_STEP_DETAILS& tsDetails(pDetails->TimeStepDetails[intervalIdx]);

         colIdx = 0;
         (*pTable)(rowIdx,colIdx++) << LABEL_INTERVAL(intervalIdx);
      
         colIdx = 1;
         const TIME_STEP_CONCRETE* pConcrete;
         if ( i == 0 )
         {
            pConcrete = &tsDetails.Girder;
         }
         else
         {
            pConcrete = &tsDetails.Deck;
         }

         (*pTable)(rowIdx,colIdx++) << pConcrete->Shrinkage.pStartDetails->shrinkage_duration;
         (*pTable)(rowIdx,colIdx++) << pConcrete->Shrinkage.pEndDetails->shrinkage_duration;

         if ( model == PrestressLossCriteria::TimeDependentConcreteModelType::AASHTO )
         {
            const std::shared_ptr<WBFL::LRFD::LRFDTimeDependentConcreteShrinkageDetails> pStartDetails = std::dynamic_pointer_cast<WBFL::LRFD::LRFDTimeDependentConcreteShrinkageDetails>(pConcrete->Shrinkage.pStartDetails);
            const std::shared_ptr<WBFL::LRFD::LRFDTimeDependentConcreteShrinkageDetails> pEndDetails   = std::dynamic_pointer_cast<WBFL::LRFD::LRFDTimeDependentConcreteShrinkageDetails>(pConcrete->Shrinkage.pEndDetails);
            if ( WBFL::LRFD::BDSManager::GetEdition() < WBFL::LRFD::BDSManager::Edition::ThirdEditionWith2005Interims )
            {
               (*pTable)(rowIdx,colIdx++) << pStartDetails->kvs;
               (*pTable)(rowIdx,colIdx++) << pEndDetails->kvs;
               (*pTable)(rowIdx,colIdx++) << pStartDetails->khs;
            }
            else if ( WBFL::LRFD::BDSManager::Edition::SeventhEditionWith2015Interims <= WBFL::LRFD::BDSManager::GetEdition() )
            {
               (*pTable)(rowIdx,colIdx++) << pStartDetails->kvs;
               (*pTable)(rowIdx,colIdx++) << pStartDetails->khs;
               (*pTable)(rowIdx,colIdx++) << pStartDetails->kf;
               (*pTable)(rowIdx,colIdx++) << pStartDetails->ktd;
               (*pTable)(rowIdx,colIdx++) << pEndDetails->ktd;
            }
            else
            {
               (*pTable)(rowIdx,colIdx++) << pStartDetails->kvs;
               (*pTable)(rowIdx,colIdx++) << pStartDetails->khs;
               (*pTable)(rowIdx,colIdx++) << pStartDetails->kf;
               (*pTable)(rowIdx,colIdx++) << pStartDetails->ktd;
               (*pTable)(rowIdx,colIdx++) << pEndDetails->ktd;
            }
         }
         else if ( model == PrestressLossCriteria::TimeDependentConcreteModelType::ACI209 )
         {
            const std::shared_ptr<WBFL::Materials::ACI209ConcreteShrinkageDetails> pStartDetails = std::dynamic_pointer_cast<WBFL::Materials::ACI209ConcreteShrinkageDetails>(pConcrete->Shrinkage.pStartDetails);
            const std::shared_ptr<WBFL::Materials::ACI209ConcreteShrinkageDetails> pEndDetails   = std::dynamic_pointer_cast<WBFL::Materials::ACI209ConcreteShrinkageDetails>(pConcrete->Shrinkage.pEndDetails);
            (*pTable)(rowIdx,colIdx++) << pStartDetails->f;
            (*pTable)(rowIdx,colIdx++) << pStartDetails->time_factor;
            (*pTable)(rowIdx,colIdx++) << pEndDetails->time_factor;
            (*pTable)(rowIdx,colIdx++) << pStartDetails->humidity_factor;
            (*pTable)(rowIdx,colIdx++) << pStartDetails->curing_factor;
            (*pTable)(rowIdx,colIdx++) << pStartDetails->vs_factor;
         }
         else if ( model == PrestressLossCriteria::TimeDependentConcreteModelType::CEBFIP )
         {
            const std::shared_ptr<WBFL::Materials::CEBFIPConcreteShrinkageDetails> pStartDetails = std::dynamic_pointer_cast<WBFL::Materials::CEBFIPConcreteShrinkageDetails>(pConcrete->Shrinkage.pStartDetails);
            const std::shared_ptr<WBFL::Materials::CEBFIPConcreteShrinkageDetails> pEndDetails   = std::dynamic_pointer_cast<WBFL::Materials::CEBFIPConcreteShrinkageDetails>(pConcrete->Shrinkage.pEndDetails);
            (*pTable)(rowIdx,colIdx++) << ecc.SetValue(pStartDetails->h);
            (*pTable)(rowIdx,colIdx++) << pStartDetails->BetaSC;
            if ( pStartDetails->BetaSRH < 0 )
            {
               (*pTable)(rowIdx,colIdx++) << _T("-");
            }
            else
            {
               (*pTable)(rowIdx,colIdx++) << pStartDetails->BetaSRH;
            }
            (*pTable)(rowIdx,colIdx++) << pStartDetails->BetaRH;
            (*pTable)(rowIdx,colIdx++) << pStartDetails->BetaS;
            (*pTable)(rowIdx,colIdx++) << pEndDetails->BetaS;
            (*pTable)(rowIdx,colIdx++) << pStartDetails->es*1E6;
            (*pTable)(rowIdx,colIdx++) << pStartDetails->ecso*1E6;
         }
         else
         {
            ATLASSERT(false);
         }

         (*pTable)(rowIdx,colIdx++) << pConcrete->Shrinkage.pStartDetails->esh*1E6;
         (*pTable)(rowIdx,colIdx++) << pConcrete->Shrinkage.pEndDetails->esh*1E6;
         (*pTable)(rowIdx,colIdx++) << pConcrete->Shrinkage.esi*1E6;

         rowIdx++;
      }
   }
}

void CTimeStepDetailsChapterBuilder::ReportStrandRelaxationDetails(rptChapter* pChapter,IBroker* pBroker,const pgsPointOfInterest& poi,IntervalIndexType firstIntervalIdx,IntervalIndexType lastIntervalIdx,IEAFDisplayUnits* pDisplayUnits) const
{
   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
   (*pChapter) << pPara;
   (*pPara) << _T("Strand relaxation details") << rptNewLine;
   pPara = new rptParagraph;
   (*pChapter) << pPara;

   GET_IFACE2(pBroker,ILossParameters,pLossParams);
   if ( pLossParams->IgnoreRelaxationEffects() )
   {
      (*pPara) << _T("Relaxation effects were ignored") << rptNewLine;
      return;
   }

   GET_IFACE2(pBroker,IMaterials,pMaterials);
   GET_IFACE2(pBroker,ILosses,pLosses);
   GET_IFACE2(pBroker,IIntervals,pIntervals);

   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   IntervalIndexType stressStrandIntervalIdx = pIntervals->GetStressStrandInterval(segmentKey);

   INIT_UV_PROTOTYPE(rptStressUnitValue,    stress,     pDisplayUnits->GetStressUnit(),          false);

   std::_tstring strImagePath(rptStyleManager::GetImagePath());

   PrestressLossCriteria::TimeDependentConcreteModelType model = pLossParams->GetTimeDependentModel();
   const auto* pPermanentStrand = pMaterials->GetStrandMaterial(segmentKey,pgsTypes::Straight); // ok to use straight since we are just getting material properties, not strand size
   const auto* pTemporaryStrand = pMaterials->GetStrandMaterial(segmentKey,pgsTypes::Temporary);
   if ( pPermanentStrand == pTemporaryStrand )
   {
      if ( model == PrestressLossCriteria::TimeDependentConcreteModelType::AASHTO || model == PrestressLossCriteria::TimeDependentConcreteModelType::ACI209 )
      {
         if (pPermanentStrand->GetType() == WBFL::Materials::PsStrand::Type::StressRelieved )
         {
            (*pPara) << rptRcImage(strImagePath + _T("ACI209RelaxationSR.png")) << rptNewLine;
         }
         else
         {
            (*pPara) << rptRcImage(strImagePath + _T("ACI209RelaxationLR.png")) << rptNewLine;
         }
      }
      else
      {
         if (pPermanentStrand->GetType() == WBFL::Materials::PsStrand::Type::StressRelieved )
         {
            (*pPara) << rptRcImage(strImagePath + _T("CEBFIPRelaxationSR.png")) << rptNewLine;
         }
         else
         {
            (*pPara) << rptRcImage(strImagePath + _T("CEBFIPRelaxationLR.png")) << rptNewLine;
         }
      }
   }
   else
   {
      *pPara << _T("Straight and Harped Strands") << rptNewLine;
      if ( model == PrestressLossCriteria::TimeDependentConcreteModelType::AASHTO || model == PrestressLossCriteria::TimeDependentConcreteModelType::ACI209 )
      {
         if (pPermanentStrand->GetType() == WBFL::Materials::PsStrand::Type::StressRelieved )
         {
            (*pPara) << rptRcImage(strImagePath + _T("ACI209RelaxationSR.png")) << rptNewLine;
         }
         else
         {
            (*pPara) << rptRcImage(strImagePath + _T("ACI209RelaxationLR.png")) << rptNewLine;
         }
      }
      else
      {
         if (pPermanentStrand->GetType() == WBFL::Materials::PsStrand::Type::StressRelieved )
         {
            (*pPara) << rptRcImage(strImagePath + _T("CEBFIPRelaxationSR.png")) << rptNewLine;
         }
         else
         {
            (*pPara) << rptRcImage(strImagePath + _T("CEBFIPRelaxationLR.png")) << rptNewLine;
         }
      }

      *pPara << rptNewLine;
      *pPara << _T("Temporary Strands") << rptNewLine;
      if ( model == PrestressLossCriteria::TimeDependentConcreteModelType::AASHTO || model == PrestressLossCriteria::TimeDependentConcreteModelType::ACI209 )
      {
         if (pTemporaryStrand->GetType() == WBFL::Materials::PsStrand::Type::StressRelieved )
         {
            (*pPara) << rptRcImage(strImagePath + _T("ACI209RelaxationSR.png")) << rptNewLine;
         }
         else
         {
            (*pPara) << rptRcImage(strImagePath + _T("ACI209RelaxationLR.png")) << rptNewLine;
         }
      }
      else
      {
         if (pTemporaryStrand->GetType() == WBFL::Materials::PsStrand::Type::StressRelieved )
         {
            (*pPara) << rptRcImage(strImagePath + _T("CEBFIPRelaxationSR.png")) << rptNewLine;
         }
         else
         {
            (*pPara) << rptRcImage(strImagePath + _T("CEBFIPRelaxationLR.png")) << rptNewLine;
         }
      }
   }

   (*pPara) << symbol(DELTA) << RPT_STRESS(_T("r")) << _T(" = -") << symbol(DELTA) << RPT_STRESS(_T("pR")) << rptNewLine;
   (*pPara) << Sub2(_T("t"),_T("b")) << _T(" = time from stressing to the beginning of the interval") << rptNewLine;
   (*pPara) << Sub2(_T("t"),_T("e")) << _T(" = time from stressing to the end of the interval") << rptNewLine;

   ColumnIndexType nColumns = 2;
   if ( model == PrestressLossCriteria::TimeDependentConcreteModelType::AASHTO || model == PrestressLossCriteria::TimeDependentConcreteModelType::ACI209 )
   {
      nColumns += 6;
   }
   else
   {
      ATLASSERT(model == PrestressLossCriteria::TimeDependentConcreteModelType::CEBFIP);
      nColumns += 8;
   }
   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(nColumns);
   *pPara << pTable << rptNewLine;

   RowIndexType rowIdx = 0;
   ColumnIndexType colIdx = 0;

   (*pTable)(rowIdx,colIdx++) << _T("Interval");
   (*pTable)(rowIdx,colIdx++) << _T("Strand");
   if ( model == PrestressLossCriteria::TimeDependentConcreteModelType::AASHTO || model == PrestressLossCriteria::TimeDependentConcreteModelType::ACI209 )
   {
      (*pTable)(rowIdx,colIdx++) << COLHDR(RPT_FPE,rptStressUnitTag,pDisplayUnits->GetStressUnit());
      (*pTable)(rowIdx,colIdx++) << COLHDR(RPT_FPY,rptStressUnitTag,pDisplayUnits->GetStressUnit());
      (*pTable)(rowIdx,colIdx++) << Sub2(_T("t"),_T("b")) << rptNewLine << _T("(day)");
      (*pTable)(rowIdx,colIdx++) << Sub2(_T("t"),_T("e")) << rptNewLine << _T("(day)");
      (*pTable)(rowIdx,colIdx++) << _T("Epoxy") << rptNewLine << _T("Factor");
   }
   else
   {
      ATLASSERT(model == PrestressLossCriteria::TimeDependentConcreteModelType::CEBFIP);
      (*pTable)(rowIdx,colIdx++) << symbol(rho);
      (*pTable)(rowIdx,colIdx++) << _T("k");
      (*pTable)(rowIdx,colIdx++) << COLHDR(RPT_FPE,rptStressUnitTag,pDisplayUnits->GetStressUnit());
      (*pTable)(rowIdx,colIdx++) << COLHDR(RPT_FPU,rptStressUnitTag,pDisplayUnits->GetStressUnit());
      (*pTable)(rowIdx,colIdx++) << Sub2(_T("t"),_T("b")) << rptNewLine << _T("(day)");
      (*pTable)(rowIdx,colIdx++) << Sub2(_T("t"),_T("e")) << rptNewLine << _T("(day)");
      (*pTable)(rowIdx,colIdx++) << _T("Epoxy") << rptNewLine << _T("Factor");
   }
   (*pTable)(rowIdx,colIdx++) << COLHDR(DELTA_FR,rptStressUnitTag,pDisplayUnits->GetStressUnit());

   std::_tstring strStrandType[] = { _T("Straight"), _T("Harped"), _T("Temporary") };

   rowIdx = pTable->GetNumberOfHeaderRows();
   for ( IntervalIndexType intervalIdx = firstIntervalIdx; intervalIdx <= lastIntervalIdx; intervalIdx++ )
   {
      if ( firstIntervalIdx != lastIntervalIdx && (intervalIdx < stressStrandIntervalIdx || ::IsZero(pIntervals->GetDuration(intervalIdx))) )
      {
         continue;
      }

      const LOSSDETAILS* pDetails = pLosses->GetLossDetails(poi,intervalIdx);
      const TIME_STEP_DETAILS& tsDetails(pDetails->TimeStepDetails[intervalIdx]);

      colIdx = 0;
      pTable->SetRowSpan(rowIdx,colIdx,3);
      (*pTable)(rowIdx,colIdx++) << LABEL_INTERVAL(intervalIdx);
      
      for ( int i = 0; i < 3; i++ )
      {
         colIdx = 1;
         pgsTypes::StrandType strandType = (pgsTypes::StrandType)i;
         (*pTable)(rowIdx,colIdx++) << strStrandType[strandType];

         if ( model == PrestressLossCriteria::TimeDependentConcreteModelType::AASHTO || model == PrestressLossCriteria::TimeDependentConcreteModelType::ACI209 )
         {
            (*pTable)(rowIdx,colIdx++) << stress.SetValue(tsDetails.Strands[strandType].Relaxation.fpi);
            (*pTable)(rowIdx,colIdx++) << stress.SetValue(tsDetails.Strands[strandType].Relaxation.fpy);
            (*pTable)(rowIdx,colIdx++) << tsDetails.Strands[strandType].Relaxation.tStart;
            (*pTable)(rowIdx,colIdx++) << tsDetails.Strands[strandType].Relaxation.tEnd;
            (*pTable)(rowIdx,colIdx++) << tsDetails.Strands[strandType].Relaxation.epoxyFactor;
         }
         else
         {
            ATLASSERT(model == PrestressLossCriteria::TimeDependentConcreteModelType::CEBFIP);
            (*pTable)(rowIdx,colIdx++) << tsDetails.Strands[strandType].Relaxation.p;
            (*pTable)(rowIdx,colIdx++) << tsDetails.Strands[strandType].Relaxation.k;
            (*pTable)(rowIdx,colIdx++) << stress.SetValue(tsDetails.Strands[strandType].Relaxation.fpi);
            (*pTable)(rowIdx,colIdx++) << stress.SetValue(tsDetails.Strands[strandType].Relaxation.fpu);
            (*pTable)(rowIdx,colIdx++) << tsDetails.Strands[strandType].Relaxation.tStart;
            (*pTable)(rowIdx,colIdx++) << tsDetails.Strands[strandType].Relaxation.tEnd;
            (*pTable)(rowIdx,colIdx++) << tsDetails.Strands[strandType].Relaxation.epoxyFactor;
         }

         (*pTable)(rowIdx,colIdx++) << stress.SetValue(tsDetails.Strands[strandType].Relaxation.fr);

         rowIdx++;
      }
   }
}

void CTimeStepDetailsChapterBuilder::ReportSegmentTendonRelaxationDetails(rptChapter* pChapter, IBroker* pBroker, const pgsPointOfInterest& poi, IntervalIndexType firstIntervalIdx, IntervalIndexType lastIntervalIdx, IEAFDisplayUnits* pDisplayUnits) const
{
   GET_IFACE2(pBroker, ILossParameters, pLossParams);
   GET_IFACE2(pBroker, IMaterials, pMaterials);
   GET_IFACE2(pBroker, ILosses, pLosses);
   GET_IFACE2(pBroker, ISegmentTendonGeometry, pTendonGeom);
   GET_IFACE2(pBroker, IIntervals, pIntervals);

   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   IntervalIndexType stressTendonIntervalIdx = pIntervals->GetStressSegmentTendonInterval(segmentKey);

   INIT_UV_PROTOTYPE(rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), false);

   std::_tstring strImagePath(rptStyleManager::GetImagePath());

   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
   (*pChapter) << pPara;
   (*pPara) << _T("Segment Tendon relaxation details") << rptNewLine;
   pPara = new rptParagraph;
   (*pChapter) << pPara;

   PrestressLossCriteria::TimeDependentConcreteModelType model = pLossParams->GetTimeDependentModel();
   const auto* pTendon = pMaterials->GetSegmentTendonMaterial(segmentKey);
   if (model == PrestressLossCriteria::TimeDependentConcreteModelType::AASHTO || model == PrestressLossCriteria::TimeDependentConcreteModelType::ACI209)
   {
      if (pTendon->GetType() == WBFL::Materials::PsStrand::Type::StressRelieved)
      {
         (*pPara) << rptRcImage(strImagePath + _T("ACI209RelaxationSR.png")) << rptNewLine;
      }
      else
      {
         (*pPara) << rptRcImage(strImagePath + _T("ACI209RelaxationLR.png")) << rptNewLine;
      }
   }
   else
   {
      if (pTendon->GetType() == WBFL::Materials::PsStrand::Type::StressRelieved)
      {
         (*pPara) << rptRcImage(strImagePath + _T("CEBFIPRelaxationSR.png")) << rptNewLine;
      }
      else
      {
         (*pPara) << rptRcImage(strImagePath + _T("CEBFIPRelaxationLR.png")) << rptNewLine;
      }
   }

   (*pPara) << Sub2(_T("t"), _T("b")) << _T(" = time from stressing to the beginning of the interval") << rptNewLine;
   (*pPara) << Sub2(_T("t"), _T("e")) << _T(" = time from stressing to the end of the interval") << rptNewLine;

   ColumnIndexType nColumns = 2;
   if (model == PrestressLossCriteria::TimeDependentConcreteModelType::AASHTO || model == PrestressLossCriteria::TimeDependentConcreteModelType::ACI209)
   {
      nColumns += 6;
   }
   else
   {
      ATLASSERT(model == PrestressLossCriteria::TimeDependentConcreteModelType::CEBFIP);
      nColumns += 8;
   }
   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(nColumns);
   *pPara << pTable << rptNewLine;

   RowIndexType rowIdx = 0;
   ColumnIndexType colIdx = 0;

   (*pTable)(rowIdx, colIdx++) << _T("Interval");
   (*pTable)(rowIdx, colIdx++) << _T("Tendon");
   if (model == PrestressLossCriteria::TimeDependentConcreteModelType::AASHTO || model == PrestressLossCriteria::TimeDependentConcreteModelType::ACI209)
   {
      (*pTable)(rowIdx, colIdx++) << COLHDR(RPT_FPE, rptStressUnitTag, pDisplayUnits->GetStressUnit());
      (*pTable)(rowIdx, colIdx++) << COLHDR(RPT_FPY, rptStressUnitTag, pDisplayUnits->GetStressUnit());
      (*pTable)(rowIdx, colIdx++) << Sub2(_T("t"), _T("b")) << rptNewLine << _T("(day)");
      (*pTable)(rowIdx, colIdx++) << Sub2(_T("t"), _T("e")) << rptNewLine << _T("(day)");
      (*pTable)(rowIdx, colIdx++) << _T("Epoxy") << rptNewLine << _T("Factor");
   }
   else
   {
      ATLASSERT(model == PrestressLossCriteria::TimeDependentConcreteModelType::CEBFIP);
      (*pTable)(rowIdx, colIdx++) << symbol(rho);
      (*pTable)(rowIdx, colIdx++) << _T("k");
      (*pTable)(rowIdx, colIdx++) << COLHDR(RPT_FPE, rptStressUnitTag, pDisplayUnits->GetStressUnit());
      (*pTable)(rowIdx, colIdx++) << COLHDR(RPT_FPU, rptStressUnitTag, pDisplayUnits->GetStressUnit());
      (*pTable)(rowIdx, colIdx++) << Sub2(_T("t"), _T("b")) << rptNewLine << _T("(day)");
      (*pTable)(rowIdx, colIdx++) << Sub2(_T("t"), _T("e")) << rptNewLine << _T("(day)");
      (*pTable)(rowIdx, colIdx++) << _T("Epoxy") << rptNewLine << _T("Factor");
   }
   (*pTable)(rowIdx, colIdx++) << COLHDR(DELTA_FR, rptStressUnitTag, pDisplayUnits->GetStressUnit());

   DuctIndexType nTendons = pTendonGeom->GetDuctCount(segmentKey);

   rowIdx = pTable->GetNumberOfHeaderRows();
   for (IntervalIndexType intervalIdx = firstIntervalIdx; intervalIdx <= lastIntervalIdx; intervalIdx++)
   {
      if (firstIntervalIdx != lastIntervalIdx && (intervalIdx < stressTendonIntervalIdx || ::IsZero(pIntervals->GetDuration(intervalIdx))))
      {
         continue;
      }

      const LOSSDETAILS* pDetails = pLosses->GetLossDetails(poi, intervalIdx);
      const TIME_STEP_DETAILS& tsDetails(pDetails->TimeStepDetails[intervalIdx]);

      colIdx = 0;
      pTable->SetRowSpan(rowIdx, colIdx, nTendons);
      (*pTable)(rowIdx, colIdx++) << LABEL_INTERVAL(intervalIdx);

      for (DuctIndexType tendonIdx = 0; tendonIdx < nTendons; tendonIdx++)
      {
         colIdx = 1;
         (*pTable)(rowIdx, colIdx++) << LABEL_DUCT(tendonIdx);

         if (model == PrestressLossCriteria::TimeDependentConcreteModelType::AASHTO || model == PrestressLossCriteria::TimeDependentConcreteModelType::ACI209)
         {
            (*pTable)(rowIdx, colIdx++) << stress.SetValue(tsDetails.SegmentTendons[tendonIdx].Relaxation.fpi);
            (*pTable)(rowIdx, colIdx++) << stress.SetValue(tsDetails.SegmentTendons[tendonIdx].Relaxation.fpy);
            (*pTable)(rowIdx, colIdx++) << tsDetails.SegmentTendons[tendonIdx].Relaxation.tStart;
            (*pTable)(rowIdx, colIdx++) << tsDetails.SegmentTendons[tendonIdx].Relaxation.tEnd;
            (*pTable)(rowIdx, colIdx++) << tsDetails.SegmentTendons[tendonIdx].Relaxation.epoxyFactor;
         }
         else
         {
            ATLASSERT(model == PrestressLossCriteria::TimeDependentConcreteModelType::CEBFIP);
            (*pTable)(rowIdx, colIdx++) << tsDetails.SegmentTendons[tendonIdx].Relaxation.p;
            (*pTable)(rowIdx, colIdx++) << tsDetails.SegmentTendons[tendonIdx].Relaxation.k;
            (*pTable)(rowIdx, colIdx++) << stress.SetValue(tsDetails.SegmentTendons[tendonIdx].Relaxation.fpi);
            (*pTable)(rowIdx, colIdx++) << stress.SetValue(tsDetails.SegmentTendons[tendonIdx].Relaxation.fpu);
            (*pTable)(rowIdx, colIdx++) << tsDetails.SegmentTendons[tendonIdx].Relaxation.tStart;
            (*pTable)(rowIdx, colIdx++) << tsDetails.SegmentTendons[tendonIdx].Relaxation.tEnd;
            (*pTable)(rowIdx, colIdx++) << tsDetails.SegmentTendons[tendonIdx].Relaxation.epoxyFactor;
         }

         (*pTable)(rowIdx, colIdx++) << stress.SetValue(tsDetails.SegmentTendons[tendonIdx].Relaxation.fr);

         rowIdx++;
      }
   }
}

void CTimeStepDetailsChapterBuilder::ReportGirderTendonRelaxationDetails(rptChapter* pChapter,IBroker* pBroker,const pgsPointOfInterest& poi,IntervalIndexType firstIntervalIdx,IntervalIndexType lastIntervalIdx,IEAFDisplayUnits* pDisplayUnits) const
{
   GET_IFACE2(pBroker,ILossParameters,pLossParams);
   GET_IFACE2(pBroker,IMaterials,pMaterials);
   GET_IFACE2(pBroker,ILosses,pLosses);
   GET_IFACE2(pBroker,IGirderTendonGeometry,pTendonGeom);
   GET_IFACE2(pBroker,IIntervals,pIntervals);

   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   IntervalIndexType stressFirstTendonIntervalIdx = pIntervals->GetFirstGirderTendonStressingInterval(segmentKey);

   INIT_UV_PROTOTYPE(rptStressUnitValue,    stress,     pDisplayUnits->GetStressUnit(),          false);

   std::_tstring strImagePath(rptStyleManager::GetImagePath());

   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
   (*pChapter) << pPara;
   (*pPara) << _T("Girder Tendon relaxation details") << rptNewLine;
   pPara = new rptParagraph;
   (*pChapter) << pPara;

   PrestressLossCriteria::TimeDependentConcreteModelType model = pLossParams->GetTimeDependentModel();
   const auto* pTendon = pMaterials->GetGirderTendonMaterial(segmentKey);
   if ( model == PrestressLossCriteria::TimeDependentConcreteModelType::AASHTO || model == PrestressLossCriteria::TimeDependentConcreteModelType::ACI209 )
   {
      if (pTendon->GetType() == WBFL::Materials::PsStrand::Type::StressRelieved )
      {
         (*pPara) << rptRcImage(strImagePath + _T("ACI209RelaxationSR.png")) << rptNewLine;
      }
      else
      {
         (*pPara) << rptRcImage(strImagePath + _T("ACI209RelaxationLR.png")) << rptNewLine;
      }
   }
   else
   {
      if (pTendon->GetType() == WBFL::Materials::PsStrand::Type::StressRelieved )
      {
         (*pPara) << rptRcImage(strImagePath + _T("CEBFIPRelaxationSR.png")) << rptNewLine;
      }
      else
      {
         (*pPara) << rptRcImage(strImagePath + _T("CEBFIPRelaxationLR.png")) << rptNewLine;
      }
   }

   (*pPara) << Sub2(_T("t"),_T("b")) << _T(" = time from stressing to the beginning of the interval") << rptNewLine;
   (*pPara) << Sub2(_T("t"),_T("e")) << _T(" = time from stressing to the end of the interval") << rptNewLine;

   ColumnIndexType nColumns = 2;
   if ( model == PrestressLossCriteria::TimeDependentConcreteModelType::AASHTO || model == PrestressLossCriteria::TimeDependentConcreteModelType::ACI209 )
   {
      nColumns += 6;
   }
   else
   {
      ATLASSERT(model == PrestressLossCriteria::TimeDependentConcreteModelType::CEBFIP);
      nColumns += 8;
   }
   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(nColumns);
   *pPara << pTable << rptNewLine;

   RowIndexType rowIdx = 0;
   ColumnIndexType colIdx = 0;

   (*pTable)(rowIdx,colIdx++) << _T("Interval");
   (*pTable)(rowIdx,colIdx++) << _T("Tendon");
   if ( model == PrestressLossCriteria::TimeDependentConcreteModelType::AASHTO || model == PrestressLossCriteria::TimeDependentConcreteModelType::ACI209 )
   {
      (*pTable)(rowIdx,colIdx++) << COLHDR(RPT_FPE,rptStressUnitTag,pDisplayUnits->GetStressUnit());
      (*pTable)(rowIdx,colIdx++) << COLHDR(RPT_FPY,rptStressUnitTag,pDisplayUnits->GetStressUnit());
      (*pTable)(rowIdx,colIdx++) << Sub2(_T("t"),_T("b")) << rptNewLine << _T("(day)");
      (*pTable)(rowIdx,colIdx++) << Sub2(_T("t"),_T("e")) << rptNewLine << _T("(day)");
      (*pTable)(rowIdx,colIdx++) << _T("Epoxy") << rptNewLine << _T("Factor");
   }
   else
   {
      ATLASSERT(model == PrestressLossCriteria::TimeDependentConcreteModelType::CEBFIP);
      (*pTable)(rowIdx,colIdx++) << symbol(rho);
      (*pTable)(rowIdx,colIdx++) << _T("k");
      (*pTable)(rowIdx,colIdx++) << COLHDR(RPT_FPE,rptStressUnitTag,pDisplayUnits->GetStressUnit());
      (*pTable)(rowIdx,colIdx++) << COLHDR(RPT_FPU,rptStressUnitTag,pDisplayUnits->GetStressUnit());
      (*pTable)(rowIdx,colIdx++) << Sub2(_T("t"),_T("b")) << rptNewLine << _T("(day)");
      (*pTable)(rowIdx,colIdx++) << Sub2(_T("t"),_T("e")) << rptNewLine << _T("(day)");
      (*pTable)(rowIdx,colIdx++) << _T("Epoxy") << rptNewLine << _T("Factor");
   }
   (*pTable)(rowIdx,colIdx++) << COLHDR(DELTA_FR,rptStressUnitTag,pDisplayUnits->GetStressUnit());

   DuctIndexType nTendons = pTendonGeom->GetDuctCount(segmentKey);

   rowIdx = pTable->GetNumberOfHeaderRows();
   for ( IntervalIndexType intervalIdx = firstIntervalIdx; intervalIdx <= lastIntervalIdx; intervalIdx++ )
   {
      if ( firstIntervalIdx != lastIntervalIdx && (intervalIdx < stressFirstTendonIntervalIdx || ::IsZero(pIntervals->GetDuration(intervalIdx))) )
      {
         continue;
      }

      const LOSSDETAILS* pDetails = pLosses->GetLossDetails(poi,intervalIdx);
      const TIME_STEP_DETAILS& tsDetails(pDetails->TimeStepDetails[intervalIdx]);

      colIdx = 0;
      pTable->SetRowSpan(rowIdx,colIdx,nTendons);
      (*pTable)(rowIdx,colIdx++) << LABEL_INTERVAL(intervalIdx);
      
      for ( DuctIndexType tendonIdx = 0; tendonIdx < nTendons; tendonIdx++ )
      {
         colIdx = 1;
         (*pTable)(rowIdx,colIdx++) << LABEL_DUCT(tendonIdx);

         if ( model == PrestressLossCriteria::TimeDependentConcreteModelType::AASHTO || model == PrestressLossCriteria::TimeDependentConcreteModelType::ACI209 )
         {
            (*pTable)(rowIdx,colIdx++) << stress.SetValue(tsDetails.GirderTendons[tendonIdx].Relaxation.fpi);
            (*pTable)(rowIdx,colIdx++) << stress.SetValue(tsDetails.GirderTendons[tendonIdx].Relaxation.fpy);
            (*pTable)(rowIdx,colIdx++) << tsDetails.GirderTendons[tendonIdx].Relaxation.tStart;
            (*pTable)(rowIdx,colIdx++) << tsDetails.GirderTendons[tendonIdx].Relaxation.tEnd;
            (*pTable)(rowIdx,colIdx++) << tsDetails.GirderTendons[tendonIdx].Relaxation.epoxyFactor;
         }
         else
         {
            ATLASSERT(model == PrestressLossCriteria::TimeDependentConcreteModelType::CEBFIP);
            (*pTable)(rowIdx,colIdx++) << tsDetails.GirderTendons[tendonIdx].Relaxation.p;
            (*pTable)(rowIdx,colIdx++) << tsDetails.GirderTendons[tendonIdx].Relaxation.k;
            (*pTable)(rowIdx,colIdx++) << stress.SetValue(tsDetails.GirderTendons[tendonIdx].Relaxation.fpi);
            (*pTable)(rowIdx,colIdx++) << stress.SetValue(tsDetails.GirderTendons[tendonIdx].Relaxation.fpu);
            (*pTable)(rowIdx,colIdx++) << tsDetails.GirderTendons[tendonIdx].Relaxation.tStart;
            (*pTable)(rowIdx,colIdx++) << tsDetails.GirderTendons[tendonIdx].Relaxation.tEnd;
            (*pTable)(rowIdx,colIdx++) << tsDetails.GirderTendons[tendonIdx].Relaxation.epoxyFactor;
         }

         (*pTable)(rowIdx,colIdx++) << stress.SetValue(tsDetails.GirderTendons[tendonIdx].Relaxation.fr);

         rowIdx++;
      }
   }
}

