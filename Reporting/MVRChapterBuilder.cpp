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

#include <Reporting\MVRChapterBuilder.h>
#include <Reporting\ReportNotes.h>
#include <Reporting\CastingYardMomentsTable.h>
#include <Reporting\ProductMomentsTable.h>
#include <Reporting\ProductShearTable.h>
#include <Reporting\ProductReactionTable.h>
#include <Reporting\ProductDisplacementsTable.h>
#include <Reporting\ProductRotationTable.h>
#include <Reporting\CombinedMomentsTable.h>
#include <Reporting\CombinedShearTable.h>
#include <Reporting\CombinedReactionTable.h>
#include <Reporting\ConcurrentShearTable.h>
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

#include <psgLib\SpecLibraryEntry.h>
#include <psgLib\RatingLibraryEntry.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CMVRChapterBuilder
****************************************************************************/

////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CMVRChapterBuilder::CMVRChapterBuilder(bool bDesign,bool bRating,bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
   m_bDesign = bDesign;
   m_bRating = bRating;
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CMVRChapterBuilder::GetName() const
{
   return TEXT("Moments, Shears, and Reactions");
}

rptChapter* CMVRChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CSpanGirderReportSpecification* pSGRptSpec = dynamic_cast<CSpanGirderReportSpecification*>(pRptSpec);
   CGirderReportSpecification* pGdrRptSpec    = dynamic_cast<CGirderReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   SpanIndexType span;
   GirderIndexType girder;

   if ( pSGRptSpec )
   {
      pSGRptSpec->GetBroker(&pBroker);
      span = pSGRptSpec->GetSpan();
      girder = pSGRptSpec->GetGirder();
   }
   else if ( pGdrRptSpec )
   {
      pGdrRptSpec->GetBroker(&pBroker);
      span = ALL_SPANS;
      girder = pGdrRptSpec->GetGirder();
   }
   else
   {
      span = ALL_SPANS;
      girder  = ALL_GIRDERS;
   }

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   rptParagraph* p = 0;

   GET_IFACE2(pBroker,ISpecification,pSpec);
   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   bool bDesign = m_bDesign;
   bool bRating;
   
   if ( m_bRating )
   {
      bRating = true;
   }
   else
   {
      // include load rating results if we are always load rating
      GET_IFACE2(pBroker,IRatingSpecification,pRatingSpec);
      bRating = pRatingSpec->AlwaysLoadRate();

      // if none of the rating types are enabled, skip the rating
      if ( !pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) &&
           !pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating) &&
           !pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) &&
           !pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) &&
           !pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) &&
           !pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) 
         )
         bRating = false;
   }


   GET_IFACE2(pBroker,IProductLoads,pProductLoads);
   bool bPedestrian = pProductLoads->HasPedestrianLoad();

   bool bIndicateControllingLoad = true;

   // Product Moments
   if ( bDesign )
   {
      p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      *p << _T("Load Responses - Casting Yard")<<rptNewLine;
      p->SetName(_T("Casting Yard Results"));
      *pChapter << p;

      p = new rptParagraph;
      *pChapter << p;
      *p << CCastingYardMomentsTable().Build(pBroker,span,girder,pDisplayUnits) << rptNewLine;
   }

   p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *p << _T("Load Responses - Bridge Site")<<rptNewLine;
   p->SetName(_T("Bridge Site Results"));
   *pChapter << p;
   p = new rptParagraph;
   *pChapter << p;
   *p << CProductMomentsTable().Build(pBroker,span,girder,analysisType,bDesign,bRating,bIndicateControllingLoad,pDisplayUnits) << rptNewLine;

   if ( bPedestrian )
      *p << _T("$ Pedestrian values are per girder") << rptNewLine;

   *p << LIVELOAD_PER_LANE << rptNewLine;
   LiveLoadTableFooter(pBroker,p,girder,bDesign,bRating);

   GET_IFACE2(pBroker,IUserDefinedLoads,pUDL);
   bool are_user_loads = pUDL->DoUserLoadsExist(span,girder);
   if (are_user_loads)
   {
      *p << CUserMomentsTable().Build(pBroker,span,girder,analysisType,pDisplayUnits) << rptNewLine;
   }

   // Product Shears
   p = new rptParagraph;
   *pChapter << p;
   *p << CProductShearTable().Build(pBroker,span,girder,analysisType,bDesign,bRating,bIndicateControllingLoad,pDisplayUnits) << rptNewLine;

   if ( bPedestrian )
      *p << _T("$ Pedestrian values are per girder") << rptNewLine;

   *p << LIVELOAD_PER_LANE << rptNewLine;
   *p << rptNewLine;
   LiveLoadTableFooter(pBroker,p,girder,bDesign,bRating);

   if (are_user_loads)
   {
      *p << CUserShearTable().Build(pBroker,span,girder,analysisType,pDisplayUnits) << rptNewLine;
   }

   // Product Reactions
   // For piers
   p = new rptParagraph;
   *pChapter << p;
   *p << CProductReactionTable().Build(pBroker,span,girder,analysisType,PierReactionsTable,true,false,bDesign,bRating,bIndicateControllingLoad,pDisplayUnits) << rptNewLine;

   if ( bPedestrian )
      *p << _T("$ Pedestrian values are per girder") << rptNewLine;

   *p << LIVELOAD_PER_LANE << rptNewLine;
   *p << rptNewLine;
   LiveLoadTableFooter(pBroker,p,girder,bDesign,bRating);

   // For girder bearing reactions
   GET_IFACE2(pBroker,IBearingDesign,pBearingDesign);
   bool bDoBearingReaction, bDummy;
   bDoBearingReaction = pBearingDesign->AreBearingReactionsAvailable(pgsTypes::BridgeSite3, span,girder,&bDummy,&bDummy);

   if(bDoBearingReaction)
   {
      *p << CProductReactionTable().Build(pBroker,span,girder,analysisType,BearingReactionsTable,true,false,bDesign,bRating,bIndicateControllingLoad,pDisplayUnits) << rptNewLine;

      if ( bPedestrian )
         *p << _T("$ Pedestrian values are per girder") << rptNewLine;

      *p << LIVELOAD_PER_LANE << rptNewLine;
      *p << rptNewLine;
      LiveLoadTableFooter(pBroker,p,girder,bDesign,bRating);
   }

   if (are_user_loads)
   {
      *p << CUserReactionTable().Build(pBroker,span,girder,analysisType,PierReactionsTable,pDisplayUnits) << rptNewLine;
      *p << CUserReactionTable().Build(pBroker,span,girder,analysisType,BearingReactionsTable,pDisplayUnits) << rptNewLine;
   }

   // Product Displacements
   if ( bDesign )
   {
      p = new rptParagraph;
      *pChapter << p;
      *p << CProductDisplacementsTable().Build(pBroker,span,girder,analysisType,bDesign,bRating,bIndicateControllingLoad,pDisplayUnits) << rptNewLine;

      if ( bPedestrian )
         *p << _T("$ Pedestrian values are per girder") << rptNewLine;

      *p << LIVELOAD_PER_LANE << rptNewLine;
      *p << rptNewLine;
      LiveLoadTableFooter(pBroker,p,girder,bDesign,bRating);

      if (are_user_loads)
      {
         *p << CUserDisplacementsTable().Build(pBroker,span,girder,analysisType,pDisplayUnits) << rptNewLine;
      }

      // Product Rotations
      p = new rptParagraph;
      *pChapter << p;
      *p << CProductRotationTable().Build(pBroker,span,girder,analysisType,true,false,bDesign,bRating,bIndicateControllingLoad,pDisplayUnits) << rptNewLine;

      if ( bPedestrian )
         *p << _T("$ Pedestrian values are per girder") << rptNewLine;

      *p << LIVELOAD_PER_LANE << rptNewLine;
      *p << rptNewLine;
      LiveLoadTableFooter(pBroker,p,girder,bDesign,bRating);

      if (are_user_loads)
      {
         *p << CUserRotationTable().Build(pBroker,span,girder,analysisType,pDisplayUnits) << rptNewLine;
      }
   }

   GET_IFACE2( pBroker, ILibrary, pLib );
   std::_tstring spec_name = pSpec->GetSpecification();
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( spec_name.c_str() );

   if (bDesign && pSpecEntry->GetDoEvaluateLLDeflection())
   {
      // Optional Live Load Displacements
      p = new rptParagraph;
      p->SetName(_T("Live Load Displacements"));
      *pChapter << p;
      *p << CProductDisplacementsTable().BuildLiveLoadTable(pBroker,span,girder,pDisplayUnits) << rptNewLine;
      *p << _T("D1 = LRFD Design truck without lane load and including impact")<< rptNewLine;
      *p << _T("D2 = 0.25*(Design truck) + lane load, including impact")<< rptNewLine;
      *p << _T("D(Controlling) = Max(D1, D2)")<< rptNewLine;
      *p << _T("EI = Bridge EI / Number of Girders") << rptNewLine;
      *p << _T("Live Load Distribution Factor = (Multiple Presence Factor)(Number of Lanes)/(Number of Girders)") << rptNewLine;
      *p << rptNewLine;
   }

   // Load Combinations (DC, DW, etc) & Limit States
   if ( bDesign )
   {
      p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      *pChapter << p;
      *p << _T("Responses - Casting Yard Stage") << rptNewLine;
      p->SetName(_T("Combined Results - Casting Yard"));
      CCombinedMomentsTable().Build(pBroker,pChapter,span,girder,pDisplayUnits,pgsTypes::CastingYard, analysisType, bDesign, bRating);
   //   CCombinedShearTable().Build(pBroker,pChapter,span,girder,pDisplayUnits,pgsTypes::CastingYard);
   //   CCombinedReactionTable().Build(pBroker,pChapter,span,girder,pDisplayUnits,pgsTypes::CastingYard);

      p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      *pChapter << p;
      *p << _T("Responses - Girder Placement") << rptNewLine;
      p->SetName(_T("Combined Results - Girder Placement"));
      CCombinedMomentsTable().Build(pBroker,pChapter,span,girder,pDisplayUnits,pgsTypes::GirderPlacement, analysisType, bDesign, bRating);
      CCombinedShearTable().Build(pBroker,pChapter,span,girder,pDisplayUnits,pgsTypes::GirderPlacement,   analysisType, bDesign, bRating);
      CCombinedReactionTable().Build(pBroker,pChapter,span,girder,pDisplayUnits,pgsTypes::GirderPlacement,analysisType,PierReactionsTable, bDesign, bRating);
      if(bDoBearingReaction)
      {
         CCombinedReactionTable().Build(pBroker,pChapter,span,girder,pDisplayUnits,pgsTypes::GirderPlacement,analysisType,BearingReactionsTable, bDesign, bRating);
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
         p->SetName(_T("Combined Results - Temporary Strand Removal"));
         CCombinedMomentsTable().Build(pBroker,pChapter,span,girder,pDisplayUnits,pgsTypes::TemporaryStrandRemoval, analysisType,bDesign,bRating);
         CCombinedShearTable().Build(pBroker,pChapter,span,girder,pDisplayUnits,pgsTypes::TemporaryStrandRemoval, analysisType, bDesign, bRating);
         CCombinedReactionTable().Build(pBroker,pChapter,span,girder,pDisplayUnits,pgsTypes::TemporaryStrandRemoval, analysisType,PierReactionsTable, bDesign, bRating);
         if(bDoBearingReaction)
         {
            CCombinedReactionTable().Build(pBroker,pChapter,span,girder,pDisplayUnits,pgsTypes::TemporaryStrandRemoval, analysisType,BearingReactionsTable, bDesign, bRating);
         }
      }

      p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      *pChapter << p;
      *p << _T("Responses - Deck and Diaphragm Placement (Bridge Site 1)") << rptNewLine;
      p->SetName(_T("Combined Results - Deck and Diaphragm Placement (Bridge Site 1)"));
      CCombinedMomentsTable().Build(pBroker,pChapter,span,girder,pDisplayUnits,pgsTypes::BridgeSite1, analysisType, bDesign, bRating);
      CCombinedShearTable().Build(pBroker,pChapter,span,girder,pDisplayUnits,pgsTypes::BridgeSite1, analysisType, bDesign, bRating);
      CCombinedReactionTable().Build(pBroker,pChapter,span,girder,pDisplayUnits,pgsTypes::BridgeSite1, analysisType,PierReactionsTable, bDesign, bRating);
      if(bDoBearingReaction)
      {
         CCombinedReactionTable().Build(pBroker,pChapter,span,girder,pDisplayUnits,pgsTypes::BridgeSite1, analysisType,BearingReactionsTable, bDesign, bRating);
      }

      p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      *pChapter << p;
      *p << _T("Responses - Final without Live Load (Bridge Site 2)") << rptNewLine;
      p->SetName(_T("Combined Results - Final without Live Load (Bridge Site 2)"));
      CCombinedMomentsTable().Build(pBroker,pChapter,span,girder,pDisplayUnits,pgsTypes::BridgeSite2, analysisType,bDesign,bRating);
      CCombinedShearTable().Build(pBroker,pChapter,span,girder,pDisplayUnits,pgsTypes::BridgeSite2, analysisType, bDesign, bRating);
      CCombinedReactionTable().Build(pBroker,pChapter,span,girder,pDisplayUnits,pgsTypes::BridgeSite2, analysisType,PierReactionsTable, bDesign, bRating);
      if(bDoBearingReaction)
      {
         CCombinedReactionTable().Build(pBroker,pChapter,span,girder,pDisplayUnits,pgsTypes::BridgeSite2, analysisType,BearingReactionsTable, bDesign, bRating);
      }
   }

   p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << p;
   *p << _T("Responses - Final with Live Load (Bridge Site 3)") << rptNewLine;
   p->SetName(_T("Combined Results - Final with Live Load (Bridge Site 3)"));
   CLiveLoadDistributionFactorTable().Build(pChapter,pBroker,span,girder,pDisplayUnits);
   CCombinedMomentsTable().Build(pBroker,pChapter,span,girder,pDisplayUnits,pgsTypes::BridgeSite3, analysisType,bDesign,bRating);
   CCombinedShearTable().Build(pBroker,pChapter,span,girder,pDisplayUnits,pgsTypes::BridgeSite3, analysisType,bDesign,bRating);
   if ( bDesign )
   {
      CCombinedReactionTable().Build(pBroker,pChapter,span,girder,pDisplayUnits,pgsTypes::BridgeSite3, analysisType,PierReactionsTable,bDesign,bRating);
      if(bDoBearingReaction)
      {
         CCombinedReactionTable().Build(pBroker,pChapter,span,girder,pDisplayUnits,pgsTypes::BridgeSite3, analysisType,BearingReactionsTable,bDesign,bRating);
      }
   }

   if ( bDesign )
   {
      p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      *pChapter << p;
      *p << _T("Live Load Reactions Without Impact") << rptNewLine;
      p->SetName(_T("Live Load Reactions Without Impact"));
      CCombinedReactionTable().BuildLiveLoad(pBroker,pChapter,span,girder,pDisplayUnits, analysisType,PierReactionsTable, false, true, false );

      if(bDoBearingReaction)
      {
         CCombinedReactionTable().BuildLiveLoad(pBroker,pChapter,span,girder,pDisplayUnits, analysisType,BearingReactionsTable, false, true, false );
      }
   }

   if ( pSpecEntry->GetShearCapacityMethod() == scmVciVcw )
   {
      p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      *pChapter << p;
      *p << _T("Concurrent Shears") << rptNewLine;
      p->SetName(_T("Concurrent Shears"));
      CConcurrentShearTable().Build(pBroker,pChapter,span,girder,pDisplayUnits,pgsTypes::BridgeSite3, analysisType);
   }

   return pChapter;
}

CChapterBuilder* CMVRChapterBuilder::Clone() const
{
   return new CMVRChapterBuilder(m_bDesign,m_bRating);
}
