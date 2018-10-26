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
#include <PgsExt\ReportStyleHolder.h>
#include <Reporting\SpanGirderReportSpecification.h>
#include <Reporting\ConstructabilityCheckTable.h>

#include "TexasCamberAndDeflectionChapterBuilder.h"
#include "TexasIBNSParagraphBuilder.h"
#include "TxDOTOptionalDesignUtilities.h"

#include <EAF\EAFDisplayUnits.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Bridge.h>
#include <IFace\Artifact.h>
#include <IFace\Project.h>
#include <IFace\Intervals.h>

#include <PgsExt\GirderPointOfInterest.h>
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
   CTexasCamberAndDeflectionChapterBuilder
****************************************************************************/


static void deflection_and_camber(rptChapter* pChapter,IBroker* pBroker, const std::vector<CGirderKey>& girderList,
                                  ColumnIndexType startIdx, ColumnIndexType endIdx, bool isSingleGirder, IEAFDisplayUnits* pDisplayUnits);

////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CTexasCamberAndDeflectionChapterBuilder::CTexasCamberAndDeflectionChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CTexasCamberAndDeflectionChapterBuilder::GetName() const
{
   return TEXT("Camber and Deflections");
}

rptChapter* CTexasCamberAndDeflectionChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
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

   bool isSingleGirder = girder_list.size()==1;

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

      deflection_and_camber( pChapter, pBroker, girder_list, start_idx, end_idx, isSingleGirder, pDisplayUnits );
   }

   // Constructability check
   rptRcTable* hdtable = CConstructabilityCheckTable().BuildSlabOffsetTable(pBroker,girder_list,pDisplayUnits);
   if (hdtable!=NULL)
   {
      rptParagraph* p = new rptParagraph;
      *p << hdtable << rptNewLine;
      *pChapter << p;
   }

   return pChapter;
}

CChapterBuilder* CTexasCamberAndDeflectionChapterBuilder::Clone() const
{
   return new CTexasCamberAndDeflectionChapterBuilder;
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
void deflection_and_camber(rptChapter* pChapter,IBroker* pBroker, const std::vector<CGirderKey>& girderList,
                           ColumnIndexType startIdx, ColumnIndexType endIdx, bool isSingleGirder, 
                           IEAFDisplayUnits* pDisplayUnits)
{
   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   ColumnIndexType numCols = isSingleGirder ? 3 : endIdx-startIdx+2; // single girder report has inch and ft units

   std::_tostringstream osTN;
   osTN << _T("Camber and Deflection");
   if (isSingleGirder)
   {
      CGirderKey girderKey(girderList[0]);
      SpanIndexType span = girderKey.groupIndex;
      GirderIndexType girder = girderKey.girderIndex;

      osTN << _T(" for Span ")<<LABEL_SPAN(span)<<_T(" Girder ")<<LABEL_GIRDER(girder);
   }

   rptRcTable* pTable = pgsReportStyleHolder::CreateTableNoHeading(numCols,osTN.str().c_str());
   *p << pTable << rptNewLine;

   // Right justify columns with numbers
   for (ColumnIndexType ic=1; ic<numCols; ic++)
   {
      pTable->SetColumnStyle( ic, pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_RIGHT) );
      pTable->SetStripeRowColumnStyle( ic, pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_RIGHT) );
   }

   // Setup up some unit value prototypes
   INIT_UV_PROTOTYPE( rptLengthUnitValue, disp,   pDisplayUnits->GetDisplacementUnit(), true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dispft, pDisplayUnits->GetSpanLengthUnit(),   true );

   // Get the interfaces we need
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,ICamber,pCamber);
   GET_IFACE2(pBroker,IPointOfInterest,pIPOI);
   GET_IFACE2(pBroker,IProductForces, pProductForces);
   GET_IFACE2(pBroker,IProductLoads, pProductLoads);
   GET_IFACE2( pBroker, ILibrary, pLib );
   GET_IFACE2( pBroker, ISpecification, pSpec );
   GET_IFACE2(pBroker,ISegmentData,pSegmentData);

   std::_tstring spec_name = pSpec->GetSpecification();
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( spec_name.c_str() );

   bool do_defl = pSpecEntry->GetDoEvaluateLLDeflection();
   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   // See if any girders in our list have a sidewalk load
   bool is_any_sidewalk(false), is_any_shearkey(false);
   for (ColumnIndexType gdr_idx=startIdx; gdr_idx<=endIdx; gdr_idx++)
   {
      CGirderKey girderKey(girderList[gdr_idx]);

      is_any_sidewalk |= pProductLoads->HasSidewalkLoad(girderKey);
      is_any_shearkey |= pProductLoads->HasShearKeyLoad(girderKey);
   }

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType castDeckIntervalIdx      = pIntervals->GetCastDeckInterval();
   IntervalIndexType railingSystemIntervalIdx = pIntervals->GetRailingSystemInterval();
   IntervalIndexType liveLoadIntervalIdx      = pIntervals->GetLiveLoadInterval();
   IntervalIndexType overlayIntervalIdx       = pIntervals->GetOverlayInterval();

   // Build table column by column
   std::vector<CSegmentKey> BeamsWithExcessCamber;
   bool bFirst(true);
   ColumnIndexType col = isSingleGirder ? 2 : 1; 
   for (ColumnIndexType gdr_idx=startIdx; gdr_idx<=endIdx; gdr_idx++)
   {
      CGirderKey girderKey(girderList[gdr_idx]);

      // Get Midspan std::vector<pgsPointOfInterest>
      std::vector<pgsPointOfInterest> vPoi = pIPOI->GetPointsOfInterest(CSegmentKey(girderKey,0),POI_MIDSPAN);
      CHECK(vPoi.size()==1);
      pgsPointOfInterest poi = *vPoi.begin();

      const CSegmentKey& segmentKey(poi.GetSegmentKey());

      // Compute mid span deflections
      Float64 delta_gdr;  // due to girder self weight
      Float64 delta_dl;   // due to dead loads on girder
      Float64 delta_sk;   // due to shear key
      Float64 delta_ol;   // due to overlay
      Float64 delta_tb;   // due to traffic barrier
      Float64 delta_sw;   // due to sidewalk
      Float64 delta_ll;   // due to live load
      Float64 delta_oll;  // due to optional live load
      Float64 temp;

      delta_gdr = pProductForces->GetGirderDeflectionForCamber( poi );

      pgsTypes::BridgeAnalysisType bat = pProductForces->GetBridgeAnalysisType(pgsTypes::Minimize);

      delta_dl = pProductForces->GetDisplacement(castDeckIntervalIdx, pftSlab, poi, bat )
               + pProductForces->GetDisplacement(castDeckIntervalIdx, pftDiaphragm, poi, bat );

      delta_sk = pProductForces->GetDisplacement(castDeckIntervalIdx, pftShearKey, poi, bat );
      
      delta_ol = pProductForces->GetDisplacement(overlayIntervalIdx, pftOverlay, poi, bat );

      delta_tb = pProductForces->GetDisplacement(railingSystemIntervalIdx, pftTrafficBarrier, poi, bat );
      delta_sw = pProductForces->GetDisplacement(railingSystemIntervalIdx, pftTrafficBarrier, poi, bat );

      Float64 delta_dcu = pProductForces->GetDisplacement(castDeckIntervalIdx,pftUserDC, poi, bat);
      delta_dcu        += pProductForces->GetDisplacement(railingSystemIntervalIdx,pftUserDC, poi, bat);

      Float64 delta_dwu = pProductForces->GetDisplacement(castDeckIntervalIdx,pftUserDW, poi, bat);
      delta_dwu        += pProductForces->GetDisplacement(railingSystemIntervalIdx,pftUserDW, poi, bat);

      pProductForces->GetLiveLoadDisplacement(pgsTypes::lltDesign, liveLoadIntervalIdx, poi, bat, true, false, &delta_ll, &temp );

      pProductForces->GetDeflLiveLoadDisplacement(IProductForces::DeflectionLiveLoadEnvelope, poi, bat, &delta_oll, &temp );

      // get # of days for creep
      Float64 min_days = ::ConvertFromSysUnits(pSpecEntry->GetCreepDuration2Min(), unitMeasure::Day);
      Float64 max_days = ::ConvertFromSysUnits(pSpecEntry->GetCreepDuration2Max(), unitMeasure::Day);

      // Populate the table
      RowIndexType row = 0;
      if (!isSingleGirder)
      {
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
      }

      if (IsEqual(min_days, max_days))
      {
         // The usual case for TxDOT
         if (bFirst)
            (*pTable)(row,0) << _T("Design Camber");

         Float64 D = pCamber->GetDCamberForGirderSchedule( poi,CREEP_MINTIME);
         if ( D < 0 )
         {
            if (isSingleGirder)
               (*pTable)(row,1) << color(Red) << disp.SetValue( D ) << color(Black);

            (*pTable)(row,col) << color(Red) << dispft.SetValue( D ) << color(Black);
         }
         else
         {
            if (isSingleGirder)
               (*pTable)(row,1) << disp.SetValue( D );

            (*pTable)(row,col) << dispft.SetValue( D );
         }
         row++;
      }
      else
      {
         if (bFirst)
            (*pTable)(row,0) << _T("Estimated camber at ")<< min_days<<_T(" days, D");

         Float64 D = pCamber->GetDCamberForGirderSchedule( poi,CREEP_MINTIME);
         if ( D < 0 )
         {
            if (isSingleGirder)
               (*pTable)(row,1) << color(Red) << disp.SetValue( D ) << color(Black);

            (*pTable)(row,col) << color(Red) << dispft.SetValue( D ) << color(Black);
         }
         else
         {
            if (isSingleGirder)
               (*pTable)(row,1) << disp.SetValue( D );

            (*pTable)(row,col) << dispft.SetValue( D );
         }
         row++;

         if (bFirst)
            (*pTable)(row,0) << _T("Estimated camber at ")<< max_days<<_T(" days, D");

         D = pCamber->GetDCamberForGirderSchedule( poi,CREEP_MAXTIME);
         if ( D < 0 )
         {
            if (isSingleGirder)
               (*pTable)(row,1) << color(Red) << disp.SetValue( D ) << color(Black);

            (*pTable)(row,col) << color(Red) << dispft.SetValue( D ) << color(Black);
         }
         else
         {
            if (isSingleGirder)
               (*pTable)(row,1) << disp.SetValue( D );

            (*pTable)(row,col) << dispft.SetValue( D );
         }

         row++;
      }

      if ( 0 < pSegmentData->GetStrandData(segmentKey)->Nstrands[pgsTypes::Temporary] && pSegmentData->GetStrandData(segmentKey)->TempStrandUsage != pgsTypes::ttsPTBeforeShipping )
      {
         if (bFirst)
            (*pTable)(row,0) << _T("Deflection (Prestressing including temp strands)");

         if (isSingleGirder)
            (*pTable)(row,1) << disp.SetValue( pCamber->GetPrestressDeflection(poi,true) );

         (*pTable)(row,col) << dispft.SetValue( pCamber->GetPrestressDeflection(poi,true) );
      }
      else
      {
         if (bFirst)
            (*pTable)(row,0) << _T("Deflection (Prestressing)");

         if (isSingleGirder)
            (*pTable)(row,1) << disp.SetValue( pCamber->GetPrestressDeflection(poi,false) );

         (*pTable)(row,col) << dispft.SetValue( pCamber->GetPrestressDeflection(poi,false) );
      }
      row++;

      if ( 0 < pSegmentData->GetStrandData(segmentKey)->Nstrands[pgsTypes::Temporary] && pSegmentData->GetStrandData(segmentKey)->TempStrandUsage != pgsTypes::ttsPTBeforeShipping )
      {
         if (bFirst)
            (*pTable)(row,0) << _T("Deflection (Temporary Strand Removal)");

         if (isSingleGirder)
            (*pTable)(row,1) << disp.SetValue( pCamber->GetReleaseTempPrestressDeflection(poi) );

         (*pTable)(row,col) << dispft.SetValue( pCamber->GetReleaseTempPrestressDeflection(poi) );
         row++;
      }

      if (bFirst)
         (*pTable)(row,0) << _T("Deflection (Girder)");

      if (isSingleGirder)
         (*pTable)(row,1) << disp.SetValue( delta_gdr );

      (*pTable)(row,col) << dispft.SetValue( delta_gdr );
      row++;

      if (bFirst)
         (*pTable)(row,0) << _T("Deflection (Slab and Diaphragms)");

      if (isSingleGirder)
         (*pTable)(row,1) << disp.SetValue( delta_dl );

      (*pTable)(row,col) << dispft.SetValue( delta_dl );
      row++;

      if ( is_any_shearkey )
      {
         if (bFirst)
            (*pTable)(row,0) << _T("Deflection (Shear Key)");

         if (isSingleGirder)
            (*pTable)(row,1) << disp.SetValue( delta_sk );

         (*pTable)(row,col) << dispft.SetValue( delta_sk );
         row++;
      }

      if( is_any_sidewalk) // Need row in table if any sidewalks exist in our girder list
      {
         if (bFirst)
            (*pTable)(row,0) << _T("Deflection (Sidewalk)");

         if (isSingleGirder)
            (*pTable)(row,1) << disp.SetValue( delta_sw );

         (*pTable)(row,col) << dispft.SetValue( delta_sw );
         row++;
      }

      if (bFirst)
         (*pTable)(row,0) << _T("Deflection (Traffic Barrier)");

      if (isSingleGirder)
         (*pTable)(row,1) << disp.SetValue( delta_tb );

      (*pTable)(row,col) << dispft.SetValue( delta_tb );
      row++;

      if (bFirst)
         (*pTable)(row,0) << _T("Deflection (Overlay)");

      if (isSingleGirder)
         (*pTable)(row,1) << disp.SetValue( delta_ol );

      (*pTable)(row,col) << dispft.SetValue( delta_ol );
      row++;

      if (bFirst)
         (*pTable)(row,0) << _T("Deflection (User Defined DC)");

      if (isSingleGirder)
         (*pTable)(row,1) << disp.SetValue( delta_dcu );

      (*pTable)(row,col) << dispft.SetValue( delta_dcu );
      row++;

      if (bFirst)
         (*pTable)(row,0) << _T("Deflection (User Defined DW)");

      if (isSingleGirder)
         (*pTable)(row,1) << disp.SetValue( delta_dwu );

      (*pTable)(row,col) << dispft.SetValue( delta_dwu );
      row++;

      if (bFirst)
         (*pTable)(row,0) << _T("Screed Camber, C");

      if (isSingleGirder)
         (*pTable)(row,1) << disp.SetValue( pCamber->GetScreedCamber(poi) );

      (*pTable)(row,col) << dispft.SetValue( pCamber->GetScreedCamber(poi) );
      row++;

      if (bFirst)
         (*pTable)(row,0) << _T("Excess Camber (Based on Design Camber)");

      Float64 excess_camber = pCamber->GetExcessCamber(poi,CREEP_MAXTIME);
      if ( excess_camber < 0 )
      {
         if (isSingleGirder)
            (*pTable)(row,1) << color(Red) << disp.SetValue( excess_camber ) << color(Black);

         (*pTable)(row,col) << color(Red) << dispft.SetValue( excess_camber ) << color(Black);
         BeamsWithExcessCamber.push_back(segmentKey);
      }
      else
      {
         if (isSingleGirder)
            (*pTable)(row,1) << disp.SetValue( excess_camber );

         (*pTable)(row,col) << dispft.SetValue( excess_camber );
      }
      row++;

      if (bFirst)
         (*pTable)(row,0) << _T("Live Load Deflection (HL93 - Per Lane)");

      if (isSingleGirder)
         (*pTable)(row,1) << disp.SetValue( delta_ll );

      (*pTable)(row,col) << dispft.SetValue( delta_ll );
      row++;

      if (do_defl)
      {
         if (bFirst)
            (*pTable)(row,0) << _T("Optional Live Load Deflection (LRFD 3.6.1.3.2)");

         if (isSingleGirder)
            (*pTable)(row,1) << disp.SetValue( delta_oll );

         (*pTable)(row,col) << dispft.SetValue( delta_oll );
         row++;
      }

      bFirst = false;
      col++;
   }

   for (std::vector<CSegmentKey>::const_iterator ite=BeamsWithExcessCamber.begin(); ite!=BeamsWithExcessCamber.end(); ite++)
   {
      const CSegmentKey& segmentKey(*ite);
      SpanIndexType span = segmentKey.groupIndex;
      GirderIndexType girder = segmentKey.girderIndex;
      ATLASSERT(segmentKey.segmentIndex == 0); // this is a precast girder so there should be only one segment

      *p<<color(Red) << _T("Warning: Excess camber is negative");

      if (!isSingleGirder)
         *p<< _T(" for Span ")<<LABEL_SPAN(span)<<_T(" Girder ")<<LABEL_GIRDER(girder);

      *p<< _T(" indicating a potential sag in the beam.") << color(Black) << rptNewLine;
   }
}



//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
//======================== INQUERY    =======================================
