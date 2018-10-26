///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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
#include <IFace\GirderHandlingPointOfInterest.h>
#include <IFace\GirderHandlingSpecCriteria.h>
#include <IFace\Project.h>

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
                              IBroker* pBroker,SpanIndexType span,GirderIndexType girder,
                              IEAFDisplayUnits* pDisplayUnits) const
{

   // First check if we even need to do anything
   GET_IFACE2(pBroker,IGirderHaulingSpecCriteria,pGirderHaulingSpecCriteria);
   if (!pGirderHaulingSpecCriteria->IsHaulingCheckEnabled())
   {
      rptParagraph* pTitle = new rptParagraph( pgsReportStyleHolder::GetHeadingStyle() );
      *pChapter << pTitle;
      *pTitle << _T("Check for Hauling to Bridge Site")<<rptNewLine;

      rptParagraph* p = new rptParagraph;
      *pChapter << p;

      *p <<color(Red)<<_T("Hauling analysis disabled in Project Criteria library entry. No analysis performed.")<<color(Black)<<rptNewLine;
      return;
   }
   else
   {
      // Get hauling results
      GET_IFACE2(pBroker,IArtifact,pArtifacts);
      const pgsGirderArtifact* pArtifact = pArtifacts->GetArtifact(span,girder);
      const pgsHaulingAnalysisArtifact* pHaulArtifact = pArtifact->GetHaulingAnalysisArtifact();

      pHaulArtifact->BuildHaulingCheckReport(span, girder, pChapter, pBroker, pDisplayUnits);
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
