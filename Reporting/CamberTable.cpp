///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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
#include <IFace\ReportOptions.h>

#include <psgLib/CreepCriteria.h>


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

void CCamberTable::GetPointsOfInterest(IBroker* pBroker,const CSegmentKey& segmentKey,PoiList* pvPoiRelease,PoiList* pvPoiStorage,PoiList* pvPoiErected) const
{
   GET_IFACE2(pBroker,IPointOfInterest,pPoi);
   pPoi->GetPointsOfInterest( segmentKey,POI_RELEASED_SEGMENT | POI_TENTH_POINTS, pvPoiRelease);
   pPoi->GetPointsOfInterest( segmentKey,POI_STORAGE_SEGMENT  | POI_TENTH_POINTS, pvPoiStorage);
   pPoi->GetPointsOfInterest( segmentKey,POI_ERECTED_SEGMENT  | POI_TENTH_POINTS, pvPoiErected);

   ATLASSERT(pvPoiRelease->size() == pvPoiStorage->size() && pvPoiStorage->size() == pvPoiErected->size());

   const pgsPointOfInterest& poiLeftRelease = pvPoiRelease->front();
   const pgsPointOfInterest& poiRightRelease = pvPoiRelease->back();
   const pgsPointOfInterest& poiLeftStorage = pvPoiStorage->front();
   const pgsPointOfInterest& poiRightStorage = pvPoiStorage->back();
   const pgsPointOfInterest& poiLeftErected = pvPoiErected->front();
   const pgsPointOfInterest& poiRightErected = pvPoiErected->back();

   // put support locations for all cases into all poi vectors
   // so we can compute relative displacements more easily
   pvPoiRelease->push_back(poiLeftStorage);
   pvPoiRelease->push_back(poiRightStorage);
   pvPoiRelease->push_back(poiLeftErected);
   pvPoiRelease->push_back(poiRightErected);
   pPoi->SortPoiList(pvPoiRelease);

   pvPoiStorage->push_back(poiLeftRelease);
   pvPoiStorage->push_back(poiRightRelease);
   pvPoiStorage->push_back(poiLeftErected);
   pvPoiStorage->push_back(poiRightErected);
   pPoi->SortPoiList(pvPoiStorage);

   pvPoiErected->push_back(poiLeftRelease);
   pvPoiErected->push_back(poiRightRelease);
   pvPoiErected->push_back(poiLeftStorage);
   pvPoiErected->push_back(poiRightStorage);
   pPoi->SortPoiList(pvPoiErected);

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
void CCamberTable::Build_Deck(IBroker* pBroker, const CSegmentKey& segmentKey,
   bool bTempStrands, bool bSidewalk, bool bShearKey, bool bLongitudinalJoint, bool bConstruction, bool bOverlay, bool bDeckPanels,
   IEAFDisplayUnits* pDisplayUnits, pgsTypes::CreepTime constructionRate, const CamberMultipliers& cm,
   rptRcTable** pTable1, rptRcTable** pTable2, rptRcTable** pTable3) const
{
   GET_IFACE2(pBroker, IBridge, pBridge);
   if (pBridge->HasAsymmetricGirders() || pBridge->HasAsymmetricPrestressing() || pBridge->HasTiltedGirders())
   {
      Build_Deck_XY(pBroker, segmentKey, bTempStrands, bSidewalk, bShearKey, bLongitudinalJoint, bConstruction, bOverlay, bDeckPanels, pDisplayUnits, constructionRate, cm, pTable1, pTable2, pTable3);
   }
   else
   {
      Build_Deck_Y(pBroker, segmentKey, bTempStrands, bSidewalk, bShearKey, bLongitudinalJoint, bConstruction, bOverlay, bDeckPanels, pDisplayUnits, constructionRate, cm, pTable1, pTable2, pTable3);
   }
}

void CCamberTable::Build_NoDeck(IBroker* pBroker, const CSegmentKey& segmentKey,
   bool bTempStrands, bool bSidewalk, bool bShearKey, bool bLongitudinalJoint, bool bConstruction, bool bOverlay,
   IEAFDisplayUnits* pDisplayUnits, pgsTypes::CreepTime constructionRate, const CamberMultipliers& cm,
   rptRcTable** pTable1, rptRcTable** pTable2, rptRcTable** pTable3) const
{
   GET_IFACE2(pBroker, IBridge, pBridge);
   if (pBridge->HasAsymmetricGirders() || pBridge->HasAsymmetricPrestressing() || pBridge->HasTiltedGirders())
   {
      Build_NoDeck_XY(pBroker, segmentKey, bTempStrands, bSidewalk, bShearKey, bLongitudinalJoint, bConstruction, bOverlay, pDisplayUnits, constructionRate, cm, pTable1, pTable2, pTable3);
   }
   else
   {
      Build_NoDeck_Y(pBroker, segmentKey, bTempStrands, bSidewalk, bShearKey, bLongitudinalJoint, bConstruction, bOverlay, pDisplayUnits, constructionRate, cm, pTable1, pTable2, pTable3);
   }
}

void CCamberTable::Build_Deck_Y(IBroker* pBroker, const CSegmentKey& segmentKey,
   bool bTempStrands, bool bSidewalk, bool bShearKey, bool bLongitudinalJoint,bool bConstruction, bool bOverlay, bool bDeckPanels,
   IEAFDisplayUnits* pDisplayUnits, pgsTypes::CreepTime constructionRate, const CamberMultipliers& cm,
   rptRcTable** pTable1, rptRcTable** pTable2, rptRcTable** pTable3) const
{
   INIT_UV_PROTOTYPE(rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false);
   INIT_UV_PROTOTYPE(rptLengthUnitValue, deflection, pDisplayUnits->GetDeflectionUnit(), false);

   GET_IFACE2(pBroker,IReportOptions,pReportOptions);
   location.IncludeSpanAndGirder(pReportOptions->IncludeSpanAndGirder4Pois(segmentKey));

   GET_IFACE2(pBroker, ILibrary, pLib);
   GET_IFACE2(pBroker, ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());

   // Get the interface pointers we need
   PoiList vPoiRelease, vPoiStorage, vPoiErected;
   GetPointsOfInterest(pBroker, segmentKey, &vPoiRelease, &vPoiStorage, &vPoiErected);

   GET_IFACE2(pBroker, ICamber, pCamber);
   GET_IFACE2(pBroker, IExternalLoading, pExtLoading);
   GET_IFACE2(pBroker, IProductForces, pProduct);
   GET_IFACE2(pBroker, IBridge, pBridge);
   GET_IFACE2(pBroker, IPointOfInterest, pPoi);

   GET_IFACE2(pBroker, IIntervals, pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType storageIntervalIdx = pIntervals->GetStorageInterval(segmentKey);
   IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
   IntervalIndexType tempStrandRemovalIntervalIdx = pIntervals->GetTemporaryStrandRemovalInterval(segmentKey);
   IntervalIndexType castShearKeyIntervalIdx = pIntervals->GetCastShearKeyInterval();
   IntervalIndexType castLongitudinalJointIntervalIdx = pIntervals->GetCastLongitudinalJointInterval();
   IntervalIndexType constructionLoadIntervalIdx = pIntervals->GetConstructionLoadInterval();
   IntervalIndexType railingSystemIntervalIdx = pIntervals->GetInstallRailingSystemInterval();
   IntervalIndexType overlayIntervalIdx = pIntervals->GetOverlayInterval();

   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   // create the tables
   rptRcTable* table1a;
   rptRcTable* table1b;
   rptRcTable* table2;
   rptRcTable* table3;

   rptRcTable* pLayoutTable = rptStyleManager::CreateLayoutTable(2);

   bool bHasPrecamber = IsZero(pCamber->GetPrecamber(segmentKey)) ? false : true;

   ColumnIndexType nColumns = 4;
   if (bHasPrecamber)
   {
      nColumns++;
   }
   table1a = rptStyleManager::CreateDefaultTable(nColumns, _T("Deflections at Release"));
   table1b = rptStyleManager::CreateDefaultTable(nColumns + 1, _T("Deflections during Storage"));
   (*pLayoutTable)(0, 0) << table1a;
   (*pLayoutTable)(0, 1) << table1b;

   ColumnIndexType ncols = 12 + (bTempStrands ? 1 : 0) + (bSidewalk ? 1 : 0) + (bOverlay ? 1 : 0) + (bShearKey ? 1 : 0) + (bLongitudinalJoint ? 1 : 0) + (bConstruction ? 1 : 0) + (bDeckPanels ? 1 : 0);
   if (bHasPrecamber)
   {
      ncols++;
   }
   table2 = rptStyleManager::CreateDefaultTable(ncols, _T("Deflections after Erection"));
   table3 = rptStyleManager::CreateDefaultTable(8, _T("Deflection Summary"));

   if (segmentKey.groupIndex == ALL_GROUPS)
   {
      table1a->SetColumnStyle(0, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table1a->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

      table1b->SetColumnStyle(0, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table1b->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

      table2->SetColumnStyle(0, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table2->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

      table3->SetColumnStyle(0, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table3->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   // Setup table headings
   ColumnIndexType col = 0;
   (*table1a)(0, col++) << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table1a)(0, col++) << COLHDR(Sub2(symbol(DELTA), _T("girder")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   (*table1a)(0, col++) << COLHDR(Sub2(symbol(DELTA), _T("ps")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   if (bHasPrecamber)
   {
      (*table1a)(0, col++) << COLHDR(Sub2(symbol(DELTA), _T("precamber")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   }
   (*table1a)(0, col++) << COLHDR(Sub2(symbol(DELTA), _T("i")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());

   col = 0;
   (*table1b)(0, col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table1b)(0, col++) << COLHDR(Sub2(symbol(DELTA), _T("girder")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   (*table1b)(0, col++) << COLHDR(Sub2(symbol(DELTA), _T("ps")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   if (bHasPrecamber)
   {
      (*table1b)(0, col++) << COLHDR(Sub2(symbol(DELTA), _T("precamber")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   }
   (*table1b)(0, col++) << COLHDR(Sub2(symbol(DELTA), _T("creep1")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   (*table1b)(0, col++) << COLHDR(Sub2(symbol(DELTA), _T("es")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());

   col = 0;
   (*table2)(0, col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table2)(0, col++) << COLHDR(Sub2(symbol(DELTA), _T("girder")) << rptNewLine << _T("Erected"), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   (*table2)(0, col++) << COLHDR(Sub2(symbol(DELTA), _T("ps")) << rptNewLine << _T("Erected"), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   (*table2)(0, col++) << COLHDR(Sub2(symbol(delta), _T("girder")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   if (bHasPrecamber)
   {
      (*table2)(0, col++) << COLHDR(Sub2(symbol(DELTA), _T("precamber")) << rptNewLine << _T("Erected"), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   }
   (*table2)(0, col++) << COLHDR(Sub2(symbol(DELTA), _T("creep1")) << rptNewLine << _T("Erected"), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());

   if (bTempStrands)
   {
      (*table2)(0, col++) << COLHDR(Sub2(symbol(DELTA), _T("tpsr")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   }

   (*table2)(0, col++) << COLHDR(Sub2(symbol(DELTA), _T("diaphragm")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());

   if (bShearKey)
   {
      (*table2)(0, col++) << COLHDR(Sub2(symbol(DELTA), _T("shear key")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   }

   if (bLongitudinalJoint)
   {
      (*table2)(0, col++) << COLHDR(Sub2(symbol(DELTA), _T("long. joint")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   }

   if (bConstruction)
   {
      (*table2)(0, col++) << COLHDR(Sub2(symbol(DELTA), _T("construction")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   }

   (*table2)(0, col++) << COLHDR(Sub2(symbol(DELTA), _T("creep2")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());

   if (bDeckPanels)
   {
      (*table2)(0, col++) << COLHDR(Sub2(symbol(DELTA), _T("deck panels")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   }

   (*table2)(0, col++) << COLHDR(Sub2(symbol(DELTA), _T("slab")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   (*table2)(0, col++) << COLHDR(Sub2(symbol(DELTA), _T("haunch")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   (*table2)(0, col++) << COLHDR(Sub2(symbol(DELTA), _T("User1")) << _T(" = ") << rptNewLine << Sub2(symbol(DELTA), _T("UserDC")) << _T(" + ") << rptNewLine << Sub2(symbol(DELTA), _T("UserDW")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());

   if (bSidewalk)
   {
      (*table2)(0, col++) << COLHDR(Sub2(symbol(DELTA), _T("sidewalk")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   }

   (*table2)(0, col++) << COLHDR(Sub2(symbol(DELTA), _T("barrier")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());

   if (bOverlay)
   {
      (*table2)(0, col++) << COLHDR(Sub2(symbol(DELTA), _T("overlay")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   }

   (*table2)(0, col++) << COLHDR(Sub2(symbol(DELTA), _T("User2")) << _T(" = ") << rptNewLine << Sub2(symbol(DELTA), _T("UserDC")) << _T(" + ") << rptNewLine << Sub2(symbol(DELTA), _T("UserDW")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());

   col = 0;
   (*table3)(0, col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table3)(0, col++) << COLHDR(Sub2(symbol(DELTA), _T("1")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   (*table3)(0, col++) << COLHDR(Sub2(symbol(DELTA), _T("2")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   (*table3)(0, col++) << COLHDR(Sub2(symbol(DELTA), _T("3")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());

   const auto& creep_criteria = pSpecEntry->GetCreepCriteria();
   Float64 days = creep_criteria.GetCreepDuration2(constructionRate);
   days = WBFL::Units::ConvertFromSysUnits(days, WBFL::Units::Measure::Day);
   std::_tostringstream os;
   os << days;
   (*table3)(0, col++) << COLHDR(Sub2(_T("D"), os.str().c_str()) << _T(" = ") << Sub2(symbol(DELTA), _T("4")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   (*table3)(0, col++) << COLHDR(Sub2(symbol(DELTA), _T("5")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   (*table3)(0, col++) << COLHDR(Sub2(symbol(DELTA), _T("excess")) << _T(" = ") << rptNewLine << Sub2(symbol(DELTA), _T("6")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   (*table3)(0, col++) << COLHDR(_T("C = ") << Sub2(symbol(DELTA), _T("4")) << _T(" - ") << Sub2(symbol(DELTA), _T("6")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());

   pgsTypes::BridgeAnalysisType bat = pProduct->GetBridgeAnalysisType(pgsTypes::Minimize);

   // Fill up the tables
   RowIndexType row1a = table1a->GetNumberOfHeaderRows();
   RowIndexType row1b = table1b->GetNumberOfHeaderRows();
   RowIndexType row2 = table2->GetNumberOfHeaderRows();
   RowIndexType row3 = table3->GetNumberOfHeaderRows();
   auto releasePoiIter(vPoiRelease.begin());
   auto releasePoiIterEnd(vPoiRelease.end());
   auto storagePoiIter(vPoiStorage.begin());
   auto erectedPoiIter(vPoiErected.begin());
   for (; releasePoiIter != releasePoiIterEnd; releasePoiIter++, storagePoiIter++, erectedPoiIter++)
   {
      const pgsPointOfInterest& releasePoi(*releasePoiIter);
      const pgsPointOfInterest& storagePoi(*storagePoiIter);
      const pgsPointOfInterest& erectedPoi(*erectedPoiIter);

      IndexType deckCastingRegionIdx = pPoi->GetDeckCastingRegion(erectedPoi);
      IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval(deckCastingRegionIdx);
      IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval(deckCastingRegionIdx);


      Float64 DpsRelease = pProduct->GetDeflection(releaseIntervalIdx, pgsTypes::pftPretension, releasePoi, bat, rtCumulative, false);
      Float64 DpsStorage = pProduct->GetDeflection(storageIntervalIdx, pgsTypes::pftPretension, storagePoi, bat, rtCumulative, false);
      Float64 DpsErected = pProduct->GetDeflection(erectionIntervalIdx, pgsTypes::pftPretension, erectedPoi, bat, rtCumulative, false);

      Float64 DgdrRelease = pProduct->GetDeflection(releaseIntervalIdx, pgsTypes::pftGirder, releasePoi, bat, rtCumulative, false);
      Float64 DgdrStorage = pProduct->GetDeflection(storageIntervalIdx, pgsTypes::pftGirder, storagePoi, bat, rtCumulative, false);
      Float64 DgdrErected = pProduct->GetDeflection(erectionIntervalIdx, pgsTypes::pftGirder, erectedPoi, bat, rtCumulative, false);
      Float64 dgdrErected = pExtLoading->GetDeflection(erectionIntervalIdx, _T("Girder_Incremental"), erectedPoi, bat, rtCumulative, false);

      Float64 Dtpsr = bTempStrands ? pCamber->GetReleaseTempPrestressDeflection(erectedPoi) : 0.0;

      Float64 DprecamberRelease = pCamber->GetPrecamber(releasePoi, pgsTypes::pddRelease);
      Float64 DprecamberStorage = pCamber->GetPrecamber(storagePoi, pgsTypes::pddStorage);
      Float64 DprecamberErected = pCamber->GetPrecamber(erectedPoi, pgsTypes::pddErected);

      // NOTE: Get the creep deflection from the ICamber interface because it takes the construction rate 
      // into account. Getting creep deflection as a product load assumes the maximum construction rate
      Float64 Dcreep1a = pCamber->GetCreepDeflection(storagePoi, ICamber::cpReleaseToDiaphragm, constructionRate, pgsTypes::pddStorage);
      Float64 Dcreep1b = pCamber->GetCreepDeflection(erectedPoi, ICamber::cpReleaseToDiaphragm, constructionRate, pgsTypes::pddErected);
      Float64 Ddiaphragm = pCamber->GetDiaphragmDeflection(erectedPoi);
      Float64 DshearKey = bShearKey ? pProduct->GetDeflection(castShearKeyIntervalIdx, pgsTypes::pftShearKey, erectedPoi, bat, rtCumulative, false) : 0.0;
      Float64 DlongJoint = bLongitudinalJoint ? pProduct->GetDeflection(castLongitudinalJointIntervalIdx, pgsTypes::pftLongitudinalJoint, erectedPoi, bat, rtCumulative, false) : 0.0;
      Float64 Dconstruction = bConstruction ? pProduct->GetDeflection(constructionLoadIntervalIdx, pgsTypes::pftConstruction, erectedPoi, bat, rtCumulative, false) : 0.0;
      Float64 Dpanel = bDeckPanels ? pProduct->GetDeflection(castDeckIntervalIdx, pgsTypes::pftSlabPanel, erectedPoi, bat, rtCumulative, false) : 0.0;
      Float64 Ddeck = pProduct->GetDeflection(castDeckIntervalIdx, pgsTypes::pftSlab, erectedPoi, bat, rtCumulative, false);
      Float64 DslabPad = pProduct->GetDeflection(castDeckIntervalIdx, pgsTypes::pftSlabPad, erectedPoi, bat, rtCumulative, false);
      Float64 Dcreep2 = pCamber->GetCreepDeflection(erectedPoi, ICamber::cpDiaphragmToDeck, constructionRate, pgsTypes::pddErected);
      Float64 Duser1 = pProduct->GetDeflection(castDeckIntervalIdx, pgsTypes::pftUserDC, erectedPoi, bat, rtCumulative, false)
         + pProduct->GetDeflection(castDeckIntervalIdx, pgsTypes::pftUserDW, erectedPoi, bat, rtCumulative, false);
      Float64 Duser2 = pProduct->GetDeflection(railingSystemIntervalIdx, pgsTypes::pftUserDC, erectedPoi, bat, rtCumulative, false)
         + pProduct->GetDeflection(railingSystemIntervalIdx, pgsTypes::pftUserDW, erectedPoi, bat, rtCumulative, false);
      Duser2 -= Duser1; // Duser2 is cumulative and it includes Duser1... remove Duser1
      Float64 Dbarrier = pProduct->GetDeflection(railingSystemIntervalIdx, pgsTypes::pftTrafficBarrier, erectedPoi, bat, rtCumulative, false);
      Float64 Dsidewalk = pProduct->GetDeflection(railingSystemIntervalIdx, pgsTypes::pftSidewalk, erectedPoi, bat, rtCumulative, false);
      Float64 Doverlay = (pBridge->HasOverlay() ? (pBridge->IsFutureOverlay() ? 0.0 : pProduct->GetDeflection(overlayIntervalIdx, pgsTypes::pftOverlay, erectedPoi, bat, rtCumulative, false)) : 0.0);

      // if we have a future overlay, the deflection due to the overlay in BridgeSite2 must be zero
      ATLASSERT(pBridge->IsFutureOverlay() ? IsZero(Doverlay) : true);


      Float64 Di = DpsRelease + DgdrRelease + DprecamberRelease; // initial camber
      Float64 Des = DpsStorage + DgdrStorage + DprecamberStorage + Dcreep1a; // camber at end of storage

                                                                             // Table 1a
      col = 0;
      (*table1a)(row1a, col++) << location.SetValue(POI_RELEASED_SEGMENT, releasePoi);
      (*table1a)(row1a, col++) << deflection.SetValue(DgdrRelease);
      (*table1a)(row1a, col++) << deflection.SetValue(DpsRelease);
      if (bHasPrecamber)
      {
         (*table1a)(row1a, col++) << deflection.SetValue(DprecamberRelease);
      }
      (*table1a)(row1a, col++) << deflection.SetValue(Di);
      row1a++;

      // Table 1b
      col = 0;
      (*table1b)(row1b, col++) << location.SetValue(POI_STORAGE_SEGMENT, storagePoi);
      (*table1b)(row1b, col++) << deflection.SetValue(DgdrStorage);
      (*table1b)(row1b, col++) << deflection.SetValue(DpsStorage);
      if (bHasPrecamber)
      {
         (*table1b)(row1b, col++) << deflection.SetValue(DprecamberStorage);
      }
      (*table1b)(row1b, col++) << deflection.SetValue(Dcreep1a);
      (*table1b)(row1b, col++) << deflection.SetValue(Des);
      if (storagePoi.IsTenthPoint(POI_ERECTED_SEGMENT) == 1 || storagePoi.IsTenthPoint(POI_ERECTED_SEGMENT) == 11)
      {
         for (ColumnIndexType i = 0; i < col; i++)
         {
            (*table1b)(row1b, i).InsertContent(0, bold(ON));
            (*table1b)(row1b, i) << bold(OFF);
         }
      }
      row1b++;

      // Table 2
      col = 0;
      (*table2)(row2, col++) << location.SetValue(POI_ERECTED_SEGMENT, erectedPoi);
      (*table2)(row2, col++) << deflection.SetValue(DgdrErected);
      (*table2)(row2, col++) << deflection.SetValue(DpsErected);
      (*table2)(row2, col++) << deflection.SetValue(dgdrErected);
      if (bHasPrecamber)
      {
         (*table2)(row2, col++) << deflection.SetValue(DprecamberErected);
      }
      (*table2)(row2, col++) << deflection.SetValue(Dcreep1b);

      if (bTempStrands)
      {
         (*table2)(row2, col++) << deflection.SetValue(Dtpsr);
      }

      (*table2)(row2, col++) << deflection.SetValue(Ddiaphragm);
      if (bShearKey)
      {
         (*table2)(row2, col++) << deflection.SetValue(DshearKey);
      }

      if (bLongitudinalJoint)
      {
         (*table2)(row2, col++) << deflection.SetValue(DlongJoint);
      }

      if (bConstruction)
      {
         (*table2)(row2, col++) << deflection.SetValue(Dconstruction);
      }

      (*table2)(row2, col++) << deflection.SetValue(Dcreep2);

      if (bDeckPanels)
      {
         (*table2)(row2, col++) << deflection.SetValue(Dpanel);
      }

      (*table2)(row2, col++) << deflection.SetValue(Ddeck);
      (*table2)(row2, col++) << deflection.SetValue(DslabPad);
      (*table2)(row2, col++) << deflection.SetValue(Duser1);

      if (bSidewalk)
      {
         (*table2)(row2, col++) << deflection.SetValue(Dsidewalk);
      }

      (*table2)(row2, col++) << deflection.SetValue(Dbarrier);

      if (bOverlay)
      {
         (*table2)(row2, col++) << deflection.SetValue(Doverlay);
      }

      (*table2)(row2, col++) << deflection.SetValue(Duser2);

      row2++;

      // Table 3
      col = 0;

      Float64 D1 = cm.ErectionFactor * (DgdrErected + DpsErected) + DprecamberErected;
      Float64 D2 = D1 + cm.CreepFactor * Dcreep1b;
      Float64 D3 = D2 + cm.DiaphragmFactor * (Ddiaphragm + DshearKey + DlongJoint + Dconstruction) + cm.ErectionFactor * Dtpsr;
      Float64 D4 = D3 + cm.CreepFactor * Dcreep2;
      Float64 D5 = D4 + cm.SlabUser1Factor * (Ddeck + Duser1) + cm.SlabPadLoadFactor*DslabPad + cm.DeckPanelFactor * Dpanel;;
      Float64 D6 = D5 + cm.BarrierSwOverlayUser2Factor * (Dbarrier + Duser2);
      if (bSidewalk)
      {
         D6 += cm.BarrierSwOverlayUser2Factor * Dsidewalk;
      }
      if (bOverlay)
      {
         D6 += cm.BarrierSwOverlayUser2Factor * Doverlay;
      }

      (*table3)(row3, col++) << location.SetValue(POI_ERECTED_SEGMENT, erectedPoi);
      (*table3)(row3, col++) << deflection.SetValue(D1);
      (*table3)(row3, col++) << deflection.SetValue(D2);
      (*table3)(row3, col++) << deflection.SetValue(D3);

      D4 = IsZero(D4) ? 0 : D4;
      if (D4 < 0)
      {
         (*table3)(row3, col++) << color(Red) << deflection.SetValue(D4) << color(Black);
      }
      else
      {
         (*table3)(row3, col++) << deflection.SetValue(D4);
      }

      (*table3)(row3, col++) << deflection.SetValue(D5);

      D6 = IsZero(D6) ? 0 : D6;
      if (D6 < 0)
      {
         (*table3)(row3, col++) << color(Red) << deflection.SetValue(D6) << color(Black);
      }
      else
      {
         (*table3)(row3, col++) << deflection.SetValue(D6);
      }
      (*table3)(row3, col++) << deflection.SetValue(D4 - D6);

#ifdef _DEBUG
      // Reality check with AnalysisAgent
      Float64 Dc = pCamber->GetDCamberForGirderSchedule(erectedPoi, constructionRate);
      ATLASSERT(IsEqual(Dc, D4, 0.001));

      Float64 Ec = pCamber->GetExcessCamber(erectedPoi, constructionRate);
      ATLASSERT(IsEqual(Ec, D6, 0.001));
#endif

      row3++;
   }

   *pTable1 = pLayoutTable;
   *pTable2 = table2;
   *pTable3 = table3;
}

void CCamberTable::Build_Deck_XY(IBroker* pBroker, const CSegmentKey& segmentKey,
   bool bTempStrands, bool bSidewalk, bool bShearKey, bool bLongitudinalJoint, bool bConstruction, bool bOverlay, bool bDeckPanels,
   IEAFDisplayUnits* pDisplayUnits, pgsTypes::CreepTime constructionRate, const CamberMultipliers& cm,
   rptRcTable** pTable1, rptRcTable** pTable2, rptRcTable** pTable3) const
{
   INIT_UV_PROTOTYPE(rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false);
   GET_IFACE2(pBroker,IReportOptions,pReportOptions);
   location.IncludeSpanAndGirder(pReportOptions->IncludeSpanAndGirder4Pois(segmentKey));

   INIT_UV_PROTOTYPE(rptLengthUnitValue, deflection, pDisplayUnits->GetDeflectionUnit(), false);

   GET_IFACE2(pBroker, ILibrary, pLib);
   GET_IFACE2(pBroker, ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());

   // Get the interface pointers we need
   PoiList vPoiRelease, vPoiStorage, vPoiErected;
   GetPointsOfInterest(pBroker, segmentKey, &vPoiRelease, &vPoiStorage, &vPoiErected);

   GET_IFACE2(pBroker, ICamber, pCamber);
   GET_IFACE2(pBroker, IExternalLoading, pExtLoading);
   GET_IFACE2(pBroker, IProductForces, pProduct);
   GET_IFACE2(pBroker, IBridge, pBridge);
   GET_IFACE2(pBroker, IPointOfInterest, pPoi);

   GET_IFACE2(pBroker, IIntervals, pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType storageIntervalIdx = pIntervals->GetStorageInterval(segmentKey);
   IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
   IntervalIndexType tempStrandRemovalIntervalIdx = pIntervals->GetTemporaryStrandRemovalInterval(segmentKey);
   IntervalIndexType castShearKeyIntervalIdx = pIntervals->GetCastShearKeyInterval();
   IntervalIndexType castLongitudinalJointIntervalIdx = pIntervals->GetCastLongitudinalJointInterval();
   IntervalIndexType constructionLoadIntervalIdx = pIntervals->GetConstructionLoadInterval();
   IntervalIndexType noncompositeUserLoadIntervalIdx = pIntervals->GetNoncompositeUserLoadInterval();
   IntervalIndexType compositeUserLoadIntervalIdx = pIntervals->GetCompositeUserLoadInterval();
   IntervalIndexType railingSystemIntervalIdx = pIntervals->GetInstallRailingSystemInterval();
   IntervalIndexType overlayIntervalIdx = pIntervals->GetOverlayInterval();

   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   // create the tables
   rptRcTable* table1a;
   rptRcTable* table1b;
   rptRcTable* table2;
   rptRcTable* table3;

   rptRcTable* pLayoutTable = rptStyleManager::CreateLayoutTable(2);

   bool bHasPrecamber = IsZero(pCamber->GetPrecamber(segmentKey)) ? false : true;

   ColumnIndexType nColumns = 7;
   if (bHasPrecamber)
   {
      nColumns++;
   }
   table1a = rptStyleManager::CreateDefaultTable(nColumns,_T("Deflections at Release"));

   nColumns = 9;
   if (bHasPrecamber)
   {
      nColumns++;
   }
   table1b = rptStyleManager::CreateDefaultTable(nColumns,_T("Deflections during Storage"));

   (*pLayoutTable)(0,0) << table1a;
   (*pLayoutTable)(0,1) << table1b;

   nColumns = 16 + (bHasPrecamber ? 1 : 0) + (bTempStrands ? 2 : 0) + (bSidewalk ? 1 : 0) + (bOverlay ? 1 : 0) + (bShearKey ? 1 : 0) + (bLongitudinalJoint ? 1 : 0) + (bConstruction ? 1 : 0) + (bDeckPanels ? 1 : 0);

   table2 = rptStyleManager::CreateDefaultTable(nColumns,_T("Deflections after Erection"));
   table3 = rptStyleManager::CreateDefaultTable(10,_T("Deflection Summary"));

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
   table1a->SetNumberOfHeaderRows(2);
   table1a->SetRowSpan(0, col, 2);
   (*table1a)(0, col++) << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

   table1a->SetColumnSpan(0, col, 3);
   (*table1a)(0, col) << _T("X");

   (*table1a)(1, col++) << COLHDR(Sub2(symbol(DELTA), _T("girder")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   (*table1a)(1, col++) << COLHDR(Sub2(symbol(DELTA), _T("ps")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   (*table1a)(1, col++) << COLHDR(Sub2(symbol(DELTA), _T("i")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());

   table1a->SetColumnSpan(0, col, bHasPrecamber ? 4 : 3);
   (*table1a)(0, col) << _T("Y");

   (*table1a)(1, col++) << COLHDR(Sub2(symbol(DELTA), _T("girder")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   (*table1a)(1, col++) << COLHDR(Sub2(symbol(DELTA), _T("ps")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   if (bHasPrecamber)
   {
      (*table1a)(1, col++) << COLHDR(Sub2(symbol(DELTA), _T("precamber")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   }
   (*table1a)(1, col++) << COLHDR(Sub2(symbol(DELTA), _T("i")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());

   ///////////////////////////
   col = 0;
   table1b->SetNumberOfHeaderRows(2);
   table1b->SetRowSpan(0, col, 2);
   (*table1b)(0, col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

   table1b->SetColumnSpan(0, col, 4);
   (*table1b)(0, col) << _T("X");

   (*table1b)(1, col++) << COLHDR(Sub2(symbol(DELTA), _T("girder")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   (*table1b)(1, col++) << COLHDR(Sub2(symbol(DELTA), _T("ps")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   (*table1b)(1, col++) << COLHDR(Sub2(symbol(DELTA), _T("creep1")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   (*table1b)(1, col++) << COLHDR(Sub2(symbol(DELTA), _T("es")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());

   table1b->SetColumnSpan(0, col, bHasPrecamber ? 5 : 4);
   (*table1b)(0, col) << _T("Y");

   (*table1b)(1, col++) << COLHDR(Sub2(symbol(DELTA), _T("girder")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   (*table1b)(1, col++) << COLHDR(Sub2(symbol(DELTA), _T("ps")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   if (bHasPrecamber)
   {
      (*table1b)(1, col++) << COLHDR(Sub2(symbol(DELTA), _T("precamber")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   }
   (*table1b)(1, col++) << COLHDR(Sub2(symbol(DELTA), _T("creep1")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   (*table1b)(1, col++) << COLHDR(Sub2(symbol(DELTA), _T("es")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());

   ////////////////////
   col = 0;
   table2->SetNumberOfHeaderRows(2);

   table2->SetRowSpan(0, col, 2);
   (*table2)(0, col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

   table2->SetColumnSpan(0, col, bTempStrands ? 5 : 4);
   (*table2)(0, col) << _T("X");
   (*table2)(1, col++) << COLHDR(Sub2(symbol(DELTA), _T("girder")) << rptNewLine << _T("Erected"), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   (*table2)(1, col++) << COLHDR(Sub2(symbol(DELTA), _T("ps")) << rptNewLine << _T("Erected"), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   (*table2)(1, col++) << COLHDR(Sub2(symbol(delta), _T("girder")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   (*table2)(1, col++) << COLHDR(Sub2(symbol(DELTA), _T("creep1")) << rptNewLine << _T("Erected"), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   if (bTempStrands)
   {
      (*table2)(1, col++) << COLHDR(Sub2(symbol(DELTA), _T("tpsr")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   }

   ColumnIndexType colSpan = 11 + (bHasPrecamber ? 1 : 0) + (bTempStrands ? 1 : 0) + (bSidewalk ? 1 : 0) + (bOverlay ? 1 : 0) + (bShearKey ? 1 : 0) + (bConstruction ? 1 : 0) + (bDeckPanels ? 1 : 0);
   table2->SetColumnSpan(0, col, colSpan);
   (*table2)(0, col) << _T("Y");
   (*table2)(1, col++) << COLHDR(Sub2(symbol(DELTA), _T("girder")) << rptNewLine << _T("Erected"), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   (*table2)(1, col++) << COLHDR(Sub2(symbol(DELTA), _T("ps")) << rptNewLine << _T("Erected"), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   (*table2)(1, col++) << COLHDR(Sub2(symbol(delta), _T("girder")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());

   if (bHasPrecamber)
   {
      (*table2)(1, col++) << COLHDR(Sub2(symbol(DELTA), _T("precamber")) << rptNewLine << _T("Erected"), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   }

   (*table2)(1, col++) << COLHDR(Sub2(symbol(DELTA), _T("creep1")) << rptNewLine << _T("Erected"), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());

   if (bTempStrands)
   {
      (*table2)(1, col++) << COLHDR(Sub2(symbol(DELTA), _T("tpsr")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   }

   (*table2)(1, col++) << COLHDR(Sub2(symbol(DELTA), _T("diaphragm")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());

   if (bShearKey)
   {
      (*table2)(1, col++) << COLHDR(Sub2(symbol(DELTA), _T("shear key")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   }

   if (bLongitudinalJoint)
   {
      (*table2)(1, col++) << COLHDR(Sub2(symbol(DELTA), _T("long. joint")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   }

   if ( bConstruction )
   {
      (*table2)(1,col++) << COLHDR(Sub2(symbol(DELTA),_T("construction")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   }

   (*table2)(1,col++) << COLHDR(Sub2(symbol(DELTA),_T("creep2")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

   if (bDeckPanels)
   {
      (*table2)(1,col++) << COLHDR(Sub2(symbol(DELTA),_T("deck panels")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   }

   (*table2)(1,col++) << COLHDR(Sub2(symbol(DELTA),_T("slab")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table2)(1,col++) << COLHDR(Sub2(symbol(DELTA),_T("haunch")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table2)(1,col++) << COLHDR(Sub2(symbol(DELTA),_T("User1")) << _T(" = ") << rptNewLine << Sub2(symbol(DELTA),_T("UserDC")) << _T(" + ") << rptNewLine << Sub2(symbol(DELTA),_T("UserDW")) , rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

   if ( bSidewalk )
   {
      (*table2)(1,col++) << COLHDR(Sub2(symbol(DELTA),_T("sidewalk")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   }

   (*table2)(1,col++) << COLHDR(Sub2(symbol(DELTA),_T("barrier")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

   if ( bOverlay )
   {
      (*table2)(1,col++) << COLHDR(Sub2(symbol(DELTA),_T("overlay")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   }

   (*table2)(1,col++) << COLHDR(Sub2(symbol(DELTA),_T("User2")) << _T(" = ") << rptNewLine << Sub2(symbol(DELTA),_T("UserDC")) << _T(" + ") << rptNewLine << Sub2(symbol(DELTA),_T("UserDW")) , rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

   //////////////////////////////////////////////
   col = 0;
   table3->SetNumberOfHeaderRows(2);

   table3->SetRowSpan(0, col, 2);
   (*table3)(0, col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

   table3->SetColumnSpan(0, col, 2);
   (*table3)(0, col) << _T("X");

   (*table3)(1, col++) << COLHDR(Sub2(symbol(DELTA), _T("1")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   (*table3)(1, col++) << COLHDR(Sub2(symbol(DELTA), _T("2")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());

   table3->SetColumnSpan(0, col, 7);
   (*table3)(0, col) << _T("Y");

   (*table3)(1, col++) << COLHDR(Sub2(symbol(DELTA), _T("1")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   (*table3)(1, col++) << COLHDR(Sub2(symbol(DELTA), _T("2")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   (*table3)(1, col++) << COLHDR(Sub2(symbol(DELTA), _T("3")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());

   const auto& creep_criteria = pSpecEntry->GetCreepCriteria();
   Float64 days = creep_criteria.GetCreepDuration2(constructionRate);
   days = WBFL::Units::ConvertFromSysUnits(days,WBFL::Units::Measure::Day);
   std::_tostringstream os;
   os << days;
   (*table3)(1,col++) << COLHDR(Sub2(_T("D"),os.str().c_str()) << _T(" = ") << Sub2(symbol(DELTA),_T("4")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table3)(1,col++) << COLHDR(Sub2(symbol(DELTA),_T("5")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table3)(1,col++) << COLHDR(Sub2(symbol(DELTA),_T("excess")) << _T(" = ") << rptNewLine << Sub2(symbol(DELTA),_T("6")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table3)(1,col++) << COLHDR(_T("C = ") << Sub2(symbol(DELTA),_T("4")) << _T(" - ") << Sub2(symbol(DELTA),_T("6")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

   pgsTypes::BridgeAnalysisType bat = pProduct->GetBridgeAnalysisType(pgsTypes::Minimize);

   // Fill up the tables
   RowIndexType row1a = table1a->GetNumberOfHeaderRows();
   RowIndexType row1b = table1b->GetNumberOfHeaderRows();
   RowIndexType row2 = table2->GetNumberOfHeaderRows();
   RowIndexType row3 = table3->GetNumberOfHeaderRows();
   auto releasePoiIter(vPoiRelease.begin());
   auto releasePoiIterEnd(vPoiRelease.end());
   auto storagePoiIter(vPoiStorage.begin());
   auto erectedPoiIter(vPoiErected.begin());
   for ( ; releasePoiIter != releasePoiIterEnd; releasePoiIter++, storagePoiIter++, erectedPoiIter++ )
   {
      const pgsPointOfInterest& releasePoi(*releasePoiIter);
      const pgsPointOfInterest& storagePoi(*storagePoiIter);
      const pgsPointOfInterest& erectedPoi(*erectedPoiIter);

      IndexType deckCastingRegionIdx = pPoi->GetDeckCastingRegion(erectedPoi);
      IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval(deckCastingRegionIdx);

      Float64 DpsRelease  = pProduct->GetDeflection(releaseIntervalIdx,pgsTypes::pftPretension,releasePoi,bat,rtCumulative,false);
      Float64 DpsStorage  = pProduct->GetDeflection(storageIntervalIdx,pgsTypes::pftPretension,storagePoi,bat,rtCumulative,false);
      Float64 DpsErected  = pProduct->GetDeflection(erectionIntervalIdx,pgsTypes::pftPretension,erectedPoi,bat,rtCumulative,false);

      Float64 DgdrRelease = pProduct->GetDeflection(releaseIntervalIdx,pgsTypes::pftGirder,releasePoi,bat,rtCumulative,false);
      Float64 DgdrStorage = pProduct->GetDeflection(storageIntervalIdx,pgsTypes::pftGirder,storagePoi,bat,rtCumulative,false);
      Float64 DgdrErected = pProduct->GetDeflection(erectionIntervalIdx,pgsTypes::pftGirder,erectedPoi,bat,rtCumulative,false);
      Float64 dgdrErected = pExtLoading->GetDeflection(erectionIntervalIdx,_T("Girder_Incremental"),erectedPoi,bat,rtCumulative,false);
      Float64 dxgdrErected = pExtLoading->GetXDeflection(erectionIntervalIdx, _T("Girder_Incremental"), erectedPoi, bat, rtCumulative);

      Float64 DXpsRelease = pProduct->GetXDeflection(releaseIntervalIdx, pgsTypes::pftPretension, releasePoi, bat, rtCumulative);
      Float64 DXgdrRelease = pProduct->GetXDeflection(releaseIntervalIdx, pgsTypes::pftGirder, releasePoi, bat, rtCumulative);

      Float64 DXpsStorage = pProduct->GetXDeflection(storageIntervalIdx, pgsTypes::pftPretension, storagePoi, bat, rtCumulative);
      Float64 DXgdrStorage = pProduct->GetXDeflection(storageIntervalIdx, pgsTypes::pftGirder, storagePoi, bat, rtCumulative);

      Float64 DXpsErected = pProduct->GetXDeflection(erectionIntervalIdx, pgsTypes::pftPretension, erectedPoi, bat, rtCumulative);
      Float64 DXgdrErected = pProduct->GetXDeflection(erectionIntervalIdx, pgsTypes::pftGirder, erectedPoi, bat, rtCumulative);

      Float64 Dtpsr(0.0), DXtpsr(0.0);
      if (bTempStrands)
      {
         pCamber->GetReleaseTempPrestressDeflection(erectedPoi, nullptr/*gdr config*/, &DXtpsr, &Dtpsr);
      }

      Float64 DprecamberRelease = pCamber->GetPrecamber(releasePoi, pgsTypes::pddRelease);
      Float64 DprecamberStorage = pCamber->GetPrecamber(storagePoi, pgsTypes::pddStorage);
      Float64 DprecamberErected = pCamber->GetPrecamber(erectedPoi, pgsTypes::pddErected);

      // NOTE: Get the creep deflection from the ICamber interface because it takes the construction rate 
      // into account. Getting creep deflection as a product load assumes the maximum construction rate
      Float64 Dcreep1a   = pCamber->GetCreepDeflection( storagePoi, ICamber::cpReleaseToDiaphragm, constructionRate, pgsTypes::pddStorage );
      Float64 DXcreep1a = pCamber->GetXCreepDeflection(storagePoi, ICamber::cpReleaseToDiaphragm, constructionRate, pgsTypes::pddStorage);
      Float64 Dcreep1b   = pCamber->GetCreepDeflection( erectedPoi, ICamber::cpReleaseToDiaphragm, constructionRate, pgsTypes::pddErected );
      Float64 DXcreep1b = pCamber->GetXCreepDeflection(erectedPoi, ICamber::cpReleaseToDiaphragm, constructionRate, pgsTypes::pddErected);
      Float64 Ddiaphragm = pCamber->GetDiaphragmDeflection( erectedPoi );
      Float64 DshearKey = bShearKey ? pProduct->GetDeflection(castShearKeyIntervalIdx, pgsTypes::pftShearKey, erectedPoi, bat, rtCumulative, false) : 0.0;
      Float64 DlongJoint = bLongitudinalJoint ? pProduct->GetDeflection(castLongitudinalJointIntervalIdx, pgsTypes::pftLongitudinalJoint, erectedPoi, bat, rtCumulative, false) : 0.0;
      Float64 Dconstruction= bConstruction ? pProduct->GetDeflection(constructionLoadIntervalIdx,pgsTypes::pftConstruction,erectedPoi,bat, rtCumulative, false) : 0.0;
      Float64 Dpanel     =  bDeckPanels ? pProduct->GetDeflection(castDeckIntervalIdx,pgsTypes::pftSlabPanel,erectedPoi,bat, rtCumulative, false) : 0.0;
      Float64 Ddeck      = pProduct->GetDeflection(castDeckIntervalIdx,pgsTypes::pftSlab,erectedPoi,bat, rtCumulative, false);
      Float64 DslabPad   = pProduct->GetDeflection(castDeckIntervalIdx,pgsTypes::pftSlabPad,erectedPoi,bat, rtCumulative, false);
      Float64 Dcreep2    = pCamber->GetCreepDeflection( erectedPoi, ICamber::cpDiaphragmToDeck, constructionRate, pgsTypes::pddErected );
      Float64 Duser1     = pProduct->GetDeflection(noncompositeUserLoadIntervalIdx,pgsTypes::pftUserDC,erectedPoi,bat, rtCumulative, false)
                         + pProduct->GetDeflection(noncompositeUserLoadIntervalIdx,pgsTypes::pftUserDW,erectedPoi,bat, rtCumulative, false);
      Float64 Duser2     = pProduct->GetDeflection(compositeUserLoadIntervalIdx,pgsTypes::pftUserDC,erectedPoi,bat, rtCumulative, false)
                         + pProduct->GetDeflection(compositeUserLoadIntervalIdx,pgsTypes::pftUserDW,erectedPoi,bat, rtCumulative, false);
      Duser2 -= Duser1; // Duser2 is cumulative and it includes Duser1... remove Duser1
      Float64 Dbarrier   = pProduct->GetDeflection(railingSystemIntervalIdx,pgsTypes::pftTrafficBarrier,erectedPoi,bat, rtCumulative, false);
      Float64 Dsidewalk  = pProduct->GetDeflection(railingSystemIntervalIdx,pgsTypes::pftSidewalk,      erectedPoi,bat, rtCumulative, false);
      Float64 Doverlay   = (pBridge->HasOverlay() ? (pBridge->IsFutureOverlay() ? 0.0 : pProduct->GetDeflection(overlayIntervalIdx,pgsTypes::pftOverlay,erectedPoi,bat, rtCumulative, false)) : 0.0);

      // if we have a future overlay, the deflection due to the overlay in BridgeSite2 must be zero
      ATLASSERT( pBridge->IsFutureOverlay() ? IsZero(Doverlay) : true );


      Float64 Di = DpsRelease + DgdrRelease + DprecamberRelease; // initial camber
      Float64 Des = DpsStorage + DgdrStorage + DprecamberStorage + Dcreep1a; // camber at end of storage

      Float64 DXi = DXpsRelease + DXgdrRelease;
      Float64 DXes = DXpsStorage + DXgdrStorage + DXcreep1a;

      // Table 1a
      col = 0;
      (*table1a)(row1a,col++) << location.SetValue( POI_RELEASED_SEGMENT, releasePoi );

      (*table1a)(row1a, col++) << deflection.SetValue(DXgdrRelease);
      (*table1a)(row1a, col++) << deflection.SetValue(DXpsRelease);
      (*table1a)(row1a, col++) << deflection.SetValue(DXi);

      (*table1a)(row1a, col++) << deflection.SetValue(DgdrRelease);
      (*table1a)(row1a, col++) << deflection.SetValue(DpsRelease);
      if (bHasPrecamber)
      {
         (*table1a)(row1a, col++) << deflection.SetValue(DprecamberRelease);
      }
      (*table1a)(row1a, col++) << deflection.SetValue(Di);

      row1a++;

      // Table 1b
      col = 0;
      (*table1b)(row1b,col++) << location.SetValue( POI_STORAGE_SEGMENT, storagePoi );

      (*table1b)(row1b, col++) << deflection.SetValue(DXgdrStorage);
      (*table1b)(row1b, col++) << deflection.SetValue(DXpsStorage);
      (*table1b)(row1b, col++) << deflection.SetValue(DXcreep1a);
      (*table1b)(row1b, col++) << deflection.SetValue(DXes);

      (*table1b)(row1b, col++) << deflection.SetValue(DgdrStorage);
      (*table1b)(row1b, col++) << deflection.SetValue(DpsStorage);
      if (bHasPrecamber)
      {
         (*table1b)(row1b, col++) << deflection.SetValue(DprecamberStorage);
      }
      (*table1b)(row1b, col++) << deflection.SetValue(Dcreep1a);
      (*table1b)(row1b, col++) << deflection.SetValue(Des);

      // make the data in the rows for the erected segment support locations bold
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
      (*table2)(row2, col++) << location.SetValue(POI_ERECTED_SEGMENT, erectedPoi);

      (*table2)(row2, col++) << deflection.SetValue(DXgdrErected);
      (*table2)(row2, col++) << deflection.SetValue(DXpsErected);
      (*table2)(row2, col++) << deflection.SetValue(dxgdrErected);
      (*table2)(row2, col++) << deflection.SetValue(DXcreep1b);
      
      if (bTempStrands)
      {
         (*table2)(row2, col++) << deflection.SetValue(DXtpsr);
      }

      (*table2)(row2, col++) << deflection.SetValue(DgdrErected);
      (*table2)(row2, col++) << deflection.SetValue(DpsErected);
      (*table2)(row2, col++) << deflection.SetValue(dgdrErected);

      if (bHasPrecamber)
      {
         (*table2)(row2, col++) << deflection.SetValue(DprecamberErected);
      }
      
      (*table2)(row2, col++) << deflection.SetValue(Dcreep1b);

      if (bTempStrands)
      {
         (*table2)(row2, col++) << deflection.SetValue(Dtpsr);
      }

      (*table2)(row2,col++) << deflection.SetValue( Ddiaphragm );
      
      if (bShearKey)
      {
         (*table2)(row2, col++) << deflection.SetValue(DshearKey);
      }

      if (bLongitudinalJoint)
      {
         (*table2)(row2, col++) << deflection.SetValue(DlongJoint);
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

      Float64 DX1 = cm.ErectionFactor * (DXgdrErected + DXpsErected);
      Float64 DX2 = DX1 + cm.CreepFactor * DXcreep1b;
      
      Float64 D1 = cm.ErectionFactor * (DgdrErected + DpsErected) + DprecamberErected;
      Float64 D2 = D1 + cm.CreepFactor * Dcreep1b;
      Float64 D3 = D2 + cm.DiaphragmFactor * (Ddiaphragm + DshearKey + DlongJoint + Dconstruction) + cm.ErectionFactor * Dtpsr;
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

      (*table3)(row3, col++) << deflection.SetValue(DX1);
      (*table3)(row3, col++) << deflection.SetValue(DX2);

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

      Float64 Sc = pCamber->GetScreedCamber(erectedPoi, constructionRate);
      ATLASSERT(IsEqual(Sc, D4 - D6));
#endif

      row3++;
   }

   *pTable1 = pLayoutTable;
   *pTable2 = table2;
   *pTable3 = table3;
}

void CCamberTable::Build_NoDeck_Y(IBroker* pBroker, const CSegmentKey& segmentKey,
   bool bTempStrands, bool bSidewalk, bool bShearKey, bool bLongitudinalJoint, bool bConstruction, bool bOverlay,
   IEAFDisplayUnits* pDisplayUnits, pgsTypes::CreepTime constructionRate, const CamberMultipliers& cm,
   rptRcTable** pTable1, rptRcTable** pTable2, rptRcTable** pTable3) const
{
   INIT_UV_PROTOTYPE(rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false);
   GET_IFACE2(pBroker,IReportOptions,pReportOptions);
   location.IncludeSpanAndGirder(pReportOptions->IncludeSpanAndGirder4Pois(segmentKey));

   INIT_UV_PROTOTYPE(rptLengthUnitValue, deflection, pDisplayUnits->GetDeflectionUnit(), false);

   GET_IFACE2(pBroker, ILibrary, pLib);
   GET_IFACE2(pBroker, ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());

   // Get the interface pointers we need
   PoiList vPoiRelease, vPoiStorage, vPoiErected;
   GetPointsOfInterest(pBroker, segmentKey, &vPoiRelease, &vPoiStorage, &vPoiErected);

   GET_IFACE2(pBroker, ICamber, pCamber);
   GET_IFACE2(pBroker, IExternalLoading, pExtLoading);
   GET_IFACE2(pBroker, IProductForces, pProduct);
   GET_IFACE2(pBroker, IBridge, pBridge);
   GET_IFACE2(pBroker, IPointOfInterest, pPoi);

   GET_IFACE2(pBroker, IIntervals, pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType storageIntervalIdx = pIntervals->GetStorageInterval(segmentKey);
   IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
   IntervalIndexType castShearKeyIntervalIdx = pIntervals->GetCastShearKeyInterval();
   IntervalIndexType castLongitudinalJointIntervalIdx = pIntervals->GetCastLongitudinalJointInterval();
   IntervalIndexType constructionLoadIntervalIdx = pIntervals->GetConstructionLoadInterval();
   IntervalIndexType tempStrandRemovalIntervalIdx = pIntervals->GetTemporaryStrandRemovalInterval(segmentKey);
   IntervalIndexType noncompositeUserLoadIntervalIdx = pIntervals->GetNoncompositeUserLoadInterval();
   IntervalIndexType compositeUserLoadIntervalIdx = pIntervals->GetCompositeUserLoadInterval();
   IntervalIndexType railingSystemIntervalIdx = pIntervals->GetInstallRailingSystemInterval();
   IntervalIndexType overlayIntervalIdx = pIntervals->GetOverlayInterval();

   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();

   bool bHasPrecamber = IsZero(pCamber->GetPrecamber(segmentKey)) ? false : true;

   // create the tables
   rptRcTable* table1a;
   rptRcTable* table1b;
   rptRcTable* table2;
   rptRcTable* table3;

   rptRcTable* pLayoutTable = rptStyleManager::CreateLayoutTable(2);

   ColumnIndexType nColumns = 4;
   if (bHasPrecamber)
   {
      nColumns++;
   }
   table1a = rptStyleManager::CreateDefaultTable(nColumns, _T("Deflections at Release"));
   table1b = rptStyleManager::CreateDefaultTable(nColumns + 1, _T("Deflections during Storage"));
   (*pLayoutTable)(0, 0) << table1a;
   (*pLayoutTable)(0, 1) << table1b;

   int ncols = 11 + (bTempStrands ? 1 : 0) + (bSidewalk ? 1 : 0) + (bOverlay ? 1 : 0) + (bShearKey ? 1 : 0) + (bLongitudinalJoint ? 1 : 0) + (bConstruction ? 1 : 0);
   if (bHasPrecamber)
   {
      ncols++;
   }

   if (deckType == pgsTypes::sdtNonstructuralOverlay)
   {
      ncols += 2;
   }

   table2 = rptStyleManager::CreateDefaultTable(ncols, _T("Deflections after Erection"));
   table3 = rptStyleManager::CreateDefaultTable(8, _T("Deflection Summary"));

   if (segmentKey.groupIndex == ALL_GROUPS)
   {
      table1a->SetColumnStyle(0, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table1a->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

      table1b->SetColumnStyle(0, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table1b->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

      table2->SetColumnStyle(0, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table2->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

      table3->SetColumnStyle(0, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table3->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   // Setup table headings
   ColumnIndexType col = 0;
   (*table1a)(0, col++) << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table1a)(0, col++) << COLHDR(Sub2(symbol(DELTA), _T("girder")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   (*table1a)(0, col++) << COLHDR(Sub2(symbol(DELTA), _T("ps")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   if (bHasPrecamber)
   {
      (*table1a)(0, col++) << COLHDR(Sub2(symbol(DELTA), _T("precamber")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   }
   (*table1a)(0, col++) << COLHDR(Sub2(symbol(DELTA), _T("i")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());

   col = 0;
   (*table1b)(0, col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table1b)(0, col++) << COLHDR(Sub2(symbol(DELTA), _T("girder")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   (*table1b)(0, col++) << COLHDR(Sub2(symbol(DELTA), _T("ps")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   if (bHasPrecamber)
   {
      (*table1b)(0, col++) << COLHDR(Sub2(symbol(DELTA), _T("precamber")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   }
   (*table1b)(0, col++) << COLHDR(Sub2(symbol(DELTA), _T("creep1")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   (*table1b)(0, col++) << COLHDR(Sub2(symbol(DELTA), _T("es")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());

   col = 0;
   (*table2)(0, col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table2)(0, col++) << COLHDR(Sub2(symbol(DELTA), _T("girder")) << rptNewLine << _T("Erected"), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   (*table2)(0, col++) << COLHDR(Sub2(symbol(DELTA), _T("ps")) << rptNewLine << _T("Erected"), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   (*table2)(0, col++) << COLHDR(Sub2(symbol(delta), _T("girder")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   if (bHasPrecamber)
   {
      (*table2)(0, col++) << COLHDR(Sub2(symbol(DELTA), _T("precamber")) << rptNewLine << _T("Erected"), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   }
   (*table2)(0, col++) << COLHDR(Sub2(symbol(DELTA), _T("creep1")) << rptNewLine << _T("Erected"), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());

   if (bTempStrands)
   {
      (*table2)(0, col++) << COLHDR(Sub2(symbol(DELTA), _T("tpsr")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   }

   (*table2)(0, col++) << COLHDR(Sub2(symbol(DELTA), _T("diaphragm")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());

   if (bShearKey)
   {
      (*table2)(0, col++) << COLHDR(Sub2(symbol(DELTA), _T("shear key")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   }

   if (bLongitudinalJoint)
   {
      (*table2)(0, col++) << COLHDR(Sub2(symbol(DELTA), _T("long. joint")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   }

   if (bConstruction)
   {
      (*table2)(0, col++) << COLHDR(Sub2(symbol(DELTA), _T("construction")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   }

   (*table2)(0, col++) << COLHDR(Sub2(symbol(DELTA), _T("creep2")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());

   if (deckType == pgsTypes::sdtNonstructuralOverlay)
   {
      (*table2)(0, col++) << COLHDR(Sub2(symbol(DELTA), _T("slab")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
      (*table2)(0, col++) << COLHDR(Sub2(symbol(DELTA), _T("haunch")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   }

   (*table2)(0, col++) << COLHDR(Sub2(symbol(DELTA), _T("User1")) << _T(" = ") << rptNewLine << Sub2(symbol(DELTA), _T("UserDC")) << _T(" + ") << rptNewLine << Sub2(symbol(DELTA), _T("UserDW")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());

   if (bSidewalk)
   {
      (*table2)(0, col++) << COLHDR(Sub2(symbol(DELTA), _T("sidewalk")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   }

   (*table2)(0, col++) << COLHDR(Sub2(symbol(DELTA), _T("barrier")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());

   if (bOverlay)
   {
      (*table2)(0, col++) << COLHDR(Sub2(symbol(DELTA), _T("overlay")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   }

   (*table2)(0, col++) << COLHDR(Sub2(symbol(DELTA), _T("User2")) << _T(" = ") << rptNewLine << Sub2(symbol(DELTA), _T("UserDC")) << _T(" + ") << rptNewLine << Sub2(symbol(DELTA), _T("UserDW")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   (*table2)(0, col++) << COLHDR(Sub2(symbol(DELTA), _T("creep3")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());

   col = 0;
   (*table3)(0, col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table3)(0, col++) << COLHDR(Sub2(symbol(DELTA), _T("1")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   (*table3)(0, col++) << COLHDR(Sub2(symbol(DELTA), _T("2")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   (*table3)(0, col++) << COLHDR(Sub2(symbol(DELTA), _T("3")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());

   const auto& creep_criteria = pSpecEntry->GetCreepCriteria();
   Float64 days = creep_criteria.GetCreepDuration2(constructionRate);
   days = WBFL::Units::ConvertFromSysUnits(days, WBFL::Units::Measure::Day);
   std::_tostringstream os;
   os << days;
   (*table3)(0, col++) << COLHDR(Sub2(_T("D"), os.str().c_str()) << _T(" = ") << Sub2(symbol(DELTA), _T("4")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   (*table3)(0, col++) << COLHDR(Sub2(symbol(DELTA), _T("5")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   (*table3)(0, col++) << COLHDR(Sub2(symbol(DELTA), _T("excess")) << _T(" = ") << rptNewLine << Sub2(symbol(DELTA), _T("6")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   (*table3)(0, col++) << COLHDR(_T("C = ") << Sub2(symbol(DELTA), _T("4")) << _T(" - ") << Sub2(symbol(DELTA), _T("6")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());

   pgsTypes::BridgeAnalysisType bat = pProduct->GetBridgeAnalysisType(pgsTypes::Minimize);

   // Fill up the tables
   RowIndexType row1a = table1a->GetNumberOfHeaderRows();
   RowIndexType row1b = table1b->GetNumberOfHeaderRows();
   RowIndexType row2 = table2->GetNumberOfHeaderRows();
   RowIndexType row3 = table3->GetNumberOfHeaderRows();
   auto releasePoiIter(vPoiRelease.begin());
   auto releasePoiIterEnd(vPoiRelease.end());
   auto storagePoiIter(vPoiStorage.begin());
   auto erectedPoiIter(vPoiErected.begin());
   for (; releasePoiIter != releasePoiIterEnd; releasePoiIter++, storagePoiIter++, erectedPoiIter++)
   {
      const pgsPointOfInterest& releasePoi(*releasePoiIter);
      const pgsPointOfInterest& storagePoi(*storagePoiIter);
      const pgsPointOfInterest& erectedPoi(*erectedPoiIter);

      IndexType deckCastingRegionIdx = pPoi->GetDeckCastingRegion(erectedPoi);
      IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval(deckCastingRegionIdx);
      
      Float64 DpsRelease = pProduct->GetDeflection(releaseIntervalIdx, pgsTypes::pftPretension, releasePoi, bat, rtCumulative, false);
      Float64 DpsStorage = pProduct->GetDeflection(storageIntervalIdx, pgsTypes::pftPretension, storagePoi, bat, rtCumulative, false);
      Float64 DpsErected = pProduct->GetDeflection(erectionIntervalIdx, pgsTypes::pftPretension, erectedPoi, bat, rtCumulative, false);

      Float64 DgdrRelease = pProduct->GetDeflection(releaseIntervalIdx, pgsTypes::pftGirder, releasePoi, bat, rtCumulative, false);
      Float64 DgdrStorage = pProduct->GetDeflection(storageIntervalIdx, pgsTypes::pftGirder, storagePoi, bat, rtCumulative, false);
      Float64 DgdrErected = pProduct->GetDeflection(erectionIntervalIdx, pgsTypes::pftGirder, erectedPoi, bat, rtCumulative, false);
      Float64 dgdrErected = pExtLoading->GetDeflection(erectionIntervalIdx, _T("Girder_Incremental"), erectedPoi, bat, rtCumulative, false);

      Float64 Dtpsr = bTempStrands ? pCamber->GetReleaseTempPrestressDeflection(erectedPoi) : 0.0;

      Float64 DprecamberRelease = pCamber->GetPrecamber(releasePoi, pgsTypes::pddRelease);
      Float64 DprecamberStorage = pCamber->GetPrecamber(storagePoi, pgsTypes::pddStorage);
      Float64 DprecamberErected = pCamber->GetPrecamber(erectedPoi, pgsTypes::pddErected);

      // NOTE: Get the creep deflection from the ICamber interface because it takes the construction rate 
      // into account. Getting creep deflection as a product load assumes the maximum construction rate
      Float64 Dcreep1a = pCamber->GetCreepDeflection(storagePoi, ICamber::cpReleaseToDiaphragm, constructionRate, pgsTypes::pddStorage);
      Float64 Dcreep1b = pCamber->GetCreepDeflection(erectedPoi, ICamber::cpReleaseToDiaphragm, constructionRate, pgsTypes::pddErected);
      Float64 Ddiaphragm = pCamber->GetDiaphragmDeflection(erectedPoi);
      Float64 DshearKey = bShearKey ? pProduct->GetDeflection(castShearKeyIntervalIdx, pgsTypes::pftShearKey, erectedPoi, bat, rtCumulative, false) : 0.0;
      Float64 DlongJoint = bLongitudinalJoint ? pProduct->GetDeflection(castLongitudinalJointIntervalIdx, pgsTypes::pftLongitudinalJoint, erectedPoi, bat, rtCumulative, false) : 0.0;
      Float64 Dconstruction = bConstruction ? pProduct->GetDeflection(constructionLoadIntervalIdx, pgsTypes::pftConstruction, erectedPoi, bat, rtCumulative, false) : 0.0;
      Float64 Dcreep2 = pCamber->GetCreepDeflection(erectedPoi, ICamber::cpDiaphragmToDeck, constructionRate, pgsTypes::pddErected);
      Float64 Duser1 = pProduct->GetDeflection(noncompositeUserLoadIntervalIdx, pgsTypes::pftUserDC, erectedPoi, bat, rtCumulative, false)
         + pProduct->GetDeflection(noncompositeUserLoadIntervalIdx, pgsTypes::pftUserDW, erectedPoi, bat, rtCumulative, false);
      Float64 Duser2 = pProduct->GetDeflection(compositeUserLoadIntervalIdx, pgsTypes::pftUserDC, erectedPoi, bat, rtCumulative, false)
         + pProduct->GetDeflection(compositeUserLoadIntervalIdx, pgsTypes::pftUserDW, erectedPoi, bat, rtCumulative, false);
      Duser2 -= Duser1; // Duser2 is cumulative and it includes Duser1... remove Duser1
      Float64 Dbarrier = pProduct->GetDeflection(railingSystemIntervalIdx, pgsTypes::pftTrafficBarrier, erectedPoi, bat, rtCumulative, false);
      Float64 Dsidewalk = pProduct->GetDeflection(railingSystemIntervalIdx, pgsTypes::pftSidewalk, erectedPoi, bat, rtCumulative, false);
      Float64 Doverlay = (pBridge->HasOverlay() ? (pBridge->IsFutureOverlay() ? 0.0 : pProduct->GetDeflection(overlayIntervalIdx, pgsTypes::pftOverlay, erectedPoi, bat, rtCumulative, false)) : 0.0);

      Float64 Dslab(0), Dhaunch(0);
      if (deckType == pgsTypes::sdtNonstructuralOverlay)
      {
         Dslab = pProduct->GetDeflection(castDeckIntervalIdx, pgsTypes::pftSlab, erectedPoi, bat, rtCumulative, false);
         Dhaunch = pProduct->GetDeflection(castDeckIntervalIdx, pgsTypes::pftSlabPad, erectedPoi, bat, rtCumulative, false);
      }

      Float64 Dcreep3 = pCamber->GetCreepDeflection(erectedPoi, ICamber::cpDeckToFinal, constructionRate, pgsTypes::pddErected);

      Float64 Di = DpsRelease + DgdrRelease + DprecamberRelease; // initial camber immedately after release
      Float64 Des = DpsStorage + DgdrStorage + DprecamberStorage + Dcreep1a; // camber at end of storage

                                                                             // if we have a future overlay, the deflection due to the overlay in BridgeSite2 must be zero
      ATLASSERT(pBridge->IsFutureOverlay() ? IsZero(Doverlay) : true);

      // Table 1a
      col = 0;
      (*table1a)(row1a, col++) << location.SetValue(POI_RELEASED_SEGMENT, releasePoi);
      (*table1a)(row1a, col++) << deflection.SetValue(DgdrRelease);
      (*table1a)(row1a, col++) << deflection.SetValue(DpsRelease);
      if (bHasPrecamber)
      {
         (*table1a)(row1a, col++) << deflection.SetValue(DprecamberRelease);
      }
      (*table1a)(row1a, col++) << deflection.SetValue(Di);
      row1a++;

      // Table 1b
      col = 0;
      (*table1b)(row1b, col++) << location.SetValue(POI_STORAGE_SEGMENT, storagePoi);
      (*table1b)(row1b, col++) << deflection.SetValue(DgdrStorage);
      (*table1b)(row1b, col++) << deflection.SetValue(DpsStorage);
      if (bHasPrecamber)
      {
         (*table1b)(row1b, col++) << deflection.SetValue(DprecamberStorage);
      }
      (*table1b)(row1b, col++) << deflection.SetValue(Dcreep1a);
      (*table1b)(row1b, col++) << deflection.SetValue(Des);
      if (storagePoi.IsTenthPoint(POI_ERECTED_SEGMENT) == 1 || storagePoi.IsTenthPoint(POI_ERECTED_SEGMENT) == 11)
      {
         for (ColumnIndexType i = 0; i < col; i++)
         {
            (*table1b)(row1b, i).InsertContent(0, bold(ON));
            (*table1b)(row1b, i) << bold(OFF);
         }
      }
      row1b++;

      // Table 2
      col = 0;
      (*table2)(row2, col++) << location.SetValue(POI_ERECTED_SEGMENT, erectedPoi);
      (*table2)(row2, col++) << deflection.SetValue(DgdrErected);
      (*table2)(row2, col++) << deflection.SetValue(DpsErected);
      (*table2)(row2, col++) << deflection.SetValue(dgdrErected);
      if (bHasPrecamber)
      {
         (*table2)(row2, col++) << deflection.SetValue(DprecamberErected);
      }
      (*table2)(row2, col++) << deflection.SetValue(Dcreep1b);

      if (bTempStrands)
      {
         (*table2)(row2, col++) << deflection.SetValue(Dtpsr);
      }

      (*table2)(row2, col++) << deflection.SetValue(Ddiaphragm);
      if (bShearKey)
      {
         (*table2)(row2, col++) << deflection.SetValue(DshearKey);
      }

      if (bLongitudinalJoint)
      {
         (*table2)(row2, col++) << deflection.SetValue(DlongJoint);
      }

      if (bConstruction)
      {
         (*table2)(row2, col++) << deflection.SetValue(Dconstruction);
      }

      (*table2)(row2, col++) << deflection.SetValue(Dcreep2);

      (*table2)(row2, col++) << deflection.SetValue(Duser1);

      if (bSidewalk)
      {
         (*table2)(row2, col++) << deflection.SetValue(Dsidewalk);
      }

      (*table2)(row2, col++) << deflection.SetValue(Dbarrier);

      if (bOverlay)
      {
         (*table2)(row2, col++) << deflection.SetValue(Doverlay);
      }

      (*table2)(row2, col++) << deflection.SetValue(Duser2);

      if (deckType == pgsTypes::sdtNonstructuralOverlay)
      {
         (*table2)(row2, col++) << deflection.SetValue(Dslab);
         (*table2)(row2, col++) << deflection.SetValue(Dhaunch);
      }

      (*table2)(row2, col++) << deflection.SetValue(Dcreep3);

      row2++;

      // Table 3
      col = 0;

      Float64 D1 = cm.ErectionFactor * (DgdrErected + DpsErected) + DprecamberErected;
      Float64 D2 = D1 + cm.CreepFactor * Dcreep1b;
      Float64 D3 = D2 + cm.DiaphragmFactor * (Ddiaphragm + DshearKey + DlongJoint + Dconstruction) + cm.ErectionFactor * Dtpsr + cm.SlabUser1Factor * Duser1;
      Float64 D4 = D3 + cm.CreepFactor * Dcreep2;
      Float64 D5 = D4 + cm.BarrierSwOverlayUser2Factor * (Dbarrier + Duser2) + cm.SlabUser1Factor*Dslab + cm.SlabPadLoadFactor*Dhaunch;
      if (bSidewalk)
      {
         D5 += cm.BarrierSwOverlayUser2Factor * Dsidewalk;
      }

      if (bOverlay)
      {
         D5 += cm.BarrierSwOverlayUser2Factor * Doverlay;
      }

      Float64 D6 = D5 + cm.CreepFactor * Dcreep3;

      (*table3)(row3, col++) << location.SetValue(POI_ERECTED_SEGMENT, erectedPoi);
      (*table3)(row3, col++) << deflection.SetValue(D1);
      (*table3)(row3, col++) << deflection.SetValue(D2);
      (*table3)(row3, col++) << deflection.SetValue(D3);

      D4 = IsZero(D4) ? 0 : D4;
      if (D4 < 0)
      {
         (*table3)(row3, col++) << color(Red) << deflection.SetValue(D4) << color(Black);
      }
      else
      {
         (*table3)(row3, col++) << deflection.SetValue(D4);
      }

      (*table3)(row3, col++) << deflection.SetValue(D5);

      D6 = IsZero(D6) ? 0 : D6;
      if (D6 < 0)
      {
         (*table3)(row3, col++) << color(Red) << deflection.SetValue(D6) << color(Black);
      }
      else
      {
         (*table3)(row3, col++) << deflection.SetValue(D6);
      }
      (*table3)(row3, col++) << deflection.SetValue(D4 - D6);

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

void CCamberTable::Build_NoDeck_XY(IBroker* pBroker,const CSegmentKey& segmentKey,
                                            bool bTempStrands, bool bSidewalk, bool bShearKey,bool bLongitudinalJoint,bool bConstruction, bool bOverlay,
                                            IEAFDisplayUnits* pDisplayUnits, pgsTypes::CreepTime constructionRate, const CamberMultipliers& cm,
                                            rptRcTable** pTable1,rptRcTable** pTable2,rptRcTable** pTable3) const
{
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   GET_IFACE2(pBroker,IReportOptions,pReportOptions);
   location.IncludeSpanAndGirder(pReportOptions->IncludeSpanAndGirder4Pois(segmentKey));

   INIT_UV_PROTOTYPE( rptLengthUnitValue, deflection, pDisplayUnits->GetDeflectionUnit(), false );

   GET_IFACE2(pBroker,ILibrary,pLib);
   GET_IFACE2(pBroker,ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());

   // Get the interface pointers we need
   PoiList vPoiRelease, vPoiStorage, vPoiErected;
   GetPointsOfInterest(pBroker,segmentKey,&vPoiRelease,&vPoiStorage,&vPoiErected);

   GET_IFACE2(pBroker,ICamber,pCamber);
   GET_IFACE2(pBroker,IExternalLoading,pExtLoading);
   GET_IFACE2(pBroker,IProductForces,pProduct);
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker, IPointOfInterest, pPoi);

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx           = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType storageIntervalIdx           = pIntervals->GetStorageInterval(segmentKey);
   IntervalIndexType erectionIntervalIdx          = pIntervals->GetErectSegmentInterval(segmentKey);
   IntervalIndexType tempStrandRemovalIntervalIdx = pIntervals->GetTemporaryStrandRemovalInterval(segmentKey);
   IntervalIndexType castShearKeyIntervalIdx = pIntervals->GetCastShearKeyInterval();
   IntervalIndexType castLongitudinalJointIntervalIdx = pIntervals->GetCastLongitudinalJointInterval();
   IntervalIndexType constructionLoadIntervalIdx = pIntervals->GetConstructionLoadInterval();
   IntervalIndexType noncompositeUserLoadIntervalIdx = pIntervals->GetNoncompositeUserLoadInterval();
   IntervalIndexType compositeUserLoadIntervalIdx = pIntervals->GetCompositeUserLoadInterval();
   IntervalIndexType railingSystemIntervalIdx     = pIntervals->GetInstallRailingSystemInterval();
   IntervalIndexType overlayIntervalIdx           = pIntervals->GetOverlayInterval();

   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();

   bool bHasPrecamber = IsZero(pCamber->GetPrecamber(segmentKey)) ? false : true;

   // create the tables
   rptRcTable* table1a;
   rptRcTable* table1b;
   rptRcTable* table2;
   rptRcTable* table3;

   rptRcTable* pLayoutTable = rptStyleManager::CreateLayoutTable(2);
   
   ColumnIndexType nColumns = 7;
   if (bHasPrecamber)
   {
      nColumns++;
   }
   table1a = rptStyleManager::CreateDefaultTable(nColumns, _T("Deflections at Release"));

   nColumns = 9;
   if (bHasPrecamber)
   {
      nColumns++;
   }

   table1b = rptStyleManager::CreateDefaultTable(nColumns, _T("Deflections during Storage"));

   (*pLayoutTable)(0, 0) << table1a;
   (*pLayoutTable)(0, 1) << table1b;

   nColumns = 15 + (bHasPrecamber ? 1 : 0) + (bTempStrands ? 2 : 0) + (bSidewalk ? 1 : 0) + (bOverlay ? 1 : 0) + (bShearKey ? 1 : 0) + (bLongitudinalJoint ? 1 : 0) + (bConstruction ? 1 : 0);

   if (deckType == pgsTypes::sdtNonstructuralOverlay)
   {
      nColumns += 2;
   }

   table2 = rptStyleManager::CreateDefaultTable(nColumns,_T("Deflections after Erection"));
   table3 = rptStyleManager::CreateDefaultTable(10,_T("Deflection Summary"));

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
   table1a->SetNumberOfHeaderRows(2);
   table1a->SetRowSpan(0, col, 2);
   (*table1a)(0,col++) << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

   table1a->SetColumnSpan(0, col, 3);
   (*table1a)(0, col) << _T("X");
   
   (*table1a)(1,col++) << COLHDR(Sub2(symbol(DELTA),_T("girder")),     rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table1a)(1,col++) << COLHDR(Sub2(symbol(DELTA),_T("ps")),         rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table1a)(1,col++) << COLHDR(Sub2(symbol(DELTA),_T("i")),          rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

   table1a->SetColumnSpan(0, col, bHasPrecamber ? 4 : 3);
   (*table1a)(0, col) << _T("Y");

   (*table1a)(1, col++) << COLHDR(Sub2(symbol(DELTA), _T("girder")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   (*table1a)(1, col++) << COLHDR(Sub2(symbol(DELTA), _T("ps")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   if (bHasPrecamber)
   {
      (*table1a)(1, col++) << COLHDR(Sub2(symbol(DELTA), _T("precamber")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   }
   (*table1a)(1, col++) << COLHDR(Sub2(symbol(DELTA), _T("i")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());

   ///////////////////////////
   col = 0;
   table1b->SetNumberOfHeaderRows(2);
   table1b->SetRowSpan(0, col, 2);
   (*table1b)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

   table1b->SetColumnSpan(0, col, 4);
   (*table1b)(0, col) << _T("X");

   (*table1b)(1, col++) << COLHDR(Sub2(symbol(DELTA), _T("girder")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   (*table1b)(1, col++) << COLHDR(Sub2(symbol(DELTA), _T("ps")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   (*table1b)(1, col++) << COLHDR(Sub2(symbol(DELTA), _T("creep1")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   (*table1b)(1, col++) << COLHDR(Sub2(symbol(DELTA), _T("es")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());

   table1b->SetColumnSpan(0, col, bHasPrecamber ? 5 : 4);
   (*table1b)(0, col) << _T("Y");

   (*table1b)(1,col++) << COLHDR(Sub2(symbol(DELTA),_T("girder")),     rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table1b)(1,col++) << COLHDR(Sub2(symbol(DELTA),_T("ps")),         rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   if (bHasPrecamber)
   {
      (*table1b)(1, col++) << COLHDR(Sub2(symbol(DELTA), _T("precamber")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   }
   (*table1b)(1,col++) << COLHDR(Sub2(symbol(DELTA),_T("creep1")),     rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table1b)(1,col++) << COLHDR(Sub2(symbol(DELTA),_T("es")),         rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

   ////////////////////
   col = 0;
   table2->SetNumberOfHeaderRows(2);

   table2->SetRowSpan(0, col, 2);
   (*table2)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

   table2->SetColumnSpan(0, col, bTempStrands ? 5 : 4);
   (*table2)(0, col) << _T("X");
   (*table2)(1, col++) << COLHDR(Sub2(symbol(DELTA), _T("girder")) << rptNewLine << _T("Erected"), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   (*table2)(1, col++) << COLHDR(Sub2(symbol(DELTA), _T("ps")) << rptNewLine << _T("Erected"), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   (*table2)(1, col++) << COLHDR(Sub2(symbol(delta), _T("girder")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   (*table2)(1, col++) << COLHDR(Sub2(symbol(DELTA), _T("creep1")) << rptNewLine << _T("Erected"), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   if (bTempStrands)
   {
      (*table2)(1, col++) << COLHDR(Sub2(symbol(DELTA), _T("tpsr")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   }

   ColumnIndexType colSpan = 10 + (bHasPrecamber ? 1 : 0) + (bTempStrands ? 1 : 0) + (bShearKey ? 1 : 0) + (bConstruction ? 1 : 0) + (bSidewalk ? 1 : 0) + (bOverlay ? 1 : 0);
   if (deckType == pgsTypes::sdtNonstructuralOverlay)
   {
      colSpan += 2;
   }

   table2->SetColumnSpan(0, col, colSpan);
   (*table2)(0, col) << _T("Y");
   (*table2)(1,col++) << COLHDR(Sub2(symbol(DELTA),_T("girder")) << rptNewLine << _T("Erected"),     rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table2)(1,col++) << COLHDR(Sub2(symbol(DELTA),_T("ps")) << rptNewLine << _T("Erected"),         rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table2)(1,col++) << COLHDR(Sub2(symbol(delta),_T("girder")),     rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   if (bHasPrecamber)
   {
      (*table2)(1, col++) << COLHDR(Sub2(symbol(DELTA), _T("precamber")) << rptNewLine << _T("Erected"), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   }
   (*table2)(1,col++) << COLHDR(Sub2(symbol(DELTA),_T("creep1")) << rptNewLine << _T("Erected"), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
    
   if (bTempStrands)
   {
      (*table2)(1,col++) << COLHDR(Sub2(symbol(DELTA),_T("tpsr")),         rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   }

   (*table2)(1,col++) << COLHDR(Sub2(symbol(DELTA),_T("diaphragm")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

   if (bShearKey)
   {
      (*table2)(1, col++) << COLHDR(Sub2(symbol(DELTA), _T("shear key")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   }

   if (bLongitudinalJoint)
   {
      (*table2)(1, col++) << COLHDR(Sub2(symbol(DELTA), _T("long. joint")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   }

   if ( bConstruction )
   {
      (*table2)(1,col++) << COLHDR(Sub2(symbol(DELTA),_T("construction")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   }

   (*table2)(1,col++) << COLHDR(Sub2(symbol(DELTA),_T("creep2")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

   if (deckType == pgsTypes::sdtNonstructuralOverlay)
   {
      (*table2)(1, col++) << COLHDR(Sub2(symbol(DELTA), _T("slab")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
      (*table2)(1, col++) << COLHDR(Sub2(symbol(DELTA), _T("haunch")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   }

   (*table2)(1,col++) << COLHDR(Sub2(symbol(DELTA),_T("User1")) << _T(" = ") << rptNewLine << Sub2(symbol(DELTA),_T("UserDC")) << _T(" + ") << rptNewLine << Sub2(symbol(DELTA),_T("UserDW")) , rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

   if ( bSidewalk )
   {
      (*table2)(1,col++) << COLHDR(Sub2(symbol(DELTA),_T("sidewalk")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   }

   (*table2)(1,col++) << COLHDR(Sub2(symbol(DELTA),_T("barrier")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

   if ( bOverlay )
   {
      (*table2)(1,col++) << COLHDR(Sub2(symbol(DELTA),_T("overlay")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   }

   (*table2)(1,col++) << COLHDR(Sub2(symbol(DELTA),_T("User2")) << _T(" = ") << rptNewLine << Sub2(symbol(DELTA),_T("UserDC")) << _T(" + ") << rptNewLine << Sub2(symbol(DELTA),_T("UserDW")) , rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

   (*table2)(1,col++) << COLHDR(Sub2(symbol(DELTA),_T("creep3")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

   ///////////////////////////
   col = 0;
   table3->SetNumberOfHeaderRows(2);

   table3->SetRowSpan(0, col, 2);
   (*table3)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

   table3->SetColumnSpan(0, col, 2);
   (*table3)(0, col) << _T("X");

   (*table3)(1,col++) << COLHDR(Sub2(symbol(DELTA),_T("1")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table3)(1,col++) << COLHDR(Sub2(symbol(DELTA),_T("2")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

   table3->SetColumnSpan(0, col, 7);
   (*table3)(0, col) << _T("Y");
   
   (*table3)(1, col++) << COLHDR(Sub2(symbol(DELTA), _T("1")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   (*table3)(1, col++) << COLHDR(Sub2(symbol(DELTA), _T("2")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   (*table3)(1, col++) << COLHDR(Sub2(symbol(DELTA), _T("3")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

   const auto& creep_criteria = pSpecEntry->GetCreepCriteria();
   Float64 days = creep_criteria.GetCreepDuration2(constructionRate);
   days = WBFL::Units::ConvertFromSysUnits(days,WBFL::Units::Measure::Day);
   std::_tostringstream os;
   os << days;
   (*table3)(1,col++) << COLHDR(Sub2(_T("D"),os.str().c_str()) << _T(" = ") << Sub2(symbol(DELTA),_T("4")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table3)(1,col++) << COLHDR(Sub2(symbol(DELTA),_T("5")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table3)(1,col++) << COLHDR(Sub2(symbol(DELTA),_T("excess")) << _T(" = ") << rptNewLine << Sub2(symbol(DELTA),_T("6")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*table3)(1,col++) << COLHDR(_T("C = ") << Sub2(symbol(DELTA),_T("4")) << _T(" - ") << Sub2(symbol(DELTA),_T("6")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

   pgsTypes::BridgeAnalysisType bat = pProduct->GetBridgeAnalysisType(pgsTypes::Minimize);

   // Fill up the tables
   RowIndexType row1a = table1a->GetNumberOfHeaderRows();
   RowIndexType row1b = table1b->GetNumberOfHeaderRows();
   RowIndexType row2 = table2->GetNumberOfHeaderRows();
   RowIndexType row3 = table3->GetNumberOfHeaderRows();
   auto releasePoiIter(vPoiRelease.begin());
   auto releasePoiIterEnd(vPoiRelease.end());
   auto storagePoiIter(vPoiStorage.begin());
   auto erectedPoiIter(vPoiErected.begin());
   for ( ; releasePoiIter != releasePoiIterEnd; releasePoiIter++, storagePoiIter++, erectedPoiIter++ )
   {
      const pgsPointOfInterest& releasePoi(*releasePoiIter);
      const pgsPointOfInterest& storagePoi(*storagePoiIter);
      const pgsPointOfInterest& erectedPoi(*erectedPoiIter);

      IndexType deckCastingRegionIdx = pPoi->GetDeckCastingRegion(erectedPoi);
      IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval(deckCastingRegionIdx);

      Float64 DpsRelease   = pProduct->GetDeflection(releaseIntervalIdx,pgsTypes::pftPretension,releasePoi,bat,rtCumulative,false);
      Float64 DpsStorage   = pProduct->GetDeflection(storageIntervalIdx,pgsTypes::pftPretension,storagePoi,bat,rtCumulative,false);
      Float64 DpsErected  = pProduct->GetDeflection(erectionIntervalIdx,pgsTypes::pftPretension,erectedPoi,bat,rtCumulative,false);

      Float64 DgdrRelease  = pProduct->GetDeflection(releaseIntervalIdx,pgsTypes::pftGirder,releasePoi,bat,rtCumulative,false);
      Float64 DgdrStorage  = pProduct->GetDeflection(storageIntervalIdx,pgsTypes::pftGirder,storagePoi,bat,rtCumulative,false);
      Float64 DgdrErected = pProduct->GetDeflection(erectionIntervalIdx,pgsTypes::pftGirder,erectedPoi,bat,rtCumulative,false);
      Float64 dgdrErected = pExtLoading->GetDeflection(erectionIntervalIdx, _T("Girder_Incremental"), erectedPoi, bat, rtCumulative, false);
      Float64 dxgdrErected = pExtLoading->GetXDeflection(erectionIntervalIdx, _T("Girder_Incremental"), erectedPoi, bat, rtCumulative);

      Float64 DXpsRelease = pProduct->GetXDeflection(releaseIntervalIdx, pgsTypes::pftPretension, releasePoi, bat, rtCumulative);
      Float64 DXgdrRelease = pProduct->GetXDeflection(releaseIntervalIdx, pgsTypes::pftGirder, releasePoi, bat, rtCumulative);

      Float64 DXpsStorage = pProduct->GetXDeflection(storageIntervalIdx, pgsTypes::pftPretension, storagePoi, bat, rtCumulative);
      Float64 DXgdrStorage = pProduct->GetXDeflection(storageIntervalIdx, pgsTypes::pftGirder, storagePoi, bat, rtCumulative);

      Float64 DXpsErected = pProduct->GetXDeflection(erectionIntervalIdx, pgsTypes::pftPretension, erectedPoi, bat, rtCumulative);
      Float64 DXgdrErected = pProduct->GetXDeflection(erectionIntervalIdx, pgsTypes::pftGirder, erectedPoi, bat, rtCumulative);

      Float64 Dtpsr(0.0), DXtpsr(0.0);
      if (bTempStrands)
      {
         pCamber->GetReleaseTempPrestressDeflection(erectedPoi, nullptr/*gdr config*/, &DXtpsr, &Dtpsr);
      }

      Float64 DprecamberRelease = pCamber->GetPrecamber(releasePoi, pgsTypes::pddRelease);
      Float64 DprecamberStorage = pCamber->GetPrecamber(storagePoi, pgsTypes::pddStorage);
      Float64 DprecamberErected = pCamber->GetPrecamber(erectedPoi, pgsTypes::pddErected);

      // NOTE: Get the creep deflection from the ICamber interface because it takes the construction rate 
      // into account. Getting creep deflection as a product load assumes the maximum construction rate
      Float64 Dcreep1a   = pCamber->GetCreepDeflection( storagePoi, ICamber::cpReleaseToDiaphragm, constructionRate, pgsTypes::pddStorage );
      Float64 DXcreep1a  = pCamber->GetXCreepDeflection(storagePoi, ICamber::cpReleaseToDiaphragm, constructionRate, pgsTypes::pddStorage);
      Float64 Dcreep1b   = pCamber->GetCreepDeflection( erectedPoi, ICamber::cpReleaseToDiaphragm, constructionRate, pgsTypes::pddErected );
      Float64 DXcreep1b = pCamber->GetXCreepDeflection( erectedPoi, ICamber::cpReleaseToDiaphragm, constructionRate, pgsTypes::pddErected);
      Float64 Ddiaphragm = pCamber->GetDiaphragmDeflection( erectedPoi );
      Float64 DshearKey = bShearKey ? pProduct->GetDeflection(castShearKeyIntervalIdx, pgsTypes::pftShearKey, erectedPoi, bat, rtCumulative, false) : 0.0;
      Float64 DlongJoint = bLongitudinalJoint ? pProduct->GetDeflection(castLongitudinalJointIntervalIdx, pgsTypes::pftLongitudinalJoint, erectedPoi, bat, rtCumulative, false) : 0.0;
      Float64 Dconstruction= bConstruction ? pProduct->GetDeflection(constructionLoadIntervalIdx,pgsTypes::pftConstruction,erectedPoi,bat, rtCumulative, false) : 0.0;
      Float64 Dcreep2    = pCamber->GetCreepDeflection( erectedPoi, ICamber::cpDiaphragmToDeck, constructionRate, pgsTypes::pddErected );
      Float64 Duser1     = pProduct->GetDeflection(noncompositeUserLoadIntervalIdx,pgsTypes::pftUserDC,erectedPoi,bat, rtCumulative, false) 
                         + pProduct->GetDeflection(noncompositeUserLoadIntervalIdx,pgsTypes::pftUserDW,erectedPoi,bat, rtCumulative, false);
      Float64 Duser2     = pProduct->GetDeflection(compositeUserLoadIntervalIdx,pgsTypes::pftUserDC,erectedPoi,bat, rtCumulative, false) 
                         + pProduct->GetDeflection(compositeUserLoadIntervalIdx,pgsTypes::pftUserDW,erectedPoi,bat, rtCumulative, false);
      Duser2 -= Duser1; // Duser2 is cumulative and it includes Duser1... remove Duser1
      Float64 Dbarrier   = pProduct->GetDeflection(railingSystemIntervalIdx,pgsTypes::pftTrafficBarrier,erectedPoi,bat, rtCumulative, false);
      Float64 Dsidewalk  = pProduct->GetDeflection(railingSystemIntervalIdx,pgsTypes::pftSidewalk,      erectedPoi,bat, rtCumulative, false);
      Float64 Doverlay   = (pBridge->HasOverlay() ? (pBridge->IsFutureOverlay() ? 0.0 : pProduct->GetDeflection(overlayIntervalIdx,pgsTypes::pftOverlay,erectedPoi,bat, rtCumulative, false)) : 0.0);

      Float64 Dslab(0), Dhaunch(0);
      if (deckType == pgsTypes::sdtNonstructuralOverlay)
      {
         Dslab = pProduct->GetDeflection(railingSystemIntervalIdx, pgsTypes::pftSlab, erectedPoi, bat, rtCumulative, false);
         Dhaunch = pProduct->GetDeflection(railingSystemIntervalIdx, pgsTypes::pftSlabPad, erectedPoi, bat, rtCumulative, false);
      }

      Float64 Dcreep3    = pCamber->GetCreepDeflection( erectedPoi, ICamber::cpDeckToFinal, constructionRate, pgsTypes::pddErected );

      Float64 Di = DpsRelease + DgdrRelease + DprecamberRelease; // initial camber immedately after release
      Float64 Des = DpsStorage + DgdrStorage + DprecamberStorage + Dcreep1a; // camber at end of storage

      Float64 DXi = DXpsRelease + DXgdrRelease;
      Float64 DXes = DXpsStorage + DXgdrStorage + DXcreep1a;

      // if we have a future overlay, the deflection due to the overlay in BridgeSite2 must be zero
      ATLASSERT( pBridge->IsFutureOverlay() ? IsZero(Doverlay) : true );

      // Table 1a
      col = 0;
      (*table1a)(row1a,col++) << location.SetValue( POI_RELEASED_SEGMENT, releasePoi );

      (*table1a)(row1a, col++) << deflection.SetValue(DXgdrRelease);
      (*table1a)(row1a, col++) << deflection.SetValue(DXpsRelease);
      (*table1a)(row1a, col++) << deflection.SetValue(DXi);

      (*table1a)(row1a,col++) << deflection.SetValue( DgdrRelease );
      (*table1a)(row1a,col++) << deflection.SetValue( DpsRelease );
      if (bHasPrecamber)
      {
         (*table1a)(row1a, col++) << deflection.SetValue(DprecamberRelease);
      }
      (*table1a)(row1a,col++) << deflection.SetValue( Di );

      row1a++;

      // Table 1b
      col = 0;
      (*table1b)(row1b,col++) << location.SetValue( POI_STORAGE_SEGMENT, storagePoi );

      (*table1b)(row1b, col++) << deflection.SetValue(DXgdrStorage);
      (*table1b)(row1b, col++) << deflection.SetValue(DXpsStorage);
      (*table1b)(row1b, col++) << deflection.SetValue(DXcreep1a);
      (*table1b)(row1b, col++) << deflection.SetValue(DXes);

      (*table1b)(row1b,col++) << deflection.SetValue( DgdrStorage );
      (*table1b)(row1b,col++) << deflection.SetValue( DpsStorage );
      if (bHasPrecamber)
      {
         (*table1b)(row1b, col++) << deflection.SetValue(DprecamberStorage);
      }
      (*table1b)(row1b,col++) << deflection.SetValue( Dcreep1a );
      (*table1b)(row1b,col++) << deflection.SetValue( Des );

      // make the data in the rows for the erected segment support locations bold
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

      (*table2)(row2, col++) << deflection.SetValue(DXgdrErected);
      (*table2)(row2, col++) << deflection.SetValue(DXpsErected);
      (*table2)(row2, col++) << deflection.SetValue(dxgdrErected);
      (*table2)(row2, col++) << deflection.SetValue(DXcreep1b);
      if (bTempStrands)
      {
         (*table2)(row2, col++) << deflection.SetValue(DXtpsr);
      }

      (*table2)(row2,col++) << deflection.SetValue( DgdrErected );
      (*table2)(row2,col++) << deflection.SetValue( DpsErected );
      (*table2)(row2,col++) << deflection.SetValue( dgdrErected );
      if (bHasPrecamber)
      {
         (*table2)(row2, col++) << deflection.SetValue(DprecamberErected);
      }
      (*table2)(row2,col++) << deflection.SetValue( Dcreep1b );

      if (bTempStrands)
      {
         (*table2)(row2,col++) << deflection.SetValue( Dtpsr );
      }

      (*table2)(row2,col++) << deflection.SetValue( Ddiaphragm );
      if (bShearKey)
      {
         (*table2)(row2, col++) << deflection.SetValue(DshearKey);
      }

      if (bLongitudinalJoint)
      {
         (*table2)(row2, col++) << deflection.SetValue(DlongJoint);
      }

      if ( bConstruction )
      {
         (*table2)(row2,col++) << deflection.SetValue( Dconstruction );
      }

      (*table2)(row2,col++) << deflection.SetValue( Dcreep2 );

      if (deckType == pgsTypes::sdtNonstructuralOverlay)
      {
         (*table2)(row2, col++) << deflection.SetValue(Dslab);
         (*table2)(row2, col++) << deflection.SetValue(Dhaunch);
      }

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

      Float64 DX1 = cm.ErectionFactor * (DXgdrErected + DXpsErected);
      Float64 DX2 = DX1 + cm.CreepFactor * DXcreep1b;

      Float64 D1 = cm.ErectionFactor * (DgdrErected + DpsErected) + DprecamberErected;
      Float64 D2 = D1 + cm.CreepFactor * Dcreep1b;
      Float64 D3 = D2 + cm.DiaphragmFactor * (Ddiaphragm + DshearKey + DlongJoint + Dconstruction) + cm.ErectionFactor * Dtpsr + cm.SlabUser1Factor * Duser1;
      Float64 D4 = D3 + cm.CreepFactor * Dcreep2;
      Float64 D5 = D4 + cm.BarrierSwOverlayUser2Factor * (Dbarrier + Duser2) + cm.SlabUser1Factor*Dslab + cm.SlabPadLoadFactor*Dhaunch;
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

      (*table3)(row3, col++) << deflection.SetValue(DX1);
      (*table3)(row3, col++) << deflection.SetValue(DX2);

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

      Float64 Sc = pCamber->GetScreedCamber(erectedPoi, constructionRate);
      ATLASSERT(IsEqual(Sc, D4 - D6));
#endif

      row3++;
   }

   *pTable1 = pLayoutTable;
   *pTable2 = table2;
   *pTable3 = table3;
}
