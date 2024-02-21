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
#include <Reporting\HaulingCheckDetailsChapterBuilder.h>
#include <Reporting\ReportNotes.h>

#include <PgsExt\PointOfInterest.h>
#include <PgsExt\GirderArtifact.h>
#include <PgsExt\CapacityToDemand.h>

#include <EAF\EAFDisplayUnits.h>
#include <IFace\GirderHandlingSpecCriteria.h>
#include <IFace\Bridge.h>
#include <IFace\Artifact.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CHaulingCheckDetailsChapterBuilder
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CHaulingCheckDetailsChapterBuilder::CHaulingCheckDetailsChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CHaulingCheckDetailsChapterBuilder::GetName() const
{
   return TEXT("Hauling Check Details");
}

rptChapter* CHaulingCheckDetailsChapterBuilder::Build(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,Uint16 level) const
{
   CComPtr<IBroker> pBroker;
   auto pBrokerRptSpec = std::dynamic_pointer_cast<const CBrokerReportSpecification>(pRptSpec);
   pBrokerRptSpec->GetBroker(&pBroker);

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);


   GET_IFACE2(pBroker,ISegmentHaulingSpecCriteria,pSegmentHaulingSpecCriteria);
   if (pSegmentHaulingSpecCriteria->IsHaulingAnalysisEnabled())
   {
      GET_IFACE2(pBroker, IArtifact, pArtifacts);
      GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);
      GET_IFACE2_NOCHECK(pBroker, IBridge, pBridge);

      std::vector<CGirderKey> vGirderKeys;

      auto pGirderReportSpec = std::dynamic_pointer_cast<const CGirderReportSpecification>(pRptSpec);
      auto pSegmentReportSpec = std::dynamic_pointer_cast<const CSegmentReportSpecification>(pRptSpec);

      if (pGirderReportSpec)
      {
         CGirderKey girderKey = pGirderReportSpec->GetGirderKey();
         pBridge->GetGirderline(girderKey, &vGirderKeys);
      }
      else
      {
         vGirderKeys.emplace_back(pSegmentReportSpec->GetSegmentKey());
      }

      for (const auto& thisGirderKey : vGirderKeys)
      {
         SegmentIndexType firstSegIdx, lastSegIdx;
         if (pGirderReportSpec)
         {
            firstSegIdx = 0;
            lastSegIdx = pBridge->GetSegmentCount(thisGirderKey) - 1;
         }
         else
         {
            firstSegIdx = pSegmentReportSpec->GetSegmentIndex();
            lastSegIdx = firstSegIdx;
         }

         for (SegmentIndexType segIdx = firstSegIdx; segIdx <= lastSegIdx; segIdx++)
         {
            CSegmentKey segmentKey(thisGirderKey, segIdx);

            if (1 < (lastSegIdx - firstSegIdx))
            {
               std::_tstringstream os;
               os << _T("Segment ") << LABEL_SEGMENT(segmentKey.segmentIndex) << std::endl;
               rptParagraph* pTitle = new rptParagraph(rptStyleManager::GetHeadingStyle());
               *pChapter << pTitle;
               pTitle->SetName(os.str().c_str());
               *pTitle << pTitle->GetName() << rptNewLine;

               rptParagraph* p = new rptParagraph;
               *pChapter << p;
            }

            // Artifact does heavy lifting
            const pgsHaulingAnalysisArtifact* pHaulArtifact = pArtifacts->GetHaulingAnalysisArtifact(segmentKey);
            pHaulArtifact->BuildHaulingDetailsReport(segmentKey, pChapter, pBroker, pDisplayUnits);
         } // next segment
      } // next group
   }
   else
   {
      rptParagraph* pTitle = new rptParagraph(rptStyleManager::GetHeadingStyle());
      *pChapter << pTitle;
      *pTitle << _T("Details for Check for Hauling to Bridge Site");
      if (WBFL::LRFD::BDSManager::Edition::NinthEdition2020 <= WBFL::LRFD::BDSManager::GetEdition())
      {
         *pTitle << _T(" [5.5.4.3]");
      }
      *pTitle << rptNewLine;

      rptParagraph* p = new rptParagraph;
      *pChapter << p;

      *p << color(Red) << _T("Hauling analysis disabled in Project Criteria. No analysis performed.") << color(Black) << rptNewLine;
   }


   return pChapter;
}



std::unique_ptr<WBFL::Reporting::ChapterBuilder> CHaulingCheckDetailsChapterBuilder::Clone() const
{
   return std::make_unique<CHaulingCheckDetailsChapterBuilder>();
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
