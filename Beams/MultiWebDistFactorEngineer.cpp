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

// MultiWebDistFactorEngineer.cpp : Implementation of CMultiWebDistFactorEngineer
#include "stdafx.h"
#include "MultiWebDistFactorEngineer.h"
#include <PGSuperException.h>
#include <Units\Convert.h>
#include <PsgLib\TrafficBarrierEntry.h>
#include <PsgLib\SpecLibraryEntry.h>
#include <PsgLib\GirderLibraryEntry.h>
#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\StatusItem.h>
#include <PgsExt\GirderLabel.h>

#include <IFace\Bridge.h>
#include <IFace\Project.h>
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
// CMultiWebFactory
HRESULT CMultiWebDistFactorEngineer::FinalConstruct()
{
   return S_OK;
}

void CMultiWebDistFactorEngineer::BuildReport(const CGirderKey& girderKey,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits)
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
      GetSpanDF(spanKey,pgsTypes::StrengthI,USE_CURRENT_FC,&span_lldf);

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

      rptParagraph* pPara;

      const CSpanData2* pSpan = pBridgeDesc->GetSpan(spanIdx);
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(pSpan);

      std::_tstring strGirderName = pGroup->GetGirder(gdrIdx)->GetGirderName();

      pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
      (*pPara) << _T("Method of Computation:")<<rptNewLine;
      (*pChapter) << pPara;
      pPara = new rptParagraph;
      (*pChapter) << pPara;
      (*pPara) << GetComputationDescription(girderKey,
                                            strGirderName,
                                            pDeck->GetDeckType(),
                                            pDeck->TransverseConnectivity);

      pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
      (*pChapter) << pPara;
      (*pPara) << _T("Distribution Factor Parameters") << rptNewLine;
      pPara = new rptParagraph;
      (*pChapter) << pPara;

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
      (*pPara) << _T("Girder Width: b = ") << xdim.SetValue(span_lldf.b) << rptNewLine;
      (*pPara) << _T("Curb Offset = ") << xdim.SetValue(span_lldf.CurbOffset) << rptNewLine;
      Float64 de = span_lldf.Side==dfLeft ? span_lldf.leftDe : span_lldf.rightDe;
      (*pPara) << Sub2(_T("d"),_T("e")) << _T(" = ") << xdim.SetValue(de) << rptNewLine;
      Float64 ro = span_lldf.Side==dfLeft ? span_lldf.leftCurbOverhang : span_lldf.rightCurbOverhang;
      (*pPara) << _T("Roadway overhang = ") << xdim.SetValue(ro) << rptNewLine;
      (*pPara) << _T("Skew Angle at start: ") << symbol(theta) << _T(" = ") << angle.SetValue(fabs(span_lldf.skew1)) << rptNewLine;
      (*pPara) << _T("Skew Angle at end: ") << symbol(theta) << _T(" = ") << angle.SetValue(fabs(span_lldf.skew2)) << rptNewLine;
      
      GET_IFACE(ISpecification, pSpec);
      GET_IFACE(ILibrary, pLibrary);
      const auto* pSpecEntry = pLibrary->GetSpecEntry(pSpec->GetSpecification().c_str());
      if (pSpecEntry->IgnoreSkewReductionForMoment())
      {
         (*pPara) << _T("Skew reduction for moment distribution factors has been ignored (LRFD 4.6.2.2.2e)") << rptNewLine;
      }

      if (pBridgeDesc->GetDistributionFactorMethod() != pgsTypes::LeverRule)
      {
         if(span_lldf.Method==LLDF_TXDOT || !span_lldf.connectedAsUnit)
         {
            if (spMode == pgsTypes::spmTransformed)
            {
               (*pPara) << _T("Gross, non-composite section properties") << rptNewLine;
            }
            (*pPara) << _T("Area: A = ") << area.SetValue(span_lldf.A) << rptNewLine;
            (*pPara) << _T("Moment of Inertia: I = ") << inertia.SetValue(span_lldf.I) << rptNewLine;
            (*pPara) << _T("Possion Ratio: ") << symbol(mu) << _T(" = ") << span_lldf.PossionRatio << rptNewLine << rptNewLine;
         }

         if(span_lldf.Method == LLDF_TXDOT)
         {
           (*pPara) << _T("For TxDOT Method, for all distribution factor types, always use AASHTO Type (j) connected only enough to prevent relative vertical displacement, regardless of input. Use 4.6.2.2.2b-1, with TxDOT modifications (K=")<< GetTxDOTKfactor() <<_T(", g not to exceed S/10). Effects of skew will be ignored.") << rptNewLine;
           (*pPara) << rptRcImage(strImagePath + _T("LLDF_Type_HIJ_TxDOT.png")) << rptNewLine;
         }
         else if (span_lldf.connectedAsUnit)
         {
            (*pPara) << _T("Modular ratio: n = ") << scalar.SetValue(span_lldf.n) << rptNewLine;
            if (spMode == pgsTypes::spmTransformed)
            {
               (*pPara) << _T("Gross, non-composite section properties") << rptNewLine;
            }

            if (IsNonstructuralDeck(pBridge->GetDeckType()) && (m_BeamType == IMultiWebDistFactorEngineer::btDeckBulbTee || m_BeamType == IMultiWebDistFactorEngineer::btMultiWebTee))
            {
               (*pPara) << _T("Girder Stem Properties") << rptNewLine;
            }

            (*pPara) << _T("Area: A")<<Sub(_T("g"))<<_T(" = ")<< area.SetValue(span_lldf.Ag) << rptNewLine;
            (*pPara) << _T("Moment of Inertia: I")<<Sub(_T("g"))<<_T(" = ")<< inertia.SetValue(span_lldf.Ig) << rptNewLine;
            (*pPara) << _T("Top centroidal distance: Y") << Sub(_T("tg")) << _T(" = ") << xdim2.SetValue(span_lldf.Yt) << rptNewLine;

            if(pBridge->IsCompositeDeck())
            {
               (*pPara) << _T("Slab Thickness (depth of CIP deck + top flange thickness): t") << Sub(_T("s")) << _T(" = ") << xdim2.SetValue(span_lldf.ts) << rptNewLine;
               (*pPara) << _T("Distance between CG of slab and girder: e") << Sub(_T("g")) <<_T(" = ") << xdim2.SetValue(span_lldf.eg) << rptNewLine;
            }
            else
            {
               (*pPara) << _T("Slab Thickness (since there is no CIP deck, use top flange thickness): t") << Sub(_T("s")) << _T(" = ") << xdim2.SetValue(span_lldf.ts) << rptNewLine;
               (*pPara) << _T("Distance between CG of slab and girder: e") << Sub(_T("g")) <<_T(" = ") << xdim2.SetValue(span_lldf.eg) << rptNewLine;
            }
            (*pPara) << _T("Stiffness Parameter: ") << rptRcImage(strImagePath + _T("Kg.png")) << rptTab
                     << _T("K") << Sub(_T("g")) << _T(" = ") << inertia.SetValue(span_lldf.Kg) << rptNewLine;
         }
         else
         {
            (*pPara) << _T("Note that skew correction is undefined for this section type (see 4.6.2.2.2e)")<< rptNewLine;
         }
      }

      (*pPara) << _T("Skew Angle at start: ") << symbol(theta) << _T(" = ") << angle.SetValue(fabs(span_lldf.skew1)) << rptNewLine;
      (*pPara) << _T("Skew Angle at end: ") << symbol(theta) << _T(" = ") << angle.SetValue(fabs(span_lldf.skew2)) << rptNewLine;



      if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
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
      if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
      {
         pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
         (*pPara) << _T("Fatigue Limit States");
         (*pChapter) << pPara;
         pPara = new rptParagraph;
         (*pChapter) << pPara;

         std::_tstring superscript;

         rptRcScalar scalar2 = scalar;
         rptRcScalar scalar3 = scalar;

         Float64 mpf;

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
            mpf = pier1_lldf.gM1.GetMultiplePresenceFactor();
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
         mpf = span_lldf.gM1.GetMultiplePresenceFactor();
         (*pPara) << _T("g") << superscript << Sub(_T("Fatigue")) << _T(" = ") << _T("mg") << superscript << Sub(_T("1")) << _T("/m =") << scalar.SetValue(span_lldf.gM1.mg) << _T("/") << scalar3.SetValue(mpf) << _T(" = ") << scalar2.SetValue(span_lldf.gM1.mg/mpf);

         if ( bContinuousAtEnd || bIntegralAtEnd )
         {
            pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
            (*pChapter) << pPara;
            (*pPara) << _T("Distribution Factor for Negative Moment over Pier ") << LABEL_PIER(pier2) << rptNewLine;
            pPara = new rptParagraph;
            (*pChapter) << pPara;

            superscript = (pier2_lldf.bExteriorGirder ? _T("ME") : _T("MI"));
            mpf = pier2_lldf.gM1.GetMultiplePresenceFactor();
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

lrfdLiveLoadDistributionFactorBase* CMultiWebDistFactorEngineer::GetLLDFParameters(IndexType spanOrPierIdx,GirderIndexType gdrIdx,DFParam dfType,Float64 fcgdr,MULTIWEB_LLDFDETAILS* plldf)
{
   GET_IFACE(ISectionProperties, pSectProp);
   GET_IFACE(IGirder,            pGirder);
   GET_IFACE(IBridge,            pBridge);
   GET_IFACE(IBarriers,          pBarriers);
   GET_IFACE(IBridgeDescription, pIBridgeDesc);

   // Determine span/pier index... This is the index of a pier and the next span.
   // If this is the last pier, span index is for the last span
   SpanIndexType span = INVALID_INDEX;
   PierIndexType pier = INVALID_INDEX;
   SpanIndexType prev_span = INVALID_INDEX;
   SpanIndexType next_span = INVALID_INDEX;
   PierIndexType prev_pier = INVALID_INDEX;
   PierIndexType next_pier = INVALID_INDEX;
   GetIndicies(spanOrPierIdx,dfType,span,pier,prev_span,next_span,prev_pier,next_pier);

   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   ATLASSERT( pBridgeDesc->GetDistributionFactorMethod() != pgsTypes::DirectlyInput );

   const CDeckDescription2* pDeck = pBridgeDesc->GetDeckDescription();
   const CSpanData2* pSpan = pBridgeDesc->GetSpan(span);
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(pSpan);
   GirderIndexType nGirders = pGroup->GetGirderCount();

   if ( nGirders <= gdrIdx )
   {
      ATLASSERT(false);
      gdrIdx = nGirders-1;
   }

   CSpanKey spanKey(span, gdrIdx);

   // determine overhang, spacing base data, and controlling poi
   pgsPointOfInterest poi;
   GetGirderSpacingAndOverhang(spanKey, dfType, plldf, &poi);

   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   // Throws exception if fails requirement (no need to catch it)
   GET_IFACE(ILiveLoadDistributionFactors, pDistFactors);
   pDistFactors->VerifyDistributionFactorRequirements(poi);

   pgsPointOfInterest spPoi(poi); // section properties poi

   if (pGirder->CanTopFlangeBeLongitudinallyThickened(segmentKey))
   {
      // for thickened top flanges, we want to use the properties for the nominal (non-thickened) section
      pgsTypes::TopFlangeThickeningType topFlangeThickeningType = pGirder->GetTopFlangeThickeningType(segmentKey);
      Float64 tft = pGirder->GetTopFlangeThickening(segmentKey);
      if (topFlangeThickeningType == pgsTypes::tftEnds && !IsZero(tft))
      {
         GET_IFACE(IPointOfInterest, pPoi);
         PoiList vPoi;
         pPoi->GetPointsOfInterest(segmentKey, POI_ERECTED_SEGMENT | POI_5L, &vPoi);
         ATLASSERT(vPoi.size() == 1);
         spPoi = vPoi.front();
      }
      else if (topFlangeThickeningType == pgsTypes::tftMiddle && !IsZero(tft))
      {
         GET_IFACE(IPointOfInterest, pPoi);
         PoiList vPoi;
         pPoi->GetPointsOfInterest(segmentKey, POI_ERECTED_SEGMENT | POI_0L, &vPoi);
         ATLASSERT(vPoi.size() == 1);
         spPoi = vPoi.front();
      }
   }

   plldf->b = pGirder->GetTopFlangeWidth(spPoi); // for LRFD C4.6.2.2.1-1

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType llIntervalIdx      = pIntervals->GetLiveLoadInterval();

   // properties for computing J (see below)
   Float64 Ix, Iy, A;
   if (0 < fcgdr)
   {
      Ix = pSectProp->GetIxx(pgsTypes::sptGross,llIntervalIdx, spPoi,fcgdr);
      Iy = pSectProp->GetIyy(pgsTypes::sptGross,llIntervalIdx, spPoi,fcgdr);
      A  = pSectProp->GetAg(pgsTypes::sptGross,llIntervalIdx, spPoi,fcgdr);
      plldf->I = Ix;
   }
   else
   {
      Ix = pSectProp->GetIxx(pgsTypes::sptGross,llIntervalIdx, spPoi);
      Iy = pSectProp->GetIyy(pgsTypes::sptGross,llIntervalIdx, spPoi);
      A  = pSectProp->GetAg(pgsTypes::sptGross,llIntervalIdx, spPoi);
      plldf->I = Ix;
   }
   
   Float64 Yt = pSectProp->GetY(pgsTypes::sptGross,releaseIntervalIdx, spPoi,pgsTypes::TopGirder);

   plldf->PossionRatio = 0.2;

   pgsTypes::TrafficBarrierOrientation side = pBarriers->GetNearestBarrier(segmentKey);
   plldf->CurbOffset = pBarriers->GetInterfaceWidth(side);

   if ( IsNonstructuralDeck(pBridge->GetDeckType()) )
   {
      plldf->n = 1.0;
   }
   else
   {
      GET_IFACE(IMaterials, pMaterials);

      GET_IFACE(IPointOfInterest, pPoi);
      IndexType deckCastingRegionIdx = pPoi->GetDeckCastingRegion(spPoi);
      ATLASSERT(deckCastingRegionIdx != INVALID_INDEX);

      Float64 EcDeck = pMaterials->GetDeckEc(deckCastingRegionIdx,llIntervalIdx);
      if ( fcgdr < 0 )
      {
         // fcgdr < 0 means use the current bridge model
         Float64 EcSegment = pMaterials->GetSegmentEc(segmentKey,llIntervalIdx);
   
         plldf->n = EcSegment/EcDeck;
      }
      else
      {
         Float64 Ecgdr = pMaterials->GetEconc(pMaterials->GetSegmentConcreteType(segmentKey),fcgdr,
                                              pMaterials->GetSegmentStrengthDensity(segmentKey),
                                              pMaterials->GetSegmentEccK1(segmentKey),
                                              pMaterials->GetSegmentEccK2(segmentKey)
                                              );
   
         plldf->n = Ecgdr / EcDeck;
      }
   }

   // compute de (inside edge of barrier to CL of exterior web)
   Float64 wd = pGirder->GetCL2ExteriorWebDistance(poi); // cl beam to cl web

   // Note this is not exactly correct because opposite exterior beam might be different, but we won't be using this data set for that beam
   plldf->leftDe  = plldf->leftCurbOverhang - wd;  
   plldf->rightDe = plldf->rightCurbOverhang - wd; 

   plldf->L = GetEffectiveSpanLength(spanOrPierIdx,gdrIdx,dfType);

   plldf->A  = A;
   plldf->Yt = Yt;
   Float64 Ip = Ix + Iy;
   plldf->Ip = Ip;
   plldf->J = A*A*A*A/(40.0*Ip);

   // Non-composite girder properties
   plldf->Ag = pSectProp->GetAg(pgsTypes::sptGross,releaseIntervalIdx, spPoi);
   plldf->Ig = pSectProp->GetIxx(pgsTypes::sptGross,releaseIntervalIdx, spPoi);

   if (IsNonstructuralDeck(pBridge->GetDeckType()) )
   {
      plldf->ts = pGirder->GetMinTopFlangeThickness(spPoi);
      plldf->eg = plldf->Yt - plldf->ts/2;

      if (m_BeamType == IMultiWebDistFactorEngineer::btDeckBulbTee || m_BeamType == IMultiWebDistFactorEngineer::btMultiWebTee)
      {
         // Ag and Ig is just the stem
         // Get the top flange shape and subtract it from the total girder shape to get the stem by itself
         GET_IFACE(IShapes, pShapes);
         CComPtr<IShape> segment_shape;
         pShapes->GetSegmentShape(releaseIntervalIdx, spPoi, false, pgsTypes::scGirder, &segment_shape);

         CComQIPtr<IFlangePoints> flangePoints(segment_shape);
         if (flangePoints)
         {
            CComPtr<IPoint2d> leftTop, leftBottom, topCL, topCentral, rightTop, rightBottom;
            flangePoints->GetTopFlangePoints(&leftTop, &leftBottom, &topCL, &topCentral, &rightTop, &rightBottom);

            CComPtr<IPolyShape> flangeShape;
            flangeShape.CoCreateInstance(CLSID_PolyShape);
            flangeShape->AddPointEx(rightBottom);
            flangeShape->AddPointEx(rightTop);
            flangeShape->AddPointEx(topCentral);
            flangeShape->AddPointEx(leftTop);
            flangeShape->AddPointEx(leftBottom);

            CComQIPtr<IShape> shape(flangeShape);
            CComPtr<IShapeProperties> shape_props;
            shape->get_ShapeProperties(&shape_props);

            Float64 Aflange, Iflange, Ytflange, Ybflange;
            shape_props->get_Area(&Aflange);
            shape_props->get_Ixx(&Iflange);
            shape_props->get_Ytop(&Ytflange);
            shape_props->get_Ybottom(&Ybflange);
            Float64 Hflange = Ytflange + Ybflange;

            // Stem = Girder - Flange
            Float64 Astem = plldf->Ag - Aflange;
            Float64 Ystem = (plldf->Ag*plldf->Yt - Aflange*Ytflange) / Astem; // Treat Aflange as a "hole"... hence the subtraction.... Ystem is measured from top of original section

            // Use parallel axis theorem to get Istem about stem CGx axis
            Float64 Istem = plldf->Ig + plldf->Ag*(plldf->Yt - Ystem)*(plldf->Yt - Ystem) - (Iflange + Aflange*(Ytflange - Ystem)*(Ytflange - Ystem));

            plldf->Ag = Astem;
            plldf->Ig = Istem;
            plldf->Yt = Ystem - Hflange; // measured from the top of the stem
            plldf->ts = (plldf->ts + Hflange) / 2; // average top flange thickness
            plldf->eg = plldf->Yt + plldf->ts / 2;
         }
      }
   }
   else
   {
      Float64 ts = pBridge->GetStructuralSlabDepth(spPoi);

      // Fillet adds to ts, if applicable, since beams are adjacent so the slab width is the same as the top flange width
      if (pSectProp->GetHaunchAnalysisSectionPropertiesType() != pgsTypes::hspZeroHaunch)
      {
         Float64 fillet = pBridge->GetFillet();
         ts += fillet;
      }

      Float64 tf =  pGirder->GetMinTopFlangeThickness(spPoi);
      plldf->ts = ts + tf;

      // location of cg of combined slab wrt top of girder
      //
      // +===========================================+ ---
      // |                                           |  ^
      // |                                           |  |   
      // |                     # ---                 |  | ts (includes fillet if defined)
      // |                        |                  |  |
      // |    ---------------  *  |  ts/2            |  |
      // |      ^      tscg|      |                  |  v
      // +======|====================================+ ------------ First moment of area taken about this axis
      // |      |                 | -tf/2            |  ^      ^
      // |      | eg           # ---                 |  | tf   |
      // |      |                                    |  v      |
      // +------|---------+ . . .+-------------------+ ---     |
      //        |         |      |                             | Yt
      //        |         |      |                             |
      //        |         |      |                             |
      //        v         |      |                             v
      //     ------------ |  **  | -----------------------------
      //                  |      |
      //                  |      |
      //                 ---/\/\-----

      Float64 tscg = (ts*ts/2.0 - tf*tf/2.0)/(ts+tf);

      // Note that the below an approximation because Yt is based on the entire bare beam and perhaps we should ony
      // be using Yt of the web portion (which is hard to obtain). No use getting ridiculous with accuracy here though since it's all
      // taken to the 0.1th, and many permutations of beam types and shapes come through this code.
      plldf->eg    = plldf->Yt + tscg;
   }


   WebIndexType nWebs = pGirder->GetWebCount(segmentKey);
   plldf->connectedAsUnit = ( pDeck->TransverseConnectivity == pgsTypes::atcConnectedAsUnit ? true : false);

   lrfdLiveLoadDistributionFactorBase* pLLDF;

   if(plldf->Method == LLDF_TXDOT)
   {
      // TxDOT K factor depends on beam type
      Float64 K = this->GetTxDOTKfactor();

      pLLDF = new lrfdTxdotLldfMultiWeb(plldf->gdrNum,
                                        plldf->Savg, 
                                        plldf->gdrSpacings,
                                        plldf->leftCurbOverhang,
                                        plldf->rightCurbOverhang,
                                        plldf->Nl, 
                                        plldf->wLane,
                                        plldf->W,
                                        plldf->L,
                                        K,
                                        plldf->skew1,
                                        plldf->skew2);

      plldf->Kg = -1.0; // doesnt apply here. set bogus value
   }
   else
   {
      bool bSkew = !( IsZero(plldf->skew1) && IsZero(plldf->skew2) );

      bool bSkewMoment = bSkew;
      bool bSkewShear  = bSkew;

      if ( lrfdVersionMgr::SeventhEdition2014 <= lrfdVersionMgr::GetVersion() )
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

      // AASHTO and WSDOT are the same
      if ( plldf->connectedAsUnit )
      {
         lrfdLldfTypeAEKIJ* ldf;
         ldf   = new lrfdLldfTypeAEKIJ(plldf->gdrNum,
                                       plldf->Savg,
                                       plldf->gdrSpacings,
                                       plldf->leftCurbOverhang,
                                       plldf->rightCurbOverhang,
                                       plldf->Nl, 
                                       plldf->wLane,
                                       plldf->leftDe,
                                       plldf->rightDe,
                                       plldf->L,
                                       plldf->ts,
                                       plldf->n,
                                       plldf->Ig,
                                       plldf->Ag,
                                       plldf->eg,
                                       plldf->skew1,
                                       plldf->skew2,
                                       bSkewMoment,bSkewShear);
            
         plldf->Kg = ldf->GetKg();

         pLLDF = ldf;

      }
      else
      {
         pLLDF = new lrfdLldfTypeHIJ(plldf->gdrNum,
                                     plldf->Savg,
                                     plldf->gdrSpacings,
                                     plldf->leftCurbOverhang,
                                     plldf->rightCurbOverhang,
                                     plldf->Nl, 
                                     plldf->wLane,
                                     plldf->L,
                                     plldf->W,
                                     plldf->I,
                                     plldf->J,
                                     plldf->PossionRatio,
                                     plldf->leftDe,
                                     plldf->rightDe,
                                     plldf->skew1,
                                     plldf->skew2,
                                     bSkewMoment, bSkewShear);

         plldf->Kg = -1.0; // doesnt apply here. set bogus value
      }
   }

   GET_IFACE(ILiveLoads,pLiveLoads);
   pLLDF->SetRangeOfApplicabilityAction( pLiveLoads->GetLldfRangeOfApplicabilityAction() );
   plldf->bExteriorGirder = pBridge->IsExteriorGirder(segmentKey);

   return pLLDF;
}

void CMultiWebDistFactorEngineer::ReportMoment(rptParagraph* pPara,MULTIWEB_LLDFDETAILS& lldf,lrfdILiveLoadDistributionFactor::DFResult& gM1,lrfdILiveLoadDistributionFactor::DFResult& gM2,Float64 gM,bool bSIUnits,IEAFDisplayUnits* pDisplayUnits)
{
   std::_tstring strImagePath(rptStyleManager::GetImagePath());

   INIT_SCALAR_PROTOTYPE(rptRcScalar, scalar, pDisplayUnits->GetScalarFormat());

   GET_IFACE(ILibrary, pLib);
   GET_IFACE(ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

   INIT_UV_PROTOTYPE( rptLengthUnitValue,    location, pDisplayUnits->GetSpanLengthUnit(),      true );
   INIT_UV_PROTOTYPE( rptAreaUnitValue,      area,     pDisplayUnits->GetAreaUnit(),            true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,    xdim,     pDisplayUnits->GetSpanLengthUnit(),      true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,    xdim2,    pDisplayUnits->GetComponentDimUnit(),    true );
   INIT_UV_PROTOTYPE( rptLength4UnitValue,   inertia,  pDisplayUnits->GetMomentOfInertiaUnit(), true );
   INIT_UV_PROTOTYPE( rptAngleUnitValue,     angle,    pDisplayUnits->GetAngleUnit(),           true );

   if ( lldf.bExteriorGirder )
   {
      // Distribution factor for exterior girder
      if (lldf.Method == LLDF_TXDOT && !(gM1.ControllingMethod & LEVER_RULE || gM1.ControllingMethod & LANES_DIV_BEAMS))
      {
         std::_tstring msg(!(gM1.ControllingMethod & SPECIAL_OVERRIDE)?_T("Spec Equation, same as for interior single lane."):_T("Controlled by S/10.0"));

         (*pPara) << Bold(_T("1 Loaded Lane: "))<< Bold(msg) << rptNewLine;
         if(!(gM1.ControllingMethod & SPECIAL_OVERRIDE))
         {
            ATLASSERT(gM1.ControllingMethod & S_OVER_D_METHOD);
            (*pPara)<< _T("K = ")<< gM1.EqnData.K << rptNewLine;
            (*pPara)<< _T("C = ")<< gM1.EqnData.C << rptNewLine;
            (*pPara)<< _T("D = ")<< xdim.SetValue(gM1.EqnData.D) << rptNewLine<< rptNewLine;
         }

         (*pPara) << _T("mg") << Super(_T("ME")) << Sub(_T("1")) << _T(" = ") << scalar.SetValue(gM1.mg) << rptNewLine;
         (*pPara) << rptNewLine;
         if ( 2 <= lldf.Nl )
         {
            (*pPara) << Bold(_T("2+ Loaded Lanes: "))<< Bold(msg) << rptNewLine;
            (*pPara) << _T("mg") << Super(_T("ME")) << Sub(_T("2+")) << _T(" = ") << scalar.SetValue(gM2.mg) << rptNewLine;
            (*pPara) << rptNewLine;
         }

         (*pPara) << Bold(_T("Skew Correction Ignored for TxDOT Method")) << rptNewLine;
         (*pPara) << _T("Corrected Factor: mg") << Super(_T("ME")) << Sub(_T("1")) << _T(" = ") << scalar.SetValue(gM1.mg);
         (lldf.Nl == 1 || gM2.mg <= gM1.mg) ? (*pPara) << Bold(_T(" < Controls")) << rptNewLine : (*pPara) << rptNewLine;
         if ( 2 <= lldf.Nl )
         {
            (*pPara) << _T("Corrected Factor: mg") << Super(_T("ME")) << Sub(_T("2+")) << _T(" = ") << scalar.SetValue(gM2.mg);
            (gM1.mg < gM2.mg) ? (*pPara) << Bold(_T(" < Controls")) << rptNewLine : (*pPara) << rptNewLine;
         }
      }
      else
      {
         if ( gM1.LeverRuleData.bWasUsed )
         {
            (*pPara) << Bold(_T("1 Loaded Lane: Lever Rule")) << rptNewLine;
            REPORT_LLDF_INTOVERRIDE(gM1);
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
            if ( gM2.EqnData.bWasUsed )
            {
               (*pPara) << Bold(_T("2+ Loaded Lanes: Spec Equation")) << rptNewLine;

               (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("mg_2_ME_Type_K_SI.png") : _T("mg_2_ME_Type_K_US.png"))) << rptNewLine;
               (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("mg_2_MI_Type_K_SI.png") : _T("mg_2_MI_Type_K_US.png"))) << rptNewLine;
               (*pPara) << _T("mg") << Super(_T("MI")) << Sub(_T("2+")) << _T(" = ") << scalar.SetValue(gM2.EqnData.mg) << rptNewLine;
               (*pPara) << _T("e = ") << scalar.SetValue(gM2.EqnData.e) << rptNewLine;

               (*pPara) << _T("mg") << Super(_T("ME")) << Sub(_T("2+")) << _T(" = ") << scalar.SetValue(gM2.EqnData.mg*gM2.EqnData.e) << rptNewLine;
            }

            if ( gM2.LeverRuleData.bWasUsed )
            {
               (*pPara) << Bold(_T("2+ Loaded Lanes: Lever Rule")) << rptNewLine;
               REPORT_LLDF_INTOVERRIDE(gM2);
               ReportLeverRule(pPara,true,1.0,gM2.LeverRuleData,m_pBroker,pDisplayUnits);
            }

            if ( gM2.RigidData.bWasUsed )
            {
               (*pPara) << Bold(_T("2+ Loaded Lanes: Rigid Method")) << rptNewLine;
               REPORT_LLDF_INTOVERRIDE(gM2);
               ReportRigidMethod(pPara,gM2.RigidData,m_pBroker,pDisplayUnits);
            }

            if ( gM2.LeverRuleData.bWasUsed && gM2.EqnData.bWasUsed )
            {
               (*pPara) << rptNewLine;
               (*pPara) << _T("Controlling value is lesser of Equation and Lever Rule mg") << Super(_T("ME")) << Sub(_T("2+")) << _T(" = ") << scalar.SetValue(gM2.mg) << rptNewLine;
            }

            if ( gM2.LanesBeamsData.bWasUsed )
            {
               (*pPara) << Bold(_T("2+ Loaded Lane: Number of Lanes over Number of Beams - Factor cannot be less than this")) << rptNewLine;
               (*pPara) << _T("Skew correction is not applied to Lanes/Beams method")<< rptNewLine;
               ReportLanesBeamsMethod(pPara,gM2.LanesBeamsData,m_pBroker,pDisplayUnits);
            }
         }

         (*pPara) << rptNewLine;

         if (lldf.connectedAsUnit)
         {
            if ( gM1.ControllingMethod & MOMENT_SKEW_CORRECTION_APPLIED )
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
            (*pPara) << _T("Controlling mg") << Super(_T("ME"))  << _T(" = ") << scalar.SetValue(Max(gM1.mg,gM2.mg)) << rptNewLine;
         }
      }

      (*pPara) << rptNewLine;

   }
   else
   {
      // Distribution factor for interior girder
      if (lldf.Method == LLDF_TXDOT && !(gM1.ControllingMethod & LEVER_RULE || gM1.ControllingMethod & LANES_DIV_BEAMS))
      {
         std::_tstring msg(!(gM1.ControllingMethod & SPECIAL_OVERRIDE)?_T("Spec Equation"):_T("Controlled by S/10.0"));

         (*pPara) << Bold(_T("1 Loaded Lane: "))<< Bold(msg) << rptNewLine;
         if (gM1.ControllingMethod & S_OVER_D_METHOD)
         {
            (*pPara)<< _T("K = ")<< gM1.EqnData.K << rptNewLine;
            (*pPara)<< _T("C = ")<< gM1.EqnData.C << rptNewLine;
            (*pPara)<< _T("D = ")<< xdim.SetValue(gM1.EqnData.D) << rptNewLine<< rptNewLine;
         }

         (*pPara) << _T("mg") << Super(_T("MI")) << Sub(_T("1")) << _T(" = ") << scalar.SetValue(gM1.mg) << rptNewLine;
         (*pPara) << rptNewLine;
         if ( 2 <= lldf.Nl )
         {
            (*pPara) << Bold(_T("2+ Loaded Lanes: "))<< Bold(msg) << Bold(_T(", same as for interior single lane moment.")) << rptNewLine;
            (*pPara) << _T("mg") << Super(_T("MI")) << Sub(_T("2+")) << _T(" = ") << scalar.SetValue(gM2.mg) << rptNewLine;
            (*pPara) << rptNewLine;
         }

         (*pPara) << Bold(_T("Skew Correction Ignored for TxDOT Method")) << rptNewLine;
         (*pPara) << _T("Corrected Factor: mg") << Super(_T("ME")) << Sub(_T("1")) << _T(" = ") << scalar.SetValue(gM1.mg);
         (lldf.Nl == 1 || gM2.mg <= gM1.mg) ? (*pPara) << Bold(_T(" < Controls")) << rptNewLine : (*pPara) << rptNewLine;
         if ( 2 <= lldf.Nl )
         {
            (*pPara) << _T("Corrected Factor: mg") << Super(_T("ME")) << Sub(_T("2+")) << _T(" = ") << scalar.SetValue(gM2.mg);
            (gM1.mg < gM2.mg) ? (*pPara) << Bold(_T(" < Controls")) << rptNewLine : (*pPara) << rptNewLine;
         }
      }
      else
      {

         if(lldf.connectedAsUnit)
         {
            if (gM1.EqnData.bWasUsed )
            {
               (*pPara) << Bold(_T("1 Loaded Lane: Spec Equations")) << rptNewLine;
               (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("mg_1_MI_Type_K_SI.png") : _T("mg_1_MI_Type_K_US.png"))) << rptNewLine;
               (*pPara) << _T("mg") << Super(_T("MI")) << Sub(_T("1")) << _T(" = ") << scalar.SetValue(gM1.EqnData.mg) << rptNewLine;
            }

            if ( gM1.LeverRuleData.bWasUsed )
            {
               (*pPara) << Bold(_T("1 Loaded Lane: Lever Rule")) << rptNewLine;
               ReportLeverRule(pPara,true,1.0,gM1.LeverRuleData,m_pBroker,pDisplayUnits);
            }

            if ( gM1.EqnData.bWasUsed && gM1.LeverRuleData.bWasUsed )
            {
               (*pPara) << rptNewLine;
               (*pPara) << _T("Controlling value is lesser of Equation and Lever Rule mg") << Super(_T("ME")) << Sub(_T("1")) << _T(" = ") << scalar.SetValue(gM1.mg) << rptNewLine;
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

               if (gM2.EqnData.bWasUsed && gM2.LeverRuleData.bWasUsed )
               {
                  (*pPara) << rptNewLine;
                  (*pPara) << _T("Controlling value is lesser of Equation and Lever Rule mg") << Super(_T("ME")) << Sub(_T("2+")) << _T(" = ") << scalar.SetValue(gM2.mg) << rptNewLine;
               }

               if ( gM2.LanesBeamsData.bWasUsed )
               {
                  (*pPara) << Bold(_T("2+ Loaded Lanes: Number of Lanes over Number of Beams - Factor cannot be less than this")) << rptNewLine;
                  (*pPara) << _T("Skew correction is not applied to Lanes/Beams method")<< rptNewLine;
                  ReportLanesBeamsMethod(pPara,gM2.LanesBeamsData,m_pBroker,pDisplayUnits);
               }

               (*pPara) << rptNewLine;
            }
         }
         else
         {
            if (gM1.LeverRuleData.bWasUsed)
            {
               (*pPara) << Bold(_T("1 Loaded Lane: Lever Rule")) << rptNewLine;
               ReportLeverRule(pPara,true,1.0,gM1.LeverRuleData,m_pBroker,pDisplayUnits);
            }

            if (gM1.EqnData.bWasUsed)
            {
               (*pPara) << Bold(_T("St. Venant torsional inertia constant")) << rptNewLine;
               (*pPara) << _T("Polar Moment of Inertia: I") << Sub(_T("p")) << _T(" = ") << inertia.SetValue(lldf.Ip) << rptNewLine;
               (*pPara) << _T("Torsional Constant: ") << rptRcImage(strImagePath + _T("J.png")) << rptTab
                        << _T("J") << _T(" = ") << inertia.SetValue(lldf.J) << rptNewLine;

               (*pPara) << Bold(_T("Regardless of the Number of Loaded Lanes:")) << rptNewLine;
               (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("LLDF_Type_HIJ_SI.png") : _T("LLDF_Type_HIJ_US.png"))) << rptNewLine;

               if (gM1.ControllingMethod & S_OVER_D_METHOD)
               {
                  (*pPara) << rptNewLine;
                  (*pPara)<< _T("K = ")<< gM1.EqnData.K << rptNewLine;
                  (*pPara)<< _T("C = ")<< gM1.EqnData.C << rptNewLine;
                  (*pPara)<< _T("D = ")<< xdim.SetValue(gM1.EqnData.D) << rptNewLine;
               }

               (*pPara) << _T("mg") << Super(_T("MI")) << Sub(_T("1")) << _T(" = ") 
                        << _T("mg") << Super(_T("MI")) << Sub(_T("2+")) << _T(" = ") << scalar.SetValue(gM1.EqnData.mg) << rptNewLine;
            }

            if ( gM1.LanesBeamsData.bWasUsed )
            {
               (*pPara) << Bold(_T("1 Loaded Lane: Number of Lanes over Number of Beams - Factor cannot be less than this")) << rptNewLine;
               (*pPara) << _T("Skew correction is not applied to Lanes/Beams method")<< rptNewLine;
               ReportLanesBeamsMethod(pPara,gM1.LanesBeamsData,m_pBroker,pDisplayUnits);
            }

            if ( 2 <= lldf.Nl && gM2.LanesBeamsData.bWasUsed )
            {
               (*pPara) << Bold(_T("2+ Loaded Lane: Number of Lanes over Number of Beams - Factor cannot be less than this")) << rptNewLine;
               (*pPara) << _T("Skew correction is not applied to Lanes/Beams method")<< rptNewLine;
               ReportLanesBeamsMethod(pPara,gM2.LanesBeamsData,m_pBroker,pDisplayUnits);
            }
         }

         (*pPara) << rptNewLine;

         if (lldf.connectedAsUnit)
         {
            if ( gM1.ControllingMethod & MOMENT_SKEW_CORRECTION_APPLIED )
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
            (*pPara) << _T("Controlling mg") << Super(_T("MI"))  << _T(" = ") << scalar.SetValue(Max(gM1.mg,gM2.mg)) << rptNewLine;
         }
      }

      (*pPara) << rptNewLine;
   }
}

void CMultiWebDistFactorEngineer::ReportShear(rptParagraph* pPara,MULTIWEB_LLDFDETAILS& lldf,lrfdILiveLoadDistributionFactor::DFResult& gV1,lrfdILiveLoadDistributionFactor::DFResult& gV2,Float64 gV,bool bSIUnits,IEAFDisplayUnits* pDisplayUnits)
{
   std::_tstring strImagePath(rptStyleManager::GetImagePath());

   INIT_SCALAR_PROTOTYPE(rptRcScalar, scalar, pDisplayUnits->GetScalarFormat());

   GET_IFACE(ILibrary, pLib);
   GET_IFACE(ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

   Int16 df_method = pSpecEntry->GetLiveLoadDistributionMethod();

   if ( lldf.bExteriorGirder )
   {
      if (lldf.Method == LLDF_TXDOT && !(gV1.ControllingMethod & LEVER_RULE || gV1.ControllingMethod & LANES_DIV_BEAMS))
      {
         std::_tstring msg(!(gV1.ControllingMethod & SPECIAL_OVERRIDE)?_T("Spec Equation, same as for interior single lane moment."):_T("Controlled by S/10.0"));

         (*pPara) << Bold(_T("1 Loaded Lane: "))<< Bold(msg) << rptNewLine;
         (*pPara) << _T("mg") << Super(_T("VE")) << Sub(_T("1")) << _T(" = ") << scalar.SetValue(gV1.mg) << rptNewLine;
         (*pPara) << rptNewLine;
         if ( 2 <= lldf.Nl )
         {
            (*pPara) << Bold(_T("2+ Loaded Lanes: "))<< Bold(msg) << rptNewLine;
            (*pPara) << _T("mg") << Super(_T("VE")) << Sub(_T("2+")) << _T(" = ") << scalar.SetValue(gV2.mg) << rptNewLine;
            (*pPara) << rptNewLine;
         }

         (*pPara) << Bold(_T("Skew Correction Ignored for TxDOT Method")) << rptNewLine;
         (*pPara) << _T("Corrected Factor: mg") << Super(_T("VE")) << Sub(_T("1")) << _T(" = ") << scalar.SetValue(gV1.mg);
         (lldf.Nl == 1 || gV2.mg <= gV1.mg) ? (*pPara) << Bold(_T(" < Controls")) << rptNewLine : (*pPara) << rptNewLine;
         if ( 2 <= lldf.Nl )
         {
            (*pPara) << _T("Corrected Factor: mg") << Super(_T("VE")) << Sub(_T("2+")) << _T(" = ") << scalar.SetValue(gV2.mg);
            (gV1.mg < gV2.mg) ? (*pPara) << Bold(_T(" < Controls")) << rptNewLine : (*pPara) << rptNewLine;
         }

      }
      else
      {

         if ( gV1.EqnData.bWasUsed )
         {
            ATLASSERT(gV1.ControllingMethod & INTERIOR_OVERRIDE); // should always be the case since equation data not normally used on exterior beam
            (*pPara) << Bold(_T("1 Loaded Lane: Spec Equations")) << rptNewLine;
            REPORT_LLDF_INTOVERRIDE(gV1);
            (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("mg_1_VI_Type_K_SI.png") : _T("mg_1_VI_Type_K_US.png"))) << rptNewLine;
            (*pPara) << _T("mg") << Super(_T("VE")) << Sub(_T("1")) << _T(" = ") <<_T("mg") << Super(_T("VI")) << Sub(_T("1")) << _T(" = ") << scalar.SetValue(gV1.EqnData.mg) << rptNewLine;
         }

         if ( gV1.LeverRuleData.bWasUsed )
         {
            (*pPara) << Bold(_T("1 Loaded Lane: Lever Rule")) << rptNewLine;
            REPORT_LLDF_INTOVERRIDE(gV1);
            ReportLeverRule(pPara,false,1.0,gV1.LeverRuleData,m_pBroker,pDisplayUnits);
         }

         if ( gV1.RigidData.bWasUsed )
         {
            (*pPara) << Bold(_T("1 Loaded Lane: Rigid Method")) << rptNewLine;
            REPORT_LLDF_INTOVERRIDE(gV1);
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
            if ( gV2.EqnData.bWasUsed )
            {
               (*pPara) << Bold(_T("2+ Loaded Lanes: Spec Equation")) << rptNewLine;
               REPORT_LLDF_INTOVERRIDE(gV2);
               (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("mg_2_VE_Type_K_SI.png") : _T("mg_2_VE_Type_K_US.png"))) << rptNewLine;
               (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("mg_2_VI_Type_K_SI.png") : _T("mg_2_VI_Type_K_US.png"))) << rptNewLine;
               (*pPara) << _T("mg") << Super(_T("VI")) << Sub(_T("2+")) << _T(" = ") << scalar.SetValue(gV2.EqnData.mg) << rptNewLine;
               (*pPara) << _T("e = ") << scalar.SetValue(gV2.EqnData.e) << rptNewLine;

               (*pPara) << _T("mg") << Super(_T("VE")) << Sub(_T("2+")) << _T(" = ") << scalar.SetValue(gV2.EqnData.mg*gV2.EqnData.e) << rptNewLine;
            }

            if ( gV2.LeverRuleData.bWasUsed )
            {
               (*pPara) << Bold(_T("2+ Loaded Lanes: Lever Rule")) << rptNewLine;
               REPORT_LLDF_INTOVERRIDE(gV2);
               ReportLeverRule(pPara,false,1.0,gV2.LeverRuleData,m_pBroker,pDisplayUnits);
            }

            if ( gV2.RigidData.bWasUsed )
            {
               (*pPara) << Bold(_T("2+ Loaded Lanes: Rigid Method")) << rptNewLine;
               REPORT_LLDF_INTOVERRIDE(gV2);
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

         (*pPara) << rptNewLine;
         if (lldf.connectedAsUnit)
         {
            if ( gV1.ControllingMethod & SHEAR_SKEW_CORRECTION_APPLIED )
            {
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
            // skew makes no sense for unconnected
            (*pPara) << _T("Controlling Factor: mg") << Super(_T("VE")) << _T(" = ") << scalar.SetValue(Max(gV1.mg,gV2.mg));
         }
      }
   }
   else
   {
      // Distribution factor for interior girder
      if (lldf.Method == LLDF_TXDOT && !(gV1.ControllingMethod & LEVER_RULE || gV1.ControllingMethod & LANES_DIV_BEAMS))
      {
         std::_tstring msg(!(gV1.ControllingMethod & SPECIAL_OVERRIDE)?_T("Spec Equation, same as for interior single lane moment."):_T("Controlled by S/10.0"));

         (*pPara) << Bold(_T("1 Loaded Lane: "))<< Bold(msg) << rptNewLine;
         (*pPara) << _T("mg") << Super(_T("VI")) << Sub(_T("1")) << _T(" = ") << scalar.SetValue(gV1.mg) << rptNewLine;
         (*pPara) << rptNewLine;
         if ( 2 <= lldf.Nl )
         {
            (*pPara) << Bold(_T("2+ Loaded Lanes: "))<< Bold(msg) << rptNewLine;
            (*pPara) << _T("mg") << Super(_T("VI")) << Sub(_T("2+")) << _T(" = ") << scalar.SetValue(gV2.mg) << rptNewLine;
            (*pPara) << rptNewLine;
         }

         (*pPara) << Bold(_T("Skew Correction Ignored for TxDOT Method")) << rptNewLine;
         (*pPara) << _T("Corrected Factor: mg") << Super(_T("VE")) << Sub(_T("1")) << _T(" = ") << scalar.SetValue(gV1.mg);
         (lldf.Nl == 1 || gV2.mg <= gV1.mg) ? (*pPara) << Bold(_T(" < Controls")) << rptNewLine : (*pPara) << rptNewLine;
         if ( 2 <= lldf.Nl )
         {
            (*pPara) << _T("Corrected Factor: mg") << Super(_T("VE")) << Sub(_T("2+")) << _T(" = ") << scalar.SetValue(gV2.mg);
            (gV1.mg < gV2.mg) ? (*pPara) << Bold(_T(" < Controls")) << rptNewLine : (*pPara) << rptNewLine;
         }

      }
      else
      {

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

         if ( 2 <= lldf.Nl )
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

         if (lldf.connectedAsUnit)
         {
            if ( gV1.ControllingMethod & SHEAR_SKEW_CORRECTION_APPLIED )
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
         else
         {
            // skew makes no sense for unconnected
            (*pPara) << _T("Controlling Factor: mg") << Super(_T("VE")) << _T(" = ") << scalar.SetValue(Max(gV1.mg,gV2.mg));
         }
      }
   }
}

std::_tstring CMultiWebDistFactorEngineer::GetComputationDescription(const CGirderKey& girderKey,const std::_tstring& libraryEntryName,pgsTypes::SupportedDeckType decktype, pgsTypes::AdjacentTransverseConnectivity connect)
{
   GET_IFACE(ILibrary, pLib);
   GET_IFACE(ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

   Int16 lldfMethod = pSpecEntry->GetLiveLoadDistributionMethod();

   std::_tostringstream osdescr;

   if ( lldfMethod == LLDF_TXDOT )
   {
      osdescr <<_T("TxDOT modifications per TxDOT Bridge Design Manual - LRFD");
   }
   else if ( lldfMethod == LLDF_WSDOT || lldfMethod == LLDF_LRFD )
   {
      if (lldfMethod == LLDF_WSDOT)
      {
         osdescr << _T("WSDOT Method per Bridge Design Manual Section 3.9.3A. Using type (i,j) cross section ");
      }
      else
      {
         osdescr << _T("AASHTO LRFD Method per Article 4.6.2.2. Using type (i,j) cross section ");
      }

      if (connect == pgsTypes::atcConnectedAsUnit)
      {
         osdescr << _T("connected transversely sufficiently to act as a unit.");
      }
      else
      {
         osdescr << _T("connected transversely only enough to prevent relative vertical displacement along interface.");
      }
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
      osdescr << strAction;
   }

   return osdescr.str();
}

IMultiWebDistFactorEngineer::BeamType CMultiWebDistFactorEngineer::GetBeamType()
{
   return m_BeamType;
}

void CMultiWebDistFactorEngineer::SetBeamType(IMultiWebDistFactorEngineer::BeamType bt)
{
   m_BeamType = bt;
}
