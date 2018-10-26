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
#include <Reporting\PretensionStressTable.h>
#include <Reporting\ReportNotes.h>

#include <PgsExt\ReportPointOfInterest.h>
#include <PgsExt\TimelineEvent.h>

#include <IFace\Bridge.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Project.h>
#include <IFace\Intervals.h>
#include <IFace\PrestressForce.h>

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
   INIT_UV_PROTOTYPE( rptPointOfInterest, rptReleasePoi, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptPointOfInterest, rptErectedPoi, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), true );
   INIT_UV_PROTOTYPE( rptForceUnitValue, force, pDisplayUnits->GetGeneralForceUnit(), true );

   //gdrpoi.IncludeSpanAndGirder(span == ALL_SPANS);
   //spanpoi.IncludeSpanAndGirder(span == ALL_SPANS);

   // for transformed section analysis, stresses are computed with prestress force P at the start of the interval because elastic effects are intrinsic to the stress analysis
   // for gross section analysis, stresses are computed with prestress force P at the end of the interval because the elastic effects during the interval must be included in the prestress force
   GET_IFACE2(pBroker,ISectionProperties,pSectProps);
   pgsTypes::IntervalTimeType intervalTime = (pSectProps->GetSectionPropertiesMode() == pgsTypes::spmTransformed ? pgsTypes::Start : pgsTypes::End);

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
         (*p_table)(0,col++) << _T("Interval ") << LABEL_INTERVAL(intervalIdx) << rptNewLine << pIntervals->GetDescription(intervalIdx);
      }
   }
   else
   {
      (*p_table)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
      (*p_table)(0,col++) << _T("Interval ") << LABEL_INTERVAL(loadRatingIntervalIdx) << rptNewLine << pIntervals->GetDescription(loadRatingIntervalIdx);
   }

   // Get the interface pointers we need
   GET_IFACE2(pBroker,IPointOfInterest,pPoi);
   std::vector<pgsPointOfInterest> vPoi( pPoi->GetPointsOfInterest( segmentKey, POI_SPAN ) );
   std::vector<pgsPointOfInterest> vMiscPoi( pPoi->GetPointsOfInterest(segmentKey,POI_PSXFER,POIFIND_OR) );
   vPoi.insert(vPoi.begin(),vMiscPoi.begin(),vMiscPoi.end());

   // don't want to report at these locations since the are off segment
   // and don't have stresses due to pre-tensioning.
   pPoi->RemovePointsOfInterest(vPoi,POI_CLOSURE);
   pPoi->RemovePointsOfInterest(vPoi,POI_BOUNDARY_PIER);

   std::sort(vPoi.begin(),vPoi.end());
   vPoi.erase(std::unique(vPoi.begin(),vPoi.end()),vPoi.end());

   GET_IFACE2(pBroker,IPretensionStresses,pPrestress);
   GET_IFACE2(pBroker,IPretensionForce,pForce);

   // Fill up the table
   RowIndexType row = p_table->GetNumberOfHeaderRows();

   BOOST_FOREACH(const pgsPointOfInterest& poi,vPoi)
   {
      col = 0;

      if ( bDesign )
      {
         (*p_table)(row,col++) << rptReleasePoi.SetValue( POI_RELEASED_SEGMENT, poi );
      }
      (*p_table)(row,col++) << rptErectedPoi.SetValue( POI_SPAN, poi  );

      if ( bDesign )
      {
         std::vector<IntervalIndexType>::iterator iter(vIntervals.begin());
         std::vector<IntervalIndexType>::iterator end(vIntervals.end());
         for ( ; iter != end; iter++ )
         {
            IntervalIndexType intervalIdx = *iter;

            Float64 Fp = pForce->GetPrestressForce(poi,pgsTypes::Permanent,intervalIdx,intervalTime);
            Float64 Ft = pForce->GetPrestressForce(poi,pgsTypes::Temporary,intervalIdx,intervalTime);
            (*p_table)(row,col) << _T("P (permanent) = ") << force.SetValue(Fp) << rptNewLine;
            (*p_table)(row,col) << _T("P (temporary) = ") << force.SetValue(Ft) << rptNewLine;

            Float64 fTop = pPrestress->GetStress(intervalIdx,poi,pgsTypes::TopGirder,true/*include live load if applicable*/);
            Float64 fBot = pPrestress->GetStress(intervalIdx,poi,pgsTypes::BottomGirder,true/*include live load if applicable*/);
            (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue( fTop ) << rptNewLine;
            (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue( fBot );
            col++;
         }
      }
      else
      {
         // Rating
         Float64 Fp = pForce->GetPrestressForce(poi,pgsTypes::Permanent,loadRatingIntervalIdx,intervalTime);
         (*p_table)(row,col) << _T("P (permanent) = ") << force.SetValue(Fp) << rptNewLine;

         Float64 fTop = pPrestress->GetStress(loadRatingIntervalIdx,poi,pgsTypes::TopGirder,true/*include live load if applicable*/);
         Float64 fBot = pPrestress->GetStress(loadRatingIntervalIdx,poi,pgsTypes::BottomGirder,true/*include live load if applicable*/);
         (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue( fTop ) << rptNewLine;
         (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue( fBot );
         col++;
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

