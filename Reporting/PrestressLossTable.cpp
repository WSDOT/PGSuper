///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2018  Washington State Department of Transportation
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

#include <PgsExt\ReportPointOfInterest.h>
#include <PgsExt\StrandData.h>

#include <IFace\Project.h>
#include <IFace\PrestressForce.h>
#include <IFace\Bridge.h>
#include <IFace\Intervals.h>
#include <IFace\AnalysisResults.h>
#include <IFace\RatingSpecification.h>

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
rptRcTable* CPrestressLossTable::Build(IBroker* pBroker, const CSegmentKey& segmentKey,
   bool bRating,
   IEAFDisplayUnits* pDisplayUnits) const
{
   GET_IFACE2(pBroker, IPretensionForce, pPrestressForce);
   GET_IFACE2(pBroker, IPointOfInterest, pPoi);
   GET_IFACE2(pBroker, ILosses, pLosses);
   GET_IFACE2(pBroker, IStrandGeometry, pStrandGeom);
   GET_IFACE2(pBroker, IBridge, pBridge);
   GET_IFACE2(pBroker, IGirder, pGirder);

   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();
   bool bHasDeck = deckType == pgsTypes::sdtNone ? false : true;
   bool bHasJoint = pGirder->HasStructuralLongitudinalJoints();

   PoiList vPoi;
   pPoi->GetPointsOfInterest(segmentKey, POI_5L | POI_ERECTED_SEGMENT, &vPoi);
   ATLASSERT(vPoi.size() == 1);
   const pgsPointOfInterest& poi(vPoi.front());

   GET_IFACE2(pBroker, ISegmentData, pSegmentData);
   const CStrandData* pStrands = pSegmentData->GetStrandData(segmentKey);

   // Setup some unit-value prototypes
   INIT_UV_PROTOTYPE(rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), true);
   INIT_UV_PROTOTYPE(rptAreaUnitValue, area, pDisplayUnits->GetAreaUnit(), true);
   INIT_UV_PROTOTYPE(rptLengthUnitValue, len, pDisplayUnits->GetComponentDimUnit(), true);
   INIT_UV_PROTOTYPE(rptForceUnitValue, force, pDisplayUnits->GetGeneralForceUnit(), true);

   bool bTempStrands = (0 < pStrandGeom->GetMaxStrands(segmentKey, pgsTypes::Temporary) ? true : false);
   if (pStrandGeom->GetStrandCount(segmentKey, pgsTypes::Temporary) == 0)
   {
      bTempStrands = false;
   }

   ColumnIndexType nCol = 5;
   if (bTempStrands)
   {
      nCol += 4;
   }


   rptRcTable* p_table = rptStyleManager::CreateDefaultTable(nCol, _T("Effective Prestress at Mid-Span"));
   p_table->SetNumberOfHeaderRows(2);

   p_table->SetColumnStyle(0, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   p_table->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   //////////////////////////////////////////
   // Label columns
   //////////////////////////////////////////
   ColumnIndexType col = 0;
   p_table->SetRowSpan(0, col, 2);
   (*p_table)(0, col++) << _T("Loss Stage");

   p_table->SetColumnSpan(0, col, 4);
   (*p_table)(0, col) << _T("Permanent Strand");
   (*p_table)(1, col++) << COLHDR(_T("Effective") << rptNewLine << _T("Force"), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());
   (*p_table)(1, col++) << COLHDR(_T("Time-Dependent") << rptNewLine << _T("Effects"), rptStressUnitTag, pDisplayUnits->GetStressUnit());
   (*p_table)(1, col++) << COLHDR(_T("Instantaneous") << rptNewLine << _T("Effects"), rptStressUnitTag, pDisplayUnits->GetStressUnit());
   (*p_table)(1, col++) << COLHDR(RPT_FPE, rptStressUnitTag, pDisplayUnits->GetStressUnit());

   if (bTempStrands)
   {
      p_table->SetColumnSpan(0, col, 4);
      (*p_table)(0, col) << _T("Temporary Strand");
      (*p_table)(1, col++) << COLHDR(_T("Effective") << rptNewLine << _T("Force"), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());
      (*p_table)(1, col++) << COLHDR(_T("Time-Dependent") << rptNewLine << _T("Effects"), rptStressUnitTag, pDisplayUnits->GetStressUnit());
      (*p_table)(1, col++) << COLHDR(_T("Instantaneous") << rptNewLine << _T("Effects"), rptStressUnitTag, pDisplayUnits->GetStressUnit());
      (*p_table)(1, col++) << COLHDR(RPT_FPE, rptStressUnitTag, pDisplayUnits->GetStressUnit());
   }


   //////////////////////////////////////////
   // Label rows
   //////////////////////////////////////////
   RowIndexType row = p_table->GetNumberOfHeaderRows();

   (*p_table)(row++, 0) << _T("At Jacking");
   (*p_table)(row++, 0) << _T("Before Prestress Transfer");
   (*p_table)(row++, 0) << _T("After Prestress Transfer");

   if (bTempStrands && pStrands->GetTemporaryStrandUsage() == pgsTypes::ttsPTBeforeLifting)
   {
      (*p_table)(row++, 0) << _T("After Temporary Strand Installation");
   }

   (*p_table)(row++, 0) << _T("At Lifting");

   if (bTempStrands && (pStrands->GetTemporaryStrandUsage() == pgsTypes::ttsPTAfterLifting || pStrands->GetTemporaryStrandUsage() == pgsTypes::ttsPTBeforeShipping))
   {
      (*p_table)(row++, 0) << _T("After Temporary Strand Installation");
   }

   (*p_table)(row++, 0) << _T("At Shipping");
   (*p_table)(row++, 0) << _T("After Erection");
   if (bTempStrands)
   {
      (*p_table)(row++, 0) << _T("Before Temporary Strands Removal");
      (*p_table)(row++, 0) << _T("After Temporary Strands Removal");
   }

   if (bHasJoint)
   {
      (*p_table)(row++, 0) << _T("After Longitudinal Joint Placement");
   }

   if (bHasDeck)
   {
      (*p_table)(row++, 0) << _T("After Deck Placement");
   }

   (*p_table)(row++, 0) << _T("After Superimposed Dead Loads");
   (*p_table)(row++, 0) << _T("Final (permanent loads only)");

   GET_IFACE2_NOCHECK(pBroker, IProductLoads, pProductLoads); // only used if we are load rating, but don't want to get them in every "if(bRating)" code block
   GET_IFACE2_NOCHECK(pBroker, IRatingSpecification, pRatingSpec);
   if (bRating)
   {
      for (int i = 0; i < (int)(pgsTypes::lrLoadRatingTypeCount); i++)
      {
         pgsTypes::LoadRatingType ratingType = (pgsTypes::LoadRatingType)(i);
         if (pRatingSpec->IsRatingEnabled(ratingType) && pRatingSpec->RateForStress(ratingType))
         {
            std::vector<pgsTypes::LimitState> vLimitStates = GetRatingLimitStates(ratingType);
            for (const auto& limitState : vLimitStates)
            {
               if (IsStrengthLimitState(limitState))
               {
                  // we only care about service limit states here
                  continue;
               }

               pgsTypes::LiveLoadType llType = LiveLoadTypeFromLimitState(limitState);

               if (IsDesignRatingType(ratingType))
               {
                  (*p_table)(row++, 0) << _T("Final with Live Load (") << GetLimitStateString(limitState) << _T(")");
               }
               else
               {
                  VehicleIndexType nVehicles = pProductLoads->GetVehicleCount(llType);
                  for (VehicleIndexType vehicleIdx = 0; vehicleIdx < nVehicles; vehicleIdx++)
                  {
                     pgsTypes::LiveLoadApplicabilityType applicability = pProductLoads->GetLiveLoadApplicability(llType, vehicleIdx);
                     if (applicability == pgsTypes::llaNegMomentAndInteriorPierReaction)
                     {
                        continue;
                     }

                     std::_tstring name = pProductLoads->GetLiveLoadName(llType, vehicleIdx);
                     if (name == NO_LIVE_LOAD_DEFINED)
                     {
                        continue;
                     }

                     // Final with Live Load (Service III, Legal Routine, Type 3S2)....
                     (*p_table)(row++, 0) << _T("Final with Live Load (") << GetLimitStateString(limitState) << _T(", ") << name << _T(")");
                  }
               }
            }
         }
      }
   }
   else
   {
      (*p_table)(row++, 0) << _T("Final with Live Load (Service I)");
      (*p_table)(row++, 0) << _T("Final with Live Load (Service III)");
      if (lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims)
      {
         (*p_table)(row++, 0) << _T("Final with Live Load (Service IA)");
      }
      else
      {
         (*p_table)(row++, 0) << _T("Final with Live Load (Fatigue I)");
      }
   }

   // Fill up the table with data.

   // this is the last place we will be using these unit value prototypes.
   // turn off the unit tag.
   force.ShowUnitTag(false);
   stress.ShowUnitTag(false);

   GET_IFACE2(pBroker, IIntervals, pIntervals);
   IntervalIndexType stressStrandsIntervalIdx = pIntervals->GetStressStrandInterval(segmentKey);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType liftSegmentIntervalIdx = pIntervals->GetLiftSegmentInterval(segmentKey);
   IntervalIndexType haulSegmentIntervalIdx = pIntervals->GetHaulSegmentInterval(segmentKey);
   IntervalIndexType erectSegmentIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
   IntervalIndexType tsInstallIntervalIdx = pIntervals->GetTemporaryStrandInstallationInterval(segmentKey);
   IntervalIndexType tsRemovalIntervalIdx = pIntervals->GetTemporaryStrandRemovalInterval(segmentKey);
   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval();
   IntervalIndexType castJointIntervalIdx = pIntervals->GetCastLongitudinalJointInterval();
   IntervalIndexType railingSystemIntervalIdx = pIntervals->GetInstallRailingSystemInterval();
   IntervalIndexType lastIntervalIdx = pIntervals->GetIntervalCount() - 1;

   ///////////////////////////////////
   // Permanent Strand Force Column
   row = p_table->GetNumberOfHeaderRows();
   col = 1;
   (*p_table)(row++, col) << force.SetValue(pPrestressForce->GetPrestressForce(poi, pgsTypes::Permanent, stressStrandsIntervalIdx, pgsTypes::Start/*pgsTypes::Jacking*/));
   (*p_table)(row++, col) << force.SetValue(pPrestressForce->GetPrestressForce(poi, pgsTypes::Permanent, releaseIntervalIdx, pgsTypes::Start/*pgsTypes::BeforeXfer*/));
   (*p_table)(row++, col) << force.SetValue(pPrestressForce->GetPrestressForce(poi, pgsTypes::Permanent, releaseIntervalIdx, pgsTypes::End/*pgsTypes::AfterXfer*/));

   if (bTempStrands && pStrands->GetTemporaryStrandUsage() == pgsTypes::ttsPTBeforeLifting)
   {
      (*p_table)(row++, col) << force.SetValue(pPrestressForce->GetPrestressForce(poi, pgsTypes::Permanent, tsInstallIntervalIdx, pgsTypes::End/*pgsTypes::AfterTemporaryStrandInstallation*/));
   }

   (*p_table)(row++, col) << force.SetValue(pPrestressForce->GetPrestressForce(poi, pgsTypes::Permanent, liftSegmentIntervalIdx, pgsTypes::End/*pgsTypes::AtLifting*/));

   if (bTempStrands && (pStrands->GetTemporaryStrandUsage() == pgsTypes::ttsPTAfterLifting || pStrands->GetTemporaryStrandUsage() == pgsTypes::ttsPTBeforeShipping))
   {
      (*p_table)(row++, col) << force.SetValue(pPrestressForce->GetPrestressForce(poi, pgsTypes::Permanent, tsInstallIntervalIdx, pgsTypes::End/*pgsTypes::AfterTemporaryStrandInstallation*/));
   }

   (*p_table)(row++, col) << force.SetValue(pPrestressForce->GetPrestressForce(poi, pgsTypes::Permanent, haulSegmentIntervalIdx, pgsTypes::End/*pgsTypes::AtShipping*/));

   (*p_table)(row++, col) << force.SetValue(pPrestressForce->GetPrestressForce(poi, pgsTypes::Permanent, erectSegmentIntervalIdx, pgsTypes::End/*pgsTypes::Erection*/));
   if (bTempStrands)
   {
      (*p_table)(row++, col) << force.SetValue(pPrestressForce->GetPrestressForce(poi, pgsTypes::Permanent, tsRemovalIntervalIdx, pgsTypes::Start/*pgsTypes::BeforeTemporaryStrandRemoval*/));
      (*p_table)(row++, col) << force.SetValue(pPrestressForce->GetPrestressForce(poi, pgsTypes::Permanent, tsRemovalIntervalIdx, pgsTypes::End/*pgsTypes::AfterTemporaryStrandRemoval*/));
   }

   if (bHasJoint)
   {
      (*p_table)(row++, col) << force.SetValue(pPrestressForce->GetPrestressForce(poi, pgsTypes::Permanent, castJointIntervalIdx, pgsTypes::End));
   }

   if (bHasDeck)
   {
      (*p_table)(row++, col) << force.SetValue(pPrestressForce->GetPrestressForce(poi, pgsTypes::Permanent, castDeckIntervalIdx, pgsTypes::End/*pgsTypes::AfterDeckPlacement*/));
   }

   (*p_table)(row++, col) << force.SetValue(pPrestressForce->GetPrestressForce(poi, pgsTypes::Permanent, railingSystemIntervalIdx, pgsTypes::End/*pgsTypes::AfterSIDL*/));
   (*p_table)(row++, col) << force.SetValue(pPrestressForce->GetPrestressForce(poi, pgsTypes::Permanent, lastIntervalIdx, pgsTypes::End/*pgsTypes::AfterLosses*/));

   if (bRating)
   {
      for (int i = 0; i < (int)(pgsTypes::lrLoadRatingTypeCount); i++)
      {
         pgsTypes::LoadRatingType ratingType = (pgsTypes::LoadRatingType)(i);

         if (pRatingSpec->IsRatingEnabled(ratingType) && pRatingSpec->RateForStress(ratingType))
         {
            std::vector<pgsTypes::LimitState> vLimitStates = GetRatingLimitStates(ratingType);
            for (const auto& limitState : vLimitStates)
            {
               if (IsStrengthLimitState(limitState))
               {
                  // we only care about service limit states here
                  continue;
               }

               pgsTypes::LiveLoadType llType = LiveLoadTypeFromLimitState(limitState);
               if (IsDesignRatingType(ratingType))
               {
                  (*p_table)(row++, col) << force.SetValue(pPrestressForce->GetPrestressForceWithLiveLoad(poi,pgsTypes::Permanent, limitState));
               }
               else
               {
                  VehicleIndexType nVehicles = pProductLoads->GetVehicleCount(llType);
                  for ( VehicleIndexType vehicleIdx = 0; vehicleIdx < nVehicles; vehicleIdx++)
                  {
                     pgsTypes::LiveLoadApplicabilityType applicability = pProductLoads->GetLiveLoadApplicability(llType, vehicleIdx);
                     if (applicability == pgsTypes::llaNegMomentAndInteriorPierReaction)
                     {
                        continue;
                     }
                     std::_tstring name = pProductLoads->GetLiveLoadName(llType, vehicleIdx);
                     if (name == NO_LIVE_LOAD_DEFINED)
                     {
                        continue;
                     }
                     (*p_table)(row++, col) << force.SetValue(pPrestressForce->GetPrestressForceWithLiveLoad(poi,pgsTypes::Permanent, limitState,vehicleIdx));
                  }
               }
            }
         }
      }
   }
   else
   {
      (*p_table)(row++, col) << force.SetValue(pPrestressForce->GetPrestressForceWithLiveLoad(poi, pgsTypes::Permanent/*pgsTypes::AfterLossesWithLiveLoad*/, pgsTypes::ServiceI));
      (*p_table)(row++, col) << force.SetValue(pPrestressForce->GetPrestressForceWithLiveLoad(poi, pgsTypes::Permanent/*pgsTypes::AfterLossesWithLiveLoad*/, pgsTypes::ServiceIII));
      if (lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims)
      {
         (*p_table)(row++, col) << force.SetValue(pPrestressForce->GetPrestressForceWithLiveLoad(poi, pgsTypes::Permanent/*pgsTypes::AfterLossesWithLiveLoad*/, pgsTypes::ServiceIA));
      }
      else
      {
         (*p_table)(row++, col) << force.SetValue(pPrestressForce->GetPrestressForceWithLiveLoad(poi, pgsTypes::Permanent/*pgsTypes::AfterLossesWithLiveLoad*/, pgsTypes::FatigueI));
      }
   }

   ///////////////////////////////////
   // Permanent Strand Loss Column
   row = p_table->GetNumberOfHeaderRows();
   col++;
   (*p_table)(row++, col) << stress.SetValue(pLosses->GetTimeDependentLosses(poi, pgsTypes::Permanent, stressStrandsIntervalIdx, pgsTypes::Start)); // at jacking
   (*p_table)(row++, col) << stress.SetValue(pLosses->GetTimeDependentLosses(poi, pgsTypes::Permanent, releaseIntervalIdx, pgsTypes::Start)/*pLosses->GetBeforeXferLosses(poi,pgsTypes::Permanent)*/);
   (*p_table)(row++, col) << stress.SetValue(pLosses->GetTimeDependentLosses(poi, pgsTypes::Permanent, releaseIntervalIdx, pgsTypes::End)/*pLosses->GetAfterXferLosses(poi,pgsTypes::Permanent)*/);
   if (bTempStrands && pStrands->GetTemporaryStrandUsage() == pgsTypes::ttsPTBeforeLifting)
   {
      (*p_table)(row++, col) << stress.SetValue(pLosses->GetTimeDependentLosses(poi, pgsTypes::Permanent, tsInstallIntervalIdx, pgsTypes::End)/*pLosses->GetAfterTemporaryStrandInstallationLosses(poi,pgsTypes::Permanent)*/);
   }

   (*p_table)(row++, col) << stress.SetValue(pLosses->GetTimeDependentLosses(poi, pgsTypes::Permanent, liftSegmentIntervalIdx, pgsTypes::End)/*pLosses->GetLiftingLosses(poi,pgsTypes::Permanent)*/);

   if (bTempStrands && (pStrands->GetTemporaryStrandUsage() == pgsTypes::ttsPTAfterLifting || pStrands->GetTemporaryStrandUsage() == pgsTypes::ttsPTBeforeShipping))
   {
      (*p_table)(row++, col) << stress.SetValue(pLosses->GetTimeDependentLosses(poi, pgsTypes::Permanent, tsInstallIntervalIdx, pgsTypes::End)/*pLosses->GetAfterTemporaryStrandInstallationLosses(poi,pgsTypes::Permanent)*/);
   }

   (*p_table)(row++, col) << stress.SetValue(pLosses->GetTimeDependentLosses(poi, pgsTypes::Permanent, haulSegmentIntervalIdx, pgsTypes::End)/*pLosses->GetShippingLosses(poi,pgsTypes::Permanent)*/);

   (*p_table)(row++, col) << stress.SetValue(pLosses->GetTimeDependentLosses(poi, pgsTypes::Permanent, erectSegmentIntervalIdx, pgsTypes::End));
   if (bTempStrands)
   {
      (*p_table)(row++, col) << stress.SetValue(pLosses->GetTimeDependentLosses(poi, pgsTypes::Permanent, tsRemovalIntervalIdx, pgsTypes::Start)/*pLosses->GetBeforeTemporaryStrandRemovalLosses(poi,pgsTypes::Permanent)*/);
      (*p_table)(row++, col) << stress.SetValue(pLosses->GetTimeDependentLosses(poi, pgsTypes::Permanent, tsRemovalIntervalIdx, pgsTypes::End)/*pLosses->GetAfterTemporaryStrandRemovalLosses(poi,pgsTypes::Permanent)*/);
   }

   if (bHasDeck)
   {
      (*p_table)(row++, col) << stress.SetValue(pLosses->GetTimeDependentLosses(poi, pgsTypes::Permanent, castDeckIntervalIdx, pgsTypes::End)/*pLosses->GetDeckPlacementLosses(poi,pgsTypes::Permanent)*/);
   }

   if (bHasJoint)
   {
      (*p_table)(row++, col) << stress.SetValue(pLosses->GetTimeDependentLosses(poi, pgsTypes::Permanent, castJointIntervalIdx, pgsTypes::End));
   }

   (*p_table)(row++, col) << stress.SetValue(pLosses->GetTimeDependentLosses(poi, pgsTypes::Permanent, railingSystemIntervalIdx, pgsTypes::End)/*pLosses->GetSIDLLosses(poi,pgsTypes::Permanent)*/);
   (*p_table)(row++, col) << stress.SetValue(pLosses->GetTimeDependentLosses(poi, pgsTypes::Permanent, lastIntervalIdx, pgsTypes::End)/*pLosses->GetFinal(poi,pgsTypes::Permanent)*/); //

   if (bRating)
   {
      for (int i = 0; i < (int)(pgsTypes::lrLoadRatingTypeCount); i++)
      {
         pgsTypes::LoadRatingType ratingType = (pgsTypes::LoadRatingType)(i);

         if (pRatingSpec->IsRatingEnabled(ratingType) && pRatingSpec->RateForStress(ratingType))
         {
            std::vector<pgsTypes::LimitState> vLimitStates = GetRatingLimitStates(ratingType);
            for (const auto& limitState : vLimitStates)
            {
               if (IsStrengthLimitState(limitState))
               {
                  // we only care about service limit states here
                  continue;
               }

               pgsTypes::LiveLoadType llType = LiveLoadTypeFromLimitState(limitState);
               if (IsDesignRatingType(ratingType))
               {
                  (*p_table)(row++, col) << stress.SetValue(pLosses->GetTimeDependentLosses(poi, pgsTypes::Permanent, lastIntervalIdx, pgsTypes::End));
               }
               else
               {
                  VehicleIndexType nVehicles = pProductLoads->GetVehicleCount(llType);
                  for (VehicleIndexType vehicleIdx = 0; vehicleIdx < nVehicles; vehicleIdx++)
                  {
                     pgsTypes::LiveLoadApplicabilityType applicability = pProductLoads->GetLiveLoadApplicability(llType, vehicleIdx);
                     if (applicability == pgsTypes::llaNegMomentAndInteriorPierReaction)
                     {
                        continue;
                     }
                     std::_tstring name = pProductLoads->GetLiveLoadName(llType, vehicleIdx);
                     if (name == NO_LIVE_LOAD_DEFINED)
                     {
                        continue;
                     }
                     (*p_table)(row++, col) << stress.SetValue(pLosses->GetTimeDependentLosses(poi, pgsTypes::Permanent, lastIntervalIdx, pgsTypes::End));
                  }
               }
            }
         }
      }
   }
   else
   {
      (*p_table)(row++, col) << stress.SetValue(pLosses->GetTimeDependentLosses(poi, pgsTypes::Permanent, lastIntervalIdx, pgsTypes::End)/*pLosses->GetFinal(poi,pgsTypes::Permanent)*/); // Service I
      (*p_table)(row++, col) << stress.SetValue(pLosses->GetTimeDependentLosses(poi, pgsTypes::Permanent, lastIntervalIdx, pgsTypes::End)/*pLosses->GetFinal(poi,pgsTypes::Permanent)*/); // Service III
      (*p_table)(row++, col) << stress.SetValue(pLosses->GetTimeDependentLosses(poi, pgsTypes::Permanent, lastIntervalIdx, pgsTypes::End)/*pLosses->GetFinal(poi,pgsTypes::Permanent)*/); // Fatigue I/Service IA
   }

   ///////////////////////////////////
   // Permanent Strand Gain Column
   // Use a negative of the elastic effects so it will be a gain
   row = p_table->GetNumberOfHeaderRows();
   col++;
   (*p_table)(row++, col) << stress.SetValue(-pLosses->GetInstantaneousEffects(poi, pgsTypes::Permanent, stressStrandsIntervalIdx, pgsTypes::Start)); // at jacking
   (*p_table)(row++, col) << stress.SetValue(-pLosses->GetInstantaneousEffects(poi, pgsTypes::Permanent, releaseIntervalIdx, pgsTypes::Start)/*pLosses->GetBeforeXferLosses(poi,pgsTypes::Permanent)*/);
   (*p_table)(row++, col) << stress.SetValue(-pLosses->GetInstantaneousEffects(poi, pgsTypes::Permanent, releaseIntervalIdx, pgsTypes::End)/*pLosses->GetAfterXferLosses(poi,pgsTypes::Permanent)*/);
   if (bTempStrands && pStrands->GetTemporaryStrandUsage() == pgsTypes::ttsPTBeforeLifting)
   {
      (*p_table)(row++, col) << stress.SetValue(-pLosses->GetInstantaneousEffects(poi, pgsTypes::Permanent, tsInstallIntervalIdx, pgsTypes::End)/*pLosses->GetAfterTemporaryStrandInstallationLosses(poi,pgsTypes::Permanent)*/);
   }

   (*p_table)(row++, col) << stress.SetValue(-pLosses->GetInstantaneousEffects(poi, pgsTypes::Permanent, liftSegmentIntervalIdx, pgsTypes::End)/*pLosses->GetLiftingLosses(poi,pgsTypes::Permanent)*/);

   if (bTempStrands && (pStrands->GetTemporaryStrandUsage() == pgsTypes::ttsPTAfterLifting || pStrands->GetTemporaryStrandUsage() == pgsTypes::ttsPTBeforeShipping))
   {
      (*p_table)(row++, col) << stress.SetValue(-pLosses->GetInstantaneousEffects(poi, pgsTypes::Permanent, tsInstallIntervalIdx, pgsTypes::End)/*pLosses->GetAfterTemporaryStrandInstallationLosses(poi,pgsTypes::Permanent)*/);
   }

   (*p_table)(row++, col) << stress.SetValue(-pLosses->GetInstantaneousEffects(poi, pgsTypes::Permanent, haulSegmentIntervalIdx, pgsTypes::End)/*pLosses->GetShippingLosses(poi,pgsTypes::Permanent)*/);

   (*p_table)(row++, col) << stress.SetValue(-pLosses->GetInstantaneousEffects(poi, pgsTypes::Permanent, erectSegmentIntervalIdx, pgsTypes::End));
   if (bTempStrands)
   {
      (*p_table)(row++, col) << stress.SetValue(-pLosses->GetInstantaneousEffects(poi, pgsTypes::Permanent, tsRemovalIntervalIdx, pgsTypes::Start)/*pLosses->GetBeforeTemporaryStrandRemovalLosses(poi,pgsTypes::Permanent)*/);
      (*p_table)(row++, col) << stress.SetValue(-pLosses->GetInstantaneousEffects(poi, pgsTypes::Permanent, tsRemovalIntervalIdx, pgsTypes::End)/*pLosses->GetAfterTemporaryStrandRemovalLosses(poi,pgsTypes::Permanent)*/);
   }

   if (bHasDeck)
   {
      (*p_table)(row++, col) << stress.SetValue(-pLosses->GetInstantaneousEffects(poi, pgsTypes::Permanent, castDeckIntervalIdx, pgsTypes::End)/*pLosses->GetDeckPlacementLosses(poi,pgsTypes::Permanent)*/);
   }

   if (bHasJoint)
   {
      (*p_table)(row++, col) << stress.SetValue(-pLosses->GetInstantaneousEffects(poi, pgsTypes::Permanent, castJointIntervalIdx, pgsTypes::End)/*pLosses->GetDeckPlacementLosses(poi,pgsTypes::Permanent)*/);
   }

   (*p_table)(row++, col) << stress.SetValue(-pLosses->GetInstantaneousEffects(poi, pgsTypes::Permanent, railingSystemIntervalIdx, pgsTypes::End)/*pLosses->GetSIDLLosses(poi,pgsTypes::Permanent)*/);
   (*p_table)(row++, col) << stress.SetValue(-pLosses->GetInstantaneousEffects(poi, pgsTypes::Permanent, lastIntervalIdx, pgsTypes::End)/*pLosses->GetFinal(poi,pgsTypes::Permanent)*/);

   if (bRating)
   {
      for (int i = 0; i < (int)(pgsTypes::lrLoadRatingTypeCount); i++)
      {
         pgsTypes::LoadRatingType ratingType = (pgsTypes::LoadRatingType)(i);

         if (pRatingSpec->IsRatingEnabled(ratingType) && pRatingSpec->RateForStress(ratingType))
         {
            std::vector<pgsTypes::LimitState> vLimitStates = GetRatingLimitStates(ratingType);
            for (const auto& limitState : vLimitStates)
            {
               if (IsStrengthLimitState(limitState))
               {
                  // we only care about service limit states here
                  continue;
               }

               pgsTypes::LiveLoadType llType = LiveLoadTypeFromLimitState(limitState);
               if (IsDesignRatingType(ratingType))
               {
                  (*p_table)(row++, col) << stress.SetValue(-pLosses->GetInstantaneousEffectsWithLiveLoad(poi, pgsTypes::Permanent, limitState));
               }
               else
               {
                  VehicleIndexType nVehicles = pProductLoads->GetVehicleCount(llType);
                  for (VehicleIndexType vehicleIdx = 0; vehicleIdx < nVehicles; vehicleIdx++)
                  {
                     pgsTypes::LiveLoadApplicabilityType applicability = pProductLoads->GetLiveLoadApplicability(llType, vehicleIdx);
                     if (applicability == pgsTypes::llaNegMomentAndInteriorPierReaction)
                     {
                        continue;
                     }
                     std::_tstring name = pProductLoads->GetLiveLoadName(llType, vehicleIdx);
                     if (name == NO_LIVE_LOAD_DEFINED)
                     {
                        continue;
                     }
                     (*p_table)(row++, col) << stress.SetValue(-pLosses->GetInstantaneousEffectsWithLiveLoad(poi, pgsTypes::Permanent, limitState, vehicleIdx));
                  }
               }
            }
         }
      }
   }
   else
   {
      (*p_table)(row++, col) << stress.SetValue(-pLosses->GetInstantaneousEffectsWithLiveLoad(poi, pgsTypes::Permanent, pgsTypes::ServiceI)/*pLosses->GetFinalWithLiveLoad(poi,pgsTypes::Permanent)*/);
      (*p_table)(row++, col) << stress.SetValue(-pLosses->GetInstantaneousEffectsWithLiveLoad(poi, pgsTypes::Permanent, pgsTypes::ServiceIII)/*pLosses->GetFinalWithLiveLoad(poi,pgsTypes::Permanent)*/);
      if (lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims)
      {
         (*p_table)(row++, col) << stress.SetValue(-pLosses->GetInstantaneousEffectsWithLiveLoad(poi, pgsTypes::Permanent, pgsTypes::ServiceIA)/*pLosses->GetFinalWithLiveLoad(poi,pgsTypes::Permanent)*/);
      }
      else
      {
         (*p_table)(row++, col) << stress.SetValue(-pLosses->GetInstantaneousEffectsWithLiveLoad(poi, pgsTypes::Permanent, pgsTypes::FatigueI)/*pLosses->GetFinalWithLiveLoad(poi,pgsTypes::Permanent)*/);
      }
   }

   ///////////////////////////////////
   // Permanent Strand Stress Column
   row = p_table->GetNumberOfHeaderRows();
   col++;
   (*p_table)(row++, col) << stress.SetValue(pPrestressForce->GetEffectivePrestress(poi, pgsTypes::Permanent, stressStrandsIntervalIdx, pgsTypes::Start));
   (*p_table)(row++, col) << stress.SetValue(pPrestressForce->GetEffectivePrestress(poi, pgsTypes::Permanent, releaseIntervalIdx, pgsTypes::Start));
   (*p_table)(row++, col) << stress.SetValue(pPrestressForce->GetEffectivePrestress(poi, pgsTypes::Permanent, releaseIntervalIdx, pgsTypes::End));

   if (bTempStrands && pStrands->GetTemporaryStrandUsage() == pgsTypes::ttsPTBeforeLifting)
   {
      (*p_table)(row++, col) << stress.SetValue(pPrestressForce->GetEffectivePrestress(poi, pgsTypes::Permanent, tsInstallIntervalIdx, pgsTypes::End));
   }

   (*p_table)(row++, col) << stress.SetValue(pPrestressForce->GetEffectivePrestress(poi, pgsTypes::Permanent, liftSegmentIntervalIdx, pgsTypes::End));

   if (bTempStrands && (pStrands->GetTemporaryStrandUsage() == pgsTypes::ttsPTAfterLifting || pStrands->GetTemporaryStrandUsage() == pgsTypes::ttsPTBeforeShipping))
   {
      (*p_table)(row++, col) << stress.SetValue(pPrestressForce->GetEffectivePrestress(poi, pgsTypes::Permanent, tsInstallIntervalIdx, pgsTypes::End));
   }

   (*p_table)(row++, col) << stress.SetValue(pPrestressForce->GetEffectivePrestress(poi, pgsTypes::Permanent, haulSegmentIntervalIdx, pgsTypes::End));

   (*p_table)(row++, col) << stress.SetValue(pPrestressForce->GetEffectivePrestress(poi, pgsTypes::Permanent, erectSegmentIntervalIdx, pgsTypes::End));
   if (bTempStrands)
   {
      (*p_table)(row++, col) << stress.SetValue(pPrestressForce->GetEffectivePrestress(poi, pgsTypes::Permanent, tsRemovalIntervalIdx, pgsTypes::Start/*pgsTypes::BeforeTemporaryStrandRemoval*/));
      (*p_table)(row++, col) << stress.SetValue(pPrestressForce->GetEffectivePrestress(poi, pgsTypes::Permanent, tsRemovalIntervalIdx, pgsTypes::End/*pgsTypes::AfterTemporaryStrandRemoval*/));
   }

   if (bHasDeck)
   {
      (*p_table)(row++, col) << stress.SetValue(pPrestressForce->GetEffectivePrestress(poi, pgsTypes::Permanent, castDeckIntervalIdx, pgsTypes::End));
   }

   if (bHasJoint)
   {
      (*p_table)(row++, col) << stress.SetValue(pPrestressForce->GetEffectivePrestress(poi, pgsTypes::Permanent, castJointIntervalIdx, pgsTypes::End));
   }

   (*p_table)(row++, col) << stress.SetValue(pPrestressForce->GetEffectivePrestress(poi, pgsTypes::Permanent, railingSystemIntervalIdx, pgsTypes::End));
   (*p_table)(row++, col) << stress.SetValue(pPrestressForce->GetEffectivePrestress(poi, pgsTypes::Permanent, lastIntervalIdx, pgsTypes::End/*pgsTypes::AfterLosses*/));

   if (bRating)
   {
      for (int i = 0; i < (int)(pgsTypes::lrLoadRatingTypeCount); i++)
      {
         pgsTypes::LoadRatingType ratingType = (pgsTypes::LoadRatingType)(i);

         if (pRatingSpec->IsRatingEnabled(ratingType) && pRatingSpec->RateForStress(ratingType))
         {
            std::vector<pgsTypes::LimitState> vLimitStates = GetRatingLimitStates(ratingType);
            for (const auto& limitState : vLimitStates)
            {
               if (IsStrengthLimitState(limitState))
               {
                  // we only care about service limit states here
                  continue;
               }

               pgsTypes::LiveLoadType llType = LiveLoadTypeFromLimitState(limitState);
               if (IsDesignRatingType(ratingType))
               {
                  (*p_table)(row++, col) << stress.SetValue(pPrestressForce->GetEffectivePrestressWithLiveLoad(poi, pgsTypes::Permanent, limitState));
               }
               else
               {
                  VehicleIndexType nVehicles = pProductLoads->GetVehicleCount(llType);
                  for (VehicleIndexType vehicleIdx = 0; vehicleIdx < nVehicles; vehicleIdx++)
                  {
                     pgsTypes::LiveLoadApplicabilityType applicability = pProductLoads->GetLiveLoadApplicability(llType, vehicleIdx);
                     if (applicability == pgsTypes::llaNegMomentAndInteriorPierReaction)
                     {
                        continue;
                     }
                     std::_tstring name = pProductLoads->GetLiveLoadName(llType, vehicleIdx);
                     if (name == NO_LIVE_LOAD_DEFINED)
                     {
                        continue;
                     }
                     (*p_table)(row++, col) << stress.SetValue(pPrestressForce->GetEffectivePrestressWithLiveLoad(poi, pgsTypes::Permanent,limitState,vehicleIdx));
                  }
               }
            }
         }
      }
   }
   else
   {
      (*p_table)(row++, col) << stress.SetValue(pPrestressForce->GetEffectivePrestressWithLiveLoad(poi, pgsTypes::Permanent/*pgsTypes::AfterLossesWithLiveLoad*/, pgsTypes::ServiceI));
      (*p_table)(row++, col) << stress.SetValue(pPrestressForce->GetEffectivePrestressWithLiveLoad(poi, pgsTypes::Permanent/*pgsTypes::AfterLossesWithLiveLoad*/, pgsTypes::ServiceIII));
      if (lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims)
      {
         (*p_table)(row++, col) << stress.SetValue(pPrestressForce->GetEffectivePrestressWithLiveLoad(poi, pgsTypes::Permanent/*pgsTypes::AfterLossesWithLiveLoad*/, pgsTypes::ServiceIA));
      }
      else
      {
         (*p_table)(row++, col) << stress.SetValue(pPrestressForce->GetEffectivePrestressWithLiveLoad(poi, pgsTypes::Permanent/*pgsTypes::AfterLossesWithLiveLoad*/, pgsTypes::FatigueI));
      }
   }

   if ( bTempStrands )
   {
      ///////////////////////////////////
      // Temporary Strand Force Column
      row = p_table->GetNumberOfHeaderRows();
      col++;
      if ( pStrands->GetTemporaryStrandUsage() == pgsTypes::ttsPretensioned )
      {
         (*p_table)(row++,col) << force.SetValue( pPrestressForce->GetPrestressForce(poi,pgsTypes::Temporary,stressStrandsIntervalIdx,pgsTypes::Start/*pgsTypes::Jacking*/) );
         (*p_table)(row++,col) << force.SetValue( pPrestressForce->GetPrestressForce(poi,pgsTypes::Temporary,releaseIntervalIdx,pgsTypes::Start/*pgsTypes::BeforeXfer*/) );
         (*p_table)(row++,col) << force.SetValue( pPrestressForce->GetPrestressForce(poi,pgsTypes::Temporary,releaseIntervalIdx,pgsTypes::End/*pgsTypes::AfterXfer*/) );
      }
      else
      {
         (*p_table)(row++,col) << _T(""); //force.SetValue( pPrestressForce->GetPrestressForce(poi,pgsTypes::Temporary,pgsTypes::Jacking) );
         (*p_table)(row++,col) << _T(""); //force.SetValue( pPrestressForce->GetPrestressForce(poi,pgsTypes::Temporary,pgsTypes::BeforeXfer) );
         (*p_table)(row++,col) << _T(""); //force.SetValue( pPrestressForce->GetPrestressForce(poi,pgsTypes::Temporary,pgsTypes::AfterXfer) );
      }
      
      if ( pStrands->GetTemporaryStrandUsage() == pgsTypes::ttsPTBeforeLifting )
      {
         (*p_table)(row++,col) << force.SetValue( pPrestressForce->GetPrestressForce(poi,pgsTypes::Temporary,tsInstallIntervalIdx,pgsTypes::End/*pgsTypes::AfterTemporaryStrandInstallation*/) );
      }

      if ( pStrands->GetTemporaryStrandUsage() == pgsTypes::ttsPTAfterLifting || pStrands->GetTemporaryStrandUsage() == pgsTypes::ttsPTBeforeShipping )
      {
         (*p_table)(row++,col) << _T("");
      }
      else
      {
         (*p_table)(row++,col) << force.SetValue( pPrestressForce->GetPrestressForce(poi,pgsTypes::Temporary,liftSegmentIntervalIdx,pgsTypes::End/*pgsTypes::AtLifting*/) );
      }

      if ( pStrands->GetTemporaryStrandUsage() == pgsTypes::ttsPTAfterLifting || pStrands->GetTemporaryStrandUsage() == pgsTypes::ttsPTBeforeShipping )
      {
         (*p_table)(row++,col) << force.SetValue( pPrestressForce->GetPrestressForce(poi,pgsTypes::Temporary,tsInstallIntervalIdx,pgsTypes::End/*pgsTypes::AfterTemporaryStrandInstallation*/) );
      }
   
      (*p_table)(row++,col) << force.SetValue( pPrestressForce->GetPrestressForce(poi,pgsTypes::Temporary,haulSegmentIntervalIdx,pgsTypes::End/*pgsTypes::AtShipping*/) );
      (*p_table)(row++,col) << force.SetValue( pPrestressForce->GetPrestressForce(poi,pgsTypes::Temporary,erectSegmentIntervalIdx,pgsTypes::End) );
      (*p_table)(row++,col) << force.SetValue( pPrestressForce->GetPrestressForce(poi,pgsTypes::Temporary,tsRemovalIntervalIdx,pgsTypes::Start/*pgsTypes::BeforeTemporaryStrandRemoval*/) );
      (*p_table)(row++,col) << _T("");//force.SetValue( pPrestressForce->GetPrestressForce(poi,pgsTypes::Temporary,pgsTypes::AfterTemporaryStrandRemoval) );

      if (bHasDeck)
      {
         (*p_table)(row++, col) << _T("");//force.SetValue( pPrestressForce->GetPrestressForce(poi,pgsTypes::Temporary,pgsTypes::AfterDeckPlacement) );
      }

      if (bHasJoint)
      {
         (*p_table)(row++, col) << _T("");//force.SetValue( pPrestressForce->GetPrestressForce(poi,pgsTypes::Temporary,pgsTypes::AfterJointPlacement) );
      }

      (*p_table)(row++,col) << _T("");//force.SetValue( pPrestressForce->GetPrestressForce(poi,pgsTypes::Temporary,pgsTypes::AfterSIDL) );
      (*p_table)(row++,col) << _T("");//force.SetValue( pPrestressForce->GetPrestressForce(poi,pgsTypes::Temporary,pgsTypes::Final) );

      if (bRating)
      {
         for (int i = 0; i < (int)(pgsTypes::lrLoadRatingTypeCount); i++)
         {
            pgsTypes::LoadRatingType ratingType = (pgsTypes::LoadRatingType)(i);

            if (pRatingSpec->IsRatingEnabled(ratingType) && pRatingSpec->RateForStress(ratingType))
            {
               std::vector<pgsTypes::LimitState> vLimitStates = GetRatingLimitStates(ratingType);
               for (const auto& limitState : vLimitStates)
               {
                  if (IsStrengthLimitState(limitState))
                  {
                     // we only care about service limit states here
                     continue;
                  }

                  pgsTypes::LiveLoadType llType = LiveLoadTypeFromLimitState(limitState);
                  if (IsDesignRatingType(ratingType))
                  {
                     (*p_table)(row++, col) << _T("");
                  }
                  else
                  {
                     VehicleIndexType nVehicles = pProductLoads->GetVehicleCount(llType);
                     for (VehicleIndexType vehicleIdx = 0; vehicleIdx < nVehicles; vehicleIdx++)
                     {
                        pgsTypes::LiveLoadApplicabilityType applicability = pProductLoads->GetLiveLoadApplicability(llType, vehicleIdx);
                        if (applicability == pgsTypes::llaNegMomentAndInteriorPierReaction)
                        {
                           continue;
                        }
                        std::_tstring name = pProductLoads->GetLiveLoadName(llType, vehicleIdx);
                        if (name == NO_LIVE_LOAD_DEFINED)
                        {
                           continue;
                        }
                        (*p_table)(row++, col) << _T("");
                     }
                  }
               }
            }
         }
      }
      else
      {
         (*p_table)(row++, col) << _T("");//force.SetValue( pPrestressForce->GetPrestressForce(poi,pgsTypes::Temporary,pgsTypes::AfterLossesWithLiveLoad) ); // Service I
         (*p_table)(row++, col) << _T("");//force.SetValue( pPrestressForce->GetPrestressForce(poi,pgsTypes::Temporary,pgsTypes::AfterLossesWithLiveLoad) ); // Service III
         (*p_table)(row++, col) << _T("");//force.SetValue( pPrestressForce->GetPrestressForce(poi,pgsTypes::Temporary,pgsTypes::AfterLossesWithLiveLoad) ); // Service IA/Fatigue I
      }

      ///////////////////////////////////
      // Temporary Strand Loss Column
      row = p_table->GetNumberOfHeaderRows();
      col++;

      if ( pStrands->GetTemporaryStrandUsage() == pgsTypes::ttsPretensioned )
      {
         (*p_table)(row++,col) << stress.SetValue( pLosses->GetTimeDependentLosses(poi,pgsTypes::Temporary,stressStrandsIntervalIdx,pgsTypes::Start) );
         (*p_table)(row++,col) << stress.SetValue( pLosses->GetTimeDependentLosses(poi,pgsTypes::Temporary,releaseIntervalIdx,pgsTypes::Start)/*pLosses->GetBeforeXferLosses(poi,pgsTypes::Temporary)*/ );
         (*p_table)(row++,col) << stress.SetValue( pLosses->GetTimeDependentLosses(poi,pgsTypes::Temporary,releaseIntervalIdx,pgsTypes::End)/*pLosses->GetAfterXferLosses(poi,pgsTypes::Temporary)*/ );
      }
      else
      {
         (*p_table)(row++,col) << _T(""); //stress.SetValue( 0.0 );
         (*p_table)(row++,col) << _T(""); //stress.SetValue( pLosses->GetBeforeXferLosses(poi,pgsTypes::Temporary) );
         (*p_table)(row++,col) << _T(""); //stress.SetValue( pLosses->GetAfterXferLosses(poi,pgsTypes::Temporary) );
      }

      if ( pStrands->GetTemporaryStrandUsage() == pgsTypes::ttsPTBeforeLifting )
      {
         (*p_table)(row++,col) << stress.SetValue(  pLosses->GetTimeDependentLosses(poi,pgsTypes::Temporary,tsInstallIntervalIdx,pgsTypes::End)/*pLosses->GetAfterTemporaryStrandInstallationLosses(poi,pgsTypes::Temporary)*/ );
      }
   
      if ( pStrands->GetTemporaryStrandUsage() == pgsTypes::ttsPTAfterLifting || pStrands->GetTemporaryStrandUsage() == pgsTypes::ttsPTBeforeShipping )
      {
         (*p_table)(row++,col) << _T("");
      }
      else
      {
         (*p_table)(row++,col) << stress.SetValue(  pLosses->GetTimeDependentLosses(poi,pgsTypes::Temporary,liftSegmentIntervalIdx,pgsTypes::End)/*pLosses->GetLiftingLosses(poi,pgsTypes::Temporary)*/ );
      }

      if ( pStrands->GetTemporaryStrandUsage() == pgsTypes::ttsPTAfterLifting || pStrands->GetTemporaryStrandUsage() == pgsTypes::ttsPTBeforeShipping )
      {
         (*p_table)(row++,col) << stress.SetValue(  pLosses->GetTimeDependentLosses(poi,pgsTypes::Temporary,tsInstallIntervalIdx,pgsTypes::End)/*pLosses->GetAfterTemporaryStrandInstallationLosses(poi,pgsTypes::Temporary)*/ );
      }
   
      (*p_table)(row++,col) << stress.SetValue( pLosses->GetTimeDependentLosses(poi,pgsTypes::Temporary,haulSegmentIntervalIdx,pgsTypes::End)/*pLosses->GetShippingLosses(poi,pgsTypes::Temporary)*/ );
      (*p_table)(row++,col) << stress.SetValue( pLosses->GetTimeDependentLosses(poi,pgsTypes::Temporary,erectSegmentIntervalIdx,pgsTypes::End) );
      (*p_table)(row++,col) << stress.SetValue( pLosses->GetTimeDependentLosses(poi,pgsTypes::Temporary,tsRemovalIntervalIdx,pgsTypes::Start)/*pLosses->GetBeforeTemporaryStrandRemovalLosses(poi,pgsTypes::Temporary)*/ );
      (*p_table)(row++,col) << _T(""); //stress.SetValue(  pLosses->GetAfterTemporaryStrandRemovalLosses(poi,pgsTypes::Temporary) );
      
      if (bHasDeck)
      {
         (*p_table)(row++, col) << _T(""); //stress.SetValue( pLosses->GetDeckPlacementLosses(poi,pgsTypes::Temporary) );
      }

      if (bHasJoint)
      {
         (*p_table)(row++, col) << _T("");
      }

      (*p_table)(row++,col) << _T(""); //stress.SetValue( pLosses->GetSIDLLosses(poi,pgsTypes::Temporary) );
      (*p_table)(row++,col) << _T(""); //stress.SetValue( pLosses->GetFinal(poi,pgsTypes::Temporary) );

      if (bRating)
      {
         for (int i = 0; i < (int)(pgsTypes::lrLoadRatingTypeCount); i++)
         {
            pgsTypes::LoadRatingType ratingType = (pgsTypes::LoadRatingType)(i);

            if (pRatingSpec->IsRatingEnabled(ratingType) && pRatingSpec->RateForStress(ratingType))
            {
               std::vector<pgsTypes::LimitState> vLimitStates = GetRatingLimitStates(ratingType);
               for (const auto& limitState : vLimitStates)
               {
                  if (IsStrengthLimitState(limitState))
                  {
                     // we only care about service limit states here
                     continue;
                  }

                  pgsTypes::LiveLoadType llType = LiveLoadTypeFromLimitState(limitState);
                  if (IsDesignRatingType(ratingType))
                  {
                     (*p_table)(row++, col) << _T("");
                  }
                  else
                  {
                     VehicleIndexType nVehicles = pProductLoads->GetVehicleCount(llType);
                     for (VehicleIndexType vehicleIdx = 0; vehicleIdx < nVehicles; vehicleIdx++)
                     {
                        pgsTypes::LiveLoadApplicabilityType applicability = pProductLoads->GetLiveLoadApplicability(llType, vehicleIdx);
                        if (applicability == pgsTypes::llaNegMomentAndInteriorPierReaction)
                        {
                           continue;
                        }
                        std::_tstring name = pProductLoads->GetLiveLoadName(llType, vehicleIdx);
                        if (name == NO_LIVE_LOAD_DEFINED)
                        {
                           continue;
                        }
                        (*p_table)(row++, col) << _T("");
                     }
                  }
               }
            }
         }
      }
      else
      {
         (*p_table)(row++, col) << _T(""); //stress.SetValue( pLosses->GetFinalWithLiveLoad(poi,pgsTypes::Temporary) ); // Service I
         (*p_table)(row++, col) << _T(""); //stress.SetValue( pLosses->GetFinalWithLiveLoad(poi,pgsTypes::Temporary) ); // Service III
         (*p_table)(row++, col) << _T(""); //stress.SetValue( pLosses->GetFinalWithLiveLoad(poi,pgsTypes::Temporary) ); // Service IA/Fatigue I
      }

      ///////////////////////////////////
      // Temporary Strand Gain Column
      row = p_table->GetNumberOfHeaderRows();
      col++;

      if ( pStrands->GetTemporaryStrandUsage() == pgsTypes::ttsPretensioned )
      {
         (*p_table)(row++,col) << stress.SetValue( -pLosses->GetInstantaneousEffects(poi,pgsTypes::Temporary,stressStrandsIntervalIdx,pgsTypes::Start) );
         (*p_table)(row++,col) << stress.SetValue( -pLosses->GetInstantaneousEffects(poi,pgsTypes::Temporary,releaseIntervalIdx,pgsTypes::Start)/*pLosses->GetBeforeXferLosses(poi,pgsTypes::Temporary)*/ );
         (*p_table)(row++,col) << stress.SetValue( -pLosses->GetInstantaneousEffects(poi,pgsTypes::Temporary,releaseIntervalIdx,pgsTypes::End)/*pLosses->GetAfterXferLosses(poi,pgsTypes::Temporary)*/ );
      }
      else
      {
         (*p_table)(row++,col) << _T(""); //stress.SetValue( 0.0 );
         (*p_table)(row++,col) << _T(""); //stress.SetValue( pLosses->GetBeforeXferLosses(poi,pgsTypes::Temporary) );
         (*p_table)(row++,col) << _T(""); //stress.SetValue( pLosses->GetAfterXferLosses(poi,pgsTypes::Temporary) );
      }

      if ( pStrands->GetTemporaryStrandUsage() == pgsTypes::ttsPTBeforeLifting )
      {
         (*p_table)(row++,col) << stress.SetValue( -pLosses->GetInstantaneousEffects(poi,pgsTypes::Temporary,tsInstallIntervalIdx,pgsTypes::End)/*pLosses->GetAfterTemporaryStrandInstallationLosses(poi,pgsTypes::Temporary)*/ );
      }
   
      if ( pStrands->GetTemporaryStrandUsage() == pgsTypes::ttsPTAfterLifting || pStrands->GetTemporaryStrandUsage() == pgsTypes::ttsPTBeforeShipping )
      {
         (*p_table)(row++,col) << _T("");
      }
      else
      {
         (*p_table)(row++,col) << stress.SetValue( -pLosses->GetInstantaneousEffects(poi,pgsTypes::Temporary,liftSegmentIntervalIdx,pgsTypes::End)/*pLosses->GetLiftingLosses(poi,pgsTypes::Temporary)*/ );
      }

      if ( pStrands->GetTemporaryStrandUsage() == pgsTypes::ttsPTAfterLifting || pStrands->GetTemporaryStrandUsage() == pgsTypes::ttsPTBeforeShipping )
      {
         (*p_table)(row++,col) << stress.SetValue( -pLosses->GetInstantaneousEffects(poi,pgsTypes::Temporary,tsInstallIntervalIdx,pgsTypes::End)/*pLosses->GetAfterTemporaryStrandInstallationLosses(poi,pgsTypes::Temporary)*/ );
      }
   
      (*p_table)(row++,col) << stress.SetValue( -pLosses->GetInstantaneousEffects(poi,pgsTypes::Temporary,haulSegmentIntervalIdx,pgsTypes::End)/*pLosses->GetShippingLosses(poi,pgsTypes::Temporary)*/ );
      (*p_table)(row++,col) << stress.SetValue( -pLosses->GetInstantaneousEffects(poi,pgsTypes::Temporary,erectSegmentIntervalIdx,pgsTypes::End) );
      (*p_table)(row++,col) << stress.SetValue( -pLosses->GetInstantaneousEffects(poi,pgsTypes::Temporary,tsRemovalIntervalIdx,pgsTypes::Start)/*pLosses->GetBeforeTemporaryStrandRemovalLosses(poi,pgsTypes::Temporary)*/ );
      (*p_table)(row++,col) << _T(""); //stress.SetValue(  pLosses->GetAfterTemporaryStrandRemovalLosses(poi,pgsTypes::Temporary) );

      if (bHasDeck)
      {
         (*p_table)(row++, col) << _T(""); //stress.SetValue( pLosses->GetDeckPlacementLosses(poi,pgsTypes::Temporary) );
      }

      if (bHasJoint)
      {
         (*p_table)(row++, col) << _T("");
      }

      (*p_table)(row++,col) << _T(""); //stress.SetValue( pLosses->GetSIDLLosses(poi,pgsTypes::Temporary) );
      (*p_table)(row++,col) << _T(""); //stress.SetValue( pLosses->GetFinal(poi,pgsTypes::Temporary) );
      
      if (bRating)
      {
         for (int i = 0; i < (int)(pgsTypes::lrLoadRatingTypeCount); i++)
         {
            pgsTypes::LoadRatingType ratingType = (pgsTypes::LoadRatingType)(i);

            if (pRatingSpec->IsRatingEnabled(ratingType) && pRatingSpec->RateForStress(ratingType))
            {
               std::vector<pgsTypes::LimitState> vLimitStates = GetRatingLimitStates(ratingType);
               for (const auto& limitState : vLimitStates)
               {
                  if (IsStrengthLimitState(limitState))
                  {
                     // we only care about service limit states here
                     continue;
                  }

                  pgsTypes::LiveLoadType llType = LiveLoadTypeFromLimitState(limitState);
                  if (IsDesignRatingType(ratingType))
                  {
                     (*p_table)(row++, col) << _T("");
                  }
                  else
                  {
                     VehicleIndexType nVehicles = pProductLoads->GetVehicleCount(llType);
                     for (VehicleIndexType vehicleIdx = 0; vehicleIdx < nVehicles; vehicleIdx++)
                     {
                        pgsTypes::LiveLoadApplicabilityType applicability = pProductLoads->GetLiveLoadApplicability(llType, vehicleIdx);
                        if (applicability == pgsTypes::llaNegMomentAndInteriorPierReaction)
                        {
                           continue;
                        }
                        std::_tstring name = pProductLoads->GetLiveLoadName(llType, vehicleIdx);
                        if (name == NO_LIVE_LOAD_DEFINED)
                        {
                           continue;
                        }
                        (*p_table)(row++, col) << _T("");
                     }
                  }
               }
            }
         }
      }
      else
      {
         (*p_table)(row++, col) << _T(""); //stress.SetValue( pLosses->GetFinalWithLiveLoad(poi,pgsTypes::Temporary) ); // Service I
         (*p_table)(row++, col) << _T(""); //stress.SetValue( pLosses->GetFinalWithLiveLoad(poi,pgsTypes::Temporary) ); // Service III
         (*p_table)(row++, col) << _T(""); //stress.SetValue( pLosses->GetFinalWithLiveLoad(poi,pgsTypes::Temporary) ); // Service IA/Fatigue I
      }

      ///////////////////////////////////
      // Temporary Strand Stress Column
      row = p_table->GetNumberOfHeaderRows();
      col++;
      if ( pStrands->GetTemporaryStrandUsage() == pgsTypes::ttsPretensioned )
      {
         (*p_table)(row++,col) << stress.SetValue( pPrestressForce->GetEffectivePrestress(poi,pgsTypes::Temporary,stressStrandsIntervalIdx,pgsTypes::Start/*pgsTypes::Jacking*/) );
         (*p_table)(row++,col) << stress.SetValue( pPrestressForce->GetEffectivePrestress(poi,pgsTypes::Temporary,releaseIntervalIdx,pgsTypes::Start/*pgsTypes::BeforeXfer*/) );
         (*p_table)(row++,col) << stress.SetValue( pPrestressForce->GetEffectivePrestress(poi,pgsTypes::Temporary,releaseIntervalIdx,pgsTypes::End/*pgsTypes::AfterXfer*/) );
      }
      else
      {
         (*p_table)(row++,col) << _T(""); //stress.SetValue( pPrestressForce->GetStrandStress(poi,pgsTypes::Temporary,pgsTypes::Jacking) );
         (*p_table)(row++,col) << _T(""); //stress.SetValue( pPrestressForce->GetStrandStress(poi,pgsTypes::Temporary,pgsTypes::BeforeXfer) );
         (*p_table)(row++,col) << _T(""); //stress.SetValue( pPrestressForce->GetStrandStress(poi,pgsTypes::Temporary,pgsTypes::AfterXfer) );
      }

      if ( pStrands->GetTemporaryStrandUsage() == pgsTypes::ttsPTBeforeLifting )
      {
         (*p_table)(row++,col) << stress.SetValue( pPrestressForce->GetEffectivePrestress(poi,pgsTypes::Temporary,tsInstallIntervalIdx,pgsTypes::End) );
      }
   
      if ( pStrands->GetTemporaryStrandUsage() == pgsTypes::ttsPTAfterLifting || pStrands->GetTemporaryStrandUsage() == pgsTypes::ttsPTBeforeShipping )
      {
         (*p_table)(row++,col) << _T("");
      }
      else
      {
         (*p_table)(row++,col) << stress.SetValue( pPrestressForce->GetEffectivePrestress(poi,pgsTypes::Temporary,liftSegmentIntervalIdx,pgsTypes::End) );
      }

      if ( pStrands->GetTemporaryStrandUsage() == pgsTypes::ttsPTAfterLifting || pStrands->GetTemporaryStrandUsage() == pgsTypes::ttsPTBeforeShipping )
      {
         (*p_table)(row++,col) << stress.SetValue( pPrestressForce->GetEffectivePrestress(poi,pgsTypes::Temporary,tsInstallIntervalIdx,pgsTypes::End) );
      }

      (*p_table)(row++,col) << stress.SetValue( pPrestressForce->GetEffectivePrestress(poi,pgsTypes::Temporary,haulSegmentIntervalIdx,pgsTypes::End) );
      (*p_table)(row++,col) << stress.SetValue( pPrestressForce->GetEffectivePrestress(poi,pgsTypes::Temporary,erectSegmentIntervalIdx,pgsTypes::End/*pgsTypes::BeforeTemporaryStrandRemoval*/) );
      (*p_table)(row++,col) << stress.SetValue( pPrestressForce->GetEffectivePrestress(poi,pgsTypes::Temporary,tsRemovalIntervalIdx,pgsTypes::Start/*pgsTypes::BeforeTemporaryStrandRemoval*/) );
      (*p_table)(row++,col) << _T(""); //stress.SetValue( pPrestressForce->GetStrandStress(poi,pgsTypes::Temporary,pgsTypes::AfterTemporaryStrandRemoval) );

      if (bHasDeck)
      {
         (*p_table)(row++, col) << _T(""); //stress.SetValue( pPrestressForce->GetStrandStress(poi,pgsTypes::Temporary,pgsTypes::AfterDeckPlacement) );
      }

      if (bHasJoint)
      {
         (*p_table)(row++, col) << _T("");
      }

      (*p_table)(row++,col) << _T(""); //stress.SetValue( pPrestressForce->GetStrandStress(poi,pgsTypes::Temporary,pgsTypes::AfterSIDL) );
      (*p_table)(row++,col) << _T(""); //stress.SetValue( pPrestressForce->GetStrandStress(poi,pgsTypes::Temporary,pgsTypes::AfterLosses) );

      if (bRating)
      {
         for (int i = 0; i < (int)(pgsTypes::lrLoadRatingTypeCount); i++)
         {
            pgsTypes::LoadRatingType ratingType = (pgsTypes::LoadRatingType)(i);

            if (pRatingSpec->IsRatingEnabled(ratingType) && pRatingSpec->RateForStress(ratingType))
            {
               std::vector<pgsTypes::LimitState> vLimitStates = GetRatingLimitStates(ratingType);
               for (const auto& limitState : vLimitStates)
               {
                  if (IsStrengthLimitState(limitState))
                  {
                     // we only care about service limit states here
                     continue;
                  }

                  pgsTypes::LiveLoadType llType = LiveLoadTypeFromLimitState(limitState);
                  if (IsDesignRatingType(ratingType))
                  {
                     (*p_table)(row++, col) << _T("");
                  }
                  else
                  {
                     VehicleIndexType nVehicles = pProductLoads->GetVehicleCount(llType);
                     for (VehicleIndexType vehicleIdx = 0; vehicleIdx < nVehicles; vehicleIdx++)
                     {
                        pgsTypes::LiveLoadApplicabilityType applicability = pProductLoads->GetLiveLoadApplicability(llType, vehicleIdx);
                        if (applicability == pgsTypes::llaNegMomentAndInteriorPierReaction)
                        {
                           continue;
                        }
                        std::_tstring name = pProductLoads->GetLiveLoadName(llType, vehicleIdx);
                        if (name == NO_LIVE_LOAD_DEFINED)
                        {
                           continue;
                        }
                        (*p_table)(row++, col) << _T("");
                     }
                  }
               }
            }
         }
      }
      else
      {
         (*p_table)(row++, col) << _T(""); //stress.SetValue( pPrestressForce->GetStrandStress(poi,pgsTypes::Temporary,pgsTypes::AfterLossesWithLiveLoad) ); // Service I
         (*p_table)(row++, col) << _T(""); //stress.SetValue( pPrestressForce->GetStrandStress(poi,pgsTypes::Temporary,pgsTypes::AfterLossesWithLiveLoad) ); // Service III
         (*p_table)(row++, col) << _T(""); //stress.SetValue( pPrestressForce->GetStrandStress(poi,pgsTypes::Temporary,pgsTypes::AfterLossesWithLiveLoad) ); // Service IA/Fatigue I
      }
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

