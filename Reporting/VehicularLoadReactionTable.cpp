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
#include <Reporting\VehicularLoadReactionTable.h>
#include <Reporting\VehicularLoadResultsTable.h>
#include <Reporting\ReportNotes.h>

#include <IFace\Bridge.h>
#include <IFace\Intervals.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Project.h>

#include <PgsExt\PierData2.h>
#include <PgsExt\GirderGroupData.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CVehicularLoadReactionTable
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CVehicularLoadReactionTable::CVehicularLoadReactionTable()
{
}

CVehicularLoadReactionTable::CVehicularLoadReactionTable(const CVehicularLoadReactionTable& rOther)
{
   MakeCopy(rOther);
}

CVehicularLoadReactionTable::~CVehicularLoadReactionTable()
{
}

//======================== OPERATORS  =======================================
CVehicularLoadReactionTable& CVehicularLoadReactionTable::operator= (const CVehicularLoadReactionTable& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
rptRcTable* CVehicularLoadReactionTable::Build(IBroker* pBroker,const CGirderKey& girderKey,
                                               pgsTypes::LiveLoadType llType,
                                               const std::_tstring& strLLName,
                                               VehicleIndexType vehicleIndex, 
                                               pgsTypes::AnalysisType analysisType,
                                               bool bReportTruckConfig,
                                               IEAFDisplayUnits* pDisplayUnits) const

{
   // Build table
   INIT_UV_PROTOTYPE( rptLengthUnitValue, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptForceUnitValue,  reaction, pDisplayUnits->GetShearUnit(), false );
   INIT_UV_PROTOTYPE( rptAngleUnitValue,  rotation, pDisplayUnits->GetRadAngleUnit(), false );

   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IIntervals,pIntervals);

   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

   bool bPermit = false;

   PierIndexType nPiers = pBridge->GetPierCount();
   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   GroupIndexType startGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
   GroupIndexType endGroupIdx   = (girderKey.groupIndex == ALL_GROUPS ? nGroups-1 : startGroupIdx);
   PierIndexType startPierIdx = pBridge->GetGirderGroupStartPier(startGroupIdx);
   PierIndexType endPierIdx   = pBridge->GetGirderGroupEndPier(  endGroupIdx);

   ColumnIndexType nCols = 5;

   if ( bReportTruckConfig )
      nCols += 4;

   std::_tstring title(_T("Live Load Reactions and Rotations for ") + strLLName);
   rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(nCols,title.c_str());

   // Set up table headings
   ColumnIndexType col = 0;
   (*p_table)(0,col++) << _T("");
   (*p_table)(0,col++) << COLHDR(_T("Reaction") << rptNewLine << _T("Max"),   rptForceUnitTag, pDisplayUnits->GetShearUnit() );

   if ( bReportTruckConfig )
   {
      (*p_table)(0,col++) << _T("Reaction") << rptNewLine << _T("Max") << rptNewLine << _T("Config");
   }

   (*p_table)(0,col++) << COLHDR(_T("Reaction") << rptNewLine << _T("Min"),   rptForceUnitTag, pDisplayUnits->GetShearUnit() );

   if ( bReportTruckConfig )
   {
      (*p_table)(0,col++) << _T("Reaction") << rptNewLine << _T("Min") << rptNewLine << _T("Config");
   }

   (*p_table)(0,col++) << COLHDR(_T("Rotation") << rptNewLine << _T("Max"),   rptAngleUnitTag, pDisplayUnits->GetRadAngleUnit() );

   if ( bReportTruckConfig )
   {
      (*p_table)(0,col++) << _T("Rotation") << rptNewLine << _T("Max") << rptNewLine << _T("Config");
   }

   (*p_table)(0,col++) << COLHDR(_T("Rotation") << rptNewLine << _T("Min"),   rptAngleUnitTag, pDisplayUnits->GetRadAngleUnit() );


   if ( bReportTruckConfig )
   {
      (*p_table)(0,col++) << _T("Rotation") << rptNewLine << _T("Min") << rptNewLine << _T("Config");
   }

   // Get POI at start and end of the span
   GET_IFACE2(pBroker,IPointOfInterest,pPOI);
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

   GET_IFACE2(pBroker,IProductForces,pForces);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);

   RowIndexType row = p_table->GetNumberOfHeaderRows();
   for ( PierIndexType pier = startPierIdx; pier <= endPierIdx; pier++ )
   {
      CGirderKey thisGirderKey(girderKey);
      if ( girderKey.groupIndex == ALL_GROUPS )
      {
         const CPierData2* pPier = pIBridgeDesc->GetPier(pier);
         if ( pier < endPierIdx )
            thisGirderKey.groupIndex = pPier->GetNextGirderGroup()->GetIndex();
         else
            thisGirderKey.groupIndex = pPier->GetPrevGirderGroup()->GetIndex();
      }

      col = 0;
      pgsPointOfInterest& poi = vPoi[pier-startPierIdx];

      (*p_table)(row,col++) << _T("Pier ") << LABEL_PIER(pier);

      if ( analysisType == pgsTypes::Envelope )
      {
         Float64 Rmin, Rmax;
         AxleConfiguration maxConfig, minConfig;
         pForces->GetVehicularLiveLoadReaction( llType, vehicleIndex, liveLoadIntervalIdx, pier, thisGirderKey, pgsTypes::MaxSimpleContinuousEnvelope,  true, false, &Rmin, &Rmax, &minConfig, &maxConfig );
         (*p_table)(row,col++) << reaction.SetValue( Rmax );

         if ( bReportTruckConfig )
         {
            CVehicularLoadResultsTable::ReportTruckConfiguration(maxConfig,p_table,row,col++,pDisplayUnits);
         }

         pForces->GetVehicularLiveLoadReaction( llType, vehicleIndex, liveLoadIntervalIdx, pier, thisGirderKey, pgsTypes::MinSimpleContinuousEnvelope,  true, false, &Rmin, &Rmax, &minConfig, &maxConfig );
         (*p_table)(row,col++) << reaction.SetValue( Rmin );

         if ( bReportTruckConfig )
         {
            CVehicularLoadResultsTable::ReportTruckConfiguration(minConfig,p_table,row,col++,pDisplayUnits);
         }

         pForces->GetVehicularLiveLoadRotation( llType, vehicleIndex, liveLoadIntervalIdx, poi, pgsTypes::MaxSimpleContinuousEnvelope, true, false, &Rmin, &Rmax, &minConfig, &maxConfig );
         (*p_table)(row,col++) << rotation.SetValue( Rmax );

         if ( bReportTruckConfig )
         {
            CVehicularLoadResultsTable::ReportTruckConfiguration(maxConfig,p_table,row,col++,pDisplayUnits);
         }

         pForces->GetVehicularLiveLoadRotation( llType, vehicleIndex, liveLoadIntervalIdx, poi, pgsTypes::MinSimpleContinuousEnvelope, true, false, &Rmin, &Rmax, &minConfig, &maxConfig );
         (*p_table)(row,col++) << rotation.SetValue( Rmin );

         if ( bReportTruckConfig )
         {
            CVehicularLoadResultsTable::ReportTruckConfiguration(minConfig,p_table,row,col++,pDisplayUnits);
         }
      }
      else
      {
         Float64 Rmin, Rmax;
         AxleConfiguration maxConfig, minConfig;
         pForces->GetVehicularLiveLoadReaction( llType, vehicleIndex, liveLoadIntervalIdx, pier, thisGirderKey, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan,  true, false, &Rmin, &Rmax, &minConfig, &maxConfig );
         (*p_table)(row,col++) << reaction.SetValue( Rmax );

         if ( bReportTruckConfig )
         {
            CVehicularLoadResultsTable::ReportTruckConfiguration(maxConfig,p_table,row,col++,pDisplayUnits);
         }

         (*p_table)(row,col++) << reaction.SetValue( Rmin );

         if ( bReportTruckConfig )
         {
            CVehicularLoadResultsTable::ReportTruckConfiguration(minConfig,p_table,row,col++,pDisplayUnits);
         }

         pForces->GetVehicularLiveLoadRotation( llType, vehicleIndex, liveLoadIntervalIdx, poi, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, true, false, &Rmin, &Rmax, &minConfig, &maxConfig );
         (*p_table)(row,col++) << rotation.SetValue( Rmax );

         if ( bReportTruckConfig )
         {
            CVehicularLoadResultsTable::ReportTruckConfiguration(maxConfig,p_table,row,col++,pDisplayUnits);
         }

         (*p_table)(row,col++) << rotation.SetValue( Rmin );

         if ( bReportTruckConfig )
         {
            CVehicularLoadResultsTable::ReportTruckConfiguration(minConfig,p_table,row,col++,pDisplayUnits);
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
void CVehicularLoadReactionTable::MakeCopy(const CVehicularLoadReactionTable& rOther)
{
   // Add copy code here...
}

void CVehicularLoadReactionTable::MakeAssignment(const CVehicularLoadReactionTable& rOther)
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
bool CVehicularLoadReactionTable::AssertValid() const
{
   return true;
}

void CVehicularLoadReactionTable::Dump(dbgDumpContext& os) const
{
   os << _T("Dump for CVehicularLoadReactionTable") << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool CVehicularLoadReactionTable::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("CVehicularLoadReactionTable");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for CVehicularLoadReactionTable");

   TESTME_EPILOG("CVehicularLoadReactionTable");
}
#endif // _UNITTEST
