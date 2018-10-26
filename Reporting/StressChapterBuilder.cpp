///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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
#include <Reporting\StressChapterBuilder.h>
#include <Reporting\ReportNotes.h>
#include <Reporting\CastingYardStressTable.h>
#include <Reporting\ProductStressTable.h>
#include <Reporting\UserStressTable.h>
#include <Reporting\CombinedStressTable.h>
#include <Reporting\PrestressStressTable.h>
#include <Reporting\LiveLoadDistributionFactorTable.h>

#include <IFace\Bridge.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Project.h>
#include <IFace\RatingSpecification.h>

#include <psgLib\SpecLibraryEntry.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CStressChapterBuilder
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CStressChapterBuilder::CStressChapterBuilder(bool bDesign,bool bRating,bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
   m_bDesign = bDesign;
   m_bRating = bRating;
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CStressChapterBuilder::GetName() const
{
   return TEXT("Stresses");
}

rptChapter* CStressChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
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
      GET_IFACE2(pBroker,IRatingSpecification,pRatingSpec);
      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) ||
         //pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating) || // operating does not apply
           pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) ||
           pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) 
         )
      {
         bRating = true;
      }
      else
      {
         // if only permit rating is enabled, there aren't any stresses to report
         bRating = false;
      }
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

   // Product Stresses
   p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *p << _T("Product Load Stresses") << rptNewLine;
   p->SetName(_T("Product Load Stresses"));
   *pChapter << p;

   if ( bDesign )
   {
      p = new rptParagraph;
      *pChapter << p;
      *p << CCastingYardStressTable().Build(pBroker,span,girder,pDisplayUnits) << rptNewLine;
   }

   p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << p;
   p = new rptParagraph;
   *pChapter << p;
   *p << CProductStressTable().Build(pBroker,span,girder,analysisType,bDesign,bRating,pDisplayUnits) << rptNewLine;
   *p << LIVELOAD_PER_LANE << rptNewLine;

   GET_IFACE2(pBroker,IUserDefinedLoads,pUDL);
   bool are_user_loads = pUDL->DoUserLoadsExist(span,girder);
   if (are_user_loads)
   {
      *p << CUserStressTable().Build(pBroker,span,girder,analysisType,pDisplayUnits) << rptNewLine;
   }

   // Load Combinations (DC, DW, etc) & Limit States
   if ( bDesign )
   {
      p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      *pChapter << p;
      p->SetName(_T("Combined Stresses - Casting Yard Stage"));
      CCombinedStressTable().Build(pBroker,pChapter,span,girder,pDisplayUnits,pgsTypes::CastingYard, analysisType, bDesign, bRating);

      p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      *pChapter << p;
      p->SetName(_T("Combined Stresses - Girder Placement"));
      CCombinedStressTable().Build(pBroker,pChapter,span,girder,pDisplayUnits,pgsTypes::GirderPlacement, analysisType, bDesign, bRating);

      GET_IFACE2(pBroker,IProductLoads,pLoads);
      pgsTypes::Stage girderLoadStage = pLoads->GetGirderDeadLoadStage(span,girder);
      if ( girderLoadStage == pgsTypes::TemporaryStrandRemoval )
      {
         p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
         *pChapter << p;
         p->SetName(_T("Combined Stresses - Temporary Strand Removal Stage"));
         CCombinedStressTable().Build(pBroker,pChapter,span,girder,pDisplayUnits,pgsTypes::TemporaryStrandRemoval, analysisType, bDesign, bRating);
      }

      p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      *pChapter << p;
      p->SetName(_T("Combined Stresses - Deck and Diaphragm Placement (Bridge Site 1)"));
      CCombinedStressTable().Build(pBroker,pChapter,span,girder,pDisplayUnits,pgsTypes::BridgeSite1, analysisType, bDesign, bRating);
      
      p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      *pChapter << p;
      p->SetName(_T("Combined Stresses - Superimposed Dead Loads (Bridge Site 2)"));
      CCombinedStressTable().Build(pBroker,pChapter,span,girder,pDisplayUnits,pgsTypes::BridgeSite2, analysisType, bDesign, bRating);
   }

   p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << p;
   p->SetName(_T("Combined Stresses - Final with Live Load (Bridge Site 3)"));
   CCombinedStressTable().Build(pBroker,pChapter,span,girder,pDisplayUnits,pgsTypes::BridgeSite3, analysisType, bDesign, bRating);

   p = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
   *pChapter << p;
   *p << LIVELOAD_PER_GIRDER << rptNewLine;

   p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << p;
   *p << _T("Stresses due to Prestress") << rptNewLine;
   p->SetName(_T("Prestress"));
   *p << CPrestressStressTable().Build(pBroker,span,girder,bDesign,pDisplayUnits) << rptNewLine;

   return pChapter;
}

CChapterBuilder* CStressChapterBuilder::Clone() const
{
   return new CStressChapterBuilder(m_bDesign,m_bRating);
}
