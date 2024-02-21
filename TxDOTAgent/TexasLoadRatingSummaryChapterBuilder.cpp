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

#include "StdAfx.h"
#include "TexasLoadRatingSummaryChapterBuilder.h"

#include <Reporting\LoadRatingReportSpecificationBuilder.h>
#include <Reporting\RatingSummaryTable.h>

#include <IFace\Artifact.h>
#include <IFace\Intervals.h>

#include <IFace\RatingSpecification.h>
#include <IFace\Bridge.h>

#include <PgsExt\RatingArtifact.h>
#include <PgsExt\CapacityToDemand.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/****************************************************************************
CLASS
   CTexasLoadRatingSummaryChapterBuilder
****************************************************************************/

CTexasLoadRatingSummaryChapterBuilder::CTexasLoadRatingSummaryChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

LPCTSTR CTexasLoadRatingSummaryChapterBuilder::GetName() const
{
   return TEXT("Load Rating Summary");
}

rptChapter* CTexasLoadRatingSummaryChapterBuilder::Build(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,Uint16 level) const
{
   auto pGirderRptSpec = std::dynamic_pointer_cast<const CGirderReportSpecification>(pRptSpec);
   if (pGirderRptSpec == nullptr)
   {
      ATLASSERT(0);
      return nullptr;
   }
   else
   {
      rptChapter* pChapter = CPGSuperChapterBuilder::Build(pGirderRptSpec, level);

      CComPtr<IBroker> pBroker;
      pGirderRptSpec->GetBroker(&pBroker);
      const CGirderKey& girderKey(pGirderRptSpec->GetGirderKey());
      std::vector<CGirderKey> girderKeys{ girderKey };

      GET_IFACE2(pBroker, IRatingSpecification, pRatingSpec);

      if (pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) || pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating))
      {
         rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec, level);

         rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
         (*pChapter) << pPara;
         pPara->SetName(_T("Design Load Rating"));
         (*pPara) << pPara->GetName() << rptNewLine;
         pPara = new rptParagraph;
         (*pChapter) << pPara;

         (*pPara) << CRatingSummaryTable::BuildByLimitState(pBroker, girderKeys, CRatingSummaryTable::Design) << rptNewLine;
         return pChapter;
      }
      else
      {
         return nullptr;
      }
   }

}

std::unique_ptr<WBFL::Reporting::ChapterBuilder> CTexasLoadRatingSummaryChapterBuilder::Clone() const
{
   return std::make_unique<CTexasLoadRatingSummaryChapterBuilder>();
}
