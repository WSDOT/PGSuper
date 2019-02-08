///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2019  Washington State Department of Transportation
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
rptRcTable* CUserRotationTable::Build(IBroker* pBroker,const CGirderKey& girderKey,pgsTypes::AnalysisType analysisType,IntervalIndexType intervalIdx,
                                      IEAFDisplayUnits* pDisplayUnits) const
{
   // Build table
   INIT_UV_PROTOTYPE( rptAngleUnitValue, rotation, pDisplayUnits->GetRadAngleUnit(), false );

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval();

   CString strTitle;
   strTitle.Format(_T("Rotations due to User Defined Loads in Interval %d: %s"),LABEL_INTERVAL(intervalIdx),pIntervals->GetDescription(intervalIdx));
   rptRcTable* p_table = CreateUserLoadHeading<rptAngleUnitTag,unitmgtAngleData>(strTitle.GetBuffer(),true,analysisType,intervalIdx,pDisplayUnits,pDisplayUnits->GetRadAngleUnit());

   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IProductForces,pProdForces);
   GET_IFACE2(pBroker,IPointOfInterest,pPOI);
   GET_IFACE2(pBroker,IBearingDesign,pBearingDesign);

   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   GroupIndexType startGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
   GroupIndexType endGroupIdx   = (girderKey.groupIndex == ALL_GROUPS ? nGroups-1 : startGroupIdx);

   PierIndexType startPierIdx = pBridge->GetGirderGroupStartPier(startGroupIdx);

   pgsTypes::BridgeAnalysisType maxBAT = pProdForces->GetBridgeAnalysisType(analysisType,pgsTypes::Maximize);
   pgsTypes::BridgeAnalysisType minBAT = pProdForces->GetBridgeAnalysisType(analysisType,pgsTypes::Minimize);

   // get poi at start and end of each segment in the girder
   PoiList vPoi;
   for ( GroupIndexType grpIdx = startGroupIdx; grpIdx <= endGroupIdx; grpIdx++ )
   {
      GirderIndexType gdrIdx = min( girderKey.girderIndex, pBridge->GetGirderCount(grpIdx)-1 );

      // don't report giders that don't exist on bridge
      SegmentIndexType nSegments = pBridge->GetSegmentCount(CGirderKey(grpIdx,gdrIdx));
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         CSegmentKey segmentKey(grpIdx,gdrIdx,segIdx);
         PoiList vSegPoi;
         pPOI->GetPointsOfInterest(segmentKey, POI_0L | POI_10L | POI_ERECTED_SEGMENT, &vSegPoi);
         ATLASSERT(vSegPoi.size() == 2);
         vPoi.insert(vPoi.end(), vSegPoi.begin(), vSegPoi.end());
      }
   }

   // TRICKY: use adapter class to get correct reaction interfaces
   std::unique_ptr<IProductReactionAdapter> pForcesAdapt(std::make_unique<BearingDesignProductReactionAdapter>(pBearingDesign, compositeDeckIntervalIdx, girderKey) );

   // User iterator to walk locations
   ReactionLocationIter iter = pForcesAdapt->GetReactionLocations(pBridge);

   RowIndexType row = p_table->GetNumberOfHeaderRows();
   for (iter.First(); !iter.IsDone(); iter.Next())
   {
      ColumnIndexType col = 0;

      const ReactionLocation& reactionLocation( iter.CurrentItem() );

      (*p_table)(row,col++) << reactionLocation.PierLabel;

      const CGirderKey& thisGirderKey(reactionLocation.GirderKey);

      IntervalIndexType castDeckIntervalIdx      = pIntervals->GetCastDeckInterval();
      IntervalIndexType railingSystemIntervalIdx = pIntervals->GetInstallRailingSystemInterval();
      IntervalIndexType liveLoadIntervalIdx      = pIntervals->GetLiveLoadInterval();
      IntervalIndexType loadRatingIntervalIdx    = pIntervals->GetLoadRatingInterval();
      IntervalIndexType overlayIntervalIdx       = pIntervals->GetOverlayInterval();

      const pgsPointOfInterest& poi = vPoi[reactionLocation.PierIdx-startPierIdx];
      IntervalIndexType erectSegmentIntervalIdx = pIntervals->GetErectSegmentInterval(poi.GetSegmentKey());

      // Use reaction decider tool to determine when to report stages
      ReactionDecider reactionDecider(BearingReactionsTable, reactionLocation, thisGirderKey, pBridge, pIntervals);


      if ( analysisType == pgsTypes::Envelope )
      {
         if (reactionDecider.DoReport(intervalIdx))
         {
            (*p_table)(row,col++) << rotation.SetValue( pProdForces->GetRotation( intervalIdx, pgsTypes::pftUserDC,   poi, maxBAT, rtCumulative, false ) );
            (*p_table)(row,col++) << rotation.SetValue( pProdForces->GetRotation( intervalIdx, pgsTypes::pftUserDC,   poi, minBAT, rtCumulative, false ) );
            (*p_table)(row,col++) << rotation.SetValue( pProdForces->GetRotation( intervalIdx, pgsTypes::pftUserDW,   poi, maxBAT, rtCumulative, false ) );
            (*p_table)(row,col++) << rotation.SetValue( pProdForces->GetRotation( intervalIdx, pgsTypes::pftUserDW,   poi, minBAT, rtCumulative, false ) );
            (*p_table)(row,col++) << rotation.SetValue( pProdForces->GetRotation( intervalIdx, pgsTypes::pftUserLLIM, poi, maxBAT, rtCumulative, false ) );
            (*p_table)(row,col++) << rotation.SetValue( pProdForces->GetRotation( intervalIdx, pgsTypes::pftUserLLIM, poi, minBAT, rtCumulative, false ) );
         }
         else
         {
            (*p_table)(row,col++) << RPT_NA;
            (*p_table)(row,col++) << RPT_NA;
            (*p_table)(row,col++) << RPT_NA;
            (*p_table)(row,col++) << RPT_NA;
            (*p_table)(row,col++) << RPT_NA;
            (*p_table)(row,col++) << RPT_NA;
         }
      }
      else
      {
         if (reactionDecider.DoReport(intervalIdx))
         {
            (*p_table)(row,col++) << rotation.SetValue( pProdForces->GetRotation( intervalIdx, pgsTypes::pftUserDC,   poi, maxBAT, rtCumulative, false ) );
            (*p_table)(row,col++) << rotation.SetValue( pProdForces->GetRotation( intervalIdx, pgsTypes::pftUserDW,   poi, maxBAT, rtCumulative, false ) );
            (*p_table)(row,col++) << rotation.SetValue( pProdForces->GetRotation( intervalIdx, pgsTypes::pftUserLLIM, poi, maxBAT, rtCumulative, false ) );
         }
         else
         {
            (*p_table)(row,col++) << RPT_NA;
            (*p_table)(row,col++) << RPT_NA;
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
