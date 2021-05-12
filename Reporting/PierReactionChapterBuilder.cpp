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

inline bool IsDifferentNumberOfGirdersPerSpan(IBridge* pBridge)
{
   GroupIndexType ngrps = pBridge->GetGirderGroupCount();
   GirderIndexType ngdrs = pBridge->GetGirderCount(0);
   for (GroupIndexType igrp = 1; igrp < ngrps; igrp++)
   {
      if (pBridge->GetGirderCount(igrp) != ngdrs)
         return true;
   }

   return false;
}

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

   GET_IFACE2(pBroker, IBridge, pBridge);
   GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);
   GET_IFACE2(pBroker, IIntervals, pIntervals);
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

   if (IsDifferentNumberOfGirdersPerSpan(pBridge))
   {
      (*pPara) << Bold(_T("This bridge is described with a different number of girders in each span.")) << rptNewLine;
      (*pPara) << _T("Plane frame analysis performed in accordance with LRFD 4.6.2.") << rptNewLine;
      (*pPara) << _T("Careful consideration should be taken when applying pier reactions below to substructure models.") << rptNewLine;
      (*pPara) << _T("Refer to the Structural Analysis Models and Reactions topics in the Technical Guide for more information.") << rptNewLine << rptNewLine;
   }

  pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pPara;
   *pPara << _T("Girder Line Reactions") << rptNewLine;

   pPara = new rptParagraph;
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

   rptRcTable* p_design_table = rptStyleManager::CreateDefaultTable(nCols, _T("Design"));
   rptRcTable* p_rating_table = rptStyleManager::CreateDefaultTable(33, _T("Load Rating"));

   std::array<rptRcTable*, 2> pTables{ p_design_table,p_rating_table };

   for (int i = 0; i < 2; i++)
   {
      pTables[i]->SetColumnStyle(0, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
      pTables[i]->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

      pTables[i]->SetColumnStyle(1, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
      pTables[i]->SetStripeRowColumnStyle(1, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

      pTables[i]->SetColumnStyle(2, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
      pTables[i]->SetStripeRowColumnStyle(2, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   ColumnIndexType col = 0;
   for (int i = 0; i < 2; i++)
   {
      col = 0;
      pTables[i]->SetNumberOfHeaderRows(2);
      pTables[i]->SetRowSpan(0, col, 2);
      (*pTables[i])(0, col++) << _T("Pier");

      pTables[i]->SetRowSpan(0, col, 2);
      (*pTables[i])(0, col++) << _T("Type");

      pTables[i]->SetRowSpan(0, col, 2);
      (*pTables[i])(0, col++) << _T("Reaction");
   }

   p_design_table->SetRowSpan(0, col, 2);
   (*p_design_table)(0, col++) << pProductLoads->GetProductLoadName(pgsTypes::pftGirder);

   p_design_table->SetRowSpan(0, col, 2);
   (*p_design_table)(0, col++) << pProductLoads->GetProductLoadName(pgsTypes::pftDiaphragm);

   if (bShearKey)
   {
      if (analysisType == pgsTypes::Envelope && bContinuousBeforeDeckCasting)
      {
         p_design_table->SetColumnSpan(0, col, 2);
         (*p_design_table)(0, col) << pProductLoads->GetProductLoadName(pgsTypes::pftShearKey);
         (*p_design_table)(1, col++) << _T("Max");
         (*p_design_table)(1, col++) << _T("Min");
      }
      else
      {
         p_design_table->SetRowSpan(0, col, 2);
         (*p_design_table)(0, col++) << pProductLoads->GetProductLoadName(pgsTypes::pftShearKey);
      }
   }

   if (bLongitudinalJoint)
   {
      if (analysisType == pgsTypes::Envelope && bContinuousBeforeDeckCasting)
      {
         p_design_table->SetColumnSpan(0, col, 2);
         (*p_design_table)(0, col) << pProductLoads->GetProductLoadName(pgsTypes::pftLongitudinalJoint);
         (*p_design_table)(1, col++) << _T("Max");
         (*p_design_table)(1, col++) << _T("Min");
      }
      else
      {
         p_design_table->SetRowSpan(0, col, 2);
         (*p_design_table)(0, col++) << pProductLoads->GetProductLoadName(pgsTypes::pftLongitudinalJoint);
      }
   }

   if (bConstruction)
   {
      if (analysisType == pgsTypes::Envelope && bContinuousBeforeDeckCasting)
      {
         p_design_table->SetColumnSpan(0, col, 2);
         (*p_design_table)(0, col) << pProductLoads->GetProductLoadName(pgsTypes::pftConstruction);
         (*p_design_table)(1, col++) << _T("Max");
         (*p_design_table)(1, col++) << _T("Min");
      }
      else
      {
         p_design_table->SetRowSpan(0, col, 2);
         (*p_design_table)(0, col++) << pProductLoads->GetProductLoadName(pgsTypes::pftConstruction);
      }
   }

   if (bDeck)
   {
      if (analysisType == pgsTypes::Envelope && bContinuousBeforeDeckCasting)
      {
         p_design_table->SetColumnSpan(0, col, 2);
         (*p_design_table)(0, col) << pProductLoads->GetProductLoadName(pgsTypes::pftSlab);
         (*p_design_table)(1, col++) << _T("Max");
         (*p_design_table)(1, col++) << _T("Min");

         p_design_table->SetColumnSpan(0, col, 2);
         (*p_design_table)(0, col) << pProductLoads->GetProductLoadName(pgsTypes::pftSlabPad);
         (*p_design_table)(1, col++) << _T("Max");
         (*p_design_table)(1, col++) << _T("Min");
      }
      else
      {
         p_design_table->SetRowSpan(0, col, 2);
         (*p_design_table)(0, col++) << pProductLoads->GetProductLoadName(pgsTypes::pftSlab);

         p_design_table->SetRowSpan(0, col, 2);
         (*p_design_table)(0, col++) << pProductLoads->GetProductLoadName(pgsTypes::pftSlabPad);
      }
   }

   if (bDeckPanels)
   {
      if (analysisType == pgsTypes::Envelope && bContinuousBeforeDeckCasting)
      {
         p_design_table->SetColumnSpan(0, col, 2);
         (*p_design_table)(0, col) << pProductLoads->GetProductLoadName(pgsTypes::pftSlabPanel);
         (*p_design_table)(1, col++) << _T("Max");
         (*p_design_table)(1, col++) << _T("Min");
      }
      else
      {
         p_design_table->SetRowSpan(0, col, 2);
         (*p_design_table)(0, col++) << pProductLoads->GetProductLoadName(pgsTypes::pftSlabPanel);
      }
   }

   if (bSidewalk)
   {
      if (analysisType == pgsTypes::Envelope)
      {
         p_design_table->SetColumnSpan(0, col, 2);
         (*p_design_table)(0, col) << pProductLoads->GetProductLoadName(pgsTypes::pftSidewalk);
         (*p_design_table)(1, col++) << _T("Max");
         (*p_design_table)(1, col++) << _T("Min");
      }
      else
      {
         p_design_table->SetRowSpan(0, col, 2);
         (*p_design_table)(0, col++) << pProductLoads->GetProductLoadName(pgsTypes::pftSidewalk);
      }
   }

   if (analysisType == pgsTypes::Envelope)
   {
      p_design_table->SetColumnSpan(0, col, 2);
      (*p_design_table)(0, col) << pProductLoads->GetProductLoadName(pgsTypes::pftTrafficBarrier);
      (*p_design_table)(1, col++) << _T("Max");
      (*p_design_table)(1, col++) << _T("Min");
   }
   else
   {
      p_design_table->SetRowSpan(0, col, 2);
      (*p_design_table)(0, col++) << pProductLoads->GetProductLoadName(pgsTypes::pftTrafficBarrier);
   }

   if (bOverlay)
   {
      if (analysisType == pgsTypes::Envelope)
      {
         p_design_table->SetColumnSpan(0, col, 2);
         if (bFutureOverlay)
         {
            (*p_design_table)(0, col) << _T("Future") << rptNewLine << pProductLoads->GetProductLoadName(pgsTypes::pftOverlay);
         }
         else
         {
            (*p_design_table)(0, col) << pProductLoads->GetProductLoadName(pgsTypes::pftOverlay);
         }
         (*p_design_table)(1, col++) << _T("Max");
         (*p_design_table)(1, col++) << _T("Min");
      }
      else
      {
         p_design_table->SetRowSpan(0, col, 2);
         if (bFutureOverlay)
         {
            (*p_design_table)(0, col++) << _T("Future") << rptNewLine << pProductLoads->GetProductLoadName(pgsTypes::pftOverlay);
         }
         else
         {
            (*p_design_table)(0, col++) << pProductLoads->GetProductLoadName(pgsTypes::pftOverlay);
         }
      }
   }

   if (bPedLoading)
   {
      p_design_table->SetColumnSpan(0, col, 2);
      (*p_design_table)(0, col) << _T("$ Pedestrian") << rptNewLine << _T("Optimize Fx");
      (*p_design_table)(1, col) << _T("Max");
      (*p_design_table)(1, col + 1) << _T("Min");
      col += 2;

      p_design_table->SetColumnSpan(0, col, 2);
      (*p_design_table)(0, col) << _T("$ Pedestrian") << rptNewLine << _T("Optimize Fy");
      (*p_design_table)(1, col) << _T("Max");
      (*p_design_table)(1, col + 1) << _T("Min");
      col += 2;

      p_design_table->SetColumnSpan(0, col, 2);
      (*p_design_table)(0, col) << _T("$ Pedestrian") << rptNewLine << _T("Optimize Mz");
      (*p_design_table)(1, col) << _T("Max");
      (*p_design_table)(1, col + 1) << _T("Min");
      col += 2;
   }

   p_design_table->SetColumnSpan(0, col, 2);
   (*p_design_table)(0, col) << _T("* Design Live Load") << rptNewLine << _T("Optimize Fx");
   (*p_design_table)(1, col) << _T("Max");
   (*p_design_table)(1, col + 1) << _T("Min");
   col += 2;

   p_design_table->SetColumnSpan(0, col, 2);
   (*p_design_table)(0, col) << _T("* Design Live Load") << rptNewLine << _T("Optimize Fy");
   (*p_design_table)(1, col) << _T("Max");
   (*p_design_table)(1, col + 1) << _T("Min");
   col += 2;

   p_design_table->SetColumnSpan(0, col, 2);
   (*p_design_table)(0, col) << _T("* Design Live Load") << rptNewLine << _T("Optimize Mz");
   (*p_design_table)(1, col) << _T("Max");
   (*p_design_table)(1, col + 1) << _T("Min");
   col += 2;

   if (lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion())
   {
      p_design_table->SetColumnSpan(0, col, 2);
      (*p_design_table)(0, col) << _T("* Fatigue Live Load") << rptNewLine << _T("Optimize Fx");
      (*p_design_table)(1, col) << _T("Max");
      (*p_design_table)(1, col + 1) << _T("Min");
      col += 2;

      p_design_table->SetColumnSpan(0, col, 2);
      (*p_design_table)(0, col) << _T("* Fatigue Live Load") << rptNewLine << _T("Optimize Fy");
      (*p_design_table)(1, col) << _T("Max");
      (*p_design_table)(1, col + 1) << _T("Min");
      col += 2;

      p_design_table->SetColumnSpan(0, col, 2);
      (*p_design_table)(0, col) << _T("* Fatigue Live Load") << rptNewLine << _T("Optimize Mz");
      (*p_design_table)(1, col) << _T("Max");
      (*p_design_table)(1, col + 1) << _T("Min");
      col += 2;
   }

   if (bPermit)
   {
      p_design_table->SetColumnSpan(0, col, 2);
      (*p_design_table)(0, col) << _T("* Permit Live Load") << rptNewLine << _T("Optimize Fx");
      (*p_design_table)(1, col) << _T("Max");
      (*p_design_table)(1, col + 1) << _T("Min");
      col += 2;

      p_design_table->SetColumnSpan(0, col, 2);
      (*p_design_table)(0, col) << _T("* Permit Live Load") << rptNewLine << _T("Optimize Fy");
      (*p_design_table)(1, col) << _T("Max");
      (*p_design_table)(1, col + 1) << _T("Min");
      col += 2;

      p_design_table->SetColumnSpan(0, col, 2);
      (*p_design_table)(0, col) << _T("* Permit Live Load") << rptNewLine << _T("Optimize Mz");
      (*p_design_table)(1, col) << _T("Max");
      (*p_design_table)(1, col + 1) << _T("Min");
      col += 2;
   }
   *pPara << p_design_table << rptNewLine;

   if (bPedLoading)
   {
      *pPara << _T("$ Pedestrian values are per girder") << rptNewLine;
   }

   *pPara << LIVELOAD_PER_LANE << rptNewLine;
   LiveLoadTableFooter(pBroker, pPara, girderKey, true, false);


   GET_IFACE2(pBroker, IUserDefinedLoads, pUDL);
   bool bAreThereUserLoads = pUDL->DoUserLoadsExist(girderKey);
   if (bAreThereUserLoads)
   {
      *pPara << CUserReactionTable().Build(pBroker, girderKey, analysisType, PierReactionsTable, intervalIdx, pDisplayUnits) << rptNewLine;
   }

   // rating table live load results columns
   col = 3;
   p_rating_table->SetColumnSpan(0, col, 2);
   (*p_rating_table)(0, col) << _T("* Legal Routine") << rptNewLine << _T("Optimize Fx");
   (*p_rating_table)(1, col) << _T("Max");
   (*p_rating_table)(1, col + 1) << _T("Min");
   col += 2;

   p_rating_table->SetColumnSpan(0, col, 2);
   (*p_rating_table)(0, col) << _T("* Legal Routine") << rptNewLine << _T("Optimize Fy");
   (*p_rating_table)(1, col) << _T("Max");
   (*p_rating_table)(1, col + 1) << _T("Min");
   col += 2;

   p_rating_table->SetColumnSpan(0, col, 2);
   (*p_rating_table)(0, col) << _T("* Legal Routine") << rptNewLine << _T("Optimize Mz");
   (*p_rating_table)(1, col) << _T("Max");
   (*p_rating_table)(1, col + 1) << _T("Min");
   col += 2;

   p_rating_table->SetColumnSpan(0, col, 2);
   (*p_rating_table)(0, col) << _T("* Legal Special") << rptNewLine << _T("Optimize Fx");
   (*p_rating_table)(1, col) << _T("Max");
   (*p_rating_table)(1, col + 1) << _T("Min");
   col += 2;

   p_rating_table->SetColumnSpan(0, col, 2);
   (*p_rating_table)(0, col) << _T("* Legal Special") << rptNewLine << _T("Optimize Fy");
   (*p_rating_table)(1, col) << _T("Max");
   (*p_rating_table)(1, col + 1) << _T("Min");
   col += 2;

   p_rating_table->SetColumnSpan(0, col, 2);
   (*p_rating_table)(0, col) << _T("* Legal Special") << rptNewLine << _T("Optimize Mz");
   (*p_rating_table)(1, col) << _T("Max");
   (*p_rating_table)(1, col + 1) << _T("Min");
   col += 2;

   p_rating_table->SetColumnSpan(0, col, 2);
   (*p_rating_table)(0, col) << _T("* Emergency") << rptNewLine << _T("Optimize Fx");
   (*p_rating_table)(1, col) << _T("Max");
   (*p_rating_table)(1, col + 1) << _T("Min");
   col += 2;

   p_rating_table->SetColumnSpan(0, col, 2);
   (*p_rating_table)(0, col) << _T("* Emergency") << rptNewLine << _T("Optimize Fy");
   (*p_rating_table)(1, col) << _T("Max");
   (*p_rating_table)(1, col + 1) << _T("Min");
   col += 2;

   p_rating_table->SetColumnSpan(0, col, 2);
   (*p_rating_table)(0, col) << _T("* Emergency") << rptNewLine << _T("Optimize Mz");
   (*p_rating_table)(1, col) << _T("Max");
   (*p_rating_table)(1, col + 1) << _T("Min");
   col += 2;

   p_rating_table->SetColumnSpan(0, col, 2);
   (*p_rating_table)(0, col) << _T("* Permit Routine") << rptNewLine << _T("Optimize Fx");
   (*p_rating_table)(1, col) << _T("Max");
   (*p_rating_table)(1, col + 1) << _T("Min");
   col += 2;

   p_rating_table->SetColumnSpan(0, col, 2);
   (*p_rating_table)(0, col) << _T("* Permit Routine") << rptNewLine << _T("Optimize Fy");
   (*p_rating_table)(1, col) << _T("Max");
   (*p_rating_table)(1, col + 1) << _T("Min");
   col += 2;

   p_rating_table->SetColumnSpan(0, col, 2);
   (*p_rating_table)(0, col) << _T("* Permit Routine") << rptNewLine << _T("Optimize Mz");
   (*p_rating_table)(1, col) << _T("Max");
   (*p_rating_table)(1, col + 1) << _T("Min");
   col += 2;

   p_rating_table->SetColumnSpan(0, col, 2);
   (*p_rating_table)(0, col) << _T("* Permit Special") << rptNewLine << _T("Optimize Fx");
   (*p_rating_table)(1, col) << _T("Max");
   (*p_rating_table)(1, col + 1) << _T("Min");
   col += 2;

   p_rating_table->SetColumnSpan(0, col, 2);
   (*p_rating_table)(0, col) << _T("* Permit Special") << rptNewLine << _T("Optimize Fy");
   (*p_rating_table)(1, col) << _T("Max");
   (*p_rating_table)(1, col + 1) << _T("Min");
   col += 2;

   p_rating_table->SetColumnSpan(0, col, 2);
   (*p_rating_table)(0, col) << _T("* Permit Special") << rptNewLine << _T("Optimize Mz");
   (*p_rating_table)(1, col) << _T("Max");
   (*p_rating_table)(1, col + 1) << _T("Min");
   col += 2;

   *pPara << p_rating_table << rptNewLine;
   *pPara << LIVELOAD_PER_LANE << rptNewLine;
   LiveLoadTableFooter(pBroker, pPara, girderKey, false, true);

   PierIndexType nPiers = pBridge->GetPierCount();
   std::vector<PierIndexType> vPiers;
   std::vector<std::pair<SupportIndexType, pgsTypes::SupportType>> vSupports;
   for (PierIndexType pierIdx = 0; pierIdx < nPiers; pierIdx++)
   {
      vPiers.push_back(pierIdx);
      vSupports.emplace_back(pierIdx, pgsTypes::stPier);
   }

   std::vector<REACTION> vGirderReactions, vDiaphragmReactions, vShearKeyMaxReactions, vShearKeyMinReactions, vLongitudinalJointMaxReactions, vLongitudinalJointMinReactions, vConstructionMaxReactions, vConstructionMinReactions, vSlabMaxReactions, vSlabMinReactions, vSlabPadMaxReactions, vSlabPadMinReactions, vDeckPanelMaxReactions, vDeckPanelMinReactions, vSidewalkMaxReactions, vSidewalkMinReactions, vTrafficBarrierMaxReactions, vTrafficBarrierMinReactions, vOverlayMaxReactions, vOverlayMinReactions;

   vGirderReactions = pReactions->GetReaction(girderKey, vSupports, intervalIdx, pgsTypes::pftGirder, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, rtCumulative);
   vDiaphragmReactions = pReactions->GetReaction(girderKey, vSupports, intervalIdx, pgsTypes::pftDiaphragm, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, rtCumulative);

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

   std::vector<REACTION> vMinLegalRoutineReactionsSimpleSpan[3], vMaxLegalRoutineReactionsSimpleSpan[3], vMinLegalRoutineReactionsContinuousSpan[3], vMaxLegalRoutineReactionsContinuousSpan[3];
   std::vector<VehicleIndexType> vMinLegalRoutineVehicleSimpleSpan[3], vMaxLegalRoutineVehicleSimpleSpan[3], vMinLegalRoutineVehicleContinuousSpan[3], vMaxLegalRoutineVehicleContinuousSpan[3];
   std::vector<REACTION> vMinLegalSpecialReactionsSimpleSpan[3], vMaxLegalSpecialReactionsSimpleSpan[3], vMinLegalSpecialReactionsContinuousSpan[3], vMaxLegalSpecialReactionsContinuousSpan[3];
   std::vector<VehicleIndexType> vMinLegalSpecialVehicleSimpleSpan[3], vMaxLegalSpecialVehicleSimpleSpan[3], vMinLegalSpecialVehicleContinuousSpan[3], vMaxLegalSpecialVehicleContinuousSpan[3];
   std::vector<REACTION> vMinLegalEmergencyReactionsSimpleSpan[3], vMaxLegalEmergencyReactionsSimpleSpan[3], vMinLegalEmergencyReactionsContinuousSpan[3], vMaxLegalEmergencyReactionsContinuousSpan[3];
   std::vector<VehicleIndexType> vMinLegalEmergencyVehicleSimpleSpan[3], vMaxLegalEmergencyVehicleSimpleSpan[3], vMinLegalEmergencyVehicleContinuousSpan[3], vMaxLegalEmergencyVehicleContinuousSpan[3];
   std::vector<REACTION> vMinPermitRoutineReactionsSimpleSpan[3], vMaxPermitRoutineReactionsSimpleSpan[3], vMinPermitRoutineReactionsContinuousSpan[3], vMaxPermitRoutineReactionsContinuousSpan[3];
   std::vector<VehicleIndexType> vMinPermitRoutineVehicleSimpleSpan[3], vMaxPermitRoutineVehicleSimpleSpan[3], vMinPermitRoutineVehicleContinuousSpan[3], vMaxPermitRoutineVehicleContinuousSpan[3];
   std::vector<REACTION> vMinPermitSpecialReactionsSimpleSpan[3], vMaxPermitSpecialReactionsSimpleSpan[3], vMinPermitSpecialReactionsContinuousSpan[3], vMaxPermitSpecialReactionsContinuousSpan[3];
   std::vector<VehicleIndexType> vMinPermitSpecialVehicleSimpleSpan[3], vMaxPermitSpecialVehicleSimpleSpan[3], vMinPermitSpecialVehicleContinuousSpan[3], vMaxPermitSpecialVehicleContinuousSpan[3];

   for ( int i = 0; i < 3; i++ )
   {
      pgsTypes::ForceEffectType fetPrimary = (pgsTypes::ForceEffectType)i;

      if (analysisType == pgsTypes::Envelope)
      {
         pReactions->GetLiveLoadReaction(intervalIdx, pgsTypes::lltDesign, vPiers, girderKey, batSS, true, fetPrimary, &vMinLLReactionsSimpleSpan[fetPrimary], &vMaxLLReactionsSimpleSpan[fetPrimary], &vMinLLVehicleSimpleSpan[fetPrimary], &vMaxLLVehicleSimpleSpan[fetPrimary]);
         pReactions->GetLiveLoadReaction(intervalIdx, pgsTypes::lltDesign, vPiers, girderKey, batCS, true, fetPrimary, &vMinLLReactionsContinuousSpan[fetPrimary], &vMaxLLReactionsContinuousSpan[fetPrimary], &vMinLLVehicleContinuousSpan[fetPrimary], &vMaxLLVehicleContinuousSpan[fetPrimary]);

         if (lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion())
         {
            pReactions->GetLiveLoadReaction(intervalIdx, pgsTypes::lltFatigue, vPiers, girderKey, batSS, true, fetPrimary, &vMinFatigueReactionsSimpleSpan[fetPrimary], &vMaxFatigueReactionsSimpleSpan[fetPrimary], &vMinFatigueVehicleSimpleSpan[fetPrimary], &vMaxFatigueVehicleSimpleSpan[fetPrimary]);
            pReactions->GetLiveLoadReaction(intervalIdx, pgsTypes::lltFatigue, vPiers, girderKey, batCS, true, fetPrimary, &vMinFatigueReactionsContinuousSpan[fetPrimary], &vMaxFatigueReactionsContinuousSpan[fetPrimary], &vMinFatigueVehicleContinuousSpan[fetPrimary], &vMaxFatigueVehicleContinuousSpan[fetPrimary]);
         }

         if (bPermit)
         {
            pReactions->GetLiveLoadReaction(intervalIdx, pgsTypes::lltPermit, vPiers, girderKey, batSS, true, fetPrimary, &vMinPermitReactionsSimpleSpan[fetPrimary], &vMaxPermitReactionsSimpleSpan[fetPrimary], &vMinPermitVehicleSimpleSpan[fetPrimary], &vMaxPermitVehicleSimpleSpan[fetPrimary]);
            pReactions->GetLiveLoadReaction(intervalIdx, pgsTypes::lltPermit, vPiers, girderKey, batCS, true, fetPrimary, &vMinPermitReactionsContinuousSpan[fetPrimary], &vMaxPermitReactionsContinuousSpan[fetPrimary], &vMinPermitVehicleContinuousSpan[fetPrimary], &vMaxPermitVehicleContinuousSpan[fetPrimary]);
         }

         if (bPedLoading)
         {
            // ped loads are funny. the LLDF is the loading so include it
            pReactions->GetLiveLoadReaction(intervalIdx, pgsTypes::lltPedestrian, vPiers, girderKey, batSS, true, fetPrimary, &vMinPedLLReactionsSimpleSpan[fetPrimary], &vMaxPedLLReactionsSimpleSpan[fetPrimary], &vMinPedLLVehicleSimpleSpan[fetPrimary], &vMaxPedLLVehicleSimpleSpan[fetPrimary]);
            pReactions->GetLiveLoadReaction(intervalIdx, pgsTypes::lltPedestrian, vPiers, girderKey, batCS, true, fetPrimary, &vMinPedLLReactionsContinuousSpan[fetPrimary], &vMaxPedLLReactionsContinuousSpan[fetPrimary], &vMinPedLLVehicleContinuousSpan[fetPrimary], &vMaxPedLLVehicleContinuousSpan[fetPrimary]);
         }

         pReactions->GetLiveLoadReaction(intervalIdx, pgsTypes::lltLegalRating_Routine, vPiers, girderKey, batSS, true, fetPrimary, &vMinLegalRoutineReactionsSimpleSpan[fetPrimary], &vMaxLegalRoutineReactionsSimpleSpan[fetPrimary], &vMinLegalRoutineVehicleSimpleSpan[fetPrimary], &vMaxLegalRoutineVehicleSimpleSpan[fetPrimary]);
         pReactions->GetLiveLoadReaction(intervalIdx, pgsTypes::lltLegalRating_Routine, vPiers, girderKey, batCS, true, fetPrimary, &vMinLegalRoutineReactionsContinuousSpan[fetPrimary], &vMaxLegalRoutineReactionsContinuousSpan[fetPrimary], &vMinLegalRoutineVehicleContinuousSpan[fetPrimary], &vMaxLegalRoutineVehicleContinuousSpan[fetPrimary]);
         pReactions->GetLiveLoadReaction(intervalIdx, pgsTypes::lltLegalRating_Special, vPiers, girderKey, batSS, true, fetPrimary, &vMinLegalSpecialReactionsSimpleSpan[fetPrimary], &vMaxLegalSpecialReactionsSimpleSpan[fetPrimary], &vMinLegalSpecialVehicleSimpleSpan[fetPrimary], &vMaxLegalSpecialVehicleSimpleSpan[fetPrimary]);
         pReactions->GetLiveLoadReaction(intervalIdx, pgsTypes::lltLegalRating_Special, vPiers, girderKey, batCS, true, fetPrimary, &vMinLegalSpecialReactionsContinuousSpan[fetPrimary], &vMaxLegalSpecialReactionsContinuousSpan[fetPrimary], &vMinLegalSpecialVehicleContinuousSpan[fetPrimary], &vMaxLegalSpecialVehicleContinuousSpan[fetPrimary]);
         pReactions->GetLiveLoadReaction(intervalIdx, pgsTypes::lltLegalRating_Emergency, vPiers, girderKey, batSS, true, fetPrimary, &vMinLegalEmergencyReactionsSimpleSpan[fetPrimary], &vMaxLegalEmergencyReactionsSimpleSpan[fetPrimary], &vMinLegalEmergencyVehicleSimpleSpan[fetPrimary], &vMaxLegalEmergencyVehicleSimpleSpan[fetPrimary]);
         pReactions->GetLiveLoadReaction(intervalIdx, pgsTypes::lltLegalRating_Emergency, vPiers, girderKey, batCS, true, fetPrimary, &vMinLegalEmergencyReactionsContinuousSpan[fetPrimary], &vMaxLegalEmergencyReactionsContinuousSpan[fetPrimary], &vMinLegalEmergencyVehicleContinuousSpan[fetPrimary], &vMaxLegalEmergencyVehicleContinuousSpan[fetPrimary]);
         pReactions->GetLiveLoadReaction(intervalIdx, pgsTypes::lltPermitRating_Routine, vPiers, girderKey, batSS, true, fetPrimary, &vMinPermitRoutineReactionsSimpleSpan[fetPrimary], &vMaxPermitRoutineReactionsSimpleSpan[fetPrimary], &vMinPermitRoutineVehicleSimpleSpan[fetPrimary], &vMaxPermitRoutineVehicleSimpleSpan[fetPrimary]);
         pReactions->GetLiveLoadReaction(intervalIdx, pgsTypes::lltPermitRating_Routine, vPiers, girderKey, batCS, true, fetPrimary, &vMinPermitRoutineReactionsContinuousSpan[fetPrimary], &vMaxPermitRoutineReactionsContinuousSpan[fetPrimary], &vMinPermitRoutineVehicleContinuousSpan[fetPrimary], &vMaxPermitRoutineVehicleContinuousSpan[fetPrimary]);
         pReactions->GetLiveLoadReaction(intervalIdx, pgsTypes::lltPermitRating_Special, vPiers, girderKey, batSS, true, fetPrimary, &vMinPermitSpecialReactionsSimpleSpan[fetPrimary], &vMaxPermitSpecialReactionsSimpleSpan[fetPrimary], &vMinPermitSpecialVehicleSimpleSpan[fetPrimary], &vMaxPermitSpecialVehicleSimpleSpan[fetPrimary]);
         pReactions->GetLiveLoadReaction(intervalIdx, pgsTypes::lltPermitRating_Special, vPiers, girderKey, batCS, true, fetPrimary, &vMinPermitSpecialReactionsContinuousSpan[fetPrimary], &vMaxPermitSpecialReactionsContinuousSpan[fetPrimary], &vMinPermitSpecialVehicleContinuousSpan[fetPrimary], &vMaxPermitSpecialVehicleContinuousSpan[fetPrimary]);
      }
      else
      {
         pReactions->GetLiveLoadReaction(intervalIdx, pgsTypes::lltDesign, vPiers, girderKey, batMax, true, fetPrimary, &vMinLLReactionsSimpleSpan[fetPrimary], &vMaxLLReactionsSimpleSpan[fetPrimary], &vMinLLVehicleSimpleSpan[fetPrimary], &vMaxLLVehicleSimpleSpan[fetPrimary]);
         pReactions->GetLiveLoadReaction(intervalIdx, pgsTypes::lltDesign, vPiers, girderKey, batMin, true, fetPrimary, &vMinLLReactionsContinuousSpan[fetPrimary], &vMaxLLReactionsContinuousSpan[fetPrimary], &vMinLLVehicleContinuousSpan[fetPrimary], &vMaxLLVehicleContinuousSpan[fetPrimary]);

         if (lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion())
         {
            pReactions->GetLiveLoadReaction(intervalIdx, pgsTypes::lltFatigue, vPiers, girderKey, batMax, true, fetPrimary, &vMinFatigueReactionsSimpleSpan[fetPrimary], &vMaxFatigueReactionsSimpleSpan[fetPrimary], &vMinFatigueVehicleSimpleSpan[fetPrimary], &vMaxFatigueVehicleSimpleSpan[fetPrimary]);
            pReactions->GetLiveLoadReaction(intervalIdx, pgsTypes::lltFatigue, vPiers, girderKey, batMin, true, fetPrimary, &vMinFatigueReactionsContinuousSpan[fetPrimary], &vMaxFatigueReactionsContinuousSpan[fetPrimary], &vMinFatigueVehicleContinuousSpan[fetPrimary], &vMaxFatigueVehicleContinuousSpan[fetPrimary]);
         }

         if (bPermit)
         {
            pReactions->GetLiveLoadReaction(intervalIdx, pgsTypes::lltPermit, vPiers, girderKey, batMax, true, fetPrimary, &vMinPermitReactionsSimpleSpan[fetPrimary], &vMaxPermitReactionsSimpleSpan[fetPrimary], &vMinPermitVehicleSimpleSpan[fetPrimary], &vMaxPermitVehicleSimpleSpan[fetPrimary]);
            pReactions->GetLiveLoadReaction(intervalIdx, pgsTypes::lltPermit, vPiers, girderKey, batMin, true, fetPrimary, &vMinPermitReactionsContinuousSpan[fetPrimary], &vMaxPermitReactionsContinuousSpan[fetPrimary], &vMinPermitVehicleContinuousSpan[fetPrimary], &vMaxPermitVehicleContinuousSpan[fetPrimary]);
         }

         if (bPedLoading)
         {
            // ped loads are funny. the LLDF is the loading so include it
            pReactions->GetLiveLoadReaction(intervalIdx, pgsTypes::lltPedestrian, vPiers, girderKey, batMax, true, fetPrimary, &vMinPedLLReactionsSimpleSpan[fetPrimary], &vMaxPedLLReactionsSimpleSpan[fetPrimary], &vMinPedLLVehicleSimpleSpan[fetPrimary], &vMaxPedLLVehicleSimpleSpan[fetPrimary]);
            pReactions->GetLiveLoadReaction(intervalIdx, pgsTypes::lltPedestrian, vPiers, girderKey, batMin, true, fetPrimary, &vMinPedLLReactionsContinuousSpan[fetPrimary], &vMaxPedLLReactionsContinuousSpan[fetPrimary], &vMinPedLLVehicleContinuousSpan[fetPrimary], &vMaxPedLLVehicleContinuousSpan[fetPrimary]);
         }

         pReactions->GetLiveLoadReaction(intervalIdx, pgsTypes::lltLegalRating_Routine, vPiers, girderKey, batMax, true, fetPrimary, &vMinLegalRoutineReactionsSimpleSpan[fetPrimary], &vMaxLegalRoutineReactionsSimpleSpan[fetPrimary], &vMinLegalRoutineVehicleSimpleSpan[fetPrimary], &vMaxLegalRoutineVehicleSimpleSpan[fetPrimary]);
         pReactions->GetLiveLoadReaction(intervalIdx, pgsTypes::lltLegalRating_Routine, vPiers, girderKey, batMin, true, fetPrimary, &vMinLegalRoutineReactionsContinuousSpan[fetPrimary], &vMaxLegalRoutineReactionsContinuousSpan[fetPrimary], &vMinLegalRoutineVehicleContinuousSpan[fetPrimary], &vMaxLegalRoutineVehicleContinuousSpan[fetPrimary]);
         pReactions->GetLiveLoadReaction(intervalIdx, pgsTypes::lltLegalRating_Special, vPiers, girderKey, batMax, true, fetPrimary, &vMinLegalSpecialReactionsSimpleSpan[fetPrimary], &vMaxLegalSpecialReactionsSimpleSpan[fetPrimary], &vMinLegalSpecialVehicleSimpleSpan[fetPrimary], &vMaxLegalSpecialVehicleSimpleSpan[fetPrimary]);
         pReactions->GetLiveLoadReaction(intervalIdx, pgsTypes::lltLegalRating_Special, vPiers, girderKey, batMin, true, fetPrimary, &vMinLegalSpecialReactionsContinuousSpan[fetPrimary], &vMaxLegalSpecialReactionsContinuousSpan[fetPrimary], &vMinLegalSpecialVehicleContinuousSpan[fetPrimary], &vMaxLegalSpecialVehicleContinuousSpan[fetPrimary]);

         pReactions->GetLiveLoadReaction(intervalIdx, pgsTypes::lltLegalRating_Emergency, vPiers, girderKey, batMax, true, fetPrimary, &vMinLegalEmergencyReactionsSimpleSpan[fetPrimary], &vMaxLegalEmergencyReactionsSimpleSpan[fetPrimary], &vMinLegalEmergencyVehicleSimpleSpan[fetPrimary], &vMaxLegalEmergencyVehicleSimpleSpan[fetPrimary]);
         pReactions->GetLiveLoadReaction(intervalIdx, pgsTypes::lltLegalRating_Emergency, vPiers, girderKey, batMin, true, fetPrimary, &vMinLegalEmergencyReactionsContinuousSpan[fetPrimary], &vMaxLegalEmergencyReactionsContinuousSpan[fetPrimary], &vMinLegalEmergencyVehicleContinuousSpan[fetPrimary], &vMaxLegalEmergencyVehicleContinuousSpan[fetPrimary]);

         pReactions->GetLiveLoadReaction(intervalIdx, pgsTypes::lltPermitRating_Routine, vPiers, girderKey, batMax, true, fetPrimary, &vMinPermitRoutineReactionsSimpleSpan[fetPrimary], &vMaxPermitRoutineReactionsSimpleSpan[fetPrimary], &vMinPermitRoutineVehicleSimpleSpan[fetPrimary], &vMaxPermitRoutineVehicleSimpleSpan[fetPrimary]);
         pReactions->GetLiveLoadReaction(intervalIdx, pgsTypes::lltPermitRating_Routine, vPiers, girderKey, batMin, true, fetPrimary, &vMinPermitRoutineReactionsContinuousSpan[fetPrimary], &vMaxPermitRoutineReactionsContinuousSpan[fetPrimary], &vMinPermitRoutineVehicleContinuousSpan[fetPrimary], &vMaxPermitRoutineVehicleContinuousSpan[fetPrimary]);
         pReactions->GetLiveLoadReaction(intervalIdx, pgsTypes::lltPermitRating_Special, vPiers, girderKey, batMax, true, fetPrimary, &vMinPermitSpecialReactionsSimpleSpan[fetPrimary], &vMaxPermitSpecialReactionsSimpleSpan[fetPrimary], &vMinPermitSpecialVehicleSimpleSpan[fetPrimary], &vMaxPermitSpecialVehicleSimpleSpan[fetPrimary]);
         pReactions->GetLiveLoadReaction(intervalIdx, pgsTypes::lltPermitRating_Special, vPiers, girderKey, batMin, true, fetPrimary, &vMinPermitSpecialReactionsContinuousSpan[fetPrimary], &vMaxPermitSpecialReactionsContinuousSpan[fetPrimary], &vMinPermitSpecialVehicleContinuousSpan[fetPrimary], &vMaxPermitSpecialVehicleContinuousSpan[fetPrimary]);
      }
   }


   RowIndexType row = p_design_table->GetNumberOfHeaderRows();
   for ( PierIndexType pierIdx = 0; pierIdx < nPiers; pierIdx++, row += 3 )
   {
      ColumnIndexType col = 0;
      for (int i = 0; i < 2; i++)
      {
         col = 0;
         pTables[i]->SetRowSpan(row, col, 3);
         (*pTables[i])(row, col++) << LABEL_PIER(pierIdx);

         pTables[i]->SetRowSpan(row, col, 3);
         if (pBridge->GetPierModelType(pierIdx) == pgsTypes::pmtIdealized)
         {
            (*pTables[i])(row, col++) << _T("Idealized");
         }
         else
         {
            (*pTables[i])(row, col++) << _T("Physical");
         }

         (*pTables[i])(row, col) << _T("Fx (") << pDisplayUnits->GetGeneralForceUnit().UnitOfMeasure.UnitTag() << _T(")");
         (*pTables[i])(row + 1, col) << _T("Fy (") << pDisplayUnits->GetGeneralForceUnit().UnitOfMeasure.UnitTag() << _T(")");
         (*pTables[i])(row + 2, col) << _T("Mz (") << pDisplayUnits->GetMomentUnit().UnitOfMeasure.UnitTag() << _T(")");
         col++;
      }

      REACTION reaction = vGirderReactions[pierIdx];
      (*p_design_table)(row,col)   << force.SetValue(reaction.Fx);
      (*p_design_table)(row+1,col) << force.SetValue(reaction.Fy);
      (*p_design_table)(row+2,col) << moment.SetValue(reaction.Mz);
      col++;

      reaction = vDiaphragmReactions[pierIdx];
      (*p_design_table)(row,col)   << force.SetValue(reaction.Fx);
      (*p_design_table)(row+1,col) << force.SetValue(reaction.Fy);
      (*p_design_table)(row+2,col) << moment.SetValue(reaction.Mz);
      col++;

      if (bShearKey)
      {
         if (analysisType == pgsTypes::Envelope && bContinuousBeforeDeckCasting)
         {
            reaction = vShearKeyMaxReactions[pierIdx];
            (*p_design_table)(row, col) << force.SetValue(reaction.Fx);
            (*p_design_table)(row + 1, col) << force.SetValue(reaction.Fy);
            (*p_design_table)(row + 2, col) << moment.SetValue(reaction.Mz);
            col++;

            reaction = vShearKeyMinReactions[pierIdx];
            (*p_design_table)(row, col) << force.SetValue(reaction.Fx);
            (*p_design_table)(row + 1, col) << force.SetValue(reaction.Fy);
            (*p_design_table)(row + 2, col) << moment.SetValue(reaction.Mz);
            col++;
         }
         else
         {
            reaction = vShearKeyMaxReactions[pierIdx];
            (*p_design_table)(row, col) << force.SetValue(reaction.Fx);
            (*p_design_table)(row + 1, col) << force.SetValue(reaction.Fy);
            (*p_design_table)(row + 2, col) << moment.SetValue(reaction.Mz);
            col++;
         }
      }

      if (bLongitudinalJoint)
      {
         if (analysisType == pgsTypes::Envelope && bContinuousBeforeDeckCasting)
         {
            reaction = vLongitudinalJointMaxReactions[pierIdx];
            (*p_design_table)(row, col) << force.SetValue(reaction.Fx);
            (*p_design_table)(row + 1, col) << force.SetValue(reaction.Fy);
            (*p_design_table)(row + 2, col) << moment.SetValue(reaction.Mz);
            col++;

            reaction = vLongitudinalJointMinReactions[pierIdx];
            (*p_design_table)(row, col) << force.SetValue(reaction.Fx);
            (*p_design_table)(row + 1, col) << force.SetValue(reaction.Fy);
            (*p_design_table)(row + 2, col) << moment.SetValue(reaction.Mz);
            col++;
         }
         else
         {
            reaction = vLongitudinalJointMaxReactions[pierIdx];
            (*p_design_table)(row, col) << force.SetValue(reaction.Fx);
            (*p_design_table)(row + 1, col) << force.SetValue(reaction.Fy);
            (*p_design_table)(row + 2, col) << moment.SetValue(reaction.Mz);
            col++;
         }
      }

      if ( bConstruction )
      {
         if (analysisType == pgsTypes::Envelope && bContinuousBeforeDeckCasting)
         {
            reaction = vConstructionMaxReactions[pierIdx];
            (*p_design_table)(row, col) << force.SetValue(reaction.Fx);
            (*p_design_table)(row + 1, col) << force.SetValue(reaction.Fy);
            (*p_design_table)(row + 2, col) << moment.SetValue(reaction.Mz);
            col++;

            reaction = vConstructionMinReactions[pierIdx];
            (*p_design_table)(row, col) << force.SetValue(reaction.Fx);
            (*p_design_table)(row + 1, col) << force.SetValue(reaction.Fy);
            (*p_design_table)(row + 2, col) << moment.SetValue(reaction.Mz);
            col++;
         }
         else
         {
            reaction = vConstructionMaxReactions[pierIdx];
            (*p_design_table)(row, col) << force.SetValue(reaction.Fx);
            (*p_design_table)(row + 1, col) << force.SetValue(reaction.Fy);
            (*p_design_table)(row + 2, col) << moment.SetValue(reaction.Mz);
            col++;
         }
      }

      if (bDeck)
      {
         if (analysisType == pgsTypes::Envelope && bContinuousBeforeDeckCasting)
         {
            reaction = vSlabMaxReactions[pierIdx];
            (*p_design_table)(row, col) << force.SetValue(reaction.Fx);
            (*p_design_table)(row + 1, col) << force.SetValue(reaction.Fy);
            (*p_design_table)(row + 2, col) << moment.SetValue(reaction.Mz);
            col++;

            reaction = vSlabMinReactions[pierIdx];
            (*p_design_table)(row, col) << force.SetValue(reaction.Fx);
            (*p_design_table)(row + 1, col) << force.SetValue(reaction.Fy);
            (*p_design_table)(row + 2, col) << moment.SetValue(reaction.Mz);
            col++;

            reaction = vSlabPadMaxReactions[pierIdx];
            (*p_design_table)(row, col) << force.SetValue(reaction.Fx);
            (*p_design_table)(row + 1, col) << force.SetValue(reaction.Fy);
            (*p_design_table)(row + 2, col) << moment.SetValue(reaction.Mz);
            col++;

            reaction = vSlabPadMinReactions[pierIdx];
            (*p_design_table)(row, col) << force.SetValue(reaction.Fx);
            (*p_design_table)(row + 1, col) << force.SetValue(reaction.Fy);
            (*p_design_table)(row + 2, col) << moment.SetValue(reaction.Mz);
            col++;
         }
         else
         {
            reaction = vSlabMaxReactions[pierIdx];
            (*p_design_table)(row, col) << force.SetValue(reaction.Fx);
            (*p_design_table)(row + 1, col) << force.SetValue(reaction.Fy);
            (*p_design_table)(row + 2, col) << moment.SetValue(reaction.Mz);
            col++;

            reaction = vSlabPadMaxReactions[pierIdx];
            (*p_design_table)(row, col) << force.SetValue(reaction.Fx);
            (*p_design_table)(row + 1, col) << force.SetValue(reaction.Fy);
            (*p_design_table)(row + 2, col) << moment.SetValue(reaction.Mz);
            col++;
         }
      }

      if ( bDeckPanels )
      {
         if (analysisType == pgsTypes::Envelope && bContinuousBeforeDeckCasting)
         {
            reaction = vDeckPanelMaxReactions[pierIdx];
            (*p_design_table)(row, col) << force.SetValue(reaction.Fx);
            (*p_design_table)(row + 1, col) << force.SetValue(reaction.Fy);
            (*p_design_table)(row + 2, col) << moment.SetValue(reaction.Mz);
            col++;

            reaction = vDeckPanelMinReactions[pierIdx];
            (*p_design_table)(row, col) << force.SetValue(reaction.Fx);
            (*p_design_table)(row + 1, col) << force.SetValue(reaction.Fy);
            (*p_design_table)(row + 2, col) << moment.SetValue(reaction.Mz);
            col++;
         }
         else
         {
            reaction = vDeckPanelMaxReactions[pierIdx];
            (*p_design_table)(row, col) << force.SetValue(reaction.Fx);
            (*p_design_table)(row + 1, col) << force.SetValue(reaction.Fy);
            (*p_design_table)(row + 2, col) << moment.SetValue(reaction.Mz);
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

            (*p_design_table)(row, col) << force.SetValue(maxReaction.Fx);
            (*p_design_table)(row + 1, col) << force.SetValue(maxReaction.Fy);
            (*p_design_table)(row + 2, col) << moment.SetValue(maxReaction.Mz);
            col++;

            (*p_design_table)(row, col) << force.SetValue(minReaction.Fx);
            (*p_design_table)(row + 1, col) << force.SetValue(minReaction.Fy);
            (*p_design_table)(row + 2, col) << moment.SetValue(minReaction.Mz);
            col++;
         }
         else
         {
            reaction = vSidewalkMaxReactions[pierIdx];
            (*p_design_table)(row, col) << force.SetValue(reaction.Fx);
            (*p_design_table)(row + 1, col) << force.SetValue(reaction.Fy);
            (*p_design_table)(row + 2, col) << moment.SetValue(reaction.Mz);
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

         (*p_design_table)(row, col) << force.SetValue(maxReaction.Fx);
         (*p_design_table)(row + 1, col) << force.SetValue(maxReaction.Fy);
         (*p_design_table)(row + 2, col) << moment.SetValue(maxReaction.Mz);
         col++;

         (*p_design_table)(row, col) << force.SetValue(minReaction.Fx);
         (*p_design_table)(row + 1, col) << force.SetValue(minReaction.Fy);
         (*p_design_table)(row + 2, col) << moment.SetValue(minReaction.Mz);
         col++;
      }
      else
      {
         reaction = vTrafficBarrierMaxReactions[pierIdx];
         (*p_design_table)(row, col) << force.SetValue(reaction.Fx);
         (*p_design_table)(row + 1, col) << force.SetValue(reaction.Fy);
         (*p_design_table)(row + 2, col) << moment.SetValue(reaction.Mz);
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

            (*p_design_table)(row, col) << force.SetValue(maxReaction.Fx);
            (*p_design_table)(row + 1, col) << force.SetValue(maxReaction.Fy);
            (*p_design_table)(row + 2, col) << moment.SetValue(maxReaction.Mz);
            col++;

            (*p_design_table)(row, col) << force.SetValue(minReaction.Fx);
            (*p_design_table)(row + 1, col) << force.SetValue(minReaction.Fy);
            (*p_design_table)(row + 2, col) << moment.SetValue(minReaction.Mz);
            col++;
         }
         else
         {
            reaction = vOverlayMaxReactions[pierIdx];
            (*p_design_table)(row, col) << force.SetValue(reaction.Fx);
            (*p_design_table)(row + 1, col) << force.SetValue(reaction.Fy);
            (*p_design_table)(row + 2, col) << moment.SetValue(reaction.Mz);
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

            (*p_design_table)(row,col)   << force.SetValue(reaction.Fx);
            (*p_design_table)(row+1,col) << force.SetValue(reaction.Fy);
            (*p_design_table)(row+2,col) << moment.SetValue(reaction.Mz);
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
            (*p_design_table)(row,col)   << force.SetValue(reaction.Fx);
            (*p_design_table)(row+1,col) << force.SetValue(reaction.Fy);
            (*p_design_table)(row+2,col) << moment.SetValue(reaction.Mz);
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

         (*p_design_table)(row,col+colOffset)   << force.SetValue(reaction.Fx);
         (*p_design_table)(row,col+colOffset)   << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << maxLLVehicle << _T(")");
         (*p_design_table)(row+1,col+colOffset) << force.SetValue(reaction.Fy);
         (*p_design_table)(row+1,col+colOffset) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << maxLLVehicle << _T(")");
         (*p_design_table)(row+2,col+colOffset) << moment.SetValue(reaction.Mz);
         (*p_design_table)(row+2,col+colOffset) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << maxLLVehicle << _T(")");
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

         (*p_design_table)(row,col+colOffset)   << force.SetValue(reaction.Fx);
         (*p_design_table)(row,col+colOffset)   << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << minLLVehicle << _T(")");
         (*p_design_table)(row+1,col+colOffset) << force.SetValue(reaction.Fy);
         (*p_design_table)(row+1,col+colOffset) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << minLLVehicle << _T(")");
         (*p_design_table)(row+2,col+colOffset) << moment.SetValue(reaction.Mz);
         (*p_design_table)(row+2,col+colOffset) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << minLLVehicle << _T(")");
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

            (*p_design_table)(row,col+colOffset)   << force.SetValue(reaction.Fx);
            (*p_design_table)(row,col+colOffset)   << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltFatigue) << maxFatigueVehicle << _T(")");
            (*p_design_table)(row+1,col+colOffset) << force.SetValue(reaction.Fy);
            (*p_design_table)(row+1,col+colOffset) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltFatigue) << maxFatigueVehicle << _T(")");
            (*p_design_table)(row+2,col+colOffset) << moment.SetValue(reaction.Mz);
            (*p_design_table)(row+2,col+colOffset) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltFatigue) << maxFatigueVehicle << _T(")");
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

            (*p_design_table)(row,col+colOffset)   << force.SetValue(reaction.Fx);
            (*p_design_table)(row,col+colOffset)   << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltFatigue) << minFatigueVehicle << _T(")");
            (*p_design_table)(row+1,col+colOffset) << force.SetValue(reaction.Fy);
            (*p_design_table)(row+1,col+colOffset) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltFatigue) << minFatigueVehicle << _T(")");
            (*p_design_table)(row+2,col+colOffset) << moment.SetValue(reaction.Mz);
            (*p_design_table)(row+2,col+colOffset) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltFatigue) << minFatigueVehicle << _T(")");
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

            (*p_design_table)(row, col + colOffset) << force.SetValue(reaction.Fx);
            (*p_design_table)(row, col + colOffset) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermit) << maxPermitVehicle << _T(")");
            (*p_design_table)(row + 1, col + colOffset) << force.SetValue(reaction.Fy);
            (*p_design_table)(row + 1, col + colOffset) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermit) << maxPermitVehicle << _T(")");
            (*p_design_table)(row + 2, col + colOffset) << moment.SetValue(reaction.Mz);
            (*p_design_table)(row + 2, col + colOffset) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermit) << maxPermitVehicle << _T(")");
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

            (*p_design_table)(row, col + colOffset) << force.SetValue(reaction.Fx);
            (*p_design_table)(row, col + colOffset) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermit) << minPermitVehicle << _T(")");
            (*p_design_table)(row + 1, col + colOffset) << force.SetValue(reaction.Fy);
            (*p_design_table)(row + 1, col + colOffset) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermit) << minPermitVehicle << _T(")");
            (*p_design_table)(row + 2, col + colOffset) << moment.SetValue(reaction.Mz);
            (*p_design_table)(row + 2, col + colOffset) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermit) << minPermitVehicle << _T(")");
            col++;
         }
      }

      // rating

      startCol = 3;
      for (int i = 0; i < 3; i++)
      {
         pgsTypes::ForceEffectType fetPrimary = (pgsTypes::ForceEffectType)i;

         IndexType maxLegalRoutineVehicle, minLegalRoutineVehicle;
         IndexType maxLegalSpecialVehicle, minLegalSpecialVehicle;
         IndexType maxLegalEmergencyVehicle, minLegalEmergencyVehicle;
         IndexType maxPermitRoutineVehicle, minPermitRoutineVehicle;
         IndexType maxPermitSpecialVehicle, minPermitSpecialVehicle;

         col = startCol + 2 * i;
         ColumnIndexType colOffset = 0;

         REACTION ssReaction = vMaxLegalRoutineReactionsSimpleSpan[fetPrimary][pierIdx];
         REACTION csReaction = vMaxLegalRoutineReactionsContinuousSpan[fetPrimary][pierIdx];
         if (fetPrimary == pgsTypes::fetFx)
         {
            reaction.Fx = Max(ssReaction.Fx, csReaction.Fx);
            reaction.Fy = MaxIndex(ssReaction.Fx, csReaction.Fx) == 0 ? ssReaction.Fy : csReaction.Fy;
            reaction.Mz = MaxIndex(ssReaction.Fx, csReaction.Fx) == 0 ? ssReaction.Mz : csReaction.Mz;
            maxLegalRoutineVehicle = MaxIndex(ssReaction.Fx, csReaction.Fx) == 0 ? vMaxLegalRoutineVehicleSimpleSpan[fetPrimary][pierIdx] : vMaxLegalRoutineVehicleContinuousSpan[fetPrimary][pierIdx];
         }
         else if (fetPrimary == pgsTypes::fetFy)
         {
            reaction.Fy = Max(ssReaction.Fy, csReaction.Fy);
            reaction.Fx = MaxIndex(ssReaction.Fy, csReaction.Fy) == 0 ? ssReaction.Fx : csReaction.Fx;
            reaction.Mz = MaxIndex(ssReaction.Fy, csReaction.Fy) == 0 ? ssReaction.Mz : csReaction.Mz;
            maxLegalRoutineVehicle = MaxIndex(ssReaction.Fy, csReaction.Fy) == 0 ? vMaxLegalRoutineVehicleSimpleSpan[fetPrimary][pierIdx] : vMaxLegalRoutineVehicleContinuousSpan[fetPrimary][pierIdx];
         }
         else
         {
            reaction.Mz = Max(ssReaction.Mz, csReaction.Mz);
            reaction.Fx = MaxIndex(ssReaction.Mz, csReaction.Mz) == 0 ? ssReaction.Fx : csReaction.Fx;
            reaction.Fy = MaxIndex(ssReaction.Mz, csReaction.Mz) == 0 ? ssReaction.Fy : csReaction.Fy;
            maxLegalRoutineVehicle = MaxIndex(ssReaction.Mz, csReaction.Mz) == 0 ? vMaxLegalRoutineVehicleSimpleSpan[fetPrimary][pierIdx] : vMaxLegalRoutineVehicleContinuousSpan[fetPrimary][pierIdx];
         }

         (*p_rating_table)(row, col + colOffset) << force.SetValue(reaction.Fx);
         (*p_rating_table)(row, col + colOffset) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Routine) << maxLegalRoutineVehicle << _T(")");
         (*p_rating_table)(row + 1, col + colOffset) << force.SetValue(reaction.Fy);
         (*p_rating_table)(row + 1, col + colOffset) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Routine) << maxLegalRoutineVehicle << _T(")");
         (*p_rating_table)(row + 2, col + colOffset) << moment.SetValue(reaction.Mz);
         (*p_rating_table)(row + 2, col + colOffset) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Routine) << maxLegalRoutineVehicle << _T(")");
         col++;

         ssReaction = vMinLegalRoutineReactionsSimpleSpan[fetPrimary][pierIdx];
         csReaction = vMinLegalRoutineReactionsContinuousSpan[fetPrimary][pierIdx];
         if (fetPrimary == pgsTypes::fetFx)
         {
            reaction.Fx = Min(ssReaction.Fx, csReaction.Fx);
            reaction.Fy = MinIndex(ssReaction.Fx, csReaction.Fx) == 0 ? ssReaction.Fy : csReaction.Fy;
            reaction.Mz = MinIndex(ssReaction.Fx, csReaction.Fx) == 0 ? ssReaction.Mz : csReaction.Mz;
            minLegalRoutineVehicle = MinIndex(ssReaction.Fx, csReaction.Fx) == 0 ? vMinLegalRoutineVehicleSimpleSpan[fetPrimary][pierIdx] : vMinLegalRoutineVehicleContinuousSpan[fetPrimary][pierIdx];
         }
         else if (fetPrimary == pgsTypes::fetFy)
         {
            reaction.Fy = Min(ssReaction.Fy, csReaction.Fy);
            reaction.Fx = MinIndex(ssReaction.Fy, csReaction.Fy) == 0 ? ssReaction.Fx : csReaction.Fx;
            reaction.Mz = MinIndex(ssReaction.Fy, csReaction.Fy) == 0 ? ssReaction.Mz : csReaction.Mz;
            minLegalRoutineVehicle = MinIndex(ssReaction.Fy, csReaction.Fy) == 0 ? vMinLegalRoutineVehicleSimpleSpan[fetPrimary][pierIdx] : vMinLegalRoutineVehicleContinuousSpan[fetPrimary][pierIdx];
         }
         else
         {
            reaction.Mz = Min(ssReaction.Mz, csReaction.Mz);
            reaction.Fx = MinIndex(ssReaction.Mz, csReaction.Mz) == 0 ? ssReaction.Fx : csReaction.Fx;
            reaction.Fy = MinIndex(ssReaction.Mz, csReaction.Mz) == 0 ? ssReaction.Fy : csReaction.Fy;
            minLegalRoutineVehicle = MinIndex(ssReaction.Mz, csReaction.Mz) == 0 ? vMinLegalRoutineVehicleSimpleSpan[fetPrimary][pierIdx] : vMinLegalRoutineVehicleContinuousSpan[fetPrimary][pierIdx];
         }

         (*p_rating_table)(row, col + colOffset) << force.SetValue(reaction.Fx);
         (*p_rating_table)(row, col + colOffset) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Routine) << minLegalRoutineVehicle << _T(")");
         (*p_rating_table)(row + 1, col + colOffset) << force.SetValue(reaction.Fy);
         (*p_rating_table)(row + 1, col + colOffset) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Routine) << minLegalRoutineVehicle << _T(")");
         (*p_rating_table)(row + 2, col + colOffset) << moment.SetValue(reaction.Mz);
         (*p_rating_table)(row + 2, col + colOffset) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Routine) << minLegalRoutineVehicle << _T(")");
         col++;

         colOffset += 4;
         ssReaction = vMaxLegalSpecialReactionsSimpleSpan[fetPrimary][pierIdx];
         csReaction = vMaxLegalSpecialReactionsContinuousSpan[fetPrimary][pierIdx];
         if (fetPrimary == pgsTypes::fetFx)
         {
            reaction.Fx = Max(ssReaction.Fx, csReaction.Fx);
            reaction.Fy = MaxIndex(ssReaction.Fx, csReaction.Fx) == 0 ? ssReaction.Fy : csReaction.Fy;
            reaction.Mz = MaxIndex(ssReaction.Fx, csReaction.Fx) == 0 ? ssReaction.Mz : csReaction.Mz;
            maxLegalSpecialVehicle = MaxIndex(ssReaction.Fx, csReaction.Fx) == 0 ? vMaxLegalSpecialVehicleSimpleSpan[fetPrimary][pierIdx] : vMaxLegalSpecialVehicleContinuousSpan[fetPrimary][pierIdx];
         }
         else if (fetPrimary == pgsTypes::fetFy)
         {
            reaction.Fy = Max(ssReaction.Fy, csReaction.Fy);
            reaction.Fx = MaxIndex(ssReaction.Fy, csReaction.Fy) == 0 ? ssReaction.Fx : csReaction.Fx;
            reaction.Mz = MaxIndex(ssReaction.Fy, csReaction.Fy) == 0 ? ssReaction.Mz : csReaction.Mz;
            maxLegalSpecialVehicle = MaxIndex(ssReaction.Fy, csReaction.Fy) == 0 ? vMaxLegalSpecialVehicleSimpleSpan[fetPrimary][pierIdx] : vMaxLegalSpecialVehicleContinuousSpan[fetPrimary][pierIdx];
         }
         else
         {
            reaction.Mz = Max(ssReaction.Mz, csReaction.Mz);
            reaction.Fx = MaxIndex(ssReaction.Mz, csReaction.Mz) == 0 ? ssReaction.Fx : csReaction.Fx;
            reaction.Fy = MaxIndex(ssReaction.Mz, csReaction.Mz) == 0 ? ssReaction.Fy : csReaction.Fy;
            maxLegalSpecialVehicle = MaxIndex(ssReaction.Mz, csReaction.Mz) == 0 ? vMaxLegalSpecialVehicleSimpleSpan[fetPrimary][pierIdx] : vMaxLegalSpecialVehicleContinuousSpan[fetPrimary][pierIdx];
         }

         (*p_rating_table)(row, col + colOffset) << force.SetValue(reaction.Fx);
         (*p_rating_table)(row, col + colOffset) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Routine) << maxLegalSpecialVehicle << _T(")");
         (*p_rating_table)(row + 1, col + colOffset) << force.SetValue(reaction.Fy);
         (*p_rating_table)(row + 1, col + colOffset) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Routine) << maxLegalSpecialVehicle << _T(")");
         (*p_rating_table)(row + 2, col + colOffset) << moment.SetValue(reaction.Mz);
         (*p_rating_table)(row + 2, col + colOffset) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Routine) << maxLegalSpecialVehicle << _T(")");
         col++;

         ssReaction = vMinLegalSpecialReactionsSimpleSpan[fetPrimary][pierIdx];
         csReaction = vMinLegalSpecialReactionsContinuousSpan[fetPrimary][pierIdx];
         if (fetPrimary == pgsTypes::fetFx)
         {
            reaction.Fx = Min(ssReaction.Fx, csReaction.Fx);
            reaction.Fy = MinIndex(ssReaction.Fx, csReaction.Fx) == 0 ? ssReaction.Fy : csReaction.Fy;
            reaction.Mz = MinIndex(ssReaction.Fx, csReaction.Fx) == 0 ? ssReaction.Mz : csReaction.Mz;
            minLegalSpecialVehicle = MinIndex(ssReaction.Fx, csReaction.Fx) == 0 ? vMinLegalSpecialVehicleSimpleSpan[fetPrimary][pierIdx] : vMinLegalSpecialVehicleContinuousSpan[fetPrimary][pierIdx];
         }
         else if (fetPrimary == pgsTypes::fetFy)
         {
            reaction.Fy = Min(ssReaction.Fy, csReaction.Fy);
            reaction.Fx = MinIndex(ssReaction.Fy, csReaction.Fy) == 0 ? ssReaction.Fx : csReaction.Fx;
            reaction.Mz = MinIndex(ssReaction.Fy, csReaction.Fy) == 0 ? ssReaction.Mz : csReaction.Mz;
            minLegalSpecialVehicle = MinIndex(ssReaction.Fy, csReaction.Fy) == 0 ? vMinLegalSpecialVehicleSimpleSpan[fetPrimary][pierIdx] : vMinLegalSpecialVehicleContinuousSpan[fetPrimary][pierIdx];
         }
         else
         {
            reaction.Mz = Min(ssReaction.Mz, csReaction.Mz);
            reaction.Fx = MinIndex(ssReaction.Mz, csReaction.Mz) == 0 ? ssReaction.Fx : csReaction.Fx;
            reaction.Fy = MinIndex(ssReaction.Mz, csReaction.Mz) == 0 ? ssReaction.Fy : csReaction.Fy;
            minLegalSpecialVehicle = MinIndex(ssReaction.Mz, csReaction.Mz) == 0 ? vMinLegalSpecialVehicleSimpleSpan[fetPrimary][pierIdx] : vMinLegalSpecialVehicleContinuousSpan[fetPrimary][pierIdx];
         }

         (*p_rating_table)(row, col + colOffset) << force.SetValue(reaction.Fx);
         (*p_rating_table)(row, col + colOffset) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Routine) << minLegalSpecialVehicle << _T(")");
         (*p_rating_table)(row + 1, col + colOffset) << force.SetValue(reaction.Fy);
         (*p_rating_table)(row + 1, col + colOffset) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Routine) << minLegalSpecialVehicle << _T(")");
         (*p_rating_table)(row + 2, col + colOffset) << moment.SetValue(reaction.Mz);
         (*p_rating_table)(row + 2, col + colOffset) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Routine) << minLegalSpecialVehicle << _T(")");
         col++;


         colOffset += 4;
         ssReaction = vMaxLegalEmergencyReactionsSimpleSpan[fetPrimary][pierIdx];
         csReaction = vMaxLegalEmergencyReactionsContinuousSpan[fetPrimary][pierIdx];
         if (fetPrimary == pgsTypes::fetFx)
         {
            reaction.Fx = Max(ssReaction.Fx, csReaction.Fx);
            reaction.Fy = MaxIndex(ssReaction.Fx, csReaction.Fx) == 0 ? ssReaction.Fy : csReaction.Fy;
            reaction.Mz = MaxIndex(ssReaction.Fx, csReaction.Fx) == 0 ? ssReaction.Mz : csReaction.Mz;
            maxLegalEmergencyVehicle = MaxIndex(ssReaction.Fx, csReaction.Fx) == 0 ? vMaxLegalEmergencyVehicleSimpleSpan[fetPrimary][pierIdx] : vMaxLegalEmergencyVehicleContinuousSpan[fetPrimary][pierIdx];
         }
         else if (fetPrimary == pgsTypes::fetFy)
         {
            reaction.Fy = Max(ssReaction.Fy, csReaction.Fy);
            reaction.Fx = MaxIndex(ssReaction.Fy, csReaction.Fy) == 0 ? ssReaction.Fx : csReaction.Fx;
            reaction.Mz = MaxIndex(ssReaction.Fy, csReaction.Fy) == 0 ? ssReaction.Mz : csReaction.Mz;
            maxLegalEmergencyVehicle = MaxIndex(ssReaction.Fy, csReaction.Fy) == 0 ? vMaxLegalEmergencyVehicleSimpleSpan[fetPrimary][pierIdx] : vMaxLegalEmergencyVehicleContinuousSpan[fetPrimary][pierIdx];
         }
         else
         {
            reaction.Mz = Max(ssReaction.Mz, csReaction.Mz);
            reaction.Fx = MaxIndex(ssReaction.Mz, csReaction.Mz) == 0 ? ssReaction.Fx : csReaction.Fx;
            reaction.Fy = MaxIndex(ssReaction.Mz, csReaction.Mz) == 0 ? ssReaction.Fy : csReaction.Fy;
            maxLegalEmergencyVehicle = MaxIndex(ssReaction.Mz, csReaction.Mz) == 0 ? vMaxLegalEmergencyVehicleSimpleSpan[fetPrimary][pierIdx] : vMaxLegalEmergencyVehicleContinuousSpan[fetPrimary][pierIdx];
         }

         (*p_rating_table)(row, col + colOffset) << force.SetValue(reaction.Fx);
         (*p_rating_table)(row, col + colOffset) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Emergency) << maxLegalEmergencyVehicle << _T(")");
         (*p_rating_table)(row + 1, col + colOffset) << force.SetValue(reaction.Fy);
         (*p_rating_table)(row + 1, col + colOffset) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Emergency) << maxLegalEmergencyVehicle << _T(")");
         (*p_rating_table)(row + 2, col + colOffset) << moment.SetValue(reaction.Mz);
         (*p_rating_table)(row + 2, col + colOffset) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Emergency) << maxLegalEmergencyVehicle << _T(")");
         col++;

         ssReaction = vMinLegalEmergencyReactionsSimpleSpan[fetPrimary][pierIdx];
         csReaction = vMinLegalEmergencyReactionsContinuousSpan[fetPrimary][pierIdx];
         if (fetPrimary == pgsTypes::fetFx)
         {
            reaction.Fx = Min(ssReaction.Fx, csReaction.Fx);
            reaction.Fy = MinIndex(ssReaction.Fx, csReaction.Fx) == 0 ? ssReaction.Fy : csReaction.Fy;
            reaction.Mz = MinIndex(ssReaction.Fx, csReaction.Fx) == 0 ? ssReaction.Mz : csReaction.Mz;
            minLegalEmergencyVehicle = MinIndex(ssReaction.Fx, csReaction.Fx) == 0 ? vMinLegalEmergencyVehicleSimpleSpan[fetPrimary][pierIdx] : vMinLegalEmergencyVehicleContinuousSpan[fetPrimary][pierIdx];
         }
         else if (fetPrimary == pgsTypes::fetFy)
         {
            reaction.Fy = Min(ssReaction.Fy, csReaction.Fy);
            reaction.Fx = MinIndex(ssReaction.Fy, csReaction.Fy) == 0 ? ssReaction.Fx : csReaction.Fx;
            reaction.Mz = MinIndex(ssReaction.Fy, csReaction.Fy) == 0 ? ssReaction.Mz : csReaction.Mz;
            minLegalEmergencyVehicle = MinIndex(ssReaction.Fy, csReaction.Fy) == 0 ? vMinLegalEmergencyVehicleSimpleSpan[fetPrimary][pierIdx] : vMinLegalEmergencyVehicleContinuousSpan[fetPrimary][pierIdx];
         }
         else
         {
            reaction.Mz = Min(ssReaction.Mz, csReaction.Mz);
            reaction.Fx = MinIndex(ssReaction.Mz, csReaction.Mz) == 0 ? ssReaction.Fx : csReaction.Fx;
            reaction.Fy = MinIndex(ssReaction.Mz, csReaction.Mz) == 0 ? ssReaction.Fy : csReaction.Fy;
            minLegalEmergencyVehicle = MinIndex(ssReaction.Mz, csReaction.Mz) == 0 ? vMinLegalEmergencyVehicleSimpleSpan[fetPrimary][pierIdx] : vMinLegalEmergencyVehicleContinuousSpan[fetPrimary][pierIdx];
         }

         (*p_rating_table)(row, col + colOffset) << force.SetValue(reaction.Fx);
         (*p_rating_table)(row, col + colOffset) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Emergency) << minLegalEmergencyVehicle << _T(")");
         (*p_rating_table)(row + 1, col + colOffset) << force.SetValue(reaction.Fy);
         (*p_rating_table)(row + 1, col + colOffset) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Emergency) << minLegalEmergencyVehicle << _T(")");
         (*p_rating_table)(row + 2, col + colOffset) << moment.SetValue(reaction.Mz);
         (*p_rating_table)(row + 2, col + colOffset) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Emergency) << minLegalEmergencyVehicle << _T(")");
         col++;

         colOffset += 4;
         ssReaction = vMaxPermitRoutineReactionsSimpleSpan[fetPrimary][pierIdx];
         csReaction = vMaxPermitRoutineReactionsContinuousSpan[fetPrimary][pierIdx];
         if (fetPrimary == pgsTypes::fetFx)
         {
            reaction.Fx = Max(ssReaction.Fx, csReaction.Fx);
            reaction.Fy = MaxIndex(ssReaction.Fx, csReaction.Fx) == 0 ? ssReaction.Fy : csReaction.Fy;
            reaction.Mz = MaxIndex(ssReaction.Fx, csReaction.Fx) == 0 ? ssReaction.Mz : csReaction.Mz;
            maxPermitRoutineVehicle = MaxIndex(ssReaction.Fx, csReaction.Fx) == 0 ? vMaxPermitRoutineVehicleSimpleSpan[fetPrimary][pierIdx] : vMaxPermitRoutineVehicleContinuousSpan[fetPrimary][pierIdx];
         }
         else if (fetPrimary == pgsTypes::fetFy)
         {
            reaction.Fy = Max(ssReaction.Fy, csReaction.Fy);
            reaction.Fx = MaxIndex(ssReaction.Fy, csReaction.Fy) == 0 ? ssReaction.Fx : csReaction.Fx;
            reaction.Mz = MaxIndex(ssReaction.Fy, csReaction.Fy) == 0 ? ssReaction.Mz : csReaction.Mz;
            maxPermitRoutineVehicle = MaxIndex(ssReaction.Fy, csReaction.Fy) == 0 ? vMaxPermitRoutineVehicleSimpleSpan[fetPrimary][pierIdx] : vMaxPermitRoutineVehicleContinuousSpan[fetPrimary][pierIdx];
         }
         else
         {
            reaction.Mz = Max(ssReaction.Mz, csReaction.Mz);
            reaction.Fx = MaxIndex(ssReaction.Mz, csReaction.Mz) == 0 ? ssReaction.Fx : csReaction.Fx;
            reaction.Fy = MaxIndex(ssReaction.Mz, csReaction.Mz) == 0 ? ssReaction.Fy : csReaction.Fy;
            maxPermitRoutineVehicle = MaxIndex(ssReaction.Mz, csReaction.Mz) == 0 ? vMaxPermitRoutineVehicleSimpleSpan[fetPrimary][pierIdx] : vMaxPermitRoutineVehicleContinuousSpan[fetPrimary][pierIdx];
         }

         (*p_rating_table)(row, col + colOffset) << force.SetValue(reaction.Fx);
         (*p_rating_table)(row, col + colOffset) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermitRating_Routine) << maxPermitRoutineVehicle << _T(")");
         (*p_rating_table)(row + 1, col + colOffset) << force.SetValue(reaction.Fy);
         (*p_rating_table)(row + 1, col + colOffset) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermitRating_Routine) << maxPermitRoutineVehicle << _T(")");
         (*p_rating_table)(row + 2, col + colOffset) << moment.SetValue(reaction.Mz);
         (*p_rating_table)(row + 2, col + colOffset) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermitRating_Routine) << maxPermitRoutineVehicle << _T(")");
         col++;

         ssReaction = vMinPermitRoutineReactionsSimpleSpan[fetPrimary][pierIdx];
         csReaction = vMinPermitRoutineReactionsContinuousSpan[fetPrimary][pierIdx];
         if (fetPrimary == pgsTypes::fetFx)
         {
            reaction.Fx = Min(ssReaction.Fx, csReaction.Fx);
            reaction.Fy = MinIndex(ssReaction.Fx, csReaction.Fx) == 0 ? ssReaction.Fy : csReaction.Fy;
            reaction.Mz = MinIndex(ssReaction.Fx, csReaction.Fx) == 0 ? ssReaction.Mz : csReaction.Mz;
            minPermitRoutineVehicle = MinIndex(ssReaction.Fx, csReaction.Fx) == 0 ? vMinPermitRoutineVehicleSimpleSpan[fetPrimary][pierIdx] : vMinPermitRoutineVehicleContinuousSpan[fetPrimary][pierIdx];
         }
         else if (fetPrimary == pgsTypes::fetFy)
         {
            reaction.Fy = Min(ssReaction.Fy, csReaction.Fy);
            reaction.Fx = MinIndex(ssReaction.Fy, csReaction.Fy) == 0 ? ssReaction.Fx : csReaction.Fx;
            reaction.Mz = MinIndex(ssReaction.Fy, csReaction.Fy) == 0 ? ssReaction.Mz : csReaction.Mz;
            minPermitRoutineVehicle = MinIndex(ssReaction.Fy, csReaction.Fy) == 0 ? vMinPermitRoutineVehicleSimpleSpan[fetPrimary][pierIdx] : vMinPermitRoutineVehicleContinuousSpan[fetPrimary][pierIdx];
         }
         else
         {
            reaction.Mz = Min(ssReaction.Mz, csReaction.Mz);
            reaction.Fx = MinIndex(ssReaction.Mz, csReaction.Mz) == 0 ? ssReaction.Fx : csReaction.Fx;
            reaction.Fy = MinIndex(ssReaction.Mz, csReaction.Mz) == 0 ? ssReaction.Fy : csReaction.Fy;
            minPermitRoutineVehicle = MinIndex(ssReaction.Mz, csReaction.Mz) == 0 ? vMinPermitRoutineVehicleSimpleSpan[fetPrimary][pierIdx] : vMinPermitRoutineVehicleContinuousSpan[fetPrimary][pierIdx];
         }

         (*p_rating_table)(row, col + colOffset) << force.SetValue(reaction.Fx);
         (*p_rating_table)(row, col + colOffset) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermitRating_Routine) << minPermitRoutineVehicle << _T(")");
         (*p_rating_table)(row + 1, col + colOffset) << force.SetValue(reaction.Fy);
         (*p_rating_table)(row + 1, col + colOffset) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermitRating_Routine) << minPermitRoutineVehicle << _T(")");
         (*p_rating_table)(row + 2, col + colOffset) << moment.SetValue(reaction.Mz);
         (*p_rating_table)(row + 2, col + colOffset) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermitRating_Routine) << minPermitRoutineVehicle << _T(")");
         col++;

         colOffset += 4;
         ssReaction = vMaxPermitSpecialReactionsSimpleSpan[fetPrimary][pierIdx];
         csReaction = vMaxPermitSpecialReactionsContinuousSpan[fetPrimary][pierIdx];
         if (fetPrimary == pgsTypes::fetFx)
         {
            reaction.Fx = Max(ssReaction.Fx, csReaction.Fx);
            reaction.Fy = MaxIndex(ssReaction.Fx, csReaction.Fx) == 0 ? ssReaction.Fy : csReaction.Fy;
            reaction.Mz = MaxIndex(ssReaction.Fx, csReaction.Fx) == 0 ? ssReaction.Mz : csReaction.Mz;
            maxPermitSpecialVehicle = MaxIndex(ssReaction.Fx, csReaction.Fx) == 0 ? vMaxPermitSpecialVehicleSimpleSpan[fetPrimary][pierIdx] : vMaxPermitSpecialVehicleContinuousSpan[fetPrimary][pierIdx];
         }
         else if (fetPrimary == pgsTypes::fetFy)
         {
            reaction.Fy = Max(ssReaction.Fy, csReaction.Fy);
            reaction.Fx = MaxIndex(ssReaction.Fy, csReaction.Fy) == 0 ? ssReaction.Fx : csReaction.Fx;
            reaction.Mz = MaxIndex(ssReaction.Fy, csReaction.Fy) == 0 ? ssReaction.Mz : csReaction.Mz;
            maxPermitSpecialVehicle = MaxIndex(ssReaction.Fy, csReaction.Fy) == 0 ? vMaxPermitSpecialVehicleSimpleSpan[fetPrimary][pierIdx] : vMaxPermitSpecialVehicleContinuousSpan[fetPrimary][pierIdx];
         }
         else
         {
            reaction.Mz = Max(ssReaction.Mz, csReaction.Mz);
            reaction.Fx = MaxIndex(ssReaction.Mz, csReaction.Mz) == 0 ? ssReaction.Fx : csReaction.Fx;
            reaction.Fy = MaxIndex(ssReaction.Mz, csReaction.Mz) == 0 ? ssReaction.Fy : csReaction.Fy;
            maxPermitSpecialVehicle = MaxIndex(ssReaction.Mz, csReaction.Mz) == 0 ? vMaxPermitSpecialVehicleSimpleSpan[fetPrimary][pierIdx] : vMaxPermitSpecialVehicleContinuousSpan[fetPrimary][pierIdx];
         }

         (*p_rating_table)(row, col + colOffset) << force.SetValue(reaction.Fx);
         (*p_rating_table)(row, col + colOffset) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermitRating_Routine) << maxPermitSpecialVehicle << _T(")");
         (*p_rating_table)(row + 1, col + colOffset) << force.SetValue(reaction.Fy);
         (*p_rating_table)(row + 1, col + colOffset) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermitRating_Routine) << maxPermitSpecialVehicle << _T(")");
         (*p_rating_table)(row + 2, col + colOffset) << moment.SetValue(reaction.Mz);
         (*p_rating_table)(row + 2, col + colOffset) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermitRating_Routine) << maxPermitSpecialVehicle << _T(")");
         col++;

         ssReaction = vMinPermitSpecialReactionsSimpleSpan[fetPrimary][pierIdx];
         csReaction = vMinPermitSpecialReactionsContinuousSpan[fetPrimary][pierIdx];
         if (fetPrimary == pgsTypes::fetFx)
         {
            reaction.Fx = Min(ssReaction.Fx, csReaction.Fx);
            reaction.Fy = MinIndex(ssReaction.Fx, csReaction.Fx) == 0 ? ssReaction.Fy : csReaction.Fy;
            reaction.Mz = MinIndex(ssReaction.Fx, csReaction.Fx) == 0 ? ssReaction.Mz : csReaction.Mz;
            minPermitSpecialVehicle = MinIndex(ssReaction.Fx, csReaction.Fx) == 0 ? vMinPermitSpecialVehicleSimpleSpan[fetPrimary][pierIdx] : vMinPermitSpecialVehicleContinuousSpan[fetPrimary][pierIdx];
         }
         else if (fetPrimary == pgsTypes::fetFy)
         {
            reaction.Fy = Min(ssReaction.Fy, csReaction.Fy);
            reaction.Fx = MinIndex(ssReaction.Fy, csReaction.Fy) == 0 ? ssReaction.Fx : csReaction.Fx;
            reaction.Mz = MinIndex(ssReaction.Fy, csReaction.Fy) == 0 ? ssReaction.Mz : csReaction.Mz;
            minPermitSpecialVehicle = MinIndex(ssReaction.Fy, csReaction.Fy) == 0 ? vMinPermitSpecialVehicleSimpleSpan[fetPrimary][pierIdx] : vMinPermitSpecialVehicleContinuousSpan[fetPrimary][pierIdx];
         }
         else
         {
            reaction.Mz = Min(ssReaction.Mz, csReaction.Mz);
            reaction.Fx = MinIndex(ssReaction.Mz, csReaction.Mz) == 0 ? ssReaction.Fx : csReaction.Fx;
            reaction.Fy = MinIndex(ssReaction.Mz, csReaction.Mz) == 0 ? ssReaction.Fy : csReaction.Fy;
            minPermitSpecialVehicle = MinIndex(ssReaction.Mz, csReaction.Mz) == 0 ? vMinPermitSpecialVehicleSimpleSpan[fetPrimary][pierIdx] : vMinPermitSpecialVehicleContinuousSpan[fetPrimary][pierIdx];
         }

         (*p_rating_table)(row, col + colOffset) << force.SetValue(reaction.Fx);
         (*p_rating_table)(row, col + colOffset) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermitRating_Routine) << minPermitSpecialVehicle << _T(")");
         (*p_rating_table)(row + 1, col + colOffset) << force.SetValue(reaction.Fy);
         (*p_rating_table)(row + 1, col + colOffset) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermitRating_Routine) << minPermitSpecialVehicle << _T(")");
         (*p_rating_table)(row + 2, col + colOffset) << moment.SetValue(reaction.Mz);
         (*p_rating_table)(row + 2, col + colOffset) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermitRating_Routine) << minPermitSpecialVehicle << _T(")");
         col++;
      }
   }

   return pChapter;
}

CChapterBuilder* CPierReactionChapterBuilder::Clone() const
{
   return new CPierReactionChapterBuilder;
}
