///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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
#include <Reporting\TemporarySupportReactionChapterBuilder.h>

#include <IFace\Bridge.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Intervals.h>
#include <IFace\Project.h>

#include <Reporting\ProductMomentsTable.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CTemporarySupportReactionChapterBuilder
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CTemporarySupportReactionChapterBuilder::CTemporarySupportReactionChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CTemporarySupportReactionChapterBuilder::GetName() const
{
   return TEXT("Temporary Support Reactions");
}

rptChapter* CTemporarySupportReactionChapterBuilder::Build(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,Uint16 level) const
{
   auto pGdrLineRptSpec = std::dynamic_pointer_cast<const CGirderLineReportSpecification>(pRptSpec);

   CComPtr<IBroker> pBroker;
   CGirderKey girderKey;

   pGdrLineRptSpec->GetBroker(&pBroker);
   girderKey = pGdrLineRptSpec->GetGirderKey();

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);
   GET_IFACE2(pBroker,IBridge,pBridge);
   if ( pBridge->GetTemporarySupportCount() == 0 )
   {
      rptParagraph* pPara = new rptParagraph;
      *pChapter << pPara;
      *pPara << _T("No temporary supports in this bridge") << rptNewLine;
      return pChapter;
   }

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker,IIntervals,pIntervals);
   GET_IFACE2(pBroker,IReactions,pReactions);
   GET_IFACE2(pBroker,IProductLoads,pProductLoads);

   GET_IFACE2(pBroker,ISpecification,pSpec);
   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();
   analysisType = (analysisType == pgsTypes::Envelope ? pgsTypes::Continuous : analysisType);
   pgsTypes::BridgeAnalysisType bat = (analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan);

   IntervalIndexType firstErectionIntervalIdx = INVALID_INDEX;
   IntervalIndexType lastRemovalIntervalIdx = 0;
   SupportIndexType nTS = pBridge->GetTemporarySupportCount();
   std::vector<std::pair<SupportIndexType,pgsTypes::SupportType>> vSupports;
   for ( SupportIndexType tsIdx = 0; tsIdx < nTS; tsIdx++ )
   {
      if ( pBridge->GetTemporarySupportType(tsIdx) == pgsTypes::ErectionTower )
      {
         IntervalIndexType tsErectionIntervalIdx = pIntervals->GetTemporarySupportErectionInterval(tsIdx);
         firstErectionIntervalIdx = Min(tsErectionIntervalIdx, firstErectionIntervalIdx);

         IntervalIndexType tsRemovalIntervalIdx = pIntervals->GetTemporarySupportRemovalInterval(tsIdx);
         lastRemovalIntervalIdx = Max(tsRemovalIntervalIdx, lastRemovalIntervalIdx);
         
         vSupports.emplace_back(tsIdx,pgsTypes::stTemporary);
      }
   }

   firstErectionIntervalIdx = Max(firstErectionIntervalIdx,pIntervals->GetFirstSegmentErectionInterval(girderKey)); // don't report any interval before first erection... reactions are just zero

   nTS = vSupports.size(); // this is the new size after strong backs are removed

   if ( nTS == 0 )
   {
      rptParagraph* pPara = new rptParagraph;
      *pChapter << pPara;
      *pPara << _T("No erection towers in this bridge") << rptNewLine;
      return pChapter;
   }

   bool bOverlay    = pBridge->HasOverlay();
   bool bFutureOverlay = pBridge->IsFutureOverlay();

   // Setup some unit-value prototypes
   INIT_UV_PROTOTYPE( rptForceUnitValue,  force,  pDisplayUnits->GetGeneralForceUnit(), false);
   INIT_UV_PROTOTYPE( rptMomentUnitValue, moment, pDisplayUnits->GetMomentUnit(), false);

   bool bDesign = false;
   bool bRating = false;
   bool bSegments, bConstruction, bDeck, bDeckPanels, bSidewalk, bShearKey, bLongitudinalJoint, bPedLoading, bPermit, bContinuousBeforeDeckCasting;
   GroupIndexType startGroup, endGroup;
   ColumnIndexType nCols = GetProductLoadTableColumnCount(pBroker,girderKey,analysisType,bDesign,bRating,false,&bSegments,&bConstruction,&bDeck,&bDeckPanels,&bSidewalk,&bShearKey,&bLongitudinalJoint,&bPedLoading,&bPermit,&bContinuousBeforeDeckCasting,&startGroup,&endGroup);
   nCols++; // add one for Type column
   nCols++; // add one for Reaction column
   nCols++; // add one for Total column

   // Remove columns that aren't applicable
   if ( bSegments )
   {
      nCols--;
      bSegments = false;
   }

   if ( bPedLoading )
   {
      nCols -= 2;
      bPedLoading = false;
   }

   if ( bPermit )
   {
      nCols -= 2;
      bPermit = false;
   }

   GET_IFACE2(pBroker, IUserDefinedLoads, pUserDefinedLoads);
   bool bUserLoads = pUserDefinedLoads->DoUserLoadsExist(girderKey);
   if (bUserLoads)
   {
      nCols += 2;
   }

   for (IntervalIndexType intervalIdx = firstErectionIntervalIdx; intervalIdx < lastRemovalIntervalIdx; intervalIdx++)
   {
      rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
      *pChapter << pPara;

      std::_tostringstream os;
      os << _T("Interval ") << LABEL_INTERVAL(intervalIdx) << _T(" ") << pIntervals->GetDescription(intervalIdx) << std::endl;
      pPara->SetName(os.str().c_str());
      *pPara << pPara->GetName() << rptNewLine;

      rptRcTable* p_table = rptStyleManager::CreateDefaultTable(nCols, _T(""));

      pPara = new rptParagraph;
      *pChapter << pPara;
      *pPara << p_table << rptNewLine;
      *pPara << _T("NOTE: Reactions are for a single girder line.") << rptNewLine;

      p_table->SetColumnStyle(0, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
      p_table->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

      p_table->SetColumnStyle(1, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
      p_table->SetStripeRowColumnStyle(1, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

      p_table->SetColumnStyle(2, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
      p_table->SetStripeRowColumnStyle(2, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

      ColumnIndexType col = 0;
      (*p_table)(0, col++) << _T("Temporary") << rptNewLine << _T("Support");

      (*p_table)(0, col++) << _T("Type");

      (*p_table)(0, col++) << _T("Reaction");

      (*p_table)(0, col++) << pProductLoads->GetProductLoadName(pgsTypes::pftGirder);

      (*p_table)(0, col++) << pProductLoads->GetProductLoadName(pgsTypes::pftDiaphragm);

      if (bShearKey)
      {
         (*p_table)(0, col++) << pProductLoads->GetProductLoadName(pgsTypes::pftShearKey);
      }

      if (bLongitudinalJoint)
      {
         (*p_table)(0, col++) << pProductLoads->GetProductLoadName(pgsTypes::pftLongitudinalJoint);
      }

      if (bConstruction)
      {
         (*p_table)(0, col++) << pProductLoads->GetProductLoadName(pgsTypes::pftConstruction);
      }

      if (bDeck)
      {
         (*p_table)(0, col++) << pProductLoads->GetProductLoadName(pgsTypes::pftSlab);
         (*p_table)(0, col++) << pProductLoads->GetProductLoadName(pgsTypes::pftSlabPad);
      }

      if (bDeckPanels)
      {
         (*p_table)(0, col++) << pProductLoads->GetProductLoadName(pgsTypes::pftSlabPanel);
      }

      if (bSidewalk)
      {
         (*p_table)(0, col++) << pProductLoads->GetProductLoadName(pgsTypes::pftSidewalk);
      }

      (*p_table)(0, col++) << pProductLoads->GetProductLoadName(pgsTypes::pftTrafficBarrier);

      if (bOverlay)
      {
         if (bFutureOverlay)
         {
            (*p_table)(0, col++) << _T("Future") << rptNewLine << pProductLoads->GetProductLoadName(pgsTypes::pftOverlay);
         }
         else
         {
            (*p_table)(0, col++) << pProductLoads->GetProductLoadName(pgsTypes::pftOverlay);
         }
      }

      if (bUserLoads)
      {
         (*p_table)(0, col++) << pProductLoads->GetProductLoadName(pgsTypes::pftUserDC);
         (*p_table)(0, col++) << pProductLoads->GetProductLoadName(pgsTypes::pftUserDW);
      }

      (*p_table)(0, col++) << _T("Total");

      // temporary supports can be removed at different times... 
      // get a list of temporary supports that are still erected during this interval
      std::vector<std::pair<SupportIndexType, pgsTypes::SupportType>> vSupportsThisInterval;
      for (auto pair : vSupports)
      {
         IntervalIndexType tsRemovalIntervalIdx = pIntervals->GetTemporarySupportRemovalInterval(pair.first);
         if (intervalIdx < tsRemovalIntervalIdx)
         {
            vSupportsThisInterval.emplace_back(pair);
         }
      }

      std::vector<REACTION> vGirderReactions, vDiaphragmReactions, vShearKeyReactions, vLongitudinalJointReactions, vConstructionReactions, vSlabReactions, vSlabPadReactions, vDeckPanelReactions, vSidewalkReactions, vTrafficBarrierReactions, vOverlayReactions, vUserDCReactions, vUserDWReactions;

      vGirderReactions = pReactions->GetReaction(girderKey, vSupportsThisInterval, intervalIdx, pgsTypes::pftGirder, bat, rtCumulative);
      vDiaphragmReactions = pReactions->GetReaction(girderKey, vSupportsThisInterval, intervalIdx, pgsTypes::pftDiaphragm, bat, rtCumulative);
      if (bShearKey)
      {
         vShearKeyReactions = pReactions->GetReaction(girderKey, vSupportsThisInterval, intervalIdx, pgsTypes::pftShearKey, bat, rtCumulative);
      }

      if (bLongitudinalJoint)
      {
         vLongitudinalJointReactions = pReactions->GetReaction(girderKey, vSupportsThisInterval, intervalIdx, pgsTypes::pftLongitudinalJoint, bat, rtCumulative);
      }

      if (bConstruction)
      {
         vConstructionReactions = pReactions->GetReaction(girderKey, vSupportsThisInterval, intervalIdx, pgsTypes::pftConstruction, bat, rtCumulative);
      }

      if (bDeck)
      {
         vSlabReactions = pReactions->GetReaction(girderKey, vSupportsThisInterval, intervalIdx, pgsTypes::pftSlab, bat, rtCumulative);
         vSlabPadReactions = pReactions->GetReaction(girderKey, vSupportsThisInterval, intervalIdx, pgsTypes::pftSlabPad, bat, rtCumulative);
      }

      if (bDeckPanels)
      {
         vDeckPanelReactions = pReactions->GetReaction(girderKey, vSupportsThisInterval, intervalIdx, pgsTypes::pftSlabPanel, bat, rtCumulative);
      }
      if (bSidewalk)
      {
         vSidewalkReactions = pReactions->GetReaction(girderKey, vSupportsThisInterval, intervalIdx, pgsTypes::pftSidewalk, bat, rtCumulative);
      }

      vTrafficBarrierReactions = pReactions->GetReaction(girderKey, vSupportsThisInterval, intervalIdx, pgsTypes::pftTrafficBarrier, bat, rtCumulative);

      if (bOverlay)
      {
         vOverlayReactions = pReactions->GetReaction(girderKey, vSupportsThisInterval, intervalIdx, pgsTypes::pftOverlay, bat, rtCumulative);
      }

      if (bUserLoads)
      {
         vUserDCReactions = pReactions->GetReaction(girderKey, vSupportsThisInterval, intervalIdx, pgsTypes::pftUserDC, bat, rtCumulative);
         vUserDWReactions = pReactions->GetReaction(girderKey, vSupportsThisInterval, intervalIdx, pgsTypes::pftUserDW, bat, rtCumulative);
      }

      RowIndexType row = p_table->GetNumberOfHeaderRows();
      SupportIndexType nTSThisInterval = vSupportsThisInterval.size();
      for (SupportIndexType tsIdx = 0; tsIdx < nTSThisInterval; tsIdx++, row += 3)
      {
         ColumnIndexType col = 0;
         p_table->SetRowSpan(row, col, 3);
         (*p_table)(row, col++) << LABEL_TEMPORARY_SUPPORT(vSupportsThisInterval[tsIdx].first);

         p_table->SetRowSpan(row, col, 3);
         (*p_table)(row, col++) << _T("Idealized");

         (*p_table)(row, col) << _T("Fx (") << pDisplayUnits->GetGeneralForceUnit().UnitOfMeasure.UnitTag() << _T(")");
         (*p_table)(row + 1, col) << _T("Fy (") << pDisplayUnits->GetGeneralForceUnit().UnitOfMeasure.UnitTag() << _T(")");
         (*p_table)(row + 2, col) << _T("Mz (") << pDisplayUnits->GetMomentUnit().UnitOfMeasure.UnitTag() << _T(")");
         col++;

         REACTION total;

         REACTION reaction = vGirderReactions[tsIdx];
         total += reaction;
         (*p_table)(row, col) << force.SetValue(reaction.Fx);
         (*p_table)(row + 1, col) << force.SetValue(reaction.Fy);
         (*p_table)(row + 2, col) << moment.SetValue(reaction.Mz);
         col++;

         reaction = vDiaphragmReactions[tsIdx];
         total += reaction;
         (*p_table)(row, col) << force.SetValue(reaction.Fx);
         (*p_table)(row + 1, col) << force.SetValue(reaction.Fy);
         (*p_table)(row + 2, col) << moment.SetValue(reaction.Mz);
         col++;

         if (bShearKey)
         {
            reaction = vShearKeyReactions[tsIdx];
            total += reaction;
            (*p_table)(row, col) << force.SetValue(reaction.Fx);
            (*p_table)(row + 1, col) << force.SetValue(reaction.Fy);
            (*p_table)(row + 2, col) << moment.SetValue(reaction.Mz);
            col++;
         }

         if (bLongitudinalJoint)
         {
            reaction = vLongitudinalJointReactions[tsIdx];
            total += reaction;
            (*p_table)(row, col) << force.SetValue(reaction.Fx);
            (*p_table)(row + 1, col) << force.SetValue(reaction.Fy);
            (*p_table)(row + 2, col) << moment.SetValue(reaction.Mz);
            col++;
         }

         if (bConstruction)
         {
            reaction = vConstructionReactions[tsIdx];
            total += reaction;
            (*p_table)(row, col) << force.SetValue(reaction.Fx);
            (*p_table)(row + 1, col) << force.SetValue(reaction.Fy);
            (*p_table)(row + 2, col) << moment.SetValue(reaction.Mz);
            col++;
         }

         if (bDeck)
         {
            reaction = vSlabReactions[tsIdx];
            total += reaction;
            (*p_table)(row, col) << force.SetValue(reaction.Fx);
            (*p_table)(row + 1, col) << force.SetValue(reaction.Fy);
            (*p_table)(row + 2, col) << moment.SetValue(reaction.Mz);
            col++;

            reaction = vSlabPadReactions[tsIdx];
            total += reaction;
            (*p_table)(row, col) << force.SetValue(reaction.Fx);
            (*p_table)(row + 1, col) << force.SetValue(reaction.Fy);
            (*p_table)(row + 2, col) << moment.SetValue(reaction.Mz);
            col++;
         }

         if (bDeckPanels)
         {
            reaction = vDeckPanelReactions[tsIdx];
            total += reaction;
            (*p_table)(row, col) << force.SetValue(reaction.Fx);
            (*p_table)(row + 1, col) << force.SetValue(reaction.Fy);
            (*p_table)(row + 2, col) << moment.SetValue(reaction.Mz);
            col++;
         }

         if (bSidewalk)
         {
            reaction = vSidewalkReactions[tsIdx];
            total += reaction;
            (*p_table)(row, col) << force.SetValue(reaction.Fx);
            (*p_table)(row + 1, col) << force.SetValue(reaction.Fy);
            (*p_table)(row + 2, col) << moment.SetValue(reaction.Mz);
            col++;
         }

         reaction = vTrafficBarrierReactions[tsIdx];
         total += reaction;
         (*p_table)(row, col) << force.SetValue(reaction.Fx);
         (*p_table)(row + 1, col) << force.SetValue(reaction.Fy);
         (*p_table)(row + 2, col) << moment.SetValue(reaction.Mz);
         col++;

         if (bOverlay)
         {
            reaction = vOverlayReactions[tsIdx];
            total += reaction;
            (*p_table)(row, col) << force.SetValue(reaction.Fx);
            (*p_table)(row + 1, col) << force.SetValue(reaction.Fy);
            (*p_table)(row + 2, col) << moment.SetValue(reaction.Mz);
            col++;
         }

         if (bUserLoads)
         {
            reaction = vUserDCReactions[tsIdx];
            total += reaction;
            (*p_table)(row, col) << force.SetValue(reaction.Fx);
            (*p_table)(row + 1, col) << force.SetValue(reaction.Fy);
            (*p_table)(row + 2, col) << moment.SetValue(reaction.Mz);
            col++;

            reaction = vUserDWReactions[tsIdx];
            total += reaction;
            (*p_table)(row, col) << force.SetValue(reaction.Fx);
            (*p_table)(row + 1, col) << force.SetValue(reaction.Fy);
            (*p_table)(row + 2, col) << moment.SetValue(reaction.Mz);
            col++;
         }

         (*p_table)(row, col) << force.SetValue(total.Fx);
         (*p_table)(row + 1, col) << force.SetValue(total.Fy);
         (*p_table)(row + 2, col) << moment.SetValue(total.Mz);
         col++;

      }
   }

   return pChapter;
}

std::unique_ptr<WBFL::Reporting::ChapterBuilder> CTemporarySupportReactionChapterBuilder::Clone() const
{
   return std::make_unique<CTemporarySupportReactionChapterBuilder>();
}
