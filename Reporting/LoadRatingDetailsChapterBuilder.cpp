///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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

   GET_IFACE2(pBroker,IBridge,pBridge);
   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   GroupIndexType firstGroupIdx = girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex;
   GroupIndexType lastGroupIdx  = girderKey.groupIndex == ALL_GROUPS ? nGroups-1 : firstGroupIdx;
   DuctIndexType nDucts = 0;
   GET_IFACE2(pBroker,ITendonGeometry,pTendonGeom);
   for ( GroupIndexType grpIdx = firstGroupIdx; grpIdx <= lastGroupIdx; grpIdx++ )
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
      GirderIndexType gdrIdx = min(girderKey.girderIndex,nGirders-1);
      nDucts += pTendonGeom->GetDuctCount(CGirderKey(grpIdx,gdrIdx));
   }
   bool bSplicedGirder = (0 < nDucts ? true : false);


   ReportRatingDetails(pChapter,pBroker,girderKey,pgsTypes::lrDesign_Inventory,bSplicedGirder);
   ReportRatingDetails(pChapter,pBroker,girderKey,pgsTypes::lrDesign_Operating,bSplicedGirder);
   ReportRatingDetails(pChapter,pBroker,girderKey,pgsTypes::lrLegal_Routine,bSplicedGirder);
   ReportRatingDetails(pChapter,pBroker,girderKey,pgsTypes::lrLegal_Special,bSplicedGirder);
   ReportRatingDetails(pChapter,pBroker,girderKey,pgsTypes::lrPermit_Routine,bSplicedGirder);
   ReportRatingDetails(pChapter,pBroker,girderKey,pgsTypes::lrPermit_Special,bSplicedGirder);

   return pChapter;
}

CChapterBuilder* CLoadRatingDetailsChapterBuilder::Clone() const
{
   return new CLoadRatingDetailsChapterBuilder;
}

void CLoadRatingDetailsChapterBuilder::ReportRatingDetails(rptChapter* pChapter,IBroker* pBroker,const CGirderKey& girderKey,pgsTypes::LoadRatingType ratingType,bool bSplicedGirder) const
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
            if ( ratingType == pgsTypes::lrPermit_Routine || ratingType == pgsTypes::lrPermit_Special )
            {
               ReinforcementYieldingDetails(pChapter,pBroker,girderKey,true,pRatingArtifact,bSplicedGirder);
               if ( bNegMoments )
               {
                  ReinforcementYieldingDetails(pChapter,pBroker,girderKey,false,pRatingArtifact,bSplicedGirder);
               }
            }
            else
            {
               StressRatingDetails(pChapter,pBroker,girderKey,pRatingArtifact,bSplicedGirder);
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

void CLoadRatingDetailsChapterBuilder::MomentRatingDetails(rptChapter* pChapter,IBroker* pBroker,const CGirderKey& girderKey,bool bPositiveMoment,const pgsRatingArtifact* pRatingArtifact,bool bSplicedGirder) const
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

   if ( bSplicedGirder )
   {
      *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("MomentRatingEquationPT.png") ) << rptNewLine;
   }
   else
   {
      *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("MomentRatingEquation.png") ) << rptNewLine;
   }
   *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("MomentRatingParameters.png") ) << rptNewLine;

   ColumnIndexType nColumns = 14;
   if ( bSplicedGirder )
   {
      nColumns += 8;
   }
   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(nColumns);

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

   pgsRatingArtifact::MomentRatings artifacts = pRatingArtifact->GetMomentRatings(bPositiveMoment);

   RowIndexType row = 1;
   pgsRatingArtifact::MomentRatings::iterator iter(artifacts.begin());
   pgsRatingArtifact::MomentRatings::iterator end(artifacts.end());
   for ( ; iter != end; iter++ )
   {
      col = 0;
      const pgsPointOfInterest& poi = iter->first;
      pgsMomentRatingArtifact& artifact = iter->second;
      Float64 RF = artifact.GetRatingFactor();
      if ( 0 < RF && !ReportAtThisPoi(poi,controllingPoi) )
      {
         continue;
      }


      Float64 pM, nM, V;
      pgsTypes::LoadRatingType ratingType = artifact.GetLoadRatingType();
      pgsTypes::LimitState limit_state = (ratingType == pgsTypes::lrPermit_Special ? pgsTypes::FatigueI : pgsTypes::StrengthI);
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
   rptParagraph* pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pPara;
   *pPara << _T("Rating for Shear") << rptNewLine;

   if ( bSplicedGirder )
   {
      *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("ShearRatingEquationPT.png") ) << rptNewLine;
   }
   else
   {
      *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("ShearRatingEquation.png") ) << rptNewLine;
   }

   ColumnIndexType nColumns = 13;
   if ( bSplicedGirder )
   {
      nColumns += 8;
   }
   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(nColumns);
   
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

   INIT_SCALAR_PROTOTYPE(rptRcScalar, scalar, pDisplayUnits->GetScalarFormat());

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

   pgsRatingArtifact::ShearRatings artifacts = pRatingArtifact->GetShearRatings();

   RowIndexType row = 1;
   pgsRatingArtifact::ShearRatings::iterator iter(artifacts.begin());
   pgsRatingArtifact::ShearRatings::iterator end(artifacts.end());
   for ( ; iter != end; iter++ )
   {
      col = 0;
      const pgsPointOfInterest& poi = iter->first;
      pgsShearRatingArtifact& artifact = iter->second;
      Float64 RF = artifact.GetRatingFactor();
      if ( !poi.HasAttribute(POI_CRITSECTSHEAR1) && !poi.HasAttribute(POI_CRITSECTSHEAR2) && 0 < RF && !ReportAtThisPoi(poi,controllingPoi) )
      {
         continue;
      }


      Float64 gpM, gnM, gV;
      pgsTypes::LoadRatingType ratingType = artifact.GetLoadRatingType();
      pgsTypes::LimitState limit_state = (ratingType == pgsTypes::lrPermit_Special ? pgsTypes::FatigueI : pgsTypes::StrengthI);
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
   rptParagraph* pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pPara;
   *pPara << _T("Rating for Stress") << rptNewLine;

   if ( bSplicedGirder )
   {
      *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("StressRatingEquationPT.png") ) << rptNewLine;
   }
   else
   {
      *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("StressRatingEquation.png") ) << rptNewLine;
   }

   ColumnIndexType nColumns = 13;
   if (bSplicedGirder)
   {
      nColumns += 9;
   }
   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(nColumns);
   
   table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
   table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   
   table->SetColumnStyle(1,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
   table->SetStripeRowColumnStyle(1,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   *pPara << table << rptNewLine;
   pPara = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
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
   (*table)(0,col++) << COLHDR(RPT_STRESS(_T("allow")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,col++) << COLHDR(RPT_STRESS(_T("ps")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   if (bSplicedGirder)
   {
      (*table)(0,col++) << COLHDR(RPT_STRESS(_T("pt")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   }
   (*table)(0,col++) << COLHDR(RPT_STRESS(_T("r")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
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

   pgsRatingArtifact::StressRatings artifacts = pRatingArtifact->GetStressRatings();

   GET_IFACE2(pBroker,IBridge,pBridge);

   LPCTSTR stressLocation[] = {_T("Bottom Girder"),_T("Top Girder"),_T("Bottom Deck"),_T("Top Deck")};

   RowIndexType row = 1;
   pgsRatingArtifact::StressRatings::iterator iter(artifacts.begin());
   pgsRatingArtifact::StressRatings::iterator end(artifacts.end());
   for ( ; iter != end; iter++ )
   {
      col = 0;
      const pgsPointOfInterest& poi = iter->first;
      pgsStressRatingArtifact& artifact = iter->second;
      Float64 RF = artifact.GetRatingFactor();

      if ( 0 < RF && !ReportAtThisPoi(poi,controllingPoi) )
      {
         continue;
      }

      Float64 pM, nM, V;
      pgsTypes::LoadRatingType ratingType = artifact.GetLoadRatingType();
      pgsTypes::LimitState limit_state = (ratingType == pgsTypes::lrPermit_Special ? pgsTypes::FatigueI : pgsTypes::StrengthI);
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
   pgsRatingArtifact::YieldStressRatios artifacts = pRatingArtifact->GetYieldStressRatios(bPositiveMoment);
   if ( artifacts.size() == 0 )
   {
      return;
   }

   rptParagraph* pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
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
      *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("ReinforcementYieldingEquationPT.png") ) << rptNewLine;
   }
   else
   {
      *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("ReinforcementYieldingEquation.png") ) << rptNewLine;
   }

   *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("ReinforcementYieldingParameters.png") ) << rptNewLine;

   ColumnIndexType nColumns = 18;
   if ( bSplicedGirder )
   {
      nColumns += 4;
   }
   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(nColumns);
   
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
      pgsYieldStressRatioArtifact& artifact = iter->second;

      Float64 rebarSR = artifact.GetRebarStressRatio();
      Float64 strandSR = artifact.GetStrandStressRatio();
      Float64 tendonSR = artifact.GetTendonStressRatio();

      Float64 SR = Min(rebarSR,strandSR,tendonSR);

      if ( 1 <= SR && !ReportAtThisPoi(poi,controllingPoi) )
      {
         continue;
      }

      IndexType srIdx = MinIndex(rebarSR,strandSR,tendonSR);

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
      else
      {
         artifact.GetTendon(&d,&f,&fy,&E);
         fcr = artifact.GetTendonCrackingStressIncrement();
         fs = artifact.GetTendonStress();
         fallow = artifact.GetTendonAllowableStress();
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
      else
      {
         (*table)(row,col++) << _T("Tendon");
      }

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
   if ( poi == controllingPoi || 
        poi.IsTenthPoint(POI_SPAN) || 
        poi.HasAttribute(POI_CLOSURE) || 
        poi.HasAttribute(POI_SPAN | POI_CANTILEVER)
      )
   {
      return true;
   }
   else
   {
      return false;
   }
}
