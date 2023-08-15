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

// IBeamDistFactorEngineer.cpp : Implementation of CIBeamDistFactorEngineer
#include "stdafx.h"
#include "IBeamDistFactorEngineer.h"
#include <PGSuperException.h>
#include <Units\Convert.h>
#include <PsgLib\TrafficBarrierEntry.h>
#include <PsgLib\SpecLibraryEntry.h>
#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\StatusItem.h>
#include <PgsExt\GirderLabel.h>

#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\DistributionFactors.h>
#include <IFace\StatusCenter.h>
#include <IFace\Intervals.h>
#include <Beams\Helper.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CIBeamFactory
HRESULT CIBeamDistFactorEngineer::FinalConstruct()
{
   return S_OK;
}

void CIBeamDistFactorEngineer::BuildReport(const CGirderKey& girderKey,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits)
{
   // Grab the interfaces that are needed
   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IPointOfInterest,pPoi);
   GET_IFACE(ISectionProperties, pSectProps);
   pgsTypes::SectionPropertyMode spMode = pSectProps->GetSectionPropertiesMode();

   bool bSIUnits = IS_SI_UNITS(pDisplayUnits);
   std::_tstring strImagePath(rptStyleManager::GetImagePath());

   INIT_UV_PROTOTYPE( rptLengthUnitValue,    location, pDisplayUnits->GetSpanLengthUnit(),      true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,    offsetFormatter, pDisplayUnits->GetSpanLengthUnit(),      false );
   INIT_UV_PROTOTYPE( rptAreaUnitValue,      area,     pDisplayUnits->GetAreaUnit(),            true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,    xdim,     pDisplayUnits->GetSpanLengthUnit(),      true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,    xdim2,    pDisplayUnits->GetComponentDimUnit(),    true );
   INIT_UV_PROTOTYPE( rptLength4UnitValue,   inertia,  pDisplayUnits->GetMomentOfInertiaUnit(), true );
   INIT_UV_PROTOTYPE( rptAngleUnitValue,     angle,    pDisplayUnits->GetAngleUnit(),           true );

   INIT_SCALAR_PROTOTYPE(rptRcScalar, scalar, pDisplayUnits->GetScalarFormat());

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CDeckDescription2* pDeck = pBridgeDesc->GetDeckDescription();

   SpanIndexType startSpanIdx = pBridge->GetGirderGroupStartSpan(girderKey.groupIndex);
   SpanIndexType endSpanIdx   = pBridge->GetGirderGroupEndSpan(girderKey.groupIndex);

   GirderIndexType gdrIdx = girderKey.girderIndex;

   for ( SpanIndexType spanIdx = startSpanIdx; spanIdx <= endSpanIdx; spanIdx++ )
   {
      CSpanKey spanKey(spanIdx,gdrIdx);

      SPANDETAILS span_lldf;
      GetSpanDF(spanKey,pgsTypes::StrengthI,USE_CURRENT_FC, &span_lldf);

      PierIndexType pier1 = spanIdx;
      PierIndexType pier2 = spanIdx+1;
      PIERDETAILS pier1_lldf, pier2_lldf;
      GetPierDF(pier1, gdrIdx, pgsTypes::StrengthI, pgsTypes::Ahead, USE_CURRENT_FC, &pier1_lldf);
      GetPierDF(pier2, gdrIdx, pgsTypes::StrengthI, pgsTypes::Back,  USE_CURRENT_FC, &pier2_lldf);

      // do a sanity check to make sure the fundimental values are correct
      ATLASSERT(span_lldf.Method  == pier1_lldf.Method);
      ATLASSERT(span_lldf.Method  == pier2_lldf.Method);
      ATLASSERT(pier1_lldf.Method == pier2_lldf.Method);

      ATLASSERT(span_lldf.bExteriorGirder  == pier1_lldf.bExteriorGirder);
      ATLASSERT(span_lldf.bExteriorGirder  == pier2_lldf.bExteriorGirder);
      ATLASSERT(pier1_lldf.bExteriorGirder == pier2_lldf.bExteriorGirder);

      // determine continuity
      bool bContinuous, bContinuousAtStart, bContinuousAtEnd;
      pBridge->IsContinuousAtPier(pier1,&bContinuous,&bContinuousAtStart);
      pBridge->IsContinuousAtPier(pier2,&bContinuousAtEnd,&bContinuous);

      bool bIntegral, bIntegralAtStart, bIntegralAtEnd;
      pBridge->IsIntegralAtPier(pier1,&bIntegral,&bIntegralAtStart);
      pBridge->IsIntegralAtPier(pier2,&bIntegralAtEnd,&bIntegral);

      // get to work building the report
      rptParagraph* pPara;

      const CSpanData2* pSpan = pBridgeDesc->GetSpan(spanIdx);
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(pSpan);

      pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
      (*pPara) << _T("Method of Computation:")<<rptNewLine;
      (*pChapter) << pPara;
      pPara = new rptParagraph;
      (*pChapter) << pPara;
      
      std::_tstring strGirderName = pGroup->GetGirder(gdrIdx)->GetGirderName();
      (*pPara) << GetComputationDescription(girderKey, 
                                            strGirderName, 
                                            pDeck->GetDeckType(),
                                            pDeck->TransverseConnectivity);

      if (pBridgeDesc->GetDistributionFactorMethod() != pgsTypes::LeverRule)
      {
         pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
         (*pPara) << _T("Longitudinal Stiffness Parameters");
         (*pChapter) << pPara;
         pPara = new rptParagraph;
         (*pChapter) << pPara;
         (*pPara) << _T("Modular ratio: n = ") << scalar.SetValue(span_lldf.n) << rptNewLine;
         if (spMode == pgsTypes::spmTransformed)
         {
            (*pPara) << _T("Gross, non-composite section properties") << rptNewLine;
         }
         (*pPara) << _T("Moment of Inertia: I") << Sub(_T("g")) << _T(" = ") << inertia.SetValue(span_lldf.I) << rptNewLine;
         (*pPara) << _T("Area: A") << Sub(_T("g")) << _T(" = ") << area.SetValue(span_lldf.A) << rptNewLine;
         (*pPara) << _T("Top centroidal distance: Y") << Sub(_T("tg")) << _T(" = ") << xdim2.SetValue(span_lldf.Yt) << rptNewLine;
         (*pPara) << _T("Slab Thickness: t") << Sub(_T("s")) << _T(" = ") << xdim2.SetValue(span_lldf.ts) << rptNewLine;
         if (pSectProps->GetHaunchAnalysisSectionPropertiesType() == pgsTypes::hspZeroHaunch)
         {
            (*pPara) << _T("Distance between CG of slab and girder: ") << rptRcImage(strImagePath + _T("eg.png")) << rptTab << rptRcImage(strImagePath + _T("eg Pic.jpg")) << rptTab
                     << _T("e") << Sub(_T("g")) << _T(" = ") << xdim2.SetValue(span_lldf.eg) << rptNewLine;
         }
         else
         {
            (*pPara) << _T("Distance between CG of slab and girder: ") << rptRcImage(strImagePath + _T("egFillet.png")) << rptTab << rptRcImage(strImagePath + _T("eg Pic.jpg")) << rptTab
                     << _T("e") << Sub(_T("g")) << _T(" = ") << xdim2.SetValue(span_lldf.eg) << rptNewLine;
         }

         (*pPara) << _T("Stiffness Parameter: ") << rptRcImage(strImagePath + _T("Kg.png")) << rptTab
                  << _T("K") << Sub(_T("g")) << _T(" = ") << inertia.SetValue(span_lldf.Kg) << rptNewLine;
      }

      pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
      (*pChapter) << pPara;
      (*pPara) << _T("Distribution Factor Parameters") << rptNewLine;
      pPara = new rptParagraph;
      (*pChapter) << pPara;

      // get poi at controlling location
      pgsPointOfInterest poi = pPoi->ConvertSpanPointToPoi(CSpanKey(spanIdx,girderKey.girderIndex),span_lldf.ControllingLocation);
      const CSegmentKey& segmentKey(poi.GetSegmentKey());

      Float64 station,offset;
      pBridge->GetStationAndOffset(poi,&station, &offset);
      Float64 supp_dist = span_lldf.ControllingLocation - pBridge->GetSegmentStartEndDistance(segmentKey);
      (*pPara) << _T("Deck Width, Girder Spacing and Deck Overhang are measured along a line that is normal to the alignment and passing through a point ") << location.SetValue(supp_dist) << _T(" from the left support along the centerline of girder. ");
      (*pPara) << _T("The measurement line passes through Station ") << rptRcStation(station, &pDisplayUnits->GetStationFormat() ) << _T(" (") << RPT_OFFSET(offset,offsetFormatter) << _T(")") << rptNewLine;
      (*pPara) << _T("Bridge Width: W = ") << xdim.SetValue(span_lldf.W) << rptNewLine;
      (*pPara) << _T("Roadway Width: w = ") << xdim.SetValue(span_lldf.wCurbToCurb) << rptNewLine;
      (*pPara) << _T("Number of Design Lanes: N") << Sub(_T("L")) << _T(" = ") << span_lldf.Nl << rptNewLine;
      (*pPara) << _T("Lane Width: wLane = ") << xdim.SetValue(span_lldf.wLane) << rptNewLine;
      (*pPara) << _T("Number of Girders: N") << Sub(_T("b")) << _T(" = ") << span_lldf.Nb << rptNewLine;
      (*pPara) << _T("Girder Spacing: ") << Sub2(_T("S"),_T("avg")) << _T(" = ") << xdim.SetValue(span_lldf.Savg) << rptNewLine;
      Float64 so = span_lldf.Side==dfLeft ? span_lldf.leftSlabOverhang:span_lldf.rightSlabOverhang;
      (*pPara) << _T("Deck Overhang = ") << xdim.SetValue(so) << rptNewLine;
      Float64 de = span_lldf.Side==dfLeft ? span_lldf.leftCurbOverhang : span_lldf.rightCurbOverhang;
      (*pPara) << _T("Distance from exterior web of exterior girder to curb line: d") << Sub(_T("e")) << _T(" = ") << xdim.SetValue(de) << rptNewLine;
      (*pPara) << _T("Skew Angle at start: ") << symbol(theta) << _T(" = ") << angle.SetValue(fabs(span_lldf.skew1)) << rptNewLine;
      (*pPara) << _T("Skew Angle at end: ") << symbol(theta) << _T(" = ") << angle.SetValue(fabs(span_lldf.skew2)) << rptNewLine;

      GET_IFACE(ISpecification, pSpec);
      GET_IFACE(ILibrary, pLibrary);
      const auto* pSpecEntry = pLibrary->GetSpecEntry(pSpec->GetSpecification().c_str());
      if (pSpecEntry->IgnoreSkewReductionForMoment())
      {
         (*pPara) << _T("Skew reduction for moment distribution factors has been ignored (LRFD 4.6.2.2.2e)") << rptNewLine;
      }

      if ( WBFL::LRFD::LRFDVersionMgr::Version::FourthEditionWith2009Interims <= WBFL::LRFD::LRFDVersionMgr::GetVersion() )
      {
         pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
         (*pPara) << _T("Strength and Service Limit States");
         (*pChapter) << pPara;
         pPara = new rptParagraph;
         (*pChapter) << pPara;
      }

      //////////////////////////////////////////////////////
      // Moments
      //////////////////////////////////////////////////////

      // Distribution factor for exterior girder
      if ( bContinuousAtStart || bIntegralAtStart )
      {
         pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
         (*pChapter) << pPara;
         (*pPara) << _T("Distribution Factor for Negative Moment over Pier ") << LABEL_PIER(pier1) << rptNewLine;
         pPara = new rptParagraph;
         (*pChapter) << pPara;

         (*pPara) << _T("Average Skew Angle: ") << symbol(theta) << _T(" = ") << angle.SetValue(fabs((pier1_lldf.skew1 + pier1_lldf.skew2)/2)) << rptNewLine;
         (*pPara) << _T("Span Length: L = ") << xdim.SetValue(pier1_lldf.L) << rptNewLine << rptNewLine;

         // Negative moment DF from pier1_lldf
         ReportMoment(pPara,
                      pier1_lldf,
                      pier1_lldf.gM1,
                      pier1_lldf.gM2,
                      pier1_lldf.gM,
                      bSIUnits,pDisplayUnits);
      }

      // Positive moment DF
      pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
      (*pChapter) << pPara;
      if (bContinuousAtStart || bContinuousAtEnd || bIntegralAtStart || bIntegralAtEnd)
      {
         (*pPara) << _T("Distribution Factor for Positive and Negative Moment in Span ") << LABEL_SPAN(spanIdx) << rptNewLine;
      }
      else
      {
         (*pPara) << _T("Distribution Factor for Positive Moment in Span ") << LABEL_SPAN(spanIdx) << rptNewLine;
      }
      pPara = new rptParagraph;
      (*pChapter) << pPara;

      (*pPara) << _T("Average Skew Angle: ") << symbol(theta) << _T(" = ") << angle.SetValue(fabs((span_lldf.skew1 + span_lldf.skew2)/2)) << rptNewLine;
      (*pPara) << _T("Span Length: L = ") << xdim.SetValue(span_lldf.L) << rptNewLine << rptNewLine;

      ReportMoment(pPara,
                   span_lldf,
                   span_lldf.gM1,
                   span_lldf.gM2,
                   span_lldf.gM,
                   bSIUnits,pDisplayUnits);

      if ( bContinuousAtEnd || bIntegralAtEnd )
      {
         pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
         (*pChapter) << pPara;
         (*pPara) << _T("Distribution Factor for Negative Moment over Pier ") << LABEL_PIER(pier2) << rptNewLine;
         pPara = new rptParagraph;
         (*pChapter) << pPara;

         (*pPara) << _T("Average Skew Angle: ") << symbol(theta) << _T(" = ") << angle.SetValue(fabs((pier2_lldf.skew1 + pier2_lldf.skew2)/2)) << rptNewLine;
         (*pPara) << _T("Span Length: L = ") << xdim.SetValue(pier2_lldf.L) << rptNewLine << rptNewLine;

         // Negative moment DF from pier2_lldf
         ReportMoment(pPara,
                      pier2_lldf,
                      pier2_lldf.gM1,
                      pier2_lldf.gM2,
                      pier2_lldf.gM,
                      bSIUnits,pDisplayUnits);
      }
      

      //////////////////////////////////////////////////////////////
      // Shears
      //////////////////////////////////////////////////////////////
      pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
      (*pChapter) << pPara;
      (*pPara) << _T("Distribution Factor for Shear in Span ") << LABEL_SPAN(spanIdx) << rptNewLine;
      pPara = new rptParagraph;
      (*pChapter) << pPara;

      (*pPara) << _T("Average Skew Angle: ") << symbol(theta) << _T(" = ") << angle.SetValue(fabs((span_lldf.skew1 + span_lldf.skew2)/2)) << rptNewLine;
      (*pPara) << _T("Span Length: L = ") << xdim.SetValue(span_lldf.L) << rptNewLine << rptNewLine;

      ReportShear(pPara,
                  span_lldf,
                  span_lldf.gV1,
                  span_lldf.gV2,
                  span_lldf.gV,
                  bSIUnits,pDisplayUnits);

      ////////////////////////////////////////////////////////////////////////////
      // Fatigue limit states
      ////////////////////////////////////////////////////////////////////////////
      if ( WBFL::LRFD::LRFDVersionMgr::Version::FourthEditionWith2009Interims <= WBFL::LRFD::LRFDVersionMgr::GetVersion() )
      {
         pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
         (*pPara) << _T("Fatigue Limit States");
         (*pChapter) << pPara;
         pPara = new rptParagraph;
         (*pChapter) << pPara;

         std::_tstring superscript;

         rptRcScalar scalar2 = scalar;
         rptRcScalar scalar3 = scalar;

         //////////////////////////////////////////////////////////////
         // Moments
         //////////////////////////////////////////////////////////////
         if ( bContinuousAtEnd || bIntegralAtEnd )
         {
            pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
            (*pChapter) << pPara;
            (*pPara) << _T("Distribution Factor for Negative Moment over Pier ") << LABEL_PIER(pier1) << rptNewLine;
            pPara = new rptParagraph;
            (*pChapter) << pPara;

            superscript = (pier1_lldf.bExteriorGirder ? _T("ME") : _T("MI"));
            Float64 mpf = pier1_lldf.gM1.GetMultiplePresenceFactor();
            (*pPara) << _T("g") << superscript << Sub(_T("Fatigue")) << _T(" = ") << _T("mg") << superscript << Sub(_T("1")) << _T("/m =") << scalar.SetValue(pier1_lldf.gM1.mg) << _T("/") << scalar3.SetValue(mpf) << _T(" = ") << scalar2.SetValue(pier1_lldf.gM1.mg/mpf);
         }

         // Positive moment DF
         pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
         (*pChapter) << pPara;
         if (bContinuousAtStart || bContinuousAtEnd || bIntegralAtStart || bIntegralAtEnd)
         {
            (*pPara) << _T("Distribution Factor for Positive and Negative Moment in Span ") << LABEL_SPAN(spanIdx) << rptNewLine;
         }
         else
         {
            (*pPara) << _T("Distribution Factor for Positive Moment in Span ") << LABEL_SPAN(spanIdx) << rptNewLine;
         }
         pPara = new rptParagraph;
         (*pChapter) << pPara;

         superscript = (span_lldf.bExteriorGirder ? _T("ME") : _T("MI"));
         Float64 mpf = span_lldf.gM1.GetMultiplePresenceFactor();
         (*pPara) << _T("g") << superscript << Sub(_T("Fatigue")) << _T(" = ") << _T("mg") << superscript << Sub(_T("1")) << _T("/m =") << scalar.SetValue(span_lldf.gM1.mg) << _T("/") << scalar3.SetValue(mpf) << _T(" = ") << scalar2.SetValue(span_lldf.gM1.mg/mpf);

         if ( bContinuousAtEnd || bIntegralAtEnd )
         {
            pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
            (*pChapter) << pPara;
            (*pPara) << _T("Distribution Factor for Negative Moment over Pier ") << LABEL_PIER(pier2) << rptNewLine;
            pPara = new rptParagraph;
            (*pChapter) << pPara;

            superscript = (pier2_lldf.bExteriorGirder ? _T("ME") : _T("MI"));
            Float64 mpf = pier2_lldf.gM1.GetMultiplePresenceFactor();
            (*pPara) << _T("g") << superscript << Sub(_T("Fatigue")) << _T(" = ") << _T("mg") << superscript << Sub(_T("1")) << _T("/m =") << scalar.SetValue(pier2_lldf.gM1.mg) << _T("/") << scalar3.SetValue(mpf) << _T(" = ") << scalar2.SetValue(pier2_lldf.gM1.mg/mpf);
         }

         //////////////////////////////////////////////////////////////
         // Shears
         //////////////////////////////////////////////////////////////
         pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
         (*pChapter) << pPara;
         (*pPara) << _T("Distribution Factor for Shear in Span ") << LABEL_SPAN(spanIdx) << rptNewLine;
         pPara = new rptParagraph;
         (*pChapter) << pPara;

         superscript = (span_lldf.bExteriorGirder ? _T("VE") : _T("VI"));
         mpf = span_lldf.gV1.GetMultiplePresenceFactor();
         (*pPara) << _T("g") << superscript << Sub(_T("Fatigue")) << _T(" = ") << _T("mg") << superscript << Sub(_T("1")) << _T("/m =") << scalar.SetValue(span_lldf.gV1.mg) << _T("/") << scalar3.SetValue(mpf) << _T(" = ") << scalar2.SetValue(span_lldf.gV1.mg/mpf);

      }
   } // next span
}

WBFL::LRFD::LiveLoadDistributionFactorBase* CIBeamDistFactorEngineer::GetLLDFParameters(IndexType spanOrPierIdx,GirderIndexType gdrIdx,DFParam dfType,Float64 fcgdr,IBEAM_LLDFDETAILS* plldf)
{
   GET_IFACE(IMaterials,    pMaterials);
   GET_IFACE(ISectionProperties,        pSectProp);
   GET_IFACE(ILibrary,          pLib);
   GET_IFACE(ISpecification,    pSpec);
   GET_IFACE(IBridge,           pBridge);
   GET_IFACE(ILiveLoads,        pLiveLoads);
   GET_IFACE(IBridgeDescription,pIBridgeDesc);

   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   ATLASSERT( pBridgeDesc->GetDistributionFactorMethod() != pgsTypes::DirectlyInput );

   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

   // Determine span/pier index... This is the index of a pier and the next span.
   // If this is the last pier, span index is for the last span
   SpanIndexType span = INVALID_INDEX;
   PierIndexType pier = INVALID_INDEX;
   SpanIndexType prev_span = INVALID_INDEX;
   SpanIndexType next_span = INVALID_INDEX;
   PierIndexType prev_pier = INVALID_INDEX;
   PierIndexType next_pier = INVALID_INDEX;
   GetIndicies(spanOrPierIdx,dfType,span,pier,prev_span,next_span,prev_pier,next_pier);

   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(pBridgeDesc->GetSpan(span));
   GirderIndexType nGirders = pGroup->GetGirderCount();

   if ( nGirders <= gdrIdx )
   {
      gdrIdx = nGirders-1;
   }

   CSpanKey spanKey(span,gdrIdx);

   // determine overhang, spacing base data, and controlling poi
   pgsPointOfInterest poi;
   GetGirderSpacingAndOverhang(spanKey,dfType, plldf, &poi);

   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   // Throws exception if fails requirement (no need to catch it)
   GET_IFACE(ILiveLoadDistributionFactors, pDistFactors);
   Int32 bridgeWideRoaFlag = pDistFactors->VerifyDistributionFactorRequirements(poi);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType llIntervalIdx = pIntervals->GetLiveLoadInterval();

   plldf->I  = pSectProp->GetIxx(pgsTypes::sptGross,releaseIntervalIdx,poi);
   plldf->A  = pSectProp->GetAg(pgsTypes::sptGross,releaseIntervalIdx,poi);
   plldf->Yt = pSectProp->GetY(pgsTypes::sptGross,releaseIntervalIdx,poi,pgsTypes::TopGirder);

   if ( IsNonstructuralDeck(pBridge->GetDeckType()) )
   {
      // no deck so modular ratio is 1.0 (Eg/Eg)
      GET_IFACE(IGirder,pGdr);
      plldf->n = 1.0;
      plldf->ts = pGdr->GetMinTopFlangeThickness(poi);
      plldf->eg = plldf->Yt - plldf->ts/2; // measure eg to the center of the top flange
   }
   else
   {
      plldf->ts = pBridge->GetStructuralSlabDepth(poi);

      GET_IFACE(IPointOfInterest, pPoi);
      IndexType deckCastingRegionIdx = pPoi->GetDeckCastingRegion(poi);
      ATLASSERT(deckCastingRegionIdx != INVALID_INDEX);

      Float64 EcDeck = pMaterials->GetDeckEc(deckCastingRegionIdx,llIntervalIdx);
   
      // use release interval for girder properties because we want the non-composite gross properties. we know it is non-composite at release
      if ( fcgdr < 0 )
      {
         // fcgdr < 0 means use the current bridge model
         Float64 EcSegment = pMaterials->GetSegmentEc(segmentKey,llIntervalIdx);
   
         plldf->n = EcSegment/EcDeck;
      }
      else
      {
         GET_IFACE(IMaterials,pMaterial);
         Float64 Ecgdr = pMaterial->GetEconc(pMaterial->GetSegmentConcreteType(segmentKey),fcgdr,
                                             pMaterial->GetSegmentStrengthDensity(segmentKey),
                                             pMaterial->GetSegmentEccK1(segmentKey),
                                             pMaterial->GetSegmentEccK2(segmentKey)
                                             );
         plldf->n  = Ecgdr / EcDeck;
      }

      // Add in haunch (fillet) if required
      if (pSectProp->GetHaunchAnalysisSectionPropertiesType() == pgsTypes::hspZeroHaunch)
      {
         plldf->eg = plldf->Yt + plldf->ts / 2;
      }
      else
      {
         Float64 fillet = pBridge->GetFillet();
         plldf->eg = plldf->Yt + plldf->ts / 2 + fillet;
      }
   }

   plldf->L = GetEffectiveSpanLength(spanOrPierIdx,gdrIdx,dfType);

   // we've had problems with computing live load distribution factors by the WSDOT method
   // when the overhang is exactly equal to half the girder spacing. The wrong criteria
   // has been selected because of rounding errors in the last few decimal places. To
   // help solve this problem, round the spacing and overhang dimensions
   plldf->Savg = RoundOff(plldf->Savg,0.001);
   plldf->leftSlabOverhang  = RoundOff(plldf->leftSlabOverhang,0.001);
   plldf->rightSlabOverhang = RoundOff(plldf->rightSlabOverhang,0.001);

   std::vector<IntermedateDiaphragm> diaphragms = pBridge->GetCastInPlaceDiaphragms(CSpanKey(span,gdrIdx));
   std::vector<IntermedateDiaphragm>::size_type nDiaphragms = diaphragms.size();

   bool bSkew = !( IsZero(plldf->skew1) && IsZero(plldf->skew2) ); 
   bool bSkewMoment = pSpecEntry->IgnoreSkewReductionForMoment() ? false : bSkew;
   bool bSkewShear  = bSkew;

   if ( WBFL::LRFD::LRFDVersionMgr::Version::SeventhEdition2014 <= WBFL::LRFD::LRFDVersionMgr::GetVersion() )
   {
      // Starting with LRFD 7th Edition, 2014, skew correction is only applied from
      // the obtuse corner to mid-span of exterior and first interior girders.
      // Use the IsObtuseCorner method to determine if there is an obtuse corner for
      // this girder. If so, apply the skew correction
      if ( dfType == dfReaction )
      {
         bool bObtuseLeft = false;
         if ( prev_span != INVALID_INDEX )
         {
            bObtuseLeft = pBridge->IsObtuseCorner(CSpanKey(prev_span,gdrIdx),pgsTypes::metEnd);
         }

         bool bObtuseRight = false;
         if ( next_span != INVALID_INDEX )
         {
            bObtuseRight = pBridge->IsObtuseCorner(CSpanKey(next_span,gdrIdx),pgsTypes::metStart);
         }

         bSkewShear = (bObtuseLeft || bObtuseRight ? true : false);
      }
      else
      {
         bool bObtuseStart = pBridge->IsObtuseCorner(CSpanKey(span,gdrIdx),pgsTypes::metStart);
         bool bObtuseEnd   = pBridge->IsObtuseCorner(CSpanKey(span,gdrIdx),pgsTypes::metEnd);
         bSkewShear = (bObtuseStart || bObtuseEnd ? true : false);
      }
   }

   WBFL::LRFD::LldfTypeAEKIJ* pLLDF;
   int lldf_method = pSpecEntry->GetLiveLoadDistributionMethod();
   if ( lldf_method == LLDF_LRFD )
   {
      bool bRigidMethod = (0 < nDiaphragms ? true : false); // must have diaphragms for rigid method
      if (WBFL::LRFD::LRFDVersionMgr::Version::SeventhEdition2014 <= WBFL::LRFD::LRFDVersionMgr::GetVersion())
      {
         // rigid method only used for steel bridges starting with LRFD 7th Edition, 2014
         // but we can override
         bRigidMethod &= pSpecEntry->UseRigidMethod();
      }


      pLLDF = new WBFL::LRFD::LldfTypeAEK(plldf->gdrNum,
                                  plldf->Savg,
                                  plldf->gdrSpacings,
                                  plldf->leftCurbOverhang,
                                  plldf->rightCurbOverhang,
                                  plldf->Nl, 
                                  plldf->wLane,
                                  plldf->L,
                                  plldf->ts,
                                  plldf->n,
                                  plldf->I, 
                                  plldf->A, 
                                  plldf->eg,
                                  bRigidMethod,
                                  plldf->skew1,
                                  plldf->skew2,
                                  bSkewMoment,bSkewShear);
   }
   else
   {
      // Note that WSDOT and TxDOT methods are identical except for slab overhang threshold
      Float64 slab_overhang_threshold;
      if (lldf_method==LLDF_WSDOT)
      {
         slab_overhang_threshold = 0.4;
      }
      else 
      {
         ATLASSERT(lldf_method==LLDF_TXDOT);
         slab_overhang_threshold = 0.5;
      }

      pLLDF = new WBFL::LRFD::WsdotLldfTypeAEK(plldf->gdrNum,
                                       plldf->Savg,
                                       plldf->gdrSpacings,
                                       plldf->leftCurbOverhang,
                                       plldf->rightCurbOverhang,
                                       plldf->Nl, 
                                       plldf->wLane,
                                       plldf->L,
                                       plldf->ts,
                                       plldf->n,
                                       plldf->I, 
                                       plldf->A, 
                                       plldf->eg,
                                       plldf->leftSlabOverhang,
                                       plldf->rightSlabOverhang,
                                       false, // rigid method never used by WSDOT or TxDOT methods
                                       plldf->skew1,
                                       plldf->skew2,
                                       bSkewMoment,bSkewShear,
                                       slab_overhang_threshold);
   }

   pLLDF->SetRangeOfApplicability( pLiveLoads->GetRangeOfApplicabilityAction(), bridgeWideRoaFlag);

   plldf->Kg = pLLDF->GetKg();

   return pLLDF;
}


void CIBeamDistFactorEngineer::ReportMoment(rptParagraph* pPara,IBEAM_LLDFDETAILS& lldf,WBFL::LRFD::ILiveLoadDistributionFactor::DFResult& gM1,WBFL::LRFD::ILiveLoadDistributionFactor::DFResult& gM2,Float64 gM,bool bSIUnits,IEAFDisplayUnits* pDisplayUnits)
{
   std::_tstring strImagePath(rptStyleManager::GetImagePath());

   INIT_SCALAR_PROTOTYPE(rptRcScalar, scalar, pDisplayUnits->GetScalarFormat());

   GET_IFACE(ILibrary, pLib);
   GET_IFACE(ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

   Int16 df_method = pSpecEntry->GetLiveLoadDistributionMethod();

   if ( lldf.bExteriorGirder )
   {
      if (gM1.EqnData.bWasUsed)
      {
         // The only way spec equations are used is if we are using WSDOT spec's, and/or
         // the slab overhang is <= half the girder spacing
         ATLASSERT(df_method==LLDF_WSDOT || df_method==LLDF_TXDOT);

         (*pPara) << Bold(_T("1 Loaded Lane: Spec Equations")) << rptNewLine;
         if (df_method == LLDF_WSDOT)
         {
            (*pPara) << _T("Note: Using distribution factor for interior girder per WSDOT BDM 3.9.3A.") << rptNewLine;
         }
         else if (df_method == LLDF_TXDOT)
         {
            (*pPara) << _T("Note: Using distribution factor for interior girder per TxDOT Bridge Design Manual - LRFD") << rptNewLine;
         }
         else if (gM1.ControllingMethod & WBFL::LRFD::INTERIOR_OVERRIDE)
         {
            (*pPara) <<  LLDF_INTOVERRIDE_STR << rptNewLine;
         }

         (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("mg_1_MI_Type_K_SI.png") : _T("mg_1_MI_Type_K_US.png"))) << rptNewLine;
         (*pPara) << _T("mg") << Super(_T("ME")) << Sub(_T("1")) << _T(" = ") << _T("mg") << Super(_T("MI")) << Sub(_T("1")) << _T(" = ") << scalar.SetValue(gM1.EqnData.mg) << rptNewLine;
      }

      if ( gM1.LeverRuleData.bWasUsed )
      {
         (*pPara) << Bold(_T("1 Loaded Lane: Lever Rule")) << rptNewLine;
         if (gM1.ControllingMethod & WBFL::LRFD::INTERIOR_OVERRIDE)
         {
            if (df_method == LLDF_WSDOT)
            {
               (*pPara) << _T("Note: Using distribution factor for interior girder per WSDOT BDM 3.9.3A.") << rptNewLine;
            }
            else
            {
               (*pPara) << LLDF_INTOVERRIDE_STR << rptNewLine;
            }
         }

         ReportLeverRule(pPara,true,1.0,gM1.LeverRuleData,m_pBroker,pDisplayUnits);
      }

      if ( gM1.RigidData.bWasUsed )
      {
         (*pPara) << Bold(_T("1 Loaded Lane: Rigid Method")) << rptNewLine;
         ReportRigidMethod(pPara,gM1.RigidData,m_pBroker,pDisplayUnits);
      }

      if ( gM1.LanesBeamsData.bWasUsed )
      {
         (*pPara) << Bold(_T("1 Loaded Lane: Number of Lanes over Number of Beams - Factor cannot be less than this")) << rptNewLine;
         (*pPara) << _T("Skew correction is not applied to Lanes/Beams method")<< rptNewLine;
         ReportLanesBeamsMethod(pPara,gM1.LanesBeamsData,m_pBroker,pDisplayUnits);
      }

      (*pPara) << rptNewLine;
      if ( 2 <= lldf.Nl )
      {

         if (gM2.EqnData.bWasUsed)
         {
            if ( gM2.ControllingMethod & WBFL::LRFD::INTERIOR_OVERRIDE )
            {
               (*pPara) << Bold(_T("2+ Loaded Lanes: Spec Equations")) << rptNewLine;
               if (df_method == LLDF_WSDOT)
               {
                  (*pPara) << _T("Note: Using distribution factor for interior girder per WSDOT BDM 3.9.3A.") << rptNewLine;
               }
               else if (df_method == LLDF_TXDOT)
               {
                  (*pPara) << _T("Note: Using distribution factor for interior girder per TxDOT Bridge Design Manual - LRFD") << rptNewLine;
               }
               else
               {
                  (*pPara) << LLDF_INTOVERRIDE_STR << rptNewLine;
               }

               (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("mg_2_MI_Type_K_SI.png") : _T("mg_2_MI_Type_K_US.png"))) << rptNewLine;
               (*pPara) << _T("mg") << Super(_T("ME")) << Sub(_T("2+")) << _T(" = ") << _T("mg") << Super(_T("MI")) << Sub(_T("2+")) << _T(" = ") << scalar.SetValue(gM2.EqnData.mg) << rptNewLine;
            }
            else
            {
               (*pPara) << Bold(_T("2+ Loaded Lanes: Spec Equations")) << rptNewLine;
               if (df_method == LLDF_WSDOT)
               {
                  (*pPara) << _T("Note: Using distribution factor for interior girder per WSDOT BDM 3.9.3A.") << rptNewLine;
               }
               else if (df_method == LLDF_TXDOT)
               {
                  (*pPara) << _T("Note: Using distribution factor for interior girder per TxDOT Bridge Design Manual - LRFD") << rptNewLine;
               }

               (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("mg_2_ME_Type_K_SI.png") : _T("mg_2_ME_Type_K_US.png"))) << rptNewLine;
               (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("mg_2_MI_Type_K_SI.png") : _T("mg_2_MI_Type_K_US.png"))) << rptNewLine;
               (*pPara) << _T("mg") << Super(_T("MI")) << Sub(_T("2+")) << _T(" = ") << scalar.SetValue(gM2.EqnData.mg) << rptNewLine;

               if (df_method == LLDF_WSDOT || df_method == LLDF_TXDOT)
               {
                  (*pPara) << _T("e ") << symbol(GTE) << _T(" 1.0") << rptNewLine;
               }
               (*pPara) << _T("e = ") << scalar.SetValue(gM2.EqnData.e) << rptNewLine;

               (*pPara) << _T("mg") << Super(_T("ME")) << Sub(_T("2+")) << _T(" = ") << scalar.SetValue(gM2.EqnData.mg * gM2.EqnData.e) << _T(" for equation") << rptNewLine;
            }
         }

         if ( gM2.LeverRuleData.bWasUsed )
         {
            (*pPara) << Bold(_T("2+ Loaded Lanes: Lever Rule")) << rptNewLine;

            if (gM2.ControllingMethod & WBFL::LRFD::INTERIOR_OVERRIDE)
            {
               if (df_method == LLDF_WSDOT)
               {
                  (*pPara) << _T("Note: Using distribution factor for interior girder per WSDOT BDM 3.9.3A.") << rptNewLine;
               }
               else
               {
                  (*pPara) << LLDF_INTOVERRIDE_STR << rptNewLine;
               }
            }
   
            ReportLeverRule(pPara,true,1.0,gM2.LeverRuleData,m_pBroker,pDisplayUnits);
         }

         if ( gM2.RigidData.bWasUsed )
         {
            (*pPara) << Bold(_T("2+ Loaded Lanes: Rigid Method")) << rptNewLine;
            ReportRigidMethod(pPara,gM2.RigidData,m_pBroker,pDisplayUnits);
         }

         if ( gM2.LanesBeamsData.bWasUsed )
         {
            (*pPara) << Bold(_T("2+ Loaded Lane: Number of Lanes over Number of Beams - Factor cannot be less than this")) << rptNewLine;
            (*pPara) << _T("Skew correction is not applied to Lanes/Beams method")<< rptNewLine;
            ReportLanesBeamsMethod(pPara,gM2.LanesBeamsData,m_pBroker,pDisplayUnits);
         }
      }

      (*pPara) << rptNewLine;

      if ( gM1.ControllingMethod & WBFL::LRFD::MOMENT_SKEW_CORRECTION_APPLIED )
      {
         (*pPara) << Bold(_T("Skew Correction")) << rptNewLine;
         Float64 skew_delta_max = WBFL::Units::ConvertToSysUnits( 10.0, WBFL::Units::Measure::Degree );
         if (fabs(lldf.skew1 - lldf.skew2) < skew_delta_max)
         {
            (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("SkewCorrection_Moment_SI.png") : _T("SkewCorrection_Moment_US.png"))) << rptNewLine;
         }

         (*pPara) << _T("Skew Correction Factor: = ") << scalar.SetValue(gM1.SkewCorrectionFactor) << rptNewLine;
         (*pPara) << rptNewLine;
         (*pPara) << _T("Skew Corrected Factor: mg") << Super(_T("ME")) << Sub(_T("1")) << _T(" = ") << scalar.SetValue(gM1.mg);
         (lldf.Nl == 1 || gM2.mg <= gM1.mg) ? (*pPara) << Bold(_T(" < Controls")) << rptNewLine : (*pPara) << rptNewLine;
         if ( 2 <= lldf.Nl )
         {
            (*pPara) << _T("Skew Corrected Factor: mg") << Super(_T("ME")) << Sub(_T("2+")) << _T(" = ") << scalar.SetValue(gM2.mg);
            (gM1.mg < gM2.mg) ? (*pPara) << Bold(_T(" < Controls")) << rptNewLine : (*pPara) << rptNewLine;
         }
      }
   }
   else
   {
      // Interior girder
      if (gM1.EqnData.bWasUsed)
      {
         (*pPara) << Bold(_T("1 Loaded Lane: Spec Equations")) << rptNewLine;
         (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("mg_1_MI_Type_K_SI.png") : _T("mg_1_MI_Type_K_US.png"))) << rptNewLine;
         (*pPara) << _T("mg") << Super(_T("MI")) << Sub(_T("1")) << _T(" = ") << scalar.SetValue(gM1.EqnData.mg) << rptNewLine;
      }

      if ( gM1.LeverRuleData.bWasUsed )
      {
         (*pPara) << Bold(_T("1 Loaded Lane: Lever Rule")) << rptNewLine;
         if (gM1.EqnData.bWasUsed )
         {
            (*pPara) << _T("Use lesser of equation and lever rule result") << rptNewLine;
         }

         ReportLeverRule(pPara,true,1.0,gM1.LeverRuleData,m_pBroker,pDisplayUnits);
      }

      if ( gM1.LanesBeamsData.bWasUsed )
      {
         (*pPara) << Bold(_T("1 Loaded Lane: Number of Lanes over Number of Beams - Factor cannot be less than this")) << rptNewLine;
         (*pPara) << _T("Skew correction is not applied to Lanes/Beams method")<< rptNewLine;
         ReportLanesBeamsMethod(pPara,gM1.LanesBeamsData,m_pBroker,pDisplayUnits);
      }

      (*pPara) << rptNewLine;


      if ( 2 <= lldf.Nl )
      {
         if (gM2.EqnData.bWasUsed )
         {
            (*pPara) << Bold(_T("2+ Loaded Lanes: Spec Equations")) << rptNewLine;
            (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("mg_2_MI_Type_K_SI.png") : _T("mg_2_MI_Type_K_US.png"))) << rptNewLine;
            (*pPara) << _T("mg") << Super(_T("MI")) << Sub(_T("2+")) << _T(" = ") << scalar.SetValue(gM2.EqnData.mg) << rptNewLine;
         }

         if ( gM2.LeverRuleData.bWasUsed )
         {
            (*pPara) << Bold(_T("2+ Loaded Lanes: Lever Rule")) << rptNewLine;
            ReportLeverRule(pPara,true,1.0,gM2.LeverRuleData,m_pBroker,pDisplayUnits);
         }

         if ( gM2.LanesBeamsData.bWasUsed )
         {
            (*pPara) << Bold(_T("2+ Loaded Lane: Number of Lanes over Number of Beams - Factor cannot be less than this")) << rptNewLine;
            (*pPara) << _T("Skew correction is not applied to Lanes/Beams method")<< rptNewLine;
            ReportLanesBeamsMethod(pPara,gM2.LanesBeamsData,m_pBroker,pDisplayUnits);
         }

         (*pPara) << rptNewLine;
      }

      if ( gM1.ControllingMethod & WBFL::LRFD::MOMENT_SKEW_CORRECTION_APPLIED )
      {
         (*pPara) << Bold(_T("Skew Correction")) << rptNewLine;
         Float64 skew_delta_max = WBFL::Units::ConvertToSysUnits( 10.0, WBFL::Units::Measure::Degree );
         if (fabs(lldf.skew1 - lldf.skew2) < skew_delta_max)
         {
            (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("SkewCorrection_Moment_SI.png") : _T("SkewCorrection_Moment_US.png"))) << rptNewLine;
         }

         (*pPara) << _T("Skew Correction Factor: = ") << scalar.SetValue(gM1.SkewCorrectionFactor) << rptNewLine;
         (*pPara) << rptNewLine;
         (*pPara) << _T("Skew Corrected Factor: mg") << Super(_T("MI")) << Sub(_T("1")) << _T(" = ") << scalar.SetValue(gM1.mg);
         (lldf.Nl == 1 || gM2.mg <= gM1.mg) ? (*pPara) << Bold(_T(" < Controls")) << rptNewLine: (*pPara) << rptNewLine;
         if ( 2 <= lldf.Nl )
         {
            (*pPara) << _T("Skew Corrected Factor: mg") << Super(_T("MI")) << Sub(_T("2+")) << _T(" = ") << scalar.SetValue(gM2.mg);
            (gM1.mg < gM2.mg) ? (*pPara) << Bold(_T(" < Controls")) << rptNewLine : (*pPara) << rptNewLine;
         }
      }
   }
}

void CIBeamDistFactorEngineer::ReportShear(rptParagraph* pPara,IBEAM_LLDFDETAILS& lldf,WBFL::LRFD::ILiveLoadDistributionFactor::DFResult& gV1,WBFL::LRFD::ILiveLoadDistributionFactor::DFResult& gV2,Float64 gV,bool bSIUnits,IEAFDisplayUnits* pDisplayUnits)
{
   std::_tstring strImagePath(rptStyleManager::GetImagePath());

   INIT_SCALAR_PROTOTYPE(rptRcScalar, scalar, pDisplayUnits->GetScalarFormat());

   GET_IFACE(ILibrary, pLib);
   GET_IFACE(ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

   Int16 df_method = pSpecEntry->GetLiveLoadDistributionMethod();

   if ( lldf.bExteriorGirder )
   {
      if ( gV1.EqnData.bWasUsed )
      {
         // The only way spec equations are used is if we are using WSDOT spec's, and
         // the slab overhang is <= half the girder spacing
         ATLASSERT(df_method==LLDF_WSDOT || df_method == LLDF_TXDOT);

         (*pPara) << Bold(_T("1 Loaded Lane: Spec Equations")) << rptNewLine;
         REPORT_LLDF_INTOVERRIDE(gV1);
         (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("mg_1_VI_Type_K_SI.png") : _T("mg_1_VI_Type_K_US.png"))) << rptNewLine;
         (*pPara) << _T("mg") << Super(_T("VE")) << Sub(_T("1")) << _T(" = ") << _T("mg") << Super(_T("VI")) << Sub(_T("1")) << _T(" = ") << scalar.SetValue(gV1.EqnData.mg) << rptNewLine;
      }

      if ( gV1.LeverRuleData.bWasUsed )
      {
         (*pPara) << Bold(_T("1 Loaded Lane: Lever Rule")) << rptNewLine;
         if (gV1.ControllingMethod & WBFL::LRFD::INTERIOR_OVERRIDE)
         {
            if (df_method == LLDF_WSDOT)
            {
               (*pPara) << _T("Note: Using distribution factor for interior girder per WSDOT BDM 3.9.3A.") << rptNewLine;
            }
            else
            {
               (*pPara) << LLDF_INTOVERRIDE_STR << rptNewLine;
            }
         }

          ReportLeverRule(pPara,false,1.0,gV1.LeverRuleData,m_pBroker,pDisplayUnits);
      }

      if ( gV1.RigidData.bWasUsed )
      {
         (*pPara) << Bold(_T("1 Loaded Lane: Rigid Method")) << rptNewLine;
         (*pPara) << _T("mg") << Super(_T("VE")) << Sub(_T("1")) << _T(" = ") << scalar.SetValue(gV1.RigidData.mg) << rptNewLine;
         (*pPara) << _T("See Moment for details") << rptNewLine;
      }

      if ( gV1.LanesBeamsData.bWasUsed )
      {
         (*pPara) << Bold(_T("1 Loaded Lane: Number of Lanes over Number of Beams - Factor cannot be less than this")) << rptNewLine;
         (*pPara) << _T("Skew correction is not applied to Lanes/Beams method")<< rptNewLine;
         ReportLanesBeamsMethod(pPara,gV1.LanesBeamsData,m_pBroker,pDisplayUnits);
      }

      (*pPara) << rptNewLine;

      if ( 2 <= lldf.Nl )
      {
         if (gV2.ControllingMethod & WBFL::LRFD::E_OVERRIDE)
         {
            ATLASSERT(gV2.ControllingMethod & WBFL::LRFD::SPEC_EQN);
            (*pPara) << Bold(_T("2+ Loaded Lanes: Spec Equation")) << rptNewLine;
            if (df_method == LLDF_WSDOT)
            {
               (*pPara) << _T("Note: Using distribution factor for interior girder per WSDOT BDM 3.9.3A.") << rptNewLine;
            }
            else if (df_method == LLDF_TXDOT)
            {
               (*pPara) << _T("Note: Using distribution factor for interior girder per TxDOT Bridge Design Manual - LRFD") << rptNewLine;
            }
            else if (gV2.ControllingMethod & WBFL::LRFD::INTERIOR_OVERRIDE)
            {
               (*pPara) << LLDF_INTOVERRIDE_STR << rptNewLine;
            }

            (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("mg_2_VE_Type_K_SI.png") : _T("mg_2_VE_Type_K_US.png"))) << rptNewLine;
            (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("mg_2_VI_Type_K_SI.png") : _T("mg_2_VI_Type_K_US.png"))) << rptNewLine;
            (*pPara) << _T("mg") << Super(_T("VI")) << Sub(_T("2+")) << _T(" = ") << scalar.SetValue(gV2.EqnData.mg) << rptNewLine;

            if (df_method == LLDF_WSDOT || df_method == LLDF_TXDOT)
            {
               (*pPara) << _T("e ") << symbol(GTE) << _T(" 1.0") << rptNewLine;
            }
            (*pPara) << _T("e = ") << scalar.SetValue(gV2.EqnData.e) << rptNewLine;

            (*pPara) << _T("mg") << Super(_T("VE")) << Sub(_T("2+")) << _T(" = ") << scalar.SetValue(gV2.EqnData.mg * gV2.EqnData.e) << rptNewLine;
         }
         else
         {
            if ( gV2.EqnData.bWasUsed )
            {
               ATLASSERT( gV2.ControllingMethod & WBFL::LRFD::INTERIOR_OVERRIDE);
               (*pPara) << Bold(_T("2+ Loaded Lanes: Spec Equation")) << rptNewLine;
               if (df_method == LLDF_WSDOT)
               {
                  (*pPara) << _T("Note: Using distribution factor for interior girder per WSDOT BDM 3.9.3A.") << rptNewLine;
               }
               else if (df_method == LLDF_TXDOT)
               {
                  (*pPara) << _T("Note: Using distribution factor for interior girder per TxDOT Bridge Design Manual - LRFD") << rptNewLine;
               }
               else
               {
                  (*pPara) << LLDF_INTOVERRIDE_STR << rptNewLine;
               }

               (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("mg_2_VE_Type_K_SI.png") : _T("mg_2_VE_Type_K_US.png"))) << rptNewLine;
               (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("mg_2_VI_Type_K_SI.png") : _T("mg_2_VI_Type_K_US.png"))) << rptNewLine;
               (*pPara) << _T("mg") << Super(_T("VI")) << Sub(_T("2+")) << _T(" = ") << scalar.SetValue(gV2.EqnData.mg) << rptNewLine;
               
               if (df_method == LLDF_WSDOT || df_method == LLDF_TXDOT)
               {
                  (*pPara) << _T("e ") << symbol(GTE) << _T(" 1.0") << rptNewLine;
               }
               (*pPara) << _T("e = ") << scalar.SetValue(gV2.EqnData.e) << rptNewLine;
               
               (*pPara) << _T("mg") << Super(_T("VE")) << Sub(_T("2+")) << _T(" = ") << scalar.SetValue(gV2.EqnData.e*gV2.EqnData.mg) << rptNewLine;
            }

            if ( gV2.LeverRuleData.bWasUsed)
            {
               (*pPara) << Bold(_T("2+ Loaded Lanes: Lever Rule")) << rptNewLine;

               if (gV2.ControllingMethod & WBFL::LRFD::INTERIOR_OVERRIDE)
               {
                  if (df_method == LLDF_WSDOT)
                  {
                     (*pPara) << _T("Note: Using distribution factor for interior girder per WSDOT BDM 3.9.3A.") << rptNewLine;
                  }
                  else
                  {
                     (*pPara) << LLDF_INTOVERRIDE_STR << rptNewLine;
                  }
               }
         
               ReportLeverRule(pPara,false,1.0,gV2.LeverRuleData,m_pBroker,pDisplayUnits);
            }
         }

         if ( gV2.RigidData.bWasUsed )
         {
            (*pPara) << Bold(_T("2+ Loaded Lanes: Rigid Method")) << rptNewLine;
            (*pPara) << _T("mg") << Super(_T("VE")) << Sub(_T("2+")) << _T(" = ") << scalar.SetValue(gV2.RigidData.mg) << rptNewLine;
            (*pPara) << _T("See Moment for details") << rptNewLine;
         }

         if ( gV2.LanesBeamsData.bWasUsed )
         {
            (*pPara) << Bold(_T("2+ Loaded Lane: Number of Lanes over Number of Beams - Factor cannot be less than this")) << rptNewLine;
            (*pPara) << _T("Skew correction is not applied to Lanes/Beams method")<< rptNewLine;
            ReportLanesBeamsMethod(pPara,gV2.LanesBeamsData,m_pBroker,pDisplayUnits);
         }
      }

      if ( gV1.ControllingMethod & WBFL::LRFD::SHEAR_SKEW_CORRECTION_APPLIED )
      {
         (*pPara) << rptNewLine;
         (*pPara) << Bold(_T("Skew Correction")) << rptNewLine;
         (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("SkewCorrection_Shear_SI.png") : _T("SkewCorrection_Shear_US.png"))) << rptNewLine;
         (*pPara) << _T("Skew Correction Factor: = ") << scalar.SetValue(gV1.SkewCorrectionFactor) << rptNewLine;
         (*pPara) << rptNewLine;
         (*pPara) << _T("Skew Corrected Factor: mg") << Super(_T("VE")) << Sub(_T("1")) << _T(" = ") << scalar.SetValue(gV1.mg);
         (lldf.Nl == 1 || gV2.mg <= gV1.mg) ? (*pPara) << Bold(_T(" < Controls")) << rptNewLine : (*pPara) << rptNewLine;
         if ( 2 <= lldf.Nl )
         {
            (*pPara) << _T("Skew Corrected Factor: mg") << Super(_T("VE")) << Sub(_T("2+")) << _T(" = ") << scalar.SetValue(gV2.mg);
            (gV1.mg < gV2.mg) ? (*pPara) << Bold(_T(" < Controls")) << rptNewLine : (*pPara)  << rptNewLine;
         }
      }
   }
   else
   {
      // Distribution factor for interior girder

      if ( gV1.EqnData.bWasUsed )
      {
         (*pPara) << Bold(_T("1 Loaded Lane: Spec Equations")) << rptNewLine;
         (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("mg_1_VI_Type_K_SI.png") : _T("mg_1_VI_Type_K_US.png"))) << rptNewLine;
         (*pPara) << _T("mg") << Super(_T("VI")) << Sub(_T("1")) << _T(" = ") << scalar.SetValue(gV1.EqnData.mg) << rptNewLine;
      }

      if ( gV1.LeverRuleData.bWasUsed )
      {
         (*pPara) << Bold(_T("1 Loaded Lane: Lever Rule")) << rptNewLine;
          ReportLeverRule(pPara,false,1.0,gV1.LeverRuleData,m_pBroker,pDisplayUnits);
      }

      if ( gV1.LanesBeamsData.bWasUsed )
      {
         (*pPara) << Bold(_T("1 Loaded Lane: Number of Lanes over Number of Beams - Factor cannot be less than this")) << rptNewLine;
         (*pPara) << _T("Skew correction is not applied to Lanes/Beams method")<< rptNewLine;
         ReportLanesBeamsMethod(pPara,gV1.LanesBeamsData,m_pBroker,pDisplayUnits);
      }

      (*pPara) << rptNewLine;

      if (2 <= lldf.Nl)
      {
         if ( gV2.EqnData.bWasUsed )
         {
            (*pPara) << Bold(_T("2+ Loaded Lanes: Spec Equations")) << rptNewLine;
            (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("mg_2_VI_Type_K_SI.png") : _T("mg_2_VI_Type_K_US.png"))) << rptNewLine;
            (*pPara) << _T("mg") << Super(_T("VI")) << Sub(_T("2+")) << _T(" = ") << scalar.SetValue(gV2.EqnData.mg) << rptNewLine;
         }

         if ( gV2.LeverRuleData.bWasUsed )
         {
            (*pPara) << Bold(_T("2+ Loaded Lanes: Lever Rule")) << rptNewLine;
            ReportLeverRule(pPara,false,1.0,gV2.LeverRuleData,m_pBroker,pDisplayUnits);
         }

         if ( gV2.LanesBeamsData.bWasUsed )
         {
            (*pPara) << Bold(_T("2+ Loaded Lane: Number of Lanes over Number of Beams - Factor cannot be less than this")) << rptNewLine;
            (*pPara) << _T("Skew correction is not applied to Lanes/Beams method")<< rptNewLine;
            ReportLanesBeamsMethod(pPara,gV2.LanesBeamsData,m_pBroker,pDisplayUnits);
         }

         (*pPara) << rptNewLine;
      }

      if ( gV1.ControllingMethod & WBFL::LRFD::SHEAR_SKEW_CORRECTION_APPLIED )
      {
         (*pPara) << Bold(_T("Skew Correction")) << rptNewLine << rptRcImage(strImagePath + (bSIUnits ? _T("SkewCorrection_Shear_SI.png") : _T("SkewCorrection_Shear_US.png"))) << rptNewLine;
         (*pPara) << _T("Skew Correction Factor: = ") << scalar.SetValue(gV1.SkewCorrectionFactor) << rptNewLine;
         (*pPara) << rptNewLine;
         (*pPara) << _T("Skew Corrected Factor: mg") << Super(_T("VI")) << Sub(_T("1")) << _T(" = ") << scalar.SetValue(gV1.mg);
         (lldf.Nl == 1 || gV2.mg <= gV1.mg) ? (*pPara) << Bold(_T(" < Controls")) << rptNewLine : (*pPara) << rptNewLine;
         if ( 2 <= lldf.Nl )
         {
            (*pPara) << _T("Skew Corrected Factor: mg") << Super(_T("VI")) << Sub(_T("2+")) << _T(" = ") << scalar.SetValue(gV2.mg);
            (gV1.mg < gV2.mg) ? (*pPara) << Bold(_T(" < Controls")) << rptNewLine : (*pPara) << rptNewLine;
         }
      }
   }
}

std::_tstring CIBeamDistFactorEngineer::GetComputationDescription(const CGirderKey& girderKey,const std::_tstring& libraryEntryName,pgsTypes::SupportedDeckType decktype, pgsTypes::AdjacentTransverseConnectivity connect)
{
   GET_IFACE(ILibrary, pLib);
   GET_IFACE(ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

   Int16 lldfMethod = pSpecEntry->GetLiveLoadDistributionMethod();

   std::_tstring descr(_T("Type (k) cross section. With "));


   if ( lldfMethod == LLDF_WSDOT )
   {
      descr += std::_tstring(_T("WSDOT Method per Bridge Design Manual Section 3.9.3A."));
   }
   else if ( lldfMethod == LLDF_TXDOT )
   {
      descr += std::_tstring(_T("TxDOT Method per per TxDOT Bridge Design Manual - LRFD"));
   }
   else if ( lldfMethod == LLDF_LRFD )
   {
      descr += std::_tstring(_T("AASHTO LRFD Method per Article 4.6.2.2"));
   }
   else
   {
      ATLASSERT(false);
   }

   // Special text if ROA is ignored
   GET_IFACE(ILiveLoads,pLiveLoads);
   std::_tstring strAction( pLiveLoads->GetLLDFSpecialActionText() );
   if ( !strAction.empty() )
   {
      descr += strAction;
   }

   return descr;
}
