///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2018  Washington State Department of Transportation
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

// UBeamDistFactorEngineer.cpp : Implementation of CUBeamDistFactorEngineer
#include "stdafx.h"
#include "UBeamDistFactorEngineer.h"
#include "..\PGSuperException.h"
#include <Units\SysUnits.h>
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
#include <Beams\Helper.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// pre-convert constant values for performance
static const Float64 D_18 = ::ConvertToSysUnits(18., unitMeasure::Inch);

/////////////////////////////////////////////////////////////////////////////
// CIBeamFactory
void CUBeamDistFactorEngineer::Init(bool bTypeB, bool bisSpreadSlab)
{
   m_bTypeB = bTypeB;
   m_bIsSpreadSlab = bisSpreadSlab;
}

HRESULT CUBeamDistFactorEngineer::FinalConstruct()
{
   return S_OK;
}

void CUBeamDistFactorEngineer::BuildReport(const CGirderKey& girderKey,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits)
{
   // Grab the interfaces that are needed
   GET_IFACE(IBridge,pBridge);

   GET_IFACE(ILibrary, pLib);
   GET_IFACE(ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   Int16 lldfMethod = pSpecEntry->GetLiveLoadDistributionMethod();

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

      REACTIONDETAILS reaction1_lldf, reaction2_lldf;
      GetPierReactionDF(pier1, gdrIdx, pgsTypes::StrengthI, USE_CURRENT_FC, &reaction1_lldf);
      GetPierReactionDF(pier2, gdrIdx, pgsTypes::StrengthI, USE_CURRENT_FC, &reaction2_lldf);

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

      Float64 station,offset;
      CSegmentKey segmentKey(girderKey,0);
      pBridge->GetStationAndOffset(pgsPointOfInterest(segmentKey,span_lldf.ControllingLocation),&station, &offset);
      Float64 supp_dist = span_lldf.ControllingLocation - pBridge->GetSegmentStartEndDistance(segmentKey);
      (*pPara) << _T("Deck Width, Girder Spacing and Deck Overhang are measured along a line that is normal to the alignment and passing through a point ") << location.SetValue(supp_dist) << _T(" from the left support along the centerline of girder. ");
      (*pPara) << _T("The measurement line passes through Station ") << rptRcStation(station, &pDisplayUnits->GetStationFormat() ) << _T(" (") << RPT_OFFSET(offset,offsetFormatter) << _T(")") << rptNewLine;
      (*pPara) << _T("Girder Spacing: ") << Sub2(_T("S"),_T("avg")) << _T(" = ") << xdim.SetValue(span_lldf.Savg) << rptNewLine;
      (*pPara) << _T("Bridge Width: W = ") << xdim.SetValue(span_lldf.W) << rptNewLine;
      (*pPara) << _T("Roadway Width: w = ") << xdim.SetValue(span_lldf.wCurbToCurb) << rptNewLine;
      (*pPara) << _T("Number of Design Lanes: N") << Sub(_T("L")) << _T(" = ") << span_lldf.Nl << rptNewLine;
      (*pPara) << _T("Lane Width: wLane = ") << xdim.SetValue(span_lldf.wLane) << rptNewLine;
      (*pPara) << _T("Number of Girders: N") << Sub(_T("b")) << _T(" = ") << span_lldf.Nb << rptNewLine;

      if ( m_bIsSpreadSlab && lldfMethod==LLDF_TXDOT && (span_lldf.d < D_18) )
      {
         (*pPara) << _T("Girder Depth = ") << xdim2.SetValue(span_lldf.d) << _T(", which is less than 18 in. In accordance to TxDOT design specifications for spread slab beams, the value of d will be pinned to 18 inches.") << rptNewLine;
         (*pPara) << _T("d = ") << xdim2.SetValue(D_18) << rptNewLine;
      }
      else
      {
         (*pPara) << _T("Girder Depth: d = ") << xdim2.SetValue(span_lldf.d) << rptNewLine;
      }

      Float64 de = span_lldf.Side==dfLeft ? span_lldf.leftDe:span_lldf.rightDe;
      (*pPara) << _T("Distance from exterior web of exterior beam to curb line: d") << Sub(_T("e")) << _T(" = ") << xdim.SetValue(de) << rptNewLine;

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
         ReportMoment(spanIdx, pPara,
                      pier1_lldf,
                      pier1_lldf.gM1,
                      pier1_lldf.gM2,
                      pier1_lldf.gM,
                      bSIUnits,pDisplayUnits);
      }

      // Positive moment DF
      pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
      (*pChapter) << pPara;
      if ( bContinuousAtStart || bContinuousAtEnd || bIntegralAtStart || bIntegralAtEnd )
         (*pPara) << _T("Distribution Factor for Positive and Negative Moment in Span ") << LABEL_SPAN(spanIdx) << rptNewLine;
      else
         (*pPara) << _T("Distribution Factor for Positive Moment in Span ") << LABEL_SPAN(spanIdx) << rptNewLine;
      pPara = new rptParagraph;
      (*pChapter) << pPara;

      (*pPara) << _T("Average Skew Angle: ") << symbol(theta) << _T(" = ") << angle.SetValue(fabs((span_lldf.skew1 + span_lldf.skew2)/2)) << rptNewLine;
      (*pPara) << _T("Span Length: L = ") << xdim.SetValue(span_lldf.L) << rptNewLine << rptNewLine;

      ReportMoment(spanIdx, pPara,
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
         ReportMoment(spanIdx, pPara,
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

      ReportShear(spanIdx, pPara,
                  span_lldf,
                  span_lldf.gV1,
                  span_lldf.gV2,
                  span_lldf.gV,
                  bSIUnits,pDisplayUnits);

      //////////////////////////////////////////////////////////////
      // Reactions
      //////////////////////////////////////////////////////////////
      pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
      (*pChapter) << pPara;
      (*pPara) << _T("Distribution Factor for Reaction at Pier ") << LABEL_PIER(pier1) << rptNewLine;
      pPara = new rptParagraph;
      (*pChapter) << pPara;

      (*pPara) << _T("Average Skew Angle: ") << symbol(theta) << _T(" = ") << angle.SetValue(fabs((reaction1_lldf.skew1 + reaction1_lldf.skew2)/2)) << rptNewLine;
      (*pPara) << _T("Span Length: L = ") << xdim.SetValue(reaction1_lldf.L) << rptNewLine << rptNewLine;

      ReportShear(spanIdx, pPara,
                  reaction1_lldf,
                  reaction1_lldf.gR1,
                  reaction1_lldf.gR2,
                  reaction1_lldf.gR,
                  bSIUnits,pDisplayUnits);

        ///////

      pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
      (*pChapter) << pPara;
      (*pPara) << _T("Distribution Factor for Reaction at Pier ") << LABEL_PIER(pier2) << rptNewLine;
      pPara = new rptParagraph;
      (*pChapter) << pPara;

      (*pPara) << _T("Average Skew Angle: ") << symbol(theta) << _T(" = ") << angle.SetValue(fabs((reaction2_lldf.skew1 + reaction2_lldf.skew2)/2)) << rptNewLine;
      (*pPara) << _T("Span Length: L = ") << xdim.SetValue(reaction2_lldf.L) << rptNewLine << rptNewLine;

      ReportShear(spanIdx, pPara,
                  reaction2_lldf,
                  reaction2_lldf.gR1,
                  reaction2_lldf.gR2,
                  reaction2_lldf.gR,
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

         //////////////////////////////////////////////////////////////
         // Reactions
         //////////////////////////////////////////////////////////////
         pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
         (*pChapter) << pPara;
         (*pPara) << _T("Distribution Factor for Reaction at Pier ") << LABEL_PIER(pier1) << rptNewLine;
         pPara = new rptParagraph;
         (*pChapter) << pPara;

         superscript = (reaction1_lldf.bExteriorGirder ? _T("VE") : _T("VI"));
         mpf = reaction1_lldf.gR1.GetMultiplePresenceFactor();
         (*pPara) << _T("g") << superscript << Sub(_T("Fatigue")) << _T(" = ") << _T("mg") << superscript << Sub(_T("1")) << _T("/m =") << scalar.SetValue(reaction1_lldf.gR1.mg) << _T("/") << scalar3.SetValue(mpf) << _T(" = ") << scalar2.SetValue(reaction1_lldf.gR1.mg/mpf);

           ///////

         pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
         (*pChapter) << pPara;
         (*pPara) << _T("Distribution Factor for Reaction at Pier ") << LABEL_PIER(pier2) << rptNewLine;
         pPara = new rptParagraph;
         (*pChapter) << pPara;

         superscript = (reaction2_lldf.bExteriorGirder ? _T("VE") : _T("VI"));
         mpf = reaction2_lldf.gR1.GetMultiplePresenceFactor();
         (*pPara) << _T("g") << superscript << Sub(_T("Fatigue")) << _T(" = ") << _T("mg") << superscript << Sub(_T("1")) << _T("/m =") << scalar.SetValue(reaction2_lldf.gR1.mg) << _T("/") << scalar3.SetValue(mpf) << _T(" = ") << scalar2.SetValue(reaction2_lldf.gR1.mg/mpf);
      }
   } // next span
}


lrfdLiveLoadDistributionFactorBase* CUBeamDistFactorEngineer::GetLLDFParameters(IndexType spanOrPierIdx,GirderIndexType gdrIdx,DFParam dfType,Float64 fcgdr,UBEAM_LLDFDETAILS* plldf)
{
   GET_IFACE(IGirder, pGdr);
   GET_IFACE(IBarriers,pBarriers);
   GET_IFACE(IBridgeDescription,pIBridgeDesc);

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

   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(pBridgeDesc->GetSpan(span));
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

   pgsTypes::TrafficBarrierOrientation side = pBarriers->GetNearestBarrier(segmentKey);
   Float64 curb_offset = pBarriers->GetInterfaceWidth(side);

   // compute de (inside edge of barrier to CL of exterior web)
   Float64 wd = pGdr->GetCL2ExteriorWebDistance(poi); // cl beam to cl web

   // Note this is not exactly correct because opposite exterior beam might be different, but we won't be using this data set for that beam
   plldf->leftDe  = plldf->leftCurbOverhang  - wd;  
   plldf->rightDe = plldf->rightCurbOverhang - wd; 

   plldf->d = pGdr->GetHeight(poi);
   plldf->L = GetEffectiveSpanLength(spanOrPierIdx,gdrIdx,dfType);

   bool bSkew = !( IsZero(plldf->skew1) && IsZero(plldf->skew2) );

   bool bSkewMoment = bSkew;
   bool bSkewShear  = bSkew;

   if ( lrfdVersionMgr::SeventhEdition2014 <= lrfdVersionMgr::GetVersion() )
   {
      // Starting with LRFD 7th Edition, 2014, skew correction is only applied from
      // the obtuse corner to mid-span of exterior and first interior girders.
      // Use the IsObtuseCorner method to determine if there is an obtuse corner for
      // this girder. If so, apply the skew correction
      GET_IFACE(IBridge,pBridge);
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

   lrfdLiveLoadDistributionFactorBase* pLLDF;
   if ( plldf->Method == LLDF_LRFD )
   {
      pLLDF = new lrfdLldfTypeBC(plldf->gdrNum, // to fix this warning, clean up the WBFL data types
                                 plldf->Savg,
                                 plldf->gdrSpacings,
                                 plldf->leftCurbOverhang,
                                 plldf->rightCurbOverhang,
                                 plldf->Nl, 
                                 plldf->wLane,
                                 plldf->d,
                                 plldf->L,
                                 plldf->leftDe,
                                 plldf->rightDe,
                                 plldf->skew1, 
                                 plldf->skew2,
                                 bSkewMoment,
                                 bSkewShear);
   }
   else if ( plldf->Method == LLDF_WSDOT )
   {
      pLLDF = new lrfdWsdotLldfTypeBC(plldf->gdrNum, // to fix this warning, clean up the WBFL data types
                                      plldf->Savg,
                                      plldf->gdrSpacings,
                                      plldf->leftCurbOverhang,
                                      plldf->rightCurbOverhang,
                                      plldf->Nl, 
                                      plldf->wLane,
                                      plldf->d,
                                      plldf->L,
                                      plldf->leftDe,
                                      plldf->rightDe,
                                      plldf->leftSlabOverhang,
                                      plldf->rightSlabOverhang,
                                      plldf->skew1, 
                                      plldf->skew2,
                                      bSkewMoment,
                                      bSkewShear);
   }
   else if ( plldf->Method == LLDF_TXDOT )
   {
      Float64 d = plldf->d;
      if (m_bIsSpreadSlab)
      {
         // TxDOT pins spread slab depth at 18"
         if (d < D_18)
         {
            d = D_18;
         }
      }

      pLLDF = new lrfdTxDotLldfTypeBC(plldf->gdrNum, // to fix this warning, clean up the WBFL data types
                                      plldf->Savg,
                                      plldf->gdrSpacings,
                                      plldf->leftCurbOverhang,
                                      plldf->rightCurbOverhang,
                                      plldf->Nl, 
                                      plldf->wLane,
                                      d,
                                      plldf->L,
                                      plldf->leftDe,
                                      plldf->rightDe,
                                      plldf->wCurbToCurb,
                                      plldf->skew1, 
                                      plldf->skew2,
                                      bSkewMoment,
                                      bSkewShear);
   }
   else
   {
      ATLASSERT(false); // big problemo
   }

   GET_IFACE(ILiveLoads,pLiveLoads);
   pLLDF->SetRangeOfApplicabilityAction( pLiveLoads->GetLldfRangeOfApplicabilityAction() );

   return pLLDF;
}

void CUBeamDistFactorEngineer::ReportMoment(IndexType spanOrPierIdx, rptParagraph* pPara,UBEAM_LLDFDETAILS& lldf,lrfdILiveLoadDistributionFactor::DFResult& gM1,lrfdILiveLoadDistributionFactor::DFResult& gM2,Float64 gM,bool bSIUnits,IEAFDisplayUnits* pDisplayUnits)
{
   std::_tstring strImagePath(rptStyleManager::GetImagePath());

   INIT_SCALAR_PROTOTYPE(rptRcScalar, scalar, pDisplayUnits->GetScalarFormat());

   GET_IFACE(ILibrary, pLib);
   GET_IFACE(ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

   if ( lldf.bExteriorGirder )
   {
      if ( gM1.ControllingMethod & INTERIOR_OVERRIDE )
      {
         (*pPara) << Bold(_T("1 Loaded Lane: Exterior factor may not be less than that for interior")) << rptNewLine;
         (*pPara) << _T("mg") << Super(_T("ME")) << Sub(_T("1")) << _T(" = ") << _T("mg") << Super(_T("MI")) << Sub(_T("1")) << _T(" = ") << scalar.SetValue(gM1.mg) << rptNewLine;
      }
      else
      {
         if (gM1.EqnData.bWasUsed )
         {
            // Using WSDOT spec's, and the slab overhang is <= half the girder spacing
            (*pPara) << Bold(_T("1 Loaded Lane: Spec Equations")) << rptNewLine;
            (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("mg_1_MI_Type_C_SI.png") : _T("mg_1_MI_Type_C_US.png"))) << rptNewLine;
            (*pPara) << _T("mg") << Super(_T("ME")) << Sub(_T("1")) << _T(" = ") << _T("mg") << Super(_T("MI")) << Sub(_T("1")) << _T(" = ") << scalar.SetValue(gM1.EqnData.mg) << rptNewLine;
         }

         if ( gM1.LeverRuleData.bWasUsed )
         {
            (*pPara) << Bold(_T("1 Loaded Lane: Lever Rule")) << rptNewLine;
            Float64 factor = 1.0;
            if (pSpecEntry->GetLiveLoadDistributionMethod() == LLDF_TXDOT)
            {
               (*pPara) <<_T("  For TxDOT method, do not apply multiple presence factor and multiply lever rule result by 0.9")<< rptNewLine;
               factor = 0.9;
            }

            ReportLeverRule(pPara,true,factor,gM1.LeverRuleData,m_pBroker,pDisplayUnits);
         }

         if ( gM1.RigidData.bWasUsed )
         {
            (*pPara) << Bold(_T("1 Loaded Lane: Rigid Method")) << rptNewLine;
            ReportRigidMethod(pPara,gM1.RigidData,m_pBroker,pDisplayUnits);
         }

         if ( gM1.LanesBeamsData.bWasUsed )
         {
            (*pPara) << Bold(_T("1 Loaded Lane: Number of Lanes over Number of Beams")) << rptNewLine;
            (*pPara) << _T("Skew correction is not applied to Lanes/Beams method")<< rptNewLine;
            ReportLanesBeamsMethod(pPara,gM1.LanesBeamsData,m_pBroker,pDisplayUnits);
         }
      }

      (*pPara) << rptNewLine;

      if ( lldf.Nl >= 2 )
      {
         if ( gM2.ControllingMethod & INTERIOR_OVERRIDE )
         {
            (*pPara) << Bold(_T("2+ Loaded Lanes: Exterior factor may not be less than that for interior")) << rptNewLine;
            (*pPara) << _T("mg") << Super(_T("ME")) << Sub(_T("2")) << _T(" = ") << _T("mg") << Super(_T("MI")) << Sub(_T("2")) << _T(" = ") << scalar.SetValue(gM2.mg) << rptNewLine;
         }
         else
         {
            if (gM2.EqnData.bWasUsed )
            {
               (*pPara) << Bold(_T("2+ Loaded Lanes: Spec Equation")) << rptNewLine;

               Float64 so = lldf.Side==dfLeft ? lldf.leftSlabOverhang: lldf.rightSlabOverhang;
               if ( pSpecEntry->GetLiveLoadDistributionMethod() == LLDF_WSDOT && so <= lldf.Savg/2 )
               {
                  (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("mg_2_MI_Type_C_SI.png") : _T("mg_2_MI_Type_C_US.png"))) << rptNewLine;
                  (*pPara) << _T("mg") << Super(_T("ME")) << Sub(_T("2+")) << _T(" = ") << _T("mg") << Super(_T("MI")) << Sub(_T("2+")) << _T(" = ") << scalar.SetValue(gM2.EqnData.mg) << rptNewLine;
               }
               else
               {
                  (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("mg_2_ME_Type_C_SI.png") : _T("mg_2_ME_Type_C_US.png"))) << rptNewLine;
                  (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("mg_2_MI_Type_C_SI.png") : _T("mg_2_MI_Type_C_US.png"))) << rptNewLine;
                  (*pPara) << _T("mg") << Super(_T("MI")) << Sub(_T("2+")) << _T(" = ") << scalar.SetValue(gM2.EqnData.mg) << rptNewLine;
                  (*pPara) << _T("e = ") << gM2.EqnData.e << rptNewLine;
                  (*pPara) << _T("mg") << Super(_T("ME")) << Sub(_T("2+")) << _T(" = ") << scalar.SetValue(gM2.EqnData.mg * gM2.EqnData.e) << rptNewLine;
               }
            }

            if ( gM2.LeverRuleData.bWasUsed )
            {
               (*pPara) << Bold(_T("2+ Loaded Lanes: Lever Rule")) << rptNewLine;
               Float64 factor = 1.0;
               if (pSpecEntry->GetLiveLoadDistributionMethod() == LLDF_TXDOT)
               {
                  (*pPara) << _T("  For TxDOT method, multiply lever rule result by 0.9")<< rptNewLine;
                  factor = 0.9;
               }

               ReportLeverRule(pPara,true,factor,gM2.LeverRuleData,m_pBroker,pDisplayUnits);
            }

            if ( gM2.RigidData.bWasUsed )
            {
               (*pPara) << Bold(_T("2+ Loaded Lanes: Rigid Method")) << rptNewLine;
               ReportRigidMethod(pPara,gM2.RigidData,m_pBroker,pDisplayUnits);
            }

            if ( gM2.LanesBeamsData.bWasUsed )
            {
               (*pPara) << Bold(_T("2+ Loaded Lanes: Number of Lanes over Number of Beams")) << rptNewLine;
               (*pPara) << _T("Skew correction is not applied to Lanes/Beams method")<< rptNewLine;
               ReportLanesBeamsMethod(pPara,gM2.LanesBeamsData,m_pBroker,pDisplayUnits);
            }
         }
         (*pPara) << rptNewLine;
      }

      if (gM1.ControllingMethod & LANES_DIV_BEAMS &&  gM2.ControllingMethod & LANES_DIV_BEAMS ) 
      {
         (*pPara) << _T("Skew Correction not applied to N")<<Sub(_T("l"))<<_T("/N")<<Sub(_T("b"))<<_T(" method")<< rptNewLine;
         if ( lldf.Nl >= 2 )
         {
            (*pPara) << _T("mg") << Super(_T("ME")) << _T(" = ") << ((gM2.mg>gM1.mg) ? scalar.SetValue(gM2.mg):scalar.SetValue(gM1.mg)) << Bold(_T(" < Controls")) << rptNewLine ;
         }
         else
         {
            (*pPara) << _T("mg") << Super(_T("ME")) << _T(" = ") << scalar.SetValue(gM1.mg)<< Bold(_T(" < Controls")) << rptNewLine;
         }
      }
      else
      {
         if ( gM1.ControllingMethod & MOMENT_SKEW_CORRECTION_APPLIED )
         {
            (*pPara) << Bold(_T("Skew Correction")) << rptNewLine;
            Float64 skew_delta_max = ::ConvertToSysUnits( 10.0, unitMeasure::Degree );
            if ( fabs(lldf.skew1 - lldf.skew2) < skew_delta_max )
               (*pPara) << rptRcImage(strImagePath + _T("SkewCorrection_Moment_TypeC.png")) << rptNewLine;
   
            (*pPara) << _T("Skew Correction Factor: = ") << scalar.SetValue(gM1.SkewCorrectionFactor) << rptNewLine;
            (*pPara) << rptNewLine;
            (*pPara) << _T("Skew Corrected Factor: mg") << Super(_T("ME")) << Sub(_T("1")) << _T(" = ") << scalar.SetValue(gM1.mg);

            bool singleControlled = !(gM1.ControllingMethod & OVERRIDE_USING_MULTILANE_FACTOR) && (lldf.Nl == 1 || gM1.mg >= gM2.mg);

            (singleControlled) ? (*pPara) << Bold(_T(" < Controls")) << rptNewLine: (*pPara) << rptNewLine;

            if ( lldf.Nl >= 2 )
            {
               (*pPara) << _T("Skew Corrected Factor: mg") << Super(_T("ME")) << Sub(_T("2+")) << _T(" = ") << scalar.SetValue(gM2.mg);
               (!singleControlled) ? (*pPara) << Bold(_T(" < Controls")) << rptNewLine : (*pPara) << rptNewLine;
            }
         }

         if (lldf.Method==LLDF_TXDOT && gM1.ControllingMethod & OVERRIDE_USING_MULTILANE_FACTOR)
         {
            (*pPara) << Italic(_T("TxDOT method, and roadway width is >= 20.0 ft: ")<<Bold(_T("multi-lane factor controls."))) << rptNewLine << rptNewLine;
         }
      }
   }
   else
   {
      // Distribution factor for interior girder
      if (gM1.EqnData.bWasUsed )
      {
         (*pPara) << Bold(_T("1 Loaded Lane: Spec Equations")) << rptNewLine;
         (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("mg_1_MI_Type_C_SI.png") : _T("mg_1_MI_Type_C_US.png"))) << rptNewLine;
         (*pPara) << _T("mg") << Super(_T("MI")) << Sub(_T("1")) << _T(" = ") << scalar.SetValue(gM1.EqnData.mg) << rptNewLine;
      }

      if ( gM1.LeverRuleData.bWasUsed )
      {
         (*pPara) << Bold(_T("1 Loaded Lane: Lever Rule")) << rptNewLine;
         ReportLeverRule(pPara,true,1.0,gM1.LeverRuleData,m_pBroker,pDisplayUnits);
      }

      if ( gM1.LanesBeamsData.bWasUsed )
      {
         (*pPara) << Bold(_T("1 Loaded Lane: Number of Lanes over Number of Beams")) << rptNewLine;
         (*pPara) << _T("Skew correction is not applied to Lanes/Beams method")<< rptNewLine;
         ReportLanesBeamsMethod(pPara,gM1.LanesBeamsData,m_pBroker,pDisplayUnits);
      }

      (*pPara) << rptNewLine;


      if ( lldf.Nl >= 2 )
      {
         if (gM2.EqnData.bWasUsed )
         {
            (*pPara) << Bold(_T("2+ Loaded Lanes: Spec Equations")) << rptNewLine;
            (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("mg_2_MI_Type_C_SI.png") : _T("mg_2_MI_Type_C_US.png"))) << rptNewLine;
            (*pPara) << _T("mg") << Super(_T("MI")) << Sub(_T("2+")) << _T(" = ") << scalar.SetValue(gM2.EqnData.mg) << rptNewLine;
         }

         if ( gM2.LeverRuleData.bWasUsed )
         {
            (*pPara) << Bold(_T("2+ Loaded Lanes: Lever Rule")) << rptNewLine;
            ReportLeverRule(pPara,true,1.0,gM2.LeverRuleData,m_pBroker,pDisplayUnits);
         }

         if ( gM2.LanesBeamsData.bWasUsed )
         {
            (*pPara) << Bold(_T("2+ Loaded Lanes: Number of Lanes over Number of Beams")) << rptNewLine;
            (*pPara) << _T("Skew correction is not applied to Lanes/Beams method")<< rptNewLine;
            ReportLanesBeamsMethod(pPara,gM2.LanesBeamsData,m_pBroker,pDisplayUnits);
         }

         (*pPara) << rptNewLine;
      }

      if (gM1.ControllingMethod & LANES_DIV_BEAMS &&  gM2.ControllingMethod & LANES_DIV_BEAMS ) 
      {
         (*pPara) << _T("Skew Correction not applied to N")<<Sub(_T("l"))<<_T("/N")<<Sub(_T("b"))<<_T(" method")<< rptNewLine;
         if ( lldf.Nl >= 2 )
         {
            (*pPara) << _T("mg") << Super(_T("ME")) << _T(" = ") << ((gM2.mg>gM1.mg) ? scalar.SetValue(gM2.mg):scalar.SetValue(gM1.mg)) << Bold(_T(" < Controls")) <<rptNewLine ;
         }
         else
         {
            (*pPara) << _T("mg") << Super(_T("ME")) << _T(" = ") << scalar.SetValue(gM1.mg) << Bold(_T(" < Controls")) << rptNewLine;
         }
      }
      else
      {
         if ( gM1.ControllingMethod & MOMENT_SKEW_CORRECTION_APPLIED )
         {
            (*pPara) << Bold(_T("Skew Correction")) << rptNewLine;
            Float64 skew_delta_max = ::ConvertToSysUnits( 10.0, unitMeasure::Degree );
            if ( fabs(lldf.skew1 - lldf.skew2) < skew_delta_max )
               (*pPara) << rptRcImage(strImagePath + _T("SkewCorrection_Moment_TypeC.png")) << rptNewLine;
   
            (*pPara) << _T("Skew Correction Factor: = ") << scalar.SetValue(gM1.SkewCorrectionFactor) << rptNewLine;
            (*pPara) << rptNewLine;
            (*pPara) << _T("Skew Corrected Factor: mg") << Super(_T("MI")) << Sub(_T("1")) << _T(" = ") << scalar.SetValue(gM1.mg);
            (lldf.Nl == 1 || gM1.mg >= gM2.mg) ? (*pPara) << Bold(_T(" < Controls")) << rptNewLine: (*pPara) << rptNewLine;
            if ( lldf.Nl >= 2 )
            {
               (*pPara) << _T("Skew Corrected Factor: mg") << Super(_T("MI")) << Sub(_T("2+")) << _T(" = ") << scalar.SetValue(gM2.mg);
               (gM2.mg > gM1.mg) ? (*pPara) << Bold(_T(" < Controls")) << rptNewLine : (*pPara) << rptNewLine;
            }
         }
      }
   }
}

void CUBeamDistFactorEngineer::ReportShear(IndexType spanOrPierIdx,rptParagraph* pPara,UBEAM_LLDFDETAILS& lldf,lrfdILiveLoadDistributionFactor::DFResult& gV1,lrfdILiveLoadDistributionFactor::DFResult& gV2,Float64 gV,bool bSIUnits,IEAFDisplayUnits* pDisplayUnits)
{
   std::_tstring strImagePath(rptStyleManager::GetImagePath());

   INIT_SCALAR_PROTOTYPE(rptRcScalar, scalar, pDisplayUnits->GetScalarFormat());

   GET_IFACE(ILibrary, pLib);
   GET_IFACE(ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

   if ( lldf.bExteriorGirder )
   {
      if ( gV1.ControllingMethod & INTERIOR_OVERRIDE )
      {
         (*pPara) << Bold(_T("1 Loaded Lane: Exterior factor may not be less than that for interior")) << rptNewLine;
         (*pPara) << _T("mg") << Super(_T("VE")) << Sub(_T("1")) << _T(" = ") << _T("mg") << Super(_T("VI")) << Sub(_T("1")) << _T(" = ") << scalar.SetValue(gV1.mg) << rptNewLine;
      }
      else
      {
         if ( gV1.EqnData.bWasUsed )
         {
            (*pPara) << Bold(_T("1 Loaded Lane: Spec Equations")) << rptNewLine;
            (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("mg_1_VI_Type_C_SI.png") : _T("mg_1_VI_Type_C_US.png"))) << rptNewLine;
            (*pPara) << _T("mg") << Super(_T("VE")) << Sub(_T("1")) << _T(" = ") << _T("mg") << Super(_T("VI")) << Sub(_T("1")) << _T(" = ") << scalar.SetValue(gV1.EqnData.mg) << rptNewLine;
         }

         if ( gV1.LeverRuleData.bWasUsed )
         {
            (*pPara) << Bold(_T("1 Loaded Lane: Lever Rule")) << rptNewLine;
            Float64 factor = 1.0;
            if (pSpecEntry->GetLiveLoadDistributionMethod() == LLDF_TXDOT)
            {
               (*pPara) << _T("  For TxDOT method, do not apply multiple presence factor and multiply lever rule result by 0.9") << rptNewLine;
               factor = 0.9;
            }

            ReportLeverRule(pPara,false,factor,gV1.LeverRuleData,m_pBroker,pDisplayUnits);
         }

         if ( gV1.RigidData.bWasUsed )
         {
            (*pPara) << Bold(_T("1 Loaded Lane: Rigid Method")) << rptNewLine;
            (*pPara) << _T("mg") << Super(_T("VE")) << Sub(_T("1")) << _T(" = ") << scalar.SetValue(gV1.RigidData.mg) << rptNewLine;
            (*pPara) << _T("See Moment for details") << rptNewLine;
         }

         if ( gV1.LanesBeamsData.bWasUsed )
         {
            (*pPara) << Bold(_T("1 Loaded Lane: Number of Lanes over Number of Beams")) << rptNewLine;
            (*pPara) << _T("Skew correction is not applied to Lanes/Beams method")<< rptNewLine;
            ReportLanesBeamsMethod(pPara,gV1.LanesBeamsData,m_pBroker,pDisplayUnits);
         }
      }

      (*pPara) << rptNewLine;

      if ( lldf.Nl >= 2 )
      {
         if ( gV2.ControllingMethod & INTERIOR_OVERRIDE )
         {
            (*pPara) << Bold(_T("2+ Loaded Lanes: Exterior factor may not be less than that for interior")) << rptNewLine;
            (*pPara) << _T("mg") << Super(_T("VE")) << Sub(_T("2")) << _T(" = ") << _T("mg") << Super(_T("VI")) << Sub(_T("2")) << _T(" = ") << scalar.SetValue(gV2.mg) << rptNewLine;
         }
         else
         {
            if ( gV2.EqnData.bWasUsed )
            {
               (*pPara) << Bold(_T("2+ Loaded Lanes: Spec Equation")) << rptNewLine;
               Float64 so = lldf.Side==dfLeft ? lldf.leftSlabOverhang: lldf.rightSlabOverhang;
               if ( pSpecEntry->GetLiveLoadDistributionMethod() == LLDF_WSDOT && so <= lldf.Savg/2 )
               {
                  (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("mg_2_VI_Typc_C_SI.png") : _T("mg_2_VI_Typc_C_US.png"))) << rptNewLine;
                  (*pPara) << _T("mg") << Super(_T("VE")) << Sub(_T("2+")) << _T(" = ") << _T("mg") << Super(_T("VI")) << Sub(_T("2+")) << _T(" = ") << scalar.SetValue(gV2.EqnData.mg) << rptNewLine;
               }
               else
               {
                  (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("mg_2_VE_Type_C_SI.png") : _T("mg_2_VE_Type_C_US.png"))) << rptNewLine;
                  (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("mg_2_VI_Typc_C_SI.png") : _T("mg_2_VI_Typc_C_US.png"))) << rptNewLine;
                  (*pPara) << _T("mg") << Super(_T("VI")) << Sub(_T("2+")) << _T(" = ") << scalar.SetValue(gV2.EqnData.mg) << rptNewLine;
                  (*pPara) << _T("e = ") << gV2.EqnData.e << rptNewLine;
                  (*pPara) << _T("mg") << Super(_T("VE")) << Sub(_T("2+")) << _T(" = ") << scalar.SetValue(gV2.EqnData.mg * gV2.EqnData.e) << rptNewLine;
               }
            }

            if ( gV2.LeverRuleData.bWasUsed )
            {
               (*pPara) << Bold(_T("2+ Loaded Lanes: Lever Rule")) << rptNewLine;
               Float64 factor = 1.0;
               if (pSpecEntry->GetLiveLoadDistributionMethod() == LLDF_TXDOT)
               {
                  (*pPara) << _T("  For TxDOT method, multiply lever rule result by 0.9") << rptNewLine;
                  factor = 0.9;
               }

               ReportLeverRule(pPara,false,factor,gV2.LeverRuleData,m_pBroker,pDisplayUnits);
            }

            if ( gV2.RigidData.bWasUsed )
            {
               (*pPara) << Bold(_T("2+ Loaded Lanes: Rigid Method")) << rptNewLine;
               (*pPara) << _T("mg") << Super(_T("VE")) << Sub(_T("2+")) << _T(" = ") << scalar.SetValue(gV2.RigidData.mg) << rptNewLine;
               (*pPara) << _T("See Moment for details") << rptNewLine;
            }

            if ( gV2.LanesBeamsData.bWasUsed )
            {
               (*pPara) << Bold(_T("2+ Loaded Lanes: Number of Lanes over Number of Beams")) << rptNewLine;
               (*pPara) << _T("Skew correction is not applied to Lanes/Beams method")<< rptNewLine;
               ReportLanesBeamsMethod(pPara,gV2.LanesBeamsData,m_pBroker,pDisplayUnits);
            }
         }
      }
      (*pPara) << rptNewLine;

      if ( gV1.ControllingMethod & LANES_DIV_BEAMS &&  gV2.ControllingMethod & LANES_DIV_BEAMS )
      {
         (*pPara) << _T("Skew Correction not applied to N")<<Sub(_T("l"))<<_T("/N")<<Sub(_T("b"))<<_T(" method")<< rptNewLine;
         if ( lldf.Nl >= 2 )
         {
            (*pPara) << _T("mg") << Super(_T("VE")) << _T(" = ") << ((gV2.mg>gV1.mg) ? scalar.SetValue(gV2.mg):scalar.SetValue(gV1.mg)) << Bold(_T(" < Controls")) << rptNewLine ;
         }
         else
         {
            (*pPara) << _T("mg") << Super(_T("VE")) << _T(" = ") << scalar.SetValue(gV1.mg)<< Bold(_T(" < Controls")) << rptNewLine;
         }
      }
      else
      {
         if ( gV1.ControllingMethod & SHEAR_SKEW_CORRECTION_APPLIED )
         {
            (*pPara) << Bold(_T("Skew Correction")) << rptNewLine;
            (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("SkewCorrection_Shear_TypeC_SI.png") : _T("SkewCorrection_Shear_TypeC_US.png"))) << rptNewLine;
            (*pPara) << _T("Skew Correction Factor: = ") << scalar.SetValue(gV1.SkewCorrectionFactor) << rptNewLine;
            (*pPara) << rptNewLine;

            bool singleControlled = !(gV1.ControllingMethod & OVERRIDE_USING_MULTILANE_FACTOR) && (lldf.Nl == 1 || gV1.mg >= gV2.mg);

            (*pPara) << _T("Skew Corrected Factor: mg") << Super(_T("VE")) << Sub(_T("1")) << _T(" = ") << scalar.SetValue(gV1.mg);
            (singleControlled) ? (*pPara) << Bold(_T(" < Controls")) << rptNewLine: (*pPara) << rptNewLine;
            if ( lldf.Nl >= 2 )
            {
               (*pPara) << _T("Skew Corrected Factor: mg") << Super(_T("VE")) << Sub(_T("2+")) << _T(" = ") << scalar.SetValue(gV2.mg);
               (!singleControlled) ? (*pPara) << Bold(_T(" < Controls")) << rptNewLine : (*pPara)  << rptNewLine;
            }
         }

         if (lldf.Method==LLDF_TXDOT && gV1.ControllingMethod & OVERRIDE_USING_MULTILANE_FACTOR)
         {
            (*pPara) << Italic(_T("TxDOT method, and roadway width is >= 20.0 ft: ")<<Bold(_T("multi-lane factor controls."))) << rptNewLine << rptNewLine;
         }
      }
   }
   else
   {
      // interior girder
      if ( gV1.EqnData.bWasUsed )
      {
         (*pPara) << Bold(_T("1 Loaded Lane: Spec Equations")) << rptNewLine;
         (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("mg_1_VI_Type_C_SI.png") : _T("mg_1_VI_Type_C_US.png"))) << rptNewLine;
         (*pPara) << _T("mg") << Super(_T("VI")) << Sub(_T("1")) << _T(" = ") << scalar.SetValue(gV1.EqnData.mg) << rptNewLine;
      }

      if ( gV1.LeverRuleData.bWasUsed )
      {
         (*pPara) << Bold(_T("1 Loaded Lane: Lever Rule")) << rptNewLine;
         ReportLeverRule(pPara,false,1.0,gV1.LeverRuleData,m_pBroker,pDisplayUnits);
      }

      if ( gV1.LanesBeamsData.bWasUsed )
      {
         (*pPara) << Bold(_T("1 Loaded Lane: Number of Lanes over Number of Beams")) << rptNewLine;
         (*pPara) << _T("Skew correction is not applied to Lanes/Beams method")<< rptNewLine;
         ReportLanesBeamsMethod(pPara,gV1.LanesBeamsData,m_pBroker,pDisplayUnits);
      }

      (*pPara) << rptNewLine;

      if ( lldf.Nl >= 2 )
      {
         if ( gV2.EqnData.bWasUsed )
         {
            (*pPara) << Bold(_T("2+ Loaded Lanes: Spec Equations")) << rptNewLine;
            (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("mg_2_VI_Typc_C_SI.png") : _T("mg_2_VI_Typc_C_US.png"))) << rptNewLine;
            (*pPara) << _T("mg") << Super(_T("VI")) << Sub(_T("2+")) << _T(" = ") << scalar.SetValue(gV2.EqnData.mg) << rptNewLine;
         }

         if ( gV2.LeverRuleData.bWasUsed )
         {
            (*pPara) << Bold(_T("2+ Loaded Lanes: Lever Rule")) << rptNewLine;
            ReportLeverRule(pPara,false,1.0,gV2.LeverRuleData,m_pBroker,pDisplayUnits);
         }

         if ( gV2.LanesBeamsData.bWasUsed )
         {
            (*pPara) << Bold(_T("2+ Loaded Lanes: Number of Lanes over Number of Beams")) << rptNewLine;
            (*pPara) << _T("Skew correction is not applied to Lanes/Beams method")<< rptNewLine;
            ReportLanesBeamsMethod(pPara,gV2.LanesBeamsData,m_pBroker,pDisplayUnits);
         }

         (*pPara) << rptNewLine;
      }

      if ( gV2.LanesBeamsData.bWasUsed && gV1.LanesBeamsData.bWasUsed )
      {
         (*pPara) << _T("Skew Correction not applied to N")<<Sub(_T("l"))<<_T("/N")<<Sub(_T("b"))<<_T(" method")<< rptNewLine;
         if ( lldf.Nl >= 2 )
         {
            (*pPara) << _T("mg") << Super(_T("VE")) << _T(" = ") << ((gV2.mg>gV1.mg) ? scalar.SetValue(gV2.mg):scalar.SetValue(gV1.mg)) << Bold(_T(" < Controls")) <<rptNewLine ;
         }
         else
         {
            (*pPara) << _T("mg") << Super(_T("VE")) << _T(" = ") << scalar.SetValue(gV1.mg) << Bold(_T(" < Controls")) << rptNewLine;
         }
      }
      else
      {
         if ( gV1.ControllingMethod & SHEAR_SKEW_CORRECTION_APPLIED )
         {
            (*pPara) << Bold(_T("Skew Correction")) << rptNewLine << rptRcImage(strImagePath + (bSIUnits ? _T("SkewCorrection_Shear_TypeC_SI.png") : _T("SkewCorrection_Shear_TypeC_US.png"))) << rptNewLine;
            (*pPara) << _T("Skew Correction Factor: = ") << scalar.SetValue(gV1.SkewCorrectionFactor) << rptNewLine;
            (*pPara) << rptNewLine;
            (*pPara) << _T("Skew Corrected Factor: mg") << Super(_T("VI")) << Sub(_T("1")) << _T(" = ") << scalar.SetValue(gV1.mg);
            (lldf.Nl == 1 || gV1.mg >= gV2.mg) ? (*pPara) << Bold(_T(" < Controls")) << rptNewLine : (*pPara) << rptNewLine;
            if ( lldf.Nl >= 2 )
            {
               (*pPara) << _T("Skew Corrected Factor: mg") << Super(_T("VI")) << Sub(_T("2+")) << _T(" = ") << scalar.SetValue(gV2.mg);
               (gV2.mg > gV1.mg) ? (*pPara) << Bold(_T(" < Controls")) << rptNewLine : (*pPara) << rptNewLine;
            }
         }
      }
   }
}

std::_tstring CUBeamDistFactorEngineer::GetComputationDescription(const CGirderKey& girderKey,const std::_tstring& libraryEntryName,pgsTypes::SupportedDeckType decktype, pgsTypes::AdjacentTransverseConnectivity connect)
{
   GET_IFACE(ILibrary, pLib);
   GET_IFACE(ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

   Int16 lldfMethod = pSpecEntry->GetLiveLoadDistributionMethod();

   std::_tstring descr;

   if ( m_bTypeB )
      descr = _T("Type (b) cross section. With ");
   else
      descr = _T("Type (c) cross section. With ");


   if ( lldfMethod == LLDF_WSDOT )
   {
      descr += std::_tstring(_T("WSDOT Method per Bridge Design Manual Section 3.9.4"));
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
