///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2021  Washington State Department of Transportation
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

rptChapter* CTemporarySupportReactionChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CGirderLineReportSpecification* pGdrLineRptSpec = dynamic_cast<CGirderLineReportSpecification*>(pRptSpec);

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

   IntervalIndexType intervalIdx = INVALID_INDEX;
   SupportIndexType nTS = pBridge->GetTemporarySupportCount();
   std::vector<std::pair<SupportIndexType,pgsTypes::SupportType>> vSupports;
   for ( SupportIndexType tsIdx = 0; tsIdx < nTS; tsIdx++ )
   {
      if ( pBridge->GetTemporarySupportType(tsIdx) == pgsTypes::ErectionTower )
      {
         IntervalIndexType tsRemovalIntervalIdx = pIntervals->GetTemporarySupportRemovalInterval(tsIdx);
         intervalIdx = Min(tsRemovalIntervalIdx,intervalIdx);
         vSupports.emplace_back(tsIdx,pgsTypes::stTemporary);
      }
   }

   nTS = vSupports.size(); // this is the new size after strong backs are removed

   if ( nTS == 0 )
   {
      rptParagraph* pPara = new rptParagraph;
      *pChapter << pPara;
      *pPara << _T("No erection towers in this bridge") << rptNewLine;
      return pChapter;
   }

   ATLASSERT(intervalIdx != 0 && intervalIdx != INVALID_INDEX);
   intervalIdx--; // want the interval before removal

   bool bOverlay    = pBridge->HasOverlay();
   bool bFutureOverlay = pBridge->IsFutureOverlay();

   // Setup some unit-value prototypes
   INIT_UV_PROTOTYPE( rptForceUnitValue,  force,  pDisplayUnits->GetGeneralForceUnit(), false);
   INIT_UV_PROTOTYPE( rptMomentUnitValue, moment, pDisplayUnits->GetMomentUnit(), false);

   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   bool bDesign = false;
   bool bRating = false;
   bool bSegments, bConstruction, bDeck, bDeckPanels, bSidewalk, bShearKey, bLongitudinalJoint, bPedLoading, bPermit, bContinuousBeforeDeckCasting;
   GroupIndexType startGroup, endGroup;
   ColumnIndexType nCols = GetProductLoadTableColumnCount(pBroker,girderKey,analysisType,bDesign,bRating,false,&bSegments,&bConstruction,&bDeck,&bDeckPanels,&bSidewalk,&bShearKey,&bLongitudinalJoint,&bPedLoading,&bPermit,&bContinuousBeforeDeckCasting,&startGroup,&endGroup);
   nCols++; // add one for Type column
   nCols++; // add one for Reaction column

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

   rptRcTable* p_table = rptStyleManager::CreateDefaultTable(nCols,_T(""));

   p_table->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   p_table->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   p_table->SetColumnStyle(1,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   p_table->SetStripeRowColumnStyle(1,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   p_table->SetColumnStyle(2,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   p_table->SetStripeRowColumnStyle(2,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   ColumnIndexType col = 0;
   p_table->SetNumberOfHeaderRows(2);
   p_table->SetRowSpan(0,col,2);
   (*p_table)(0,col++) << _T("Temporary") << rptNewLine << _T("Support");

   p_table->SetRowSpan(0,col,2);
   (*p_table)(0,col++) << _T("Type");

   p_table->SetRowSpan(0,col,2);
   (*p_table)(0,col++) << _T("Reaction");

   p_table->SetRowSpan(0,col,2);
   (*p_table)(0,col++) << pProductLoads->GetProductLoadName(pgsTypes::pftGirder);

   p_table->SetRowSpan(0,col,2);
   (*p_table)(0,col++) << pProductLoads->GetProductLoadName(pgsTypes::pftDiaphragm);

   if (bShearKey)
   {
      p_table->SetRowSpan(0, col, 2);
      (*p_table)(0, col++) << pProductLoads->GetProductLoadName(pgsTypes::pftShearKey);
   }

   if (bLongitudinalJoint)
   {
      p_table->SetRowSpan(0, col, 2);
      (*p_table)(0, col++) << pProductLoads->GetProductLoadName(pgsTypes::pftLongitudinalJoint);
   }

   if ( bConstruction )
   {
      p_table->SetRowSpan(0,col,2);
      (*p_table)(0,col++) << pProductLoads->GetProductLoadName(pgsTypes::pftConstruction);
   }

   if (bDeck)
   {
      p_table->SetRowSpan(0, col, 2);
      (*p_table)(0, col++) << pProductLoads->GetProductLoadName(pgsTypes::pftSlab);

      p_table->SetRowSpan(0, col, 2);
      (*p_table)(0, col++) << pProductLoads->GetProductLoadName(pgsTypes::pftSlabPad);
   }

   if ( bDeckPanels )
   {
      p_table->SetRowSpan(0,col,2);
      (*p_table)(0,col++) << pProductLoads->GetProductLoadName(pgsTypes::pftSlabPanel);
   }

   if ( bSidewalk )
   {
      p_table->SetRowSpan(0,col,2);
      (*p_table)(0,col++) << pProductLoads->GetProductLoadName(pgsTypes::pftSidewalk);
   }

   p_table->SetRowSpan(0,col,2);
   (*p_table)(0,col++) << pProductLoads->GetProductLoadName(pgsTypes::pftTrafficBarrier);

   if ( bOverlay )
   {
      p_table->SetRowSpan(0,col,2);
      if ( bFutureOverlay )
      {
         (*p_table)(0,col++) << _T("Future") << rptNewLine << pProductLoads->GetProductLoadName(pgsTypes::pftOverlay);
      }
      else
      {
         (*p_table)(0,col++) << pProductLoads->GetProductLoadName(pgsTypes::pftOverlay);
      }
   }

   *pPara << p_table << rptNewLine;
   
   std::vector<REACTION> vGirderReactions, vDiaphragmReactions, vShearKeyReactions, vLongitudinalJointReactions, vConstructionReactions, vSlabReactions, vSlabPadReactions, vDeckPanelReactions, vSidewalkReactions, vTrafficBarrierReactions, vOverlayReactions;
   
   vGirderReactions = pReactions->GetReaction(girderKey,vSupports,intervalIdx,pgsTypes::pftGirder,bat,rtCumulative);
   vDiaphragmReactions = pReactions->GetReaction(girderKey,vSupports,intervalIdx,pgsTypes::pftDiaphragm,bat,rtCumulative);
   if ( bShearKey )
   {
      vShearKeyReactions = pReactions->GetReaction(girderKey,vSupports,intervalIdx,pgsTypes::pftShearKey,bat,rtCumulative);
   }

   if (bLongitudinalJoint)
   {
      vLongitudinalJointReactions = pReactions->GetReaction(girderKey, vSupports, intervalIdx, pgsTypes::pftLongitudinalJoint, bat, rtCumulative);
   }

   if ( bConstruction )
   {
      vConstructionReactions = pReactions->GetReaction(girderKey,vSupports,intervalIdx,pgsTypes::pftConstruction,bat,rtCumulative);
   }

   if (bDeck)
   {
      vSlabReactions = pReactions->GetReaction(girderKey, vSupports, intervalIdx, pgsTypes::pftSlab, bat, rtCumulative);
      vSlabPadReactions = pReactions->GetReaction(girderKey, vSupports, intervalIdx, pgsTypes::pftSlabPad, bat, rtCumulative);
   }

   if ( bDeckPanels )
   {
      vDeckPanelReactions = pReactions->GetReaction(girderKey,vSupports,intervalIdx,pgsTypes::pftSlabPanel,bat,rtCumulative);
   }
   if ( bSidewalk )
   {
      vSidewalkReactions = pReactions->GetReaction(girderKey,vSupports,intervalIdx,pgsTypes::pftSidewalk,bat,rtCumulative);
   }
   
   vTrafficBarrierReactions = pReactions->GetReaction(girderKey,vSupports,intervalIdx,pgsTypes::pftTrafficBarrier,bat,rtCumulative);
   
   if ( bOverlay )
   {
      vOverlayReactions = pReactions->GetReaction(girderKey,vSupports,intervalIdx,pgsTypes::pftOverlay,bat,rtCumulative);
   }

   RowIndexType row = p_table->GetNumberOfHeaderRows();
   for ( SupportIndexType tsIdx = 0; tsIdx < nTS; tsIdx++, row += 3 )
   {
      ColumnIndexType col = 0;
      p_table->SetRowSpan(row,col,3);
      (*p_table)(row,col++) << LABEL_TEMPORARY_SUPPORT(vSupports[tsIdx].first);

      p_table->SetRowSpan(row,col,3);
      (*p_table)(row,col++) << _T("Idealized");

      (*p_table)(row,col) << _T("Fx (") << pDisplayUnits->GetGeneralForceUnit().UnitOfMeasure.UnitTag() << _T(")");
      (*p_table)(row+1,col) << _T("Fy (") << pDisplayUnits->GetGeneralForceUnit().UnitOfMeasure.UnitTag() << _T(")");
      (*p_table)(row+2,col) << _T("Mz (") << pDisplayUnits->GetMomentUnit().UnitOfMeasure.UnitTag() << _T(")");
      col++;

      REACTION reaction = vGirderReactions[tsIdx];
      (*p_table)(row,col)   << force.SetValue(reaction.Fx);
      (*p_table)(row+1,col) << force.SetValue(reaction.Fy);
      (*p_table)(row+2,col) << moment.SetValue(reaction.Mz);
      col++;

      reaction = vDiaphragmReactions[tsIdx];
      (*p_table)(row,col)   << force.SetValue(reaction.Fx);
      (*p_table)(row+1,col) << force.SetValue(reaction.Fy);
      (*p_table)(row+2,col) << moment.SetValue(reaction.Mz);
      col++;

      if (bShearKey)
      {
         reaction = vShearKeyReactions[tsIdx];
         (*p_table)(row, col) << force.SetValue(reaction.Fx);
         (*p_table)(row + 1, col) << force.SetValue(reaction.Fy);
         (*p_table)(row + 2, col) << moment.SetValue(reaction.Mz);
         col++;
      }

      if (bLongitudinalJoint)
      {
         reaction = vLongitudinalJointReactions[tsIdx];
         (*p_table)(row, col) << force.SetValue(reaction.Fx);
         (*p_table)(row + 1, col) << force.SetValue(reaction.Fy);
         (*p_table)(row + 2, col) << moment.SetValue(reaction.Mz);
         col++;
      }

      if ( bConstruction )
      {
         reaction = vConstructionReactions[tsIdx];
         (*p_table)(row,col)   << force.SetValue(reaction.Fx);
         (*p_table)(row+1,col) << force.SetValue(reaction.Fy);
         (*p_table)(row+2,col) << moment.SetValue(reaction.Mz);
         col++;
      }

      if (bDeck)
      {
         reaction = vSlabReactions[tsIdx];
         (*p_table)(row, col) << force.SetValue(reaction.Fx);
         (*p_table)(row + 1, col) << force.SetValue(reaction.Fy);
         (*p_table)(row + 2, col) << moment.SetValue(reaction.Mz);
         col++;

         reaction = vSlabPadReactions[tsIdx];
         (*p_table)(row, col) << force.SetValue(reaction.Fx);
         (*p_table)(row + 1, col) << force.SetValue(reaction.Fy);
         (*p_table)(row + 2, col) << moment.SetValue(reaction.Mz);
         col++;
      }

      if ( bDeckPanels )
      {
         reaction = vDeckPanelReactions[tsIdx];
         (*p_table)(row,col)   << force.SetValue(reaction.Fx);
         (*p_table)(row+1,col) << force.SetValue(reaction.Fy);
         (*p_table)(row+2,col) << moment.SetValue(reaction.Mz);
         col++;
      }

      if ( bSidewalk )
      {
         reaction = vSidewalkReactions[tsIdx];
         (*p_table)(row,col)   << force.SetValue(reaction.Fx);
         (*p_table)(row+1,col) << force.SetValue(reaction.Fy);
         (*p_table)(row+2,col) << moment.SetValue(reaction.Mz);
         col++;
      }

      reaction = vTrafficBarrierReactions[tsIdx];
      (*p_table)(row,col)   << force.SetValue(reaction.Fx);
      (*p_table)(row+1,col) << force.SetValue(reaction.Fy);
      (*p_table)(row+2,col) << moment.SetValue(reaction.Mz);
      col++;

      if ( bOverlay )
      {
         reaction = vOverlayReactions[tsIdx];
         (*p_table)(row,col)   << force.SetValue(reaction.Fx);
         (*p_table)(row+1,col) << force.SetValue(reaction.Fy);
         (*p_table)(row+2,col) << moment.SetValue(reaction.Mz);
         col++;
      }
   }

   return pChapter;
}

CChapterBuilder* CTemporarySupportReactionChapterBuilder::Clone() const
{
   return new CTemporarySupportReactionChapterBuilder;
}
