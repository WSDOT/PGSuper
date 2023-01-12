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
#include <Reporting\HaulingCheck.h>
#include <Reporting\ReportNotes.h>

#include <IFace\Artifact.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\GirderHandlingSpecCriteria.h>
#include <IFace\Project.h>
#include <IFace\Bridge.h>

#include <PgsExt\PointOfInterest.h>
#include <PgsExt\HaulingAnalysisArtifact.h>
#include <PgsExt\GirderArtifact.h>
#include <PgsExt\CapacityToDemand.h>

#include <PsgLib\SpecLibraryEntry.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CHaulingCheck
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CHaulingCheck::CHaulingCheck()
{
}

CHaulingCheck::CHaulingCheck(const CHaulingCheck& rOther)
{
   MakeCopy(rOther);
}

CHaulingCheck::~CHaulingCheck()
{
}

//======================== OPERATORS  =======================================
CHaulingCheck& CHaulingCheck::operator= (const CHaulingCheck& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
void CHaulingCheck::Build(rptChapter* pChapter,
                              IBroker* pBroker,const CGirderKey& girderKey,
                              IEAFDisplayUnits* pDisplayUnits) const
{
   GET_IFACE2(pBroker,ISegmentHaulingSpecCriteria,pSegmentHaulingSpecCriteria);
   if (pSegmentHaulingSpecCriteria->IsHaulingAnalysisEnabled())
   {
      GET_IFACE2(pBroker,IBridge,pBridge);
      SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         CSegmentKey segmentKey(girderKey,segIdx);
         Build(pChapter, pBroker, segmentKey, pDisplayUnits);
      }
   }
   else
   {
      rptParagraph* pTitle = new rptParagraph( rptStyleManager::GetHeadingStyle() );
      *pChapter << pTitle;
      *pTitle << _T("Check for Hauling to Bridge Site")<<rptNewLine;

      rptParagraph* p = new rptParagraph;
      *pChapter << p;

      *p <<color(Red)<<_T("Hauling analysis disabled in Project Criteria. No analysis performed.")<<color(Black)<<rptNewLine;
      if (lrfdVersionMgr::NinthEdition2020 <= lrfdVersionMgr::GetVersion())
      {
         *p << color(Red) << _T("Per LRFD 5.5.4.3, \"Buckling and stability of precast members during handling, transportation, and erection shall be investigated.\" Also see C5.5.4.3 and C5.12.3.2.1.") << color(Black) << rptNewLine;
      }
   }
}


void CHaulingCheck::Build(rptChapter* pChapter,
   IBroker* pBroker, const CSegmentKey& segmentKey,
   IEAFDisplayUnits* pDisplayUnits) const
{
   GET_IFACE2(pBroker, ISegmentHaulingSpecCriteria, pSegmentHaulingSpecCriteria);
   if (pSegmentHaulingSpecCriteria->IsHaulingAnalysisEnabled())
   {
      GET_IFACE2(pBroker, IBridge, pBridge);
      GET_IFACE2(pBroker, IArtifact, pArtifacts);
      SegmentIndexType nSegments = pBridge->GetSegmentCount(segmentKey);
      if (1 < nSegments)
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

      const pgsHaulingAnalysisArtifact* pHaulArtifact = pArtifacts->GetHaulingAnalysisArtifact(segmentKey);
      pHaulArtifact->BuildHaulingCheckReport(segmentKey, pChapter, pBroker, pDisplayUnits);
   }
   else
   {
      rptParagraph* pTitle = new rptParagraph(rptStyleManager::GetHeadingStyle());
      *pChapter << pTitle;
      *pTitle << _T("Check for Hauling to Bridge Site") << rptNewLine;

      rptParagraph* p = new rptParagraph;
      *pChapter << p;

      *p << color(Red) << _T("Hauling analysis disabled in Project Criteria. No analysis performed.") << color(Black) << rptNewLine;
      if (lrfdVersionMgr::NinthEdition2020 <= lrfdVersionMgr::GetVersion())
      {
         *p << color(Red) << _T("Per LRFD 5.5.4.3, \"Buckling and stability of precast members during handling, transportation, and erection shall be investigated.\" Also see C5.5.4.3 and C5.12.3.2.1.") << color(Black) << rptNewLine;
      }
   }
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void CHaulingCheck::MakeCopy(const CHaulingCheck& rOther)
{
   // Add copy code here...
}

void CHaulingCheck::MakeAssignment(const CHaulingCheck& rOther)
{
   MakeCopy( rOther );
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PRIVATE    ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================


//======================== ACCESS     =======================================
//======================== INQUERY    =======================================

//======================== DEBUG      =======================================
#if defined _DEBUG
bool CHaulingCheck::AssertValid() const
{
   return true;
}

void CHaulingCheck::Dump(dbgDumpContext& os) const
{
   os << _T("Dump for CHaulingCheck") << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool CHaulingCheck::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("CHaulingCheck");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for CHaulingCheck");

   TESTME_EPILOG("CHaulingCheck");
}
#endif // _UNITTEST
