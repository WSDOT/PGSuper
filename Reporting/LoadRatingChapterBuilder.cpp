///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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
#include <Reporting\LoadRatingChapterBuilder.h>
#include <Reporting\LoadRatingReportSpecificationBuilder.h>
#include <Reporting\RatingSummaryTable.h>

#include <IFace\Artifact.h>
#include <IFace\Intervals.h>

#include <IFace\RatingSpecification.h>
#include <IFace\Bridge.h>

#include <Reporting\LongReinfShearCheck.h>

#include <PgsExt\RatingArtifact.h>
#include <PgsExt\CapacityToDemand.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/****************************************************************************
CLASS
   CLoadRatingChapterBuilder
****************************************************************************/

CLoadRatingChapterBuilder::CLoadRatingChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

LPCTSTR CLoadRatingChapterBuilder::GetName() const
{
   return TEXT("Load Rating");
}

rptChapter* CLoadRatingChapterBuilder::Build(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,Uint16 level) const
{
   auto pGirderRptSpec = std::dynamic_pointer_cast<const CBrokerReportSpecification>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pGirderRptSpec->GetBroker(&pBroker);

   auto pLrGirderRptSpec = std::dynamic_pointer_cast<const CLoadRatingReportSpecificationBase>(pRptSpec);
   if (!pLrGirderRptSpec)
   {
      ATLASSERT(0);
      return nullptr;
   }

   std::vector<CGirderKey> girderKeys = pLrGirderRptSpec->GetGirderKeys();

   GET_IFACE2(pBroker,IRatingSpecification,pRatingSpec);

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   rptParagraph* pPara;
   pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   (*pChapter) << pPara;

   (*pPara) << _T("Controlling Rating Factors for Selected Girders: ") << pGirderRptSpec->GetReportContextString() << rptNewLine;

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType loadRatingIntervalIdx = pIntervals->GetLoadRatingInterval();
   CString str;
   str.Format(_T("Load rating occurs in Interval %d: %s"),LABEL_INTERVAL(loadRatingIntervalIdx),pIntervals->GetDescription(loadRatingIntervalIdx).c_str());
   (*pPara) << str << rptNewLine;

   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) || pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating) )
   {
      pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
      (*pChapter) << pPara;
      pPara->SetName(_T("Design Load Rating"));
      (*pPara) << pPara->GetName() << rptNewLine;
      pPara = new rptParagraph;
      (*pChapter) << pPara;
      (*pPara) << CRatingSummaryTable::BuildByLimitState(pBroker,girderKeys,CRatingSummaryTable::Design ) << rptNewLine;
   }

   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) || pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) || pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Emergency))
   {
      pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
      (*pChapter) << pPara;
      pPara->SetName(_T("Legal Load Rating"));
      (*pPara) << pPara->GetName() << rptNewLine;
      pPara = new rptParagraph;
      (*pChapter) << pPara;

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
      {
         rptRcTable* pTable = CRatingSummaryTable::BuildByVehicle(pBroker,girderKeys, pgsTypes::lrLegal_Routine);
         if ( pTable )
         {
            (*pPara) << pTable << rptNewLine;
         }

         bool bMustCloseBridge;
         pTable = CRatingSummaryTable::BuildLoadPosting(pBroker,girderKeys, pgsTypes::lrLegal_Routine,&bMustCloseBridge);
         if ( pTable )
         {
            (*pPara) << pTable << rptNewLine;
            if (bMustCloseBridge)
            {
               *pPara << color(Red) << Bold(_T("Minimum gross live load weight is less than three tons - bridge must be closed. (MBE 6A.8.1, 6A.8.3, C6A.8.3)")) << color(Black) << rptNewLine;
            }
         }
      }

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
      {
         rptRcTable* pTable = CRatingSummaryTable::BuildByVehicle(pBroker,girderKeys, pgsTypes::lrLegal_Special);
         if ( pTable )
         {
            (*pPara) << pTable << rptNewLine;
         }

         bool bMustCloseBridge;
         pTable = CRatingSummaryTable::BuildLoadPosting(pBroker,girderKeys, pgsTypes::lrLegal_Special, &bMustCloseBridge);
         if ( pTable )
         {
            (*pPara) << pTable << rptNewLine;
            if (bMustCloseBridge)
            {
               *pPara << color(Red) << Bold(_T("Minimum gross live load weight is less than three tons - bridge must be closed. (MBE 6A.8.1, 6A.8.3, C6A.8.3)")) << color(Black) << rptNewLine;
            }
         }
      }

      if (pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Emergency))
      {
         rptRcTable* pTable = CRatingSummaryTable::BuildByVehicle(pBroker, girderKeys, pgsTypes::lrLegal_Emergency);
         if (pTable)
         {
            (*pPara) << pTable << rptNewLine;
         }

         pTable = CRatingSummaryTable::BuildEmergencyVehicleLoadPosting(pBroker, girderKeys);
         if (pTable)
         {
            (*pPara) << pTable << rptNewLine;
         }
      }
   }

   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) || pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
   {
      pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
      (*pChapter) << pPara;
      pPara->SetName(_T("Permit Load Rating"));
      (*pPara) << pPara->GetName() << rptNewLine;
      pPara = new rptParagraph;
      (*pChapter) << pPara;
      (*pPara) << Super(_T("*")) << _T("MBE 6A.4.5.2 Permit load rating should only be used if the bridge has a rating factor greater than 1.0 when evaluated for AASHTO legal loads.") << rptNewLine;

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
      {
         rptRcTable* pTable = CRatingSummaryTable::BuildByVehicle(pBroker,girderKeys, pgsTypes::lrPermit_Routine);
         if ( pTable )
         {
            (*pPara) << pTable << rptNewLine;
         }

         pTable = CRatingSummaryTable::BuildYieldStressRatio(pBroker, girderKeys, pgsTypes::lrPermit_Routine);
         if (pTable)
         {
            (*pPara) << pTable << rptNewLine;
         }
      }

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
      {
         rptRcTable* pTable = CRatingSummaryTable::BuildByVehicle(pBroker,girderKeys, pgsTypes::lrPermit_Special);
         if ( pTable )
         {
            (*pPara) << pTable << rptNewLine;
         }

         pTable = CRatingSummaryTable::BuildYieldStressRatio(pBroker, girderKeys, pgsTypes::lrPermit_Special);
         if (pTable)
         {
            (*pPara) << pTable << rptNewLine;
         }
      }
   }

   return pChapter;
}

std::unique_ptr<WBFL::Reporting::ChapterBuilder> CLoadRatingChapterBuilder::Clone() const
{
   return std::make_unique<CLoadRatingChapterBuilder>();
}
