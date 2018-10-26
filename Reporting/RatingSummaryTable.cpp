///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2011  Washington State Department of Transportation
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
#include <Reporting\RatingSummaryTable.h>

#include <PgsExt\RatingArtifact.h>
#include <PgsExt\CapacityToDemand.h>

#include <EAF\EAFDisplayUnits.h>
#include <IFace\Bridge.h>
#include <IFace\Artifact.h>
#include <IFace\AnalysisResults.h>
#include <IFace\RatingSpecification.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CRatingSummaryTable::CRatingSummaryTable()
{
}

CRatingSummaryTable::~CRatingSummaryTable()
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
rptRcTable* CRatingSummaryTable::BuildByLimitState(IBroker* pBroker,GirderIndexType gdrLineIdx,CRatingSummaryTable::RatingTableType ratingTableType) const
{
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker,IRatingSpecification,pRatingSpec);
   GET_IFACE2(pBroker,IBridge,pBridge);

   bool bNegMoments = pBridge->ProcessNegativeMoments(ALL_SPANS);

   rptCapacityToDemand rating_factor;

   INIT_UV_PROTOTYPE( rptForceUnitValue, force, pDisplayUnits->GetGeneralForceUnit(), false );
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), true );

   location.IncludeSpanAndGirder(true);

   rptRcScalar scalar;
   scalar.SetFormat( sysNumericFormatTool::Fixed );
   scalar.SetWidth(6);
   scalar.SetPrecision(3);
   scalar.SetTolerance(1.0e-6);


   std::_tstring strRatingType;
   std::_tstring strRoutine, strSpecial;
   pgsTypes::LoadRatingType routine_rating_type, special_rating_type;
   switch( ratingTableType )
   {
   case Design:
      strRoutine = _T("Inventory");
      strSpecial = _T("Operating");
      routine_rating_type = pgsTypes::lrDesign_Inventory;
      special_rating_type = pgsTypes::lrDesign_Operating;
      break;
   case Legal:
      strRoutine = _T("Routine Commercial Traffic");
      strSpecial = _T("Specialized Hauling Vehicles");
      routine_rating_type = pgsTypes::lrLegal_Routine;
      special_rating_type = pgsTypes::lrLegal_Special;
      break;
   case Permit:
      strRoutine = _T("Routine Permit");
      strSpecial = _T("Special Permit");
      routine_rating_type = pgsTypes::lrPermit_Routine;
      special_rating_type = pgsTypes::lrPermit_Special;
      break;
   }

   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(8,_T(""));
   table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
   table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   table->SetColumnStyle(1,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
   table->SetStripeRowColumnStyle(1,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   table->SetColumnStyle(4,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
   table->SetStripeRowColumnStyle(4,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   table->SetColumnStyle(7,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
   table->SetStripeRowColumnStyle(7,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   table->SetNumberOfHeaderRows(2);

   table->SetRowSpan(0,0,2);
   table->SetRowSpan(1,0,SKIP_CELL);
   (*table)(0,0) << _T("Limit State");

   table->SetRowSpan(0,1,2);
   table->SetRowSpan(1,1,SKIP_CELL);
   (*table)(0,1) << _T("Type");

   table->SetColumnSpan(0,2,3);
   table->SetColumnSpan(0,3,SKIP_CELL);
   table->SetColumnSpan(0,4,SKIP_CELL);
   (*table)(0,2) << strRoutine;
   (*table)(1,2) << _T("RF");
   (*table)(1,3) << Sub2(symbol(gamma),_T("LL"));
   (*table)(1,4) << RPT_LFT_SUPPORT_LOCATION;
   table->SetColumnSpan(0,5,3);
   table->SetColumnSpan(0,6,SKIP_CELL);
   table->SetColumnSpan(0,7,SKIP_CELL);
   (*table)(0,5) << strSpecial;
   (*table)(1,5) << _T("RF");
   (*table)(1,6) << Sub2(symbol(gamma),_T("LL"));
   (*table)(1,7) << RPT_LFT_SUPPORT_LOCATION;
 
   RowIndexType row0 = 2; // row counter for column 0
   RowIndexType row1 = 2; // row counter for column 1
   // row labels
   if ( bNegMoments )
   {
      if ( ratingTableType == Design || ratingTableType == Legal )
      {
         table->SetRowSpan(row0,0,3);
         (*table)(row0++,0) << _T("Strength I");
         table->SetRowSpan(row0++,0,SKIP_CELL);
         table->SetRowSpan(row0++,0,SKIP_CELL);

         (*table)(row1++,1) << _T("Flexure (+M)");
         (*table)(row1++,1) << _T("Flexure (-M)");
         (*table)(row1++,1) << _T("Shear");
      }

      if ( ratingTableType == Permit )
      {
         table->SetRowSpan(row0,0,3);
         (*table)(row0++,0) << _T("Strength II");
         table->SetRowSpan(row0++,0,SKIP_CELL);
         table->SetRowSpan(row0++,0,SKIP_CELL);

         (*table)(row1++,1) << _T("Flexure (+M)");
         (*table)(row1++,1) << _T("Flexure (-M)");
         (*table)(row1++,1) << _T("Shear");

         table->SetRowSpan(row0,0,2);
         (*table)(row0++,0) << _T("Service I");
         table->SetRowSpan(row0++,0,SKIP_CELL);
         (*table)(row1++,1) << _T("Stress Ratio (+M)");
         (*table)(row1++,1) << _T("Stress Ratio (-M)");
      }

      if ( ratingTableType == Design || ratingTableType == Legal )
      {
         (*table)(row0++,0) << _T("Service III");
         (*table)(row1++,1) << _T("Stress");
      }
   }
   else
   {
      if ( ratingTableType == Design || ratingTableType == Legal )
      {
         table->SetRowSpan(row0,0,2);
         (*table)(row0++,0) << _T("Strength I");
         table->SetRowSpan(row0++,0,SKIP_CELL);
         (*table)(row1++,1) << _T("Flexure");
         (*table)(row1++,1) << _T("Shear");
      }

      if ( ratingTableType == Permit )
      {
         table->SetRowSpan(row0,0,2);
         (*table)(row0++,0) << _T("Strength II");
         table->SetRowSpan(row0++,0,SKIP_CELL);
         (*table)(row1++,1) << _T("Flexure");
         (*table)(row1++,1) << _T("Shear");

         (*table)(row0++,0) << _T("Service I");
         (*table)(row1++,1) << _T("Stress Ratio");
      }

      if ( ratingTableType == Design || ratingTableType == Legal )
      {
         (*table)(row0++,0) << _T("Service III");
         (*table)(row1++,1) << _T("Stress");
      }
   }

   GET_IFACE2(pBroker,IArtifact,pArtifact);
   const pgsRatingArtifact* pRoutineRatingArtifact = pArtifact->GetRatingArtifact(gdrLineIdx,routine_rating_type,INVALID_INDEX);
   const pgsRatingArtifact* pSpecialRatingArtifact = pArtifact->GetRatingArtifact(gdrLineIdx,special_rating_type,INVALID_INDEX);

   bool bReportPostingAnalysis = false;

    ColumnIndexType col = 2;
   for (int i = 0; i < 2; i++)
   {
      RowIndexType row = table->GetNumberOfHeaderRows();

      const pgsRatingArtifact* pRatingArtifact = (i == 0 ? pRoutineRatingArtifact : pSpecialRatingArtifact);
      pgsTypes::LoadRatingType ratingType      = (i == 0 ? routine_rating_type    : special_rating_type);
      if ( pRatingArtifact )
      {
         // Strength I
         if ( ratingTableType == Design || ratingTableType == Legal )
         {
            const pgsMomentRatingArtifact* pMomentArtifact;
            Float64 RF = pRatingArtifact->GetMomentRatingFactorEx(true, &pMomentArtifact);
            if ( RF < 1 )
               (*table)(row,col) << RF_FAIL(rating_factor,RF);
            else
               (*table)(row,col) << RF_PASS(rating_factor,RF);

            (*table)(row,col+1) << scalar.SetValue(pMomentArtifact->GetLiveLoadFactor());
 
            pgsPointOfInterest poi = pMomentArtifact->GetPointOfInterest();
            Float64 end_size = pBridge->GetGirderStartConnectionLength(poi.GetSpan(),poi.GetGirder());
            
            (*table)(row,col+2) << location.SetValue(pgsTypes::BridgeSite3, poi,end_size);
            row++;

            if ( bNegMoments )
            {
               RF = pRatingArtifact->GetMomentRatingFactorEx(false, &pMomentArtifact);
               if ( RF < 1 )
                  (*table)(row,col) << RF_FAIL(rating_factor,RF);
               else
                  (*table)(row,col) << RF_PASS(rating_factor,RF);

               (*table)(row,col+1) << scalar.SetValue(pMomentArtifact->GetLiveLoadFactor());

               pgsPointOfInterest poi = pMomentArtifact->GetPointOfInterest();
               Float64 end_size = pBridge->GetGirderStartConnectionLength(poi.GetSpan(),poi.GetGirder());
               
               (*table)(row,col+2) << location.SetValue(pgsTypes::BridgeSite3, poi,end_size);
               row++;
            }

            if ( pRatingSpec->RateForShear(ratingType) )
            {
               const pgsShearRatingArtifact* pShearArtifact;
               Float64 RF = pRatingArtifact->GetShearRatingFactorEx(&pShearArtifact);
               if ( RF < 1 )
                  (*table)(row,col) << RF_FAIL(rating_factor,RF);
               else
                  (*table)(row,col) << RF_PASS(rating_factor,RF);

               (*table)(row,col+1) << scalar.SetValue(pShearArtifact->GetLiveLoadFactor());

               pgsPointOfInterest poi = pShearArtifact->GetPointOfInterest();
               Float64 end_size = pBridge->GetGirderStartConnectionLength(poi.GetSpan(),poi.GetGirder());

               (*table)(row,col+2) << location.SetValue(pgsTypes::BridgeSite3, poi,end_size);
               row++;
            }
            else
            {
               (*table)(row,col)   << _T("");
               (*table)(row,col+1) << _T("");
               (*table)(row,col+2) << _T("");
               row++;
            }
         }

         // Strength II
         if ( ratingTableType == Permit )
         {
            const pgsMomentRatingArtifact* pMomentArtifact;
            Float64 RF = pRatingArtifact->GetMomentRatingFactorEx(true, &pMomentArtifact);
            if ( RF < 1 )
               (*table)(row,col) << RF_FAIL(rating_factor,RF);
            else
               (*table)(row,col) << RF_PASS(rating_factor,RF);

            (*table)(row,col+1) << scalar.SetValue(pMomentArtifact->GetLiveLoadFactor());
            pgsPointOfInterest poi = pMomentArtifact->GetPointOfInterest();
            Float64 end_size = pBridge->GetGirderStartConnectionLength(poi.GetSpan(),poi.GetGirder());
            
            (*table)(row,col+2) << location.SetValue(pgsTypes::BridgeSite3, poi,end_size);
            row++;

            if ( bNegMoments )
            {
               RF = pRatingArtifact->GetMomentRatingFactorEx(false, &pMomentArtifact);
               if ( RF < 1 )
                  (*table)(row,col) << RF_FAIL(rating_factor,RF);
               else
                  (*table)(row,col) << RF_PASS(rating_factor,RF);

               (*table)(row,col+1) << scalar.SetValue(pMomentArtifact->GetLiveLoadFactor());

               pgsPointOfInterest poi = pMomentArtifact->GetPointOfInterest();
               Float64 end_size = pBridge->GetGirderStartConnectionLength(poi.GetSpan(),poi.GetGirder());
               
               (*table)(row,col+2) << location.SetValue(pgsTypes::BridgeSite3, poi,end_size);
               row++;
            }

            if ( pRatingSpec->RateForShear(ratingType) )
            {
               const pgsShearRatingArtifact* pShearArtifact;
               Float64 RF = pRatingArtifact->GetShearRatingFactorEx(&pShearArtifact);
               if ( RF < 1 )
                  (*table)(row,col) << RF_FAIL(rating_factor,RF);
               else
                  (*table)(row,col) << RF_PASS(rating_factor,RF);

               (*table)(row,col+1) << scalar.SetValue(pShearArtifact->GetLiveLoadFactor());
               pgsPointOfInterest poi = pShearArtifact->GetPointOfInterest();
               Float64 end_size = pBridge->GetGirderStartConnectionLength(poi.GetSpan(),poi.GetGirder());

               (*table)(row,col+2) << location.SetValue(pgsTypes::BridgeSite3, poi,end_size);
               row++;
            }
            else
            {
               (*table)(row,col)   << _T("");
               (*table)(row,col+1) << _T("");
               (*table)(row,col+2) << _T("");
               row++;
            }
         }

         // Service I
         if ( ratingTableType == Permit )
         {
            if ( pRatingSpec->RateForStress(ratingType) )
            {
               if ( bNegMoments )
               {
                  const pgsYieldStressRatioArtifact* pYieldStressArtifact;
                  Float64 RF = pRatingArtifact->GetYieldStressRatioEx(true, &pYieldStressArtifact);
                  if ( RF < 1 )
                     (*table)(row,col) << RF_FAIL(rating_factor,RF);
                  else
                     (*table)(row,col) << RF_PASS(rating_factor,RF);

                  (*table)(row,col+1) << scalar.SetValue(pYieldStressArtifact->GetLiveLoadFactor());
                  pgsPointOfInterest poi = pYieldStressArtifact->GetPointOfInterest();
                  Float64 end_size = pBridge->GetGirderStartConnectionLength(poi.GetSpan(),poi.GetGirder());
                  
                  (*table)(row,col+2) << location.SetValue(pgsTypes::BridgeSite3, poi,end_size);
                  row++;

                  RF = pRatingArtifact->GetYieldStressRatioEx(false, &pYieldStressArtifact);
                  if ( RF < 1 )
                     (*table)(row,col) << RF_FAIL(rating_factor,RF);
                  else
                     (*table)(row,col) << RF_PASS(rating_factor,RF);

                  (*table)(row,col+1) << scalar.SetValue(pYieldStressArtifact->GetLiveLoadFactor());
                  poi = pYieldStressArtifact->GetPointOfInterest();
                  end_size = pBridge->GetGirderStartConnectionLength(poi.GetSpan(),poi.GetGirder());
                  
                  (*table)(row,col+2) << location.SetValue(pgsTypes::BridgeSite3, poi,end_size);
                  row++;
               }
               else
               {
                  const pgsYieldStressRatioArtifact* pYieldStressArtifact;
                  Float64 RF = pRatingArtifact->GetYieldStressRatioEx(true, &pYieldStressArtifact);
                  if ( RF < 1 )
                     (*table)(row,col) << RF_FAIL(rating_factor,RF);
                  else
                     (*table)(row,col) << RF_PASS(rating_factor,RF);

                  (*table)(row,col+1) << scalar.SetValue(pYieldStressArtifact->GetLiveLoadFactor());
                  pgsPointOfInterest poi = pYieldStressArtifact->GetPointOfInterest();
                  Float64 end_size = pBridge->GetGirderStartConnectionLength(poi.GetSpan(),poi.GetGirder());
                  
                  (*table)(row,col+2) << location.SetValue(pgsTypes::BridgeSite3, poi,end_size);
                  row++;
               }
            }
            else
            {
               if ( bNegMoments )
               {
                  (*table)(row,col)   << _T("");
                  (*table)(row,col+1) << _T("");
                  (*table)(row,col+2) << _T("");
                  row++;

                  (*table)(row,col)   << _T("");
                  (*table)(row,col+1) << _T("");
                  (*table)(row,col+2) << _T("");
                  row++;
               }
               else
               {
                  (*table)(row,col)   << _T("");
                  (*table)(row,col+1) << _T("");
                  (*table)(row,col+2) << _T("");
                  row++;
               }
            }
         }

         // Service III
         if ( ratingTableType == Design || ratingTableType == Legal )
         {
            if ( ratingTableType == Design && ratingType == pgsTypes::lrDesign_Operating )
            {
               (*table)(row,col)   << _T("");
               (*table)(row,col+1) << _T("");
               (*table)(row,col+2) << _T("");
               row++;
            }
            else
            {
               const pgsStressRatingArtifact* pStressArtifact;
               Float64 RF = pRatingArtifact->GetStressRatingFactorEx(&pStressArtifact);
               if ( RF < 1 )
                  (*table)(row,col) << RF_FAIL(rating_factor,RF);
               else
                  (*table)(row,col) << RF_PASS(rating_factor,RF);

               (*table)(row,col+1) << scalar.SetValue(pStressArtifact->GetLiveLoadFactor());
               pgsPointOfInterest poi = pStressArtifact->GetPointOfInterest();
               Float64 end_size = pBridge->GetGirderStartConnectionLength(poi.GetSpan(),poi.GetGirder());
               
               (*table)(row,col+2) << location.SetValue(pgsTypes::BridgeSite3, poi,end_size);
               row++;
            }
         }
      }
      else
      {
         if ( ratingTableType == Design || ratingTableType == Legal )
         {
            // Strength I
            if ( bNegMoments )
            {
               (*table)(row,col)   << _T("");
               (*table)(row,col+1) << _T("");
               (*table)(row,col+2) << _T("");
               row++;

               (*table)(row,col)   << _T("");
               (*table)(row,col+1) << _T("");
               (*table)(row,col+2) << _T("");
               row++;

               (*table)(row,col)   << _T("");
               (*table)(row,col+1) << _T("");
               (*table)(row,col+2) << _T("");
               row++;
            }
            else
            {
               (*table)(row,col)   << _T("");
               (*table)(row,col+1) << _T("");
               (*table)(row,col+2) << _T("");
               row++;

               (*table)(row,col)   << _T("");
               (*table)(row,col+1) << _T("");
               (*table)(row,col+2) << _T("");
               row++;
            }
         }

         if ( ratingTableType == Permit )
         {
            // Strength II
            if ( bNegMoments )
            {
               (*table)(row,col)   << _T("");
               (*table)(row,col+1) << _T("");
               (*table)(row,col+2) << _T("");
               row++;

               (*table)(row,col)   << _T("");
               (*table)(row,col+1) << _T("");
               (*table)(row,col+2) << _T("");
               row++;

               (*table)(row,col)   << _T("");
               (*table)(row,col+1) << _T("");
               (*table)(row,col+2) << _T("");
               row++;
            }
            else
            {
               (*table)(row,col)   << _T("");
               (*table)(row,col+1) << _T("");
               (*table)(row,col+2) << _T("");
               row++;

               (*table)(row,col)   << _T("");
               (*table)(row,col+1) << _T("");
               (*table)(row,col+2) << _T("");
               row++;
            }

            // Service I
            if ( bNegMoments )
            {
               (*table)(row,col)   << _T("");
               (*table)(row,col+1) << _T("");
               (*table)(row,col+2) << _T("");
               row++;

               (*table)(row,col)   << _T("");
               (*table)(row,col+1) << _T("");
               (*table)(row,col+2) << _T("");
               row++;
            }
            else
            {
               (*table)(row,col)   << _T("");
               (*table)(row,col+1) << _T("");
               (*table)(row,col+2) << _T("");
               row++;
            }
         }

         if ( ratingTableType == Design || ratingTableType == Legal )
         {
            // Service III
            (*table)(row,col)   << _T("");
            (*table)(row,col+1) << _T("");
            (*table)(row,col+2) << _T("");
            row++;
         }
      }

      col += 3;
   }

   return table;
}

rptRcTable* CRatingSummaryTable::BuildByVehicle(IBroker* pBroker,GirderIndexType gdrLineIdx,pgsTypes::LoadRatingType ratingType) const
{
   USES_CONVERSION;

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker,IProductLoads,pProductLoads);
   GET_IFACE2(pBroker,IArtifact,pArtifact);
   GET_IFACE2(pBroker,IBridge,pBridge);

   pgsTypes::LiveLoadType llType = ::GetLiveLoadType(ratingType);

   std::_tstring strName = pProductLoads->GetLiveLoadName(llType,0);
   if ( strName == _T("No Live Load Defined") )
      return NULL;

   rptCapacityToDemand rating_factor;

   rptRcScalar scalar;
   scalar.SetFormat( sysNumericFormatTool::Fixed );
   scalar.SetWidth(6);
   scalar.SetPrecision(3);
   scalar.SetTolerance(1.0e-6);

   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), true );
   location.IncludeSpanAndGirder(true);

   CComBSTR bstrTitle = ::GetLiveLoadTypeName(ratingType);
   rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(5,OLE2T(bstrTitle));

   pTable->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
   pTable->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   pTable->SetColumnStyle(3,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
   pTable->SetStripeRowColumnStyle(3,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   pTable->SetColumnStyle(4,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
   pTable->SetStripeRowColumnStyle(4,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   (*pTable)(0,0) << _T("Truck");
   (*pTable)(0,1) << _T("RF");
   (*pTable)(0,2) << Sub2(symbol(gamma),_T("LL"));
   (*pTable)(0,3)  << _T("Controlling Point") << rptNewLine << RPT_LFT_SUPPORT_LOCATION;
   (*pTable)(0,4) << _T("Cause");

   RowIndexType row = pTable->GetNumberOfHeaderRows();
   VehicleIndexType nVehicles = pProductLoads->GetVehicleCount(llType);
   for ( VehicleIndexType vehIdx = 0; vehIdx < nVehicles; vehIdx++ )
   {
      std::_tstring strName = pProductLoads->GetLiveLoadName(llType,vehIdx);

      const pgsRatingArtifact* pRatingArtifact = pArtifact->GetRatingArtifact(gdrLineIdx,ratingType,vehIdx);

      const pgsMomentRatingArtifact* pPositiveMoment;
      const pgsMomentRatingArtifact* pNegativeMoment;
      const pgsShearRatingArtifact* pShear;
      const pgsStressRatingArtifact* pStress;

      Float64 RF = pRatingArtifact->GetRatingFactorEx(&pPositiveMoment,&pNegativeMoment,&pShear,&pStress);

      Float64 gLL;
      std::_tstring strControlling;
      pgsPointOfInterest poi;
      if ( pPositiveMoment )
      {
         ATLASSERT(vehIdx == pPositiveMoment->GetVehicleIndex());
         gLL = pPositiveMoment->GetLiveLoadFactor();
         strControlling = _T("Positive Moment");
         poi = pPositiveMoment->GetPointOfInterest();
      }
      else if ( pNegativeMoment )
      {
         ATLASSERT(vehIdx == pNegativeMoment->GetVehicleIndex());
         gLL = pNegativeMoment->GetLiveLoadFactor();
         strControlling = _T("Negative Moment");
         poi = pNegativeMoment->GetPointOfInterest();
      }
      else if ( pShear )
      {
         ATLASSERT(vehIdx == pShear->GetVehicleIndex());
         gLL = pShear->GetLiveLoadFactor();
         strControlling = _T("Shear");
         poi = pShear->GetPointOfInterest();
      }
      else if ( pStress )
      {
         ATLASSERT(vehIdx == pStress->GetVehicleIndex());
         gLL = pStress->GetLiveLoadFactor();
         strControlling = _T("Stress");
         poi = pStress->GetPointOfInterest();
      }
      else
      {
         gLL = -1;
         strControlling = _T("UNKNOWN");
      }

      Float64 end_size = pBridge->GetGirderStartConnectionLength(poi.GetSpan(),poi.GetGirder());
      if ( 0 < gLL )
      {
         (*pTable)(row,0) << strName;

         if ( RF < 1 )
            (*pTable)(row,1) << RF_FAIL(rating_factor,RF);
         else
            (*pTable)(row,1) << RF_PASS(rating_factor,RF);

         (*pTable)(row,2) << scalar.SetValue(gLL);
         (*pTable)(row,3) << location.SetValue( pgsTypes::BridgeSite3, poi,end_size );
         (*pTable)(row,4) << strControlling;

         row++;
      }
   }

   return pTable;
}

rptRcTable* CRatingSummaryTable::BuildByVehicleStressTable(IBroker* pBroker,GirderIndexType gdrLineIdx,pgsTypes::LoadRatingType ratingType) const
{
   USES_CONVERSION;

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker,IProductLoads,pProductLoads);
   GET_IFACE2(pBroker,IArtifact,pArtifact);
   GET_IFACE2(pBroker,IBridge,pBridge);

   pgsTypes::LiveLoadType llType = ::GetLiveLoadType(ratingType);

   std::_tstring strName = pProductLoads->GetLiveLoadName(llType,0);
   if ( strName == _T("No Live Load Defined") )
      return NULL;

   rptCapacityToDemand rating_factor;

   rptRcScalar scalar;
   scalar.SetFormat( sysNumericFormatTool::Fixed );
   scalar.SetWidth(6);
   scalar.SetPrecision(3);
   scalar.SetTolerance(1.0e-6);

   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), true );
   location.IncludeSpanAndGirder(true);

   CComBSTR bstrTitle = ::GetLiveLoadTypeName(ratingType);
   rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(5,OLE2T(bstrTitle));

   pTable->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
   pTable->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   pTable->SetColumnStyle(3,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
   pTable->SetStripeRowColumnStyle(3,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   pTable->SetColumnStyle(4,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
   pTable->SetStripeRowColumnStyle(4,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   (*pTable)(0,0) << _T("Truck");
   (*pTable)(0,1) << _T("Stress Ratio");
   (*pTable)(0,2) << Sub2(symbol(gamma),_T("LL"));
   (*pTable)(0,3)  << _T("Controlling Point") << rptNewLine << RPT_LFT_SUPPORT_LOCATION;
   (*pTable)(0,4) << _T("Cause");

   RowIndexType row = pTable->GetNumberOfHeaderRows();
   VehicleIndexType nVehicles = pProductLoads->GetVehicleCount(llType);
   for ( VehicleIndexType vehIdx = 0; vehIdx < nVehicles; vehIdx++ )
   {
      std::_tstring strName = pProductLoads->GetLiveLoadName(llType,vehIdx);

      const pgsRatingArtifact* pRatingArtifact = pArtifact->GetRatingArtifact(gdrLineIdx,ratingType,vehIdx);

      const pgsYieldStressRatioArtifact* pYieldStressPositiveMoment;
      const pgsYieldStressRatioArtifact* pYieldStressNegativeMoment;

	  Float64 SR_PM = pRatingArtifact->GetYieldStressRatioEx(true,&pYieldStressPositiveMoment);
	  Float64 SR_NM = pRatingArtifact->GetYieldStressRatioEx(false,&pYieldStressNegativeMoment);

	  Float64 SR = min(SR_PM,SR_NM);

      Float64 gLL;
      std::_tstring strControlling;
      pgsPointOfInterest poi;
      if ( SR_PM < SR_NM )
      {
         ATLASSERT(vehIdx == pYieldStressPositiveMoment->GetVehicleIndex());
         gLL = pYieldStressPositiveMoment->GetLiveLoadFactor();
         strControlling = _T("Yield Stress Positive Moment");
         poi = pYieldStressPositiveMoment->GetPointOfInterest();
      }
      else if ( pYieldStressNegativeMoment )
      {
         ATLASSERT(vehIdx == pYieldStressNegativeMoment->GetVehicleIndex());
         gLL = pYieldStressNegativeMoment->GetLiveLoadFactor();
         strControlling = _T("Yield Stress Negative Moment");
         poi = pYieldStressNegativeMoment->GetPointOfInterest();
      }

      Float64 end_size = pBridge->GetGirderStartConnectionLength(poi.GetSpan(),poi.GetGirder());
      if ( 0 < gLL )
      {
         (*pTable)(row,0) << strName;

         if ( SR < 1 )
            (*pTable)(row,1) << RF_FAIL(rating_factor,SR);
         else
            (*pTable)(row,1) << RF_PASS(rating_factor,SR);

         (*pTable)(row,2) << scalar.SetValue(gLL);
         (*pTable)(row,3) << location.SetValue( pgsTypes::BridgeSite3, poi,end_size );
         (*pTable)(row,4) << strControlling;

         row++;
      }
   }

   return pTable;
}

rptRcTable* CRatingSummaryTable::BuildLoadPosting(IBroker* pBroker,GirderIndexType gdrLineIdx,pgsTypes::LoadRatingType ratingType) const
{
   GET_IFACE2(pBroker,IProductLoads,pProductLoads);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker,IArtifact,pArtifact);

   rptCapacityToDemand rating_factor;
   INIT_UV_PROTOTYPE( rptForceUnitValue, tonnage, pDisplayUnits->GetTonnageUnit(), false );

   pgsTypes::LiveLoadType llType = ::GetLiveLoadType(ratingType);

   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(5,_T("Load Posting (MBE 6A.8)"));

   table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
   table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   // across the top
   (*table)(0,0) << _T("Vehicle");
   (*table)(0,1) << COLHDR(_T("Vehicle Weight"), rptForceUnitTag, pDisplayUnits->GetTonnageUnit());
   (*table)(0,2) << _T("RF");
   (*table)(0,3) << COLHDR(_T("Safe Load Capacity"), rptForceUnitTag, pDisplayUnits->GetTonnageUnit());
   (*table)(0,4) << COLHDR(_T("Safe Posting Load"),  rptForceUnitTag, pDisplayUnits->GetTonnageUnit());

   bool bLoadPostingRequired = false;

   RowIndexType row = table->GetNumberOfHeaderRows();
   VehicleIndexType nVehicles = pProductLoads->GetVehicleCount(llType);
   for ( VehicleIndexType vehIdx = 0; vehIdx < nVehicles; vehIdx++ )
   {
      ColumnIndexType col = 0;
      const pgsRatingArtifact* pRatingArtifact = pArtifact->GetRatingArtifact(gdrLineIdx,ratingType,vehIdx);
      if ( pRatingArtifact )
      {
         Float64 postingLoad, W, RF;
         std::_tstring strName;
         pRatingArtifact->GetSafePostingLoad(&postingLoad,&W,&RF,&strName);

         (*table)(row,col++) << strName;
         (*table)(row,col++) << tonnage.SetValue(W);

         if ( RF < 1 )
            (*table)(row,col++) << RF_FAIL(rating_factor,RF);
         else
            (*table)(row,col++) << RF_PASS(rating_factor,RF);

         (*table)(row,col++) << tonnage.SetValue(::FloorOff(W*RF,0.01));

         if ( RF < 1 )
            (*table)(row,col++) << tonnage.SetValue(postingLoad);
         else
            (*table)(row,col++) << _T("-");

         if ( RF < 1 )
            bLoadPostingRequired = true;
      }
      else
      {
         (*table)(row,col++) << _T("");
         (*table)(row,col++) << _T("");
         (*table)(row,col++) << _T("");
         (*table)(row,col++) << _T("");
         (*table)(row,col++) << _T("");
      }

      row++;
   }

   if ( !bLoadPostingRequired )
   {
      delete table;
      table = NULL;
   }

   return table;
}