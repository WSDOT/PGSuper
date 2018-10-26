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
#include "TOGATitlePageBuilder.h"
#include <Reporting\ReportStyleHolder.h>
#include <Reporting\SpanGirderReportSpecification.h>
#include <Reporting\LibraryUsageParagraph.h>
#include <Reporting\GirderSeedDataComparisonParagraph.h>

#include "TxDOTOptionalDesignData.h"
#include "TxDOTOptionalDesignUtilities.h"

#include <IFace\VersionInfo.h>
#include <IFace\Project.h>
#include <IFace\StatusCenter.h>
#include <EAF\EAFUIIntegration.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CTOGATitlePageBuilder::CTOGATitlePageBuilder(IBroker* pBroker,const char* strTitle,bool bFullVersion) :
m_pBroker(pBroker),
m_Title(strTitle),
m_bFullVersion(bFullVersion)
{
}

CTOGATitlePageBuilder::~CTOGATitlePageBuilder(void)
{
}

bool CTOGATitlePageBuilder::NeedsUpdate(CReportHint* pHint,boost::shared_ptr<CReportSpecification>& pRptSpec)
{
   // don't let the title page control whether or not a report needs updating
   return false;
}

rptChapter* CTOGATitlePageBuilder::Build(boost::shared_ptr<CReportSpecification>& pRptSpec)
{
   // Create a title page for the report
   rptChapter* pTitlePage = new rptChapter;

   // A bit tricky here, but status center and other results won't be rebuilt until the toga model is built.
   // Let's ask for some results to get the ball rolling
   GET_IFACE(IGetTogaResults,pGetTogaResults);
   pGetTogaResults->GetRequiredFc();

   rptParagraph* pPara = new rptParagraph;
   pPara->SetStyleName(pgsReportStyleHolder::GetReportTitleStyle());
   *pTitlePage << pPara;
   *pPara << "TOGA"<< Super(symbol(TRADEMARK))<<", A PGSuper"<<Super(symbol(TRADEMARK))<<" Extension"<< rptNewLine;

   pPara = new rptParagraph(pgsReportStyleHolder::GetCopyrightStyle());
   *pTitlePage << pPara;
   *pPara << "Copyright " << symbol(COPYRIGHT) << " " << sysDate().Year() << ", TxDOT, All Rights Reserved" << rptNewLine;

   pPara = new rptParagraph;
   pPara->SetStyleName(pgsReportStyleHolder::GetReportSubtitleStyle());
   *pTitlePage << pPara;
   GET_IFACE(IVersionInfo,pVerInfo);
   *pPara << pVerInfo->GetVersionString() << rptNewLine;

   const std::string& strImage = pgsReportStyleHolder::GetImagePath() + std::string("TxDOT_Logo.gif");
   WIN32_FIND_DATA file_find_data;
   HANDLE hFind;
   hFind = FindFirstFile(strImage.c_str(),&file_find_data);
   if ( hFind != INVALID_HANDLE_VALUE )
   {
      *pPara << rptRcImage(strImage) << rptNewLine;
   }

   GET_IFACE(IProjectProperties,pProps);
   GET_IFACE(IEAFDocument,pDocument);

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
   (*pTbl)(2,0) << "Job Number";
   (*pTbl)(2,1) << pProps->GetJobNumber();
   (*pTbl)(3,0) << "Engineer";
   (*pTbl)(3,1) << pProps->GetEngineer();
   (*pTbl)(4,0) << "Company";
   (*pTbl)(4,1) << pProps->GetCompany();
   (*pTbl)(5,0) << "Comments";
   (*pTbl)(5,1) << pProps->GetComments();
   (*pTbl)(6,0) << "File";
   (*pTbl)(6,1) << pDocument->GetFilePath();

   // report library usage information
   rptParagraph* p = new rptParagraph( pgsReportStyleHolder::GetHeadingStyle() );
   *pTitlePage << p;

   *p << "Library Usage" << rptNewLine;
   p = CLibraryUsageParagraph().Build(m_pBroker, false);
   *pTitlePage << p;


   // girder seed data comparison
   p = CGirderSeedDataComparisonParagraph().Build(m_pBroker, TOGA_SPAN, TOGA_FABR_GDR);
   if (p != NULL)
   {
      // only report if we have data
      *pTitlePage << p;
   }

   rptRcTable* pTable;
   int row = 0;

   // Status Center Items
   GET_IFACE(IEAFStatusCenter,pStatusCenter);
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

      // Don't allow duplicate strings in table
      std::set<std::string> messages;

      row = 1;
      CString strSeverityType[] = { "Info", "Warning", "Error" };
      for ( CollectionIndexType i = 0; i < nItems; i++ )
      {
         CEAFStatusItem* pItem = pStatusCenter->GetByIndex(i);

         // Trim span/girder information. TOGA doesn't want this
         // Blasts anything left of the first ":"
         std::string msg = pItem->GetDescription();
         std::size_t loc = msg.find(':');
         if (loc != std::string::npos)
            msg.erase(0,loc+1);

         std::pair< std::set<std::string>::iterator, bool > it = messages.insert(msg);
         if (it.second) // no dup's
         {
            eafTypes::StatusSeverityType severity = pStatusCenter->GetSeverity(pItem);

            (*pTable)(row,0) << strSeverityType[severity];
            (*pTable)(row++,1) << msg;
         }
      }
   }

   return pTitlePage;
}
