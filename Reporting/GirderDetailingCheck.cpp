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
#include <Reporting\GirderDetailingCheck.h>
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
   CGirderDetailingCheck
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CGirderDetailingCheck::CGirderDetailingCheck():
m_BasicVersion(false)
{
}

CGirderDetailingCheck::CGirderDetailingCheck(bool basic):
m_BasicVersion(basic)
{
}

CGirderDetailingCheck::CGirderDetailingCheck(const CGirderDetailingCheck& rOther)
{
   MakeCopy(rOther);
}

CGirderDetailingCheck::~CGirderDetailingCheck()
{
}

//======================== OPERATORS  =======================================
CGirderDetailingCheck& CGirderDetailingCheck::operator= (const CGirderDetailingCheck& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
void CGirderDetailingCheck::Build(rptChapter* pChapter,
                              IBroker* pBroker,SpanIndexType span,GirderIndexType girder,
                              IEAFDisplayUnits* pDisplayUnits) const
{

   if (!m_BasicVersion)
   {
      // girder dimensions check table
      CGirderDetailingCheck::BuildDimensionCheck(pChapter, pBroker, span, girder, pDisplayUnits);
   }

   // Stirrup detailing check
   rptParagraph* p = new rptParagraph;
   bool write_note;
   *p << CStirrupDetailingCheckTable().Build(pBroker,span,girder,pDisplayUnits,pgsTypes::BridgeSite3,pgsTypes::StrengthI,&write_note) << rptNewLine;
   *pChapter << p;

   if (write_note)
   {
      *p << _T("* - Transverse reinforcement not required if ") << Sub2(_T("V"),_T("u")) << _T(" < 0.5") << symbol(phi) << _T("(") << Sub2(_T("V"),_T("c"));
      *p  << _T(" + ") << Sub2(_T("V"),_T("p")) << _T(") [Eqn 5.8.2.4-1]")<< rptNewLine;
   }
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void CGirderDetailingCheck::MakeCopy(const CGirderDetailingCheck& rOther)
{
   // Add copy code here...
}

void CGirderDetailingCheck::MakeAssignment(const CGirderDetailingCheck& rOther)
{
   MakeCopy( rOther );
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PRIVATE    ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void CGirderDetailingCheck::BuildDimensionCheck(rptChapter* pChapter,
                              IBroker* pBroker,SpanIndexType span,GirderIndexType girder,
                              IEAFDisplayUnits* pDisplayUnits) const
{
   GET_IFACE2(pBroker,IArtifact,pIArtifact);
   const pgsGirderArtifact* pGdrArtifact = pIArtifact->GetArtifact(span,girder);
   const pgsPrecastIGirderDetailingArtifact* pArtifact = pGdrArtifact->GetPrecastIGirderDetailingArtifact();
   
   rptParagraph* pBody = new rptParagraph;
   *pChapter << pBody;

   rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(4,_T("Girder Dimensions Detailing Check [5.14.1.2.2]"));
   *pBody << pTable;

   pTable->SetColumnStyle(0, pgsReportStyleHolder::GetTableCellStyle( CB_NONE | CJ_LEFT) );
   pTable->SetStripeRowColumnStyle(0, pgsReportStyleHolder::GetTableStripeRowCellStyle( CB_NONE | CJ_LEFT) );

   (*pTable)(0,0)  << _T("Dimension");
   (*pTable)(0,1)  << COLHDR(_T("Minimum"),  rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*pTable)(0,2)  << COLHDR(_T("Actual"),       rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*pTable)(0,3)  << _T("Status");

   (*pTable)(1,0) << _T("Top Flange Thickness");
   (*pTable)(2,0) << _T("Web Thickness");
   (*pTable)(3,0) << _T("Bottom Flange Thickness");

   INIT_UV_PROTOTYPE( rptLengthSectionValue,      dim,      pDisplayUnits->GetComponentDimUnit(),  false );

   if ( IsZero(pArtifact->GetProvidedTopFlangeThickness()) )
   {
      // There is no top flange... Assume this is not a bulb-T or I type section
      (*pTable)(1,1) << _T("-");
      (*pTable)(1,2) << _T("-");
      (*pTable)(1,3) << RPT_NA << rptNewLine << _T("See LRFD C5.14.1.2.2");
   }
   else
   {
      (*pTable)(1,1) << dim.SetValue(pArtifact->GetMinTopFlangeThickness());
      (*pTable)(1,2) << dim.SetValue(pArtifact->GetProvidedTopFlangeThickness());
      if ( pArtifact->GetMinTopFlangeThickness() > pArtifact->GetProvidedTopFlangeThickness())
         (*pTable)(1,3) << RPT_FAIL;
      else
         (*pTable)(1,3) << RPT_PASS;
   }

   if ( IsZero(pArtifact->GetProvidedWebThickness()) )
   {
      // There is no web... voided slab type girder
      (*pTable)(2,1) << _T("-");
      (*pTable)(2,2) << _T("-");
      (*pTable)(2,3) << RPT_NA;
   }
   else
   {
      (*pTable)(2,1) << dim.SetValue(pArtifact->GetMinWebThickness());
      (*pTable)(2,2) << dim.SetValue(pArtifact->GetProvidedWebThickness());
      if ( pArtifact->GetMinWebThickness() > pArtifact->GetProvidedWebThickness())
         (*pTable)(2,3) << RPT_FAIL;
      else
         (*pTable)(2,3) << RPT_PASS;
   }

   if ( IsZero(pArtifact->GetProvidedBottomFlangeThickness()) )
   {
      // There is no bottom flange... Assume this is stemmed girder
      (*pTable)(3,1) << _T("-");
      (*pTable)(3,2) << _T("-");
      (*pTable)(3,3) << RPT_NA;
   }
   else
   {
      (*pTable)(3,1) << dim.SetValue(pArtifact->GetMinBottomFlangeThickness());
      (*pTable)(3,2) << dim.SetValue(pArtifact->GetProvidedBottomFlangeThickness());
      if ( pArtifact->GetMinBottomFlangeThickness() > pArtifact->GetProvidedBottomFlangeThickness())
         (*pTable)(3,3) << RPT_FAIL;
      else
         (*pTable)(3,3) << RPT_PASS;
   }
}



//======================== ACCESS     =======================================
//======================== INQUERY    =======================================

//======================== DEBUG      =======================================
#if defined _DEBUG
bool CGirderDetailingCheck::AssertValid() const
{
   return true;
}

void CGirderDetailingCheck::Dump(dbgDumpContext& os) const
{
   os << _T("Dump for CGirderDetailingCheck") << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool CGirderDetailingCheck::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("CGirderDetailingCheck");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for CGirderDetailingCheck");

   TESTME_EPILOG("LiveLoadDistributionFactorTable");
}
#endif // _UNITTEST
