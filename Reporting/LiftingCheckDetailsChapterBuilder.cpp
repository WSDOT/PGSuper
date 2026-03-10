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

#include "StdAfx.h"
#include <IFace\Artifact.h>

#include <Reporting\LiftingCheckDetailsChapterBuilder.h>
#include <Reporting\ReportNotes.h>

#include <PgsExt\ReportPointOfInterest.h>
#include <PgsExt\GirderArtifact.h>
#include <PgsExt\CapacityToDemand.h>

#include <IFace/Tools.h>
#include <EAF/EAFDisplayUnits.h>
#include <IFace\PointOfInterest.h>
#include <IFace\GirderHandlingSpecCriteria.h>
#include <IFace\Bridge.h>

#include <Stability\Stability.h>

#include <limits>

#include <EAF/EAFApp.h>

CLiftingCheckDetailsChapterBuilder::CLiftingCheckDetailsChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

LPCTSTR CLiftingCheckDetailsChapterBuilder::GetName() const
{
   return TEXT("Lifting Check Details");
}

rptChapter* CLiftingCheckDetailsChapterBuilder::Build(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,Uint16 level) const
{
   auto pBrokerRptSpec = std::dynamic_pointer_cast<const CBrokerReportSpecification>(pRptSpec);
   auto pBroker = pBrokerRptSpec->GetBroker();

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);
   
   GET_IFACE2(pBroker,ISegmentLiftingSpecCriteria,pSegmentLiftingSpecCriteria);
   if (pSegmentLiftingSpecCriteria->IsLiftingAnalysisEnabled())
   {
      GET_IFACE2(pBroker, IArtifact, pArtifacts);
      GET_IFACE2(pBroker, IGirder, pGirder);
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

            if (1 < (lastSegIdx-firstSegIdx))
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

            auto pArtifact = pArtifacts->GetLiftingCheckArtifact(segmentKey);
            auto pStabilityModel = pGirder->GetSegmentLiftingStabilityModel(segmentKey);
            auto pStabilityProblem = pGirder->GetSegmentLiftingStabilityProblem(segmentKey);
            auto results = pArtifact->GetLiftingResults();

            Float64 Ll, Lr;
            pStabilityProblem->GetSupportLocations(&Ll, &Lr);
            WBFL::Stability::LiftingStabilityReporter reporter;
            auto* pApp = EAFGetApp();
            reporter.BuildDetailsChapter(pStabilityModel, pStabilityProblem, &results, pChapter, pApp->GetDisplayUnits(), _T("Location from<BR/>Left Pick Point"), Ll);
         } // next segment
      } // next group
   }
   else
   {
      rptParagraph* pTitle = new rptParagraph(rptStyleManager::GetHeadingStyle());
      *pChapter << pTitle;
      *pTitle << _T("Details for Check for Lifting In Casting Yard");
      if (WBFL::LRFD::BDSManager::Edition::NinthEdition2020 <= WBFL::LRFD::BDSManager::GetEdition())
      {
         *pTitle << _T(" [5.5.4.3]");
      }
      *pTitle << rptNewLine;

      rptParagraph* p = new rptParagraph;
      *pChapter << p;

      *p << color(Red) << _T("Lifting analysis disabled in Project Criteria. No analysis performed.") << color(Black) << rptNewLine;
   }

   return pChapter;
}
