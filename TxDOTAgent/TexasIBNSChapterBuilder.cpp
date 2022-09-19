///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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

#include <Reporting\SpanGirderReportSpecification.h>

#include "TexasIBNSChapterBuilder.h"
#include "TexasIBNSParagraphBuilder.h"

#include <EAF\EAFDisplayUnits.h>
#include <EAF\EAFAutoProgress.h>

#include <IFace\MomentCapacity.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Bridge.h>
#include <IFace\Artifact.h>
#include <IFace\Project.h>
#include <IFace\DistributionFactors.h>

#include <PgsExt\ReportPointOfInterest.h>
#include <PgsExt\GirderArtifact.h>

#include <psgLib\SpecLibraryEntry.h>
#include <psgLib\GirderLibraryEntry.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS	CTexasIBNSChapterBuilder
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CTexasIBNSChapterBuilder::CTexasIBNSChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
/*--------------------------------------------------------------------*/
LPCTSTR CTexasIBNSChapterBuilder::GetName() const
{
   return TEXT("Girder Schedule");
}

/*--------------------------------------------------------------------*/
rptChapter* CTexasIBNSChapterBuilder::Build(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,Uint16 level) const
{
   auto pMultiGirderRptSpec = std::dynamic_pointer_cast<const CMultiGirderReportSpecification>(pRptSpec);
   if (pMultiGirderRptSpec != nullptr)
   {
      CComPtr<IBroker> pBroker;
      pMultiGirderRptSpec->GetBroker(&pBroker);

      GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
      GET_IFACE2(pBroker,IArtifact,pIArtifact);

      rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);
      bool bUnitsSI = IS_SI_UNITS(pDisplayUnits);

      const std::vector<CGirderKey>& girderKeys( pMultiGirderRptSpec->GetGirderKeys() );

      // Give progress window a progress meter if needed
      bool bMultiGirderReport = (1 < girderKeys.size() ? true : false);

      GET_IFACE2(pBroker,IProgress,pProgress);
      DWORD mask = bMultiGirderReport ? PW_ALL|PW_NOCANCEL : PW_ALL|PW_NOGAUGE|PW_NOCANCEL;

      CEAFAutoProgress ap(pProgress,0,mask); 

      if (bMultiGirderReport)
      {
         pProgress->Init(0,(short)girderKeys.size(),1);  // and for multi-girders, a gauge.
      }

      // paragraph builder eventually wants segments, so build list
      bool bPassed=true;
      std::vector<CSegmentKey> segmentKeys;
      for (std::vector<CGirderKey>::const_iterator sit = girderKeys.begin(); sit!=girderKeys.end(); sit++)
      {
         const CGirderKey girderKey=*sit;

         // This is a single segment report
         CSegmentKey segmentKey(girderKey,0);
         segmentKeys.push_back(segmentKey);

         // Do global spec check while were at it
         const pgsSegmentArtifact* pSegmentArtifact = pIArtifact->GetSegmentArtifact(segmentKey);

         if( bPassed && !pSegmentArtifact->Passed() )
         {
            bPassed = false;
         }

         if (bMultiGirderReport)
         {
            pProgress->Increment();
         }
      }

      if( bPassed )
      {
         rptParagraph* pPara = new rptParagraph;
         *pChapter << pPara;
         *pPara << color(Green) << _T("The Specification Check was Successful") << color(Black) << rptNewLine;
      }
      else
      {
         rptParagraph* pPara = new rptParagraph;
         *pChapter << pPara;
         *pPara << color(Red) << _T("The Specification Check Was Not Successful") << color(Black);
      }

      // let the paragraph builder do all the work here...
      bool doEjectPage;
      CTexasIBNSParagraphBuilder parabuilder;
      rptParagraph* pcontent = parabuilder.Build(pBroker,segmentKeys,pDisplayUnits,level, doEjectPage);

      *pChapter << pcontent;

      return pChapter;
   }

   ATLASSERT(false);
   return nullptr;

}

/*--------------------------------------------------------------------*/
std::unique_ptr<WBFL::Reporting::ChapterBuilder>CTexasIBNSChapterBuilder::Clone() const
{
   return std::make_unique<CTexasIBNSChapterBuilder>();
}

