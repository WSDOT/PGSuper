///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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

#include <PgsExt\PointOfInterest.h>
#include <PgsExt\GirderData.h>

#include <IFace\Project.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\PrestressForce.h>
#include <IFace\Bridge.h>

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
rptRcTable* CPrestressLossTable::Build(IBroker* pBroker,SpanIndexType span,GirderIndexType gdr,
                                            IEAFDisplayUnits* pDisplayUnits) const
{
   GET_IFACE2(pBroker,IPrestressForce, pPrestressForce ); 
   GET_IFACE2(pBroker,IPointOfInterest,pIPOI);
   GET_IFACE2(pBroker,ILosses,pLosses);
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);

   std::vector<pgsPointOfInterest> vPoi = pIPOI->GetPointsOfInterest(span,gdr,pgsTypes::BridgeSite3,POI_MIDSPAN);
   pgsPointOfInterest poi = *vPoi.begin();

   GET_IFACE2(pBroker,IGirderData,pGirderData);
   CGirderData girderData = pGirderData->GetGirderData(span,gdr);

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

   bool bTempStrands = (0 < pStrandGeom->GetMaxStrands(span,gdr,pgsTypes::Temporary) ? true : false);

   int nCol = 5;
   if ( bTempStrands )
      nCol += 4;



   rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(nCol,_T("Strand Stress at Various Stages of Prestress Loss at Girder Mid-point"));
   p_table->SetNumberOfHeaderRows(2);

   p_table->SetColumnStyle(0, pgsReportStyleHolder::GetTableCellStyle( CB_NONE | CJ_LEFT) );
   p_table->SetStripeRowColumnStyle(0, pgsReportStyleHolder::GetTableStripeRowCellStyle( CB_NONE | CJ_LEFT) );

   //////////////////////////////////////////
   // Label columns
   //////////////////////////////////////////
   (*p_table)(0,0) << _T("Loss Stage");
   p_table->SetRowSpan(0,0,2);
   p_table->SetRowSpan(1,0,-1);

   p_table->SetColumnSpan(0,1,4);
   (*p_table)(0,1) << _T("Permanent Strand");
   (*p_table)(1,1) << COLHDR(_T("Force"),  rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit() );
   (*p_table)(1,2) << COLHDR(_T("Loss"),   rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table)(1,3) << COLHDR(_T("Stress"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table)(1,4) << _T("% Loss");

   if ( bTempStrands )
   {
      p_table->SetColumnSpan(0,2,4);
      p_table->SetColumnSpan(0,3,-1);
      p_table->SetColumnSpan(0,4,-1);
      p_table->SetColumnSpan(0,5,-1);
      p_table->SetColumnSpan(0,6,-1);
      p_table->SetColumnSpan(0,7,-1);
      p_table->SetColumnSpan(0,8,-1);

      (*p_table)(0,2) << _T("Temporary Strand");

      (*p_table)(1,5) << COLHDR(_T("Force"),  rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit() );
      (*p_table)(1,6) << COLHDR(_T("Loss"),   rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*p_table)(1,7) << COLHDR(_T("Stress"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*p_table)(1,8) << _T("% Loss");
   }
   else
   {
      p_table->SetColumnSpan(0,2,-1);
      p_table->SetColumnSpan(0,3,-1);
      p_table->SetColumnSpan(0,4,-1);
   }


   //////////////////////////////////////////
   // Label rows
   //////////////////////////////////////////
   int row = 2;
   (*p_table)(row++,0) << _T("At Jacking");
   (*p_table)(row++,0) << _T("Before Prestress Transfer");
   (*p_table)(row++,0) << _T("After Prestress Transfer");

   if ( bTempStrands && girderData.TempStrandUsage == pgsTypes::ttsPTBeforeLifting )
   {
      (*p_table)(row++,0) << _T("After Temporary Strand Installation");
   }

   (*p_table)(row++,0) << _T("At Lifting");

   if ( bTempStrands && (girderData.TempStrandUsage == pgsTypes::ttsPTAfterLifting || girderData.TempStrandUsage == pgsTypes::ttsPTBeforeShipping) )
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

   // Fill up the table with data.

   // this is the last place we will be using these unit value prototypes.
   // turn off the unit tag.
   force.ShowUnitTag(false);
   stress.ShowUnitTag(false);

   int dataStartRow = 2;

   ///////////////////////////////////
   // Permanent Strand Force Column
   row = dataStartRow;
   int col = 1;
   (*p_table)(row++,col) << force.SetValue( pPrestressForce->GetPrestressForce(poi,pgsTypes::Permanent,pgsTypes::Jacking) );
   (*p_table)(row++,col) << force.SetValue( pPrestressForce->GetPrestressForce(poi,pgsTypes::Permanent,pgsTypes::BeforeXfer) );
   (*p_table)(row++,col) << force.SetValue( pPrestressForce->GetPrestressForce(poi,pgsTypes::Permanent,pgsTypes::AfterXfer) );

   if ( bTempStrands && girderData.TempStrandUsage == pgsTypes::ttsPTBeforeLifting )
   {
      (*p_table)(row++,col) << force.SetValue( pPrestressForce->GetPrestressForce(poi,pgsTypes::Permanent,pgsTypes::AfterTemporaryStrandInstallation) );
   }
   
   (*p_table)(row++,col) << force.SetValue( pPrestressForce->GetPrestressForce(poi,pgsTypes::Permanent,pgsTypes::AtLifting) );

   if ( bTempStrands && (girderData.TempStrandUsage == pgsTypes::ttsPTAfterLifting || girderData.TempStrandUsage == pgsTypes::ttsPTBeforeShipping))
   {
      (*p_table)(row++,col) << force.SetValue( pPrestressForce->GetPrestressForce(poi,pgsTypes::Permanent,pgsTypes::AfterTemporaryStrandInstallation) );
   }
   
   (*p_table)(row++,col) << force.SetValue( pPrestressForce->GetPrestressForce(poi,pgsTypes::Permanent,pgsTypes::AtShipping) );
   
   if ( bTempStrands )
   {
      (*p_table)(row++,col) << force.SetValue( pPrestressForce->GetPrestressForce(poi,pgsTypes::Permanent,pgsTypes::BeforeTemporaryStrandRemoval) );
      (*p_table)(row++,col) << force.SetValue( pPrestressForce->GetPrestressForce(poi,pgsTypes::Permanent,pgsTypes::AfterTemporaryStrandRemoval) );
   }
   
   (*p_table)(row++,col) << force.SetValue( pPrestressForce->GetPrestressForce(poi,pgsTypes::Permanent,pgsTypes::AfterDeckPlacement) );
   (*p_table)(row++,col) << force.SetValue( pPrestressForce->GetPrestressForce(poi,pgsTypes::Permanent,pgsTypes::AfterSIDL) );
   (*p_table)(row++,col) << force.SetValue( pPrestressForce->GetPrestressForce(poi,pgsTypes::Permanent,pgsTypes::AfterLosses) );

   ///////////////////////////////////
   // Permanent Strand Loss Column
   row = dataStartRow;
   col++;
   (*p_table)(row++,col) << stress.SetValue( 0.0 );
   (*p_table)(row++,col) << stress.SetValue( pLosses->GetBeforeXferLosses(poi,pgsTypes::Permanent) );
   (*p_table)(row++,col) << stress.SetValue( pLosses->GetAfterXferLosses(poi,pgsTypes::Permanent) );
   if ( bTempStrands && girderData.TempStrandUsage == pgsTypes::ttsPTBeforeLifting )
   {
      (*p_table)(row++,col) << stress.SetValue(  pLosses->GetAfterTemporaryStrandInstallationLosses(poi,pgsTypes::Permanent) );
   }
   
   (*p_table)(row++,col) << stress.SetValue(  pLosses->GetLiftingLosses(poi,pgsTypes::Permanent) );

   if ( bTempStrands && (girderData.TempStrandUsage == pgsTypes::ttsPTAfterLifting || girderData.TempStrandUsage == pgsTypes::ttsPTBeforeShipping))
   {
      (*p_table)(row++,col) << stress.SetValue(  pLosses->GetAfterTemporaryStrandInstallationLosses(poi,pgsTypes::Permanent) );
   }
   
   (*p_table)(row++,col) << stress.SetValue( pLosses->GetShippingLosses(poi,pgsTypes::Permanent) );
   
   if ( bTempStrands )
   {
      (*p_table)(row++,col) << stress.SetValue(  pLosses->GetBeforeTemporaryStrandRemovalLosses(poi,pgsTypes::Permanent) );
      (*p_table)(row++,col) << stress.SetValue(  pLosses->GetAfterTemporaryStrandRemovalLosses(poi,pgsTypes::Permanent) );
   }
   
   (*p_table)(row++,col) << stress.SetValue( pLosses->GetDeckPlacementLosses(poi,pgsTypes::Permanent) );
   (*p_table)(row++,col) << stress.SetValue( pLosses->GetSIDLLosses(poi,pgsTypes::Permanent) );
   (*p_table)(row++,col) << stress.SetValue( pLosses->GetFinal(poi,pgsTypes::Permanent) );

   
   ///////////////////////////////////
   // Permanent Strand Stress Column
   row = dataStartRow;
   col++;
   (*p_table)(row++,col) << stress.SetValue( pPrestressForce->GetStrandStress(poi,pgsTypes::Permanent,pgsTypes::Jacking) );
   (*p_table)(row++,col) << stress.SetValue( pPrestressForce->GetStrandStress(poi,pgsTypes::Permanent,pgsTypes::BeforeXfer) );
   (*p_table)(row++,col) << stress.SetValue( pPrestressForce->GetStrandStress(poi,pgsTypes::Permanent,pgsTypes::AfterXfer) );

   if ( bTempStrands && girderData.TempStrandUsage == pgsTypes::ttsPTBeforeLifting )
   {
      (*p_table)(row++,col) << stress.SetValue( pPrestressForce->GetStrandStress(poi,pgsTypes::Permanent,pgsTypes::AfterTemporaryStrandInstallation) );
   }
   
   (*p_table)(row++,col) << stress.SetValue( pPrestressForce->GetStrandStress(poi,pgsTypes::Permanent,pgsTypes::AtLifting) );

   if ( bTempStrands && (girderData.TempStrandUsage == pgsTypes::ttsPTAfterLifting || girderData.TempStrandUsage == pgsTypes::ttsPTBeforeShipping))
   {
      (*p_table)(row++,col) << stress.SetValue( pPrestressForce->GetStrandStress(poi,pgsTypes::Permanent,pgsTypes::AfterTemporaryStrandInstallation) );
   }

   (*p_table)(row++,col) << stress.SetValue( pPrestressForce->GetStrandStress(poi,pgsTypes::Permanent,pgsTypes::AtShipping) );
   if ( bTempStrands )
   {
      (*p_table)(row++,col) << stress.SetValue( pPrestressForce->GetStrandStress(poi,pgsTypes::Permanent,pgsTypes::BeforeTemporaryStrandRemoval) );
      (*p_table)(row++,col) << stress.SetValue( pPrestressForce->GetStrandStress(poi,pgsTypes::Permanent,pgsTypes::AfterTemporaryStrandRemoval) );
   }
   (*p_table)(row++,col) << stress.SetValue( pPrestressForce->GetStrandStress(poi,pgsTypes::Permanent,pgsTypes::AfterDeckPlacement) );
   (*p_table)(row++,col) << stress.SetValue( pPrestressForce->GetStrandStress(poi,pgsTypes::Permanent,pgsTypes::AfterSIDL) );
   (*p_table)(row++,col) << stress.SetValue( pPrestressForce->GetStrandStress(poi,pgsTypes::Permanent,pgsTypes::AfterLosses) );
   
   ///////////////////////////////////
   // Permanent Strand % Loss Column
   row = dataStartRow;
   col++;

   // NOTE: multiplying PercentChange by -1 because prestress loss is a decrease. The this is represented by a negative percent change
   //       since we are reporting a % Loss we need a positive value
   //       don't use fabs because if we somehow get a gain we want to see that as a negative loss
   double Pjack = pPrestressForce->GetPrestressForce(poi,pgsTypes::Permanent,pgsTypes::Jacking);
   (*p_table)(row++,col) << scalar.SetValue( -1*PercentChange(Pjack,Pjack) );
   (*p_table)(row++,col) << scalar.SetValue( -1*PercentChange(Pjack,pPrestressForce->GetPrestressForce(poi,pgsTypes::Permanent,pgsTypes::BeforeXfer) ) );
   (*p_table)(row++,col) << scalar.SetValue( -1*PercentChange(Pjack,pPrestressForce->GetPrestressForce(poi,pgsTypes::Permanent,pgsTypes::AfterXfer) ) );

   if ( bTempStrands && girderData.TempStrandUsage == pgsTypes::ttsPTBeforeLifting )
   {
      (*p_table)(row++,col) << scalar.SetValue( -1*PercentChange(Pjack,pPrestressForce->GetPrestressForce(poi,pgsTypes::Permanent,pgsTypes::AfterTemporaryStrandInstallation) ) );
   }
   
   (*p_table)(row++,col) << scalar.SetValue( -1*PercentChange(Pjack,pPrestressForce->GetPrestressForce(poi,pgsTypes::Permanent,pgsTypes::AtLifting) ) );

   if ( bTempStrands && (girderData.TempStrandUsage == pgsTypes::ttsPTAfterLifting || girderData.TempStrandUsage == pgsTypes::ttsPTBeforeShipping))
   {
      (*p_table)(row++,col) << scalar.SetValue( -1*PercentChange(Pjack,pPrestressForce->GetPrestressForce(poi,pgsTypes::Permanent,pgsTypes::AfterTemporaryStrandInstallation) ) );
   }
   
   (*p_table)(row++,col) << scalar.SetValue( -1*PercentChange(Pjack,pPrestressForce->GetPrestressForce(poi,pgsTypes::Permanent,pgsTypes::AtShipping) ) );
   
   if ( bTempStrands )
   {
      (*p_table)(row++,col) << scalar.SetValue( -1*PercentChange(Pjack,pPrestressForce->GetPrestressForce(poi,pgsTypes::Permanent,pgsTypes::BeforeTemporaryStrandRemoval) ) );
      (*p_table)(row++,col) << scalar.SetValue( -1*PercentChange(Pjack,pPrestressForce->GetPrestressForce(poi,pgsTypes::Permanent,pgsTypes::AfterTemporaryStrandRemoval) ) );
   }
   
   (*p_table)(row++,col) << scalar.SetValue( -1*PercentChange(Pjack,pPrestressForce->GetPrestressForce(poi,pgsTypes::Permanent,pgsTypes::AfterDeckPlacement) ) );
   (*p_table)(row++,col) << scalar.SetValue( -1*PercentChange(Pjack,pPrestressForce->GetPrestressForce(poi,pgsTypes::Permanent,pgsTypes::AfterSIDL) ) );
   (*p_table)(row++,col) << scalar.SetValue( -1*PercentChange(Pjack,pPrestressForce->GetPrestressForce(poi,pgsTypes::Permanent,pgsTypes::AfterLosses) ) );
   

   if ( bTempStrands )
   {
      ///////////////////////////////////
      // Temporary Strand Force Column
      row = dataStartRow;
      col++;
      if ( girderData.TempStrandUsage == pgsTypes::ttsPretensioned )
      {
         (*p_table)(row++,col) << force.SetValue( pPrestressForce->GetPrestressForce(poi,pgsTypes::Temporary,pgsTypes::Jacking) );
         (*p_table)(row++,col) << force.SetValue( pPrestressForce->GetPrestressForce(poi,pgsTypes::Temporary,pgsTypes::BeforeXfer) );
         (*p_table)(row++,col) << force.SetValue( pPrestressForce->GetPrestressForce(poi,pgsTypes::Temporary,pgsTypes::AfterXfer) );
      }
      else
      {
         (*p_table)(row++,col) << _T(""); //force.SetValue( pPrestressForce->GetPrestressForce(poi,pgsTypes::Temporary,pgsTypes::Jacking) );
         (*p_table)(row++,col) << _T(""); //force.SetValue( pPrestressForce->GetPrestressForce(poi,pgsTypes::Temporary,pgsTypes::BeforeXfer) );
         (*p_table)(row++,col) << _T(""); //force.SetValue( pPrestressForce->GetPrestressForce(poi,pgsTypes::Temporary,pgsTypes::AfterXfer) );
      }
      
      if ( girderData.TempStrandUsage == pgsTypes::ttsPTBeforeLifting )
      {
         (*p_table)(row++,col) << force.SetValue( pPrestressForce->GetPrestressForce(poi,pgsTypes::Temporary,pgsTypes::AfterTemporaryStrandInstallation) );
      }

      if ( girderData.TempStrandUsage == pgsTypes::ttsPTAfterLifting || girderData.TempStrandUsage == pgsTypes::ttsPTBeforeShipping )
         (*p_table)(row++,col) << _T("");
      else
         (*p_table)(row++,col) << force.SetValue( pPrestressForce->GetPrestressForce(poi,pgsTypes::Temporary,pgsTypes::AtLifting) );

      if ( girderData.TempStrandUsage == pgsTypes::ttsPTAfterLifting || girderData.TempStrandUsage == pgsTypes::ttsPTBeforeShipping )
      {
         (*p_table)(row++,col) << force.SetValue( pPrestressForce->GetPrestressForce(poi,pgsTypes::Temporary,pgsTypes::AfterTemporaryStrandInstallation) );
      }
   
      (*p_table)(row++,col) << force.SetValue( pPrestressForce->GetPrestressForce(poi,pgsTypes::Temporary,pgsTypes::AtShipping) );
      (*p_table)(row++,col) << force.SetValue( pPrestressForce->GetPrestressForce(poi,pgsTypes::Temporary,pgsTypes::BeforeTemporaryStrandRemoval) );
      (*p_table)(row++,col) << _T("");//force.SetValue( pPrestressForce->GetPrestressForce(poi,pgsTypes::Temporary,pgsTypes::AfterTemporaryStrandRemoval) );
      (*p_table)(row++,col) << _T("");//force.SetValue( pPrestressForce->GetPrestressForce(poi,pgsTypes::Temporary,pgsTypes::AfterDeckPlacement) );
      (*p_table)(row++,col) << _T("");//force.SetValue( pPrestressForce->GetPrestressForce(poi,pgsTypes::Temporary,pgsTypes::AfterSIDL) );
      (*p_table)(row++,col) << _T("");//force.SetValue( pPrestressForce->GetPrestressForce(poi,pgsTypes::Temporary,pgsTypes::AfterLosses) );

      ///////////////////////////////////
      // Temporary Strand Loss Column
      row = dataStartRow;
      col++;

      if ( girderData.TempStrandUsage == pgsTypes::ttsPretensioned )
      {
         (*p_table)(row++,col) << stress.SetValue( 0.0 );
         (*p_table)(row++,col) << stress.SetValue( pLosses->GetBeforeXferLosses(poi,pgsTypes::Temporary) );
         (*p_table)(row++,col) << stress.SetValue( pLosses->GetAfterXferLosses(poi,pgsTypes::Temporary) );
      }
      else
      {
         (*p_table)(row++,col) << _T(""); //stress.SetValue( 0.0 );
         (*p_table)(row++,col) << _T(""); //stress.SetValue( pLosses->GetBeforeXferLosses(poi,pgsTypes::Temporary) );
         (*p_table)(row++,col) << _T(""); //stress.SetValue( pLosses->GetAfterXferLosses(poi,pgsTypes::Temporary) );
      }

      if ( girderData.TempStrandUsage == pgsTypes::ttsPTBeforeLifting )
      {
         (*p_table)(row++,col) << stress.SetValue(  pLosses->GetAfterTemporaryStrandInstallationLosses(poi,pgsTypes::Temporary) );
      }
   
      if ( girderData.TempStrandUsage == pgsTypes::ttsPTAfterLifting || girderData.TempStrandUsage == pgsTypes::ttsPTBeforeShipping )
         (*p_table)(row++,col) << _T("");
      else
         (*p_table)(row++,col) << stress.SetValue(  pLosses->GetLiftingLosses(poi,pgsTypes::Temporary) );

      if ( girderData.TempStrandUsage == pgsTypes::ttsPTAfterLifting || girderData.TempStrandUsage == pgsTypes::ttsPTBeforeShipping )
      {
         (*p_table)(row++,col) << stress.SetValue(  pLosses->GetAfterTemporaryStrandInstallationLosses(poi,pgsTypes::Temporary) );
      }
   
      (*p_table)(row++,col) << stress.SetValue( pLosses->GetShippingLosses(poi,pgsTypes::Temporary) );
      (*p_table)(row++,col) << stress.SetValue(  pLosses->GetBeforeTemporaryStrandRemovalLosses(poi,pgsTypes::Temporary) );
      (*p_table)(row++,col) << _T(""); //stress.SetValue(  pLosses->GetAfterTemporaryStrandRemovalLosses(poi,pgsTypes::Temporary) );
      (*p_table)(row++,col) << _T(""); //stress.SetValue( pLosses->GetDeckPlacementLosses(poi,pgsTypes::Temporary) );
      (*p_table)(row++,col) << _T(""); //stress.SetValue( pLosses->GetSIDLLosses(poi,pgsTypes::Temporary) );
      (*p_table)(row++,col) << _T(""); //stress.SetValue( pLosses->GetFinal(poi,pgsTypes::Temporary) );
   
      ///////////////////////////////////
      // Temporary Strand Stress Column
      row = dataStartRow;
      col++;
      if ( girderData.TempStrandUsage == pgsTypes::ttsPretensioned )
      {
         (*p_table)(row++,col) << stress.SetValue( pPrestressForce->GetStrandStress(poi,pgsTypes::Temporary,pgsTypes::Jacking) );
         (*p_table)(row++,col) << stress.SetValue( pPrestressForce->GetStrandStress(poi,pgsTypes::Temporary,pgsTypes::BeforeXfer) );
         (*p_table)(row++,col) << stress.SetValue( pPrestressForce->GetStrandStress(poi,pgsTypes::Temporary,pgsTypes::AfterXfer) );
      }
      else
      {
         (*p_table)(row++,col) << _T(""); //stress.SetValue( pPrestressForce->GetStrandStress(poi,pgsTypes::Temporary,pgsTypes::Jacking) );
         (*p_table)(row++,col) << _T(""); //stress.SetValue( pPrestressForce->GetStrandStress(poi,pgsTypes::Temporary,pgsTypes::BeforeXfer) );
         (*p_table)(row++,col) << _T(""); //stress.SetValue( pPrestressForce->GetStrandStress(poi,pgsTypes::Temporary,pgsTypes::AfterXfer) );
      }

      if ( girderData.TempStrandUsage == pgsTypes::ttsPTBeforeLifting )
      {
         (*p_table)(row++,col) << stress.SetValue( pPrestressForce->GetStrandStress(poi,pgsTypes::Temporary,pgsTypes::AfterTemporaryStrandInstallation) );
      }
   
      if ( girderData.TempStrandUsage == pgsTypes::ttsPTAfterLifting || girderData.TempStrandUsage == pgsTypes::ttsPTBeforeShipping )
         (*p_table)(row++,col) << _T("");
      else
         (*p_table)(row++,col) << stress.SetValue( pPrestressForce->GetStrandStress(poi,pgsTypes::Temporary,pgsTypes::AtLifting) );

      if ( girderData.TempStrandUsage == pgsTypes::ttsPTAfterLifting || girderData.TempStrandUsage == pgsTypes::ttsPTBeforeShipping )
      {
         (*p_table)(row++,col) << stress.SetValue( pPrestressForce->GetStrandStress(poi,pgsTypes::Temporary,pgsTypes::AfterTemporaryStrandInstallation) );
      }

      (*p_table)(row++,col) << stress.SetValue( pPrestressForce->GetStrandStress(poi,pgsTypes::Temporary,pgsTypes::AtShipping) );
      (*p_table)(row++,col) << stress.SetValue( pPrestressForce->GetStrandStress(poi,pgsTypes::Temporary,pgsTypes::BeforeTemporaryStrandRemoval) );
      (*p_table)(row++,col) << _T(""); //stress.SetValue( pPrestressForce->GetStrandStress(poi,pgsTypes::Temporary,pgsTypes::AfterTemporaryStrandRemoval) );
      (*p_table)(row++,col) << _T(""); //stress.SetValue( pPrestressForce->GetStrandStress(poi,pgsTypes::Temporary,pgsTypes::AfterDeckPlacement) );
      (*p_table)(row++,col) << _T(""); //stress.SetValue( pPrestressForce->GetStrandStress(poi,pgsTypes::Temporary,pgsTypes::AfterSIDL) );
      (*p_table)(row++,col) << _T(""); //stress.SetValue( pPrestressForce->GetStrandStress(poi,pgsTypes::Temporary,pgsTypes::AfterLosses) );
      
      ///////////////////////////////////
      // Temporary Strand % Loss Column
      row = dataStartRow;
      col++;
      double Pjack = 0;
      if ( girderData.TempStrandUsage == pgsTypes::ttsPretensioned )
      {
         Pjack = pPrestressForce->GetPrestressForce(poi,pgsTypes::Temporary,pgsTypes::Jacking);
         (*p_table)(row++,col) << scalar.SetValue( -1*PercentChange(Pjack,Pjack) );
         (*p_table)(row++,col) << scalar.SetValue( -1*PercentChange(Pjack,pPrestressForce->GetPrestressForce(poi,pgsTypes::Temporary,pgsTypes::BeforeXfer) ) );
         (*p_table)(row++,col) << scalar.SetValue( -1*PercentChange(Pjack,pPrestressForce->GetPrestressForce(poi,pgsTypes::Temporary,pgsTypes::AfterXfer) ) );
      }
      else
      {
         (*p_table)(row++,col) << _T("");
         (*p_table)(row++,col) << _T("");
         (*p_table)(row++,col) << _T("");
      }
      
      if ( girderData.TempStrandUsage == pgsTypes::ttsPTBeforeLifting )
      {
         (*p_table)(row++,col) << scalar.SetValue( -1*PercentChange(Pjack,pPrestressForce->GetPrestressForce(poi,pgsTypes::Temporary,pgsTypes::AfterTemporaryStrandInstallation) ) );
      }

      if ( girderData.TempStrandUsage == pgsTypes::ttsPTAfterLifting || girderData.TempStrandUsage == pgsTypes::ttsPTBeforeShipping )
         (*p_table)(row++,col) << _T("");
      else
         (*p_table)(row++,col) << scalar.SetValue( -1*PercentChange(Pjack,pPrestressForce->GetPrestressForce(poi,pgsTypes::Temporary,pgsTypes::AtLifting) ) );

      if ( girderData.TempStrandUsage == pgsTypes::ttsPTAfterLifting || girderData.TempStrandUsage == pgsTypes::ttsPTBeforeShipping )
      {
         (*p_table)(row++,col) << scalar.SetValue( -1*PercentChange(Pjack,pPrestressForce->GetPrestressForce(poi,pgsTypes::Temporary,pgsTypes::AfterTemporaryStrandInstallation) ) );
      }
   
      (*p_table)(row++,col) << scalar.SetValue( -1*PercentChange(Pjack,pPrestressForce->GetPrestressForce(poi,pgsTypes::Temporary,pgsTypes::AtShipping) ) );
      (*p_table)(row++,col) << scalar.SetValue( -1*PercentChange(Pjack,pPrestressForce->GetPrestressForce(poi,pgsTypes::Temporary,pgsTypes::BeforeTemporaryStrandRemoval) ) );
      (*p_table)(row++,col) << _T("");
      (*p_table)(row++,col) << _T("");
      (*p_table)(row++,col) << _T("");
      (*p_table)(row++,col) << _T("");
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

