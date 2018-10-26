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
#include <Reporting\StirrupDetailingCheckChapterBuilder.h>
#include <Reporting\ReportNotes.h>

#include <PgsExt\PointOfInterest.h>
#include <PgsExt\GirderArtifact.h>

#include <EAF\EAFDisplayUnits.h>
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\Artifact.h>
#include <IFace\TransverseReinforcementSpec.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CStirrupDetailingCheckChapterBuilder
****************************************************************************/


rptParagraph* build_min_avs_paragraph(IBroker* pBroker,SpanIndexType span,GirderIndexType girder,
                                      pgsTypes::Stage stage,
                                      IEAFDisplayUnits* pDisplayUnits);

rptParagraph* build_max_spacing_paragraph(IBroker* pBroker,SpanIndexType span,GirderIndexType girder,
                                          pgsTypes::Stage stage, pgsTypes::LimitState ls,
                                          IEAFDisplayUnits* pDisplayUnits);

////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CStirrupDetailingCheckChapterBuilder::CStirrupDetailingCheckChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CStirrupDetailingCheckChapterBuilder::GetName() const
{
   return TEXT("Stirrup Detailing Check Details");
}

rptChapter* CStirrupDetailingCheckChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CSpanGirderReportSpecification* pSGRptSpec = dynamic_cast<CSpanGirderReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pSGRptSpec->GetBroker(&pBroker);
   SpanIndexType span = pSGRptSpec->GetSpan();
   GirderIndexType girder = pSGRptSpec->GetGirder();

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   pgsTypes::Stage stage = pgsTypes::BridgeSite3;

   GET_IFACE2(pBroker,ILiveLoads,pLiveLoads);
   bool bPermit = pLiveLoads->IsLiveLoadDefined(pgsTypes::lltPermit);

   *pChapter << build_min_avs_paragraph(pBroker,span,girder,stage,pDisplayUnits);

   *pChapter << build_max_spacing_paragraph(pBroker,span,girder,stage,pgsTypes::StrengthI,pDisplayUnits);
   if ( bPermit )
      *pChapter << build_max_spacing_paragraph(pBroker,span,girder,stage,pgsTypes::StrengthII,pDisplayUnits);

   return pChapter;
}

rptParagraph* build_min_avs_paragraph(IBroker* pBroker,SpanIndexType span,GirderIndexType girder,
                                      pgsTypes::Stage stage, 
                                      IEAFDisplayUnits* pDisplayUnits)
{
   rptParagraph* pParagraph;
   pParagraph = new rptParagraph();

  // Av/S check 5.8.2.5
   // picture depends on units
   std::string strImage;
   GET_IFACE2(pBroker,IBridgeMaterialEx,pMaterial);
   pgsTypes::ConcreteType concType = pMaterial->GetGdrConcreteType(span,girder);
   bool bHasAggSplittingStrength = pMaterial->DoesGdrConcreteHaveAggSplittingStrength(span,girder);
   switch( concType )
   {
   case pgsTypes::Normal:
      strImage = (IS_US_UNITS(pDisplayUnits) ? "AvOverSMin_NWC_US.png" : "AvOverSMin_NWC_SI.png");
      break;

   case pgsTypes::AllLightweight:
      if ( bHasAggSplittingStrength )
         strImage = (IS_US_UNITS(pDisplayUnits) ? "AvOverSMin_LWC_US.png" : "AvOverSMin_LWC_SI.png");
      else
         strImage = (IS_US_UNITS(pDisplayUnits) ? "AvOverSMin_ALWC_US.png" : "AvOverSMin_ALWC_SI.png");
      break;

   case pgsTypes::SandLightweight:
      if ( bHasAggSplittingStrength )
         strImage = (IS_US_UNITS(pDisplayUnits) ? "AvOverSMin_LWC_US.png" : "AvOverSMin_LWC_SI.png");
      else
         strImage = (IS_US_UNITS(pDisplayUnits) ? "AvOverSMin_SLWC_US.png" : "AvOverSMin_SLWC_SI.png");
      break;

   default:
      ATLASSERT(false);
   }

   *pParagraph << rptRcImage(pgsReportStyleHolder::GetImagePath() + strImage) << rptNewLine;


   INIT_UV_PROTOTYPE( rptPointOfInterest,    location, pDisplayUnits->GetSpanLengthUnit(),   false );
   INIT_UV_PROTOTYPE( rptStressUnitValue,    stress,   pDisplayUnits->GetStressUnit(),          false );
   INIT_UV_PROTOTYPE( rptAreaPerLengthValue, avs,      pDisplayUnits->GetAvOverSUnit(),  false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,    dim,     pDisplayUnits->GetComponentDimUnit(),  false );

   location.IncludeSpanAndGirder(span == ALL_SPANS);

   GET_IFACE2(pBroker,IArtifact,pIArtifact);
   const pgsGirderArtifact* gdrArtifact = pIArtifact->GetArtifact(span,girder);
   const pgsStirrupCheckArtifact* pstirrup_artifact= gdrArtifact->GetStirrupCheckArtifact();
   CHECK(pstirrup_artifact);

   *pParagraph << RPT_FC << " = "<<stress.SetValue(pstirrup_artifact->GetFc())<<" "<< stress.GetUnitTag()<<rptNewLine;
   *pParagraph << RPT_FY << " = "<<stress.SetValue(pstirrup_artifact->GetFy())<<" "<< stress.GetUnitTag()<<rptNewLine;

   if ( concType != pgsTypes::Normal && bHasAggSplittingStrength )
   {
      *pParagraph << RPT_STRESS("ct") << " = "<<stress.SetValue(pMaterial->GetGdrConcreteAggSplittingStrength(span,girder))<<" "<< stress.GetUnitTag()<<rptNewLine;
   }

   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(3,"");

   if ( span == ALL_SPANS )
   {
      table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   *pParagraph << table << rptNewLine;

   table->TableLabel() << "Details for Minimum Transverse Reinforcement Check - 5.8.2.5-1";
  
   if ( stage == pgsTypes::CastingYard )
      (*table)(0,0)  << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   else
      (*table)(0,0)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

   (*table)(0,1)  << COLHDR("b"<<Sub("v"),  rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,2)  << COLHDR("A" << Sub("v")<<"/S"<<Sub("min") , rptAreaPerLengthUnitTag, pDisplayUnits->GetAvOverSUnit() );

   // Fill up the table

   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);

   std::vector<pgsPointOfInterest> vPoi = pIPoi->GetPointsOfInterest( span, girder, stage, POI_TABULAR|POI_SHEAR );

   Float64 end_size = pBridge->GetGirderStartConnectionLength(span,girder);
   if ( stage == pgsTypes::CastingYard )
      end_size = 0; // don't adjust if CY stage

   RowIndexType row = table->GetNumberOfHeaderRows();
   std::vector<pgsPointOfInterest>::const_iterator i;
   for ( i = vPoi.begin(); i != vPoi.end(); i++ )
   {
      const pgsPointOfInterest& poi = *i;

      // it is ok to use a hard coded StrengthI limit state here because
      // we are only after Bv and Avs Min withc are not dependent on loading
      const pgsStirrupCheckAtPoisArtifact* psArtifact = pstirrup_artifact->GetStirrupCheckAtPoisArtifact( pgsStirrupCheckAtPoisArtifactKey(stage,pgsTypes::StrengthI,poi.GetDistFromStart()) );
      if ( psArtifact == NULL )
         continue;

      const pgsStirrupDetailArtifact* pArtifact = psArtifact->GetStirrupDetailArtifact();

      (*table)(row,0) << location.SetValue( stage, poi, end_size );

      (*table)(row,1) << dim.SetValue(pArtifact->GetBv());

      if (pArtifact->IsApplicable())
         (*table)(row,2) << avs.SetValue(pArtifact->GetAvsMin());
      else
         (*table)(row,2) << RPT_NA;

      row++;
   }

   return pParagraph;
}

rptParagraph* build_max_spacing_paragraph(IBroker* pBroker,SpanIndexType span,GirderIndexType girder,
                                    pgsTypes::Stage stage, pgsTypes::LimitState ls,
                                    IEAFDisplayUnits* pDisplayUnits)
{
   // Spacing check 5.8.2.7
   rptParagraph* pParagraph;
   pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());

   if ( ls == pgsTypes::StrengthI )
      *pParagraph << "Strength I";
   else
      *pParagraph << "Strength II";

   *pParagraph <<" - Details for Maximum Transverse Reinforcement Spacing Check - 5.8.2.7"<<rptNewLine;

   INIT_UV_PROTOTYPE( rptPointOfInterest,    location, pDisplayUnits->GetSpanLengthUnit(),   false );
   INIT_UV_PROTOTYPE( rptStressUnitValue,    stress,   pDisplayUnits->GetStressUnit(),          false );
   INIT_UV_PROTOTYPE( rptAreaUnitValue,      area,     pDisplayUnits->GetAreaUnit(),            false );
   INIT_UV_PROTOTYPE( rptAreaPerLengthValue, avs,      pDisplayUnits->GetAvOverSUnit(),  false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,    dim,     pDisplayUnits->GetComponentDimUnit(),  false );
   INIT_UV_PROTOTYPE( rptForceSectionValue,  shear,    pDisplayUnits->GetShearUnit(),        false );

   location.IncludeSpanAndGirder(span == ALL_SPANS);

   // get a little fancy here with the equation so it lines up
   rptRcTable* petable = pgsReportStyleHolder::CreateDefaultTable(2,"");

   if ( span == ALL_SPANS )
   {
      petable->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      petable->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   *pParagraph << petable << rptNewLine;

   GET_IFACE2(pBroker,ITransverseReinforcementSpec,pTransverseReinforcementSpec);
   Float64 s_under, s_over;
   pTransverseReinforcementSpec->GetMaxStirrupSpacing(&s_under, &s_over);

   GET_IFACE2(pBroker,ILibrary,pLib);
   GET_IFACE2(pBroker,ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   bool bAfter1999 = ( pSpecEntry->GetSpecificationType() >= lrfdVersionMgr::SecondEditionWith2000Interims ? true : false );

   if ( bAfter1999 )
      (*petable)(1,0) << "if V"<<Sub("u")<<" < 0.125 " << RPT_FC <<"b"<<Sub("v")<<"d"<<Sub("v")<<" : ";
   else
      (*petable)(1,0) << "if V"<<Sub("u")<<" < 0.1 " << RPT_FC <<"b"<<Sub("v")<<"d"<<Sub("v")<<" : ";

   (*petable)(1,1) << " S"<<Sub("max")<<"= min(0.8 d"<<Sub("v")<<", "<<dim.SetValue(s_under)<<dim.GetUnitTag()<<")";
   (*petable)(2,0) <<"Else : ";
   (*petable)(2,1) <<" S"<<Sub("max")<<"= min(0.4 d"<<Sub("v")<<", "<<dim.SetValue(s_over)<<dim.GetUnitTag()<<")";

   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(6,"");
   *pParagraph << table << rptNewLine;

   if ( stage == pgsTypes::CastingYard )
      (*table)(0,0)  << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   else
      (*table)(0,0)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

   (*table)(0,1)  << COLHDR("V"<<Sub("u"),       rptForceUnitTag,  pDisplayUnits->GetShearUnit() );

   if ( bAfter1999 )
      (*table)(0,2)  << COLHDR("0.125 " << RPT_FC <<"b"<<Sub("v")<<"d"<<Sub("v"),  rptForceUnitTag,  pDisplayUnits->GetShearUnit() );
   else
      (*table)(0,2)  << COLHDR("0.1 " << RPT_FC <<"b"<<Sub("v")<<"d"<<Sub("v"),  rptForceUnitTag,  pDisplayUnits->GetShearUnit() );

   (*table)(0,3)  << COLHDR("b"<<Sub("v"),  rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,4)  << COLHDR("d"<<Sub("v"),  rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,5)  << COLHDR("S" << Sub("max"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );

   // Fill up the table
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);
   GET_IFACE2(pBroker,IArtifact,pIArtifact);

   const pgsGirderArtifact* gdrArtifact = pIArtifact->GetArtifact(span,girder);
   const pgsStirrupCheckArtifact* pstirrup_artifact= gdrArtifact->GetStirrupCheckArtifact();
   CHECK(pstirrup_artifact);

   std::vector<pgsPointOfInterest> vPoi = pIPoi->GetPointsOfInterest( span, girder, stage, POI_TABULAR|POI_SHEAR );

   Float64 end_size = pBridge->GetGirderStartConnectionLength(span,girder);
   if ( stage == pgsTypes::CastingYard )
      end_size = 0; // don't adjust if CY stage

   RowIndexType row = table->GetNumberOfHeaderRows();
   std::vector<pgsPointOfInterest>::const_iterator i;
   for ( i = vPoi.begin(); i != vPoi.end(); i++ )
   {
      const pgsPointOfInterest& poi = *i;

      const pgsStirrupCheckAtPoisArtifact* psArtifact = pstirrup_artifact->GetStirrupCheckAtPoisArtifact( pgsStirrupCheckAtPoisArtifactKey(stage,ls,poi.GetDistFromStart()) );
      if ( psArtifact == NULL )
         continue;

      const pgsStirrupDetailArtifact* pArtifact = psArtifact->GetStirrupDetailArtifact();

      (*table)(row,0) << location.SetValue( stage, poi, end_size );
      (*table)(row,1) << shear.SetValue(pArtifact->GetVu());
      (*table)(row,2) << shear.SetValue(pArtifact->GetVuLimit());
      (*table)(row,3) << dim.SetValue(pArtifact->GetBv());
      (*table)(row,4) << dim.SetValue(pArtifact->GetDv());

      if (pArtifact->IsApplicable())
         (*table)(row,5) << dim.SetValue(pArtifact->GetSMax());
      else
         (*table)(row,5) << RPT_NA;

      row++;
   }

   return pParagraph;
}

CChapterBuilder* CStirrupDetailingCheckChapterBuilder::Clone() const
{
   return new CStirrupDetailingCheckChapterBuilder;
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PRIVATE    ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
//======================== INQUERY    =======================================
