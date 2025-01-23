///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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
#include <Reporting\PosttensionStressTable.h>
#include <Reporting\ReportNotes.h>

#include <PgsExt\ReportPointOfInterest.h>
#include <PgsExt\TimelineEvent.h>

#include <IFace\Bridge.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Project.h>
#include <IFace\Intervals.h>
#include <IFace\Limits.h>
#include <IFace\ReportOptions.h>

rptRcTable* CPosttensionStressTable::Build(IBroker* pBroker,const CGirderKey& girderKey,
                                            bool bDesign,IEAFDisplayUnits* pDisplayUnits,bool bGirderStresses) const
{
   pgsTypes::StressLocation topLocation = (bGirderStresses ? pgsTypes::TopGirder    : pgsTypes::TopDeck);
   pgsTypes::StressLocation botLocation = (bGirderStresses ? pgsTypes::BottomGirder : pgsTypes::BottomDeck);

   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), false );

   GET_IFACE2(pBroker,IReportOptions,pReportOptions);
   location.IncludeSpanAndGirder(pReportOptions->IncludeSpanAndGirder4Pois(girderKey));

   GET_IFACE2(pBroker, IStressCheck, pStressCheck);
   std::vector<IntervalIndexType> vIntervals(pStressCheck->GetStressCheckIntervals(girderKey));

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType lastCompositeDeckIntervalIdx = pIntervals->GetLastCompositeDeckInterval();
   IntervalIndexType loadRatingIntervalIdx           = pIntervals->GetLoadRatingInterval();
   IntervalIndexType firstTendonStressingIntervalIdx = pIntervals->GetFirstGirderTendonStressingInterval(girderKey);

   ATLASSERT(firstTendonStressingIntervalIdx != INVALID_INDEX);

   // we only want to report stresses due to PT in spec check intervals after the first tendon
   // is stressed. determine the first interval to report and remove all intervals from
   // vIntervals that are earlier
   IntervalIndexType minIntervalIdx = firstTendonStressingIntervalIdx;
   if ( !bGirderStresses )
   {
      // if we are reporting stresses in the deck, don't report any intervals before
      // the deck is composite
      minIntervalIdx = Max(minIntervalIdx,lastCompositeDeckIntervalIdx);
   }

   // remove the intervals
   vIntervals.erase(std::remove_if(vIntervals.begin(), vIntervals.end(), [&minIntervalIdx](const auto& intervalIdx) {return intervalIdx < minIntervalIdx;}), vIntervals.end());
   ATLASSERT(0 < vIntervals.size());

   ColumnIndexType nColumns;
   if ( bDesign )
   {
      nColumns = 1 // location column
               + vIntervals.size(); // one for each interval
   }
   else
   {
      // Load Rating
      nColumns = 2; // location column and column for live load stage
   }

   std::_tstring strTitle(bGirderStresses ? _T("Girder Stresses") : _T("Deck Stresses"));
   rptRcTable* p_table = rptStyleManager::CreateDefaultTable(nColumns,strTitle);

   if ( girderKey.groupIndex == ALL_GROUPS )
   {
      p_table->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
      p_table->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

      if ( bDesign )
      {
         p_table->SetColumnStyle(1,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
         p_table->SetStripeRowColumnStyle(1,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
      }
   }

   // Set up table headings
   ColumnIndexType col = 0;
   if ( bDesign )
   {
      (*p_table)(0,col++) << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );

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
   PoiList vPoi;
   pIPoi->GetPointsOfInterest(CSegmentKey(girderKey, ALL_SEGMENTS), POI_SPAN, &vPoi);

   GET_IFACE2(pBroker,IProductForces,pProductForces);
   pgsTypes::BridgeAnalysisType bat = pProductForces->GetBridgeAnalysisType(pgsTypes::Maximize);

   // Fill up the table
   RowIndexType row = p_table->GetNumberOfHeaderRows();

   for (const pgsPointOfInterest& poi : vPoi)
   {
      col = 0;

      (*p_table)(row,col++) << location.SetValue( POI_SPAN, poi );

      Float64 fTop, fBot;
      if ( bDesign )
      {
         for ( const auto& intervalIdx : vIntervals)
         {
            pProductForces->GetStress(intervalIdx,pgsTypes::pftPostTensioning,poi,bat,rtCumulative,topLocation,botLocation,&fTop,&fBot);

            (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue( fTop ) << rptNewLine;
            (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue( fBot );
            col++;
         }
      }
      else
      {
         // Rating
         pProductForces->GetStress(loadRatingIntervalIdx,pgsTypes::pftPostTensioning,poi,bat,rtCumulative,topLocation,botLocation,&fTop,&fBot);
         (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue( fTop ) << rptNewLine;
         (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue( fBot );
         col++;
      }

      row++;
   }

   return p_table;
}
