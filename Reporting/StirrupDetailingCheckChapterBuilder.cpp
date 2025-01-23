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
#include <Reporting\StirrupDetailingCheckChapterBuilder.h>
#include <Reporting\ReportNotes.h>

#include <PgsExt\ReportPointOfInterest.h>
#include <PgsExt\GirderArtifact.h>

#include <EAF\EAFDisplayUnits.h>
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\Artifact.h>
#include <IFace\TransverseReinforcementSpec.h>
#include <IFace\Intervals.h>
#include <IFace\AnalysisResults.h>
#include <IFace\ReportOptions.h>

#include <psgLib/ShearCapacityCriteria.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

void build_min_avs_paragraph(IBroker* pBroker,rptChapter* pChapter,const CGirderKey& girderKey,
                                      IntervalIndexType intervalIdx, bool doIncludeSpanAndGirderForPois,
                                      IEAFDisplayUnits* pDisplayUnits);

void build_max_spacing_paragraph(IBroker* pBroker,rptChapter* pChapter,const CGirderKey& girderKey,
                                          IntervalIndexType intervalIdx, pgsTypes::LimitState ls,bool doIncludeSpanAndGirderForPois,
                                          IEAFDisplayUnits* pDisplayUnits);

void build_max_spacing_paragraph_uhpc(IBroker* pBroker, rptChapter* pChapter, const CGirderKey& girderKey,
   IntervalIndexType intervalIdx, pgsTypes::LimitState ls,bool doIncludeSpanAndGirderForPois,
   IEAFDisplayUnits* pDisplayUnits);

/****************************************************************************
CLASS
   CStirrupDetailingCheckChapterBuilder
****************************************************************************/

CStirrupDetailingCheckChapterBuilder::CStirrupDetailingCheckChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

LPCTSTR CStirrupDetailingCheckChapterBuilder::GetName() const
{
   return TEXT("Stirrup Detailing Check Details");
}

rptChapter* CStirrupDetailingCheckChapterBuilder::Build(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,Uint16 level) const
{
   auto pGirderRptSpec = std::dynamic_pointer_cast<const CGirderReportSpecification>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pGirderRptSpec->GetBroker(&pBroker);
   const CGirderKey& girderKey = pGirderRptSpec->GetGirderKey();

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType intervalIdx = pIntervals->GetIntervalCount()-1;

   GET_IFACE2(pBroker,ILimitStateForces,pLimitStateForces);
   bool bPermit = pLimitStateForces->IsStrengthIIApplicable(girderKey);

   GET_IFACE2(pBroker,IReportOptions,pReportOptions);
   bool bIncludeSpanAndGirderForPois = pReportOptions->IncludeSpanAndGirder4Pois(girderKey);

   build_min_avs_paragraph(pBroker,pChapter,girderKey,intervalIdx, bIncludeSpanAndGirderForPois, pDisplayUnits);

   GET_IFACE2(pBroker, IMaterials, pMaterials);
   if (pMaterials->GetSegmentConcreteType(CSegmentKey(girderKey, 0)) == pgsTypes::UHPC)
   {
      build_max_spacing_paragraph_uhpc(pBroker, pChapter, girderKey, intervalIdx, pgsTypes::StrengthI, bIncludeSpanAndGirderForPois, pDisplayUnits);
      if (bPermit)
      {
         build_max_spacing_paragraph_uhpc(pBroker, pChapter, girderKey, intervalIdx, pgsTypes::StrengthII,bIncludeSpanAndGirderForPois, pDisplayUnits);
      }
   }
   else
   {
      build_max_spacing_paragraph(pBroker, pChapter, girderKey, intervalIdx, pgsTypes::StrengthI,bIncludeSpanAndGirderForPois, pDisplayUnits);
      if (bPermit)
      {
         build_max_spacing_paragraph(pBroker, pChapter, girderKey, intervalIdx, pgsTypes::StrengthII,bIncludeSpanAndGirderForPois,pDisplayUnits);
      }
   }

   return pChapter;
}

std::unique_ptr<WBFL::Reporting::ChapterBuilder> CStirrupDetailingCheckChapterBuilder::Clone() const
{
   return std::make_unique<CStirrupDetailingCheckChapterBuilder>();
}

void build_min_avs_paragraph(IBroker* pBroker,rptChapter* pChapter,const CGirderKey& girderKey,
                                      IntervalIndexType intervalIdx,bool bIncludeSpanAndGirderForPois,
                                      IEAFDisplayUnits* pDisplayUnits)
{
   rptParagraph* pParagraph;

   INIT_UV_PROTOTYPE( rptPointOfInterest,    location, pDisplayUnits->GetSpanLengthUnit(),   false );
   INIT_UV_PROTOTYPE( rptStressUnitValue,    stress,   pDisplayUnits->GetStressUnit(),          false );
   INIT_UV_PROTOTYPE( rptAreaPerLengthValue, avs,      pDisplayUnits->GetAvOverSUnit(),  false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,    dim,     pDisplayUnits->GetComponentDimUnit(),  false );

   location.IncludeSpanAndGirder(bIncludeSpanAndGirderForPois);

   GET_IFACE2(pBroker,IBridge,pBridge);
   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);

   GET_IFACE2(pBroker,IArtifact,pIArtifact);
   const pgsGirderArtifact* pGirderArtifact = pIArtifact->GetGirderArtifact(girderKey);

   bool bLambda = (WBFL::LRFD::BDSManager::Edition::SeventhEditionWith2016Interims <= WBFL::LRFD::BDSManager::GetEdition() ? true : false);

   GET_IFACE2(pBroker,IMaterials,pMaterial);
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      if ( 1 < nSegments )
      {
         pParagraph = new rptParagraph(rptStyleManager::GetSubheadingStyle());
         *pChapter << pParagraph;
         *pParagraph << _T("Segment ") << LABEL_SEGMENT(segIdx) << rptNewLine;
      }

      pParagraph = new rptParagraph;
      *pChapter << pParagraph;

      CSegmentKey segmentKey(girderKey,segIdx);
      pgsTypes::ConcreteType concType = pMaterial->GetSegmentConcreteType(segmentKey);

      const pgsSegmentArtifact* pSegmentArtifact = pGirderArtifact->GetSegmentArtifact(segIdx);
      const pgsStirrupCheckArtifact* pStirrupArtifact = pSegmentArtifact->GetStirrupCheckArtifact();
      ATLASSERT(pStirrupArtifact);

      if (concType == pgsTypes::PCI_UHPC)
      {
         *pParagraph << _T("PCI SDG E.7.2.2 - There is no requirement for minimum transverse reinforcement in PCI-UHPC members.") << rptNewLine;
         return;
      }
      else if (concType == pgsTypes::UHPC)
      {
         pParagraph = new rptParagraph(rptStyleManager::GetHeadingStyle());
         *pChapter << pParagraph;
         *pParagraph << _T("Details for Minimum Transverse Reinforcement Check - GS 1.7.2.5");

         pParagraph = new rptParagraph;
         *pChapter << pParagraph;
         *pParagraph << _T("Transverse shear reinforcement need not be provided where not required, as specifed in GS Article 1.7.2.3") << rptNewLine;
         return;
      }
      else
      {
         bool bHasAggSplittingStrength = pMaterial->DoesSegmentConcreteHaveAggSplittingStrength(segmentKey);

         // Av/S check 5.7.2.5 (pre2017: 5.8.2.5)
         // picture depends on units
         std::_tstring strImage;
         if (bLambda)
         {
            strImage = _T("AvOverSMin_2016.png");
         }
         else
         {
            switch (concType)
            {
            case pgsTypes::Normal:
               strImage = (IS_US_UNITS(pDisplayUnits) ? _T("AvOverSMin_NWC_US.png") : _T("AvOverSMin_NWC_SI.png"));
               break;

            case pgsTypes::AllLightweight:
               if (bHasAggSplittingStrength)
               {
                  strImage = (IS_US_UNITS(pDisplayUnits) ? _T("AvOverSMin_LWC_US.png") : _T("AvOverSMin_LWC_SI.png"));
               }
               else
               {
                  strImage = (IS_US_UNITS(pDisplayUnits) ? _T("AvOverSMin_ALWC_US.png") : _T("AvOverSMin_ALWC_SI.png"));
               }
               break;

            case pgsTypes::SandLightweight:
               if (bHasAggSplittingStrength)
               {
                  strImage = (IS_US_UNITS(pDisplayUnits) ? _T("AvOverSMin_LWC_US.png") : _T("AvOverSMin_LWC_SI.png"));
               }
               else
               {
                  strImage = (IS_US_UNITS(pDisplayUnits) ? _T("AvOverSMin_SLWC_US.png") : _T("AvOverSMin_SLWC_SI.png"));
               }
               break;

            case pgsTypes::PCI_UHPC:
            case pgsTypes::UHPC:
            default:
               ATLASSERT(false);
            }
         }

         *pParagraph << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + strImage) << rptNewLine;

         const pgsStirrupCheckAtPoisArtifact* psArtifact = pStirrupArtifact->GetStirrupCheckAtPoisArtifact(intervalIdx, pgsTypes::StrengthI, 0);
         const pgsStirrupDetailArtifact* pArtifact = psArtifact->GetStirrupDetailArtifact();

         *pParagraph << RPT_FC << _T(" = ") << stress.SetValue(pArtifact->GetFc()) << _T(" ") << stress.GetUnitTag() << rptNewLine;
         *pParagraph << RPT_FY << _T(" = ") << stress.SetValue(pArtifact->GetFy()) << _T(" ") << stress.GetUnitTag() << rptNewLine;

         if (concType != pgsTypes::Normal && bHasAggSplittingStrength)
         {
            *pParagraph << RPT_STRESS(_T("ct")) << _T(" = ") << stress.SetValue(pMaterial->GetSegmentConcreteAggSplittingStrength(segmentKey)) << _T(" ") << stress.GetUnitTag() << rptNewLine;
         }
      } // end if non-uhpc segment

      if ( segIdx != nSegments-1 )
      {
         // closure joints can't be UHPC, yet... however, the UHPC code is stubbed out here so it's ready to go in the future
         ATLASSERT(!IsUHPC(concType)); 

         CClosureKey closureKey(girderKey,segIdx);
         pgsTypes::ConcreteType concType = pMaterial->GetClosureJointConcreteType(closureKey);
         bool bHasAggSplittingStrength = pMaterial->DoesClosureJointConcreteHaveAggSplittingStrength(closureKey);

         pParagraph = new rptParagraph(rptStyleManager::GetSubheadingStyle());
         *pChapter << pParagraph;
         *pParagraph << _T("Closure Joint ") << LABEL_SEGMENT(segIdx) << rptNewLine;

         pParagraph = new rptParagraph;
         *pChapter << pParagraph;

         if (concType == pgsTypes::PCI_UHPC)
         {
            *pParagraph << _T("PCI SDG E.7.2.2 - There is no requirement for minimum transverse reinforcement in PCI-UHPC members.") << rptNewLine;
         }
         else if (concType == pgsTypes::UHPC)
         {
            *pParagraph << _T("GS 1.7.2.5 - There is no requirement for minimum transverse reinforcement in UHPC members.") << rptNewLine;
         }
         else
         {
            // Av/S check 5.7.2.5 (pre2017: 5.8.2.5)
            // picture depends on units
            std::_tstring strImage;
            switch (concType)
            {
            case pgsTypes::Normal:
               strImage = (IS_US_UNITS(pDisplayUnits) ? _T("AvOverSMin_NWC_US.png") : _T("AvOverSMin_NWC_SI.png"));
               break;

            case pgsTypes::AllLightweight:
               if (bHasAggSplittingStrength)
               {
                  strImage = (IS_US_UNITS(pDisplayUnits) ? _T("AvOverSMin_LWC_US.png") : _T("AvOverSMin_LWC_SI.png"));
               }
               else
               {
                  strImage = (IS_US_UNITS(pDisplayUnits) ? _T("AvOverSMin_ALWC_US.png") : _T("AvOverSMin_ALWC_SI.png"));
               }
               break;

            case pgsTypes::SandLightweight:
               if (bHasAggSplittingStrength)
               {
                  strImage = (IS_US_UNITS(pDisplayUnits) ? _T("AvOverSMin_LWC_US.png") : _T("AvOverSMin_LWC_SI.png"));
               }
               else
               {
                  strImage = (IS_US_UNITS(pDisplayUnits) ? _T("AvOverSMin_SLWC_US.png") : _T("AvOverSMin_SLWC_SI.png"));
               }
               break;

            case pgsTypes::PCI_UHPC:
            case pgsTypes::UHPC:
            default:
               ATLASSERT(false);
            }

            *pParagraph << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + strImage) << rptNewLine;

            IndexType nArtifacts = pStirrupArtifact->GetStirrupCheckAtPoisArtifactCount(intervalIdx, pgsTypes::StrengthI);
            const pgsStirrupCheckAtPoisArtifact* psArtifact = pStirrupArtifact->GetStirrupCheckAtPoisArtifact(intervalIdx, pgsTypes::StrengthI, nArtifacts - 1);
            const pgsStirrupDetailArtifact* pArtifact = psArtifact->GetStirrupDetailArtifact();

            *pParagraph << RPT_FC << _T(" = ") << stress.SetValue(pArtifact->GetFc()) << _T(" ") << stress.GetUnitTag() << rptNewLine;
            *pParagraph << RPT_FY << _T(" = ") << stress.SetValue(pArtifact->GetFy()) << _T(" ") << stress.GetUnitTag() << rptNewLine;

            if (concType != pgsTypes::Normal && bHasAggSplittingStrength)
            {
               *pParagraph << RPT_STRESS(_T("ct")) << _T(" = ") << stress.SetValue(pMaterial->GetClosureJointConcreteAggSplittingStrength(closureKey)) << _T(" ") << stress.GetUnitTag() << rptNewLine;
            }
         } // end if non-uhpc closure joint
      } // end if closure joint
   } // next segment

   pParagraph = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pParagraph;
   *pParagraph << _T("Details for Minimum Transverse Reinforcement Check - ") << WBFL::LRFD::LrfdCw8th(_T("5.8.2.5-1"),_T("5.7.2.5-1"));

   pParagraph = new rptParagraph;
   *pChapter << pParagraph;

   rptRcTable* table = rptStyleManager::CreateDefaultTable(3);

   *pParagraph << table << rptNewLine;
  
   (*table)(0,0)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table)(0,1)  << COLHDR(_T("b")<<Sub(_T("v")),  rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,2)  << COLHDR(_T("A") << Sub(_T("v"))<<_T("/S")<<Sub(_T("min")) , rptAreaPerLengthUnitTag, pDisplayUnits->GetAvOverSUnit() );

   // Fill up the table

   RowIndexType row = table->GetNumberOfHeaderRows();
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      const pgsSegmentArtifact* pSegmentArtifact = pGirderArtifact->GetSegmentArtifact(segIdx);
      const pgsStirrupCheckArtifact* pstirrup_artifact = pSegmentArtifact->GetStirrupCheckArtifact();
      ATLASSERT(pstirrup_artifact);

      IndexType nArtifacts = pstirrup_artifact->GetStirrupCheckAtPoisArtifactCount( intervalIdx,pgsTypes::StrengthI );
      for ( IndexType idx = 0; idx < nArtifacts; idx++ )
      {
         // it is ok to use a hard coded StrengthI limit state here because
         // we are only after Bv and Avs Min which are not dependent on loading
         const pgsStirrupCheckAtPoisArtifact* psArtifact = pstirrup_artifact->GetStirrupCheckAtPoisArtifact( intervalIdx,pgsTypes::StrengthI,idx );

         const pgsPointOfInterest& poi = psArtifact->GetPointOfInterest();

         const pgsStirrupDetailArtifact* pArtifact = psArtifact->GetStirrupDetailArtifact();

         (*table)(row,0) << location.SetValue( POI_ERECTED_SEGMENT, poi );
         (*table)(row,1) << dim.SetValue(pArtifact->GetBv());

         if (pArtifact->IsApplicable())
         {
            (*table)(row,2) << avs.SetValue(pArtifact->GetAvsMin());
         }
         else
         {
            (*table)(row,2) << RPT_NA;
         }

         row++;
      } // next artifact
   } // next segment
}

void build_max_spacing_paragraph(IBroker* pBroker,rptChapter* pChapter,const CGirderKey& girderKey,
                                    IntervalIndexType intervalIdx, pgsTypes::LimitState ls, bool bIncludeSpanAndGirderForPois,
                                    IEAFDisplayUnits* pDisplayUnits)
{
   // Spacing check 5.7.2.6 (pre2017: 5.8.2.7)
   rptParagraph* pParagraph;
   pParagraph = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pParagraph;

   if ( ls == pgsTypes::StrengthI )
   {
      *pParagraph << _T("Strength I");
   }
   else
   {
      *pParagraph << _T("Strength II");
   }

   *pParagraph <<_T(" - Details for Maximum Transverse Reinforcement Spacing Check - ") << WBFL::LRFD::LrfdCw8th(_T("5.8.2.7"),_T("5.7.2.6"))<<rptNewLine;

   pParagraph = new rptParagraph;
   *pChapter << pParagraph;

   INIT_UV_PROTOTYPE( rptPointOfInterest,    location, pDisplayUnits->GetSpanLengthUnit(),   false );
   INIT_UV_PROTOTYPE( rptStressUnitValue,    stress,   pDisplayUnits->GetStressUnit(),          false );
   INIT_UV_PROTOTYPE( rptAreaUnitValue,      area,     pDisplayUnits->GetAreaUnit(),            false );
   INIT_UV_PROTOTYPE( rptAreaPerLengthValue, avs,      pDisplayUnits->GetAvOverSUnit(),  false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,    dim,     pDisplayUnits->GetComponentDimUnit(),  false );
   INIT_UV_PROTOTYPE( rptForceSectionValue,  shear,    pDisplayUnits->GetShearUnit(),        false );

   location.IncludeSpanAndGirder(bIncludeSpanAndGirderForPois);

   GET_IFACE2(pBroker,ILibrary,pLib);
   GET_IFACE2(pBroker,ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   const auto& shear_capacity_criteria = pSpecEntry->GetShearCapacityCriteria();

   bool bAfter1999 = (WBFL::LRFD::BDSManager::Edition::SecondEditionWith2000Interims <= WBFL::LRFD::BDSManager::GetEdition() ? true : false );

   Float64 k1 = shear_capacity_criteria.StirrupSpacingCoefficient[0];
   Float64 k2 = shear_capacity_criteria.StirrupSpacingCoefficient[1];
   Float64 s1 = shear_capacity_criteria.MaxStirrupSpacing[0];
   Float64 s2 = shear_capacity_criteria.MaxStirrupSpacing[1];

   rptRcTable* petable = rptStyleManager::CreateDefaultTable(2);
   if (bAfter1999)
   {
      (*petable)(1, 0) << RPT_vu << _T(" < 0.125") << RPT_FC;
   }
   else
   {
      (*petable)(1, 0) << RPT_Vu << _T(" < 0.1") << RPT_FC << _T("b") << Sub(_T("v")) << _T("d") << Sub(_T("v"));
   }

   (*petable)(1,1) << Sub2(_T("S"),_T("max"))<<_T("= min(") << k1 << Sub2(_T("d"),_T("v"))<<_T(", ")<<dim.SetValue(s1)<<dim.GetUnitTag()<<_T(")");
   
   if (bAfter1999)
   {
      (*petable)(2, 0) << RPT_vu << _T(" ") << symbol(GTE) << _T(" 0.125") << RPT_FC;
   }
   else
   {
      (*petable)(2, 0) << RPT_vu << _T(" ") << symbol(GTE) << _T(" 0.1") << RPT_FC << Sub2(_T("b"), _T("v")) << Sub2(_T("d"), _T("v"));
   }

   (*petable)(2,1) << Sub2(_T("S"),_T("max"))<<_T("= min(") << k2 << Sub2(_T("d"),_T("v"))<<_T(", ")<<dim.SetValue(s2)<<dim.GetUnitTag()<<_T(")");

   rptRcTable* table = rptStyleManager::CreateDefaultTable(6);
   (*table)(0,0)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   if ( bAfter1999 )
   {
      (*table)(0,1)  << COLHDR(RPT_vu, rptStressUnitTag,  pDisplayUnits->GetStressUnit() );
      (*table)(0,2)  << COLHDR(_T("0.125") << RPT_FC,  rptStressUnitTag,  pDisplayUnits->GetStressUnit() );
   }
   else
   {
      (*table)(0,1)  << COLHDR(Sub2(_T("V"),_T("u")),       rptForceUnitTag,  pDisplayUnits->GetShearUnit() );
      (*table)(0,2)  << COLHDR(_T("0.1") << RPT_FC <<Sub2(_T("b"),_T("v"))<<Sub2(_T("d"),_T("v")),  rptForceUnitTag,  pDisplayUnits->GetShearUnit() );
   }

   (*table)(0,3)  << COLHDR(Sub2(_T("b"),_T("v")),   rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,4)  << COLHDR(Sub2(_T("d"),_T("v")),   rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,5)  << COLHDR(Sub2(_T("S"),_T("max")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );

   // Fill up the table
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IArtifact,pIArtifact);

   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);

   const pgsGirderArtifact* pGirderArtifact = pIArtifact->GetGirderArtifact(girderKey);

   Float64 end_size = pBridge->GetSegmentStartEndDistance(CSegmentKey(girderKey,0));

   RowIndexType row = table->GetNumberOfHeaderRows();

   GET_IFACE2(pBroker, IMaterials, pMaterial);
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      CSegmentKey segmentKey(girderKey,segIdx);
      pgsTypes::ConcreteType concType = pMaterial->GetSegmentConcreteType(segmentKey);
      if (concType == pgsTypes::PCI_UHPC)
      {
         *pParagraph << _T("PCI SDG E.7.2.2 - There is no requirement for transverse reinforcement in PCI-UHPC members.") << rptNewLine;
      }

      const pgsSegmentArtifact* pSegmentArtifact = pIArtifact->GetSegmentArtifact( segmentKey );
      const pgsStirrupCheckArtifact* pstirrup_artifact = pSegmentArtifact->GetStirrupCheckArtifact();
      ATLASSERT(pstirrup_artifact);

      IndexType nArtifacts = pstirrup_artifact->GetStirrupCheckAtPoisArtifactCount( intervalIdx,pgsTypes::StrengthI );
      for ( IndexType idx = 0; idx < nArtifacts; idx++ )
      {
         // it is ok to use a hard coded StrengthI limit state here because
         // we are only after Bv and Avs Min withc are not dependent on loading
         const pgsStirrupCheckAtPoisArtifact* psArtifact = pstirrup_artifact->GetStirrupCheckAtPoisArtifact( intervalIdx,pgsTypes::StrengthI,idx );

         const pgsPointOfInterest& poi = psArtifact->GetPointOfInterest();
         ATLASSERT(poi.GetSegmentKey() == segmentKey);

         const pgsStirrupDetailArtifact* pArtifact = psArtifact->GetStirrupDetailArtifact();

         (*table)(row,0) << location.SetValue( POI_ERECTED_SEGMENT, poi );
         if ( bAfter1999 )
         {
            (*table)(row,1) << stress.SetValue(pArtifact->Getvu());
            (*table)(row,2) << stress.SetValue(pArtifact->GetvuLimit());
         }
         else
         {
            (*table)(row,1) << shear.SetValue(pArtifact->GetVu());
            (*table)(row,2) << shear.SetValue(pArtifact->GetVuLimit());
         }
         (*table)(row,3) << dim.SetValue(pArtifact->GetBv());
         (*table)(row,4) << dim.SetValue(pArtifact->GetDv());
   
         if (pArtifact->IsApplicable())
         {
            (*table)(row,5) << dim.SetValue(pArtifact->GetSMax());
         }
         else
         {
            (*table)(row,5) << RPT_NA;
         }

         row++;
      } // next artifact
   } // next segment


   // we want a specific order of the tables so they are added here at the end
   *pParagraph << table << rptNewLine;
   *pParagraph << rptNewLine;
   *pParagraph << petable << rptNewLine;
}


void build_max_spacing_paragraph_uhpc(IBroker* pBroker, rptChapter* pChapter, const CGirderKey& girderKey,
   IntervalIndexType intervalIdx, pgsTypes::LimitState ls, bool bIncludeSpanAndGirderForPois,
   IEAFDisplayUnits* pDisplayUnits)
{
   // Spacing check 5.7.2.6 (pre2017: 5.8.2.7)
   rptParagraph* pParagraph;
   pParagraph = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pParagraph;

   if (ls == pgsTypes::StrengthI)
   {
      *pParagraph << _T("Strength I");
   }
   else
   {
      *pParagraph << _T("Strength II");
   }

   *pParagraph << _T(" - Details for Maximum Transverse Reinforcement Spacing Check - GS 1.7.2.6") << rptNewLine;

   pParagraph = new rptParagraph;
   *pChapter << pParagraph;

   INIT_UV_PROTOTYPE(rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false);
   INIT_UV_PROTOTYPE(rptAngleUnitValue, angle, pDisplayUnits->GetAngleUnit(), false);
   INIT_UV_PROTOTYPE(rptLengthUnitValue, dim, pDisplayUnits->GetComponentDimUnit(), false);
   INIT_UV_PROTOTYPE(rptForceSectionValue, shear, pDisplayUnits->GetShearUnit(), false);

   location.IncludeSpanAndGirder(bIncludeSpanAndGirderForPois);

   Float64 Smax = WBFL::Units::ConvertToSysUnits(24.0,WBFL::Units::Measure::Inch); // maximum spacing = 24.0 in GS 1.7.2.6
   dim.ShowUnitTag(true);
   *pParagraph << Sub2(_T("S"), _T("max")) << _T(" = 0.25") << Sub2(_T("d"), _T("v,UHPC")) << _T(" cot ") << symbol(theta) << _T(" ") << symbol(LTE) << _T(" ") << dim.SetValue(Smax) << rptNewLine;
   dim.ShowUnitTag(false);


   rptRcTable* table = rptStyleManager::CreateDefaultTable(4);
   *pParagraph << table << rptNewLine;
   *pParagraph << rptNewLine;

   ColumnIndexType col = 0;
   (*table)(0, col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table)(0, col++) << COLHDR(Sub2(_T("d"), _T("v,UHPC")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
   (*table)(0, col++) << COLHDR(symbol(theta), rptAngleUnitTag, pDisplayUnits->GetAngleUnit());
   (*table)(0, col++) << COLHDR(Sub2(_T("S"), _T("max")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());

   // Fill up the table
   GET_IFACE2(pBroker, IBridge, pBridge);
   GET_IFACE2(pBroker, IArtifact, pIArtifact);

   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);

   const pgsGirderArtifact* pGirderArtifact = pIArtifact->GetGirderArtifact(girderKey);

   Float64 end_size = pBridge->GetSegmentStartEndDistance(CSegmentKey(girderKey, 0));

   RowIndexType row = table->GetNumberOfHeaderRows();

   for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
   {
      CSegmentKey segmentKey(girderKey, segIdx);
      const pgsSegmentArtifact* pSegmentArtifact = pIArtifact->GetSegmentArtifact(segmentKey);
      const pgsStirrupCheckArtifact* pstirrup_artifact = pSegmentArtifact->GetStirrupCheckArtifact();
      ATLASSERT(pstirrup_artifact);

      IndexType nArtifacts = pstirrup_artifact->GetStirrupCheckAtPoisArtifactCount(intervalIdx, pgsTypes::StrengthI);
      for (IndexType idx = 0; idx < nArtifacts; idx++)
      {
         // it is ok to use a hard coded StrengthI limit state here because
         // we are only after Bv and Avs Min which are not dependent on loading
         const pgsStirrupCheckAtPoisArtifact* psArtifact = pstirrup_artifact->GetStirrupCheckAtPoisArtifact(intervalIdx, pgsTypes::StrengthI, idx);

         const pgsPointOfInterest& poi = psArtifact->GetPointOfInterest();
         ATLASSERT(poi.GetSegmentKey() == segmentKey);

         const pgsStirrupDetailArtifact* pArtifact = psArtifact->GetStirrupDetailArtifact();


         col = 0;

         (*table)(row, col++) << location.SetValue(POI_ERECTED_SEGMENT, poi);
         (*table)(row, col++) << dim.SetValue(pArtifact->GetDv());
         (*table)(row, col++) << angle.SetValue(pArtifact->GetTheta());
         (*table)(row, col++) << dim.SetValue(pArtifact->GetSMax());

         row++;
      } // next artifact
   } // next segment
}
