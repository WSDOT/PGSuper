///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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

#include <PgsExt\ReportPointOfInterest.h>
#include <PgsExt\GirderArtifact.h>

#include <IFace\DocumentType.h>
#include <IFace\Bridge.h>
#include <IFace\Project.h>
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
rptRcTable* CShearCheckTable::Build(IBroker* pBroker,const pgsGirderArtifact* pGirderArtifact,
                                               IEAFDisplayUnits* pDisplayUnits,
                                               IntervalIndexType intervalIdx,
                                               pgsTypes::LimitState ls,bool& bStrutAndTieRequired) const
{
   const CGirderKey& girderKey(pGirderArtifact->GetGirderKey());

   rptRcTable* table = rptStyleManager::CreateDefaultTable(6,_T(" "));

   if ( girderKey.groupIndex == ALL_GROUPS )
   {
      table->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   if (ls == pgsTypes::StrengthI)
   {
      table->TableLabel() << _T("Ultimate Shears for Strength I Limit State ") << LrfdCw8th(_T("[5.7]"),_T("[5.8]"));
   }
   else
   {
      table->TableLabel() << _T("Ultimate Shears for Strength II Limit State ") << LrfdCw8th(_T("[5.7]"),_T("[5.8]"));
   }
  
   (*table)(0,0)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

   (*table)(0,1) << _T("Stirrups") << rptNewLine << _T("Required");
   (*table)(0,2) << _T("Stirrups") << rptNewLine << _T("Provided");
   (*table)(0,3)  << COLHDR(_T("|V") << Sub(_T("u")) << _T("|"), rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   (*table)(0,4) << COLHDR(symbol(phi) << _T("V") << Sub(_T("n")), rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   (*table)(0,5) << _T("Status") << rptNewLine << _T("(") << symbol(phi) << Sub2(_T("V"),_T("n")) << _T("/") << Sub2(_T("V"),_T("u")) << _T(")");

   INIT_UV_PROTOTYPE( rptPointOfInterest, location,  pDisplayUnits->GetSpanLengthUnit(),   false );
   INIT_UV_PROTOTYPE( rptForceSectionValue,  shear,  pDisplayUnits->GetShearUnit(),        false );

   GET_IFACE2(pBroker, IDocumentType, pDocType);
   location.IncludeSpanAndGirder(pDocType->IsPGSpliceDocument() || girderKey.groupIndex == ALL_GROUPS);

   rptCapacityToDemand cap_demand;

   // Fill up the table
   RowIndexType row = table->GetNumberOfHeaderRows();

   bool bIsStrutAndTieRequired = false;
   GET_IFACE2(pBroker,IBridge,pBridge);
   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      const pgsSegmentArtifact* pSegmentArtifact = pGirderArtifact->GetSegmentArtifact(segIdx);

      const pgsStirrupCheckArtifact* pStirrupArtifact = pSegmentArtifact->GetStirrupCheckArtifact();
      ATLASSERT(pStirrupArtifact);
      CollectionIndexType nArtifacts = pStirrupArtifact->GetStirrupCheckAtPoisArtifactCount( intervalIdx,ls );
      for ( CollectionIndexType idx = 0; idx < nArtifacts; idx++ )
      {
         const pgsStirrupCheckAtPoisArtifact* psArtifact = pStirrupArtifact->GetStirrupCheckAtPoisArtifact( intervalIdx,ls,idx );
         if ( psArtifact == nullptr )
         {
            continue;
         }

         const pgsPointOfInterest& poi = psArtifact->GetPointOfInterest();

         const pgsVerticalShearArtifact* pArtifact = psArtifact->GetVerticalShearArtifact();

         bool needs_strut_tie = pArtifact->IsStrutAndTieRequired();

         if ( pArtifact->IsApplicable() )
         {
            (*table)(row,0) << location.SetValue( POI_SPAN, poi );
            (*table)(row,1) << (pArtifact->GetAreStirrupsReqd()     ? _T("Yes") : _T("No"));
            (*table)(row,2) << (pArtifact->GetAreStirrupsProvided() ? _T("Yes") : _T("No"));

            Float64 Vu;
            Float64 Vr;
            Vu = pArtifact->GetDemand();
            Vr = pArtifact->GetCapacity();

            (*table)(row,3) << shear.SetValue( Vu );
            (*table)(row,4) << shear.SetValue( Vr );

            if(needs_strut_tie)
            {
               (*table)(row,5) << _T("*");
            }

            bool bPassed = pArtifact->Passed();
            if ( bPassed )
            {
               (*table)(row,5) << RPT_PASS;
            }
            else
            {
               (*table)(row,5) << RPT_FAIL;
            }

            (*table)(row,5) << rptNewLine << _T("(") << cap_demand.SetValue(Vr,Vu,bPassed) << _T(")");

            row++;
         }// next artifact

         if (needs_strut_tie)
         {
            bIsStrutAndTieRequired = true;
         }

      }

   } // next segment

   bStrutAndTieRequired = bIsStrutAndTieRequired;

   return table;
}

void CShearCheckTable::BuildNotes(rptChapter* pChapter, 
                           IBroker* pBroker,const pgsGirderArtifact* pGirderArtifact,
                           IEAFDisplayUnits* pDisplayUnits,
                           IntervalIndexType intervalIdx, pgsTypes::LimitState ls, bool bStrutAndTieRequired) const
{
   const CGirderKey& girderKey(pGirderArtifact->GetGirderKey());

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

      Float64 end_size = pBridge->GetSegmentStartEndDistance(CSegmentKey(girderKey,0));
      INIT_UV_PROTOTYPE( rptPointOfInterest, location,  pDisplayUnits->GetSpanLengthUnit(),   true );

      *p << SUPPORT_COMPRESSION << rptNewLine << rptNewLine;

      SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         const pgsSegmentArtifact* pSegmentArtifact = pGirderArtifact->GetSegmentArtifact(segIdx);
         const pgsStirrupCheckArtifact* pStirrupArtifact = pSegmentArtifact->GetStirrupCheckArtifact();
         ATLASSERT(pStirrupArtifact);

         // Cycle through artifacts to see if av/s decreases past CSS - generate a FAIL if so
         CollectionIndexType nArtifacts = pStirrupArtifact->GetStirrupCheckAtPoisArtifactCount( intervalIdx,ls );
         for ( CollectionIndexType idx = 0; idx < nArtifacts; idx++ )
         {
            const pgsStirrupCheckAtPoisArtifact* psArtifact = pStirrupArtifact->GetStirrupCheckAtPoisArtifact( intervalIdx,ls,idx );
            if ( psArtifact == nullptr )
            {
               continue;
            }

            const pgsPointOfInterest& poi = psArtifact->GetPointOfInterest();

            const pgsVerticalShearArtifact* pArtifact = psArtifact->GetVerticalShearArtifact();

            if ( pArtifact->DidAvsDecreaseAtEnd() )
            {
               *p << RPT_FAIL << _T(" - The shear capacity, V")<< Sub(_T("s")) << _T(" at ") << location.SetValue(POI_SPAN, poi)
                  << _T(" is less than the capacity at the design section (CS). Revise stirrup details to increase ") << Sub2(_T("A"),_T("v")) << _T("/S")
                  << _T(" at this location.") << rptNewLine;
            }
         }
      } // next segment
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

void CShearCheckTable::Dump(WBFL::Debug::LogContext& os) const
{
   os << _T("Dump for CShearCheckTable") << WBFL::Debug::endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool CShearCheckTable::TestMe(WBFL::Debug::Log& rlog)
{
   TESTME_PROLOGUE("CShearCheckTable");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for CShearCheckTable");

   TESTME_EPILOG("CShearCheckTable");
}
#endif // _UNITTEST
