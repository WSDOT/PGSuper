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
#include <Reporting\StirrupDetailingCheckTable.h>
#include <Reporting\ReportNotes.h>

#include <PgsExt\ReportPointOfInterest.h>
#include <PgsExt\GirderArtifact.h>

#include <IFace\DocumentType.h>
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\Artifact.h>
#include <IFace\Intervals.h>

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
rptRcTable* CStirrupDetailingCheckTable::Build(IBroker* pBroker,const pgsGirderArtifact* pGirderArtifact,
                                               IEAFDisplayUnits* pDisplayUnits,
                                               IntervalIndexType intervalIdx,
                                               pgsTypes::LimitState ls,
                                               bool* pWriteNote) const
{
   const CGirderKey& girderKey(pGirderArtifact->GetGirderKey());

   *pWriteNote = false;

   GET_IFACE2(pBroker, IBridge, pBridge);
   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);

   bool IsUHPC = false;
   GET_IFACE2(pBroker, IMaterials, pMaterials);
   for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
   {
      if (pMaterials->GetSegmentConcreteType(CSegmentKey(girderKey, segIdx)) == pgsTypes::UHPC)
      {
         IsUHPC = true;
         break;
      }
   }

   std::_tstring strSpecArticles;
   if (IsUHPC)
      strSpecArticles = _T("1.7.2.5, 1.7.2.6, 1.10.3");
   else
      strSpecArticles = LrfdCw8th(_T("5.8.2.5, 5.8.2.7, 5.10.3.1.2"), _T("5.7.2.5, 5.7.2.6, 5.10.3.1.2"));

   rptRcTable* table = rptStyleManager::CreateDefaultTable(8,_T(" "));
   table->TableLabel() << _T("Stirrup Detailing Check: ") << GetLimitStateString(ls) << _T(" [") << strSpecArticles << _T("]");

   ColumnIndexType col = 0;

   if ( girderKey.groupIndex == ALL_GROUPS )
   {
      table->SetColumnStyle(col,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table->SetStripeRowColumnStyle(col,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   (*table)(0, col++)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table)(0, col++)  << _T("Bar Size");
   (*table)(0, col++)  << COLHDR(_T("S"),            rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0, col++)  << COLHDR(_T("S")<<Sub(_T("max")),  rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0, col++)  << COLHDR(_T("S")<<Sub(_T("min")),  rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0, col++)  << COLHDR(_T("A") << Sub(_T("v"))<<_T("/S") , rptAreaPerLengthUnitTag, pDisplayUnits->GetAvOverSUnit() );
   (*table)(0, col++)  << COLHDR(_T("A") << Sub(_T("v"))<<_T("/S")<<Sub(_T("min")) , rptAreaPerLengthUnitTag, pDisplayUnits->GetAvOverSUnit() );
   (*table)(0, col++)  << _T("Status");

   INIT_UV_PROTOTYPE( rptPointOfInterest,    location, pDisplayUnits->GetSpanLengthUnit(),   false );
   INIT_UV_PROTOTYPE( rptAreaPerLengthValue, AvS,      pDisplayUnits->GetAvOverSUnit(),      false );
   INIT_UV_PROTOTYPE( rptLengthSectionValue, dim,      pDisplayUnits->GetComponentDimUnit(), false );

   GET_IFACE2(pBroker, IDocumentType, pDocType);
   location.IncludeSpanAndGirder(pDocType->IsPGSpliceDocument() || girderKey.groupIndex == ALL_GROUPS);

   // Fill up the table

   lrfdRebarPool* pool = lrfdRebarPool::GetInstance();
   ATLASSERT(pool != nullptr);

   RowIndexType row = table->GetNumberOfHeaderRows();

   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      const pgsSegmentArtifact* pSegmentArtifact = pGirderArtifact->GetSegmentArtifact(segIdx);
      const pgsStirrupCheckArtifact* pStirrupArtifact = pSegmentArtifact->GetStirrupCheckArtifact();
      ATLASSERT(pStirrupArtifact != nullptr);

      CollectionIndexType nArtifacts = pStirrupArtifact->GetStirrupCheckAtPoisArtifactCount( intervalIdx,ls );
      for ( CollectionIndexType idx = 0; idx < nArtifacts; idx++ )
      {
         const pgsStirrupCheckAtPoisArtifact* psArtifact = pStirrupArtifact->GetStirrupCheckAtPoisArtifact( intervalIdx,ls,idx );
         if ( psArtifact == nullptr )
         {
            continue;
         }

         const pgsPointOfInterest& poi = psArtifact->GetPointOfInterest();

         const pgsStirrupDetailArtifact* pArtifact = psArtifact->GetStirrupDetailArtifact();

         col = 0;

         (*table)(row, col++) << location.SetValue( POI_SPAN, poi );
         (*table)(row, col++) << lrfdRebarPool::GetBarSize(pArtifact->GetBarSize()).c_str();

         Float64 s = pArtifact->GetS();
         if (0 < s)
         {
            (*table)(row, col++) << dim.SetValue(s);
         }
         else
         {
            (*table)(row, col++) << _T("-");
         }

         (*table)(row, col++) << dim.SetValue( pArtifact->GetSMax() );
         (*table)(row, col++) << dim.SetValue( pArtifact->GetSMin() );

         (*table)(row, col++) << AvS.SetValue(pArtifact->GetAvs());
         (*table)(row, col++) << AvS.SetValue(pArtifact->GetAvsMin());

         if (!pArtifact->IsApplicable())
         {
            *pWriteNote = true; // need to write note that stirrups are not required
         }

         if ( pArtifact->Passed() )
         {
            (*table)(row, col++) << RPT_PASS;
         }
         else
         {
            (*table)(row, col++) << RPT_FAIL;
         }

         row++;
      }
   } // next segment

   if (*pWriteNote)
   {
      // add the '*' to the column header only if the foot note is required
      (*table)(0,6)  << superscript(ON)<<_T("*")<<superscript(OFF);
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

void CStirrupDetailingCheckTable::Dump(WBFL::Debug::LogContext& os) const
{
   os << _T("Dump for CStirrupDetailingCheckTable") << WBFL::Debug::endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool CStirrupDetailingCheckTable::TestMe(WBFL::Debug::Log& rlog)
{
   TESTME_PROLOGUE("CStirrupDetailingCheckTable");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for CStirrupDetailingCheckTable");

   TESTME_EPILOG("CStirrupDetailingCheckTable");
}
#endif // _UNITTEST
