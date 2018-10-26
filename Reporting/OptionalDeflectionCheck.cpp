///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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
#include <EAF\EAFDisplayUnits.h>

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
void COptionalDeflectionCheck::Build(rptChapter* pChapter, const pgsGirderArtifact* pArtifact,
                              SpanIndexType span,GirderIndexType girder, IEAFDisplayUnits* pDisplayUnits) const
{
   const pgsDeflectionCheckArtifact* pDef = pArtifact->GetDeflectionCheckArtifact();

   if (pDef->IsApplicable())
   {
      INIT_UV_PROTOTYPE( rptLengthUnitValue, defu,    pDisplayUnits->GetComponentDimUnit(), true );

      rptParagraph* pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      pPara->SetName(_T("Optional Live Load Deflection Check"));
      *pChapter << pPara;
      (*pPara) << _T("Optional Live Load Deflection Check (LRFD 2.5.2.6.2)") << rptNewLine;

      rptParagraph* p = new rptParagraph;
      *pChapter<<p;

      Float64 min, max;
      pDef->GetDemand(&min,&max);

      *p<< _T("Allowable deflection span ratio = L/")<<pDef->GetAllowableSpanRatio()<<rptNewLine;
      *p<< _T("Allowable maximum deflection  = ")<< symbol(PLUS_MINUS) << defu.SetValue(pDef->GetCapacity())<<rptNewLine;
      *p<< _T("Minimum live load deflection along girder  = ")<<defu.SetValue(min)<<rptNewLine;
      *p<< _T("Maximum live load deflection along girder  = ")<<defu.SetValue(max)<<rptNewLine;
      *p<< _T("Status = ");
      if (pDef->Passed())
         *p<< RPT_PASS<<rptNewLine;
      else
         *p<<RPT_FAIL<<rptNewLine;
   }
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

void COptionalDeflectionCheck::Dump(dbgDumpContext& os) const
{
   os << _T("Dump for COptionalDeflectionCheck") << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool COptionalDeflectionCheck::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("COptionalDeflectionCheck");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for COptionalDeflectionCheck");

   TESTME_EPILOG("LiveLoadDistributionFactorTable");
}
#endif // _UNITTEST
