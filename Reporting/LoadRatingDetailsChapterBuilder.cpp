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
#include <Reporting\LoadRatingDetailsChapterBuilder.h>

#include <IFace\AnalysisResults.h>
#include <IFace\Artifact.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\Bridge.h>
#include <IFace\RatingSpecification.h>

#include <PgsExt\RatingArtifact.h>
#include <PgsExt\CapacityToDemand.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/****************************************************************************
CLASS
   CLoadRatingDetailsChapterBuilder
****************************************************************************/

CLoadRatingDetailsChapterBuilder::CLoadRatingDetailsChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

LPCTSTR CLoadRatingDetailsChapterBuilder::GetName() const
{
   return TEXT("Load Rating Details");
}

rptChapter* CLoadRatingDetailsChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CGirderReportSpecification* pGdrRptSpec = dynamic_cast<CGirderReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pGdrRptSpec->GetBroker(&pBroker);
   GirderIndexType gdrLineIdx = pGdrRptSpec->GetGirder();

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   ReportRatingDetails(pChapter,pBroker,gdrLineIdx,pgsTypes::lrDesign_Inventory);
   ReportRatingDetails(pChapter,pBroker,gdrLineIdx,pgsTypes::lrDesign_Operating);
   ReportRatingDetails(pChapter,pBroker,gdrLineIdx,pgsTypes::lrLegal_Routine);
   ReportRatingDetails(pChapter,pBroker,gdrLineIdx,pgsTypes::lrLegal_Special);
   ReportRatingDetails(pChapter,pBroker,gdrLineIdx,pgsTypes::lrPermit_Routine);
   ReportRatingDetails(pChapter,pBroker,gdrLineIdx,pgsTypes::lrPermit_Special);

   return pChapter;
}

CChapterBuilder* CLoadRatingDetailsChapterBuilder::Clone() const
{
   return new CLoadRatingDetailsChapterBuilder;
}

void CLoadRatingDetailsChapterBuilder::ReportRatingDetails(rptChapter* pChapter,IBroker* pBroker,GirderIndexType gdrLineIdx,pgsTypes::LoadRatingType ratingType) const
{
   USES_CONVERSION;

   GET_IFACE2(pBroker,IProductLoads,pProductLoads);
   GET_IFACE2(pBroker,IArtifact,pArtifact);
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IRatingSpecification,pRatingSpec);

   if ( !pRatingSpec->IsRatingEnabled(ratingType) )
      return;

   bool bNegMoments = pBridge->ProcessNegativeMoments(ALL_SPANS);

   CComBSTR bstrLiveLoadType = ::GetLiveLoadTypeName(ratingType);
   rptParagraph* pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pPara;
   pPara->SetName(OLE2A(bstrLiveLoadType));
   *pPara << pPara->GetName() << rptNewLine;

   pgsTypes::LiveLoadType llType = ::GetLiveLoadType(ratingType);
   VehicleIndexType nVehicles = pProductLoads->GetVehicleCount(llType);
   VehicleIndexType firstVehicleIdx = (ratingType == pgsTypes::lrDesign_Inventory || ratingType == pgsTypes::lrDesign_Operating ? INVALID_INDEX : 0);
   VehicleIndexType lastVehicleIdx  = (ratingType == pgsTypes::lrDesign_Inventory || ratingType == pgsTypes::lrDesign_Operating ? 0 : nVehicles);
   for ( VehicleIndexType vehIdx = firstVehicleIdx; vehIdx < lastVehicleIdx; vehIdx++ )
   {
      const pgsRatingArtifact* pRatingArtifact = pArtifact->GetRatingArtifact(gdrLineIdx,ratingType,vehIdx);
      if ( pRatingArtifact )
      {
         std::string strVehicleName = pProductLoads->GetLiveLoadName(llType,vehIdx);

         pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
         *pChapter << pPara;
         *pPara << strVehicleName << rptNewLine;

         MomentRatingDetails(pChapter,pBroker,gdrLineIdx,true,pRatingArtifact);
         if ( bNegMoments )
            MomentRatingDetails(pChapter,pBroker,gdrLineIdx,false,pRatingArtifact);
         
         if ( pRatingSpec->RateForShear(ratingType) )
            ShearRatingDetails(pChapter,pBroker,gdrLineIdx,pRatingArtifact);

         if ( pRatingSpec->RateForStress(ratingType) )
         {
            if ( ratingType == pgsTypes::lrPermit_Routine || ratingType == pgsTypes::lrPermit_Special )
            {
               ReinforcementYieldingDetails(pChapter,pBroker,gdrLineIdx,true,pRatingArtifact);
               if ( bNegMoments )
                  ReinforcementYieldingDetails(pChapter,pBroker,gdrLineIdx,false,pRatingArtifact);
            }
            else
            {
               StressRatingDetails(pChapter,pBroker,gdrLineIdx,pRatingArtifact);
            }
         }

         if ( (ratingType == pgsTypes::lrLegal_Routine || ratingType == pgsTypes::lrLegal_Special) && 
               pRatingArtifact->GetRatingFactor() < 1 
            )
         {
            LoadPostingDetails(pChapter,pBroker,gdrLineIdx,pRatingArtifact);
         }
      }
   }
}

void CLoadRatingDetailsChapterBuilder::MomentRatingDetails(rptChapter* pChapter,IBroker* pBroker,GirderIndexType gdrLineIdx,bool bPositiveMoment,const pgsRatingArtifact* pRatingArtifact) const
{
   rptParagraph* pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pPara;
   if ( bPositiveMoment )
      *pPara << "Rating for Positive Moment" << rptNewLine;
   else
      *pPara << "Rating for Negative Moment" << rptNewLine;

   *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + "MomentRatingEquation.png" ) << rptNewLine;

   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(13,"");
   
   table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
   table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   *pPara << table << rptNewLine;

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(),   false );
   location.IncludeSpanAndGirder(true);

   INIT_UV_PROTOTYPE( rptMomentUnitValue, moment,   pDisplayUnits->GetMomentUnit(),       false );

   rptRcScalar scalar;
   scalar.SetFormat( pDisplayUnits->GetScalarFormat().Format );
   scalar.SetWidth( pDisplayUnits->GetScalarFormat().Width );
   scalar.SetPrecision( pDisplayUnits->GetScalarFormat().Precision );

   rptCapacityToDemand rating_factor;

   GET_IFACE2(pBroker,IBridge,pBridge);
   ColumnIndexType col = 0;

   (*table)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table)(0,col++) << Sub2(symbol(phi),"c");
   (*table)(0,col++) << Sub2(symbol(phi),"s");
   (*table)(0,col++) << Sub2(symbol(phi),"n");
   (*table)(0,col++) << "K";
   (*table)(0,col++) << COLHDR(Sub2("M","n"), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   (*table)(0,col++) << Sub2(symbol(gamma),"DC");
   (*table)(0,col++) << COLHDR(Sub2("M","DC"), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   (*table)(0,col++) << Sub2(symbol(gamma),"DW");
   (*table)(0,col++) << COLHDR(Sub2("M","DW"), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   (*table)(0,col++) << Sub2(symbol(gamma),"LL");
   (*table)(0,col++) << COLHDR(Sub2("M","LL+IM"), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   (*table)(0,col++) << "RF";

   pgsRatingArtifact::MomentRatings artifacts = pRatingArtifact->GetMomentRatings(bPositiveMoment);

   RowIndexType row = 1;
   pgsRatingArtifact::MomentRatings::iterator iter;
   for ( iter = artifacts.begin(); iter != artifacts.end(); iter++ )
   {
      col = 0;
      const pgsPointOfInterest& poi = iter->first;
      pgsMomentRatingArtifact& artifact = iter->second;

      SpanIndexType spanIdx = poi.GetSpan();
      GirderIndexType gdrIdx = poi.GetGirder();
      Float64 end_size = pBridge->GetGirderStartConnectionLength(spanIdx,gdrIdx);

      (*table)(row,col++) << location.SetValue( pgsTypes::BridgeSite3, poi, end_size );
      (*table)(row,col++) << scalar.SetValue(artifact.GetConditionFactor());
      (*table)(row,col++) << scalar.SetValue(artifact.GetSystemFactor());
      (*table)(row,col++) << scalar.SetValue(artifact.GetCapacityReductionFactor());
      (*table)(row,col++) << scalar.SetValue(artifact.GetMinimumReinforcementFactor());
      (*table)(row,col++) << moment.SetValue(artifact.GetNominalMomentCapacity());
      (*table)(row,col++) << scalar.SetValue(artifact.GetDeadLoadFactor());
      (*table)(row,col++) << moment.SetValue(artifact.GetDeadLoadMoment());
      (*table)(row,col++) << scalar.SetValue(artifact.GetWearingSurfaceFactor());
      (*table)(row,col++) << moment.SetValue(artifact.GetWearingSurfaceMoment());
      (*table)(row,col++) << scalar.SetValue(artifact.GetLiveLoadFactor());
      (*table)(row,col++) << moment.SetValue(artifact.GetLiveLoadMoment());

      Float64 RF = artifact.GetRatingFactor();
      if ( RF < 1 )
         (*table)(row,col++) << RF_FAIL(rating_factor,RF);
      else
         (*table)(row,col++) << RF_PASS(rating_factor,RF);

      row++;
   }
}

void CLoadRatingDetailsChapterBuilder::ShearRatingDetails(rptChapter* pChapter,IBroker* pBroker,GirderIndexType gdrLineIdx,const pgsRatingArtifact* pRatingArtifact) const
{
   rptParagraph* pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pPara;
   *pPara << "Rating for Shear" << rptNewLine;

   *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + "ShearRatingEquation.png" ) << rptNewLine;

   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(12,"");
   
   table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
   table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   *pPara << table << rptNewLine;

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(),   false );
   location.IncludeSpanAndGirder(true);

   INIT_UV_PROTOTYPE( rptForceUnitValue,  shear,    pDisplayUnits->GetShearUnit(),        false );

   rptRcScalar scalar;
   scalar.SetFormat( pDisplayUnits->GetScalarFormat().Format );
   scalar.SetWidth( pDisplayUnits->GetScalarFormat().Width );
   scalar.SetPrecision( pDisplayUnits->GetScalarFormat().Precision );

   rptCapacityToDemand rating_factor;

   GET_IFACE2(pBroker,IBridge,pBridge);

   ColumnIndexType col = 0;

   (*table)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table)(0,col++) << Sub2(symbol(phi),"c");
   (*table)(0,col++) << Sub2(symbol(phi),"s");
   (*table)(0,col++) << Sub2(symbol(phi),"n");
   (*table)(0,col++) << COLHDR(Sub2("V","n"), rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   (*table)(0,col++) << Sub2(symbol(gamma),"DC");
   (*table)(0,col++) << COLHDR(Sub2("V","DC"), rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   (*table)(0,col++) << Sub2(symbol(gamma),"DW");
   (*table)(0,col++) << COLHDR(Sub2("V","DW"), rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   (*table)(0,col++) << Sub2(symbol(gamma),"LL");
   (*table)(0,col++) << COLHDR(Sub2("V","LL+IM"), rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   (*table)(0,col++) << "RF";

   pgsRatingArtifact::ShearRatings artifacts = pRatingArtifact->GetShearRatings();

   RowIndexType row = 1;
   pgsRatingArtifact::ShearRatings::iterator iter;
   for ( iter = artifacts.begin(); iter != artifacts.end(); iter++ )
   {
      col = 0;
      const pgsPointOfInterest& poi = iter->first;
      pgsShearRatingArtifact& artifact = iter->second;

      SpanIndexType spanIdx = poi.GetSpan();
      GirderIndexType gdrIdx = poi.GetGirder();
      Float64 end_size = pBridge->GetGirderStartConnectionLength(spanIdx,gdrIdx);

      (*table)(row,col++) << location.SetValue( pgsTypes::BridgeSite3, poi, end_size );
      (*table)(row,col++) << scalar.SetValue(artifact.GetConditionFactor());
      (*table)(row,col++) << scalar.SetValue(artifact.GetSystemFactor());
      (*table)(row,col++) << scalar.SetValue(artifact.GetCapacityReductionFactor());
      (*table)(row,col++) << shear.SetValue(artifact.GetNominalShearCapacity());
      (*table)(row,col++) << scalar.SetValue(artifact.GetDeadLoadFactor());
      (*table)(row,col++) << shear.SetValue(artifact.GetDeadLoadShear());
      (*table)(row,col++) << scalar.SetValue(artifact.GetWearingSurfaceFactor());
      (*table)(row,col++) << shear.SetValue(artifact.GetWearingSurfaceShear());
      (*table)(row,col++) << scalar.SetValue(artifact.GetLiveLoadFactor());
      (*table)(row,col++) << shear.SetValue(artifact.GetLiveLoadShear());

      Float64 RF = artifact.GetRatingFactor();
      if ( RF < 1 )
         (*table)(row,col++) << RF_FAIL(rating_factor,RF);
      else
         (*table)(row,col++) << RF_PASS(rating_factor,RF);

      row++;
   }
}

void CLoadRatingDetailsChapterBuilder::StressRatingDetails(rptChapter* pChapter,IBroker* pBroker,GirderIndexType gdrLineIdx,const pgsRatingArtifact* pRatingArtifact) const
{
   rptParagraph* pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pPara;
   *pPara << "Rating for Stress" << rptNewLine;

   *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + "StressRatingEquation.png" ) << rptNewLine;

   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(10,"");
   
   table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
   table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   *pPara << table << rptNewLine;

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(),   false );
   location.IncludeSpanAndGirder(true);

   INIT_UV_PROTOTYPE( rptStressUnitValue,  stress,    pDisplayUnits->GetStressUnit(),        false );

   rptRcScalar scalar;
   scalar.SetFormat( pDisplayUnits->GetScalarFormat().Format );
   scalar.SetWidth( pDisplayUnits->GetScalarFormat().Width );
   scalar.SetPrecision( pDisplayUnits->GetScalarFormat().Precision );

   rptCapacityToDemand rating_factor;

   GET_IFACE2(pBroker,IBridge,pBridge);

   ColumnIndexType col = 0;

   (*table)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table)(0,col++) << Sub2("f","r");
   (*table)(0,col++) << Sub2(symbol(gamma),"DC");
   (*table)(0,col++) << COLHDR(Sub2("f","DC"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,col++) << COLHDR(Sub2("f","PS"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,col++) << Sub2(symbol(gamma),"DW");
   (*table)(0,col++) << COLHDR(Sub2("f","DW"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,col++) << Sub2(symbol(gamma),"LL");
   (*table)(0,col++) << COLHDR(Sub2("f","LL+IM"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,col++) << "RF";

   pgsRatingArtifact::StressRatings artifacts = pRatingArtifact->GetStressRatings();

   RowIndexType row = 1;
   pgsRatingArtifact::StressRatings::iterator iter;
   for ( iter = artifacts.begin(); iter != artifacts.end(); iter++ )
   {
      col = 0;
      const pgsPointOfInterest& poi = iter->first;
      pgsStressRatingArtifact& artifact = iter->second;

      SpanIndexType spanIdx = poi.GetSpan();
      GirderIndexType gdrIdx = poi.GetGirder();
      Float64 end_size = pBridge->GetGirderStartConnectionLength(spanIdx,gdrIdx);

      (*table)(row,col++) << location.SetValue( pgsTypes::BridgeSite3,  poi, end_size );
      (*table)(row,col++) << stress.SetValue(artifact.GetAllowableStress());
      (*table)(row,col++) << scalar.SetValue(artifact.GetDeadLoadFactor());
      (*table)(row,col++) << stress.SetValue(artifact.GetDeadLoadStress());
      (*table)(row,col++) << stress.SetValue(artifact.GetPrestressStress());
      (*table)(row,col++) << scalar.SetValue(artifact.GetWearingSurfaceFactor());
      (*table)(row,col++) << stress.SetValue(artifact.GetWearingSurfaceStress());
      (*table)(row,col++) << scalar.SetValue(artifact.GetLiveLoadFactor());
      (*table)(row,col++) << stress.SetValue(artifact.GetLiveLoadStress());

      Float64 RF = artifact.GetRatingFactor();
      if ( RF < 1 )
         (*table)(row,col++) << RF_FAIL(rating_factor,RF);
      else
         (*table)(row,col++) << RF_PASS(rating_factor,RF);

      row++;
   }
}

void CLoadRatingDetailsChapterBuilder::ReinforcementYieldingDetails(rptChapter* pChapter,IBroker* pBroker,GirderIndexType gdrLineIdx,bool bPositiveMoment,const pgsRatingArtifact* pRatingArtifact) const
{
   rptParagraph* pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pPara;
   if ( bPositiveMoment )
      *pPara << "Check Reinforcement Yielding for Positive Moment" << rptNewLine;
   else
      *pPara << "Check Reinforcement Yielding for Negative Moment" << rptNewLine;

   *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + "ReinforcementYieldingEquation.png" ) << rptNewLine;

   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(16,"");
   
   table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
   table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   //*pPara << table << rptNewLine; // don't add table here... see below

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   INIT_UV_PROTOTYPE( rptPointOfInterest,  location, pDisplayUnits->GetSpanLengthUnit(),      false );
   location.IncludeSpanAndGirder(true);

   INIT_UV_PROTOTYPE( rptLengthUnitValue,  dim,      pDisplayUnits->GetComponentDimUnit(),    false );
   INIT_UV_PROTOTYPE( rptStressUnitValue,  stress,   pDisplayUnits->GetStressUnit(),          false );
   INIT_UV_PROTOTYPE( rptMomentUnitValue,  moment,   pDisplayUnits->GetMomentUnit(),          false );
   INIT_UV_PROTOTYPE( rptLength4UnitValue, mom_i,    pDisplayUnits->GetMomentOfInertiaUnit(), false );
   INIT_UV_PROTOTYPE( rptStressUnitValue,  mod_e,    pDisplayUnits->GetModEUnit(),            false );

   rptRcScalar scalar;
   scalar.SetFormat( pDisplayUnits->GetScalarFormat().Format );
   scalar.SetWidth( pDisplayUnits->GetScalarFormat().Width );
   scalar.SetPrecision( pDisplayUnits->GetScalarFormat().Precision );

   rptCapacityToDemand rating_factor;

   GET_IFACE2(pBroker,IBridge,pBridge);

   ColumnIndexType col = 0;

   (*table)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table)(0,col++) << COLHDR(Sub2("M","DC"), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   (*table)(0,col++) << COLHDR(Sub2("M","DW"), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   (*table)(0,col++) << COLHDR(Sub2("M","LL+IM"), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   (*table)(0,col++) << COLHDR(Sub2("M","cr"), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   (*table)(0,col++) << COLHDR(Sub2("M","bcr"), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   (*table)(0,col++) << COLHDR(Sub2("d","ps"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,col++) << COLHDR("c", rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,col++) << COLHDR(Sub2("I","cr"), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit() );
   (*table)(0,col++) << COLHDR(Sub2("f","pe"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,col++) << COLHDR(Sub2("f","bcr"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,col++) << COLHDR(Sub2("E","s"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,col++) << COLHDR(Sub2("E","g"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,col++) << COLHDR(Sub2("f","s"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,col++) << COLHDR(Sub2("f","r"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,col++) << "Stress" << rptNewLine << "Ratio" << rptNewLine << Sub2("f","r") << "/" << Sub2("f","s");

   pgsRatingArtifact::YieldStressRatios artifacts = pRatingArtifact->GetYieldStressRatios(bPositiveMoment);

   pPara = new rptParagraph;
   *pChapter << pPara;

   *pPara << Sub2(symbol(gamma),"DC") << " = " << scalar.SetValue(artifacts[0].second.GetDeadLoadFactor()) << rptNewLine;
   *pPara << Sub2(symbol(gamma),"DW") << " = " << scalar.SetValue(artifacts[0].second.GetWearingSurfaceFactor()) << rptNewLine;
   *pPara << Sub2(symbol(gamma),"LL") << " = " << scalar.SetValue(artifacts[0].second.GetLiveLoadFactor()) << rptNewLine;
   *pPara << table << rptNewLine;

   RowIndexType row = 1;
   pgsRatingArtifact::YieldStressRatios::iterator iter;
   for ( iter = artifacts.begin(); iter != artifacts.end(); iter++ )
   {
      col = 0;
      const pgsPointOfInterest& poi = iter->first;
      pgsYieldStressRatioArtifact& artifact = iter->second;

      SpanIndexType spanIdx = poi.GetSpan();
      GirderIndexType gdrIdx = poi.GetGirder();
      Float64 end_size = pBridge->GetGirderStartConnectionLength(spanIdx,gdrIdx);

      (*table)(row,col++) << location.SetValue( pgsTypes::BridgeSite3,  poi, end_size );
      (*table)(row,col++) << moment.SetValue(artifact.GetDeadLoadMoment());
      (*table)(row,col++) << moment.SetValue(artifact.GetWearingSurfaceMoment());
      (*table)(row,col++) << moment.SetValue(artifact.GetLiveLoadMoment());
      (*table)(row,col++) << moment.SetValue(artifact.GetCrackingMoment());
      (*table)(row,col++) << moment.SetValue(artifact.GetExcessMoment());
      (*table)(row,col++) << dim.SetValue(artifact.GetReinforcementDepth());
      (*table)(row,col++) << dim.SetValue(artifact.GetCrackDepth());
      (*table)(row,col++) << mom_i.SetValue(artifact.GetIcr());
      (*table)(row,col++) << stress.SetValue(artifact.GetEffectivePrestress());
      (*table)(row,col++) << stress.SetValue(artifact.GetCrackingStressIncrement());
      (*table)(row,col++) << mod_e.SetValue(artifact.GetEs());
      (*table)(row,col++) << mod_e.SetValue(artifact.GetEg());
      (*table)(row,col++) << stress.SetValue(artifact.GetStrandStress());
      (*table)(row,col++) << stress.SetValue(artifact.GetAllowableStress());

      Float64 stress_ratio = artifact.GetStressRatio();
      if ( stress_ratio < 1 )
         (*table)(row,col++) << RF_FAIL(rating_factor,stress_ratio);
      else
         (*table)(row,col++) << RF_PASS(rating_factor,stress_ratio);

      row++;
   }
}

void CLoadRatingDetailsChapterBuilder::LoadPostingDetails(rptChapter* pChapter,IBroker* pBroker,GirderIndexType gdrLineIdx,const pgsRatingArtifact* pRatingArtifact) const
{
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   rptParagraph* pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pPara;
   *pPara << "Load Posting Analysis Details [MBE 6A.8]" << rptNewLine;

   *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + "SafePostingLoad.png" ) << rptNewLine;

   INIT_UV_PROTOTYPE( rptForceUnitValue, tonnage, pDisplayUnits->GetTonnageUnit(), false );
   rptCapacityToDemand rating_factor;

   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(5,"");
   
   table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
   table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   *pPara << table << rptNewLine;

   ColumnIndexType col = 0;
   RowIndexType row = 0;

   (*table)(row,col++) << "Truck";
   (*table)(row,col++) << COLHDR("Weight", rptForceUnitTag, pDisplayUnits->GetTonnageUnit() );
   (*table)(row,col++) << "RF";
   (*table)(row,col++) << COLHDR("Safe" << rptNewLine << "Load" << rptNewLine << "Capacity", rptForceUnitTag, pDisplayUnits->GetTonnageUnit() );
   (*table)(row,col++) << COLHDR("Safe" << rptNewLine << "Posting" << rptNewLine << "Load", rptForceUnitTag, pDisplayUnits->GetTonnageUnit() );

   row++;
   col = 0;

   Float64 posting_load, W, RF;
   std::string strName;
   pRatingArtifact->GetSafePostingLoad(&posting_load,&W,&RF,&strName);
   (*table)(row,col++) << strName;
   (*table)(row,col++) << tonnage.SetValue(W);

   if ( RF < 1 )
      (*table)(row,col++) << RF_FAIL(rating_factor,RF);
   else
      (*table)(row,col++) << RF_PASS(rating_factor,RF);

   (*table)(row,col++) << tonnage.SetValue(::FloorOff(W*RF,0.01));
   if ( RF < 1 )
      (*table)(row,col++) << tonnage.SetValue(posting_load);
   else
      (*table)(row,col++) << "-";
}
