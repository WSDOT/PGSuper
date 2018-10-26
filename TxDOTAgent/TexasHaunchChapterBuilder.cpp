///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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
#include <PgsExt\ReportStyleHolder.h>
#include <Reporting\SpanGirderReportSpecification.h>
#include <Reporting\ConstructabilityCheckTable.h>

#include "TexasHaunchChapterBuilder.h"
#include "TexasIBNSParagraphBuilder.h"
#include "TxDOTOptionalDesignUtilities.h"

#include <EAF\EAFDisplayUnits.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Bridge.h>
#include <IFace\Artifact.h>
#include <IFace\Project.h>
#include <IFace\Intervals.h>
#include <IFace\Constructability.h>

#include <PgsExt\ReportPointOfInterest.h>
#include <PgsExt\StrandData.h>
#include <PgsExt\GirderArtifact.h>
#include <PgsExt\PierData2.h>

#include <psgLib\ConnectionLibraryEntry.h>

#include <WBFLCogo.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CTexasHaunchChapterBuilder
****************************************************************************/


static void haunch_summary(rptChapter* pChapter,IBroker* pBroker, const std::vector<CGirderKey>& girderList,
                                  ColumnIndexType startIdx, ColumnIndexType endIdx, IEAFDisplayUnits* pDisplayUnits);

////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CTexasHaunchChapterBuilder::CTexasHaunchChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CTexasHaunchChapterBuilder::GetName() const
{
   return TEXT("TxDOT Haunch Summary");
}

rptChapter* CTexasHaunchChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   // This can be called for multi or single girders
   std::vector<CGirderKey> girder_list;

   CComPtr<IBroker> pBroker;

   CGirderReportSpecification* pGirderRptSpec = dynamic_cast<CGirderReportSpecification*>(pRptSpec);
   if (pGirderRptSpec!=NULL)
   {
      pGirderRptSpec->GetBroker(&pBroker);
      girder_list.push_back( pGirderRptSpec->GetGirderKey() );
   }
   else
   {
      CMultiGirderReportSpecification* pReportSpec = dynamic_cast<CMultiGirderReportSpecification*>(pRptSpec);
      pReportSpec->GetBroker(&pBroker);

      girder_list = pReportSpec->GetGirderKeys();
   }
   ATLASSERT(!girder_list.empty());

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

   return pChapter;
}

CChapterBuilder* CTexasHaunchChapterBuilder::Clone() const
{
   return new CTexasHaunchChapterBuilder;
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
void haunch_summary(rptChapter* pChapter,IBroker* pBroker, const std::vector<CGirderKey>& girderList,
                           ColumnIndexType startIdx, ColumnIndexType endIdx,
                           IEAFDisplayUnits* pDisplayUnits)
{
   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   ColumnIndexType numCols =  endIdx-startIdx+2;

   rptRcTable* pTable = pgsReportStyleHolder::CreateTableNoHeading(numCols);
   *p << pTable;

   // Right justify columns
   for (ColumnIndexType ic=0; ic<numCols; ic++)
   {
      pTable->SetColumnStyle( ic, pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_RIGHT) );
      pTable->SetStripeRowColumnStyle( ic, pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_RIGHT) );
   }

   // Setup up some unit value prototypes
   static unitmgtLength3Data large_volume_unit(unitMeasure::Feet3);
   if ( pDisplayUnits->GetUnitMode() == eafTypes::umUS )
      large_volume_unit.Update(unitMeasure::Yard3,0.001,12,2,sysNumericFormatTool::Fixed);
   else
      large_volume_unit.Update(unitMeasure::Meter3,0.001,12,2,sysNumericFormatTool::Fixed);

   INIT_UV_PROTOTYPE( rptLengthUnitValue, disp,   pDisplayUnits->GetDeflectionUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dispft, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptLength3UnitValue, volume,  large_volume_unit, false);

   // Get the interfaces we need
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IGirder,pGirder);
   GET_IFACE2(pBroker,IPointOfInterest,pIPOI);
   GET_IFACE2(pBroker,IProductForces, pProductForces);
   GET_IFACE2( pBroker, ILibrary, pLib );
   GET_IFACE2( pBroker, ISpecification, pSpec );
   GET_IFACE2(pBroker,IGirderHaunch,pGdrHaunch);
   GET_IFACE2(pBroker,IBearingDesign,pBearingDesign);
   GET_IFACE2(pBroker,IMaterials,pMaterials);

   std::_tstring spec_name = pSpec->GetSpecification();
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( spec_name.c_str() );

   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();
   pgsTypes::BridgeAnalysisType bat = (analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan);

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType castDeckIntervalIdx      = pIntervals->GetCastDeckInterval();

   // PINTA here, but we need to predetermine if A's and deflections are non symmetrical so we can add extra rows to our table
   bool areAsSymm = true;
   bool areDeflsSymm = true;
   for (ColumnIndexType gdr_idx=startIdx; gdr_idx<=endIdx; gdr_idx++)
   {
      CGirderKey girderKey(girderList[gdr_idx]);

      Float64 Astart = pBridge->GetSlabOffset(girderKey.groupIndex, girderKey.groupIndex,   girderKey.girderIndex);
      Float64 Aend   = pBridge->GetSlabOffset(girderKey.groupIndex, girderKey.groupIndex+1, girderKey.girderIndex);
      if (!IsEqual(Astart, Aend))
      {
         areAsSymm = false;
      }

      // Get Midspan poi and take averages at 0.2, 0.3 points to compute quarter point reactions
      std::vector<pgsPointOfInterest> vPoi = pIPOI->GetPointsOfInterest(CSegmentKey(girderKey,0),POI_2L |POI_3L |POI_7L |POI_8L | POI_ERECTED_SEGMENT);
      ATLASSERT(vPoi.size()==4);
      const pgsPointOfInterest& poi_2 = vPoi[0];
      const pgsPointOfInterest& poi_3 = vPoi[1];
      const pgsPointOfInterest& poi_7 = vPoi[2];
      const pgsPointOfInterest& poi_8 = vPoi[3];

      Float64 delta_slab2 = pProductForces->GetDeflection(castDeckIntervalIdx, pgsTypes::pftSlab, poi_2, bat, rtCumulative, false );
      Float64 delta_slab3 = pProductForces->GetDeflection(castDeckIntervalIdx, pgsTypes::pftSlab, poi_3, bat, rtCumulative, false );
      Float64 delta_slab7 = pProductForces->GetDeflection(castDeckIntervalIdx, pgsTypes::pftSlab, poi_7, bat, rtCumulative, false );
      Float64 delta_slab8 = pProductForces->GetDeflection(castDeckIntervalIdx, pgsTypes::pftSlab, poi_8, bat, rtCumulative, false );

      if (!IsEqual(delta_slab2+delta_slab3, delta_slab7+delta_slab8, 1.0e-3))
      {
         areDeflsSymm = false;
      }
   }

   // Now Build table column by column
   bool bFirst(true);
   ColumnIndexType col = 1; 
   for (ColumnIndexType gdr_idx=startIdx; gdr_idx<=endIdx; gdr_idx++)
   {
      CGirderKey girderKey(girderList[gdr_idx]);

      // Get Midspan poi and take averages at 0.2, 0.3 points to compute quarter point reactions
      std::vector<pgsPointOfInterest> vPoi = pIPOI->GetPointsOfInterest(CSegmentKey(girderKey,0),POI_TENTH_POINTS | POI_ERECTED_SEGMENT);
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
      Float64 Xstart = pBridge->GetSlabOffset(girderKey.groupIndex, startPierIdx, girderKey.girderIndex);
      Float64 Xend   = pBridge->GetSlabOffset(girderKey.groupIndex, endPierIdx,   girderKey.girderIndex);

      Float64 height = pGirder->GetHeight(poi_0); // assume prismatic girders

      // deflections from slab loading
      Float64 delta_slab2 = pProductForces->GetDeflection(castDeckIntervalIdx, pgsTypes::pftSlab, poi_2, bat, rtCumulative, false );
      Float64 delta_slab3 = pProductForces->GetDeflection(castDeckIntervalIdx, pgsTypes::pftSlab, poi_3, bat, rtCumulative, false );
      Float64 delta_slab5 = pProductForces->GetDeflection(castDeckIntervalIdx, pgsTypes::pftSlab, poi_5, bat, rtCumulative, false );
      Float64 delta_slab7 = pProductForces->GetDeflection(castDeckIntervalIdx, pgsTypes::pftSlab, poi_7, bat, rtCumulative, false );
      Float64 delta_slab8 = pProductForces->GetDeflection(castDeckIntervalIdx, pgsTypes::pftSlab, poi_8, bat, rtCumulative, false );

      // Haunch depth at mid-span
      CSpanKey spanKey(girderKey.groupIndex, girderKey.girderIndex);  // precast girder bridge assumption here

      HAUNCHDETAILS haunch_details; // all along girder
      pGdrHaunch->GetHaunchDetails(spanKey, &haunch_details);

      // find Z value at mid-span
      Float64 Z(Float64_Max);
      Float64 midloc = poi_5.GetDistFromStart();
      for (std::vector<SECTIONHAUNCH>::const_iterator ith=haunch_details.Haunch.begin(); ith!=haunch_details.Haunch.end(); ith++)
      {
         const SECTIONHAUNCH& haunch = *ith;
         if (ith->PointOfInterest.GetDistFromStart()==midloc)
         {
            if (0.0 > haunch.GirderOrientationEffect)
            {
               Z = haunch.TopSlabToTopGirder; // cl girder in a valley
            }
            else
            {
               Z = haunch.TopSlabToTopGirder - haunch.tSlab - haunch.GirderOrientationEffect;
            }
            break;
         }
      }

      ATLASSERT(Z!=Float64_Max); // not found?

      // Haunch concrete volume - use bearing reactions to determine
      ReactionLocation reacloc;
      reacloc.Face = rftAhead;
      reacloc.GirderKey = girderKey;
      reacloc.PierIdx = startPierIdx;

      Float64 startR = pBearingDesign->GetBearingProductReaction(castDeckIntervalIdx, reacloc, pgsTypes::pftSlabPad, bat, rtCumulative);

      reacloc.Face = rftBack;
      reacloc.GirderKey = girderKey;
      reacloc.PierIdx = endPierIdx;

      Float64 endR   = pBearingDesign->GetBearingProductReaction(castDeckIntervalIdx, reacloc, pgsTypes::pftSlabPad, bat, rtCumulative);

      Float64 haunchWDensity = pMaterials->GetDeckWeightDensity(castDeckIntervalIdx) * unitSysUnitsMgr::GetGravitationalAcceleration();

      Float64 haunchVolume = (startR + endR)/haunchWDensity;

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

      if (areAsSymm)
      {
         if (bFirst)
            (*pTable)(row,0) << _T("X (") << disp.GetUnitTag() << _T(") ");

         (*pTable)(row,col) << disp.SetValue( Xstart );
         row++;

         if (bFirst)
            (*pTable)(row,0) << _T("Y (") << disp.GetUnitTag() << _T(") ");

         (*pTable)(row,col) << disp.SetValue( Xstart+height );
         row++;

      }
      else
      {
         if (bFirst)
            (*pTable)(row,0) << _T("X Start (") << disp.GetUnitTag() << _T(") ");

         (*pTable)(row,col) << disp.SetValue( Xstart );
         row++;

         if (bFirst)
            (*pTable)(row,0) << _T("X End (") << disp.GetUnitTag() << _T(") ");

         (*pTable)(row,col) << disp.SetValue( Xend );
         row++;

         if (bFirst)
            (*pTable)(row,0) << _T("Y Start (") << disp.GetUnitTag() << _T(") ");

         (*pTable)(row,col) << disp.SetValue( Xstart+height );
         row++;

         if (bFirst)
            (*pTable)(row,0) << _T("Y End (") << disp.GetUnitTag() << _T(") ");

         (*pTable)(row,col) << disp.SetValue( Xend+height );
         row++;
      }

      // Z
      if (bFirst)
         (*pTable)(row,0) << _T("Z (") << disp.GetUnitTag() << _T(") ");

      (*pTable)(row,col) << disp.SetValue( Z );
      row++;

      // slab deflections
      if (bFirst)
         (*pTable)(row,0) << _T("DL Defl Slab @ Pt A {1/4 pt} (") << dispft.GetUnitTag() << _T(") ");

      (*pTable)(row,col) << dispft.SetValue( (delta_slab2+delta_slab3)/2.0 );
      row++;

      if (bFirst)
         (*pTable)(row,0) << _T("DL Defl Slab @ Pt B {1/2 pt} (") << dispft.GetUnitTag() << _T(") ");

      (*pTable)(row,col) << dispft.SetValue( delta_slab5 );
      row++;

      if (!areDeflsSymm)
      {
         if (bFirst)
            (*pTable)(row,0) << _T("DL Defl Slab @ Pt A2 {3/4 pt} (") << dispft.GetUnitTag() << _T(") ");

         (*pTable)(row,col) << dispft.SetValue( (delta_slab7+delta_slab8)/2.0 );
         row++;
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
