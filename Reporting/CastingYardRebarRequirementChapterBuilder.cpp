///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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

#include <PgsExt\ReportPointOfInterest.h>
#include <PgsExt\GirderArtifact.h>

#include <IFace\Artifact.h>
#include <IFace\Bridge.h>
#include <IFace\Intervals.h>
#include <IFace\Allowables.h>

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

rptChapter* CCastingYardRebarRequirementChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CGirderReportSpecification* pGirderRptSpec = dynamic_cast<CGirderReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pGirderRptSpec->GetBroker(&pBroker);
   const CGirderKey& girderKey(pGirderRptSpec->GetGirderKey());

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;
   *pPara << _T("Minimum amount of bonded reinforcement sufficent to resist the tensile force in the concrete [5.9.4][C5.9.4.1.2]") << rptNewLine;

   GET_IFACE2(pBroker,IBridge,pBridge);

   GET_IFACE2(pBroker,IAllowableConcreteStress,pAllowStress);

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
      BuildTable(pBroker,pPara,segmentKey,releaseIntervalIdx);

      if ( pAllowStress->CheckTemporaryStresses() )
      {
         GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);
         if ( 0 < pStrandGeom->GetStrandCount(segmentKey,pgsTypes::Temporary) )
         {
            IntervalIndexType tsRemovalIntervalIdx = pIntervals->GetTemporaryStrandRemovalInterval(segmentKey);
            pPara = new rptParagraph;
            *pChapter << pPara;
            BuildTable(pBroker,pPara,segmentKey,tsRemovalIntervalIdx);
         }
      }
   } // next segment

   // Report for Closure Joints
   // need to report for all spec-check intervals after a closure joint
   // is composite with the girder
   GET_IFACE2_NOCHECK(pBroker,IPointOfInterest,pIPoi); // only used if there is more than one segment in the girder
   std::vector<IntervalIndexType> vSpecCheckIntervals = pIntervals->GetSpecCheckIntervals(girderKey);
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments-1; segIdx++ )
   {
      CClosureKey closureKey(girderKey,segIdx);
      IntervalIndexType compositeClosureJointIntervalIdx = pIntervals->GetCompositeClosureJointInterval(closureKey);

      std::vector<pgsPointOfInterest> vPoi(pIPoi->GetPointsOfInterest(closureKey,POI_CLOSURE));
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

            bool bIsApplicable = pAllowStress->IsStressCheckApplicable(girderKey,intervalIdx,limitState,pgsTypes::Tension);
            if ( bIsApplicable )
            {
               pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
               *pChapter << pPara;
               *pPara << _T("Interval ") << LABEL_INTERVAL(intervalIdx) << _T(" : ") << pIntervals->GetDescription(intervalIdx) << rptNewLine;

               pPara = new rptParagraph;
               *pChapter << pPara;

               BuildTable(pBroker,pPara,poi,intervalIdx);
            }
         }
      }
   }

   // Report for Deck
   // need to report for all intervals when post-tensioning occurs after the
   // deck is composite
   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval();
   GET_IFACE2(pBroker,ITendonGeometry,pTendonGeom);
   DuctIndexType nDucts = pTendonGeom->GetDuctCount(girderKey);
   std::set<IntervalIndexType> vIntervals;
   for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
   {
      IntervalIndexType intervalIdx = pIntervals->GetStressTendonInterval(girderKey,ductIdx);
      if ( compositeDeckIntervalIdx <= intervalIdx )
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

         BuildTable(pBroker,pPara,girderKey,intervalIdx);
      }
   }

   return pChapter;
}


CChapterBuilder* CCastingYardRebarRequirementChapterBuilder::Clone() const
{
   return new CCastingYardRebarRequirementChapterBuilder;
}

void CCastingYardRebarRequirementChapterBuilder::BuildTable(IBroker* pBroker,rptParagraph* pPara,const CSegmentKey& segmentKey,IntervalIndexType intervalIdx) const
{
   pgsTypes::StressLocation botLocation = pgsTypes::BottomGirder;
   pgsTypes::StressLocation topLocation = pgsTypes::TopGirder;

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   rptRcTable* pTable = CreateTable(segmentKey,topLocation,botLocation,pDisplayUnits);
   *pPara << pTable << rptNewLine;

   pgsPointOfInterest poi;
   poi.SetSegmentKey(segmentKey);
   FillTable(pBroker,pTable,topLocation,botLocation,intervalIdx,poi);

   *pPara << _T("* Bar areas are ajusted for development, and bars must lie within tension portion of section before they are considered.");
   *pPara << Sub2(_T("Y"),_T("na")) << _T(" is measured from the top of the girder") << rptNewLine;
}

void CCastingYardRebarRequirementChapterBuilder::BuildTable(IBroker* pBroker,rptParagraph* pPara,const pgsPointOfInterest& poi,IntervalIndexType intervalIdx) const
{
   pgsTypes::StressLocation botLocation = pgsTypes::BottomGirder;
   pgsTypes::StressLocation topLocation = pgsTypes::TopGirder;

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   rptRcTable* pTable = CreateTable(poi.GetSegmentKey(),topLocation,botLocation,pDisplayUnits);
   *pPara << pTable << rptNewLine;

   FillTable(pBroker,pTable,topLocation,botLocation,intervalIdx,poi);

   *pPara << _T("* Bar areas are ajusted for development, and bars must lie within tension portion of section before they are considered.");
   *pPara << Sub2(_T("Y"),_T("na")) << _T(" is measured from the top of the closure joint") << rptNewLine;
}

void CCastingYardRebarRequirementChapterBuilder::BuildTable(IBroker* pBroker,rptParagraph* pPara,const CGirderKey& girderKey,IntervalIndexType intervalIdx) const
{
   pgsTypes::StressLocation botLocation = pgsTypes::BottomDeck;
   pgsTypes::StressLocation topLocation = pgsTypes::TopDeck;

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   rptRcTable* pTable = CreateTable(girderKey,topLocation,botLocation,pDisplayUnits);
   *pPara << pTable << rptNewLine;

   CSegmentKey segmentKey(girderKey.groupIndex,girderKey.girderIndex,ALL_SEGMENTS);
   pgsPointOfInterest poi;
   poi.SetSegmentKey(segmentKey);

   FillTable(pBroker,pTable,topLocation,botLocation,intervalIdx,poi);

   *pPara << _T("* Bars must be fully developed and lie within tension portion of section before they are considered.") << rptNewLine;
   *pPara << Sub2(_T("Y"),_T("na")) << _T(" is measured from the top of the non-composite girder") << rptNewLine;
}

rptRcTable* CCastingYardRebarRequirementChapterBuilder::CreateTable(const CGirderKey& girderKey,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,IEAFDisplayUnits* pDisplayUnits) const
{
   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(12,_T("Reinforcement requirements for Tensile Stress Limit [C5.9.4.1.2]"));

   pTable->SetNumberOfHeaderRows(2);

   if ( girderKey.groupIndex == ALL_GROUPS )
   {
      pTable->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
      pTable->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   ColumnIndexType col = 0;
   // build first heading row
   pTable->SetRowSpan(0,col,2);
   (*pTable)(0,col++) << COLHDR(RPT_GDR_END_LOCATION,  rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );

   pTable->SetRowSpan(0,col,2);
   (*pTable)(0,col++) << COLHDR(Sub2(_T("Y"),_T("na")),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );

   pTable->SetColumnSpan(0,col, 5);
   if ( IsGirderStressLocation(topLocation) )
   {
      (*pTable)(0,col++) << _T("Top Girder");
   }
   else
   {
      (*pTable)(0,col++) << _T("Top Deck");
   }

   pTable->SetColumnSpan(0,col, 5);
   if ( IsGirderStressLocation(botLocation) )
   {
      (*pTable)(0,col++) << _T("Bottom Girder");
   }
   else
   {
      (*pTable)(0,col++) << _T("Bottom Deck");
   }

   ColumnIndexType i;
   for ( i = col; i < pTable->GetNumberOfColumns(); i++ )
   {
      pTable->SetColumnSpan(0,i,SKIP_CELL);
   }

   // build second hearing row
   col = 0;
   pTable->SetRowSpan(1,col++,SKIP_CELL);
   pTable->SetRowSpan(1,col++,SKIP_CELL);
   (*pTable)(1,col++) << COLHDR(RPT_STRESS(_T("t")),rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*pTable)(1,col++) << COLHDR(Sub2(_T("A"),_T("t")),rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );
   (*pTable)(1,col++) << COLHDR(_T("T"),rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit() );
   (*pTable)(1,col++) << COLHDR(Sub2(_T("A"),_T("s"))<< rptNewLine << _T("Required"),rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );
   (*pTable)(1,col++) << COLHDR(Sub2(_T("* A"),_T("s"))<< rptNewLine << _T("Provided"),rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );
   (*pTable)(1,col++) << COLHDR(RPT_STRESS(_T("b")),rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*pTable)(1,col++) << COLHDR(Sub2(_T("A"),_T("t")),rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );
   (*pTable)(1,col++) << COLHDR(_T("T"),rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit() );
   (*pTable)(1,col++) << COLHDR(Sub2(_T("A"),_T("s"))<< rptNewLine << _T("Required"),rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );
   (*pTable)(1,col++) << COLHDR(Sub2(_T("* A"),_T("s"))<< rptNewLine << _T("Provided"),rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );

   return pTable;
}

void CCastingYardRebarRequirementChapterBuilder::FillTable(IBroker* pBroker,rptRcTable* pTable,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) const
{
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

   // allowable tension stresses are checked in the Service I limit state before live load is applied and in the
   // Service III limit state after live load is applied
   pgsTypes::LimitState limitState = (liveLoadIntervalIdx <= intervalIdx ? pgsTypes::ServiceIII : pgsTypes::ServiceI);

   INIT_UV_PROTOTYPE( rptPointOfInterest, location,       pDisplayUnits->GetSpanLengthUnit(), false );
   location.IncludeSpanAndGirder(segmentKey.segmentIndex == ALL_SEGMENTS || poi.GetID() != INVALID_ID);

   INIT_UV_PROTOTYPE( rptForceUnitValue,  force,  pDisplayUnits->GetShearUnit(),        false );
   INIT_UV_PROTOTYPE( rptAreaUnitValue,   area,   pDisplayUnits->GetAreaUnit(),         false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim,    pDisplayUnits->GetComponentDimUnit(), false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(),       false );

   RowIndexType row = pTable->GetNumberOfHeaderRows();

   GET_IFACE2(pBroker,IBridge,pBridge);
   SegmentIndexType nSegments = pBridge->GetSegmentCount(segmentKey);
   SegmentIndexType firstSegIdx = (segmentKey.segmentIndex == ALL_SEGMENTS ? 0 : segmentKey.segmentIndex);
   SegmentIndexType lastSegIdx  = (segmentKey.segmentIndex == ALL_SEGMENTS ? nSegments-1 : firstSegIdx );

   GET_IFACE2(pBroker,IArtifact,pIArtifact);
   for ( SegmentIndexType segIdx = firstSegIdx; segIdx <= lastSegIdx; segIdx++ )
   {
      CSegmentKey thisSegmentKey(segmentKey.groupIndex,segmentKey.girderIndex,segIdx);
      const pgsSegmentArtifact* pSegmentArtifact = pIArtifact->GetSegmentArtifact(thisSegmentKey);

      CollectionIndexType nArtifacts;
      if ( poi.GetID() == INVALID_ID )
      {
         // reporting for all vPoi
         nArtifacts = pSegmentArtifact->GetFlexuralStressArtifactCount(intervalIdx,limitState,pgsTypes::Tension);
      }
      else
      {
         // reporting for a specific POI
         nArtifacts = 1;
      }

      for ( CollectionIndexType artifactIdx = 0; artifactIdx < nArtifacts; artifactIdx++ )
      {
         const pgsFlexuralStressArtifact* pArtifact;
         if ( poi.GetID() == INVALID_ID )
         {
            pArtifact = pSegmentArtifact->GetFlexuralStressArtifact( intervalIdx,limitState,pgsTypes::Tension,artifactIdx );
         }
         else
         {
            pArtifact = pSegmentArtifact->GetFlexuralStressArtifactAtPoi(intervalIdx,limitState,pgsTypes::Tension,poi.GetID());
         }

         ATLASSERT(pArtifact != NULL);
         if ( pArtifact == NULL )
         {
            // safety net just incase we get a NULL pointer during release builds
            continue;
         }

         const pgsPointOfInterest& thisPoi(pArtifact->GetPointOfInterest());

         IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(thisPoi.GetSegmentKey());

         (*pTable)(row,0) << location.SetValue( intervalIdx == releaseIntervalIdx ? POI_RELEASED_SEGMENT : POI_ERECTED_SEGMENT, thisPoi );

         Float64 Yna,At,T,AsProvided,AsRequired;
         pArtifact->GetAlternativeTensileStressParameters(botLocation,&Yna,&At,&T,&AsProvided,&AsRequired);

         if (Yna < 0 )
         {
            // Entire section is in compression
            for ( ColumnIndexType ic = 1; ic < pTable->GetNumberOfColumns(); ic++ )
            {
               (*pTable)(row,ic) << RPT_NA;
            }
         }
         else
         {
            // report depth to neutral axis from top of girder... tension usually governs on the top
            // and top rebar is usually measured from the top downwards
            GET_IFACE2(pBroker,ISectionProperties,pSectProp);
            Float64 Hg = pSectProp->GetHg(releaseIntervalIdx,thisPoi);
            Float64 Y = Hg - Yna;

            (*pTable)(row,1) << dim.SetValue(Y);

            // We have a neutral axis. See which side is in tension
            Float64 fTop = pArtifact->GetDemand(topLocation);
            Float64 fBot = pArtifact->GetDemand(botLocation);

            // Half of the table is always n/a. Determine which half to fill
            ColumnIndexType dataStart, dataEnd;
            ColumnIndexType blankStart, blankEnd;
            if (0.0 < fTop)
            {
               dataStart  = 3;
               dataEnd    = 7;
               blankStart = 8;
               blankEnd   = 12;
            }
            else
            {
               dataStart  = 8;
               dataEnd    = 12;
               blankStart = 3;
               blankEnd   = 7;
            }

            // Stresses
            (*pTable)(row,2) << stress.SetValue(fTop);
            (*pTable)(row,7) << stress.SetValue(fBot);

            // Fill in compression columns with n/a's first
            ColumnIndexType col;
            for (col = blankStart; col < blankEnd; col++)
            {
               (*pTable)(row,col) << RPT_NA;
            }

            // Now fill in tension side with data
            col = dataStart;
            (*pTable)(row,col++) << area.SetValue(At);
            (*pTable)(row,col++) << force.SetValue(T);
            (*pTable)(row,col++) << area.SetValue(AsRequired);
            (*pTable)(row,col++) << area.SetValue(AsProvided);
            ATLASSERT(col==dataEnd);
         }

         row++;
      } // next artifact
   } // next segment
}