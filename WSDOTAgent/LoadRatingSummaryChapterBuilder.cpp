///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2012  Washington State Department of Transportation
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
#include <Reporting\ReportStyleHolder.h>
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

rptChapter* CLoadRatingSummaryChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CGirderReportSpecification* pGdrRptSpec = dynamic_cast<CGirderReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pGdrRptSpec->GetBroker(&pBroker);
   GirderIndexType gdrLineIdx = pGdrRptSpec->GetGirder();

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);
   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   std::vector<std::_tstring>::iterator found;
   bool bIsWSDOTRating = true;
   GET_IFACE2(pBroker,ILiveLoads,pLiveLoads);
   std::vector<std::_tstring> routine_legal_loads = pLiveLoads->GetLiveLoadNames(pgsTypes::lltLegalRating_Routine);
   if ( routine_legal_loads.size() != 1 || routine_legal_loads[0] != _T("AASHTO Legal Loads") )
   {
      bIsWSDOTRating = false;
   }

   std::vector<std::_tstring> special_legal_loads = pLiveLoads->GetLiveLoadNames(pgsTypes::lltLegalRating_Special);
   if ( special_legal_loads.size() != 1 || special_legal_loads[0] != _T("Notional Rating Load (NRL)") )
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
         bOL1 = true;

      if ( load.compare(_T("OL2")) == 0 )
         bOL2 = true;

      // check for neg moment version of the OL trucks... if found and neg moment analysis is required
      // set the value to true.... if found and neg moment analysis is not required, set the value to vale
      if ( load.compare(_T("OL1 (Neg Moment)")) == 0 )
         bOL1NegMoment = (bNegMoments ? true : false);

      if ( load.compare(_T("OL2 (Neg Moment)")) == 0 )
         bOL2NegMoment = (bNegMoments ? true : false);
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
      bIsWSDOTRating = false;

   if ( !pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) )
      bIsWSDOTRating = false;

   if ( !pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating) )
      bIsWSDOTRating = false;

   if ( !pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
      bIsWSDOTRating = false;

   if ( !pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
      bIsWSDOTRating = false;

   if ( !pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
      bIsWSDOTRating = false;

   if ( !bIsWSDOTRating )
   {
      (*pPara) << _T("The load rating settings do not conform to the requirements specified in Chapter 13 of the WSDOT Bridge Design Manual.") << rptNewLine;
      (*pPara) << _T("Select Project | Load Rating Options to change the load rating settings to the required settings shown below.") << rptNewLine;

	   rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(3,_T("Load Rating Criteria"));

      // left justify all columns
      pTable->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      pTable->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
      pTable->SetColumnStyle(1,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      pTable->SetStripeRowColumnStyle(1,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
      pTable->SetColumnStyle(2,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      pTable->SetStripeRowColumnStyle(2,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

      RowIndexType row = 0;
      (*pPara) << pTable << rptNewLine;
      (*pTable)(row,0) << _T("");
      (*pTable)(row,1) << _T("Current Setting");
      (*pTable)(row,2) << _T("Required Setting");
      row++;

      (*pTable)(row,0) << _T("Load Rating Criteria");
      (*pTable)(row,1) << pRatingSpec->GetRatingSpecification();
      (*pTable)(row,2) << _T("WSDOT");
      row++;

      (*pTable)(row,0) << _T("Design Rating");
      (*pTable)(row,1) << ((!pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) || !pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating) ) ? _T("False") : _T("True"));
      (*pTable)(row,2) << _T("True");
      row++;

      (*pTable)(row,0) << _T("Legal Rating");
      (*pTable)(row,1) << ((!pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) || !pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) ) ? _T("False") : _T("True"));
      (*pTable)(row,2) << _T("True");
      row++;

      (*pTable)(row,0) << _T("Permit Rating");
      (*pTable)(row,1) << ((!pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) ) ? _T("False") : _T("True"));
      (*pTable)(row,2) << _T("True");
      row++;

      (*pTable)(row,0) << _T("Design Load Rating: Live Loads for Design");
      std::vector<std::_tstring>::iterator nameIter(design_permit_loads.begin());
      std::vector<std::_tstring>::iterator nameIterEnd(design_permit_loads.end());
      for ( ; nameIter != nameIterEnd; nameIter++ )
      {
         (*pTable)(row,1) << (*nameIter) << rptNewLine;
      }
      (*pTable)(row,1) << _T("");
      (*pTable)(row,2) << _T("HL-93");
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
      row;

      return pChapter;
   }


   INIT_UV_PROTOTYPE( rptLengthUnitValue, length, pDisplayUnits->GetSpanLengthUnit(),   true );

   sysDate date;
   GET_IFACE2(pBroker,IProjectProperties,pProjectProperties);

   (*pPara) << _T("Bridge Name: ") << pProjectProperties->GetBridgeName() << rptNewLine;
   (*pPara) << _T("Bridge Number: ") << pProjectProperties->GetBridgeId() << rptNewLine;
   (*pPara) << _T("Span Types : PCG") << rptNewLine;
   (*pPara) << _T("Bridge Length : ") << length.SetValue(pBridge->GetLength()) << rptNewLine;
   (*pPara) << _T("Rated By: ") << pProjectProperties->GetEngineer() << rptNewLine;
   (*pPara) << _T("Date : ") << date.AsString() << rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;

   rptParagraph* pRemarks = new rptParagraph;
   (*pRemarks) << _T("Remarks:") << rptNewLine;

   rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(4,_T(""));
   (*pPara) << pTable << rptNewLine;

   pTable->SetColumnStyle(0, pgsReportStyleHolder::GetTableCellStyle( CB_NONE | CJ_LEFT) );
   pTable->SetStripeRowColumnStyle(0, pgsReportStyleHolder::GetTableStripeRowCellStyle( CB_NONE | CJ_LEFT) );

   pTable->SetColumnStyle(3, pgsReportStyleHolder::GetTableCellStyle( CB_NONE | CJ_LEFT) );
   pTable->SetStripeRowColumnStyle(3, pgsReportStyleHolder::GetTableStripeRowCellStyle( CB_NONE | CJ_LEFT) );

   (*pTable)(0,0) << _T("Truck");
   (*pTable)(0,1) << _T("RF");
   (*pTable)(0,2) << Sub2(symbol(gamma),_T("LL"));
   (*pTable)(0,3)  << COLHDR(_T("Controlling Point") << rptNewLine << RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

   GET_IFACE2(pBroker,IArtifact,pIArtifact);

   RowIndexType row = pTable->GetNumberOfHeaderRows();

   GET_IFACE2(pBroker,IProductLoads,pProductLoads);

   pgsTypes::LiveLoadType llType = ::GetLiveLoadType(pgsTypes::lrLegal_Routine);
   VehicleIndexType nVehicles = pProductLoads->GetVehicleCount(llType);
   for ( VehicleIndexType vehIdx = 0; vehIdx < nVehicles; vehIdx++ )
   {
      ReportRatingFactor(pBroker,pTable,row++,pIArtifact->GetRatingArtifact(gdrLineIdx,pgsTypes::lrLegal_Routine,vehIdx),pDisplayUnits,pRemarks);
   }


   llType = ::GetLiveLoadType(pgsTypes::lrLegal_Special);
   nVehicles = pProductLoads->GetVehicleCount(llType);
   for ( VehicleIndexType vehIdx = 0; vehIdx < nVehicles; vehIdx++ )
   {
      ReportRatingFactor(pBroker,pTable,row++,pIArtifact->GetRatingArtifact(gdrLineIdx,pgsTypes::lrLegal_Special,vehIdx),pDisplayUnits,pRemarks);
   }

   // Current WSDOT default is to have no trucks in the Permit Routine case so there is nothing to report
   //llType = ::GetLiveLoadType(pgsTypes::lrPermit_Routine);
   //nVehicles = pProductLoads->GetVehicleCount(llType);
   //for ( VehicleIndexType vehIdx = 0; vehIdx < nVehicles; vehIdx++ )
   //{
   //   ReportRatingFactor(pBroker,pTable,row++,pIArtifact->GetRatingArtifact(gdrLineIdx,pgsTypes::lrPermit_Routine,vehIdx),pDisplayUnits,pRemarks);
   //}


   llType = ::GetLiveLoadType(pgsTypes::lrPermit_Special);
   nVehicles = pProductLoads->GetVehicleCount(llType);
   for ( VehicleIndexType vehIdx = 0; vehIdx < nVehicles; vehIdx++ )
   {
      ReportRatingFactor(pBroker,pTable,row++,pIArtifact->GetRatingArtifact(gdrLineIdx,pgsTypes::lrPermit_Special,vehIdx),pDisplayUnits,pRemarks);
   }

   pTable = pgsReportStyleHolder::CreateDefaultTable(3,_T(""));
   (*pPara) << pTable << rptNewLine;

   pTable->SetColumnStyle(0, pgsReportStyleHolder::GetTableCellStyle( CB_NONE | CJ_LEFT) );
   pTable->SetStripeRowColumnStyle(0, pgsReportStyleHolder::GetTableStripeRowCellStyle( CB_NONE | CJ_LEFT) );

   pTable->SetColumnStyle(2, pgsReportStyleHolder::GetTableCellStyle( CB_NONE | CJ_LEFT) );
   pTable->SetStripeRowColumnStyle(2, pgsReportStyleHolder::GetTableStripeRowCellStyle( CB_NONE | CJ_LEFT) );

   (*pTable)(0,0) << _T("NBI Rating");
   (*pTable)(0,1) << _T("RF");
   (*pTable)(0,2)  << COLHDR(_T("Controlling Point") << rptNewLine << RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

   row = 1;
   ReportRatingFactor2(pBroker,pTable,row++,_T("Inventory"),pIArtifact->GetRatingArtifact(gdrLineIdx,pgsTypes::lrDesign_Inventory,INVALID_INDEX),pDisplayUnits,pRemarks);
   ReportRatingFactor2(pBroker,pTable,row++,_T("Operating"),pIArtifact->GetRatingArtifact(gdrLineIdx,pgsTypes::lrDesign_Operating,INVALID_INDEX),pDisplayUnits,pRemarks);

   *pChapter << pRemarks;

   return pChapter;
}

CChapterBuilder* CLoadRatingSummaryChapterBuilder::Clone() const
{
   return new CLoadRatingSummaryChapterBuilder;
}

void CLoadRatingSummaryChapterBuilder::ReportRatingFactor(IBroker* pBroker,rptRcTable* pTable,RowIndexType row,const pgsRatingArtifact* pRatingArtifact,IEAFDisplayUnits* pDisplayUnits,rptParagraph* pRemarks) const
{
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(),   false );
   location.IncludeSpanAndGirder(true);

   rptRcScalar scalar;
   scalar.SetFormat( pDisplayUnits->GetScalarFormat().Format );
   scalar.SetWidth( pDisplayUnits->GetScalarFormat().Width );
   scalar.SetPrecision( pDisplayUnits->GetScalarFormat().Precision );

   rptCapacityToDemand rating_factor;

   const pgsMomentRatingArtifact* pPositiveMoment;
   const pgsMomentRatingArtifact* pNegativeMoment;
   const pgsShearRatingArtifact*  pShear;
   const pgsStressRatingArtifact* pStress;
   const pgsYieldStressRatioArtifact* pYieldStressPositiveMoment;
   const pgsYieldStressRatioArtifact* pYieldStressNegativeMoment;

   GET_IFACE2(pBroker,IBridge,pBridge);
   
   Float64 RF = pRatingArtifact->GetRatingFactorEx(&pPositiveMoment,&pNegativeMoment,&pShear,&pStress,&pYieldStressPositiveMoment,&pYieldStressNegativeMoment);
   if ( pPositiveMoment )
   {
      (*pTable)(row,0) << pPositiveMoment->GetVehicleName();
      
      if ( RF < 1 )
         (*pTable)(row,1) << RF_FAIL(rating_factor,RF);
      else
         (*pTable)(row,1) << RF_PASS(rating_factor,RF);

      (*pTable)(row,2) << scalar.SetValue(pPositiveMoment->GetLiveLoadFactor());

      pgsPointOfInterest poi = pPositiveMoment->GetPointOfInterest();
      Float64 endSize = pBridge->GetGirderStartConnectionLength(poi.GetSpan(),poi.GetGirder());
      (*pTable)(row,3) << location.SetValue(pgsTypes::BridgeSite3,poi,endSize) << _T(" (Positive Moment)");
   }
   else if ( pNegativeMoment )
   {
      (*pTable)(row,0) << pNegativeMoment->GetVehicleName();
      
      if ( RF < 1 )
         (*pTable)(row,1) << RF_FAIL(rating_factor,RF);
      else
         (*pTable)(row,1) << RF_PASS(rating_factor,RF);

      (*pTable)(row,2) << scalar.SetValue(pNegativeMoment->GetLiveLoadFactor());

      pgsPointOfInterest poi = pNegativeMoment->GetPointOfInterest();
      Float64 endSize = pBridge->GetGirderStartConnectionLength(poi.GetSpan(),poi.GetGirder());
      (*pTable)(row,3) << location.SetValue(pgsTypes::BridgeSite3,poi,endSize) << _T(" (Negative Moment)");
   }
   else if ( pShear )
   {
      (*pTable)(row,0) << pShear->GetVehicleName();
      
      if ( RF < 1 )
         (*pTable)(row,1) << RF_FAIL(rating_factor,RF);
      else
         (*pTable)(row,1) << RF_PASS(rating_factor,RF);

      (*pTable)(row,2) << scalar.SetValue(pShear->GetLiveLoadFactor());

      pgsPointOfInterest poi = pShear->GetPointOfInterest();
      Float64 endSize = pBridge->GetGirderStartConnectionLength(poi.GetSpan(),poi.GetGirder());
      (*pTable)(row,3) << location.SetValue(pgsTypes::BridgeSite3,poi,endSize) << _T(" (Shear)");
   }
   else if ( pStress )
   {
      (*pTable)(row,0) << pStress->GetVehicleName();
      
      if ( RF < 1 )
         (*pTable)(row,1) << RF_FAIL(rating_factor,RF);
      else
         (*pTable)(row,1) << RF_PASS(rating_factor,RF);

      (*pTable)(row,2) << scalar.SetValue(pStress->GetLiveLoadFactor());

      pgsPointOfInterest poi = pStress->GetPointOfInterest();
      Float64 endSize = pBridge->GetGirderStartConnectionLength(poi.GetSpan(),poi.GetGirder());
      (*pTable)(row,3) << location.SetValue(pgsTypes::BridgeSite3,poi,endSize) << _T(" (Stress)");
   }
   else if ( pYieldStressPositiveMoment )
   {
      (*pTable)(row,0) << pYieldStressPositiveMoment->GetVehicleName();
      
      if ( RF < 1 )
         (*pTable)(row,1) << RF_FAIL(rating_factor,RF);
      else
         (*pTable)(row,1) << RF_PASS(rating_factor,RF);

      (*pTable)(row,1) << rptNewLine << _T("(Stress Ratio)");

      (*pTable)(row,2) << scalar.SetValue(pYieldStressPositiveMoment->GetLiveLoadFactor());

      pgsPointOfInterest poi = pYieldStressPositiveMoment->GetPointOfInterest();
      Float64 endSize = pBridge->GetGirderStartConnectionLength(poi.GetSpan(),poi.GetGirder());
      (*pTable)(row,3) << location.SetValue(pgsTypes::BridgeSite3,poi,endSize) << _T(" (Yield Stress - Positive Moment)");

      if ( 0 < pYieldStressPositiveMoment->GetCrackingStressIncrement() )
      {
         (*pRemarks) << pYieldStressPositiveMoment->GetVehicleName() << _T(": Section is cracked for Service I limit state") << rptNewLine;
      }
   }
   else if ( pYieldStressNegativeMoment )
   {
      (*pTable)(row,0) << pYieldStressNegativeMoment->GetVehicleName();
      
      if ( RF < 1 )
         (*pTable)(row,1) << RF_FAIL(rating_factor,RF);
      else
         (*pTable)(row,1) << RF_PASS(rating_factor,RF);

      (*pTable)(row,1) << rptNewLine << _T("(Stress Ratio)");

      (*pTable)(row,2) << scalar.SetValue(pYieldStressNegativeMoment->GetLiveLoadFactor());

      pgsPointOfInterest poi = pYieldStressNegativeMoment->GetPointOfInterest();
      Float64 endSize = pBridge->GetGirderStartConnectionLength(poi.GetSpan(),poi.GetGirder());
      (*pTable)(row,3) << location.SetValue(pgsTypes::BridgeSite3,poi,endSize) << _T(" (Yield Stress - Negative Moment)");

      if ( 0 < pYieldStressNegativeMoment->GetCrackingStressIncrement() )
      {
         (*pRemarks) << pYieldStressPositiveMoment->GetVehicleName() << _T(": Section is cracked for Service I limit state") << rptNewLine;
      }
   }
}

void CLoadRatingSummaryChapterBuilder::ReportRatingFactor2(IBroker* pBroker,rptRcTable* pTable,RowIndexType row,LPCTSTR strTruck,const pgsRatingArtifact* pRatingArtifact,IEAFDisplayUnits* pDisplayUnits,rptParagraph* pRemarks) const
{
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(),   false );
   location.IncludeSpanAndGirder(true);

   rptRcScalar scalar;
   scalar.SetFormat( pDisplayUnits->GetScalarFormat().Format );
   scalar.SetWidth( pDisplayUnits->GetScalarFormat().Width );
   scalar.SetPrecision( pDisplayUnits->GetScalarFormat().Precision );

   rptCapacityToDemand rating_factor;

   const pgsMomentRatingArtifact* pPositiveMoment;
   const pgsMomentRatingArtifact* pNegativeMoment;
   const pgsShearRatingArtifact*  pShear;
   const pgsStressRatingArtifact* pStress;
   const pgsYieldStressRatioArtifact* pYieldStressPositiveMoment;
   const pgsYieldStressRatioArtifact* pYieldStressNegativeMoment;

   GET_IFACE2(pBroker,IBridge,pBridge);
   
   Float64 RF = pRatingArtifact->GetRatingFactorEx(&pPositiveMoment,&pNegativeMoment,&pShear,&pStress,&pYieldStressPositiveMoment,&pYieldStressNegativeMoment);
   if ( pPositiveMoment )
   {
      (*pTable)(row,0) << strTruck;
      
      if ( RF < 1 )
         (*pTable)(row,1) << RF_FAIL(rating_factor,RF);
      else
         (*pTable)(row,1) << RF_PASS(rating_factor,RF);


      pgsPointOfInterest poi = pPositiveMoment->GetPointOfInterest();
      Float64 endSize = pBridge->GetGirderStartConnectionLength(poi.GetSpan(),poi.GetGirder());
      (*pTable)(row,2) << location.SetValue(pgsTypes::BridgeSite3,poi,endSize) << _T(" (Positive Moment)");
   }
   else if ( pNegativeMoment )
   {
      (*pTable)(row,0) << strTruck;
      
      if ( RF < 1 )
         (*pTable)(row,1) << RF_FAIL(rating_factor,RF);
      else
         (*pTable)(row,1) << RF_PASS(rating_factor,RF);


      pgsPointOfInterest poi = pNegativeMoment->GetPointOfInterest();
      Float64 endSize = pBridge->GetGirderStartConnectionLength(poi.GetSpan(),poi.GetGirder());
      (*pTable)(row,2) << location.SetValue(pgsTypes::BridgeSite3,poi,endSize) << _T(" (Negative Moment)");
   }
   else if ( pShear )
   {
      (*pTable)(row,0) << strTruck;
      
      if ( RF < 1 )
         (*pTable)(row,1) << RF_FAIL(rating_factor,RF);
      else
         (*pTable)(row,1) << RF_PASS(rating_factor,RF);


      pgsPointOfInterest poi = pShear->GetPointOfInterest();
      Float64 endSize = pBridge->GetGirderStartConnectionLength(poi.GetSpan(),poi.GetGirder());
      (*pTable)(row,2) << location.SetValue(pgsTypes::BridgeSite3,poi,endSize) << _T(" (Shear)");
   }
   else if ( pStress )
   {
      (*pTable)(row,0) << strTruck;
      
      if ( RF < 1 )
         (*pTable)(row,1) << RF_FAIL(rating_factor,RF);
      else
         (*pTable)(row,1) << RF_PASS(rating_factor,RF);


      pgsPointOfInterest poi = pStress->GetPointOfInterest();
      Float64 endSize = pBridge->GetGirderStartConnectionLength(poi.GetSpan(),poi.GetGirder());
      (*pTable)(row,2) << location.SetValue(pgsTypes::BridgeSite3,poi,endSize) << _T(" (Stress)");
   }
   else if ( pYieldStressPositiveMoment )
   {
      (*pTable)(row,0) << strTruck;
      
      if ( RF < 1 )
         (*pTable)(row,1) << RF_FAIL(rating_factor,RF);
      else
         (*pTable)(row,1) << RF_PASS(rating_factor,RF);

      (*pTable)(row,1) << rptNewLine << _T("(Stress Ratio)");


      pgsPointOfInterest poi = pYieldStressPositiveMoment->GetPointOfInterest();
      Float64 endSize = pBridge->GetGirderStartConnectionLength(poi.GetSpan(),poi.GetGirder());
      (*pTable)(row,2) << location.SetValue(pgsTypes::BridgeSite3,poi,endSize) << _T(" (Yield Stress - Positive Moment)");
   }
   else if ( pYieldStressNegativeMoment )
   {
      (*pTable)(row,0) << strTruck;
      
      if ( RF < 1 )
         (*pTable)(row,1) << RF_FAIL(rating_factor,RF);
      else
         (*pTable)(row,1) << RF_PASS(rating_factor,RF);

      (*pTable)(row,1) << rptNewLine << _T("(Stress Ratio)");


      pgsPointOfInterest poi = pYieldStressNegativeMoment->GetPointOfInterest();
      Float64 endSize = pBridge->GetGirderStartConnectionLength(poi.GetSpan(),poi.GetGirder());
      (*pTable)(row,2) << location.SetValue(pgsTypes::BridgeSite3,poi,endSize) << _T(" (Yield Stress - Negative Moment)");
   }
}

