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
#include <Reporting\PrestressForceChapterBuilder.h>
#include <Reporting\PrestressLossTable.h>

#include <IFace\Bridge.h>
#include <IFace\PrestressForce.h>
#include <IFace\Project.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Intervals.h>
#include <IFace\DocumentType.h>

#include <Material\PsStrand.h>

#include <PgsExt\StrandData.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CPrestressForceChapterBuilder
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CPrestressForceChapterBuilder::CPrestressForceChapterBuilder(bool bRating,bool bSelect) :
CPGSuperChapterBuilder(bSelect), m_bRating(bRating)
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CPrestressForceChapterBuilder::GetName() const
{
   return TEXT("Prestressing Force and Strand Stresses");
}

rptChapter* CPrestressForceChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
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
   else
   {
      pGdrLineRptSpec->GetBroker(&pBroker);
      girderKey = pGdrLineRptSpec->GetGirderKey();
   }

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   // These are the interfaces we are going to be using
   GET_IFACE2(pBroker,IStrandGeometry, pStrandGeom);
   GET_IFACE2(pBroker,IPretensionForce, pPrestressForce ); 
   GET_IFACE2(pBroker,ISegmentData,pSegmentData);
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IPointOfInterest, pPoi);
   GET_IFACE2(pBroker,IIntervals,pIntervals);

   GET_IFACE2(pBroker, ISectionProperties, pSectProps);
   bool bIncludeElasticEffects = (pSectProps->GetSectionPropertiesMode() == pgsTypes::spmGross ? true : false);

   GET_IFACE2(pBroker, IDocumentType, pDocType);
   bool bIsSplicedGirder = (pDocType->IsPGSpliceDocument() ? true : false);

   // Setup some unit-value prototypes
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(),       true );
   INIT_UV_PROTOTYPE( rptAreaUnitValue,   area,   pDisplayUnits->GetAreaUnit(),         true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, len,    pDisplayUnits->GetComponentDimUnit(), true );
   INIT_UV_PROTOTYPE( rptForceUnitValue,  force,  pDisplayUnits->GetGeneralForceUnit(), true );

   std::vector<CGirderKey> vGirderKeys;
   pBridge->GetGirderline(girderKey, &vGirderKeys);
   for(const auto& thisGirderKey : vGirderKeys)
   {
      SegmentIndexType nSegments = pBridge->GetSegmentCount(thisGirderKey);
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         CSegmentKey thisSegmentKey(thisGirderKey,segIdx);

         PoiList vPoi;
         pPoi->GetPointsOfInterest(thisSegmentKey, POI_5L | POI_RELEASED_SEGMENT, &vPoi);
         ATLASSERT(vPoi.size() == 1);
         const pgsPointOfInterest& poiMiddle(vPoi.front());

         IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(thisSegmentKey);
            
         const CStrandData* pStrands = pSegmentData->GetStrandData(thisSegmentKey);

         // Write out what we have for prestressing in this girder
         if ( girderKey.groupIndex == ALL_GROUPS || 1 < nSegments)
         {
            rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
            *pChapter << pPara;

            if (bIsSplicedGirder)
            {
               (*pPara) << _T("Group ") << LABEL_GROUP(thisSegmentKey.groupIndex) << _T( " Girder ") << LABEL_GIRDER(thisSegmentKey.girderIndex) << _T(" Segment ") << LABEL_SEGMENT(thisSegmentKey.segmentIndex) << rptNewLine;
            }
            else
            {
               (*pPara) << _T("Span ") << LABEL_SPAN(thisSegmentKey.groupIndex) << _T(" Girder ") << LABEL_GIRDER(thisSegmentKey.girderIndex) << rptNewLine;
            }
         }

         bool harpedAreStraight = pStrandGeom->GetAreHarpedStrandsForcedStraight(thisSegmentKey);

         rptParagraph* pPara = new rptParagraph;
         *pChapter << pPara;
         StrandIndexType Ns = pStrandGeom->GetStrandCount(thisSegmentKey,pgsTypes::Straight);
         StrandIndexType Nh = pStrandGeom->GetStrandCount(thisSegmentKey,pgsTypes::Harped);
         *pPara << _T("Straight strands: ") << Sub2(_T("N"),_T("s")) << _T(" = ") << Ns << _T(", ") 
            << Sub2(_T("P"),_T("jack")) << _T(" = ") << force.SetValue(pStrandGeom->GetPjack(thisSegmentKey,pgsTypes::Straight)) << _T(", ")
            << RPT_APS << _T(" = ") << area.SetValue(pStrandGeom->GetStrandArea(poiMiddle,releaseIntervalIdx,pgsTypes::Straight)) << rptNewLine;
         *pPara << LABEL_HARP_TYPE(harpedAreStraight) <<_T(" strands: ") << Sub2(_T("N"),_T("h")) << _T(" = ") << Nh << _T(", ") 
            << Sub2(_T("P"),_T("jack")) << _T(" = ") << force.SetValue(pStrandGeom->GetPjack(thisSegmentKey,pgsTypes::Harped)) << _T(", ")
            << RPT_APS << _T(" = ") << area.SetValue(pStrandGeom->GetStrandArea(poiMiddle,releaseIntervalIdx,pgsTypes::Harped)) << rptNewLine;

         if ( 0 < pStrandGeom->GetMaxStrands(thisSegmentKey,pgsTypes::Temporary ) )
         {
            *pPara << _T("Temporary strands: ") << Sub2(_T("N"),_T("t")) << _T(" = ") << pStrandGeom->GetStrandCount(thisSegmentKey,pgsTypes::Temporary) << _T(", ") 
               << Sub2(_T("P"),_T("jack")) << _T(" = ") << force.SetValue(pStrandGeom->GetPjack(thisSegmentKey,pgsTypes::Temporary)) << _T(", ")   
               << RPT_APS << _T(" = ") << area.SetValue(pStrandGeom->GetStrandArea(poiMiddle,releaseIntervalIdx,pgsTypes::Temporary)) << rptNewLine;

            *pPara << rptNewLine;
                  
            switch(pStrands->GetTemporaryStrandUsage())
            {
            case pgsTypes::ttsPretensioned:
               *pPara << _T("Temporary Strands pretensioned with permanent strands") << rptNewLine;
               break;

            case pgsTypes::ttsPTBeforeShipping:
               *pPara << _T("Temporary Strands post-tensioned immedately before shipping") << rptNewLine;
               break;

            case pgsTypes::ttsPTAfterLifting:
               *pPara << _T("Temporary Strands post-tensioned immedately after lifting") << rptNewLine;
               break;

            case pgsTypes::ttsPTBeforeLifting:
               *pPara << _T("Temporary Strands post-tensioned before lifting") << rptNewLine;
               break;
            }

            *pPara << _T("Total permanent strands, N = ") << Ns+Nh << _T(", ") 
               << Sub2(_T("P"),_T("jack")) << _T(" = ") << force.SetValue(pStrandGeom->GetPjack(thisSegmentKey,pgsTypes::Straight)+pStrandGeom->GetPjack(thisSegmentKey,pgsTypes::Harped)) << _T(", ")
               << RPT_APS << _T(" = ") << area.SetValue(pStrandGeom->GetStrandArea(poiMiddle,releaseIntervalIdx,pgsTypes::Permanent)) << rptNewLine;

            *pPara << rptNewLine;

            *pPara << _T("Prestress Transfer Length (Straight) = ") << len.SetValue(pPrestressForce->GetTransferLength(thisSegmentKey, pgsTypes::Straight)) << rptNewLine;
            *pPara << _T("Prestress Transfer Length (Harped) = ") << len.SetValue(pPrestressForce->GetTransferLength(thisSegmentKey, pgsTypes::Harped)) << rptNewLine;
            *pPara << _T("Prestress Transfer Length (Temporary) = ") << len.SetValue( pPrestressForce->GetTransferLength(thisSegmentKey,pgsTypes::Temporary) ) << rptNewLine;
         }
         else
         {
            *pPara << RPT_APS << _T(" = ") << area.SetValue( pStrandGeom->GetStrandArea(poiMiddle,releaseIntervalIdx,pgsTypes::Permanent) )<< rptNewLine;
            *pPara << Sub2(_T("P"),_T("jack")) << _T(" = ") << force.SetValue( pStrandGeom->GetPjack(thisSegmentKey,false)) << rptNewLine;
            *pPara << _T("Prestress Transfer Length (Straight) = ") << len.SetValue(pPrestressForce->GetTransferLength(thisSegmentKey, pgsTypes::Straight)) << rptNewLine;
            *pPara << _T("Prestress Transfer Length (Harped) = ") << len.SetValue(pPrestressForce->GetTransferLength(thisSegmentKey, pgsTypes::Harped)) << rptNewLine;
         }

         // Write out strand forces and stresses at the various stages of prestress loss
         pPara = new rptParagraph;
         *pChapter << pPara;
         *pPara << CPrestressLossTable(bIsSplicedGirder).Build(pBroker,thisSegmentKey,bIncludeElasticEffects,m_bRating,pDisplayUnits) << rptNewLine;

         pPara = new rptParagraph(rptStyleManager::GetFootnoteStyle());
         *pChapter << pPara;
         *pPara << _T("Time-Dependent Effects = change in strand stress due to creep, shrinkage, and relaxation") << rptNewLine;
         if (bIncludeElasticEffects)
         {
            *pPara << _T("Instantaneous Effects = change in strand stress due to elastic shortening and externally applied loads") << rptNewLine;
            *pPara << RPT_FPE << _T(" = ") << RPT_FPJ << _T(" - Time-Dependent Effects - Instantaneous Effects") << rptNewLine;
         }
         else
         {
            *pPara << RPT_FPE << _T(" = ") << RPT_FPJ << _T(" - Time-Dependent Effects") << rptNewLine;
         }
      } // segIdx
   } // gdrIdx

   return pChapter;
}

CChapterBuilder* CPrestressForceChapterBuilder::Clone() const
{
   return new CPrestressForceChapterBuilder(m_bRating,m_bSelect);
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
