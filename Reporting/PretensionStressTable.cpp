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
#include <Reporting\PretensionStressTable.h>
#include <Reporting\ReportNotes.h>

#include <PgsExt\GirderPointOfInterest.h>
#include <PgsExt\TimelineEvent.h>

#include <IFace\Bridge.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Project.h>
#include <IFace\Intervals.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CPretensionStressTable
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CPretensionStressTable::CPretensionStressTable()
{
}

CPretensionStressTable::CPretensionStressTable(const CPretensionStressTable& rOther)
{
   MakeCopy(rOther);
}

CPretensionStressTable::~CPretensionStressTable()
{
}

//======================== OPERATORS  =======================================
CPretensionStressTable& CPretensionStressTable::operator= (const CPretensionStressTable& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
rptRcTable* CPretensionStressTable::Build(IBroker* pBroker,const CSegmentKey& segmentKey,
                                            bool bDesign,IEAFDisplayUnits* pDisplayUnits) const
{
   // Build table
   INIT_UV_PROTOTYPE( rptPointOfInterest, gdrpoi, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptPointOfInterest, spanpoi, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), false );

   //gdrpoi.IncludeSpanAndGirder(span == ALL_SPANS);
   //spanpoi.IncludeSpanAndGirder(span == ALL_SPANS);

   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   std::vector<IntervalIndexType> vIntervals(pIntervals->GetSpecCheckIntervals(segmentKey));
   IntervalIndexType nIntervals = vIntervals.size();
   IntervalIndexType loadRatingIntervalIdx = pIntervals->GetLoadRatingInterval();

   ColumnIndexType nColumns;
   if ( bDesign )
   {
      nColumns = 2 // two location columns
               + nIntervals; // one for each interval
   }
   else
   {
      // Load Rating
      nColumns = 2; // location column and column for live load stage
   }

   rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(nColumns,_T("Girder Stresses"));

   if ( segmentKey.groupIndex == ALL_GROUPS )
   {
      p_table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      p_table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

      if ( bDesign )
      {
         p_table->SetColumnStyle(1,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
         p_table->SetStripeRowColumnStyle(1,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
      }
   }

   // Set up table headings
   ColumnIndexType col = 0;
   if ( bDesign )
   {
      (*p_table)(0,col++) << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
      (*p_table)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );

      std::vector<IntervalIndexType>::iterator iter(vIntervals.begin());
      std::vector<IntervalIndexType>::iterator end(vIntervals.end());
      for ( ; iter != end; iter++ )
      {
         IntervalIndexType intervalIdx = *iter;
         (*p_table)(0,col++) << COLHDR(_T("Interval ") << LABEL_INTERVAL(intervalIdx) << rptNewLine << pIntervals->GetDescription(intervalIdx), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      }
   }
   else
   {
      (*p_table)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
      (*p_table)(0,col++) << COLHDR(_T("Interval ") << LABEL_INTERVAL(loadRatingIntervalIdx) << rptNewLine << pIntervals->GetDescription(loadRatingIntervalIdx), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   }

   // Get the interface pointers we need
   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);
   std::vector<pgsPointOfInterest> vPoi( pIPoi->GetPointsOfInterest( segmentKey ) );

   // don't want to report at these locations since the are off segment
   // and don't have stresses due to pre-tensioning.
   pIPoi->RemovePointsOfInterest(vPoi,POI_CLOSURE);
   pIPoi->RemovePointsOfInterest(vPoi,POI_BOUNDARY_PIER);

   GET_IFACE2(pBroker,IPretensionStresses,pPrestress);

   // Fill up the table
   pgsPointOfInterest prev_poi(CSegmentKey(0,0,0),0.0);
   bool bSkipToNextRow = false;

   RowIndexType row = p_table->GetNumberOfHeaderRows();

   std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
   for ( ; iter != end; iter++ )
   {
      bSkipToNextRow = false;
      col = 0;

      const pgsPointOfInterest& poi = *iter;
      const CSegmentKey& thisSegmentKey = poi.GetSegmentKey();

      if ( row != 1 && IsEqual(poi.GetDistFromStart(),prev_poi.GetDistFromStart()) )
      {
         bSkipToNextRow = true;
         row--;
      }

      Float64 end_size = pBridge->GetSegmentStartEndDistance(thisSegmentKey);
      (*p_table)(row,col++) << gdrpoi.SetValue( POI_RELEASED_SEGMENT, poi );
      (*p_table)(row,col++) << spanpoi.SetValue( POI_ERECTED_SEGMENT, poi, end_size  );

      if ( !bSkipToNextRow )
      {
         Float64 fTop, fBot;
         if ( bDesign )
         {
            std::vector<IntervalIndexType>::iterator iter(vIntervals.begin());
            std::vector<IntervalIndexType>::iterator end(vIntervals.end());
            for ( ; iter != end; iter++ )
            {
               IntervalIndexType intervalIdx = *iter;

               fTop = pPrestress->GetStress(intervalIdx,poi,pgsTypes::TopGirder);
               fBot = pPrestress->GetStress(intervalIdx,poi,pgsTypes::BottomGirder);
               (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue( fTop ) << rptNewLine;
               (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue( fBot );
               col++;
            }
         }
         else
         {
            // Rating
            fTop = pPrestress->GetStress(loadRatingIntervalIdx,poi,pgsTypes::TopGirder);
            fBot = pPrestress->GetStress(loadRatingIntervalIdx,poi,pgsTypes::BottomGirder);
            (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue( fTop ) << rptNewLine;
            (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue( fBot );
            col++;
         }
      }

      row++;
      prev_poi = poi;
   }

   return p_table;
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void CPretensionStressTable::MakeCopy(const CPretensionStressTable& rOther)
{
   // Add copy code here...
}

void CPretensionStressTable::MakeAssignment(const CPretensionStressTable& rOther)
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

