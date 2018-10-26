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

void CCamberTable::GetPointsOfInterest(IBroker* pBroker,const CSegmentKey& segmentKey,std::vector<pgsPointOfInterest>* pvPoiRelease,std::vector<pgsPointOfInterest>* pvPoiStorage,std::vector<pgsPointOfInterest>* pvPoiErected) const
{
   GET_IFACE2(pBroker,IPointOfInterest,pPoi);
   *pvPoiRelease = pPoi->GetPointsOfInterest( segmentKey,POI_RELEASED_SEGMENT | POI_TENTH_POINTS);
   *pvPoiStorage = pPoi->GetPointsOfInterest( segmentKey,POI_STORAGE_SEGMENT  | POI_TENTH_POINTS);
   *pvPoiErected = pPoi->GetPointsOfInterest( segmentKey,POI_ERECTED_SEGMENT  | POI_TENTH_POINTS);

   ATLASSERT(pvPoiRelease->size() == pvPoiStorage->size() && pvPoiStorage->size() == pvPoiErected->size());

   pgsPointOfInterest poiLeftRelease = pvPoiRelease->front();
   pgsPointOfInterest poiRightRelease = pvPoiRelease->back();
   pgsPointOfInterest poiLeftStorage = pvPoiStorage->front();
   pgsPointOfInterest poiRightStorage = pvPoiStorage->back();
   pgsPointOfInterest poiLeftErected = pvPoiErected->front();
   pgsPointOfInterest poiRightErected = pvPoiErected->back();

   // put support locations for all cases into all poi vectors
   // so we can compute relative displacements more easily
   pvPoiRelease->push_back(poiLeftStorage);
   pvPoiRelease->push_back(poiRightStorage);
   pvPoiRelease->push_back(poiLeftErected);
   pvPoiRelease->push_back(poiRightErected);
   std::sort(pvPoiRelease->begin(),pvPoiRelease->end());
   pvPoiRelease->erase(std::unique(pvPoiRelease->begin(),pvPoiRelease->end()),pvPoiRelease->end());

   pvPoiStorage->push_back(poiLeftRelease);
   pvPoiStorage->push_back(poiRightRelease);
   pvPoiStorage->push_back(poiLeftErected);
   pvPoiStorage->push_back(poiRightErected);
   std::sort(pvPoiStorage->begin(),pvPoiStorage->end());
   pvPoiStorage->erase(std::unique(pvPoiStorage->begin(),pvPoiStorage->end()),pvPoiStorage->end());

   pvPoiErected->push_back(poiLeftRelease);
   pvPoiErected->push_back(poiRightRelease);
   pvPoiErected->push_back(poiLeftStorage);
   pvPoiErected->push_back(poiRightStorage);
   std::sort(pvPoiErected->begin(),pvPoiErected->end());
   pvPoiErected->erase(std::unique(pvPoiErected->begin(),pvPoiErected->end()),pvPoiErected->end());

   ATLASSERT(pvPoiRelease->size() == pvPoiStorage->size() && pvPoiStorage->size() == pvPoiErected->size());
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

void CCamberTable::Build_Deck(IBroker* pBroker,const CSegmentKey& segmentKey, 
                             bool bTempStrands, bool bSidewalk, bool bShearKey,bool bConstruction, bool bOverlay, bool bDeckPanels,
                             IEAFDisplayUnits* pDisplayUnits,Int16 constructionRate, const CamberMultipliers& cm,
                             rptRcTable** pTable1,rptRcTable** pTable2,rptRcTable** pTable3) const
{
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   location.IncludeSpanAndGirder(segmentKey.groupIndex == ALL_GROUPS);
   INIT_UV_PROTOTYPE( rptLengthUnitValue, deflection, pDisplayUnits->GetDeflectionUnit(), false );

   GET_IFACE2(pBroker,ILibrary,pLib);
   GET_IFACE2(pBroker,ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());

   // Get the interface pointers we need
   std::vector<pgsPointOfInterest> vPoiRelease, vPoiStorage, vPoiErected;
   GetPointsOfInterest(pBroker,segmentKey,&vPoiRelease,&vPoiStorage,&vPoiErected);

   GET_IFACE2(pBroker,ICamber,pCamber);
   GET_IFACE2(pBroker,IExternalLoading,pExtLoading);
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

   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   // create the tables
   rptRcTable* table1a;
   rptRcTable* table1b;
   rptRcTable* table2;
   rptRcTable* table3;

   rptRcTable* pLayoutTable = rptStyleManager::CreateLayoutTable(2);
   
   table1a = rptStyleManager::CreateDefaultTable(4,_T("Deflections at Release"));
   table1b = rptStyleManager::CreateDefaultTable(5,_T("Deflections during Storage"));
   (*pLayoutTable)(0,0) << table1a;
   (*pLayoutTable)(0,1) << table1b;

   int ncols = 12 + (bTempStrands ? 1 : 0) + (bSidewalk ? 1 : 0) + (bOverlay ? 1 : 0) + (bShearKey ? 1 : 0) + (bConstruction ? 1 : 0) + (bDeckPanels ? 1 : 0);
   table2 = rptStyleManager::CreateDefaultTable(ncols,_T("Deflections after Erection"));
   table3 = rptStyleManager::CreateDefaultTable(8,_T("Deflection Summary"));

   if ( segmentKey.groupIndex == ALL_GROUPS )
   {
      table1a->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table1a->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

      table1b->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table1b->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

      table2->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table2->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

      table3->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table3->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   // Setup table headings
   ColumnIndexType col = 0;
   (*table1a)(0,col++) << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table1a)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("girder")),     rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table1a)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("ps")),         rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table1a)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("i")),          rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   
   col = 0;
   (*table1b)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table1b)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("girder")),     rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table1b)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("ps")),         rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table1b)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("creep1")),     rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table1b)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("es")),         rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

   col = 0;
   (*table2)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("girder")) << rptNewLine << _T("Erected"),     rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("ps")) << rptNewLine << _T("Erected"),         rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table2)(0,col++) << COLHDR(Sub2(symbol(delta),_T("girder")),     rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("creep1")) << rptNewLine << _T("Erected"), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
    
   if (bTempStrands)
   {
      (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("tpsr")),         rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   }

   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("diaphragm")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

   if ( bShearKey )
   {
      (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("shear key")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   }

   if ( bConstruction )
   {
      (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("construction")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   }

   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("creep2")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

   if (bDeckPanels)
   {
      (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("deck panels")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   }

   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("slab")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("haunch")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("User1")) << _T(" = ") << rptNewLine << Sub2(symbol(DELTA),_T("UserDC")) << _T(" + ") << rptNewLine << Sub2(symbol(DELTA),_T("UserDW")) , rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

   if ( bSidewalk )
   {
      (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("sidewalk")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   }

   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("barrier")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

   if ( bOverlay )
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

      Float64 DpsRelease  = pProduct->GetDeflection(releaseIntervalIdx,pgsTypes::pftPretension,releasePoi,bat,rtCumulative,false);
      Float64 DpsStorage  = pProduct->GetDeflection(storageIntervalIdx,pgsTypes::pftPretension,storagePoi,bat,rtCumulative,false);
      Float64 DpsErected  = pProduct->GetDeflection(erectionIntervalIdx,pgsTypes::pftPretension,erectedPoi,bat,rtCumulative,false);

      Float64 DgdrRelease = pProduct->GetDeflection(releaseIntervalIdx,pgsTypes::pftGirder,releasePoi,bat,rtCumulative,false);
      Float64 DgdrStorage = pProduct->GetDeflection(storageIntervalIdx,pgsTypes::pftGirder,storagePoi,bat,rtCumulative,false);
      Float64 DgdrErected = pProduct->GetDeflection(erectionIntervalIdx,pgsTypes::pftGirder,erectedPoi,bat,rtCumulative,false);
      Float64 dgdrErected = pExtLoading->GetDeflection(erectionIntervalIdx,_T("Girder_Incremental"),erectedPoi,bat,rtCumulative,false);

      Float64 Dtpsr = bTempStrands ? pCamber->GetReleaseTempPrestressDeflection( erectedPoi ) : 0.0;

      // NOTE: Get the creep deflection from the ICamber interface because it takes the construction rate 
      // into account. Getting creep deflection as a product load assumes the maximum construction rate
      Float64 Dcreep1a   = pCamber->GetCreepDeflection( storagePoi, ICamber::cpReleaseToDiaphragm, constructionRate, pgsTypes::pddStorage );
      Float64 Dcreep1b   = pCamber->GetCreepDeflection( erectedPoi, ICamber::cpReleaseToDiaphragm, constructionRate, pgsTypes::pddErected );
      Float64 Ddiaphragm = pCamber->GetDiaphragmDeflection( erectedPoi );
      Float64 DshearKey  = bShearKey ? pProduct->GetDeflection(castDeckIntervalIdx,pgsTypes::pftShearKey,erectedPoi,bat, rtCumulative, false) : 0.0;
      Float64 Dconstruction= bConstruction ? pProduct->GetDeflection(castDeckIntervalIdx,pgsTypes::pftConstruction,erectedPoi,bat, rtCumulative, false) : 0.0;
      Float64 Dpanel     =  bDeckPanels ? pProduct->GetDeflection(castDeckIntervalIdx,pgsTypes::pftSlabPanel,erectedPoi,bat, rtCumulative, false) : 0.0;
      Float64 Ddeck      = pProduct->GetDeflection(castDeckIntervalIdx,pgsTypes::pftSlab,erectedPoi,bat, rtCumulative, false);
      Float64 DslabPad   = pProduct->GetDeflection(castDeckIntervalIdx,pgsTypes::pftSlabPad,erectedPoi,bat, rtCumulative, false);
      Float64 Dcreep2    = pCamber->GetCreepDeflection( erectedPoi, ICamber::cpDiaphragmToDeck, constructionRate, pgsTypes::pddErected );
      Float64 Duser1     = pProduct->GetDeflection(castDeckIntervalIdx,pgsTypes::pftUserDC,erectedPoi,bat, rtCumulative, false) 
                         + pProduct->GetDeflection(castDeckIntervalIdx,pgsTypes::pftUserDW,erectedPoi,bat, rtCumulative, false);
      Float64 Duser2     = pProduct->GetDeflection(compositeDeckIntervalIdx,pgsTypes::pftUserDC,erectedPoi,bat, rtCumulative, false) 
                         + pProduct->GetDeflection(compositeDeckIntervalIdx,pgsTypes::pftUserDW,erectedPoi,bat, rtCumulative, false);
      Duser2 -= Duser1; // Duser2 is cumulative and it includes Duser1... remove Duser1
      Float64 Dbarrier   = pProduct->GetDeflection(railingSystemIntervalIdx,pgsTypes::pftTrafficBarrier,erectedPoi,bat, rtCumulative, false);
      Float64 Dsidewalk  = pProduct->GetDeflection(railingSystemIntervalIdx,pgsTypes::pftSidewalk,      erectedPoi,bat, rtCumulative, false);
      Float64 Doverlay   = (pBridge->HasOverlay() ? (pBridge->IsFutureOverlay() ? 0.0 : pProduct->GetDeflection(overlayIntervalIdx,pgsTypes::pftOverlay,erectedPoi,bat, rtCumulative, false)) : 0.0);

      // if we have a future overlay, the deflection due to the overlay in BridgeSite2 must be zero
      ATLASSERT( pBridge->IsFutureOverlay() ? IsZero(Doverlay) : true );


      Float64 Di = DpsRelease + DgdrRelease; // initial camber
      Float64 Des = DpsStorage + DgdrStorage + Dcreep1a; // camber at end of storage

      // Table 1a
      col = 0;
      (*table1a)(row1a,col++) << location.SetValue( POI_RELEASED_SEGMENT, releasePoi );
      (*table1a)(row1a,col++) << deflection.SetValue( DgdrRelease );
      (*table1a)(row1a,col++) << deflection.SetValue( DpsRelease );
      (*table1a)(row1a,col++) << deflection.SetValue( Di );
      row1a++;

      // Table 1b
      col = 0;
      (*table1b)(row1b,col++) << location.SetValue( POI_STORAGE_SEGMENT, storagePoi );
      (*table1b)(row1b,col++) << deflection.SetValue( DgdrStorage );
      (*table1b)(row1b,col++) << deflection.SetValue( DpsStorage );
      (*table1b)(row1b,col++) << deflection.SetValue( Dcreep1a );
      (*table1b)(row1b,col++) << deflection.SetValue( Des );
      if ( storagePoi.IsTenthPoint(POI_ERECTED_SEGMENT) == 1 || storagePoi.IsTenthPoint(POI_ERECTED_SEGMENT) == 11 )
      {
         for ( ColumnIndexType i = 0; i < col; i++ )
         {
            (*table1b)(row1b,i).InsertContent(0,bold(ON));
            (*table1b)(row1b,i) << bold(OFF);
         }
      }
      row1b++;

      // Table 2
      col = 0;
      (*table2)(row2,col++) << location.SetValue( POI_ERECTED_SEGMENT, erectedPoi );
      (*table2)(row2,col++) << deflection.SetValue( DgdrErected );
      (*table2)(row2,col++) << deflection.SetValue( DpsErected );
      (*table2)(row2,col++) << deflection.SetValue( dgdrErected );
      (*table2)(row2,col++) << deflection.SetValue( Dcreep1b );

      if (bTempStrands)
      {
         (*table2)(row2,col++) << deflection.SetValue( Dtpsr );
      }

      (*table2)(row2,col++) << deflection.SetValue( Ddiaphragm );
      if ( bShearKey )
      {
         (*table2)(row2,col++) << deflection.SetValue( DshearKey );
      }

      if ( bConstruction )
      {
         (*table2)(row2,col++) << deflection.SetValue( Dconstruction );
      }

      (*table2)(row2,col++) << deflection.SetValue( Dcreep2 );

      if (bDeckPanels)
      {
         (*table2)(row2,col++) << deflection.SetValue( Dpanel );
      }

      (*table2)(row2,col++) << deflection.SetValue( Ddeck );
      (*table2)(row2,col++) << deflection.SetValue( DslabPad );
      (*table2)(row2,col++) << deflection.SetValue( Duser1 );

      if ( bSidewalk )
      {
         (*table2)(row2,col++) << deflection.SetValue( Dsidewalk );
      }

      (*table2)(row2,col++) << deflection.SetValue( Dbarrier );

      if ( bOverlay )
      {
         (*table2)(row2,col++) << deflection.SetValue(Doverlay);
      }

      (*table2)(row2,col++) << deflection.SetValue( Duser2 );

      row2++;

      // Table 3
      col = 0;

      Float64 D1 = cm.ErectionFactor * (DgdrErected + DpsErected);
      Float64 D2 = D1 + cm.CreepFactor * Dcreep1b;
      Float64 D3 = D2 + cm.DiaphragmFactor * (Ddiaphragm + DshearKey + Dconstruction) + cm.ErectionFactor * Dtpsr;
      Float64 D4 = D3 + cm.CreepFactor * Dcreep2;
      Float64 D5 = D4 + cm.SlabUser1Factor * (Ddeck + Duser1) + cm.SlabPadLoadFactor*DslabPad  + cm.DeckPanelFactor * Dpanel;;
      Float64 D6 = D5 + cm.BarrierSwOverlayUser2Factor * (Dbarrier + Duser2);
      if ( bSidewalk )
      {
         D6 += cm.BarrierSwOverlayUser2Factor * Dsidewalk;
      }
      if ( bOverlay )
      {
         D6 += cm.BarrierSwOverlayUser2Factor * Doverlay;
      }

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

#ifdef _DEBUG
      // Reality check with AnalysisAgent
      Float64 Dc = pCamber->GetDCamberForGirderSchedule(erectedPoi, constructionRate);
      ATLASSERT(IsEqual(Dc, D4,0.001));

      Float64 Ec = pCamber->GetExcessCamber(erectedPoi, constructionRate);
      ATLASSERT(IsEqual(Ec, D6,0.001));
#endif

      row3++;
   }

   *pTable1 = pLayoutTable;
   *pTable2 = table2;
   *pTable3 = table3;
}

void CCamberTable::Build_NoDeck(IBroker* pBroker,const CSegmentKey& segmentKey,
                                            bool bTempStrands, bool bSidewalk, bool bShearKey,bool bConstruction, bool bOverlay,
                                            IEAFDisplayUnits* pDisplayUnits,Int16 constructionRate, const CamberMultipliers& cm,
                                            rptRcTable** pTable1,rptRcTable** pTable2,rptRcTable** pTable3) const
{
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   location.IncludeSpanAndGirder(segmentKey.groupIndex == ALL_GROUPS);
   INIT_UV_PROTOTYPE( rptLengthUnitValue, deflection, pDisplayUnits->GetDeflectionUnit(), false );

   GET_IFACE2(pBroker,ILibrary,pLib);
   GET_IFACE2(pBroker,ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());

   // Get the interface pointers we need
   std::vector<pgsPointOfInterest> vPoiRelease, vPoiStorage, vPoiErected;
   GetPointsOfInterest(pBroker,segmentKey,&vPoiRelease,&vPoiStorage,&vPoiErected);

   GET_IFACE2(pBroker,ICamber,pCamber);
   GET_IFACE2(pBroker,IExternalLoading,pExtLoading);
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

   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   // create the tables
   rptRcTable* table1a;
   rptRcTable* table1b;
   rptRcTable* table2;
   rptRcTable* table3;

   rptRcTable* pLayoutTable = rptStyleManager::CreateLayoutTable(2);
   
   table1a = rptStyleManager::CreateDefaultTable(4,_T("Deflections at Release"));
   table1b = rptStyleManager::CreateDefaultTable(5,_T("Deflections during Storage"));
   (*pLayoutTable)(0,0) << table1a;
   (*pLayoutTable)(0,1) << table1b;

   int ncols = 11 + (bTempStrands ? 1 : 0) + (bSidewalk ? 1 : 0) + (bOverlay ? 1 : 0) + (bShearKey ? 1 : 0) + (bConstruction ? 1 : 0);
   table2 = rptStyleManager::CreateDefaultTable(ncols,_T("Deflections after Erection"));
   table3 = rptStyleManager::CreateDefaultTable(8,_T("Deflection Summary"));

   if ( segmentKey.groupIndex == ALL_GROUPS )
   {
      table1a->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table1a->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

      table1b->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table1b->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

      table2->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table2->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

      table3->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table3->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   // Setup table headings
   ColumnIndexType col = 0;
   (*table1a)(0,col++) << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table1a)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("girder")),     rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table1a)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("ps")),         rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table1a)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("i")),          rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   
   col = 0;
   (*table1b)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table1b)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("girder")),     rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table1b)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("ps")),         rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table1b)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("creep1")),     rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table1b)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("es")),         rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

   col = 0;
   (*table2)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("girder")) << rptNewLine << _T("Erected"),     rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("ps")) << rptNewLine << _T("Erected"),         rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table2)(0,col++) << COLHDR(Sub2(symbol(delta),_T("girder")),     rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("creep1")) << rptNewLine << _T("Erected"), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
    
   if (bTempStrands)
   {
      (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("tpsr")),         rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   }

   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("diaphragm")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

   if ( bShearKey )
   {
      (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("shear key")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   }

   if ( bConstruction )
   {
      (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("construction")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   }

   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("creep2")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("User1")) << _T(" = ") << rptNewLine << Sub2(symbol(DELTA),_T("UserDC")) << _T(" + ") << rptNewLine << Sub2(symbol(DELTA),_T("UserDW")) , rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

   if ( bSidewalk )
   {
      (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("sidewalk")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   }

   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("barrier")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

   if ( bOverlay )
   {
      (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("overlay")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   }

   (*table2)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("User2")) << _T(" = ") << rptNewLine << Sub2(symbol(DELTA),_T("UserDC")) << _T(" + ") << rptNewLine << Sub2(symbol(DELTA),_T("UserDW")) , rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
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

      Float64 DpsRelease   = pProduct->GetDeflection(releaseIntervalIdx,pgsTypes::pftPretension,releasePoi,bat,rtCumulative,false);
      Float64 DpsStorage   = pProduct->GetDeflection(storageIntervalIdx,pgsTypes::pftPretension,storagePoi,bat,rtCumulative,false);
      Float64 DpsErected  = pProduct->GetDeflection(erectionIntervalIdx,pgsTypes::pftPretension,erectedPoi,bat,rtCumulative,false);

      Float64 DgdrRelease  = pProduct->GetDeflection(releaseIntervalIdx,pgsTypes::pftGirder,releasePoi,bat,rtCumulative,false);
      Float64 DgdrStorage  = pProduct->GetDeflection(storageIntervalIdx,pgsTypes::pftGirder,storagePoi,bat,rtCumulative,false);
      Float64 DgdrErected = pProduct->GetDeflection(erectionIntervalIdx,pgsTypes::pftGirder,erectedPoi,bat,rtCumulative,false);
      Float64 dgdrErected = pExtLoading->GetDeflection(erectionIntervalIdx,_T("Girder_Incremental"),erectedPoi,bat,rtCumulative,false);

      Float64 Dtpsr = bTempStrands ? pCamber->GetReleaseTempPrestressDeflection( erectedPoi ) : 0.0;

      // NOTE: Get the creep deflection from the ICamber interface because it takes the construction rate 
      // into account. Getting creep deflection as a product load assumes the maximum construction rate
      Float64 Dcreep1a   = pCamber->GetCreepDeflection( storagePoi, ICamber::cpReleaseToDiaphragm, constructionRate, pgsTypes::pddStorage );
      Float64 Dcreep1b   = pCamber->GetCreepDeflection( erectedPoi, ICamber::cpReleaseToDiaphragm, constructionRate, pgsTypes::pddErected );
      Float64 Ddiaphragm = pCamber->GetDiaphragmDeflection( erectedPoi );
      Float64 DshearKey  = bShearKey ? pProduct->GetDeflection(castDeckIntervalIdx,pgsTypes::pftShearKey,erectedPoi,bat, rtCumulative, false) : 0.0;
      Float64 Dconstruction= bConstruction ? pProduct->GetDeflection(castDeckIntervalIdx,pgsTypes::pftConstruction,erectedPoi,bat, rtCumulative, false) : 0.0;
      Float64 Dcreep2    = pCamber->GetCreepDeflection( erectedPoi, ICamber::cpDiaphragmToDeck, constructionRate, pgsTypes::pddErected );
      Float64 Duser1     = pProduct->GetDeflection(castDeckIntervalIdx,pgsTypes::pftUserDC,erectedPoi,bat, rtCumulative, false) 
                         + pProduct->GetDeflection(castDeckIntervalIdx,pgsTypes::pftUserDW,erectedPoi,bat, rtCumulative, false);
      Float64 Duser2     = pProduct->GetDeflection(compositeDeckIntervalIdx,pgsTypes::pftUserDC,erectedPoi,bat, rtCumulative, false) 
                         + pProduct->GetDeflection(compositeDeckIntervalIdx,pgsTypes::pftUserDW,erectedPoi,bat, rtCumulative, false);
      Duser2 -= Duser1; // Duser2 is cumulative and it includes Duser1... remove Duser1
      Float64 Dbarrier   = pProduct->GetDeflection(railingSystemIntervalIdx,pgsTypes::pftTrafficBarrier,erectedPoi,bat, rtCumulative, false);
      Float64 Dsidewalk  = pProduct->GetDeflection(railingSystemIntervalIdx,pgsTypes::pftSidewalk,      erectedPoi,bat, rtCumulative, false);
      Float64 Doverlay   = (pBridge->HasOverlay() ? (pBridge->IsFutureOverlay() ? 0.0 : pProduct->GetDeflection(overlayIntervalIdx,pgsTypes::pftOverlay,erectedPoi,bat, rtCumulative, false)) : 0.0);
      Float64 Dcreep3    = pCamber->GetCreepDeflection( erectedPoi, ICamber::cpDeckToFinal, constructionRate, pgsTypes::pddErected );

      Float64 Di = DpsRelease + DgdrRelease; // initial camber immedately after release
      Float64 Des = DpsStorage + DgdrStorage + Dcreep1a; // camber at end of storage

      // if we have a future overlay, the deflection due to the overlay in BridgeSite2 must be zero
      ATLASSERT( pBridge->IsFutureOverlay() ? IsZero(Doverlay) : true );

      // Table 1a
      col = 0;
      (*table1a)(row1a,col++) << location.SetValue( POI_RELEASED_SEGMENT, releasePoi );
      (*table1a)(row1a,col++) << deflection.SetValue( DgdrRelease );
      (*table1a)(row1a,col++) << deflection.SetValue( DpsRelease );
      (*table1a)(row1a,col++) << deflection.SetValue( Di );
      row1a++;

      // Table 1b
      col = 0;
      (*table1b)(row1b,col++) << location.SetValue( POI_STORAGE_SEGMENT, storagePoi );
      (*table1b)(row1b,col++) << deflection.SetValue( DgdrStorage );
      (*table1b)(row1b,col++) << deflection.SetValue( DpsStorage );
      (*table1b)(row1b,col++) << deflection.SetValue( Dcreep1a );
      (*table1b)(row1b,col++) << deflection.SetValue( Des );
      if ( storagePoi.IsTenthPoint(POI_ERECTED_SEGMENT) == 1 || storagePoi.IsTenthPoint(POI_ERECTED_SEGMENT) == 11 )
      {
         for ( ColumnIndexType i = 0; i < col; i++ )
         {
            (*table1b)(row1b,i).InsertContent(0,bold(ON));
            (*table1b)(row1b,i) << bold(OFF);
         }
      }
      row1b++;

      // Table 2
      col = 0;
      (*table2)(row2,col++) << location.SetValue( POI_ERECTED_SEGMENT, erectedPoi );
      (*table2)(row2,col++) << deflection.SetValue( DgdrErected );
      (*table2)(row2,col++) << deflection.SetValue( DpsErected );
      (*table2)(row2,col++) << deflection.SetValue( dgdrErected );
      (*table2)(row2,col++) << deflection.SetValue( Dcreep1b );

      if (bTempStrands)
      {
         (*table2)(row2,col++) << deflection.SetValue( Dtpsr );
      }

      (*table2)(row2,col++) << deflection.SetValue( Ddiaphragm );
      if ( bShearKey )
      {
         (*table2)(row2,col++) << deflection.SetValue( DshearKey );
      }

      if ( bConstruction )
      {
         (*table2)(row2,col++) << deflection.SetValue( Dconstruction );
      }

      (*table2)(row2,col++) << deflection.SetValue( Dcreep2 );

      (*table2)(row2,col++) << deflection.SetValue( Duser1 );

      if ( bSidewalk )
      {
         (*table2)(row2,col++) << deflection.SetValue( Dsidewalk );
      }

      (*table2)(row2,col++) << deflection.SetValue( Dbarrier );

      if ( bOverlay )
      {
         (*table2)(row2,col++) << deflection.SetValue(Doverlay);
      }

      (*table2)(row2,col++) << deflection.SetValue( Duser2 );
      (*table2)(row2,col++) << deflection.SetValue( Dcreep3 );

      row2++;

      // Table 3
      col = 0;

      Float64 D1 = cm.ErectionFactor * (DgdrErected + DpsErected);
      Float64 D2 = D1 + cm.CreepFactor * Dcreep1b;
      Float64 D3 = D2 + cm.DiaphragmFactor * (Ddiaphragm + DshearKey + Dconstruction) + cm.ErectionFactor * Dtpsr + cm.SlabUser1Factor * Duser1;
      Float64 D4 = D3 + cm.CreepFactor * Dcreep2;
      Float64 D5 = D4 + cm.BarrierSwOverlayUser2Factor * (Dbarrier + Duser2);
      if ( bSidewalk )
      {
         D5 += cm.BarrierSwOverlayUser2Factor * Dsidewalk;
      }

      if ( bOverlay )
      {
         D5 += cm.BarrierSwOverlayUser2Factor * Doverlay;
      }

      Float64 D6 = D5 + cm.CreepFactor * Dcreep3;

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

#ifdef _DEBUG
      // Reality check with AnalysisAgent
      Float64 Dc = pCamber->GetDCamberForGirderSchedule(erectedPoi, constructionRate);
      ATLASSERT(IsEqual(Dc, D4));

      Float64 Ec = pCamber->GetExcessCamber(erectedPoi, constructionRate);
      ATLASSERT(IsEqual(Ec, D6));
#endif

      row3++;
   }

   *pTable1 = pLayoutTable;
   *pTable2 = table2;
   *pTable3 = table3;
}
