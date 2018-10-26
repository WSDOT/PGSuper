///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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
#include <Reporting\ReportStyleHolder.h>
#include <Reporting\SpanGirderReportSpecification.h>
#include <Reporting\LibraryUsageParagraph.h>
#include <Reporting\GirderSeedDataComparisonParagraph.h>

#include <IFace\VersionInfo.h>
#include <IFace\Project.h>
#include <IFace\StatusCenter.h>
#include <EAF\EAFUIIntegration.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CPGSuperTitlePageBuilder::CPGSuperTitlePageBuilder(IBroker* pBroker,const char* strTitle,bool bFullVersion) :
m_pBroker(pBroker),
m_Title(strTitle),
m_bFullVersion(bFullVersion)
{
}

CPGSuperTitlePageBuilder::~CPGSuperTitlePageBuilder(void)
{
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
   boost::shared_ptr<CSpanReportSpecification> pSpanRptSpec = boost::dynamic_pointer_cast<CSpanReportSpecification,CReportSpecification>(pRptSpec);
   boost::shared_ptr<CSpanGirderReportSpecification> pSpanGirderRptSpec = boost::dynamic_pointer_cast<CSpanGirderReportSpecification,CReportSpecification>(pRptSpec);

   if ( pSpanGirderRptSpec != NULL )
   {
      SpanIndexType spanIdx = pSpanGirderRptSpec->GetSpan();
      GirderIndexType gdrIdx = pSpanGirderRptSpec->GetGirder();
      if ( spanIdx != INVALID_INDEX && gdrIdx != INVALID_INDEX )
      {
         *pPara << "For" << rptNewLine << rptNewLine;
         *pPara << "Span " << LABEL_SPAN(spanIdx) << " Girder " << LABEL_GIRDER(gdrIdx) << rptNewLine;
         *pPara << rptNewLine;
      }
      else if( spanIdx != INVALID_INDEX )
      {
         *pPara << "For" << rptNewLine << rptNewLine;
         *pPara << "Span " << LABEL_SPAN(spanIdx) << rptNewLine;
         *pPara << rptNewLine;
      }
      else if ( gdrIdx != NULL )
      {
         *pPara << "For" << rptNewLine << rptNewLine;
         *pPara << "Girder Line " << LABEL_GIRDER(gdrIdx) << rptNewLine;
         *pPara << rptNewLine;
      }
   }
   else if ( pSpanRptSpec != NULL )
   {
      SpanIndexType spanIdx = pSpanRptSpec->GetSpan();
      if ( spanIdx != INVALID_INDEX )
      {
         *pPara << "For" << rptNewLine << rptNewLine;
         *pPara << "Span " << LABEL_SPAN(spanIdx) << rptNewLine;
         *pPara << rptNewLine;
      }
   }

   pPara = new rptParagraph;
   pPara->SetStyleName(pgsReportStyleHolder::GetReportSubtitleStyle());
   *pTitlePage << pPara;
   *pPara << rptRcDateTime() << rptNewLine;

   if (m_bFullVersion)
      *pPara << rptNewLine << rptNewLine;

   pPara = new rptParagraph;
   pPara->SetStyleName(pgsReportStyleHolder::GetReportTitleStyle());
   *pTitlePage << pPara;
   *pPara << "PGSuper" << Super(symbol(TRADEMARK)) << rptNewLine;

   pPara = new rptParagraph(pgsReportStyleHolder::GetCopyrightStyle());
   *pTitlePage << pPara;
   *pPara << "Copyright " << symbol(COPYRIGHT) << " " << sysDate().Year() << ", WSDOT, All Rights Reserved" << rptNewLine;

   pPara = new rptParagraph;
   pPara->SetStyleName(pgsReportStyleHolder::GetReportSubtitleStyle());
   *pTitlePage << pPara;
   GET_IFACE(IVersionInfo,pVerInfo);
   *pPara << pVerInfo->GetVersionString() << rptNewLine;

   const std::string& strImage = pgsReportStyleHolder::GetReportCoverImage();
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
   GET_IFACE(IDocument,pDocument);

   rptParagraph* pPara3 = new rptParagraph( pgsReportStyleHolder::GetHeadingStyle() );
   *pTitlePage << pPara3;

   rptRcTable* pTbl = pgsReportStyleHolder::CreateTableNoHeading(2,"Project Properties");

   pTbl->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle( CB_NONE | CJ_LEFT ) );
   pTbl->SetColumnStyle(1,pgsReportStyleHolder::GetTableCellStyle( CB_NONE | CJ_LEFT ) );
   pTbl->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle( CB_NONE | CJ_LEFT ) );
   pTbl->SetStripeRowColumnStyle(1,pgsReportStyleHolder::GetTableStripeRowCellStyle( CB_NONE | CJ_LEFT ) );

   if (m_bFullVersion)
      *pPara3 << rptNewLine << rptNewLine << rptNewLine;

   *pPara3 << pTbl;
   (*pTbl)(0,0) << "Bridge Name";
   (*pTbl)(0,1) << pProps->GetBridgeName();
   (*pTbl)(1,0) << "Bridge ID";
   (*pTbl)(1,1) << pProps->GetBridgeId();
   (*pTbl)(2,0) << "Company";
   (*pTbl)(2,1) << pProps->GetCompany();
   (*pTbl)(3,0) << "Engineer";
   (*pTbl)(3,1) << pProps->GetEngineer();
   (*pTbl)(4,0) << "Job Number";
   (*pTbl)(4,1) << pProps->GetJobNumber();
   (*pTbl)(5,0) << "Comments";
   (*pTbl)(5,1) << pProps->GetComments();
   (*pTbl)(6,0) << "File";
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

   *p << "Library Usage" << rptNewLine;
   p = CLibraryUsageParagraph().Build(m_pBroker);
   *pTitlePage << p;

   // girder seed data comparison
   if ( pSpanGirderRptSpec != NULL || pSpanRptSpec != NULL )
   {
      SpanIndexType spanIdx  = ALL_SPANS;
      GirderIndexType gdrIdx = ALL_GIRDERS;

      if (pSpanGirderRptSpec != NULL )
      {
         spanIdx = pSpanGirderRptSpec->GetSpan();
         gdrIdx = pSpanGirderRptSpec->GetGirder();
      }
      else
      {
         spanIdx = pSpanRptSpec->GetSpan();
      }

      if ( spanIdx != INVALID_INDEX )
      {
         p = CGirderSeedDataComparisonParagraph().Build(m_pBroker, spanIdx, gdrIdx);

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

      *pPara << "Notes" << rptNewLine;

      pTable = pgsReportStyleHolder::CreateDefaultTable(2,"");
      pTable->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      pTable->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
      pTable->SetColumnStyle(1,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      pTable->SetStripeRowColumnStyle(1,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

      *pPara << pTable << rptNewLine;

      row = 0;
      (*pTable)(row,0) << "Symbol";
      (*pTable)(row++,1) << "Definition";

      (*pTable)(row,0) << Sub2("L","g");
      (*pTable)(row++,1) << "Length of Girder";

      (*pTable)(row,0) << Sub2("L","s");
      (*pTable)(row++,1) << "Length of Span";

      (*pTable)(row,0) << "FoS";
      (*pTable)(row++,1) << "Face of Support";

      //(*pTable)(row,0) << "XS";
      //(*pTable)(row++,1) << "Cross section change";

      (*pTable)(row,0) << "Debond";
      (*pTable)(row++,1) << "Point where bond begins for a debonded strand";

      (*pTable)(row,0) << "PSXFR";
      (*pTable)(row++,1) << "Point of prestress transfer";

      (*pTable)(row,0) << "DCS";
      (*pTable)(row++,1) << "Critical Section for Shear based on Design (Strength I) Loads";

      (*pTable)(row,0) << "PCS";
      (*pTable)(row++,1) << "Critical Section for Shear based on Permit (Strength II) Loads";

      (*pTable)(row,0) << "H";
      (*pTable)(row++,1) << "H from end of girder or face of support";

      (*pTable)(row,0) << "1.5H";
      (*pTable)(row++,1) << "1.5H from end of girder or face of support";

      (*pTable)(row,0) << "HP";
      (*pTable)(row++,1) << "Harp Point";

      (*pTable)(row,0) << "Pick Point";
      (*pTable)(row++,1) << "Support point where girder is lifted from form";

      (*pTable)(row,0) << "Bunk Point";
      (*pTable)(row++,1) << "Point where girder is supported during transportation";
   }

   // Status Center Items
   GET_IFACE(IStatusCenter,pStatusCenter);
   CollectionIndexType nItems = pStatusCenter->Count();

   if ( nItems != 0 )
   {
      pPara = new rptParagraph;
      pPara->SetStyleName(pgsReportStyleHolder::GetHeadingStyle());
      *pTitlePage << pPara;

      *pPara << "Status Items" << rptNewLine;

      pTable = pgsReportStyleHolder::CreateDefaultTable(2,"");
      pTable->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      pTable->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
      pTable->SetColumnStyle(1,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      pTable->SetStripeRowColumnStyle(1,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

      *pPara << pTable << rptNewLine;

      (*pTable)(0,0) << "Level";
      (*pTable)(0,1) << "Description";

      row = 1;
      CString strSeverityType[] = { "Info", "Warning", "Error" };
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
