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
#include <Reporting\CastingYardRebarRequirementChapterBuilder.h>
#include <Reporting\ReportNotes.h>
#include <WBFLGenericBridgeTools\GeneralSectionDetailsTable.h>

#include <PgsExt\ReportPointOfInterest.h>
#include <PgsExt\GirderArtifact.h>

#include <IFace\Artifact.h>
#include <IFace\Bridge.h>
#include <IFace\Intervals.h>
#include <IFace\Allowables.h>
#include <IFace\DocumentType.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/****************************************************************************
CLASS
   CCastingYardRebarRequirementChapterBuilder
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CCastingYardRebarRequirementChapterBuilder::CCastingYardRebarRequirementChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CCastingYardRebarRequirementChapterBuilder::GetName() const
{
   return TEXT("Reinforcement Requirements for Tension Limits");
}

rptChapter* CCastingYardRebarRequirementChapterBuilder::Build(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,Uint16 level) const
{
   auto pGirderRptSpec = std::dynamic_pointer_cast<const CGirderReportSpecification>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pGirderRptSpec->GetBroker(&pBroker);
   const CGirderKey& girderKey(pGirderRptSpec->GetGirderKey());

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;
   *pPara << _T("Minimum amount of bonded reinforcement sufficient to resist the tensile force in the concrete ") << WBFL::LRFD::LrfdCw8th(_T("[5.9.4][C5.9.4.1.2]"),_T("[5.9.2.3][C5.9.2.3.1b]")) << rptNewLine;

   GET_IFACE2(pBroker,IBridge,pBridge);

   GET_IFACE2(pBroker,IAllowableConcreteStress,pAllowStress);

   bool bSimpleTable = true;
   if (pBridge->HasAsymmetricGirders() || pBridge->HasAsymmetricPrestressing())
   {
      bSimpleTable = false;
   }


   // Report for Girder Segments
   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      if ( 1 < nSegments )
      {
         pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
         *pChapter << pPara;
         *pPara << _T("Segment ") << LABEL_SEGMENT(segIdx) << rptNewLine;
      }

      CSegmentKey segmentKey(girderKey,segIdx);

      IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
      pPara = new rptParagraph;
      *pChapter << pPara;
      BuildTable(pBroker,pPara,segmentKey,releaseIntervalIdx,bSimpleTable);

      if ( pAllowStress->CheckTemporaryStresses() )
      {
         GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);
         if ( 0 < pStrandGeom->GetStrandCount(segmentKey,pgsTypes::Temporary) )
         {
            IntervalIndexType tsRemovalIntervalIdx = pIntervals->GetTemporaryStrandRemovalInterval(segmentKey);
            pPara = new rptParagraph;
            *pChapter << pPara;
            BuildTable(pBroker,pPara,segmentKey,tsRemovalIntervalIdx,bSimpleTable);
         }
      }
   } // next segment

   // Report for Closure Joints
   // need to report for all spec-check intervals after a closure joint
   // is composite with the girder
   GET_IFACE2_NOCHECK(pBroker,IPointOfInterest,pIPoi); // only used if there is more than one segment in the girder

   GET_IFACE2(pBroker, IStressCheck, pStressCheck);
   std::vector<IntervalIndexType> vSpecCheckIntervals = pStressCheck->GetStressCheckIntervals(girderKey);
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments-1; segIdx++ )
   {
      CClosureKey closureKey(girderKey,segIdx);
      IntervalIndexType compositeClosureJointIntervalIdx = pIntervals->GetCompositeClosureJointInterval(closureKey);

      PoiList vPoi;
      pIPoi->GetPointsOfInterest(closureKey, POI_CLOSURE, &vPoi);
      ATLASSERT(vPoi.size() == 1);
      pgsPointOfInterest poi(vPoi.front());
      ATLASSERT(poi.GetID() != INVALID_ID);
      ATLASSERT(poi.HasAttribute(POI_CLOSURE));

      pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
      *pChapter << pPara;
      *pPara << _T("Closure Joint Between Segment ") << LABEL_SEGMENT(segIdx) << _T(" and Segment ") << LABEL_SEGMENT(segIdx+1) << rptNewLine;

      std::vector<IntervalIndexType>::iterator iter(vSpecCheckIntervals.begin());
      std::vector<IntervalIndexType>::iterator end(vSpecCheckIntervals.end());
      for ( ; iter != end; iter++ )
      {
         IntervalIndexType intervalIdx = *iter;
         if ( compositeClosureJointIntervalIdx <= intervalIdx )
         {
            // allowable tension stresses are checked in the Service I limit state before live load is applied and in the
            // Service III limit state after live load is applied
            pgsTypes::LimitState limitState = (liveLoadIntervalIdx <= intervalIdx ? pgsTypes::ServiceIII : pgsTypes::ServiceI);

            bool bIsApplicable = pAllowStress->IsStressCheckApplicable(girderKey,StressCheckTask(intervalIdx,limitState,pgsTypes::Tension));
            if ( bIsApplicable )
            {
               pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
               *pChapter << pPara;
               *pPara << _T("Interval ") << LABEL_INTERVAL(intervalIdx) << _T(" : ") << pIntervals->GetDescription(intervalIdx) << rptNewLine;

               pPara = new rptParagraph;
               *pChapter << pPara;

               BuildTable(pBroker,pPara,poi,intervalIdx,true/*simple table*/);
            }
         }
      }
   }

   // Report for Deck
   // need to report for all intervals when post-tensioning occurs after the
   // deck is composite
   IntervalIndexType lastCompositeDeckIntervalIdx = pIntervals->GetLastCompositeDeckInterval();
   GET_IFACE2(pBroker,IGirderTendonGeometry,pTendonGeom);
   DuctIndexType nDucts = pTendonGeom->GetDuctCount(girderKey);
   std::set<IntervalIndexType> vIntervals;
   for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
   {
      IntervalIndexType intervalIdx = pIntervals->GetStressGirderTendonInterval(girderKey,ductIdx);
      if (lastCompositeDeckIntervalIdx <= intervalIdx )
      {
         vIntervals.insert(intervalIdx);
      }
   }

   if ( 0 < vIntervals.size() )
   {
      pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
      *pChapter << pPara;
      *pPara << _T("Deck") << rptNewLine;

      std::set<IntervalIndexType>::iterator iter(vIntervals.begin());
      std::set<IntervalIndexType>::iterator end(vIntervals.end());
      for ( ; iter != end; iter++ )
      {
         IntervalIndexType intervalIdx = *iter;

         pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
         *pChapter << pPara;
         *pPara << _T("Interval ") << LABEL_INTERVAL(intervalIdx) << _T(" : ") << pIntervals->GetDescription(intervalIdx) << rptNewLine;

         pPara = new rptParagraph;
         *pChapter << pPara;

         BuildTable(pBroker,pPara,girderKey,intervalIdx,true/*simple table*/);
      }
   }

   return pChapter;
}


std::unique_ptr<WBFL::Reporting::ChapterBuilder>CCastingYardRebarRequirementChapterBuilder::Clone() const
{
   return std::make_unique<CCastingYardRebarRequirementChapterBuilder>();
}

void CCastingYardRebarRequirementChapterBuilder::BuildTable(IBroker* pBroker,rptParagraph* pPara,const CSegmentKey& segmentKey,IntervalIndexType intervalIdx, bool bSimpleTable) const
{
   pgsTypes::StressLocation botLocation = pgsTypes::BottomGirder;
   pgsTypes::StressLocation topLocation = pgsTypes::TopGirder;

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   rptRcTable* pTable = CreateTable(pBroker,segmentKey,bSimpleTable,topLocation,botLocation,intervalIdx,pDisplayUnits);
   *pPara << pTable << rptNewLine;

   pgsPointOfInterest poi;
   poi.SetSegmentKey(segmentKey);
   FillTable(pBroker,pTable,bSimpleTable,topLocation,botLocation,intervalIdx,poi);

   if (bSimpleTable)
   {
      (*pPara) << Sub2(_T("Y"), _T("na")) << _T(", the location of the neutral axis, is measured from the top of the non-composite girder") << rptNewLine;
   }
   else
   {
      (*pPara) << _T("The neutral axis is defined by its location with respect to the top center of the girder (") << Sub2(_T("Y"), _T("na")) << _T(") and its slope (Slope NA)") << rptNewLine;
   }
   (*pPara) << Super(_T("*")) << _T(" to be considered sufficient, reinforcement must be fully developed and lie within the tension area of the section") << rptNewLine;
   (*pPara) << _T("** minimum area of sufficiently bonded reinforcement needed to use the alternative tensile stress limit") << rptNewLine;
}

void CCastingYardRebarRequirementChapterBuilder::BuildTable(IBroker* pBroker,rptParagraph* pPara,const pgsPointOfInterest& poi,IntervalIndexType intervalIdx, bool bSimpleTable) const
{
   pgsTypes::StressLocation botLocation = pgsTypes::BottomGirder;
   pgsTypes::StressLocation topLocation = pgsTypes::TopGirder;

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   rptRcTable* pTable = CreateTable(pBroker, poi.GetSegmentKey(), bSimpleTable, topLocation,botLocation,intervalIdx,pDisplayUnits);
   *pPara << pTable << rptNewLine;

   FillTable(pBroker,pTable,bSimpleTable, topLocation,botLocation,intervalIdx,poi);

   if (bSimpleTable)
   {
      (*pPara) << Sub2(_T("Y"), _T("na")) << _T(", the location of the neutral axis, is measured from the top of the closure joint based on the non-composite girder section") << rptNewLine;
   }
   else
   {
      (*pPara) << _T("The neutral axis is defined by its location with respect to the top center of the closure joint based on the non-composite girder section (") << Sub2(_T("Y"), _T("na")) << _T(") and its slope (Slope NA)") << rptNewLine;
   }
   (*pPara) << Super(_T("*")) << _T(" to be considered sufficient, reinforcement must be fully developed and lie within the tension area of the section") << rptNewLine;
   (*pPara) << _T("** minimum area of sufficiently bonded reinforcement needed to use the alternative tensile stress limit") << rptNewLine;
}

void CCastingYardRebarRequirementChapterBuilder::BuildTable(IBroker* pBroker,rptParagraph* pPara,const CGirderKey& girderKey,IntervalIndexType intervalIdx, bool bSimpleTable) const
{
   pgsTypes::StressLocation botLocation = pgsTypes::BottomDeck;
   pgsTypes::StressLocation topLocation = pgsTypes::TopDeck;

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   rptRcTable* pTable = CreateTable(pBroker, girderKey, bSimpleTable,topLocation,botLocation,intervalIdx,pDisplayUnits);
   *pPara << pTable << rptNewLine;

   CSegmentKey segmentKey(girderKey.groupIndex,girderKey.girderIndex,ALL_SEGMENTS);
   pgsPointOfInterest poi;
   poi.SetSegmentKey(segmentKey);

   FillTable(pBroker,pTable,bSimpleTable,topLocation,botLocation,intervalIdx,poi);

   if (bSimpleTable)
   {
      (*pPara) << Sub2(_T("Y"), _T("na")) << _T(", the location of the neutral axis, is measured from the top of the non-composite girder") << rptNewLine;
   }
   else
   {
      (*pPara) << _T("The neutral axis is defined by its location with respect to the top center of the girder (") << Sub2(_T("Y"), _T("na")) << _T(") and its slope (Slope NA)") << rptNewLine;
   }
   (*pPara) << Super(_T("*")) << _T(" to be considered sufficient, reinforcement must be fully developed and lie within the tension area of the section") << rptNewLine;
   (*pPara) << _T("** minimum area of sufficiently bonded reinforcement needed to use the alternative tensile stress limit") << rptNewLine;
}

rptRcTable* CCastingYardRebarRequirementChapterBuilder::CreateTable(IBroker* pBroker, const CGirderKey& girderKey, bool bSimpleTable, pgsTypes::StressLocation topLocation, pgsTypes::StressLocation botLocation, IntervalIndexType intervalIdx, IEAFDisplayUnits* pDisplayUnits) const
{
   GET_IFACE2(pBroker, IIntervals, pIntervals);
   CString strTitle;
   strTitle.Format(_T("Reinforcement required for tension stress limit, Interval %d - %s, [%s]"), LABEL_INTERVAL(intervalIdx), pIntervals->GetDescription(intervalIdx).c_str(), WBFL::LRFD::LrfdCw8th(_T("C5.9.4.1.2"), _T("C5.9.2.3.1b")));

   ColumnIndexType nColumns = (bSimpleTable ? 8 : 19);
   nColumns++; // for tension force details
   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(nColumns, strTitle);

   pTable->SetNumberOfHeaderRows(1);

   if (girderKey.groupIndex == ALL_GROUPS)
   {
      pTable->SetColumnStyle(0, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
      pTable->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   ColumnIndexType col = 0;
   (*pTable)(0, col++) << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*pTable)(0, col++) << COLHDR(Sub2(_T("Y"), _T("na")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());

   if (bSimpleTable)
   {
      (*pTable)(0, col++) << COLHDR(RPT_STRESS(_T("t")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
      (*pTable)(0, col++) << COLHDR(RPT_STRESS(_T("b")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
   }
   else
   {
      pTable->SetNumberOfHeaderRows(2);

      col = 0;

      // location
      pTable->SetRowSpan(0, col++, 2);

      // Yna
      pTable->SetRowSpan(0, col++, 2);

      pTable->SetRowSpan(0, col, 2);
      (*pTable)(0, col++) << _T("Slope NA");

      pTable->SetColumnSpan(0, col, 3);
      (*pTable)(0, col) << _T("Top Left");
      (*pTable)(1, col++) << COLHDR(Sub2(_T("x"),_T("tl")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      (*pTable)(1, col++) << COLHDR(Sub2(_T("y"), _T("tl")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      (*pTable)(1, col++) << COLHDR(RPT_STRESS(_T("tl")), rptStressUnitTag, pDisplayUnits->GetStressUnit());

      pTable->SetColumnSpan(0, col, 3);
      (*pTable)(0, col) << _T("Top Right");
      (*pTable)(1, col++) << COLHDR(Sub2(_T("x"), _T("tr")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      (*pTable)(1, col++) << COLHDR(Sub2(_T("y"), _T("tr")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      (*pTable)(1, col++) << COLHDR(RPT_STRESS(_T("tr")), rptStressUnitTag, pDisplayUnits->GetStressUnit());

      pTable->SetColumnSpan(0, col, 3);
      (*pTable)(0, col) << _T("Bottom Left");
      (*pTable)(1, col++) << COLHDR(Sub2(_T("x"), _T("bl")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      (*pTable)(1, col++) << COLHDR(Sub2(_T("y"), _T("bl")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      (*pTable)(1, col++) << COLHDR(RPT_STRESS(_T("bl")), rptStressUnitTag, pDisplayUnits->GetStressUnit());

      pTable->SetColumnSpan(0, col, 3);
      (*pTable)(0, col) << _T("Bottom Right");
      (*pTable)(1, col++) << COLHDR(Sub2(_T("x"), _T("br")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      (*pTable)(1, col++) << COLHDR(Sub2(_T("y"), _T("br")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      (*pTable)(1, col++) << COLHDR(RPT_STRESS(_T("br")), rptStressUnitTag, pDisplayUnits->GetStressUnit());

      ColumnIndexType colOffset = 0;
      pTable->SetRowSpan(0, col + colOffset, 2); colOffset++; // Tension Force Details
      pTable->SetRowSpan(0, col + colOffset, 2); colOffset++; // At
      pTable->SetRowSpan(0, col + colOffset, 2); colOffset++; // T
      pTable->SetRowSpan(0, col + colOffset, 2); colOffset++; // As Provided
      pTable->SetRowSpan(0, col + colOffset, 2); colOffset++; // As Required
   }

   (*pTable)(0, col++) << _T("Tension Force Details");

   (*pTable)(0, col++) << COLHDR(Sub2(_T("A"), _T("t")), rptAreaUnitTag, pDisplayUnits->GetAreaUnit());
   (*pTable)(0, col++) << COLHDR(_T("T"), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());
   (*pTable)(0, col++) << COLHDR(Sub2(_T("A"), _T("s")) << rptNewLine << _T("Provided") << Super(_T("*")), rptAreaUnitTag, pDisplayUnits->GetAreaUnit());
   (*pTable)(0, col++) << COLHDR(Sub2(_T("A"), _T("s")) << rptNewLine << _T("Required") << Super(_T("**")), rptAreaUnitTag, pDisplayUnits->GetAreaUnit());

   return pTable;
}

void CCastingYardRebarRequirementChapterBuilder::FillTable(IBroker* pBroker,rptRcTable* pTable,bool bSimpleTable,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) const
{
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();
   IntervalIndexType overlayIntervalIdx = pIntervals->GetOverlayInterval();

   // allowable tension stresses are checked in the Service I limit state before live load is applied and in the
   // Service III limit state after live load is applied
   pgsTypes::LimitState limitState = (liveLoadIntervalIdx <= intervalIdx ? pgsTypes::ServiceIII : pgsTypes::ServiceI);

   INIT_UV_PROTOTYPE( rptPointOfInterest, location,       pDisplayUnits->GetSpanLengthUnit(), false );
   location.IncludeSpanAndGirder(segmentKey.segmentIndex == ALL_SEGMENTS || poi.GetID() != INVALID_ID);

   INIT_SCALAR_PROTOTYPE(rptRcScalar, scalar, pDisplayUnits->GetScalarFormat());
   INIT_UV_PROTOTYPE( rptForceUnitValue,  force,  pDisplayUnits->GetShearUnit(),        false );
   INIT_UV_PROTOTYPE( rptAreaUnitValue,   area,   pDisplayUnits->GetAreaUnit(),         false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim,    pDisplayUnits->GetComponentDimUnit(), false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(),       false );

   RowIndexType row = pTable->GetNumberOfHeaderRows();

   GET_IFACE2(pBroker,IBridge,pBridge);
   SegmentIndexType nSegments = pBridge->GetSegmentCount(segmentKey);
   SegmentIndexType firstSegIdx = (segmentKey.segmentIndex == ALL_SEGMENTS ? 0 : segmentKey.segmentIndex);
   SegmentIndexType lastSegIdx  = (segmentKey.segmentIndex == ALL_SEGMENTS ? nSegments-1 : firstSegIdx );

   StressCheckTask task(intervalIdx, limitState, pgsTypes::Tension);

   GET_IFACE2(pBroker,IArtifact,pIArtifact);
   for ( SegmentIndexType segIdx = firstSegIdx; segIdx <= lastSegIdx; segIdx++ )
   {
      CSegmentKey thisSegmentKey(segmentKey.groupIndex,segmentKey.girderIndex,segIdx);
      const pgsSegmentArtifact* pSegmentArtifact = pIArtifact->GetSegmentArtifact(thisSegmentKey);

      IndexType nArtifacts;
      if ( poi.GetID() == INVALID_ID )
      {
         // reporting for all vPoi
         nArtifacts = pSegmentArtifact->GetFlexuralStressArtifactCount(task);
      }
      else
      {
         // reporting for a specific POI
         nArtifacts = 1;
      }

      for ( IndexType artifactIdx = 0; artifactIdx < nArtifacts; artifactIdx++ )
      {
         const pgsFlexuralStressArtifact* pArtifact;
         if ( poi.GetID() == INVALID_ID )
         {
            pArtifact = pSegmentArtifact->GetFlexuralStressArtifact( task,artifactIdx );
         }
         else
         {
            pArtifact = pSegmentArtifact->GetFlexuralStressArtifactAtPoi(task,poi.GetID());
         }

         ATLASSERT(pArtifact != nullptr);
         if ( pArtifact == nullptr )
         {
            // safety net just incase we get a nullptr pointer during release builds
            continue;
         }

         const pgsPointOfInterest& thisPoi(pArtifact->GetPointOfInterest());

         IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(thisPoi.GetSegmentKey());

         ColumnIndexType col = 0;

         (*pTable)(row,col++) << location.SetValue( intervalIdx == releaseIntervalIdx ? POI_RELEASED_SEGMENT : POI_ERECTED_SEGMENT, thisPoi );

         const gbtAlternativeTensileStressRequirements& requirements = pArtifact->GetAlternativeTensileStressRequirements(botLocation);

         (*pTable)(row, col++) << dim.SetValue(requirements.Yna);
         if (bSimpleTable)
         {
            Float64 fTop = requirements.pntTopLeft.Z();
            Float64 fBot = requirements.pntBottomLeft.Z();
            (*pTable)(row, col++) << stress.SetValue(fTop);
            (*pTable)(row, col++) << stress.SetValue(fBot);
         }
         else
         {
            (*pTable)(row, col++) << scalar.SetValue(requirements.NAslope);

            (*pTable)(row, col++) << dim.SetValue(requirements.pntTopLeft.X());
            (*pTable)(row, col++) << dim.SetValue(requirements.pntTopLeft.Y());
            (*pTable)(row, col++) << stress.SetValue(requirements.pntTopLeft.Z());

            (*pTable)(row, col++) << dim.SetValue(requirements.pntTopRight.X());
            (*pTable)(row, col++) << dim.SetValue(requirements.pntTopRight.Y());
            (*pTable)(row, col++) << stress.SetValue(requirements.pntTopRight.Z());

            (*pTable)(row, col++) << dim.SetValue(requirements.pntBottomLeft.X());
            (*pTable)(row, col++) << dim.SetValue(requirements.pntBottomLeft.Y());
            (*pTable)(row, col++) << stress.SetValue(requirements.pntBottomLeft.Z());

            (*pTable)(row, col++) << dim.SetValue(requirements.pntBottomRight.X());
            (*pTable)(row, col++) << dim.SetValue(requirements.pntBottomRight.Y());
            (*pTable)(row, col++) << stress.SetValue(requirements.pntBottomRight.Z());
         }

         if (requirements.tensionForceSolution)
         {
            rptRcTable* pDetailsTable = CreateGeneralSectionDetailsTable(requirements.tensionForceSolution, requirements.Ytg, bSimpleTable, pDisplayUnits);
            (*pTable)(row, col++) << pDetailsTable;
         }
         else
         {
            (*pTable)(row, col++) << _T("-");
         }

         (*pTable)(row, col++) << area.SetValue(requirements.AreaTension);
         (*pTable)(row, col++) << force.SetValue(requirements.T);
         (*pTable)(row, col++) << area.SetValue(requirements.AsProvided);
         if (requirements.AsRequired < 0)
         {
            (*pTable)(row, col++) << _T("-");
         }
         else
         {
            (*pTable)(row, col++) << area.SetValue(requirements.AsRequired);
         }

         row++;
      } // next artifact
   } // next segment
}