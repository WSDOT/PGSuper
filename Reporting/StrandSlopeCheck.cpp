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
#include <Reporting\StrandSlopeCheck.h>

#include <IFace\Artifact.h>
#include <EAF\EAFDisplayUnits.h>

#include <PgsExt\GirderArtifact.h>
#include <PgsExt\StrandSlopeArtifact.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CStrandSlopeCheck
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CStrandSlopeCheck::CStrandSlopeCheck()
{
}

CStrandSlopeCheck::CStrandSlopeCheck(const CStrandSlopeCheck& rOther)
{
   MakeCopy(rOther);
}

CStrandSlopeCheck::~CStrandSlopeCheck()
{
}

//======================== OPERATORS  =======================================
CStrandSlopeCheck& CStrandSlopeCheck::operator= (const CStrandSlopeCheck& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
void CStrandSlopeCheck::Build(rptChapter* pChapter,
                              IBroker* pBroker,SpanIndexType span,GirderIndexType girder,
                              IEAFDisplayUnits* pDisplayUnits) const
{
   GET_IFACE2(pBroker,IArtifact,pIArtifact);
   const pgsGirderArtifact* pGdrArtifact = pIArtifact->GetArtifact(span,girder);
   const pgsStrandSlopeArtifact* pArtifact = pGdrArtifact->GetStrandSlopeArtifact();

   // no report if not applicable
   if ( pArtifact->IsApplicable() )
   {
      rptParagraph* pTitle = new rptParagraph( pgsReportStyleHolder::GetHeadingStyle() );
      *pChapter << pTitle;
      *pTitle << _T("Strand Slope");

   
      rptRcScalar slope;
      slope.SetFormat(pDisplayUnits->GetScalarFormat().Format);
      slope.SetWidth(pDisplayUnits->GetScalarFormat().Width);
      slope.SetPrecision(pDisplayUnits->GetScalarFormat().Precision);

      rptParagraph* pBody = new rptParagraph;
      *pChapter << pBody;

      *pBody << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("Strand Slope.jpg") ) << rptNewLine;
   
      rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(2,_T(""));
      *pBody << pTable;

      pTable->SetColumnStyle(0, pgsReportStyleHolder::GetTableCellStyle( CB_NONE | CJ_LEFT) );
      pTable->SetStripeRowColumnStyle(0, pgsReportStyleHolder::GetTableStripeRowCellStyle( CB_NONE | CJ_LEFT) );

      (*pTable)(0,0) << _T("");
      (*pTable)(0,1) << _T("1 : n");

      (*pTable)(1,0) << _T("Allowable Slope");
      (*pTable)(2,0) << _T("Strand Slope");
      (*pTable)(3,0) << _T("Status");

      (*pTable)(1,1) << slope.SetValue(pArtifact->GetCapacity());
      (*pTable)(2,1) << slope.SetValue(pArtifact->GetDemand());

      if ( pArtifact->Passed() )
         (*pTable)(3,1) << RPT_PASS;
      else
         (*pTable)(3,1) << RPT_FAIL;
   }
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void CStrandSlopeCheck::MakeCopy(const CStrandSlopeCheck& rOther)
{
   // Add copy code here...
}

void CStrandSlopeCheck::MakeAssignment(const CStrandSlopeCheck& rOther)
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
bool CStrandSlopeCheck::AssertValid() const
{
   return true;
}

void CStrandSlopeCheck::Dump(dbgDumpContext& os) const
{
   os << _T("Dump for CStrandSlopeCheck") << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool CStrandSlopeCheck::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("CStrandSlopeCheck");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for CStrandSlopeCheck");

   TESTME_EPILOG("LiveLoadDistributionFactorTable");
}
#endif // _UNITTEST
