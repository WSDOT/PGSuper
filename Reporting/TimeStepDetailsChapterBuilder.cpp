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

#include "StdAfx.h"
#include <Reporting\TimeStepDetailsChapterBuilder.h>
#include <Reporting\TimeStepDetailsReportSpecification.h>

#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\PointOfInterest.h>
#include <IFace\PrestressForce.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Intervals.h>

#include <WBFLGenericBridgeTools.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define DELTA_P    symbol(DELTA) << _T("P")
#define DELTA_M    symbol(DELTA) << _T("M")
#define DELTA_Pk   symbol(DELTA) << Sub2(_T("P"),_T("k"))
#define DELTA_Mk   symbol(DELTA) << Sub2(_T("M"),_T("k"))
#define DELTA_E    symbol(DELTA) << symbol(epsilon)
#define DELTA_R    symbol(DELTA) << symbol(varphi)
#define DELTA_FR   symbol(DELTA) << RPT_STRESS(_T("r"))
#define DELTA_ER   symbol(DELTA) << Sub2(symbol(epsilon),_T("r"))

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

rptChapter* CTimeStepDetailsChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);
   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   CTimeStepDetailsReportSpecification* pTSDRptSpec = dynamic_cast<CTimeStepDetailsReportSpecification*>(pRptSpec);

   CComPtr<IBroker> pBroker;
   pTSDRptSpec->GetBroker(&pBroker);

   GET_IFACE2(pBroker, ILossParameters, pLossParams);
   if ( pLossParams->GetLossMethod() != pgsTypes::TIME_STEP )
   {
      *pPara << color(Red) << _T("Time Step analysis results not available.") << color(Black) << rptNewLine;
      return pChapter;
   }

   const pgsPointOfInterest& rptPoi(pTSDRptSpec->GetPointOfInterest());
   const CGirderKey& girderKey(rptPoi.GetSegmentKey());
   std::vector<pgsPointOfInterest> vPoi;
   if ( pTSDRptSpec->ReportAtAllLocations() )
   {
      GET_IFACE2(pBroker,IPointOfInterest,pPoi);
      vPoi = pPoi->GetPointsOfInterest(CSegmentKey(ALL_GROUPS,girderKey.girderIndex,ALL_SEGMENTS));
   }
   else
   {
      vPoi.push_back(rptPoi);
   }

   IntervalIndexType rptIntervalIdx = pTSDRptSpec->GetInterval();

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker,ILosses,pLosses);
   GET_IFACE2(pBroker,IIntervals,pIntervals);

   INIT_UV_PROTOTYPE(rptPointOfInterest,    location,   pDisplayUnits->GetSpanLengthUnit(),      true);

   std::_tstring strImagePath(pgsReportStyleHolder::GetImagePath());

   location.IncludeSpanAndGirder(true);


   // reporting for a specific poi... list poi at top of report
   if ( !pTSDRptSpec->ReportAtAllLocations() )
   {
      pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
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
      pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      *pChapter << pPara;

      CString str;
      str.Format(_T("Interval %d : %s"),LABEL_INTERVAL(rptIntervalIdx),pIntervals->GetDescription(rptIntervalIdx));
      *pPara << str << rptNewLine;
      pPara->SetName(str);
   }

   IntervalIndexType nIntervals = pIntervals->GetIntervalCount();
   IntervalIndexType firstIntervalIdx = (rptIntervalIdx == INVALID_INDEX ? 0 : rptIntervalIdx);
   IntervalIndexType lastIntervalIdx  = (rptIntervalIdx == INVALID_INDEX ? nIntervals-1 : rptIntervalIdx);
   for ( IntervalIndexType intervalIdx = firstIntervalIdx; intervalIdx <= lastIntervalIdx; intervalIdx++ )
   {
      CGirderKey prevGirderKey;
      BOOST_FOREACH(const pgsPointOfInterest& poi,vPoi)
      {
         if ( pTSDRptSpec->ReportAtAllLocations() )
         {
            pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
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
            pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
            *pChapter << pPara;

            CString str;
            str.Format(_T("Interval %d : %s"),LABEL_INTERVAL(intervalIdx),pIntervals->GetDescription(intervalIdx));
            *pPara << str << rptNewLine;
            pPara->SetName(str);
         }

         std::vector<pgsTypes::ProductForceType> vLoads;
         if ( !prevGirderKey.IsEqual(CGirderKey(poi.GetSegmentKey())) )
         {
            // we only want to do this when the girder changes
            vLoads = GetProductForces(pBroker,intervalIdx,poi.GetSegmentKey());
         }

         const LOSSDETAILS* pDetails = pLosses->GetLossDetails(poi,intervalIdx);
         const TIME_STEP_DETAILS& tsDetails(pDetails->TimeStepDetails[intervalIdx]);

         // Interval Time Parameters
         pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
         (*pChapter) << pPara;
         (*pPara) << _T("Interval Details") << rptNewLine;
         pPara = new rptParagraph;
         *pChapter << pPara;
         rptRcTable* pIntervalTable = BuildIntervalTable(tsDetails,pIntervals,pDisplayUnits);
         *pPara << pIntervalTable << rptNewLine;
         *pPara << rptNewLine;

         // Properties
         pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
         (*pChapter) << pPara;
         (*pPara) << _T("Component Net Section Properties") << rptNewLine;
         pPara = new rptParagraph;
         (*pChapter) << pPara;
         rptRcTable* pComponentPropertiesTable = BuildComponentPropertiesTable(tsDetails,pDisplayUnits);
         *pPara << pComponentPropertiesTable << rptNewLine;
         *pPara << Sub2(_T("Y"),_T("k")) << _T(" is measured positive upwards from the top of girder.") << rptNewLine;
         *pPara << rptNewLine;

         pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
         (*pChapter) << pPara;
         (*pPara) << _T("Composite Transformed Section Properties") << rptNewLine;
         pPara = new rptParagraph;
         (*pChapter) << pPara;
         (*pPara) << rptRcImage(strImagePath + _T("TransformedProperties.png")) << rptNewLine;
         rptRcTable* pSectionPropertiesTable = BuildSectionPropertiesTable(tsDetails,pDisplayUnits);
         *pPara << pSectionPropertiesTable << rptNewLine;
         *pPara << Sub2(_T("Y"),_T("tr")) << _T(" is measured positive upwards from the top of girder (at the girder/deck interface).") << rptNewLine;
         *pPara << rptNewLine;

         // Unrestrained creep deformation in concrete parts due to loads applied in previous intervals
         pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
         (*pChapter) << pPara;
         (*pPara) << _T("Unrestrained creep deformation of concrete components due to loads applied in previous intervals") << rptNewLine;
         pPara = new rptParagraph;
         (*pChapter) << pPara;
         (*pPara) << rptRcImage(strImagePath + _T("FreeCreep_Axial.png")) << rptNewLine;
         (*pPara) << rptRcImage(strImagePath + _T("FreeCreep_Curvature.png")) << rptNewLine;
         rptRcTable* pFreeCreepTable = BuildFreeCreepDeformationTable(tsDetails,pDisplayUnits);
         *pPara << pFreeCreepTable << rptNewLine;
         *pPara << rptNewLine;

         // Unrestrained shrinkage during this interval
         pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
         (*pChapter) << pPara;
         (*pPara) << _T("Unrestrained shrinkage deformation of concrete components") << rptNewLine;
         pPara = new rptParagraph;
         (*pChapter) << pPara;
         *pPara << _T("Girder: ") << DELTA_E << Sub(_T("sh")) << _T(" = ") << tsDetails.Girder.esi << rptNewLine;
         *pPara << _T("Deck: ")   << DELTA_E << Sub(_T("sh")) << _T(" = ") << tsDetails.Deck.esi   << rptNewLine;
         *pPara << rptNewLine;

         // Unrestrained strand relaxation
         pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
         (*pChapter) << pPara;
         (*pPara) << _T("Apparent unrestrained deformation of strands due to relaxation") << rptNewLine;
         pPara = new rptParagraph;
         (*pChapter) << pPara;
         (*pPara) << rptRcImage(strImagePath + _T("ApparentRelaxationStrain.png")) << rptNewLine;
         rptRcTable* pStrandRelaxationTable = BuildStrandRelaxationTable(tsDetails,pDisplayUnits);
         (*pPara) << pStrandRelaxationTable << rptNewLine;
         (*pPara) << rptNewLine;

         // Unrestrained tendon relaxation
         pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
         (*pChapter) << pPara;
         (*pPara) << _T("Apparent unrestrained deformation of tendons due to relaxation") << rptNewLine;
         pPara = new rptParagraph;
         (*pChapter) << pPara;
         (*pPara) << rptRcImage(strImagePath + _T("ApparentRelaxationStrain.png")) << rptNewLine;
         rptRcTable* pTendonRelaxationTable = BuildTendonRelaxationTable(tsDetails,pDisplayUnits);
         (*pPara) << pTendonRelaxationTable << rptNewLine;
         (*pPara) << rptNewLine;

         // Forces required to fully restrain each component
         pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
         (*pChapter) << pPara;
         (*pPara) << _T("Component Restraining Forces") << rptNewLine;
         pPara = new rptParagraph;
         (*pChapter) << pPara;
         (*pPara) << rptRcImage(strImagePath + _T("IncrementalRestrainingForce_Axial.png")) << rptNewLine;
         (*pPara) << rptRcImage(strImagePath + _T("IncrementalRestrainingForce_Moment.png")) << rptNewLine;
         rptRcTable* pComponentRestrainingForcesTable = BuildComponentRestrainingForceTable(tsDetails,pDisplayUnits);
         (*pPara) << pComponentRestrainingForcesTable << rptNewLine;
         (*pPara) << rptNewLine;


         // Forces required to fully restrain each component
         pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
         (*pChapter) << pPara;
         (*pPara) << _T("Section Restraining Forces") << rptNewLine;
         pPara = new rptParagraph;
         (*pChapter) << pPara;
         (*pPara) << rptRcImage(strImagePath + _T("TotalRestrainingForce_Axial.png")) << rptNewLine;
         (*pPara) << rptRcImage(strImagePath + _T("TotalRestrainingForce_Moment.png")) << rptNewLine;
         rptRcTable* pSectionRestrainingForcesTable = BuildSectionRestrainingForceTable(tsDetails,pDisplayUnits);
         (*pPara) << pSectionRestrainingForcesTable << rptNewLine;
         (*pPara) << rptNewLine;

         // Deformations required to fully restrain each component
         pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
         (*pChapter) << pPara;
         (*pPara) << _T("Section Restraining Deformations") << rptNewLine;
         pPara = new rptParagraph;
         (*pChapter) << pPara;
         (*pPara) << rptRcImage(strImagePath + _T("TotalRestrainingDeformation_Axial.png")) << rptNewLine;
         (*pPara) << rptRcImage(strImagePath + _T("TotalRestrainingDeformation_Curvature.png")) << rptNewLine;
         rptRcTable* pSectionRestrainingDeformationTable = BuildSectionRestrainingDeformationTable(tsDetails,pDisplayUnits);
         (*pPara) << pSectionRestrainingDeformationTable << rptNewLine;
         (*pPara) << _T("Section restraining deformations are computed at multiple sections along the girder. It is assumed that deformations vary linerally between sections. The girder is analyzed for these deformations. The resulting section forces are listed in the Restrained Section Forces table below.") << rptNewLine;
         (*pPara) << rptNewLine;

         // Forces due to the external restraints of the strutural system
         pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
         (*pChapter) << pPara;
         (*pPara) << _T("Restrained Section Forces") << rptNewLine;
         pPara = new rptParagraph;
         (*pChapter) << pPara;
         rptRcTable* pRestrainedSectionForcesTable = BuildRestrainedSectionForceTable(tsDetails,pDisplayUnits);
         (*pPara) << pRestrainedSectionForcesTable << rptNewLine;
         (*pPara) << _T("The restrained section forces are the secondary forces due to the structural system restraining the deformations due to creep, shrinkage, and relaxation.") << rptNewLine;
         *pPara << rptNewLine;

         // Restrained component forces
         pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
         (*pChapter) << pPara;
         (*pPara) << _T("Restrained Component Forces") << rptNewLine;
         pPara = new rptParagraph;
         (*pChapter) << pPara;
         (*pPara) << rptRcImage(strImagePath + _T("RestrainedComponentForces_Axial.png")) << rptNewLine;
         (*pPara) << rptRcImage(strImagePath + _T("RestrainedComponentForces_Moment.png")) << rptNewLine;
         rptRcTable* pRestrainedComponentForcesTable = BuildRestrainedComponentForceTable(tsDetails,pDisplayUnits);
         (*pPara) << pRestrainedComponentForcesTable << rptNewLine;
         *pPara << rptNewLine;

         // Incremental Forces due to loads applied during this interval
         pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
         (*pChapter) << pPara;
         (*pPara) << _T("Increment forces due to external loads and restrained component forces during this interval.") << rptNewLine;

         pPara = new rptParagraph;
         (*pChapter) << pPara;
         (*pPara) << rptRcImage(strImagePath + _T("IncrementalForce_Axial.png")) << rptNewLine;
         (*pPara) << rptRcImage(strImagePath + _T("IncrementalForce_Moment.png")) << rptNewLine;

         rptRcTable* pIncForcesTable = BuildIncrementalForceTable(pBroker,vLoads,tsDetails,pDisplayUnits);
         (*pPara) << pIncForcesTable << rptNewLine;
         *pPara << rptNewLine;

         // Incremental stresses due to loads applied during this interval
         pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
         (*pChapter) << pPara;
         (*pPara) << _T("Increment component stress") << rptNewLine;
         pPara = new rptParagraph;
         (*pChapter) << pPara;
         (*pPara) << rptRcImage(strImagePath + _T("IncrementalComponentStress.png")) << rptNewLine;
         rptRcTable* pIncStressTable = BuildIncrementalStressTable(pBroker,vLoads,tsDetails,pDisplayUnits);
         (*pPara) << pIncStressTable << rptNewLine;
         (*pPara) << _T("Change in stress in strands and tendons are the prestress losses during this interval.") << rptNewLine;
         *pPara << rptNewLine;

         prevGirderKey = CGirderKey(poi.GetSegmentKey());

         // start a new page for the next poi
         *pPara << rptNewPage;
      } // next POI
   } // next interval

   return pChapter;
}

CChapterBuilder* CTimeStepDetailsChapterBuilder::Clone() const
{
   return new CTimeStepDetailsChapterBuilder;
}

rptRcTable* CTimeStepDetailsChapterBuilder::BuildIntervalTable(const TIME_STEP_DETAILS& tsDetails,IIntervals* pIntervals,IEAFDisplayUnits* pDisplayUnits) const
{
   rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(4);
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

rptRcTable* CTimeStepDetailsChapterBuilder::BuildComponentPropertiesTable(const TIME_STEP_DETAILS& tsDetails,IEAFDisplayUnits* pDisplayUnits) const
{
   INIT_UV_PROTOTYPE(rptLengthUnitValue,    ecc,        pDisplayUnits->GetComponentDimUnit(),    false);
   INIT_UV_PROTOTYPE(rptLengthUnitValue,    height,     pDisplayUnits->GetComponentDimUnit(),    false);
   INIT_UV_PROTOTYPE(rptLength2UnitValue,   area,       pDisplayUnits->GetAreaUnit(),            false);
   INIT_UV_PROTOTYPE(rptLength4UnitValue,   momI,       pDisplayUnits->GetMomentOfInertiaUnit(), false);
   INIT_UV_PROTOTYPE(rptStressUnitValue,    modE,       pDisplayUnits->GetModEUnit(),            false);

   rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(6);
   pTable->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CJ_LEFT));
   pTable->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CJ_LEFT));

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
   (*pTable)(rowIdx,colIdx++) << modE.SetValue(tsDetails.Girder.E);
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

   rowIdx++;
   colIdx = 0;
   (*pTable)(rowIdx,colIdx++) << _T("Deck");
   (*pTable)(rowIdx,colIdx++) << modE.SetValue(tsDetails.Deck.E);
   (*pTable)(rowIdx,colIdx++) << area.SetValue(tsDetails.Deck.An);
   (*pTable)(rowIdx,colIdx++) << momI.SetValue(tsDetails.Deck.In);
   (*pTable)(rowIdx,colIdx++) << ecc.SetValue(tsDetails.Deck.Yn);
   (*pTable)(rowIdx,colIdx++) << height.SetValue(tsDetails.Deck.H);

   for ( int i = 0; i < 2; i++ )
   {
      pgsTypes::DeckRebarMatType matType = (pgsTypes::DeckRebarMatType)i;
      for ( int j = 0; j < 2; j++ )
      {
         rowIdx++;
         colIdx = 0;
         pgsTypes::DeckRebarBarType barType = (pgsTypes::DeckRebarBarType)j;
         if ( matType == pgsTypes::drmTop )
         {
            if ( barType == pgsTypes::drbIndividual )
            {
               (*pTable)(rowIdx,colIdx++) << _T("Deck Top Mat Individual Rebar");
            }
            else
            {
               (*pTable)(rowIdx,colIdx++) << _T("Deck Top Mat Lump Sum Rebar");
            }
         }
         else
         {
            if ( barType == pgsTypes::drbIndividual )
            {
               (*pTable)(rowIdx,colIdx++) << _T("Deck Bottom Mat Individual Rebar");
            }
            else
            {
               (*pTable)(rowIdx,colIdx++) << _T("Deck Bottom Mat Lump Sum Rebar");
            }
         }

         (*pTable)(rowIdx,colIdx++) << modE.SetValue(tsDetails.DeckRebar[matType][barType].E);
         (*pTable)(rowIdx,colIdx++) << area.SetValue(tsDetails.DeckRebar[matType][barType].As);
         (*pTable)(rowIdx,colIdx++) << _T("-");
         (*pTable)(rowIdx,colIdx++) << ecc.SetValue(tsDetails.DeckRebar[matType][barType].Ys);
         (*pTable)(rowIdx,colIdx++) << _T("-");
      }
   }

   DuctIndexType nTendons = tsDetails.Tendons.size();
   for ( DuctIndexType tendonIdx = 0; tendonIdx < nTendons; tendonIdx++ )
   {
      const TIME_STEP_STRAND& tsTendon = tsDetails.Tendons[tendonIdx];

      rowIdx++;
      colIdx = 0;
      (*pTable)(rowIdx,colIdx++) << _T("Tendon ") << LABEL_DUCT(tendonIdx);
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

   rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(6);
   pTable->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CJ_LEFT));
   pTable->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CJ_LEFT));

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
   (*pTable)(rowIdx,colIdx++) << modE.SetValue(tsDetails.Girder.E);
   (*pTable)(rowIdx,colIdx++) << area.SetValue(tsDetails.Atr);
   (*pTable)(rowIdx,colIdx++) << momI.SetValue(tsDetails.Itr);
   (*pTable)(rowIdx,colIdx++) << ecc.SetValue(tsDetails.Ytr);
   (*pTable)(rowIdx,colIdx++) << height.SetValue(tsDetails.Girder.H + tsDetails.Deck.H);

   return pTable;
}

rptRcTable* CTimeStepDetailsChapterBuilder::BuildFreeCreepDeformationTable(const TIME_STEP_DETAILS& tsDetails,IEAFDisplayUnits* pDisplayUnits) const
{
   INIT_UV_PROTOTYPE(rptLength2UnitValue,   area,       pDisplayUnits->GetAreaUnit(),            true);
   INIT_UV_PROTOTYPE(rptLength4UnitValue,   momI,       pDisplayUnits->GetMomentOfInertiaUnit(), true);
   INIT_UV_PROTOTYPE(rptStressUnitValue,    modE,       pDisplayUnits->GetModEUnit(),            true);
   INIT_UV_PROTOTYPE(rptForceUnitValue,     force,      pDisplayUnits->GetGeneralForceUnit(),    true);
   INIT_UV_PROTOTYPE(rptMomentUnitValue,    moment,     pDisplayUnits->GetSmallMomentUnit(),     true);
   INIT_UV_PROTOTYPE(rptPerLengthUnitValue, curvature,  pDisplayUnits->GetCurvatureUnit(),       false);

   rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(5);
   pTable->SetNumberOfHeaderRows(2);

   RowIndexType rowIdx = 0;
   ColumnIndexType colIdx = 0;
   pTable->SetRowSpan(rowIdx,colIdx,2);
   pTable->SetRowSpan(rowIdx+1,colIdx,SKIP_CELL);
   (*pTable)(rowIdx,colIdx++) << _T("Loading") << rptNewLine << _T("Interval");

   pTable->SetColumnSpan(rowIdx,colIdx,2);
   (*pTable)(rowIdx,colIdx++) << _T("Girder");
   pTable->SetColumnSpan(rowIdx,colIdx++,SKIP_CELL);

   pTable->SetColumnSpan(rowIdx,colIdx,2);
   (*pTable)(rowIdx,colIdx++) << _T("Deck");
   pTable->SetColumnSpan(rowIdx,colIdx++,SKIP_CELL);

   rowIdx++;
   colIdx = 1;
   (*pTable)(rowIdx,colIdx++) << DELTA_E;
   (*pTable)(rowIdx,colIdx++) << COLHDR(DELTA_R,rptPerLengthUnitTag,pDisplayUnits->GetCurvatureUnit());
   (*pTable)(rowIdx,colIdx++) << DELTA_E;
   (*pTable)(rowIdx,colIdx++) << COLHDR(DELTA_R,rptPerLengthUnitTag,pDisplayUnits->GetCurvatureUnit());

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
      (*pTable)(rowIdx,colIdx++) << _T("(") << force.SetValue(girder_creep_strain.P) << _T("/(") << area.SetValue(girder_creep_strain.A) << _T(")(") << modE.SetValue(girder_creep_strain.E) << _T("))(") << girder_creep_strain.Ce << _T(" - ") << girder_creep_strain.Cs << _T(")=") << girder_creep_strain.e;
      (*pTable)(rowIdx,colIdx++) << _T("(") << moment.SetValue(girder_creep_curvature.M) << _T("/(") << momI.SetValue(girder_creep_curvature.I) << _T(")(") << modE.SetValue(girder_creep_curvature.E) << _T("))(") << girder_creep_curvature.Ce << _T(" - ") << girder_creep_curvature.Cs << _T(")=") << curvature.SetValue(girder_creep_curvature.r);
      (*pTable)(rowIdx,colIdx++) << _T("(") << force.SetValue(deck_creep_strain.P) << _T("/(") << area.SetValue(deck_creep_strain.A) << _T(")(") << modE.SetValue(deck_creep_strain.E) << _T("))(") << deck_creep_strain.Ce << _T(" - ") << deck_creep_strain.Cs << _T(")=") << deck_creep_strain.e;
      (*pTable)(rowIdx,colIdx++) << _T("(") << moment.SetValue(deck_creep_curvature.M) << _T("/(") << momI.SetValue(deck_creep_curvature.I) << _T(")(") << modE.SetValue(deck_creep_curvature.E) << _T("))(") << deck_creep_curvature.Ce << _T(" - ") << deck_creep_curvature.Cs << _T(")=") << curvature.SetValue(deck_creep_curvature.r);
   }

   colIdx = 0;
   (*pTable)(rowIdx,colIdx++) << _T("Total");
   (*pTable)(rowIdx,colIdx++) << tsDetails.Girder.eci;
   (*pTable)(rowIdx,colIdx++) << curvature.SetValue(tsDetails.Girder.rci);
   (*pTable)(rowIdx,colIdx++) << tsDetails.Deck.eci;
   (*pTable)(rowIdx,colIdx++) << curvature.SetValue(tsDetails.Deck.rci);

   return pTable;
}

rptRcTable* CTimeStepDetailsChapterBuilder::BuildStrandRelaxationTable(const TIME_STEP_DETAILS& tsDetails,IEAFDisplayUnits* pDisplayUnits) const
{
   INIT_UV_PROTOTYPE(rptStressUnitValue,    stress,     pDisplayUnits->GetStressUnit(),          false);

   rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(3);
   RowIndexType rowIdx = 0;
   ColumnIndexType colIdx = 0;

   (*pTable)(rowIdx,colIdx++) << _T("Strand");
   (*pTable)(rowIdx,colIdx++) << COLHDR(DELTA_FR,rptStressUnitTag,pDisplayUnits->GetStressUnit());
   (*pTable)(rowIdx,colIdx++) << DELTA_ER;

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

      (*pTable)(rowIdx,colIdx++) << stress.SetValue(tsDetails.Strands[strandType].fr);
      (*pTable)(rowIdx,colIdx++) << tsDetails.Strands[strandType].er;
   }

   return pTable;
}

rptRcTable* CTimeStepDetailsChapterBuilder::BuildTendonRelaxationTable(const TIME_STEP_DETAILS& tsDetails,IEAFDisplayUnits* pDisplayUnits) const
{
   INIT_UV_PROTOTYPE(rptStressUnitValue,    stress,     pDisplayUnits->GetStressUnit(),          false);

   rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(3);
   RowIndexType rowIdx = 0;
   ColumnIndexType colIdx = 0;

   (*pTable)(rowIdx,colIdx++) << _T("Tendon");
   (*pTable)(rowIdx,colIdx++) << COLHDR(DELTA_FR,rptStressUnitTag,pDisplayUnits->GetStressUnit());
   (*pTable)(rowIdx,colIdx++) << DELTA_ER;

   rowIdx = pTable->GetNumberOfHeaderRows();
   DuctIndexType nTendons = tsDetails.Tendons.size();
   for ( DuctIndexType tendonIdx = 0; tendonIdx < nTendons; tendonIdx++, rowIdx++ )
   {
      colIdx = 0;
      (*pTable)(rowIdx,colIdx++) << LABEL_DUCT(tendonIdx);
      (*pTable)(rowIdx,colIdx++) << stress.SetValue(tsDetails.Tendons[tendonIdx].fr);
      (*pTable)(rowIdx,colIdx++) << tsDetails.Tendons[tendonIdx].er;
   }

   return pTable;
}

rptRcTable* CTimeStepDetailsChapterBuilder::BuildComponentRestrainingForceTable(const TIME_STEP_DETAILS& tsDetails,IEAFDisplayUnits* pDisplayUnits) const
{
   INIT_UV_PROTOTYPE(rptStressUnitValue,    stress,     pDisplayUnits->GetStressUnit(),          true);
   INIT_UV_PROTOTYPE(rptLength2UnitValue,   area,       pDisplayUnits->GetAreaUnit(),            true);
   INIT_UV_PROTOTYPE(rptLength4UnitValue,   momI,       pDisplayUnits->GetMomentOfInertiaUnit(), true);
   INIT_UV_PROTOTYPE(rptStressUnitValue,    modE,       pDisplayUnits->GetModEUnit(),            true);
   INIT_UV_PROTOTYPE(rptForceUnitValue,     force,      pDisplayUnits->GetGeneralForceUnit(),    true);
   INIT_UV_PROTOTYPE(rptMomentUnitValue,    moment,     pDisplayUnits->GetSmallMomentUnit(),     true);
   INIT_UV_PROTOTYPE(rptPerLengthUnitValue, curvature,  pDisplayUnits->GetCurvatureUnit(),       true);

   rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(7);
   RowIndexType rowIdx = 0;
   ColumnIndexType colIdx = 0;

   pTable->SetNumberOfHeaderRows(2);
   pTable->SetRowSpan(rowIdx,colIdx,2);
   pTable->SetRowSpan(rowIdx+1,colIdx,SKIP_CELL);
   (*pTable)(rowIdx,colIdx++) << _T("Component");

   pTable->SetColumnSpan(rowIdx,colIdx,2);
   (*pTable)(rowIdx,colIdx++) << _T("Creep");
   pTable->SetColumnSpan(rowIdx,colIdx++,SKIP_CELL);

   pTable->SetColumnSpan(rowIdx,colIdx,2);
   (*pTable)(rowIdx,colIdx++) << _T("Shrinkage");
   pTable->SetColumnSpan(rowIdx,colIdx++,SKIP_CELL);

   pTable->SetColumnSpan(rowIdx,colIdx,2);
   (*pTable)(rowIdx,colIdx++) << _T("Relaxation");
   pTable->SetColumnSpan(rowIdx,colIdx++,SKIP_CELL);
   
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
   (*pTable)(rowIdx,colIdx++) << _T("-(") << tsDetails.Girder.eci << _T(")(") << modE.SetValue(tsDetails.Girder.E) << _T(")(") << area.SetValue(tsDetails.Girder.An) << _T(") = ") << force.SetValue(tsDetails.Girder.PrCreep);
   (*pTable)(rowIdx,colIdx++) << _T("-(") << curvature.SetValue(tsDetails.Girder.rci) << _T(")(") << modE.SetValue(tsDetails.Girder.E) << _T(")(") << momI.SetValue(tsDetails.Girder.In) << _T(") = ") << moment.SetValue(tsDetails.Girder.MrCreep);
   (*pTable)(rowIdx,colIdx++) << _T("-(") << tsDetails.Girder.esi << _T(")(") << modE.SetValue(tsDetails.Girder.E) << _T(")(") << area.SetValue(tsDetails.Girder.An) << _T(") = ") << force.SetValue(tsDetails.Girder.PrShrinkage);
   (*pTable)(rowIdx,colIdx++) << _T("");
   (*pTable)(rowIdx,colIdx++) << _T("");
   (*pTable)(rowIdx,colIdx++) << _T("");

   rowIdx++;
   colIdx = 0;
   (*pTable)(rowIdx,colIdx++) << _T("Deck");
   (*pTable)(rowIdx,colIdx++) << _T("-(") << tsDetails.Deck.eci << _T(")(") << modE.SetValue(tsDetails.Deck.E) << _T(")(") << area.SetValue(tsDetails.Deck.An) << _T(") = ") << force.SetValue(tsDetails.Deck.PrCreep);
   (*pTable)(rowIdx,colIdx++) << _T("-(") << curvature.SetValue(tsDetails.Deck.rci) << _T(")(") << modE.SetValue(tsDetails.Deck.E) << _T(")(") << momI.SetValue(tsDetails.Deck.In) << _T(") = ") << moment.SetValue(tsDetails.Deck.MrCreep);
   (*pTable)(rowIdx,colIdx++) << _T("-(") << tsDetails.Deck.esi << _T(")(") << modE.SetValue(tsDetails.Deck.E) << _T(")(") << area.SetValue(tsDetails.Deck.An) << _T(") = ") << force.SetValue(tsDetails.Deck.PrShrinkage);
   (*pTable)(rowIdx,colIdx++) << _T("");
   (*pTable)(rowIdx,colIdx++) << _T("");
   (*pTable)(rowIdx,colIdx++) << _T("");

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
      (*pTable)(rowIdx,colIdx++) << _T("-(") << tsDetails.Strands[strandType].er << _T(")(") << modE.SetValue(tsDetails.Strands[strandType].E) << _T(")(") << area.SetValue(tsDetails.Strands[strandType].As) << _T(") = ") << force.SetValue(tsDetails.Strands[strandType].PrRelaxation);
      (*pTable)(rowIdx,colIdx++) << _T("");
   }

   DuctIndexType nTendons = tsDetails.Tendons.size();
   for ( DuctIndexType tendonIdx = 0; tendonIdx < nTendons; tendonIdx++ )
   {
      rowIdx++;
      colIdx = 0;
      (*pTable)(rowIdx,colIdx++) << _T("Tendon ") << LABEL_DUCT(tendonIdx);
      (*pTable)(rowIdx,colIdx++) << _T("");
      (*pTable)(rowIdx,colIdx++) << _T("");
      (*pTable)(rowIdx,colIdx++) << _T("");
      (*pTable)(rowIdx,colIdx++) << _T("");
      (*pTable)(rowIdx,colIdx++) << _T("-(") << tsDetails.Tendons[tendonIdx].er << _T(")(") << modE.SetValue(tsDetails.Tendons[tendonIdx].E) << _T(")(") << area.SetValue(tsDetails.Tendons[tendonIdx].As) << _T(") = ") << force.SetValue(tsDetails.Tendons[tendonIdx].PrRelaxation);
      (*pTable)(rowIdx,colIdx++) << _T("");
   }

   return pTable;
}

rptRcTable* CTimeStepDetailsChapterBuilder::BuildSectionRestrainingForceTable(const TIME_STEP_DETAILS& tsDetails,IEAFDisplayUnits* pDisplayUnits) const
{
   INIT_UV_PROTOTYPE(rptForceUnitValue,     force,      pDisplayUnits->GetGeneralForceUnit(),    true);
   INIT_UV_PROTOTYPE(rptMomentUnitValue,    moment,     pDisplayUnits->GetSmallMomentUnit(),     true);

   rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(7);
   RowIndexType rowIdx = 0;
   ColumnIndexType colIdx = 0;

   pTable->SetNumberOfHeaderRows(2);
   pTable->SetRowSpan(rowIdx,colIdx,2);
   pTable->SetRowSpan(rowIdx+1,colIdx,SKIP_CELL);
   (*pTable)(rowIdx,colIdx++) << _T("Component");

   pTable->SetColumnSpan(rowIdx,colIdx,2);
   (*pTable)(rowIdx,colIdx++) << _T("Creep");
   pTable->SetColumnSpan(rowIdx,colIdx++,SKIP_CELL);

   pTable->SetColumnSpan(rowIdx,colIdx,2);
   (*pTable)(rowIdx,colIdx++) << _T("Shrinkage");
   pTable->SetColumnSpan(rowIdx,colIdx++,SKIP_CELL);

   pTable->SetColumnSpan(rowIdx,colIdx,2);
   (*pTable)(rowIdx,colIdx++) << _T("Relaxation");
   pTable->SetColumnSpan(rowIdx,colIdx++,SKIP_CELL);
   
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
   INIT_UV_PROTOTYPE(rptPerLengthUnitValue, curvature,  pDisplayUnits->GetCurvatureUnit(),       true);

   rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(7);
   RowIndexType rowIdx = 0;
   ColumnIndexType colIdx = 0;

   pTable->SetNumberOfHeaderRows(2);
   pTable->SetRowSpan(rowIdx,colIdx,2);
   pTable->SetRowSpan(rowIdx+1,colIdx,SKIP_CELL);
   (*pTable)(rowIdx,colIdx++) << _T("Component");

   pTable->SetColumnSpan(rowIdx,colIdx,2);
   (*pTable)(rowIdx,colIdx++) << _T("Creep");
   pTable->SetColumnSpan(rowIdx,colIdx++,SKIP_CELL);

   pTable->SetColumnSpan(rowIdx,colIdx,2);
   (*pTable)(rowIdx,colIdx++) << _T("Shrinkage");
   pTable->SetColumnSpan(rowIdx,colIdx++,SKIP_CELL);

   pTable->SetColumnSpan(rowIdx,colIdx,2);
   (*pTable)(rowIdx,colIdx++) << _T("Relaxation");
   pTable->SetColumnSpan(rowIdx,colIdx++,SKIP_CELL);
   
   rowIdx++;
   colIdx = 1;
   (*pTable)(rowIdx,colIdx++) << Sub2(symbol(epsilon),_T("r"));
   (*pTable)(rowIdx,colIdx++) << COLHDR(Sub2(symbol(varphi),_T("r")),rptPerLengthUnitTag,pDisplayUnits->GetCurvatureUnit());
   (*pTable)(rowIdx,colIdx++) << Sub2(symbol(epsilon),_T("r"));
   (*pTable)(rowIdx,colIdx++) << COLHDR(Sub2(symbol(varphi),_T("r")),rptPerLengthUnitTag,pDisplayUnits->GetCurvatureUnit());
   (*pTable)(rowIdx,colIdx++) << Sub2(symbol(epsilon),_T("r"));
   (*pTable)(rowIdx,colIdx++) << COLHDR(Sub2(symbol(varphi),_T("r")),rptPerLengthUnitTag,pDisplayUnits->GetCurvatureUnit());

   rowIdx = pTable->GetNumberOfHeaderRows();
   colIdx = 0;
   (*pTable)(rowIdx,colIdx++) << _T("Composite Section");
   (*pTable)(rowIdx,colIdx++) << tsDetails.e[TIMESTEP_CR];
   (*pTable)(rowIdx,colIdx++) << curvature.SetValue(tsDetails.r[TIMESTEP_CR]);
   (*pTable)(rowIdx,colIdx++) << tsDetails.e[TIMESTEP_SH];
   (*pTable)(rowIdx,colIdx++) << curvature.SetValue(tsDetails.r[TIMESTEP_SH]);
   (*pTable)(rowIdx,colIdx++) << tsDetails.e[TIMESTEP_RE];
   (*pTable)(rowIdx,colIdx++) << curvature.SetValue(tsDetails.r[TIMESTEP_RE]);

   return pTable;
}

rptRcTable* CTimeStepDetailsChapterBuilder::BuildRestrainedSectionForceTable(const TIME_STEP_DETAILS& tsDetails,IEAFDisplayUnits* pDisplayUnits) const
{
   INIT_UV_PROTOTYPE(rptForceUnitValue,     force,      pDisplayUnits->GetGeneralForceUnit(),    false);
   INIT_UV_PROTOTYPE(rptMomentUnitValue,    moment,     pDisplayUnits->GetSmallMomentUnit(),     false);

   rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(7);
   RowIndexType rowIdx = 0;
   ColumnIndexType colIdx = 0;

   pTable->SetNumberOfHeaderRows(2);
   pTable->SetRowSpan(rowIdx,colIdx,2);
   pTable->SetRowSpan(rowIdx+1,colIdx,SKIP_CELL);
   (*pTable)(rowIdx,colIdx++) << _T("Component");

   pTable->SetColumnSpan(rowIdx,colIdx,2);
   (*pTable)(rowIdx,colIdx++) << _T("Creep");
   pTable->SetColumnSpan(rowIdx,colIdx++,SKIP_CELL);

   pTable->SetColumnSpan(rowIdx,colIdx,2);
   (*pTable)(rowIdx,colIdx++) << _T("Shrinkage");
   pTable->SetColumnSpan(rowIdx,colIdx++,SKIP_CELL);

   pTable->SetColumnSpan(rowIdx,colIdx,2);
   (*pTable)(rowIdx,colIdx++) << _T("Relaxation");
   pTable->SetColumnSpan(rowIdx,colIdx++,SKIP_CELL);
   
   rowIdx++;
   colIdx = 1;
   (*pTable)(rowIdx,colIdx++) << COLHDR(Overline(_T("P")) << Sub(_T("r")),rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());
   (*pTable)(rowIdx,colIdx++) << COLHDR(Overline(_T("M")) << Sub(_T("r")),rptMomentUnitTag,pDisplayUnits->GetSmallMomentUnit());
   (*pTable)(rowIdx,colIdx++) << COLHDR(Overline(_T("P")) << Sub(_T("r")),rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());
   (*pTable)(rowIdx,colIdx++) << COLHDR(Overline(_T("M")) << Sub(_T("r")),rptMomentUnitTag,pDisplayUnits->GetSmallMomentUnit());
   (*pTable)(rowIdx,colIdx++) << COLHDR(Overline(_T("P")) << Sub(_T("r")),rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());
   (*pTable)(rowIdx,colIdx++) << COLHDR(Overline(_T("M")) << Sub(_T("r")),rptMomentUnitTag,pDisplayUnits->GetSmallMomentUnit());


   rowIdx = pTable->GetNumberOfHeaderRows();
   colIdx = 0;
   (*pTable)(rowIdx,colIdx++) << _T("Composite Section");
   (*pTable)(rowIdx,colIdx++) << force.SetValue(tsDetails.dPi[pgsTypes::pftCreep]);
   (*pTable)(rowIdx,colIdx++) << moment.SetValue(tsDetails.dMi[pgsTypes::pftCreep]);
   (*pTable)(rowIdx,colIdx++) << force.SetValue(tsDetails.dPi[pgsTypes::pftShrinkage]);
   (*pTable)(rowIdx,colIdx++) << moment.SetValue(tsDetails.dPi[pgsTypes::pftShrinkage]);
   (*pTable)(rowIdx,colIdx++) << force.SetValue(tsDetails.dPi[pgsTypes::pftRelaxation]);
   (*pTable)(rowIdx,colIdx++) << moment.SetValue(tsDetails.dPi[pgsTypes::pftRelaxation]);

   return pTable;
}

rptRcTable* CTimeStepDetailsChapterBuilder::BuildRestrainedComponentForceTable(const TIME_STEP_DETAILS& tsDetails,IEAFDisplayUnits* pDisplayUnits) const
{
   INIT_UV_PROTOTYPE(rptForceUnitValue,     force,      pDisplayUnits->GetGeneralForceUnit(),    false);
   INIT_UV_PROTOTYPE(rptMomentUnitValue,    moment,     pDisplayUnits->GetSmallMomentUnit(),     false);

   rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(7);
   RowIndexType rowIdx = 0;
   ColumnIndexType colIdx = 0;

   pTable->SetNumberOfHeaderRows(2);
   pTable->SetRowSpan(rowIdx,colIdx,2);
   pTable->SetRowSpan(rowIdx+1,colIdx,SKIP_CELL);
   (*pTable)(rowIdx,colIdx++) << _T("Component");

   pTable->SetColumnSpan(rowIdx,colIdx,2);
   (*pTable)(rowIdx,colIdx++) << _T("Creep");
   pTable->SetColumnSpan(rowIdx,colIdx++,SKIP_CELL);

   pTable->SetColumnSpan(rowIdx,colIdx,2);
   (*pTable)(rowIdx,colIdx++) << _T("Shrinkage");
   pTable->SetColumnSpan(rowIdx,colIdx++,SKIP_CELL);

   pTable->SetColumnSpan(rowIdx,colIdx,2);
   (*pTable)(rowIdx,colIdx++) << _T("Relaxation");
   pTable->SetColumnSpan(rowIdx,colIdx++,SKIP_CELL);
   
   rowIdx++;
   colIdx = 1;
   (*pTable)(rowIdx,colIdx++) << COLHDR(Overline(symbol(DELTA) << _T("P")) << Sub(_T("r")),rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());
   (*pTable)(rowIdx,colIdx++) << COLHDR(Overline(symbol(DELTA) << _T("M")) << Sub(_T("r")),rptMomentUnitTag,pDisplayUnits->GetSmallMomentUnit());
   (*pTable)(rowIdx,colIdx++) << COLHDR(Overline(symbol(DELTA) << _T("P")) << Sub(_T("r")),rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());
   (*pTable)(rowIdx,colIdx++) << COLHDR(Overline(symbol(DELTA) << _T("M")) << Sub(_T("r")),rptMomentUnitTag,pDisplayUnits->GetSmallMomentUnit());
   (*pTable)(rowIdx,colIdx++) << COLHDR(Overline(symbol(DELTA) << _T("P")) << Sub(_T("r")),rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());
   (*pTable)(rowIdx,colIdx++) << COLHDR(Overline(symbol(DELTA) << _T("M")) << Sub(_T("r")),rptMomentUnitTag,pDisplayUnits->GetSmallMomentUnit());


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

   rowIdx++;
   colIdx = 0;
   (*pTable)(rowIdx,colIdx++) << _T("Deck");
   (*pTable)(rowIdx,colIdx++) << force.SetValue(tsDetails.Deck.dPi[pgsTypes::pftCreep]);
   (*pTable)(rowIdx,colIdx++) << moment.SetValue(tsDetails.Deck.dMi[pgsTypes::pftCreep]);
   (*pTable)(rowIdx,colIdx++) << force.SetValue(tsDetails.Deck.dPi[pgsTypes::pftShrinkage]);
   (*pTable)(rowIdx,colIdx++) << moment.SetValue(tsDetails.Deck.dMi[pgsTypes::pftShrinkage]);
   (*pTable)(rowIdx,colIdx++) << force.SetValue(tsDetails.Deck.dPi[pgsTypes::pftRelaxation]);
   (*pTable)(rowIdx,colIdx++) << moment.SetValue(tsDetails.Deck.dMi[pgsTypes::pftRelaxation]);

   for ( int i = 0; i < 2; i++ )
   {
      pgsTypes::DeckRebarMatType matType = (pgsTypes::DeckRebarMatType)i;
      for ( int j = 0; j < 2; j++ )
      {
         rowIdx++;
         colIdx = 0;
         pgsTypes::DeckRebarBarType barType = (pgsTypes::DeckRebarBarType)j;
         if ( matType == pgsTypes::drmTop )
         {
            if ( barType == pgsTypes::drbIndividual )
            {
               (*pTable)(rowIdx,colIdx++) << _T("Deck Top Mat Individual Rebar");
            }
            else
            {
               (*pTable)(rowIdx,colIdx++) << _T("Deck Top Mat Lump Sum Rebar");
            }
         }
         else
         {
            if ( barType == pgsTypes::drbIndividual )
            {
               (*pTable)(rowIdx,colIdx++) << _T("Deck Bottom Mat Individual Rebar");
            }
            else
            {
               (*pTable)(rowIdx,colIdx++) << _T("Deck Bottom Mat Lump Sum Rebar");
            }
         }

         (*pTable)(rowIdx,colIdx++) << force.SetValue(tsDetails.DeckRebar[matType][barType].dPi[pgsTypes::pftCreep]);
         (*pTable)(rowIdx,colIdx++) << _T("");
         (*pTable)(rowIdx,colIdx++) << force.SetValue(tsDetails.DeckRebar[matType][barType].dPi[pgsTypes::pftShrinkage]);
         (*pTable)(rowIdx,colIdx++) << _T("");
         (*pTable)(rowIdx,colIdx++) << force.SetValue(tsDetails.DeckRebar[matType][barType].dPi[pgsTypes::pftRelaxation]);
         (*pTable)(rowIdx,colIdx++) << _T("");
      }
   }

   DuctIndexType nTendons = tsDetails.Tendons.size();
   for ( DuctIndexType tendonIdx = 0; tendonIdx < nTendons; tendonIdx++ )
   {
      rowIdx++;
      colIdx = 0;
      (*pTable)(rowIdx,colIdx++) << _T("Tendon ") << LABEL_DUCT(tendonIdx);
      (*pTable)(rowIdx,colIdx++) << force.SetValue(tsDetails.Tendons[tendonIdx].dPi[pgsTypes::pftCreep]);
      (*pTable)(rowIdx,colIdx++) << _T("");
      (*pTable)(rowIdx,colIdx++) << force.SetValue(tsDetails.Tendons[tendonIdx].dPi[pgsTypes::pftShrinkage]);
      (*pTable)(rowIdx,colIdx++) << _T("");
      (*pTable)(rowIdx,colIdx++) << force.SetValue(tsDetails.Tendons[tendonIdx].dPi[pgsTypes::pftRelaxation]);
      (*pTable)(rowIdx,colIdx++) << _T("");
   }

   return pTable;
}

rptRcTable* CTimeStepDetailsChapterBuilder::BuildIncrementalForceTable(IBroker* pBroker,const std::vector<pgsTypes::ProductForceType>& vLoads,const TIME_STEP_DETAILS& tsDetails,IEAFDisplayUnits* pDisplayUnits) const
{
   GET_IFACE2(pBroker,IProductLoads,pProductLoads);

   INIT_UV_PROTOTYPE(rptForceUnitValue,     force,      pDisplayUnits->GetGeneralForceUnit(),    false);
   INIT_UV_PROTOTYPE(rptMomentUnitValue,    moment,     pDisplayUnits->GetSmallMomentUnit(),     false);

   IndexType nLoads = vLoads.size();
   rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(nLoads+3);
   pTable->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CJ_LEFT));
   pTable->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CJ_LEFT));
   pTable->SetNumberOfHeaderRows(2);

   // label loading types across the top row
   RowIndexType rowIdx = 0;
   ColumnIndexType colIdx = 0;
   pTable->SetRowSpan(rowIdx,colIdx,2);
   pTable->SetRowSpan(rowIdx+1,colIdx,SKIP_CELL);
   (*pTable)(rowIdx,colIdx++) << _T("Component");

   pTable->SetRowSpan(rowIdx,colIdx,2);
   pTable->SetRowSpan(rowIdx+1,colIdx,SKIP_CELL);
   (*pTable)(rowIdx,colIdx++) << _T("");

   pTable->SetColumnSpan(rowIdx,colIdx,nLoads+1);
   (*pTable)(rowIdx,colIdx++) << _T("Loading");
   for ( IndexType i = 0; i < nLoads; i++ )
   {
      pTable->SetColumnSpan(rowIdx,colIdx++,SKIP_CELL);
   }
   
   rowIdx++;
   colIdx = 2;
   for ( IndexType i = 0; i < nLoads; i++ )
   {
      pgsTypes::ProductForceType pfType = vLoads[i];
      (*pTable)(rowIdx,colIdx++) << pProductLoads->GetProductLoadName(pfType);
   }
   (*pTable)(rowIdx,colIdx++) << _T("Total");

   // Label the rows in column 0
   rowIdx++;
   colIdx = 0;
   (*pTable)(rowIdx,colIdx++) << _T("Composite");
   (*pTable)(rowIdx,colIdx  ) << DELTA_P << _T("(") << rptForceUnitTag(&pDisplayUnits->GetGeneralForceUnit().UnitOfMeasure) << _T(")") << rptNewLine;
   (*pTable)(rowIdx,colIdx++) << DELTA_M << _T("(") << rptMomentUnitTag(&pDisplayUnits->GetSmallMomentUnit().UnitOfMeasure) << _T(")");

   rowIdx++;
   colIdx = 0;
   (*pTable)(rowIdx,colIdx++) << _T("Girder");
   (*pTable)(rowIdx,colIdx  ) << DELTA_Pk << _T("(") << rptForceUnitTag(&pDisplayUnits->GetGeneralForceUnit().UnitOfMeasure) << _T(")") << rptNewLine;
   (*pTable)(rowIdx,colIdx++) << DELTA_Mk << _T("(") << rptMomentUnitTag(&pDisplayUnits->GetSmallMomentUnit().UnitOfMeasure) << _T(")");

   IndexType nRebar = tsDetails.GirderRebar.size();
   for ( IndexType i = 0; i < nRebar; i++ )
   {
      const TIME_STEP_REBAR& tsRebar = tsDetails.GirderRebar[i];

      rowIdx++;
      colIdx = 0;
      (*pTable)(rowIdx,colIdx++) << _T("Girder Rebar ") << (i+1);
      (*pTable)(rowIdx,colIdx++) << DELTA_Pk << _T("(") << rptForceUnitTag(&pDisplayUnits->GetGeneralForceUnit().UnitOfMeasure) << _T(")");
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

      (*pTable)(rowIdx,colIdx++) << DELTA_Pk << _T("(") << rptForceUnitTag(&pDisplayUnits->GetGeneralForceUnit().UnitOfMeasure) << _T(")");
   }

   rowIdx++;
   colIdx = 0;
   (*pTable)(rowIdx,colIdx++) << _T("Deck");
   (*pTable)(rowIdx,colIdx  ) << DELTA_Pk << _T("(") << rptForceUnitTag(&pDisplayUnits->GetGeneralForceUnit().UnitOfMeasure) << _T(")") << rptNewLine;
   (*pTable)(rowIdx,colIdx++) << DELTA_Mk << _T("(") << rptMomentUnitTag(&pDisplayUnits->GetSmallMomentUnit().UnitOfMeasure) << _T(")");

   for ( int i = 0; i < 2; i++ )
   {
      pgsTypes::DeckRebarMatType matType = (pgsTypes::DeckRebarMatType)i;
      for ( int j = 0; j < 2; j++ )
      {
         rowIdx++;
         colIdx = 0;
         pgsTypes::DeckRebarBarType barType = (pgsTypes::DeckRebarBarType)j;
         if ( matType == pgsTypes::drmTop )
         {
            if ( barType == pgsTypes::drbIndividual )
            {
               (*pTable)(rowIdx,colIdx++) << _T("Deck Top Mat Individual Rebar");
            }
            else
            {
               (*pTable)(rowIdx,colIdx++) << _T("Deck Top Mat Lump Sum Rebar");
            }
         }
         else
         {
            if ( barType == pgsTypes::drbIndividual )
            {
               (*pTable)(rowIdx,colIdx++) << _T("Deck Bottom Mat Individual Rebar");
            }
            else
            {
               (*pTable)(rowIdx,colIdx++) << _T("Deck Bottom Mat Lump Sum Rebar");
            }
         }

         (*pTable)(rowIdx,colIdx++) << DELTA_Pk << _T("(") << rptForceUnitTag(&pDisplayUnits->GetGeneralForceUnit().UnitOfMeasure) << _T(")");
      }
   }

   DuctIndexType nTendons = tsDetails.Tendons.size();
   for ( DuctIndexType tendonIdx = 0; tendonIdx < nTendons; tendonIdx++ )
   {
      const TIME_STEP_STRAND& tsTendon = tsDetails.Tendons[tendonIdx];

      rowIdx++;
      colIdx = 0;
      (*pTable)(rowIdx,colIdx++) << _T("Tendon ") << LABEL_DUCT(tendonIdx);
      (*pTable)(rowIdx,colIdx++) << DELTA_Pk << _T("(") << rptForceUnitTag(&pDisplayUnits->GetGeneralForceUnit().UnitOfMeasure) << _T(")");
   }

   // fill the table
   colIdx = 2;
   for ( IndexType i = 0; i < nLoads; i++, colIdx++ )
   {
      rowIdx = pTable->GetNumberOfHeaderRows();

      pgsTypes::ProductForceType pfType = vLoads[i];

      // Composite
      (*pTable)(rowIdx,colIdx) << force.SetValue(tsDetails.dPi[pfType]) << rptNewLine;
      (*pTable)(rowIdx,colIdx) << moment.SetValue(tsDetails.dMi[pfType]);
      rowIdx++;

      // Girder
      (*pTable)(rowIdx,colIdx) << force.SetValue(tsDetails.Girder.dPi[pfType]) << rptNewLine;
      (*pTable)(rowIdx,colIdx) << moment.SetValue(tsDetails.Girder.dMi[pfType]);
      rowIdx++;

      // Girder Rebar
      BOOST_FOREACH(const TIME_STEP_REBAR& tsRebar,tsDetails.GirderRebar)
      {
         (*pTable)(rowIdx++,colIdx) << force.SetValue(tsRebar.dPi[pfType]);
      }

      // Strands
      for ( int i = 0; i < 3; i++ )
      {
         pgsTypes::StrandType strandType = (pgsTypes::StrandType)i;
         (*pTable)(rowIdx++,colIdx) << force.SetValue(tsDetails.Strands[strandType].dPi[pfType]);
      }

      // Deck
      (*pTable)(rowIdx,colIdx) << force.SetValue(tsDetails.Deck.dPi[pfType]) << rptNewLine;
      (*pTable)(rowIdx,colIdx) << moment.SetValue(tsDetails.Deck.dMi[pfType]);
      rowIdx++;

      // Deck Rebar
      for ( int i = 0; i < 2; i++ )
      {
         pgsTypes::DeckRebarMatType matType = (pgsTypes::DeckRebarMatType)i;
         for ( int j = 0; j < 2; j++ )
         {
            pgsTypes::DeckRebarBarType barType = (pgsTypes::DeckRebarBarType)j;
            (*pTable)(rowIdx++,colIdx) << force.SetValue(tsDetails.DeckRebar[matType][barType].dPi[pfType]);
         }
      }

      // Tendons
      BOOST_FOREACH(const TIME_STEP_STRAND& tsTendon,tsDetails.Tendons)
      {
         (*pTable)(rowIdx++,colIdx) << force.SetValue(tsTendon.dPi[pfType]);
      }
   } // next loading

   // Totals
   rowIdx = pTable->GetNumberOfHeaderRows();

   // Composite
   (*pTable)(rowIdx,colIdx) << force.SetValue(tsDetails.dP) << rptNewLine;
   (*pTable)(rowIdx,colIdx) << moment.SetValue(tsDetails.dM);
   rowIdx++;

   // Girder
   (*pTable)(rowIdx,colIdx) << force.SetValue(tsDetails.Girder.dP) << rptNewLine;
   (*pTable)(rowIdx,colIdx) << moment.SetValue(tsDetails.Girder.dM);
   rowIdx++;

   // Girder Rebar
   BOOST_FOREACH(const TIME_STEP_REBAR& tsRebar,tsDetails.GirderRebar)
   {
      (*pTable)(rowIdx++,colIdx) << force.SetValue(tsRebar.dP);
   }

   // Strands
   for ( int i = 0; i < 3; i++ )
   {
      pgsTypes::StrandType strandType = (pgsTypes::StrandType)i;
      (*pTable)(rowIdx++,colIdx) << force.SetValue(tsDetails.Strands[strandType].dP);
   }

   // Deck
   (*pTable)(rowIdx,colIdx) << force.SetValue(tsDetails.Deck.dP) << rptNewLine;
   (*pTable)(rowIdx,colIdx) << moment.SetValue(tsDetails.Deck.dM);
   rowIdx++;

   // Deck Rebar
   for ( int i = 0; i < 2; i++ )
   {
      pgsTypes::DeckRebarMatType matType = (pgsTypes::DeckRebarMatType)i;
      for ( int j = 0; j < 2; j++ )
      {
         pgsTypes::DeckRebarBarType barType = (pgsTypes::DeckRebarBarType)j;
         (*pTable)(rowIdx++,colIdx) << force.SetValue(tsDetails.DeckRebar[matType][barType].dP);
      }
   }

   // Tendons
   BOOST_FOREACH(const TIME_STEP_STRAND& tsTendon,tsDetails.Tendons)
   {
      (*pTable)(rowIdx++,colIdx) << force.SetValue(tsTendon.dP);
   }


   return pTable;
}

rptRcTable* CTimeStepDetailsChapterBuilder::BuildIncrementalStressTable(IBroker* pBroker,const std::vector<pgsTypes::ProductForceType>& vLoads,const TIME_STEP_DETAILS& tsDetails,IEAFDisplayUnits* pDisplayUnits) const
{
   GET_IFACE2(pBroker,IProductLoads,pProductLoads);

   INIT_UV_PROTOTYPE(rptStressUnitValue,     stress,      pDisplayUnits->GetStressUnit(),    false);

   IndexType nLoads = vLoads.size();
   rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(nLoads+2);
   pTable->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CJ_LEFT));
   pTable->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CJ_LEFT));
   pTable->SetNumberOfHeaderRows(2);

   // label loading types across the top row
   RowIndexType rowIdx = 0;
   ColumnIndexType colIdx = 0;
   pTable->SetRowSpan(rowIdx,colIdx,2);
   pTable->SetRowSpan(rowIdx+1,colIdx,SKIP_CELL);
   (*pTable)(rowIdx,colIdx++) << _T("Component");

   pTable->SetColumnSpan(rowIdx,colIdx,nLoads+1);
   (*pTable)(rowIdx,colIdx++) << _T("Loading");
   for ( IndexType i = 0; i < nLoads; i++ )
   {
      pTable->SetColumnSpan(rowIdx,colIdx++,SKIP_CELL);
   }
   
   rowIdx++;
   colIdx = 1;
   for ( IndexType i = 0; i < nLoads; i++ )
   {
      pgsTypes::ProductForceType pfType = vLoads[i];
      (*pTable)(rowIdx,colIdx++) << COLHDR(pProductLoads->GetProductLoadName(pfType),rptStressUnitTag,pDisplayUnits->GetStressUnit());
   }
   (*pTable)(rowIdx,colIdx++) << COLHDR(_T("Total"),rptStressUnitTag,pDisplayUnits->GetStressUnit());

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

   (*pTable)(rowIdx++,colIdx) << _T("Top Deck");
   (*pTable)(rowIdx++,colIdx) << _T("Bottom Deck");

   DuctIndexType nTendons = tsDetails.Tendons.size();
   for ( DuctIndexType tendonIdx = 0; tendonIdx < nTendons; tendonIdx++ )
   {
      const TIME_STEP_STRAND& tsTendon = tsDetails.Tendons[tendonIdx];
      (*pTable)(rowIdx++,colIdx) << _T("Tendon ") << LABEL_DUCT(tendonIdx);
   }

   // fill the table
   Float64 f_top_girder = 0;
   Float64 f_bot_girder = 0;
   Float64 f_top_deck   = 0;
   Float64 f_bot_deck   = 0;
   colIdx = 1;
   for ( IndexType i = 0; i < nLoads; i++, colIdx++ )
   {
      rowIdx = pTable->GetNumberOfHeaderRows();

      pgsTypes::ProductForceType pfType = vLoads[i];

      // Girder
      f_top_girder += tsDetails.Girder.f[pgsTypes::TopFace][pfType][rtIncremental];
      f_bot_girder += tsDetails.Girder.f[pgsTypes::BottomFace][pfType][rtIncremental];
      (*pTable)(rowIdx++,colIdx) << stress.SetValue(tsDetails.Girder.f[pgsTypes::TopFace][pfType][rtIncremental]);
      (*pTable)(rowIdx++,colIdx) << stress.SetValue(tsDetails.Girder.f[pgsTypes::BottomFace][pfType][rtIncremental]);

      // Strands
      for ( int i = 0; i < 3; i++ )
      {
         pgsTypes::StrandType strandType = (pgsTypes::StrandType)i;
         (*pTable)(rowIdx++,colIdx) << stress.SetValue(tsDetails.Strands[strandType].dfpei[pfType]);
      }

      // Deck
      f_top_deck += tsDetails.Deck.f[pgsTypes::TopFace][pfType][rtIncremental];
      f_bot_deck += tsDetails.Deck.f[pgsTypes::BottomFace][pfType][rtIncremental];
      (*pTable)(rowIdx++,colIdx) << stress.SetValue(tsDetails.Deck.f[pgsTypes::TopFace][pfType][rtIncremental]);
      (*pTable)(rowIdx++,colIdx) << stress.SetValue(tsDetails.Deck.f[pgsTypes::BottomFace][pfType][rtIncremental]);

      // Tendons
      BOOST_FOREACH(const TIME_STEP_STRAND& tsTendon,tsDetails.Tendons)
      {
         (*pTable)(rowIdx++,colIdx) << stress.SetValue(tsTendon.dfpei[pfType]);
      }
   } // next loading

   // Totals
   rowIdx = pTable->GetNumberOfHeaderRows();

   // Girder
   (*pTable)(rowIdx++,colIdx) << stress.SetValue(f_top_girder);
   (*pTable)(rowIdx++,colIdx) << stress.SetValue(f_bot_girder);

   // Strands
   for ( int i = 0; i < 3; i++ )
   {
      pgsTypes::StrandType strandType = (pgsTypes::StrandType)i;
      (*pTable)(rowIdx++,colIdx) << stress.SetValue(tsDetails.Strands[strandType].dfpe);
   }

   // Deck
   (*pTable)(rowIdx++,colIdx) << stress.SetValue(f_top_deck);
   (*pTable)(rowIdx++,colIdx) << stress.SetValue(f_bot_deck);

   // Tendons
   BOOST_FOREACH(const TIME_STEP_STRAND& tsTendon,tsDetails.Tendons)
   {
      (*pTable)(rowIdx++,colIdx) << stress.SetValue(tsTendon.dfpe);
   }

   return pTable;
}

std::vector<pgsTypes::ProductForceType> CTimeStepDetailsChapterBuilder::GetProductForces(IBroker* pBroker,IntervalIndexType intervalIdx,const CGirderKey& girderKey) const
{
   std::vector<pgsTypes::ProductForceType> vProductForces;

   GET_IFACE2(pBroker,IProductLoads,pLoads);
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IUserDefinedLoadData,pUserLoads);

   vProductForces.push_back(pgsTypes::pftGirder);

   if ( !IsZero(pUserLoads->GetConstructionLoad()) )
   {
      vProductForces.push_back(pgsTypes::pftConstruction);
   }

   if ( pBridge->GetDeckType() != pgsTypes::sdtNone )
   {
      vProductForces.push_back(pgsTypes::pftSlab);
      vProductForces.push_back(pgsTypes::pftSlabPad);

      if ( pBridge->GetDeckType() == pBridge->GetDeckType() == pgsTypes::sdtCompositeSIP )
      {
         vProductForces.push_back(pgsTypes::pftSlabPanel);
      }
   }

   vProductForces.push_back(pgsTypes::pftDiaphragm);

   if ( pBridge->HasOverlay() )
   {
      vProductForces.push_back(pgsTypes::pftOverlay);
   }

   if ( pLoads->HasSidewalkLoad(girderKey) )
   {
      vProductForces.push_back(pgsTypes::pftSidewalk);
   }

   vProductForces.push_back(pgsTypes::pftTrafficBarrier);
   if ( pUserLoads->HasUserDC(girderKey) )
   {
      vProductForces.push_back(pgsTypes::pftUserDC);
   }

   if ( pUserLoads->HasUserDW(girderKey) )
   {
      vProductForces.push_back(pgsTypes::pftUserDW);
   }

   if ( pLoads->HasShearKeyLoad(girderKey) )
   {
      vProductForces.push_back(pgsTypes::pftShearKey);
   }

   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);
   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      if ( 0 < pStrandGeom->GetStrandCount(CSegmentKey(girderKey,segIdx),pgsTypes::Straight) ||
           0 < pStrandGeom->GetStrandCount(CSegmentKey(girderKey,segIdx),pgsTypes::Harped)   ||
           0 < pStrandGeom->GetStrandCount(CSegmentKey(girderKey,segIdx),pgsTypes::Temporary)
           )
      {
         vProductForces.push_back(pgsTypes::pftPretension);
         break;
      }
   }

   GET_IFACE2(pBroker,ITendonGeometry,pTendonGeom);
   DuctIndexType nDucts = pTendonGeom->GetDuctCount(girderKey);
   if ( 0 < nDucts )
   {
      vProductForces.push_back(pgsTypes::pftPostTensioning);
      vProductForces.push_back(pgsTypes::pftSecondaryEffects);
   }

      // time-depending effects
   GET_IFACE2(pBroker, ILossParameters, pLossParams);
   if ( !pLossParams->IgnoreCreepEffects() )
   {
      vProductForces.push_back(pgsTypes::pftCreep);
   }

   if ( !pLossParams->IgnoreShrinkageEffects() )
   {
      vProductForces.push_back(pgsTypes::pftShrinkage);
   }

   if ( !pLossParams->IgnoreRelaxationEffects() )
   {
      vProductForces.push_back(pgsTypes::pftRelaxation);
   }

   return vProductForces;
}
