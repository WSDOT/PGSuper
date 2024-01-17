///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2024  Washington State Department of Transportation
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

CTOGATitlePageBuilder::CTOGATitlePageBuilder(IBroker* pBroker,LPCTSTR strTitle,bool bFullVersion) :
WBFL::Reporting::TitlePageBuilder(strTitle),
m_pBroker(pBroker),
m_bFullVersion(bFullVersion)
{
}

CTOGATitlePageBuilder::CTOGATitlePageBuilder(const CTOGATitlePageBuilder& other) :
WBFL::Reporting::TitlePageBuilder(other),
m_pBroker(other.m_pBroker),
m_bFullVersion(other.m_bFullVersion)
{
}

CTOGATitlePageBuilder::~CTOGATitlePageBuilder(void)
{
}

std::unique_ptr<WBFL::Reporting::TitlePageBuilder> CTOGATitlePageBuilder::Clone() const
{
   return std::make_unique<CTOGATitlePageBuilder>(*this);
}


bool CTOGATitlePageBuilder::NeedsUpdate(const std::shared_ptr<const WBFL::Reporting::ReportHint>& pHint, const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec) const
{
   // don't let the title page control whether or not a report needs updating
   return false;
}

rptChapter* CTOGATitlePageBuilder::Build(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec) const
{
   // Create a title page for the report
   rptChapter* pTitlePage = new rptChapter;

   // A bit tricky here, but status center and other results won't be rebuilt until the toga model is built.
   // Let's ask for some results to get the ball rolling
   GET_IFACE(IGetTogaResults,pGetTogaResults);
   pGetTogaResults->GetRequiredFc();

   rptParagraph* pPara = new rptParagraph;
   pPara->SetStyleName(rptStyleManager::GetReportTitleStyle());
   *pTitlePage << pPara;
#if defined _WIN64
   *pPara << _T("TOGA")<< Super(symbol(TRADEMARK))<<_T(" (x64), a PGSuper")<<Super(symbol(TRADEMARK))<<_T(" Extension")<< rptNewLine;
#else
   *pPara << _T("TOGA")<< Super(symbol(TRADEMARK))<<_T(", a PGSuper")<<Super(symbol(TRADEMARK))<<_T(" Extension")<< rptNewLine;
#endif

   pPara = new rptParagraph(rptStyleManager::GetCopyrightStyle());
   *pTitlePage << pPara;
   *pPara << _T("Copyright ") << symbol(COPYRIGHT) << _T(" ") << WBFL::System::Date().Year() << _T(", TxDOT, All Rights Reserved") << rptNewLine;

   pPara = new rptParagraph;
   pPara->SetStyleName(rptStyleManager::GetReportSubtitleStyle());
   *pTitlePage << pPara;
   GET_IFACE(IVersionInfo,pVerInfo);
   *pPara << pVerInfo->GetVersionString() << rptNewLine;

   const std::_tstring& strImage = std::_tstring(rptStyleManager::GetImagePath()) + std::_tstring(_T("TxDOT_Logo.gif"));
   WIN32_FIND_DATA file_find_data;
   HANDLE hFind;
   hFind = FindFirstFile(strImage.c_str(),&file_find_data);
   if ( hFind != INVALID_HANDLE_VALUE )
   {
      *pPara << rptRcImage(strImage) << rptNewLine;
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
      *pPara3 << rptNewLine << rptNewLine << rptNewLine;

   *pPara3 << pTbl;
   (*pTbl)(0,0) << _T("Bridge Name");
   (*pTbl)(0,1) << pProps->GetBridgeName();
   (*pTbl)(1,0) << _T("Bridge ID");
   (*pTbl)(1,1) << pProps->GetBridgeID();
   (*pTbl)(2,0) << _T("Job Number");
   (*pTbl)(2,1) << pProps->GetJobNumber();
   (*pTbl)(3,0) << _T("Engineer");
   (*pTbl)(3,1) << pProps->GetEngineer();
   (*pTbl)(4,0) << _T("Company");
   (*pTbl)(4,1) << pProps->GetCompany();
   (*pTbl)(5,0) << _T("Comments");
   (*pTbl)(5,1) << pProps->GetComments();
   (*pTbl)(6,0) << _T("File");
   (*pTbl)(6,1) << pDocument->GetFilePath();

   // report library usage information
   rptParagraph* p = new rptParagraph( rptStyleManager::GetHeadingStyle() );
   *pTitlePage << p;

   *p << _T("Configuration") << rptNewLine;
   p = CLibraryUsageParagraph().Build(m_pBroker, false);
   *pTitlePage << p;


   // girder seed data comparison
   CSegmentKey fabrSegmentKey(TOGA_SPAN,TOGA_FABR_GDR,0);
   p = CGirderSeedDataComparisonParagraph().Build(m_pBroker, fabrSegmentKey);
   if (p != nullptr)
   {
      // only report if we have data
      *pTitlePage << p;
   }

   rptRcTable* pTable;
   int row = 0;

   // Status Center Items
   GET_IFACE(IEAFStatusCenter,pStatusCenter);
   IndexType nItems = pStatusCenter->Count();

   if ( nItems != 0 )
   {
      pPara = new rptParagraph;
      pPara->SetStyleName(rptStyleManager::GetHeadingStyle());
      *pTitlePage << pPara;

      *pPara << _T("Status Items") << rptNewLine;

      pTable = rptStyleManager::CreateDefaultTable(2,_T(""));
      pTable->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
      pTable->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
      pTable->SetColumnStyle(1,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
      pTable->SetStripeRowColumnStyle(1,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

      *pPara << pTable << rptNewLine;

      (*pTable)(0,0) << _T("Level");
      (*pTable)(0,1) << _T("Description");

      // Don't allow duplicate strings in table
      std::set<std::_tstring> messages;

      row = 1;
      CString strSeverityType[] = { _T("Information"), _T("Warning"), _T("Error") };
      for ( IndexType i = 0; i < nItems; i++ )
      {
         CEAFStatusItem* pItem = pStatusCenter->GetByIndex(i);

         // Trim span/girder information. TOGA doesn't want this
         // Blasts anything left of the first ":"
         std::_tstring msg = pItem->GetDescription();
         std::size_t loc = msg.find(_T(':'));
         if (loc != std::_tstring::npos)
            msg.erase(0,loc+1);

         std::pair< std::set<std::_tstring>::iterator, bool > it = messages.insert(msg);
         if (it.second) // no dup's
         {
            eafTypes::StatusSeverityType severity = pStatusCenter->GetSeverity(pItem);

            // Set text and cell background
            rptRiStyle::FontColor colors[] = {rptRiStyle::LightGreen, rptRiStyle::Yellow, rptRiStyle::Red };
            rptRiStyle::FontColor color = colors[severity];
            (*pTable)(row, 0) << new rptRcBgColor(color);
            (*pTable)(row, 0).SetFillBackGroundColor(color);

            (*pTable)(row,0) << strSeverityType[severity];
            (*pTable)(row++,1) << msg;
         }
      }
   }

   return pTitlePage;
}
