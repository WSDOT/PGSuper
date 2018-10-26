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

   std::vector<std::string>::iterator found;
   bool bIsWSDOTRating = true;
   GET_IFACE2(pBroker,ILiveLoads,pLiveLoads);
   std::vector<std::string> routine_legal_loads = pLiveLoads->GetLiveLoadNames(pgsTypes::lltLegalRating_Routine);
   if ( routine_legal_loads.size() == 0 || routine_legal_loads[0] != "AASHTO Legal Loads" )
   {
      bIsWSDOTRating = false;
   }

   std::vector<std::string> special_legal_loads = pLiveLoads->GetLiveLoadNames(pgsTypes::lltLegalRating_Special);
   if ( special_legal_loads.size() == 0 || special_legal_loads[0] != "Notional Rating Load (NRL)" )
   {
      bIsWSDOTRating = false;
   }

   std::vector<std::string> routine_permit_loads = pLiveLoads->GetLiveLoadNames(pgsTypes::lltPermitRating_Routine);
   if ( routine_permit_loads.size() != 0 )
   {
      bIsWSDOTRating = false;
   }

   std::vector<std::string> special_permit_loads = pLiveLoads->GetLiveLoadNames(pgsTypes::lltPermitRating_Special);
   if ( special_permit_loads.size() != 2 || (special_permit_loads.size() == 2 && special_permit_loads[0] != "OL1"  && special_permit_loads[1] != "OL2") )
   {
      bIsWSDOTRating = false;
   }

   std::vector<std::string> design_permit_loads = pLiveLoads->GetLiveLoadNames(pgsTypes::lltDesign);
   if ( design_permit_loads.size() == 0 || design_permit_loads[0] != "HL-93" )
   {
      bIsWSDOTRating = false;
   }

   GET_IFACE2(pBroker,IRatingSpecification,pRatingSpec);
   if ( pRatingSpec->GetRatingSpecification() != "WSDOT" )
      bIsWSDOTRating = false;

   if ( !bIsWSDOTRating )
   {
      (*pPara) << "The loading conditions do not conform to the requirements specified in Chapter 13 of the WSDOT Bridge Design Manual." << rptNewLine;
      (*pPara) << "The Load Rating Criteria must be WSDOT" << rptNewLine;
      (*pPara) << "Bridges must be rated for HL-93, AASHTO Legal Loads, Notational Rating Load (NRL), OL1 and OL2." << rptNewLine;
      (*pPara) << "Select Project | Load Rating Options to change the load rating settings" << rptNewLine;
      (*pPara) << rptNewLine;
      (*pPara) << "The current settings are:" << rptNewLine;
      (*pPara) << "Load Rating Criteria: " << pRatingSpec->GetRatingSpecification() << rptNewLine;
      (*pPara) << "Design Live Loads: ";
      std::vector<std::string>::iterator iter;
      for ( iter = design_permit_loads.begin(); iter != design_permit_loads.end(); iter++ )
      {
         if ( iter != design_permit_loads.begin() )
            (*pPara) << ", ";

         (*pPara) << (*iter);
      }
      (*pPara) << rptNewLine;

      (*pPara) << "Routine Commercial Traffic (Legal Live Loads): ";
      for ( iter = routine_legal_loads.begin(); iter != routine_legal_loads.end(); iter++ )
      {
         if ( iter != routine_legal_loads.begin() )
            (*pPara) << ", ";

         (*pPara) << (*iter);
      }
      (*pPara) << rptNewLine;

      (*pPara) << "Specialized Hauling Vehicles (Legal Live Loads): ";
      for ( iter = special_legal_loads.begin(); iter != special_legal_loads.end(); iter++ )
      {
         if ( iter != special_legal_loads.begin() )
            (*pPara) << ", ";

         (*pPara) << (*iter);
      }
      (*pPara) << rptNewLine;

      (*pPara) << "Routine/Annual Permit Vehicles: ";
      for ( iter = routine_permit_loads.begin(); iter != routine_permit_loads.end(); iter++ )
      {
         if ( iter != routine_permit_loads.begin() )
            (*pPara) << ", ";

         (*pPara) << (*iter);
      }
      (*pPara) << rptNewLine;

      (*pPara) << "Special/Limited Crossing Permit Vehicles: ";
      for ( iter = special_permit_loads.begin(); iter != special_permit_loads.end(); iter++ )
      {
         if ( iter != special_permit_loads.begin() )
            (*pPara) << ", ";

         (*pPara) << (*iter);
      }
      (*pPara) << rptNewLine;

      return pChapter;
   }


   INIT_UV_PROTOTYPE( rptLengthUnitValue, length, pDisplayUnits->GetSpanLengthUnit(),   true );

   sysDate date;
   GET_IFACE2(pBroker,IProjectProperties,pProjectProperties);
   GET_IFACE2(pBroker,IBridge,pBridge);

   (*pPara) << "Bridge Name: " << pProjectProperties->GetBridgeName() << rptNewLine;
   (*pPara) << "Bridge Number: " << pProjectProperties->GetBridgeId() << rptNewLine;
   (*pPara) << "Span Types : PCG" << rptNewLine;
   (*pPara) << "Bridge Length : " << length.SetValue(pBridge->GetLength()) << rptNewLine;
   (*pPara) << "Rated By: " << pProjectProperties->GetEngineer() << rptNewLine;
   (*pPara) << "Date : " << date.AsString() << rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;

   rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(4,"");
   (*pPara) << pTable << rptNewLine;

   pTable->SetColumnStyle(0, pgsReportStyleHolder::GetTableCellStyle( CB_NONE | CJ_LEFT) );
   pTable->SetStripeRowColumnStyle(0, pgsReportStyleHolder::GetTableStripeRowCellStyle( CB_NONE | CJ_LEFT) );

   pTable->SetColumnStyle(3, pgsReportStyleHolder::GetTableCellStyle( CB_NONE | CJ_LEFT) );
   pTable->SetStripeRowColumnStyle(3, pgsReportStyleHolder::GetTableStripeRowCellStyle( CB_NONE | CJ_LEFT) );

   (*pTable)(0,0) << "Truck";
   (*pTable)(0,1) << "RF";
   (*pTable)(0,2) << Sub2(symbol(gamma),"LL");
   (*pTable)(0,3)  << COLHDR("Controlling Point" << rptNewLine << RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

   GET_IFACE2(pBroker,IArtifact,pIArtifact);

   RowIndexType row = pTable->GetNumberOfHeaderRows();

   GET_IFACE2(pBroker,IProductLoads,pProductLoads);

   pgsTypes::LiveLoadType llType = ::GetLiveLoadType(pgsTypes::lrLegal_Routine);
   VehicleIndexType nVehicles = pProductLoads->GetVehicleCount(llType);
   for ( VehicleIndexType vehIdx = 0; vehIdx < nVehicles; vehIdx++ )
   {
      ReportRatingFactor(pBroker,pTable,row++,pIArtifact->GetRatingArtifact(gdrLineIdx,pgsTypes::lrLegal_Routine,vehIdx),pDisplayUnits);
   }


   llType = ::GetLiveLoadType(pgsTypes::lrLegal_Special);
   nVehicles = pProductLoads->GetVehicleCount(llType);
   for ( VehicleIndexType vehIdx = 0; vehIdx < nVehicles; vehIdx++ )
   {
      ReportRatingFactor(pBroker,pTable,row++,pIArtifact->GetRatingArtifact(gdrLineIdx,pgsTypes::lrLegal_Special,vehIdx),pDisplayUnits);
   }

   // Current WSDOT default is to have no trucks in the Permit Routine case so there is nothing to report
   //llType = ::GetLiveLoadType(pgsTypes::lrPermit_Routine);
   //nVehicles = pProductLoads->GetVehicleCount(llType);
   //for ( VehicleIndexType vehIdx = 0; vehIdx < nVehicles; vehIdx++ )
   //{
   //   ReportRatingFactor(pBroker,pTable,row++,pIArtifact->GetRatingArtifact(gdrLineIdx,pgsTypes::lrPermit_Routine,vehIdx),pDisplayUnits);
   //}


   llType = ::GetLiveLoadType(pgsTypes::lrPermit_Special);
   nVehicles = pProductLoads->GetVehicleCount(llType);
   for ( VehicleIndexType vehIdx = 0; vehIdx < nVehicles; vehIdx++ )
   {
      ReportRatingFactor(pBroker,pTable,row++,pIArtifact->GetRatingArtifact(gdrLineIdx,pgsTypes::lrPermit_Special,vehIdx),pDisplayUnits);
   }

   pTable = pgsReportStyleHolder::CreateDefaultTable(3,"");
   (*pPara) << pTable << rptNewLine;

   pTable->SetColumnStyle(0, pgsReportStyleHolder::GetTableCellStyle( CB_NONE | CJ_LEFT) );
   pTable->SetStripeRowColumnStyle(0, pgsReportStyleHolder::GetTableStripeRowCellStyle( CB_NONE | CJ_LEFT) );

   pTable->SetColumnStyle(2, pgsReportStyleHolder::GetTableCellStyle( CB_NONE | CJ_LEFT) );
   pTable->SetStripeRowColumnStyle(2, pgsReportStyleHolder::GetTableStripeRowCellStyle( CB_NONE | CJ_LEFT) );

   (*pTable)(0,0) << "NBI Rating";
   (*pTable)(0,1) << "RF";
   (*pTable)(0,2)  << COLHDR("Controlling Point" << rptNewLine << RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

   row = 1;
   ReportRatingFactor2(pBroker,pTable,row++,"Inventory",pIArtifact->GetRatingArtifact(gdrLineIdx,pgsTypes::lrDesign_Inventory,INVALID_INDEX),pDisplayUnits);
   ReportRatingFactor2(pBroker,pTable,row++,"Operating",pIArtifact->GetRatingArtifact(gdrLineIdx,pgsTypes::lrDesign_Operating,INVALID_INDEX),pDisplayUnits);

   return pChapter;
}

CChapterBuilder* CLoadRatingSummaryChapterBuilder::Clone() const
{
   return new CLoadRatingSummaryChapterBuilder;
}

void CLoadRatingSummaryChapterBuilder::ReportRatingFactor(IBroker* pBroker,rptRcTable* pTable,RowIndexType row,const pgsRatingArtifact* pRatingArtifact,IEAFDisplayUnits* pDisplayUnits) const
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
      (*pTable)(row,3) << location.SetValue(pgsTypes::BridgeSite3,poi,endSize) << " (Positive Moment)";
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
      (*pTable)(row,3) << location.SetValue(pgsTypes::BridgeSite3,poi,endSize) << " (Negative Moment)";
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
      (*pTable)(row,3) << location.SetValue(pgsTypes::BridgeSite3,poi,endSize) << " (Shear)";
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
      (*pTable)(row,3) << location.SetValue(pgsTypes::BridgeSite3,poi,endSize) << " (Stress)";
   }
   else if ( pYieldStressPositiveMoment )
   {
      (*pTable)(row,0) << pYieldStressPositiveMoment->GetVehicleName();
      
      if ( RF < 1 )
         (*pTable)(row,1) << RF_FAIL(rating_factor,RF);
      else
         (*pTable)(row,1) << RF_PASS(rating_factor,RF);

      (*pTable)(row,1) << rptNewLine << "(Stress Ratio)";

      (*pTable)(row,2) << scalar.SetValue(pYieldStressPositiveMoment->GetLiveLoadFactor());

      pgsPointOfInterest poi = pYieldStressPositiveMoment->GetPointOfInterest();
      Float64 endSize = pBridge->GetGirderStartConnectionLength(poi.GetSpan(),poi.GetGirder());
      (*pTable)(row,3) << location.SetValue(pgsTypes::BridgeSite3,poi,endSize) << " (Yield Stress - Positive Moment)";
   }
   else if ( pYieldStressNegativeMoment )
   {
      (*pTable)(row,0) << pYieldStressNegativeMoment->GetVehicleName();
      
      if ( RF < 1 )
         (*pTable)(row,1) << RF_FAIL(rating_factor,RF);
      else
         (*pTable)(row,1) << RF_PASS(rating_factor,RF);

      (*pTable)(row,1) << rptNewLine << "(Stress Ratio)";

      (*pTable)(row,2) << scalar.SetValue(pYieldStressNegativeMoment->GetLiveLoadFactor());

      pgsPointOfInterest poi = pYieldStressNegativeMoment->GetPointOfInterest();
      Float64 endSize = pBridge->GetGirderStartConnectionLength(poi.GetSpan(),poi.GetGirder());
      (*pTable)(row,3) << location.SetValue(pgsTypes::BridgeSite3,poi,endSize) << " (Yield Stress - Negative Moment)";
   }
}

void CLoadRatingSummaryChapterBuilder::ReportRatingFactor2(IBroker* pBroker,rptRcTable* pTable,RowIndexType row,const char* strTruck,const pgsRatingArtifact* pRatingArtifact,IEAFDisplayUnits* pDisplayUnits) const
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
      (*pTable)(row,2) << location.SetValue(pgsTypes::BridgeSite3,poi,endSize) << " (Positive Moment)";
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
      (*pTable)(row,2) << location.SetValue(pgsTypes::BridgeSite3,poi,endSize) << " (Negative Moment)";
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
      (*pTable)(row,2) << location.SetValue(pgsTypes::BridgeSite3,poi,endSize) << " (Shear)";
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
      (*pTable)(row,2) << location.SetValue(pgsTypes::BridgeSite3,poi,endSize) << " (Stress)";
   }
   else if ( pYieldStressPositiveMoment )
   {
      (*pTable)(row,0) << strTruck;
      
      if ( RF < 1 )
         (*pTable)(row,1) << RF_FAIL(rating_factor,RF);
      else
         (*pTable)(row,1) << RF_PASS(rating_factor,RF);

      (*pTable)(row,1) << rptNewLine << "(Stress Ratio)";


      pgsPointOfInterest poi = pYieldStressPositiveMoment->GetPointOfInterest();
      Float64 endSize = pBridge->GetGirderStartConnectionLength(poi.GetSpan(),poi.GetGirder());
      (*pTable)(row,2) << location.SetValue(pgsTypes::BridgeSite3,poi,endSize) << " (Yield Stress - Positive Moment)";
   }
   else if ( pYieldStressNegativeMoment )
   {
      (*pTable)(row,0) << strTruck;
      
      if ( RF < 1 )
         (*pTable)(row,1) << RF_FAIL(rating_factor,RF);
      else
         (*pTable)(row,1) << RF_PASS(rating_factor,RF);

      (*pTable)(row,1) << rptNewLine << "(Stress Ratio)";


      pgsPointOfInterest poi = pYieldStressNegativeMoment->GetPointOfInterest();
      Float64 endSize = pBridge->GetGirderStartConnectionLength(poi.GetSpan(),poi.GetGirder());
      (*pTable)(row,2) << location.SetValue(pgsTypes::BridgeSite3,poi,endSize) << " (Yield Stress - Negative Moment)";
   }
}
