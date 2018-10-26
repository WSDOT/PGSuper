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
   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(6,_T(" "));

   if ( span == ALL_SPANS )
   {
      table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   if (ls==pgsTypes::StrengthI)
      table->TableLabel() << _T("Ultimate Shears for Strength I Limit State for Bridge Site Stage 3 [5.8]");
   else
      table->TableLabel() << _T("Ultimate Shears for Strength II Limit State for Bridge Site Stage 3 [5.8]");
  
   if ( stage == pgsTypes::CastingYard )
      (*table)(0,0)  << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   else
      (*table)(0,0)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

   (*table)(0,1) << _T("Stirrups") << rptNewLine << _T("Required");
   (*table)(0,2) << _T("Stirrups") << rptNewLine << _T("Provided");
   (*table)(0,3)  << COLHDR(_T("|V") << Sub(_T("u")) << _T("|"), rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   (*table)(0,4) << COLHDR(symbol(phi) << _T("V") << Sub(_T("n")), rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   (*table)(0,5) << _T("Status") << rptNewLine << _T("(") << symbol(phi) << Sub2(_T("V"),_T("n")) << _T("/") << Sub2(_T("V"),_T("u")) << _T(")");

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

   std::vector<pgsPointOfInterest> vPoi = pIPoi->GetPointsOfInterest( span, girder, stage, POI_TABULAR|POI_SHEAR );

   Float64 end_size = pBridge->GetGirderStartConnectionLength(span,girder);
   if ( stage == pgsTypes::CastingYard )
      end_size = 0; // don't adjust if CY stage

   Float64 Lg = pBridge->GetGirderLength(span,girder);

   bStrutAndTieRequired = false;
   RowIndexType row = table->GetNumberOfHeaderRows();
   std::vector<pgsPointOfInterest>::const_iterator i;
   for ( i = vPoi.begin(); i != vPoi.end(); i++ )
   {
      const pgsPointOfInterest& poi = *i;
      const pgsStirrupCheckAtPoisArtifact* psArtifact = pstirrup_artifact->GetStirrupCheckAtPoisArtifact( pgsStirrupCheckAtPoisArtifactKey(stage,ls,poi.GetDistFromStart()) );
      if ( psArtifact == NULL )
         continue;

      const pgsVerticalShearArtifact* pArtifact = psArtifact->GetVerticalShearArtifact();

      bool needs_strut_tie = pArtifact->IsStrutAndTieRequired(poi.GetDistFromStart() < Lg/2 ? pgsTypes::metStart : pgsTypes::metEnd);

      if ( pArtifact->IsApplicable() )
      {
         (*table)(row,0) << location.SetValue( stage, poi, end_size );
         (*table)(row,1) << (pArtifact->GetAreStirrupsReqd()     ? _T("Yes") : _T("No"));
         (*table)(row,2) << (pArtifact->GetAreStirrupsProvided() ? _T("Yes") : _T("No"));

         Float64 Vu;
         Float64 Vr;
         Vu = pArtifact->GetDemand();
         Vr = pArtifact->GetCapacity();

         (*table)(row,3) << shear.SetValue( Vu );
         (*table)(row,4) << shear.SetValue( Vr );

         if(needs_strut_tie)
            (*table)(row,5) << _T("*");

         bool bPassed = pArtifact->Passed();
         if ( bPassed )
            (*table)(row,5) << RPT_PASS;
         else
            (*table)(row,5) << RPT_FAIL;

         (*table)(row,5) << rptNewLine << _T("(") << cap_demand.SetValue(Vr,Vu,bPassed) << _T(")");

         row++;
      }

      if (needs_strut_tie)
         bStrutAndTieRequired = true;
   }

   return table;
}

void CShearCheckTable::BuildNotes(rptChapter* pChapter, 
                           IBroker* pBroker,SpanIndexType span,GirderIndexType girder,
                           IEAFDisplayUnits* pDisplayUnits,
                           pgsTypes::Stage stage, pgsTypes::LimitState ls, bool bStrutAndTieRequired) const
{
   if ( bStrutAndTieRequired )
   {
      rptParagraph* p = new rptParagraph();
      *pChapter << p;
      *p << STRUT_AND_TIE_REQUIRED << rptNewLine << rptNewLine;
   }
   else
   {
      rptParagraph* p = new rptParagraph();
      *pChapter << p;

      GET_IFACE2(pBroker,IBridge,pBridge);
      GET_IFACE2(pBroker,IPointOfInterest,pIPoi);
      GET_IFACE2(pBroker,IArtifact,pIArtifact);

      const pgsGirderArtifact* gdrArtifact = pIArtifact->GetArtifact(span,girder);
      const pgsStirrupCheckArtifact* pstirrup_artifact= gdrArtifact->GetStirrupCheckArtifact();
      CHECK(pstirrup_artifact);

      std::vector<pgsPointOfInterest> vPoi = pIPoi->GetPointsOfInterest( span, girder, stage, POI_TABULAR|POI_SHEAR );

      Float64 end_size = pBridge->GetGirderStartConnectionLength(span,girder);

      INIT_UV_PROTOTYPE( rptPointOfInterest, location,  pDisplayUnits->GetSpanLengthUnit(),   true );

      *p << SUPPORT_COMPRESSION << rptNewLine << rptNewLine;

      // Cycle through artifacts to see if av/s decreases past CSS - generate a FAIL if so
      std::vector<pgsPointOfInterest>::const_iterator i;
      for ( i = vPoi.begin(); i != vPoi.end(); i++ )
      {
         const pgsPointOfInterest& poi = *i;
         const pgsStirrupCheckAtPoisArtifact* psArtifact = pstirrup_artifact->GetStirrupCheckAtPoisArtifact( pgsStirrupCheckAtPoisArtifactKey(stage,ls,poi.GetDistFromStart()) );
         if ( psArtifact == NULL )
            continue;

         const pgsVerticalShearArtifact* pArtifact = psArtifact->GetVerticalShearArtifact();

         if ( pArtifact->DidAvsDecreaseAtEnd() )
         {

            *p << RPT_FAIL << _T(" - The shear capacity, V")<< Sub(_T("s")) << _T(" at ") << location.SetValue(stage, poi, end_size)
               << _T(" is less than the capacity at the design section (CS). Revise stirrup details to increase ") << Sub2(_T("A"),_T("v")) << _T("/S")
               << _T(" at this location.") << rptNewLine;
         }
      }
   }

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
   os << _T("Dump for CShearCheckTable") << endl;
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
