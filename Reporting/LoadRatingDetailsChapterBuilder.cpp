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
#include <Reporting\LoadRatingDetailsChapterBuilder.h>

#include <IFace\AnalysisResults.h>
#include <IFace\Artifact.h>

#include <IFace\Bridge.h>
#include <IFace\RatingSpecification.h>
#include <IFace\DistributionFactors.h>

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
   CGirderLineReportSpecification* pGdrLineRptSpec = dynamic_cast<CGirderLineReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   CGirderKey girderKey;

   if ( pGdrRptSpec )
   {
      pGdrRptSpec->GetBroker(&pBroker);
      girderKey = pGdrRptSpec->GetGirderKey();
   }
   else if ( pGdrLineRptSpec)
   {
      pGdrLineRptSpec->GetBroker(&pBroker);
      girderKey = pGdrLineRptSpec->GetGirderKey();
   }
   else
   {
      ATLASSERT(false); // not expecting a different kind of report spec
   }

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   ReportRatingDetails(pChapter,pBroker,girderKey,pgsTypes::lrDesign_Inventory);
   ReportRatingDetails(pChapter,pBroker,girderKey,pgsTypes::lrDesign_Operating);
   ReportRatingDetails(pChapter,pBroker,girderKey,pgsTypes::lrLegal_Routine);
   ReportRatingDetails(pChapter,pBroker,girderKey,pgsTypes::lrLegal_Special);
   ReportRatingDetails(pChapter,pBroker,girderKey,pgsTypes::lrPermit_Routine);
   ReportRatingDetails(pChapter,pBroker,girderKey,pgsTypes::lrPermit_Special);

   return pChapter;
}

CChapterBuilder* CLoadRatingDetailsChapterBuilder::Clone() const
{
   return new CLoadRatingDetailsChapterBuilder;
}

void CLoadRatingDetailsChapterBuilder::ReportRatingDetails(rptChapter* pChapter,IBroker* pBroker,const CGirderKey& girderKey,pgsTypes::LoadRatingType ratingType) const
{
   USES_CONVERSION;

   GET_IFACE2(pBroker,IRatingSpecification,pRatingSpec);

   if ( !pRatingSpec->IsRatingEnabled(ratingType) )
   {
      return;
   }

   GET_IFACE2(pBroker,IProductLoads,pProductLoads);
   GET_IFACE2(pBroker,IBridge,pBridge);

   pgsTypes::LiveLoadType llType = ::GetLiveLoadType(ratingType);

   bool bNegMoments = pBridge->ProcessNegativeMoments(ALL_SPANS);

   CComBSTR bstrLiveLoadType = ::GetLiveLoadTypeName(ratingType);
   rptParagraph* pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pPara;
   pPara->SetName(OLE2T(bstrLiveLoadType));
   *pPara << pPara->GetName() << rptNewLine;

   std::_tstring strName = pProductLoads->GetLiveLoadName(llType,0);
   if ( strName == _T("No Live Load Defined") )
   {
      pPara = new rptParagraph;
      *pChapter << pPara;
      *pPara << strName << rptNewLine;
      return;
   }

   GET_IFACE2(pBroker,IArtifact,pArtifact);

   VehicleIndexType nVehicles = pProductLoads->GetVehicleCount(llType);
   VehicleIndexType firstVehicleIdx = 0;
   VehicleIndexType lastVehicleIdx  = (ratingType == pgsTypes::lrDesign_Inventory || ratingType == pgsTypes::lrDesign_Operating ? 0 : nVehicles-1);
   for ( VehicleIndexType vehIdx = firstVehicleIdx; vehIdx <= lastVehicleIdx; vehIdx++ )
   {
      const pgsRatingArtifact* pRatingArtifact = pArtifact->GetRatingArtifact(girderKey,ratingType,
         (ratingType == pgsTypes::lrDesign_Inventory || ratingType == pgsTypes::lrDesign_Operating) ? INVALID_INDEX : vehIdx);

      if ( pRatingArtifact )
      {
         std::_tstring strVehicleName = pProductLoads->GetLiveLoadName(llType,vehIdx);

         pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
         *pChapter << pPara;
         *pPara << strVehicleName << rptNewLine;

         MomentRatingDetails(pChapter,pBroker,girderKey,true,pRatingArtifact);
         if ( bNegMoments )
         {
            MomentRatingDetails(pChapter,pBroker,girderKey,false,pRatingArtifact);
         }
         
         if ( pRatingSpec->RateForShear(ratingType) )
         {
            ShearRatingDetails(pChapter,pBroker,girderKey,pRatingArtifact);
         }

         if ( pRatingSpec->RateForStress(ratingType) )
         {
            if ( ratingType == pgsTypes::lrPermit_Routine || ratingType == pgsTypes::lrPermit_Special )
            {
               ReinforcementYieldingDetails(pChapter,pBroker,girderKey,true,pRatingArtifact);
               if ( bNegMoments )
               {
                  ReinforcementYieldingDetails(pChapter,pBroker,girderKey,false,pRatingArtifact);
               }
            }
            else
            {
               StressRatingDetails(pChapter,pBroker,girderKey,pRatingArtifact);
            }
         }

         if ( (ratingType == pgsTypes::lrLegal_Routine || ratingType == pgsTypes::lrLegal_Special) && 
               pRatingArtifact->GetRatingFactor() < 1 
            )
         {
            LoadPostingDetails(pChapter,pBroker,girderKey,pRatingArtifact);
         }
      }
   }
}

void CLoadRatingDetailsChapterBuilder::MomentRatingDetails(rptChapter* pChapter,IBroker* pBroker,const CGirderKey& girderKey,bool bPositiveMoment,const pgsRatingArtifact* pRatingArtifact) const
{
   rptParagraph* pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pPara;
   if ( bPositiveMoment )
   {
      *pPara << _T("Rating for Positive Moment") << rptNewLine;
   }
   else
   {
      *pPara << _T("Rating for Negative Moment") << rptNewLine;
   }

   *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("MomentRatingEquation.png") ) << rptNewLine;

   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(14,_T(""));

   table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
   table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   *pPara << table << rptNewLine;

   pPara = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
   *pChapter << pPara;
   *pPara << LIVELOAD_PER_GIRDER << rptNewLine;

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker,ILiveLoadDistributionFactors,pDistFact);

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
   (*table)(0,col++) << Sub2(symbol(phi),_T("c"));
   (*table)(0,col++) << Sub2(symbol(phi),_T("s"));
   (*table)(0,col++) << Sub2(symbol(phi),_T("n"));
   (*table)(0,col++) << _T("K");
   (*table)(0,col++) << COLHDR(Sub2(_T("M"),_T("n")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   (*table)(0,col++) << Sub2(symbol(gamma),_T("DC"));
   (*table)(0,col++) << COLHDR(Sub2(_T("M"),_T("DC")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   (*table)(0,col++) << Sub2(symbol(gamma),_T("DW"));
   (*table)(0,col++) << COLHDR(Sub2(_T("M"),_T("DW")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   (*table)(0,col++) << Sub2(symbol(gamma),_T("LL"));
   (*table)(0,col++) << _T("gM");
   (*table)(0,col++) << COLHDR(Sub2(_T("M"),_T("LL+IM")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   (*table)(0,col++) << _T("RF");

   const pgsMomentRatingArtifact* pControllingRating;
   pRatingArtifact->GetMomentRatingFactorEx(bPositiveMoment,&pControllingRating);
   pgsPointOfInterest controllingPoi = pControllingRating->GetPointOfInterest();

   pgsRatingArtifact::MomentRatings artifacts = pRatingArtifact->GetMomentRatings(bPositiveMoment);

   RowIndexType row = 1;
   pgsRatingArtifact::MomentRatings::iterator iter(artifacts.begin());
   pgsRatingArtifact::MomentRatings::iterator end(artifacts.end());
   for ( ; iter != end; iter++ )
   {
      col = 0;
      const pgsPointOfInterest& poi = iter->first;
      if ( !poi.IsTenthPoint(POI_SPAN) && poi != controllingPoi )
      {
         continue;
      }

      pgsMomentRatingArtifact& artifact = iter->second;

      const CSegmentKey& segmentKey = poi.GetSegmentKey();
      Float64 end_size = pBridge->GetSegmentStartEndDistance(segmentKey);

      Float64 pM, nM, V;
      pgsTypes::LoadRatingType ratingType = artifact.GetLoadRatingType();
      pgsTypes::LimitState limit_state = (ratingType == pgsTypes::lrPermit_Special ? pgsTypes::FatigueI : pgsTypes::StrengthI);
      pDistFact->GetDistributionFactors(poi,limit_state,&pM,&nM,&V);

      (*table)(row,col++) << location.SetValue( POI_SPAN, poi, end_size );
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
      (*table)(row,col++) << scalar.SetValue(bPositiveMoment ? pM : nM);
      (*table)(row,col++) << moment.SetValue(artifact.GetLiveLoadMoment());

      Float64 RF = artifact.GetRatingFactor();
      if ( RF < 1 )
         (*table)(row,col++) << RF_FAIL(rating_factor,RF);
      else
         (*table)(row,col++) << RF_PASS(rating_factor,RF);

      row++;
   }
}

void CLoadRatingDetailsChapterBuilder::ShearRatingDetails(rptChapter* pChapter,IBroker* pBroker,const CGirderKey& girderKey,const pgsRatingArtifact* pRatingArtifact) const
{
   rptParagraph* pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pPara;
   *pPara << _T("Rating for Shear") << rptNewLine;

   *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("ShearRatingEquation.png") ) << rptNewLine;

   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(13,_T(""));
   
   table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
   table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   *pPara << table << rptNewLine;
   pPara = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
   *pChapter << pPara;
   *pPara << LIVELOAD_PER_GIRDER << rptNewLine;

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker,ILiveLoadDistributionFactors,pDistFact);

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
   (*table)(0,col++) << Sub2(symbol(phi),_T("c"));
   (*table)(0,col++) << Sub2(symbol(phi),_T("s"));
   (*table)(0,col++) << Sub2(symbol(phi),_T("n"));
   (*table)(0,col++) << COLHDR(Sub2(_T("V"),_T("n")), rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   (*table)(0,col++) << Sub2(symbol(gamma),_T("DC"));
   (*table)(0,col++) << COLHDR(Sub2(_T("V"),_T("DC")), rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   (*table)(0,col++) << Sub2(symbol(gamma),_T("DW"));
   (*table)(0,col++) << COLHDR(Sub2(_T("V"),_T("DW")), rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   (*table)(0,col++) << Sub2(symbol(gamma),_T("LL"));
   (*table)(0,col++) << _T("gV");
   (*table)(0,col++) << COLHDR(Sub2(_T("V"),_T("LL+IM")), rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   (*table)(0,col++) << _T("RF");

   
   const pgsShearRatingArtifact* pControllingRating;
   pRatingArtifact->GetShearRatingFactorEx(&pControllingRating);
   pgsPointOfInterest controllingPoi = pControllingRating->GetPointOfInterest();

   pgsRatingArtifact::ShearRatings artifacts = pRatingArtifact->GetShearRatings();

   RowIndexType row = 1;
   pgsRatingArtifact::ShearRatings::iterator iter(artifacts.begin());
   pgsRatingArtifact::ShearRatings::iterator end(artifacts.end());
   for ( ; iter != end; iter++ )
   {
      col = 0;
      const pgsPointOfInterest& poi = iter->first;
      if ( !poi.IsTenthPoint(POI_SPAN) && poi != controllingPoi )
      {
         continue;
      }

      pgsShearRatingArtifact& artifact = iter->second;

      const CSegmentKey& segmentKey = poi.GetSegmentKey();
      Float64 end_size = pBridge->GetSegmentStartEndDistance(segmentKey);

      Float64 pM, nM, V;
      pgsTypes::LoadRatingType ratingType = artifact.GetLoadRatingType();
      pgsTypes::LimitState limit_state = (ratingType == pgsTypes::lrPermit_Special ? pgsTypes::FatigueI : pgsTypes::StrengthI);
      pDistFact->GetDistributionFactors(poi,limit_state,&pM,&nM,&V);

      (*table)(row,col++) << location.SetValue( POI_SPAN, poi, end_size );
      (*table)(row,col++) << scalar.SetValue(artifact.GetConditionFactor());
      (*table)(row,col++) << scalar.SetValue(artifact.GetSystemFactor());
      (*table)(row,col++) << scalar.SetValue(artifact.GetCapacityReductionFactor());
      (*table)(row,col++) << shear.SetValue(artifact.GetNominalShearCapacity());
      (*table)(row,col++) << scalar.SetValue(artifact.GetDeadLoadFactor());
      (*table)(row,col++) << shear.SetValue(artifact.GetDeadLoadShear());
      (*table)(row,col++) << scalar.SetValue(artifact.GetWearingSurfaceFactor());
      (*table)(row,col++) << shear.SetValue(artifact.GetWearingSurfaceShear());
      (*table)(row,col++) << scalar.SetValue(artifact.GetLiveLoadFactor());
      (*table)(row,col++) << scalar.SetValue(V);
      (*table)(row,col++) << shear.SetValue(artifact.GetLiveLoadShear());

      Float64 RF = artifact.GetRatingFactor();
      if ( RF < 1 )
         (*table)(row,col++) << RF_FAIL(rating_factor,RF);
      else
         (*table)(row,col++) << RF_PASS(rating_factor,RF);

      row++;
   }
}

void CLoadRatingDetailsChapterBuilder::StressRatingDetails(rptChapter* pChapter,IBroker* pBroker,const CGirderKey& girderKey,const pgsRatingArtifact* pRatingArtifact) const
{
   rptParagraph* pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pPara;
   *pPara << _T("Rating for Stress") << rptNewLine;

   *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("StressRatingEquation.png") ) << rptNewLine;

   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(11,_T(""));
   
   table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
   table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   *pPara << table << rptNewLine;
   pPara = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
   *pChapter << pPara;
   *pPara << LIVELOAD_PER_GIRDER << rptNewLine;

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker,ILiveLoadDistributionFactors,pDistFact);

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
   (*table)(0,col++) << RPT_STRESS(_T("r"));
   (*table)(0,col++) << Sub2(symbol(gamma),_T("DC"));
   (*table)(0,col++) << COLHDR(RPT_STRESS(_T("DC")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,col++) << COLHDR(RPT_STRESS(_T("PS")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,col++) << Sub2(symbol(gamma),_T("DW"));
   (*table)(0,col++) << COLHDR(RPT_STRESS(_T("DW")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,col++) << Sub2(symbol(gamma),_T("LL"));
   (*table)(0,col++) << _T("gM");
   (*table)(0,col++) << COLHDR(RPT_STRESS(_T("LL+IM")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,col++) << _T("RF");

   const pgsStressRatingArtifact* pControllingRating;
   pRatingArtifact->GetStressRatingFactorEx(&pControllingRating);
   pgsPointOfInterest controllingPoi = pControllingRating->GetPointOfInterest();

   pgsRatingArtifact::StressRatings artifacts = pRatingArtifact->GetStressRatings();

   RowIndexType row = 1;
   pgsRatingArtifact::StressRatings::iterator iter(artifacts.begin());
   pgsRatingArtifact::StressRatings::iterator end(artifacts.end());
   for ( ; iter != end; iter++ )
   {
      col = 0;
      const pgsPointOfInterest& poi = iter->first;
      if ( !poi.IsTenthPoint(POI_SPAN) && poi != controllingPoi )
      {
         continue;
      }

      pgsStressRatingArtifact& artifact = iter->second;

      const CSegmentKey& segmentKey = poi.GetSegmentKey();
      Float64 end_size = pBridge->GetSegmentStartEndDistance(segmentKey);

      Float64 pM, nM, V;
      pgsTypes::LoadRatingType ratingType = artifact.GetLoadRatingType();
      pgsTypes::LimitState limit_state = (ratingType == pgsTypes::lrPermit_Special ? pgsTypes::FatigueI : pgsTypes::StrengthI);
      pDistFact->GetDistributionFactors(poi,limit_state,&pM,&nM,&V);

      (*table)(row,col++) << location.SetValue( POI_SPAN,  poi, end_size );
      (*table)(row,col++) << stress.SetValue(artifact.GetAllowableStress());
      (*table)(row,col++) << scalar.SetValue(artifact.GetDeadLoadFactor());
      (*table)(row,col++) << stress.SetValue(artifact.GetDeadLoadStress());
      (*table)(row,col++) << stress.SetValue(artifact.GetPrestressStress());
      (*table)(row,col++) << scalar.SetValue(artifact.GetWearingSurfaceFactor());
      (*table)(row,col++) << stress.SetValue(artifact.GetWearingSurfaceStress());
      (*table)(row,col++) << scalar.SetValue(artifact.GetLiveLoadFactor());
      (*table)(row,col++) << scalar.SetValue(pM);
      (*table)(row,col++) << stress.SetValue(artifact.GetLiveLoadStress());

      Float64 RF = artifact.GetRatingFactor();
      if ( RF < 1 )
         (*table)(row,col++) << RF_FAIL(rating_factor,RF);
      else
         (*table)(row,col++) << RF_PASS(rating_factor,RF);

      row++;
   }
}

void CLoadRatingDetailsChapterBuilder::ReinforcementYieldingDetails(rptChapter* pChapter,IBroker* pBroker,const CGirderKey& girderKey,bool bPositiveMoment,const pgsRatingArtifact* pRatingArtifact) const
{
   pgsRatingArtifact::YieldStressRatios artifacts = pRatingArtifact->GetYieldStressRatios(bPositiveMoment);
   if ( artifacts.size() == 0 )
   {
      return;
   }

   rptParagraph* pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pPara;
   if ( bPositiveMoment )
      *pPara << _T("Check Reinforcement Yielding for Positive Moment") << rptNewLine;
   else
      *pPara << _T("Check Reinforcement Yielding for Negative Moment") << rptNewLine;

   *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("ReinforcementYieldingEquation.png") ) << rptNewLine;

   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(17,_T(""));
   
   table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
   table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   //*pPara << table << rptNewLine; // don't add table here... see below

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker,ILiveLoadDistributionFactors,pDistFact);

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
   (*table)(0,col++) << COLHDR(Sub2(_T("M"),_T("DC")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   (*table)(0,col++) << COLHDR(Sub2(_T("M"),_T("DW")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   (*table)(0,col++) << _T("gM");
   (*table)(0,col++) << COLHDR(Sub2(_T("M"),_T("LL+IM")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   (*table)(0,col++) << COLHDR(Sub2(_T("M"),_T("cr")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   (*table)(0,col++) << COLHDR(Sub2(_T("M"),_T("bcr")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   (*table)(0,col++) << COLHDR(Sub2(_T("d"),_T("ps")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,col++) << COLHDR(_T("c"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,col++) << COLHDR(Sub2(_T("I"),_T("cr")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit() );
   (*table)(0,col++) << COLHDR(RPT_STRESS(_T("pe")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,col++) << COLHDR(RPT_STRESS(_T("bcr")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,col++) << COLHDR(Sub2(_T("E"),_T("s")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,col++) << COLHDR(Sub2(_T("E"),_T("g")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,col++) << COLHDR(RPT_STRESS(_T("s")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,col++) << COLHDR(RPT_STRESS(_T("r")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,col++) << _T("Stress") << rptNewLine << _T("Ratio") << rptNewLine << RPT_STRESS(_T("r")) << _T("/") << RPT_STRESS(_T("s"));

   const pgsYieldStressRatioArtifact* pControllingRating;
   pRatingArtifact->GetYieldStressRatioEx(bPositiveMoment,&pControllingRating);
   pgsPointOfInterest controllingPoi = pControllingRating->GetPointOfInterest();

   pPara = new rptParagraph;
   *pChapter << pPara;

   *pPara << Sub2(symbol(gamma),_T("DC")) << _T(" = ") << scalar.SetValue(artifacts[0].second.GetDeadLoadFactor()) << rptNewLine;
   *pPara << Sub2(symbol(gamma),_T("DW")) << _T(" = ") << scalar.SetValue(artifacts[0].second.GetWearingSurfaceFactor()) << rptNewLine;
   *pPara << Sub2(symbol(gamma),_T("LL")) << _T(" = ") << scalar.SetValue(artifacts[0].second.GetLiveLoadFactor()) << rptNewLine;

   // Add table here
   *pPara << table << rptNewLine;
   pPara = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
   *pChapter << pPara;
   *pPara << LIVELOAD_PER_GIRDER << rptNewLine;

   RowIndexType row = 1;
   pgsRatingArtifact::YieldStressRatios::iterator iter(artifacts.begin());
   pgsRatingArtifact::YieldStressRatios::iterator end(artifacts.end());
   for ( ; iter != end; iter++ )
   {
      col = 0;
      const pgsPointOfInterest& poi = iter->first;
      if ( !poi.IsTenthPoint(POI_SPAN) && poi != controllingPoi )
      {
         continue;
      }

      pgsYieldStressRatioArtifact& artifact = iter->second;

      const CSegmentKey& segmentKey = poi.GetSegmentKey();
      Float64 end_size = pBridge->GetSegmentStartEndDistance(segmentKey);

      Float64 pM, nM, V;
      pgsTypes::LoadRatingType ratingType = artifact.GetLoadRatingType();
      pgsTypes::LimitState limit_state = (ratingType == pgsTypes::lrPermit_Special ? pgsTypes::FatigueI : pgsTypes::StrengthI);
      pDistFact->GetDistributionFactors(poi,limit_state,&pM,&nM,&V);

      (*table)(row,col++) << location.SetValue( POI_SPAN,  poi, end_size );
      (*table)(row,col++) << moment.SetValue(artifact.GetDeadLoadMoment());
      (*table)(row,col++) << moment.SetValue(artifact.GetWearingSurfaceMoment());
      (*table)(row,col++) << scalar.SetValue(bPositiveMoment ? pM : nM);
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

void CLoadRatingDetailsChapterBuilder::LoadPostingDetails(rptChapter* pChapter,IBroker* pBroker,const CGirderKey& girderKey,const pgsRatingArtifact* pRatingArtifact) const
{
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   rptParagraph* pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pPara;
   *pPara << _T("Load Posting Analysis Details [MBE 6A.8]") << rptNewLine;

   *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("SafePostingLoad.png") ) << rptNewLine;

   INIT_UV_PROTOTYPE( rptForceUnitValue, tonnage, pDisplayUnits->GetTonnageUnit(), false );
   rptCapacityToDemand rating_factor;

   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(5,_T(""));
   
   table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
   table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   *pPara << table << rptNewLine;

   ColumnIndexType col = 0;
   RowIndexType row = 0;

   (*table)(row,col++) << _T("Truck");
   (*table)(row,col++) << COLHDR(_T("Weight"), rptForceUnitTag, pDisplayUnits->GetTonnageUnit() );
   (*table)(row,col++) << _T("RF");
   (*table)(row,col++) << COLHDR(_T("Safe") << rptNewLine << _T("Load") << rptNewLine << _T("Capacity"), rptForceUnitTag, pDisplayUnits->GetTonnageUnit() );
   (*table)(row,col++) << COLHDR(_T("Safe") << rptNewLine << _T("Posting") << rptNewLine << _T("Load"), rptForceUnitTag, pDisplayUnits->GetTonnageUnit() );

   row++;
   col = 0;

   Float64 posting_load, W, RF;
   std::_tstring strName;
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
      (*table)(row,col++) << _T("-");
}
