///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright (C) 2006  Washington State Department of Transportation
//                     Bridge and Structures Office
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
#include <IFace\DisplayUnits.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Project.h>

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
CStressChapterBuilder::CStressChapterBuilder()
{
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
   CComPtr<IBroker> pBroker;
   pSGRptSpec->GetBroker(&pBroker);
   SpanIndexType span = pSGRptSpec->GetSpan();
   GirderIndexType girder = pSGRptSpec->GetGirder();

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   GET_IFACE2(pBroker,IDisplayUnits,pDispUnit);

   rptParagraph* p = 0;

   GET_IFACE2(pBroker,ISpecification,pSpec);
   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   // Product Stresses
   p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *p << "Product Load Stresses" << rptNewLine;
   p->SetName("Product Load Stresses");
   *pChapter << p;

   p = new rptParagraph;
   *pChapter << p;
   *p << CCastingYardStressTable().Build(pBroker,span,girder,pDispUnit) << rptNewLine;

   p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << p;
   p = new rptParagraph;
   *pChapter << p;
   *p << CProductStressTable().Build(pBroker,span,girder,analysisType,pDispUnit) << rptNewLine;
   *p << LIVELOAD_PER_LANE << rptNewLine;

   GET_IFACE2(pBroker,IUserDefinedLoads,pUDL);
   bool are_user_loads = pUDL->DoUserLoadsExist(span,girder);
   if (are_user_loads)
   {
      *p << CUserStressTable().Build(pBroker,span,girder,analysisType,pDispUnit) << rptNewLine;
   }

   // Load Combinations (DC, DW, etc) & Limit States
   p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << p;
   p->SetName("Combined Stresses - Casting Yard Stage");
   CCombinedStressTable().Build(pBroker,pChapter,span,girder,pDispUnit,pgsTypes::CastingYard, analysisType);

   p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << p;
   p->SetName("Combined Stresses - Girder Placement");
   CCombinedStressTable().Build(pBroker,pChapter,span,girder,pDispUnit,pgsTypes::GirderPlacement, analysisType);

   GET_IFACE2(pBroker,IProductForces,pForces);
   pgsTypes::Stage girderLoadStage = pForces->GetGirderDeadLoadStage(span,girder);
   if ( girderLoadStage == pgsTypes::TemporaryStrandRemoval )
   {
      p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      *pChapter << p;
      p->SetName("Combined Stresses - Temporary Strand Removal Stage");
      CCombinedStressTable().Build(pBroker,pChapter,span,girder,pDispUnit,pgsTypes::TemporaryStrandRemoval, analysisType);
   }

   p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << p;
   p->SetName("Combined Stresses - Deck and Diaphragm Placement (Bridge Site 1)");
   CCombinedStressTable().Build(pBroker,pChapter,span,girder,pDispUnit,pgsTypes::BridgeSite1, analysisType);
   
   p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << p;
   p->SetName("Combined Stresses - Superimposed Dead Loads (Bridge Site 2)");
   CCombinedStressTable().Build(pBroker,pChapter,span,girder,pDispUnit,pgsTypes::BridgeSite2, analysisType);

   p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << p;
   p->SetName("Combined Stresses - Final with Live Load (Bridge Site 3)");
   CCombinedStressTable().Build(pBroker,pChapter,span,girder,pDispUnit,pgsTypes::BridgeSite3, analysisType);

   p = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
   *pChapter << p;
   *p << LIVELOAD_PER_GIRDER << rptNewLine;

   p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << p;
   *p << "Stresses due to Prestress" << rptNewLine;
   p->SetName("Prestress");
   *p << CPrestressStressTable().Build(pBroker,span,girder,pDispUnit) << rptNewLine;

   return pChapter;
}

CChapterBuilder* CStressChapterBuilder::Clone() const
{
   return new CStressChapterBuilder;
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PRIVATE    ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
//======================== INQUERY    =======================================
