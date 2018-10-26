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
#include <Reporting\CamberTable.h>
#include <Reporting\ReportNotes.h>

#include <PgsExt\ReportPointOfInterest.h>

#include <IFace\Bridge.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Project.h>
#include <IFace\Intervals.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#pragma Reminder("UPDATE: camber reporting needs to be re-worked to include change in deflection due to girder erection")
// There is a change in deflection between storage and erection when the girder isn't support at the same
// location during this two intervals. The deflection analysis needs to be updated to account for that.
// The creep due to the change in girder loading (as a result of change in support location) is
// computed similarly to the secondary creep due to temporary strand removal and diaphragm dead load.
//
// This is done for CIP. Update the rest so the all work the same

/****************************************************************************
CLASS
   CCamberTable
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CCamberTable::CCamberTable()
{
}

CCamberTable::CCamberTable(const CCamberTable& rOther)
{
   MakeCopy(rOther);
}

CCamberTable::~CCamberTable()
{
}

//======================== OPERATORS  =======================================
CCamberTable& CCamberTable::operator= (const CCamberTable& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void CCamberTable::MakeCopy(const CCamberTable& rOther)
{
   // Add copy code here...
}

void CCamberTable::MakeAssignment(const CCamberTable& rOther)
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
bool CCamberTable::AssertValid() const
{
   return true;
}

void CCamberTable::Dump(dbgDumpContext& os) const
{
   os << _T("Dump for CCamberTable") << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool CCamberTable::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("CCamberTable");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for CCamberTable");

   TESTME_EPILOG("CCamberTable");
}
#endif // _UNITTEST


void CCamberTable::Build_CIP_TempStrands(IBroker* pBroker,const CSegmentKey& segmentKey,
                                         IEAFDisplayUnits* pDisplayUnits,Int16 constructionRate,
                                         rptRcTable** pTable1,rptRcTable** pTable2,rptRcTable** pTable3) const
{
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   location.IncludeSpanAndGirder(segmentKey.groupIndex == ALL_GROUPS);
   INIT_UV_PROTOTYPE( rptLengthUnitValue, deflection, pDisplayUnits->GetDeflectionUnit(), false );

   GET_IFACE2(pBroker,ILibrary,pLib);
   GET_IFACE2(pBroker,ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());

   // Get the interface pointers we need
   GET_IFACE2(pBroker,IPointOfInterest,pPoi);
   std::vector<pgsPointOfInterest> vPoiRelease( pPoi->GetPointsOfInterest( segmentKey,POI_RELEASED_SEGMENT | POI_TENTH_POINTS) );
   std::vector<pgsPointOfInterest> vPoiStorage( pPoi->GetPointsOfInterest( segmentKey,POI_STORAGE_SEGMENT | POI_TENTH_POINTS ) );
   std::vector<pgsPointOfInterest> vPoiErected( pPoi->GetPointsOfInterest( segmentKey,POI_ERECTED_SEGMENT | POI_TENTH_POINTS ) );

   ATLASSERT(vPoiRelease.size() == vPoiStorage.size() && vPoiStorage.size() == vPoiErected.size());

   GET_IFACE2(pBroker,ICamber,pCamber);
   GET_IFACE2(pBroker,IProductLoads,pProductLoads);
   GET_IFACE2(pBroker,IProductForces,pProduct);
   GET_IFACE2(pBroker,IBridge,pBridge);

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx           = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType storageIntervalIdx           = pIntervals->GetStorageInterval(segmentKey);
   IntervalIndexType erectionIntervalIdx          = pIntervals->GetErectSegmentInterval(segmentKey);
   IntervalIndexType tempStrandRemovalIntervalIdx = pIntervals->GetTemporaryStrandRemovalInterval(segmentKey);
   IntervalIndexType castDeckIntervalIdx          = pIntervals->GetCastDeckInterval();
   IntervalIndexType compositeDeckIntervalIdx     = pIntervals->GetCompositeDeckInterval();
   IntervalIndexType railingSystemIntervalIdx     = pIntervals->GetInstallRailingSystemInterval();
   IntervalIndexType overlayIntervalIdx           = pIntervals->GetOverlayInterval();

   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();

   bool bSidewalk = pProductLoads->HasSidewalkLoad(segmentKey);
   bool bShearKey = pProductLoads->HasShearKeyLoad(segmentKey);

   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   // create the tables
   rptRcTable* table1a;
   rptRcTable* table1b;
   rptRcTable* table2;
   rptRcTable* table3;

   rptRcTable* pLayoutTable = pgsReportStyleHolder::CreateLayoutTable(2,_T("Camber - Part 1 (upwards is positive)"));
   
   table1a = pgsReportStyleHolder::CreateDefaultTable(3);
   table1b = pgsReportStyleHolder::CreateDefaultTable(4);
   (*pLayoutTable)(0,0) << table1a;
   (*pLayoutTable)(0,1) << table1b;

   table2 = pgsReportStyleHolder::CreateDefaultTable(10 + (bSidewalk ? 1 : 0) + (!pBridge->IsFutureOverlay() ? 1 : 0) + (bShearKey ? 1 : 0),_T("Camber - Part 2 (upwards is positive)"));
   table3 = pgsReportStyleHolder::CreateDefaultTable(8,_T("Camber - Part 3 (upwards is positive)"));

   if ( segmentKey.groupIndex == ALL_GROUPS )
   {
      table1a->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table1a->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

      table1b->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table1b->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

      table2->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table2->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

      table3->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table3->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   // Setup table headings
   ColumnIndexType col = 0;
   (*table1a)(0,col++) << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table1a)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("girder")) << rptNewLine << _T("Release"),     rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table1a)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("ps")) << rptNewLine << _T("Release"),         rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   
   col = 0;
   (*table1b)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table1b)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("girder")) << rptNewLine << _T("Storage"),     rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table1b)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("ps")) << rptNewLine << _T("Storage"),         rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table1b)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("creep1")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

   col = 0;
   (*table2)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("girder")) << rptNewLine << _T("Erected"),     rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("ps")) << rptNewLine << _T("Erected"),         rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("tpsr")),         rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("diaphragm")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

   if ( bShearKey )
   {
      (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("shear key")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   }

   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("creep2")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("deck")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("User1")) << _T(" = ") << rptNewLine << Sub2(symbol(DELTA),_T("UserDC")) << _T(" + ") << rptNewLine << Sub2(symbol(DELTA),_T("UserDW")) , rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

   if ( bSidewalk )
   {
      (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("sidewalk")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   }

   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("barrier")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

   if ( !pBridge->IsFutureOverlay() )
   {
      (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("overlay")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   }

   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("User2")) << _T(" = ") << rptNewLine << Sub2(symbol(DELTA),_T("UserDC")) << _T(" + ") << rptNewLine << Sub2(symbol(DELTA),_T("UserDW")) , rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

   col = 0;
   (*table3)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table3)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("1")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table3)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("2")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table3)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("3")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

   Float64 days = (constructionRate == CREEP_MINTIME ? pSpecEntry->GetCreepDuration2Min() : pSpecEntry->GetCreepDuration2Max());
   days = ::ConvertFromSysUnits(days,unitMeasure::Day);
   std::_tostringstream os;
   os << days;
   (*table3)(0,col++) << COLHDR(Sub2(_T("D"),os.str().c_str()) << _T(" = ") << Sub2(symbol(DELTA),_T("4")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table3)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("5")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table3)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("excess")) << _T(" = ") << rptNewLine << Sub2(symbol(DELTA),_T("6")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table3)(0,col++) << COLHDR(_T("C = ") << Sub2(symbol(DELTA),_T("4")) << _T(" - ") << Sub2(symbol(DELTA),_T("6")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

   pgsTypes::BridgeAnalysisType bat = pProduct->GetBridgeAnalysisType(pgsTypes::Minimize);

   // Fill up the tables
   RowIndexType row1a = table1a->GetNumberOfHeaderRows();
   RowIndexType row1b = table1b->GetNumberOfHeaderRows();
   RowIndexType row2 = table2->GetNumberOfHeaderRows();
   RowIndexType row3 = table3->GetNumberOfHeaderRows();
   std::vector<pgsPointOfInterest>::iterator releasePoiIter(vPoiRelease.begin());
   std::vector<pgsPointOfInterest>::iterator releasePoiIterEnd(vPoiRelease.end());
   std::vector<pgsPointOfInterest>::iterator storagePoiIter(vPoiStorage.begin());
   std::vector<pgsPointOfInterest>::iterator erectedPoiIter(vPoiErected.begin());
   for ( ; releasePoiIter != releasePoiIterEnd; releasePoiIter++, storagePoiIter++, erectedPoiIter++ )
   {
      const pgsPointOfInterest& releasePoi(*releasePoiIter);
      const pgsPointOfInterest& storagePoi(*storagePoiIter);
      const pgsPointOfInterest& erectedPoi(*erectedPoiIter);

      Float64 DpsRelease   = pProduct->GetDeflection(releaseIntervalIdx,pftPretension,releasePoi,bat,rtCumulative,false);
      Float64 DpsStorage   = pProduct->GetDeflection(storageIntervalIdx,pftPretension,storagePoi,bat,rtCumulative,false);
      Float64 DpsErected  = pProduct->GetDeflection(erectionIntervalIdx,pftPretension,erectedPoi,bat,rtCumulative,false);

      Float64 DgdrRelease  = pProduct->GetDeflection(releaseIntervalIdx,pftGirder,releasePoi,bat,rtCumulative,false);
      Float64 DgdrStorage  = pProduct->GetDeflection(storageIntervalIdx,pftGirder,storagePoi,bat,rtCumulative,false);
      Float64 DgdrErected = pProduct->GetDeflection(erectionIntervalIdx,pftGirder,erectedPoi,bat,rtCumulative,false);

      //Float64 Dtpsr = pProduct->GetDeflection(tempStrandRemovalIntervalIdx,pftPretension,erectedPoi,bat,rtIncremental,false);
      Float64 Dtpsr = pCamber->GetReleaseTempPrestressDeflection( erectedPoi );

      //Dps1       = pCamber->GetPrestressDeflection( poi, false );
      //Dps        = pCamber->GetPrestressDeflection( poi, true );
      //Dps1       = pProduct->GetDeflection(releaseIntervalIdx,pftPretension,poi,bat,rtCumulative,false);
      //Dps        = pProduct->GetDeflection(storageIntervalIdx,pftPretension,poi,bat,rtCumulative,false);
      //Dtpsi      = pCamber->GetInitialTempPrestressDeflection( poi,true );
      //Float64 Dtpsr      = pCamber->GetReleaseTempPrestressDeflection( erectedPoi );
      //Dgirder    = pProductForces->GetGirderDeflectionForCamber( poi );
      //Dgirder    = pProduct->GetDeflection(storageIntervalIdx,pftGirder,poi,bat,rtCumulative,false);
      Float64 Dcreep1    = pCamber->GetCreepDeflection( storagePoi, ICamber::cpReleaseToDiaphragm, constructionRate );
      Float64 Ddiaphragm = pCamber->GetDiaphragmDeflection( erectedPoi );
      Float64 Dshearkey  = pProduct->GetDeflection(castDeckIntervalIdx,pftShearKey,erectedPoi,bat, rtCumulative, false);
      Float64 Ddeck      = pProduct->GetDeflection(castDeckIntervalIdx,pftSlab,erectedPoi,bat, rtCumulative, false);
              Ddeck     += pProduct->GetDeflection(castDeckIntervalIdx,pftSlabPad,erectedPoi,bat, rtCumulative, false);
      Float64 Dcreep2    = pCamber->GetCreepDeflection( erectedPoi, ICamber::cpDiaphragmToDeck, constructionRate );
      Float64 Duser1     = pProduct->GetDeflection(castDeckIntervalIdx,pftUserDC,erectedPoi,bat, rtCumulative, false) 
                         + pProduct->GetDeflection(castDeckIntervalIdx,pftUserDW,erectedPoi,bat, rtCumulative, false);
      Float64 Duser2     = pProduct->GetDeflection(compositeDeckIntervalIdx,pftUserDC,erectedPoi,bat, rtCumulative, false) 
                         + pProduct->GetDeflection(compositeDeckIntervalIdx,pftUserDW,erectedPoi,bat, rtCumulative, false);
      Float64 Dbarrier   = pProduct->GetDeflection(railingSystemIntervalIdx,pftTrafficBarrier,erectedPoi,bat, rtCumulative, false);
      Float64 Dsidewalk  = pProduct->GetDeflection(railingSystemIntervalIdx,pftSidewalk,      erectedPoi,bat, rtCumulative, false);
      Float64 Doverlay   = (pBridge->HasOverlay() ? (pBridge->IsFutureOverlay() ? 0.0 : pProduct->GetDeflection(overlayIntervalIdx,pftOverlay,erectedPoi,bat, rtCumulative, false)) : 0.0);

      // if we have a future overlay, the deflection due to the overlay in BridgeSite2 must be zero
      ATLASSERT( pBridge->IsFutureOverlay() ? IsZero(Doverlay) : true );


      // Table 1a
      col = 0;
      (*table1a)(row1a,col++) << location.SetValue( POI_RELEASED_SEGMENT, releasePoi );
      (*table1a)(row1a,col++) << deflection.SetValue( DgdrRelease );
      (*table1a)(row1a,col++) << deflection.SetValue( DpsRelease );
      row1a++;

      // Table 1b
      col = 0;
      (*table1b)(row1b,col++) << location.SetValue( POI_STORAGE_SEGMENT, storagePoi );
      (*table1b)(row1b,col++) << deflection.SetValue( DgdrStorage );
      (*table1b)(row1b,col++) << deflection.SetValue( DpsStorage );
      (*table1b)(row1b,col++) << deflection.SetValue( Dcreep1 );
      row1b++;

      // Table 2
      col = 0;
      (*table2)(row2,col++) << location.SetValue( POI_ERECTED_SEGMENT, erectedPoi );
      (*table2)(row2,col++) << deflection.SetValue( DgdrErected );
      (*table2)(row2,col++) << deflection.SetValue( DpsErected );
      (*table2)(row2,col++) << deflection.SetValue( Dtpsr );
      (*table2)(row2,col++) << deflection.SetValue( Ddiaphragm );
      if ( bShearKey )
      {
         (*table2)(row2,col++) << deflection.SetValue( Dshearkey );
      }

      (*table2)(row2,col++) << deflection.SetValue( Dcreep2 );
      (*table2)(row2,col++) << deflection.SetValue( Ddeck );
      (*table2)(row2,col++) << deflection.SetValue( Duser1 );
      if ( bSidewalk )
      {
         (*table2)(row2,col++) << deflection.SetValue( Dsidewalk );
      }

      (*table2)(row2,col++) << deflection.SetValue( Dbarrier );

      if ( !pBridge->IsFutureOverlay() )
      {
         (*table2)(row2,col++) << deflection.SetValue(Doverlay);
      }

      (*table2)(row2,col++) << deflection.SetValue( Duser2 );

      row2++;

      // Table 3
      col = 0;
#pragma Reminder("UPDATE: D1 should be based on deflections after erection")
      // PGSuper v2 used deflections at end of storage. Since RDP and TxDOT are
      // going to totally redo camber, make it match v2 and fix it later
      //Float64 D1 = DgdrErected + DpsErected;
      Float64 D1 = DgdrStorage + DpsStorage;
      Float64 D2 = D1 + Dcreep1;
      Float64 D3 = D2 + Ddiaphragm + Dshearkey + Dtpsr;
      Float64 D4 = D3 + Dcreep2;
      Float64 D5 = D4 + Ddeck + Duser1;
      Float64 D6 = D5 + Dsidewalk + Dbarrier + Doverlay + Duser2;

      (*table3)(row3,col++) << location.SetValue( POI_ERECTED_SEGMENT, erectedPoi );
      (*table3)(row3,col++) << deflection.SetValue( D1 );
      (*table3)(row3,col++) << deflection.SetValue( D2 );
      (*table3)(row3,col++) << deflection.SetValue( D3 );

      D4 = IsZero(D4) ? 0 : D4;
      if ( D4 < 0 )
      {
         (*table3)(row3,col++) << color(Red) << deflection.SetValue(D4) << color(Black);
      }
      else
      {
         (*table3)(row3,col++) << deflection.SetValue( D4 );
      }

      (*table3)(row3,col++) << deflection.SetValue( D5 );

      D6 = IsZero(D6) ? 0 : D6;
      if ( D6 < 0 )
      {
         (*table3)(row3,col++) << color(Red) << deflection.SetValue(D6) << color(Black);
      }
      else
      {
         (*table3)(row3,col++) << deflection.SetValue( D6 );
      }
      (*table3)(row3,col++) << deflection.SetValue( D4 - D6 );

      row3++;
   }

   *pTable1 = pLayoutTable;
   *pTable2 = table2;
   *pTable3 = table3;
}


void CCamberTable::Build_CIP(IBroker* pBroker,const CSegmentKey& segmentKey,
                             IEAFDisplayUnits* pDisplayUnits,Int16 constructionRate,
                             rptRcTable** pTable1,rptRcTable** pTable2,rptRcTable** pTable3) const
{
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   location.IncludeSpanAndGirder(segmentKey.groupIndex == ALL_GROUPS);
   INIT_UV_PROTOTYPE( rptLengthUnitValue, deflection, pDisplayUnits->GetDeflectionUnit(), false );

   GET_IFACE2(pBroker,ILibrary,pLib);
   GET_IFACE2(pBroker,ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());

   // Get the interface pointers we need

   GET_IFACE2(pBroker,ICamber,pCamber);
   GET_IFACE2(pBroker,IProductForces,pProduct);
   GET_IFACE2(pBroker,IProductLoads,pProductLoads);
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IPointOfInterest,pPoi);
   GET_IFACE2(pBroker,IIntervals,pIntervals);

   std::vector<pgsPointOfInterest> vPoiRelease( pPoi->GetPointsOfInterest( segmentKey,POI_RELEASED_SEGMENT | POI_TENTH_POINTS) );
   std::vector<pgsPointOfInterest> vPoiStorage( pPoi->GetPointsOfInterest( segmentKey,POI_STORAGE_SEGMENT | POI_TENTH_POINTS ) );
   std::vector<pgsPointOfInterest> vPoiErected( pPoi->GetPointsOfInterest( segmentKey,POI_ERECTED_SEGMENT | POI_TENTH_POINTS ) );

   ATLASSERT(vPoiRelease.size() == vPoiStorage.size() && vPoiStorage.size() == vPoiErected.size());

   IntervalIndexType releaseIntervalIdx       = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType storageIntervalIdx       = pIntervals->GetStorageInterval(segmentKey);
   IntervalIndexType erectionIntervalIdx      = pIntervals->GetErectSegmentInterval(segmentKey);
   IntervalIndexType castDeckIntervalIdx      = pIntervals->GetCastDeckInterval();
   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval();
   IntervalIndexType railingSystemIntervalIdx = pIntervals->GetInstallRailingSystemInterval();
   IntervalIndexType overlayIntervalIdx       = pIntervals->GetOverlayInterval();

   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();

   bool bSidewalk = pProductLoads->HasSidewalkLoad(segmentKey);
   bool bShearKey = pProductLoads->HasShearKeyLoad(segmentKey);

   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   // create the tables
   rptRcTable* table1a;
   rptRcTable* table1b;
   rptRcTable* table2;
   rptRcTable* table3;

   rptRcTable* pLayoutTable = pgsReportStyleHolder::CreateLayoutTable(2,_T("Camber - Part 1 (upwards is positive)"));

   table1a = pgsReportStyleHolder::CreateDefaultTable(3);
   table1b = pgsReportStyleHolder::CreateDefaultTable(4);
   (*pLayoutTable)(0,0) << table1a;
   (*pLayoutTable)(0,1) << table1b;

   table2 = pgsReportStyleHolder::CreateDefaultTable(8 + (bSidewalk ? 1 : 0) + (!pBridge->IsFutureOverlay() ? 1 : 0)+ (bShearKey ? 1 : 0),_T("Camber - Part 2 (upwards is positive)"));
   table3 = pgsReportStyleHolder::CreateDefaultTable(6,_T("Camber - Part 3 (upwards is positive)"));

   if ( segmentKey.groupIndex == ALL_GROUPS )
   {
      table1a->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table1a->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

      table1b->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table1b->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

      table2->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table2->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

      table3->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table3->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   // Setup table headings
   ColumnIndexType col = 0;
   (*table1a)(0,col++) << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table1a)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("girder")) << rptNewLine << _T("Release"),     rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table1a)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("ps")) << rptNewLine << _T("Release"),         rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   
   col = 0;
   (*table1b)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table1b)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("girder")) << rptNewLine << _T("Storage"),     rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table1b)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("ps")) << rptNewLine << _T("Storage"),         rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table1b)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("creep")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

   col = 0;
   (*table2)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("girder")) << rptNewLine << _T("Erected"),     rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("ps")) << rptNewLine << _T("Erected"),         rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("diaphragm")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

   if ( bShearKey )
   {
      (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("shear key")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   }

   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("deck")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("User1")) << _T(" = ") << rptNewLine << Sub2(symbol(DELTA),_T("UserDC")) << _T(" + ") << rptNewLine  << Sub2(symbol(DELTA),_T("UserDW")) , rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

   if ( bSidewalk )
   {
      (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("sidewalk")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   }

   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("barrier")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

   if ( !pBridge->IsFutureOverlay() )
   {
      (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("overlay")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   }

   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("User2")) << _T(" = ") << rptNewLine << Sub2(symbol(DELTA),_T("UserDC")) << _T(" + ") << rptNewLine  << Sub2(symbol(DELTA),_T("UserDW")) , rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

   col = 0;
   (*table3)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table3)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("1")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

   Float64 days = (constructionRate == CREEP_MINTIME ? pSpecEntry->GetCreepDuration2Min() : pSpecEntry->GetCreepDuration2Max());
   days = ::ConvertFromSysUnits(days,unitMeasure::Day);
   std::_tostringstream os;
   os << days;
   (*table3)(0,col++) << COLHDR(Sub2(_T("D"),os.str().c_str()) << _T(" = ") << Sub2(symbol(DELTA),_T("2")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table3)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("3")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table3)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("excess")) << _T(" = ") << rptNewLine << Sub2(symbol(DELTA),_T("4")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table3)(0,col++) << COLHDR(_T("C = ") << Sub2(symbol(DELTA),_T("2")) << _T(" - ") << Sub2(symbol(DELTA),_T("4")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

   pgsTypes::BridgeAnalysisType bat = pProduct->GetBridgeAnalysisType(pgsTypes::Minimize);

   // Fill up the tables
   RowIndexType row1a = table1a->GetNumberOfHeaderRows();
   RowIndexType row1b = table1b->GetNumberOfHeaderRows();
   RowIndexType row2 = table2->GetNumberOfHeaderRows();
   RowIndexType row3 = table3->GetNumberOfHeaderRows();
   
   std::vector<pgsPointOfInterest>::iterator releasePoiIter(vPoiRelease.begin());
   std::vector<pgsPointOfInterest>::iterator releasePoiIterEnd(vPoiRelease.end());
   std::vector<pgsPointOfInterest>::iterator storagePoiIter(vPoiStorage.begin());
   std::vector<pgsPointOfInterest>::iterator erectedPoiIter(vPoiErected.begin());
   for ( ; releasePoiIter != releasePoiIterEnd; releasePoiIter++, storagePoiIter++, erectedPoiIter++ )
   {
      const pgsPointOfInterest& releasePoi(*releasePoiIter);
      const pgsPointOfInterest& storagePoi(*storagePoiIter);
      const pgsPointOfInterest& erectedPoi(*erectedPoiIter);

      Float64 DpsRelease  = pProduct->GetDeflection(releaseIntervalIdx,pftPretension,releasePoi,bat,rtCumulative,false);
      Float64 DpsStorage  = pProduct->GetDeflection(storageIntervalIdx,pftPretension,storagePoi,bat,rtCumulative,false);
      Float64 DpsErected  = pProduct->GetDeflection(erectionIntervalIdx,pftPretension,erectedPoi,bat,rtCumulative,false);

      Float64 DgdrRelease = pProduct->GetDeflection(releaseIntervalIdx,pftGirder,releasePoi,bat,rtCumulative,false);
      Float64 DgdrStorage = pProduct->GetDeflection(storageIntervalIdx,pftGirder,storagePoi,bat,rtCumulative,false);
      Float64 DgdrErected = pProduct->GetDeflection(erectionIntervalIdx,pftGirder,erectedPoi,bat,rtCumulative,false);

      // NOTE: Get the creep deflection from the ICamber interface because it takes the construction rate 
      // into account. Getting creep deflection as a product load assumes the maximum construction rate
      //Float64 Dcreep       = pProduct->GetDeflection(storageIntervalIdx,pftCreep,storagePoi,bat,rtCumulative,false);
      Float64 Dcreep       = pCamber->GetCreepDeflection( storagePoi, ICamber::cpReleaseToDeck, constructionRate );

      Float64 Ddiaphragm   = pProduct->GetDeflection(castDeckIntervalIdx,pftDiaphragm,erectedPoi,bat, rtCumulative, false);
      Float64 Dshearkey    = pProduct->GetDeflection(castDeckIntervalIdx,pftShearKey,erectedPoi,bat, rtCumulative, false);
      Float64 Ddeck        = pProduct->GetDeflection(castDeckIntervalIdx,pftSlab,erectedPoi,bat, rtCumulative, false)
                           + pProduct->GetDeflection(castDeckIntervalIdx,pftSlabPad,erectedPoi,bat, rtCumulative, false);
      Float64 Duser1       = pProduct->GetDeflection(castDeckIntervalIdx,pftUserDC,erectedPoi,bat, rtCumulative, false) 
                           + pProduct->GetDeflection(castDeckIntervalIdx,pftUserDW,erectedPoi,bat, rtCumulative, false);
      Float64 Duser2       = pProduct->GetDeflection(compositeDeckIntervalIdx,pftUserDC,erectedPoi,bat, rtCumulative, false) 
                           + pProduct->GetDeflection(compositeDeckIntervalIdx,pftUserDW,erectedPoi,bat, rtCumulative, false);
      Float64 Dbarrier     = pProduct->GetDeflection(railingSystemIntervalIdx,pftTrafficBarrier,erectedPoi,bat, rtCumulative, false);
      Float64 Dsidewalk    = pProduct->GetDeflection(railingSystemIntervalIdx,pftSidewalk,      erectedPoi,bat, rtCumulative, false);
      Float64 Doverlay     = (pBridge->HasOverlay() ? (pBridge->IsFutureOverlay() ? 0.0 : pProduct->GetDeflection(overlayIntervalIdx,pftOverlay,erectedPoi,bat, rtCumulative, false)) : 0.0);

      // Table 1a
      col = 0;
      (*table1a)(row1a,col++) << location.SetValue( POI_RELEASED_SEGMENT, releasePoi );
      (*table1a)(row1a,col++) << deflection.SetValue( DgdrRelease );
      (*table1a)(row1a,col++) << deflection.SetValue( DpsRelease );
      row1a++;

      // Table 1b
      col = 0;
      (*table1b)(row1b,col++) << location.SetValue( POI_STORAGE_SEGMENT, storagePoi );
      (*table1b)(row1b,col++) << deflection.SetValue( DgdrStorage );
      (*table1b)(row1b,col++) << deflection.SetValue( DpsStorage );
      (*table1b)(row1b,col++) << deflection.SetValue( Dcreep );
      row1b++;

      // Table 2
      col = 0;
      (*table2)(row2,col++) << location.SetValue( POI_ERECTED_SEGMENT, erectedPoi );
      (*table2)(row2,col++) << deflection.SetValue( DgdrErected );
      (*table2)(row2,col++) << deflection.SetValue( DpsErected );
      (*table2)(row2,col++) << deflection.SetValue( Ddiaphragm );

      if ( bShearKey )
      {
         (*table2)(row2,col++) << deflection.SetValue( Dshearkey );
      }

      (*table2)(row2,col++) << deflection.SetValue( Ddeck );
      (*table2)(row2,col++) << deflection.SetValue( Duser1 );

      if ( bSidewalk )
      {
         (*table2)(row2,col++) << deflection.SetValue( Dsidewalk );
      }

      (*table2)(row2,col++) << deflection.SetValue( Dbarrier);

      if ( !pBridge->IsFutureOverlay() )
      {
         (*table2)(row2,col++) << deflection.SetValue(Doverlay);
      }

      (*table2)(row2,col++) << deflection.SetValue( Duser2 );

      row2++;

      // Table 3
      col = 0;
#pragma Reminder("UPDATE: D1 should be based on deflections after erection")
      // PGSuper v2 used deflections at end of storage. Since RDP and TxDOT are
      // going to totally redo camber, make it match v2 and fix it later
      //Float64 D1 = DgdrErected + DpsErected;
      Float64 D1 = DgdrStorage + DpsStorage;
      Float64 D2 = D1 + Dcreep;
      Float64 D3 = D2 + Ddiaphragm + Ddeck + Dshearkey + Duser1;
      Float64 D4 = D3 + Dsidewalk + Dbarrier + Duser2 + Doverlay;

      (*table3)(row3,col++) << location.SetValue(POI_ERECTED_SEGMENT, erectedPoi );
      (*table3)(row3,col++) << deflection.SetValue( D1 );

      D2 = IsZero(D2) ? 0 : D2;
      if ( D2 < 0 )
      {
         (*table3)(row3,col++) << color(Red) << deflection.SetValue(D2) << color(Black);
      }
      else
      {
         (*table3)(row3,col++) << deflection.SetValue( D2 );
      }

      (*table3)(row3,col++) << deflection.SetValue( D3 );

      D4 = IsZero(D4) ? 0 : D4;
      if ( D4 < 0 )
      {
         (*table3)(row3,col++) << color(Red) << deflection.SetValue(D4) << color(Black);
      }
      else
      {
         (*table3)(row3,col++) << deflection.SetValue( D4 );
      }

      (*table3)(row3,col++) << deflection.SetValue( D2 - D4 );

      row3++;
   }

   *pTable1 = pLayoutTable;
   *pTable2 = table2;
   *pTable3 = table3;
}

void CCamberTable::Build_SIP_TempStrands(IBroker* pBroker,const CSegmentKey& segmentKey,
                                         IEAFDisplayUnits* pDisplayUnits,Int16 constructionRate,
                                         rptRcTable** pTable1,rptRcTable** pTable2,rptRcTable** pTable3) const
{
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   location.IncludeSpanAndGirder(segmentKey.groupIndex == ALL_GROUPS);
   INIT_UV_PROTOTYPE( rptLengthUnitValue, deflection, pDisplayUnits->GetDeflectionUnit(), false );

   GET_IFACE2(pBroker,ILibrary,pLib);
   GET_IFACE2(pBroker,ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());

   // Get the interface pointers we need
   GET_IFACE2(pBroker,IPointOfInterest,pPoi);

   std::vector<pgsPointOfInterest> vPoiRelease( pPoi->GetPointsOfInterest( segmentKey,POI_RELEASED_SEGMENT | POI_TENTH_POINTS) );
   std::vector<pgsPointOfInterest> vPoiStorage( pPoi->GetPointsOfInterest( segmentKey,POI_STORAGE_SEGMENT | POI_TENTH_POINTS ) );
   std::vector<pgsPointOfInterest> vPoiErected( pPoi->GetPointsOfInterest( segmentKey,POI_ERECTED_SEGMENT | POI_TENTH_POINTS ) );

   ATLASSERT(vPoiRelease.size() == vPoiStorage.size() && vPoiStorage.size() == vPoiErected.size());

   GET_IFACE2(pBroker,ICamber,pCamber);
   GET_IFACE2(pBroker,IProductLoads,pProductLoads);
   GET_IFACE2(pBroker,IProductForces,pProduct);
   GET_IFACE2(pBroker,IBridge,pBridge);

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx       = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType storageIntervalIdx       = pIntervals->GetStorageInterval(segmentKey);
   IntervalIndexType erectionIntervalIdx      = pIntervals->GetErectSegmentInterval(segmentKey);
   IntervalIndexType castDeckIntervalIdx      = pIntervals->GetCastDeckInterval();
   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval();
   IntervalIndexType railingSystemIntervalIdx = pIntervals->GetInstallRailingSystemInterval();
   IntervalIndexType overlayIntervalIdx       = pIntervals->GetOverlayInterval();

   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();

   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   bool bSidewalk = pProductLoads->HasSidewalkLoad(segmentKey);
   bool bShearKey = pProductLoads->HasShearKeyLoad(segmentKey);

   // create the tables
   rptRcTable* table1a;
   rptRcTable* table1b;
   rptRcTable* table2;
   rptRcTable* table3;

   rptRcTable* pLayoutTable = pgsReportStyleHolder::CreateLayoutTable(2,_T("Camber - Part 1 (upwards is positive)"));

   table1a = pgsReportStyleHolder::CreateDefaultTable(3);
   table1b = pgsReportStyleHolder::CreateDefaultTable(4);
   (*pLayoutTable)(0,0) << table1a;
   (*pLayoutTable)(0,1) << table1b;

   table2 = pgsReportStyleHolder::CreateDefaultTable(11 + (bSidewalk ? 1 : 0) + (!pBridge->IsFutureOverlay() ? 1 : 0) + (bShearKey ? 1 : 0),_T("Camber - Part 2 (upwards is positive)"));
   table3 = pgsReportStyleHolder::CreateDefaultTable(8,_T("Camber - Part 3 (upwards is positive)"));

   if ( segmentKey.groupIndex == ALL_GROUPS )
   {
      table1a->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table1a->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

      table1b->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table1b->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

      table2->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table2->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

      table3->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table3->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   // Setup table headings
   ColumnIndexType col = 0;
   (*table1a)(0,col++) << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table1a)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("girder")) << rptNewLine << _T("Release"),     rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table1a)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("ps")) << rptNewLine << _T("Release"),         rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   
   col = 0;
   (*table1b)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table1b)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("girder")) << rptNewLine << _T("Storage"),     rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table1b)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("ps")) << rptNewLine << _T("Storage"),         rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table1b)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("creep")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

   col = 0;
   (*table2)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("girder")) << rptNewLine << _T("Erected"),     rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("ps")) << rptNewLine << _T("Erected"),         rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("tpsr")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("diaphragm")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

   if ( bShearKey )
   {
      (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("shear key")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   }

   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("creep2")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("panels")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("deck")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("User1")) << _T(" = ") << rptNewLine << Sub2(symbol(DELTA),_T("UserDC")) << _T(" + ") << rptNewLine  << Sub2(symbol(DELTA),_T("UserDW")) , rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

   if ( bSidewalk )
   {
      (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("sidewalk")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   }

   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("barrier")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

   if ( !pBridge->IsFutureOverlay() )
   {
      (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("overlay")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   }

   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("User2")) << _T(" = ") << rptNewLine << Sub2(symbol(DELTA),_T("UserDC")) << _T(" + ") << rptNewLine  << Sub2(symbol(DELTA),_T("UserDW")) , rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

   col = 0;
   (*table3)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table3)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("1")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table3)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("2")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table3)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("3")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

   Float64 days = (constructionRate == CREEP_MINTIME ? pSpecEntry->GetCreepDuration2Min() : pSpecEntry->GetCreepDuration2Max());
   days = ::ConvertFromSysUnits(days,unitMeasure::Day);
   std::_tostringstream os;
   os << days;
   (*table3)(0,col++) << COLHDR(Sub2(_T("D"),os.str().c_str()) << _T(" = ") << Sub2(symbol(DELTA),_T("4")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table3)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("5")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table3)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("excess")) << _T(" = ") << rptNewLine << Sub2(symbol(DELTA),_T("6")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table3)(0,col++) << COLHDR(_T("C = ") << Sub2(symbol(DELTA),_T("4")) << _T(" - ") << Sub2(symbol(DELTA),_T("6")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

   pgsTypes::BridgeAnalysisType bat = pProduct->GetBridgeAnalysisType(pgsTypes::Minimize);

   // Fill up the tables
   RowIndexType row1a = table1a->GetNumberOfHeaderRows();
   RowIndexType row1b = table1b->GetNumberOfHeaderRows();
   RowIndexType row2 = table2->GetNumberOfHeaderRows();
   RowIndexType row3 = table3->GetNumberOfHeaderRows();
   
   std::vector<pgsPointOfInterest>::iterator releasePoiIter(vPoiRelease.begin());
   std::vector<pgsPointOfInterest>::iterator releasePoiIterEnd(vPoiRelease.end());
   std::vector<pgsPointOfInterest>::iterator storagePoiIter(vPoiStorage.begin());
   std::vector<pgsPointOfInterest>::iterator erectedPoiIter(vPoiErected.begin());
   for ( ; releasePoiIter != releasePoiIterEnd; releasePoiIter++, storagePoiIter++, erectedPoiIter++ )
   {
      const pgsPointOfInterest& releasePoi(*releasePoiIter);
      const pgsPointOfInterest& storagePoi(*storagePoiIter);
      const pgsPointOfInterest& erectedPoi(*erectedPoiIter);

      Float64 DpsRelease  = pProduct->GetDeflection(releaseIntervalIdx,pftPretension,releasePoi,bat,rtCumulative,false);
      Float64 DpsStorage  = pProduct->GetDeflection(storageIntervalIdx,pftPretension,storagePoi,bat,rtCumulative,false);
      Float64 DpsErected  = pProduct->GetDeflection(erectionIntervalIdx,pftPretension,erectedPoi,bat,rtCumulative,false);

      Float64 DgdrRelease = pProduct->GetDeflection(releaseIntervalIdx,pftGirder,releasePoi,bat,rtCumulative,false);
      Float64 DgdrStorage = pProduct->GetDeflection(storageIntervalIdx,pftGirder,storagePoi,bat,rtCumulative,false);
      Float64 DgdrErected = pProduct->GetDeflection(erectionIntervalIdx,pftGirder,erectedPoi,bat,rtCumulative,false);

      // NOTE: Get the creep deflection from the ICamber interface because it takes the construction rate 
      // into account. Getting creep deflection as a product load assumes the maximum construction rate
      //Float64 Dtpsr = pProduct->GetDeflection(tempStrandRemovalIntervalIdx,pftPretension,erectedPoi,bat,rtIncremental,false);
      Float64 Dtpsr = pCamber->GetReleaseTempPrestressDeflection( erectedPoi );

      Float64 Dcreep1    = pCamber->GetCreepDeflection( erectedPoi, ICamber::cpReleaseToDiaphragm, constructionRate );
      Float64 Ddiaphragm = pCamber->GetDiaphragmDeflection( erectedPoi );
      Float64 Ddeck      = pProduct->GetDeflection(castDeckIntervalIdx,pftSlab,erectedPoi,bat, rtCumulative, false);
              Ddeck     += pProduct->GetDeflection(castDeckIntervalIdx,pftSlabPad,erectedPoi,bat, rtCumulative, false);
      Float64 Dpanel     = pProduct->GetDeflection(castDeckIntervalIdx,pftSlabPanel,erectedPoi,bat, rtCumulative, false);
      Float64 Dshearkey  = pProduct->GetDeflection(castDeckIntervalIdx,pftShearKey,erectedPoi,bat, rtCumulative, false);
      Float64 Dcreep2    = pCamber->GetCreepDeflection( erectedPoi, ICamber::cpDiaphragmToDeck, constructionRate );
      Float64 Duser1     = pProduct->GetDeflection(castDeckIntervalIdx,pftUserDC,erectedPoi,bat, rtCumulative, false) 
                         + pProduct->GetDeflection(castDeckIntervalIdx,pftUserDW,erectedPoi,bat, rtCumulative, false);
      Float64 Duser2     = pProduct->GetDeflection(compositeDeckIntervalIdx,pftUserDC,erectedPoi,bat, rtCumulative, false) 
                         + pProduct->GetDeflection(compositeDeckIntervalIdx,pftUserDW,erectedPoi,bat, rtCumulative, false);
      Float64 Dbarrier   = pProduct->GetDeflection(railingSystemIntervalIdx,pftTrafficBarrier,erectedPoi,bat, rtCumulative, false);
      Float64 Dsidewalk  = pProduct->GetDeflection(railingSystemIntervalIdx,pftSidewalk,      erectedPoi,bat, rtCumulative, false);
      Float64 Doverlay   = (pBridge->HasOverlay() ? (pBridge->IsFutureOverlay() ? 0.0 : pProduct->GetDeflection(overlayIntervalIdx,pftOverlay,erectedPoi,bat, rtCumulative, false)) : 0.0);


      // Table 1a
      col = 0;
      (*table1a)(row1a,col++) << location.SetValue( POI_RELEASED_SEGMENT, releasePoi );
      (*table1a)(row1a,col++) << deflection.SetValue( DgdrRelease );
      (*table1a)(row1a,col++) << deflection.SetValue( DpsRelease );
      row1a++;

      // Table 1b
      col = 0;
      (*table1b)(row1b,col++) << location.SetValue( POI_STORAGE_SEGMENT, storagePoi );
      (*table1b)(row1b,col++) << deflection.SetValue( DgdrStorage );
      (*table1b)(row1b,col++) << deflection.SetValue( DpsStorage );
      (*table1b)(row1b,col++) << deflection.SetValue( Dcreep1 );
      row1b++;

      // Table 2
      col = 0;
      (*table2)(row2,col++) << location.SetValue( POI_ERECTED_SEGMENT, erectedPoi );
      (*table2)(row2,col++) << deflection.SetValue( DgdrErected );
      (*table2)(row2,col++) << deflection.SetValue( DpsErected );
      (*table2)(row2,col++) << deflection.SetValue( Dtpsr );
      (*table2)(row2,col++) << deflection.SetValue( Ddiaphragm );

      if ( bShearKey )
      {
         (*table2)(row2,col++) << deflection.SetValue(Dshearkey);
      }

      (*table2)(row2,col++) << deflection.SetValue( Dcreep2 );
      (*table2)(row2,col++) << deflection.SetValue( Dpanel );
      (*table2)(row2,col++) << deflection.SetValue( Ddeck );
      (*table2)(row2,col++) << deflection.SetValue( Duser1 );

      if ( bSidewalk )
      {
         (*table2)(row2,col++) << deflection.SetValue(Dsidewalk);
      }

      (*table2)(row2,col++) << deflection.SetValue( Dbarrier );
   
      if (!pBridge->IsFutureOverlay() )
      {
         (*table2)(row2,col++) << deflection.SetValue(Doverlay);
      }

      (*table2)(row2,col++) << deflection.SetValue( Duser2 );

      row2++;

      // Table 3
      col = 0;
#pragma Reminder("UPDATE: D1 should be based on deflections after erection")
      // PGSuper v2 used deflections at end of storage. Since RDP and TxDOT are
      // going to totally redo camber, make it match v2 and fix it later
      //Float64 D1 = DgdrErected + DpsErected;
      Float64 D1 = DgdrStorage + DpsStorage;
      Float64 D2 = D1 + Dcreep1;
      Float64 D3 = D2 + Ddiaphragm + Dshearkey + Dtpsr + Dpanel;
      Float64 D4 = D3 + Dcreep2;
      Float64 D5 = D4 + Ddeck + Duser1;
      Float64 D6 = D5 + Dsidewalk + Dbarrier + Doverlay + Duser2;

      (*table3)(row3,col++) << location.SetValue( POI_ERECTED_SEGMENT,erectedPoi );
      (*table3)(row3,col++) << deflection.SetValue( D1 );
      (*table3)(row3,col++) << deflection.SetValue( D2 );
      (*table3)(row3,col++) << deflection.SetValue( D3 );

      D4 = IsZero(D4) ? 0 : D4;
      if ( D4 < 0 )
      {
         (*table3)(row3,col++) << color(Red) << deflection.SetValue(D4) << color(Black);
      }
      else
      {
         (*table3)(row3,col++) << deflection.SetValue( D4 );
      }

      (*table3)(row3,col++) << deflection.SetValue( D5 );

      D6 = IsZero(D6) ? 0 : D6;
      if ( D6 < 0 )
      {
         (*table3)(row3,col++) << color(Red) << deflection.SetValue(D6) << color(Black);
      }
      else
      {
         (*table3)(row3,col++) << deflection.SetValue( D6 );
      }
      (*table3)(row3,col++) << deflection.SetValue( D4 - D6 );

      row3++;
   }


   *pTable1 = pLayoutTable;
   *pTable2 = table2;
   *pTable3 = table3;
}


void CCamberTable::Build_SIP(IBroker* pBroker,const CSegmentKey& segmentKey,
                             IEAFDisplayUnits* pDisplayUnits,Int16 constructionRate,
                             rptRcTable** pTable1,rptRcTable** pTable2,rptRcTable** pTable3) const
{
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   location.IncludeSpanAndGirder(segmentKey.groupIndex == ALL_GROUPS);
   INIT_UV_PROTOTYPE( rptLengthUnitValue, deflection, pDisplayUnits->GetDeflectionUnit(), false );

   GET_IFACE2(pBroker,ILibrary,pLib);
   GET_IFACE2(pBroker,ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());

   // Get the interface pointers we need
   GET_IFACE2(pBroker,IPointOfInterest,pPoi);
   std::vector<pgsPointOfInterest> vPoiRelease( pPoi->GetPointsOfInterest( segmentKey,POI_RELEASED_SEGMENT | POI_TENTH_POINTS) );
   std::vector<pgsPointOfInterest> vPoiStorage( pPoi->GetPointsOfInterest( segmentKey,POI_STORAGE_SEGMENT | POI_TENTH_POINTS ) );
   std::vector<pgsPointOfInterest> vPoiErected( pPoi->GetPointsOfInterest( segmentKey,POI_ERECTED_SEGMENT | POI_TENTH_POINTS ) );

   ATLASSERT(vPoiRelease.size() == vPoiStorage.size() && vPoiStorage.size() == vPoiErected.size());

   GET_IFACE2(pBroker,ICamber,pCamber);
   GET_IFACE2(pBroker,IProductForces,pProduct);
   GET_IFACE2(pBroker,IProductLoads,pProductLoads);
   GET_IFACE2(pBroker,IBridge,pBridge);

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx       = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType storageIntervalIdx       = pIntervals->GetStorageInterval(segmentKey);
   IntervalIndexType erectionIntervalIdx      = pIntervals->GetErectSegmentInterval(segmentKey);
   IntervalIndexType castDeckIntervalIdx      = pIntervals->GetCastDeckInterval();
   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval();
   IntervalIndexType railingSystemIntervalIdx = pIntervals->GetInstallRailingSystemInterval();
   IntervalIndexType overlayIntervalIdx       = pIntervals->GetOverlayInterval();

   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();

   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   bool bSidewalk = pProductLoads->HasSidewalkLoad(segmentKey);
   bool bShearKey = pProductLoads->HasShearKeyLoad(segmentKey);

   // create the tables
   rptRcTable* table1a;
   rptRcTable* table1b;
   rptRcTable* table2;
   rptRcTable* table3;

   rptRcTable* pLayoutTable = pgsReportStyleHolder::CreateLayoutTable(2,_T("Camber - Part 1 (upwards is positive)"));

   table1a = pgsReportStyleHolder::CreateDefaultTable(3);
   table1b = pgsReportStyleHolder::CreateDefaultTable(4);
   (*pLayoutTable)(0,0) << table1a;
   (*pLayoutTable)(0,1) << table1b;

   table2 = pgsReportStyleHolder::CreateDefaultTable(9 + (bSidewalk ? 1 : 0) + (!pBridge->IsFutureOverlay() ? 1 : 0) + (bShearKey ? 1 : 0),_T("Camber - Part 2 (upwards is positive)"));
   table3 = pgsReportStyleHolder::CreateDefaultTable(6,_T("Camber - Part 3 (upwards is positive)"));

   if ( segmentKey.groupIndex == ALL_GROUPS )
   {
      table1a->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table1a->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

      table1b->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table1b->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

      table2->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table2->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

      table3->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table3->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   // Setup table headings
   ColumnIndexType col = 0;
   (*table1a)(0,col++) << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table1a)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("girder")) << rptNewLine << _T("Release"),     rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table1a)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("ps")) << rptNewLine << _T("Release"),         rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   
   col = 0;
   (*table1b)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table1b)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("girder")) << rptNewLine << _T("Storage"),     rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table1b)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("ps")) << rptNewLine << _T("Storage"),         rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table1b)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("creep")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

   col = 0;
   (*table2)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("girder")) << rptNewLine << _T("Erected"),     rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("ps")) << rptNewLine << _T("Erected"),         rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("diaphragm")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

   if ( bShearKey )
   {
      (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("shear key")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   }

   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("panel")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("deck")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("User1")) << _T(" = ") << rptNewLine << Sub2(symbol(DELTA),_T("UserDC")) << _T(" + ") << rptNewLine  << Sub2(symbol(DELTA),_T("UserDW")) , rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

   if ( bSidewalk )
   {
      (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("sidewalk")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   }

   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("barrier")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

   if ( !pBridge->IsFutureOverlay() )
   {
      (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("overlay")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   }

   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("User2")) << _T(" = ") << rptNewLine << Sub2(symbol(DELTA),_T("UserDC")) << _T(" + ") << rptNewLine  << Sub2(symbol(DELTA),_T("UserDW")) , rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

   col = 0;
   (*table3)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table3)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("1")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

   Float64 days = (constructionRate == CREEP_MINTIME ? pSpecEntry->GetCreepDuration2Min() : pSpecEntry->GetCreepDuration2Max());
   days = ::ConvertFromSysUnits(days,unitMeasure::Day);
   std::_tostringstream os;
   os << days;
   (*table3)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("2")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table3)(0,col++) << COLHDR(Sub2(_T("D"),os.str().c_str()) << _T(" = ") << Sub2(symbol(DELTA),_T("3")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table3)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("excess")) << _T(" = ") << rptNewLine << Sub2(symbol(DELTA),_T("4")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table3)(0,col++) << COLHDR(_T("C = ") << Sub2(symbol(DELTA),_T("2")) << _T(" - ") << Sub2(symbol(DELTA),_T("4")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

   pgsTypes::BridgeAnalysisType bat = pProduct->GetBridgeAnalysisType(pgsTypes::Minimize);

   // Fill up the tables
   RowIndexType row1a = table1a->GetNumberOfHeaderRows();
   RowIndexType row1b = table1b->GetNumberOfHeaderRows();
   RowIndexType row2 = table2->GetNumberOfHeaderRows();
   RowIndexType row3 = table3->GetNumberOfHeaderRows();
   
   std::vector<pgsPointOfInterest>::iterator releasePoiIter(vPoiRelease.begin());
   std::vector<pgsPointOfInterest>::iterator releasePoiIterEnd(vPoiRelease.end());
   std::vector<pgsPointOfInterest>::iterator storagePoiIter(vPoiStorage.begin());
   std::vector<pgsPointOfInterest>::iterator erectedPoiIter(vPoiErected.begin());
   for ( ; releasePoiIter != releasePoiIterEnd; releasePoiIter++, storagePoiIter++, erectedPoiIter++ )
   {
      const pgsPointOfInterest& releasePoi(*releasePoiIter);
      const pgsPointOfInterest& storagePoi(*storagePoiIter);
      const pgsPointOfInterest& erectedPoi(*erectedPoiIter);

      Float64 DpsRelease  = pProduct->GetDeflection(releaseIntervalIdx,pftPretension,releasePoi,bat,rtCumulative,false);
      Float64 DpsStorage  = pProduct->GetDeflection(storageIntervalIdx,pftPretension,storagePoi,bat,rtCumulative,false);
      Float64 DpsErected  = pProduct->GetDeflection(erectionIntervalIdx,pftPretension,erectedPoi,bat,rtCumulative,false);

      Float64 DgdrRelease = pProduct->GetDeflection(releaseIntervalIdx,pftGirder,releasePoi,bat,rtCumulative,false);
      Float64 DgdrStorage = pProduct->GetDeflection(storageIntervalIdx,pftGirder,storagePoi,bat,rtCumulative,false);
      Float64 DgdrErected = pProduct->GetDeflection(erectionIntervalIdx,pftGirder,erectedPoi,bat,rtCumulative,false);

      Float64 Dcreep     = pCamber->GetCreepDeflection( erectedPoi, ICamber::cpReleaseToDeck, constructionRate );
      Float64 Ddiaphragm = pProduct->GetDeflection(castDeckIntervalIdx,pftDiaphragm,erectedPoi,bat, rtCumulative, false);
      Float64 Dshearkey  = pProduct->GetDeflection(castDeckIntervalIdx,pftShearKey,erectedPoi,bat, rtCumulative, false);
      Float64 Ddeck      = pProduct->GetDeflection(castDeckIntervalIdx,pftSlab,erectedPoi,bat, rtCumulative, false);
              Ddeck     += pProduct->GetDeflection(castDeckIntervalIdx,pftSlabPad,erectedPoi,bat, rtCumulative, false);
      Float64 Dpanel     = pProduct->GetDeflection(castDeckIntervalIdx,pftSlabPanel,erectedPoi,bat, rtCumulative, false);
      Float64 Duser1     = pProduct->GetDeflection(castDeckIntervalIdx,pftUserDC,erectedPoi,bat, rtCumulative, false) 
                         + pProduct->GetDeflection(castDeckIntervalIdx,pftUserDW,erectedPoi,bat, rtCumulative, false);
      Float64 Duser2     = pProduct->GetDeflection(compositeDeckIntervalIdx,pftUserDC,erectedPoi,bat, rtCumulative, false) 
                         + pProduct->GetDeflection(compositeDeckIntervalIdx,pftUserDW,erectedPoi,bat, rtCumulative, false);
      Float64 Dbarrier   = pProduct->GetDeflection(railingSystemIntervalIdx,pftTrafficBarrier,erectedPoi,bat, rtCumulative, false);
      Float64 Dsidewalk  = pProduct->GetDeflection(railingSystemIntervalIdx,pftSidewalk,      erectedPoi,bat, rtCumulative, false);
      Float64 Doverlay   = (pBridge->HasOverlay() ? (pBridge->IsFutureOverlay() ? 0.0 : pProduct->GetDeflection(overlayIntervalIdx,pftOverlay,erectedPoi,bat, rtCumulative, false)) : 0.0);


      // Table 1a
      col = 0;
      (*table1a)(row1a,col++) << location.SetValue( POI_RELEASED_SEGMENT, releasePoi );
      (*table1a)(row1a,col++) << deflection.SetValue( DgdrRelease );
      (*table1a)(row1a,col++) << deflection.SetValue( DpsRelease );
      row1a++;

      // Table 1b
      col = 0;
      (*table1b)(row1b,col++) << location.SetValue( POI_STORAGE_SEGMENT, storagePoi );
      (*table1b)(row1b,col++) << deflection.SetValue( DgdrStorage );
      (*table1b)(row1b,col++) << deflection.SetValue( DpsStorage );
      (*table1b)(row1b,col++) << deflection.SetValue( Dcreep );
      row1b++;

      // Table 2
      col = 0;
      (*table2)(row2,col++) << location.SetValue( POI_ERECTED_SEGMENT, erectedPoi );
      (*table2)(row2,col++) << deflection.SetValue( DgdrErected );
      (*table2)(row2,col++) << deflection.SetValue( DpsErected );
      (*table2)(row2,col++) << deflection.SetValue( Ddiaphragm );

      if ( bShearKey )
      {
         (*table2)(row2,col++) << deflection.SetValue( Dshearkey);
      }

      (*table2)(row2,col++) << deflection.SetValue( Dpanel );
      (*table2)(row2,col++) << deflection.SetValue( Ddeck );
      (*table2)(row2,col++) << deflection.SetValue( Duser1 );

      if ( bSidewalk )
      {
         (*table2)(row2,col++) << deflection.SetValue( Dsidewalk);
      }

      (*table2)(row2,col++) << deflection.SetValue( Dbarrier );

      if (!pBridge->IsFutureOverlay() )
      {
         (*table2)(row2,col++) << deflection.SetValue(Doverlay);
      }

      (*table2)(row2,col++) << deflection.SetValue( Duser2 );

      row2++;

      // Table 3
      col = 0;
#pragma Reminder("UPDATE: D1 should be based on deflections after erection")
      // PGSuper v2 used deflections at end of storage. Since RDP and TxDOT are
      // going to totally redo camber, make it match v2 and fix it later
      //Float64 D1 = DgdrErected + DpsErected;
      Float64 D1 = DgdrStorage + DpsStorage;
      Float64 D2 = D1 + Dcreep;
      Float64 D3 = D2 + Ddiaphragm + Dshearkey + Dpanel;
      Float64 D4 = D3 + Ddeck + Duser1 + Dsidewalk + Dbarrier + Doverlay + Duser2;

      (*table3)(row3,col++) << location.SetValue( POI_ERECTED_SEGMENT, erectedPoi );
      (*table3)(row3,col++) << deflection.SetValue( D1 );

      D2 = IsZero(D2) ? 0 : D2;
      if ( D2 < 0 )
      {
         (*table3)(row3,col++) << color(Red) << deflection.SetValue(D2) << color(Black);
      }
      else
      {
         (*table3)(row3,col++) << deflection.SetValue( D2 );
      }

      (*table3)(row3,col++) << deflection.SetValue( D3 );

      D4 = IsZero(D4) ? 0 : D4;
      if ( D4 < 0 )
      {
         (*table3)(row3,col++) << color(Red) << deflection.SetValue(D4) << color(Black);
      }
      else
      {
         (*table3)(row3,col++) << deflection.SetValue( D4 );
      }
      (*table3)(row3,col++) << deflection.SetValue( D2 - D4 );

      row3++;
   }

   *pTable1 = pLayoutTable;
   *pTable2 = table2;
   *pTable3 = table3;
}

void CCamberTable::Build_NoDeck_TempStrands(IBroker* pBroker,const CSegmentKey& segmentKey,
                                            IEAFDisplayUnits* pDisplayUnits,Int16 constructionRate,
                                            rptRcTable** pTable1,rptRcTable** pTable2,rptRcTable** pTable3) const
{
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   location.IncludeSpanAndGirder(segmentKey.groupIndex == ALL_GROUPS);
   INIT_UV_PROTOTYPE( rptLengthUnitValue, deflection, pDisplayUnits->GetDeflectionUnit(), false );

   GET_IFACE2(pBroker,ILibrary,pLib);
   GET_IFACE2(pBroker,ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());

   // Get the interface pointers we need
   GET_IFACE2(pBroker,IPointOfInterest,pPoi);
   std::vector<pgsPointOfInterest> vPoiRelease( pPoi->GetPointsOfInterest( segmentKey,POI_RELEASED_SEGMENT | POI_TENTH_POINTS) );
   std::vector<pgsPointOfInterest> vPoiStorage( pPoi->GetPointsOfInterest( segmentKey,POI_STORAGE_SEGMENT | POI_TENTH_POINTS ) );
   std::vector<pgsPointOfInterest> vPoiErected( pPoi->GetPointsOfInterest( segmentKey,POI_ERECTED_SEGMENT | POI_TENTH_POINTS ) );

   ATLASSERT(vPoiRelease.size() == vPoiStorage.size() && vPoiStorage.size() == vPoiErected.size());

   GET_IFACE2(pBroker,ICamber,pCamber);
   GET_IFACE2(pBroker,IProductLoads,pProductLoads);
   GET_IFACE2(pBroker,IProductForces,pProduct);
   GET_IFACE2(pBroker,IBridge,pBridge);

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx       = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType storageIntervalIdx       = pIntervals->GetStorageInterval(segmentKey);
   IntervalIndexType erectionIntervalIdx      = pIntervals->GetErectSegmentInterval(segmentKey);
   IntervalIndexType tempStrandRemovalIntervalIdx = pIntervals->GetTemporaryStrandRemovalInterval(segmentKey);
   IntervalIndexType castDeckIntervalIdx      = pIntervals->GetCastDeckInterval();
   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval();
   IntervalIndexType railingSystemIntervalIdx = pIntervals->GetInstallRailingSystemInterval();
   IntervalIndexType overlayIntervalIdx       = pIntervals->GetOverlayInterval();

   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();

   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   bool bSidewalk = pProductLoads->HasSidewalkLoad(segmentKey);
   bool bShearKey = pProductLoads->HasShearKeyLoad(segmentKey);

   // create the tables
   // create the tables
   rptRcTable* table1a;
   rptRcTable* table1b;
   rptRcTable* table2;
   rptRcTable* table3;

   rptRcTable* pLayoutTable = pgsReportStyleHolder::CreateLayoutTable(2,_T("Camber - Part 1 (upwards is positive)"));
   
   table1a = pgsReportStyleHolder::CreateDefaultTable(3);
   table1b = pgsReportStyleHolder::CreateDefaultTable(4);
   (*pLayoutTable)(0,0) << table1a;
   (*pLayoutTable)(0,1) << table1b;

   table2 = pgsReportStyleHolder::CreateDefaultTable(10 + (bSidewalk ? 1 : 0) + (!pBridge->IsFutureOverlay() ? 1 : 0) + (bShearKey ? 1 : 0),_T("Camber - Part 2 (upwards is positive)"));
   table3 = pgsReportStyleHolder::CreateDefaultTable(8,_T("Camber - Part 3 (upwards is positive)"));

   if ( segmentKey.groupIndex == ALL_GROUPS )
   {
      table1a->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table1a->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

      table1b->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table1b->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

      table2->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table2->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

      table3->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table3->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   // Setup table headings
   ColumnIndexType col = 0;
   (*table1a)(0,col++) << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table1a)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("girder")) << rptNewLine << _T("Release"),     rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table1a)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("ps")) << rptNewLine << _T("Release"),         rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   
   col = 0;
   (*table1b)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table1b)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("girder")) << rptNewLine << _T("Storage"),     rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table1b)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("ps")) << rptNewLine << _T("Storage"),         rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table1b)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("creep1")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

   col = 0;
   (*table2)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("girder")) << rptNewLine << _T("Erected"),     rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("ps")) << rptNewLine << _T("Erected"),         rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("tpsr")),         rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("diaphragm")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

   if ( bShearKey )
   {
      (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("shear key")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   }

   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("creep2")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("deck")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("User1")) << _T(" = ") << rptNewLine << Sub2(symbol(DELTA),_T("UserDC")) << _T(" + ") << rptNewLine  << Sub2(symbol(DELTA),_T("UserDW")) , rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

   if (bSidewalk)
   {
      (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("sidewalk")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   }

   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("barrier")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

   if ( !pBridge->IsFutureOverlay() )
   {
      (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("overlay")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   }

   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("User2")) << _T(" = ") << rptNewLine << Sub2(symbol(DELTA),_T("UserDC")) << _T(" + ") << rptNewLine  << Sub2(symbol(DELTA),_T("UserDW")) , rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("creep3")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

   col = 0;
   (*table3)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table3)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("1")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table3)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("2")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table3)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("3")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   
   Float64 days = (constructionRate == CREEP_MINTIME ? pSpecEntry->GetCreepDuration2Min() : pSpecEntry->GetCreepDuration2Max());
   days = ::ConvertFromSysUnits(days,unitMeasure::Day);
   std::_tostringstream os;
   os << days;
   (*table3)(0,col++) << COLHDR(Sub2(_T("D"),os.str().c_str()) << _T(" = ") << Sub2(symbol(DELTA),_T("4")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table3)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("5")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table3)(0,col++) << COLHDR(_T("C = ") << Sub2(symbol(DELTA),_T("excess")) << _T(" = ") << rptNewLine << Sub2(symbol(DELTA),_T("6")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

   pgsTypes::BridgeAnalysisType bat = pProduct->GetBridgeAnalysisType(pgsTypes::Minimize);

   // Fill up the tables
   RowIndexType row1a = table1a->GetNumberOfHeaderRows();
   RowIndexType row1b = table1b->GetNumberOfHeaderRows();
   RowIndexType row2 = table2->GetNumberOfHeaderRows();
   RowIndexType row3 = table3->GetNumberOfHeaderRows();
   std::vector<pgsPointOfInterest>::iterator releasePoiIter(vPoiRelease.begin());
   std::vector<pgsPointOfInterest>::iterator releasePoiIterEnd(vPoiRelease.end());
   std::vector<pgsPointOfInterest>::iterator storagePoiIter(vPoiStorage.begin());
   std::vector<pgsPointOfInterest>::iterator erectedPoiIter(vPoiErected.begin());
   for ( ; releasePoiIter != releasePoiIterEnd; releasePoiIter++, storagePoiIter++, erectedPoiIter++ )
   {
      const pgsPointOfInterest& releasePoi(*releasePoiIter);
      const pgsPointOfInterest& storagePoi(*storagePoiIter);
      const pgsPointOfInterest& erectedPoi(*erectedPoiIter);

      Float64 DpsRelease  = pProduct->GetDeflection(releaseIntervalIdx,pftPretension,releasePoi,bat,rtCumulative,false);
      Float64 DpsStorage  = pProduct->GetDeflection(storageIntervalIdx,pftPretension,storagePoi,bat,rtCumulative,false);
      Float64 DpsErected  = pProduct->GetDeflection(erectionIntervalIdx,pftPretension,erectedPoi,bat,rtCumulative,false);

      Float64 DgdrRelease = pProduct->GetDeflection(releaseIntervalIdx,pftGirder,releasePoi,bat,rtCumulative,false);
      Float64 DgdrStorage = pProduct->GetDeflection(storageIntervalIdx,pftGirder,storagePoi,bat,rtCumulative,false);
      Float64 DgdrErected = pProduct->GetDeflection(erectionIntervalIdx,pftGirder,erectedPoi,bat,rtCumulative,false);

      //Float64 Dtpsr = pProduct->GetDeflection(tempStrandRemovalIntervalIdx,pftPretension,erectedPoi,bat,rtIncremental,false);
      Float64 Dtpsr = pCamber->GetReleaseTempPrestressDeflection( erectedPoi );

      Float64 Dcreep1    = pCamber->GetCreepDeflection( erectedPoi, ICamber::cpReleaseToDiaphragm, constructionRate );
      Float64 Ddiaphragm = pCamber->GetDiaphragmDeflection( erectedPoi );
      Float64 Dshearkey  = pProduct->GetDeflection(castDeckIntervalIdx,pftShearKey,erectedPoi,bat, rtCumulative, false);
      Float64 Ddeck      = pProduct->GetDeflection(castDeckIntervalIdx,pftSlab,erectedPoi,bat, rtCumulative, false);
              Ddeck     += pProduct->GetDeflection(castDeckIntervalIdx,pftSlabPad,erectedPoi,bat, rtCumulative, false);
      Float64 Dcreep2    = pCamber->GetCreepDeflection( erectedPoi, ICamber::cpDiaphragmToDeck, constructionRate );
      Float64 Duser1     = pProduct->GetDeflection(castDeckIntervalIdx,pftUserDC,erectedPoi,bat, rtCumulative, false) 
                         + pProduct->GetDeflection(castDeckIntervalIdx,pftUserDW,erectedPoi,bat, rtCumulative, false);
      Float64 Duser2     = pProduct->GetDeflection(compositeDeckIntervalIdx,pftUserDC,erectedPoi,bat, rtCumulative, false) 
                         + pProduct->GetDeflection(compositeDeckIntervalIdx,pftUserDW,erectedPoi,bat, rtCumulative, false);
      Float64 Dsidewalk  = pProduct->GetDeflection(railingSystemIntervalIdx,pftSidewalk,      erectedPoi,bat, rtCumulative, false);
      Float64 Dbarrier   = pProduct->GetDeflection(railingSystemIntervalIdx,pftTrafficBarrier,erectedPoi,bat, rtCumulative, false);
      Float64 Doverlay   = (pBridge->HasOverlay() ? (pBridge->IsFutureOverlay() ? 0.0 : pProduct->GetDeflection(overlayIntervalIdx,pftOverlay,erectedPoi,bat, rtCumulative, false)) : 0.0);
      Float64 Dcreep3    = pCamber->GetCreepDeflection( erectedPoi, ICamber::cpDeckToFinal, constructionRate );

      // Table 1a
      col = 0;
      (*table1a)(row1a,col++) << location.SetValue( POI_RELEASED_SEGMENT, releasePoi );
      (*table1a)(row1a,col++) << deflection.SetValue( DgdrRelease );
      (*table1a)(row1a,col++) << deflection.SetValue( DpsRelease );
      row1a++;

      // Table 1b
      col = 0;
      (*table1b)(row1b,col++) << location.SetValue( POI_STORAGE_SEGMENT, storagePoi );
      (*table1b)(row1b,col++) << deflection.SetValue( DgdrStorage );
      (*table1b)(row1b,col++) << deflection.SetValue( DpsStorage );
      (*table1b)(row1b,col++) << deflection.SetValue( Dcreep1 );
      row1b++;

      // Table 2
      col = 0;
      (*table2)(row2,col++) << location.SetValue( POI_ERECTED_SEGMENT, erectedPoi );
      (*table2)(row2,col++) << deflection.SetValue( DgdrErected );
      (*table2)(row2,col++) << deflection.SetValue( DpsErected );
      (*table2)(row2,col++) << deflection.SetValue( Dtpsr );
      (*table2)(row2,col++) << deflection.SetValue( Ddiaphragm );

      if (bShearKey)
      (*table2)(row2,col++) << deflection.SetValue( Dshearkey );

      (*table2)(row2,col++) << deflection.SetValue( Dcreep2 );
      (*table2)(row2,col++) << deflection.SetValue( Ddeck );
      (*table2)(row2,col++) << deflection.SetValue( Duser1 );

      if (bSidewalk)
      {
         (*table2)(row2,col++) << deflection.SetValue(Dsidewalk);
      }

      (*table2)(row2,col++) << deflection.SetValue( Dbarrier );

      if (!pBridge->IsFutureOverlay())
      {
         (*table2)(row2,col++) << deflection.SetValue(Doverlay);
      }

      (*table2)(row2,col++) << deflection.SetValue( Duser2 );
      (*table2)(row2,col++) << deflection.SetValue( Dcreep3 );

      row2++;

      // Table 3
      col = 0;
#pragma Reminder("UPDATE: D1 should be based on deflections after erection")
      // PGSuper v2 used deflections at end of storage. Since RDP and TxDOT are
      // going to totally redo camber, make it match v2 and fix it later
      //Float64 D1 = DgdrErected + DpsErected;
      Float64 D1 = DgdrStorage + DpsStorage;
      Float64 D2 = D1 + Dcreep1;
      Float64 D3 = D2 + Ddiaphragm + Dshearkey + Dtpsr + Duser1;
      Float64 D4 = D3 + Dcreep2;
      Float64 D5 = D4 + Dsidewalk + Dbarrier + Doverlay + Duser2;
      Float64 D6 = D5 + Dcreep3;

      (*table3)(row3,col++) << location.SetValue( POI_ERECTED_SEGMENT,erectedPoi );
      (*table3)(row3,col++) << deflection.SetValue( D1 );
      (*table3)(row3,col++) << deflection.SetValue( D2 );
      (*table3)(row3,col++) << deflection.SetValue( D3 );

      D4 = IsZero(D4) ? 0 : D4;
      if ( D4 < 0 )
      {
         (*table3)(row3,col++) << color(Red) << deflection.SetValue(D4) << color(Black);
      }
      else
      {
         (*table3)(row3,col++) << deflection.SetValue( D4 );
      }
      
      (*table3)(row3,col++) << deflection.SetValue( D5 );

      D6 = IsZero(D6) ? 0 : D6;
      if ( D6 < 0 )
      {
         (*table3)(row3,col++) << color(Red) << deflection.SetValue(D6) << color(Black);
      }
      else
      {
         (*table3)(row3,col++) << deflection.SetValue( D6 );
      }

      row3++;
   }

   *pTable1 = pLayoutTable;
   *pTable2 = table2;
   *pTable3 = table3;
}

void CCamberTable::Build_NoDeck(IBroker* pBroker,const CSegmentKey& segmentKey,
                                IEAFDisplayUnits* pDisplayUnits,Int16 constructionRate,
                                rptRcTable** pTable1,rptRcTable** pTable2,rptRcTable** pTable3) const
{
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   location.IncludeSpanAndGirder(segmentKey.groupIndex == ALL_GROUPS);
   INIT_UV_PROTOTYPE( rptLengthUnitValue, deflection, pDisplayUnits->GetDeflectionUnit(), false );

   GET_IFACE2(pBroker,ILibrary,pLib);
   GET_IFACE2(pBroker,ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());

   // Get the interface pointers we need
   GET_IFACE2(pBroker,IPointOfInterest,pPoi);
   std::vector<pgsPointOfInterest> vPoiRelease( pPoi->GetPointsOfInterest( segmentKey,POI_RELEASED_SEGMENT | POI_TENTH_POINTS) );
   std::vector<pgsPointOfInterest> vPoiStorage( pPoi->GetPointsOfInterest( segmentKey,POI_STORAGE_SEGMENT | POI_TENTH_POINTS ) );
   std::vector<pgsPointOfInterest> vPoiErected( pPoi->GetPointsOfInterest( segmentKey,POI_ERECTED_SEGMENT | POI_TENTH_POINTS ) );

   ATLASSERT(vPoiRelease.size() == vPoiStorage.size() && vPoiStorage.size() == vPoiErected.size());

   GET_IFACE2(pBroker,ICamber,pCamber);
   GET_IFACE2(pBroker,IProductForces,pProduct);
   GET_IFACE2(pBroker,IProductLoads,pProductLoads);
   GET_IFACE2(pBroker,IBridge,pBridge);

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx       = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType storageIntervalIdx       = pIntervals->GetStorageInterval(segmentKey);
   IntervalIndexType erectionIntervalIdx      = pIntervals->GetErectSegmentInterval(segmentKey);
   IntervalIndexType castDeckIntervalIdx      = pIntervals->GetCastDeckInterval();
   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval();
   IntervalIndexType railingSystemIntervalIdx = pIntervals->GetInstallRailingSystemInterval();
   IntervalIndexType overlayIntervalIdx       = pIntervals->GetOverlayInterval();

   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();

   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   bool bSidewalk = pProductLoads->HasSidewalkLoad(segmentKey);
   bool bShearKey = pProductLoads->HasShearKeyLoad(segmentKey);

   // create the tables
   rptRcTable* table1a;
   rptRcTable* table1b;
   rptRcTable* table2;
   rptRcTable* table3;

   rptRcTable* pLayoutTable = pgsReportStyleHolder::CreateLayoutTable(2,_T("Camber - Part 1 (upwards is positive)"));
   
   table1a = pgsReportStyleHolder::CreateDefaultTable(3);
   table1b = pgsReportStyleHolder::CreateDefaultTable(4);
   (*pLayoutTable)(0,0) << table1a;
   (*pLayoutTable)(0,1) << table1b;

   table2 = pgsReportStyleHolder::CreateDefaultTable(9 + (bSidewalk ? 1 : 0) + (!pBridge->IsFutureOverlay() ? 1 : 0) + (bShearKey ? 1 : 0),_T("Camber - Part 2 (upwards is positive)"));
   table3 = pgsReportStyleHolder::CreateDefaultTable(7,_T("Camber - Part 3 (upwards is positive)"));

   if ( segmentKey.groupIndex == ALL_GROUPS )
   {
      table1a->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table1a->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

      table1b->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table1b->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

      table2->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table2->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

      table3->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table3->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   // Setup table headings
   ColumnIndexType col = 0;
   (*table1a)(0,col++) << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table1a)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("girder")) << rptNewLine << _T("Release"),     rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table1a)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("ps")) << rptNewLine << _T("Release"),         rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   
   col = 0;
   (*table1b)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table1b)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("girder")) << rptNewLine << _T("Storage"),     rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table1b)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("ps")) << rptNewLine << _T("Storage"),         rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table1b)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("creep1")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

   col = 0;
   (*table2)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("girder")) << rptNewLine << _T("Erected"),     rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("ps")) << rptNewLine << _T("Erected"),         rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("diaphragm")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

   if ( bShearKey )
   {
      (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("shear key")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   }

   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("User1")) << _T(" = ") << rptNewLine << Sub2(symbol(DELTA),_T("UserDC")) << _T(" + ") << rptNewLine  << Sub2(symbol(DELTA),_T("UserDW")) , rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("creep2")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

   if ( bSidewalk )
   {
      (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("sidewalk")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   }

   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("barrier")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

   if ( !pBridge->IsFutureOverlay() )
   {
      (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("overlay")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   }

   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("User2")) << _T(" = ") << rptNewLine << Sub2(symbol(DELTA),_T("UserDC")) << _T(" + ") << rptNewLine  << Sub2(symbol(DELTA),_T("UserDW")) , rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("creep3")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

   col = 0;
   (*table3)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table3)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("1")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table3)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("2")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table3)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("3")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

   Float64 days = (constructionRate == CREEP_MINTIME ? pSpecEntry->GetCreepDuration2Min() : pSpecEntry->GetCreepDuration2Max());
   days = ::ConvertFromSysUnits(days,unitMeasure::Day);
   std::_tostringstream os;
   os << days;
   (*table3)(0,col++) << COLHDR(Sub2(_T("D"),os.str().c_str()) << _T(" = ") << Sub2(symbol(DELTA),_T("4")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table3)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("5")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table3)(0,col++) << COLHDR(_T("C = ") << Sub2(symbol(DELTA),_T("excess")) << _T(" = ") << rptNewLine << Sub2(symbol(DELTA),_T("6")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

   pgsTypes::BridgeAnalysisType bat = pProduct->GetBridgeAnalysisType(pgsTypes::Minimize);

   // Fill up the tables
   RowIndexType row1a = table1a->GetNumberOfHeaderRows();
   RowIndexType row1b = table1b->GetNumberOfHeaderRows();
   RowIndexType row2 = table2->GetNumberOfHeaderRows();
   RowIndexType row3 = table3->GetNumberOfHeaderRows();
   std::vector<pgsPointOfInterest>::iterator releasePoiIter(vPoiRelease.begin());
   std::vector<pgsPointOfInterest>::iterator releasePoiIterEnd(vPoiRelease.end());
   std::vector<pgsPointOfInterest>::iterator storagePoiIter(vPoiStorage.begin());
   std::vector<pgsPointOfInterest>::iterator erectedPoiIter(vPoiErected.begin());
   for ( ; releasePoiIter != releasePoiIterEnd; releasePoiIter++, storagePoiIter++, erectedPoiIter++ )
   {
      const pgsPointOfInterest& releasePoi(*releasePoiIter);
      const pgsPointOfInterest& storagePoi(*storagePoiIter);
      const pgsPointOfInterest& erectedPoi(*erectedPoiIter);

      Float64 DpsRelease  = pProduct->GetDeflection(releaseIntervalIdx,pftPretension,releasePoi,bat,rtCumulative,false);
      Float64 DpsStorage  = pProduct->GetDeflection(storageIntervalIdx,pftPretension,storagePoi,bat,rtCumulative,false);
      Float64 DpsErected  = pProduct->GetDeflection(erectionIntervalIdx,pftPretension,erectedPoi,bat,rtCumulative,false);

      Float64 DgdrRelease = pProduct->GetDeflection(releaseIntervalIdx,pftGirder,releasePoi,bat,rtCumulative,false);
      Float64 DgdrStorage = pProduct->GetDeflection(storageIntervalIdx,pftGirder,storagePoi,bat,rtCumulative,false);
      Float64 DgdrErected = pProduct->GetDeflection(erectionIntervalIdx,pftGirder,erectedPoi,bat,rtCumulative,false);

      Float64 Dcreep1    = pCamber->GetCreepDeflection( erectedPoi, ICamber::cpReleaseToDiaphragm, constructionRate );
      Float64 Ddiaphragm = pCamber->GetDiaphragmDeflection( erectedPoi );
      Float64 Dshearkey  = pProduct->GetDeflection(castDeckIntervalIdx,pftShearKey,      erectedPoi,bat, rtCumulative, false);
      Float64 Dcreep2    = pCamber->GetCreepDeflection( erectedPoi, ICamber::cpDiaphragmToDeck, constructionRate );
      Float64 Duser1     = pProduct->GetDeflection(castDeckIntervalIdx,pftUserDC,erectedPoi,bat, rtCumulative, false) 
                         + pProduct->GetDeflection(castDeckIntervalIdx,pftUserDW,erectedPoi,bat, rtCumulative, false);
      Float64 Duser2     = pProduct->GetDeflection(compositeDeckIntervalIdx,pftUserDC,erectedPoi,bat, rtCumulative, false) 
                         + pProduct->GetDeflection(compositeDeckIntervalIdx,pftUserDW,erectedPoi,bat, rtCumulative, false);
      Float64 Dsidewalk  = pProduct->GetDeflection(railingSystemIntervalIdx,pftSidewalk,      erectedPoi,bat, rtCumulative, false);
      Float64 Dbarrier   = pProduct->GetDeflection(railingSystemIntervalIdx,pftTrafficBarrier,erectedPoi,bat, rtCumulative, false);
      Float64 Doverlay   = (pBridge->HasOverlay() ? (pBridge->IsFutureOverlay() ? 0.0 : pProduct->GetDeflection(overlayIntervalIdx,pftOverlay,erectedPoi,bat, rtCumulative, false)) : 0.0);
      Float64 Dcreep3    = pCamber->GetCreepDeflection( erectedPoi, ICamber::cpDeckToFinal, constructionRate );

      // Table 1a
      col = 0;
      (*table1a)(row1a,col++) << location.SetValue( POI_RELEASED_SEGMENT, releasePoi );
      (*table1a)(row1a,col++) << deflection.SetValue( DgdrRelease );
      (*table1a)(row1a,col++) << deflection.SetValue( DpsRelease );
      row1a++;

      // Table 1b
      col = 0;
      (*table1b)(row1b,col++) << location.SetValue( POI_STORAGE_SEGMENT, storagePoi );
      (*table1b)(row1b,col++) << deflection.SetValue( DgdrStorage );
      (*table1b)(row1b,col++) << deflection.SetValue( DpsStorage );
      (*table1b)(row1b,col++) << deflection.SetValue( Dcreep1 );
      row1b++;

      // Table 2
      col = 0;
      (*table2)(row2,col++) << location.SetValue( POI_ERECTED_SEGMENT, erectedPoi );
      (*table2)(row2,col++) << deflection.SetValue( DgdrErected );
      (*table2)(row2,col++) << deflection.SetValue( DpsErected );
      (*table2)(row2,col++) << deflection.SetValue( Ddiaphragm );

      if ( bShearKey )
      {
         (*table2)(row2,col++) << deflection.SetValue( Dshearkey );
      }

      (*table2)(row2,col++) << deflection.SetValue( Duser1 );
      (*table2)(row2,col++) << deflection.SetValue( Dcreep2 );

      if ( bSidewalk )
      {
         (*table2)(row2,col++) << deflection.SetValue(Dsidewalk);
      }

      (*table2)(row2,col++) << deflection.SetValue( Dbarrier );

      if (!pBridge->IsFutureOverlay() )
      {
         (*table2)(row2,col++) << deflection.SetValue(Doverlay);
      }

      (*table2)(row2,col++) << deflection.SetValue( Duser2 );
      (*table2)(row2,col++) << deflection.SetValue( Dcreep3 );

      row2++;

      // Table 3
      col = 0;
#pragma Reminder("UPDATE: D1 should be based on deflections after erection")
      // PGSuper v2 used deflections at end of storage. Since RDP and TxDOT are
      // going to totally redo camber, make it match v2 and fix it later
      //Float64 D1 = DgdrErected + DpsErected;
      Float64 D1 = DgdrStorage + DpsStorage;
      Float64 D2 = D1 + Dcreep1;
      Float64 D3 = D2 + Ddiaphragm + Dshearkey + Duser1;
      Float64 D4 = D3 + Dcreep2;
      Float64 D5 = D4 + Dsidewalk + Dbarrier + Doverlay + Duser2;
      Float64 D6 = D5 + Dcreep3;

      (*table3)(row3,col++) << location.SetValue( POI_ERECTED_SEGMENT,erectedPoi );
      (*table3)(row3,col++) << deflection.SetValue( D1 );
      (*table3)(row3,col++) << deflection.SetValue( D2 );
      (*table3)(row3,col++) << deflection.SetValue( D3 );

      D4 = IsZero(D4) ? 0 : D4;
      if ( D4 < 0 )
      {
         (*table3)(row3,col++) << color(Red) << deflection.SetValue(D4) << color(Black);
      }
      else
      {
         (*table3)(row3,col++) << deflection.SetValue( D4 );
      }

      (*table3)(row3,col++) << deflection.SetValue( D5 );

      D6 = IsZero(D6) ? 0 : D6;
      if ( D6 < 0 )
      {
         (*table3)(row3,col++) << color(Red) << deflection.SetValue(D6) << color(Black);
      }
      else
      {
         (*table3)(row3,col++) << deflection.SetValue( D6 );
      }

      row3++;
   }


   *pTable1 = pLayoutTable;
   *pTable2 = table2;
   *pTable3 = table3;
}
