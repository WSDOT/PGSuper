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

#include "stdafx.h"
#include <Reporting\PGSuperTitlePageBuilder.h>
#include <PgsExt\ReportStyleHolder.h>
#include <Reporting\SpanGirderReportSpecification.h>
#include <Reporting\LibraryUsageParagraph.h>
#include <Reporting\GirderSeedDataComparisonParagraph.h>

#include <PgsExt\SegmentRelatedStatusItem.h>

#include <IFace\VersionInfo.h>
#include <IFace\Project.h>
#include <IFace\StatusCenter.h>
#include <EAF\EAFUIIntegration.h>
#include <IFace\Bridge.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// inline functions to determine whether to print status center items
static bool DoPrintStatusItem(CEAFStatusItem* pItem, const CGirderKey& girderKey,SegmentIndexType nSegments)
{
   pgsSegmentRelatedStatusItem* pSegmentStatusItem = dynamic_cast<pgsSegmentRelatedStatusItem*>(pItem);
   if (pSegmentStatusItem != NULL)
   {
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         CSegmentKey segmentKey(girderKey,segIdx);
         return pSegmentStatusItem->IsRelatedTo(segmentKey);
      }
   }
   else
   {
      return true;
   }

   return false;
}

static bool DoPrintStatusCenter(IEAFStatusCenter* pStatusCenter, CollectionIndexType nItems, const CGirderKey& girderKey,SegmentIndexType nSegments)
{
   for ( CollectionIndexType i = 0; i < nItems; i++ )
   {
      CEAFStatusItem* pItem = pStatusCenter->GetByIndex(i);

      if (DoPrintStatusItem(pItem, girderKey, nSegments))
         return true;
   }

   return false;
}

CPGSuperTitlePageBuilder::CPGSuperTitlePageBuilder(IBroker* pBroker,LPCTSTR strTitle,bool bFullVersion) :
m_pBroker(pBroker),
m_Title(strTitle),
m_bFullVersion(bFullVersion)
{
}

CPGSuperTitlePageBuilder::~CPGSuperTitlePageBuilder(void)
{
}

bool CPGSuperTitlePageBuilder::NeedsUpdate(CReportHint* pHint,boost::shared_ptr<CReportSpecification>& pRptSpec)
{
   // don't let the title page control whether or not a report needs updating
   return false;
}

rptChapter* CPGSuperTitlePageBuilder::Build(boost::shared_ptr<CReportSpecification>& pRptSpec)
{
   // Create a title page for the report
   rptChapter* pTitlePage = new rptChapter;

   rptParagraph* pPara = new rptParagraph;
   pPara->SetStyleName(pgsReportStyleHolder::GetReportTitleStyle());
   *pTitlePage << pPara;

   *pPara << m_Title.c_str();

   pPara = new rptParagraph;
   pPara->SetStyleName(pgsReportStyleHolder::GetReportSubtitleStyle());
   *pTitlePage << pPara;

   // Determine if the report spec has span/girder information
   boost::shared_ptr<CSpanReportSpecification>       pSpanRptSpec       = boost::dynamic_pointer_cast<CSpanReportSpecification,CReportSpecification>(pRptSpec);
   boost::shared_ptr<CGirderReportSpecification>     pGirderRptSpec     = boost::dynamic_pointer_cast<CGirderReportSpecification,CReportSpecification>(pRptSpec);
   boost::shared_ptr<CGirderLineReportSpecification> pGirderLineRptSpec = boost::dynamic_pointer_cast<CGirderLineReportSpecification,CReportSpecification>(pRptSpec);

   CGirderKey girderKey;
   GET_IFACE(IBridge,pBridge);

   bool bGirderReport = true;
   if ( pGirderRptSpec != NULL )
   {
      girderKey = pGirderRptSpec->GetGirderKey();
      GroupIndexType grpIdx  = girderKey.groupIndex;
      GirderIndexType gdrIdx = girderKey.girderIndex;

      if ( grpIdx != INVALID_INDEX && gdrIdx != INVALID_INDEX )
      {
         *pPara << _T("For") << rptNewLine << rptNewLine;
         *pPara << _T("Span ") << LABEL_SPAN(grpIdx) << _T(" Girder ") << LABEL_GIRDER(gdrIdx) << rptNewLine;
      }
      else if( grpIdx != INVALID_INDEX )
      {
         *pPara << _T("For") << rptNewLine << rptNewLine;
         *pPara << _T("Span ") << LABEL_SPAN(grpIdx) << rptNewLine;
      }
      else if ( gdrIdx != NULL )
      {
         *pPara << _T("For") << rptNewLine << rptNewLine;
         *pPara << _T("Girder ") << LABEL_GIRDER(gdrIdx) << rptNewLine;
      }
   }
   else if ( pSpanRptSpec != NULL )
   {
      SpanIndexType spanIdx = pSpanRptSpec->GetSpan();
      girderKey.girderIndex = pBridge->GetGirderGroupIndex(spanIdx);

      if ( spanIdx != INVALID_INDEX )
      {
         *pPara << _T("For") << rptNewLine << rptNewLine;
         *pPara << _T("Span ") << LABEL_SPAN(spanIdx) << rptNewLine;
      }
   }
   else if ( pGirderLineRptSpec != NULL )
   {
      bGirderReport = false;
      GirderIndexType gdrIdx = pGirderLineRptSpec->GetGirderIndex();
      girderKey.girderIndex = gdrIdx;
      ATLASSERT(gdrIdx != INVALID_INDEX);
      *pPara << _T("For") << rptNewLine << rptNewLine;
      *pPara << _T("Girder Line ") << LABEL_GIRDER(gdrIdx) << rptNewLine;
   }
   else
   {
      bGirderReport = false;
   }
   *pPara << rptNewLine;

   pPara = new rptParagraph;
   pPara->SetStyleName(pgsReportStyleHolder::GetReportSubtitleStyle());
   *pTitlePage << pPara;
   *pPara << rptRcDateTime() << rptNewLine;

   if (m_bFullVersion)
      *pPara << rptNewLine << rptNewLine;

   pPara = new rptParagraph;
   pPara->SetStyleName(pgsReportStyleHolder::GetReportTitleStyle());
   *pTitlePage << pPara;
#if defined _WIN64
   *pPara << _T("PGSuper") << Super(symbol(TRADEMARK)) << _T(" (x64)") << rptNewLine;
#else
   *pPara << _T("PGSuper") << Super(symbol(TRADEMARK)) << _T(" (x86)") << rptNewLine;
#endif

   pPara = new rptParagraph(pgsReportStyleHolder::GetCopyrightStyle());
   *pTitlePage << pPara;
   *pPara << _T("Copyright ") << symbol(COPYRIGHT) << _T(" ") << sysDate().Year() << _T(", WSDOT, All Rights Reserved") << rptNewLine;

   pPara = new rptParagraph;
   pPara->SetStyleName(pgsReportStyleHolder::GetReportSubtitleStyle());
   *pTitlePage << pPara;
   GET_IFACE(IVersionInfo,pVerInfo);
   *pPara << pVerInfo->GetVersionString() << rptNewLine;

   const std::_tstring& strImage = pgsReportStyleHolder::GetReportCoverImage();
   WIN32_FIND_DATA file_find_data;
   HANDLE hFind;
   hFind = FindFirstFile(strImage.c_str(),&file_find_data);
   if ( hFind != INVALID_HANDLE_VALUE )
   {
      *pPara << rptRcImage(strImage) << rptNewLine;
   }

   if (m_bFullVersion)
      *pPara << rptNewLine << rptNewLine;

   GET_IFACE(IProjectProperties,pProps);
   GET_IFACE(IEAFDocument,pDocument);

   rptParagraph* pPara3 = new rptParagraph( pgsReportStyleHolder::GetHeadingStyle() );
   *pTitlePage << pPara3;

   rptRcTable* pTbl = pgsReportStyleHolder::CreateTableNoHeading(2,_T("Project Properties"));

   pTbl->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle( CB_NONE | CJ_LEFT ) );
   pTbl->SetColumnStyle(1,pgsReportStyleHolder::GetTableCellStyle( CB_NONE | CJ_LEFT ) );
   pTbl->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle( CB_NONE | CJ_LEFT ) );
   pTbl->SetStripeRowColumnStyle(1,pgsReportStyleHolder::GetTableStripeRowCellStyle( CB_NONE | CJ_LEFT ) );

   if (m_bFullVersion)
      *pPara3 << rptNewLine << rptNewLine << rptNewLine;

   *pPara3 << pTbl;
   (*pTbl)(0,0) << _T("Bridge Name");
   (*pTbl)(0,1) << pProps->GetBridgeName();
   (*pTbl)(1,0) << _T("Bridge ID");
   (*pTbl)(1,1) << pProps->GetBridgeId();
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
      *p << rptNewPage;

   // report library usage information
   p = new rptParagraph( pgsReportStyleHolder::GetHeadingStyle() );
   *pTitlePage << p;

   if (m_bFullVersion)
      *p << rptNewLine << rptNewLine;

   *p << _T("Library Usage") << rptNewLine;
   p = CLibraryUsageParagraph().Build(m_pBroker);
   *pTitlePage << p;

   // girder seed data comparison
   if ( pGirderRptSpec != NULL || pSpanRptSpec != NULL )
   {
      if ( girderKey.groupIndex != INVALID_INDEX )
      {
         p = CGirderSeedDataComparisonParagraph().Build(m_pBroker,girderKey);

         if (p != NULL)
         {
            // only report if we have data
            *pTitlePage << p;
         }
      }
   }

   rptRcTable* pTable;
   int row = 0;

   if (m_bFullVersion)
   {
      rptParagraph* pPara = new rptParagraph;
      pPara->SetStyleName(pgsReportStyleHolder::GetHeadingStyle());
      *pTitlePage << pPara;

      *pPara << _T("Notes") << rptNewLine;

      pTable = pgsReportStyleHolder::CreateDefaultTable(2);
      pTable->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      pTable->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
      pTable->SetColumnStyle(1,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      pTable->SetStripeRowColumnStyle(1,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

      *pPara << pTable << rptNewLine;

      row = 0;
      (*pTable)(row,0) << _T("Symbol");
      (*pTable)(row++,1) << _T("Definition");

      (*pTable)(row,0) << Sub2(_T("L"),_T("g"));
      (*pTable)(row++,1) << _T("Length of Girder");

      (*pTable)(row,0) << Sub2(_T("L"),_T("s"));
      (*pTable)(row++,1) << _T("Length of Span");

      (*pTable)(row,0) << _T("FoS");
      (*pTable)(row++,1) << _T("Face of Support");

      //(*pTable)(row,0) << _T("XS");
      //(*pTable)(row++,1) << _T("Cross section change");

      (*pTable)(row,0) << _T("Debond");
      (*pTable)(row++,1) << _T("Point where bond begins for a debonded strand");

      (*pTable)(row,0) << _T("PSXFR");
      (*pTable)(row++,1) << _T("Point of prestress transfer");

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
   if ( bGirderReport )
   {
      GET_IFACE(IEAFStatusCenter,pStatusCenter);
      CollectionIndexType nItems = pStatusCenter->Count();

      GroupIndexType firstGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
      GroupIndexType lastGroupIdx  = (girderKey.groupIndex == ALL_GROUPS ? firstGroupIdx : pBridge->GetGirderGroupCount()-1);
      for ( GroupIndexType grpIdx = firstGroupIdx; grpIdx <= lastGroupIdx; grpIdx++ )
      {
         CGirderKey thisGirderKey(grpIdx,girderKey.girderIndex);
         SegmentIndexType nSegments = pBridge->GetSegmentCount(thisGirderKey);
         if ( DoPrintStatusCenter(pStatusCenter, nItems, thisGirderKey, nSegments) )
         {
            pPara = new rptParagraph;
            pPara->SetStyleName(pgsReportStyleHolder::GetHeadingStyle());
            *pTitlePage << pPara;

            *pPara << _T("Status Items") << rptNewLine;

            pTable = pgsReportStyleHolder::CreateDefaultTable(2);
            pTable->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
            pTable->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
            pTable->SetColumnStyle(1,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
            pTable->SetStripeRowColumnStyle(1,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

            *pPara << pTable << rptNewLine;

            (*pTable)(0,0) << _T("Level");
            (*pTable)(0,1) << _T("Description");

            row = 1;
            CString strSeverityType[] = { _T("Info"), _T("Warning"), _T("Error") };
            for ( CollectionIndexType i = 0; i < nItems; i++ )
            {
               CEAFStatusItem* pItem = pStatusCenter->GetByIndex(i);
               
               if ( DoPrintStatusItem(pItem, thisGirderKey, nSegments) )
               {
                  eafTypes::StatusSeverityType severity = pStatusCenter->GetSeverity(pItem);

                  (*pTable)(row,0) << strSeverityType[severity];
                  (*pTable)(row++,1) << pItem->GetDescription();
               }
            }
         } // next segment
      } // next group
   } // end if

   // Throw in a page break
   p = new rptParagraph;
   *pTitlePage << p;
   *p << rptNewPage;

   return pTitlePage;
}
