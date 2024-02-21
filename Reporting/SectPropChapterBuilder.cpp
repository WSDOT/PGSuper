///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2024  Washington State Department of Transportation
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
#include <Reporting\SectPropChapterBuilder.h>
#include <Reporting\SectPropTable.h>
#include <Reporting\SectPropTable2.h>
#include <Reporting\NetGirderPropertiesTable.h>
#include <Reporting\ColumnPropertiesTable.h>

#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\ReportPointOfInterest.h>

#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\Intervals.h>
#include <IFace\DocumentType.h>
#include <IFace/Limits.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CSectPropChapterBuilder
****************************************************************************/



////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CSectPropChapterBuilder::CSectPropChapterBuilder(bool bSelect,bool simplifiedVersion) :
CPGSuperChapterBuilder(bSelect),
m_SimplifiedVersion(simplifiedVersion)
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CSectPropChapterBuilder::GetName() const
{
   return TEXT("Section Properties");
}

rptChapter* CSectPropChapterBuilder::Build(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,Uint16 level) const
{
   auto pGdrRptSpec = std::dynamic_pointer_cast<const CGirderReportSpecification>(pRptSpec);
   auto pGdrLineRptSpec = std::dynamic_pointer_cast<const CGirderLineReportSpecification>(pRptSpec);
   auto pMultiGirderRptSpec = std::dynamic_pointer_cast<const CMultiGirderReportSpecification>(pRptSpec);

   CComPtr<IBroker> pBroker;
   std::vector<CGirderKey> girderKeys;
   if ( pGdrRptSpec )
   {
      pGdrRptSpec->GetBroker(&pBroker);
      girderKeys.push_back(pGdrRptSpec->GetGirderKey());
   }
   else if ( pGdrLineRptSpec)
   {
      pGdrLineRptSpec->GetBroker(&pBroker);
      GET_IFACE2(pBroker,IBridge,pBridge);

      CGirderKey girderKey = pGdrLineRptSpec->GetGirderKey();
      pBridge->GetGirderline(girderKey, &girderKeys);
   }
   else if ( pMultiGirderRptSpec)
   {
      pMultiGirderRptSpec->GetBroker(&pBroker);
      girderKeys = pMultiGirderRptSpec->GetGirderKeys();
   }
   else
   {
      ATLASSERT(false); // not expecting a different kind of report spec
   }

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   rptParagraph* pPara = new rptParagraph();
   *pChapter << pPara;

   GET_IFACE2(pBroker, IEAFDisplayUnits,   pDisplayUnits);
   GET_IFACE2(pBroker, IGirder,            pGirder);
   GET_IFACE2(pBroker, IBridge,            pBridge);
   GET_IFACE2_NOCHECK(pBroker, IDocumentType,      pDocType);
   GET_IFACE2_NOCHECK(pBroker, IIntervals,         pIntervals); // not always used... depends on if the segment is prismatic

   INIT_UV_PROTOTYPE( rptAreaUnitValue,           l2,   pDisplayUnits->GetAreaUnit(),            false );
   INIT_UV_PROTOTYPE( rptLength4UnitValue,        ui,   pDisplayUnits->GetMomentOfInertiaUnit(), true );
   INIT_UV_PROTOTYPE( rptForceLength2UnitValue,   uei,  pDisplayUnits->GetStiffnessUnit(),       true );
   INIT_UV_PROTOTYPE( rptStressUnitValue,         modE, pDisplayUnits->GetModEUnit(),            true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,         dim,  pDisplayUnits->GetComponentDimUnit(),    true );
   INIT_UV_PROTOTYPE( rptForcePerLengthUnitValue, fpl,  pDisplayUnits->GetForcePerLengthUnit(),  true );


   *pPara << rptNewLine;

   bool bComposite = pBridge->IsCompositeDeck();
   if (pGirder->HasStructuralLongitudinalJoints())
   {
      bComposite = true;
   }


   GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   if (!m_SimplifiedVersion)
   {
      // Write out traffic barrier properties
      GET_IFACE2(pBroker, IBarriers,          pBarriers);

      pPara = new rptParagraph();
      *pChapter << pPara;
      l2.ShowUnitTag( true );
      (*pPara) << _T("Left Traffic Barrier Area = ") << l2.SetValue( pBarriers->GetAtb(pgsTypes::tboLeft) ) << rptNewLine;
      (*pPara) << _T("Left Traffic Barrier ") << Sub2(_T("I"),_T("yy")) << _T(" = ") << ui.SetValue( pBarriers->GetItb(pgsTypes::tboLeft) ) << rptNewLine;
      (*pPara) << _T("Left Traffic Barrier ") << Sub2(_T("Y"),_T("b")) << _T(" = ") << dim.SetValue( pBarriers->GetYbtb(pgsTypes::tboLeft) ) << rptNewLine;

      const CRailingSystem* pLeftRailing = pBridgeDesc->GetLeftRailingSystem();

      if ( pLeftRailing->GetExteriorRailing()->GetWeightMethod() == TrafficBarrierEntry::Compute )
      {
         (*pPara) << _T("Left Exterior Traffic Barrier Weight (computed from area) = ")<<fpl.SetValue( pBarriers->GetExteriorBarrierWeight(pgsTypes::tboLeft) ) << rptNewLine;
      }
      else
      {
         (*pPara) << _T("Left Exterior Traffic Barrier Weight = ")<<fpl.SetValue( pBarriers->GetExteriorBarrierWeight(pgsTypes::tboLeft) ) << rptNewLine;
      }

      (*pPara) << _T("Distance from CG of Left Exterior Traffic Barrier to Left Edge of Deck = ")<<dim.SetValue( pBarriers->GetExteriorBarrierCgToDeckEdge(pgsTypes::tboLeft) ) << rptNewLine;

      if ( pLeftRailing->bUseInteriorRailing)
      {
         if ( pLeftRailing->GetInteriorRailing()->GetWeightMethod() == TrafficBarrierEntry::Compute )
         {
            (*pPara) << _T("Left Interior Traffic Barrier Weight (computed from area) = ")<<fpl.SetValue( pBarriers->GetInteriorBarrierWeight(pgsTypes::tboLeft) ) << rptNewLine;
         }
         else
         {
            (*pPara) << _T("Left Interior Traffic Barrier Weight = ")<<fpl.SetValue( pBarriers->GetInteriorBarrierWeight(pgsTypes::tboLeft) ) << rptNewLine;
         }

         (*pPara) << _T("Distance from CG of Left Interior Traffic Barrier to Left Edge of Deck = ")<<dim.SetValue( pBarriers->GetInteriorBarrierCgToDeckEdge(pgsTypes::tboLeft) ) << rptNewLine;
      }

      if ( pLeftRailing->bUseSidewalk)
      {
         (*pPara) << _T("Left Sidewalk Nominal Width = ")<<dim.SetValue( pLeftRailing->Width ) << rptNewLine;
         Float64 intEdge, extEdge;
         pBarriers->GetSidewalkPedLoadEdges(pgsTypes::tboLeft, &intEdge, &extEdge);
         Float64 dist = fabs(intEdge+extEdge )/2.0;
         (*pPara) << _T("Distance from Nominal Centerline of Left Sidewalk to Left Edge of Deck = ") << dim.SetValue( dist ) << rptNewLine;
         pBarriers->GetSidewalkDeadLoadEdges(pgsTypes::tboLeft, &intEdge, &extEdge);
         (*pPara) << _T("Left Sidewalk Actual Width =  ") << dim.SetValue( fabs(intEdge-extEdge) ) << rptNewLine;
         (*pPara) << _T("Left Sidewalk Face   Depth =  ") << dim.SetValue( pLeftRailing->RightDepth ) << rptNewLine;
         (*pPara) << _T("Left Sidewalk Back   Depth =  ") << dim.SetValue( pLeftRailing->LeftDepth ) << rptNewLine;
         (*pPara) << _T("Left Sidewalk Weight = ") << fpl.SetValue( pBarriers->GetSidewalkWeight(pgsTypes::tboLeft) ) << rptNewLine;
      }

      *pPara << rptNewLine;

      pPara = new rptParagraph();
      *pChapter << pPara;
      l2.ShowUnitTag( true );
      (*pPara) << _T("Right Traffic Barrier Area = ") << l2.SetValue( pBarriers->GetAtb(pgsTypes::tboRight) ) << rptNewLine;
      (*pPara) << _T("Right Traffic Barrier ") << Sub2(_T("I"),_T("yy")) << _T(" = ") << ui.SetValue( pBarriers->GetItb(pgsTypes::tboRight) ) << rptNewLine;
      (*pPara) << _T("Right Traffic Barrier ") << Sub2(_T("Y"),_T("b")) << _T(" = ") << dim.SetValue( pBarriers->GetYbtb(pgsTypes::tboRight) ) << rptNewLine;

      const CRailingSystem* pRightRailing = pBridgeDesc->GetRightRailingSystem();

      if ( pRightRailing->GetExteriorRailing()->GetWeightMethod() == TrafficBarrierEntry::Compute )
      {
         (*pPara) << _T("Right Traffic Barrier Weight (computed from area) = ") << fpl.SetValue( pBarriers->GetExteriorBarrierWeight(pgsTypes::tboRight) ) << rptNewLine;
      }
      else
      {
         (*pPara) << _T("Right Traffic Barrier Weight = ") << fpl.SetValue( pBarriers->GetExteriorBarrierWeight(pgsTypes::tboRight) ) << rptNewLine;
      }

      (*pPara) << _T("Distance from CG of Right Exterior Traffic Barrier to Right Edge of Deck = ") << dim.SetValue( pBarriers->GetExteriorBarrierCgToDeckEdge(pgsTypes::tboRight) ) << rptNewLine;

      if ( pRightRailing->bUseInteriorRailing)
      {
         if ( pRightRailing->GetInteriorRailing()->GetWeightMethod() == TrafficBarrierEntry::Compute )
         {
            (*pPara) << _T("Right Interior Traffic Barrier Weight (computed from area) = ") << fpl.SetValue( pBarriers->GetInteriorBarrierWeight(pgsTypes::tboRight) ) << rptNewLine;
         }
         else
         {
            (*pPara) << _T("Right Interior Traffic Barrier Weight = ") << fpl.SetValue( pBarriers->GetInteriorBarrierWeight(pgsTypes::tboRight) ) << rptNewLine;
         }

         (*pPara) << _T("Distance from CG of Right Interior Traffic Barrier to Right Edge of Deck = ") << dim.SetValue( pBarriers->GetInteriorBarrierCgToDeckEdge(pgsTypes::tboRight) ) << rptNewLine;
      }

      if ( pRightRailing->bUseSidewalk)
      {
         (*pPara) << _T("Right Sidewalk Nominal Width = ") << dim.SetValue( pRightRailing->Width ) << rptNewLine;
         Float64 intEdge, extEdge;
         pBarriers->GetSidewalkPedLoadEdges(pgsTypes::tboRight, &intEdge, &extEdge);
         Float64 dist = fabs(intEdge+extEdge )/2.0;
         (*pPara) << _T("Distance from Nominal Centerline of Right Sidewalk to Right Edge of Deck = ")<<dim.SetValue( dist ) << rptNewLine;
         pBarriers->GetSidewalkDeadLoadEdges(pgsTypes::tboRight, &intEdge, &extEdge);
         (*pPara) << _T("Right Sidewalk Actual Width = ") << dim.SetValue( fabs(intEdge-extEdge) ) << rptNewLine;
         (*pPara) << _T("Right Sidewalk Face Depth = ") << dim.SetValue( pRightRailing->RightDepth ) << rptNewLine;
         (*pPara) << _T("Right Sidewalk Back Depth = ") << dim.SetValue( pRightRailing->LeftDepth ) << rptNewLine;
         (*pPara) << _T("Right Sidewalk Weight = ") << fpl.SetValue( pBarriers->GetSidewalkWeight(pgsTypes::tboRight) ) << rptNewLine;
      }

      *pPara << rptNewLine;

      if ( IsStructuralDeck(pBridge->GetDeckType()) )
      {
         GET_IFACE2(pBroker, IMaterials,         pMaterial);
        (*pPara) << _T("Deck   ") << RPT_EC << _T(" = ") << modE.SetValue( pMaterial->GetDeckEc28() ) << rptNewLine;
        (*pPara) << rptNewLine;
      }

      if (pGirder->HasStructuralLongitudinalJoints())
      {
         GET_IFACE2(pBroker, IMaterials, pMaterial);
         (*pPara) << _T("Longitudinal Joint ") << RPT_EC << _T(" = ") << modE.SetValue(pMaterial->GetLongitudinalJointEc28()) << rptNewLine;
         (*pPara) << rptNewLine;
      }
   }


   if ( !m_SimplifiedVersion )
   {
      GET_IFACE2(pBroker, IPointOfInterest,   pPoi);
      GET_IFACE2(pBroker, ISectionProperties, pSectProp);
      for (const auto& thisGirderKey : girderKeys)
      {
         const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(thisGirderKey.groupIndex);
         PierIndexType startPierIdx = pGroup->GetPierIndex(pgsTypes::metStart);
         PierIndexType endPierIdx   = pGroup->GetPierIndex(pgsTypes::metEnd);

         SpanIndexType startSpanIdx = (SpanIndexType)startPierIdx;
         SpanIndexType endSpanIdx   = (SpanIndexType)(endPierIdx-1);

         for ( SpanIndexType spanIdx = startSpanIdx; spanIdx <= endSpanIdx; spanIdx++ )
         {
            Float64 startStation = pBridge->GetPierStation((PierIndexType)spanIdx);
            Float64 endStation   = pBridge->GetPierStation((PierIndexType)spanIdx+1);
            Float64 XbStart = pPoi->ConvertRouteToBridgeLineCoordinate(startStation);
            Float64 XbEnd   = pPoi->ConvertRouteToBridgeLineCoordinate(endStation);
            Float64 Xb = 0.5*(XbStart + XbEnd);

            Float64 EIxx, EIyy, EIxy;
            pSectProp->GetBridgeStiffness(Xb, &EIxx, &EIyy, &EIxy);
            (*pPara) << _T("Bending Stiffness of Entire Bridge Section at center of Span ") << LABEL_SPAN(spanIdx) << rptNewLine;
            (*pPara) << Sub2(_T("EI"),_T("xx")) << _T(" = ") << uei.SetValue( EIxx ) << _T(" (used to compute Live Load Deflections per LRFD 3.6.1.3.2)") << rptNewLine;
            (*pPara) << Sub2(_T("EI"),_T("yy")) << _T(" = ") << uei.SetValue( EIyy ) << rptNewLine;
            (*pPara) << Sub2(_T("EI"),_T("xy")) << _T(" = ") << uei.SetValue( EIxy ) << rptNewLine;
            *pPara << rptNewLine;
         }
      }
   }

   IntervalIndexType lastIntervalIdx = pIntervals->GetIntervalCount() - 1;

   // a little fake out here... if we have a no deck bridge and there is a sacrifical depth to
   // be worn off the girder themselves, the bridge site stage 3 properties are different from
   // the rest of the properties. This is the same as if there is a composite deck and the bridge
   // site stage 3 properties are different because there is a composite deck in the section
   if (pBridge->GetDeckType() == pgsTypes::sdtNone && 0 < pBridge->GetSacrificalDepth())
   {
      bComposite = true;
   }

   for (const auto& thisGirderKey : girderKeys)
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(thisGirderKey.groupIndex);
      const CSplicedGirderData* pSplicedGirder = pGroup->GetGirder(thisGirderKey.girderIndex);
      GirderIDType gdrID = pSplicedGirder->GetID();

      SegmentIndexType nSegments = pSplicedGirder->GetSegmentCount();
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         CSegmentKey thisSegmentKey(thisGirderKey,segIdx);

         IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(thisSegmentKey);

         if (!m_SimplifiedVersion && 1 < nSegments)
         {
            pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
            *pChapter << pPara;
            pPara->SetName(SEGMENT_LABEL(thisSegmentKey));
            *pPara << pPara->GetName() << rptNewLine;
         }

         pPara = new rptParagraph();
         *pChapter << pPara;

         bool bIsPrismatic_CastingYard = pGirder->IsPrismatic(releaseIntervalIdx,thisSegmentKey);
         bool bIsPrismatic_Final       = pGirder->IsPrismatic(lastIntervalIdx,thisSegmentKey);

         if ( bIsPrismatic_CastingYard && bIsPrismatic_Final )
         {
            // simple table
            rptRcTable* pTable = CSectionPropertiesTable().Build(pBroker,thisSegmentKey,bComposite,pDisplayUnits);
            *pPara << pTable << rptNewLine;
         }
         else if ( bIsPrismatic_CastingYard && !bIsPrismatic_Final )
         {
            // simple table for bare girder (don't report composite)
            rptRcTable* pTable = CSectionPropertiesTable().Build(pBroker,thisSegmentKey,false,pDisplayUnits);
            *pPara << pTable << rptNewLine;

            if ( bComposite )
            {
               // there is a deck so we have composite, non-prismatic results
               pTable = CSectionPropertiesTable2().Build(pBroker,pgsTypes::sptGross,thisSegmentKey,lastIntervalIdx,pDisplayUnits);
               *pPara << pTable << rptNewLine;
            }
         }
         else if ( !bIsPrismatic_CastingYard && !bIsPrismatic_Final )
         {
            GET_IFACE2(pBroker, ISectionProperties, pSectProp);
            GET_IFACE2(pBroker,ILossParameters,pLossParams);
            //if ( pLossParams->GetLossMethod() == PrestressLossCriteria::LossMethodType::TIME_STEP )
            //{
            //   IntervalIndexType nIntervals = pIntervals->GetIntervalCount();
            //   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(thisSegmentKey);
            //   for ( IntervalIndexType intervalIdx = releaseIntervalIdx; intervalIdx < nIntervals; intervalIdx++ )
            //   {
            //      rptRcTable* pTable = CSectionPropertiesTable2().Build(pBroker,pgsTypes::sptTransformed,thisSegmentKey,intervalIdx,pDisplayUnits);
            //      *pPara << pTable << rptNewLine;
            //   }
            //}
            //else
            {
               GET_IFACE2(pBroker, IStressCheck, pStressCheck);
               std::vector<IntervalIndexType> vIntervals = pStressCheck->GetStressCheckIntervals(thisSegmentKey);
               vIntervals.push_back(pIntervals->GetLiveLoadInterval());
               std::sort(vIntervals.begin(),vIntervals.end());
               vIntervals.erase(std::unique(vIntervals.begin(),vIntervals.end()),vIntervals.end());
               IntervalIndexType lastCompositeDeckIntervalIdx = pIntervals->GetLastCompositeDeckInterval();
               for (const auto& intervalIdx : vIntervals)
               {
                  pgsTypes::SectionPropertyType spType1 = (pSectProp->GetSectionPropertiesMode() == pgsTypes::spmGross ? pgsTypes::sptGross : pgsTypes::sptTransformedNoncomposite);
                  pgsTypes::SectionPropertyType spType2 = (pSectProp->GetSectionPropertiesMode() == pgsTypes::spmGross ? pgsTypes::sptGross : pgsTypes::sptTransformed);

                  rptRcTable* pTable = CSectionPropertiesTable2().Build(pBroker,spType1,thisSegmentKey,intervalIdx,pDisplayUnits);
                  *pPara << pTable << rptNewLine;

                  if (lastCompositeDeckIntervalIdx <= intervalIdx && pSectProp->GetSectionPropertiesMode() == pgsTypes::spmTransformed )
                  {
                     rptRcTable* pTable = CSectionPropertiesTable2().Build(pBroker,spType2,thisSegmentKey,intervalIdx,pDisplayUnits);
                     *pPara << pTable << rptNewLine;
                  }
               }
            }

            if ( pLossParams->GetLossMethod() == PrestressLossCriteria::LossMethodType::TIME_STEP )
            {
               pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
               *pChapter << pPara;
               (*pPara) << _T("Net Section Properties") << rptNewLine;

               pPara = new rptParagraph;
               *pChapter << pPara;

               GET_IFACE2(pBroker, IStressCheck, pStressCheck);
               std::vector<IntervalIndexType> vIntervals = pStressCheck->GetStressCheckIntervals(thisSegmentKey);
               vIntervals.push_back(pIntervals->GetLiveLoadInterval());
               std::sort(vIntervals.begin(),vIntervals.end());
               vIntervals.erase(std::unique(vIntervals.begin(),vIntervals.end()),vIntervals.end());
               for (const auto& intervalIdx : vIntervals)
               {
                  rptRcTable* pTable = CNetGirderPropertiesTable().Build(pBroker,thisSegmentKey,intervalIdx,pDisplayUnits);
                  *pPara << pTable << rptNewLine;
               }
            }
         }
         else if ( !bIsPrismatic_CastingYard && bIsPrismatic_Final )
         {
            ATLASSERT(false); // this is an impossible case
         }
      } // segIdx
   } // grpIdx

   pPara = new rptParagraph;
   *pChapter << pPara;

   (*pPara) << CColumnPropertiesTable().Build(pBroker,pDisplayUnits) << rptNewLine;

   return pChapter;
}

std::unique_ptr<WBFL::Reporting::ChapterBuilder> CSectPropChapterBuilder::Clone() const
{
   return std::make_unique<CSectPropChapterBuilder> ();
}
