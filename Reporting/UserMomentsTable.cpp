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
#include <Reporting\UserMomentsTable.h>
#include <Reporting\ReportNotes.h>

#include <PgsExt\GirderPointOfInterest.h>

#include <IFace\Bridge.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Intervals.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CUserMomentsTable
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CUserMomentsTable::CUserMomentsTable()
{
}

CUserMomentsTable::CUserMomentsTable(const CUserMomentsTable& rOther)
{
   MakeCopy(rOther);
}

CUserMomentsTable::~CUserMomentsTable()
{
}

//======================== OPERATORS  =======================================
CUserMomentsTable& CUserMomentsTable::operator= (const CUserMomentsTable& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
rptRcTable* CUserMomentsTable::Build(IBroker* pBroker,const CGirderKey& girderKey,pgsTypes::AnalysisType analysisType,
                                      IEAFDisplayUnits* pDisplayUnits) const
{
   // Build table
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptMomentSectionValue, moment, pDisplayUnits->GetMomentUnit(), false );
   location.IncludeSpanAndGirder(girderKey.groupIndex == ALL_GROUPS);

   rptRcTable* p_table = CreateUserLoadHeading<rptMomentUnitTag,unitmgtMomentData>(_T("Moments - User Defined Loads"),false,analysisType,pDisplayUnits,pDisplayUnits->GetMomentUnit());

   if (girderKey.groupIndex == ALL_GROUPS)
   {
      p_table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      p_table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   // Get the interface pointers we need
   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);
   GET_IFACE2(pBroker,IProductForces2,pForces2);
   GET_IFACE2(pBroker,IBridge,pBridge);

   GET_IFACE2(pBroker,IProductForces,pForces);
   pgsTypes::BridgeAnalysisType maxBAT = pForces->GetBridgeAnalysisType(pgsTypes::Maximize);
   pgsTypes::BridgeAnalysisType minBAT = pForces->GetBridgeAnalysisType(pgsTypes::Minimize);

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType castDeckIntervalIdx      = pIntervals->GetCastDeckInterval();
   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval();
   IntervalIndexType liveLoadIntervalIdx      = pIntervals->GetLiveLoadInterval();

   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   GroupIndexType startGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
   GroupIndexType endGroupIdx   = (girderKey.groupIndex == ALL_GROUPS ? nGroups-1 : startGroupIdx);

   RowIndexType row = p_table->GetNumberOfHeaderRows();
   for ( GroupIndexType grpIdx = startGroupIdx; grpIdx <= endGroupIdx; grpIdx++ )
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
      GirderIndexType gdrIdx = (nGirders <= girderKey.girderIndex ? nGirders-1 : girderKey.girderIndex);

      std::vector<pgsPointOfInterest> vPoi( pIPoi->GetPointsOfInterest(CSegmentKey(grpIdx,gdrIdx,ALL_SEGMENTS)) );

      Float64 end_size = pBridge->GetSegmentStartEndDistance(CSegmentKey(grpIdx,gdrIdx,0));

      std::vector<Float64> minDC1, maxDC1, minDC2, maxDC2;
      std::vector<Float64> minDW1, maxDW1, minDW2, maxDW2;
      std::vector<Float64> minLL3, maxLL3;


      maxDC1 = pForces2->GetMoment( castDeckIntervalIdx, pftUserDC, vPoi, maxBAT );
      minDC1 = pForces2->GetMoment( castDeckIntervalIdx, pftUserDC, vPoi, minBAT );
      maxDC2 = pForces2->GetMoment( compositeDeckIntervalIdx, pftUserDC, vPoi, maxBAT );
      minDC2 = pForces2->GetMoment( compositeDeckIntervalIdx, pftUserDC, vPoi, minBAT );


      maxDW1 = pForces2->GetMoment( castDeckIntervalIdx, pftUserDW, vPoi, maxBAT );
      minDW1 = pForces2->GetMoment( castDeckIntervalIdx, pftUserDW, vPoi, minBAT );
      maxDW2 = pForces2->GetMoment( compositeDeckIntervalIdx, pftUserDW, vPoi, maxBAT );
      minDW2 = pForces2->GetMoment( compositeDeckIntervalIdx, pftUserDW, vPoi, minBAT );
      maxLL3 = pForces2->GetMoment( liveLoadIntervalIdx, pftUserLLIM, vPoi, maxBAT );
      minLL3 = pForces2->GetMoment( liveLoadIntervalIdx, pftUserLLIM, vPoi, minBAT );

      // Fill up the table
      IndexType index = 0;
      std::vector<pgsPointOfInterest>::const_iterator i(vPoi.begin());
      std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
      for ( ; i != end; i++, index++ )
      {
         ColumnIndexType col = 0;
         const pgsPointOfInterest& poi = *i;

         (*p_table)(row,col++) << location.SetValue( POI_ERECTED_SEGMENT, poi, end_size );

         if ( analysisType == pgsTypes::Envelope )
         {
            (*p_table)(row,col++) << moment.SetValue( maxDC1[index] );
            (*p_table)(row,col++) << moment.SetValue( minDC1[index] );
            (*p_table)(row,col++) << moment.SetValue( maxDW1[index] );
            (*p_table)(row,col++) << moment.SetValue( minDW1[index] );

            (*p_table)(row,col++) << moment.SetValue( maxDC2[index] );
            (*p_table)(row,col++) << moment.SetValue( minDC2[index] );
            (*p_table)(row,col++) << moment.SetValue( maxDW2[index] );
            (*p_table)(row,col++) << moment.SetValue( minDW2[index] );
            
            (*p_table)(row,col++) << moment.SetValue( maxLL3[index] );
            (*p_table)(row,col++) << moment.SetValue( minLL3[index] );
         }
         else
         {
            (*p_table)(row,col++) << moment.SetValue( maxDC1[index] );
            (*p_table)(row,col++) << moment.SetValue( maxDW1[index] );
            (*p_table)(row,col++) << moment.SetValue( maxDC2[index] );
            (*p_table)(row,col++) << moment.SetValue( maxDW2[index] );
            (*p_table)(row,col++) << moment.SetValue( maxLL3[index] );
         }

         row++;
      }
   }

   return p_table;
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void CUserMomentsTable::MakeCopy(const CUserMomentsTable& rOther)
{
   // Add copy code here...
}

void CUserMomentsTable::MakeAssignment(const CUserMomentsTable& rOther)
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
bool CUserMomentsTable::AssertValid() const
{
   return true;
}

void CUserMomentsTable::Dump(dbgDumpContext& os) const
{
   os << _T("Dump for CUserMomentsTable") << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool CUserMomentsTable::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("CUserMomentsTable");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for CUserMomentsTable");

   TESTME_EPILOG("CUserMomentsTable");
}
#endif // _UNITTEST
