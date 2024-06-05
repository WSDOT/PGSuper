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

// TxDOTSpreadSlabBeamDistFactorEngineer.cpp : Implementation of CTxDOTSpreadSlabBeamDistFactorEngineer
#include "stdafx.h"
#include "TxDOTSpreadSlabBeamDistFactorEngineer.h"
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
#include <Beams\Helper.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// pre-convert constant values for performance
static const Float64 D_18 = WBFL::Units::ConvertToSysUnits(18., WBFL::Units::Measure::Inch);

/////////////////////////////////////////////////////////////////////////////

HRESULT CTxDOTSpreadSlabBeamDistFactorEngineer::FinalConstruct()
{
   return S_OK;
}

void CTxDOTSpreadSlabBeamDistFactorEngineer::BuildReport(const CGirderKey& girderKey,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits)
{
   // Grab the interfaces that are needed
   GET_IFACE(IBridge,pBridge);

   GET_IFACE(ILibrary, pLib);
   GET_IFACE(ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   const auto& live_load_distribution_criteria = pSpecEntry->GetLiveLoadDistributionCriteria();
   auto lldfMethod = live_load_distribution_criteria.LldfMethod;

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

      SPANDETAILS span_lldf = GetSpanDF(spanKey, pgsTypes::StrengthI);

      PierIndexType pier1 = spanIdx;
      PierIndexType pier2 = spanIdx + 1;

      PIERDETAILS pier1_lldf = GetPierDF(pier1, gdrIdx, pgsTypes::StrengthI, pgsTypes::Ahead);
      PIERDETAILS pier2_lldf = GetPierDF(pier2, gdrIdx, pgsTypes::StrengthI, pgsTypes::Back);

      // do a sanity check to make sure the fundamental values are correct
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
      (*pPara) << _T("Girder Depth: d = ") << xdim2.SetValue(span_lldf.d) << rptNewLine;

      if ( WBFL::LRFD::BDSManager::Edition::FourthEditionWith2009Interims <= WBFL::LRFD::BDSManager::GetEdition() )
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

      ////////////////////////////////////////////////////////////////////////////
      // Fatigue limit states
      ////////////////////////////////////////////////////////////////////////////
      if ( WBFL::LRFD::BDSManager::Edition::FourthEditionWith2009Interims <= WBFL::LRFD::BDSManager::GetEdition() )
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


WBFL::LRFD::LiveLoadDistributionFactorBase* CTxDOTSpreadSlabBeamDistFactorEngineer::GetLLDFParameters(IndexType spanOrPierIdx,GirderIndexType gdrIdx,DFParam dfType,TXSPREADSLABBEAM_LLDFDETAILS* plldf,const GDRCONFIG* pConfig)
{
   GET_IFACE(IGirder, pGdr);
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   GET_IFACE(IBridge,pBridge);

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
   Int32 roaVal = pDistFactors->VerifyDistributionFactorRequirements(poi);

   plldf->d = pGdr->GetHeight(poi);
   plldf->L = GetEffectiveSpanLength(spanOrPierIdx,gdrIdx,dfType);
   plldf->ts = pBridge->GetStructuralSlabDepth(poi);

   WBFL::LRFD::LiveLoadDistributionFactorBase* pLLDF;
   pLLDF = new WBFL::LRFD::TxdotLldfSpreadSlab(plldf->gdrNum, // to fix this warning, clean up the WBFL data types
                              plldf->Savg,
                              plldf->gdrSpacings,
                              plldf->leftCurbOverhang,
                              plldf->rightCurbOverhang,
                              plldf->Nl,
                              plldf->wLane,
                              plldf->d,
                              plldf->L,
                              plldf->ts,
                              plldf->skew1, 
                              plldf->skew2);

   GET_IFACE(ILiveLoads,pLiveLoads);
   pLLDF->SetRangeOfApplicability( pLiveLoads->GetRangeOfApplicabilityAction(), roaVal );

   return pLLDF;
}

void CTxDOTSpreadSlabBeamDistFactorEngineer::ReportMoment(IndexType spanOrPierIdx, rptParagraph* pPara,TXSPREADSLABBEAM_LLDFDETAILS& lldf,WBFL::LRFD::ILiveLoadDistributionFactor::DFResult& gM1,WBFL::LRFD::ILiveLoadDistributionFactor::DFResult& gM2,Float64 gM,bool bSIUnits,IEAFDisplayUnits* pDisplayUnits)
{
   std::_tstring strImagePath(rptStyleManager::GetImagePath());

   INIT_SCALAR_PROTOTYPE(rptRcScalar, scalar, pDisplayUnits->GetScalarFormat());

   GET_IFACE(ILibrary, pLib);
   GET_IFACE(ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   const auto& live_load_distribution_criteria = pSpecEntry->GetLiveLoadDistributionCriteria();

   if ( lldf.bExteriorGirder )
   {
      if ( gM1.ControllingMethod & WBFL::LRFD::INTERIOR_OVERRIDE )
      {
         (*pPara) << Bold(_T("1 Loaded Lane: Exterior factor may not be less than that for adjacent interior. See interior details report for computation details.")) << rptNewLine;
         (*pPara) << _T("mg") << Super(_T("ME")) << Sub(_T("1")) << _T(" = ") << _T("mg") << Super(_T("MI")) << Sub(_T("1")) << _T(" = ") << scalar.SetValue(gM1.mg) << rptNewLine;
      }
      else
      {
         if (gM1.EqnData.bWasUsed )
         {
            (*pPara) << Bold(_T("1 Loaded Lane: Spec Equations")) << rptNewLine;
            (*pPara) << rptRcImage(strImagePath + _T("mg_1_ME_TxDOT_SpreadSlab.png")) << rptNewLine;
            (*pPara) << _T("mg") << Super(_T("ME")) << Sub(_T("1")) << _T(" = ") << _T("mg") << Super(_T("MI")) << Sub(_T("1")) << _T(" = ") << scalar.SetValue(gM1.EqnData.mg) << rptNewLine;
         }

         if ( gM1.LeverRuleData.bWasUsed )
         {
            (*pPara) << Bold(_T("1 Loaded Lane: Lever Rule")) << rptNewLine;
            Float64 factor = 1.0;
            if (live_load_distribution_criteria.LldfMethod == pgsTypes::LiveLoadDistributionFactorMethod::TxDOT)
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
         if ( gM2.ControllingMethod & WBFL::LRFD::INTERIOR_OVERRIDE )
         {
            (*pPara) << Bold(_T("2+ Loaded Lanes: Exterior factor may not be less than that for adjacent interior. See interior details report for computation details.")) << rptNewLine;
            (*pPara) << _T("mg") << Super(_T("ME")) << Sub(_T("2")) << _T(" = ") << _T("mg") << Super(_T("MI")) << Sub(_T("2")) << _T(" = ") << scalar.SetValue(gM2.mg) << rptNewLine;
         }
         else
         {
            if (gM2.EqnData.bWasUsed )
            {
               (*pPara) << Bold(_T("2+ Loaded Lanes: Spec Equation")) << rptNewLine;

               (*pPara) << rptRcImage(strImagePath + _T("mg_2_ME_TxDOT_SpreadSlab.png")) << rptNewLine;
               (*pPara) << _T("mg") << Super(_T("ME")) << Sub(_T("2+")) << _T(" = ") << scalar.SetValue(gM2.EqnData.mg) << rptNewLine;
            }

            if ( gM2.LeverRuleData.bWasUsed )
            {
               (*pPara) << Bold(_T("2+ Loaded Lanes: Lever Rule")) << rptNewLine;
               Float64 factor = 1.0;
               if (live_load_distribution_criteria.LldfMethod == pgsTypes::LiveLoadDistributionFactorMethod::TxDOT)
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

      if (gM1.ControllingMethod & WBFL::LRFD::LANES_DIV_BEAMS &&  gM2.ControllingMethod & WBFL::LRFD::LANES_DIV_BEAMS ) 
      {
         if ( lldf.Nl >= 2 )
         {
            (*pPara) << _T("mg") << Super(_T("ME")) << _T(" = ") << ((gM2.mg>gM1.mg) ? scalar.SetValue(gM2.mg):scalar.SetValue(gM1.mg)) << Bold(_T(" < Controls")) << rptNewLine ;
         }
         else
         {
            (*pPara) << _T("mg") << Super(_T("ME")) << _T(" = ") << scalar.SetValue(gM1.mg)<< Bold(_T(" < Controls")) << rptNewLine;
         }
      }
   }
   else
   {
      // Distribution factor for interior girder
      if (gM1.EqnData.bWasUsed )
      {
         (*pPara) << Bold(_T("1 Loaded Lane: Spec Equations")) << rptNewLine;
         (*pPara) << rptRcImage(strImagePath + _T("mg_1_MI_TxDOT_SpreadSlab.png")) << rptNewLine;
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
            (*pPara) << rptRcImage(strImagePath + _T("mg_2_MI_TxDOT_SpreadSlab.png")) << rptNewLine;
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
            ReportLanesBeamsMethod(pPara,gM2.LanesBeamsData,m_pBroker,pDisplayUnits);
         }

         (*pPara) << rptNewLine;
      }

      if (gM1.ControllingMethod & WBFL::LRFD::LANES_DIV_BEAMS &&  gM2.ControllingMethod & WBFL::LRFD::LANES_DIV_BEAMS ) 
      {
         if ( lldf.Nl >= 2 )
         {
            (*pPara) << _T("mg") << Super(_T("ME")) << _T(" = ") << ((gM2.mg>gM1.mg) ? scalar.SetValue(gM2.mg):scalar.SetValue(gM1.mg)) << Bold(_T(" < Controls")) <<rptNewLine ;
         }
         else
         {
            (*pPara) << _T("mg") << Super(_T("ME")) << _T(" = ") << scalar.SetValue(gM1.mg) << Bold(_T(" < Controls")) << rptNewLine;
         }
      }
   }
}

void CTxDOTSpreadSlabBeamDistFactorEngineer::ReportShear(IndexType spanOrPierIdx,rptParagraph* pPara,TXSPREADSLABBEAM_LLDFDETAILS& lldf,WBFL::LRFD::ILiveLoadDistributionFactor::DFResult& gV1,WBFL::LRFD::ILiveLoadDistributionFactor::DFResult& gV2,Float64 gV,bool bSIUnits,IEAFDisplayUnits* pDisplayUnits)
{
   std::_tstring strImagePath(rptStyleManager::GetImagePath());

   INIT_SCALAR_PROTOTYPE(rptRcScalar, scalar, pDisplayUnits->GetScalarFormat());

   GET_IFACE(ILibrary, pLib);
   GET_IFACE(ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   const auto& live_load_distribution_criteria = pSpecEntry->GetLiveLoadDistributionCriteria();

   if ( lldf.bExteriorGirder )
   {
      if ( gV1.ControllingMethod & WBFL::LRFD::INTERIOR_OVERRIDE )
      {
         (*pPara) << Bold(_T("1 Loaded Lane: Exterior factor may not be less than that for adjacent interior. See interior details report for computation details.")) << rptNewLine;
         (*pPara) << _T("mg") << Super(_T("VE")) << Sub(_T("1")) << _T(" = ") << _T("mg") << Super(_T("VI")) << Sub(_T("1")) << _T(" = ") << scalar.SetValue(gV1.mg) << rptNewLine;
      }
      else
      {
         if ( gV1.EqnData.bWasUsed )
         {
            (*pPara) << Bold(_T("1 Loaded Lane: Spec Equations")) << rptNewLine;
            (*pPara) << rptRcImage(strImagePath + _T("mg_1_VE_TxDOT_SpreadSlab.png")) << rptNewLine;
            (*pPara) << _T("mg") << Super(_T("VE")) << Sub(_T("1")) << _T(" = ") << _T("mg") << Super(_T("VI")) << Sub(_T("1")) << _T(" = ") << scalar.SetValue(gV1.EqnData.mg) << rptNewLine;
         }

         if ( gV1.LeverRuleData.bWasUsed )
         {
            (*pPara) << Bold(_T("1 Loaded Lane: Lever Rule")) << rptNewLine;
            Float64 factor = 1.0;
            if (live_load_distribution_criteria.LldfMethod == pgsTypes::LiveLoadDistributionFactorMethod::TxDOT)
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
            ReportLanesBeamsMethod(pPara,gV1.LanesBeamsData,m_pBroker,pDisplayUnits);
         }
      }

      (*pPara) << rptNewLine;

      if ( lldf.Nl >= 2 )
      {
         if ( gV2.ControllingMethod & WBFL::LRFD::INTERIOR_OVERRIDE )
         {
            (*pPara) << Bold(_T("2+ Loaded Lanes: Exterior factor may not be less than that for adjacent interior. See interior details report for computation details.")) << rptNewLine;
            (*pPara) << _T("mg") << Super(_T("VE")) << Sub(_T("2")) << _T(" = ") << _T("mg") << Super(_T("VI")) << Sub(_T("2")) << _T(" = ") << scalar.SetValue(gV2.mg) << rptNewLine;
         }
         else
         {
            if ( gV2.EqnData.bWasUsed )
            {
               (*pPara) << Bold(_T("2+ Loaded Lanes: Spec Equation")) << rptNewLine;
               (*pPara) << rptRcImage(strImagePath + _T("mg_2_VE_TxDOT_SpreadSlab.png")) << rptNewLine;
               (*pPara) << _T("mg") << Super(_T("VE")) << Sub(_T("2+")) << _T(" = ") << _T("mg") << Super(_T("VI")) << Sub(_T("2+")) << _T(" = ") << scalar.SetValue(gV2.EqnData.mg) << rptNewLine;
            }

            if ( gV2.LeverRuleData.bWasUsed )
            {
               (*pPara) << Bold(_T("2+ Loaded Lanes: Lever Rule")) << rptNewLine;
               Float64 factor = 1.0;
               if (live_load_distribution_criteria.LldfMethod == pgsTypes::LiveLoadDistributionFactorMethod::TxDOT)
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
               ReportLanesBeamsMethod(pPara,gV2.LanesBeamsData,m_pBroker,pDisplayUnits);
            }
         }
      }
      (*pPara) << rptNewLine;

      if ( gV1.ControllingMethod & WBFL::LRFD::LANES_DIV_BEAMS &&  gV2.ControllingMethod & WBFL::LRFD::LANES_DIV_BEAMS )
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
   }
   else
   {
      // interior girder
      if ( gV1.EqnData.bWasUsed )
      {
         (*pPara) << Bold(_T("1 Loaded Lane: Spec Equations")) << rptNewLine;
         (*pPara) << rptRcImage(strImagePath + _T("mg_1_VI_TxDOT_SpreadSlab.png")) << rptNewLine;
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
         ReportLanesBeamsMethod(pPara,gV1.LanesBeamsData,m_pBroker,pDisplayUnits);
      }

      (*pPara) << rptNewLine;

      if ( lldf.Nl >= 2 )
      {
         if ( gV2.EqnData.bWasUsed )
         {
            (*pPara) << Bold(_T("2+ Loaded Lanes: Spec Equations")) << rptNewLine;
            (*pPara) << rptRcImage(strImagePath + _T("mg_2_VI_TxDOT_SpreadSlab.png")) << rptNewLine;
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
   }
}

std::_tstring CTxDOTSpreadSlabBeamDistFactorEngineer::GetComputationDescription(const CGirderKey& girderKey,const std::_tstring& libraryEntryName,pgsTypes::SupportedDeckType decktype, pgsTypes::AdjacentTransverseConnectivity connect)
{
   std::_tstring descr(_T("TxDOT Spread Slab Beam cross section. LLDF's are computed per the TxDOT Bridge Design Manual - LRFD"));
   return descr;
}
