///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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
#include <Reporting\StirrupDetailingCheckTable.h>
#include <Reporting\ReportNotes.h>

#include <PgsExt\PointOfInterest.h>
#include <PgsExt\GirderArtifact.h>

#include <IFace\Bridge.h>
#include <IFace\DisplayUnits.h>
#include <IFace\Artifact.h>

#include <Lrfd\Rebar.h>
#include <Lrfd\RebarPool.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CStirrupDetailingCheckTable
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CStirrupDetailingCheckTable::CStirrupDetailingCheckTable()
{
}

CStirrupDetailingCheckTable::~CStirrupDetailingCheckTable()
{
}

//======================== OPERATORS  =======================================

//======================== OPERATIONS =======================================
rptRcTable* CStirrupDetailingCheckTable::Build(IBroker* pBroker,SpanIndexType span,GirderIndexType girder,
                                               IDisplayUnits* pDisplayUnits,
                                               pgsTypes::Stage stage,
                                               pgsTypes::LimitState ls,
                                               bool* pWriteNote) const
{
   *pWriteNote = false; // 

   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(8," ");
   table->TableLabel() << "Stirrup Detailing Check [5.8.2.5, 5.8.2.7, 5.10.3.1.2]";
  
   if ( stage == pgsTypes::CastingYard )
      (*table)(0,0)  << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   else
      (*table)(0,0)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

   (*table)(0,1)  << "Bar Size";
   (*table)(0,2)  << COLHDR("S",            rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,3)  << COLHDR("S"<<Sub("max"),  rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,4)  << COLHDR("S"<<Sub("min"),  rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,5)  << COLHDR("A" << Sub("v")<<"/S" , rptAreaPerLengthUnitTag, pDisplayUnits->GetAvOverSUnit() );
   (*table)(0,6)  << COLHDR("A" << Sub("v")<<"/S"<<Sub("min") , rptAreaPerLengthUnitTag, pDisplayUnits->GetAvOverSUnit() );
   (*table)(0,7)  << "Status";

   INIT_UV_PROTOTYPE( rptPointOfInterest,         location, pDisplayUnits->GetSpanLengthUnit(),   false );
   INIT_UV_PROTOTYPE( rptAreaPerLengthValue,      AvS,      pDisplayUnits->GetAvOverSUnit(),  false );
   INIT_UV_PROTOTYPE( rptLengthSectionValue,      dim,      pDisplayUnits->GetComponentDimUnit(),  false );

   // Fill up the table
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);
   GET_IFACE2(pBroker,IArtifact,pIArtifact);

   const pgsGirderArtifact* gdrArtifact = pIArtifact->GetArtifact(span,girder);
   const pgsStirrupCheckArtifact* pstirrup_artifact= gdrArtifact->GetStirrupCheckArtifact();
   CHECK(pstirrup_artifact);

   std::vector<pgsPointOfInterest> vPoi = pIPoi->GetPointsOfInterest( stage, span, girder, POI_TABULAR|POI_SHEAR );

   Float64 end_size = pBridge->GetGirderStartConnectionLength(span,girder);
   if ( stage == pgsTypes::CastingYard )
      end_size = 0; // don't adjust if CY stage

   lrfdRebarPool* pool = lrfdRebarPool::GetInstance();
   CHECK(pool!=0);

   RowIndexType row = table->GetNumberOfHeaderRows();
   std::vector<pgsPointOfInterest>::const_iterator i;
   for ( i = vPoi.begin(); i != vPoi.end(); i++ )
   {
      const pgsPointOfInterest& poi = *i;

      const pgsStirrupCheckAtPoisArtifact* psArtifact = pstirrup_artifact->GetStirrupCheckAtPoisArtifact( pgsStirrupCheckAtPoisArtifactKey(stage,ls,poi.GetDistFromStart()) );
      if ( psArtifact == NULL )
         continue;

      const pgsStirrupDetailArtifact* pArtifact = psArtifact->GetStirrupDetailArtifact();

      (*table)(row,0) << location.SetValue( poi, end_size );

      BarSizeType bs= pArtifact->GetBarSize();
      if (bs!=0)
         (*table)(row,1) << pool->GetRebar(bs)->GetName();
      else
         (*table)(row,1) << "none";

      Float64 s = pArtifact->GetS();
      if (s>0)
         (*table)(row,2) << dim.SetValue(s);
      else
         (*table)(row,2) << "-";

      (*table)(row,3) << dim.SetValue( pArtifact->GetSMax() );
      (*table)(row,4) << dim.SetValue( pArtifact->GetSMin() );

      (*table)(row,5) << AvS.SetValue(pArtifact->GetAvs());
      (*table)(row,6) << AvS.SetValue(pArtifact->GetAvsMin());

      if (!pArtifact->IsApplicable())
      {
         *pWriteNote = true; // need to write note that stirrups are not required
      }

      if ( pArtifact->Passed() )
         (*table)(row,7) << RPT_PASS;
      else
         (*table)(row,7) << RPT_FAIL;

      row++;
   }

   if (*pWriteNote)
   {
      (*table)(0,6)  << superscript(ON)<<"*"<<superscript(OFF);
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
bool CStirrupDetailingCheckTable::AssertValid() const
{
   return true;
}

void CStirrupDetailingCheckTable::Dump(dbgDumpContext& os) const
{
   os << "Dump for CStirrupDetailingCheckTable" << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool CStirrupDetailingCheckTable::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("CStirrupDetailingCheckTable");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for CStirrupDetailingCheckTable");

   TESTME_EPILOG("CStirrupDetailingCheckTable");
}
#endif // _UNITTEST
