///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2011  Washington State Department of Transportation
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
#include <Reporting\ConfinementCheckTable.h>

#include <IFace\Bridge.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\Artifact.h>

#include <PgsExt\GirderArtifact.h>

#include <Lrfd\Rebar.h>
#include <Lrfd\RebarPool.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CConfinementCheckTable
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CConfinementCheckTable::CConfinementCheckTable()
{
}

CConfinementCheckTable::~CConfinementCheckTable()
{
}

//======================== OPERATORS  =======================================

//======================== OPERATIONS =======================================
rptRcTable* CConfinementCheckTable::Build(IBroker* pBroker,SpanIndexType span,GirderIndexType girder,
                                               IEAFDisplayUnits* pDisplayUnits,
                                               pgsTypes::Stage stage) const
{
   GET_IFACE2(pBroker,IStirrupGeometry, pStirrupGeometry);
   // no table if no stirrup zones
   ZoneIndexType nz = pStirrupGeometry->GetNumZones(span,girder);
   if (nz==0)
      return 0;

   INIT_UV_PROTOTYPE( rptLengthUnitValue,    location, pDisplayUnits->GetSpanLengthUnit(),   false );
   INIT_UV_PROTOTYPE( rptLengthSectionValue, dim,      pDisplayUnits->GetComponentDimUnit(),  false );

   // get length of confinement zone
   GET_IFACE2(pBroker,IArtifact,pIArtifact);
   const pgsGirderArtifact* gdrArtifact = pIArtifact->GetArtifact(span,girder);
   const pgsStirrupCheckArtifact* pstirrup_artifact= gdrArtifact->GetStirrupCheckArtifact();
   CHECK(pstirrup_artifact);
   const pgsStirrupCheckAtZonesArtifact* pz0  = pstirrup_artifact->GetStirrupCheckAtZonesArtifact( 0 );
   const pgsConfinementArtifact*  pcz0 = pz0->GetConfinementArtifact();
   CHECK(pcz0->IsApplicable()); // should be blocked by caller
   Float64 conend = pcz0->GetApplicableZoneLength();

   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(7,_T(" "));
   table->TableLabel() << _T("Confinement Stirrup Check [5.10.10.2]")<< rptNewLine <<_T("Length of confinement zone is ")<<location.SetValue(conend)<<location.GetUnitTag();
  
   (*table)(0,0)  << _T("Zone #");
   (*table)(0,1)  << COLHDR(_T("End Location")<<rptNewLine<<_T("From Girder End"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table)(0,2)  << COLHDR(_T("S") ,  rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,3)  << COLHDR(_T("S") << Sub(_T("max")) ,  rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,4)  << _T("Bar")<<rptNewLine<<_T("Size");
   (*table)(0,5)  << _T("Min")<<rptNewLine<<_T("Bar")<<rptNewLine<<_T("Size");
   (*table)(0,6)  << _T("Status");

   lrfdRebarPool* pool = lrfdRebarPool::GetInstance();
   CHECK(pool!=0);

   // Fill up the table - zones only go half way across girder and table needs to cover
   // entire girder - mirror them

   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 girder_len = pBridge->GetGirderLength(span,girder);

   RowIndexType row = table->GetNumberOfHeaderRows();

   for ( Uint16 i=0; i<nz; i++)
   {
      (*table)(row,0) << pStirrupGeometry->GetZoneId(span,girder,i);

      const pgsStirrupCheckAtZonesArtifact* psArtifact = pstirrup_artifact->GetStirrupCheckAtZonesArtifact( i );
      const pgsConfinementArtifact* pArtifact = psArtifact->GetConfinementArtifact();

      (*table)(row,1) << location.SetValue(pArtifact->GetZoneEnd());

      bool is_app = pArtifact->IsApplicable();
      if (is_app)
      {
         (*table)(row,2) << dim.SetValue(pArtifact->GetS());
         (*table)(row,3) << dim.SetValue(pArtifact->GetSMax() );
         (*table)(row,4) << lrfdRebarPool::GetBarSize(pArtifact->GetBar() ? pArtifact->GetBar()->GetSize() : matRebar::bsNone).c_str();
         (*table)(row,5) << lrfdRebarPool::GetBarSize(pArtifact->GetMinBar()->GetSize()).c_str();

         if ( pArtifact->Passed() )
            (*table)(row,6) << RPT_PASS;
         else
            (*table)(row,6) << RPT_FAIL;
      }
      else
      {
         (*table)(row,2) << _T("-");
         (*table)(row,3) << _T("-");
         (*table)(row,4) << _T("-");
         (*table)(row,5) << _T("-");
         (*table)(row,6) << RPT_NA;
      }

      row++;
   }

   return table;
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

//======================== DEBUG      =======================================
#if defined _DEBUG
bool CConfinementCheckTable::AssertValid() const
{
   return true;
}

void CConfinementCheckTable::Dump(dbgDumpContext& os) const
{
   os << _T("Dump for CConfinementCheckTable") << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool CConfinementCheckTable::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("CConfinementCheckTable");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for CConfinementCheckTable");

   TESTME_EPILOG("CConfinementCheckTable");
}
#endif // _UNITTEST
