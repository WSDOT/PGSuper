///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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
#include <Reporting\InterfaceShearDetails.h>
#include <Reporting\ReportNotes.h>

#include <PgsExt\ReportPointOfInterest.h>
#include <PgsExt\GirderArtifact.h>
#include <PgsExt\RatingArtifact.h>

#include <IFace\DocumentType.h>
#include <IFace\Bridge.h>
#include <IFace\Artifact.h>
#include <IFace\Project.h>
#include <IFace\Intervals.h>
#include <IFace\AnalysisResults.h>

#include <PsgLib\SpecLibraryEntry.h>

#include <Lrfd\ConcreteUtil.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CInterfaceShearDetails
****************************************************************************/

CInterfaceShearDetails::CInterfaceShearDetails(IEAFDisplayUnits* pDisplayUnits)
{
   DEFINE_UV_PROTOTYPE(location, pDisplayUnits->GetSpanLengthUnit(), false);
   DEFINE_UV_PROTOTYPE(shear, pDisplayUnits->GetGeneralForceUnit(), false);
   DEFINE_UV_PROTOTYPE(shear_per_length, pDisplayUnits->GetForcePerLengthUnit(), false);
   DEFINE_UV_PROTOTYPE(fy, pDisplayUnits->GetStressUnit(), false);
   DEFINE_UV_PROTOTYPE(stress, pDisplayUnits->GetStressUnit(), false);
   DEFINE_UV_PROTOTYPE(stress_with_tag, pDisplayUnits->GetStressUnit(), true);
   DEFINE_UV_PROTOTYPE(AvS, pDisplayUnits->GetAvOverSUnit(), false);
   DEFINE_UV_PROTOTYPE(dim, pDisplayUnits->GetComponentDimUnit(), false);
   DEFINE_UV_PROTOTYPE(area, pDisplayUnits->GetAreaUnit(), false);
   DEFINE_UV_PROTOTYPE(l3, pDisplayUnits->GetSectModulusUnit(), false);
   DEFINE_UV_PROTOTYPE(l4, pDisplayUnits->GetMomentOfInertiaUnit(), false);
}

CInterfaceShearDetails::~CInterfaceShearDetails()
{
}

void CInterfaceShearDetails::Build(IBroker* pBroker, rptChapter* pChapter,
   const CGirderKey& girderKey,
   IEAFDisplayUnits* pDisplayUnits,
   IntervalIndexType intervalIdx,
   pgsTypes::LimitState ls)
{
   GET_IFACE2(pBroker, ILibrary, pLib);
   GET_IFACE2(pBroker, ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());
   m_bIsSpec2007orOlder = lrfdVersionMgr::FourthEdition2007 <= pSpecEntry->GetSpecificationType();
   m_ShearFlowMethod = pSpecEntry->GetShearFlowMethod();

   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pPara;

   (*pPara) << _T("Details for Horizontal Interface Shear Capacity (") << GetLimitStateString(ls) << _T(") [") << LrfdCw8th(_T("5.8.4.1"), _T("5.7.4.1")) << _T("]") << rptNewLine;

   GET_IFACE2(pBroker, IMaterials, pMaterials);
   if (pMaterials->GetSegmentConcreteType(CSegmentKey(girderKey, 0)) == pgsTypes::PCI_UHPC)
   {
      (*pPara) << _T("PCI UHPC SDG E.7.4.1") << rptNewLine;
   }

   GET_IFACE2(pBroker, IDocumentType, pDocType);
   location.IncludeSpanAndGirder(pDocType->IsPGSpliceDocument() || girderKey.groupIndex == ALL_GROUPS);

   if (IsDesignLimitState(ls))
   {
      BuildDesign(pBroker, pChapter, girderKey, pDisplayUnits, intervalIdx, ls);
   }
   else
   {
      BuildRating(pBroker, pChapter, girderKey, pDisplayUnits, intervalIdx, ls);
   }
}

void CInterfaceShearDetails::BuildDesign( IBroker* pBroker, rptChapter* pChapter, const CGirderKey& girderKey, IEAFDisplayUnits* pDisplayUnits, IntervalIndexType intervalIdx, pgsTypes::LimitState ls)
{
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IArtifact,pIArtifact);

   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);

   const pgsGirderArtifact* pGirderArtifact = pIArtifact->GetGirderArtifact(girderKey);

   // create vector of horiz shear artifacts that will be used to report common information
   // this is typically the first applicable artifact in each segment
   std::vector<std::pair<SegmentIndexType,const pgsHorizontalShearArtifact*>> vSegmentHorizShearArtifacts;
   std::vector<std::pair<SegmentIndexType, const pgsHorizontalShearArtifact*>> vClosureJointHorizShearArtifacts;
   for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
   {
      const pgsStirrupCheckArtifact* pStirrupCheckArtifact = pGirderArtifact->GetSegmentArtifact(segIdx)->GetStirrupCheckArtifact();
      IndexType nArtifacts = pStirrupCheckArtifact->GetStirrupCheckAtPoisArtifactCount(intervalIdx, ls);
      for (IndexType idx = 0; idx < nArtifacts; idx++)
      {
         const pgsHorizontalShearArtifact* pHorizShearArtifact = pStirrupCheckArtifact->GetStirrupCheckAtPoisArtifact(intervalIdx, ls, idx)->GetHorizontalShearArtifact();
         if (pHorizShearArtifact->IsApplicable())
         {
            vSegmentHorizShearArtifacts.emplace_back(segIdx, pHorizShearArtifact);
            break;
         }
      }

      if (segIdx < nSegments - 1)
      {
         const pgsStirrupCheckAtPoisArtifact* pStirrupAtPoiArtifact = pStirrupCheckArtifact->GetStirrupCheckAtPoisArtifact(intervalIdx, ls, nArtifacts - 1);
         ATLASSERT(pStirrupAtPoiArtifact->GetPointOfInterest().HasAttribute(POI_CLOSURE));
         const pgsHorizontalShearArtifact* pHorizShearArtifact = pStirrupAtPoiArtifact->GetHorizontalShearArtifact();
         ATLASSERT(pHorizShearArtifact->IsApplicable());
         vClosureJointHorizShearArtifacts.emplace_back(segIdx, pHorizShearArtifact);
      }
   }

   // Initial Capacity Table
   rptRcTable* vui_table = CreateVuiTable(pBroker,pChapter,pDisplayUnits); // creates the table and adds it to the chapter. also addes table footnotes

   rptRcTable* avf_table = CreateAvfTable(pDisplayUnits);

   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;
   *pPara << avf_table;

   rptRcTable* vni_table = CreateVniTable(pBroker, pChapter, pDisplayUnits, vSegmentHorizShearArtifacts, vClosureJointHorizShearArtifacts);

   // Fill up the tables
   RowIndexType vui_row = vui_table->GetNumberOfHeaderRows();
   RowIndexType avf_row = avf_table->GetNumberOfHeaderRows();
   RowIndexType vni_row = vni_table->GetNumberOfHeaderRows();

   // these are needed later
   bool is_roughened (false);
   bool do_all_stirrups_engage_deck(false);

   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      CSegmentKey segmentKey(girderKey,segIdx);
      const pgsSegmentArtifact* pSegmentArtifact = pIArtifact->GetSegmentArtifact(segmentKey);
      const pgsStirrupCheckArtifact* pstirrup_artifact= pSegmentArtifact->GetStirrupCheckArtifact();
      ATLASSERT(pstirrup_artifact);

      CollectionIndexType nArtifacts = pstirrup_artifact->GetStirrupCheckAtPoisArtifactCount(intervalIdx,ls);
      for ( CollectionIndexType idx = 0; idx < nArtifacts; idx++ )
      {
         const pgsStirrupCheckAtPoisArtifact* psArtifact = pstirrup_artifact->GetStirrupCheckAtPoisArtifact( intervalIdx, ls, idx );

         const pgsPointOfInterest& poi = psArtifact->GetPointOfInterest();
         ATLASSERT(poi.GetSegmentKey() == segmentKey);

         const pgsHorizontalShearArtifact* pArtifact = psArtifact->GetHorizontalShearArtifact();

         // Don't report values in vui and capacity table for poi's in end zone outside of CSS
         bool is_app = pArtifact->IsApplicable();

         is_roughened |= pArtifact->IsTopFlangeRoughened();
         do_all_stirrups_engage_deck |= pArtifact->DoAllPrimaryStirrupsEngageDeck();


         if (is_app)
         {
            // vui table
            FillVuiTable(vui_table, vui_row, poi, pArtifact);
            vui_row++;
         }

         // av/s table
         FillAvfTable(avf_table, avf_row, poi, pArtifact);
         avf_row++;

         if (is_app)
         {
            FillVniTable(vni_table, vni_row, poi, pArtifact);
            vni_row++;
         }
      } // next artifact
   } // next segment

   // Next Create MinAvfTable
   rptRcTable* min_avf_table = CreateMinAvfTable(pChapter, pBridge, pDisplayUnits,  is_roughened, do_all_stirrups_engage_deck);

   Float64 llss = lrfdConcreteUtil::LowerLimitOfShearStrength(is_roughened, do_all_stirrups_engage_deck);

   // Fill up the table
   RowIndexType row = 1;
   for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
   {
      CSegmentKey segmentKey(girderKey, segIdx);
      const pgsSegmentArtifact* pSegmentArtifact = pIArtifact->GetSegmentArtifact(segmentKey);
      const pgsStirrupCheckArtifact* pstirrup_artifact = pSegmentArtifact->GetStirrupCheckArtifact();
      ATLASSERT(pstirrup_artifact);

      CollectionIndexType nArtifacts = pstirrup_artifact->GetStirrupCheckAtPoisArtifactCount(intervalIdx, ls);
      for (CollectionIndexType idx = 0; idx < nArtifacts; idx++)
      {
         const pgsStirrupCheckAtPoisArtifact* psArtifact = pstirrup_artifact->GetStirrupCheckAtPoisArtifact(intervalIdx, ls, idx);
         if (psArtifact == nullptr)
         {
            continue;
         }

         const pgsPointOfInterest& poi = psArtifact->GetPointOfInterest();
         ATLASSERT(poi.GetSegmentKey() == segmentKey);

         const pgsHorizontalShearArtifact* pArtifact = psArtifact->GetHorizontalShearArtifact();

         FillMinAvfTable(min_avf_table, row, poi, pArtifact, llss, pDisplayUnits);

         row++;
      } // next artifact
   } // next segment
}

void CInterfaceShearDetails::BuildRating(IBroker* pBroker, rptChapter* pChapter, const CGirderKey& girderKey, IEAFDisplayUnits* pDisplayUnits, IntervalIndexType intervalIdx, pgsTypes::LimitState ls)
{
   GET_IFACE2(pBroker, IBridge, pBridge);
   GET_IFACE2(pBroker, IArtifact, pIArtifact);

   pgsTypes::LoadRatingType ratingType = RatingTypeFromLimitState(ls);
   const pgsRatingArtifact* pArtifact = pIArtifact->GetRatingArtifact(girderKey, ratingType, INVALID_INDEX);

  // Initial Capacity Table
   rptRcTable* vui_table = CreateVuiTable(pBroker, pChapter, pDisplayUnits); // creates the table and adds it to the chapter. also addes table footnotes

   rptRcTable* avf_table = CreateAvfTable(pDisplayUnits);

   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;
   *pPara << avf_table;

   const pgsRatingArtifact::ShearRatings& ratings = pArtifact->GetShearRatings();
   std::vector<std::pair<SegmentIndexType, const pgsHorizontalShearArtifact*>> vSegmentArtifacts;
   std::vector<std::pair<SegmentIndexType, const pgsHorizontalShearArtifact*>> vClosureArtifacts;
   CSegmentKey last_segment_key;
   for (const auto& value : ratings)
   {
      const pgsPointOfInterest& poi(value.first);
      const pgsShearRatingArtifact& shear_rating_artifact(value.second);
      if (poi.HasAttribute(POI_CLOSURE))
      {
         vClosureArtifacts.emplace_back(poi.GetSegmentKey().segmentIndex, &shear_rating_artifact.GetHorizontalInterfaceShearArtifact());
      }
      else if (!poi.GetSegmentKey().IsEqual(last_segment_key))
      {
         last_segment_key = poi.GetSegmentKey();
         vSegmentArtifacts.emplace_back(last_segment_key.segmentIndex, &shear_rating_artifact.GetHorizontalInterfaceShearArtifact());
      }
   }

   rptRcTable* vni_table = CreateVniTable(pBroker, pChapter, pDisplayUnits, vSegmentArtifacts, vClosureArtifacts);

   // Fill up the tables
   RowIndexType vui_row = vui_table->GetNumberOfHeaderRows();
   RowIndexType avf_row = avf_table->GetNumberOfHeaderRows();
   RowIndexType vni_row = vni_table->GetNumberOfHeaderRows();

   // these are needed later
   bool is_roughened(false);
   bool do_all_stirrups_engage_deck(false);

   for (const auto& shear_rating : ratings)
   {
      const pgsPointOfInterest& poi(shear_rating.first);
      const auto& shear_rating_artifact(shear_rating.second);
      const auto& horiz_shear_artifact = shear_rating_artifact.GetHorizontalInterfaceShearArtifact();

      // Don't report values in vui and capacity table for poi's in end zone outside of CSS
      bool is_app = horiz_shear_artifact.IsApplicable();

      is_roughened |= horiz_shear_artifact.IsTopFlangeRoughened();
      do_all_stirrups_engage_deck |= horiz_shear_artifact.DoAllPrimaryStirrupsEngageDeck();

      if (is_app)
      {
         // vui table
         FillVuiTable(vui_table, vui_row, poi, &horiz_shear_artifact);
         vui_row++;
      }

      // av/s table
      FillAvfTable(avf_table, avf_row, poi, &horiz_shear_artifact);
      avf_row++;

      if (is_app)
      {
         FillVniTable(vni_table, vni_row, poi, &horiz_shear_artifact);
         vni_row++;
      }
   } // next rating

   // Next Create MinAvfTable
   rptRcTable* min_avf_table = CreateMinAvfTable(pChapter, pBridge, pDisplayUnits, is_roughened, do_all_stirrups_engage_deck);

   Float64 llss = lrfdConcreteUtil::LowerLimitOfShearStrength(is_roughened, do_all_stirrups_engage_deck);

   // Fill up the table
   RowIndexType min_avf_row = min_avf_table->GetNumberOfHeaderRows();
   for (const auto& shear_rating : ratings)
   {
      const pgsPointOfInterest& poi(shear_rating.first);
      const auto& shear_rating_artifact(shear_rating.second);
      const auto& horiz_shear_artifact = shear_rating_artifact.GetHorizontalInterfaceShearArtifact();

      FillMinAvfTable(min_avf_table, min_avf_row, poi, &horiz_shear_artifact, llss, pDisplayUnits);

      min_avf_row++;
   } // next rating
}

rptRcTable* CInterfaceShearDetails::CreateVuiTable(IBroker* pBroker,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits)
{
   ColumnIndexType nCol = m_ShearFlowMethod == pgsTypes::sfmLRFD ? 6 : 7;

   rptRcTable* vui_table = rptStyleManager::CreateDefaultTable(nCol);

   rptParagraph* pPara = new rptParagraph();
   *pChapter << pPara;
   *pPara << vui_table << rptNewLine;

   ColumnIndexType col = 0;

   (*vui_table)(0, col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

   if (m_ShearFlowMethod == pgsTypes::sfmLRFD)
   {
      (*vui_table)(0, col++) << COLHDR(Sub2(_T("d"), _T("vi")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
   }
   else
   {
      (*vui_table)(0, col++) << COLHDR(_T("I"), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
      (*vui_table)(0, col++) << COLHDR(Sub2(_T("Q"), _T("slab")), rptLength3UnitTag, pDisplayUnits->GetSectModulusUnit());
   }

   (*vui_table)(0, col++) << COLHDR(Sub2(_T("V"), _T("u")), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());

   if (m_ShearFlowMethod == pgsTypes::sfmLRFD)
   {
      (*vui_table)(0, col++) << COLHDR(Sub2(_T("v"), _T("ui")) << _T(" = ") << Sub2(_T("V"), _T("u")) << _T("/") << Sub2(_T("d"), _T("vi")), rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit());
   }
   else
   {
      (*vui_table)(0, col++) << COLHDR(Sub2(_T("v"), _T("ui")) << _T(" = ") << Sub2(_T("V"), _T("u")) << _T("Q/I"), rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit());
   }

   (*vui_table)(0, col++) << COLHDR(Sub2(_T("b"), _T("vi")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
   (*vui_table)(0, col++) << COLHDR(Sub2(symbol(nu), _T("ui")), rptStressUnitTag, pDisplayUnits->GetStressUnit());

   if (m_ShearFlowMethod == pgsTypes::sfmLRFD)
   {
      pPara = new rptParagraph(rptStyleManager::GetFootnoteStyle());
      *pChapter << pPara;
      *pPara << Sub2(_T("d"), _T("vi")) << _T(" = ") << Sub2(_T("Y"), _T("t girder")) << _T(" + Strand Eccentricity + ") << Sub2(_T("t"), _T("slab")) << _T("/2") << rptNewLine;

      pPara = new rptParagraph;
      *pChapter << pPara;
   }

   return vui_table;
}

void CInterfaceShearDetails::FillVuiTable(rptRcTable* pTable,RowIndexType row,const pgsPointOfInterest& poi,const pgsHorizontalShearArtifact* pArtifact)
{
   ColumnIndexType col = 0;
   Float64 Vui = pArtifact->GetDemand();
   (*pTable)(row, col++) << location.SetValue(POI_SPAN, poi);

   if (m_ShearFlowMethod == pgsTypes::sfmLRFD)
   {
      (*pTable)(row, col++) << dim.SetValue(pArtifact->GetDv());
   }
   else
   {
      (*pTable)(row, col++) << l4.SetValue(pArtifact->GetI());
      (*pTable)(row, col++) << l3.SetValue(pArtifact->GetQ());
   }

   (*pTable)(row, col++) << shear.SetValue(pArtifact->GetVu());
   (*pTable)(row, col++) << shear_per_length.SetValue(Vui);
   (*pTable)(row, col++) << dim.SetValue(pArtifact->GetBv());
   (*pTable)(row, col++) << stress.SetValue(Vui / pArtifact->GetBv());
}

rptRcTable* CInterfaceShearDetails::CreateAvfTable(IEAFDisplayUnits* pDisplayUnits)
{
   rptRcTable* avf_table = rptStyleManager::CreateDefaultTable(6);

   (*avf_table)(0, 0) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*avf_table)(0, 1) << COLHDR(_T("A") << Sub(_T("vf")) << rptNewLine << _T("Primary"), rptAreaUnitTag, pDisplayUnits->GetAreaUnit());
   (*avf_table)(0, 2) << COLHDR(_T("S") << rptNewLine << _T("Primary"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
   (*avf_table)(0, 3) << COLHDR(_T("A") << Sub(_T("vf")) << rptNewLine << _T("Additional"), rptAreaUnitTag, pDisplayUnits->GetAreaUnit());
   (*avf_table)(0, 4) << COLHDR(_T("S") << rptNewLine << _T("Additional"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
   (*avf_table)(0, 5) << COLHDR(_T("a") << Sub(_T("vf")) << rptNewLine << _T("Composite"), rptAreaPerLengthUnitTag, pDisplayUnits->GetAvOverSUnit());

   return avf_table;
}

void CInterfaceShearDetails::FillAvfTable(rptRcTable* pTable,RowIndexType row,const pgsPointOfInterest& poi,const pgsHorizontalShearArtifact* pArtifact)
{
   ColumnIndexType col = 0;
   (*pTable)(row, col++) << location.SetValue(POI_SPAN, poi);
   (*pTable)(row, col++) << area.SetValue(pArtifact->GetAvfGirder());

   Float64 sv = pArtifact->GetSGirder();
   if (0.0 < sv)
   {
      (*pTable)(row, col++) << dim.SetValue(sv);
   }
   else
   {
      (*pTable)(row, col++) << symbol(infinity);
   }

   (*pTable)(row, col++) << area.SetValue(pArtifact->GetAvfAdditional());

   sv = pArtifact->GetSAdditional();
   if (0.0 < sv)
   {
      (*pTable)(row, col++) << dim.SetValue(sv);
   }
   else
   {
      (*pTable)(row, col++) << symbol(infinity);
   }

   (*pTable)(row, col++) << AvS.SetValue(pArtifact->GetAvOverS());
}

rptRcTable* CInterfaceShearDetails::CreateVniTable(IBroker* pBroker,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits,const std::vector<std::pair<SegmentIndexType,const pgsHorizontalShearArtifact*>>& vSegmentArtifacts, std::vector<std::pair<SegmentIndexType, const pgsHorizontalShearArtifact*>>& vClosureArtifacts)
{
   Float64 fy_max = ::ConvertToSysUnits(60.0, unitMeasure::KSI); // LRFD 5.7.4.2 (pre2017: 2013 5.8.4.1)

   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   ATLASSERT(vSegmentArtifacts.size() - 1 == vClosureArtifacts.size());

   auto& segIter = vSegmentArtifacts.begin();
   auto& segEnd = vSegmentArtifacts.end();
   auto& cjIter = vClosureArtifacts.begin();
   auto& cjEnd = vClosureArtifacts.end();

   for (; segIter != segEnd; segIter++)
   {
      if (1 < vSegmentArtifacts.size())
      {
         *pPara << _T("Segment ") << LABEL_SEGMENT(segIter->first) << rptNewLine;
      }

      const auto* pArtifact = segIter->second;

      *pPara << _T("Coeff. of Friction, ") << symbol(mu) << _T(" = ") << pArtifact->GetFrictionFactor() << _T(", ")
         << _T("Cohesion Factor, c = ") << stress_with_tag.SetValue(pArtifact->GetCohesionFactor()) << _T(", ")
         << _T("Net Compression Force Load Factor, ") << Sub2(symbol(gamma), _T("DC")) << _T(" = ") << pArtifact->GetNormalCompressionForceLoadFactor() << _T(", ");

      if (m_bIsSpec2007orOlder)
      {
         *pPara << Sub2(_T("K"), _T("1")) << _T(" = ") << pArtifact->GetK1() << _T(", ")
            << Sub2(_T("K"), _T("2")) << _T(" = ") << stress_with_tag.SetValue(pArtifact->GetK2()) << _T(", ");
      }

      *pPara << symbol(phi) << _T(" = ") << pArtifact->GetPhi() << _T(", ")
         << RPT_FC << _T(" = ") << stress_with_tag.SetValue(pArtifact->GetFc()) << _T(", ")
         << RPT_FY << _T(" = ") << stress_with_tag.SetValue(pArtifact->GetFy());

      if (pArtifact->WasFyLimited())
      {
         *pPara << _T(", ") << RPT_FY << _T(" is limited to ") << stress_with_tag.SetValue(fy_max) << _T(" (LRFD ") << LrfdCw8th(_T("5.8.4.1"), _T("5.7.4.2")) << _T(")");
      }

      *pPara << rptNewLine;

      if (cjIter != cjEnd)
      {
         *pPara << _T("Closure Joint ") << LABEL_SEGMENT(cjIter->first) << rptNewLine;

         const auto* pArtifact = cjIter->second;

         *pPara << _T("Coeff. of Friction, ") << symbol(mu) << _T(" = ") << pArtifact->GetFrictionFactor() << _T(", ")
            << _T("Cohesion Factor, c = ") << stress_with_tag.SetValue(pArtifact->GetCohesionFactor()) << _T(", ")
            << _T("Net Compression Force Load Factor, ") << Sub2(symbol(gamma), _T("DC")) << _T(" = ") << pArtifact->GetNormalCompressionForceLoadFactor() << _T(", ");

         if (m_bIsSpec2007orOlder)
         {
            *pPara << Sub2(_T("K"), _T("1")) << _T(" = ") << pArtifact->GetK1() << _T(", ")
               << Sub2(_T("K"), _T("2")) << _T(" = ") << stress_with_tag.SetValue(pArtifact->GetK2()) << _T(", ");
         }

         *pPara << symbol(phi) << _T(" = ") << pArtifact->GetPhi() << _T(", ")
            << RPT_FC << _T(" = ") << stress_with_tag.SetValue(pArtifact->GetFc()) << _T(", ")
            << RPT_FY << _T(" = ") << stress_with_tag.SetValue(pArtifact->GetFy());

         if (pArtifact->WasFyLimited())
         {
            *pPara << _T(", ") << RPT_FY << _T(" is limited to ") << stress_with_tag.SetValue(fy_max) << _T(" (LRFD ") << LrfdCw8th(_T("5.8.4.1"), _T("5.7.4.2")) << _T(")");
         }

         cjIter++;
         *pPara << rptNewLine;
      }
   }

   if (m_bIsSpec2007orOlder)
   {
      *pPara << Sub2(_T("v"), _T("ni")) << _T(" = min( c") << Sub2(_T("a"), _T("cv")) << _T(" + ") << symbol(mu) << _T("[ ") << Sub2(_T("a"), _T("vf")) << RPT_FY << _T(" + ") << Sub2(symbol(gamma), _T("DC")) << Sub2(_T("p"), _T("c")) << _T("], ")
         << Sub2(_T("K"), _T("1")) << RPT_FC << Sub2(_T("a"), _T("cv")) << _T(", ") << Sub2(_T("K"), _T("2")) << Sub2(_T("a"), _T("cv")) << _T(" )") << rptNewLine;
   }
   else
   {
      *pPara << Sub2(_T("v"), _T("ni")) << _T(" = min( ca") << Sub(_T("cv")) << _T(" + ") << symbol(mu) << _T("[ a") << Sub(_T("vf ")) << RPT_FY << _T(" + ") << Sub2(symbol(gamma), _T("DC")) << Sub2(_T("p"), _T("c")) << _T("], ")
         << _T("0.2 ") << RPT_FC << _T("a") << Sub(_T("cv")) << _T(", ");

      if (IS_SI_UNITS(pDisplayUnits))
      {
         *pPara << _T(" 5.5 a") << Sub(_T("cv")) << _T(" )") << rptNewLine;
      }
      else
      {
         *pPara << _T(" 0.8 a") << Sub(_T("cv")) << _T(" )") << rptNewLine;
      }
   }

   rptRcTable* vni_table = rptStyleManager::CreateDefaultTable(8, _T(""));
   *pPara << vni_table;

   ColumnIndexType col = 0;

   (*vni_table)(0, col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*vni_table)(0, col++) << COLHDR(Sub2(_T("a"), _T("cv")), rptAreaPerLengthUnitTag, pDisplayUnits->GetAvOverSUnit());
   (*vni_table)(0, col++) << COLHDR(Sub2(_T("a"), _T("vf")), rptAreaPerLengthUnitTag, pDisplayUnits->GetAvOverSUnit());
   (*vni_table)(0, col++) << COLHDR(Sub2(_T("p"), _T("c")), rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit());
   (*vni_table)(0, col++) << COLHDR(_T("c a") << Sub(_T("cv")) << _T(" + ") << rptNewLine << symbol(mu) << _T("[a") << Sub(_T("vf ")) << _T("f") << Sub(_T("y")) << _T(" + ") << Sub2(symbol(gamma), _T("DC")) << Sub2(_T("p"), _T("c")) << _T("]"), rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit());

   if (m_bIsSpec2007orOlder)
   {
      (*vni_table)(0, col++) << COLHDR(Sub2(_T("K"), _T("1")) << RPT_FC << Sub2(_T("a"), _T("cv")), rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit());
      (*vni_table)(0, col++) << COLHDR(Sub2(_T("K"), _T("2")) << Sub2(_T("a"), _T("cv")), rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit());
   }
   else
   {
      (*vni_table)(0, col++) << COLHDR(_T("0.2 f'") << Sub(_T("c")) << _T("a") << Sub(_T("cv")), rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit());
      if (IS_SI_UNITS(pDisplayUnits))
      {
         (*vni_table)(0, col++) << COLHDR(_T("5.5 a") << Sub(_T("cv")), rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit());
      }
      else
      {
         (*vni_table)(0, col++) << COLHDR(_T("0.8 a") << Sub(_T("cv")), rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit());
      }
   }

   (*vni_table)(0, col++) << COLHDR(symbol(phi) << Sub2(_T("v"), _T("ni")), rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit());

   return vni_table;
}

void CInterfaceShearDetails::FillVniTable(rptRcTable* pTable,RowIndexType row,const pgsPointOfInterest& poi,const pgsHorizontalShearArtifact* pArtifact)
{
   ColumnIndexType col = 0;
   (*pTable)(row, col++) << location.SetValue(POI_SPAN, poi);
   (*pTable)(row, col++) << AvS.SetValue(pArtifact->GetAcv());
   (*pTable)(row, col++) << AvS.SetValue(pArtifact->GetAvOverS());
   (*pTable)(row, col++) << shear_per_length.SetValue(pArtifact->GetNormalCompressionForce());

   Float64 Vn1, Vn2, Vn3;
   pArtifact->GetVn(&Vn1, &Vn2, &Vn3);

   (*pTable)(row, col++) << shear_per_length.SetValue(Vn1);
   (*pTable)(row, col++) << shear_per_length.SetValue(Vn2);
   (*pTable)(row, col++) << shear_per_length.SetValue(Vn3);
   (*pTable)(row, col++) << shear_per_length.SetValue(pArtifact->GetCapacity());

}

rptRcTable* CInterfaceShearDetails::CreateMinAvfTable(rptChapter* pChapter,IBridge* pBridge,IEAFDisplayUnits* pDisplayUnits,bool bIsRoughened,bool doAllStirrupsEngageDeck)
{
   // Next, fill table for min Avf
   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pPara;
   *pPara << _T("Details for Minimum Horizontal Interface Shear Reinforcement [") << LrfdCw8th(_T("5.8.4.4"), _T("5.7.4.2")) << _T("]") << rptNewLine;

   rptParagraph* pParaEqn = new rptParagraph(); // for equation
   *pChapter << pParaEqn;
   pPara = new rptParagraph();
   *pChapter << pPara;

   Float64 llss = lrfdConcreteUtil::LowerLimitOfShearStrength(bIsRoughened, doAllStirrupsEngageDeck);
   if (0 < llss)
   {
      *pPara << _T("Girder/slab interfaces are intentionally roughened and all primary vertical shear reinforcement is extended across the interface. ")
         << _T("Hence, the minimum reinforcement requirement of ")
         << Sub2(_T("a"), _T("vf")) << _T(" may be waived if ");

      if (m_bIsSpec2007orOlder)
      {
         *pPara << Sub2(_T("v"), _T("ui"));
      }
      else
      {
         *pPara << Sub2(_T("v"), _T("ni")) << _T("/") << Sub2(_T("a"), _T("cv"));
      }

      *pPara << _T(" is less than ") << stress_with_tag.SetValue(llss) << rptNewLine;
   }

   rptRcTable* table = rptStyleManager::CreateDefaultTable(m_bIsSpec2007orOlder ? 7 : 5, _T(""));
   *pPara << table << rptNewLine;

   if (pBridge->GetDeckType() != pgsTypes::sdtNone && !doAllStirrupsEngageDeck)
   {
      *pPara << _T("Minimum reinforcement requirements cannot be waived. All of the primary vertical shear reinforcement does not extend across the girder/slab interface.") << rptNewLine;
   }

   //if ( span == ALL_SPANS )
   //{
   //   table->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   //   table->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   //}

   ColumnIndexType col = 0;
   (*table)(0, col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table)(0, col++) << COLHDR(_T("a") << Sub(_T("cv")), rptAreaPerLengthUnitTag, pDisplayUnits->GetAvOverSUnit());

   if (m_bIsSpec2007orOlder)
   {
      (*table)(0, col++) << COLHDR(Sub2(_T("v"), _T("ui")), rptStressUnitTag, pDisplayUnits->GetStressUnit());

      (*table)(0, col++) << COLHDR(Sub2(_T("a"), _T("vf min")) << rptNewLine << _T("(") << LrfdCw8th(_T("5.8.4.4-1"), _T("5.7.4.2-1")) << _T(")"), rptAreaPerLengthUnitTag, pDisplayUnits->GetAvOverSUnit());
      (*table)(0, col++) << COLHDR(Sub2(_T("a"), _T("vf min")) << rptNewLine << _T("(") << LrfdCw8th(_T("5.8.4.1-3"), _T("5.7.4.3-3")) << _T(")"), rptAreaPerLengthUnitTag, pDisplayUnits->GetAvOverSUnit());
      (*table)(0, col++) << COLHDR(Sub2(_T("a"), _T("vf min")), rptAreaPerLengthUnitTag, pDisplayUnits->GetAvOverSUnit());

      if (IS_SI_UNITS(pDisplayUnits))
      {
         *pParaEqn << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("AvfMin_SI.png")) << rptNewLine;
      }
      else
      {

         if (lrfdVersionMgr::GetVersion() < lrfdVersionMgr::EighthEdition2017)
         {
            *pParaEqn << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("AvfMin_US.png")) << rptNewLine;
         }
         else
         {
            *pParaEqn << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("AvfMin_US_2017.png")) << rptNewLine;
         }
      }
   }
   else
   {
      (*table)(0, col++) << COLHDR(Sub2(_T("v"), _T("ni")) << _T("/") << Sub2(_T("a"), _T("cv")), rptStressUnitTag, pDisplayUnits->GetStressUnit());

      if (IS_SI_UNITS(pDisplayUnits))
      {
         (*table)(0, col++) << COLHDR(Sub2(_T("a"), _T("vf min")) << _T(" = ") << Sub2(_T("0.35a"), _T("cv")) << _T("/") << RPT_FY, rptAreaPerLengthUnitTag, pDisplayUnits->GetAvOverSUnit());
      }
      else
      {
         (*table)(0, col++) << COLHDR(Sub2(_T("a"), _T("vf min")) << _T(" = ") << Sub2(_T("0.05a"), _T("cv")) << _T("/") << RPT_FY, rptAreaPerLengthUnitTag, pDisplayUnits->GetAvOverSUnit());
      }
   }

   (*table)(0, col) << _T("Min Reinforcement") << rptNewLine << _T("Requirement") << rptNewLine << _T("Waived?");

   return table;
}

void CInterfaceShearDetails::FillMinAvfTable(rptRcTable* pTable, RowIndexType row, const pgsPointOfInterest& poi, const pgsHorizontalShearArtifact* pArtifact,Float64 llss,IEAFDisplayUnits* pDisplayUnits)
{
   // Don't report values in vui and capacity table for poi's in end zone outside of CSS
   if (!pArtifact->IsApplicable())
   {
      return;
   }

   ColumnIndexType col = 0;
   (*pTable)(row, col++) << location.SetValue(POI_SPAN, poi);
   (*pTable)(row, col++) << AvS.SetValue(pArtifact->GetAcv());
   (*pTable)(row, col++) << stress.SetValue(pArtifact->GetVsAvg());

   if (m_bIsSpec2007orOlder)
   {
      (*pTable)(row, col++) << AvS.SetValue(pArtifact->GetAvOverSMin_5_7_4_2_1());
      (*pTable)(row, col++) << AvS.SetValue(pArtifact->GetAvOverSMin_5_7_4_1_3());
   }

   (*pTable)(row, col++) << AvS.SetValue(pArtifact->GetAvOverSMin());
   (*pTable)(row, col++) << (pArtifact->GetVsAvg() < llss ? _T("Yes") : _T("No"));
}
