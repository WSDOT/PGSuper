///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2018  Washington State Department of Transportation
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

rptChapter* CHaulingCheckDetailsChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CGirderReportSpecification* pGirderRptSpec = dynamic_cast<CGirderReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pGirderRptSpec->GetBroker(&pBroker);
   const CGirderKey& girderKey(pGirderRptSpec->GetGirderKey());

   GET_IFACE2(pBroker,IBridge,pBridge);
   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);


   GET_IFACE2(pBroker,ISegmentHaulingSpecCriteria,pSegmentHaulingSpecCriteria);
   if (!pSegmentHaulingSpecCriteria->IsHaulingAnalysisEnabled())
   {
      rptParagraph* pTitle = new rptParagraph( rptStyleManager::GetHeadingStyle() );
      *pChapter << pTitle;
      *pTitle << _T("Details for Check for Hauling to Bridge Site [5.5.4.3][5.9.4.1]")<<rptNewLine;

      rptParagraph* p = new rptParagraph;
      *pChapter << p;

      *p <<color(Red)<<_T("Hauling analysis disabled in Project Criteria. No analysis performed.")<<color(Black)<<rptNewLine;
   }
   else
   {
      GET_IFACE2(pBroker,IArtifact,pArtifacts);
      GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   
      GET_IFACE2(pBroker,IBridge,pBridge);
      GroupIndexType nGroups = pBridge->GetGirderGroupCount();
      GroupIndexType firstGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
      GroupIndexType lastGroupIdx  = (girderKey.groupIndex == ALL_GROUPS ? nGroups-1 : firstGroupIdx );
      for ( GroupIndexType grpIdx = firstGroupIdx; grpIdx <= lastGroupIdx; grpIdx++ )
      {
         GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
         GirderIndexType firstGdrIdx = (girderKey.girderIndex == ALL_GIRDERS ? 0 : girderKey.girderIndex);
         GirderIndexType lastGdrIdx  = (girderKey.girderIndex == ALL_GIRDERS ? nGirders-1 : firstGdrIdx);
         for ( GirderIndexType gdrIdx = firstGdrIdx; gdrIdx <= lastGdrIdx; gdrIdx++ )
         {
            for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
            {
               CSegmentKey segmentKey(girderKey,segIdx);
         
               if ( 1 < nSegments )
               {
                  rptParagraph* pTitle = new rptParagraph(rptStyleManager::GetHeadingStyle() );
                  *pChapter << pTitle;
                  *pTitle << _T("Segment ") << LABEL_SEGMENT(segmentKey.segmentIndex) << rptNewLine;

                  rptParagraph* p = new rptParagraph;
                  *pChapter << p;
               }

               // Artifact does heavy lifting
               const pgsHaulingAnalysisArtifact* pHaulArtifact = pArtifacts->GetHaulingAnalysisArtifact(segmentKey);
               pHaulArtifact->BuildHaulingDetailsReport(segmentKey, pChapter, pBroker, pDisplayUnits);
            } // next segment
         } // next girder
      } // next group
   }


   return pChapter;
}



CChapterBuilder* CHaulingCheckDetailsChapterBuilder::Clone() const
{
   return new CHaulingCheckDetailsChapterBuilder;
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
