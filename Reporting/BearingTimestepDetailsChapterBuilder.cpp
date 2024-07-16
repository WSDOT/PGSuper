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

   //for poi and per interval.... i guess I need the poi in outer loop?

   //create new strain table


   return pChapter;
}

std::unique_ptr<WBFL::Reporting::ChapterBuilder> CBearingTimeStepDetailsChapterBuilder::Clone() const
{
   return std::make_unique<CBearingTimeStepDetailsChapterBuilder>();
}

rptRcTable* CBearingTimeStepDetailsChapterBuilder::BuildIncrementalStrainTable(IBroker* pBroker, const std::vector<pgsTypes::ProductForceType>& vLoads, const TIME_STEP_DETAILS& tsDetails, bool bHasDeck, IEAFDisplayUnits* pDisplayUnits) const
{
   GET_IFACE2(pBroker, IProductLoads, pProductLoads);

   IndexType nLoads = vLoads.size();
   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(nLoads + 3 + 3);
   pTable->SetColumnStyle(0, rptStyleManager::GetTableCellStyle(CJ_LEFT));
   pTable->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle(CJ_LEFT));
   pTable->SetNumberOfHeaderRows(2);

   // label loading types across the top row
   RowIndexType rowIdx = 0;
   ColumnIndexType colIdx = 0;
   pTable->SetRowSpan(rowIdx, colIdx, 2);
   (*pTable)(rowIdx, colIdx++) << _T("Component");

   pTable->SetColumnSpan(rowIdx, colIdx, nLoads);
   (*pTable)(rowIdx, colIdx) << _T("Loading");
   colIdx += nLoads;

   rowIdx++;
   colIdx = 1;
   for (IndexType i = 0; i < nLoads; i++)
   {
      pgsTypes::ProductForceType pfType = vLoads[i];
      (*pTable)(rowIdx, colIdx++) << pProductLoads->GetProductLoadName(pfType) << rptNewLine << symbol(epsilon) << Super2(_T("x10"), _T("6"));
   }

   rowIdx = 0;
   pTable->SetRowSpan(rowIdx, colIdx, 2);
   (*pTable)(rowIdx, colIdx++) << _T("Incremental") << rptNewLine << _T("Total") << rptNewLine << symbol(epsilon) << Super2(_T("x10"), _T("6"));

   pTable->SetRowSpan(rowIdx, colIdx, 2);
   (*pTable)(rowIdx, colIdx++) << _T("Cumulative") << rptNewLine << _T("Total") << rptNewLine << symbol(epsilon) << Super2(_T("x10"), _T("6"));

   pTable->SetColumnSpan(rowIdx, colIdx, 3);
   (*pTable)(rowIdx, colIdx) << _T("Cumulative Strains");
   rowIdx++;
   (*pTable)(rowIdx, colIdx++) << pProductLoads->GetProductLoadName(pgsTypes::pftCreep) << rptNewLine << symbol(epsilon) << Super2(_T("x10"), _T("6"));
   (*pTable)(rowIdx, colIdx++) << pProductLoads->GetProductLoadName(pgsTypes::pftShrinkage) << rptNewLine << symbol(epsilon) << Super2(_T("x10"), _T("6"));
   (*pTable)(rowIdx, colIdx++) << pProductLoads->GetProductLoadName(pgsTypes::pftRelaxation) << rptNewLine << symbol(epsilon) << Super2(_T("x10"), _T("6"));


   // Label the rows in column 0
   rowIdx = pTable->GetNumberOfHeaderRows();
   colIdx = 0;
   (*pTable)(rowIdx++, colIdx) << _T("Top Girder");
   (*pTable)(rowIdx++, colIdx) << _T("Bottom Girder");

   //for (int i = 0; i < 3; i++)
   //{
   //   pgsTypes::StrandType strandType = (pgsTypes::StrandType)i;
   //   if (strandType == pgsTypes::Straight)
   //   {
   //      (*pTable)(rowIdx++, colIdx) << _T("Straight Strands");
   //   }
   //   else if (strandType == pgsTypes::Harped)
   //   {
   //      (*pTable)(rowIdx++, colIdx) << _T("Harped Strands");
   //   }
   //   else if (strandType == pgsTypes::Temporary)
   //   {
   //      (*pTable)(rowIdx++, colIdx) << _T("Temporary Strands");
   //   }
   //}

   if (bHasDeck)
   {
      (*pTable)(rowIdx++, colIdx) << _T("Top Deck");
      (*pTable)(rowIdx++, colIdx) << _T("Bottom Deck");
   }

   //DuctIndexType nTendons = tsDetails.SegmentTendons.size();
   //for (DuctIndexType tendonIdx = 0; tendonIdx < nTendons; tendonIdx++)
   //{
   //   const TIME_STEP_STRAND& tsTendon = tsDetails.SegmentTendons[tendonIdx];
   //   (*pTable)(rowIdx++, colIdx) << _T("Segment Tendon ") << LABEL_DUCT(tendonIdx);
   //}

   //nTendons = tsDetails.GirderTendons.size();
   //for (DuctIndexType tendonIdx = 0; tendonIdx < nTendons; tendonIdx++)
   //{
   //   const TIME_STEP_STRAND& tsTendon = tsDetails.GirderTendons[tendonIdx];
   //   (*pTable)(rowIdx++, colIdx) << _T("Girder Tendon ") << LABEL_DUCT(tendonIdx);
   //}

   // fill the table
   colIdx = 1;
   for (IndexType i = 0; i < nLoads; i++, colIdx++)
   {
      rowIdx = pTable->GetNumberOfHeaderRows();

      pgsTypes::ProductForceType pfType = vLoads[i];

      // Girder
      (*pTable)(rowIdx++, colIdx) << tsDetails.Girder.strain_by_load_type[pgsTypes::TopFace][pfType][rtIncremental] * 1E6;
      (*pTable)(rowIdx++, colIdx) << tsDetails.Girder.strain_by_load_type[pgsTypes::BottomFace][pfType][rtIncremental] * 1E6;

      //// Strands
      //for (int i = 0; i < 3; i++)
      //{
      //   pgsTypes::StrandType strandType = (pgsTypes::StrandType)i;
      //   (*pTable)(rowIdx++, colIdx) << stress.SetValue(tsDetails.Strands[strandType].dfpei[pfType]);
      //}

      if (bHasDeck)
      {
         // Deck
         (*pTable)(rowIdx++, colIdx) << tsDetails.Deck.strain_by_load_type[pgsTypes::TopFace][pfType][rtIncremental] * 1E6;
         (*pTable)(rowIdx++, colIdx) << tsDetails.Deck.strain_by_load_type[pgsTypes::BottomFace][pfType][rtIncremental] * 1E6;
      }

      //// Segment Tendons
      //for (const auto& tsTendon : tsDetails.SegmentTendons)
      //{
      //   (*pTable)(rowIdx++, colIdx) << stress.SetValue(tsTendon.dfpei[pfType]);
      //}

      //// Girder Tendons
      //for (const auto& tsTendon : tsDetails.GirderTendons)
      //{
      //   (*pTable)(rowIdx++, colIdx) << stress.SetValue(tsTendon.dfpei[pfType]);
      //}
   } // next loading

   // Incremental Totals
   rowIdx = pTable->GetNumberOfHeaderRows();

   // Girder
   (*pTable)(rowIdx++, colIdx) << tsDetails.Girder.strain[pgsTypes::TopFace][rtIncremental] * 1E6;
   (*pTable)(rowIdx++, colIdx) << tsDetails.Girder.strain[pgsTypes::BottomFace][rtIncremental] * 1E6;

   //// Strands
   //for (int i = 0; i < 3; i++)
   //{
   //   pgsTypes::StrandType strandType = (pgsTypes::StrandType)i;
   //   (*pTable)(rowIdx++, colIdx) << stress.SetValue(tsDetails.Strands[strandType].dfpe);
   //}

   if (bHasDeck)
   {
      // Deck
      (*pTable)(rowIdx++, colIdx) << tsDetails.Deck.strain[pgsTypes::TopFace][rtIncremental] * 1E6;
      (*pTable)(rowIdx++, colIdx) << tsDetails.Deck.strain[pgsTypes::BottomFace][rtIncremental] * 1E6;
   }

   //// Segment Tendons
   //for (const auto& tsTendon : tsDetails.SegmentTendons)
   //{
   //   (*pTable)(rowIdx++, colIdx) << stress.SetValue(tsTendon.dfpe);
   //}

   //// Girder Tendons
   //for (const auto& tsTendon : tsDetails.GirderTendons)
   //{
   //   (*pTable)(rowIdx++, colIdx) << stress.SetValue(tsTendon.dfpe);
   //}

   // Cumulative Totals
   colIdx++;
   rowIdx = pTable->GetNumberOfHeaderRows();

   // Girder
   (*pTable)(rowIdx++, colIdx) << tsDetails.Girder.strain[pgsTypes::TopFace][rtCumulative] * 1E6;
   (*pTable)(rowIdx++, colIdx) << tsDetails.Girder.strain[pgsTypes::BottomFace][rtCumulative] * 1E6;

   //// Strands
   //for (int i = 0; i < 3; i++)
   //{
   //   pgsTypes::StrandType strandType = (pgsTypes::StrandType)i;
   //   (*pTable)(rowIdx++, colIdx) << stress.SetValue(tsDetails.Strands[strandType].fpe);
   //}

   if (bHasDeck)
   {
      // Deck
      (*pTable)(rowIdx++, colIdx) << tsDetails.Deck.strain[pgsTypes::TopFace][rtCumulative] * 1E6;
      (*pTable)(rowIdx++, colIdx) << tsDetails.Deck.strain[pgsTypes::BottomFace][rtCumulative] * 1E6;
   }

   //// Segment Tendons
   //for (const auto& tsTendon : tsDetails.SegmentTendons)
   //{
   //   (*pTable)(rowIdx++, colIdx) << stress.SetValue(tsTendon.fpe);
   //}

   //// Girder Tendons
   //for (const auto& tsTendon : tsDetails.GirderTendons)
   //{
   //   (*pTable)(rowIdx++, colIdx) << stress.SetValue(tsTendon.fpe);
   //}


   // Cumulative Time-Dependent Effects
   colIdx++;
   rowIdx = pTable->GetNumberOfHeaderRows();

   // Girder
   (*pTable)(rowIdx, colIdx) << tsDetails.Girder.strain_by_load_type[pgsTypes::TopFace][pgsTypes::pftCreep][rtCumulative] * 1E6;
   (*pTable)(rowIdx, colIdx + 1) << tsDetails.Girder.strain_by_load_type[pgsTypes::TopFace][pgsTypes::pftShrinkage][rtCumulative] * 1E6;
   (*pTable)(rowIdx, colIdx + 2) << tsDetails.Girder.strain_by_load_type[pgsTypes::TopFace][pgsTypes::pftRelaxation][rtCumulative] * 1E6;
   rowIdx++;

   (*pTable)(rowIdx, colIdx) << tsDetails.Girder.strain_by_load_type[pgsTypes::BottomFace][pgsTypes::pftCreep][rtCumulative] * 1E6;
   (*pTable)(rowIdx, colIdx + 1) << tsDetails.Girder.strain_by_load_type[pgsTypes::BottomFace][pgsTypes::pftShrinkage][rtCumulative] * 1E6;
   (*pTable)(rowIdx, colIdx + 2) << tsDetails.Girder.strain_by_load_type[pgsTypes::BottomFace][pgsTypes::pftRelaxation][rtCumulative] * 1E6;
   rowIdx++;

   //// Strands
   //for (int i = 0; i < 3; i++)
   //{
   //   pgsTypes::StrandType strandType = (pgsTypes::StrandType)i;
   //   (*pTable)(rowIdx, colIdx) << stress.SetValue(tsDetails.Strands[strandType].fpei[pgsTypes::pftCreep]);
   //   (*pTable)(rowIdx, colIdx + 1) << stress.SetValue(tsDetails.Strands[strandType].fpei[pgsTypes::pftShrinkage]);
   //   (*pTable)(rowIdx, colIdx + 2) << stress.SetValue(tsDetails.Strands[strandType].fpei[pgsTypes::pftRelaxation]);
   //   rowIdx++;
   //}

   if (bHasDeck)
   {
      // Deck
      (*pTable)(rowIdx, colIdx) << tsDetails.Deck.strain_by_load_type[pgsTypes::TopFace][pgsTypes::pftCreep][rtCumulative] * 1E6;
      (*pTable)(rowIdx, colIdx + 1) << tsDetails.Deck.strain_by_load_type[pgsTypes::TopFace][pgsTypes::pftShrinkage][rtCumulative] * 1E6;
      (*pTable)(rowIdx, colIdx + 2) << tsDetails.Deck.strain_by_load_type[pgsTypes::TopFace][pgsTypes::pftRelaxation][rtCumulative] * 1E6;
      rowIdx++;

      (*pTable)(rowIdx, colIdx) << tsDetails.Deck.strain_by_load_type[pgsTypes::BottomFace][pgsTypes::pftCreep][rtCumulative] * 1E6;
      (*pTable)(rowIdx, colIdx + 1) << tsDetails.Deck.strain_by_load_type[pgsTypes::BottomFace][pgsTypes::pftShrinkage][rtCumulative] * 1E6;
      (*pTable)(rowIdx, colIdx + 2) << tsDetails.Deck.strain_by_load_type[pgsTypes::BottomFace][pgsTypes::pftRelaxation][rtCumulative] * 1E6;
      rowIdx++;
   }

   //// Segment Tendons
   //for (const auto& tsTendon : tsDetails.SegmentTendons)
   //{
   //   (*pTable)(rowIdx, colIdx) << stress.SetValue(tsTendon.fpei[pgsTypes::pftCreep]);
   //   (*pTable)(rowIdx, colIdx + 1) << stress.SetValue(tsTendon.fpei[pgsTypes::pftShrinkage]);
   //   (*pTable)(rowIdx, colIdx + 2) << stress.SetValue(tsTendon.fpei[pgsTypes::pftRelaxation]);
   //   rowIdx++;
   //}

   //// Girder Tendons
   //for (const auto& tsTendon : tsDetails.GirderTendons)
   //{
   //   (*pTable)(rowIdx, colIdx) << stress.SetValue(tsTendon.fpei[pgsTypes::pftCreep]);
   //   (*pTable)(rowIdx, colIdx + 1) << stress.SetValue(tsTendon.fpei[pgsTypes::pftShrinkage]);
   //   (*pTable)(rowIdx, colIdx + 2) << stress.SetValue(tsTendon.fpei[pgsTypes::pftRelaxation]);
   //   rowIdx++;
   //}

   return pTable;
}


