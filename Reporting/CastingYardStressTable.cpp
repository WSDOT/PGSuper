///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2017  Washington State Department of Transportation
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
#include <Reporting\CastingYardStressTable.h>
#include <Reporting\ReportNotes.h>

#include <PgsExt\ReportPointOfInterest.h>
#include <PgsExt\TimelineEvent.h>

#include <IFace\Project.h>
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
   CCastingYardStressTable
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CCastingYardStressTable::CCastingYardStressTable()
{
}

CCastingYardStressTable::CCastingYardStressTable(const CCastingYardStressTable& rOther)
{
   MakeCopy(rOther);
}

CCastingYardStressTable::~CCastingYardStressTable()
{
}

//======================== OPERATORS  =======================================
CCastingYardStressTable& CCastingYardStressTable::operator= (const CCastingYardStressTable& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
rptRcTable* CCastingYardStressTable::Build(IBroker* pBroker,const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,PoiAttributeType poiRefAttribute,LPCTSTR strTableTitle,
                                            IEAFDisplayUnits* pDisplayUnits) const
{
   // Build table
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), false );

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType storageIntervalIdx = pIntervals->GetStorageInterval(segmentKey);
   ATLASSERT( intervalIdx == releaseIntervalIdx || intervalIdx == storageIntervalIdx );

   location.IncludeSpanAndGirder(segmentKey.groupIndex == ALL_GROUPS ? true : false);

   rptRcTable* p_table = rptStyleManager::CreateDefaultTable(3,strTableTitle);

   if (segmentKey.groupIndex == ALL_GROUPS)
   {
      p_table->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
      p_table->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   // Set up table headings
   (*p_table)(0,0) << COLHDR(RPT_GDR_END_LOCATION,                  rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   (*p_table)(0,1) << COLHDR(RPT_FTOP << rptNewLine << _T("Girder"),    rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table)(0,2) << COLHDR(RPT_FBOT << rptNewLine << _T("Girder"),    rptStressUnitTag, pDisplayUnits->GetStressUnit() );

   // Get the interface pointers we need
   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);
   std::vector<pgsPointOfInterest> vPoi( pIPoi->GetPointsOfInterest(segmentKey,poiRefAttribute) );
   std::vector<pgsPointOfInterest> vPoi2( pIPoi->GetPointsOfInterest(segmentKey,POI_START_FACE | POI_END_FACE | POI_HARPINGPOINT | POI_PSXFER | POI_DEBOND,POIFIND_OR) );
   vPoi.insert(vPoi.end(),vPoi2.begin(),vPoi2.end());
   std::sort(vPoi.begin(),vPoi.end());
   vPoi.erase(std::unique(vPoi.begin(),vPoi.end()),vPoi.end());
   pIPoi->RemovePointsOfInterest(vPoi,POI_CLOSURE);
   pIPoi->RemovePointsOfInterest(vPoi,POI_BOUNDARY_PIER);

   GET_IFACE2(pBroker,IProductForces,pProductForces);

   pgsTypes::BridgeAnalysisType bat = pProductForces->GetBridgeAnalysisType(pgsTypes::Maximize);

   // Fill up the table
   RowIndexType row = p_table->GetNumberOfHeaderRows();
   std::vector<pgsPointOfInterest>::iterator i(vPoi.begin());
   std::vector<pgsPointOfInterest>::iterator end(vPoi.end());
   for ( ; i != end; i++ )
   {
      const pgsPointOfInterest& poi = *i;
      (*p_table)(row,0) << location.SetValue( poiRefAttribute, poi );

      Float64 fTop, fBot;
      pProductForces->GetStress(intervalIdx, pgsTypes::pftGirder, poi, bat, rtCumulative, pgsTypes::TopGirder, pgsTypes::BottomGirder, &fTop, &fBot);
      (*p_table)(row,1) << stress.SetValue( fTop );
      (*p_table)(row,2) << stress.SetValue( fBot );

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
void CCastingYardStressTable::MakeCopy(const CCastingYardStressTable& rOther)
{
   // Add copy code here...
}

void CCastingYardStressTable::MakeAssignment(const CCastingYardStressTable& rOther)
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

