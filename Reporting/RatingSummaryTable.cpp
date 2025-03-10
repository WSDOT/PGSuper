///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2025  Washington State Department of Transportation
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
#include <PgsExt\ISummaryRatingArtifact.h>
#include <PgsExt\CapacityToDemand.h>
#include <PgsExt\Helpers.h>

#include <IFace\Bridge.h>
#include <IFace\Artifact.h>
#include <IFace\AnalysisResults.h>
#include <IFace\RatingSpecification.h>

rptRcTable* CRatingSummaryTable::BuildByLimitState(IBroker* pBroker,const std::vector<CGirderKey>& girderKeys,CRatingSummaryTable::RatingTableType ratingTableType)
{
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker,IRatingSpecification,pRatingSpec);
   GET_IFACE2(pBroker,IBridge,pBridge);

   bool bNegMoments = pBridge->ProcessNegativeMoments(ALL_SPANS);

   rptCapacityToDemand rating_factor;

   INIT_UV_PROTOTYPE( rptForceUnitValue,  force,    pDisplayUnits->GetGeneralForceUnit(), false );
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(),   true  );

   location.IncludeSpanAndGirder(true);

   rptRcScalar scalar;
   scalar.SetFormat( WBFL::System::NumericFormatTool::Format::Fixed );
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

   rptRcTable* table = rptStyleManager::CreateDefaultTable(8);
   table->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   table->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   table->SetColumnStyle(1,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   table->SetStripeRowColumnStyle(1,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   table->SetColumnStyle(4,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   table->SetStripeRowColumnStyle(4,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   table->SetColumnStyle(7,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   table->SetStripeRowColumnStyle(7,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   table->SetNumberOfHeaderRows(2);

   table->SetRowSpan(0,0,2);
   (*table)(0,0) << _T("Limit State");

   table->SetRowSpan(0,1,2);
   (*table)(0,1) << _T("Type");

   table->SetColumnSpan(0,2,3);
   (*table)(0,2) << strRoutine;
   (*table)(1,2) << _T("RF");
   (*table)(1,3) << Sub2(symbol(gamma),_T("LL"));
   (*table)(1,4) << RPT_LFT_SUPPORT_LOCATION;
   table->SetColumnSpan(0,5,3);
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
         (*table)(row0,0) << _T("Strength I");
         row0 += 3;

         (*table)(row1++,1) << _T("Flexure (+M)");
         (*table)(row1++,1) << _T("Flexure (-M)");
         (*table)(row1++,1) << _T("Shear");
      }

      if ( ratingTableType == Permit )
      {
         table->SetRowSpan(row0,0,3);
         (*table)(row0,0) << _T("Strength II");
         row0 += 3;

         (*table)(row1++,1) << _T("Flexure (+M)");
         (*table)(row1++,1) << _T("Flexure (-M)");
         (*table)(row1++,1) << _T("Shear");

         table->SetRowSpan(row0,0,2);
         (*table)(row0,0) << _T("Service I");
         row0 += 2;

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
         (*table)(row0,0) << _T("Strength I");
         row0 += 2;

         (*table)(row1++,1) << _T("Flexure");
         (*table)(row1++,1) << _T("Shear");
      }

      if ( ratingTableType == Permit )
      {
         table->SetRowSpan(row0,0,2);
         (*table)(row0,0) << _T("Strength II");
         row0 += 2;
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
   std::shared_ptr<const pgsISummaryRatingArtifact> pRoutineRatingArtifact = pArtifact->GetSummaryRatingArtifact(girderKeys,routine_rating_type,INVALID_INDEX);
   std::shared_ptr<const pgsISummaryRatingArtifact> pSpecialRatingArtifact = pArtifact->GetSummaryRatingArtifact(girderKeys,special_rating_type,INVALID_INDEX);

   bool bReportPostingAnalysis = false;

    ColumnIndexType col = 2;
   for (int i = 0; i < 2; i++)
   {
      RowIndexType row = table->GetNumberOfHeaderRows();

      std::shared_ptr<const pgsISummaryRatingArtifact> pRatingArtifact = (i == 0 ? pRoutineRatingArtifact : pSpecialRatingArtifact);
      pgsTypes::LoadRatingType ratingType      = (i == 0 ? routine_rating_type    : special_rating_type);
      if ( pRatingArtifact )
      {
         // Strength I
         if ( ratingTableType == Design || ratingTableType == Legal )
         {
            const pgsMomentRatingArtifact* pMomentArtifact;
            Float64 RF = pRatingArtifact->GetMomentRatingFactorEx(true, &pMomentArtifact);
            if ( RF < 1 )
            {
               (*table)(row,col) << RF_FAIL(rating_factor,RF);
            }
            else
            {
               (*table)(row,col) << RF_PASS(rating_factor,RF);
            }

            if ( pMomentArtifact )
            {
               (*table)(row,col+1) << scalar.SetValue(pMomentArtifact->GetLiveLoadFactor());
               const pgsPointOfInterest& poi = pMomentArtifact->GetPointOfInterest();
               (*table)(row,col+2) << location.SetValue(POI_SPAN, poi);
            }
            else
            {
               (*table)(row,col+1) << _T("");
               (*table)(row,col+2) << _T("");
            }
            row++;

            if ( bNegMoments )
            {
               RF = pRatingArtifact->GetMomentRatingFactorEx(false, &pMomentArtifact);
               if ( RF < 1 )
               {
                  (*table)(row,col) << RF_FAIL(rating_factor,RF);
               }
               else
               {
                  (*table)(row,col) << RF_PASS(rating_factor,RF);
               }

               if ( pMomentArtifact )
               {
                  (*table)(row,col+1) << scalar.SetValue(pMomentArtifact->GetLiveLoadFactor());

                  const pgsPointOfInterest& poi = pMomentArtifact->GetPointOfInterest();
                  (*table)(row,col+2) << location.SetValue(POI_SPAN, poi);
               }
               else
               {
                  (*table)(row,col+1) << _T("");
                  (*table)(row,col+2) << _T("");
               }
               row++;
            }

            if ( pRatingSpec->RateForShear(ratingType) )
            {
               const pgsShearRatingArtifact* pShearArtifact;
               Float64 RF = pRatingArtifact->GetShearRatingFactorEx(&pShearArtifact);
               if ( RF < 1 )
               {
                  (*table)(row,col) << RF_FAIL(rating_factor,RF);
               }
               else
               {
                  (*table)(row,col) << RF_PASS(rating_factor,RF);
               }

               if ( pShearArtifact )
               {
                  (*table)(row,col+1) << scalar.SetValue(pShearArtifact->GetLiveLoadFactor());

                  const pgsPointOfInterest& poi = pShearArtifact->GetPointOfInterest();
                  (*table)(row,col+2) << location.SetValue(POI_SPAN, poi);
               }
               else
               {
                  (*table)(row,col+1) << _T("");
                  (*table)(row,col+2) << _T("");
               }
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
            {
               (*table)(row,col) << RF_FAIL(rating_factor,RF);
            }
            else
            {
               (*table)(row,col) << RF_PASS(rating_factor,RF);
            }

            if ( pMomentArtifact )
            {
               (*table)(row,col+1) << scalar.SetValue(pMomentArtifact->GetLiveLoadFactor());
               const pgsPointOfInterest& poi = pMomentArtifact->GetPointOfInterest();
               (*table)(row,col+2) << location.SetValue(POI_SPAN, poi);
            }
            else
            {
               (*table)(row,col+1) << _T("");
               (*table)(row,col+2) << _T("");
            }
            row++;

            if ( bNegMoments )
            {
               RF = pRatingArtifact->GetMomentRatingFactorEx(false, &pMomentArtifact);
               if ( RF < 1 )
               {
                  (*table)(row,col) << RF_FAIL(rating_factor,RF);
               }
               else
               {
                  (*table)(row,col) << RF_PASS(rating_factor,RF);
               }

               if ( pMomentArtifact )
               {
                  (*table)(row,col+1) << scalar.SetValue(pMomentArtifact->GetLiveLoadFactor());

                  const pgsPointOfInterest& poi = pMomentArtifact->GetPointOfInterest();
                  (*table)(row,col+2) << location.SetValue(POI_SPAN, poi);
               }
               else
               {
                  (*table)(row,col+1) << _T("");
                  (*table)(row,col+2) << _T("");
               }
               row++;
            }

            if ( pRatingSpec->RateForShear(ratingType) )
            {
               const pgsShearRatingArtifact* pShearArtifact;
               Float64 RF = pRatingArtifact->GetShearRatingFactorEx(&pShearArtifact);
               if ( RF < 1 )
               {
                  (*table)(row,col) << RF_FAIL(rating_factor,RF);
               }
               else
               {
                  (*table)(row,col) << RF_PASS(rating_factor,RF);
               }

               if ( pShearArtifact )
               {
                  (*table)(row,col+1) << scalar.SetValue(pShearArtifact->GetLiveLoadFactor());

                  const pgsPointOfInterest& poi = pShearArtifact->GetPointOfInterest();
                  (*table)(row,col+2) << location.SetValue(POI_SPAN, poi);
               }
               else
               {
                  (*table)(row,col+1) << _T("");
                  (*table)(row,col+2) << _T("");
               }
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
                  {
                     (*table)(row,col) << RF_FAIL(rating_factor,RF);
                  }
                  else
                  {
                     (*table)(row,col) << RF_PASS(rating_factor,RF);
                  }

                  if ( pYieldStressArtifact )
                  {
                     (*table)(row,col+1) << scalar.SetValue(pYieldStressArtifact->GetLiveLoadFactor());

                     pgsPointOfInterest poi = pYieldStressArtifact->GetPointOfInterest();
                     (*table)(row,col+2) << location.SetValue(POI_SPAN, poi);
                  }
                  else
                  {
                     (*table)(row,col+1) << _T("");
                     (*table)(row,col+2) << _T("");
                  }
                  row++;

                  RF = pRatingArtifact->GetYieldStressRatioEx(false, &pYieldStressArtifact);
                  if ( RF < 1 )
                  {
                     (*table)(row,col) << RF_FAIL(rating_factor,RF);
                  }
                  else
                  {
                     (*table)(row,col) << RF_PASS(rating_factor,RF);
                  }

                  if ( pYieldStressArtifact )
                  {
                     (*table)(row,col+1) << scalar.SetValue(pYieldStressArtifact->GetLiveLoadFactor());

                     pgsPointOfInterest poi = pYieldStressArtifact->GetPointOfInterest();
                     (*table)(row,col+2) << location.SetValue(POI_SPAN, poi);
                  }
                  else
                  {
                     (*table)(row,col+1) << _T("");
                     (*table)(row,col+2) << _T("");
                  }
                  row++;
               }
               else
               {
                  const pgsYieldStressRatioArtifact* pYieldStressArtifact;
                  Float64 RF = pRatingArtifact->GetYieldStressRatioEx(true, &pYieldStressArtifact);
                  if ( RF < 1 )
                  {
                     (*table)(row,col) << RF_FAIL(rating_factor,RF);
                  }
                  else
                  {
                     (*table)(row,col) << RF_PASS(rating_factor,RF);
                  }

                  if ( pYieldStressArtifact )
                  {
                     (*table)(row,col+1) << scalar.SetValue(pYieldStressArtifact->GetLiveLoadFactor());

                     const pgsPointOfInterest& poi = pYieldStressArtifact->GetPointOfInterest();
                     (*table)(row,col+2) << location.SetValue(POI_SPAN, poi);
                  }
                  else
                  {
                     (*table)(row,col+1) << _T("");
                     (*table)(row,col+2) << _T("");
                  }
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
               {
                  (*table)(row,col) << RF_FAIL(rating_factor,RF);
               }
               else
               {
                  (*table)(row,col) << RF_PASS(rating_factor,RF);
               }

               if ( pStressArtifact )
               {
                  (*table)(row,col+1) << scalar.SetValue(pStressArtifact->GetLiveLoadFactor());

                  const pgsPointOfInterest& poi = pStressArtifact->GetPointOfInterest();
                  (*table)(row,col+2) << location.SetValue(POI_SPAN, poi);
               }
               else
               {
                  (*table)(row,col+1) << _T("");
                  (*table)(row,col+2) << _T("");
               }
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

rptRcTable* CRatingSummaryTable::BuildByVehicle(IBroker* pBroker, const std::vector<CGirderKey>& girderKeys, pgsTypes::LoadRatingType ratingType)
{
   GET_IFACE2(pBroker, IProductLoads, pProductLoads);

   pgsTypes::LiveLoadType llType = ::GetLiveLoadType(ratingType);

   std::_tstring strName = pProductLoads->GetLiveLoadName(llType, 0);
   if (strName == NO_LIVE_LOAD_DEFINED)
   {
      return nullptr;
   }

   GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);
   GET_IFACE2(pBroker, IArtifact, pArtifact);
   GET_IFACE2(pBroker, IBridge, pBridge);

   rptCapacityToDemand rating_factor;

   rptRcScalar scalar;
   scalar.SetFormat( WBFL::System::NumericFormatTool::Format::Fixed );
   scalar.SetWidth(6);
   scalar.SetPrecision(3);
   scalar.SetTolerance(1.0e-6);

   INIT_UV_PROTOTYPE(rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), true);
   location.IncludeSpanAndGirder(true);

   ColumnIndexType nColumns = 5;
   CString strTitle = ::GetLiveLoadTypeName(ratingType);
   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(nColumns, strTitle);

   ColumnIndexType col = 0;

   (*pTable)(0, col++) << _T("Truck");
   (*pTable)(0, col++) << _T("RF");
   (*pTable)(0, col++) << Sub2(symbol(gamma), _T("LL"));
   (*pTable)(0, col++) << _T("Controlling Point") << rptNewLine << RPT_LFT_SUPPORT_LOCATION;
   (*pTable)(0, col++) << _T("Cause");

   RowIndexType row = pTable->GetNumberOfHeaderRows();
   VehicleIndexType nVehicles = pProductLoads->GetVehicleCount(llType);
   for ( VehicleIndexType vehicleIdx = 0; vehicleIdx < nVehicles; vehicleIdx++, row++ )
   {
      col = 0;
      std::_tstring strName = pProductLoads->GetLiveLoadName(llType,vehicleIdx);

      std::shared_ptr<const pgsISummaryRatingArtifact> pRatingArtifact = pArtifact->GetSummaryRatingArtifact(girderKeys,ratingType,vehicleIdx);

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
         ATLASSERT(vehicleIdx == pPositiveMoment->GetVehicleIndex());
         gLL = pPositiveMoment->GetLiveLoadFactor();
         strControlling = _T("Positive Moment");
         poi = pPositiveMoment->GetPointOfInterest();
      }
      else if ( pNegativeMoment )
      {
         ATLASSERT(vehicleIdx == pNegativeMoment->GetVehicleIndex());
         gLL = pNegativeMoment->GetLiveLoadFactor();
         strControlling = _T("Negative Moment");
         poi = pNegativeMoment->GetPointOfInterest();
      }
      else if ( pShear )
      {
         ATLASSERT(vehicleIdx == pShear->GetVehicleIndex());
         gLL = pShear->GetLiveLoadFactor();
         strControlling = _T("Shear");
         poi = pShear->GetPointOfInterest();
      }
      else if ( pStress )
      {
         ATLASSERT(vehicleIdx == pStress->GetVehicleIndex());
         gLL = pStress->GetLiveLoadFactor();
         strControlling = _T("Stress");
         poi = pStress->GetPointOfInterest();
      }
      else
      {
         gLL = 0;
         strControlling = _T("UNKNOWN");
      }

     const CSegmentKey& segmentKey = poi.GetSegmentKey();
     Float64 end_size = pBridge->GetSegmentStartEndDistance(segmentKey);

     (*pTable)(row,col++) << strName;

     if ( RF < 1 )
     {
        (*pTable)(row, col++) << RF_FAIL(rating_factor,RF);
     }
     else
     {
        (*pTable)(row, col++) << RF_PASS(rating_factor,RF);
     }

     (*pTable)(row, col++) << scalar.SetValue(gLL);
     (*pTable)(row, col++) << location.SetValue( POI_SPAN, poi );
     (*pTable)(row, col++) << strControlling;
   }

   return pTable;
}

rptRcTable* CRatingSummaryTable::BuildLoadPosting(IBroker* pBroker,const std::vector<CGirderKey>& girderKeys,pgsTypes::LoadRatingType ratingType,bool* pbMustCloseBridge)
{
   GET_IFACE2(pBroker,IProductLoads,pProductLoads);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker,IArtifact,pArtifact);

   rptCapacityToDemand rating_factor;
   INIT_UV_PROTOTYPE( rptForceUnitValue, tonnage, pDisplayUnits->GetTonnageUnit(), false );

   pgsTypes::LiveLoadType llType = ::GetLiveLoadType(ratingType);

   rptRcTable* table = rptStyleManager::CreateDefaultTable(5,_T("Load Posting (MBE 6A.8)"));

   table->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   table->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   // across the top
   (*table)(0,0) << _T("Vehicle");
   (*table)(0,1) << COLHDR(_T("Vehicle Weight"), rptForceUnitTag, pDisplayUnits->GetTonnageUnit());
   (*table)(0,2) << _T("RF");
   (*table)(0,3) << COLHDR(_T("Safe Load Capacity (RT)"), rptForceUnitTag, pDisplayUnits->GetTonnageUnit());
   (*table)(0,4) << COLHDR(_T("Safe Posting Load"),  rptForceUnitTag, pDisplayUnits->GetTonnageUnit());

   bool bLoadPostingRequired = false;
   bool bMustCloseBridge = false; // MBE 6A.8.1 and .3 - bridges not capable of carrying a minimum gross live load of weight of three tons must be closed.
   Float64 RTmin = WBFL::Units::ConvertToSysUnits(3.0, WBFL::Units::Measure::Ton);

   RowIndexType row = table->GetNumberOfHeaderRows();
   VehicleIndexType nVehicles = pProductLoads->GetVehicleCount(llType);
   for ( VehicleIndexType vehicleIdx = 0; vehicleIdx < nVehicles; vehicleIdx++ )
   {
      ColumnIndexType col = 0;
      std::shared_ptr<const pgsISummaryRatingArtifact> pRatingArtifact = pArtifact->GetSummaryRatingArtifact(girderKeys,ratingType,vehicleIdx);
      if ( pRatingArtifact )
      {
         Float64 postingLoad, W, RF;
         std::_tstring strName;
         pRatingArtifact->GetSafePostingLoad(&postingLoad,&W,&RF,&strName);

         if ( 1 <= RF )
         {
            continue;
         }

         (*table)(row,col++) << strName;
         (*table)(row,col++) << tonnage.SetValue(W);

         if ( RF < 1 )
         {
            (*table)(row,col++) << RF_FAIL(rating_factor,RF);
         }
         else
         {
            (*table)(row,col++) << RF_PASS(rating_factor,RF);
         }

         Float64 RT = ::FloorOff(W*RF, 0.01);
         if (RT < RTmin || RF < 0.3) // RF < 0.3 is from MBE C6A.8.3
         {
            bMustCloseBridge = true;
         }
         (*table)(row,col++) << tonnage.SetValue(RT);

         if ( RF < 1 )
         {
            (*table)(row,col++) << tonnage.SetValue(postingLoad);
         }
         else
         {
            (*table)(row,col++) << _T("-");
         }

         if ( RF < 1 )
         {
            bLoadPostingRequired = true;
         }
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
      table = nullptr;
   }

   *pbMustCloseBridge = bMustCloseBridge;
   return table;
}

rptRcTable* CRatingSummaryTable::BuildEmergencyVehicleLoadPosting(IBroker* pBroker, const std::vector<CGirderKey>& girderKeys)
{
   GET_IFACE2(pBroker, IArtifact, pArtifact);

   std::shared_ptr<const pgsISummaryRatingArtifact> pEV2Artifact = pArtifact->GetSummaryRatingArtifact(girderKeys, pgsTypes::lrLegal_Emergency, 0);
   std::shared_ptr<const pgsISummaryRatingArtifact> pEV3Artifact = pArtifact->GetSummaryRatingArtifact(girderKeys, pgsTypes::lrLegal_Emergency, 1);

   Float64 RF2 = pEV2Artifact->GetRatingFactor();
   Float64 RF3 = pEV3Artifact->GetRatingFactor();

   if (1.0 <= RF3)
   {
      // if RF for Type EV3 is OK, no load posting required
      return nullptr;
   }


   Float64 wgtEV2 = WBFL::Units::ConvertToSysUnits(57.5, WBFL::Units::Measure::Kip);
   Float64 wgtEV3 = WBFL::Units::ConvertToSysUnits(86.0, WBFL::Units::Measure::Kip);

   Float64 axleEV2 = WBFL::Units::ConvertToSysUnits(33.5, WBFL::Units::Measure::Kip);
   Float64 tandemEV3 = WBFL::Units::ConvertToSysUnits(62.0, WBFL::Units::Measure::Kip);

   ATLASSERT(RF3 < 1.0);

   Float64 W2, W3, GVW;
   if (RF2 < 1.0)
   {
      W2 = RF2*axleEV2;
      W3 = RF3*tandemEV3;
      GVW = Min(RF2*wgtEV2, RF3*wgtEV3);
   }
   else
   {
      W2 = axleEV2;
      W3 = RF3*tandemEV3;
      GVW = RF3*wgtEV3;
   }


   GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);

   INIT_UV_PROTOTYPE(rptForceUnitValue, tonnage, pDisplayUnits->GetTonnageUnit(), false);
   rptCapacityToDemand rating_factor;

   rptRcTable* table = rptStyleManager::CreateDefaultTable(7, _T("Emergency Vehicle Load Posting"));

   table->SetColumnStyle(0, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   table->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   ColumnIndexType col = 0;
   RowIndexType row = 0;

   table->SetNumberOfHeaderRows(2);

   table->SetColumnSpan(0, col, 2);

   (*table)(0, col) << _T("Type EV2");
   (*table)(1, col++) << COLHDR(_T("Weight"), rptForceUnitTag, pDisplayUnits->GetTonnageUnit());
   (*table)(1, col++) << _T("RF");

   table->SetColumnSpan(0, col, 2);

   (*table)(0, col) << _T("Type EV3");
   (*table)(1, col++) << COLHDR(_T("Weight"), rptForceUnitTag, pDisplayUnits->GetTonnageUnit());
   (*table)(1, col++) << _T("RF");

   table->SetRowSpan(0, col, 2);
   (*table)(0, col++) << COLHDR(_T("Single Axle"), rptForceUnitTag, pDisplayUnits->GetTonnageUnit());

   table->SetRowSpan(0, col, 2);
   (*table)(0, col++) << COLHDR(_T("Tandem"), rptForceUnitTag, pDisplayUnits->GetTonnageUnit());

   table->SetRowSpan(0, col, 2);
   (*table)(0, col++) << COLHDR(_T("Gross"), rptForceUnitTag, pDisplayUnits->GetTonnageUnit());

   row = table->GetNumberOfHeaderRows();
   col = 0;
   (*table)(row, col++) << tonnage.SetValue(wgtEV2);
   if (RF2 < 1.0)
   {
      (*table)(row, col++) << RF_FAIL(rating_factor, RF2);
   }
   else
   {
      (*table)(row, col++) << RF_PASS(rating_factor, RF2);
   }

   (*table)(row, col++) << tonnage.SetValue(wgtEV3);
   if (RF3 < 1.0)
   {
      (*table)(row, col++) << RF_FAIL(rating_factor, RF3);
   }
   else
   {
      (*table)(row, col++) << RF_PASS(rating_factor, RF3);
   }

   (*table)(row, col++) << tonnage.SetValue(W2);
   (*table)(row, col++) << tonnage.SetValue(W3);
   (*table)(row, col++) << tonnage.SetValue(GVW);

   return table;
}

rptRcTable* CRatingSummaryTable::BuildYieldStressRatio(IBroker* pBroker, const std::vector<CGirderKey>& girderKeys, pgsTypes::LoadRatingType ratingType)
{
   PRECONDITION(::IsPermitRatingType(ratingType));

   GET_IFACE2(pBroker, IProductLoads, pProductLoads);

   pgsTypes::LiveLoadType llType = ::GetLiveLoadType(ratingType);

   std::_tstring strName = pProductLoads->GetLiveLoadName(llType, 0);
   if (strName == NO_LIVE_LOAD_DEFINED)
   {
      return nullptr;
   }

   GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);
   GET_IFACE2(pBroker, IArtifact, pArtifact);

   rptCapacityToDemand rating_factor;

   rptRcScalar scalar;
   scalar.SetFormat(WBFL::System::NumericFormatTool::Format::Fixed);
   scalar.SetWidth(6);
   scalar.SetPrecision(3);
   scalar.SetTolerance(1.0e-6);

   INIT_UV_PROTOTYPE(rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), true);
   location.IncludeSpanAndGirder(true);

   ColumnIndexType nColumns = 5;
   CString strTitle = ::GetLiveLoadTypeName(ratingType);
   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(nColumns, strTitle);

   ColumnIndexType col = 0;

   (*pTable)(0, col++) << _T("Truck");
   (*pTable)(0, col++) << _T("Yield") << rptNewLine << _T("Stress") << rptNewLine << _T("Ratio");
   (*pTable)(0, col++) << Sub2(symbol(gamma), _T("LL"));
   (*pTable)(0, col++) << _T("Controlling Point") << rptNewLine << RPT_LFT_SUPPORT_LOCATION;
   (*pTable)(0, col++) << _T("Cause");

   RowIndexType row = pTable->GetNumberOfHeaderRows();
   VehicleIndexType nVehicles = pProductLoads->GetVehicleCount(llType);
   for (VehicleIndexType vehicleIdx = 0; vehicleIdx < nVehicles; vehicleIdx++, row++)
   {
      col = 0;
      std::_tstring strName = pProductLoads->GetLiveLoadName(llType, vehicleIdx);

      std::shared_ptr<const pgsISummaryRatingArtifact> pRatingArtifact = pArtifact->GetSummaryRatingArtifact(girderKeys, ratingType, vehicleIdx);

      const pgsYieldStressRatioArtifact* pYieldStressPositiveMoment;
      const pgsYieldStressRatioArtifact* pYieldStressNegativeMoment;
      Float64 SR = pRatingArtifact->GetYieldStressRatio(&pYieldStressPositiveMoment, &pYieldStressNegativeMoment);
      Float64 gLL;
      std::_tstring strControlling;
      pgsPointOfInterest poi;
      if (pYieldStressPositiveMoment || pYieldStressNegativeMoment)
      {
         if (pYieldStressPositiveMoment)
         {
            CHECK(vehicleIdx == pYieldStressPositiveMoment->GetVehicleIndex());
            CHECK(pYieldStressNegativeMoment == nullptr);
            gLL = pYieldStressPositiveMoment->GetLiveLoadFactor();
            poi = pYieldStressPositiveMoment->GetPointOfInterest();
            strControlling = _T("Positive Moment");
         }
         else
         {
            CHECK(vehicleIdx == pYieldStressNegativeMoment->GetVehicleIndex());
            CHECK(pYieldStressPositiveMoment == nullptr);
            gLL = pYieldStressNegativeMoment->GetLiveLoadDistributionFactor();
            gLL = pYieldStressNegativeMoment->GetLiveLoadFactor();
            poi = pYieldStressNegativeMoment->GetPointOfInterest();
            strControlling = _T("Negative Moment");
         }

         col = 0;

         (*pTable)(row, col++) << strName;

         if (SR < 1)
         {
            (*pTable)(row, col++) << RF_FAIL(rating_factor, SR);
         }
         else
         {
            (*pTable)(row, col++) << RF_PASS(rating_factor, SR);
         }

         (*pTable)(row, col++) << scalar.SetValue(gLL);
         (*pTable)(row, col++) << location.SetValue(POI_SPAN, poi);
         (*pTable)(row, col++) << strControlling;
      }
   }

   return pTable;
}
