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

#include "stdafx.h"
#include <Reporting\PGSpliceTitlePageBuilder.h>

#include <Reporting\SpanGirderReportSpecification.h>
#include <Reporting\LibraryUsageParagraph.h>
#include <Reporting\GirderSeedDataComparisonParagraph.h>

#include <IFace\VersionInfo.h>
#include <IFace\Project.h>
#include <IFace\StatusCenter.h>
#include <IFace\Bridge.h>
#include <EAF\EAFUIIntegration.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CPGSpliceTitlePageBuilder::CPGSpliceTitlePageBuilder(IBroker* pBroker,LPCTSTR strTitle,bool bFullVersion) :
CTitlePageBuilder(strTitle),
m_pBroker(pBroker),
m_bFullVersion(bFullVersion)
{
}

CPGSpliceTitlePageBuilder::CPGSpliceTitlePageBuilder(const CPGSpliceTitlePageBuilder& other) :
CTitlePageBuilder(other),
m_pBroker(other.m_pBroker),
m_bFullVersion(other.m_bFullVersion)
{
}


CPGSpliceTitlePageBuilder::~CPGSpliceTitlePageBuilder(void)
{
}

bool CPGSpliceTitlePageBuilder::NeedsUpdate(CReportHint* pHint,std::shared_ptr<CReportSpecification>& pRptSpec)
{
   // don't let the title page control whether or not a report needs updating
   return false;
}

rptChapter* CPGSpliceTitlePageBuilder::Build(std::shared_ptr<CReportSpecification>& pRptSpec)
{
   // Create a title page for the report
   rptChapter* pTitlePage = new rptChapter;

   rptParagraph* pPara = new rptParagraph;
   pPara->SetStyleName(rptStyleManager::GetReportTitleStyle());
   *pTitlePage << pPara;

   *pPara << GetReportTitle();

   pPara = new rptParagraph;
   pPara->SetStyleName(rptStyleManager::GetReportSubtitleStyle());
   *pTitlePage << pPara;

   // Determine if the report spec has span/girder information
   std::shared_ptr<CSpanReportSpecification>       pSpanRptSpec       = std::dynamic_pointer_cast<CSpanReportSpecification,CReportSpecification>(pRptSpec);
   std::shared_ptr<CGirderReportSpecification>     pGirderRptSpec     = std::dynamic_pointer_cast<CGirderReportSpecification,CReportSpecification>(pRptSpec);
   std::shared_ptr<CGirderLineReportSpecification> pGirderLineRptSpec = std::dynamic_pointer_cast<CGirderLineReportSpecification,CReportSpecification>(pRptSpec);

   if ( pGirderRptSpec != nullptr )
   {
      const CGirderKey& girderKey(pGirderRptSpec->GetGirderKey());
      GroupIndexType grpIdx  = girderKey.groupIndex;
      GirderIndexType gdrIdx = girderKey.girderIndex;

      if ( grpIdx != INVALID_INDEX && gdrIdx != INVALID_INDEX )
      {
         *pPara << _T("For") << rptNewLine << rptNewLine;
         *pPara << _T("Group ") << LABEL_GROUP(grpIdx) << _T(" Girder ") << LABEL_GIRDER(gdrIdx) << rptNewLine;
      }
      else if( grpIdx != INVALID_INDEX )
      {
         *pPara << _T("For") << rptNewLine << rptNewLine;
         *pPara << _T("Group ") << LABEL_GROUP(grpIdx) << rptNewLine;
      }
      else if ( gdrIdx != INVALID_INDEX )
      {
         *pPara << _T("For") << rptNewLine << rptNewLine;
         *pPara << _T("Girder ") << LABEL_GIRDER(gdrIdx) << rptNewLine;
      }
   }
   else if ( pSpanRptSpec != nullptr )
   {
      SpanIndexType spanIdx = pSpanRptSpec->GetSpan();
      if ( spanIdx != INVALID_INDEX )
      {
         *pPara << _T("For") << rptNewLine << rptNewLine;
         *pPara << _T("Span ") << LABEL_SPAN(spanIdx) << rptNewLine;
      }
   }
   else if ( pGirderLineRptSpec != nullptr )
   {
      GirderIndexType gdrIdx = pGirderLineRptSpec->GetGirderIndex();
      ATLASSERT(gdrIdx != INVALID_INDEX);
      *pPara << _T("For") << rptNewLine << rptNewLine;
      *pPara << _T("Girder Line ") << LABEL_GIRDER(gdrIdx) << rptNewLine;
   }
   *pPara << rptNewLine;

   pPara = new rptParagraph;
   pPara->SetStyleName(rptStyleManager::GetReportSubtitleStyle());
   *pTitlePage << pPara;
   *pPara << rptRcDateTime() << rptNewLine;

   if (m_bFullVersion)
   {
      *pPara << rptNewLine << rptNewLine;
   }

   pPara = new rptParagraph;
   pPara->SetStyleName(rptStyleManager::GetReportTitleStyle());
   *pTitlePage << pPara;
#if defined _WIN64
   *pPara << _T("PGSplice") << Super(symbol(TRADEMARK)) << _T(" (x64)") << rptNewLine;
#else
   *pPara << _T("PGSplice") << Super(symbol(TRADEMARK)) << _T(" (x86)") << rptNewLine;
#endif

   pPara = new rptParagraph(rptStyleManager::GetCopyrightStyle());
   *pTitlePage << pPara;
   *pPara << _T("Copyright ") << symbol(COPYRIGHT) << _T(" ") << sysDate().Year() << _T(", WSDOT, All Rights Reserved") << rptNewLine;

   pPara = new rptParagraph;
   pPara->SetStyleName(rptStyleManager::GetReportSubtitleStyle());
   *pTitlePage << pPara;
   GET_IFACE(IVersionInfo,pVerInfo);
   *pPara << pVerInfo->GetVersionString() << rptNewLine;

   const std::_tstring& strImage = rptStyleManager::GetReportCoverImage();
   WIN32_FIND_DATA file_find_data;
   HANDLE hFind;
   hFind = FindFirstFile(strImage.c_str(),&file_find_data);
   if ( hFind != INVALID_HANDLE_VALUE )
   {
      *pPara << rptRcImage(strImage) << rptNewLine;
   }

   if (m_bFullVersion)
   {
      *pPara << rptNewLine << rptNewLine;
   }

   GET_IFACE(IProjectProperties,pProps);
   GET_IFACE(IEAFDocument,pDocument);

   rptParagraph* pPara3 = new rptParagraph( rptStyleManager::GetHeadingStyle() );
   *pTitlePage << pPara3;

   rptRcTable* pTbl = rptStyleManager::CreateTableNoHeading(2,_T("Project Properties"));

   pTbl->SetColumnStyle(0,rptStyleManager::GetTableCellStyle( CB_NONE | CJ_LEFT ) );
   pTbl->SetColumnStyle(1,rptStyleManager::GetTableCellStyle( CB_NONE | CJ_LEFT ) );
   pTbl->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle( CB_NONE | CJ_LEFT ) );
   pTbl->SetStripeRowColumnStyle(1,rptStyleManager::GetTableStripeRowCellStyle( CB_NONE | CJ_LEFT ) );

   if (m_bFullVersion)
   {
      *pPara3 << rptNewLine << rptNewLine << rptNewLine;
   }

   *pPara3 << pTbl;
   (*pTbl)(0,0) << _T("Bridge Name");
   (*pTbl)(0,1) << pProps->GetBridgeName();
   (*pTbl)(1,0) << _T("Bridge ID");
   (*pTbl)(1,1) << pProps->GetBridgeID();
   (*pTbl)(2,0) << _T("Company");
   (*pTbl)(2,1) << pProps->GetCompany();
   (*pTbl)(3,0) << _T("Engineer");
   (*pTbl)(3,1) << pProps->GetEngineer();
   (*pTbl)(4,0) << _T("Job Number");
   (*pTbl)(4,1) << pProps->GetJobNumber();
   (*pTbl)(5,0) << _T("Comments");
   (*pTbl)(5,1) << pProps->GetComments();
   (*pTbl)(6,0) << _T("File");
   (*pTbl)(6,1) << pDocument->GetFilePath();


   rptParagraph* p = new rptParagraph;
   *pTitlePage << p;

   // Throw in a page break
   if (m_bFullVersion)
   {
      *p << rptNewPage;
   }

   // report library usage information
   p = new rptParagraph( rptStyleManager::GetHeadingStyle() );
   *pTitlePage << p;

   if (m_bFullVersion)
   {
      *p << rptNewLine << rptNewLine;
   }

   *p << _T("Configuration") << rptNewLine;
   p = CLibraryUsageParagraph().Build(m_pBroker);
   *pTitlePage << p;

   // There isn't any seed data for spliced girders
   //// girder seed data comparison
   //if ( pGirderRptSpec != nullptr || pSpanRptSpec != nullptr )
   //{
   //   CGirderKey girderKey;

   //   if (pGirderRptSpec != nullptr )
   //   {
   //      girderKey = pGirderRptSpec->GetGirderKey();
   //   }
   //   else
   //   {
   //      GET_IFACE(IBridge,pBridge);
   //      girderKey.groupIndex = pBridge->GetGirderGroupIndex(pSpanRptSpec->GetSpan());
   //      girderKey.girderIndex = ALL_GIRDERS;
   //   }

   //   p = CGirderSeedDataComparisonParagraph().Build(m_pBroker,girderKey);

   //   if (p != nullptr)
   //   {
   //      // only report if we have data
   //      *pTitlePage << p;
   //   }
   //}

   rptRcTable* pTable;
   int row = 0;

   if (m_bFullVersion)
   {      
      rptParagraph* pPara = new rptParagraph;
      pPara->SetStyleName(rptStyleManager::GetHeadingStyle());
      *pTitlePage << pPara;

      *pPara << _T("Notes") << rptNewLine;

      pTable = rptStyleManager::CreateDefaultTable(2);
      pTable->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
      pTable->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
      pTable->SetColumnStyle(1,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
      pTable->SetStripeRowColumnStyle(1,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

      *pPara << pTable << rptNewLine;

      row = 0;
      (*pTable)(row,0) << _T("Symbol");
      (*pTable)(row++,1) << _T("Definition");

      (*pTable)(row,0) << Sub2(_T("L"),_T("r"));
      (*pTable)(row++,1) << _T("Span Length of Segment at Release");

      (*pTable)(row,0) << Sub2(_T("L"),_T("l"));
      (*pTable)(row++,1) << _T("Span Length of Segment during Lifting");

      (*pTable)(row,0) << Sub2(_T("L"),_T("st"));
      (*pTable)(row++,1) << _T("Span Length of Segment during Storage");

      (*pTable)(row,0) << Sub2(_T("L"),_T("h"));
      (*pTable)(row++,1) << _T("Span Length of Segment during Hauling");

      (*pTable)(row,0) << Sub2(_T("L"),_T("e"));
      (*pTable)(row++,1) << _T("Span Length of Segment after Erection");

      (*pTable)(row,0) << Sub2(_T("L"),_T("s"));
      (*pTable)(row++,1) << _T("Length of Span");

      (*pTable)(row,0) << _T("FoS");
      (*pTable)(row++,1) << _T("Face of Support");

      (*pTable)(row, 0) << _T("ST");
      (*pTable)(row++, 1) << _T("Section Transitions");

      (*pTable)(row, 0) << _T("STLF");
      (*pTable)(row++, 1) << _T("Section Transitions, Left Face");

      (*pTable)(row, 0) << _T("STRF");
      (*pTable)(row++, 1) << _T("Section Transitions, Right Face");

      (*pTable)(row,0) << _T("IP");
      (*pTable)(row++,1) << _T("Interior Pier");

      (*pTable)(row,0) << _T("CJ");
      (*pTable)(row++,1) << _T("Closure Joint");

      (*pTable)(row,0) << _T("ITS");
      (*pTable)(row++,1) << _T("Interior Temporary Support");

      (*pTable)(row,0) << _T("Debond");
      (*pTable)(row++,1) << _T("Point where bond begins for a debonded strand");

      (*pTable)(row,0) << _T("PSXFR");
      (*pTable)(row++,1) << _T("Point of prestress transfer");

      (*pTable)(row,0) << _T("Diaphragm");
      (*pTable)(row++,1) << _T("Location of a precast or cast in place diaphragm");

      (*pTable)(row,0) << _T("Bar Cutoff");
      (*pTable)(row++,1) << _T("End of a reinforcing bar in the girder");

      (*pTable)(row,0) << _T("Deck Bar Cutoff");
      (*pTable)(row++,1) << _T("End of a reinforcing bar in the deck");

      if ( lrfdVersionMgr::ThirdEdition2004 <= lrfdVersionMgr::GetVersion() )
      {
         (*pTable)(row,0) << _T("CS");
         (*pTable)(row++,1) << _T("Critical Section for Shear");
      }
      else
      {
         (*pTable)(row,0) << _T("DCS");
         (*pTable)(row++,1) << _T("Critical Section for Shear based on Design (Strength I) Loads");

         (*pTable)(row,0) << _T("PCS");
         (*pTable)(row++,1) << _T("Critical Section for Shear based on Permit (Strength II) Loads");
      }

      (*pTable)(row,0) << _T("SZB");
      (*pTable)(row++,1) << _T("Stirrup Zone Boundary");

      (*pTable)(row,0) << _T("H");
      (*pTable)(row++,1) << _T("H from end of girder or face of support");

      (*pTable)(row,0) << _T("1.5H");
      (*pTable)(row++,1) << _T("1.5H from end of girder or face of support");

      (*pTable)(row,0) << _T("HP");
      (*pTable)(row++,1) << _T("Harp Point");

      (*pTable)(row,0) << _T("Pick Point");
      (*pTable)(row++,1) << _T("Support point where girder is lifted from form");

      (*pTable)(row,0) << _T("Bunk Point");
      (*pTable)(row++,1) << _T("Point where girder is supported during transportation");
   }

   // Status Center Items
   GET_IFACE(IEAFStatusCenter,pStatusCenter);
   CollectionIndexType nItems = pStatusCenter->Count();

   if ( nItems != 0 )
   {
      pPara = new rptParagraph;
      pPara->SetStyleName(rptStyleManager::GetHeadingStyle());
      *pTitlePage << pPara;

      *pPara << _T("Status Items") << rptNewLine;

      pTable = rptStyleManager::CreateDefaultTable(2);
      pTable->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
      pTable->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
      pTable->SetColumnStyle(1,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
      pTable->SetStripeRowColumnStyle(1,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

      *pPara << pTable << rptNewLine;

      (*pTable)(0,0) << _T("Level");
      (*pTable)(0,1) << _T("Description");

      row = 1;
      CString strSeverityType[] = { _T("Info"), _T("Warning"), _T("Error") };
      for ( CollectionIndexType i = 0; i < nItems; i++ )
      {
         CEAFStatusItem* pItem = pStatusCenter->GetByIndex(i);

         eafTypes::StatusSeverityType severity = pStatusCenter->GetSeverity(pItem);

         (*pTable)(row,0) << strSeverityType[severity];
         (*pTable)(row++,1) << pItem->GetDescription();
      }
   }

   // Throw in a page break
   p = new rptParagraph;
   *pTitlePage << p;
   *p << rptNewPage;

   return pTitlePage;
}

CTitlePageBuilder* CPGSpliceTitlePageBuilder::Clone() const
{
   return new CPGSpliceTitlePageBuilder(*this);
}
