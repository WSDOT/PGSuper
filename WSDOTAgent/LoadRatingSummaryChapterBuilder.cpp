///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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
#include "LoadRatingSummaryChapterBuilder.h"

#include <Reporting\SpanGirderReportSpecification.h>
#include <Reporting\ReportNotes.h>

#include <PgsExt\CapacityToDemand.h>

#include <EAF\EAFDisplayUnits.h>
#include <IFace\Artifact.h>
#include <IFace\RatingSpecification.h>
#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\AnalysisResults.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CLoadRatingSummaryChapterBuilder
****************************************************************************/

CLoadRatingSummaryChapterBuilder::CLoadRatingSummaryChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

LPCTSTR CLoadRatingSummaryChapterBuilder::GetName() const
{
   return TEXT("WSDOT Load Rating Summary");
}

rptChapter* CLoadRatingSummaryChapterBuilder::Build(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,Uint16 level) const
{
   auto pGdrRptSpec = std::dynamic_pointer_cast<const CGirderLineReportSpecification>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pGdrRptSpec->GetBroker(&pBroker);
   GirderIndexType gdrIdx = pGdrRptSpec->GetGirderIndex();
   CGirderKey girderKey(ALL_GROUPS,gdrIdx);

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);
   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   std::vector<std::_tstring>::iterator found;
   bool bIsWSDOTRating = true;
   GET_IFACE2(pBroker,ILiveLoads,pLiveLoads);
   std::vector<std::_tstring> routine_legal_loads = pLiveLoads->GetLiveLoadNames(pgsTypes::lltLegalRating_Routine);
   if ( routine_legal_loads.size() != 2 || (routine_legal_loads[0] != _T("AASHTO Legal Loads") && routine_legal_loads[1] != _T("WA-105")))
   {
      bIsWSDOTRating = false;
   }

   std::vector<std::_tstring> special_legal_loads = pLiveLoads->GetLiveLoadNames(pgsTypes::lltLegalRating_Special);
   if (special_legal_loads.size() != 1 || special_legal_loads[0] != _T("Notional Rating Load (NRL)"))
   {
      bIsWSDOTRating = false;
   }

   std::vector<std::_tstring> emergency_legal_loads = pLiveLoads->GetLiveLoadNames(pgsTypes::lltLegalRating_Emergency);
   if (emergency_legal_loads.size() != 1 || emergency_legal_loads[0] != _T("Emergency Vehicles"))
   {
      bIsWSDOTRating = false;
   }

   std::vector<std::_tstring> routine_permit_loads = pLiveLoads->GetLiveLoadNames(pgsTypes::lltPermitRating_Routine);
   if ( routine_permit_loads.size() != 0 )
   {
      bIsWSDOTRating = false;
   }

   GET_IFACE2(pBroker,IBridge,pBridge);
   bool bNegMoments = pBridge->ProcessNegativeMoments(ALL_SPANS);
   bool bOL1 = false;
   bool bOL2 = false;
   bool bOL1NegMoment = (bNegMoments ? false : true);
   bool bOL2NegMoment = (bNegMoments ? false : true);
   std::vector<std::_tstring> special_permit_loads = pLiveLoads->GetLiveLoadNames(pgsTypes::lltPermitRating_Special);
   std::vector<std::_tstring>::iterator iter(special_permit_loads.begin());
   std::vector<std::_tstring>::iterator end(special_permit_loads.end());
   for ( ; iter != end; iter++ )
   {
      std::_tstring load(*iter);
      if ( load.compare(_T("OL1")) == 0 )
      {
         bOL1 = true;
      }

      if ( load.compare(_T("OL2")) == 0 )
      {
         bOL2 = true;
      }

      // check for neg moment version of the OL trucks... if found and neg moment analysis is required
      // set the value to true.... if found and neg moment analysis is not required, set the value to vale
      if ( load.compare(_T("OL1 (Neg Moment)")) == 0 )
      {
         bOL1NegMoment = (bNegMoments ? true : false);
      }

      if ( load.compare(_T("OL2 (Neg Moment)")) == 0 )
      {
         bOL2NegMoment = (bNegMoments ? true : false);
      }
   }
   if ( bOL1 == false || bOL2 == false || bOL1NegMoment == false || bOL2NegMoment == false )
   {
      bIsWSDOTRating = false;
   }

   std::vector<std::_tstring> design_permit_loads = pLiveLoads->GetLiveLoadNames(pgsTypes::lltDesign);
   if ( design_permit_loads.size() == 0 || design_permit_loads[0] != _T("HL-93") )
   {
      bIsWSDOTRating = false;
   }

   GET_IFACE2(pBroker,IRatingSpecification,pRatingSpec);
   if ( pRatingSpec->GetRatingSpecification() != _T("WSDOT") )
   {
      bIsWSDOTRating = false;
   }

   if ( !pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) )
   {
      bIsWSDOTRating = false;
   }

   if ( !pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating) )
   {
      bIsWSDOTRating = false;
   }

   if ( !pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
   {
      bIsWSDOTRating = false;
   }

   if (!pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special))
   {
      bIsWSDOTRating = false;
   }

   if (!pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Emergency))
   {
      bIsWSDOTRating = false;
   }

   if ( !pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
   {
      bIsWSDOTRating = false;
   }

   if ( !pRatingSpec->RateForShear(pgsTypes::lrDesign_Inventory) || !pRatingSpec->RateForShear(pgsTypes::lrDesign_Operating) ||
        !pRatingSpec->RateForShear(pgsTypes::lrLegal_Routine)    || !pRatingSpec->RateForShear(pgsTypes::lrLegal_Special)    || !pRatingSpec->RateForShear(pgsTypes::lrLegal_Emergency) ||
        !pRatingSpec->RateForShear(pgsTypes::lrPermit_Routine)   || !pRatingSpec->RateForShear(pgsTypes::lrPermit_Special)
      )
   {
      bIsWSDOTRating = false;
   }

   if ( !pRatingSpec->RateForStress(pgsTypes::lrLegal_Routine)  || !pRatingSpec->RateForStress(pgsTypes::lrLegal_Special) || !pRatingSpec->RateForStress(pgsTypes::lrLegal_Emergency) ||
        !pRatingSpec->RateForStress(pgsTypes::lrPermit_Routine) || !pRatingSpec->RateForStress(pgsTypes::lrPermit_Special)
      )
   {
      bIsWSDOTRating = false;
   }

   if ( !pRatingSpec->CheckYieldStress(pgsTypes::lrPermit_Routine) || !pRatingSpec->CheckYieldStress(pgsTypes::lrPermit_Special)
      )
   {
      bIsWSDOTRating = false;
   }

   if ( !bIsWSDOTRating )
   {
      (*pPara) << _T("The selected load rating options do not conform to the requirements specified in Chapter 13 of the WSDOT Bridge Design Manual.") << rptNewLine;
      (*pPara) << _T("Select ") << Bold(_T("Project > Load Rating Options")) << _T(" to change the load rating options to the required settings given below.") << rptNewLine;

	   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(3,_T("Required Load Rating Options"));

      // left justify all columns
      pTable->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
      pTable->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
      pTable->SetColumnStyle(1,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
      pTable->SetStripeRowColumnStyle(1,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
      pTable->SetColumnStyle(2,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
      pTable->SetStripeRowColumnStyle(2,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

      RowIndexType row = 0;
      (*pPara) << pTable << rptNewLine;
      (*pTable)(row,0) << _T("Load Rating Option");
      (*pTable)(row,1) << _T("Current Setting");
      (*pTable)(row,2) << _T("Required Setting");
      row++;

      // General Tab
      pTable->SetColumnSpan(row,0,3);
      (*pTable)(row,0) << Bold(_T("General"));
      row++;

      (*pTable)(row,0) << _T("Load Rating Criteria");
      (*pTable)(row,1) << pRatingSpec->GetRatingSpecification();
      (*pTable)(row,2) << _T("WSDOT");
      row++;

      (*pTable)(row,0) << _T("Rating Type: Design");
      (*pTable)(row,1) << ((!pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) || !pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating) ) ? _T("Unselected (unchecked)") : _T("Selected (checked)"));
      (*pTable)(row,2) << _T("Selected (checked)");
      row++;

      (*pTable)(row,0) << _T("Rating Type: Legal");
      (*pTable)(row,1) << ((!pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) || !pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) || !pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Emergency)) ? _T("Unselected (unchecked)") : _T("Selected (checked)"));
      (*pTable)(row,2) << _T("Selected (checked)");
      row++;

      (*pTable)(row,0) << _T("Rating Type: Permit");
      (*pTable)(row,1) << ((!pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) ) ? _T("Unselected (unchecked)") : _T("Selected (checked)"));
      (*pTable)(row,2) << _T("Selected (checked)");
      row++;

      // Design Tab
      pTable->SetColumnSpan(row,0,3);
      (*pTable)(row,0) << Bold(_T("Design"));
      row++;

      (*pTable)(row,0) << _T("Design Load Rating: Live Loads for Design") << rptNewLine << _T("(Select ") << Bold(_T("Loads > Live Loads...")) << _T(" for this setting)");
      std::vector<std::_tstring>::iterator nameIter(design_permit_loads.begin());
      std::vector<std::_tstring>::iterator nameIterEnd(design_permit_loads.end());
      for ( ; nameIter != nameIterEnd; nameIter++ )
      {
         (*pTable)(row,1) << (*nameIter) << rptNewLine;
      }
      (*pTable)(row,1) << _T("");
      (*pTable)(row,2) << _T("HL-93");
      row++;

      (*pTable)(row,0) << _T("Rate for Shear");
      (*pTable)(row,1) << ((!pRatingSpec->RateForShear(pgsTypes::lrDesign_Inventory) || !pRatingSpec->RateForShear(pgsTypes::lrDesign_Operating) ) ? _T("Unselected (unchecked)") : _T("Selected (checked)"));
      (*pTable)(row,2) << _T("Selected (checked)");
      row++;

      // Legal Tab
      pTable->SetColumnSpan(row,0,3);
      (*pTable)(row,0) << Bold(_T("Legal"));
      row++;

      (*pTable)(row,0) << _T("Legal Load Rating: Live Loads for Routine Commercial Vehicles");
      nameIter    = routine_legal_loads.begin();
      nameIterEnd = routine_legal_loads.end();
      for ( ; nameIter != nameIterEnd; nameIter++ )
      {
         (*pTable)(row,1) << (*nameIter) << rptNewLine;
      }
      (*pTable)(row,1) << _T("");
      (*pTable)(row,2) << _T("AASHTO Legal Loads");
      row++;

      (*pTable)(row,0) << _T("Legal Load Rating: Live Loads for Specialized Hauling Vehicles");
      nameIter    = special_legal_loads.begin();
      nameIterEnd = special_legal_loads.end();
      for ( ; nameIter != nameIterEnd; nameIter++ )
      {
         (*pTable)(row,1) << (*nameIter) << rptNewLine;
      }
      (*pTable)(row,1) << _T("");
      (*pTable)(row,2) << _T("Notional Rating Load (NRL)");
      row++;

      (*pTable)(row, 0) << _T("Legal Load Rating: Live Loads for Emergency Vehicles");
      nameIter = emergency_legal_loads.begin();
      nameIterEnd = emergency_legal_loads.end();
      for (; nameIter != nameIterEnd; nameIter++)
      {
         (*pTable)(row, 1) << (*nameIter) << rptNewLine;
      }
      (*pTable)(row, 1) << _T("");
      (*pTable)(row, 2) << _T("Emergency Vehicles");
      row++;

      (*pTable)(row,0) << _T("Rate for Service III Stress");
      (*pTable)(row,1) << ((!pRatingSpec->RateForStress(pgsTypes::lrLegal_Routine) || !pRatingSpec->RateForStress(pgsTypes::lrLegal_Special) || !pRatingSpec->RateForStress(pgsTypes::lrLegal_Emergency) ) ? _T("Unselected (unchecked)") : _T("Selected (checked)"));
      (*pTable)(row,2) << _T("Selected (checked)");
      row++;

      (*pTable)(row,0) << _T("Rate for Shear");
      (*pTable)(row,1) << ((!pRatingSpec->RateForShear(pgsTypes::lrLegal_Routine) || !pRatingSpec->RateForShear(pgsTypes::lrLegal_Special) || !pRatingSpec->RateForShear(pgsTypes::lrLegal_Emergency)) ? _T("Unselected (unchecked)") : _T("Selected (checked)"));
      (*pTable)(row,2) << _T("Selected (checked)");
      row++;

      // Permit Tab
      pTable->SetColumnSpan(row,0,3);
      (*pTable)(row,0) << Bold(_T("Permit"));
      row++;

      (*pTable)(row,0) << _T("Permit Load Rating: Live Loads for Routine/Annual Permit Vehicles");
      nameIter    = routine_permit_loads.begin();
      nameIterEnd = routine_permit_loads.end();
      for ( ; nameIter != nameIterEnd; nameIter++ )
      {
         (*pTable)(row,1) << (*nameIter) << rptNewLine;
      }
      (*pTable)(row,1) << _T("");
      (*pTable)(row,2) << _T("");
      row++;

      (*pTable)(row,0) << _T("Permit Load Rating: Live Loads for Special/Limited Crossing Permit Vehicles");
      nameIter    = special_permit_loads.begin();
      nameIterEnd = special_permit_loads.end();
      for ( ; nameIter != nameIterEnd; nameIter++ )
      {
         (*pTable)(row,1) << (*nameIter) << rptNewLine;
      }
      (*pTable)(row,1) << _T("");
      (*pTable)(row,2) << _T("OL1") << rptNewLine;
      if ( bNegMoments )
      {
         (*pTable)(row,2) << _T("OL1 (Neg Moment)") << rptNewLine;
      }
      (*pTable)(row,2) << _T("OL2") << rptNewLine;
      if ( bNegMoments )
      {
         (*pTable)(row,2) << _T("OL2 (Neg Moment)") << rptNewLine;
      }
      row++;

      (*pTable)(row,0) << _T("Rate for Service III Stress");
      (*pTable)(row,1) << ((!pRatingSpec->RateForStress(pgsTypes::lrPermit_Routine) || !pRatingSpec->RateForStress(pgsTypes::lrPermit_Special) ) ? _T("Unselected (unchecked)") : _T("Selected (checked)"));
      (*pTable)(row,2) << _T("Selected (checked)");
      row++;

      (*pTable)(row,0) << _T("Check reinforcement yielding");
      (*pTable)(row,1) << ((!pRatingSpec->CheckYieldStress(pgsTypes::lrPermit_Routine) || !pRatingSpec->CheckYieldStress(pgsTypes::lrPermit_Special) ) ? _T("Unselected (unchecked)") : _T("Selected (checked)"));
      (*pTable)(row,2) << _T("Selected (checked)");
      row++;

      (*pTable)(row,0) << _T("Rate for Shear");
      (*pTable)(row,1) << ((!pRatingSpec->RateForShear(pgsTypes::lrPermit_Routine) || !pRatingSpec->RateForShear(pgsTypes::lrPermit_Special) ) ? _T("Unselected (unchecked)") : _T("Selected (checked)"));
      (*pTable)(row,2) << _T("Selected (checked)");
      row++;

      return pChapter;
   }

   // The rating settings are consistent with WSDOT policies... report the rating
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   INIT_UV_PROTOTYPE( rptLengthUnitValue, length, pDisplayUnits->GetSpanLengthUnit(),   true );

   WBFL::System::Date date;
   GET_IFACE2(pBroker,IProjectProperties,pProjectProperties);

   (*pPara) << _T("Bridge Name: ") << pProjectProperties->GetBridgeName() << rptNewLine;
   (*pPara) << _T("Bridge Number: ") << pProjectProperties->GetBridgeID() << rptNewLine;
   (*pPara) << _T("Span Types : PCG") << rptNewLine;
   (*pPara) << _T("Bridge Length : ") << length.SetValue(pBridge->GetLength()) << rptNewLine;
   (*pPara) << _T("Rated By: ") << pProjectProperties->GetEngineer() << rptNewLine;
   (*pPara) << _T("Date : ") << date.AsString() << rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;

   rptParagraph* pRemarks = new rptParagraph;
   (*pRemarks) << _T("Remarks:") << rptNewLine;
   (*pRemarks) << _T("This load rating does not include rating factors for substructure elements.") << rptNewLine;

   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(5,_T(""));
   (*pPara) << pTable << rptNewLine;

   pTable->SetColumnStyle(0, rptStyleManager::GetTableCellStyle( CB_NONE | CJ_LEFT) );
   pTable->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle( CB_NONE | CJ_LEFT) );

   pTable->SetColumnStyle(4, rptStyleManager::GetTableCellStyle( CB_NONE | CJ_LEFT) );
   pTable->SetStripeRowColumnStyle(4, rptStyleManager::GetTableStripeRowCellStyle( CB_NONE | CJ_LEFT) );

   ColumnIndexType col = 0;
   (*pTable)(0, col++) << _T("Truck");
   (*pTable)(0, col++) << _T("RF");
   (*pTable)(0, col++) << _T("Yield") << rptNewLine << _T("Stress") << rptNewLine << _T("Ratio");
   (*pTable)(0, col++) << Sub2(symbol(gamma),_T("LL"));
   (*pTable)(0, col++)  << COLHDR(_T("Controlling Point") << rptNewLine << RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

   GET_IFACE2(pBroker,IArtifact,pIArtifact);

   RowIndexType row = pTable->GetNumberOfHeaderRows();

   GET_IFACE2(pBroker,IProductLoads,pProductLoads);

   pgsTypes::LiveLoadType llType = ::GetLiveLoadType(pgsTypes::lrLegal_Routine);
   VehicleIndexType nVehicles = pProductLoads->GetVehicleCount(llType);
   for ( VehicleIndexType vehicleIdx = 0; vehicleIdx < nVehicles; vehicleIdx++ )
   {
      ReportRatingFactor(pBroker,pTable,row,pIArtifact->GetRatingArtifact(girderKey,pgsTypes::lrLegal_Routine,vehicleIdx),pDisplayUnits,pRemarks);
      row++;
   }


   llType = ::GetLiveLoadType(pgsTypes::lrLegal_Special);
   nVehicles = pProductLoads->GetVehicleCount(llType);
   for (VehicleIndexType vehicleIdx = 0; vehicleIdx < nVehicles; vehicleIdx++)
   {
      ReportRatingFactor(pBroker, pTable, row, pIArtifact->GetRatingArtifact(girderKey, pgsTypes::lrLegal_Special, vehicleIdx), pDisplayUnits, pRemarks);
      row++;
   }

   llType = ::GetLiveLoadType(pgsTypes::lrLegal_Emergency);
   nVehicles = pProductLoads->GetVehicleCount(llType);
   for (VehicleIndexType vehicleIdx = 0; vehicleIdx < nVehicles; vehicleIdx++)
   {
      ReportRatingFactor(pBroker, pTable, row, pIArtifact->GetRatingArtifact(girderKey, pgsTypes::lrLegal_Emergency, vehicleIdx), pDisplayUnits, pRemarks);
      row++;
   }

   // Current WSDOT default is to have no trucks in the Permit Routine case so there is nothing to report
   //llType = ::GetLiveLoadType(pgsTypes::lrPermit_Routine);
   //nVehicles = pProductLoads->GetVehicleCount(llType);
   //for ( VehicleIndexType vehicleIdx = 0; vehicleIdx < nVehicles; vehicleIdx++ )
   //{
   //   ReportRatingFactor(pBroker,pTable,row++,pIArtifact->GetRatingArtifact(gdrLineIdx,pgsTypes::lrPermit_Routine,vehicleIdx),pDisplayUnits,pRemarks);
   //}


   llType = ::GetLiveLoadType(pgsTypes::lrPermit_Special);
   nVehicles = pProductLoads->GetVehicleCount(llType);
   for ( VehicleIndexType vehicleIdx = 0; vehicleIdx < nVehicles; vehicleIdx++ )
   {
      ReportRatingFactor(pBroker,pTable,row,pIArtifact->GetRatingArtifact(girderKey,pgsTypes::lrPermit_Special,vehicleIdx),pDisplayUnits,pRemarks);
      row++;
   }

   pTable = rptStyleManager::CreateDefaultTable(3,_T(""));
   (*pPara) << pTable << rptNewLine;

   pTable->SetColumnStyle(0, rptStyleManager::GetTableCellStyle( CB_NONE | CJ_LEFT) );
   pTable->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle( CB_NONE | CJ_LEFT) );

   pTable->SetColumnStyle(2, rptStyleManager::GetTableCellStyle( CB_NONE | CJ_LEFT) );
   pTable->SetStripeRowColumnStyle(2, rptStyleManager::GetTableStripeRowCellStyle( CB_NONE | CJ_LEFT) );

   (*pTable)(0,0) << _T("NBI Rating");
   (*pTable)(0,1) << _T("RF");
   (*pTable)(0,2)  << COLHDR(_T("Controlling Point") << rptNewLine << RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

   row = 1;
   ReportRatingFactor2(pBroker,pTable,row++,_T("Inventory"),pIArtifact->GetRatingArtifact(girderKey,pgsTypes::lrDesign_Inventory,INVALID_INDEX),pDisplayUnits,pRemarks);
   ReportRatingFactor2(pBroker,pTable,row++,_T("Operating"),pIArtifact->GetRatingArtifact(girderKey,pgsTypes::lrDesign_Operating,INVALID_INDEX),pDisplayUnits,pRemarks);

   *pChapter << pRemarks;

   return pChapter;
}

std::unique_ptr<WBFL::Reporting::ChapterBuilder> CLoadRatingSummaryChapterBuilder::Clone() const
{
   return std::make_unique<CLoadRatingSummaryChapterBuilder>();
}

void CLoadRatingSummaryChapterBuilder::ReportRatingFactor(IBroker* pBroker,rptRcTable* pTable,RowIndexType& row,const pgsRatingArtifact* pRatingArtifact,IEAFDisplayUnits* pDisplayUnits,rptParagraph* pRemarks) const
{
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(),   false );
   location.IncludeSpanAndGirder(true);

   INIT_SCALAR_PROTOTYPE(rptRcScalar, scalar, pDisplayUnits->GetScalarFormat());

   rptCapacityToDemand rating_factor;

   const pgsMomentRatingArtifact* pPositiveMoment;
   const pgsMomentRatingArtifact* pNegativeMoment;
   const pgsShearRatingArtifact*  pShear;
   const pgsStressRatingArtifact* pStress;
   
   Float64 RF = pRatingArtifact->GetRatingFactorEx(&pPositiveMoment,&pNegativeMoment,&pShear,&pStress);

   ColumnIndexType col = 0;
   if ( pPositiveMoment )
   {
      (*pTable)(row, col++) << pPositiveMoment->GetVehicleName();
      
      if ( RF < 1 )
      {
         (*pTable)(row,col++) << RF_FAIL(rating_factor,RF);
      }
      else
      {
         (*pTable)(row,col++) << RF_PASS(rating_factor,RF);
      }

      (*pTable)(row, col++) << _T(""); // yield stress ratio

      (*pTable)(row, col++) << scalar.SetValue(pPositiveMoment->GetLiveLoadFactor());

      pgsPointOfInterest poi( pPositiveMoment->GetPointOfInterest() );
      (*pTable)(row, col++) << location.SetValue(POI_SPAN,poi) << _T(" (Positive Moment)");
   }
   else if ( pNegativeMoment )
   {
      (*pTable)(row, col++) << pNegativeMoment->GetVehicleName();
      
      if ( RF < 1 )
      {
         (*pTable)(row, col++) << RF_FAIL(rating_factor,RF);
      }
      else
      {
         (*pTable)(row, col++) << RF_PASS(rating_factor,RF);
      }

      (*pTable)(row, col++) << _T(""); // yield stress ratio

      (*pTable)(row, col++) << scalar.SetValue(pNegativeMoment->GetLiveLoadFactor());

      pgsPointOfInterest poi( pNegativeMoment->GetPointOfInterest() );
      (*pTable)(row, col++) << location.SetValue(POI_SPAN,poi) << _T(" (Negative Moment)");
   }
   else if ( pShear )
   {
      (*pTable)(row, col++) << pShear->GetVehicleName();
      
      if ( RF < 1 )
      {
         (*pTable)(row, col++) << RF_FAIL(rating_factor,RF);
      }
      else
      {
         (*pTable)(row, col++) << RF_PASS(rating_factor,RF);
      }

      (*pTable)(row, col++) << _T(""); // yield stress ratio

      (*pTable)(row, col++) << scalar.SetValue(pShear->GetLiveLoadFactor());

      pgsPointOfInterest poi( pShear->GetPointOfInterest() );
      (*pTable)(row, col++) << location.SetValue(POI_SPAN,poi) << _T(" (Shear)");
   }
   else if ( pStress )
   {
      (*pTable)(row, col++) << pStress->GetVehicleName();
      
      if ( RF < 1 )
      {
         (*pTable)(row, col++) << RF_FAIL(rating_factor,RF);
      }
      else
      {
         (*pTable)(row, col++) << RF_PASS(rating_factor,RF);
      }

      (*pTable)(row, col++) << _T(""); // yield stress ratio

      (*pTable)(row, col++) << scalar.SetValue(pStress->GetLiveLoadFactor());

      pgsPointOfInterest poi(  pStress->GetPointOfInterest() );
      (*pTable)(row, col++) << location.SetValue(POI_SPAN,poi) << _T(" (Stress)");
   }

   const pgsYieldStressRatioArtifact* pYieldStressPositiveMoment;
   const pgsYieldStressRatioArtifact* pYieldStressNegativeMoment;
   Float64 SR = pRatingArtifact->GetYieldStressRatio(&pYieldStressPositiveMoment, &pYieldStressNegativeMoment);
   if ( pYieldStressPositiveMoment )
   {
      row++;
      col = 0;
      (*pTable)(row, col++) << pYieldStressPositiveMoment->GetVehicleName();

      (*pTable)(row, col++) << _T(""); // rating factor

      if ( SR < 1 )
      {
         (*pTable)(row, col++) << RF_FAIL(rating_factor,SR);
      }
      else
      {
         (*pTable)(row, col++) << RF_PASS(rating_factor,SR);
      }

      (*pTable)(row, col++) << scalar.SetValue(pYieldStressPositiveMoment->GetLiveLoadFactor());

      pgsPointOfInterest poi(  pYieldStressPositiveMoment->GetPointOfInterest() );
      (*pTable)(row, col++) << location.SetValue(POI_SPAN,poi) << _T(" (Positive Moment)");

      if ( 0 < pYieldStressPositiveMoment->GetRebarCrackingStressIncrement() )
      {
         (*pRemarks) << pYieldStressPositiveMoment->GetVehicleName() << _T(": Section is cracked at rebar for Service I limit state") << rptNewLine;
      }

      if ( 0 < pYieldStressPositiveMoment->GetStrandCrackingStressIncrement() )
      {
         (*pRemarks) << pYieldStressPositiveMoment->GetVehicleName() << _T(": Section is cracked at strand for Service I limit state") << rptNewLine;
      }

      if (0 < pYieldStressPositiveMoment->GetSegmentTendonCrackingStressIncrement())
      {
         (*pRemarks) << pYieldStressPositiveMoment->GetVehicleName() << _T(": Section is cracked at segment tendon for Service I limit state") << rptNewLine;
      }

      if (0 < pYieldStressPositiveMoment->GetGirderTendonCrackingStressIncrement())
      {
         (*pRemarks) << pYieldStressPositiveMoment->GetVehicleName() << _T(": Section is cracked at girder tendon for Service I limit state") << rptNewLine;
      }
   }
   else if ( pYieldStressNegativeMoment )
   {
      row++;
      col = 0;
      (*pTable)(row, col++) << pYieldStressNegativeMoment->GetVehicleName();

      (*pTable)(row, col++) << _T(""); // rating factor

      if ( SR < 1 )
      {
         (*pTable)(row, col++) << RF_FAIL(rating_factor,SR);
      }
      else
      {
         (*pTable)(row, col++) << RF_PASS(rating_factor,SR);
      }

      (*pTable)(row, col++) << scalar.SetValue(pYieldStressNegativeMoment->GetLiveLoadFactor());

      pgsPointOfInterest poi(  pYieldStressNegativeMoment->GetPointOfInterest() );
      (*pTable)(row, col++) << location.SetValue(POI_SPAN,poi) << _T(" (Negative Moment)");

      if ( 0 < pYieldStressNegativeMoment->GetRebarCrackingStressIncrement() )
      {
         (*pRemarks) << pYieldStressNegativeMoment->GetVehicleName() << _T(": Section is cracked at rebar for Service I limit state") << rptNewLine;
      }

      if ( 0 < pYieldStressNegativeMoment->GetStrandCrackingStressIncrement() )
      {
         (*pRemarks) << pYieldStressNegativeMoment->GetVehicleName() << _T(": Section is cracked at strand for Service I limit state") << rptNewLine;
      }

      if (0 < pYieldStressNegativeMoment->GetSegmentTendonCrackingStressIncrement())
      {
         (*pRemarks) << pYieldStressNegativeMoment->GetVehicleName() << _T(": Section is cracked at segment tendon for Service I limit state") << rptNewLine;
      }

      if (0 < pYieldStressNegativeMoment->GetGirderTendonCrackingStressIncrement())
      {
         (*pRemarks) << pYieldStressNegativeMoment->GetVehicleName() << _T(": Section is cracked at girder tendon for Service I limit state") << rptNewLine;
      }
   }
}

void CLoadRatingSummaryChapterBuilder::ReportRatingFactor2(IBroker* pBroker,rptRcTable* pTable,RowIndexType row,LPCTSTR strTruck,const pgsRatingArtifact* pRatingArtifact,IEAFDisplayUnits* pDisplayUnits,rptParagraph* pRemarks) const
{
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(),   false );
   location.IncludeSpanAndGirder(true);

   INIT_SCALAR_PROTOTYPE(rptRcScalar, scalar, pDisplayUnits->GetScalarFormat());

   rptCapacityToDemand rating_factor;

   const pgsMomentRatingArtifact* pPositiveMoment;
   const pgsMomentRatingArtifact* pNegativeMoment;
   const pgsShearRatingArtifact*  pShear;
   const pgsStressRatingArtifact* pStress;

   Float64 RF = pRatingArtifact->GetRatingFactorEx(&pPositiveMoment,&pNegativeMoment,&pShear,&pStress);
   if ( pPositiveMoment )
   {
      (*pTable)(row,0) << strTruck;
      
      if ( RF < 1 )
      {
         (*pTable)(row,1) << RF_FAIL(rating_factor,RF);
      }
      else
      {
         (*pTable)(row,1) << RF_PASS(rating_factor,RF);
      }


      pgsPointOfInterest poi(  pPositiveMoment->GetPointOfInterest() );
      (*pTable)(row,2) << location.SetValue(POI_SPAN,poi) << _T(" (Positive Moment)");
   }
   else if ( pNegativeMoment )
   {
      (*pTable)(row,0) << strTruck;
      
      if ( RF < 1 )
      {
         (*pTable)(row,1) << RF_FAIL(rating_factor,RF);
      }
      else
      {
         (*pTable)(row,1) << RF_PASS(rating_factor,RF);
      }


      pgsPointOfInterest poi( pNegativeMoment->GetPointOfInterest() );
      (*pTable)(row,2) << location.SetValue(POI_SPAN,poi) << _T(" (Negative Moment)");
   }
   else if ( pShear )
   {
      (*pTable)(row,0) << strTruck;
      
      if ( RF < 1 )
      {
         (*pTable)(row,1) << RF_FAIL(rating_factor,RF);
      }
      else
      {
         (*pTable)(row,1) << RF_PASS(rating_factor,RF);
      }


      pgsPointOfInterest poi( pShear->GetPointOfInterest() );
      (*pTable)(row,2) << location.SetValue(POI_SPAN,poi) << _T(" (Shear)");
   }
   else if ( pStress )
   {
      (*pTable)(row,0) << strTruck;
      
      if ( RF < 1 )
      {
         (*pTable)(row,1) << RF_FAIL(rating_factor,RF);
      }
      else
      {
         (*pTable)(row,1) << RF_PASS(rating_factor,RF);
      }


      pgsPointOfInterest poi( pStress->GetPointOfInterest() );
      (*pTable)(row,2) << location.SetValue(POI_SPAN,poi) << _T(" (Stress)");
   }
}
