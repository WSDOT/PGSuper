///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2011  Washington State Department of Transportation
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

#include <Reporting\LongReinfShearCheckChapterBuilder.h>
#include <Reporting\ReportNotes.h>

#include <PgsExt\PointOfInterest.h>
#include <PgsExt\GirderArtifact.h>
#include <PgsExt\RatingArtifact.h>

#include <PsgLib\SpecLibraryEntry.h>

#include <EAF\EAFDisplayUnits.h>
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\Artifact.h>
#include <IFace\RatingSpecification.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/****************************************************************************
CLASS
   CLongReinfShearCheckChapterBuilder
****************************************************************************/

   rptParagraph* create_table1_design(IBroker* pBroker,
                              pgsTypes::Stage stage,
                              pgsTypes::LimitState ls,
                              const std::vector<pgsPointOfInterest>& rPoi,
                              Float64 endSize,
                              const pgsStirrupCheckArtifact* pstirrupArtifact,
                              IEAFDisplayUnits* pDisplayUnits,
                              Uint16 level);

   rptParagraph* create_table2_design(IBroker* pBroker,
                              pgsTypes::Stage stage,
                              pgsTypes::LimitState ls,
                              const std::vector<pgsPointOfInterest>& rPoi,
                              Float64 endSize,
                              const pgsStirrupCheckArtifact* pstirrupArtifact,
                              IEAFDisplayUnits* pDisplayUnits,
                              Uint16 level);

   rptParagraph* create_table3_design(IBroker* pBroker,
                              pgsTypes::Stage stage,
                              pgsTypes::LimitState ls,
                              const std::vector<pgsPointOfInterest>& rPoi,
                              Float64 endSize,
                              const pgsStirrupCheckArtifact* pstirrupArtifact,
                              IEAFDisplayUnits* pDisplayUnits,
                              Uint16 level);

   rptParagraph* create_table1_rating(IBroker* pBroker,
                              pgsTypes::Stage stage,
                              pgsTypes::LimitState ls,
                              const pgsRatingArtifact::ShearRatings shearRatings,
                              IEAFDisplayUnits* pDisplayUnits,
                              Uint16 level);

   rptParagraph* create_table2_rating(IBroker* pBroker,
                              pgsTypes::Stage stage,
                              pgsTypes::LimitState ls,
                              const pgsRatingArtifact::ShearRatings shearRatings,
                              IEAFDisplayUnits* pDisplayUnits,
                              Uint16 level);

   rptParagraph* create_table3_rating(IBroker* pBroker,
                              pgsTypes::Stage stage,
                              pgsTypes::LimitState ls,
                              const pgsRatingArtifact::ShearRatings shearRatings,
                              IEAFDisplayUnits* pDisplayUnits,
                              Uint16 level);
////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CLongReinfShearCheckChapterBuilder::CLongReinfShearCheckChapterBuilder(bool bDesign,bool bRating,bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
   m_bDesign = bDesign;
   m_bRating = bRating;
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CLongReinfShearCheckChapterBuilder::GetName() const
{
   return TEXT("Longitudinal Reinforcement for Shear");
}

rptChapter* CLongReinfShearCheckChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CBrokerReportSpecification* pBrokerRptSpec = dynamic_cast<CBrokerReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pBrokerRptSpec->GetBroker(&pBroker);

   bool bDesign = m_bDesign;
   bool bRating;
   
   if ( m_bRating )
   {
      bRating = true;
   }
   else
   {
      // include load rating results if we are always load rating
      GET_IFACE2(pBroker,IRatingSpecification,pRatingSpec);
      bRating = pRatingSpec->AlwaysLoadRate();

      // if none of the rating types are enabled, skip the rating
      if ( !pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) &&
           !pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating) &&
           !pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) &&
           !pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) &&
           !pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) &&
           !pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) 
         )
         bRating = false;
   }

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   if ( bDesign )
      BuildForDesign(pChapter,pRptSpec,level);

   if ( bRating )
      BuildForRating(pChapter,pRptSpec,level);

   return pChapter;
      
}

void CLongReinfShearCheckChapterBuilder::BuildForDesign(rptChapter* pChapter,CReportSpecification* pRptSpec,Uint16 level) const
{
   USES_CONVERSION;
   CSpanGirderReportSpecification* pSGRptSpec = dynamic_cast<CSpanGirderReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pSGRptSpec->GetBroker(&pBroker);
   SpanIndexType span = pSGRptSpec->GetSpan();
   GirderIndexType girder = pSGRptSpec->GetGirder();

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker,ILiveLoads,pLiveLoads);
   bool bPermit = pLiveLoads->IsLiveLoadDefined(pgsTypes::lltPermit);
   
   pgsTypes::Stage stage = pgsTypes::BridgeSite3;
   pgsTypes::LimitState ls = pgsTypes::StrengthI;

   rptParagraph* pParagraph;

   pParagraph = new rptParagraph;
   *pChapter << pParagraph;
   if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
      *pParagraph <<rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("LongitudinalReinforcementForShear2005.png"))<<rptNewLine;
   else
      *pParagraph <<rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("LongitudinalReinforcementForShear.png"))<<rptNewLine;

   pParagraph = new rptParagraph();
   *pChapter << pParagraph;

   INIT_UV_PROTOTYPE( rptPointOfInterest,    location, pDisplayUnits->GetSpanLengthUnit(),      false );
   INIT_UV_PROTOTYPE( rptStressUnitValue,    stress,   pDisplayUnits->GetStressUnit(),          false );
   INIT_UV_PROTOTYPE( rptAreaPerLengthValue, avs,      pDisplayUnits->GetAvOverSUnit(),         false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,    dim,      pDisplayUnits->GetComponentDimUnit(),    false );

   location.IncludeSpanAndGirder(span == ALL_SPANS);

   GET_IFACE2(pBroker,IArtifact,pIArtifact);
   const pgsGirderArtifact* gdrArtifact = pIArtifact->GetArtifact(span,girder);
   const pgsStirrupCheckArtifact* pstirrup_artifact= gdrArtifact->GetStirrupCheckArtifact();
   CHECK(pstirrup_artifact);

   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);
   std::vector<pgsPointOfInterest> vPoi = pIPoi->GetPointsOfInterest( span, girder, stage, POI_TABULAR|POI_SHEAR );

   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 end_size = pBridge->GetGirderStartConnectionLength(span,girder);
   if ( stage == pgsTypes::CastingYard )
      end_size = 0; // don't adjust if CY stage

   pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;
   GET_IFACE2(pBroker,IStageMap,pStageMap);
   *pParagraph << OLE2T(pStageMap->GetLimitStateName(ls)) << rptNewLine;

   // tables of details
   rptParagraph* ppar1 = create_table1_design(pBroker, stage, ls, vPoi, end_size, pstirrup_artifact, pDisplayUnits, level);
   *pChapter <<ppar1;
   pParagraph = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
   *pChapter << pParagraph;
   *pParagraph << Super(_T("*")) << _T(" ") << _T("Adjusted for development length") << rptNewLine;

   rptParagraph* ppar2 = create_table2_design(pBroker, stage, ls, vPoi, end_size, pstirrup_artifact, pDisplayUnits, level);
   *pChapter <<ppar2;

   if ( lrfdVersionMgr::SecondEditionWith2000Interims <= lrfdVersionMgr::GetVersion()  )
   {
      pParagraph = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
      *pChapter << pParagraph;
      *pParagraph << Super(_T("*")) << _T(" ") << Sub2(_T("V"),_T("s")) << _T(" shall not be taken greater than ") << Sub2(_T("V"),_T("u")) << _T("/") << Sub2(symbol(phi),_T("v")) << rptNewLine;
   }

   rptParagraph* ppar3 = create_table3_design(pBroker, stage, ls, vPoi, end_size, pstirrup_artifact, pDisplayUnits, level);
   *pChapter <<ppar3;

   if ( bPermit )
   {
      ls = pgsTypes::StrengthII;

      pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      *pChapter << pParagraph;
      *pParagraph << _T("Strength II") << rptNewLine;

      // tables of details
      ppar1 = create_table1_design(pBroker, stage, ls, vPoi, end_size, pstirrup_artifact, pDisplayUnits, level);
      *pChapter <<ppar1;
      pParagraph = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
      *pChapter << pParagraph;
      *pParagraph << Super(_T("*")) << _T(" ") << _T("Adjusted for development length") << rptNewLine;

      ppar2 = create_table2_design(pBroker, stage, ls, vPoi, end_size, pstirrup_artifact, pDisplayUnits, level);
      *pChapter <<ppar2;

      if ( lrfdVersionMgr::SecondEditionWith2000Interims <= lrfdVersionMgr::GetVersion()  )
      {
         pParagraph = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
         *pChapter << pParagraph;
         *pParagraph << Super(_T("*")) << _T(" ") << Sub2(_T("V"),_T("s")) << _T(" shall not be taken greater than ") << Sub2(_T("V"),_T("u")) << _T("/") << Sub2(symbol(phi),_T("v")) << rptNewLine;
      }

      ppar3 = create_table3_design(pBroker, stage, ls, vPoi, end_size, pstirrup_artifact, pDisplayUnits, level);
      *pChapter <<ppar3;
   }
}


void CLongReinfShearCheckChapterBuilder::BuildForRating(rptChapter* pChapter,CReportSpecification* pRptSpec,Uint16 level) const
{
   USES_CONVERSION;
   CGirderReportSpecification* pGdrRptSpec = dynamic_cast<CGirderReportSpecification*>(pRptSpec);
   CSpanGirderReportSpecification* pSpanGdrRptSpec = dynamic_cast<CSpanGirderReportSpecification*>(pRptSpec);

   GirderIndexType gdrLineIdx;
   CComPtr<IBroker> pBroker;

   if ( pGdrRptSpec )
   {
      pGdrRptSpec->GetBroker(&pBroker);
      gdrLineIdx = pGdrRptSpec->GetGirder();
   }
   else
   {
      pSpanGdrRptSpec->GetBroker(&pBroker);
      gdrLineIdx = pSpanGdrRptSpec->GetGirder();
   }

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   
   pgsTypes::Stage stage = pgsTypes::BridgeSite3;

   GET_IFACE2(pBroker,IRatingSpecification,pRatingSpec);
   std::vector<pgsTypes::LimitState> limitStates;
   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) && pRatingSpec->RateForShear(pgsTypes::lrDesign_Inventory) )
      limitStates.push_back(pgsTypes::StrengthI_Inventory);

   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating) && pRatingSpec->RateForShear(pgsTypes::lrDesign_Operating) )
      limitStates.push_back(pgsTypes::StrengthI_Operating);

   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) && pRatingSpec->RateForShear(pgsTypes::lrLegal_Routine) )
      limitStates.push_back(pgsTypes::StrengthI_LegalRoutine);

   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) && pRatingSpec->RateForShear(pgsTypes::lrLegal_Special) )
      limitStates.push_back(pgsTypes::StrengthI_LegalSpecial);

   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) && pRatingSpec->RateForShear(pgsTypes::lrPermit_Routine) )
      limitStates.push_back(pgsTypes::StrengthII_PermitRoutine);

   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) && pRatingSpec->RateForShear(pgsTypes::lrPermit_Special) )
      limitStates.push_back(pgsTypes::StrengthII_PermitSpecial);

   std::vector<pgsTypes::LimitState>::iterator ls_iter;
   for ( ls_iter = limitStates.begin(); ls_iter != limitStates.end(); ls_iter++ )
   {
      pgsTypes::LimitState ls = *ls_iter;
      pgsTypes::LoadRatingType ratingType = ::RatingTypeFromLimitState(ls);

      rptParagraph* pParagraph;

      pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      *pChapter << pParagraph;
      *pParagraph << _T("5.8.3.5") << rptNewLine;
      if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
         *pParagraph <<rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("LongitudinalReinforcementForShear2005.png"))<<rptNewLine;
      else
         *pParagraph <<rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("LongitudinalReinforcementForShear.png"))<<rptNewLine;

      pParagraph = new rptParagraph();
      *pChapter << pParagraph;

      INIT_UV_PROTOTYPE( rptPointOfInterest,    location, pDisplayUnits->GetSpanLengthUnit(),      false );
      INIT_UV_PROTOTYPE( rptStressUnitValue,    stress,   pDisplayUnits->GetStressUnit(),          false );
      INIT_UV_PROTOTYPE( rptAreaPerLengthValue, avs,      pDisplayUnits->GetAvOverSUnit(),         false );
      INIT_UV_PROTOTYPE( rptLengthUnitValue,    dim,      pDisplayUnits->GetComponentDimUnit(),    false );

      location.IncludeSpanAndGirder(true);

      GET_IFACE2(pBroker,IArtifact,pIArtifact);
      const pgsRatingArtifact* pRatingArtifact = pIArtifact->GetRatingArtifact(gdrLineIdx,ratingType,INVALID_INDEX);
      pgsRatingArtifact::ShearRatings shearRatings = pRatingArtifact->GetShearRatings();


      pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      *pChapter << pParagraph;
      GET_IFACE2(pBroker,IStageMap,pStageMap);
      *pParagraph << OLE2T(pStageMap->GetLimitStateName(ls)) << rptNewLine;

      // tables of details
      rptParagraph* ppar1 = create_table1_rating(pBroker, stage, ls, shearRatings, pDisplayUnits, level);
      *pChapter <<ppar1;
      pParagraph = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
      *pChapter << pParagraph;
      *pParagraph << Super(_T("*")) << _T(" ") << _T("Adjusted for development length") << rptNewLine;

      rptParagraph* ppar2 = create_table2_rating(pBroker, stage, ls, shearRatings, pDisplayUnits, level);
      *pChapter <<ppar2;

      if ( lrfdVersionMgr::SecondEditionWith2000Interims <= lrfdVersionMgr::GetVersion()  )
      {
         pParagraph = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
         *pChapter << pParagraph;
         *pParagraph << Super(_T("*")) << _T(" ") << Sub2(_T("V"),_T("s")) << _T(" shall not be taken greater than ") << Sub2(_T("V"),_T("u")) << _T("/") << Sub2(symbol(phi),_T("v")) << rptNewLine;
      }

      rptParagraph* ppar3 = create_table3_rating(pBroker, stage, ls, shearRatings, pDisplayUnits, level);
      *pChapter <<ppar3;
   }
}

CChapterBuilder* CLongReinfShearCheckChapterBuilder::Clone() const
{
   return new CLongReinfShearCheckChapterBuilder(m_bDesign,m_bRating);
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

rptParagraph* create_table1_design(IBroker* pBroker,
                           pgsTypes::Stage stage,
                           pgsTypes::LimitState ls,
                           const std::vector<pgsPointOfInterest>& rPoi,
                           Float64 endSize,
                           const pgsStirrupCheckArtifact* pstirrupArtifact,
                           IEAFDisplayUnits* pDisplayUnits,
                           Uint16 level)
{
   rptParagraph* pParagraph = new rptParagraph();

   INIT_UV_PROTOTYPE( rptPointOfInterest,    location, pDisplayUnits->GetSpanLengthUnit(),   false );
   INIT_UV_PROTOTYPE( rptStressUnitValue,    stress,   pDisplayUnits->GetStressUnit(),          false );
   INIT_UV_PROTOTYPE( rptAreaUnitValue,      area,      pDisplayUnits->GetAreaUnit(),  false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,    dim,     pDisplayUnits->GetComponentDimUnit(),  false );
   INIT_UV_PROTOTYPE( rptForceSectionValue, shear,  pDisplayUnits->GetShearUnit(), false );
   INIT_UV_PROTOTYPE( rptMomentSectionValue, moment,  pDisplayUnits->GetMomentUnit(), false );

   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(8,_T("Longitudinal Reinforcement Shear Check Details - Table 1 of 3"));
   *pParagraph << table;

   if ( stage == pgsTypes::CastingYard )
      (*table)(0,0)  << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   else
      (*table)(0,0)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

   (*table)(0,1)  << COLHDR(Sub2(_T("A"),_T("s")),rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );
   (*table)(0,2)  << COLHDR(RPT_FY << Super(_T("*")),rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,3)  << COLHDR(Sub2(_T("A"),_T("ps")),rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );
   (*table)(0,4)  << COLHDR(_T("f")<<Sub(_T("ps")) << Super(_T("*")),rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,5)  << COLHDR(_T("M")<<Sub(_T("u")),rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   (*table)(0,6)  << COLHDR(_T("d")<<Sub(_T("v")),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,7)  << _T("Flexure") << rptNewLine << Sub2(symbol(phi),_T("f"));

   RowIndexType row = table->GetNumberOfHeaderRows();

   std::vector<pgsPointOfInterest>::const_iterator i;
   for ( i = rPoi.begin(); i != rPoi.end(); i++ )
   {
      const pgsPointOfInterest& poi = *i;

      const pgsStirrupCheckAtPoisArtifact* psArtifact = pstirrupArtifact->GetStirrupCheckAtPoisArtifact( pgsStirrupCheckAtPoisArtifactKey(stage,ls,poi.GetDistFromStart()) );
      if ( psArtifact == NULL )
         continue;

      const pgsLongReinfShearArtifact* pArtifact = psArtifact->GetLongReinfShearArtifact();

      if ( pArtifact->IsApplicable() )
      {
         (*table)(row,0) << location.SetValue( pgsTypes::BridgeSite3, poi, endSize );
         (*table)(row,1) << area.SetValue( pArtifact->GetAs());
         (*table)(row,2) << stress.SetValue( pArtifact->GetFy());
         (*table)(row,3) << area.SetValue( pArtifact->GetAps());
         (*table)(row,4) << stress.SetValue( pArtifact->GetFps());
         (*table)(row,5) << moment.SetValue( pArtifact->GetMu());
         (*table)(row,6) << dim.SetValue( pArtifact->GetDv());
         (*table)(row,7) << pArtifact->GetFlexuralPhi();

         row++;
      }
   }

   return pParagraph;
}

rptParagraph* create_table2_design(IBroker* pBroker,
                           pgsTypes::Stage stage,
                           pgsTypes::LimitState ls,
                           const std::vector<pgsPointOfInterest>& rPoi,
                           Float64 endSize,
                           const pgsStirrupCheckArtifact* pstirrupArtifact,
                           IEAFDisplayUnits* pDisplayUnits,
                           Uint16 level)
{
   rptParagraph* pParagraph = new rptParagraph();

   INIT_UV_PROTOTYPE( rptPointOfInterest,    location, pDisplayUnits->GetSpanLengthUnit(),   false );
   INIT_UV_PROTOTYPE( rptStressUnitValue,    stress,   pDisplayUnits->GetStressUnit(),          false );
   INIT_UV_PROTOTYPE( rptAreaUnitValue,      area,      pDisplayUnits->GetAreaUnit(),  false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,    dim,     pDisplayUnits->GetComponentDimUnit(),  false );
   INIT_UV_PROTOTYPE( rptForceSectionValue,  shear,  pDisplayUnits->GetShearUnit(), false );
   INIT_UV_PROTOTYPE( rptMomentSectionValue, moment,  pDisplayUnits->GetMomentUnit(), false );
   INIT_UV_PROTOTYPE( rptAngleUnitValue,     angle, pDisplayUnits->GetAngleUnit(), false );

   rptRcScalar scalar;
   scalar.SetFormat( sysNumericFormatTool::Automatic );
   scalar.SetWidth(6);
   scalar.SetPrecision(3);

   GET_IFACE2(pBroker,ISpecification,pSpec);
   GET_IFACE2(pBroker,ILibrary,pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   ATLASSERT(pSpecEntry!=0);

   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(8,_T("Longitudinal Reinforcement Shear Check Details - Table 2 of 3"));
   *pParagraph << table;

   if ( stage == pgsTypes::CastingYard )
      (*table)(0,0)  << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   else
      (*table)(0,0)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

   (*table)(0,1)  << COLHDR(_T("N")<<Sub(_T("u")),rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   (*table)(0,2)  << _T("Axial") << rptNewLine << Sub2(symbol(phi),_T("a"));
   (*table)(0,3)  << COLHDR(_T("V")<<Sub(_T("u")),rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   (*table)(0,4)  << _T("Shear") << rptNewLine << Sub2(symbol(phi),_T("v"));
   
   if ( pSpecEntry->GetSpecificationType() < lrfdVersionMgr::SecondEditionWith2000Interims )
      (*table)(0,5)  << COLHDR(_T("V")<<Sub(_T("s")),rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   else
      (*table)(0,5)  << COLHDR(_T("V")<<Sub(_T("s")) << Super(_T("*")),rptForceUnitTag, pDisplayUnits->GetShearUnit() );

   (*table)(0,6)  << COLHDR(_T("V")<<Sub(_T("p")),rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   (*table)(0,7)  << COLHDR(symbol(theta),rptAngleUnitTag, pDisplayUnits->GetAngleUnit() );

   RowIndexType row = table->GetNumberOfHeaderRows();

   std::vector<pgsPointOfInterest>::const_iterator i;
   for ( i = rPoi.begin(); i != rPoi.end(); i++ )
   {
      const pgsPointOfInterest& poi = *i;

      const pgsStirrupCheckAtPoisArtifact* psArtifact = pstirrupArtifact->GetStirrupCheckAtPoisArtifact( pgsStirrupCheckAtPoisArtifactKey(stage,ls,poi.GetDistFromStart()) );
      if ( psArtifact == NULL )
         continue;

      const pgsLongReinfShearArtifact* pArtifact = psArtifact->GetLongReinfShearArtifact();

      if ( pArtifact->IsApplicable() )
      {
         (*table)(row,0) << location.SetValue( pgsTypes::BridgeSite3, poi, endSize );
         (*table)(row,1) << shear.SetValue( pArtifact->GetNu());
         (*table)(row,2) << scalar.SetValue( pArtifact->GetAxialPhi());
         (*table)(row,3) << shear.SetValue( pArtifact->GetVu());
         (*table)(row,4) << scalar.SetValue( pArtifact->GetShearPhi());
         (*table)(row,5) << shear.SetValue( pArtifact->GetVs());
         (*table)(row,6) << shear.SetValue( pArtifact->GetVp());
         (*table)(row,7) << angle.SetValue( pArtifact->GetTheta());

         row++;
      }
   }

   return pParagraph;
}


rptParagraph* create_table3_design(IBroker* pBroker,
                           pgsTypes::Stage stage,
                           pgsTypes::LimitState ls,
                           const std::vector<pgsPointOfInterest>& rPoi,
                           Float64 endSize,
                           const pgsStirrupCheckArtifact* pstirrupArtifact,
                           IEAFDisplayUnits* pDisplayUnits,
                           Uint16 level)
{
   rptParagraph* pParagraph = new rptParagraph();

   INIT_UV_PROTOTYPE( rptPointOfInterest,    location, pDisplayUnits->GetSpanLengthUnit(),   false );
   INIT_UV_PROTOTYPE( rptStressUnitValue,    stress,   pDisplayUnits->GetStressUnit(),          false );
   INIT_UV_PROTOTYPE( rptAreaUnitValue,      area,      pDisplayUnits->GetAreaUnit(),  false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,    dim,     pDisplayUnits->GetComponentDimUnit(),  false );
   INIT_UV_PROTOTYPE( rptForceSectionValue,  shear,  pDisplayUnits->GetShearUnit(), false );
   INIT_UV_PROTOTYPE( rptMomentSectionValue, moment,  pDisplayUnits->GetMomentUnit(), false );
   INIT_UV_PROTOTYPE( rptAngleUnitValue,     angle, pDisplayUnits->GetAngleUnit(), false );

   rptRcScalar scalar;
   scalar.SetFormat( sysNumericFormatTool::Automatic );
   scalar.SetWidth(6); 
   scalar.SetPrecision(3);

   GET_IFACE2(pBroker,ISpecification,pSpec);
   GET_IFACE2(pBroker,ILibrary,pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   CHECK(pSpecEntry!=0);

   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(4,_T("Longitudinal Reinforcement Shear Check Details - Table 3 of 3"));
   *pParagraph << table;

   if ( stage == pgsTypes::CastingYard )
      (*table)(0,0)  << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   else
      (*table)(0,0)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

   (*table)(0,1)  << COLHDR(_T("Demand"),rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   (*table)(0,2)  << COLHDR(_T("Capacity"),rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   (*table)(0,3)  << _T("Equation");

   RowIndexType row = table->GetNumberOfHeaderRows();

   std::vector<pgsPointOfInterest>::const_iterator i;
   for ( i = rPoi.begin(); i != rPoi.end(); i++ )
   {
      const pgsPointOfInterest& poi = *i;

      const pgsStirrupCheckAtPoisArtifact* psArtifact = pstirrupArtifact->GetStirrupCheckAtPoisArtifact( pgsStirrupCheckAtPoisArtifactKey(stage,ls,poi.GetDistFromStart()) );
      if ( psArtifact == NULL )
         continue;

      const pgsLongReinfShearArtifact* pArtifact = psArtifact->GetLongReinfShearArtifact();

      if ( pArtifact->IsApplicable() )
      {
         (*table)(row,0) << location.SetValue( pgsTypes::BridgeSite3, poi, endSize );
         (*table)(row,1) << shear.SetValue( pArtifact->GetDemandForce());
         (*table)(row,2) << shear.SetValue( pArtifact->GetCapacityForce());
         (*table)(row,3) << (pArtifact->GetEquation() == 1 ? _T("5.8.3.5-1") : _T("5.8.3.5-2"));

         row++;
      }
   }

   return pParagraph;
}

//////////////////////////////////////////////////////////////////////////
rptParagraph* create_table1_rating(IBroker* pBroker,
                           pgsTypes::Stage stage,
                           pgsTypes::LimitState ls,
                           const pgsRatingArtifact::ShearRatings shearRatings,
                           IEAFDisplayUnits* pDisplayUnits,
                           Uint16 level)
{
   rptParagraph* pParagraph = new rptParagraph();

   INIT_UV_PROTOTYPE( rptPointOfInterest,    location, pDisplayUnits->GetSpanLengthUnit(),   false );
   INIT_UV_PROTOTYPE( rptStressUnitValue,    stress,   pDisplayUnits->GetStressUnit(),          false );
   INIT_UV_PROTOTYPE( rptAreaUnitValue,      area,      pDisplayUnits->GetAreaUnit(),  false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,    dim,     pDisplayUnits->GetComponentDimUnit(),  false );
   INIT_UV_PROTOTYPE( rptForceSectionValue, shear,  pDisplayUnits->GetShearUnit(), false );
   INIT_UV_PROTOTYPE( rptMomentSectionValue, moment,  pDisplayUnits->GetMomentUnit(), false );

   location.IncludeSpanAndGirder(true);

   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(8,_T("Longitudinal Reinforcement Shear Check Details - Table 1 of 3"));
   
   table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
   table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   *pParagraph << table;

   if ( stage == pgsTypes::CastingYard )
      (*table)(0,0)  << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   else
      (*table)(0,0)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

   (*table)(0,1)  << COLHDR(Sub2(_T("A"),_T("s")),rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );
   (*table)(0,2)  << COLHDR(RPT_FY << Super(_T("*")),rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,3)  << COLHDR(Sub2(_T("A"),_T("ps")),rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );
   (*table)(0,4)  << COLHDR(RPT_FPS << Super(_T("*")),rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,5)  << COLHDR(_T("M")<<Sub(_T("u")),rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   (*table)(0,6)  << COLHDR(_T("d")<<Sub(_T("v")),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,7)  << _T("Flexure") << rptNewLine << Sub2(symbol(phi),_T("f"));

   RowIndexType row = table->GetNumberOfHeaderRows();

   GET_IFACE2(pBroker,IBridge,pBridge);

   pgsRatingArtifact::ShearRatings::const_iterator i;
   for ( i = shearRatings.begin(); i != shearRatings.end(); i++ )
   {
      const pgsPointOfInterest& poi = i->first;
      const pgsShearRatingArtifact& rating = i->second;
      const pgsLongReinfShearArtifact& artifact = rating.GetLongReinfShearArtifact();

      Float64 endSize = pBridge->GetGirderStartConnectionLength(poi.GetSpan(),poi.GetGirder());

      if ( artifact.IsApplicable() )
      {
         (*table)(row,0) << location.SetValue( pgsTypes::BridgeSite3, poi, endSize );
         (*table)(row,1) << area.SetValue( artifact.GetAs());
         (*table)(row,2) << stress.SetValue( artifact.GetFy());
         (*table)(row,3) << area.SetValue( artifact.GetAps());
         (*table)(row,4) << stress.SetValue( artifact.GetFps());
         (*table)(row,5) << moment.SetValue( artifact.GetMu());
         (*table)(row,6) << dim.SetValue( artifact.GetDv());
         (*table)(row,7) << artifact.GetFlexuralPhi();

         row++;
      }
   }

   return pParagraph;
}

rptParagraph* create_table2_rating(IBroker* pBroker,
                           pgsTypes::Stage stage,
                           pgsTypes::LimitState ls,
                           const pgsRatingArtifact::ShearRatings shearRatings,
                           IEAFDisplayUnits* pDisplayUnits,
                           Uint16 level)
{
   rptParagraph* pParagraph = new rptParagraph();

   INIT_UV_PROTOTYPE( rptPointOfInterest,    location, pDisplayUnits->GetSpanLengthUnit(),   false );
   INIT_UV_PROTOTYPE( rptStressUnitValue,    stress,   pDisplayUnits->GetStressUnit(),          false );
   INIT_UV_PROTOTYPE( rptAreaUnitValue,      area,      pDisplayUnits->GetAreaUnit(),  false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,    dim,     pDisplayUnits->GetComponentDimUnit(),  false );
   INIT_UV_PROTOTYPE( rptForceSectionValue,  shear,  pDisplayUnits->GetShearUnit(), false );
   INIT_UV_PROTOTYPE( rptMomentSectionValue, moment,  pDisplayUnits->GetMomentUnit(), false );
   INIT_UV_PROTOTYPE( rptAngleUnitValue,     angle, pDisplayUnits->GetAngleUnit(), false );

   location.IncludeSpanAndGirder(true);

   rptRcScalar scalar;
   scalar.SetFormat( sysNumericFormatTool::Automatic );
   scalar.SetWidth(6);
   scalar.SetPrecision(3);

   GET_IFACE2(pBroker,ISpecification,pSpec);
   GET_IFACE2(pBroker,ILibrary,pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   ATLASSERT(pSpecEntry!=0);

   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(8,_T("Longitudinal Reinforcement Shear Check Details - Table 2 of 3"));
   
   table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
   table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   *pParagraph << table;

   if ( stage == pgsTypes::CastingYard )
      (*table)(0,0)  << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   else
      (*table)(0,0)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

   (*table)(0,1)  << COLHDR(_T("N")<<Sub(_T("u")),rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   (*table)(0,2)  << _T("Axial") << rptNewLine << Sub2(symbol(phi),_T("a"));
   (*table)(0,3)  << COLHDR(_T("V")<<Sub(_T("u")),rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   (*table)(0,4)  << _T("Shear") << rptNewLine << Sub2(symbol(phi),_T("v"));
   
   if ( pSpecEntry->GetSpecificationType() < lrfdVersionMgr::SecondEditionWith2000Interims )
      (*table)(0,5)  << COLHDR(_T("V")<<Sub(_T("s")),rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   else
      (*table)(0,5)  << COLHDR(_T("V")<<Sub(_T("s")) << Super(_T("*")),rptForceUnitTag, pDisplayUnits->GetShearUnit() );

   (*table)(0,6)  << COLHDR(_T("V")<<Sub(_T("p")),rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   (*table)(0,7)  << COLHDR(symbol(theta),rptAngleUnitTag, pDisplayUnits->GetAngleUnit() );

   RowIndexType row = table->GetNumberOfHeaderRows();

   GET_IFACE2(pBroker,IBridge,pBridge);

   pgsRatingArtifact::ShearRatings::const_iterator i;
   for ( i = shearRatings.begin(); i != shearRatings.end(); i++ )
   {
      const pgsPointOfInterest& poi = i->first;
      const pgsShearRatingArtifact& rating = i->second;
      const pgsLongReinfShearArtifact& artifact = rating.GetLongReinfShearArtifact();

      Float64 endSize = pBridge->GetGirderStartConnectionLength(poi.GetSpan(),poi.GetGirder());

      if ( artifact.IsApplicable() )
      {
         (*table)(row,0) << location.SetValue( pgsTypes::BridgeSite3, poi, endSize );
         (*table)(row,1) << shear.SetValue( artifact.GetNu());
         (*table)(row,2) << scalar.SetValue( artifact.GetAxialPhi());
         (*table)(row,3) << shear.SetValue( artifact.GetVu());
         (*table)(row,4) << scalar.SetValue( artifact.GetShearPhi());
         (*table)(row,5) << shear.SetValue( artifact.GetVs());
         (*table)(row,6) << shear.SetValue( artifact.GetVp());
         (*table)(row,7) << angle.SetValue( artifact.GetTheta());

         row++;
      }
   }

   return pParagraph;
}


rptParagraph* create_table3_rating(IBroker* pBroker,
                           pgsTypes::Stage stage,
                           pgsTypes::LimitState ls,
                           const pgsRatingArtifact::ShearRatings shearRatings,
                           IEAFDisplayUnits* pDisplayUnits,
                           Uint16 level)
{
   rptParagraph* pParagraph = new rptParagraph();

   INIT_UV_PROTOTYPE( rptPointOfInterest,    location, pDisplayUnits->GetSpanLengthUnit(),   false );
   INIT_UV_PROTOTYPE( rptStressUnitValue,    stress,   pDisplayUnits->GetStressUnit(),          false );
   INIT_UV_PROTOTYPE( rptAreaUnitValue,      area,      pDisplayUnits->GetAreaUnit(),  false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,    dim,     pDisplayUnits->GetComponentDimUnit(),  false );
   INIT_UV_PROTOTYPE( rptForceSectionValue,  shear,  pDisplayUnits->GetShearUnit(), false );
   INIT_UV_PROTOTYPE( rptMomentSectionValue, moment,  pDisplayUnits->GetMomentUnit(), false );
   INIT_UV_PROTOTYPE( rptAngleUnitValue,     angle, pDisplayUnits->GetAngleUnit(), false );

   location.IncludeSpanAndGirder(true);

   rptRcScalar scalar;
   scalar.SetFormat( sysNumericFormatTool::Automatic );
   scalar.SetWidth(6); 
   scalar.SetPrecision(3);

   GET_IFACE2(pBroker,ISpecification,pSpec);
   GET_IFACE2(pBroker,ILibrary,pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   CHECK(pSpecEntry!=0);

   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(4,_T("Longitudinal Reinforcement Shear Check Details - Table 3 of 3"));
   
   table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
   table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   *pParagraph << table;

   if ( stage == pgsTypes::CastingYard )
      (*table)(0,0)  << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   else
      (*table)(0,0)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

   (*table)(0,1)  << COLHDR(_T("Demand"),rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   (*table)(0,2)  << COLHDR(_T("Capacity"),rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   (*table)(0,3)  << _T("Equation");

   RowIndexType row = table->GetNumberOfHeaderRows();

   GET_IFACE2(pBroker,IBridge,pBridge);

   pgsRatingArtifact::ShearRatings::const_iterator i;
   for ( i = shearRatings.begin(); i != shearRatings.end(); i++ )
   {
      const pgsPointOfInterest& poi = i->first;
      const pgsShearRatingArtifact& rating = i->second;
      const pgsLongReinfShearArtifact& artifact = rating.GetLongReinfShearArtifact();

      Float64 endSize = pBridge->GetGirderStartConnectionLength(poi.GetSpan(),poi.GetGirder());

      if ( artifact.IsApplicable() )
      {
         (*table)(row,0) << location.SetValue( pgsTypes::BridgeSite3, poi, endSize );
         (*table)(row,1) << shear.SetValue( artifact.GetDemandForce());
         (*table)(row,2) << shear.SetValue( artifact.GetCapacityForce());
         (*table)(row,3) << (artifact.GetEquation() == 1 ? _T("5.8.3.5-1") : _T("5.8.3.5-2"));

         row++;
      }
   }

   return pParagraph;
}


