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
#include <Reporting\UserRotationTable.h>
#include <Reporting\UserMomentsTable.h>
#include <Reporting\ReactionInterfaceAdapters.h>

#include <IFace\Bridge.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Intervals.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CUserRotationTable
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CUserRotationTable::CUserRotationTable()
{
}

CUserRotationTable::CUserRotationTable(const CUserRotationTable& rOther)
{
   MakeCopy(rOther);
}

CUserRotationTable::~CUserRotationTable()
{
}

//======================== OPERATORS  =======================================
CUserRotationTable& CUserRotationTable::operator= (const CUserRotationTable& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
rptRcTable* CUserRotationTable::Build(IBroker* pBroker,const CGirderKey& girderKey,pgsTypes::AnalysisType analysisType,
                                      IEAFDisplayUnits* pDisplayUnits) const
{
   // Build table
   INIT_UV_PROTOTYPE( rptLengthUnitValue, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptAngleUnitValue, rotation, pDisplayUnits->GetRadAngleUnit(), false );

   rptRcTable* p_table = CreateUserLoadHeading<rptAngleUnitTag,unitmgtAngleData>(_T("Rotations - User Defined Loads"),true,analysisType,pDisplayUnits,pDisplayUnits->GetRadAngleUnit());

   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IProductForces,pForces);
   GET_IFACE2(pBroker,IPointOfInterest,pPOI);
   GET_IFACE2(pBroker,IBearingDesign,pBearingDesign);

   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   GroupIndexType startGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
   GroupIndexType endGroupIdx   = (girderKey.groupIndex == ALL_GROUPS ? nGroups-1 : startGroupIdx);

   PierIndexType startPierIdx = pBridge->GetGirderGroupStartPier(startGroupIdx);

   GET_IFACE2(pBroker,IProductForces,pProdForces);
   pgsTypes::BridgeAnalysisType maxBAT = pProdForces->GetBridgeAnalysisType(analysisType,pgsTypes::Maximize);
   pgsTypes::BridgeAnalysisType minBAT = pProdForces->GetBridgeAnalysisType(analysisType,pgsTypes::Minimize);

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType castDeckIntervalIdx      = pIntervals->GetCastDeckInterval();
   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval();
   IntervalIndexType railingSystemIntervalIdx = pIntervals->GetInstallRailingSystemInterval();
   IntervalIndexType liveLoadIntervalIdx      = pIntervals->GetLiveLoadInterval();

   // get poi at start and end of each segment in the girder
   std::vector<pgsPointOfInterest> vPoi;
   for ( GroupIndexType grpIdx = startGroupIdx; grpIdx <= endGroupIdx; grpIdx++ )
   {
      SegmentIndexType nSegments = pBridge->GetSegmentCount(CGirderKey(grpIdx,girderKey.girderIndex));
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         CSegmentKey segmentKey(grpIdx,girderKey.girderIndex,segIdx);
         std::vector<pgsPointOfInterest> segPoi1(pPOI->GetPointsOfInterest(segmentKey,POI_ERECTED_SEGMENT | POI_0L, POIFIND_AND));
         std::vector<pgsPointOfInterest> segPoi2(pPOI->GetPointsOfInterest(segmentKey,POI_ERECTED_SEGMENT | POI_10L,POIFIND_AND));
         ATLASSERT(segPoi1.size() == 1);
         ATLASSERT(segPoi2.size() == 1);
         vPoi.push_back(segPoi1.front());
         vPoi.push_back(segPoi2.front());
      }
   }

   // TRICKY: use adapter class to get correct reaction interfaces
   std::auto_ptr<IProductReactionAdapter> pForcesAdapt =  std::auto_ptr<BearingDesignProductReactionAdapter>(new BearingDesignProductReactionAdapter(pBearingDesign, compositeDeckIntervalIdx, girderKey) );

   // User iterator to walk locations
   ReactionLocationIter iter = pForcesAdapt->GetReactionLocations(pBridge);

   RowIndexType row = p_table->GetNumberOfHeaderRows();
   for (iter.First(); !iter.IsDone(); iter.Next())
   {
      ColumnIndexType col = 0;

      const ReactionLocation& reactionLocation( iter.CurrentItem() );

      (*p_table)(row,col++) << reactionLocation.PierLabel;

      pgsPointOfInterest& poi = vPoi[reactionLocation.PierIdx-startPierIdx];
      IntervalIndexType erectSegmentIntervalIdx = pIntervals->GetErectSegmentInterval(poi.GetSegmentKey());

      // Use reaction decider tool to determine when to report stages
      ReactionDecider rctdr(BearingReactionsTable, reactionLocation, pBridge, pIntervals);


      if ( analysisType == pgsTypes::Envelope )
      {
         if (rctdr.DoReport(castDeckIntervalIdx))
         {
            (*p_table)(row,col++) << rotation.SetValue( pForces->GetRotation( castDeckIntervalIdx, pftUserDC, poi, maxBAT ) );
            (*p_table)(row,col++) << rotation.SetValue( pForces->GetRotation( castDeckIntervalIdx, pftUserDC, poi, minBAT ) );
            (*p_table)(row,col++) << rotation.SetValue( pForces->GetRotation( castDeckIntervalIdx, pftUserDW, poi, maxBAT ) );
            (*p_table)(row,col++) << rotation.SetValue( pForces->GetRotation( castDeckIntervalIdx, pftUserDW, poi, minBAT ) );
         }
         else
         {
            (*p_table)(row,col++) << RPT_NA;
            (*p_table)(row,col++) << RPT_NA;
            (*p_table)(row,col++) << RPT_NA;
            (*p_table)(row,col++) << RPT_NA;
         }

         if (rctdr.DoReport(railingSystemIntervalIdx))
         {
            (*p_table)(row,col++) << rotation.SetValue( pForces->GetRotation( railingSystemIntervalIdx, pftUserDC,    poi, maxBAT ) );
            (*p_table)(row,col++) << rotation.SetValue( pForces->GetRotation( railingSystemIntervalIdx, pftUserDC,    poi, minBAT ) );
            (*p_table)(row,col++) << rotation.SetValue( pForces->GetRotation( railingSystemIntervalIdx, pftUserDW,    poi, maxBAT ) );
            (*p_table)(row,col++) << rotation.SetValue( pForces->GetRotation( railingSystemIntervalIdx, pftUserDW,    poi, minBAT ) );
         }
         else
         {
            (*p_table)(row,col++) << RPT_NA;
            (*p_table)(row,col++) << RPT_NA;
            (*p_table)(row,col++) << RPT_NA;
            (*p_table)(row,col++) << RPT_NA;
         }
         
         if (rctdr.DoReport(liveLoadIntervalIdx))
         {
            (*p_table)(row,col++) << rotation.SetValue( pForces->GetRotation( liveLoadIntervalIdx, pftUserLLIM, poi, maxBAT ) );
            (*p_table)(row,col++) << rotation.SetValue( pForces->GetRotation( liveLoadIntervalIdx, pftUserLLIM, poi, minBAT ) );
         }
         else
         {
            (*p_table)(row,col++) << RPT_NA;
            (*p_table)(row,col++) << RPT_NA;
         }
      }
      else
      {
         if (rctdr.DoReport(castDeckIntervalIdx))
         {
            (*p_table)(row,col++) << rotation.SetValue( pForces->GetRotation( castDeckIntervalIdx, pftUserDC, poi, maxBAT ) );
            (*p_table)(row,col++) << rotation.SetValue( pForces->GetRotation( castDeckIntervalIdx, pftUserDW, poi, maxBAT ) );
         }
         else
         {
            (*p_table)(row,col++) << RPT_NA;
            (*p_table)(row,col++) << RPT_NA;
         }

         if (rctdr.DoReport(railingSystemIntervalIdx))
         {
            (*p_table)(row,col++) << rotation.SetValue( pForces->GetRotation( railingSystemIntervalIdx, pftUserDC, poi, maxBAT ) );
            (*p_table)(row,col++) << rotation.SetValue( pForces->GetRotation( railingSystemIntervalIdx, pftUserDW, poi, maxBAT ) );
         }
         else
         {
            (*p_table)(row,col++) << RPT_NA;
            (*p_table)(row,col++) << RPT_NA;
         }

         if (rctdr.DoReport(liveLoadIntervalIdx))
         {
            (*p_table)(row,col++) << rotation.SetValue( pForces->GetRotation( liveLoadIntervalIdx, pftUserLLIM, poi, maxBAT ) );
         }
         else
         {
            (*p_table)(row,col++) << RPT_NA;
         }
      }

      row++;
   }

   return p_table;
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void CUserRotationTable::MakeCopy(const CUserRotationTable& rOther)
{
   // Add copy code here...
}

void CUserRotationTable::MakeAssignment(const CUserRotationTable& rOther)
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
bool CUserRotationTable::AssertValid() const
{
   return true;
}

void CUserRotationTable::Dump(dbgDumpContext& os) const
{
   os << _T("Dump for CUserRotationTable") << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool CUserRotationTable::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("CUserRotationTable");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for CUserRotationTable");

   TESTME_EPILOG("CUserRotationTable");
}
#endif // _UNITTEST
