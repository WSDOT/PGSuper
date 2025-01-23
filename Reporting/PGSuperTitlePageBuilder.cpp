///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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
bool DoPrintStatusItem(CEAFStatusItem* pItem, const CGirderKey& girderKey,SegmentIndexType nSegments)
{
   pgsSegmentRelatedStatusItem* pSegmentStatusItem = dynamic_cast<pgsSegmentRelatedStatusItem*>(pItem);
   if (pSegmentStatusItem != nullptr)
   {
      if ( nSegments == ALL_SEGMENTS )
      {
         return true;
      }
      else
      {
         for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
         {
            CSegmentKey segmentKey(girderKey,segIdx);
            return pSegmentStatusItem->IsRelatedTo(segmentKey);
         }
      }
   }
   else
   {
      return true;
   }

   return false;
}

bool DoPrintStatusCenter(IEAFStatusCenter* pStatusCenter, IndexType nItems, const CGirderKey& girderKey,SegmentIndexType nSegments)
{
   for ( IndexType i = 0; i < nItems; i++ )
   {
      CEAFStatusItem* pItem = pStatusCenter->GetByIndex(i);

      if (DoPrintStatusItem(pItem, girderKey, nSegments))
      {
         return true;
      }
   }

   return false;
}

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


CPGSuperTitlePageBuilder::CPGSuperTitlePageBuilder(IBroker* pBroker,LPCTSTR strTitle,bool bFullVersion, bool bPageBreakAfter) :
WBFL::Reporting::TitlePageBuilder(strTitle),
m_pBroker(pBroker),
m_bFullVersion(bFullVersion),
m_bPageBreakAfter(bPageBreakAfter)
{
}

CPGSuperTitlePageBuilder::CPGSuperTitlePageBuilder(const CPGSuperTitlePageBuilder& other) :
WBFL::Reporting::TitlePageBuilder(other),
m_pBroker(other.m_pBroker),
m_bFullVersion(other.m_bFullVersion),
m_bPageBreakAfter(other.m_bPageBreakAfter)
{
}

CPGSuperTitlePageBuilder::~CPGSuperTitlePageBuilder(void)
{
}

std::unique_ptr<WBFL::Reporting::TitlePageBuilder> CPGSuperTitlePageBuilder::Clone() const
{
   return std::make_unique<CPGSuperTitlePageBuilder>(*this);
}

bool CPGSuperTitlePageBuilder::NeedsUpdate(const std::shared_ptr<const WBFL::Reporting::ReportHint>& pHint,const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec) const
{
   // don't let the title page control whether or not a report needs updating
   return false;
}

rptChapter* CPGSuperTitlePageBuilder::Build(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec) const
{
   // Create a title page for the report
   rptChapter* pTitlePage = new rptChapter;

   rptParagraph* pPara = new rptParagraph;
   pPara->SetStyleName(rptStyleManager::GetReportTitleStyle());
   *pTitlePage << pPara;

   std::_tstring title = GetReportTitle();

   *pPara << title.c_str();

   pPara = new rptParagraph;
   pPara->SetStyleName(rptStyleManager::GetReportSubtitleStyle());
   *pTitlePage << pPara;

   // write location
   std::_tstring strloc = pRptSpec->GetReportContextString();
   if (!strloc.empty())
   {
      *pPara << _T("For ") << strloc << rptNewLine;
   }
   else
   {
      *pPara << rptNewLine;
   }

   // Determine if the report spec has span/girder information
   auto pSpanRptSpec       = std::dynamic_pointer_cast<const CSpanReportSpecification>(pRptSpec);
   auto pGirderRptSpec     = std::dynamic_pointer_cast<const CGirderReportSpecification>(pRptSpec);
   auto pGirderLineRptSpec = std::dynamic_pointer_cast<const CGirderLineReportSpecification>(pRptSpec);

   CGirderKey girderKey;

   bool bGirderReport = true;
   if ( pGirderRptSpec != nullptr )
   {
      girderKey = pGirderRptSpec->GetGirderKey();
   }
   else if ( pSpanRptSpec != nullptr )
   {
   }
   else if ( pGirderLineRptSpec != nullptr )
   {
      bGirderReport = false;
      GirderIndexType gdrIdx = pGirderLineRptSpec->GetGirderIndex();
      girderKey.girderIndex = gdrIdx;
      ATLASSERT(gdrIdx != INVALID_INDEX);
   }
   else
   {
      bGirderReport = false;
   }

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
   *pPara << _T("PGSuper") << Super(symbol(TRADEMARK)) << _T(" (x64)") << rptNewLine;
#else
   *pPara << _T("PGSuper") << Super(symbol(TRADEMARK)) << _T(" (x86)") << rptNewLine;
#endif

   pPara = new rptParagraph(rptStyleManager::GetCopyrightStyle());
   *pTitlePage << pPara;
   *pPara << _T("Copyright ") << symbol(COPYRIGHT) << _T(" ") << WBFL::System::Date().Year() << _T(", WSDOT, All Rights Reserved") << rptNewLine;

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

   // girder seed data comparison
   if ( pGirderRptSpec != nullptr || pSpanRptSpec != nullptr )
   {
      if ( girderKey.groupIndex != INVALID_INDEX )
      {
         p = CGirderSeedDataComparisonParagraph().Build(m_pBroker,girderKey);

         if (p != nullptr)
         {
            // only report if we have data
            *pTitlePage << p;
         }
      }
   }

   p = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pTitlePage << p;
   *p << _T("Analysis Controls") << rptNewLine;

   GET_IFACE(ISpecification, pSpec);
   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   p = new rptParagraph();
   *pTitlePage << p;
   *p << _T("Structural Analysis Method: ");
   switch (analysisType)
   {
   case pgsTypes::Simple:
      *p << _T("Simple Span");
      break;

   case pgsTypes::Continuous:
      *p << _T("Simple Spans made Continuous");
      break;

   case pgsTypes::Envelope:
      *p << _T("Envelope of Simple Span and Simple Spans made Continuous");
      break;
   }
   *p << rptNewLine;

   GET_IFACE(ISectionProperties, pSectProps);
   if (pSectProps->GetSectionPropertiesMode() == pgsTypes::spmGross)
   {
      *p << _T("Section Properties: Gross") << rptNewLine;
   }
   else
   {
      *p << _T("Section Properties: Transformed") << rptNewLine;
   }

   GET_IFACE(ILossParameters, pLossParams);
   *p << _T("Losses: ") << pLossParams->GetLossMethodDescription() << rptNewLine;


   GET_IFACE_NOCHECK(IBridge, pBridge);
   if (girderKey.girderIndex != INVALID_INDEX && IsDifferentNumberOfGirdersPerSpan(pBridge) )
   {
      p = new rptParagraph(rptStyleManager::GetHeadingStyle());
      *pTitlePage << p;
      *p << _T("Beams Used in Girder Line Analysis Models") << rptNewLine;

      p = new rptParagraph();
      *pTitlePage << p;
      *p << Bold(_T("This bridge is described with a different number of girders in each span.")) << rptNewLine;
      *p << _T("Plane frame analysis is performed in accordance with LRFD 4.6.2. ") << rptNewLine;
      *p << _T("The structural analysis model for Girder Line ") << LABEL_GIRDER(girderKey.girderIndex) << _T(" consists of ");

      std::vector<CGirderKey> vGirderKeys;
      pBridge->GetGirderline(girderKey.girderIndex, &vGirderKeys);
      size_t cnt = vGirderKeys.size();
      size_t ic = 0;
      for (const auto& thisGirderKey : vGirderKeys)
      {
         *p << _T("Span ") << LABEL_SPAN(thisGirderKey.groupIndex) << _T(" Girder ") << LABEL_GIRDER(thisGirderKey.girderIndex); 

         if (++ic < cnt)
         {
            *p << _T(", ");
         }
      }

      *p << _T(".") << rptNewLine;
      *p << _T("The pier cap is assumed to be torsionally rigid for transfer of continuous moment.") << rptNewLine;
      *p << _T("See the Structural Analysis Models and Reactions topics in the Technical Guide for more information.") << rptNewLine;
   }


   rptRcTable* pTable;
   int row = 0;

   if (m_bFullVersion)
   {
      rptParagraph* pPara = new rptParagraph;
      pPara->SetStyleName(rptStyleManager::GetHeadingStyle());
      *pTitlePage << pPara;

      pTable = rptStyleManager::CreateDefaultTable(2, _T("Notes"));
      pTable->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
      pTable->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
      pTable->SetColumnStyle(1,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
      pTable->SetStripeRowColumnStyle(1,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

      *pPara << pTable << rptNewLine;

      row = 0;
      (*pTable)(row,0) << _T("Symbol");
      (*pTable)(row++,1) << _T("Definition");

      (*pTable)(row,0) << Sub2(_T("L"),_T("r"));
      (*pTable)(row++,1) << _T("Span Length of Girder at Release");

      (*pTable)(row,0) << Sub2(_T("L"),_T("l"));
      (*pTable)(row++,1) << _T("Span Length of Girder during Lifting");

      (*pTable)(row,0) << Sub2(_T("L"),_T("st"));
      (*pTable)(row++,1) << _T("Span Length of Girder during Storage");

      (*pTable)(row,0) << Sub2(_T("L"),_T("h"));
      (*pTable)(row++,1) << _T("Span Length of Girder during Hauling");

      (*pTable)(row,0) << Sub2(_T("L"),_T("e"));
      (*pTable)(row++,1) << _T("Span Length of Girder after Erection");

      (*pTable)(row,0) << Sub2(_T("L"),_T("s"));
      (*pTable)(row++,1) << _T("Length of Span");

      (*pTable)(row,0) << _T("Debond");
      (*pTable)(row++,1) << _T("Point where bond begins for a debonded strand");

      (*pTable)(row, 0) << _T("PSXFR");
      (*pTable)(row++, 1) << _T("Point of prestress transfer");

      (*pTable)(row, 0) << _T("FoS");
      (*pTable)(row++, 1) << _T("Face of Support in final bridge configuration");

      (*pTable)(row, 0) << _T("ST");
      (*pTable)(row++, 1) << _T("Section Transitions");

      (*pTable)(row, 0) << _T("STLF");
      (*pTable)(row++, 1) << _T("Section Transitions, Left Face");

      (*pTable)(row, 0) << _T("STRF");
      (*pTable)(row++, 1) << _T("Section Transitions, Right Face");

      (*pTable)(row, 0) << _T("SDCR");
      (*pTable)(row++, 1) << _T("Start of Deck Casting Region");

      (*pTable)(row, 0) << _T("EDCR");
      (*pTable)(row++, 1) << _T("End of Deck Casting Region");

      (*pTable)(row,0) << _T("Diaphragm");
      (*pTable)(row++,1) << _T("Location of a precast or cast in place diaphragm");

      (*pTable)(row,0) << _T("Bar Cutoff");
      (*pTable)(row++,1) << _T("End of a reinforcing bar in the girder");

      (*pTable)(row,0) << _T("Deck Bar Cutoff");
      (*pTable)(row++,1) << _T("End of a reinforcing bar in the deck");

      if ( WBFL::LRFD::BDSManager::Edition::ThirdEdition2004 <= WBFL::LRFD::BDSManager::GetEdition() )
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
      GET_IFACE(IBridge,pBridge);
      
      GET_IFACE(IEAFStatusCenter,pStatusCenter);
      IndexType nItems = pStatusCenter->Count();

      GroupIndexType firstGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
      GroupIndexType lastGroupIdx  = (girderKey.groupIndex == ALL_GROUPS ? pBridge->GetGirderGroupCount()-1 : firstGroupIdx);
      for ( GroupIndexType grpIdx = firstGroupIdx; grpIdx <= lastGroupIdx; grpIdx++ )
      {
         CGirderKey thisGirderKey(grpIdx,girderKey.girderIndex);
         SegmentIndexType nSegments = (thisGirderKey.girderIndex == ALL_GIRDERS ? ALL_SEGMENTS : pBridge->GetSegmentCount(thisGirderKey));
         if ( DoPrintStatusCenter(pStatusCenter, nItems, thisGirderKey, nSegments) )
         {
            pPara = new rptParagraph;
            pPara->SetStyleName(rptStyleManager::GetHeadingStyle());
            *pTitlePage << pPara;

            pTable = rptStyleManager::CreateDefaultTable(2, _T("Status Items"));
            pTable->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
            pTable->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
            pTable->SetColumnStyle(1,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
            pTable->SetStripeRowColumnStyle(1,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

            *pPara << pTable << rptNewLine;

            (*pTable)(0,0) << _T("Level");
            (*pTable)(0,1) << _T("Description");

            row = 1;
            CString strSeverityType[] = { _T("Information"), _T("Warning"), _T("Error") };
            for ( IndexType i = 0; i < nItems; i++ )
            {
               CEAFStatusItem* pItem = pStatusCenter->GetByIndex(i);
               
               if ( DoPrintStatusItem(pItem, thisGirderKey, nSegments) )
               {
                  eafTypes::StatusSeverityType severity = pStatusCenter->GetSeverity(pItem);

                  // Set text and cell background
                  rptRiStyle::FontColor colors[] = {rptRiStyle::LightGreen, rptRiStyle::Yellow, rptRiStyle::Red };
                  rptRiStyle::FontColor color = colors[severity];
                  (*pTable)(row, 0) << new rptRcBgColor(color);
                  (*pTable)(row, 0).SetFillBackGroundColor(color);

                  (*pTable)(row,0) << strSeverityType[severity];
                  (*pTable)(row++,1) << pItem->GetDescription();
               }
            }
         } // next segment
      } // next group
   } // end if

   // Throw in a page break

   if (m_bPageBreakAfter)
   {
      p = new rptParagraph;
      *pTitlePage << p;
      *p << rptNewPage;
   }

   return pTitlePage;
}
