///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2025  Washington State Department of Transportation
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
#include <Reporting\TimeStepParametersChapterBuilder.h>

#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\PointOfInterest.h>
#include <IFace\PrestressForce.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Intervals.h>
#include <IFace\Project.h>
#include <IFace\ReportOptions.h>

#include <WBFLGenericBridgeTools.h>

#include <PgsExt\TimelineEvent.h>
#include <PgsExt\CastDeckActivity.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// When defined, the computation details for initial strains are reported
// This is the summation part of Tadros 1977 Eqns 3 and 4.
//#define REPORT_INITIAL_STRAIN_DETAILS

//#define REPORT_PRODUCT_LOAD_DETAILS

/****************************************************************************
CLASS
   CTimeStepParametersChapterBuilder
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CTimeStepParametersChapterBuilder::CTimeStepParametersChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CTimeStepParametersChapterBuilder::GetName() const
{
   return TEXT("Time Step Parameters");
}

rptChapter* CTimeStepParametersChapterBuilder::Build(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,Uint16 level) const
{
   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);
   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   auto pGirderRptSpec = std::dynamic_pointer_cast<const CGirderReportSpecification>(pRptSpec);

   CComPtr<IBroker> pBroker;
   pGirderRptSpec->GetBroker(&pBroker);

   const CGirderKey& girderKey(pGirderRptSpec->GetGirderKey());

   GET_IFACE2(pBroker, ILossParameters, pLossParams);
   if ( pLossParams->GetLossMethod() != PrestressLossCriteria::LossMethodType::TIME_STEP )
   {
      *pPara << color(Red) << _T("Time Step analysis results not available.") << color(Black) << rptNewLine;
      return pChapter;
   }

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker, ISegmentTendonGeometry, pSegmentTendonGeometry);
   GET_IFACE2(pBroker, IGirderTendonGeometry, pGirderTendonGeometry);
   GET_IFACE2(pBroker,IPointOfInterest,pPoi);
   GET_IFACE2(pBroker,ILosses,pLosses);
   GET_IFACE2(pBroker,IIntervals,pIntervals);
   GET_IFACE2(pBroker,IMaterials,pMaterials);
   GET_IFACE2(pBroker,ILongRebarGeometry,pRebarGeom);

#if !defined LUMP_STRANDS
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);
#endif

   IntervalIndexType nIntervals = pIntervals->GetIntervalCount();

   DuctIndexType nGirderDucts = pGirderTendonGeometry->GetDuctCount(girderKey);

   INIT_UV_PROTOTYPE(rptPointOfInterest,    location,   pDisplayUnits->GetSpanLengthUnit(),      false);
   INIT_UV_PROTOTYPE(rptLengthUnitValue,    ecc,        pDisplayUnits->GetComponentDimUnit(),    false);
   INIT_UV_PROTOTYPE(rptStressUnitValue,    stress,     pDisplayUnits->GetStressUnit(),          false);
   INIT_UV_PROTOTYPE(rptLength2UnitValue,   area,       pDisplayUnits->GetAreaUnit(),            false);
   INIT_UV_PROTOTYPE(rptLength4UnitValue,   momI,       pDisplayUnits->GetMomentOfInertiaUnit(), false);
   INIT_UV_PROTOTYPE(rptStressUnitValue,    modE,       pDisplayUnits->GetModEUnit(),            false);
   INIT_UV_PROTOTYPE(rptForceUnitValue,     force,      pDisplayUnits->GetGeneralForceUnit(),    false);
   INIT_UV_PROTOTYPE(rptMomentUnitValue,    moment,     pDisplayUnits->GetMomentUnit(),          false);
   INIT_UV_PROTOTYPE(rptPerLengthUnitValue, curvature,  pDisplayUnits->GetCurvatureUnit(),       false);
   INIT_UV_PROTOTYPE(rptLengthUnitValue,    length,     pDisplayUnits->GetSpanLengthUnit(),      false);
   INIT_UV_PROTOTYPE(rptLengthUnitValue,    deflection, pDisplayUnits->GetDeflectionUnit(),      false);

   GET_IFACE2(pBroker,IReportOptions,pReportOptions);
   location.IncludeSpanAndGirder(pReportOptions->IncludeSpanAndGirder4Pois(girderKey));

   ///////////////////////////////////////////////////////////////////////////////////
   // Time Step Parameters that are independent of POI
   ///////////////////////////////////////////////////////////////////////////////////

   // Report Concrete Parameters
   GET_IFACE2(pBroker,IBridge,pBridge);
   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      CSegmentKey thisSegmentKey(girderKey.groupIndex,girderKey.girderIndex,segIdx);

      pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
      *pChapter << pPara;
      (*pPara) << _T("Segment ") << LABEL_SEGMENT(segIdx) << rptNewLine;

      pPara = new rptParagraph;
      *pChapter << pPara;

      ColumnIndexType col = 0;
      rptRcTable* pTable = rptStyleManager::CreateDefaultTable(9,_T("Concrete Parameters"));
      (*pPara) << pTable << rptNewLine;

      (*pTable)(0,col++) << _T("Interval");
      (*pTable)(0,col++) << COLHDR(_T("Age"),rptTimeUnitTag,pDisplayUnits->GetWholeDaysUnit());
      (*pTable)(0,col++) << COLHDR(_T("f'c"),rptStressUnitTag,pDisplayUnits->GetStressUnit());
      (*pTable)(0,col++) << COLHDR(_T("Ec"),rptStressUnitTag,pDisplayUnits->GetStressUnit());
      (*pTable)(0,col++) << symbol(psi);
      (*pTable)(0,col++) << symbol(chi);
      (*pTable)(0,col++) << COLHDR(_T("E'c = Ec/(") << _T("1+") << symbol(chi) << symbol(psi) << _T(")"),rptStressUnitTag,pDisplayUnits->GetStressUnit());
      (*pTable)(0,col++) << symbol(DELTA) << Sub2(symbol(epsilon),_T("sh")) << _T("x10") << Super(_T("6"));
      (*pTable)(0,col++) << Sub2(symbol(epsilon),_T("sh")) << _T("x10") << Super(_T("6"));


      RowIndexType row = pTable->GetNumberOfHeaderRows();
      for ( IntervalIndexType intervalIdx = 0; intervalIdx < nIntervals; intervalIdx++, row++ )
      {
         col = 0;
         (*pTable)(row,col++) << LABEL_INTERVAL(intervalIdx);
         (*pTable)(row,col++) << pMaterials->GetSegmentConcreteAge(thisSegmentKey,intervalIdx,pgsTypes::Middle);
         (*pTable)(row,col++) << stress.SetValue(pMaterials->GetSegmentFc(thisSegmentKey,intervalIdx));
         (*pTable)(row,col++) << modE.SetValue(pMaterials->GetSegmentEc(thisSegmentKey,intervalIdx));
         (*pTable)(row,col++) << pMaterials->GetSegmentCreepCoefficient(thisSegmentKey,intervalIdx,pgsTypes::Middle,intervalIdx,pgsTypes::End);
         (*pTable)(row,col++) << pMaterials->GetSegmentAgingCoefficient(thisSegmentKey,intervalIdx);
         (*pTable)(row,col++) << modE.SetValue(pMaterials->GetSegmentAgeAdjustedEc(thisSegmentKey,intervalIdx));
         (*pTable)(row,col++) << 1E6*pMaterials->GetIncrementalSegmentFreeShrinkageStrain(thisSegmentKey,intervalIdx);
         (*pTable)(row,col++) << 1E6*pMaterials->GetTotalSegmentFreeShrinkageStrain(thisSegmentKey,intervalIdx,pgsTypes::End);
      }

      rptRcTable* pCreepTable = rptStyleManager::CreateDefaultTable(1 + 2*(nIntervals-1),_T("Creep Coefficients"));
      *pPara << pCreepTable << rptNewLine;
      pCreepTable->SetNumberOfHeaderRows(2);

      col = 0;
      pCreepTable->SetRowSpan(0,col,2);
      (*pCreepTable)(0,col++) << _T("Interval") << rptNewLine << _T("i");

      for (IntervalIndexType intervalIdx = 0; intervalIdx < nIntervals-1; intervalIdx++ )
      {
         pCreepTable->SetColumnSpan(1, col, 2);
         (*pCreepTable)(0,col)   << symbol(psi) << _T("(") << Sub2(_T("t"),_T("ie")) << _T(",") << Sub2(_T("t"),_T("jm")) << _T(")");
         (*pCreepTable)(0,col+1) << symbol(psi) << _T("(") << Sub2(_T("t"),_T("ib")) << _T(",") << Sub2(_T("t"),_T("jm")) << _T(")");
         (*pCreepTable)(1,col) << _T("j = ") << LABEL_INTERVAL(intervalIdx);
         col += 2;
      }

      row = pCreepTable->GetNumberOfHeaderRows();
      for ( IntervalIndexType intervalIdx = 0; intervalIdx < nIntervals; intervalIdx++, row++ )
      {
         col = 0;
         (*pCreepTable)(row,col++) << LABEL_INTERVAL(intervalIdx);
         for ( IntervalIndexType loadIntIdx = 0; loadIntIdx < intervalIdx && loadIntIdx < nIntervals-1; loadIntIdx++ )
         {
            (*pCreepTable)(row,col++) << pMaterials->GetSegmentCreepCoefficient(thisSegmentKey,loadIntIdx,pgsTypes::Middle,intervalIdx,pgsTypes::End);
            (*pCreepTable)(row,col++) << pMaterials->GetSegmentCreepCoefficient(thisSegmentKey,loadIntIdx,pgsTypes::Middle,intervalIdx,pgsTypes::Start);
         }
         for ( IntervalIndexType i = intervalIdx; i < nIntervals-1; i++ )
         {
            (*pCreepTable)(row,col++) << _T("");
            (*pCreepTable)(row,col++) << _T("");
         }
      }

      if ( segIdx != nSegments-1 )
      {
         CSegmentKey closureKey(thisSegmentKey);

         pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
         *pChapter << pPara;
         (*pPara) << _T("Closure Joint at TS ") << LABEL_SEGMENT(segIdx) << rptNewLine;

         pPara = new rptParagraph;
         *pChapter << pPara;

         ColumnIndexType col = 0;
         rptRcTable* pTable = rptStyleManager::CreateDefaultTable(9,_T("Concrete Parameters"));
         (*pPara) << pTable << rptNewLine;

         (*pTable)(0,col++) << _T("Interval");
         (*pTable)(0,col++) << COLHDR(_T("Age"),rptTimeUnitTag,pDisplayUnits->GetWholeDaysUnit());
         (*pTable)(0,col++) << COLHDR(_T("f'c"),rptStressUnitTag,pDisplayUnits->GetStressUnit());
         (*pTable)(0,col++) << COLHDR(_T("Ec"),rptStressUnitTag,pDisplayUnits->GetStressUnit());
         (*pTable)(0,col++) << symbol(psi);
         (*pTable)(0,col++) << symbol(chi);
         (*pTable)(0,col++) << COLHDR(_T("E'c = Ec/(") << _T("1+") << symbol(chi) << symbol(psi) << _T(")"),rptStressUnitTag,pDisplayUnits->GetStressUnit());
         (*pTable)(0,col++) << symbol(DELTA) << Sub2(symbol(epsilon),_T("sh")) << _T("x10") << Super(_T("6"));
         (*pTable)(0,col++) << Sub2(symbol(epsilon),_T("sh")) << _T("x10") << Super(_T("6"));

         RowIndexType row = pTable->GetNumberOfHeaderRows();
         for ( IntervalIndexType intervalIdx = 0; intervalIdx < nIntervals; intervalIdx++, row++ )
         {
            col = 0;
            (*pTable)(row,col++) << LABEL_INTERVAL(intervalIdx);
            (*pTable)(row,col++) << pMaterials->GetClosureJointConcreteAge(closureKey,intervalIdx,pgsTypes::Middle);
            (*pTable)(row,col++) << stress.SetValue(pMaterials->GetClosureJointFc(closureKey,intervalIdx));
            (*pTable)(row,col++) << modE.SetValue(pMaterials->GetClosureJointEc(closureKey,intervalIdx));
            (*pTable)(row,col++) << pMaterials->GetClosureJointCreepCoefficient(closureKey,intervalIdx,pgsTypes::Middle,intervalIdx,pgsTypes::End);
            (*pTable)(row,col++) << pMaterials->GetClosureJointAgingCoefficient(closureKey,intervalIdx);
            (*pTable)(row,col++) << modE.SetValue(pMaterials->GetClosureJointAgeAdjustedEc(closureKey,intervalIdx));
            (*pTable)(row,col++) << 1E6*pMaterials->GetIncrementalClosureJointFreeShrinkageStrain(closureKey,intervalIdx);
            (*pTable)(row,col++) << 1E6*pMaterials->GetTotalClosureJointFreeShrinkageStrain(closureKey,intervalIdx,pgsTypes::End);
         }


         rptRcTable* pCreepTable = rptStyleManager::CreateDefaultTable(1 + 2*(nIntervals-1),_T("Creep Coefficients"));
         *pPara << pCreepTable << rptNewLine;
         pCreepTable->SetNumberOfHeaderRows(2);

         col = 0;
         pCreepTable->SetRowSpan(0,col,2);
         (*pCreepTable)(0,col++) << _T("Interval") << rptNewLine << _T("i");

         for (IntervalIndexType intervalIdx = 0; intervalIdx < nIntervals-1; intervalIdx++ )
         {
            pCreepTable->SetColumnSpan(1, col, 2);
            (*pCreepTable)(0,col)   << symbol(psi) << _T("(") << Sub2(_T("t"),_T("ie")) << _T(",") << Sub2(_T("t"),_T("jm")) << _T(")");
            (*pCreepTable)(0,col+1) << symbol(psi) << _T("(") << Sub2(_T("t"),_T("ib")) << _T(",") << Sub2(_T("t"),_T("jm")) << _T(")");

            (*pCreepTable)(1,col) << _T("j = ") << LABEL_INTERVAL(intervalIdx);
            col += 2;
         }

         row = pCreepTable->GetNumberOfHeaderRows();
         for ( IntervalIndexType intervalIdx = 0; intervalIdx < nIntervals; intervalIdx++, row++ )
         {
            col = 0;
            (*pCreepTable)(row,col++) << LABEL_INTERVAL(intervalIdx);
            for ( IntervalIndexType loadIntIdx = 0; loadIntIdx < intervalIdx && loadIntIdx < nIntervals-1; loadIntIdx++ )
            {
               (*pCreepTable)(row,col++) << pMaterials->GetClosureJointCreepCoefficient(closureKey,loadIntIdx,pgsTypes::Middle,intervalIdx,pgsTypes::End);
               (*pCreepTable)(row,col++) << pMaterials->GetClosureJointCreepCoefficient(closureKey,loadIntIdx,pgsTypes::Middle,intervalIdx,pgsTypes::Start);
            }
            for ( IntervalIndexType i = intervalIdx; i < nIntervals-1; i++ )
            {
               (*pCreepTable)(row,col++) << _T("");
               (*pCreepTable)(row,col++) << _T("");
            }
         }
      } // if segIdx != 0
   } // next segment

   pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pPara;
   (*pPara) << _T("Deck") << rptNewLine;


   GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
   EventIndexType castDeckEventIdx = pIBridgeDesc->GetCastDeckEventIndex();
   const auto* pTimelineEvent = pIBridgeDesc->GetEventByIndex(castDeckEventIdx);
   const auto& castDeckActivity = pTimelineEvent->GetCastDeckActivity();
   ATLASSERT(castDeckActivity.IsEnabled());
   IndexType nCastings = castDeckActivity.GetCastingCount();
   for (IndexType castingIdx = 0; castingIdx < nCastings; castingIdx++)
   {
      pPara = new rptParagraph;
      *pChapter << pPara;

      IndexType sequenceIdx = castDeckActivity.GetSequenceNumber(castingIdx);
      (*pPara) << _T("Sequence ") << LABEL_INDEX(sequenceIdx) << _T(" for regions ");
      std::vector<IndexType> vRegions = castDeckActivity.GetRegions(castingIdx);
      auto begin = std::begin(vRegions);
      auto iter = begin;
      auto end = std::end(vRegions);
      for (; iter != end; iter++)
      {
         if (iter != begin)
         {
            (*pPara) << _T(",");
         }
         (*pPara) << _T(" ") << LABEL_INDEX(*iter);
      }
      (*pPara) << rptNewLine;

      IndexType deckCastingRegionIdx = vRegions.front();

      ColumnIndexType col = 0;
      rptRcTable* pTable = rptStyleManager::CreateDefaultTable(9, _T("Concrete Parameters"));
      (*pPara) << pTable << rptNewLine;

      (*pTable)(0, col++) << _T("Interval");
      (*pTable)(0, col++) << COLHDR(_T("Age"), rptTimeUnitTag, pDisplayUnits->GetWholeDaysUnit());
      (*pTable)(0, col++) << COLHDR(_T("f'c"), rptStressUnitTag, pDisplayUnits->GetStressUnit());
      (*pTable)(0, col++) << COLHDR(_T("Ec"), rptStressUnitTag, pDisplayUnits->GetStressUnit());
      (*pTable)(0, col++) << symbol(psi);
      (*pTable)(0, col++) << symbol(chi);
      (*pTable)(0, col++) << COLHDR(_T("E'c = Ec/(") << _T("1+") << symbol(chi) << symbol(psi) << _T(")"), rptStressUnitTag, pDisplayUnits->GetStressUnit());
      (*pTable)(0, col++) << symbol(DELTA) << Sub2(symbol(epsilon), _T("sh")) << _T("x10") << Super(_T("6"));
      (*pTable)(0, col++) << Sub2(symbol(epsilon), _T("sh")) << _T("x10") << Super(_T("6"));


      RowIndexType row = pTable->GetNumberOfHeaderRows();
      for (IntervalIndexType intervalIdx = 0; intervalIdx < nIntervals; intervalIdx++, row++)
      {
         col = 0;
         (*pTable)(row, col++) << LABEL_INTERVAL(intervalIdx);
         (*pTable)(row, col++) << pMaterials->GetDeckConcreteAge(deckCastingRegionIdx,intervalIdx, pgsTypes::Middle);
         (*pTable)(row, col++) << stress.SetValue(pMaterials->GetDeckFc(deckCastingRegionIdx, intervalIdx));
         (*pTable)(row, col++) << modE.SetValue(pMaterials->GetDeckEc(deckCastingRegionIdx, intervalIdx));
         (*pTable)(row, col++) << pMaterials->GetDeckCreepCoefficient(deckCastingRegionIdx, intervalIdx, pgsTypes::Middle, intervalIdx, pgsTypes::End);
         (*pTable)(row, col++) << pMaterials->GetDeckAgingCoefficient(deckCastingRegionIdx, intervalIdx);
         (*pTable)(row, col++) << modE.SetValue(pMaterials->GetDeckAgeAdjustedEc(deckCastingRegionIdx, intervalIdx));
         (*pTable)(row, col++) << 1E6*pMaterials->GetIncrementalDeckFreeShrinkageStrain(deckCastingRegionIdx, intervalIdx);
         (*pTable)(row, col++) << 1E6*pMaterials->GetTotalDeckFreeShrinkageStrain(deckCastingRegionIdx, intervalIdx, pgsTypes::End);
      }

      rptRcTable* pCreepTable = rptStyleManager::CreateDefaultTable(1 + 2 * (nIntervals - 1), _T("Creep Coefficients"));
      *pPara << pCreepTable << rptNewLine;
      pCreepTable->SetNumberOfHeaderRows(2);

      col = 0;
      pCreepTable->SetRowSpan(0, col, 2);
      (*pCreepTable)(0, col++) << _T("Interval") << rptNewLine << _T("i");

      for (IntervalIndexType intervalIdx = 0; intervalIdx < nIntervals - 1; intervalIdx++)
      {
         pCreepTable->SetColumnSpan(1, col, 2);
         (*pCreepTable)(0, col) << symbol(psi) << _T("(") << Sub2(_T("t"), _T("ie")) << _T(",") << Sub2(_T("t"), _T("jm")) << _T(")");
         (*pCreepTable)(0, col + 1) << symbol(psi) << _T("(") << Sub2(_T("t"), _T("ib")) << _T(",") << Sub2(_T("t"), _T("jm")) << _T(")");
         (*pCreepTable)(1, col) << _T("j = ") << LABEL_INTERVAL(intervalIdx);
         col += 2;
      }

      row = pCreepTable->GetNumberOfHeaderRows();
      for (IntervalIndexType intervalIdx = 0; intervalIdx < nIntervals; intervalIdx++, row++)
      {
         col = 0;
         (*pCreepTable)(row, col++) << LABEL_INTERVAL(intervalIdx);
         for (IntervalIndexType loadIntIdx = 0; loadIntIdx < intervalIdx && loadIntIdx < nIntervals - 1; loadIntIdx++)
         {
            (*pCreepTable)(row, col++) << pMaterials->GetDeckCreepCoefficient(deckCastingRegionIdx, loadIntIdx, pgsTypes::Middle, intervalIdx, pgsTypes::End);
            (*pCreepTable)(row, col++) << pMaterials->GetDeckCreepCoefficient(deckCastingRegionIdx, loadIntIdx, pgsTypes::Middle, intervalIdx, pgsTypes::Start);
         }
         for (IntervalIndexType i = intervalIdx; i < nIntervals - 1; i++)
         {
            (*pCreepTable)(row, col++) << _T("");
            (*pCreepTable)(row, col++) << _T("");
         }
      }
   } // next deck casting


   ///////////////////////////////////////////////////////////////////////////////////
   // Time Step Parameters that are dependent on POI
   ///////////////////////////////////////////////////////////////////////////////////

   PoiList vPoi;
   pPoi->GetPointsOfInterest(CSegmentKey(girderKey, ALL_SEGMENTS), &vPoi);

   for (const pgsPointOfInterest& poi : vPoi)
   {
      const CSegmentKey& segmentKey(poi.GetSegmentKey());

      DuctIndexType nSegmentDucts = pSegmentTendonGeometry->GetDuctCount(segmentKey);
      if (!pPoi->IsOnSegment(poi))
      {
         nSegmentDucts = 0;
      }

      *pPara << location.SetValue(POI_SPAN,poi) << _T(" (ID = " ) << poi.GetID() << _T(")") << rptNewLine;

      CComPtr<IRebarSection> rebar_section;
      pRebarGeom->GetRebars(poi,&rebar_section);

      IndexType nRebar = 0;
      rebar_section->get_Count(&nRebar);

      ColumnIndexType nColumns = 126 + 4*nRebar;
      rptRcTable* pTable2 = rptStyleManager::CreateDefaultTable(nColumns,_T("Time Step Parameters"));
      *pPara << pTable2 << rptNewLine << rptNewLine;

      ColumnIndexType col2 = 0;
      (*pTable2)(0,col2++) << _T("Interval");

      // Transformed properties
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("A"),_T("tr")), rptLength2UnitTag, pDisplayUnits->GetAreaUnit());
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("Y"),_T("tr")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("I"),_T("tr")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
      (*pTable2)(0,col2++) << COLHDR(symbol(DELTA) << Sub2(_T("N"),_T("e")), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());
      (*pTable2)(0,col2++) << COLHDR(symbol(DELTA) << Sub2(_T("M"),_T("e")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit());

      // Net girder properties, unrestrained deformations, and component restraining forces
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("A"),_T("ng")), rptLength2UnitTag, pDisplayUnits->GetAreaUnit());
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("Y"),_T("tng")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("Y"),_T("bng")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("I"),_T("ng")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
      (*pTable2)(0,col2++) << symbol(DELTA) << Sub2(Overline(symbol(epsilon)),_T("gcr")) << _T("(i)");
      (*pTable2)(0,col2++) << COLHDR(symbol(DELTA) << Sub2(Overline(symbol(phi)),_T("gcr")) << _T("(i)"), rptPerLengthUnitTag, pDisplayUnits->GetCurvatureUnit());
      (*pTable2)(0,col2++) << COLHDR(Sub2(Overline(_T("N")),_T("gcr")), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());
      (*pTable2)(0,col2++) << COLHDR(Sub2(Overline(_T("M")),_T("gcr")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit());
      (*pTable2)(0,col2++) << symbol(DELTA) << Sub2(Overline(symbol(epsilon)),_T("gsh")) << _T("(i)");
      (*pTable2)(0,col2++) << COLHDR(Sub2(Overline(_T("N")),_T("gsh")), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());

      // Net slab properties, unrestrained deformations, and component restraining forces
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("A"),_T("nd")), rptLength2UnitTag, pDisplayUnits->GetAreaUnit());
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("Y"),_T("tnd")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("Y"),_T("bnd")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("I"),_T("nd")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
      (*pTable2)(0,col2++) << symbol(DELTA) << Sub2(Overline(symbol(epsilon)),_T("dcr")) << _T("(i)");
      (*pTable2)(0,col2++) << COLHDR(symbol(DELTA) << Sub2(Overline(symbol(phi)),_T("dcr")) << _T("(i)"), rptPerLengthUnitTag, pDisplayUnits->GetCurvatureUnit());
      (*pTable2)(0,col2++) << COLHDR(Sub2(Overline(_T("N")),_T("dcr")), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());
      (*pTable2)(0,col2++) << COLHDR(Sub2(Overline(_T("M")),_T("dcr")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit());
      (*pTable2)(0,col2++) << symbol(DELTA) << Sub2(Overline(symbol(epsilon)),_T("dsh")) << _T("(i)");
      (*pTable2)(0,col2++) << COLHDR(Sub2(Overline(_T("N")),_T("dsh")), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());

      // Slab top mat rebar properties
      (*pTable2)(0, col2++) << COLHDR(Sub2(_T("A"), _T("tm i")), rptLength2UnitTag, pDisplayUnits->GetAreaUnit());
      (*pTable2)(0, col2++) << COLHDR(Sub2(_T("Y"), _T("tm i")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      (*pTable2)(0, col2++) << COLHDR(Sub2(_T("A"), _T("tm l")), rptLength2UnitTag, pDisplayUnits->GetAreaUnit());
      (*pTable2)(0, col2++) << COLHDR(Sub2(_T("Y"), _T("tm l")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());

      // Slab bottom mat rebar properties
      (*pTable2)(0, col2++) << COLHDR(Sub2(_T("A"), _T("bm i")), rptLength2UnitTag, pDisplayUnits->GetAreaUnit());
      (*pTable2)(0, col2++) << COLHDR(Sub2(_T("Y"), _T("bm i")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      (*pTable2)(0, col2++) << COLHDR(Sub2(_T("A"), _T("bm l")), rptLength2UnitTag, pDisplayUnits->GetAreaUnit());
      (*pTable2)(0, col2++) << COLHDR(Sub2(_T("Y"), _T("bm l")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());

      // Girder Rebar
      for (IndexType rebarIdx = 0; rebarIdx < nRebar; rebarIdx++ )
      {
         (*pTable2)(0,col2++) << COLHDR(Sub2(_T("A"),_T("gb")), rptLength2UnitTag, pDisplayUnits->GetAreaUnit());
         (*pTable2)(0,col2++) << COLHDR(Sub2(_T("Y"),_T("gb")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      }

      // unrestrained deformations and component restraining forces for strand (straight)
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("A"),_T("ps")), rptLength2UnitTag, pDisplayUnits->GetAreaUnit());
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("Y"),_T("ps")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      (*pTable2)(0,col2++) << COLHDR(symbol(DELTA) << Sub2(_T("f"),_T("r ps")) << _T("(i)"), rptStressUnitTag, pDisplayUnits->GetStressUnit());
      (*pTable2)(0,col2++) << symbol(DELTA) << Sub2(Overline(symbol(epsilon)),_T("ps")) << _T("(i)");
      (*pTable2)(0,col2++) << COLHDR(Sub2(Overline(_T("N")),_T("ps")), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());

      // unrestrained deformations and component restraining forces for strand (harped)
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("A"),_T("ps")), rptLength2UnitTag, pDisplayUnits->GetAreaUnit());
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("Y"),_T("ps")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      (*pTable2)(0,col2++) << COLHDR(symbol(DELTA) << Sub2(_T("f"),_T("r ps")) << _T("(i)"), rptStressUnitTag, pDisplayUnits->GetStressUnit());
      (*pTable2)(0,col2++) << symbol(DELTA) << Sub2(Overline(symbol(epsilon)),_T("ps")) << _T("(i)");
      (*pTable2)(0,col2++) << COLHDR(Sub2(Overline(_T("N")),_T("ps")), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());

      // unrestrained deformations and component restraining forces for strand (temporary)
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("A"),_T("ps")), rptLength2UnitTag, pDisplayUnits->GetAreaUnit());
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("Y"),_T("ps")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      (*pTable2)(0,col2++) << COLHDR(symbol(DELTA) << Sub2(_T("f"),_T("r ps")) << _T("(i)"), rptStressUnitTag, pDisplayUnits->GetStressUnit());
      (*pTable2)(0,col2++) << symbol(DELTA) << Sub2(Overline(symbol(epsilon)),_T("ps")) << _T("(i)");
      (*pTable2)(0,col2++) << COLHDR(Sub2(Overline(_T("N")),_T("ps")), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());

      // unrestrained deformations and component restraining forces for segment tendon
      (*pTable2)(0, col2++) << COLHDR(Sub2(_T("A"), _T("pt s")), rptLength2UnitTag, pDisplayUnits->GetAreaUnit());
      (*pTable2)(0, col2++) << COLHDR(Sub2(_T("Y"), _T("pt s")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      (*pTable2)(0, col2++) << COLHDR(symbol(DELTA) << Sub2(_T("f"), _T("r pt s")) << _T("(i)"), rptStressUnitTag, pDisplayUnits->GetStressUnit());
      (*pTable2)(0, col2++) << symbol(DELTA) << Sub2(Overline(symbol(epsilon)), _T("pt s")) << _T("(i)");
      (*pTable2)(0, col2++) << COLHDR(Sub2(Overline(_T("N")), _T("pt s")), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());

      // unrestrained deformations and component restraining forces for girder tendon
      (*pTable2)(0, col2++) << COLHDR(Sub2(_T("A"), _T("pt g")), rptLength2UnitTag, pDisplayUnits->GetAreaUnit());
      (*pTable2)(0, col2++) << COLHDR(Sub2(_T("Y"), _T("pt g")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      (*pTable2)(0, col2++) << COLHDR(symbol(DELTA) << Sub2(_T("f"), _T("r pt g")) << _T("(i)"), rptStressUnitTag, pDisplayUnits->GetStressUnit());
      (*pTable2)(0, col2++) << symbol(DELTA) << Sub2(Overline(symbol(epsilon)), _T("pt g")) << _T("(i)");
      (*pTable2)(0, col2++) << COLHDR(Sub2(Overline(_T("N")), _T("pt g")), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());

      // total cross section restraining force
      (*pTable2)(0,col2++) << COLHDR(Overline(_T("Ncr")), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());
      (*pTable2)(0,col2++) << COLHDR(Overline(_T("Nsh")), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());
      (*pTable2)(0,col2++) << COLHDR(Overline(_T("Nre")), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());

      (*pTable2)(0,col2++) << COLHDR(Overline(_T("Mcr")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit());
      (*pTable2)(0,col2++) << COLHDR(Overline(_T("Msh")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit());
      (*pTable2)(0,col2++) << COLHDR(Overline(_T("Mre")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit());

      // initial strains
      (*pTable2)(0,col2++) << Overline(symbol(epsilon) << _T("cr"));
      (*pTable2)(0,col2++) << Overline(symbol(epsilon) << _T("sh"));
      (*pTable2)(0,col2++) << Overline(symbol(epsilon) << _T("re"));
      (*pTable2)(0,col2++) << COLHDR(Overline(symbol(phi)) << _T("cr"), rptPerLengthUnitTag, pDisplayUnits->GetCurvatureUnit());
      (*pTable2)(0,col2++) << COLHDR(Overline(symbol(phi)) << _T("sh"), rptPerLengthUnitTag, pDisplayUnits->GetCurvatureUnit());
      (*pTable2)(0,col2++) << COLHDR(Overline(symbol(phi)) << _T("re"), rptPerLengthUnitTag, pDisplayUnits->GetCurvatureUnit());

      // cross section forces in this interval due to initial strains
      (*pTable2)(0,col2++) << COLHDR(symbol(DELTA) << Sub2(_T("N"),_T("i cr")), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());
      (*pTable2)(0,col2++) << COLHDR(symbol(DELTA) << Sub2(_T("N"),_T("i sh")), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());
      (*pTable2)(0,col2++) << COLHDR(symbol(DELTA) << Sub2(_T("N"),_T("i re")), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());
      (*pTable2)(0,col2++) << COLHDR(symbol(DELTA) << Sub2(_T("M"),_T("i cr")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit());
      (*pTable2)(0,col2++) << COLHDR(symbol(DELTA) << Sub2(_T("M"),_T("i sh")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit());
      (*pTable2)(0,col2++) << COLHDR(symbol(DELTA) << Sub2(_T("M"),_T("i re")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit());

      // Deformations due to externally applied loads and restraining forces (symbols might not be exactly correct)
      (*pTable2)(0,col2++) << symbol(DELTA) << symbol(epsilon) << _T("(i)");
      (*pTable2)(0,col2++) << COLHDR(symbol(DELTA) << symbol(phi) << _T("(i)"), rptPerLengthUnitTag, pDisplayUnits->GetCurvatureUnit());

      // Component forces due to deformations during this interval

      // girder
      (*pTable2)(0,col2++) << COLHDR(symbol(DELTA) << Sub2(_T("N"),_T("g")), rptForceUnitTag,  pDisplayUnits->GetGeneralForceUnit());
      (*pTable2)(0,col2++) << COLHDR(symbol(DELTA) << Sub2(_T("M"),_T("g")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit());

      // deck
      (*pTable2)(0,col2++) << COLHDR(symbol(DELTA) << Sub2(_T("N"),_T("d")), rptForceUnitTag,  pDisplayUnits->GetGeneralForceUnit());
      (*pTable2)(0,col2++) << COLHDR(symbol(DELTA) << Sub2(_T("M"),_T("d")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit());

      // deck top mat rebar
      (*pTable2)(0, col2++) << COLHDR(symbol(DELTA) << Sub2(_T("N"), _T("tm i")), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());
      (*pTable2)(0, col2++) << COLHDR(symbol(DELTA) << Sub2(_T("N"), _T("tm l")), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());

      // deck bottom mat rebar
      (*pTable2)(0, col2++) << COLHDR(symbol(DELTA) << Sub2(_T("N"), _T("bm i")), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());
      (*pTable2)(0, col2++) << COLHDR(symbol(DELTA) << Sub2(_T("N"), _T("bm l")), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());

      // girder rebar
      for ( IndexType rebarIdx = 0; rebarIdx < nRebar; rebarIdx++ )
      {
         (*pTable2)(0,col2++) << COLHDR(symbol(DELTA) << Sub2(_T("N"),_T("gb")), rptForceUnitTag,  pDisplayUnits->GetGeneralForceUnit());
      }

      // strands (straight)
      (*pTable2)(0,col2++) << COLHDR(symbol(DELTA) << Sub2(_T("N"),_T("ps s")), rptForceUnitTag,  pDisplayUnits->GetGeneralForceUnit());

      // strands (harped)
      (*pTable2)(0,col2++) << COLHDR(symbol(DELTA) << Sub2(_T("N"),_T("ps h")), rptForceUnitTag,  pDisplayUnits->GetGeneralForceUnit());

      // strands (temporary)
      (*pTable2)(0,col2++) << COLHDR(symbol(DELTA) << Sub2(_T("N"),_T("ps t")), rptForceUnitTag,  pDisplayUnits->GetGeneralForceUnit());

      // segment tendons
      (*pTable2)(0, col2++) << COLHDR(symbol(DELTA) << Sub2(_T("N"), _T("pt s")), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());

      // girder tendons
      (*pTable2)(0, col2++) << COLHDR(symbol(DELTA) << Sub2(_T("N"), _T("pt g")), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());

      // Component forces at the end of this interval

      // girder
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("N"),_T("g")), rptForceUnitTag,  pDisplayUnits->GetGeneralForceUnit());
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("M"),_T("g")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit());

      // deck
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("N"),_T("d")), rptForceUnitTag,  pDisplayUnits->GetGeneralForceUnit());
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("M"),_T("d")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit());

      // deck top mat rebar
      (*pTable2)(0, col2++) << COLHDR(Sub2(_T("N"), _T("tm i")), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());
      (*pTable2)(0, col2++) << COLHDR(Sub2(_T("N"), _T("tm l")), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());

      // deck bottom mat rebar
      (*pTable2)(0, col2++) << COLHDR(Sub2(_T("N"), _T("bm i")), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());
      (*pTable2)(0, col2++) << COLHDR(Sub2(_T("N"), _T("bm l")), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());

      // girder rebar
      for ( IndexType rebarIdx = 0; rebarIdx < nRebar; rebarIdx++ )
      {
         (*pTable2)(0,col2++) << COLHDR(Sub2(_T("N"),_T("gb")), rptForceUnitTag,  pDisplayUnits->GetGeneralForceUnit());
      }

      // strands (straight)
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("N"),_T("ps s")), rptForceUnitTag,  pDisplayUnits->GetGeneralForceUnit());

      // strands (harped)
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("N"),_T("ps h")), rptForceUnitTag,  pDisplayUnits->GetGeneralForceUnit());

      // strands (temporary)
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("N"),_T("ps t")), rptForceUnitTag,  pDisplayUnits->GetGeneralForceUnit());

      // segment tendons
      (*pTable2)(0, col2++) << COLHDR(Sub2(_T("N"), _T("pt s")), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());

      // girder tendons
      (*pTable2)(0, col2++) << COLHDR(Sub2(_T("N"), _T("pt g")), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());

      // Change in strand stress during this interval
      
      // strands (straight)
      (*pTable2)(0,col2++) << COLHDR(symbol(DELTA) << Sub2(_T("f"),_T("ps s")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("f"),_T("pse s")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
      
      // strands (harped)
      (*pTable2)(0,col2++) << COLHDR(symbol(DELTA) << Sub2(_T("f"),_T("ps h")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("f"),_T("pse h")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
      
      // strands (temporary)
      (*pTable2)(0,col2++) << COLHDR(symbol(DELTA) << Sub2(_T("f"),_T("ps t")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("f"),_T("pse t")), rptStressUnitTag, pDisplayUnits->GetStressUnit());

      // segment tendons
      (*pTable2)(0, col2++) << COLHDR(symbol(DELTA) << Sub2(_T("f"), _T("pt s")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
      (*pTable2)(0, col2++) << COLHDR(Sub2(_T("f"), _T("pte s")), rptStressUnitTag, pDisplayUnits->GetStressUnit());

      // girder tendons
      (*pTable2)(0, col2++) << COLHDR(symbol(DELTA) << Sub2(_T("f"), _T("pt g")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
      (*pTable2)(0, col2++) << COLHDR(Sub2(_T("f"), _T("pte g")), rptStressUnitTag, pDisplayUnits->GetStressUnit());

      // girder stress
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("f"),_T("bot girder")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("f"),_T("top girder")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("f"),_T("top deck")), rptStressUnitTag, pDisplayUnits->GetStressUnit());

      // check
      (*pTable2)(0,col2++) << COLHDR(symbol(DELTA) << Sub2(_T("N"),_T("ext")), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("N"),_T("ext")), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());
      (*pTable2)(0,col2++) << COLHDR(symbol(DELTA) << Sub2(_T("N"),_T("int")), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("N"),_T("int")), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());

      // check
      (*pTable2)(0,col2++) << COLHDR(symbol(DELTA) << Sub2(_T("M"),_T("ext")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit());
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("M"),_T("ext")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit());
      (*pTable2)(0,col2++) << COLHDR(symbol(DELTA) << Sub2(_T("M"),_T("int")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit());
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("M"),_T("int")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit());

      const LOSSDETAILS* pDetails = pLosses->GetLossDetails(poi,INVALID_INDEX);

      RowIndexType row2 = pTable2->GetNumberOfHeaderRows();

      for ( IntervalIndexType intervalIdx = 0; intervalIdx < nIntervals; intervalIdx++, row2++ )
      {
         const TIME_STEP_DETAILS& tsDetails(pDetails->TimeStepDetails[intervalIdx]);

         int N = sizeof(tsDetails.dPi)/sizeof(tsDetails.dPi[0]);

         col2 = 0;
         (*pTable2)(row2,col2++) << LABEL_INTERVAL(intervalIdx);

         (*pTable2)(row2,col2++) << area.SetValue(tsDetails.Atr);
         (*pTable2)(row2,col2++) << ecc.SetValue(tsDetails.Ytr);
         (*pTable2)(row2,col2++) << momI.SetValue(tsDetails.Itr);

         // summation of externally applied loads
#if defined REPORT_PRODUCT_LOAD_DETAILS
         GET_IFACE2(pBroker,IProductLoads,pProductLoads);
#endif //REPORT_PRODUCT_LOAD_DETAILS
         Float64 dP = 0;
         Float64 dM = 0;
         for ( int i = 0; i < N; i++ )
         {
            pgsTypes::ProductForceType pfType = (pgsTypes::ProductForceType)i;
            if ( pfType == pgsTypes::pftCreep || pfType == pgsTypes::pftShrinkage || pfType == pgsTypes::pftRelaxation )
            {
               continue;
            }
#if defined REPORT_PRODUCT_LOAD_DETAILS
            (*pTable2)(row2,col2  ) << pProductLoads->GetProductLoadName(pfType) << _T(" ") << force.SetValue(tsDetails.dPi[pfType]) << rptNewLine;
            (*pTable2)(row2,col2+1) << pProductLoads->GetProductLoadName(pfType) << _T(" ") << moment.SetValue(tsDetails.dMi[pfType]) << rptNewLine;
#endif //REPORT_PRODUCT_LOAD_DETAILS
            dP += tsDetails.dPi[pfType];
            dM += tsDetails.dMi[pfType];
         }

         (*pTable2)(row2,col2++) << force.SetValue(dP);
         (*pTable2)(row2,col2++) << moment.SetValue(dM);

         (*pTable2)(row2,col2++) << area.SetValue(tsDetails.Girder.An);
         (*pTable2)(row2,col2++) << ecc.SetValue(tsDetails.Girder.Yn);
         (*pTable2)(row2,col2++) << ecc.SetValue(tsDetails.Girder.H + tsDetails.Girder.Yn);
         (*pTable2)(row2,col2++) << momI.SetValue(tsDetails.Girder.In);

#if defined REPORT_INITIAL_STRAIN_DETAILS
         std::vector<TIME_STEP_CONCRETE::CREEP_STRAIN>::const_iterator creepStrainIter(tsDetails.Girder.ec.begin());
         std::vector<TIME_STEP_CONCRETE::CREEP_STRAIN>::const_iterator creepStrainIterEnd(tsDetails.Girder.ec.end());
         std::vector<TIME_STEP_CONCRETE::CREEP_STRAIN>::size_type i = tsDetails.Girder.ec.size()-1;
         for ( ; creepStrainIter != creepStrainIterEnd; creepStrainIter++, i-- )
         {
            const TIME_STEP_CONCRETE::CREEP_STRAIN& creepStrain(*creepStrainIter);
            (*pTable2)(row2,col2) << _T("[") << force.SetValue(creepStrain.P) << _T("/((") << modE.SetValue(creepStrain.E) << _T(")(") << area.SetValue(creepStrain.A) << _T("))](") << creepStrain.Ce << _T(" - ") << creepStrain.Cs << _T(")");
            if ( i != 0 )
            {
               (*pTable2)(row2,col2) << _T(" + ") << rptNewLine;
            }
            else
            {
               (*pTable2)(row2,col2) << _T(" = ") << rptNewLine;
            }
         }
#endif // REPORT_INITIAL_STRAIN_DETAILS
         (*pTable2)(row2,col2++) << tsDetails.Girder.eci;

         
#if defined REPORT_INITIAL_STRAIN_DETAILS
         std::vector<TIME_STEP_CONCRETE::CREEP_CURVATURE>::const_iterator creepCurvatureIter(tsDetails.Girder.rc.begin());
         std::vector<TIME_STEP_CONCRETE::CREEP_CURVATURE>::const_iterator creepCurvatureIterEnd(tsDetails.Girder.rc.end());
         i = tsDetails.Girder.rc.size()-1;
         for ( ; creepCurvatureIter != creepCurvatureIterEnd; creepCurvatureIter++, i-- )
         {
            const TIME_STEP_CONCRETE::CREEP_CURVATURE& creepCurvature(*creepCurvatureIter);
            (*pTable2)(row2,col2) << _T("[(") << moment.SetValue(creepCurvature.M) << _T(")(12)/((") << modE.SetValue(creepCurvature.E) << _T(")(") << momI.SetValue(creepCurvature.I) << _T("))](") << creepCurvature.Ce << _T(" - ") << creepCurvature.Cs << _T(")");
            if ( i != 0 )
            {
               (*pTable2)(row2,col2) << _T(" + ") << rptNewLine;
            }
            else
            {
               (*pTable2)(row2,col2) << _T(" = ") << rptNewLine;
            }
         }
#endif // REPORT_INITIAL_STRAIN_DETAILS
         (*pTable2)(row2,col2++) << curvature.SetValue(tsDetails.Girder.rci);

         (*pTable2)(row2,col2++) << force.SetValue(tsDetails.Girder.PrCreep);
         (*pTable2)(row2,col2++) << moment.SetValue(tsDetails.Girder.MrCreep);
         (*pTable2)(row2,col2++) << tsDetails.Girder.Shrinkage.esi;
         (*pTable2)(row2,col2++) << force.SetValue(tsDetails.Girder.PrShrinkage);

         (*pTable2)(row2,col2++) << area.SetValue(tsDetails.Deck.An);
         (*pTable2)(row2,col2++) << ecc.SetValue(tsDetails.Deck.H - tsDetails.Deck.Yn);
         (*pTable2)(row2,col2++) << ecc.SetValue(tsDetails.Deck.Yn);
         (*pTable2)(row2,col2++) << momI.SetValue(tsDetails.Deck.In);

         
#if defined REPORT_INITIAL_STRAIN_DETAILS
         creepStrainIter    = tsDetails.Deck.ec.begin();
         creepStrainIterEnd = tsDetails.Deck.ec.end();
         i = tsDetails.Deck.ec.size()-1;
         for ( ; creepStrainIter != creepStrainIterEnd; creepStrainIter++, i-- )
         {
            const TIME_STEP_CONCRETE::CREEP_STRAIN& creepStrain(*creepStrainIter);
            (*pTable2)(row2,col2) << _T("[") << force.SetValue(creepStrain.P) << _T("/((") << modE.SetValue(creepStrain.E) << _T(")(") << area.SetValue(creepStrain.A) << _T("))](") << creepStrain.Ce << _T(" - ") << creepStrain.Cs << _T(")");
            if ( i != 0 )
            {
               (*pTable2)(row2,col2) << _T(" + ") << rptNewLine;
            }
            else
            {
               (*pTable2)(row2,col2) << _T(" = ") << rptNewLine;
            }
         }
#endif // REPORT_INITIAL_STRAIN_DETAILS
         (*pTable2)(row2,col2++) << tsDetails.Deck.eci;

         
#if defined REPORT_INITIAL_STRAIN_DETAILS
         creepCurvatureIter    = tsDetails.Deck.rc.begin();
         creepCurvatureIterEnd = tsDetails.Deck.rc.end();
         i = tsDetails.Deck.rc.size()-1;
         for ( ; creepCurvatureIter != creepCurvatureIterEnd; creepCurvatureIter++, i-- )
         {
            const TIME_STEP_CONCRETE::CREEP_CURVATURE& creepCurvature(*creepCurvatureIter);
            (*pTable2)(row2,col2) << _T("[(") << moment.SetValue(creepCurvature.M) << _T(")(12)/((") << modE.SetValue(creepCurvature.E) << _T(")(") << momI.SetValue(creepCurvature.I) << _T("))](") << creepCurvature.Ce << _T(" - ") << creepCurvature.Cs << _T(")");
            if ( i != 0 )
            {
               (*pTable2)(row2,col2) << _T(" + ") << rptNewLine;
            }
            else
            {
               (*pTable2)(row2,col2) << _T(" = ") << rptNewLine;
            }
         }
#endif // REPORT_INITIAL_STRAIN_DETAILS
         (*pTable2)(row2,col2++) << curvature.SetValue(tsDetails.Deck.rci);

         (*pTable2)(row2,col2++) << force.SetValue(tsDetails.Deck.PrCreep);
         (*pTable2)(row2,col2++) << moment.SetValue(tsDetails.Deck.MrCreep);
         (*pTable2)(row2,col2++) << tsDetails.Deck.Shrinkage.esi;
         (*pTable2)(row2,col2++) << force.SetValue(tsDetails.Deck.PrShrinkage);

         (*pTable2)(row2, col2++) << area.SetValue(tsDetails.DeckRebar[pgsTypes::drmTop][pgsTypes::drbIndividual].As);
         (*pTable2)(row2, col2++) << ecc.SetValue(tsDetails.DeckRebar[pgsTypes::drmTop][pgsTypes::drbIndividual].Ys);
         (*pTable2)(row2, col2++) << area.SetValue(tsDetails.DeckRebar[pgsTypes::drmTop][pgsTypes::drbLumpSum].As);
         (*pTable2)(row2, col2++) << ecc.SetValue(tsDetails.DeckRebar[pgsTypes::drmTop][pgsTypes::drbLumpSum].Ys);

         (*pTable2)(row2, col2++) << area.SetValue(tsDetails.DeckRebar[pgsTypes::drmBottom][pgsTypes::drbIndividual].As);
         (*pTable2)(row2, col2++) << ecc.SetValue(tsDetails.DeckRebar[pgsTypes::drmBottom][pgsTypes::drbIndividual].Ys);
         (*pTable2)(row2, col2++) << area.SetValue(tsDetails.DeckRebar[pgsTypes::drmBottom][pgsTypes::drbLumpSum].As);
         (*pTable2)(row2, col2++) << ecc.SetValue(tsDetails.DeckRebar[pgsTypes::drmBottom][pgsTypes::drbLumpSum].Ys);

         std::vector<TIME_STEP_REBAR>::const_iterator iter(tsDetails.GirderRebar.begin());
         std::vector<TIME_STEP_REBAR>::const_iterator end(tsDetails.GirderRebar.end());
         for ( ; iter != end; iter++ )
         {
            const TIME_STEP_REBAR& tsRebar(*iter);

            (*pTable2)(row2,col2++) << area.SetValue(tsRebar.As);
            (*pTable2)(row2,col2++) << ecc.SetValue(tsRebar.Ys);
         }

         for ( int i = 0; i < 3; i++ )
         {
            pgsTypes::StrandType strandType = pgsTypes::StrandType(i);
#if defined LUMP_STRANDS
            (*pTable2)(row2,col2++) << area.SetValue(tsDetails.Strands[strandType].As);
            (*pTable2)(row2,col2++) << ecc.SetValue(tsDetails.Strands[strandType].Ys);
            (*pTable2)(row2,col2++) << stress.SetValue(tsDetails.Strands[strandType].Relaxation.fr);
            (*pTable2)(row2,col2++) << tsDetails.Strands[strandType].er;
            (*pTable2)(row2,col2++) << force.SetValue(tsDetails.Strands[strandType].PrRelaxation);
#else
            StrandIndexType nStrands = pStrandGeom->GetStrandCount(segmentKey,strandType);
            for ( StrandIndexType strandIdx = 0; strandIdx < nStrands; strandIdx++ )
            {
               const TIME_STEP_STRAND& strand = tsDetails.Strands[strandType][strandIdx];
               (*pTable2)(row2,col2+0) << area.SetValue(strand.As) << rptNewLine;
               (*pTable2)(row2,col2+1) << ecc.SetValue(strand.Ys) << rptNewLine;
               (*pTable2)(row2,col2+2) << stress.SetValue(strand.Relaxation.fr) << rptNewLine;
               (*pTable2)(row2,col2+3) << strand.er << rptNewLine;
               (*pTable2)(row2,col2+4) << force.SetValue(strand.PrRelaxation) << rptNewLine;
            } // next strand
            col2 += 5;
#endif
         } // next strand type

         for (DuctIndexType ductIdx = 0; ductIdx < nSegmentDucts; ductIdx++)
         {
            const TIME_STEP_STRAND& tendon(tsDetails.SegmentTendons[ductIdx]);
            (*pTable2)(row2, col2 + 0) << area.SetValue(tendon.As) << rptNewLine;
            (*pTable2)(row2, col2 + 1) << ecc.SetValue(tendon.Ys) << rptNewLine;
            (*pTable2)(row2, col2 + 2) << stress.SetValue(tendon.Relaxation.fr) << rptNewLine;
            (*pTable2)(row2, col2 + 3) << tendon.er << rptNewLine;
            (*pTable2)(row2, col2 + 4) << force.SetValue(tendon.PrRelaxation) << rptNewLine;
         }
         col2 += 5;

         for (DuctIndexType ductIdx = 0; ductIdx < nGirderDucts; ductIdx++)
         {
            const TIME_STEP_STRAND& tendon(tsDetails.GirderTendons[ductIdx]);
            (*pTable2)(row2, col2 + 0) << area.SetValue(tendon.As) << rptNewLine;
            (*pTable2)(row2, col2 + 1) << ecc.SetValue(tendon.Ys) << rptNewLine;
            (*pTable2)(row2, col2 + 2) << stress.SetValue(tendon.Relaxation.fr) << rptNewLine;
            (*pTable2)(row2, col2 + 3) << tendon.er << rptNewLine;
            (*pTable2)(row2, col2 + 4) << force.SetValue(tendon.PrRelaxation) << rptNewLine;
         }
         col2 += 5;

         (*pTable2)(row2,col2++) << force.SetValue( tsDetails.Pr[TIMESTEP_CR] );
         (*pTable2)(row2,col2++) << force.SetValue( tsDetails.Pr[TIMESTEP_SH] );
         (*pTable2)(row2,col2++) << force.SetValue( tsDetails.Pr[TIMESTEP_RE] );

         (*pTable2)(row2,col2++) << moment.SetValue(tsDetails.Mr[TIMESTEP_CR] );
         (*pTable2)(row2,col2++) << moment.SetValue(tsDetails.Mr[TIMESTEP_SH] );
         (*pTable2)(row2,col2++) << moment.SetValue(tsDetails.Mr[TIMESTEP_RE] );

         (*pTable2)(row2,col2++) << tsDetails.e[TIMESTEP_CR];
         (*pTable2)(row2,col2++) << tsDetails.e[TIMESTEP_SH];
         (*pTable2)(row2,col2++) << tsDetails.e[TIMESTEP_RE];
         (*pTable2)(row2,col2++) << curvature.SetValue(tsDetails.r[TIMESTEP_CR]);
         (*pTable2)(row2,col2++) << curvature.SetValue(tsDetails.r[TIMESTEP_SH]);
         (*pTable2)(row2,col2++) << curvature.SetValue(tsDetails.r[TIMESTEP_RE]);

         (*pTable2)(row2,col2++) << force.SetValue( tsDetails.dPi[pgsTypes::pftCreep] );
         (*pTable2)(row2,col2++) << force.SetValue( tsDetails.dPi[pgsTypes::pftShrinkage] );
         (*pTable2)(row2,col2++) << force.SetValue( tsDetails.dPi[pgsTypes::pftRelaxation] );
         (*pTable2)(row2,col2++) << moment.SetValue(tsDetails.dMi[pgsTypes::pftCreep]);
         (*pTable2)(row2,col2++) << moment.SetValue(tsDetails.dMi[pgsTypes::pftShrinkage]);
         (*pTable2)(row2,col2++) << moment.SetValue(tsDetails.dMi[pgsTypes::pftRelaxation]);

         Float64 der = 0;
         Float64 drr = 0;

         for ( int i = 0; i < N; i++ )
         {
            der += tsDetails.der[i];
            drr += tsDetails.drr[i];
         }

         (*pTable2)(row2,col2++) << der;
         (*pTable2)(row2,col2++) << curvature.SetValue(drr);

         Float64 dPgirder = 0;
         Float64 dMgirder = 0;
         Float64 dPdeck = 0;
         Float64 dMdeck = 0;
         std::array<Float64, 2> dPtopMat{ 0.,0. };
         std::array<Float64, 2> dPbotMat{ 0.,0. };

         for ( int i = 0; i < N; i++ )
         {
            dPgirder += tsDetails.Girder.dPi[i];
            dMgirder += tsDetails.Girder.dMi[i];

            dPdeck += tsDetails.Deck.dPi[i];
            dMdeck += tsDetails.Deck.dMi[i];

            dPtopMat[pgsTypes::drbIndividual] += tsDetails.DeckRebar[pgsTypes::drmTop][pgsTypes::drbIndividual].dPi[i];
            dPtopMat[pgsTypes::drbLumpSum] += tsDetails.DeckRebar[pgsTypes::drmTop][pgsTypes::drbLumpSum].dPi[i];
            dPbotMat[pgsTypes::drbIndividual] += tsDetails.DeckRebar[pgsTypes::drmBottom][pgsTypes::drbIndividual].dPi[i];
            dPbotMat[pgsTypes::drbLumpSum] += tsDetails.DeckRebar[pgsTypes::drmBottom][pgsTypes::drbLumpSum].dPi[i];
         }

         (*pTable2)(row2,col2++) << force.SetValue( dPgirder );
         (*pTable2)(row2,col2++) << moment.SetValue( dMgirder );
         (*pTable2)(row2,col2++) << force.SetValue( dPdeck );
         (*pTable2)(row2,col2++) << moment.SetValue( dMdeck );
         (*pTable2)(row2, col2++) << force.SetValue(dPtopMat[pgsTypes::drbIndividual]);
         (*pTable2)(row2, col2++) << force.SetValue(dPtopMat[pgsTypes::drbLumpSum]);
         (*pTable2)(row2, col2++) << force.SetValue(dPbotMat[pgsTypes::drbIndividual]);
         (*pTable2)(row2, col2++) << force.SetValue(dPbotMat[pgsTypes::drbLumpSum]);

         iter = tsDetails.GirderRebar.begin();
         for ( ; iter != end; iter++ )
         {
            const TIME_STEP_REBAR& tsRebar(*iter);
            Float64 dPgirderRebar = 0;
            for ( int i = 0; i < N; i++ )
            {
               dPgirderRebar += tsRebar.dPi[i];
            }
            (*pTable2)(row2,col2++) << force.SetValue(dPgirderRebar);
         }

         for ( int i = 0; i < 3; i++ )
         {
            pgsTypes::StrandType strandType = pgsTypes::StrandType(i);
#if defined LUMP_STRANDS
            Float64 dPstrand = 0;
            for ( int i = 0; i < N; i++ )
            {
               dPstrand += tsDetails.Strands[strandType].dPi[i];
            }
            (*pTable2)(row2,col2++) << force.SetValue(dPstrand);
#else
            StrandIndexType nStrands = pStrandGeom->GetStrandCount(segmentKey,strandType);
            for ( StrandIndexType strandIdx = 0; strandIdx < nStrands; strandIdx++ )
            {
               const TIME_STEP_STRAND& strand(tsDetails.Strands[strandType][strandIdx]);
               Float64 dPstrand = 0;
               for ( int i = 0; i < N; i++ )
               {
                  dPstrand += strand.dPi[i];
               }
               (*pTable2)(row2,col2) << force.SetValue(dPstrand) << rptNewLine;
            } // next strand
            col2 += 1;
#endif
         } // next strand type

         for (DuctIndexType ductIdx = 0; ductIdx < nSegmentDucts; ductIdx++)
         {
            const TIME_STEP_STRAND& tendon(tsDetails.SegmentTendons[ductIdx]);
            Float64 dPtendon = 0;
            for (int i = 0; i < N; i++)
            {
               dPtendon += tendon.dPi[i];
            }
            (*pTable2)(row2, col2) << force.SetValue(dPtendon) << rptNewLine;
         }
         col2 += 1;

         for ( DuctIndexType ductIdx = 0; ductIdx < nGirderDucts; ductIdx++ )
         {
            const TIME_STEP_STRAND& tendon(tsDetails.GirderTendons[ductIdx]);
            Float64 dPtendon = 0;
            for ( int i = 0; i < N; i++ )
            {
               dPtendon += tendon.dPi[i];
            }
            (*pTable2)(row2,col2) << force.SetValue(dPtendon) << rptNewLine;
         }
         col2 += 1;

         (*pTable2)(row2,col2++) << force.SetValue(tsDetails.Girder.P);
         (*pTable2)(row2,col2++) << moment.SetValue(tsDetails.Girder.M);
         (*pTable2)(row2,col2++) << force.SetValue(tsDetails.Deck.P);
         (*pTable2)(row2,col2++) << moment.SetValue(tsDetails.Deck.M);
         (*pTable2)(row2, col2++) << force.SetValue(tsDetails.DeckRebar[pgsTypes::drmTop][pgsTypes::drbIndividual].P);
         (*pTable2)(row2, col2++) << force.SetValue(tsDetails.DeckRebar[pgsTypes::drmTop][pgsTypes::drbLumpSum].P);
         (*pTable2)(row2, col2++) << force.SetValue(tsDetails.DeckRebar[pgsTypes::drmBottom][pgsTypes::drbIndividual].P);
         (*pTable2)(row2, col2++) << force.SetValue(tsDetails.DeckRebar[pgsTypes::drmBottom][pgsTypes::drbLumpSum].P);

         iter = tsDetails.GirderRebar.begin();
         for ( ; iter != end; iter++ )
         {
            const TIME_STEP_REBAR& tsRebar(*iter);
            Float64 PgirderRebar = 0;
            for ( int i = 0; i < N; i++ )
            {
               PgirderRebar += tsRebar.Pi[i];
            }
            (*pTable2)(row2,col2++) << force.SetValue(PgirderRebar);
         }

         for ( int i = 0; i < 3; i++ )
         {
            pgsTypes::StrandType strandType = pgsTypes::StrandType(i);
#if defined LUMP_STRANDS
            (*pTable2)(row2,col2++) << force.SetValue(tsDetails.Strands[strandType].P);
#else
            StrandIndexType nStrands = pStrandGeom->GetStrandCount(segmentKey,strandType);
            for ( StrandIndexType strandIdx = 0; strandIdx < nStrands; strandIdx++ )
            {
               const TIME_STEP_STRAND& strand(tsDetails.Strands[strandType][strandIdx]);
               (*pTable2)(row2,col2) << force.SetValue(strand.P) << rptNewLine;
            } // next strand
            col2 += 1;
#endif
         } // next strand type

         for (DuctIndexType ductIdx = 0; ductIdx < nSegmentDucts; ductIdx++)
         {
            const TIME_STEP_STRAND& tendon(tsDetails.SegmentTendons[ductIdx]);
            (*pTable2)(row2, col2) << force.SetValue(tendon.P) << rptNewLine;
         }
         col2 += 1;

         for ( DuctIndexType ductIdx = 0; ductIdx < nGirderDucts; ductIdx++ )
         {
            const TIME_STEP_STRAND& tendon(tsDetails.GirderTendons[ductIdx]);
            (*pTable2)(row2,col2) << force.SetValue(tendon.P) << rptNewLine;
         }
         col2 += 1;

         for ( int i = 0; i < 3; i++ )
         {
            pgsTypes::StrandType strandType = pgsTypes::StrandType(i);
#if defined LUMP_STRANDS
            const TIME_STEP_STRAND& strand(tsDetails.Strands[strandType]);
            (*pTable2)(row2,col2++) << stress.SetValue(strand.dfpe);
            (*pTable2)(row2,col2++) << stress.SetValue(strand.fpe);
#else
            StrandIndexType nStrands = pStrandGeom->GetStrandCount(segmentKey,strandType);
            for ( StrandIndexType strandIdx = 0; strandIdx < nStrands; strandIdx++ )
            {
               const TIME_STEP_STRAND& strand(tsDetails.Strands[strandType][strandIdx]);
               (*pTable2)(row2,col2+0) << stress.SetValue(strand.dfpe) << rptNewLine;
               (*pTable2)(row2,col2+1) << stress.SetValue(strand.fpe) << rptNewLine;
            } // next strand
            col2 += 2;
#endif
         } // next strand type

         for (DuctIndexType ductIdx = 0; ductIdx < nSegmentDucts; ductIdx++)
         {
            const TIME_STEP_STRAND& tendon(tsDetails.SegmentTendons[ductIdx]);
            (*pTable2)(row2, col2 + 0) << stress.SetValue(tendon.dfpe) << rptNewLine;
            (*pTable2)(row2, col2 + 1) << stress.SetValue(tendon.fpe) << rptNewLine;
         }
         col2 += 2;

         for ( DuctIndexType ductIdx = 0; ductIdx < nGirderDucts; ductIdx++ )
         {
            const TIME_STEP_STRAND& tendon(tsDetails.GirderTendons[ductIdx]);
            (*pTable2)(row2,col2+0) << stress.SetValue(tendon.dfpe) << rptNewLine;
            (*pTable2)(row2,col2+1) << stress.SetValue(tendon.fpe)  << rptNewLine;
         }
         col2 += 2;

         Float64 fBotGirder = 0;
         Float64 fTopGirder = 0;
         Float64 fTopDeck   = 0;
         for ( int i = 0; i < N; i++ )
         {
            fBotGirder += tsDetails.Girder.f[pgsTypes::BottomFace][i][rtCumulative];
            fTopGirder += tsDetails.Girder.f[pgsTypes::TopFace][i][rtCumulative];
            fTopDeck   += tsDetails.Deck.f[pgsTypes::TopFace][i][rtCumulative];
         }

         (*pTable2)(row2,col2++) << stress.SetValue(fBotGirder);
         (*pTable2)(row2,col2++) << stress.SetValue(fTopGirder);
         (*pTable2)(row2,col2++) << stress.SetValue(fTopDeck);

         (*pTable2)(row2,col2++) << force.SetValue(tsDetails.dPext);
         (*pTable2)(row2,col2++) << force.SetValue(tsDetails.Pext);
         (*pTable2)(row2,col2++) << force.SetValue(tsDetails.dPint);
         (*pTable2)(row2,col2++) << force.SetValue(tsDetails.Pint);

         (*pTable2)(row2,col2++) << moment.SetValue(tsDetails.dMext);
         (*pTable2)(row2,col2++) << moment.SetValue(tsDetails.Mext);
         (*pTable2)(row2,col2++) << moment.SetValue(tsDetails.dMint);
         (*pTable2)(row2,col2++) << moment.SetValue(tsDetails.Mint);
      } // next interval


      // After last interval, report total loss
      const TIME_STEP_DETAILS& tsDetails(pDetails->TimeStepDetails[nIntervals-1]);
      col2 = 0;
      (*pTable2)(row2,col2++) << _T("Total");

      (*pTable2)(row2,col2++) << _T(""); //area.SetValue(tsDetails.Atr);
      (*pTable2)(row2,col2++) << _T(""); //ecc.SetValue(tsDetails.Ytr);
      (*pTable2)(row2,col2++) << _T(""); //momI.SetValue(tsDetails.Itr);
      (*pTable2)(row2,col2++) << _T(""); //force.SetValue(tsDetails.P);
      (*pTable2)(row2,col2++) << _T(""); //moment.SetValue(tsDetails.M);

      (*pTable2)(row2,col2++) << _T(""); //area.SetValue(tsDetails.Girder.An);
      (*pTable2)(row2,col2++) << _T(""); //ecc.SetValue(tsDetails.Girder.Ynt);
      (*pTable2)(row2,col2++) << _T(""); //ecc.SetValue(tsDetails.Girder.Ynb);
      (*pTable2)(row2,col2++) << _T(""); //momI.SetValue(tsDetails.Girder.In);
      (*pTable2)(row2,col2++) << _T(""); //tsDetails.Girder.e;
      (*pTable2)(row2,col2++) << _T(""); //curvature.SetValue(tsDetails.Girder.r);
      (*pTable2)(row2,col2++) << _T(""); //force.SetValue(tsDetails.Girder.PrCreep);
      (*pTable2)(row2,col2++) << _T(""); //moment.SetValue(tsDetails.Girder.MrCreep);
      (*pTable2)(row2,col2++) << _T(""); //tsDetails.Girder.esi;
      (*pTable2)(row2,col2++) << _T(""); //force.SetValue(tsDetails.Girder.PrShrinkage);

      (*pTable2)(row2,col2++) << _T(""); //area.SetValue(tsDetails.Deck.An);
      (*pTable2)(row2,col2++) << _T(""); //ecc.SetValue(tsDetails.Deck.Ynt);
      (*pTable2)(row2,col2++) << _T(""); //ecc.SetValue(tsDetails.Deck.Ynb);
      (*pTable2)(row2,col2++) << _T(""); //momI.SetValue(tsDetails.Deck.In);
      (*pTable2)(row2,col2++) << _T(""); //tsDetails.Deck.e;
      (*pTable2)(row2,col2++) << _T(""); //curvature.SetValue(tsDetails.Deck.r);
      (*pTable2)(row2,col2++) << _T(""); //force.SetValue(tsDetails.Deck.PrCreep);
      (*pTable2)(row2,col2++) << _T(""); //moment.SetValue(tsDetails.Deck.MrCreep);
      (*pTable2)(row2,col2++) << _T(""); //tsDetails.Deck.esi;
      (*pTable2)(row2,col2++) << _T(""); //force.SetValue(tsDetails.Deck.PrShrinkage);

      (*pTable2)(row2, col2++) << _T(""); //area.SetValue(tsDetails.DeckRebar[pgsTypes::drmTop][pgsTypes::drbIndividual].As);
      (*pTable2)(row2, col2++) << _T(""); //ecc.SetValue(tsDetails.DeckRebar[pgsTypes::drmTop][pgsTypes::drbIndividual].Ys);
      (*pTable2)(row2, col2++) << _T(""); //area.SetValue(tsDetails.DeckRebar[pgsTypes::drmTop][pgsTypes::drbLumpSum].As);
      (*pTable2)(row2, col2++) << _T(""); //ecc.SetValue(tsDetails.DeckRebar[pgsTypes::drmTop][pgsTypes::drbLumpSum].Ys);

      (*pTable2)(row2, col2++) << _T(""); //area.SetValue(tsDetails.DeckRebar[pgsTypes::drmBottom][pgsTypes::drbIndividual].As);
      (*pTable2)(row2, col2++) << _T(""); //ecc.SetValue(tsDetails.DeckRebar[pgsTypes::drmBottom][pgsTypes::drbIndividual].Ys);
      (*pTable2)(row2, col2++) << _T(""); //area.SetValue(tsDetails.DeckRebar[pgsTypes::drmBottom][pgsTypes::drbLumpSum].As);
      (*pTable2)(row2, col2++) << _T(""); //ecc.SetValue(tsDetails.DeckRebar[pgsTypes::drmBottom][pgsTypes::drbLumpSum].Ys);

      std::vector<TIME_STEP_REBAR>::const_iterator iter(tsDetails.GirderRebar.begin());
      std::vector<TIME_STEP_REBAR>::const_iterator end(tsDetails.GirderRebar.end());
      for ( ; iter != end; iter++ )
      {
         const TIME_STEP_REBAR& tsRebar(*iter);

         (*pTable2)(row2,col2++) << _T(""); //area.SetValue(tsRebar.As);
         (*pTable2)(row2,col2++) << _T(""); //ecc.SetValue(tsRebar.Ys);
      }

      for ( int i = 0; i < 3; i++ )
      {
         pgsTypes::StrandType strandType = pgsTypes::StrandType(i);
         (*pTable2)(row2,col2++) << _T(""); //area.SetValue(tsDetails.Strands[strandType].As);
         (*pTable2)(row2,col2++) << _T(""); //ecc.SetValue(tsDetails.Strands[strandType].Ys);
         (*pTable2)(row2,col2++) << _T(""); //stress.SetValue(tsDetails.Strands[strandType].fr);
         (*pTable2)(row2,col2++) << _T(""); //tsDetails.Strands[strandType].er;
         (*pTable2)(row2,col2++) << _T(""); //force.SetValue(tsDetails.Strands[strandType].Pr);
      }

      for (DuctIndexType ductIdx = 0; ductIdx < nSegmentDucts; ductIdx++)
      {
         (*pTable2)(row2, col2 + 0) << _T(""); //area.SetValue(tsDetails.SegmentTendons[ductIdx].As) << rptNewLine;
         (*pTable2)(row2, col2 + 1) << _T(""); //ecc.SetValue(tsDetails.SegmentTendons[ductIdx].Ys) << rptNewLine;
         (*pTable2)(row2, col2 + 2) << _T(""); //stress.SetValue(tsDetails.SegmentTendons[ductIdx].fr) << rptNewLine;
         (*pTable2)(row2, col2 + 3) << _T(""); //tsDetails.SegmentTendons[ductIdx].er << rptNewLine;
         (*pTable2)(row2, col2 + 4) << _T(""); //force.SetValue(tsDetails.SegmentTendons[ductIdx].Pr) << rptNewLine;
      }
      col2 += 5;

      for (DuctIndexType ductIdx = 0; ductIdx < nGirderDucts; ductIdx++)
      {
         (*pTable2)(row2, col2 + 0) << _T(""); //area.SetValue(tsDetails.GirderTendons[ductIdx].As) << rptNewLine;
         (*pTable2)(row2, col2 + 1) << _T(""); //ecc.SetValue(tsDetails.GirderTendons[ductIdx].Ys) << rptNewLine;
         (*pTable2)(row2, col2 + 2) << _T(""); //stress.SetValue(tsDetails.GirderTendons[ductIdx].fr) << rptNewLine;
         (*pTable2)(row2, col2 + 3) << _T(""); //tsDetails.GirderTendons[ductIdx].er << rptNewLine;
         (*pTable2)(row2, col2 + 4) << _T(""); //force.SetValue(tsDetails.GirderTendons[ductIdx].Pr) << rptNewLine;
      }
      col2 += 5;

      (*pTable2)(row2,col2++) << _T(""); //(*pTable2)(row2,col2++) << force.SetValue( tsDetails.Pr[TIMESTEP_CR] );
      (*pTable2)(row2,col2++) << _T(""); //(*pTable2)(row2,col2++) << force.SetValue( tsDetails.Pr[TIMESTEP_SH] );
      (*pTable2)(row2,col2++) << _T(""); //(*pTable2)(row2,col2++) << force.SetValue( tsDetails.Pr[TIMESTEP_RE] );
      (*pTable2)(row2,col2++) << _T(""); //(*pTable2)(row2,col2++) << moment.SetValue(tsDetails.Mr[TIMESTEP_CR] );
      (*pTable2)(row2,col2++) << _T(""); //(*pTable2)(row2,col2++) << moment.SetValue(tsDetails.Mr[TIMESTEP_SH] );
      (*pTable2)(row2,col2++) << _T(""); //(*pTable2)(row2,col2++) << moment.SetValue(tsDetails.Mr[TIMESTEP_RE] );

      (*pTable2)(row2,col2++) << _T(""); //(*pTable2)(row2,col2++) << tsDetails.e[TIMESTEP_CR][pgsTypes::Ahead];
      (*pTable2)(row2,col2++) << _T(""); //(*pTable2)(row2,col2++) << tsDetails.e[TIMESTEP_SH][pgsTypes::Ahead];
      (*pTable2)(row2,col2++) << _T(""); //(*pTable2)(row2,col2++) << tsDetails.e[TIMESTEP_RE][pgsTypes::Ahead];
      (*pTable2)(row2,col2++) << _T(""); //(*pTable2)(row2,col2++) << curvature.SetValue(tsDetails.r[TIMESTEP_CR][pgsTypes::Ahead]);
      (*pTable2)(row2,col2++) << _T(""); //(*pTable2)(row2,col2++) << curvature.SetValue(tsDetails.r[TIMESTEP_SH][pgsTypes::Ahead]);
      (*pTable2)(row2,col2++) << _T(""); //(*pTable2)(row2,col2++) << curvature.SetValue(tsDetails.r[TIMESTEP_RE][pgsTypes::Ahead]);

      (*pTable2)(row2,col2++) << _T(""); //(*pTable2)(row2,col2++) << force.SetValue( tsDetails.Pre[TIMESTEP_CR] );
      (*pTable2)(row2,col2++) << _T(""); //(*pTable2)(row2,col2++) << force.SetValue( tsDetails.Pre[TIMESTEP_SH] );
      (*pTable2)(row2,col2++) << _T(""); //(*pTable2)(row2,col2++) << force.SetValue( tsDetails.Pre[TIMESTEP_RE] );
      (*pTable2)(row2,col2++) << _T(""); //(*pTable2)(row2,col2++) << moment.SetValue(tsDetails.Mre[TIMESTEP_CR]);
      (*pTable2)(row2,col2++) << _T(""); //(*pTable2)(row2,col2++) << moment.SetValue(tsDetails.Mre[TIMESTEP_SH]);
      (*pTable2)(row2,col2++) << _T(""); //(*pTable2)(row2,col2++) << moment.SetValue(tsDetails.Mre[TIMESTEP_RE]);

      (*pTable2)(row2,col2++) << _T(""); //tsDetails.er;
      (*pTable2)(row2,col2++) << _T(""); //WBFL::Units::ConvertFromSysUnits(tsDetails.rr,pDisplayUnits->GetCurvatureUnit().UnitOfMeasure);

      (*pTable2)(row2,col2++) << _T(""); //force.SetValue(tsDetails.Girder.dP);
      (*pTable2)(row2,col2++) << _T(""); //moment.SetValue(tsDetails.Girder.dM);
      (*pTable2)(row2,col2++) << _T(""); //force.SetValue(tsDetails.Deck.dP);
      (*pTable2)(row2,col2++) << _T(""); //moment.SetValue(tsDetails.Deck.dM);
      (*pTable2)(row2, col2++) << _T(""); //force.SetValue(tsDetails.DeckRebar[pgsTypes::drmTop][pgsTypes::drbIndividual].dP);
      (*pTable2)(row2, col2++) << _T(""); //force.SetValue(tsDetails.DeckRebar[pgsTypes::drmTop][pgsTypes::drbLumpSum].dP);
      (*pTable2)(row2, col2++) << _T(""); //force.SetValue(tsDetails.DeckRebar[pgsTypes::drmBottom][pgsTypes::drbIndividual].dP);
      (*pTable2)(row2, col2++) << _T(""); //force.SetValue(tsDetails.DeckRebar[pgsTypes::drmBottom][pgsTypes::drbLumpSum].dP);

      iter = tsDetails.GirderRebar.begin();
      for ( ; iter != end; iter++ )
      {
         const TIME_STEP_REBAR& tsRebar(*iter);
         (*pTable2)(row2,col2++) << _T(""); //force.SetValue(tsRebar.dP);
      }

      for ( int i = 0; i < 3; i++ )
      {
         (*pTable2)(row2,col2++) << _T(""); //force.SetValue(tsDetails.Strands[i].dP);
      }

      for (DuctIndexType ductIdx = 0; ductIdx < nSegmentDucts; ductIdx++)
      {
         (*pTable2)(row2, col2) << _T(""); //force.SetValue(tsDetails.SegmentTendons[ductIdx].dP) << rptNewLine;
      }
      col2 += 1;

      for (DuctIndexType ductIdx = 0; ductIdx < nGirderDucts; ductIdx++)
      {
         (*pTable2)(row2, col2) << _T(""); //force.SetValue(tsDetails.GirderTendons[ductIdx].dP) << rptNewLine;
      }
      col2 += 1;

      (*pTable2)(row2,col2++) << _T(""); //force.SetValue(tsDetails.Girder.P);
      (*pTable2)(row2,col2++) << _T(""); //moment.SetValue(tsDetails.Girder.M);
      (*pTable2)(row2,col2++) << _T(""); //force.SetValue(tsDetails.Deck.P);
      (*pTable2)(row2,col2++) << _T(""); //moment.SetValue(tsDetails.Deck.M);
      (*pTable2)(row2, col2++) << _T(""); //force.SetValue(tsDetails.DeckRebar[pgsTypes::drmTop][pgsTypes::drbIndividual].P);
      (*pTable2)(row2, col2++) << _T(""); //force.SetValue(tsDetails.DeckRebar[pgsTypes::drmTop][pgsTypes::drbLumpSum].P);
      (*pTable2)(row2, col2++) << _T(""); //force.SetValue(tsDetails.DeckRebar[pgsTypes::drmBottom][pgsTypes::drbIndividual].P);
      (*pTable2)(row2, col2++) << _T(""); //force.SetValue(tsDetails.DeckRebar[pgsTypes::drmBottom][pgsTypes::drbLumpSum].P);

      iter = tsDetails.GirderRebar.begin();
      for ( ; iter != end; iter++ )
      {
         const TIME_STEP_REBAR& tsRebar(*iter);
         (*pTable2)(row2,col2++) << _T(""); //force.SetValue(tsRebar.P);
      }

      for ( int i = 0; i < 3; i++ )
      {
         (*pTable2)(row2,col2++) << _T(""); //force.SetValue(tsDetails.Strands[i].P);
      }

      for (DuctIndexType ductIdx = 0; ductIdx < nSegmentDucts; ductIdx++)
      {
         (*pTable2)(row2, col2) << _T(""); //force.SetValue(tsDetails.SegmentTendons[ductIdx].P) << rptNewLine;
      }
      col2 += 1;

      for (DuctIndexType ductIdx = 0; ductIdx < nGirderDucts; ductIdx++)
      {
         (*pTable2)(row2, col2) << _T(""); //force.SetValue(tsDetails.GirderTendons[ductIdx].P) << rptNewLine;
      }
      col2 += 1;

      for ( int i = 0; i < 3; i++ )
      {
         pgsTypes::StrandType strandType = pgsTypes::StrandType(i);
#if defined LUMP_STRANDS
         (*pTable2)(row2,col2++) << stress.SetValue(tsDetails.Strands[strandType].loss);
         (*pTable2)(row2,col2++) << _T(""); //stress.SetValue(tsDetails.Strands[strandType].fpe);
#else
         StrandIndexType nStrands = pStrandGeom->GetStrandCount(segmentKey,strandType);
         for ( StrandIndexType strandIdx = 0; strandIdx < nStrands; strandIdx++ )
         {
            const TIME_STEP_STRAND& strand(tsDetails.Strands[strandType][strandIdx]);
            (*pTable2)(row2,col2+0) << stress.SetValue(strand.loss) << rptNewLine;
            (*pTable2)(row2,col2+1) << _T("") << rptNewLine; //stress.SetValue(tsDetails.Strands[i].fpe);
         }
         col2 += 2;
#endif
      }

      for (DuctIndexType ductIdx = 0; ductIdx < nSegmentDucts; ductIdx++)
      {
         const TIME_STEP_STRAND& tendon(tsDetails.SegmentTendons[ductIdx]);
         (*pTable2)(row2, col2 + 0) << stress.SetValue(tendon.loss) << rptNewLine;
         (*pTable2)(row2, col2 + 1) << _T(""); //stress.SetValue(tsDetails.SegmentTendons[ductIdx].fpe)  << rptNewLine;
      }
      col2 += 2;

      for ( DuctIndexType ductIdx = 0; ductIdx < nGirderDucts; ductIdx++ )
      {
         const TIME_STEP_STRAND& tendon(tsDetails.GirderTendons[ductIdx]);
         (*pTable2)(row2,col2+0) << stress.SetValue(tendon.loss) << rptNewLine;
         (*pTable2)(row2,col2+1) << _T(""); //stress.SetValue(tsDetails.GirderTendons[ductIdx].fpe)  << rptNewLine;
      }
      col2 += 2;

      (*pTable2)(row2,col2++) << _T(""); //stress.SetValue(tsDetails.Stress[pgsTypes::BottomGirder]);
      (*pTable2)(row2,col2++) << _T(""); //stress.SetValue(tsDetails.Stress[pgsTypes::TopGirder]);
      (*pTable2)(row2,col2++) << _T(""); //stress.SetValue(tsDetails.Stress[pgsTypes::TopDeck]);

      (*pTable2)(row2,col2++) << _T(""); //force.SetValue(tsDetails.dPext);
      (*pTable2)(row2,col2++) << _T(""); //force.SetValue(tsDetails.Pext);
      (*pTable2)(row2,col2++) << _T(""); //force.SetValue(tsDetails.dPint);
      (*pTable2)(row2,col2++) << _T(""); //force.SetValue(tsDetails.Pint);

      (*pTable2)(row2,col2++) << _T(""); //moment.SetValue(tsDetails.dMext);
      (*pTable2)(row2,col2++) << _T(""); //moment.SetValue(tsDetails.Mext);
      (*pTable2)(row2,col2++) << _T(""); //moment.SetValue(tsDetails.dMint);
      (*pTable2)(row2,col2++) << _T(""); //moment.SetValue(tsDetails.Mint);
   } // next poi


   rptRcTable* pTable3 = rptStyleManager::CreateDefaultTable(1+6*nIntervals,_T("Initial Strain Analysis"));
   *pPara << pTable3 << rptNewLine;
   pTable3->SetNumberOfHeaderRows(2);
   ColumnIndexType col3 = 0;
   pTable3->SetRowSpan(0,col3,2);
   (*pTable3)(0,col3++) << _T("POI");
   for ( IntervalIndexType intervalIdx = 0; intervalIdx < nIntervals; intervalIdx++ )
   {
      pTable3->SetColumnSpan(0,col3,6);
      (*pTable3)(0,col3) << _T("Interval ") << LABEL_INTERVAL(intervalIdx);
      (*pTable3)(1,col3++) << COLHDR(Sub2(_T("P"),_T("r")), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());
      (*pTable3)(1,col3++) << COLHDR(Sub2(_T("M"),_T("r")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit());
      (*pTable3)(1,col3++) << Sub2(symbol(epsilon),_T("i"));
      (*pTable3)(1,col3++) << COLHDR(Sub2(symbol(phi),_T("i")), rptPerLengthUnitTag, pDisplayUnits->GetCurvatureUnit());
      (*pTable3)(1,col3++) << COLHDR(Sub2(_T("P"),_T("re")), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());
      (*pTable3)(1,col3++) << COLHDR(Sub2(_T("M"),_T("re")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit());
   }

   RowIndexType row3 = pTable3->GetNumberOfHeaderRows();
   for(const pgsPointOfInterest& poi : vPoi)
   {
      col3 = 0;

      const LOSSDETAILS* pDetails = pLosses->GetLossDetails(poi,INVALID_INDEX);

      (*pTable3)(row3,col3++) << location.SetValue(POI_SPAN,poi);

      for ( IntervalIndexType intervalIdx = 0; intervalIdx < nIntervals; intervalIdx++ )
      {
         const TIME_STEP_DETAILS& tsDetails(pDetails->TimeStepDetails[intervalIdx]);
         (*pTable3)(row3,col3++) << force.SetValue(tsDetails.Pr[TIMESTEP_CR]+tsDetails.Pr[TIMESTEP_SH]+tsDetails.Pr[TIMESTEP_RE]);
         (*pTable3)(row3,col3++) << moment.SetValue(tsDetails.Mr[TIMESTEP_CR]+tsDetails.Mr[TIMESTEP_SH]+tsDetails.Mr[TIMESTEP_RE]);
         (*pTable3)(row3, col3++) << tsDetails.e[TIMESTEP_CR] + tsDetails.e[TIMESTEP_SH] + tsDetails.e[TIMESTEP_RE];
         (*pTable3)(row3,col3++) << curvature.SetValue(tsDetails.r[TIMESTEP_CR]+tsDetails.r[TIMESTEP_SH]+tsDetails.r[TIMESTEP_RE]);
         (*pTable3)(row3,col3++) << force.SetValue(tsDetails.dPi[pgsTypes::pftCreep]+tsDetails.dPi[pgsTypes::pftShrinkage]+tsDetails.dPi[pgsTypes::pftRelaxation]);
         (*pTable3)(row3,col3++) << moment.SetValue(tsDetails.dMi[pgsTypes::pftCreep]+tsDetails.dMi[pgsTypes::pftShrinkage]+tsDetails.dMi[pgsTypes::pftRelaxation]);
      }

      row3++;
   }

   return pChapter;
}

std::unique_ptr<WBFL::Reporting::ChapterBuilder> CTimeStepParametersChapterBuilder::Clone() const
{
   return std::make_unique<CTimeStepParametersChapterBuilder>();
}
