///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2020  Washington State Department of Transportation
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

#include "TogaCamberAndDeflectionChapterBuilder.h"
#include "TxDOTOptionalDesignUtilities.h"

#include <EAF\EAFDisplayUnits.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Bridge.h>
#include <IFace\Artifact.h>
#include <IFace\Project.h>
#include <IFace\Intervals.h>

#include <PgsExt\ReportPointOfInterest.h>
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
   CTogaCamberAndDeflectionChapterBuilder
****************************************************************************/


static void deflection_and_camber(rptChapter* pChapter,IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits);

////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CTogaCamberAndDeflectionChapterBuilder::CTogaCamberAndDeflectionChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CTogaCamberAndDeflectionChapterBuilder::GetName() const
{
   return TEXT("Camber and Deflections");
}

rptChapter* CTogaCamberAndDeflectionChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CBrokerReportSpecification* pSpec = dynamic_cast<CBrokerReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pSpec->GetBroker(&pBroker);

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   deflection_and_camber( pChapter, pBroker, pDisplayUnits );

   return pChapter;
}

CChapterBuilder* CTogaCamberAndDeflectionChapterBuilder::Clone() const
{
   return new CTogaCamberAndDeflectionChapterBuilder;
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
void deflection_and_camber(rptChapter* pChapter,IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits)
{
   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(5,_T(""));
   *p << pTable << rptNewLine;

   (*pTable)(0,0)  << _T("Stage");

   pTable->SetColumnSpan(0,1,2);
   (*pTable)(0,1) << _T("Original Girder Design");

   pTable->SetColumnSpan(0,2,2);
   (*pTable)(0,2) << _T("Fabricator Optional Design");

   pTable->SetColumnSpan(0,3,-1);
   pTable->SetColumnSpan(0,4,-1);

   // Setup up some unit value prototypes
   INIT_UV_PROTOTYPE( rptLengthUnitValue, disp,   pDisplayUnits->GetDeflectionUnit(), true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dispft, pDisplayUnits->GetSpanLengthUnit(),   true );

   // Get the interfaces we need
   GET_IFACE2(pBroker,ICamber,pCamber);
   GET_IFACE2(pBroker,IPointOfInterest,pIPOI);
   GET_IFACE2(pBroker,IProductForces, pProductForces);
   GET_IFACE2( pBroker, ILibrary, pLib );
   GET_IFACE2( pBroker, ISpecification, pSpec );

   CSegmentKey origSegmentKey(TOGA_SPAN,TOGA_ORIG_GDR,0);
   CSegmentKey fabrSegmentKey(TOGA_SPAN,TOGA_FABR_GDR,0);

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType castDiaphragmIntervalIdx = pIntervals->GetLastNoncompositeInterval();
   IntervalIndexType castDeckIntervalIdx      = pIntervals->GetLastNoncompositeInterval();
   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetLastCompositeInterval();
   IntervalIndexType railingSystemIntervalIdx = pIntervals->GetInstallRailingSystemInterval();
   IntervalIndexType overlayIntervalIdx       = pIntervals->GetOverlayInterval();
   IntervalIndexType liveLoadIntervalIdx      = pIntervals->GetLiveLoadInterval();
   IntervalIndexType noncompositeUserLoadIntervalIdx = pIntervals->GetNoncompositeUserLoadInterval();
   IntervalIndexType compositeUserLoadIntervalIdx = pIntervals->GetCompositeUserLoadInterval();

   std::_tstring spec_name = pSpec->GetSpecification();
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( spec_name.c_str() );

   bool do_defl = pSpecEntry->GetDoEvaluateLLDeflection();
   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   // Get Midspan poi's
   PoiList vPoi_orig;
   pIPOI->GetPointsOfInterest(origSegmentKey, POI_5L | POI_ERECTED_SEGMENT, &vPoi_orig);
   ATLASSERT(vPoi_orig.size()==1);
   const pgsPointOfInterest& poi_orig( vPoi_orig.front() );

   PoiList vPoi_fabr;
   pIPOI->GetPointsOfInterest(fabrSegmentKey, POI_5L | POI_ERECTED_SEGMENT, &vPoi_fabr);
   ATLASSERT(vPoi_fabr.size()==1);
   const pgsPointOfInterest& poi_fabr( vPoi_fabr.front() );

   // Compute mid span deflections
   Float64 delta_gdr_orig, delta_gdr_fabr; // due to girder self weight
   Float64 delta_dl_orig,  delta_dl_fabr;  // due to dead loads on girder
   Float64 delta_ol_orig,  delta_ol_fabr;  // due to overlay
   Float64 delta_tb_orig,  delta_tb_fabr;  // due to traffic barrier
   Float64 delta_ll_orig,  delta_ll_fabr;  // due to live load
   Float64 delta_oll_orig, delta_oll_fabr; // due to optional live load
   Float64 temp;

   delta_gdr_orig = pProductForces->GetGirderDeflectionForCamber( poi_orig );
   delta_gdr_fabr = pProductForces->GetGirderDeflectionForCamber( poi_fabr );

   pgsTypes::BridgeAnalysisType bat = pProductForces->GetBridgeAnalysisType(pgsTypes::Minimize);

   delta_dl_orig = pProductForces->GetDeflection(castDeckIntervalIdx, pgsTypes::pftSlab, poi_orig, bat, rtCumulative, false )
                 + pProductForces->GetDeflection(castDeckIntervalIdx, pgsTypes::pftSlabPanel, poi_orig, bat, rtCumulative, false ) 
                 + pProductForces->GetDeflection(castDiaphragmIntervalIdx, pgsTypes::pftDiaphragm, poi_orig, bat, rtCumulative, false );

   delta_dl_fabr = pProductForces->GetDeflection(castDeckIntervalIdx, pgsTypes::pftSlab, poi_fabr, bat, rtCumulative, false )
                 + pProductForces->GetDeflection(castDeckIntervalIdx, pgsTypes::pftSlabPanel, poi_fabr, bat, rtCumulative, false ) 
                 + pProductForces->GetDeflection(castDiaphragmIntervalIdx, pgsTypes::pftDiaphragm, poi_fabr, bat, rtCumulative, false );

   if ( overlayIntervalIdx == INVALID_INDEX )
   {
      delta_ol_orig = 0;
      delta_ol_fabr = 0;
   }
   else
   {
      delta_ol_orig = pProductForces->GetDeflection(overlayIntervalIdx, pgsTypes::pftOverlay, poi_orig, bat, rtCumulative, false );
      delta_ol_fabr = pProductForces->GetDeflection(overlayIntervalIdx, pgsTypes::pftOverlay, poi_fabr, bat, rtCumulative, false );
   }

   delta_tb_orig = pProductForces->GetDeflection(railingSystemIntervalIdx, pgsTypes::pftTrafficBarrier, poi_orig, bat, rtCumulative, false );
   delta_tb_fabr = pProductForces->GetDeflection(railingSystemIntervalIdx, pgsTypes::pftTrafficBarrier, poi_fabr, bat, rtCumulative, false );

   Float64 delta_dcu_orig = pProductForces->GetDeflection(compositeUserLoadIntervalIdx,pgsTypes::pftUserDC, poi_orig, bat, rtCumulative, false);
   Float64 delta_dcu_fabr = pProductForces->GetDeflection(compositeUserLoadIntervalIdx,pgsTypes::pftUserDC, poi_fabr, bat, rtCumulative, false);

   Float64 delta_dwu_orig = pProductForces->GetDeflection(compositeUserLoadIntervalIdx,pgsTypes::pftUserDW, poi_orig, bat, rtCumulative, false);
   Float64 delta_dwu_fabr = pProductForces->GetDeflection(compositeUserLoadIntervalIdx,pgsTypes::pftUserDW, poi_fabr, bat, rtCumulative, false);

   pProductForces->GetLiveLoadDeflection(liveLoadIntervalIdx, pgsTypes::lltDesign, poi_orig, bat, true, false, &delta_ll_orig, &temp );
   pProductForces->GetLiveLoadDeflection(liveLoadIntervalIdx, pgsTypes::lltDesign, poi_fabr, bat, true, false, &delta_ll_fabr, &temp );

   pProductForces->GetDeflLiveLoadDeflection(IProductForces::DeflectionLiveLoadEnvelope, poi_orig, bat, &delta_oll_orig, &temp );
   pProductForces->GetDeflLiveLoadDeflection(IProductForces::DeflectionLiveLoadEnvelope, poi_fabr, bat, &delta_oll_fabr, &temp );

   // get # of days for creep
   Float64 max_days = ::ConvertFromSysUnits(pSpecEntry->GetCreepDuration2Max(), unitMeasure::Day);

   // Populate the table
   bool is_negative_camber = false;
   Uint16 row = 1;
   (*pTable)(row, 0) << _T("Unfactored Design Camber at ") << max_days << _T(" days, D");;
   Float64 D_uorig = pCamber->GetDCamberForGirderScheduleUnfactored( poi_orig,CREEP_MAXTIME);
   if ( D_uorig < 0 )
   {
      (*pTable)(row,1) << color(Red) << disp.SetValue( D_uorig ) << color(Black);
      (*pTable)(row,2) << color(Red) << dispft.SetValue( D_uorig ) << color(Black);
   }
   else
   {
      (*pTable)(row,1) << disp.SetValue( D_uorig );
      (*pTable)(row,2) << dispft.SetValue( D_uorig );
   }

   Float64 D_ufabr = pCamber->GetDCamberForGirderScheduleUnfactored( poi_fabr,CREEP_MAXTIME);
   if ( D_ufabr < 0 )
   {
      (*pTable)(row,3) << color(Red) << disp.SetValue( D_ufabr ) << color(Black);
      (*pTable)(row,4) << color(Red) << dispft.SetValue( D_ufabr ) << color(Black);
   }
   else
   {
      (*pTable)(row,3) << disp.SetValue( D_ufabr );
      (*pTable)(row,4) << dispft.SetValue( D_ufabr );
   }

   row++;

   (*pTable)(row,0) << _T("Deflection (Prestressing)");
   (*pTable)(row,1) << disp.SetValue( pCamber->GetPrestressDeflection(poi_orig,pgsTypes::pddRelease) );
   (*pTable)(row,2) << dispft.SetValue( pCamber->GetPrestressDeflection(poi_orig,pgsTypes::pddRelease) );

   (*pTable)(row,3) << disp.SetValue( pCamber->GetPrestressDeflection(poi_fabr,pgsTypes::pddRelease) );
   (*pTable)(row,4) << dispft.SetValue( pCamber->GetPrestressDeflection(poi_fabr,pgsTypes::pddRelease) );

   row++;

   (*pTable)(row,0) << _T("Deflection (Girder)");
   (*pTable)(row,1) << disp.SetValue( delta_gdr_orig );
   (*pTable)(row,2) << dispft.SetValue( delta_gdr_orig );

   (*pTable)(row,3) << disp.SetValue( delta_gdr_fabr );
   (*pTable)(row,4) << dispft.SetValue( delta_gdr_fabr );

   row++;

   (*pTable)(row, 0) << _T("Deflection (Deck and Diaphragms)"); // << Super(_T("*"));;
   (*pTable)(row,1) << disp.SetValue( delta_dl_orig );
   (*pTable)(row,2) << dispft.SetValue( delta_dl_orig );

   (*pTable)(row,3) << disp.SetValue( delta_dl_fabr );
   (*pTable)(row,4) << dispft.SetValue( delta_dl_fabr );

   row++;

   (*pTable)(row,0) << _T("Deflection (User Defined DC)");
   (*pTable)(row,1) << disp.SetValue( delta_dcu_orig );
   (*pTable)(row,2) << dispft.SetValue( delta_dcu_orig );

   (*pTable)(row,3) << disp.SetValue( delta_dcu_fabr );
   (*pTable)(row,4) << dispft.SetValue( delta_dcu_fabr );

   row++;

   (*pTable)(row,0) << _T("Deflection (User Defined DW)");
   (*pTable)(row,1) << disp.SetValue( delta_dwu_orig );
   (*pTable)(row,2) << dispft.SetValue( delta_dwu_orig );
   (*pTable)(row,3) << disp.SetValue( delta_dwu_fabr );
   (*pTable)(row,4) << dispft.SetValue( delta_dwu_fabr );

   row++;

   (*pTable)(row,0) << _T("Live Load Deflection (HL93 - Per Lane)");
   (*pTable)(row,1) << disp.SetValue( delta_ll_orig );
   (*pTable)(row,2) << dispft.SetValue( delta_ll_orig );

   (*pTable)(row,3) << disp.SetValue( delta_ll_fabr );
   (*pTable)(row,4) << dispft.SetValue( delta_ll_fabr );

   row++;

   if (do_defl)
   {
      (*pTable)(row,0) << _T("Optional Live Load Deflection (LRFD 3.6.1.3.2)");
      (*pTable)(row,1) << disp.SetValue( delta_oll_orig );
      (*pTable)(row,2) << dispft.SetValue( delta_oll_orig );

      (*pTable)(row,3) << disp.SetValue( delta_oll_fabr );
      (*pTable)(row,4) << dispft.SetValue( delta_oll_fabr );

      row++;
   }

   if (is_negative_camber)
   {
      *p<<color(Red) << _T("Warning:  Excess camber is negative indicating a potential sag in the beam.") << color(Black) << rptNewLine;
   }

   Float64 min_days = ::ConvertFromSysUnits(pSpecEntry->GetCreepDuration2Min(), unitMeasure::Day);
   if (max_days != min_days)
   {
      *p<<color(Red) << _T("Warning: Camber min and max timings in project criteria are different. Values for max timing are shown only.") << color(Black) << rptNewLine;
   }
}



//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
//======================== INQUERY    =======================================
