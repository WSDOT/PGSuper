///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2019  Washington State Department of Transportation
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
#include <Reporting\UserReactionTable.h>


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

rptChapter* CPierReactionChapterBuilder::Build(CReportSpecification* pRptSpec, Uint16 level) const
{
   CGirderLineReportSpecification* pGdrLineRptSpec = dynamic_cast<CGirderLineReportSpecification*>(pRptSpec);

   CComPtr<IBroker> pBroker;
   CGirderKey girderKey;

   pGdrLineRptSpec->GetBroker(&pBroker);
   girderKey = pGdrLineRptSpec->GetGirderKey();

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec, level);

   GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);
   GET_IFACE2(pBroker, IIntervals, pIntervals);
   GET_IFACE2(pBroker, IBridge, pBridge);
   GET_IFACE2(pBroker, IReactions, pReactions);
   GET_IFACE2(pBroker, IProductLoads, pProductLoads);

   GET_IFACE2(pBroker, ISpecification, pSpec);
   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   GET_IFACE2(pBroker, IProductForces, pProdForces);
   pgsTypes::BridgeAnalysisType batMax = pProdForces->GetBridgeAnalysisType(analysisType, pgsTypes::Maximize);
   pgsTypes::BridgeAnalysisType batMin = pProdForces->GetBridgeAnalysisType(analysisType, pgsTypes::Minimize);

   pgsTypes::BridgeAnalysisType batSS = pgsTypes::SimpleSpan;
   pgsTypes::BridgeAnalysisType batCS = pgsTypes::ContinuousSpan;

   IntervalIndexType intervalIdx = pIntervals->GetIntervalCount() - 1;

   bool bOverlay = pBridge->HasOverlay();
   bool bFutureOverlay = pBridge->IsFutureOverlay();

   // Setup some unit-value prototypes
   INIT_UV_PROTOTYPE(rptForceUnitValue, force, pDisplayUnits->GetGeneralForceUnit(), false);
   INIT_UV_PROTOTYPE(rptMomentUnitValue, moment, pDisplayUnits->GetMomentUnit(), false);

   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   bool bDesign = true;
   bool bRating = false;
   bool bSegments, bConstruction, bDeck, bDeckPanels, bSidewalk, bShearKey, bLongitudinalJoint, bPedLoading, bPermit, bContinuousBeforeDeckCasting;
   GroupIndexType startGroup, endGroup;
   ColumnIndexType nCols = GetProductLoadTableColumnCount(pBroker, girderKey, analysisType, bDesign, bRating, false, &bSegments, &bConstruction, &bDeck, &bDeckPanels, &bSidewalk, &bShearKey, &bLongitudinalJoint, &bPedLoading, &bPermit, &bContinuousBeforeDeckCasting, &startGroup, &endGroup);
   nCols++; // add one for Type column
   nCols++; // add one for Reaction column
   nCols += 4; // min/max for design live load reactions
   if (lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion())
   {
      nCols += 4; // min/max for fatigue live load reactions
   }

   if (bPermit)
   {
      nCols += 4; // min/max for permit live load reactions
   }

   if (bPedLoading)
   {
      nCols += 4; // min/max for pedestrian live load reactions
   }

   rptRcTable* p_table = rptStyleManager::CreateDefaultTable(nCols, _T(""));

   p_table->SetColumnStyle(0, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   p_table->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   p_table->SetColumnStyle(1, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   p_table->SetStripeRowColumnStyle(1, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   p_table->SetColumnStyle(2, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   p_table->SetStripeRowColumnStyle(2, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   ColumnIndexType col = 0;
   p_table->SetNumberOfHeaderRows(2);
   p_table->SetRowSpan(0, col, 2);
   (*p_table)(0, col++) << _T("Pier");

   p_table->SetRowSpan(0, col, 2);
   (*p_table)(0, col++) << _T("Type");

   p_table->SetRowSpan(0, col, 2);
   (*p_table)(0, col++) << _T("Reaction");

   p_table->SetRowSpan(0, col, 2);
   (*p_table)(0, col++) << pProductLoads->GetProductLoadName(pgsTypes::pftGirder);

   p_table->SetRowSpan(0, col, 2);
   (*p_table)(0, col++) << pProductLoads->GetProductLoadName(pgsTypes::pftDiaphragm);

   if (bShearKey)
   {
      if (analysisType == pgsTypes::Envelope && bContinuousBeforeDeckCasting)
      {
         p_table->SetColumnSpan(0, col, 2);
         (*p_table)(0, col) << pProductLoads->GetProductLoadName(pgsTypes::pftShearKey);
         (*p_table)(1, col++) << _T("Max");
         (*p_table)(1, col++) << _T("Min");
      }
      else
      {
         p_table->SetRowSpan(0, col, 2);
         (*p_table)(0, col++) << pProductLoads->GetProductLoadName(pgsTypes::pftShearKey);
      }
   }

   if (bLongitudinalJoint)
   {
      if (analysisType == pgsTypes::Envelope && bContinuousBeforeDeckCasting)
      {
         p_table->SetColumnSpan(0, col, 2);
         (*p_table)(0, col) << pProductLoads->GetProductLoadName(pgsTypes::pftLongitudinalJoint);
         (*p_table)(1, col++) << _T("Max");
         (*p_table)(1, col++) << _T("Min");
      }
      else
      {
         p_table->SetRowSpan(0, col, 2);
         (*p_table)(0, col++) << pProductLoads->GetProductLoadName(pgsTypes::pftLongitudinalJoint);
      }
   }

   if (bConstruction)
   {
      if (analysisType == pgsTypes::Envelope && bContinuousBeforeDeckCasting)
      {
         p_table->SetColumnSpan(0, col, 2);
         (*p_table)(0, col) << pProductLoads->GetProductLoadName(pgsTypes::pftConstruction);
         (*p_table)(1, col++) << _T("Max");
         (*p_table)(1, col++) << _T("Min");
      }
      else
      {
         p_table->SetRowSpan(0, col, 2);
         (*p_table)(0, col++) << pProductLoads->GetProductLoadName(pgsTypes::pftConstruction);
      }
   }

   if (bDeck)
   {
      if (analysisType == pgsTypes::Envelope && bContinuousBeforeDeckCasting)
      {
         p_table->SetColumnSpan(0, col, 2);
         (*p_table)(0, col) << pProductLoads->GetProductLoadName(pgsTypes::pftSlab);
         (*p_table)(1, col++) << _T("Max");
         (*p_table)(1, col++) << _T("Min");

         p_table->SetColumnSpan(0, col, 2);
         (*p_table)(0, col) << pProductLoads->GetProductLoadName(pgsTypes::pftSlabPad);
         (*p_table)(1, col++) << _T("Max");
         (*p_table)(1, col++) << _T("Min");
      }
      else
      {
         p_table->SetRowSpan(0, col, 2);
         (*p_table)(0, col++) << pProductLoads->GetProductLoadName(pgsTypes::pftSlab);

         p_table->SetRowSpan(0, col, 2);
         (*p_table)(0, col++) << pProductLoads->GetProductLoadName(pgsTypes::pftSlabPad);
      }
   }

   if (bDeckPanels)
   {
      if (analysisType == pgsTypes::Envelope && bContinuousBeforeDeckCasting)
      {
         p_table->SetColumnSpan(0, col, 2);
         (*p_table)(0, col) << pProductLoads->GetProductLoadName(pgsTypes::pftSlabPanel);
         (*p_table)(1, col++) << _T("Max");
         (*p_table)(1, col++) << _T("Min");
      }
      else
      {
         p_table->SetRowSpan(0, col, 2);
         (*p_table)(0, col++) << pProductLoads->GetProductLoadName(pgsTypes::pftSlabPanel);
      }
   }

   if (bSidewalk)
   {
      if (analysisType == pgsTypes::Envelope)
      {
         p_table->SetColumnSpan(0, col, 2);
         (*p_table)(0, col) << pProductLoads->GetProductLoadName(pgsTypes::pftSidewalk);
         (*p_table)(1, col++) << _T("Max");
         (*p_table)(1, col++) << _T("Min");
      }
      else
      {
         p_table->SetRowSpan(0, col, 2);
         (*p_table)(0, col++) << pProductLoads->GetProductLoadName(pgsTypes::pftSidewalk);
      }
   }

   if (analysisType == pgsTypes::Envelope)
   {
      p_table->SetColumnSpan(0, col, 2);
      (*p_table)(0, col) << pProductLoads->GetProductLoadName(pgsTypes::pftTrafficBarrier);
      (*p_table)(1, col++) << _T("Max");
      (*p_table)(1, col++) << _T("Min");
   }
   else
   {
      p_table->SetRowSpan(0, col, 2);
      (*p_table)(0, col++) << pProductLoads->GetProductLoadName(pgsTypes::pftTrafficBarrier);
   }

   if (bOverlay)
   {
      if (analysisType == pgsTypes::Envelope)
      {
         p_table->SetColumnSpan(0, col, 2);
         if (bFutureOverlay)
         {
            (*p_table)(0, col) << _T("Future") << rptNewLine << pProductLoads->GetProductLoadName(pgsTypes::pftOverlay);
         }
         else
         {
            (*p_table)(0, col) << pProductLoads->GetProductLoadName(pgsTypes::pftOverlay);
         }
         (*p_table)(1, col++) << _T("Max");
         (*p_table)(1, col++) << _T("Min");
      }
      else
      {
         p_table->SetRowSpan(0, col, 2);
         if (bFutureOverlay)
         {
            (*p_table)(0, col++) << _T("Future") << rptNewLine << pProductLoads->GetProductLoadName(pgsTypes::pftOverlay);
         }
         else
         {
            (*p_table)(0, col++) << pProductLoads->GetProductLoadName(pgsTypes::pftOverlay);
         }
      }
   }

   if (bPedLoading)
   {
      p_table->SetColumnSpan(0, col, 2);
      (*p_table)(0, col) << _T("$ Pedestrian") << rptNewLine << _T("Optimize Fx");
      (*p_table)(1, col) << _T("Max");
      (*p_table)(1, col + 1) << _T("Min");
      col += 2;

      p_table->SetColumnSpan(0, col, 2);
      (*p_table)(0, col) << _T("$ Pedestrian") << rptNewLine << _T("Optimize Fy");
      (*p_table)(1, col) << _T("Max");
      (*p_table)(1, col + 1) << _T("Min");
      col += 2;

      p_table->SetColumnSpan(0, col, 2);
      (*p_table)(0, col) << _T("$ Pedestrian") << rptNewLine << _T("Optimize Mz");
      (*p_table)(1, col) << _T("Max");
      (*p_table)(1, col + 1) << _T("Min");
      col += 2;
   }

   p_table->SetColumnSpan(0, col, 2);
   (*p_table)(0, col) << _T("* Design Live Load") << rptNewLine << _T("Optimize Fx");
   (*p_table)(1, col) << _T("Max");
   (*p_table)(1, col + 1) << _T("Min");
   col += 2;

   p_table->SetColumnSpan(0, col, 2);
   (*p_table)(0, col) << _T("* Design Live Load") << rptNewLine << _T("Optimize Fy");
   (*p_table)(1, col) << _T("Max");
   (*p_table)(1, col + 1) << _T("Min");
   col += 2;

   p_table->SetColumnSpan(0, col, 2);
   (*p_table)(0, col) << _T("* Design Live Load") << rptNewLine << _T("Optimize Mz");
   (*p_table)(1, col) << _T("Max");
   (*p_table)(1, col + 1) << _T("Min");
   col += 2;

   if (lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion())
   {
      p_table->SetColumnSpan(0, col, 2);
      (*p_table)(0, col) << _T("* Fatigue Live Load") << rptNewLine << _T("Optimize Fx");
      (*p_table)(1, col) << _T("Max");
      (*p_table)(1, col + 1) << _T("Min");
      col += 2;

      p_table->SetColumnSpan(0, col, 2);
      (*p_table)(0, col) << _T("* Fatigue Live Load") << rptNewLine << _T("Optimize Fy");
      (*p_table)(1, col) << _T("Max");
      (*p_table)(1, col + 1) << _T("Min");
      col += 2;

      p_table->SetColumnSpan(0, col, 2);
      (*p_table)(0, col) << _T("* Fatigue Live Load") << rptNewLine << _T("Optimize Mz");
      (*p_table)(1, col) << _T("Max");
      (*p_table)(1, col + 1) << _T("Min");
      col += 2;
   }

   if (bPermit)
   {
      p_table->SetColumnSpan(0, col, 2);
      (*p_table)(0, col) << _T("* Permit Live Load") << rptNewLine << _T("Optimize Fx");
      (*p_table)(1, col) << _T("Max");
      (*p_table)(1, col + 1) << _T("Min");
      col += 2;

      p_table->SetColumnSpan(0, col, 2);
      (*p_table)(0, col) << _T("* Permit Live Load") << rptNewLine << _T("Optimize Fy");
      (*p_table)(1, col) << _T("Max");
      (*p_table)(1, col + 1) << _T("Min");
      col += 2;

      p_table->SetColumnSpan(0, col, 2);
      (*p_table)(0, col) << _T("* Permit Live Load") << rptNewLine << _T("Optimize Mz");
      (*p_table)(1, col) << _T("Max");
      (*p_table)(1, col + 1) << _T("Min");
      col += 2;
   }

   *pPara << p_table << rptNewLine;

   if (bPedLoading)
   {
      *pPara << _T("$ Pedestrian values are per girder") << rptNewLine;
   }

   *pPara << LIVELOAD_PER_LANE << rptNewLine;
   LiveLoadTableFooter(pBroker, pPara, girderKey, true, false);

   PierIndexType nPiers = pBridge->GetPierCount();
   std::vector<PierIndexType> vPiers;
   std::vector<std::pair<SupportIndexType, pgsTypes::SupportType>> vSupports;
   for (PierIndexType pierIdx = 0; pierIdx < nPiers; pierIdx++)
   {
      vPiers.push_back(pierIdx);
      vSupports.emplace_back(pierIdx, pgsTypes::stPier);
   }

   std::vector<REACTION> vGirderReactions, vDiaphragmReactions, vShearKeyMaxReactions, vShearKeyMinReactions, vLongitudinalJointMaxReactions, vLongitudinalJointMinReactions, vConstructionMaxReactions, vConstructionMinReactions, vSlabMaxReactions, vSlabMinReactions, vSlabPadMaxReactions, vSlabPadMinReactions, vDeckPanelMaxReactions, vDeckPanelMinReactions, vSidewalkMaxReactions, vSidewalkMinReactions, vTrafficBarrierMaxReactions, vTrafficBarrierMinReactions, vOverlayMaxReactions, vOverlayMinReactions;

   vGirderReactions = pReactions->GetReaction(girderKey, vSupports, intervalIdx, pgsTypes::pftGirder, pgsTypes::SimpleSpan, rtCumulative);
   vDiaphragmReactions = pReactions->GetReaction(girderKey, vSupports, intervalIdx, pgsTypes::pftDiaphragm, pgsTypes::SimpleSpan, rtCumulative);

   if (bShearKey)
   {
      if (analysisType == pgsTypes::Envelope && bContinuousBeforeDeckCasting)
      {
         vShearKeyMaxReactions = pReactions->GetReaction(girderKey, vSupports, intervalIdx, pgsTypes::pftShearKey, batMax, rtCumulative);
         vShearKeyMinReactions = pReactions->GetReaction(girderKey, vSupports, intervalIdx, pgsTypes::pftShearKey, batMin, rtCumulative);
      }
      else
      {
         vShearKeyMaxReactions = pReactions->GetReaction(girderKey, vSupports, intervalIdx, pgsTypes::pftShearKey, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, rtCumulative);
      }
   }

   if (bLongitudinalJoint)
   {
      if (analysisType == pgsTypes::Envelope && bContinuousBeforeDeckCasting)
      {
         vLongitudinalJointMaxReactions = pReactions->GetReaction(girderKey, vSupports, intervalIdx, pgsTypes::pftLongitudinalJoint, batMax, rtCumulative);
         vLongitudinalJointMinReactions = pReactions->GetReaction(girderKey, vSupports, intervalIdx, pgsTypes::pftLongitudinalJoint, batMin, rtCumulative);
      }
      else
      {
         vLongitudinalJointMaxReactions = pReactions->GetReaction(girderKey, vSupports, intervalIdx, pgsTypes::pftLongitudinalJoint, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, rtCumulative);
      }
   }

   if (bConstruction)
   {
      if (analysisType == pgsTypes::Envelope && bContinuousBeforeDeckCasting)
      {
         vConstructionMaxReactions = pReactions->GetReaction(girderKey, vSupports, intervalIdx, pgsTypes::pftConstruction, batMax, rtCumulative);
         vConstructionMinReactions = pReactions->GetReaction(girderKey, vSupports, intervalIdx, pgsTypes::pftConstruction, batMin, rtCumulative);
      }
      else
      {
         vConstructionMaxReactions = pReactions->GetReaction(girderKey, vSupports, intervalIdx, pgsTypes::pftConstruction, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, rtCumulative);
      }
   }

   if (bDeck)
   {
      if (analysisType == pgsTypes::Envelope && bContinuousBeforeDeckCasting)
      {
         vSlabMaxReactions = pReactions->GetReaction(girderKey, vSupports, intervalIdx, pgsTypes::pftSlab, batMax, rtCumulative);
         vSlabMinReactions = pReactions->GetReaction(girderKey, vSupports, intervalIdx, pgsTypes::pftSlab, batMin, rtCumulative);
         vSlabPadMaxReactions = pReactions->GetReaction(girderKey, vSupports, intervalIdx, pgsTypes::pftSlabPad, batMax, rtCumulative);
         vSlabPadMinReactions = pReactions->GetReaction(girderKey, vSupports, intervalIdx, pgsTypes::pftSlabPad, batMin, rtCumulative);
      }
      else
      {
         vSlabMaxReactions = pReactions->GetReaction(girderKey, vSupports, intervalIdx, pgsTypes::pftSlab, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, rtCumulative);
         vSlabPadMaxReactions = pReactions->GetReaction(girderKey, vSupports, intervalIdx, pgsTypes::pftSlabPad, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, rtCumulative);
      }
   }

   if (bDeckPanels)
   {
      if (analysisType == pgsTypes::Envelope && bContinuousBeforeDeckCasting)
      {
         vDeckPanelMaxReactions = pReactions->GetReaction(girderKey, vSupports, intervalIdx, pgsTypes::pftSlabPanel, batMax, rtCumulative);
         vDeckPanelMinReactions = pReactions->GetReaction(girderKey, vSupports, intervalIdx, pgsTypes::pftSlabPanel, batMin, rtCumulative);
      }
      else
      {
         vDeckPanelMaxReactions = pReactions->GetReaction(girderKey, vSupports, intervalIdx, pgsTypes::pftSlabPanel, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, rtCumulative);
      }
   }

   if (bSidewalk)
   {
      if (analysisType == pgsTypes::Envelope)
      {
         vSidewalkMaxReactions = pReactions->GetReaction(girderKey, vSupports, intervalIdx, pgsTypes::pftSidewalk, batSS, rtCumulative);
         vSidewalkMinReactions = pReactions->GetReaction(girderKey, vSupports, intervalIdx, pgsTypes::pftSidewalk, batCS, rtCumulative);
      }
      else
      {
         vSidewalkMaxReactions = pReactions->GetReaction(girderKey, vSupports, intervalIdx, pgsTypes::pftSidewalk, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, rtCumulative);
      }
   }

   if (analysisType == pgsTypes::Envelope)
   {
      vTrafficBarrierMaxReactions = pReactions->GetReaction(girderKey, vSupports, intervalIdx, pgsTypes::pftTrafficBarrier, batSS, rtCumulative);
      vTrafficBarrierMinReactions = pReactions->GetReaction(girderKey, vSupports, intervalIdx, pgsTypes::pftTrafficBarrier, batCS, rtCumulative);
   }
   else
   {
      vTrafficBarrierMaxReactions = pReactions->GetReaction(girderKey, vSupports, intervalIdx, pgsTypes::pftTrafficBarrier, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, rtCumulative);
   }
   
   if ( bOverlay )
   {
      if (analysisType == pgsTypes::Envelope)
      {
         vOverlayMaxReactions = pReactions->GetReaction(girderKey, vSupports, intervalIdx, pgsTypes::pftOverlay, batSS, rtCumulative);
         vOverlayMinReactions = pReactions->GetReaction(girderKey, vSupports, intervalIdx, pgsTypes::pftOverlay, batCS, rtCumulative);
      }
      else
      {
         vOverlayMaxReactions = pReactions->GetReaction(girderKey, vSupports, intervalIdx, pgsTypes::pftOverlay, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, rtCumulative);
      }
   }

   std::vector<REACTION> vMinPedLLReactionsSimpleSpan[3], vMaxPedLLReactionsSimpleSpan[3], vMinPedLLReactionsContinuousSpan[3], vMaxPedLLReactionsContinuousSpan[3];
   std::vector<VehicleIndexType> vMinPedLLVehicleSimpleSpan[3], vMaxPedLLVehicleSimpleSpan[3], vMinPedLLVehicleContinuousSpan[3], vMaxPedLLVehicleContinuousSpan[3];
   std::vector<REACTION> vMinLLReactionsSimpleSpan[3], vMaxLLReactionsSimpleSpan[3], vMinLLReactionsContinuousSpan[3], vMaxLLReactionsContinuousSpan[3];
   std::vector<VehicleIndexType> vMinLLVehicleSimpleSpan[3], vMaxLLVehicleSimpleSpan[3], vMinLLVehicleContinuousSpan[3], vMaxLLVehicleContinuousSpan[3];
   std::vector<REACTION> vMinFatigueReactionsSimpleSpan[3], vMaxFatigueReactionsSimpleSpan[3], vMinFatigueReactionsContinuousSpan[3], vMaxFatigueReactionsContinuousSpan[3];
   std::vector<VehicleIndexType> vMinFatigueVehicleSimpleSpan[3], vMaxFatigueVehicleSimpleSpan[3], vMinFatigueVehicleContinuousSpan[3], vMaxFatigueVehicleContinuousSpan[3];
   std::vector<REACTION> vMinPermitReactionsSimpleSpan[3], vMaxPermitReactionsSimpleSpan[3], vMinPermitReactionsContinuousSpan[3], vMaxPermitReactionsContinuousSpan[3];
   std::vector<VehicleIndexType> vMinPermitVehicleSimpleSpan[3], vMaxPermitVehicleSimpleSpan[3], vMinPermitVehicleContinuousSpan[3], vMaxPermitVehicleContinuousSpan[3];

   for ( int i = 0; i < 3; i++ )
   {
      pgsTypes::ForceEffectType fetPrimary = (pgsTypes::ForceEffectType)i;

      if (analysisType == pgsTypes::Envelope)
      {
         pReactions->GetLiveLoadReaction(intervalIdx, pgsTypes::lltDesign, vPiers, girderKey, batSS, true, false, fetPrimary, &vMinLLReactionsSimpleSpan[fetPrimary], &vMaxLLReactionsSimpleSpan[fetPrimary], &vMinLLVehicleSimpleSpan[fetPrimary], &vMaxLLVehicleSimpleSpan[fetPrimary]);
         pReactions->GetLiveLoadReaction(intervalIdx, pgsTypes::lltDesign, vPiers, girderKey, batCS, true, false, fetPrimary, &vMinLLReactionsContinuousSpan[fetPrimary], &vMaxLLReactionsContinuousSpan[fetPrimary], &vMinLLVehicleContinuousSpan[fetPrimary], &vMaxLLVehicleContinuousSpan[fetPrimary]);

         if (lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion())
         {
            pReactions->GetLiveLoadReaction(intervalIdx, pgsTypes::lltFatigue, vPiers, girderKey, batSS, true, false, fetPrimary, &vMinFatigueReactionsSimpleSpan[fetPrimary], &vMaxFatigueReactionsSimpleSpan[fetPrimary], &vMinFatigueVehicleSimpleSpan[fetPrimary], &vMaxFatigueVehicleSimpleSpan[fetPrimary]);
            pReactions->GetLiveLoadReaction(intervalIdx, pgsTypes::lltFatigue, vPiers, girderKey, batCS, true, false, fetPrimary, &vMinFatigueReactionsContinuousSpan[fetPrimary], &vMaxFatigueReactionsContinuousSpan[fetPrimary], &vMinFatigueVehicleContinuousSpan[fetPrimary], &vMaxFatigueVehicleContinuousSpan[fetPrimary]);
         }

         if (bPermit)
         {
            pReactions->GetLiveLoadReaction(intervalIdx, pgsTypes::lltPermit, vPiers, girderKey, batSS, true, false, fetPrimary, &vMinPermitReactionsSimpleSpan[fetPrimary], &vMaxPermitReactionsSimpleSpan[fetPrimary], &vMinPermitVehicleSimpleSpan[fetPrimary], &vMaxPermitVehicleSimpleSpan[fetPrimary]);
            pReactions->GetLiveLoadReaction(intervalIdx, pgsTypes::lltPermit, vPiers, girderKey, batCS, true, false, fetPrimary, &vMinPermitReactionsContinuousSpan[fetPrimary], &vMaxPermitReactionsContinuousSpan[fetPrimary], &vMinPermitVehicleContinuousSpan[fetPrimary], &vMaxPermitVehicleContinuousSpan[fetPrimary]);
         }

         if (bPedLoading)
         {
            // ped loads are funny. the LLDF is the loading so include it
            pReactions->GetLiveLoadReaction(intervalIdx, pgsTypes::lltPedestrian, vPiers, girderKey, batSS, true, true, fetPrimary, &vMinPedLLReactionsSimpleSpan[fetPrimary], &vMaxPedLLReactionsSimpleSpan[fetPrimary], &vMinPedLLVehicleSimpleSpan[fetPrimary], &vMaxPedLLVehicleSimpleSpan[fetPrimary]);
            pReactions->GetLiveLoadReaction(intervalIdx, pgsTypes::lltPedestrian, vPiers, girderKey, batCS, true, true, fetPrimary, &vMinPedLLReactionsContinuousSpan[fetPrimary], &vMaxPedLLReactionsContinuousSpan[fetPrimary], &vMinPedLLVehicleContinuousSpan[fetPrimary], &vMaxPedLLVehicleContinuousSpan[fetPrimary]);
         }
      }
      else
      {
         pReactions->GetLiveLoadReaction(intervalIdx, pgsTypes::lltDesign, vPiers, girderKey, batMax, true, false, fetPrimary, &vMinLLReactionsSimpleSpan[fetPrimary], &vMaxLLReactionsSimpleSpan[fetPrimary], &vMinLLVehicleSimpleSpan[fetPrimary], &vMaxLLVehicleSimpleSpan[fetPrimary]);
         pReactions->GetLiveLoadReaction(intervalIdx, pgsTypes::lltDesign, vPiers, girderKey, batMin, true, false, fetPrimary, &vMinLLReactionsContinuousSpan[fetPrimary], &vMaxLLReactionsContinuousSpan[fetPrimary], &vMinLLVehicleContinuousSpan[fetPrimary], &vMaxLLVehicleContinuousSpan[fetPrimary]);

         if (lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion())
         {
            pReactions->GetLiveLoadReaction(intervalIdx, pgsTypes::lltFatigue, vPiers, girderKey, batMax, true, false, fetPrimary, &vMinFatigueReactionsSimpleSpan[fetPrimary], &vMaxFatigueReactionsSimpleSpan[fetPrimary], &vMinFatigueVehicleSimpleSpan[fetPrimary], &vMaxFatigueVehicleSimpleSpan[fetPrimary]);
            pReactions->GetLiveLoadReaction(intervalIdx, pgsTypes::lltFatigue, vPiers, girderKey, batMin, true, false, fetPrimary, &vMinFatigueReactionsContinuousSpan[fetPrimary], &vMaxFatigueReactionsContinuousSpan[fetPrimary], &vMinFatigueVehicleContinuousSpan[fetPrimary], &vMaxFatigueVehicleContinuousSpan[fetPrimary]);
         }

         if (bPermit)
         {
            pReactions->GetLiveLoadReaction(intervalIdx, pgsTypes::lltPermit, vPiers, girderKey, batMax, true, false, fetPrimary, &vMinPermitReactionsSimpleSpan[fetPrimary], &vMaxPermitReactionsSimpleSpan[fetPrimary], &vMinPermitVehicleSimpleSpan[fetPrimary], &vMaxPermitVehicleSimpleSpan[fetPrimary]);
            pReactions->GetLiveLoadReaction(intervalIdx, pgsTypes::lltPermit, vPiers, girderKey, batMin, true, false, fetPrimary, &vMinPermitReactionsContinuousSpan[fetPrimary], &vMaxPermitReactionsContinuousSpan[fetPrimary], &vMinPermitVehicleContinuousSpan[fetPrimary], &vMaxPermitVehicleContinuousSpan[fetPrimary]);
         }

         if (bPedLoading)
         {
            // ped loads are funny. the LLDF is the loading so include it
            pReactions->GetLiveLoadReaction(intervalIdx, pgsTypes::lltPedestrian, vPiers, girderKey, batMax, true, true, fetPrimary, &vMinPedLLReactionsSimpleSpan[fetPrimary], &vMaxPedLLReactionsSimpleSpan[fetPrimary], &vMinPedLLVehicleSimpleSpan[fetPrimary], &vMaxPedLLVehicleSimpleSpan[fetPrimary]);
            pReactions->GetLiveLoadReaction(intervalIdx, pgsTypes::lltPedestrian, vPiers, girderKey, batMin, true, true, fetPrimary, &vMinPedLLReactionsContinuousSpan[fetPrimary], &vMaxPedLLReactionsContinuousSpan[fetPrimary], &vMinPedLLVehicleContinuousSpan[fetPrimary], &vMaxPedLLVehicleContinuousSpan[fetPrimary]);
         }
      }
   }


   RowIndexType row = p_table->GetNumberOfHeaderRows();
   for ( PierIndexType pierIdx = 0; pierIdx < nPiers; pierIdx++, row += 3 )
   {
      ColumnIndexType col = 0;
      p_table->SetRowSpan(row,col,3);
      (*p_table)(row,col++) << LABEL_PIER(pierIdx);

      p_table->SetRowSpan(row,col,3);
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

      if (bShearKey)
      {
         if (analysisType == pgsTypes::Envelope && bContinuousBeforeDeckCasting)
         {
            reaction = vShearKeyMaxReactions[pierIdx];
            (*p_table)(row, col) << force.SetValue(reaction.Fx);
            (*p_table)(row + 1, col) << force.SetValue(reaction.Fy);
            (*p_table)(row + 2, col) << moment.SetValue(reaction.Mz);
            col++;

            reaction = vShearKeyMinReactions[pierIdx];
            (*p_table)(row, col) << force.SetValue(reaction.Fx);
            (*p_table)(row + 1, col) << force.SetValue(reaction.Fy);
            (*p_table)(row + 2, col) << moment.SetValue(reaction.Mz);
            col++;
         }
         else
         {
            reaction = vShearKeyMaxReactions[pierIdx];
            (*p_table)(row, col) << force.SetValue(reaction.Fx);
            (*p_table)(row + 1, col) << force.SetValue(reaction.Fy);
            (*p_table)(row + 2, col) << moment.SetValue(reaction.Mz);
            col++;
         }
      }

      if (bLongitudinalJoint)
      {
         if (analysisType == pgsTypes::Envelope && bContinuousBeforeDeckCasting)
         {
            reaction = vLongitudinalJointMaxReactions[pierIdx];
            (*p_table)(row, col) << force.SetValue(reaction.Fx);
            (*p_table)(row + 1, col) << force.SetValue(reaction.Fy);
            (*p_table)(row + 2, col) << moment.SetValue(reaction.Mz);
            col++;

            reaction = vLongitudinalJointMinReactions[pierIdx];
            (*p_table)(row, col) << force.SetValue(reaction.Fx);
            (*p_table)(row + 1, col) << force.SetValue(reaction.Fy);
            (*p_table)(row + 2, col) << moment.SetValue(reaction.Mz);
            col++;
         }
         else
         {
            reaction = vLongitudinalJointMaxReactions[pierIdx];
            (*p_table)(row, col) << force.SetValue(reaction.Fx);
            (*p_table)(row + 1, col) << force.SetValue(reaction.Fy);
            (*p_table)(row + 2, col) << moment.SetValue(reaction.Mz);
            col++;
         }
      }

      if ( bConstruction )
      {
         if (analysisType == pgsTypes::Envelope && bContinuousBeforeDeckCasting)
         {
            reaction = vConstructionMaxReactions[pierIdx];
            (*p_table)(row, col) << force.SetValue(reaction.Fx);
            (*p_table)(row + 1, col) << force.SetValue(reaction.Fy);
            (*p_table)(row + 2, col) << moment.SetValue(reaction.Mz);
            col++;

            reaction = vConstructionMinReactions[pierIdx];
            (*p_table)(row, col) << force.SetValue(reaction.Fx);
            (*p_table)(row + 1, col) << force.SetValue(reaction.Fy);
            (*p_table)(row + 2, col) << moment.SetValue(reaction.Mz);
            col++;
         }
         else
         {
            reaction = vConstructionMaxReactions[pierIdx];
            (*p_table)(row, col) << force.SetValue(reaction.Fx);
            (*p_table)(row + 1, col) << force.SetValue(reaction.Fy);
            (*p_table)(row + 2, col) << moment.SetValue(reaction.Mz);
            col++;
         }
      }

      if (bDeck)
      {
         if (analysisType == pgsTypes::Envelope && bContinuousBeforeDeckCasting)
         {
            reaction = vSlabMaxReactions[pierIdx];
            (*p_table)(row, col) << force.SetValue(reaction.Fx);
            (*p_table)(row + 1, col) << force.SetValue(reaction.Fy);
            (*p_table)(row + 2, col) << moment.SetValue(reaction.Mz);
            col++;

            reaction = vSlabMinReactions[pierIdx];
            (*p_table)(row, col) << force.SetValue(reaction.Fx);
            (*p_table)(row + 1, col) << force.SetValue(reaction.Fy);
            (*p_table)(row + 2, col) << moment.SetValue(reaction.Mz);
            col++;

            reaction = vSlabPadMaxReactions[pierIdx];
            (*p_table)(row, col) << force.SetValue(reaction.Fx);
            (*p_table)(row + 1, col) << force.SetValue(reaction.Fy);
            (*p_table)(row + 2, col) << moment.SetValue(reaction.Mz);
            col++;

            reaction = vSlabPadMinReactions[pierIdx];
            (*p_table)(row, col) << force.SetValue(reaction.Fx);
            (*p_table)(row + 1, col) << force.SetValue(reaction.Fy);
            (*p_table)(row + 2, col) << moment.SetValue(reaction.Mz);
            col++;
         }
         else
         {
            reaction = vSlabMaxReactions[pierIdx];
            (*p_table)(row, col) << force.SetValue(reaction.Fx);
            (*p_table)(row + 1, col) << force.SetValue(reaction.Fy);
            (*p_table)(row + 2, col) << moment.SetValue(reaction.Mz);
            col++;

            reaction = vSlabPadMaxReactions[pierIdx];
            (*p_table)(row, col) << force.SetValue(reaction.Fx);
            (*p_table)(row + 1, col) << force.SetValue(reaction.Fy);
            (*p_table)(row + 2, col) << moment.SetValue(reaction.Mz);
            col++;
         }
      }

      if ( bDeckPanels )
      {
         if (analysisType == pgsTypes::Envelope && bContinuousBeforeDeckCasting)
         {
            reaction = vDeckPanelMaxReactions[pierIdx];
            (*p_table)(row, col) << force.SetValue(reaction.Fx);
            (*p_table)(row + 1, col) << force.SetValue(reaction.Fy);
            (*p_table)(row + 2, col) << moment.SetValue(reaction.Mz);
            col++;

            reaction = vDeckPanelMinReactions[pierIdx];
            (*p_table)(row, col) << force.SetValue(reaction.Fx);
            (*p_table)(row + 1, col) << force.SetValue(reaction.Fy);
            (*p_table)(row + 2, col) << moment.SetValue(reaction.Mz);
            col++;
         }
         else
         {
            reaction = vDeckPanelMaxReactions[pierIdx];
            (*p_table)(row, col) << force.SetValue(reaction.Fx);
            (*p_table)(row + 1, col) << force.SetValue(reaction.Fy);
            (*p_table)(row + 2, col) << moment.SetValue(reaction.Mz);
            col++;
         }
      }

      if ( bSidewalk )
      {
         if (analysisType == pgsTypes::Envelope)
         {
            auto maxReaction = vSidewalkMaxReactions[pierIdx];
            auto minReaction = vSidewalkMinReactions[pierIdx];
            if (maxReaction.Fy < minReaction.Fy)
            {
               std::swap(maxReaction, minReaction);
            }

            (*p_table)(row, col) << force.SetValue(maxReaction.Fx);
            (*p_table)(row + 1, col) << force.SetValue(maxReaction.Fy);
            (*p_table)(row + 2, col) << moment.SetValue(maxReaction.Mz);
            col++;

            (*p_table)(row, col) << force.SetValue(minReaction.Fx);
            (*p_table)(row + 1, col) << force.SetValue(minReaction.Fy);
            (*p_table)(row + 2, col) << moment.SetValue(minReaction.Mz);
            col++;
         }
         else
         {
            reaction = vSidewalkMaxReactions[pierIdx];
            (*p_table)(row, col) << force.SetValue(reaction.Fx);
            (*p_table)(row + 1, col) << force.SetValue(reaction.Fy);
            (*p_table)(row + 2, col) << moment.SetValue(reaction.Mz);
            col++;
         }
      }

      if (analysisType == pgsTypes::Envelope)
      {
         auto maxReaction = vTrafficBarrierMaxReactions[pierIdx];
         auto minReaction = vTrafficBarrierMinReactions[pierIdx];
         if (maxReaction.Fy < minReaction.Fy)
         {
            std::swap(maxReaction, minReaction);
         }

         (*p_table)(row, col) << force.SetValue(maxReaction.Fx);
         (*p_table)(row + 1, col) << force.SetValue(maxReaction.Fy);
         (*p_table)(row + 2, col) << moment.SetValue(maxReaction.Mz);
         col++;

         (*p_table)(row, col) << force.SetValue(minReaction.Fx);
         (*p_table)(row + 1, col) << force.SetValue(minReaction.Fy);
         (*p_table)(row + 2, col) << moment.SetValue(minReaction.Mz);
         col++;
      }
      else
      {
         reaction = vTrafficBarrierMaxReactions[pierIdx];
         (*p_table)(row, col) << force.SetValue(reaction.Fx);
         (*p_table)(row + 1, col) << force.SetValue(reaction.Fy);
         (*p_table)(row + 2, col) << moment.SetValue(reaction.Mz);
         col++;
      }

      if ( bOverlay )
      {
         if (analysisType == pgsTypes::Envelope)
         {
            auto maxReaction = vOverlayMaxReactions[pierIdx];
            auto minReaction = vOverlayMinReactions[pierIdx];
            if (maxReaction.Fy < minReaction.Fy)
            {
               std::swap(maxReaction, minReaction);
            }

            (*p_table)(row, col) << force.SetValue(maxReaction.Fx);
            (*p_table)(row + 1, col) << force.SetValue(maxReaction.Fy);
            (*p_table)(row + 2, col) << moment.SetValue(maxReaction.Mz);
            col++;

            (*p_table)(row, col) << force.SetValue(minReaction.Fx);
            (*p_table)(row + 1, col) << force.SetValue(minReaction.Fy);
            (*p_table)(row + 2, col) << moment.SetValue(minReaction.Mz);
            col++;
         }
         else
         {
            reaction = vOverlayMaxReactions[pierIdx];
            (*p_table)(row, col) << force.SetValue(reaction.Fx);
            (*p_table)(row + 1, col) << force.SetValue(reaction.Fy);
            (*p_table)(row + 2, col) << moment.SetValue(reaction.Mz);
            col++;
         }
      }

      ColumnIndexType startCol = col;
      for ( int i = 0; i < 3; i++ )
      {
         pgsTypes::ForceEffectType fetPrimary = (pgsTypes::ForceEffectType)i;

         IndexType maxLLVehicle, minLLVehicle;
         IndexType maxFatigueVehicle, minFatigueVehicle;
         IndexType maxPermitVehicle, minPermitVehicle;

         col = startCol + 2*i;
         ColumnIndexType colOffset = 0;
         if ( bPedLoading )
         {
            REACTION ssReaction = vMaxPedLLReactionsSimpleSpan[fetPrimary][pierIdx];
            REACTION csReaction = vMaxPedLLReactionsContinuousSpan[fetPrimary][pierIdx];
            if (fetPrimary == pgsTypes::fetFx)
            {
               reaction.Fx = Max(ssReaction.Fx, csReaction.Fx);
               reaction.Fy = MaxIndex(ssReaction.Fx, csReaction.Fx) == 0 ? ssReaction.Fy : csReaction.Fy;
               reaction.Mz = MaxIndex(ssReaction.Fx, csReaction.Fx) == 0 ? ssReaction.Mz : csReaction.Mz;
            }
            else if (fetPrimary == pgsTypes::fetFy)
            {
               reaction.Fy = Max(ssReaction.Fy, csReaction.Fy);
               reaction.Fx = MaxIndex(ssReaction.Fy, csReaction.Fy) == 0 ? ssReaction.Fx : csReaction.Fx;
               reaction.Mz = MaxIndex(ssReaction.Fy, csReaction.Fy) == 0 ? ssReaction.Mz : csReaction.Mz;
            }
            else
            {
               reaction.Mz = Max(ssReaction.Mz, csReaction.Mz);
               reaction.Fx = MaxIndex(ssReaction.Mz, csReaction.Mz) == 0 ? ssReaction.Fx : csReaction.Fx;
               reaction.Fy = MaxIndex(ssReaction.Mz, csReaction.Mz) == 0 ? ssReaction.Fy : csReaction.Fy;
            }

            (*p_table)(row,col)   << force.SetValue(reaction.Fx);
            (*p_table)(row+1,col) << force.SetValue(reaction.Fy);
            (*p_table)(row+2,col) << moment.SetValue(reaction.Mz);
            col++;

            ssReaction = vMinPedLLReactionsSimpleSpan[fetPrimary][pierIdx];
            csReaction = vMinPedLLReactionsContinuousSpan[fetPrimary][pierIdx];
            if (fetPrimary == pgsTypes::fetFx)
            {
               reaction.Fx = Min(ssReaction.Fx, csReaction.Fx);
               reaction.Fy = MinIndex(ssReaction.Fx, csReaction.Fx) == 0 ? ssReaction.Fy : csReaction.Fy;
               reaction.Mz = MinIndex(ssReaction.Fx, csReaction.Fx) == 0 ? ssReaction.Mz : csReaction.Mz;
            }
            else if (fetPrimary == pgsTypes::fetFy)
            {
               reaction.Fy = Min(ssReaction.Fy, csReaction.Fy);
               reaction.Fx = MinIndex(ssReaction.Fy, csReaction.Fy) == 0 ? ssReaction.Fx : csReaction.Fx;
               reaction.Mz = MinIndex(ssReaction.Fy, csReaction.Fy) == 0 ? ssReaction.Mz : csReaction.Mz;
            }
            else
            {
               reaction.Mz = Min(ssReaction.Mz, csReaction.Mz);
               reaction.Fx = MinIndex(ssReaction.Mz, csReaction.Mz) == 0 ? ssReaction.Fx : csReaction.Fx;
               reaction.Fy = MinIndex(ssReaction.Mz, csReaction.Mz) == 0 ? ssReaction.Fy : csReaction.Fy;
            }
            (*p_table)(row,col)   << force.SetValue(reaction.Fx);
            (*p_table)(row+1,col) << force.SetValue(reaction.Fy);
            (*p_table)(row+2,col) << moment.SetValue(reaction.Mz);
            col++;

            colOffset += 4;
         }

         REACTION ssReaction = vMaxLLReactionsSimpleSpan[fetPrimary][pierIdx];
         REACTION csReaction = vMaxLLReactionsContinuousSpan[fetPrimary][pierIdx];
         if (fetPrimary == pgsTypes::fetFx)
         {
            reaction.Fx = Max(ssReaction.Fx, csReaction.Fx);
            reaction.Fy = MaxIndex(ssReaction.Fx, csReaction.Fx) == 0 ? ssReaction.Fy : csReaction.Fy;
            reaction.Mz = MaxIndex(ssReaction.Fx, csReaction.Fx) == 0 ? ssReaction.Mz : csReaction.Mz;
            maxLLVehicle = MaxIndex(ssReaction.Fx, csReaction.Fx) == 0 ? vMaxLLVehicleSimpleSpan[fetPrimary][pierIdx] : vMaxLLVehicleContinuousSpan[fetPrimary][pierIdx];
         }
         else if (fetPrimary == pgsTypes::fetFy)
         {
            reaction.Fy = Max(ssReaction.Fy, csReaction.Fy);
            reaction.Fx = MaxIndex(ssReaction.Fy, csReaction.Fy) == 0 ? ssReaction.Fx : csReaction.Fx;
            reaction.Mz = MaxIndex(ssReaction.Fy, csReaction.Fy) == 0 ? ssReaction.Mz : csReaction.Mz;
            maxLLVehicle = MaxIndex(ssReaction.Fy, csReaction.Fy) == 0 ? vMaxLLVehicleSimpleSpan[fetPrimary][pierIdx] : vMaxLLVehicleContinuousSpan[fetPrimary][pierIdx];
         }
         else
         {
            reaction.Mz = Max(ssReaction.Mz, csReaction.Mz);
            reaction.Fx = MaxIndex(ssReaction.Mz, csReaction.Mz) == 0 ? ssReaction.Fx : csReaction.Fx;
            reaction.Fy = MaxIndex(ssReaction.Mz, csReaction.Mz) == 0 ? ssReaction.Fy : csReaction.Fy;
            maxLLVehicle = MaxIndex(ssReaction.Mz, csReaction.Mz) == 0 ? vMaxLLVehicleSimpleSpan[fetPrimary][pierIdx] : vMaxLLVehicleContinuousSpan[fetPrimary][pierIdx];
         }

         (*p_table)(row,col+colOffset)   << force.SetValue(reaction.Fx);
         (*p_table)(row,col+colOffset)   << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << maxLLVehicle << _T(")");
         (*p_table)(row+1,col+colOffset) << force.SetValue(reaction.Fy);
         (*p_table)(row+1,col+colOffset) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << maxLLVehicle << _T(")");
         (*p_table)(row+2,col+colOffset) << moment.SetValue(reaction.Mz);
         (*p_table)(row+2,col+colOffset) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << maxLLVehicle << _T(")");
         col++;

         ssReaction = vMinLLReactionsSimpleSpan[fetPrimary][pierIdx];
         csReaction = vMinLLReactionsContinuousSpan[fetPrimary][pierIdx];
         if (fetPrimary == pgsTypes::fetFx)
         {
            reaction.Fx = Min(ssReaction.Fx, csReaction.Fx);
            reaction.Fy = MinIndex(ssReaction.Fx, csReaction.Fx) == 0 ? ssReaction.Fy : csReaction.Fy;
            reaction.Mz = MinIndex(ssReaction.Fx, csReaction.Fx) == 0 ? ssReaction.Mz : csReaction.Mz;
            minLLVehicle = MinIndex(ssReaction.Fx, csReaction.Fx) == 0 ? vMinLLVehicleSimpleSpan[fetPrimary][pierIdx] : vMinLLVehicleContinuousSpan[fetPrimary][pierIdx];
         }
         else if (fetPrimary == pgsTypes::fetFy)
         {
            reaction.Fy = Min(ssReaction.Fy, csReaction.Fy);
            reaction.Fx = MinIndex(ssReaction.Fy, csReaction.Fy) == 0 ? ssReaction.Fx : csReaction.Fx;
            reaction.Mz = MinIndex(ssReaction.Fy, csReaction.Fy) == 0 ? ssReaction.Mz : csReaction.Mz;
            minLLVehicle = MinIndex(ssReaction.Fy, csReaction.Fy) == 0 ? vMinLLVehicleSimpleSpan[fetPrimary][pierIdx] : vMinLLVehicleContinuousSpan[fetPrimary][pierIdx];
         }
         else
         {
            reaction.Mz = Min(ssReaction.Mz, csReaction.Mz);
            reaction.Fx = MinIndex(ssReaction.Mz, csReaction.Mz) == 0 ? ssReaction.Fx : csReaction.Fx;
            reaction.Fy = MinIndex(ssReaction.Mz, csReaction.Mz) == 0 ? ssReaction.Fy : csReaction.Fy;
            minLLVehicle = MinIndex(ssReaction.Mz, csReaction.Mz) == 0 ? vMinLLVehicleSimpleSpan[fetPrimary][pierIdx] : vMinLLVehicleContinuousSpan[fetPrimary][pierIdx];
         }

         (*p_table)(row,col+colOffset)   << force.SetValue(reaction.Fx);
         (*p_table)(row,col+colOffset)   << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << minLLVehicle << _T(")");
         (*p_table)(row+1,col+colOffset) << force.SetValue(reaction.Fy);
         (*p_table)(row+1,col+colOffset) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << minLLVehicle << _T(")");
         (*p_table)(row+2,col+colOffset) << moment.SetValue(reaction.Mz);
         (*p_table)(row+2,col+colOffset) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << minLLVehicle << _T(")");
         col++;

         if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
         {
            colOffset += 4;

            ssReaction = vMaxFatigueReactionsSimpleSpan[fetPrimary][pierIdx];
            csReaction = vMaxFatigueReactionsContinuousSpan[fetPrimary][pierIdx];
            if (fetPrimary == pgsTypes::fetFx)
            {
               reaction.Fx = Max(ssReaction.Fx, csReaction.Fx);
               reaction.Fy = MaxIndex(ssReaction.Fx, csReaction.Fx) == 0 ? ssReaction.Fy : csReaction.Fy;
               reaction.Mz = MaxIndex(ssReaction.Fx, csReaction.Fx) == 0 ? ssReaction.Mz : csReaction.Mz;
               maxFatigueVehicle = MaxIndex(ssReaction.Fx, csReaction.Fx) == 0 ? vMaxFatigueVehicleSimpleSpan[fetPrimary][pierIdx] : vMaxFatigueVehicleContinuousSpan[fetPrimary][pierIdx];
            }
            else if (fetPrimary == pgsTypes::fetFy)
            {
               reaction.Fy = Max(ssReaction.Fy, csReaction.Fy);
               reaction.Fx = MaxIndex(ssReaction.Fy, csReaction.Fy) == 0 ? ssReaction.Fx : csReaction.Fx;
               reaction.Mz = MaxIndex(ssReaction.Fy, csReaction.Fy) == 0 ? ssReaction.Mz : csReaction.Mz;
               maxFatigueVehicle = MaxIndex(ssReaction.Fy, csReaction.Fy) == 0 ? vMaxFatigueVehicleSimpleSpan[fetPrimary][pierIdx] : vMaxFatigueVehicleContinuousSpan[fetPrimary][pierIdx];
            }
            else
            {
               reaction.Mz = Max(ssReaction.Mz, csReaction.Mz);
               reaction.Fx = MaxIndex(ssReaction.Mz, csReaction.Mz) == 0 ? ssReaction.Fx : csReaction.Fx;
               reaction.Fy = MaxIndex(ssReaction.Mz, csReaction.Mz) == 0 ? ssReaction.Fy : csReaction.Fy;
               maxFatigueVehicle = MaxIndex(ssReaction.Mz, csReaction.Mz) == 0 ? vMaxFatigueVehicleSimpleSpan[fetPrimary][pierIdx] : vMaxFatigueVehicleContinuousSpan[fetPrimary][pierIdx];
            }

            (*p_table)(row,col+colOffset)   << force.SetValue(reaction.Fx);
            (*p_table)(row,col+colOffset)   << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltFatigue) << maxFatigueVehicle << _T(")");
            (*p_table)(row+1,col+colOffset) << force.SetValue(reaction.Fy);
            (*p_table)(row+1,col+colOffset) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltFatigue) << maxFatigueVehicle << _T(")");
            (*p_table)(row+2,col+colOffset) << moment.SetValue(reaction.Mz);
            (*p_table)(row+2,col+colOffset) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltFatigue) << maxFatigueVehicle << _T(")");
            col++;

            ssReaction = vMinFatigueReactionsSimpleSpan[fetPrimary][pierIdx];
            csReaction = vMinFatigueReactionsContinuousSpan[fetPrimary][pierIdx];
            if (fetPrimary == pgsTypes::fetFx)
            {
               reaction.Fx = Min(ssReaction.Fx, csReaction.Fx);
               reaction.Fy = MinIndex(ssReaction.Fx, csReaction.Fx) == 0 ? ssReaction.Fy : csReaction.Fy;
               reaction.Mz = MinIndex(ssReaction.Fx, csReaction.Fx) == 0 ? ssReaction.Mz : csReaction.Mz;
               minFatigueVehicle = MinIndex(ssReaction.Fx, csReaction.Fx) == 0 ? vMinFatigueVehicleSimpleSpan[fetPrimary][pierIdx] : vMinFatigueVehicleContinuousSpan[fetPrimary][pierIdx];
            }
            else if (fetPrimary == pgsTypes::fetFy)
            {
               reaction.Fy = Min(ssReaction.Fy, csReaction.Fy);
               reaction.Fx = MinIndex(ssReaction.Fy, csReaction.Fy) == 0 ? ssReaction.Fx : csReaction.Fx;
               reaction.Mz = MinIndex(ssReaction.Fy, csReaction.Fy) == 0 ? ssReaction.Mz : csReaction.Mz;
               minFatigueVehicle = MinIndex(ssReaction.Fy, csReaction.Fy) == 0 ? vMinFatigueVehicleSimpleSpan[fetPrimary][pierIdx] : vMinFatigueVehicleContinuousSpan[fetPrimary][pierIdx];
            }
            else
            {
               reaction.Mz = Min(ssReaction.Mz, csReaction.Mz);
               reaction.Fx = MinIndex(ssReaction.Mz, csReaction.Mz) == 0 ? ssReaction.Fx : csReaction.Fx;
               reaction.Fy = MinIndex(ssReaction.Mz, csReaction.Mz) == 0 ? ssReaction.Fy : csReaction.Fy;
               minFatigueVehicle = MinIndex(ssReaction.Mz, csReaction.Mz) == 0 ? vMinFatigueVehicleSimpleSpan[fetPrimary][pierIdx] : vMinFatigueVehicleContinuousSpan[fetPrimary][pierIdx];
            }

            (*p_table)(row,col+colOffset)   << force.SetValue(reaction.Fx);
            (*p_table)(row,col+colOffset)   << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltFatigue) << minFatigueVehicle << _T(")");
            (*p_table)(row+1,col+colOffset) << force.SetValue(reaction.Fy);
            (*p_table)(row+1,col+colOffset) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltFatigue) << minFatigueVehicle << _T(")");
            (*p_table)(row+2,col+colOffset) << moment.SetValue(reaction.Mz);
            (*p_table)(row+2,col+colOffset) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltFatigue) << minFatigueVehicle << _T(")");
            col++;
         }


         if (bPermit)
         {
            colOffset += 4;

            ssReaction = vMaxPermitReactionsSimpleSpan[fetPrimary][pierIdx];
            csReaction = vMaxPermitReactionsContinuousSpan[fetPrimary][pierIdx];
            if (fetPrimary == pgsTypes::fetFx)
            {
               reaction.Fx = Max(ssReaction.Fx, csReaction.Fx);
               reaction.Fy = MaxIndex(ssReaction.Fx, csReaction.Fx) == 0 ? ssReaction.Fy : csReaction.Fy;
               reaction.Mz = MaxIndex(ssReaction.Fx, csReaction.Fx) == 0 ? ssReaction.Mz : csReaction.Mz;
               maxPermitVehicle = MaxIndex(ssReaction.Fx, csReaction.Fx) == 0 ? vMaxPermitVehicleSimpleSpan[fetPrimary][pierIdx] : vMaxPermitVehicleContinuousSpan[fetPrimary][pierIdx];
            }
            else if (fetPrimary == pgsTypes::fetFy)
            {
               reaction.Fy = Max(ssReaction.Fy, csReaction.Fy);
               reaction.Fx = MaxIndex(ssReaction.Fy, csReaction.Fy) == 0 ? ssReaction.Fx : csReaction.Fx;
               reaction.Mz = MaxIndex(ssReaction.Fy, csReaction.Fy) == 0 ? ssReaction.Mz : csReaction.Mz;
               maxPermitVehicle = MaxIndex(ssReaction.Fy, csReaction.Fy) == 0 ? vMaxPermitVehicleSimpleSpan[fetPrimary][pierIdx] : vMaxPermitVehicleContinuousSpan[fetPrimary][pierIdx];
            }
            else
            {
               reaction.Mz = Max(ssReaction.Mz, csReaction.Mz);
               reaction.Fx = MaxIndex(ssReaction.Mz, csReaction.Mz) == 0 ? ssReaction.Fx : csReaction.Fx;
               reaction.Fy = MaxIndex(ssReaction.Mz, csReaction.Mz) == 0 ? ssReaction.Fy : csReaction.Fy;
               maxPermitVehicle = MaxIndex(ssReaction.Mz, csReaction.Mz) == 0 ? vMaxPermitVehicleSimpleSpan[fetPrimary][pierIdx] : vMaxPermitVehicleContinuousSpan[fetPrimary][pierIdx];
            }

            (*p_table)(row, col + colOffset) << force.SetValue(reaction.Fx);
            (*p_table)(row, col + colOffset) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermit) << maxPermitVehicle << _T(")");
            (*p_table)(row + 1, col + colOffset) << force.SetValue(reaction.Fy);
            (*p_table)(row + 1, col + colOffset) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermit) << maxPermitVehicle << _T(")");
            (*p_table)(row + 2, col + colOffset) << moment.SetValue(reaction.Mz);
            (*p_table)(row + 2, col + colOffset) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermit) << maxPermitVehicle << _T(")");
            col++;

            ssReaction = vMinPermitReactionsSimpleSpan[fetPrimary][pierIdx];
            csReaction = vMinPermitReactionsContinuousSpan[fetPrimary][pierIdx];
            if (fetPrimary == pgsTypes::fetFx)
            {
               reaction.Fx = Min(ssReaction.Fx, csReaction.Fx);
               reaction.Fy = MinIndex(ssReaction.Fx, csReaction.Fx) == 0 ? ssReaction.Fy : csReaction.Fy;
               reaction.Mz = MinIndex(ssReaction.Fx, csReaction.Fx) == 0 ? ssReaction.Mz : csReaction.Mz;
               minPermitVehicle = MinIndex(ssReaction.Fx, csReaction.Fx) == 0 ? vMinPermitVehicleSimpleSpan[fetPrimary][pierIdx] : vMinPermitVehicleContinuousSpan[fetPrimary][pierIdx];
            }
            else if (fetPrimary == pgsTypes::fetFy)
            {
               reaction.Fy = Min(ssReaction.Fy, csReaction.Fy);
               reaction.Fx = MinIndex(ssReaction.Fy, csReaction.Fy) == 0 ? ssReaction.Fx : csReaction.Fx;
               reaction.Mz = MinIndex(ssReaction.Fy, csReaction.Fy) == 0 ? ssReaction.Mz : csReaction.Mz;
               minPermitVehicle = MinIndex(ssReaction.Fy, csReaction.Fy) == 0 ? vMinPermitVehicleSimpleSpan[fetPrimary][pierIdx] : vMinPermitVehicleContinuousSpan[fetPrimary][pierIdx];
            }
            else
            {
               reaction.Mz = Min(ssReaction.Mz, csReaction.Mz);
               reaction.Fx = MinIndex(ssReaction.Mz, csReaction.Mz) == 0 ? ssReaction.Fx : csReaction.Fx;
               reaction.Fy = MinIndex(ssReaction.Mz, csReaction.Mz) == 0 ? ssReaction.Fy : csReaction.Fy;
               minPermitVehicle = MinIndex(ssReaction.Mz, csReaction.Mz) == 0 ? vMinPermitVehicleSimpleSpan[fetPrimary][pierIdx] : vMinPermitVehicleContinuousSpan[fetPrimary][pierIdx];
            }

            (*p_table)(row, col + colOffset) << force.SetValue(reaction.Fx);
            (*p_table)(row, col + colOffset) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermit) << minPermitVehicle << _T(")");
            (*p_table)(row + 1, col + colOffset) << force.SetValue(reaction.Fy);
            (*p_table)(row + 1, col + colOffset) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermit) << minPermitVehicle << _T(")");
            (*p_table)(row + 2, col + colOffset) << moment.SetValue(reaction.Mz);
            (*p_table)(row + 2, col + colOffset) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermit) << minPermitVehicle << _T(")");
            col++;
         }
      }
   }

   GET_IFACE2(pBroker, IUserDefinedLoads, pUDL);
   bool bAreThereUserLoads = pUDL->DoUserLoadsExist(girderKey);
   if (bAreThereUserLoads)
   {
      *pPara << CUserReactionTable().Build(pBroker, girderKey, analysisType, PierReactionsTable, intervalIdx, pDisplayUnits) << rptNewLine;
   }

   return pChapter;
}

CChapterBuilder* CPierReactionChapterBuilder::Clone() const
{
   return new CPierReactionChapterBuilder;
}
