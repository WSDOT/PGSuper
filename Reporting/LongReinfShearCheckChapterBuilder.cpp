///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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

#include <PgsExt\ReportPointOfInterest.h>
#include <PgsExt\GirderArtifact.h>
#include <PgsExt\RatingArtifact.h>

#include <PsgLib\SpecLibraryEntry.h>

#include <EAF\EAFDisplayUnits.h>
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\Artifact.h>
#include <IFace\RatingSpecification.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Intervals.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/****************************************************************************
CLASS
   CLongReinfShearCheckChapterBuilder
****************************************************************************/

   void  create_table1_design(rptChapter* pChapter,IBroker* pBroker,
                              IntervalIndexType intervalIdx,
                              pgsTypes::LimitState ls,
                              bool bUHPC,
                              const pgsGirderArtifact* pGirderArtifact,
                              IEAFDisplayUnits* pDisplayUnits,
                              Uint16 level);

   void create_table2_design(rptChapter* pChapter,IBroker* pBroker,
                              IntervalIndexType intervalIdx,
                              pgsTypes::LimitState ls,
                              bool bUHPC,
                              const pgsGirderArtifact* pGirderArtifact,
                              IEAFDisplayUnits* pDisplayUnits,
                              Uint16 level);

   void create_table3_design(rptChapter* pChapter, IBroker* pBroker,
                              IntervalIndexType intervalIdx,
                              pgsTypes::LimitState ls,
                              bool bUHPC,
                              const pgsGirderArtifact* pGirderArtifact,
                              IEAFDisplayUnits* pDisplayUnits,
                              Uint16 level);

   void create_table1_rating(rptChapter* pChapter,IBroker* pBroker,
                              IntervalIndexType intervalIdx,
                              pgsTypes::LimitState ls,
                              bool bUHPC,
                              const pgsRatingArtifact::LongitudinalReinforcementForShear& longReinfShear,
                              IEAFDisplayUnits* pDisplayUnits,
                              Uint16 level);

   void create_table2_rating(rptChapter* pChapter,IBroker* pBroker,
                              IntervalIndexType intervalIdx,
                              pgsTypes::LimitState ls,
                              bool bUHPC,
                              const pgsRatingArtifact::LongitudinalReinforcementForShear& longReinfShear,
                              IEAFDisplayUnits* pDisplayUnits,
                              Uint16 level);

   void create_table3_rating(rptChapter* pChapter,IBroker* pBroker,
                              IntervalIndexType intervalIdx,
                              pgsTypes::LimitState ls,
                              bool bUHPC,
                              const pgsRatingArtifact::LongitudinalReinforcementForShear& longReinfShear,
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
   return TEXT("Longitudinal Reinforcement for Shear Details");
}

rptChapter* CLongReinfShearCheckChapterBuilder::Build(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,Uint16 level) const
{
   auto pBrokerRptSpec = std::dynamic_pointer_cast<const CBrokerReportSpecification>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pBrokerRptSpec->GetBroker(&pBroker);

   bool bDesign = m_bDesign;
   bool bRating = m_bRating;

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   // material type is encoded in the longitudinal reinforcement artifact, but we have to dig down and find them
   // it is easier to just get that information from the material model
   auto pGdrRptSpec = std::dynamic_pointer_cast<const CGirderReportSpecification>(pRptSpec);
   auto pGdrLineRptSpec = std::dynamic_pointer_cast<const CGirderLineReportSpecification>(pRptSpec);

   CGirderKey girderKey;

   if (pGdrRptSpec)
   {
      girderKey = pGdrRptSpec->GetGirderKey();
   }
   else
   {
      girderKey = pGdrLineRptSpec->GetGirderKey();
   }

   GET_IFACE2(pBroker, IBridge, pBridge);
   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);

   bool bUHPC = false;
   GET_IFACE2(pBroker, IMaterials, pMaterials);
   for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
   {
      if (pMaterials->GetSegmentConcreteType(CSegmentKey(girderKey, segIdx)) == pgsTypes::UHPC)
      {
         bUHPC = true;
         break;
      }
   }

   if ( bDesign )
   {
      BuildForDesign(pChapter,pRptSpec, bUHPC,level);
   }

   if ( bRating )
   {
      BuildForRating(pChapter,pRptSpec, bUHPC, level);
   }

   return pChapter;
      
}

void CLongReinfShearCheckChapterBuilder::BuildForDesign(rptChapter* pChapter,const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,bool bUHPC, Uint16 level) const
{
   auto pGirderRptSpec = std::dynamic_pointer_cast<const CGirderReportSpecification>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pGirderRptSpec->GetBroker(&pBroker);
   const CGirderKey& girderKey(pGirderRptSpec->GetGirderKey());

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker,ILimitStateForces,pLimitStateForces);
   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType intervalIdx = pIntervals->GetIntervalCount()-1;

   bool bPermit = pLimitStateForces->IsStrengthIIApplicable(girderKey);
   pgsTypes::LimitState ls = pgsTypes::StrengthI;

   GET_IFACE2(pBroker,IGirderTendonGeometry,pTendonGeom);
   DuctIndexType nDucts = pTendonGeom->GetDuctCount(girderKey);
   
   rptParagraph* pParagraph;
   pParagraph = new rptParagraph;
   *pChapter << pParagraph;

   if (bUHPC)
   {
      *pParagraph << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("UHPC_LongitudinalReinforcementForShear.png")) << rptNewLine;
   }
   else
   {
      if (0 < nDucts)
      {
         if (WBFL::LRFD::LRFDVersionMgr::Version::EighthEdition2017 <= WBFL::LRFD::LRFDVersionMgr::GetVersion())
         {
            *pParagraph << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("LongitudinalReinforcementForShear2017_with_PT.png")) << rptNewLine;
         }
         else if (WBFL::LRFD::LRFDVersionMgr::Version::ThirdEditionWith2005Interims <= WBFL::LRFD::LRFDVersionMgr::GetVersion())
         {
            *pParagraph << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("LongitudinalReinforcementForShear2005_with_PT.png")) << rptNewLine;
         }
         else
         {
            *pParagraph << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("LongitudinalReinforcementForShear_with_PT.png")) << rptNewLine;
         }
      }
      else
      {
         if (WBFL::LRFD::LRFDVersionMgr::Version::EighthEdition2017 <= WBFL::LRFD::LRFDVersionMgr::GetVersion())
         {
            *pParagraph << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("LongitudinalReinforcementForShear2017.png")) << rptNewLine;
         }
         else if (WBFL::LRFD::LRFDVersionMgr::Version::ThirdEditionWith2005Interims <= WBFL::LRFD::LRFDVersionMgr::GetVersion())
         {
            *pParagraph << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("LongitudinalReinforcementForShear2005.png")) << rptNewLine;
         }
         else
         {
            *pParagraph << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("LongitudinalReinforcementForShear.png")) << rptNewLine;
         }
      }
   }

   pParagraph = new rptParagraph();
   *pChapter << pParagraph;

   GET_IFACE2(pBroker,IArtifact,pIArtifact);
   const pgsGirderArtifact* pGirderArtifact = pIArtifact->GetGirderArtifact(girderKey);

   pParagraph = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pParagraph;
   *pParagraph << GetLimitStateString(ls) << rptNewLine;

   // tables of details
   create_table1_design(pChapter, pBroker, intervalIdx, ls, bUHPC, pGirderArtifact, pDisplayUnits, level);
   create_table2_design(pChapter, pBroker, intervalIdx, ls, bUHPC, pGirderArtifact, pDisplayUnits, level);
   create_table3_design(pChapter, pBroker, intervalIdx, ls, bUHPC, pGirderArtifact, pDisplayUnits, level);

   if ( bPermit )
   {
      ls = pgsTypes::StrengthII;

      pParagraph = new rptParagraph(rptStyleManager::GetHeadingStyle());
      *pChapter << pParagraph;
      *pParagraph << GetLimitStateString(ls) << rptNewLine;

      // tables of details
      create_table1_design(pChapter, pBroker, intervalIdx, ls, bUHPC, pGirderArtifact, pDisplayUnits, level);
      create_table2_design(pChapter, pBroker, intervalIdx, ls, bUHPC, pGirderArtifact, pDisplayUnits, level);
      create_table3_design(pChapter, pBroker, intervalIdx, ls, bUHPC, pGirderArtifact, pDisplayUnits, level);
   }
}


void CLongReinfShearCheckChapterBuilder::BuildForRating(rptChapter* pChapter, const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,bool bUHPC,Uint16 level) const
{
   auto pGdrRptSpec = std::dynamic_pointer_cast<const CGirderReportSpecification>(pRptSpec);
   auto pGdrLineRptSpec = std::dynamic_pointer_cast<const CGirderLineReportSpecification>(pRptSpec);

   CComPtr<IBroker> pBroker;
   CGirderKey girderKey;

   if ( pGdrRptSpec )
   {
      pGdrRptSpec->GetBroker(&pBroker);
      girderKey = pGdrRptSpec->GetGirderKey();
   }
   else
   {
      pGdrLineRptSpec->GetBroker(&pBroker);
      girderKey = pGdrLineRptSpec->GetGirderKey();
   }

   GET_IFACE2(pBroker, IGirderTendonGeometry, pTendonGeom);
   GET_IFACE2(pBroker, IArtifact, pIArtifact);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType intervalIdx = pIntervals->GetLiveLoadInterval();

   GET_IFACE2(pBroker,IRatingSpecification,pRatingSpec);
   std::vector<pgsTypes::LimitState> limitStates;
   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) && pRatingSpec->RateForShear(pgsTypes::lrDesign_Inventory) )
   {
      limitStates.push_back(pgsTypes::StrengthI_Inventory);
   }

   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating) && pRatingSpec->RateForShear(pgsTypes::lrDesign_Operating) )
   {
      limitStates.push_back(pgsTypes::StrengthI_Operating);
   }

   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) && pRatingSpec->RateForShear(pgsTypes::lrLegal_Routine) )
   {
      limitStates.push_back(pgsTypes::StrengthI_LegalRoutine);
   }

   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) && pRatingSpec->RateForShear(pgsTypes::lrLegal_Special) )
   {
      limitStates.push_back(pgsTypes::StrengthI_LegalSpecial);
   }

   if (pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Emergency) && pRatingSpec->RateForShear(pgsTypes::lrLegal_Emergency))
   {
      limitStates.push_back(pgsTypes::StrengthI_LegalEmergency);
   }

   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) && pRatingSpec->RateForShear(pgsTypes::lrPermit_Routine) )
   {
      limitStates.push_back(pgsTypes::StrengthII_PermitRoutine);
   }

   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) && pRatingSpec->RateForShear(pgsTypes::lrPermit_Special) )
   {
      limitStates.push_back(pgsTypes::StrengthII_PermitSpecial);
   }

   GET_IFACE2(pBroker,IBridge,pBridge);
   std::vector<CGirderKey> vGirderKeys;
   pBridge->GetGirderline(girderKey,&vGirderKeys);
   for(const auto& thisGirderKey : vGirderKeys)
   {
      DuctIndexType nDucts = pTendonGeom->GetDuctCount(thisGirderKey);

      std::vector<pgsTypes::LimitState>::iterator ls_iter;
      for ( ls_iter = limitStates.begin(); ls_iter != limitStates.end(); ls_iter++ )
      {
         pgsTypes::LimitState ls = *ls_iter;
         pgsTypes::LoadRatingType ratingType = ::RatingTypeFromLimitState(ls);

         rptParagraph* pParagraph = new rptParagraph;
         *pChapter << pParagraph;

         if (bUHPC)
         {
            *pParagraph << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("UHPC_LongitudinalReinforcementForShear.png")) << rptNewLine;
         }
         else
         {
            WBFL::LRFD::LRFDVersionMgr::Version vers = WBFL::LRFD::LRFDVersionMgr::GetVersion();
            if (0 < nDucts)
            {
               if (WBFL::LRFD::LRFDVersionMgr::Version::EighthEdition2017 <= vers)
               {
                  *pParagraph << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("LongitudinalReinforcementForShear2017_with_PT.png")) << rptNewLine;
               }
               else if (WBFL::LRFD::LRFDVersionMgr::Version::ThirdEditionWith2005Interims <= vers)
               {
                  *pParagraph << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("LongitudinalReinforcementForShear2005_with_PT.png")) << rptNewLine;
               }
               else
               {
                  *pParagraph << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("LongitudinalReinforcementForShear_with_PT.png")) << rptNewLine;
               }
            }
            else
            {
               if (WBFL::LRFD::LRFDVersionMgr::Version::EighthEdition2017 <= vers)
               {
                  *pParagraph << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("LongitudinalReinforcementForShear2017.png")) << rptNewLine;
               }
               else if (WBFL::LRFD::LRFDVersionMgr::Version::ThirdEditionWith2005Interims <= vers)
               {
                  *pParagraph << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("LongitudinalReinforcementForShear2005.png")) << rptNewLine;
               }
               else
               {
                  *pParagraph << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("LongitudinalReinforcementForShear.png")) << rptNewLine;
               }
            }
         }

         pParagraph = new rptParagraph();
         *pChapter << pParagraph;

         INIT_UV_PROTOTYPE( rptPointOfInterest,    location, pDisplayUnits->GetSpanLengthUnit(),      false );
         INIT_UV_PROTOTYPE( rptStressUnitValue,    stress,   pDisplayUnits->GetStressUnit(),          false );
         INIT_UV_PROTOTYPE( rptAreaPerLengthValue, avs,      pDisplayUnits->GetAvOverSUnit(),         false );
         INIT_UV_PROTOTYPE( rptLengthUnitValue,    dim,      pDisplayUnits->GetComponentDimUnit(),    false );

         location.IncludeSpanAndGirder(true);

         const pgsRatingArtifact* pRatingArtifact = pIArtifact->GetRatingArtifact(thisGirderKey,ratingType,INVALID_INDEX/*all vehicles*/);
         const pgsRatingArtifact::LongitudinalReinforcementForShear& longReinfShear = pRatingArtifact->GetLongitudinalReinforcementForShear();


         pParagraph = new rptParagraph(rptStyleManager::GetHeadingStyle());
         *pChapter << pParagraph;
         *pParagraph << GetLimitStateString(ls) << rptNewLine;

         // tables of details
         create_table1_rating(pChapter, pBroker, intervalIdx, ls, bUHPC, longReinfShear, pDisplayUnits, level);
         create_table2_rating(pChapter, pBroker, intervalIdx, ls, bUHPC, longReinfShear, pDisplayUnits, level);
         create_table3_rating(pChapter, pBroker, intervalIdx, ls, bUHPC, longReinfShear, pDisplayUnits, level);
      }
   } // next group
}

std::unique_ptr<WBFL::Reporting::ChapterBuilder> CLongReinfShearCheckChapterBuilder::Clone() const
{
   return std::make_unique<CLongReinfShearCheckChapterBuilder>(m_bDesign,m_bRating);
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

void create_table1_design(rptChapter* pChapter,IBroker* pBroker,
                           IntervalIndexType intervalIdx,
                           pgsTypes::LimitState ls,
                           bool bUHPC,
                           const pgsGirderArtifact* pGirderArtifact,
                           IEAFDisplayUnits* pDisplayUnits,
                           Uint16 level)
{
   const CGirderKey& girderKey(pGirderArtifact->GetGirderKey());

   INIT_UV_PROTOTYPE( rptPointOfInterest,    location, pDisplayUnits->GetSpanLengthUnit(),   false );
   INIT_UV_PROTOTYPE( rptStressUnitValue,    stress,   pDisplayUnits->GetStressUnit(),       false );
   INIT_UV_PROTOTYPE( rptAreaUnitValue,      area,     pDisplayUnits->GetAreaUnit(),         false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,    dim,      pDisplayUnits->GetComponentDimUnit(), false );
   INIT_UV_PROTOTYPE( rptForceSectionValue,  shear,    pDisplayUnits->GetShearUnit(),        false );
   INIT_UV_PROTOTYPE( rptMomentSectionValue, moment,   pDisplayUnits->GetMomentUnit(),       false );
   INIT_UV_PROTOTYPE(rptStressUnitValue, modE, pDisplayUnits->GetModEUnit(), false);

   rptRcScalar scalar;
   scalar.SetFormat(WBFL::System::NumericFormatTool::Format::Automatic);
   scalar.SetWidth(6);
   scalar.SetPrecision(3);

   GET_IFACE2(pBroker, IBridge, pBridge);
   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
   location.IncludeSpanAndGirder(1 < nSegments);

   GET_IFACE2(pBroker, ISegmentTendonGeometry, pSegmentTendonGeometry);
   DuctIndexType nMaxSegmentDucts = pSegmentTendonGeometry->GetMaxDuctCount(girderKey);

   GET_IFACE2(pBroker, IGirderTendonGeometry, pGirderTendonGeometry);
   DuctIndexType nGirderDucts = pGirderTendonGeometry->GetDuctCount(girderKey);

   ColumnIndexType nColumns = 5;
   if (0 < nMaxSegmentDucts)
   {
      nColumns += 2;
   }

   if ( 0 < nGirderDucts )
   {
      nColumns += 2;
   }

   if (bUHPC)
   {
      nColumns += 4;
   }

   rptParagraph* pParagraph = new rptParagraph();
   *pChapter << pParagraph;

   rptRcTable* table = rptStyleManager::CreateDefaultTable(nColumns,_T("Longitudinal Reinforcement Shear Check Details - Table 1 of 3"));
   *pParagraph << table;

   pParagraph = new rptParagraph(rptStyleManager::GetFootnoteStyle());
   *pChapter << pParagraph;
   *pParagraph << Super(_T("*")) << _T(" ") << _T("Adjusted for development length. ") << RPT_FPS << _T(" = ") << RPT_STRESS(_T("ps,avg")) << _T(" from moment capacity analysis") << rptNewLine;
   if (0 < nMaxSegmentDucts)
   {
      *pParagraph << Sub2(_T("A"), _T("pts")) << _T(" = Area of segment tendons") << rptNewLine;
      *pParagraph << RPT_STRESS(_T("pts")) << _T(" = Stress in segment tendons") << rptNewLine;
   }
   if (0 < nGirderDucts)
   {
      *pParagraph << Sub2(_T("A"), _T("ptg")) << _T(" = Area of girder tendons") << rptNewLine;
      *pParagraph << RPT_STRESS(_T("ptg")) << _T(" = Stress in girder tendons") << rptNewLine;
   }
   if (0 < nMaxSegmentDucts + nGirderDucts)
   {
      *pParagraph << Sub2(_T("A"), _T("pt")) << RPT_STRESS(_T("pt")) << _T(" = ") << Sub2(_T("A"), _T("pts")) << RPT_STRESS(_T("pts")) << _T(" + ") << Sub2(_T("A"), _T("ptg")) << RPT_STRESS(_T("ptg")) << rptNewLine;
   }

   ColumnIndexType col = 0;

   (*table)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table)(0,col++) << COLHDR(Sub2(_T("A"),_T("s")),rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );
   (*table)(0,col++) << COLHDR(RPT_FY << Super(_T("*")),rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   if (bUHPC)
   {
      (*table)(0, col++) << COLHDR(RPT_ES, rptStressUnitTag, pDisplayUnits->GetStressUnit());
      (*table)(0, col++) << Sub2(symbol(gamma), _T("u")) << symbol(epsilon) << Sub(_T("t,loc")) << _T(" x 1000");
   }

   (*table)(0,col++) << COLHDR(Sub2(_T("A"),_T("ps")),rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );
   (*table)(0,col++) << COLHDR(RPT_FPS << Super(_T("*")),rptStressUnitTag, pDisplayUnits->GetStressUnit() );

   if (0 < nMaxSegmentDucts)
   {
      (*table)(0, col++) << COLHDR(Sub2(_T("A"), _T("pts")), rptAreaUnitTag, pDisplayUnits->GetAreaUnit());
      (*table)(0, col++) << COLHDR(RPT_STRESS(_T("pts")) << Super(_T("*")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
   }

   if ( 0 < nGirderDucts )
   {
      (*table)(0,col++) << COLHDR(Sub2(_T("A"),_T("ptg")),rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );
      (*table)(0,col++) << COLHDR(RPT_STRESS(_T("ptg")) << Super(_T("*")),rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   }

   if (bUHPC)
   {
      (*table)(0, col++) << COLHDR(Sub2(_T("A"), _T("ct")), rptAreaUnitTag, pDisplayUnits->GetAreaUnit());
      (*table)(0, col++) << COLHDR(Sub2(symbol(gamma), _T("u")) << RPT_STRESS(_T("t,cr")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
   }

   RowIndexType row = table->GetNumberOfHeaderRows();

   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      const pgsSegmentArtifact* pSegmentArtifact = pGirderArtifact->GetSegmentArtifact(segIdx);
      const pgsStirrupCheckArtifact* pStirrupArtifact = pSegmentArtifact->GetStirrupCheckArtifact();
      IndexType nArtifacts = pStirrupArtifact->GetStirrupCheckAtPoisArtifactCount( intervalIdx,ls );
      for ( IndexType idx = 0; idx < nArtifacts; idx++ )
      {
         const pgsStirrupCheckAtPoisArtifact* psArtifact = pStirrupArtifact->GetStirrupCheckAtPoisArtifact( intervalIdx,ls,idx );
         if ( psArtifact == nullptr )
         {
            continue;
         }

         const pgsPointOfInterest& poi = psArtifact->GetPointOfInterest();

         const CSegmentKey& segmentKey(poi.GetSegmentKey());
         ATLASSERT(CSegmentKey(girderKey, segIdx).IsEqual(segmentKey));

         const pgsLongReinfShearArtifact* pArtifact = psArtifact->GetLongReinfShearArtifact();

         if ( pArtifact->IsApplicable() )
         {
            col = 0;
            (*table)(row,col++) << location.SetValue( POI_SPAN, poi );
            (*table)(row,col++) << area.SetValue( pArtifact->GetAs());
            (*table)(row,col++) << stress.SetValue( pArtifact->GetFy());
            if (bUHPC)
            {
               if(pArtifact->IsUHPC())
               {
                  (*table)(row, col++) << modE.SetValue(pArtifact->GetEs());

                  Float64 gamma_u = pArtifact->GetFiberOrientationReductionFactor();
                  (*table)(row, col++) << scalar.SetValue(gamma_u*pArtifact->GetCrackLocalizationStrain() * 1000);
               }
               else
               {
                  (*table)(row, col++) << _T("");
                  (*table)(row, col++) << _T("");
               }
            }

            (*table)(row,col++) << area.SetValue( pArtifact->GetAps());
            (*table)(row,col++) << stress.SetValue( pArtifact->GetFps());

            if (0 < nMaxSegmentDucts)
            {
               (*table)(row, col++) << area.SetValue(pArtifact->GetAptSegment());
               (*table)(row, col++) << stress.SetValue(pArtifact->GetFptSegment());
            }

            if ( 0 < nGirderDucts )
            {
               (*table)(row,col++) << area.SetValue( pArtifact->GetAptGirder());
               (*table)(row,col++) << stress.SetValue( pArtifact->GetFptGirder());
            }

            if (bUHPC)
            {
               if (pArtifact->IsUHPC())
               {
                  (*table)(row, col++) << area.SetValue(pArtifact->GetAct());

                  Float64 gamma_u = pArtifact->GetFiberOrientationReductionFactor();
                  (*table)(row, col++) << stress.SetValue(gamma_u * pArtifact->GetDesignEffectiveConcreteStrength());
               }
               else
               {
                  (*table)(row, col++) << _T("");
                  (*table)(row, col++) << _T("");
               }
            }

            row++;
         }
      } // next artifact
   } // next segment
}

void create_table2_design(rptChapter* pChapter,IBroker* pBroker,
                           IntervalIndexType intervalIdx,
                           pgsTypes::LimitState ls,
                           bool bUHPC,
                           const pgsGirderArtifact* pGirderArtifact,
                           IEAFDisplayUnits* pDisplayUnits,
                           Uint16 level)
{
   const CGirderKey& girderKey(pGirderArtifact->GetGirderKey());


   INIT_UV_PROTOTYPE( rptPointOfInterest,    location, pDisplayUnits->GetSpanLengthUnit(),   false );
   INIT_UV_PROTOTYPE( rptStressUnitValue,    stress,   pDisplayUnits->GetStressUnit(),       false );
   INIT_UV_PROTOTYPE( rptAreaUnitValue,      area,     pDisplayUnits->GetAreaUnit(),         false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,    dim,      pDisplayUnits->GetComponentDimUnit(), false );
   INIT_UV_PROTOTYPE( rptForceSectionValue,  shear,    pDisplayUnits->GetShearUnit(),        false );
   INIT_UV_PROTOTYPE( rptMomentSectionValue, moment,   pDisplayUnits->GetMomentUnit(),       false );
   INIT_UV_PROTOTYPE( rptAngleUnitValue,     angle,    pDisplayUnits->GetAngleUnit(),        false );

   rptRcScalar scalar;
   scalar.SetFormat( WBFL::System::NumericFormatTool::Format::Automatic );
   scalar.SetWidth(6);
   scalar.SetPrecision(3);

   GET_IFACE2(pBroker,ISpecification,pSpec);
   GET_IFACE2(pBroker,ILibrary,pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   ATLASSERT(pSpecEntry!=nullptr);

   rptParagraph* pParagraph = new rptParagraph();
   *pChapter << pParagraph;

   rptRcTable* table = rptStyleManager::CreateDefaultTable(11,_T("Longitudinal Reinforcement Shear Check Details - Table 2 of 3"));
   *pParagraph << table;

   if (WBFL::LRFD::LRFDVersionMgr::Version::SecondEditionWith2000Interims <= WBFL::LRFD::LRFDVersionMgr::GetVersion())
   {
      pParagraph = new rptParagraph(rptStyleManager::GetFootnoteStyle());
      *pChapter << pParagraph;
      *pParagraph << Super(_T("*")) << _T(" ") << Sub2(_T("V"), _T("s")) << _T(" shall not be taken greater than ") << Sub2(_T("V"), _T("u")) << _T("/") << Sub2(symbol(phi), _T("v")) << rptNewLine;
   }

   ColumnIndexType col = 0;

   (*table)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table)(0, col++) << COLHDR(_T("M") << Sub(_T("u")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit());
   (*table)(0, col++) << COLHDR(_T("d") << Sub(_T("v")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
   (*table)(0, col++) << _T("Flexure") << rptNewLine << Sub2(symbol(phi), _T("f"));
   (*table)(0,col++) << COLHDR(_T("N")<<Sub(_T("u")),rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   (*table)(0,col++) << _T("Axial") << rptNewLine << Sub2(symbol(phi),_T("a"));
   (*table)(0,col++) << COLHDR(_T("V")<<Sub(_T("u")),rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   (*table)(0,col++) << _T("Shear") << rptNewLine << Sub2(symbol(phi),_T("v"));
   
   if ( pSpecEntry->GetSpecificationType() < WBFL::LRFD::LRFDVersionMgr::Version::SecondEditionWith2000Interims )
   {
      (*table)(0,col++) << COLHDR(_T("V")<<Sub(_T("s")),rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   }
   else
   {
      (*table)(0,col++) << COLHDR(_T("V")<<Sub(_T("s")) << Super(_T("*")),rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   }

   (*table)(0,col++) << COLHDR(_T("V")<<Sub(_T("p")),rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   (*table)(0,col++) << COLHDR(symbol(theta),rptAngleUnitTag, pDisplayUnits->GetAngleUnit() );

   RowIndexType row = table->GetNumberOfHeaderRows();

   GET_IFACE2(pBroker,IBridge,pBridge);
   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
   location.IncludeSpanAndGirder(1 < nSegments);

   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      const pgsSegmentArtifact* pSegmentArtifact = pGirderArtifact->GetSegmentArtifact(segIdx);
      const pgsStirrupCheckArtifact* pStirrupArtifact = pSegmentArtifact->GetStirrupCheckArtifact();
      IndexType nArtifacts = pStirrupArtifact->GetStirrupCheckAtPoisArtifactCount( intervalIdx,ls );

      for ( IndexType idx = 0; idx < nArtifacts; idx++ )
      {
         const pgsStirrupCheckAtPoisArtifact* psArtifact = pStirrupArtifact->GetStirrupCheckAtPoisArtifact( intervalIdx,ls,idx );
         if ( psArtifact == nullptr )
            continue;

         const pgsPointOfInterest& poi = psArtifact->GetPointOfInterest();

         const pgsLongReinfShearArtifact* pArtifact = psArtifact->GetLongReinfShearArtifact();

         if ( pArtifact->IsApplicable() )
         {
            col = 0;
            (*table)(row,col++) << location.SetValue( POI_SPAN, poi );
            (*table)(row, col++) << moment.SetValue(pArtifact->GetMu());
            (*table)(row, col++) << dim.SetValue(pArtifact->GetDv());
            (*table)(row, col++) << pArtifact->GetFlexuralPhi();
            (*table)(row,col++) << shear.SetValue( pArtifact->GetNu());
            (*table)(row,col++) << scalar.SetValue( pArtifact->GetAxialPhi());
            (*table)(row,col++) << shear.SetValue( pArtifact->GetVu());
            (*table)(row,col++) << scalar.SetValue( pArtifact->GetShearPhi());
            (*table)(row,col++) << shear.SetValue( pArtifact->GetVs());
            (*table)(row,col++) << shear.SetValue( pArtifact->GetVp());
            (*table)(row,col++) << angle.SetValue( pArtifact->GetTheta());

            row++;
         }
      } // next artifact
   } // next segment
}


void create_table3_design(rptChapter* pChapter, IBroker* pBroker,
                           IntervalIndexType intervalIdx,
                           pgsTypes::LimitState ls,
                           bool bUHPC,
                           const pgsGirderArtifact* pGirderArtifact,
                           IEAFDisplayUnits* pDisplayUnits,
                           Uint16 level)
{
   const CGirderKey& girderKey(pGirderArtifact->GetGirderKey());

   INIT_UV_PROTOTYPE( rptPointOfInterest,    location, pDisplayUnits->GetSpanLengthUnit(),   false );
   INIT_UV_PROTOTYPE( rptStressUnitValue,    stress,   pDisplayUnits->GetStressUnit(),       false );
   INIT_UV_PROTOTYPE( rptAreaUnitValue,      area,     pDisplayUnits->GetAreaUnit(),         false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,    dim,      pDisplayUnits->GetComponentDimUnit(), false );
   INIT_UV_PROTOTYPE( rptForceSectionValue,  shear,    pDisplayUnits->GetShearUnit(),        false );
   INIT_UV_PROTOTYPE( rptMomentSectionValue, moment,   pDisplayUnits->GetMomentUnit(),       false );
   INIT_UV_PROTOTYPE( rptAngleUnitValue,     angle,    pDisplayUnits->GetAngleUnit(),        false );

   rptRcScalar scalar;
   scalar.SetFormat( WBFL::System::NumericFormatTool::Format::Automatic );
   scalar.SetWidth(6); 
   scalar.SetPrecision(3);

   GET_IFACE2(pBroker,ISpecification,pSpec);
   GET_IFACE2(pBroker,ILibrary,pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   ATLASSERT(pSpecEntry != nullptr);

   rptParagraph* pParagraph = new rptParagraph();
   *pChapter << pParagraph;

   rptRcTable* table = rptStyleManager::CreateDefaultTable(4,_T("Longitudinal Reinforcement Shear Check Details - Table 3 of 3"));
   *pParagraph << table;

   ColumnIndexType col = 0;

   (*table)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table)(0,col++) << COLHDR(_T("Demand"),rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   (*table)(0,col++) << COLHDR(_T("Capacity"),rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   (*table)(0,col++) << _T("Equation");

   RowIndexType row = table->GetNumberOfHeaderRows();


   GET_IFACE2(pBroker,IBridge,pBridge);
   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
   location.IncludeSpanAndGirder(1 < nSegments);

   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      const pgsSegmentArtifact* pSegmentArtifact = pGirderArtifact->GetSegmentArtifact(segIdx);
      const pgsStirrupCheckArtifact* pStirrupArtifact = pSegmentArtifact->GetStirrupCheckArtifact();
      IndexType nArtifacts = pStirrupArtifact->GetStirrupCheckAtPoisArtifactCount( intervalIdx,ls );

      for ( IndexType idx = 0; idx < nArtifacts; idx++ )
      {
         const pgsStirrupCheckAtPoisArtifact* psArtifact = pStirrupArtifact->GetStirrupCheckAtPoisArtifact( intervalIdx,ls,idx );
         if ( psArtifact == nullptr )
            continue;

         const pgsPointOfInterest& poi = psArtifact->GetPointOfInterest();

         const pgsLongReinfShearArtifact* pArtifact = psArtifact->GetLongReinfShearArtifact();

         if ( pArtifact->IsApplicable() )
         {
            col = 0;
            (*table)(row,col++) << location.SetValue( POI_SPAN, poi );
            (*table)(row,col++) << shear.SetValue( pArtifact->GetDemandForce());
            (*table)(row,col++) << shear.SetValue( pArtifact->GetCapacityForce());
            if (pArtifact->IsUHPC())
            {
               (*table)(row, col++) << (pArtifact->GetEquation() == 1 ? _T("1.7.3.5-1") : _T("1.7.3.5-2"));
            }
            else
            {
               (*table)(row, col++) << (pArtifact->GetEquation() == 1 ? WBFL::LRFD::LrfdCw8th(_T("5.8.3.5-1"), _T("5.7.3.5-1")) : WBFL::LRFD::LrfdCw8th(_T("5.8.3.5-2"), _T("5.7.3.5-2")));
            }

            row++;
         }
      } // next artifact
   } // next segment
}

//////////////////////////////////////////////////////////////////////////
void create_table1_rating(rptChapter* pChapter,IBroker* pBroker,
                           IntervalIndexType intervalIdx,
                           pgsTypes::LimitState ls,
                           bool bUHPC,
                           const pgsRatingArtifact::LongitudinalReinforcementForShear& longReinfShear,
                           IEAFDisplayUnits* pDisplayUnits,
                           Uint16 level)
{
   const CSegmentKey& segmentKey = longReinfShear.front().first.GetSegmentKey();
   CGirderKey girderKey(segmentKey);

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);

   INIT_UV_PROTOTYPE( rptPointOfInterest,    location, pDisplayUnits->GetSpanLengthUnit(),   false );
   INIT_UV_PROTOTYPE( rptStressUnitValue,    stress,   pDisplayUnits->GetStressUnit(),       false );
   INIT_UV_PROTOTYPE( rptAreaUnitValue,      area,     pDisplayUnits->GetAreaUnit(),         false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,    dim,      pDisplayUnits->GetComponentDimUnit(), false );
   INIT_UV_PROTOTYPE( rptForceSectionValue,  shear,    pDisplayUnits->GetShearUnit(),        false );
   INIT_UV_PROTOTYPE( rptMomentSectionValue, moment,   pDisplayUnits->GetMomentUnit(),       false );
   INIT_UV_PROTOTYPE(rptStressUnitValue, modE, pDisplayUnits->GetModEUnit(), false);

   rptRcScalar scalar;
   scalar.SetFormat(WBFL::System::NumericFormatTool::Format::Automatic);
   scalar.SetWidth(6);
   scalar.SetPrecision(3);

   location.IncludeSpanAndGirder(true);

   GET_IFACE2(pBroker, IBridge, pBridge);

   GET_IFACE2(pBroker, ISegmentTendonGeometry, pSegmentTendonGeometry);
   DuctIndexType nMaxSegmentDucts = pSegmentTendonGeometry->GetMaxDuctCount(girderKey);;

   GET_IFACE2(pBroker, IGirderTendonGeometry, pGirderTendonGeometry);
   DuctIndexType nGirderDucts = pGirderTendonGeometry->GetDuctCount(girderKey);

   ColumnIndexType nColumns = 5;
   if (0 < nMaxSegmentDucts)
   {
      nColumns += 2;
   }

   if (0 < nGirderDucts)
   {
      nColumns += 2;
   }

   if (bUHPC)
   {
      nColumns += 4;
   }

   rptRcTable* table = rptStyleManager::CreateDefaultTable(nColumns,_T("Longitudinal Reinforcement Shear Check Details - Table 1 of 3"));
   
   table->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   table->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   rptParagraph* pParagraph = new rptParagraph();
   *pChapter << pParagraph;
   *pParagraph << table;

   pParagraph = new rptParagraph(rptStyleManager::GetFootnoteStyle());
   *pChapter << pParagraph;
   *pParagraph << Super(_T("*")) << _T(" ") << _T("Adjusted for development length. ") << RPT_FPS << _T(" = ") << RPT_STRESS(_T("ps,avg")) << _T(" from moment capacity analysis") << rptNewLine;
   if (0 < nMaxSegmentDucts)
   {
      *pParagraph << Sub2(_T("A"), _T("pts")) << _T(" = Area of segment tendons") << rptNewLine;
      *pParagraph << Sub2(_T("f"), _T("pts")) << _T(" = Stress in segment tendons") << rptNewLine;
   }
   if (0 < nGirderDucts)
   {
      *pParagraph << Sub2(_T("A"), _T("ptg")) << _T(" = Area of girder tendons") << rptNewLine;
      *pParagraph << Sub2(_T("f"), _T("ptg")) << _T(" = Stress in girder tendons") << rptNewLine;
   }
   if (0 < nMaxSegmentDucts + nGirderDucts)
   {
      *pParagraph << Sub2(_T("A"), _T("pt")) << Sub2(_T("f"), _T("pt")) << _T(" = ") << Sub2(_T("A"), _T("pts")) << Sub2(_T("f"), _T("pts")) << _T(" + ") << Sub2(_T("A"), _T("ptg")) << Sub2(_T("f"), _T("ptg")) << rptNewLine;
   }

   ColumnIndexType col = 0;

   if ( intervalIdx == releaseIntervalIdx )
   {
      (*table)(0,col++) << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   }
   else
   {
      (*table)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   }

   (*table)(0,col++) << COLHDR(Sub2(_T("A"),_T("s")),rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );
   (*table)(0,col++) << COLHDR(RPT_FY << Super(_T("*")),rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   if (bUHPC)
   {
      (*table)(0, col++) << COLHDR(RPT_ES, rptStressUnitTag, pDisplayUnits->GetStressUnit());
      (*table)(0, col++) << symbol(epsilon) << Sub(_T("t,loc")) << _T(" x 1000");
   }

   (*table)(0,col++) << COLHDR(Sub2(_T("A"),_T("ps")),rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );
   (*table)(0,col++) << COLHDR(RPT_FPS << Super(_T("*")),rptStressUnitTag, pDisplayUnits->GetStressUnit() );

   if (0 < nMaxSegmentDucts)
   {
      (*table)(0, col++) << COLHDR(Sub2(_T("A"), _T("pts")), rptAreaUnitTag, pDisplayUnits->GetAreaUnit());
      (*table)(0, col++) << COLHDR(RPT_FPT << Super(_T("*")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
   }

   if ( 0 < nGirderDucts )
   {
      (*table)(0,col++) << COLHDR(Sub2(_T("A"),_T("ptg")),rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );
      (*table)(0,col++) << COLHDR(RPT_FPT << Super(_T("*")),rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   }

   if (bUHPC)
   {
      (*table)(0, col++) << COLHDR(Sub2(_T("A"), _T("ct")), rptAreaUnitTag, pDisplayUnits->GetAreaUnit());
      (*table)(0, col++) << COLHDR(Sub2(symbol(gamma), _T("u")) << RPT_STRESS(_T("t,cr")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
   }

   RowIndexType row = table->GetNumberOfHeaderRows();

   pgsRatingArtifact::LongitudinalReinforcementForShear::const_iterator i(longReinfShear.begin());
   pgsRatingArtifact::LongitudinalReinforcementForShear::const_iterator end(longReinfShear.end());
   for (; i != end; i++)
   {
      col = 0;
      const pgsPointOfInterest& poi = i->first;
      const CSegmentKey& segmentKey = poi.GetSegmentKey();

      const pgsLongReinfShearArtifact& artifact = i->second;
      if (artifact.IsApplicable())
      {
         Float64 endSize = pBridge->GetSegmentStartEndDistance(segmentKey);

         (*table)(row, col++) << location.SetValue(POI_SPAN, poi);
         (*table)(row, col++) << area.SetValue(artifact.GetAs());
         (*table)(row, col++) << stress.SetValue(artifact.GetFy());
         if (bUHPC)
         {
            if (artifact.IsUHPC())
            {
               (*table)(row, col++) << modE.SetValue(artifact.GetEs());
               (*table)(row, col++) << scalar.SetValue(artifact.GetCrackLocalizationStrain() * 1000);
            }
            else
            {
               (*table)(row, col++) << _T("");
               (*table)(row, col++) << _T("");
            }
         }

         (*table)(row, col++) << area.SetValue(artifact.GetAps());
         (*table)(row, col++) << stress.SetValue(artifact.GetFps());

         if (0 < nMaxSegmentDucts)
         {
            (*table)(row, col++) << area.SetValue(artifact.GetAptSegment());
            (*table)(row, col++) << stress.SetValue(artifact.GetFptSegment());
         }

         if (0 < nGirderDucts)
         {
            (*table)(row, col++) << area.SetValue(artifact.GetAptGirder());
            (*table)(row, col++) << stress.SetValue(artifact.GetFptGirder());
         }


         if (bUHPC)
         {
            if (artifact.IsUHPC())
            {
               (*table)(row, col++) << area.SetValue(artifact.GetAct());
               (*table)(row, col++) << stress.SetValue(artifact.GetDesignEffectiveConcreteStrength());
            }
            else
            {
               (*table)(row, col++) << _T("");
               (*table)(row, col++) << _T("");
            }
         }

         row++;
      }
   }
}

void create_table2_rating(rptChapter* pChapter,IBroker* pBroker,
                           IntervalIndexType intervalIdx,
                           pgsTypes::LimitState ls,
                           bool bUHPC,
                           const pgsRatingArtifact::LongitudinalReinforcementForShear& longReinfShear,
                           IEAFDisplayUnits* pDisplayUnits,
                           Uint16 level)
{
   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(longReinfShear.front().first.GetSegmentKey());

   INIT_UV_PROTOTYPE( rptPointOfInterest,    location, pDisplayUnits->GetSpanLengthUnit(),   false );
   INIT_UV_PROTOTYPE( rptStressUnitValue,    stress,   pDisplayUnits->GetStressUnit(),       false );
   INIT_UV_PROTOTYPE( rptAreaUnitValue,      area,     pDisplayUnits->GetAreaUnit(),         false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,    dim,      pDisplayUnits->GetComponentDimUnit(), false );
   INIT_UV_PROTOTYPE( rptForceSectionValue,  shear,    pDisplayUnits->GetShearUnit(),        false );
   INIT_UV_PROTOTYPE( rptMomentSectionValue, moment,   pDisplayUnits->GetMomentUnit(),       false );
   INIT_UV_PROTOTYPE( rptAngleUnitValue,     angle,    pDisplayUnits->GetAngleUnit(),        false );

   location.IncludeSpanAndGirder(true);

   rptRcScalar scalar;
   scalar.SetFormat( WBFL::System::NumericFormatTool::Format::Automatic );
   scalar.SetWidth(6);
   scalar.SetPrecision(3);

   GET_IFACE2(pBroker,ISpecification,pSpec);
   GET_IFACE2(pBroker,ILibrary,pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   ATLASSERT(pSpecEntry != nullptr);

   rptRcTable* table = rptStyleManager::CreateDefaultTable(11,_T("Longitudinal Reinforcement Shear Check Details - Table 2 of 3"));
   
   table->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   table->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   rptParagraph* pParagraph = new rptParagraph();
   *pChapter << pParagraph;

   *pParagraph << table;

   if (WBFL::LRFD::LRFDVersionMgr::Version::SecondEditionWith2000Interims <= WBFL::LRFD::LRFDVersionMgr::GetVersion())
   {
      pParagraph = new rptParagraph(rptStyleManager::GetFootnoteStyle());
      *pChapter << pParagraph;
      *pParagraph << Super(_T("*")) << _T(" ") << Sub2(_T("V"), _T("s")) << _T(" shall not be taken greater than ") << Sub2(_T("V"), _T("u")) << _T("/") << Sub2(symbol(phi), _T("v")) << rptNewLine;
   }

   ColumnIndexType col = 0;

   if ( intervalIdx == releaseIntervalIdx )
   {
      (*table)(0, col++) << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   }
   else
   {
      (*table)(0, col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   }


   (*table)(0, col++) << COLHDR(_T("M") << Sub(_T("u")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit());
   (*table)(0, col++) << COLHDR(_T("d") << Sub(_T("v")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
   (*table)(0, col++) << _T("Flexure") << rptNewLine << Sub2(symbol(phi), _T("f"));

   (*table)(0, col++) << COLHDR(_T("N")<<Sub(_T("u")),rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   (*table)(0, col++) << _T("Axial") << rptNewLine << Sub2(symbol(phi),_T("a"));
   (*table)(0, col++) << COLHDR(_T("V")<<Sub(_T("u")),rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   (*table)(0, col++) << _T("Shear") << rptNewLine << Sub2(symbol(phi),_T("v"));
   
   if ( pSpecEntry->GetSpecificationType() < WBFL::LRFD::LRFDVersionMgr::Version::SecondEditionWith2000Interims )
   {
      (*table)(0, col++) << COLHDR(_T("V")<<Sub(_T("s")),rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   }
   else
   {
      (*table)(0, col++) << COLHDR(_T("V")<<Sub(_T("s")) << Super(_T("*")),rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   }

   (*table)(0, col++) << COLHDR(_T("V")<<Sub(_T("p")),rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   (*table)(0, col++) << COLHDR(symbol(theta),rptAngleUnitTag, pDisplayUnits->GetAngleUnit() );

   RowIndexType row = table->GetNumberOfHeaderRows();

   GET_IFACE2(pBroker,IBridge,pBridge);

   pgsRatingArtifact::LongitudinalReinforcementForShear::const_iterator i(longReinfShear.begin());
   pgsRatingArtifact::LongitudinalReinforcementForShear::const_iterator end(longReinfShear.end());
   for ( ; i != end; i++ )
   {
      const pgsPointOfInterest& poi = i->first;
      const CSegmentKey& segmentKey = poi.GetSegmentKey();

      const pgsLongReinfShearArtifact& artifact = i->second;

      Float64 endSize = pBridge->GetSegmentStartEndDistance(segmentKey);

      if ( artifact.IsApplicable() )
      {
         col = 0;
         (*table)(row, col++) << location.SetValue( POI_SPAN, poi );
         (*table)(row, col++) << moment.SetValue(artifact.GetMu());
         (*table)(row, col++) << dim.SetValue(artifact.GetDv());
         (*table)(row, col++) << artifact.GetFlexuralPhi();
         (*table)(row, col++) << shear.SetValue( artifact.GetNu());
         (*table)(row, col++) << scalar.SetValue( artifact.GetAxialPhi());
         (*table)(row, col++) << shear.SetValue( artifact.GetVu());
         (*table)(row, col++) << scalar.SetValue( artifact.GetShearPhi());
         (*table)(row, col++) << shear.SetValue( artifact.GetVs());
         (*table)(row, col++) << shear.SetValue( artifact.GetVp());
         (*table)(row, col++) << angle.SetValue( artifact.GetTheta());

         row++;
      }
   }
}


void create_table3_rating(rptChapter* pChapter,IBroker* pBroker,
                           IntervalIndexType intervalIdx,
                           pgsTypes::LimitState ls,
                           bool bUHPC,
                           const pgsRatingArtifact::LongitudinalReinforcementForShear& longReinfShear,
                           IEAFDisplayUnits* pDisplayUnits,
                           Uint16 level)
{
   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(longReinfShear.front().first.GetSegmentKey());

   INIT_UV_PROTOTYPE( rptPointOfInterest,    location, pDisplayUnits->GetSpanLengthUnit(),   false );
   INIT_UV_PROTOTYPE( rptStressUnitValue,    stress,   pDisplayUnits->GetStressUnit(),       false );
   INIT_UV_PROTOTYPE( rptAreaUnitValue,      area,     pDisplayUnits->GetAreaUnit(),         false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,    dim,      pDisplayUnits->GetComponentDimUnit(), false );
   INIT_UV_PROTOTYPE( rptForceSectionValue,  shear,    pDisplayUnits->GetShearUnit(),        false );
   INIT_UV_PROTOTYPE( rptMomentSectionValue, moment,   pDisplayUnits->GetMomentUnit(),       false );
   INIT_UV_PROTOTYPE( rptAngleUnitValue,     angle,    pDisplayUnits->GetAngleUnit(),        false );

   location.IncludeSpanAndGirder(true);

   rptRcScalar scalar;
   scalar.SetFormat( WBFL::System::NumericFormatTool::Format::Automatic );
   scalar.SetWidth(6); 
   scalar.SetPrecision(3);

   GET_IFACE2(pBroker,ISpecification,pSpec);
   GET_IFACE2(pBroker,ILibrary,pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   ATLASSERT(pSpecEntry != nullptr);

   rptRcTable* table = rptStyleManager::CreateDefaultTable(4,_T("Longitudinal Reinforcement Shear Check Details - Table 3 of 3"));
   
   table->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   table->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   rptParagraph* pParagraph = new rptParagraph();
   *pChapter << pParagraph;

   *pParagraph << table;

   ColumnIndexType col = 0;

   if ( intervalIdx == releaseIntervalIdx )
   {
      (*table)(0, col++) << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   }
   else
   {
      (*table)(0, col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   }

   (*table)(0, col++) << COLHDR(_T("Demand"),rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   (*table)(0, col++) << COLHDR(_T("Capacity"),rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   (*table)(0, col++) << _T("Equation");

   RowIndexType row = table->GetNumberOfHeaderRows();

   GET_IFACE2(pBroker,IBridge,pBridge);

   pgsRatingArtifact::LongitudinalReinforcementForShear::const_iterator i(longReinfShear.begin());
   pgsRatingArtifact::LongitudinalReinforcementForShear::const_iterator end(longReinfShear.end());
   for ( ; i != end; i++ )
   {
      const pgsPointOfInterest& poi = i->first;
      const CSegmentKey& segmentKey = poi.GetSegmentKey();

      const pgsLongReinfShearArtifact& artifact = i->second;

      Float64 endSize = pBridge->GetSegmentStartEndDistance(segmentKey);

      if ( artifact.IsApplicable() )
      {
         col = 0;
         (*table)(row, col++) << location.SetValue( POI_SPAN, poi );
         (*table)(row, col++) << shear.SetValue( artifact.GetDemandForce());
         (*table)(row, col++) << shear.SetValue( artifact.GetCapacityForce());
         if (artifact.IsUHPC())
         {
            (*table)(row, col++) << _T("GS ") << (artifact.GetEquation() == 1 ? _T("1.7.3.5-1") : _T("1.7.3.5-2"));
         }
         else
         {
            (*table)(row, col++) << (artifact.GetEquation() == 1 ? WBFL::LRFD::LrfdCw8th(_T("5.8.3.5-1"), _T("5.7.3.5-1")) : WBFL::LRFD::LrfdCw8th(_T("5.8.3.5-2"), _T("5.7.3.5-2")));
         }

         row++;
      }
   }
}

