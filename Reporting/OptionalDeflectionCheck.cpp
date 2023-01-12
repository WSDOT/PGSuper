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
#include <Reporting\OptionalDeflectionCheck.h>
#include <Reporting\StirrupDetailingCheckTable.h>

#include <IFace\Artifact.h>
#include <IFace\Bridge.h>


#include <PgsExt\GirderArtifact.h>
#include <PgsExt\PrecastIGirderDetailingArtifact.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   COptionalDeflectionCheck
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
COptionalDeflectionCheck::COptionalDeflectionCheck()
{
}

COptionalDeflectionCheck::COptionalDeflectionCheck(const COptionalDeflectionCheck& rOther)
{
   MakeCopy(rOther);
}

COptionalDeflectionCheck::~COptionalDeflectionCheck()
{
}

//======================== OPERATORS  =======================================
COptionalDeflectionCheck& COptionalDeflectionCheck::operator= (const COptionalDeflectionCheck& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
void COptionalDeflectionCheck::Build(rptChapter* pChapter, IBroker* pBroker,const pgsGirderArtifact* pGirderArtifact,
                              IEAFDisplayUnits* pDisplayUnits) const
{
   const CGirderKey& girderKey(pGirderArtifact->GetGirderKey());

   INIT_UV_PROTOTYPE( rptLengthUnitValue, defu,    pDisplayUnits->GetComponentDimUnit(), true );

   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   pPara->SetName(_T("Live Load Deflection Check"));
   *pChapter << pPara;
   (*pPara) << _T("Live Load Deflection Check [2.5.2.6.2]") << rptNewLine;

   rptParagraph* p = new rptParagraph;
   *pChapter<<p;

   GET_IFACE2(pBroker,IBridge,pBridge);
   SpanIndexType grpStartSpanIdx, grpEndSpanIdx;
   pBridge->GetGirderGroupSpans(girderKey.groupIndex,&grpStartSpanIdx,&grpEndSpanIdx);
   for ( SpanIndexType spanIdx = grpStartSpanIdx; spanIdx <= grpEndSpanIdx; spanIdx++ )
   {
      IndexType idx = spanIdx - grpStartSpanIdx;
      const pgsDeflectionCheckArtifact* pArtifact = pGirderArtifact->GetDeflectionCheckArtifact(idx);

      if ( grpStartSpanIdx != grpEndSpanIdx )
      {
         p = new rptParagraph;
         *pChapter << p;
         *p << _T("Span ") << LABEL_SPAN(spanIdx) << rptNewLine;
      }
   
      
      if (pArtifact->IsApplicable())
      {
         Float64 min, max;
         pArtifact->GetDemand(&min,&max);

         *p<< _T("Allowable deflection span ratio = L/")<<pArtifact->GetAllowableSpanRatio()<<rptNewLine;
         *p<< _T("Allowable maximum deflection  = ")<< symbol(PLUS_MINUS) << defu.SetValue(pArtifact->GetCapacity())<<rptNewLine;
         *p<< _T("Minimum live load deflection along girder  = ")<<defu.SetValue(min)<<rptNewLine;
         *p<< _T("Maximum live load deflection along girder  = ")<<defu.SetValue(max)<<rptNewLine;
         *p<< _T("Status = ");
         if (pArtifact->Passed())
            *p<< RPT_PASS<<rptNewLine;
         else
            *p<<RPT_FAIL<<rptNewLine;
      }
   } // next span
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void COptionalDeflectionCheck::MakeCopy(const COptionalDeflectionCheck& rOther)
{
   // Add copy code here...
}

void COptionalDeflectionCheck::MakeAssignment(const COptionalDeflectionCheck& rOther)
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
bool COptionalDeflectionCheck::AssertValid() const
{
   return true;
}

void COptionalDeflectionCheck::Dump(WBFL::Debug::LogContext& os) const
{
   os << _T("Dump for COptionalDeflectionCheck") << WBFL::Debug::endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool COptionalDeflectionCheck::TestMe(WBFL::Debug::Log& rlog)
{
   TESTME_PROLOGUE("COptionalDeflectionCheck");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for COptionalDeflectionCheck");

   TESTME_EPILOG("LiveLoadDistributionFactorTable");
}
#endif // _UNITTEST
