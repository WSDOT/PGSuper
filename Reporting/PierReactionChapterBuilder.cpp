///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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
#include <Reporting\PierReactionChapterBuilder.h>

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
   CPierReactionChapterBuilder
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CPierReactionChapterBuilder::CPierReactionChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CPierReactionChapterBuilder::GetName() const
{
   return TEXT("Pier Reactions");
}

rptChapter* CPierReactionChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CGirderLineReportSpecification* pGdrLineRptSpec = dynamic_cast<CGirderLineReportSpecification*>(pRptSpec);

   CComPtr<IBroker> pBroker;
   CGirderKey girderKey;

   pGdrLineRptSpec->GetBroker(&pBroker);
   girderKey = pGdrLineRptSpec->GetGirderKey();

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker,IIntervals,pIntervals);
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IReactions,pReactions);
   GET_IFACE2(pBroker,IProductLoads,pProductLoads);

   GET_IFACE2(pBroker,ISpecification,pSpec);
   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();
   analysisType = (analysisType == pgsTypes::Envelope ? pgsTypes::Continuous : analysisType);
   pgsTypes::BridgeAnalysisType bat = (analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan);

   IntervalIndexType intervalIdx = pIntervals->GetIntervalCount() - 1;

   bool bOverlay    = pBridge->HasOverlay();
   bool bFutureOverlay = pBridge->IsFutureOverlay();

   // Setup some unit-value prototypes
   INIT_UV_PROTOTYPE( rptForceUnitValue,  force,  pDisplayUnits->GetGeneralForceUnit(), false);
   INIT_UV_PROTOTYPE( rptMomentUnitValue, moment, pDisplayUnits->GetMomentUnit(), false);

   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   bool bDesign = true;
   bool bRating = false;
   bool bSegments, bConstruction, bDeckPanels, bSidewalk, bShearKey, bPedLoading, bPermit, bContinuousBeforeDeckCasting;
   GroupIndexType startGroup, endGroup;
   ColumnIndexType nCols = GetProductLoadTableColumnCount(pBroker,girderKey,analysisType,bDesign,bRating,false,&bSegments,&bConstruction,&bDeckPanels,&bSidewalk,&bShearKey,&bPedLoading,&bPermit,&bContinuousBeforeDeckCasting,&startGroup,&endGroup);
   nCols++; // add one for Type column
   nCols++; // add one for Reaction column
   nCols += 4; // min/max for design live load reactions
   if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
   {
      nCols += 4; // min/max for fatigue live load reactions
   }

   if ( bPedLoading )
   {
      nCols += 4; // min/max for pedestrian live load reactions
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
   p_table->SetRowSpan(1,col,SKIP_CELL);
   (*p_table)(0,col++) << _T("Pier");

   p_table->SetRowSpan(0,col,2);
   p_table->SetRowSpan(1,col,SKIP_CELL);
   (*p_table)(0,col++) << _T("Type");

   p_table->SetRowSpan(0,col,2);
   p_table->SetRowSpan(1,col,SKIP_CELL);
   (*p_table)(0,col++) << _T("Reaction");

   p_table->SetRowSpan(0,col,2);
   p_table->SetRowSpan(1,col,SKIP_CELL);
   (*p_table)(0,col++) << pProductLoads->GetProductLoadName(pgsTypes::pftGirder);

   p_table->SetRowSpan(0,col,2);
   p_table->SetRowSpan(1,col,SKIP_CELL);
   (*p_table)(0,col++) << pProductLoads->GetProductLoadName(pgsTypes::pftDiaphragm);

   if ( bShearKey )
   {
      p_table->SetRowSpan(0,col,2);
      p_table->SetRowSpan(1,col,SKIP_CELL);
      (*p_table)(0,col++) << pProductLoads->GetProductLoadName(pgsTypes::pftShearKey);
   }

   if ( bConstruction )
   {
      p_table->SetRowSpan(0,col,2);
      p_table->SetRowSpan(1,col,SKIP_CELL);
      (*p_table)(0,col++) << pProductLoads->GetProductLoadName(pgsTypes::pftConstruction);
   }

   p_table->SetRowSpan(0,col,2);
   p_table->SetRowSpan(1,col,SKIP_CELL);
   (*p_table)(0,col++) << pProductLoads->GetProductLoadName(pgsTypes::pftSlab);

   p_table->SetRowSpan(0,col,2);
   p_table->SetRowSpan(1,col,SKIP_CELL);
   (*p_table)(0,col++) << pProductLoads->GetProductLoadName(pgsTypes::pftSlabPad);

   if ( bDeckPanels )
   {
      p_table->SetRowSpan(0,col,2);
      p_table->SetRowSpan(1,col,SKIP_CELL);
      (*p_table)(0,col++) << pProductLoads->GetProductLoadName(pgsTypes::pftSlabPanel);
   }

   if ( bSidewalk )
   {
      p_table->SetRowSpan(0,col,2);
      p_table->SetRowSpan(1,col,SKIP_CELL);
      (*p_table)(0,col++) << pProductLoads->GetProductLoadName(pgsTypes::pftSidewalk);
   }

   p_table->SetRowSpan(0,col,2);
   p_table->SetRowSpan(1,col,SKIP_CELL);
   (*p_table)(0,col++) << pProductLoads->GetProductLoadName(pgsTypes::pftTrafficBarrier);

   if ( bOverlay )
   {
      p_table->SetRowSpan(0,col,2);
      p_table->SetRowSpan(1,col,SKIP_CELL);
      if ( bFutureOverlay )
      {
         (*p_table)(0,col++) << _T("Future") << rptNewLine << pProductLoads->GetProductLoadName(pgsTypes::pftOverlay);
      }
      else
      {
         (*p_table)(0,col++) << pProductLoads->GetProductLoadName(pgsTypes::pftOverlay);
      }
   }

   if ( bPedLoading )
   {
      p_table->SetColumnSpan(0,col,2);
      p_table->SetColumnSpan(0,col+1,SKIP_CELL);
      (*p_table)(0,col) << _T("$ Pedestrian") << rptNewLine << _T("Optimize Fx");
      (*p_table)(1,col) << _T("Max");
      (*p_table)(1,col+1) << _T("Min");
      col += 2;

      p_table->SetColumnSpan(0,col,2);
      p_table->SetColumnSpan(0,col+1,SKIP_CELL);
      (*p_table)(0,col) << _T("$ Pedestrian") << rptNewLine << _T("Optimize Fy");
      (*p_table)(1,col) << _T("Max");
      (*p_table)(1,col+1) << _T("Min");
      col += 2;

      p_table->SetColumnSpan(0,col,2);
      p_table->SetColumnSpan(0,col+1,SKIP_CELL);
      (*p_table)(0,col) << _T("$ Pedestrian") << rptNewLine << _T("Optimize Mz");
      (*p_table)(1,col) << _T("Max");
      (*p_table)(1,col+1) << _T("Min");
      col += 2;
   }

   p_table->SetColumnSpan(0,col,2);
   p_table->SetColumnSpan(0,col+1,SKIP_CELL);
   (*p_table)(0,col) << _T("* Design Live Load") << rptNewLine << _T("Optimize Fx");
   (*p_table)(1,col) << _T("Max");
   (*p_table)(1,col+1) << _T("Min");
   col += 2;

   p_table->SetColumnSpan(0,col,2);
   p_table->SetColumnSpan(0,col+1,SKIP_CELL);
   (*p_table)(0,col) << _T("* Design Live Load") << rptNewLine << _T("Optimize Fy");
   (*p_table)(1,col) << _T("Max");
   (*p_table)(1,col+1) << _T("Min");
   col += 2;

   p_table->SetColumnSpan(0,col,2);
   p_table->SetColumnSpan(0,col+1,SKIP_CELL);
   (*p_table)(0,col) << _T("* Design Live Load") << rptNewLine << _T("Optimize Mz");
   (*p_table)(1,col) << _T("Max");
   (*p_table)(1,col+1) << _T("Min");
   col += 2;

   if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
   {
      p_table->SetColumnSpan(0,col,2);
      p_table->SetColumnSpan(0,col+1,SKIP_CELL);
      (*p_table)(0,col) << _T("* Fatigue Live Load") << rptNewLine << _T("Optimize Fx");
      (*p_table)(1,col) << _T("Max");
      (*p_table)(1,col+1) << _T("Min");
      col += 2;

      p_table->SetColumnSpan(0,col,2);
      p_table->SetColumnSpan(0,col+1,SKIP_CELL);
      (*p_table)(0,col) << _T("* Fatigue Live Load") << rptNewLine << _T("Optimize Fy");
      (*p_table)(1,col) << _T("Max");
      (*p_table)(1,col+1) << _T("Min");
      col += 2;

      p_table->SetColumnSpan(0,col,2);
      p_table->SetColumnSpan(0,col+1,SKIP_CELL);
      (*p_table)(0,col) << _T("* Fatigue Live Load") << rptNewLine << _T("Optimize Mz");
      (*p_table)(1,col) << _T("Max");
      (*p_table)(1,col+1) << _T("Min");
      col += 2;
   }

   *pPara << p_table << rptNewLine;
   *pPara << LIVELOAD_PER_LANE << rptNewLine;
   LiveLoadTableFooter(pBroker,pPara,girderKey,true,false);

   PierIndexType nPiers = pBridge->GetPierCount();
   std::vector<PierIndexType> vPiers;
   std::vector<std::pair<SupportIndexType,pgsTypes::SupportType>> vSupports;
   for ( PierIndexType pierIdx = 0; pierIdx < nPiers; pierIdx++ )
   {
      vPiers.push_back(pierIdx);
      vSupports.push_back(std::make_pair(pierIdx,pgsTypes::stPier));
   }
   
   std::vector<REACTION> vGirderReactions, vDiaphragmReactions, vShearKeyReactions, vConstructionReactions, vSlabReactions, vSlabPadReactions, vDeckPanelReactions, vSidewalkReactions, vTrafficBarrierReactions, vOverlayReactions;
   
   vGirderReactions = pReactions->GetReaction(girderKey,vSupports,intervalIdx,pgsTypes::pftGirder,bat,rtCumulative);
   vDiaphragmReactions = pReactions->GetReaction(girderKey,vSupports,intervalIdx,pgsTypes::pftDiaphragm,bat,rtCumulative);
   if ( bShearKey )
   {
      vShearKeyReactions = pReactions->GetReaction(girderKey,vSupports,intervalIdx,pgsTypes::pftShearKey,bat,rtCumulative);
   }
   if ( bConstruction )
   {
      vConstructionReactions = pReactions->GetReaction(girderKey,vSupports,intervalIdx,pgsTypes::pftConstruction,bat,rtCumulative);
   }
   vSlabReactions = pReactions->GetReaction(girderKey,vSupports,intervalIdx,pgsTypes::pftSlab,bat,rtCumulative);
   vSlabPadReactions = pReactions->GetReaction(girderKey,vSupports,intervalIdx,pgsTypes::pftSlabPad,bat,rtCumulative);
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

   std::vector<REACTION> vMinPedLLReactions[3], vMaxPedLLReactions[3];
   std::vector<VehicleIndexType> vMinPedLLVehicle[3], vMaxPedLLVehicle[3];
   std::vector<REACTION> vMinLLReactions[3], vMaxLLReactions[3];
   std::vector<VehicleIndexType> vMinLLVehicle[3], vMaxLLVehicle[3];
   std::vector<REACTION> vMinFatigueReactions[3], vMaxFatigueReactions[3];
   std::vector<VehicleIndexType> vMinFatigueVehicle[3], vMaxFatigueVehicle[3];
   for ( int i = 0; i < 3; i++ )
   {
      pgsTypes::ForceEffectType fetPrimary = (pgsTypes::ForceEffectType)i;
      pReactions->GetLiveLoadReaction(intervalIdx,pgsTypes::lltDesign,vPiers,girderKey,bat,true,false,fetPrimary,&vMinLLReactions[fetPrimary],&vMaxLLReactions[fetPrimary],&vMinLLVehicle[fetPrimary],&vMaxLLVehicle[fetPrimary]);

      if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
      {
         pReactions->GetLiveLoadReaction(intervalIdx,pgsTypes::lltFatigue,vPiers,girderKey,bat,true,false,fetPrimary,&vMinFatigueReactions[fetPrimary],&vMaxFatigueReactions[fetPrimary],&vMinFatigueVehicle[fetPrimary],&vMaxFatigueVehicle[fetPrimary]);
      }

      if ( bPedLoading )
      {
         pReactions->GetLiveLoadReaction(intervalIdx,pgsTypes::lltPedestrian,vPiers,girderKey,bat,true,false,fetPrimary,&vMinPedLLReactions[fetPrimary],&vMaxPedLLReactions[fetPrimary],&vMinPedLLVehicle[fetPrimary],&vMaxPedLLVehicle[fetPrimary]);
      }
   }


   RowIndexType row = p_table->GetNumberOfHeaderRows();
   for ( PierIndexType pierIdx = 0; pierIdx < nPiers; pierIdx++, row += 3 )
   {
      ColumnIndexType col = 0;
      p_table->SetRowSpan(row,col,3);
      p_table->SetRowSpan(row+1,col,SKIP_CELL);
      p_table->SetRowSpan(row+2,col,SKIP_CELL);
      (*p_table)(row,col++) << LABEL_PIER(pierIdx);

      p_table->SetRowSpan(row,col,3);
      p_table->SetRowSpan(row+1,col,SKIP_CELL);
      p_table->SetRowSpan(row+2,col,SKIP_CELL);
      if ( pBridge->GetPierModelType(pierIdx) == pgsTypes::pmtIdealized )
      {
         (*p_table)(row,col++) << _T("Idealized");
      }
      else
      {
         (*p_table)(row,col++) << _T("Physical");
      }

      (*p_table)(row,col) << _T("Fx (") << pDisplayUnits->GetGeneralForceUnit().UnitOfMeasure.UnitTag() << _T(")");
      (*p_table)(row+1,col) << _T("Fy (") << pDisplayUnits->GetGeneralForceUnit().UnitOfMeasure.UnitTag() << _T(")");
      (*p_table)(row+2,col) << _T("Mz (") << pDisplayUnits->GetMomentUnit().UnitOfMeasure.UnitTag() << _T(")");
      col++;

      REACTION reaction = vGirderReactions[pierIdx];
      (*p_table)(row,col)   << force.SetValue(reaction.Fx);
      (*p_table)(row+1,col) << force.SetValue(reaction.Fy);
      (*p_table)(row+2,col) << moment.SetValue(reaction.Mz);
      col++;

      reaction = vDiaphragmReactions[pierIdx];
      (*p_table)(row,col)   << force.SetValue(reaction.Fx);
      (*p_table)(row+1,col) << force.SetValue(reaction.Fy);
      (*p_table)(row+2,col) << moment.SetValue(reaction.Mz);
      col++;

      if ( bShearKey )
      {
         reaction = vShearKeyReactions[pierIdx];
         (*p_table)(row,col)   << force.SetValue(reaction.Fx);
         (*p_table)(row+1,col) << force.SetValue(reaction.Fy);
         (*p_table)(row+2,col) << moment.SetValue(reaction.Mz);
         col++;
      }

      if ( bConstruction )
      {
         reaction = vConstructionReactions[pierIdx];
         (*p_table)(row,col)   << force.SetValue(reaction.Fx);
         (*p_table)(row+1,col) << force.SetValue(reaction.Fy);
         (*p_table)(row+2,col) << moment.SetValue(reaction.Mz);
         col++;
      }

      reaction = vSlabReactions[pierIdx];
      (*p_table)(row,col)   << force.SetValue(reaction.Fx);
      (*p_table)(row+1,col) << force.SetValue(reaction.Fy);
      (*p_table)(row+2,col) << moment.SetValue(reaction.Mz);
      col++;

      reaction = vSlabPadReactions[pierIdx];
      (*p_table)(row,col)   << force.SetValue(reaction.Fx);
      (*p_table)(row+1,col) << force.SetValue(reaction.Fy);
      (*p_table)(row+2,col) << moment.SetValue(reaction.Mz);
      col++;

      if ( bDeckPanels )
      {
         reaction = vDeckPanelReactions[pierIdx];
         (*p_table)(row,col)   << force.SetValue(reaction.Fx);
         (*p_table)(row+1,col) << force.SetValue(reaction.Fy);
         (*p_table)(row+2,col) << moment.SetValue(reaction.Mz);
         col++;
      }

      if ( bSidewalk )
      {
         reaction = vSidewalkReactions[pierIdx];
         (*p_table)(row,col)   << force.SetValue(reaction.Fx);
         (*p_table)(row+1,col) << force.SetValue(reaction.Fy);
         (*p_table)(row+2,col) << moment.SetValue(reaction.Mz);
         col++;
      }

      reaction = vTrafficBarrierReactions[pierIdx];
      (*p_table)(row,col)   << force.SetValue(reaction.Fx);
      (*p_table)(row+1,col) << force.SetValue(reaction.Fy);
      (*p_table)(row+2,col) << moment.SetValue(reaction.Mz);
      col++;

      if ( bOverlay )
      {
         reaction = vOverlayReactions[pierIdx];
         (*p_table)(row,col)   << force.SetValue(reaction.Fx);
         (*p_table)(row+1,col) << force.SetValue(reaction.Fy);
         (*p_table)(row+2,col) << moment.SetValue(reaction.Mz);
         col++;
      }

      ColumnIndexType startCol = col;
      for ( int i = 0; i < 3; i++ )
      {
         pgsTypes::ForceEffectType fetPrimary = (pgsTypes::ForceEffectType)i;
         
         col = startCol + 2*i;
         ColumnIndexType colOffset = 0;
         if ( bPedLoading )
         {
            reaction = vMaxPedLLReactions[fetPrimary][pierIdx];
            (*p_table)(row,col)   << force.SetValue(reaction.Fx);
            (*p_table)(row+1,col) << force.SetValue(reaction.Fy);
            (*p_table)(row+2,col) << moment.SetValue(reaction.Mz);
            col++;

            reaction = vMinPedLLReactions[fetPrimary][pierIdx];
            (*p_table)(row,col)   << force.SetValue(reaction.Fx);
            (*p_table)(row+1,col) << force.SetValue(reaction.Fy);
            (*p_table)(row+2,col) << moment.SetValue(reaction.Mz);
            col++;

            colOffset += 4;
         }

         reaction = vMaxLLReactions[fetPrimary][pierIdx];
         (*p_table)(row,col+colOffset)   << force.SetValue(reaction.Fx);
         (*p_table)(row,col+colOffset)   << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << vMaxLLVehicle[fetPrimary][pierIdx] << _T(")");
         (*p_table)(row+1,col+colOffset) << force.SetValue(reaction.Fy);
         (*p_table)(row+1,col+colOffset) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << vMaxLLVehicle[fetPrimary][pierIdx] << _T(")");
         (*p_table)(row+2,col+colOffset) << moment.SetValue(reaction.Mz);
         (*p_table)(row+2,col+colOffset) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << vMaxLLVehicle[fetPrimary][pierIdx] << _T(")");
         col++;

         reaction = vMinLLReactions[fetPrimary][pierIdx];
         (*p_table)(row,col+colOffset)   << force.SetValue(reaction.Fx);
         (*p_table)(row,col+colOffset)   << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << vMinLLVehicle[fetPrimary][pierIdx] << _T(")");
         (*p_table)(row+1,col+colOffset) << force.SetValue(reaction.Fy);
         (*p_table)(row+1,col+colOffset) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << vMinLLVehicle[fetPrimary][pierIdx] << _T(")");
         (*p_table)(row+2,col+colOffset) << moment.SetValue(reaction.Mz);
         (*p_table)(row+2,col+colOffset) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << vMinLLVehicle[fetPrimary][pierIdx] << _T(")");
         col++;

         colOffset += 4;

         if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
         {
            reaction = vMaxFatigueReactions[fetPrimary][pierIdx];
            (*p_table)(row,col+colOffset)   << force.SetValue(reaction.Fx);
            (*p_table)(row,col+colOffset)   << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltFatigue) << vMaxFatigueVehicle[fetPrimary][pierIdx] << _T(")");
            (*p_table)(row+1,col+colOffset) << force.SetValue(reaction.Fy);
            (*p_table)(row+1,col+colOffset) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltFatigue) << vMaxFatigueVehicle[fetPrimary][pierIdx] << _T(")");
            (*p_table)(row+2,col+colOffset) << moment.SetValue(reaction.Mz);
            (*p_table)(row+2,col+colOffset) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltFatigue) << vMaxFatigueVehicle[fetPrimary][pierIdx] << _T(")");
            col++;

            reaction = vMinFatigueReactions[fetPrimary][pierIdx];
            (*p_table)(row,col+colOffset)   << force.SetValue(reaction.Fx);
            (*p_table)(row,col+colOffset)   << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltFatigue) << vMinFatigueVehicle[fetPrimary][pierIdx] << _T(")");
            (*p_table)(row+1,col+colOffset) << force.SetValue(reaction.Fy);
            (*p_table)(row+1,col+colOffset) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltFatigue) << vMinFatigueVehicle[fetPrimary][pierIdx] << _T(")");
            (*p_table)(row+2,col+colOffset) << moment.SetValue(reaction.Mz);
            (*p_table)(row+2,col+colOffset) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltFatigue) << vMinFatigueVehicle[fetPrimary][pierIdx] << _T(")");
            col++;
         }
      }
   }

   return pChapter;
}

CChapterBuilder* CPierReactionChapterBuilder::Clone() const
{
   return new CPierReactionChapterBuilder;
}
