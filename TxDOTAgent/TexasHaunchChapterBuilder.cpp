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

#include <Reporting\SpanGirderReportSpecification.h>
#include <Reporting\ConstructabilityCheckTable.h>

#include "TexasHaunchChapterBuilder.h"
#include "TexasIBNSParagraphBuilder.h"
#include "TxDOTOptionalDesignUtilities.h"

#include <IFace\Tools.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Bridge.h>
#include <IFace\Artifact.h>
#include <IFace\Project.h>
#include <IFace\Intervals.h>
#include <IFace\Constructability.h>
#include <IFace/PointOfInterest.h>

#include <PgsExt\ReportPointOfInterest.h>
#include <PsgLib\StrandData.h>
#include <PgsExt\GirderArtifact.h>

#include <PsgLib\PierData2.h>
#include <psgLib\ConnectionLibraryEntry.h>
#include <psgLib/SlabOffsetCriteria.h>
#include <psgLib/SpecLibraryEntry.h>

#include <WBFLCogo.h>



static void haunch_summary(rptChapter* pChapter,std::shared_ptr<WBFL::EAF::Broker> pBroker, const std::vector<CGirderKey>& girderList,
                                  ColumnIndexType startIdx, ColumnIndexType endIdx, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits);

static void haunch_minimum_note(rptChapter* pChapter,std::shared_ptr<WBFL::EAF::Broker> pBroker, const std::vector<CGirderKey>& girderList, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits);


CTexasHaunchChapterBuilder::CTexasHaunchChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

LPCTSTR CTexasHaunchChapterBuilder::GetName() const
{
   return TEXT("TxDOT Haunch Summary");
}

rptChapter* CTexasHaunchChapterBuilder::Build(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,Uint16 level) const
{
   // This can be called for multi or single girders
   std::shared_ptr<WBFL::EAF::Broker> pBroker;
   std::vector<CGirderKey> girder_list;

   auto pGirderRptSpec = std::dynamic_pointer_cast<const CGirderReportSpecification>(pRptSpec);
   if (pGirderRptSpec!=nullptr)
   {
      pBroker = pGirderRptSpec->GetBroker();
      girder_list.push_back( pGirderRptSpec->GetGirderKey() );
   }
   else
   {
      auto pReportSpec = std::dynamic_pointer_cast<const CMultiGirderReportSpecification>(pRptSpec);
      pBroker = pReportSpec->GetBroker();

      girder_list = pReportSpec->GetGirderKeys();
   }
   ATLASSERT(!girder_list.empty());

   // don't report if no slab
   GET_IFACE2(pBroker,IBridge,pBridge);
   if (pBridge->GetDeckType() == pgsTypes::sdtNone)
   {
      return nullptr;
   }

   // Do not generate output if haunch check is disabled
   GET_IFACE2(pBroker,ILibrary, pLib );
   GET_IFACE2(pBroker,ISpecification, pSpec );
   std::_tstring spec_name = pSpec->GetSpecification();
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( spec_name.c_str() );
   const auto& slab_offset_criteria = pSpecEntry->GetSlabOffsetCriteria();
   if (!slab_offset_criteria.bCheck)
   {
      return nullptr;
   }

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);


   // Compute a list of tables to be created. Each item in the list is the number of columns for
   // a table
   std::list<ColumnIndexType> table_list = ComputeTableCols(girder_list);

   // Cycle over tables
   bool tbfirst = true;
   ColumnIndexType start_idx, end_idx;
   for (std::list<ColumnIndexType>::iterator itcol = table_list.begin(); itcol!=table_list.end(); itcol++)
   {
      if (tbfirst)
      {
         start_idx = 0;
         end_idx = *itcol-1;
         tbfirst = false;
      }
      else
      {
         start_idx = end_idx+1;
         end_idx += *itcol;
         ATLASSERT(end_idx < table_list.size());
      }

      haunch_summary( pChapter, pBroker, girder_list, start_idx, end_idx, pDisplayUnits );
   }

   // Constructability check
   CConstructabilityCheckTable().BuildSlabOffsetTable(pChapter,pBroker,girder_list,pDisplayUnits);

   // Min Haunch at bearing centerlines check
   CConstructabilityCheckTable().BuildMinimumHaunchCLCheck(pChapter,pBroker,girder_list,pDisplayUnits);

   // Fillet Check
   CConstructabilityCheckTable().BuildMinimumFilletCheck(pChapter,pBroker,girder_list,pDisplayUnits);

   // Note for minimum least haunch along girder
   haunch_minimum_note(pChapter, pBroker, girder_list, pDisplayUnits);

   // Haunch Geometry Check
   CConstructabilityCheckTable().BuildHaunchGeometryComplianceCheck(pChapter,pBroker,girder_list,pDisplayUnits);

   return pChapter;
}



void haunch_minimum_note(rptChapter* pChapter, std::shared_ptr<WBFL::EAF::Broker> pBroker, const std::vector<CGirderKey>& girderList, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits)
{
   GET_IFACE2(pBroker,IGirderHaunch,pGdrHaunch);

   INIT_UV_PROTOTYPE( rptLengthUnitValue, disp,   pDisplayUnits->GetDeflectionUnit(), true );

   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   bool is_multiple = girderList.size() > 1;

   for (const auto& girderKey : girderList)
   {
      CSegmentKey segKey(girderKey, 0); // no spliced girders here

      const auto& haunch_details = pGdrHaunch->GetSlabOffsetDetails(segKey);

      // Get min haunch along girder
      Float64 minHaunch(Float64_Max);
      for (const auto& slab_offset : haunch_details.SlabOffset)
      {

         Float64 dHaunch;
         if (slab_offset.GirderOrientationEffect < 0.0)
         {
            dHaunch = slab_offset.TopSlabToTopGirder; // cl girder in a valley
         }
         else
         {
            dHaunch = slab_offset.TopSlabToTopGirder - slab_offset.tSlab - slab_offset.GirderOrientationEffect;
         }

         minHaunch = min(minHaunch, dHaunch);
      }

      if (is_multiple)
      {
         *p << _T("Computed minimum haunch depth at edges of top flange along girder for Span ") << LABEL_SPAN(girderKey.groupIndex) << _T(" Girder ") << LABEL_GIRDER(girderKey.girderIndex) << _T(" = ") << Bold(disp.SetValue(minHaunch)) << rptNewLine;
      }
      else
      {
         *p << Bold(_T("Computed minimum haunch depth at edges of top flange along girder = ") << disp.SetValue(minHaunch)) << _T(". ");
      }

   }

   *p << _T("Refer to the Least Haunch Depth column in the Haunch Details chapter in the Details report for the location of the minimum haunch value.");
}

void haunch_summary(rptChapter* pChapter,std::shared_ptr<WBFL::EAF::Broker> pBroker, const std::vector<CGirderKey>& girderList,
                           ColumnIndexType startIdx, ColumnIndexType endIdx,
                           std::shared_ptr<IEAFDisplayUnits> pDisplayUnits)
{
   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   ColumnIndexType numCols =  endIdx-startIdx+2;

   rptRcTable* pTable = rptStyleManager::CreateTableNoHeading(numCols);
   *p << pTable;

   // Right justify columns
   for (ColumnIndexType ic=0; ic<numCols; ic++)
   {
      pTable->SetColumnStyle( ic, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_RIGHT) );
      pTable->SetStripeRowColumnStyle( ic, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_RIGHT) );
   }

   // Setup up some unit value prototypes
   static WBFL::Units::Length3Data large_volume_unit(WBFL::Units::Measure::Feet3);
   if ( pDisplayUnits->GetUnitMode() == WBFL::EAF::UnitMode::US )
      large_volume_unit.Update(WBFL::Units::Measure::Yard3,0.001,12,2,WBFL::System::NumericFormatTool::Format::Fixed);
   else
      large_volume_unit.Update(WBFL::Units::Measure::Meter3,0.001,12,2,WBFL::System::NumericFormatTool::Format::Fixed);

   INIT_UV_PROTOTYPE( rptLengthUnitValue, disp,   pDisplayUnits->GetDeflectionUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dispft, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptLength3UnitValue, volume,  large_volume_unit, false);

   // X, Y, Z rounded up to nearest 1/8"
   Float64 xyzToler = WBFL::Units::ConvertToSysUnits(0.125, WBFL::Units::Measure::Inch); 

   // Get the interfaces we need
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IGirder,pGirder);
   GET_IFACE2(pBroker,IPointOfInterest,pIPOI);
   GET_IFACE2(pBroker, ILibrary, pLib );
   GET_IFACE2(pBroker, ISpecification, pSpec );
   GET_IFACE2(pBroker,IGirderHaunch,pGdrHaunch);
   GET_IFACE2(pBroker, IProductLoads,pProductLoads);

   std::_tstring spec_name = pSpec->GetSpecification();
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( spec_name.c_str() );

   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();
   pgsTypes::BridgeAnalysisType bat = (analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan);

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType castDeckIntervalIdx      = pIntervals->GetCastDeckInterval(0); // assume deck casting region 0
   IntervalIndexType shearKeyIntervalIdx = pIntervals->GetCastShearKeyInterval();

   // PINTA here, but we need to predetermine if A's and deflections are non symmetrical so we can add extra rows to our table
   bool areAsSymm = true;
   bool areDeflsSymm = true;
   bool reportShearKey(false);
   for (ColumnIndexType gdr_idx=startIdx; gdr_idx<=endIdx; gdr_idx++)
   {
      CGirderKey girderKey(girderList[gdr_idx]);
      CSegmentKey segmentKey(girderKey, 0);

      Float64 Astart = pBridge->GetSlabOffset(segmentKey,pgsTypes::metStart);
      Float64 Aend   = pBridge->GetSlabOffset(segmentKey,pgsTypes::metEnd);
      if (!IsEqual(Astart, Aend))
      {
         areAsSymm = false;
      }

      // Get Midspan poi and take averages at 0.2, 0.3 points to compute quarter point reactions
      PoiList vPoi;
      pIPOI->GetPointsOfInterest(segmentKey, POI_2L | POI_3L | POI_7L | POI_8L | POI_SPAN, &vPoi);
      ATLASSERT(vPoi.size()==4);
      const pgsPointOfInterest& poi_2 = vPoi[0];
      const pgsPointOfInterest& poi_3 = vPoi[1];
      const pgsPointOfInterest& poi_7 = vPoi[2];
      const pgsPointOfInterest& poi_8 = vPoi[3];

      Float64 delta_slab2(0), delta_slab3(0), delta_slab7(0), delta_slab8(0);
      if (castDeckIntervalIdx != INVALID_INDEX)
      {
         GET_IFACE2(pBroker, IProductForces, pProductForces);
         delta_slab2 = pProductForces->GetDeflection(castDeckIntervalIdx, pgsTypes::pftSlab, poi_2, bat, rtCumulative, false);
         delta_slab3 = pProductForces->GetDeflection(castDeckIntervalIdx, pgsTypes::pftSlab, poi_3, bat, rtCumulative, false);
         delta_slab7 = pProductForces->GetDeflection(castDeckIntervalIdx, pgsTypes::pftSlab, poi_7, bat, rtCumulative, false);
         delta_slab8 = pProductForces->GetDeflection(castDeckIntervalIdx, pgsTypes::pftSlab, poi_8, bat, rtCumulative, false);
      }

      if (!IsEqual(delta_slab2+delta_slab3, delta_slab7+delta_slab8, 1.0e-3))
      {
         areDeflsSymm = false;
      }


      if (pProductLoads->HasShearKeyLoad(girderKey))
      {
         reportShearKey = true;
      }
   }

   Float64 val;

   // Now Build table column by column
   bool bFirst(true);
   ColumnIndexType col = 1; 
   for (ColumnIndexType gdr_idx=startIdx; gdr_idx<=endIdx; gdr_idx++)
   {
      CGirderKey girderKey(girderList[gdr_idx]);
      CSegmentKey segmentKey(girderKey, 0);

      // Get Midspan poi and take averages at 0.2, 0.3 points to compute quarter point reactions
      PoiList vPoi;
      pIPOI->GetPointsOfInterest(segmentKey, POI_TENTH_POINTS | POI_ERECTED_SEGMENT, &vPoi);
      ATLASSERT(vPoi.size()==11);
      const pgsPointOfInterest& poi_0 = vPoi[0];
      const pgsPointOfInterest& poi_2 = vPoi[2];
      const pgsPointOfInterest& poi_3 = vPoi[3];
      const pgsPointOfInterest& poi_5 = vPoi[5];
      const pgsPointOfInterest& poi_7 = vPoi[7];
      const pgsPointOfInterest& poi_8 = vPoi[8];
      const pgsPointOfInterest& poi_10 = vPoi[10];

      // X,Y,Z
      PierIndexType startPierIdx = girderKey.groupIndex; // again, assumptions of pgsuper abound here
      PierIndexType endPierIdx   = girderKey.groupIndex+1;
      Float64 Xstart = pBridge->GetSlabOffset(segmentKey,pgsTypes::metStart);
      Float64 Xend   = pBridge->GetSlabOffset(segmentKey,pgsTypes::metEnd);

      Float64 height = pGirder->GetHeight(poi_0); // assume prismatic girders

      // deflections from slab loading
      Float64 delta_slab2(0), delta_slab3(0), delta_slab5(0), delta_slab7(0), delta_slab8(0);
      if (castDeckIntervalIdx != INVALID_INDEX)
      {
         GET_IFACE2(pBroker, IProductForces, pProductForces);
         delta_slab2 = pProductForces->GetDeflection(castDeckIntervalIdx, pgsTypes::pftSlab, poi_2, bat, rtCumulative, false);
         delta_slab3 = pProductForces->GetDeflection(castDeckIntervalIdx, pgsTypes::pftSlab, poi_3, bat, rtCumulative, false);
         delta_slab5 = pProductForces->GetDeflection(castDeckIntervalIdx, pgsTypes::pftSlab, poi_5, bat, rtCumulative, false);
         delta_slab7 = pProductForces->GetDeflection(castDeckIntervalIdx, pgsTypes::pftSlab, poi_7, bat, rtCumulative, false);
         delta_slab8 = pProductForces->GetDeflection(castDeckIntervalIdx, pgsTypes::pftSlab, poi_8, bat, rtCumulative, false);
      }

      // haunch all along the girder
      const auto& slab_offset_details = pGdrHaunch->GetSlabOffsetDetails(segmentKey);

      // find Z value at mid-span
      Float64 Z(0);
      Float64 Wtop(0);
      Float64 tslab(0);
      Float64 midloc = poi_5.GetDistFromStart();
      for( const auto& slab_offset : slab_offset_details.SlabOffset)
      {
         if (slab_offset.PointOfInterest.GetDistFromStart()==midloc)
         {
            Z = slab_offset.TopSlabToTopGirder;
            Wtop = slab_offset.Wtop;
            tslab = slab_offset.tSlab;
            break;
         }
      }

      ATLASSERT(tslab != 0.0); // Z dimension not found. GetSlabOffsetDetails not returning our POI?

      // haunch volume. use simplified txdot method
      Float64 L = pBridge->GetGirderLayoutLength(girderKey); // CL Pier to CL Pier
      Float64 Xavg = (Xstart+Xend)/2.0;

      Float64 haunchVolume = L * Wtop * ((Z-tslab) + (Xavg-Z)/3.0);

      // Populate the table
      RowIndexType row = 0;
      SpanIndexType span = girderKey.groupIndex;
      GirderIndexType girder = girderKey.girderIndex;

      if (bFirst)
         (*pTable)(row,0) << Bold(_T("Span"));

      (*pTable)(row,col) <<  Bold(LABEL_SPAN(span));
      row++;

      if (bFirst)
         (*pTable)(row,0) << Bold( _T(" Girder "));
      
      (*pTable)(row,col) <<  Bold(LABEL_GIRDER(girder));
      row++;

      const WBFL::Units::LengthData& length_unit(pDisplayUnits->GetSpanLengthUnit()); // feet
      rptFormattedLengthUnitValue cmpdim(&length_unit.UnitOfMeasure,length_unit.Tol,false,true,8,true,rptFormattedLengthUnitValue::RoundOff);
      cmpdim.SetFormat(length_unit.Format);
      cmpdim.SetWidth(length_unit.Width);
      cmpdim.SetPrecision(length_unit.Precision);

      if (areAsSymm)
      {
         if (bFirst)
            (*pTable)(row,0) << _T("X");

         val = ::CeilOffTol(Xstart, xyzToler);

         (*pTable)(row,col) << cmpdim.SetValue( val );
         row++;

         if (bFirst)
            (*pTable)(row,0) << _T("Y");

         val = ::CeilOffTol(Xstart+height, xyzToler);

         (*pTable)(row,col) << cmpdim.SetValue( val );
         row++;

      }
      else
      {
         if (bFirst)
            (*pTable)(row,0) << _T("X Start");

         val = ::CeilOffTol(Xstart, xyzToler);

         (*pTable)(row,col) << cmpdim.SetValue( val );
         row++;

         if (bFirst)
            (*pTable)(row,0) << _T("X End");

         val = ::CeilOffTol(Xend, xyzToler);

         (*pTable)(row,col) << cmpdim.SetValue( val );
         row++;

         if (bFirst)
            (*pTable)(row,0) << _T("Y Start");

         val = ::CeilOffTol(Xstart+height, xyzToler);

         (*pTable)(row,col) << cmpdim.SetValue( val );
         row++;

         if (bFirst)
            (*pTable)(row,0) << _T("Y End");

         val = ::CeilOffTol(Xend+height, xyzToler);

         (*pTable)(row,col) << cmpdim.SetValue( val );
         row++;
      }

      // Z
      if (bFirst)
         (*pTable)(row,0) << _T("Z");

      val = ::CeilOffTol(Z, xyzToler);

      (*pTable)(row,col) << cmpdim.SetValue( val );
      row++;

      // slab deflections
      if (bFirst)
         (*pTable)(row,0) << _T("DL Defl Deck @ Pt A {1/4 pt} (") << dispft.GetUnitTag() << _T(") ");

      (*pTable)(row,col) << dispft.SetValue( (delta_slab2+delta_slab3)/2.0 );
      row++;

      if (bFirst)
         (*pTable)(row,0) << _T("DL Defl Deck @ Pt B {1/2 pt} (") << dispft.GetUnitTag() << _T(") ");

      (*pTable)(row,col) << dispft.SetValue( delta_slab5 );
      row++;

      if (!areDeflsSymm)
      {
         if (bFirst)
            (*pTable)(row,0) << _T("DL Defl Deck @ Pt A2 {3/4 pt} (") << dispft.GetUnitTag() << _T(") ");

         (*pTable)(row,col) << dispft.SetValue( (delta_slab7+delta_slab8)/2.0 );
         row++;
      }

      if (reportShearKey)
      {
         // deflections from shear key loading, if it exists
         GET_IFACE2(pBroker, IProductForces, pProductForces);
         Float64 delta_shearkey2 = pProductForces->GetDeflection(shearKeyIntervalIdx, pgsTypes::pftShearKey, poi_2, bat, rtCumulative, false );
         Float64 delta_shearkey3 = pProductForces->GetDeflection(shearKeyIntervalIdx, pgsTypes::pftShearKey, poi_3, bat, rtCumulative, false );
         Float64 delta_shearkey5 = pProductForces->GetDeflection(shearKeyIntervalIdx, pgsTypes::pftShearKey, poi_5, bat, rtCumulative, false );
         Float64 delta_shearkey7 = pProductForces->GetDeflection(shearKeyIntervalIdx, pgsTypes::pftShearKey, poi_7, bat, rtCumulative, false );
         Float64 delta_shearkey8 = pProductForces->GetDeflection(shearKeyIntervalIdx, pgsTypes::pftShearKey, poi_8, bat, rtCumulative, false );

         if (bFirst)
            (*pTable)(row,0) << _T("DL Defl Shear Key @ Pt A {1/4 pt} (") << dispft.GetUnitTag() << _T(") ");

         (*pTable)(row,col) << dispft.SetValue( (delta_shearkey2+delta_shearkey3)/2.0 );
         row++;

         if (bFirst)
            (*pTable)(row,0) << _T("DL Defl Shear Key @ Pt B {1/2 pt} (") << dispft.GetUnitTag() << _T(") ");

         (*pTable)(row,col) << dispft.SetValue( delta_shearkey5 );
         row++;

         if (!areDeflsSymm)
         {
            if (bFirst)
               (*pTable)(row,0) << _T("DL Defl Shear Key @ Pt A2 {3/4 pt} (") << dispft.GetUnitTag() << _T(") ");

            (*pTable)(row,col) << dispft.SetValue( (delta_shearkey7+delta_shearkey8)/2.0 );
            row++;
         }
      }

      // haunch concrete
      if (bFirst)
         (*pTable)(row,0) << _T("Haunch Concrete (") << volume.GetUnitTag() << _T(") ");

      (*pTable)(row,col) << volume.SetValue( haunchVolume );
      row++;

      bFirst = false;
      col++;
   }

   *p << _T("User-input Fillet and Slab Offset dimensions are used to define the geometry of the bottom of haunch for computing the haunch concrete volume.")<<rptNewLine;
}



//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
//======================== INQUERY    =======================================
