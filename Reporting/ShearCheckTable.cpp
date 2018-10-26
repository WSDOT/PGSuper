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
#include <Reporting\ShearCheckTable.h>
#include <Reporting\ReportNotes.h>

#include <PgsExt\PointOfInterest.h>
#include <PgsExt\GirderArtifact.h>

#include <IFace\Bridge.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\Artifact.h>

#include <PgsExt\CapacityToDemand.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CShearCheckTable
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CShearCheckTable::CShearCheckTable()
{
}


CShearCheckTable::~CShearCheckTable()
{
}

//======================== OPERATORS  =======================================

//======================== OPERATIONS =======================================
rptRcTable* CShearCheckTable::Build(IBroker* pBroker,SpanIndexType span,GirderIndexType girder,
                                               IEAFDisplayUnits* pDisplayUnits,
                                               pgsTypes::Stage stage,
                                               pgsTypes::LimitState ls,bool& bStrutAndTieRequired) const
{
   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(6," ");

   if ( span == ALL_SPANS )
   {
      table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   if (ls==pgsTypes::StrengthI)
      table->TableLabel() << "Ultimate Shears for Strength I Limit State for Bridge Site Stage 3 [5.8]";
   else
      table->TableLabel() << "Ultimate Shears for Strength II Limit State for Bridge Site Stage 3 [5.8]";
  
   if ( stage == pgsTypes::CastingYard )
      (*table)(0,0)  << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   else
      (*table)(0,0)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

   (*table)(0,1) << "Stirrups" << rptNewLine << "Required";
   (*table)(0,2) << "Stirrups" << rptNewLine << "Provided";
   (*table)(0,3)  << COLHDR("|V" << Sub("u") << "|", rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   (*table)(0,4) << COLHDR(symbol(phi) << "V" << Sub("n"), rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   (*table)(0,5) << "Status" << rptNewLine << "(" << symbol(phi) << Sub2("V","n") << "/" << Sub2("V","u") << ")";

   INIT_UV_PROTOTYPE( rptPointOfInterest, location,  pDisplayUnits->GetSpanLengthUnit(),   false );
   INIT_UV_PROTOTYPE( rptForceSectionValue,  shear,  pDisplayUnits->GetShearUnit(),        false );

   location.IncludeSpanAndGirder(span == ALL_SPANS);

   rptCapacityToDemand cap_demand;

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

   bStrutAndTieRequired = false;
   RowIndexType row = table->GetNumberOfHeaderRows();
   std::vector<pgsPointOfInterest>::const_iterator i;
   for ( i = vPoi.begin(); i != vPoi.end(); i++ )
   {
      const pgsPointOfInterest& poi = *i;
      const pgsStirrupCheckAtPoisArtifact* psArtifact = pstirrup_artifact->GetStirrupCheckAtPoisArtifact( pgsStirrupCheckAtPoisArtifactKey(stage,ls,poi.GetDistFromStart()) );
      if ( psArtifact == NULL )
         continue;

      (*table)(row,0) << location.SetValue( poi, end_size );

      const pgsVerticalShearArtifact* pArtifact = psArtifact->GetVerticalShearArtifact();

      (*table)(row,1) << (pArtifact->GetAreStirrupsReqd()     ? "Yes" : "No");
      (*table)(row,2) << (pArtifact->GetAreStirrupsProvided() ? "Yes" : "No");

      Float64 Vu;
      Float64 Vr;
      if ( pArtifact->IsApplicable() )
      {
         Vu = pArtifact->GetDemand();
         Vr = pArtifact->GetCapacity();

         (*table)(row,3) << shear.SetValue( Vu );
         (*table)(row,4) << shear.SetValue( Vr );
      }
      else
      {
         if ( pArtifact->IsStrutAndTieRequired() )
         {
            (*table)(row,3) << "*";
            (*table)(row,4) << "*";
         }
         else
         {
            (*table)(row,3) << "$";
            (*table)(row,4) << "$";
         }
      }

      if ( pArtifact->IsApplicable() )
      {
         bool bPassed = pArtifact->Passed();
         if ( bPassed )
            (*table)(row,5) << RPT_PASS;
         else
            (*table)(row,5) << RPT_FAIL;

         (*table)(row,5) << rptNewLine << "(" << cap_demand.SetValue(Vr,Vu,bPassed) << ")";
      }
      else
      {
         if ( pArtifact->IsStrutAndTieRequired() )
         {
            (*table)(row,5) << "*";
         }
         else
         {
            if ( pArtifact->Passed() )
               (*table)(row,5) << RPT_PASS;
            else
               (*table)(row,5) << RPT_FAIL;
         }
      }

      if ( pArtifact->IsStrutAndTieRequired() )
         bStrutAndTieRequired = true;

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
bool CShearCheckTable::AssertValid() const
{
   return true;
}

void CShearCheckTable::Dump(dbgDumpContext& os) const
{
   os << "Dump for CShearCheckTable" << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool CShearCheckTable::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("CShearCheckTable");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for CShearCheckTable");

   TESTME_EPILOG("CShearCheckTable");
}
#endif // _UNITTEST
