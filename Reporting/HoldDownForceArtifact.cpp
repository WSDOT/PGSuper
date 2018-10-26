///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2012  Washington State Department of Transportation
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
#include <Reporting\HoldDownForceCheck.h>

#include <IFace\Artifact.h>
#include <EAF\EAFDisplayUnits.h>

#include <PgsExt\GirderArtifact.h>
#include <PgsExt\HoldDownForceArtifact.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CHoldDownForceCheck
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CHoldDownForceCheck::CHoldDownForceCheck()
{
}

CHoldDownForceCheck::CHoldDownForceCheck(const CHoldDownForceCheck& rOther)
{
   MakeCopy(rOther);
}

CHoldDownForceCheck::~CHoldDownForceCheck()
{
}

//======================== OPERATORS  =======================================
CHoldDownForceCheck& CHoldDownForceCheck::operator= (const CHoldDownForceCheck& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
rptRcTable* CHoldDownForceCheck::Build(IBroker* pBroker,SpanIndexType span,GirderIndexType girder,
                                       IEAFDisplayUnits* pDisplayUnits) const
{
   GET_IFACE2(pBroker,IArtifact,pIArtifact);
   const pgsGirderArtifact* pGdrArtifact = pIArtifact->GetArtifact(span,girder);
   const pgsHoldDownForceArtifact* pArtifact = pGdrArtifact->GetHoldDownForceArtifact();

   if ( pArtifact->IsApplicable() )
   {
      INIT_UV_PROTOTYPE( rptForceUnitValue, force, pDisplayUnits->GetGeneralForceUnit(), false );

      rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(3,_T("Hold Down Force"));

      (*pTable)(0,0) << COLHDR(_T("Hold Down Force"),    rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit() );
      (*pTable)(0,1) << COLHDR(_T("Max Hold Down Force"),   rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit() );
      (*pTable)(0,2) << _T("Status");

      (*pTable)(1,0) << force.SetValue(pArtifact->GetDemand());
      (*pTable)(1,1) << force.SetValue(pArtifact->GetCapacity());

      if ( pArtifact->Passed() )
         (*pTable)(1,2) << RPT_PASS;
      else
         (*pTable)(1,2) << RPT_FAIL;

      return pTable;
   }
   else
   {
      // no table if not applicable
      return NULL;
   }
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void CHoldDownForceCheck::MakeCopy(const CHoldDownForceCheck& rOther)
{
   // Add copy code here...
}

void CHoldDownForceCheck::MakeAssignment(const CHoldDownForceCheck& rOther)
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
bool CHoldDownForceCheck::AssertValid() const
{
   return true;
}

void CHoldDownForceCheck::Dump(dbgDumpContext& os) const
{
   os << _T("Dump for CHoldDownForceCheck") << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool CHoldDownForceCheck::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("CHoldDownForceCheck");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for CHoldDownForceCheck");

   TESTME_EPILOG("LiveLoadDistributionFactorTable");
}
#endif // _UNITTEST
