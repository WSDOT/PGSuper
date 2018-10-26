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
#include <Reporting\BridgeAnalysisChapterBuilder.h>
#include <Reporting\ReportNotes.h>

#include <Reporting\BridgeAnalysisReportSpecification.h>

#include <Reporting\CastingYardMomentsTable.h>
#include <Reporting\ProductMomentsTable.h>
#include <Reporting\ProductShearTable.h>
#include <Reporting\ProductReactionTable.h>
#include <Reporting\ProductDisplacementsTable.h>
#include <Reporting\ProductRotationTable.h>

#include <Reporting\CombinedMomentsTable.h>
#include <Reporting\CombinedShearTable.h>
#include <Reporting\CombinedReactionTable.h>

#include <Reporting\UserMomentsTable.h>
#include <Reporting\UserShearTable.h>
#include <Reporting\UserReactionTable.h>
#include <Reporting\UserDisplacementsTable.h>
#include <Reporting\UserRotationTable.h>

#include <Reporting\LiveLoadDistributionFactorTable.h>

#include <Reporting\VehicularLoadResultsTable.h>
#include <Reporting\VehicularLoadReactionTable.h>
#include <Reporting\CombinedReactionTable.h>

#include <IFace\Bridge.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Project.h>
#include <IFace\RatingSpecification.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CBridgeAnalysisChapterBuilder
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CBridgeAnalysisChapterBuilder::CBridgeAnalysisChapterBuilder(LPCTSTR strTitle,pgsTypes::AnalysisType analysisType,bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
   m_strTitle = strTitle;
   m_AnalysisType = analysisType;
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CBridgeAnalysisChapterBuilder::GetName() const
{
   return m_strTitle.c_str();
}

rptChapter* CBridgeAnalysisChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CBridgeAnalysisReportSpecification* pBridgeAnalysisRptSpec = dynamic_cast<CBridgeAnalysisReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pBridgeAnalysisRptSpec->GetBroker(&pBroker);
   GirderIndexType girder = pBridgeAnalysisRptSpec->GetGirder();

   SpanIndexType span = ALL_SPANS;

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);
   rptParagraph* p = 0;

   GET_IFACE2(pBroker,ILimitStateForces,pLimitStateForces);
   bool bPermit = pLimitStateForces->IsStrengthIIApplicable(span, girder);

   bool bDesign = pBridgeAnalysisRptSpec->ReportDesignResults();
   bool bRating = pBridgeAnalysisRptSpec->ReportRatingResults();

   GET_IFACE2(pBroker,IProductLoads,pProductLoads);
   bool bPedestrian = pProductLoads->HasPedestrianLoad();

   bool bIndicateControllingLoad = true;

   // Product Moments
   p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *p << _T("Load Responses - Bridge Site")<<rptNewLine;
   p->SetName(_T("Bridge Site Results"));
   *pChapter << p;
   p = new rptParagraph;
   *pChapter << p;
   *p << CProductMomentsTable().Build(pBroker,span,girder,m_AnalysisType,bDesign,bRating,bIndicateControllingLoad,pDisplayUnits) << rptNewLine;

   if ( bPedestrian )
      *p << _T("$ Pedestrian values are per girder") << rptNewLine;
 
   *p << LIVELOAD_PER_LANE << rptNewLine;
   LiveLoadTableFooter(pBroker,p,girder,bDesign,bRating);
    

   GET_IFACE2(pBroker,IUserDefinedLoads,pUDL);
   bool are_user_loads = pUDL->DoUserLoadsExist(span,girder);
   if (are_user_loads)
   {
      *p << CUserMomentsTable().Build(pBroker,span,girder,m_AnalysisType,pDisplayUnits) << rptNewLine;
   }

   // Product Shears
   p = new rptParagraph;
   *pChapter << p;
   *p << CProductShearTable().Build(pBroker,span,girder,m_AnalysisType,bDesign,bRating,bIndicateControllingLoad,pDisplayUnits) << rptNewLine;

   if ( bPedestrian )
      *p << _T("$ Pedestrian values are per girder") << rptNewLine;

   *p << LIVELOAD_PER_LANE << rptNewLine;
   *p << rptNewLine;
   LiveLoadTableFooter(pBroker,p,girder,bDesign,bRating);

   if (are_user_loads)
   {
      *p << CUserShearTable().Build(pBroker,span,girder,m_AnalysisType,pDisplayUnits) << rptNewLine;
   }

   // Product Reactions
   p = new rptParagraph;
   *pChapter << p;
   *p << CProductReactionTable().Build(pBroker,span,girder,m_AnalysisType,PierReactionsTable,true,false,bDesign,bRating,bIndicateControllingLoad,pDisplayUnits) << rptNewLine;

   if ( bPedestrian )
      *p << _T("$ Pedestrian values are per girder") << rptNewLine;

   *p << LIVELOAD_PER_LANE << rptNewLine;
   *p << rptNewLine;
   LiveLoadTableFooter(pBroker,p,girder,bDesign,bRating);

   GET_IFACE2(pBroker,IBearingDesign,pBearingDesign);
   bool bDoBearingReaction, bDummy;
   bDoBearingReaction = pBearingDesign->AreBearingReactionsAvailable(pgsTypes::BridgeSite3, span,girder,&bDummy,&bDummy);

   if(bDoBearingReaction)
   {
      *p << CProductReactionTable().Build(pBroker,span,girder,m_AnalysisType,BearingReactionsTable,true,false,bDesign,bRating,bIndicateControllingLoad,pDisplayUnits) << rptNewLine;

      if ( bPedestrian )
         *p << _T("$ Pedestrian values are per girder") << rptNewLine;

      *p << LIVELOAD_PER_LANE << rptNewLine;
      *p << rptNewLine;
      LiveLoadTableFooter(pBroker,p,girder,bDesign,bRating);
   }

   if (are_user_loads)
   {
      *p << CUserReactionTable().Build(pBroker,span,girder,m_AnalysisType,PierReactionsTable,pDisplayUnits) << rptNewLine;
      *p << CUserReactionTable().Build(pBroker,span,girder,m_AnalysisType,BearingReactionsTable,pDisplayUnits) << rptNewLine;
   }

   // Product Displacements
   p = new rptParagraph;
   *pChapter << p;
   *p << CProductDisplacementsTable().Build(pBroker,span,girder,m_AnalysisType,bDesign,bRating,bIndicateControllingLoad,pDisplayUnits) << rptNewLine;

   if ( bPedestrian )
      *p << _T("$ Pedestrian values are per girder") << rptNewLine;

   *p << LIVELOAD_PER_LANE << rptNewLine;
   *p << rptNewLine;
   LiveLoadTableFooter(pBroker,p,girder,bDesign,bRating);

   if (are_user_loads)
   {
      *p << CUserDisplacementsTable().Build(pBroker,span,girder,m_AnalysisType,pDisplayUnits) << rptNewLine;
   }

   // Product Rotations
   p = new rptParagraph;
   *pChapter << p;
   *p << CProductRotationTable().Build(pBroker,span,girder,m_AnalysisType,true,false,bDesign,bRating,bIndicateControllingLoad,pDisplayUnits) << rptNewLine;

   if ( bPedestrian )
      *p << _T("$ Pedestrian values are per girder") << rptNewLine;

   *p << LIVELOAD_PER_LANE << rptNewLine;
   *p << rptNewLine;
   LiveLoadTableFooter(pBroker,p,girder,bDesign,bRating);

   if (are_user_loads)
   {
      *p << CUserRotationTable().Build(pBroker,span,girder,m_AnalysisType,pDisplayUnits) << rptNewLine;
   }

   // Responses from individual live load vehicules
   GET_IFACE2(pBroker,ISpecification,pSpec);
   std::vector<pgsTypes::LiveLoadType> live_load_types;
   if ( bDesign )
   {
      live_load_types.push_back(pgsTypes::lltDesign);

      if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
        live_load_types.push_back(pgsTypes::lltFatigue);

      GET_IFACE2(pBroker,ILiveLoads,pLiveLoads);
      bool bPermit = pLiveLoads->IsLiveLoadDefined(pgsTypes::lltPermit);
      if ( bPermit )
         live_load_types.push_back(pgsTypes::lltPermit);
   }

   GET_IFACE2(pBroker,IRatingSpecification,pRatingSpec);
   if ( bRating )
   {
      // if lltDesign isn't included because we aren't reporting design and if we are doing Design Inventory or Operating rating
      // then add the lltDesign
      if ( !bDesign && (pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) || pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating)) )
         live_load_types.push_back(pgsTypes::lltDesign);

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
         live_load_types.push_back(pgsTypes::lltLegalRating_Routine);

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
         live_load_types.push_back(pgsTypes::lltLegalRating_Special);

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
         live_load_types.push_back(pgsTypes::lltPermitRating_Routine);

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
         live_load_types.push_back(pgsTypes::lltPermitRating_Special);
   }

   std::vector<pgsTypes::LiveLoadType>::iterator iter;
   for ( iter = live_load_types.begin(); iter != live_load_types.end(); iter++ )
   {
      pgsTypes::LiveLoadType llType = *iter;

      GET_IFACE2( pBroker, IProductLoads, pProductLoads);
      std::vector<std::_tstring> strLLNames = pProductLoads->GetVehicleNames(llType,girder);

      // nothing to report if there are no loads
      if ( strLLNames.size() == 0 )
         continue;

      // if the only loading is the DUMMY load, then move on
      if ( strLLNames.size() == 1 && strLLNames[0] == std::_tstring(_T("No Live Load Defined")) )
         continue;

      p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      *pChapter << p;

      if ( llType == pgsTypes::lltDesign )
      {
         p->SetName(_T("Design Live Load Individual Vehicle Response"));
         *p << p->GetName() << rptNewLine;
      }
      else if ( llType == pgsTypes::lltPermit )
      {
         p->SetName(_T("Permit Live Load Individual Vehicle Response"));
         *p << p->GetName() << rptNewLine;
      }
      else if ( llType == pgsTypes::lltFatigue )
      {
         p->SetName(_T("Fatigue Live Load Individual Vehicle Response"));
         *p << p->GetName() << rptNewLine;
      }
      else if ( llType == pgsTypes::lltPedestrian )
      {
         p->SetName(_T("Pedestrian Live Load Response"));
         *p << p->GetName() << rptNewLine;
      }
      else if ( llType == pgsTypes::lltLegalRating_Routine )
      {
         p->SetName(_T("AASHTO Legal Rating Routine Commercial Vehicle Individual Vehicle Live Load Response"));
         *p << p->GetName() << rptNewLine;
      }
      else if ( llType == pgsTypes::lltLegalRating_Special )
      {
         p->SetName(_T("AASHTO Legal Rating Specialized Hauling Vehicle Individual Vehicle Live Load Response"));
         *p << p->GetName() << rptNewLine;
      }
      else if ( llType == pgsTypes::lltPermitRating_Routine )
      {
         p->SetName(_T("Routine Permit Rating Individual Vehicle Live Load Response"));
         *p << p->GetName() << rptNewLine;
      }
      else if ( llType == pgsTypes::lltPermitRating_Special )
      {
         p->SetName(_T("Special Permit Rating Individual Vehicle Live Load Response"));
         *p << p->GetName() << rptNewLine;
      }
      else
      {
         ATLASSERT(false); // is there a new live load type???
      }

      std::vector<std::_tstring>::iterator iter;
      for ( iter = strLLNames.begin(); iter != strLLNames.end(); iter++ )
      {
         std::_tstring strLLName = *iter;

         VehicleIndexType index = iter - strLLNames.begin();

         p = new rptParagraph;
         *pChapter << p;
         p->SetName( strLLName.c_str() );
         *p << CVehicularLoadResultsTable().Build(pBroker,span,girder,llType,strLLName,index,m_AnalysisType,true,pDisplayUnits) << rptNewLine;
         *p << LIVELOAD_PER_LANE << rptNewLine;

         *p << rptNewLine;

         *p << CVehicularLoadReactionTable().Build(pBroker,span,girder,llType,strLLName,index,m_AnalysisType,true,true,pDisplayUnits) << rptNewLine;
         *p << LIVELOAD_PER_LANE << rptNewLine;
      }
   }

   // Load Combinations (DC, DW, etc) & Limit States
   p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << p;
   *p << _T("Responses - Casting Yard Stage") << rptNewLine;
   p->SetName(_T("Casting Yard"));
   CCombinedMomentsTable().Build(pBroker,pChapter,span,girder,pDisplayUnits,pgsTypes::CastingYard, m_AnalysisType, bDesign, bRating);
//   CCombinedShearTable().Build(pBroker,pChapter,span,girder,pDisplayUnits,pgsTypes::CastingYard);
//   CCombinedReactionTable().Build(pBroker,pChapter,span,girder,pDisplayUnits,pgsTypes::CastingYard);

   p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << p;
   *p << _T("Responses - Girder Placement") << rptNewLine;
   p->SetName(_T("Girder Placement"));
   CCombinedMomentsTable().Build(pBroker,pChapter,span,girder,pDisplayUnits,pgsTypes::GirderPlacement, m_AnalysisType, bDesign, bRating);
   CCombinedShearTable().Build(pBroker,pChapter,span,girder,pDisplayUnits,pgsTypes::GirderPlacement, m_AnalysisType, bDesign, bRating);
   CCombinedReactionTable().Build(pBroker,pChapter,span,girder,pDisplayUnits,pgsTypes::GirderPlacement, m_AnalysisType,PierReactionsTable, bDesign, bRating);
   if(bDoBearingReaction && span!=ALL_SPANS)
   {
      CCombinedReactionTable().Build(pBroker,pChapter,span,girder,pDisplayUnits,pgsTypes::GirderPlacement, m_AnalysisType,BearingReactionsTable, bDesign, bRating);
   }

   GET_IFACE2(pBroker,IBridge,pBridge);
   SpanIndexType nSpans = pBridge->GetSpanCount();
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);
   bool bTempStrands = false;
   SpanIndexType firstSpanIdx = (span == ALL_SPANS ? 0 : span);
   SpanIndexType lastSpanIdx  = (span == ALL_SPANS ? nSpans : firstSpanIdx+1);
   for ( SpanIndexType spanIdx = firstSpanIdx; spanIdx < lastSpanIdx; spanIdx++ )
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(spanIdx);
      GirderIndexType gdrIdx = (nGirders <= girder ? nGirders-1 : girder);
      if ( 0 < pStrandGeom->GetMaxStrands(spanIdx,gdrIdx,pgsTypes::Temporary) )
      {
         bTempStrands = true;
         break;
      }
   }
   if ( bTempStrands )
   {
      // if there can be temporary strands, report the loads at the temporary strand removal stage
      // because this is when the girder load is applied
      p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      *pChapter << p;
      *p << _T("Responses - Temporary Strand Removal Stage") << rptNewLine;
      p->SetName(_T("Temporary Strand Removal"));
      CCombinedMomentsTable().Build(pBroker,pChapter,span,girder,pDisplayUnits,pgsTypes::TemporaryStrandRemoval, m_AnalysisType, bDesign, bRating);
      CCombinedShearTable().Build(pBroker,pChapter,span,girder,pDisplayUnits,pgsTypes::TemporaryStrandRemoval, m_AnalysisType, bDesign, bRating);
      CCombinedReactionTable().Build(pBroker,pChapter,span,girder,pDisplayUnits,pgsTypes::TemporaryStrandRemoval, m_AnalysisType,PierReactionsTable, bDesign, bRating);
      if(bDoBearingReaction && span!=ALL_SPANS)
      {
         CCombinedReactionTable().Build(pBroker,pChapter,span,girder,pDisplayUnits,pgsTypes::TemporaryStrandRemoval, m_AnalysisType,BearingReactionsTable, bDesign, bRating);
      }
   }

   p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << p;
   *p << _T("Responses - Deck and Diaphragm Placement (Bridge Site 1)") << rptNewLine;
   p->SetName(_T("Deck and Diaphragm Placement (Bridge Site 1)"));
   CCombinedMomentsTable().Build(pBroker,pChapter,span,girder,pDisplayUnits,pgsTypes::BridgeSite1,m_AnalysisType, bDesign, bRating);
   CCombinedShearTable().Build(pBroker,pChapter,span,girder,pDisplayUnits,pgsTypes::BridgeSite1,m_AnalysisType, bDesign, bRating);
   CCombinedReactionTable().Build(pBroker,pChapter,span,girder,pDisplayUnits,pgsTypes::BridgeSite1,m_AnalysisType,PierReactionsTable, bDesign, bRating);
   if(bDoBearingReaction && span!=ALL_SPANS)
   {
      CCombinedReactionTable().Build(pBroker,pChapter,span,girder,pDisplayUnits,pgsTypes::BridgeSite1,m_AnalysisType,BearingReactionsTable, bDesign, bRating);
   }

   p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << p;
   *p << _T("Responses - Final without Live Load (Bridge Site 2)") << rptNewLine;
   p->SetName(_T("Final without Live Load (Bridge Site 2)"));
   CCombinedMomentsTable().Build(pBroker,pChapter,span,girder,pDisplayUnits,pgsTypes::BridgeSite2,m_AnalysisType, bDesign, bRating);
   CCombinedShearTable().Build(pBroker,pChapter,span,girder,pDisplayUnits,pgsTypes::BridgeSite2,m_AnalysisType, bDesign, bRating);
   CCombinedReactionTable().Build(pBroker,pChapter,span,girder,pDisplayUnits,pgsTypes::BridgeSite2,m_AnalysisType,PierReactionsTable, bDesign, bRating);
   if(bDoBearingReaction && span!=ALL_SPANS)
   {
      CCombinedReactionTable().Build(pBroker,pChapter,span,girder,pDisplayUnits,pgsTypes::BridgeSite2,m_AnalysisType,BearingReactionsTable, bDesign, bRating);
   }

   p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << p;
   *p << _T("Responses - Final with Live Load (Bridge Site 3)") << rptNewLine;
   p->SetName(_T("Final with Live Load (Bridge Site 3)"));

   CCombinedMomentsTable().Build(pBroker,pChapter,span,girder,pDisplayUnits,pgsTypes::BridgeSite3,m_AnalysisType,bDesign,bRating);
   CCombinedShearTable().Build(pBroker,pChapter,span,girder,pDisplayUnits,pgsTypes::BridgeSite3,m_AnalysisType,bDesign,bRating);
   CCombinedReactionTable().Build(pBroker,pChapter,span,girder,pDisplayUnits,pgsTypes::BridgeSite3,m_AnalysisType,PierReactionsTable,bDesign,bRating);
   if(bDoBearingReaction && span!=ALL_SPANS)
   {
      CCombinedReactionTable().Build(pBroker,pChapter,span,girder,pDisplayUnits,pgsTypes::BridgeSite3,m_AnalysisType,BearingReactionsTable,bDesign,bRating);
   }

   p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << p;
   *p << _T("Live Load Reactions without Impact") << rptNewLine;
   p->SetName(_T("Live Load Reactions without Impact"));
   CCombinedReactionTable().BuildLiveLoad(pBroker,pChapter,span,girder,pDisplayUnits, m_AnalysisType,PierReactionsTable, false, true, false  );
   CCombinedReactionTable().BuildLiveLoad(pBroker,pChapter,span,girder,pDisplayUnits, m_AnalysisType,PierReactionsTable, false, false, true  );

   if(bDoBearingReaction && span!=ALL_SPANS)
   {
      CCombinedReactionTable().BuildLiveLoad(pBroker,pChapter,span,girder,pDisplayUnits, m_AnalysisType,BearingReactionsTable, false, true, false  );
      CCombinedReactionTable().BuildLiveLoad(pBroker,pChapter,span,girder,pDisplayUnits, m_AnalysisType,BearingReactionsTable, false, false, true  );
   }
   return pChapter;
}

CChapterBuilder* CBridgeAnalysisChapterBuilder::Clone() const
{
   return new CBridgeAnalysisChapterBuilder(m_strTitle.c_str(),m_AnalysisType);
}
