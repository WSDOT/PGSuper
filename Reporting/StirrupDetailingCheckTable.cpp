///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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

#include <PgsExt\GirderPointOfInterest.h>
#include <PgsExt\GirderArtifact.h>

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

   *pWriteNote = false; // 

   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(8,_T(" "));
   table->TableLabel() << _T("Stirrup Detailing Check [5.8.2.5, 5.8.2.7, 5.10.3.1.2]");
  
   if ( girderKey.groupIndex == ALL_GROUPS )
   {
      table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   (*table)(0,0)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table)(0,1)  << _T("Bar Size");
   (*table)(0,2)  << COLHDR(_T("S"),            rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,3)  << COLHDR(_T("S")<<Sub(_T("max")),  rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,4)  << COLHDR(_T("S")<<Sub(_T("min")),  rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,5)  << COLHDR(_T("A") << Sub(_T("v"))<<_T("/S") , rptAreaPerLengthUnitTag, pDisplayUnits->GetAvOverSUnit() );
   (*table)(0,6)  << COLHDR(_T("A") << Sub(_T("v"))<<_T("/S")<<Sub(_T("min")) , rptAreaPerLengthUnitTag, pDisplayUnits->GetAvOverSUnit() );
   (*table)(0,7)  << _T("Status");

   INIT_UV_PROTOTYPE( rptPointOfInterest,         location, pDisplayUnits->GetSpanLengthUnit(),   false );
   INIT_UV_PROTOTYPE( rptAreaPerLengthValue,      AvS,      pDisplayUnits->GetAvOverSUnit(),  false );
   INIT_UV_PROTOTYPE( rptLengthSectionValue,      dim,      pDisplayUnits->GetComponentDimUnit(),  false );

   location.IncludeSpanAndGirder(girderKey.groupIndex == ALL_GROUPS);

   // Fill up the table
   GET_IFACE2(pBroker,IBridge,pBridge);

   Float64 end_size = pBridge->GetSegmentStartEndDistance(CSegmentKey(girderKey,0));


   lrfdRebarPool* pool = lrfdRebarPool::GetInstance();
   ATLASSERT(pool != NULL);

   RowIndexType row = table->GetNumberOfHeaderRows();

   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      const pgsSegmentArtifact* pSegmentArtifact = pGirderArtifact->GetSegmentArtifact(segIdx);
      const pgsStirrupCheckArtifact* pStirrupArtifact = pSegmentArtifact->GetStirrupCheckArtifact();
      ATLASSERT(pStirrupArtifact != NULL);


      CollectionIndexType nArtifacts = pStirrupArtifact->GetStirrupCheckAtPoisArtifactCount( intervalIdx,ls );
      for ( CollectionIndexType idx = 0; idx < nArtifacts; idx++ )
      {
         const pgsStirrupCheckAtPoisArtifact* psArtifact = pStirrupArtifact->GetStirrupCheckAtPoisArtifact( intervalIdx,ls,idx );
         if ( psArtifact == NULL )
            continue;

         const pgsPointOfInterest& poi = psArtifact->GetPointOfInterest();

         const pgsStirrupDetailArtifact* pArtifact = psArtifact->GetStirrupDetailArtifact();

         (*table)(row,0) << location.SetValue( POI_ERECTED_SEGMENT, poi, end_size );
         (*table)(row,1) << lrfdRebarPool::GetBarSize(pArtifact->GetBarSize()).c_str();

         Float64 s = pArtifact->GetS();
         if (0 < s)
            (*table)(row,2) << dim.SetValue(s);
         else
            (*table)(row,2) << _T("-");

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
   } // next segment

   if (*pWriteNote)
   {
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

void CStirrupDetailingCheckTable::Dump(dbgDumpContext& os) const
{
   os << _T("Dump for CStirrupDetailingCheckTable") << endl;
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
