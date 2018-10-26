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
#include <Reporting\PrestressLossTable.h>
#include <Reporting\ReportNotes.h>

#include <PgsExt\GirderPointOfInterest.h>
#include <PgsExt\StrandData.h>

#include <IFace\Project.h>
#include <IFace\PrestressForce.h>
#include <IFace\Bridge.h>
#include <IFace\Intervals.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CPrestressLossTable
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CPrestressLossTable::CPrestressLossTable()
{
}

CPrestressLossTable::CPrestressLossTable(const CPrestressLossTable& rOther)
{
   MakeCopy(rOther);
}

CPrestressLossTable::~CPrestressLossTable()
{
}

//======================== OPERATORS  =======================================
CPrestressLossTable& CPrestressLossTable::operator= (const CPrestressLossTable& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
rptRcTable* CPrestressLossTable::Build(IBroker* pBroker,const CSegmentKey& segmentKey,
                                            IEAFDisplayUnits* pDisplayUnits) const
{
   GET_IFACE2(pBroker,IPretensionForce, pPrestressForce ); 
   GET_IFACE2(pBroker,IPointOfInterest,pIPOI);
   GET_IFACE2(pBroker,ILosses,pLosses);
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);

   std::vector<pgsPointOfInterest> vPoi( pIPOI->GetPointsOfInterest(segmentKey,POI_ERECTED_SEGMENT | POI_MIDSPAN,POIFIND_AND) );
   pgsPointOfInterest poi( *vPoi.begin() );

   GET_IFACE2(pBroker,ISegmentData,pSegmentData);
   const CStrandData* pStrands = pSegmentData->GetStrandData(segmentKey);

   // Setup some unit-value prototypes
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(),       true );
   INIT_UV_PROTOTYPE( rptAreaUnitValue,   area,   pDisplayUnits->GetAreaUnit(),         true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, len,    pDisplayUnits->GetComponentDimUnit(), true );
   INIT_UV_PROTOTYPE( rptForceUnitValue,  force,  pDisplayUnits->GetGeneralForceUnit(), true );

   rptRcScalar scalar;
   scalar.SetFormat( sysNumericFormatTool::Fixed );
   scalar.SetWidth(6); // -99.99
   scalar.SetPrecision(2);
   scalar.SetTolerance(1.0e-6);

   bool bTempStrands = (0 < pStrandGeom->GetMaxStrands(segmentKey,pgsTypes::Temporary) ? true : false);

   ColumnIndexType nCol = 4;
   if ( bTempStrands )
      nCol += 3;


   rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(nCol,_T("Effective Prestress at Mid-Span"));
   p_table->SetNumberOfHeaderRows(2);

   p_table->SetColumnStyle(0, pgsReportStyleHolder::GetTableCellStyle( CB_NONE | CJ_LEFT) );
   p_table->SetStripeRowColumnStyle(0, pgsReportStyleHolder::GetTableStripeRowCellStyle( CB_NONE | CJ_LEFT) );

   //////////////////////////////////////////
   // Label columns
   //////////////////////////////////////////
   (*p_table)(0,0) << _T("Loss Stage");
   p_table->SetRowSpan(0,0,2);
   p_table->SetRowSpan(1,0,SKIP_CELL);

   p_table->SetColumnSpan(0,1,3/*4*/);
   (*p_table)(0,1) << _T("Permanent Strand");
   (*p_table)(1,1) << COLHDR(_T("Force"),  rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit() );
   (*p_table)(1,2) << COLHDR(_T("Loss"),   rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table)(1,3) << COLHDR(RPT_STRESS(_T("pe")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );

   if ( bTempStrands )
   {
      p_table->SetColumnSpan(0,2,3);
      p_table->SetColumnSpan(0,3,SKIP_CELL);
      p_table->SetColumnSpan(0,4,SKIP_CELL);
      p_table->SetColumnSpan(0,5,SKIP_CELL);
      p_table->SetColumnSpan(0,6,SKIP_CELL);

      (*p_table)(0,2) << _T("Temporary Strand");

      (*p_table)(1,4) << COLHDR(_T("Force"),  rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit() );
      (*p_table)(1,5) << COLHDR(_T("Loss"),   rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*p_table)(1,6) << COLHDR(RPT_STRESS(_T("pe")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   }
   else
   {
      p_table->SetColumnSpan(0,2,SKIP_CELL);
      p_table->SetColumnSpan(0,3,SKIP_CELL);
   }


   //////////////////////////////////////////
   // Label rows
   //////////////////////////////////////////
   RowIndexType row = 2;
   (*p_table)(row++,0) << _T("At Jacking");
   (*p_table)(row++,0) << _T("Before Prestress Transfer");
   (*p_table)(row++,0) << _T("After Prestress Transfer");

   if ( bTempStrands && pStrands->TempStrandUsage == pgsTypes::ttsPTBeforeLifting )
   {
      (*p_table)(row++,0) << _T("After Temporary Strand Installation");
   }

   (*p_table)(row++,0) << _T("At Lifting");

   if ( bTempStrands && (pStrands->TempStrandUsage == pgsTypes::ttsPTAfterLifting || pStrands->TempStrandUsage == pgsTypes::ttsPTBeforeShipping) )
   {
      (*p_table)(row++,0) << _T("After Temporary Strand Installation");
   }

   (*p_table)(row++,0) << _T("At Shipping");
   if ( bTempStrands )
   {
      (*p_table)(row++,0) << _T("Before Temporary Strands Removal");
      (*p_table)(row++,0) << _T("After Temporary Strands Removal");
   }
   (*p_table)(row++,0) << _T("After Deck Placement");
   (*p_table)(row++,0) << _T("After Superimposed Dead Loads");
   (*p_table)(row++,0) << _T("Final");
   (*p_table)(row++,0) << _T("Final with Live Load");

   // Fill up the table with data.

   // this is the last place we will be using these unit value prototypes.
   // turn off the unit tag.
   force.ShowUnitTag(false);
   stress.ShowUnitTag(false);

   RowIndexType dataStartRow = 2;

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType stressStrandsIntervalIdx = pIntervals->GetStressStrandInterval(segmentKey);
   IntervalIndexType releaseIntervalIdx       = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType liftSegmentIntervalIdx   = pIntervals->GetLiftSegmentInterval(segmentKey);
   IntervalIndexType haulSegmentIntervalIdx   = pIntervals->GetHaulSegmentInterval(segmentKey);
   IntervalIndexType erectSegmentIntervalIdx  = pIntervals->GetErectSegmentInterval(segmentKey);
   IntervalIndexType tsInstallIntervalIdx     = pIntervals->GetTemporaryStrandInstallationInterval(segmentKey);
   IntervalIndexType tsRemovalIntervalIdx     = pIntervals->GetTemporaryStrandRemovalInterval(segmentKey);
   IntervalIndexType castDeckIntervalIdx      = pIntervals->GetCastDeckInterval(segmentKey);
   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval(segmentKey);
   IntervalIndexType liveLoadIntervalIdx      = pIntervals->GetLiveLoadInterval(segmentKey);

   ///////////////////////////////////
   // Permanent Strand Force Column
   row = dataStartRow;
   ColumnIndexType col = 1;
   (*p_table)(row++,col) << force.SetValue( pPrestressForce->GetPrestressForce(poi,pgsTypes::Permanent,stressStrandsIntervalIdx,pgsTypes::Start/*pgsTypes::Jacking*/) );
   (*p_table)(row++,col) << force.SetValue( pPrestressForce->GetPrestressForce(poi,pgsTypes::Permanent,stressStrandsIntervalIdx,pgsTypes::End/*pgsTypes::BeforeXfer*/) );
   (*p_table)(row++,col) << force.SetValue( pPrestressForce->GetPrestressForce(poi,pgsTypes::Permanent,releaseIntervalIdx,pgsTypes::End/*pgsTypes::AfterXfer*/) );

   if ( bTempStrands && pStrands->TempStrandUsage == pgsTypes::ttsPTBeforeLifting )
   {
      (*p_table)(row++,col) << force.SetValue( pPrestressForce->GetPrestressForce(poi,pgsTypes::Permanent,tsInstallIntervalIdx,pgsTypes::End/*pgsTypes::AfterTemporaryStrandInstallation*/) );
   }
   
   (*p_table)(row++,col) << force.SetValue( pPrestressForce->GetPrestressForce(poi,pgsTypes::Permanent,liftSegmentIntervalIdx,pgsTypes::End/*pgsTypes::AtLifting*/) );

   if ( bTempStrands && (pStrands->TempStrandUsage == pgsTypes::ttsPTAfterLifting || pStrands->TempStrandUsage == pgsTypes::ttsPTBeforeShipping))
   {
      (*p_table)(row++,col) << force.SetValue( pPrestressForce->GetPrestressForce(poi,pgsTypes::Permanent,tsInstallIntervalIdx,pgsTypes::End/*pgsTypes::AfterTemporaryStrandInstallation*/) );
   }
   
   (*p_table)(row++,col) << force.SetValue( pPrestressForce->GetPrestressForce(poi,pgsTypes::Permanent,haulSegmentIntervalIdx,pgsTypes::End/*pgsTypes::AtShipping*/) );
   
   if ( bTempStrands )
   {
      (*p_table)(row++,col) << force.SetValue( pPrestressForce->GetPrestressForce(poi,pgsTypes::Permanent,erectSegmentIntervalIdx,pgsTypes::End/*pgsTypes::BeforeTemporaryStrandRemoval*/) );
      (*p_table)(row++,col) << force.SetValue( pPrestressForce->GetPrestressForce(poi,pgsTypes::Permanent,tsRemovalIntervalIdx,pgsTypes::End/*pgsTypes::AfterTemporaryStrandRemoval*/) );
   }
   
   (*p_table)(row++,col) << force.SetValue( pPrestressForce->GetPrestressForce(poi,pgsTypes::Permanent,castDeckIntervalIdx,pgsTypes::End/*pgsTypes::AfterDeckPlacement*/) );
   (*p_table)(row++,col) << force.SetValue( pPrestressForce->GetPrestressForce(poi,pgsTypes::Permanent,compositeDeckIntervalIdx,pgsTypes::End/*pgsTypes::AfterSIDL*/) );
   (*p_table)(row++,col) << force.SetValue( pPrestressForce->GetPrestressForce(poi,pgsTypes::Permanent,liveLoadIntervalIdx,pgsTypes::End/*pgsTypes::AfterLosses*/) );
   (*p_table)(row++,col) << force.SetValue( pPrestressForce->GetPrestressForceWithLiveLoad(poi,pgsTypes::Permanent/*pgsTypes::AfterLossesWithLiveLoad*/) );

   ///////////////////////////////////
   // Permanent Strand Loss Column
   row = dataStartRow;
   col++;
   (*p_table)(row++,col) << stress.SetValue( pLosses->GetPrestressLoss(poi,pgsTypes::Permanent,stressStrandsIntervalIdx,pgsTypes::Start)); // at jacking
   (*p_table)(row++,col) << stress.SetValue( pLosses->GetPrestressLoss(poi,pgsTypes::Permanent,stressStrandsIntervalIdx,pgsTypes::End)/*pLosses->GetBeforeXferLosses(poi,pgsTypes::Permanent)*/ );
   (*p_table)(row++,col) << stress.SetValue( pLosses->GetPrestressLoss(poi,pgsTypes::Permanent,releaseIntervalIdx,pgsTypes::End)/*pLosses->GetAfterXferLosses(poi,pgsTypes::Permanent)*/ );
   if ( bTempStrands && pStrands->TempStrandUsage == pgsTypes::ttsPTBeforeLifting )
   {
      (*p_table)(row++,col) << stress.SetValue( pLosses->GetPrestressLoss(poi,pgsTypes::Permanent,tsInstallIntervalIdx,pgsTypes::End)/*pLosses->GetAfterTemporaryStrandInstallationLosses(poi,pgsTypes::Permanent)*/ );
   }
   
   (*p_table)(row++,col) << stress.SetValue(  pLosses->GetPrestressLoss(poi,pgsTypes::Permanent,liftSegmentIntervalIdx,pgsTypes::End)/*pLosses->GetLiftingLosses(poi,pgsTypes::Permanent)*/ );

   if ( bTempStrands && (pStrands->TempStrandUsage == pgsTypes::ttsPTAfterLifting || pStrands->TempStrandUsage == pgsTypes::ttsPTBeforeShipping))
   {
      (*p_table)(row++,col) << stress.SetValue(  pLosses->GetPrestressLoss(poi,pgsTypes::Permanent,tsInstallIntervalIdx,pgsTypes::End)/*pLosses->GetAfterTemporaryStrandInstallationLosses(poi,pgsTypes::Permanent)*/ );
   }
   
   (*p_table)(row++,col) << stress.SetValue( pLosses->GetPrestressLoss(poi,pgsTypes::Permanent,haulSegmentIntervalIdx,pgsTypes::End)/*pLosses->GetShippingLosses(poi,pgsTypes::Permanent)*/ );
   
   if ( bTempStrands )
   {
      (*p_table)(row++,col) << stress.SetValue(  pLosses->GetPrestressLoss(poi,pgsTypes::Permanent,erectSegmentIntervalIdx,pgsTypes::End)/*pLosses->GetBeforeTemporaryStrandRemovalLosses(poi,pgsTypes::Permanent)*/ );
      (*p_table)(row++,col) << stress.SetValue(  pLosses->GetPrestressLoss(poi,pgsTypes::Permanent,tsRemovalIntervalIdx,pgsTypes::End)/*pLosses->GetAfterTemporaryStrandRemovalLosses(poi,pgsTypes::Permanent)*/ );
   }
   
   (*p_table)(row++,col) << stress.SetValue( pLosses->GetPrestressLoss(poi,pgsTypes::Permanent,castDeckIntervalIdx,pgsTypes::End)/*pLosses->GetDeckPlacementLosses(poi,pgsTypes::Permanent)*/ );
   (*p_table)(row++,col) << stress.SetValue( pLosses->GetPrestressLoss(poi,pgsTypes::Permanent,compositeDeckIntervalIdx,pgsTypes::End)/*pLosses->GetSIDLLosses(poi,pgsTypes::Permanent)*/ );
   (*p_table)(row++,col) << stress.SetValue( pLosses->GetPrestressLoss(poi,pgsTypes::Permanent,liveLoadIntervalIdx,pgsTypes::End)/*pLosses->GetFinal(poi,pgsTypes::Permanent)*/ );
   (*p_table)(row++,col) << stress.SetValue( pLosses->GetPrestressLossWithLiveLoad(poi,pgsTypes::Permanent)/*pLosses->GetFinalWithLiveLoad(poi,pgsTypes::Permanent)*/ );
   
   ///////////////////////////////////
   // Permanent Strand Stress Column
   row = dataStartRow;
   col++;
   (*p_table)(row++,col) << stress.SetValue( pPrestressForce->GetEffectivePrestress(poi,pgsTypes::Permanent,stressStrandsIntervalIdx,pgsTypes::Start) );
   (*p_table)(row++,col) << stress.SetValue( pPrestressForce->GetEffectivePrestress(poi,pgsTypes::Permanent,stressStrandsIntervalIdx,pgsTypes::End) );
   (*p_table)(row++,col) << stress.SetValue( pPrestressForce->GetEffectivePrestress(poi,pgsTypes::Permanent,releaseIntervalIdx,pgsTypes::End) );

   if ( bTempStrands && pStrands->TempStrandUsage == pgsTypes::ttsPTBeforeLifting )
   {
      (*p_table)(row++,col) << stress.SetValue( pPrestressForce->GetEffectivePrestress(poi,pgsTypes::Permanent,tsInstallIntervalIdx,pgsTypes::End) );
   }
   
   (*p_table)(row++,col) << stress.SetValue( pPrestressForce->GetEffectivePrestress(poi,pgsTypes::Permanent,liftSegmentIntervalIdx,pgsTypes::End) );

   if ( bTempStrands && (pStrands->TempStrandUsage == pgsTypes::ttsPTAfterLifting || pStrands->TempStrandUsage == pgsTypes::ttsPTBeforeShipping))
   {
      (*p_table)(row++,col) << stress.SetValue( pPrestressForce->GetEffectivePrestress(poi,pgsTypes::Permanent,tsInstallIntervalIdx,pgsTypes::End) );
   }

   (*p_table)(row++,col) << stress.SetValue( pPrestressForce->GetEffectivePrestress(poi,pgsTypes::Permanent,haulSegmentIntervalIdx,pgsTypes::End) );
   if ( bTempStrands )
   {
      (*p_table)(row++,col) << stress.SetValue( pPrestressForce->GetEffectivePrestress(poi,pgsTypes::Permanent,erectSegmentIntervalIdx,pgsTypes::End/*pgsTypes::BeforeTemporaryStrandRemoval*/) );
      (*p_table)(row++,col) << stress.SetValue( pPrestressForce->GetEffectivePrestress(poi,pgsTypes::Permanent,tsRemovalIntervalIdx,pgsTypes::End/*pgsTypes::AfterTemporaryStrandRemoval*/) );
   }
   (*p_table)(row++,col) << stress.SetValue( pPrestressForce->GetEffectivePrestress(poi,pgsTypes::Permanent,castDeckIntervalIdx,pgsTypes::End) );
   (*p_table)(row++,col) << stress.SetValue( pPrestressForce->GetEffectivePrestress(poi,pgsTypes::Permanent,compositeDeckIntervalIdx,pgsTypes::End) );
   (*p_table)(row++,col) << stress.SetValue( pPrestressForce->GetEffectivePrestress(poi,pgsTypes::Permanent,liveLoadIntervalIdx,pgsTypes::End/*pgsTypes::AfterLosses*/) );
   (*p_table)(row++,col) << stress.SetValue( pPrestressForce->GetEffectivePrestressWithLiveLoad(poi,pgsTypes::Permanent/*pgsTypes::AfterLossesWithLiveLoad*/) );

   if ( bTempStrands )
   {
      ///////////////////////////////////
      // Temporary Strand Force Column
      row = dataStartRow;
      col++;
      if ( pStrands->TempStrandUsage == pgsTypes::ttsPretensioned )
      {
         (*p_table)(row++,col) << force.SetValue( pPrestressForce->GetPrestressForce(poi,pgsTypes::Temporary,stressStrandsIntervalIdx,pgsTypes::Start/*pgsTypes::Jacking*/) );
         (*p_table)(row++,col) << force.SetValue( pPrestressForce->GetPrestressForce(poi,pgsTypes::Temporary,stressStrandsIntervalIdx,pgsTypes::End/*pgsTypes::BeforeXfer*/) );
         (*p_table)(row++,col) << force.SetValue( pPrestressForce->GetPrestressForce(poi,pgsTypes::Temporary,releaseIntervalIdx,pgsTypes::End/*pgsTypes::AfterXfer*/) );
      }
      else
      {
         (*p_table)(row++,col) << _T(""); //force.SetValue( pPrestressForce->GetPrestressForce(poi,pgsTypes::Temporary,pgsTypes::Jacking) );
         (*p_table)(row++,col) << _T(""); //force.SetValue( pPrestressForce->GetPrestressForce(poi,pgsTypes::Temporary,pgsTypes::BeforeXfer) );
         (*p_table)(row++,col) << _T(""); //force.SetValue( pPrestressForce->GetPrestressForce(poi,pgsTypes::Temporary,pgsTypes::AfterXfer) );
      }
      
      if ( pStrands->TempStrandUsage == pgsTypes::ttsPTBeforeLifting )
      {
         (*p_table)(row++,col) << force.SetValue( pPrestressForce->GetPrestressForce(poi,pgsTypes::Temporary,tsInstallIntervalIdx,pgsTypes::End/*pgsTypes::AfterTemporaryStrandInstallation*/) );
      }

      if ( pStrands->TempStrandUsage == pgsTypes::ttsPTAfterLifting || pStrands->TempStrandUsage == pgsTypes::ttsPTBeforeShipping )
         (*p_table)(row++,col) << _T("");
      else
         (*p_table)(row++,col) << force.SetValue( pPrestressForce->GetPrestressForce(poi,pgsTypes::Temporary,liftSegmentIntervalIdx,pgsTypes::End/*pgsTypes::AtLifting*/) );

      if ( pStrands->TempStrandUsage == pgsTypes::ttsPTAfterLifting || pStrands->TempStrandUsage == pgsTypes::ttsPTBeforeShipping )
      {
         (*p_table)(row++,col) << force.SetValue( pPrestressForce->GetPrestressForce(poi,pgsTypes::Temporary,tsInstallIntervalIdx,pgsTypes::End/*pgsTypes::AfterTemporaryStrandInstallation*/) );
      }
   
      (*p_table)(row++,col) << force.SetValue( pPrestressForce->GetPrestressForce(poi,pgsTypes::Temporary,haulSegmentIntervalIdx,pgsTypes::End/*pgsTypes::AtShipping*/) );
      (*p_table)(row++,col) << force.SetValue( pPrestressForce->GetPrestressForce(poi,pgsTypes::Temporary,erectSegmentIntervalIdx,pgsTypes::End/*pgsTypes::BeforeTemporaryStrandRemoval*/) );
      (*p_table)(row++,col) << _T("");//force.SetValue( pPrestressForce->GetPrestressForce(poi,pgsTypes::Temporary,pgsTypes::AfterTemporaryStrandRemoval) );
      (*p_table)(row++,col) << _T("");//force.SetValue( pPrestressForce->GetPrestressForce(poi,pgsTypes::Temporary,pgsTypes::AfterDeckPlacement) );
      (*p_table)(row++,col) << _T("");//force.SetValue( pPrestressForce->GetPrestressForce(poi,pgsTypes::Temporary,pgsTypes::AfterSIDL) );
      (*p_table)(row++,col) << _T("");//force.SetValue( pPrestressForce->GetPrestressForce(poi,pgsTypes::Temporary,pgsTypes::AfterLosses) );
      (*p_table)(row++,col) << _T("");//force.SetValue( pPrestressForce->GetPrestressForce(poi,pgsTypes::Temporary,pgsTypes::AfterLossesWithLiveLoad) );

      ///////////////////////////////////
      // Temporary Strand Loss Column
      row = dataStartRow;
      col++;

      if ( pStrands->TempStrandUsage == pgsTypes::ttsPretensioned )
      {
         (*p_table)(row++,col) << stress.SetValue( pLosses->GetPrestressLoss(poi,pgsTypes::Temporary,stressStrandsIntervalIdx,pgsTypes::Start) );
         (*p_table)(row++,col) << stress.SetValue( pLosses->GetPrestressLoss(poi,pgsTypes::Temporary,stressStrandsIntervalIdx,pgsTypes::End)/*pLosses->GetBeforeXferLosses(poi,pgsTypes::Temporary)*/ );
         (*p_table)(row++,col) << stress.SetValue( pLosses->GetPrestressLoss(poi,pgsTypes::Temporary,releaseIntervalIdx,pgsTypes::End)/*pLosses->GetAfterXferLosses(poi,pgsTypes::Temporary)*/ );
      }
      else
      {
         (*p_table)(row++,col) << _T(""); //stress.SetValue( 0.0 );
         (*p_table)(row++,col) << _T(""); //stress.SetValue( pLosses->GetBeforeXferLosses(poi,pgsTypes::Temporary) );
         (*p_table)(row++,col) << _T(""); //stress.SetValue( pLosses->GetAfterXferLosses(poi,pgsTypes::Temporary) );
      }

      if ( pStrands->TempStrandUsage == pgsTypes::ttsPTBeforeLifting )
      {
         (*p_table)(row++,col) << stress.SetValue(  pLosses->GetPrestressLoss(poi,pgsTypes::Temporary,tsInstallIntervalIdx,pgsTypes::End)/*pLosses->GetAfterTemporaryStrandInstallationLosses(poi,pgsTypes::Temporary)*/ );
      }
   
      if ( pStrands->TempStrandUsage == pgsTypes::ttsPTAfterLifting || pStrands->TempStrandUsage == pgsTypes::ttsPTBeforeShipping )
         (*p_table)(row++,col) << _T("");
      else
         (*p_table)(row++,col) << stress.SetValue(  pLosses->GetPrestressLoss(poi,pgsTypes::Temporary,liftSegmentIntervalIdx,pgsTypes::End)/*pLosses->GetLiftingLosses(poi,pgsTypes::Temporary)*/ );

      if ( pStrands->TempStrandUsage == pgsTypes::ttsPTAfterLifting || pStrands->TempStrandUsage == pgsTypes::ttsPTBeforeShipping )
      {
         (*p_table)(row++,col) << stress.SetValue(  pLosses->GetPrestressLoss(poi,pgsTypes::Temporary,tsInstallIntervalIdx,pgsTypes::End)/*pLosses->GetAfterTemporaryStrandInstallationLosses(poi,pgsTypes::Temporary)*/ );
      }
   
      (*p_table)(row++,col) << stress.SetValue( pLosses->GetPrestressLoss(poi,pgsTypes::Temporary,haulSegmentIntervalIdx,pgsTypes::End)/*pLosses->GetShippingLosses(poi,pgsTypes::Temporary)*/ );
      (*p_table)(row++,col) << stress.SetValue( pLosses->GetPrestressLoss(poi,pgsTypes::Temporary,erectSegmentIntervalIdx,pgsTypes::End)/*pLosses->GetBeforeTemporaryStrandRemovalLosses(poi,pgsTypes::Temporary)*/ );
      (*p_table)(row++,col) << _T(""); //stress.SetValue(  pLosses->GetAfterTemporaryStrandRemovalLosses(poi,pgsTypes::Temporary) );
      (*p_table)(row++,col) << _T(""); //stress.SetValue( pLosses->GetDeckPlacementLosses(poi,pgsTypes::Temporary) );
      (*p_table)(row++,col) << _T(""); //stress.SetValue( pLosses->GetSIDLLosses(poi,pgsTypes::Temporary) );
      (*p_table)(row++,col) << _T(""); //stress.SetValue( pLosses->GetFinal(poi,pgsTypes::Temporary) );
      (*p_table)(row++,col) << _T(""); //stress.SetValue( pLosses->GetFinalWithLiveLoad(poi,pgsTypes::Temporary) );
   
      ///////////////////////////////////
      // Temporary Strand Stress Column
      row = dataStartRow;
      col++;
      if ( pStrands->TempStrandUsage == pgsTypes::ttsPretensioned )
      {
         (*p_table)(row++,col) << stress.SetValue( pPrestressForce->GetEffectivePrestress(poi,pgsTypes::Temporary,stressStrandsIntervalIdx,pgsTypes::Start/*pgsTypes::Jacking*/) );
         (*p_table)(row++,col) << stress.SetValue( pPrestressForce->GetEffectivePrestress(poi,pgsTypes::Temporary,stressStrandsIntervalIdx,pgsTypes::End/*pgsTypes::BeforeXfer*/) );
         (*p_table)(row++,col) << stress.SetValue( pPrestressForce->GetEffectivePrestress(poi,pgsTypes::Temporary,releaseIntervalIdx,pgsTypes::End/*pgsTypes::AfterXfer*/) );
      }
      else
      {
         (*p_table)(row++,col) << _T(""); //stress.SetValue( pPrestressForce->GetStrandStress(poi,pgsTypes::Temporary,pgsTypes::Jacking) );
         (*p_table)(row++,col) << _T(""); //stress.SetValue( pPrestressForce->GetStrandStress(poi,pgsTypes::Temporary,pgsTypes::BeforeXfer) );
         (*p_table)(row++,col) << _T(""); //stress.SetValue( pPrestressForce->GetStrandStress(poi,pgsTypes::Temporary,pgsTypes::AfterXfer) );
      }

      if ( pStrands->TempStrandUsage == pgsTypes::ttsPTBeforeLifting )
      {
         (*p_table)(row++,col) << stress.SetValue( pPrestressForce->GetEffectivePrestress(poi,pgsTypes::Temporary,tsInstallIntervalIdx,pgsTypes::End) );
      }
   
      if ( pStrands->TempStrandUsage == pgsTypes::ttsPTAfterLifting || pStrands->TempStrandUsage == pgsTypes::ttsPTBeforeShipping )
         (*p_table)(row++,col) << _T("");
      else
         (*p_table)(row++,col) << stress.SetValue( pPrestressForce->GetEffectivePrestress(poi,pgsTypes::Temporary,liftSegmentIntervalIdx,pgsTypes::End) );

      if ( pStrands->TempStrandUsage == pgsTypes::ttsPTAfterLifting || pStrands->TempStrandUsage == pgsTypes::ttsPTBeforeShipping )
      {
         (*p_table)(row++,col) << stress.SetValue( pPrestressForce->GetEffectivePrestress(poi,pgsTypes::Temporary,tsInstallIntervalIdx,pgsTypes::End) );
      }

      (*p_table)(row++,col) << stress.SetValue( pPrestressForce->GetEffectivePrestress(poi,pgsTypes::Temporary,haulSegmentIntervalIdx,pgsTypes::End) );
      (*p_table)(row++,col) << stress.SetValue( pPrestressForce->GetEffectivePrestress(poi,pgsTypes::Temporary,erectSegmentIntervalIdx,pgsTypes::End/*pgsTypes::BeforeTemporaryStrandRemoval*/) );
      (*p_table)(row++,col) << _T(""); //stress.SetValue( pPrestressForce->GetStrandStress(poi,pgsTypes::Temporary,pgsTypes::AfterTemporaryStrandRemoval) );
      (*p_table)(row++,col) << _T(""); //stress.SetValue( pPrestressForce->GetStrandStress(poi,pgsTypes::Temporary,pgsTypes::AfterDeckPlacement) );
      (*p_table)(row++,col) << _T(""); //stress.SetValue( pPrestressForce->GetStrandStress(poi,pgsTypes::Temporary,pgsTypes::AfterSIDL) );
      (*p_table)(row++,col) << _T(""); //stress.SetValue( pPrestressForce->GetStrandStress(poi,pgsTypes::Temporary,pgsTypes::AfterLosses) );
      (*p_table)(row++,col) << _T(""); //stress.SetValue( pPrestressForce->GetStrandStress(poi,pgsTypes::Temporary,pgsTypes::AfterLossesWithLiveLoad) );
   }

   return p_table;
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void CPrestressLossTable::MakeCopy(const CPrestressLossTable& rOther)
{
   // Add copy code here...
}

void CPrestressLossTable::MakeAssignment(const CPrestressLossTable& rOther)
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

