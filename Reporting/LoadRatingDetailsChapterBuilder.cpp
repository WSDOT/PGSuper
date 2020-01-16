///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2020  Washington State Department of Transportation
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
#include <Reporting\LoadRatingReportSpecificationBuilder.h>

#include <IFace\AnalysisResults.h>
#include <IFace\Artifact.h>

#include <IFace\Bridge.h>
#include <IFace\RatingSpecification.h>
#include <IFace\DistributionFactors.h>

#include <PgsExt\RatingArtifact.h>
#include <PgsExt\CapacityToDemand.h>
#include <PgsExt\Helpers.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

inline pgsTypes::LimitState GetLimitState(pgsTypes::LoadRatingType ratingType) { return (ratingType == pgsTypes::lrPermit_Special ? pgsTypes::FatigueI : pgsTypes::StrengthI); }


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
   CLoadRatingReportSpecification* pLoadRatingRptSpec = dynamic_cast<CLoadRatingReportSpecification*>(pRptSpec);
   ATLASSERT(pLoadRatingRptSpec);
   
   CComPtr<IBroker> pBroker;
   CGirderKey girderKey;

   pLoadRatingRptSpec->GetBroker(&pBroker);
   girderKey = pLoadRatingRptSpec->GetGirderKey();
   m_bReportAtAllPoi = pLoadRatingRptSpec->ReportAtAllPointsOfInterest();

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   GET_IFACE2(pBroker, IGirderTendonGeometry, pTendonGeom);
   GET_IFACE2(pBroker,IBridge,pBridge);
   std::vector<CGirderKey> vGirderKeys;
   pBridge->GetGirderline(girderKey, &vGirderKeys);
   bool bSplicedGirder = false;
   for(const auto& thisGirderKey : vGirderKeys)
   {
      if (0 < pTendonGeom->GetDuctCount(thisGirderKey))
      {
         bSplicedGirder = true;
         break;
      }
   }

   ReportRatingDetails(pChapter, pBroker, girderKey, pgsTypes::lrDesign_Inventory, bSplicedGirder);
   ReportRatingDetails(pChapter, pBroker, girderKey, pgsTypes::lrDesign_Operating, bSplicedGirder);
   ReportRatingDetails(pChapter, pBroker, girderKey, pgsTypes::lrLegal_Routine, bSplicedGirder);
   ReportRatingDetails(pChapter, pBroker, girderKey, pgsTypes::lrLegal_Special, bSplicedGirder);
   ReportRatingDetails(pChapter, pBroker, girderKey, pgsTypes::lrLegal_Emergency, bSplicedGirder);
   ReportRatingDetails(pChapter, pBroker, girderKey, pgsTypes::lrPermit_Routine, bSplicedGirder);
   ReportRatingDetails(pChapter, pBroker, girderKey, pgsTypes::lrPermit_Special, bSplicedGirder);

   return pChapter;
}

CChapterBuilder* CLoadRatingDetailsChapterBuilder::Clone() const
{
   return new CLoadRatingDetailsChapterBuilder;
}

void CLoadRatingDetailsChapterBuilder::ReportRatingDetails(rptChapter* pChapter,IBroker* pBroker,const CGirderKey& girderKey,pgsTypes::LoadRatingType ratingType,bool bSplicedGirder) const
{
   GET_IFACE2(pBroker,IRatingSpecification,pRatingSpec);

   if ( !pRatingSpec->IsRatingEnabled(ratingType) )
   {
      return;
   }

   GET_IFACE2(pBroker,IProductLoads,pProductLoads);
   GET_IFACE2(pBroker,IBridge,pBridge);

   pgsTypes::LiveLoadType llType = ::GetLiveLoadType(ratingType);

   bool bNegMoments = pBridge->ProcessNegativeMoments(ALL_SPANS);

   CString strLiveLoadType = ::GetLiveLoadTypeName(ratingType);
   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pPara;
   pPara->SetName(strLiveLoadType);
   *pPara << pPara->GetName() << rptNewLine;

   std::_tstring strName = pProductLoads->GetLiveLoadName(llType,0);
   if ( strName == NO_LIVE_LOAD_DEFINED )
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
   for ( VehicleIndexType vehicleIdx = firstVehicleIdx; vehicleIdx <= lastVehicleIdx; vehicleIdx++ )
   {
      const pgsRatingArtifact* pRatingArtifact = pArtifact->GetRatingArtifact(girderKey,ratingType,
         (ratingType == pgsTypes::lrDesign_Inventory || ratingType == pgsTypes::lrDesign_Operating) ? INVALID_INDEX : vehicleIdx);

      if ( pRatingArtifact )
      {
         std::_tstring strVehicleName = pProductLoads->GetLiveLoadName(llType,(ratingType == pgsTypes::lrDesign_Inventory || ratingType == pgsTypes::lrDesign_Operating) ? INVALID_INDEX : vehicleIdx);

         pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
         *pChapter << pPara;
         *pPara << strVehicleName << rptNewLine;

         MomentRatingDetails(pChapter,pBroker,girderKey,true,pRatingArtifact,bSplicedGirder);
         if ( bNegMoments )
         {
            MomentRatingDetails(pChapter,pBroker,girderKey,false,pRatingArtifact,bSplicedGirder);
         }
         
         if ( pRatingSpec->RateForShear(ratingType) )
         {
            ShearRatingDetails(pChapter,pBroker,girderKey,pRatingArtifact,bSplicedGirder);
         }

         if ( pRatingSpec->RateForStress(ratingType) )
         {
            StressRatingDetails(pChapter,pBroker,girderKey,pRatingArtifact,bSplicedGirder);
         }

         if ( pRatingSpec->CheckYieldStress(ratingType) )
         {
            ReinforcementYieldingDetails(pChapter,pBroker,girderKey,true,pRatingArtifact,bSplicedGirder);
            if ( bNegMoments )
            {
               ReinforcementYieldingDetails(pChapter,pBroker,girderKey,false,pRatingArtifact,bSplicedGirder);
            }
         }

         if (pRatingArtifact->IsLoadPostingRequired() )
         {
            LoadPostingDetails(pChapter,pBroker,girderKey,pRatingArtifact);
         }
      }
   }
}

void CLoadRatingDetailsChapterBuilder::MomentRatingDetails(rptChapter* pChapter,IBroker* pBroker,const CGirderKey& girderKey,bool bPositiveMoment,const pgsRatingArtifact* pRatingArtifact,bool bSplicedGirder) const
{
   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pPara;
   if ( bPositiveMoment )
   {
      *pPara << _T("Rating for Positive Moment") << rptNewLine;
   }
   else
   {
      *pPara << _T("Rating for Negative Moment") << rptNewLine;
   }

   if ( bSplicedGirder )
   {
      *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("MomentRatingEquationPT.png") ) << rptNewLine;
   }
   else
   {
      *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("MomentRatingEquation.png") ) << rptNewLine;
   }

   if (lrfdVersionMgr::SixthEdition2012 <= lrfdVersionMgr::GetVersion() )
   {
      *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("MomentRating_K_Equation_2012.png")) << rptNewLine;
   }
   else
   {
      *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("MomentRating_K_Equation.png")) << rptNewLine;
   }

   ColumnIndexType nColumns = 14;
   if ( bSplicedGirder )
   {
      nColumns += 8;
   }
   rptRcTable* table = rptStyleManager::CreateDefaultTable(nColumns);

   table->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   table->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   *pPara << table << rptNewLine;

   pPara = new rptParagraph(rptStyleManager::GetFootnoteStyle());
   *pChapter << pPara;
   *pPara << LIVELOAD_PER_GIRDER << rptNewLine;

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker,ILiveLoadDistributionFactors,pDistFact);

   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(),   false );
   location.IncludeSpanAndGirder(true);

   INIT_UV_PROTOTYPE( rptMomentUnitValue, moment,   pDisplayUnits->GetMomentUnit(),       false );

   INIT_SCALAR_PROTOTYPE(rptRcScalar, scalar, pDisplayUnits->GetScalarFormat());

   rptCapacityToDemand rating_factor;

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
   if ( bSplicedGirder )
   {
      (*table)(0,col++) << Sub2(symbol(gamma),_T("CR"));
      (*table)(0,col++) << COLHDR(Sub2(_T("M"),_T("CR")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
      (*table)(0,col++) << Sub2(symbol(gamma),_T("SR"));
      (*table)(0,col++) << COLHDR(Sub2(_T("M"),_T("SR")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
      (*table)(0,col++) << Sub2(symbol(gamma),_T("RE"));
      (*table)(0,col++) << COLHDR(Sub2(_T("M"),_T("RE")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
      (*table)(0,col++) << Sub2(symbol(gamma),_T("PS"));
      (*table)(0,col++) << COLHDR(Sub2(_T("M"),_T("PS")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   }
   (*table)(0,col++) << Sub2(symbol(gamma),_T("LL"));
   (*table)(0,col++) << _T("gM");
   (*table)(0,col++) << COLHDR(Sub2(_T("M"),_T("LL+IM")) << _T(" (*)"), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   (*table)(0,col++) << _T("RF");

   const pgsMomentRatingArtifact* pControllingRating;
   pRatingArtifact->GetMomentRatingFactorEx(bPositiveMoment,&pControllingRating);
   pgsPointOfInterest controllingPoi = pControllingRating->GetPointOfInterest();

   const pgsRatingArtifact::MomentRatings& artifacts = pRatingArtifact->GetMomentRatings(bPositiveMoment);

   RowIndexType row = table->GetNumberOfHeaderRows();
   pgsRatingArtifact::MomentRatings::const_iterator iter(artifacts.begin());
   pgsRatingArtifact::MomentRatings::const_iterator end(artifacts.end());
   for ( ; iter != end; iter++ )
   {
      col = 0;
      const pgsPointOfInterest& poi = iter->first;
      const pgsMomentRatingArtifact& artifact = iter->second;
      Float64 RF = artifact.GetRatingFactor();
      if ( 1.0 <= RF && !ReportAtThisPoi(poi,controllingPoi) )
      {
         continue;
      }


      Float64 pM, nM, V;
      pgsTypes::LoadRatingType ratingType = artifact.GetLoadRatingType();
      pgsTypes::LimitState limit_state = GetLimitState(ratingType);
      pDistFact->GetDistributionFactors(poi,limit_state,&pM,&nM,&V);

      (*table)(row,col++) << location.SetValue( POI_SPAN, poi );
      (*table)(row,col++) << scalar.SetValue(artifact.GetConditionFactor());
      (*table)(row,col++) << scalar.SetValue(artifact.GetSystemFactor());
      (*table)(row,col++) << scalar.SetValue(artifact.GetCapacityReductionFactor());
      (*table)(row,col++) << scalar.SetValue(artifact.GetMinimumReinforcementFactor());
      (*table)(row,col++) << moment.SetValue(artifact.GetNominalMomentCapacity());
      (*table)(row,col++) << scalar.SetValue(artifact.GetDeadLoadFactor());
      (*table)(row,col++) << moment.SetValue(artifact.GetDeadLoadMoment());
      (*table)(row,col++) << scalar.SetValue(artifact.GetWearingSurfaceFactor());
      (*table)(row,col++) << moment.SetValue(artifact.GetWearingSurfaceMoment());
      if ( bSplicedGirder )
      {
         (*table)(row,col++) << scalar.SetValue(artifact.GetCreepFactor());
         (*table)(row,col++) << moment.SetValue(artifact.GetCreepMoment());
         (*table)(row,col++) << scalar.SetValue(artifact.GetShrinkageFactor());
         (*table)(row,col++) << moment.SetValue(artifact.GetShrinkageMoment());
         (*table)(row,col++) << scalar.SetValue(artifact.GetRelaxationFactor());
         (*table)(row,col++) << moment.SetValue(artifact.GetRelaxationMoment());
         (*table)(row,col++) << scalar.SetValue(artifact.GetSecondaryEffectsFactor());
         (*table)(row,col++) << moment.SetValue(artifact.GetSecondaryEffectsMoment());
      }
      (*table)(row,col++) << scalar.SetValue(artifact.GetLiveLoadFactor());
      (*table)(row,col++) << scalar.SetValue(bPositiveMoment ? pM : nM);
      (*table)(row,col++) << moment.SetValue(artifact.GetLiveLoadMoment());

      if ( RF < 1 )
      {
         (*table)(row,col++) << RF_FAIL(rating_factor,RF);
      }
      else
      {
         (*table)(row,col++) << RF_PASS(rating_factor,RF);
      }

      row++;
   }
}

void CLoadRatingDetailsChapterBuilder::ShearRatingDetails(rptChapter* pChapter,IBroker* pBroker,const CGirderKey& girderKey,const pgsRatingArtifact* pRatingArtifact,bool bSplicedGirder) const
{
   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pPara;
   *pPara << _T("Rating for Shear") << rptNewLine;

   if ( bSplicedGirder )
   {
      *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("ShearRatingEquationPT.png") ) << rptNewLine;
   }
   else
   {
      *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("ShearRatingEquation.png") ) << rptNewLine;
   }

   ColumnIndexType nColumns = 13;
   if ( bSplicedGirder )
   {
      nColumns += 8;
   }
   rptRcTable* table = rptStyleManager::CreateDefaultTable(nColumns);
   
   table->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   table->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   *pPara << table << rptNewLine;
   pPara = new rptParagraph(rptStyleManager::GetFootnoteStyle());
   *pChapter << pPara;
   *pPara << LIVELOAD_PER_GIRDER << rptNewLine;

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker,ILiveLoadDistributionFactors,pDistFact);

   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(),   false );
   location.IncludeSpanAndGirder(true);

   INIT_UV_PROTOTYPE( rptForceUnitValue,  shear,    pDisplayUnits->GetShearUnit(),        false );

   INIT_SCALAR_PROTOTYPE(rptRcScalar, scalar, pDisplayUnits->GetScalarFormat());

   rptCapacityToDemand rating_factor;

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
   if ( bSplicedGirder )
   {
      (*table)(0,col++) << Sub2(symbol(gamma),_T("CR"));
      (*table)(0,col++) << COLHDR(Sub2(_T("V"),_T("CR")), rptForceUnitTag, pDisplayUnits->GetShearUnit() );
      (*table)(0,col++) << Sub2(symbol(gamma),_T("SH"));
      (*table)(0,col++) << COLHDR(Sub2(_T("V"),_T("SH")), rptForceUnitTag, pDisplayUnits->GetShearUnit() );
      (*table)(0,col++) << Sub2(symbol(gamma),_T("RE"));
      (*table)(0,col++) << COLHDR(Sub2(_T("V"),_T("RE")), rptForceUnitTag, pDisplayUnits->GetShearUnit() );
      (*table)(0,col++) << Sub2(symbol(gamma),_T("PS"));
      (*table)(0,col++) << COLHDR(Sub2(_T("V"),_T("PS")), rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   }
   (*table)(0,col++) << Sub2(symbol(gamma),_T("LL"));
   (*table)(0,col++) << _T("gV");
   (*table)(0,col++) << COLHDR(Sub2(_T("V"),_T("LL+IM")) << _T(" (*)"), rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   (*table)(0,col++) << _T("RF");

   
   const pgsShearRatingArtifact* pControllingRating;
   pRatingArtifact->GetShearRatingFactorEx(&pControllingRating);
   pgsPointOfInterest controllingPoi = pControllingRating->GetPointOfInterest();

   const pgsRatingArtifact::ShearRatings& artifacts = pRatingArtifact->GetShearRatings();

   RowIndexType row = table->GetNumberOfHeaderRows();
   pgsRatingArtifact::ShearRatings::const_iterator iter(artifacts.begin());
   pgsRatingArtifact::ShearRatings::const_iterator end(artifacts.end());
   for ( ; iter != end; iter++ )
   {
      col = 0;
      const pgsPointOfInterest& poi = iter->first;
      const pgsShearRatingArtifact& artifact = iter->second;
      Float64 RF = artifact.GetRatingFactor();
      if ( !poi.HasAttribute(POI_CRITSECTSHEAR1) && !poi.HasAttribute(POI_CRITSECTSHEAR2) && 1.0 <= RF && !ReportAtThisPoi(poi,controllingPoi) )
      {
         continue;
      }


      Float64 gpM, gnM, gV;
      pgsTypes::LoadRatingType ratingType = artifact.GetLoadRatingType();
      pgsTypes::LimitState limit_state = GetLimitState(ratingType);
      pDistFact->GetDistributionFactors(poi,limit_state,&gpM,&gnM,&gV);

      (*table)(row,col++) << location.SetValue( POI_SPAN, poi );
      (*table)(row,col++) << scalar.SetValue(artifact.GetConditionFactor());
      (*table)(row,col++) << scalar.SetValue(artifact.GetSystemFactor());
      (*table)(row,col++) << scalar.SetValue(artifact.GetCapacityReductionFactor());
      (*table)(row,col++) << shear.SetValue(artifact.GetNominalShearCapacity());
      (*table)(row,col++) << scalar.SetValue(artifact.GetDeadLoadFactor());
      (*table)(row,col++) << shear.SetValue(artifact.GetDeadLoadShear());
      (*table)(row,col++) << scalar.SetValue(artifact.GetWearingSurfaceFactor());
      (*table)(row,col++) << shear.SetValue(artifact.GetWearingSurfaceShear());
      if ( bSplicedGirder )
      {
         (*table)(row,col++) << scalar.SetValue(artifact.GetCreepFactor());
         (*table)(row,col++) << shear.SetValue(artifact.GetCreepShear());
         (*table)(row,col++) << scalar.SetValue(artifact.GetShrinkageFactor());
         (*table)(row,col++) << shear.SetValue(artifact.GetShrinkageShear());
         (*table)(row,col++) << scalar.SetValue(artifact.GetRelaxationFactor());
         (*table)(row,col++) << shear.SetValue(artifact.GetRelaxationShear());
         (*table)(row,col++) << scalar.SetValue(artifact.GetSecondaryEffectsFactor());
         (*table)(row,col++) << shear.SetValue(artifact.GetSecondaryEffectsShear());
      }
      (*table)(row,col++) << scalar.SetValue(artifact.GetLiveLoadFactor());
      (*table)(row,col++) << scalar.SetValue(gV);
      (*table)(row,col++) << shear.SetValue(artifact.GetLiveLoadShear());

      if ( RF < 1 )
      {
         (*table)(row,col++) << RF_FAIL(rating_factor,RF);
      }
      else
      {
         (*table)(row,col++) << RF_PASS(rating_factor,RF);
      }

      row++;
   }
}

void CLoadRatingDetailsChapterBuilder::StressRatingDetails(rptChapter* pChapter,IBroker* pBroker,const CGirderKey& girderKey,const pgsRatingArtifact* pRatingArtifact,bool bSplicedGirder) const
{
   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pPara;
   *pPara << _T("Rating for Stress") << rptNewLine;

   if ( bSplicedGirder )
   {
      *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("StressRatingEquationPT.png") ) << rptNewLine;
   }
   else
   {
      *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("StressRatingEquation.png") ) << rptNewLine;
   }

   ColumnIndexType nColumns = 13;
   if (bSplicedGirder)
   {
      nColumns += 9;
   }
   rptRcTable* table = rptStyleManager::CreateDefaultTable(nColumns);
   
   table->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   table->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   
   table->SetColumnStyle(1,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   table->SetStripeRowColumnStyle(1,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   *pPara << table << rptNewLine;
   pPara = new rptParagraph(rptStyleManager::GetFootnoteStyle());
   *pChapter << pPara;
   *pPara << LIVELOAD_PER_GIRDER << rptNewLine;

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker,ILiveLoadDistributionFactors,pDistFact);

   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(),   false );
   location.IncludeSpanAndGirder(true);

   INIT_UV_PROTOTYPE( rptStressUnitValue,  stress,    pDisplayUnits->GetStressUnit(),        false );

   INIT_SCALAR_PROTOTYPE(rptRcScalar, scalar, pDisplayUnits->GetScalarFormat());

   rptCapacityToDemand rating_factor;

   ColumnIndexType col = 0;

   (*table)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table)(0,col++) << _T("Location");
   (*table)(0,col++) << COLHDR(RPT_STRESS(_T("limit")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,col++) << COLHDR(RPT_STRESS(_T("ps")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   if (bSplicedGirder)
   {
      (*table)(0,col++) << COLHDR(RPT_STRESS(_T("pt")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   }
   (*table)(0,col++) << COLHDR(RPT_STRESS(_T("R")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,col++) << Sub2(symbol(gamma),_T("DC"));
   (*table)(0,col++) << COLHDR(RPT_STRESS(_T("DC")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,col++) << Sub2(symbol(gamma),_T("DW"));
   (*table)(0,col++) << COLHDR(RPT_STRESS(_T("DW")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );

   if(bSplicedGirder)
   {
      (*table)(0,col++) << Sub2(symbol(gamma),_T("CR"));
      (*table)(0,col++) << COLHDR(RPT_STRESS(_T("CR")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*table)(0,col++) << Sub2(symbol(gamma),_T("SH"));
      (*table)(0,col++) << COLHDR(RPT_STRESS(_T("SH")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*table)(0,col++) << Sub2(symbol(gamma),_T("RE"));
      (*table)(0,col++) << COLHDR(RPT_STRESS(_T("RE")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*table)(0,col++) << Sub2(symbol(gamma),_T("PS"));
      (*table)(0,col++) << COLHDR(RPT_STRESS(_T("PS")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   }

   (*table)(0,col++) << Sub2(symbol(gamma),_T("LL"));
   (*table)(0,col++) << _T("gM");
   (*table)(0,col++) << COLHDR(RPT_STRESS(_T("LL+IM")) << _T(" (*)"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,col++) << _T("RF");

   const pgsStressRatingArtifact* pControllingRating;
   pRatingArtifact->GetStressRatingFactorEx(&pControllingRating);
   pgsPointOfInterest controllingPoi = pControllingRating->GetPointOfInterest();

   const pgsRatingArtifact::StressRatings& artifacts = pRatingArtifact->GetStressRatings();

   LPCTSTR stressLocation[] = {_T("Bottom Girder"),_T("Top Girder"),_T("Bottom Deck"),_T("Top Deck")};

   RowIndexType row = table->GetNumberOfHeaderRows();
   pgsRatingArtifact::StressRatings::const_iterator iter(artifacts.begin());
   pgsRatingArtifact::StressRatings::const_iterator end(artifacts.end());
   for ( ; iter != end; iter++ )
   {
      col = 0;
      const pgsPointOfInterest& poi = iter->first;
      const pgsStressRatingArtifact& artifact = iter->second;
      Float64 RF = artifact.GetRatingFactor();

      if ( 1.0 <= RF && !ReportAtThisPoi(poi,controllingPoi) )
      {
         continue;
      }

      Float64 pM, nM, V;
      pgsTypes::LoadRatingType ratingType = artifact.GetLoadRatingType();
      pgsTypes::LimitState limit_state = GetLimitState(ratingType);
      pDistFact->GetDistributionFactors(poi,limit_state,&pM,&nM,&V);

      (*table)(row,col++) << location.SetValue( POI_SPAN,  poi );
      (*table)(row,col++) << stressLocation[artifact.GetStressLocation()];
      (*table)(row,col++) << stress.SetValue(artifact.GetAllowableStress());
      (*table)(row,col++) << stress.SetValue(artifact.GetPrestressStress());
      if (bSplicedGirder)
      {
         (*table)(row,col++) << stress.SetValue(artifact.GetPostTensionStress());
      }
      (*table)(row,col++) << stress.SetValue(artifact.GetResistance());
      (*table)(row,col++) << scalar.SetValue(artifact.GetDeadLoadFactor());
      (*table)(row,col++) << stress.SetValue(artifact.GetDeadLoadStress());
      (*table)(row,col++) << scalar.SetValue(artifact.GetWearingSurfaceFactor());
      (*table)(row,col++) << stress.SetValue(artifact.GetWearingSurfaceStress());
      if (bSplicedGirder)
      {
         (*table)(row,col++) << scalar.SetValue(artifact.GetCreepFactor());
         (*table)(row,col++) << stress.SetValue(artifact.GetCreepStress());
         (*table)(row,col++) << scalar.SetValue(artifact.GetShrinkageFactor());
         (*table)(row,col++) << stress.SetValue(artifact.GetShrinkageStress());
         (*table)(row,col++) << scalar.SetValue(artifact.GetRelaxationFactor());
         (*table)(row,col++) << stress.SetValue(artifact.GetRelaxationStress());
         (*table)(row,col++) << scalar.SetValue(artifact.GetSecondaryEffectsFactor());
         (*table)(row,col++) << stress.SetValue(artifact.GetSecondaryEffectsStress());
      }
      (*table)(row,col++) << scalar.SetValue(artifact.GetLiveLoadFactor());
      (*table)(row,col++) << scalar.SetValue(pM);
      (*table)(row,col++) << stress.SetValue(artifact.GetLiveLoadStress());

      if ( RF < 1 )
      {
         (*table)(row,col++) << RF_FAIL(rating_factor,RF);
      }
      else
      {
         (*table)(row,col++) << RF_PASS(rating_factor,RF);
      }

      row++;
   }
}

void CLoadRatingDetailsChapterBuilder::ReinforcementYieldingDetails(rptChapter* pChapter,IBroker* pBroker,const CGirderKey& girderKey,bool bPositiveMoment,const pgsRatingArtifact* pRatingArtifact,bool bSplicedGirder) const
{
   const pgsRatingArtifact::YieldStressRatios& artifacts = pRatingArtifact->GetYieldStressRatios(bPositiveMoment);
   if ( artifacts.size() == 0 )
   {
      return;
   }

   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pPara;
   if ( bPositiveMoment )
   {
      *pPara << _T("Check Reinforcement Yielding for Positive Moment") << rptNewLine;
   }
   else
   {
      *pPara << _T("Check Reinforcement Yielding for Negative Moment") << rptNewLine;
   }

   if ( bSplicedGirder )
   {
      *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("ReinforcementYieldingEquationPT.png") ) << rptNewLine;
   }
   else
   {
      *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("ReinforcementYieldingEquation.png") ) << rptNewLine;
   }

   ColumnIndexType nColumns = 18;
   if ( bSplicedGirder )
   {
      nColumns += 4;
   }
   rptRcTable* table = rptStyleManager::CreateDefaultTable(nColumns);
   
   table->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   table->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   //*pPara << table << rptNewLine; // don't add table here... see below

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   INIT_UV_PROTOTYPE( rptPointOfInterest,  location, pDisplayUnits->GetSpanLengthUnit(),      false );
   location.IncludeSpanAndGirder(true);

   INIT_UV_PROTOTYPE( rptLengthUnitValue,  dim,      pDisplayUnits->GetComponentDimUnit(),    false );
   INIT_UV_PROTOTYPE( rptStressUnitValue,  stress,   pDisplayUnits->GetStressUnit(),          false );
   INIT_UV_PROTOTYPE( rptMomentUnitValue,  moment,   pDisplayUnits->GetMomentUnit(),          false );
   INIT_UV_PROTOTYPE( rptLength4UnitValue, mom_i,    pDisplayUnits->GetMomentOfInertiaUnit(), false );
   INIT_UV_PROTOTYPE( rptStressUnitValue,  mod_e,    pDisplayUnits->GetModEUnit(),            false );

   INIT_SCALAR_PROTOTYPE(rptRcScalar, scalar, pDisplayUnits->GetScalarFormat());

   rptCapacityToDemand rating_factor;

   ColumnIndexType col = 0;

   (*table)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table)(0,col++) << COLHDR(Sub2(_T("M"),_T("DC")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   (*table)(0,col++) << COLHDR(Sub2(_T("M"),_T("DW")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   if ( bSplicedGirder )
   {
      (*table)(0,col++) << COLHDR(Sub2(_T("M"),_T("CR")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
      (*table)(0,col++) << COLHDR(Sub2(_T("M"),_T("SH")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
      (*table)(0,col++) << COLHDR(Sub2(_T("M"),_T("RE")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
      (*table)(0,col++) << COLHDR(Sub2(_T("M"),_T("PS")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   }
   (*table)(0,col++) << _T("gM");
   (*table)(0,col++) << COLHDR(Sub2(_T("M"),_T("LL+IM")) << _T("( *)"), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   (*table)(0,col++) << COLHDR(Sub2(_T("M"),_T("cr")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   (*table)(0,col++) << COLHDR(Sub2(_T("M"),_T("bcr")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   (*table)(0,col++) << COLHDR(Sub2(_T("d"),_T("s")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,col++) << COLHDR(_T("c"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,col++) << COLHDR(Sub2(_T("I"),_T("cr")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit() );
   (*table)(0,col++) << COLHDR(RPT_STRESS(_T("pe")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,col++) << COLHDR(RPT_STRESS(_T("bcr")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,col++) << COLHDR(Sub2(_T("E"),_T("s")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,col++) << COLHDR(Sub2(_T("E"),_T("g")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,col++) << COLHDR(RPT_STRESS(_T("s")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,col++) << COLHDR(RPT_STRESS(_T("r")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,col++) << _T("Stress") << rptNewLine << _T("Ratio") << rptNewLine << RPT_STRESS(_T("r")) << _T("/") << RPT_STRESS(_T("s"));
   (*table)(0,col++) << _T("Reinf.") << rptNewLine << _T("Type");

   const pgsYieldStressRatioArtifact* pControllingRating;
   pRatingArtifact->GetYieldStressRatioEx(bPositiveMoment,&pControllingRating);
   pgsPointOfInterest controllingPoi = pControllingRating->GetPointOfInterest();

   pPara = new rptParagraph;
   *pChapter << pPara;

   *pPara << Sub2(symbol(gamma),_T("DC")) << _T(" = ") << scalar.SetValue(artifacts[0].second.GetDeadLoadFactor()) << rptNewLine;
   *pPara << Sub2(symbol(gamma),_T("DW")) << _T(" = ") << scalar.SetValue(artifacts[0].second.GetWearingSurfaceFactor()) << rptNewLine;
   if ( bSplicedGirder )
   {
      *pPara << Sub2(symbol(gamma),_T("CR")) << _T(" = ") << scalar.SetValue(artifacts[0].second.GetCreepFactor()) << rptNewLine;
      *pPara << Sub2(symbol(gamma),_T("SH")) << _T(" = ") << scalar.SetValue(artifacts[0].second.GetShrinkageFactor()) << rptNewLine;
      *pPara << Sub2(symbol(gamma),_T("RE")) << _T(" = ") << scalar.SetValue(artifacts[0].second.GetRelaxationFactor()) << rptNewLine;
      *pPara << Sub2(symbol(gamma),_T("PS")) << _T(" = ") << scalar.SetValue(artifacts[0].second.GetSecondaryEffectsFactor()) << rptNewLine;
   }
   *pPara << Sub2(symbol(gamma),_T("LL")) << _T(" = ") << scalar.SetValue(artifacts[0].second.GetLiveLoadFactor()) << rptNewLine;

   // Add table here
   *pPara << table << rptNewLine;
   pPara = new rptParagraph(rptStyleManager::GetFootnoteStyle());
   *pChapter << pPara;
   *pPara << LIVELOAD_PER_GIRDER << rptNewLine;

   RowIndexType row = table->GetNumberOfHeaderRows();
   pgsRatingArtifact::YieldStressRatios::const_iterator iter(artifacts.begin());
   pgsRatingArtifact::YieldStressRatios::const_iterator end(artifacts.end());
   for ( ; iter != end; iter++ )
   {
      col = 0;
      const pgsPointOfInterest& poi = iter->first;
      const pgsYieldStressRatioArtifact& artifact = iter->second;

      Float64 rebarSR = artifact.GetRebarStressRatio();
      Float64 strandSR = artifact.GetStrandStressRatio();
      Float64 segmentTendonSR = artifact.GetSegmentTendonStressRatio();
      Float64 girderTendonSR = artifact.GetGirderTendonStressRatio();

      Float64 SR = Min(rebarSR,strandSR,segmentTendonSR,girderTendonSR);

      if ( 1.0 <= SR && !ReportAtThisPoi(poi,controllingPoi) )
      {
         continue;
      }

      IndexType srIdx = MinIndex(rebarSR,strandSR,segmentTendonSR,girderTendonSR);

      Float64 d;
      Float64 E;
      Float64 f;
      Float64 fy;
      Float64 fs;
      Float64 fcr;
      Float64 fallow;
      if ( srIdx == 0 )
      {
         artifact.GetRebar(&d,&f,&fy,&E);
         fcr = artifact.GetRebarCrackingStressIncrement();
         fs = artifact.GetRebarStress();
         fallow = artifact.GetRebarAllowableStress();
      }
      else if ( srIdx == 1 )
      {
         artifact.GetStrand(&d,&f,&fy,&E);
         fcr = artifact.GetStrandCrackingStressIncrement();
         fs = artifact.GetStrandStress();
         fallow = artifact.GetStrandAllowableStress();
      }
      else if (srIdx == 2)
      {
         artifact.GetSegmentTendon(&d,&f,&fy,&E);
         fcr = artifact.GetSegmentTendonCrackingStressIncrement();
         fs = artifact.GetSegmentTendonStress();
         fallow = artifact.GetSegmentTendonAllowableStress();
      }
      else
      {
         artifact.GetGirderTendon(&d, &f, &fy, &E);
         fcr = artifact.GetGirderTendonCrackingStressIncrement();
         fs = artifact.GetGirderTendonStress();
         fallow = artifact.GetGirderTendonAllowableStress();
      }


      (*table)(row,col++) << location.SetValue( POI_SPAN,  poi );
      (*table)(row,col++) << moment.SetValue(artifact.GetDeadLoadMoment());
      (*table)(row,col++) << moment.SetValue(artifact.GetWearingSurfaceMoment());
      if ( bSplicedGirder )
      {
         (*table)(row,col++) << moment.SetValue(artifact.GetCreepMoment());
         (*table)(row,col++) << moment.SetValue(artifact.GetShrinkageMoment());
         (*table)(row,col++) << moment.SetValue(artifact.GetRelaxationMoment());
         (*table)(row,col++) << moment.SetValue(artifact.GetSecondaryEffectsMoment());
      }
      (*table)(row,col++) << scalar.SetValue(artifact.GetLiveLoadDistributionFactor());
      (*table)(row,col++) << moment.SetValue(artifact.GetLiveLoadMoment());
      (*table)(row,col++) << moment.SetValue(artifact.GetCrackingMoment());
      (*table)(row,col++) << moment.SetValue(artifact.GetExcessMoment());
      (*table)(row,col++) << dim.SetValue(d);
      (*table)(row,col++) << dim.SetValue(artifact.GetCrackDepth());
      (*table)(row,col++) << mom_i.SetValue(artifact.GetIcr());
      (*table)(row,col++) << stress.SetValue(f);
      (*table)(row,col++) << stress.SetValue(fcr);
      (*table)(row,col++) << mod_e.SetValue(E);
      (*table)(row,col++) << mod_e.SetValue(artifact.GetEg());
      (*table)(row,col++) << stress.SetValue(fs);
      (*table)(row,col++) << stress.SetValue(fallow);


      if ( SR < 1 )
      {
         (*table)(row,col++) << RF_FAIL(rating_factor,SR);
      }
      else
      {
         (*table)(row,col++) << RF_PASS(rating_factor,SR);
      }

      if ( srIdx == 0 )
      {
         (*table)(row,col++) << _T("Rebar");
      }
      else if ( srIdx == 1 )
      {
         (*table)(row,col++) << _T("Strand");
      }
      else if (srIdx == 2)
      {
         (*table)(row, col++) << _T("Segment Tendon");
      }
      else
      {
         (*table)(row,col++) << _T("Girder Tendon");
      }

      row++;
   }
}

void CLoadRatingDetailsChapterBuilder::LoadPostingDetails(rptChapter* pChapter,IBroker* pBroker,const CGirderKey& girderKey,const pgsRatingArtifact* pRatingArtifact) const
{
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pPara;
   *pPara << _T("Load Posting Analysis Details [MBE 6A.8]") << rptNewLine;

   *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("SafePostingLoad.png") ) << rptNewLine;

   INIT_UV_PROTOTYPE( rptForceUnitValue, tonnage, pDisplayUnits->GetTonnageUnit(), false );
   rptCapacityToDemand rating_factor;

   rptRcTable* table = rptStyleManager::CreateDefaultTable(5,_T(""));
   
   table->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   table->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

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
   {
      (*table)(row,col++) << RF_FAIL(rating_factor,RF);
   }
   else
   {
      (*table)(row,col++) << RF_PASS(rating_factor,RF);
   }

   (*table)(row,col++) << tonnage.SetValue(::FloorOff(W*RF,0.01));
   if ( RF < 1 )
   {
      (*table)(row,col++) << tonnage.SetValue(posting_load);
   }
   else
   {
      (*table)(row,col++) << _T("-");
   }
}

bool CLoadRatingDetailsChapterBuilder::ReportAtThisPoi(const pgsPointOfInterest& poi,const pgsPointOfInterest& controllingPoi) const
{
   if (m_bReportAtAllPoi ||
        poi == controllingPoi ||
        poi.IsTenthPoint(POI_SPAN) || 
        poi.HasAttribute(POI_CLOSURE) || 
        poi.HasAttribute(POI_CANTILEVER)
      )
   {
      return true;
   }
   else
   {
      return false;
   }
}
