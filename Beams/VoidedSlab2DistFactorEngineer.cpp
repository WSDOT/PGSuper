///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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

// VoidedSlab2DistFactorEngineer.cpp : Implementation of VoidedSlab2DistFactorEngineer
#include "stdafx.h"
#include "Beams.h"
#include <Beams/VoidedSlab2DistFactorEngineer.h>
#include <PGSuperException.h>
#include <Units\Convert.h>
#include <PsgLib\TrafficBarrierEntry.h>
#include <PsgLib\SpecLibraryEntry.h>
#include <PsgLib\BridgeDescription2.h>
#include <PsgLib\GirderLabel.h>
#include <psgLib/GirderLibraryEntry.h>

#include <PgsExt\StatusItem.h>
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\DistributionFactors.h>
#include <EAF/EAFStatusCenter.h>
#include <IFace\Intervals.h>
#include <Beams\Helper.h>

using namespace PGS::Beams;

void VoidedSlab2DistFactorEngineer::BuildReport(const CGirderKey& girderKey,rptChapter* pChapter,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits)
{
   // Grab the interfaces that are needed
   GET_IFACE2(GetBroker(), IBridge,pBridge);

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

   GET_IFACE2(GetBroker(), IBridgeDescription,pIBridgeDesc);
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

      rptParagraph* pPara = nullptr;
      const CSpanData2* pSpan = pBridgeDesc->GetSpan(spanIdx);
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(pSpan);
      const CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);

      std::_tstring strGirderName = pGirder->GetGirderName();

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
      pBridge->GetStationAndOffset(pgsPointOfInterest(CSegmentKey(girderKey,0),span_lldf.ControllingLocation),&station, &offset);
      Float64 supp_dist = span_lldf.ControllingLocation - pBridge->GetSegmentStartEndDistance(CSegmentKey(girderKey,0));
      (*pPara) << _T("Deck Width, Girder Spacing and Deck Overhang are measured along a line that is normal to the alignment and passing through a point ") << location.SetValue(supp_dist) << _T(" from the left support along the centerline of girder. ");
      (*pPara) << _T("The measurement line passes through Station ") << rptRcStation(station, &pDisplayUnits->GetStationFormat() ) << _T(" (") << RPT_OFFSET(offset,offsetFormatter) << _T(")") << rptNewLine;
      (*pPara) << _T("Bridge Width: W = ") << xdim.SetValue(span_lldf.W) << rptNewLine;
      (*pPara) << _T("Roadway Width: w = ") << xdim.SetValue(span_lldf.wCurbToCurb) << rptNewLine;
      (*pPara) << _T("Number of Design Lanes: N") << Sub(_T("L")) << _T(" = ") << span_lldf.Nl << rptNewLine;
      (*pPara) << _T("Lane Width: wLane = ") << xdim.SetValue(span_lldf.wLane) << rptNewLine;
      (*pPara) << _T("Number of Girders: N") << Sub(_T("b")) << _T(" = ") << span_lldf.Nb << rptNewLine;
      (*pPara) << _T("Girder Spacing: ") << Sub2(_T("S"),_T("avg")) << _T(" = ") << xdim.SetValue(span_lldf.Savg) << rptNewLine;
      (*pPara) << _T("Span Length: L = ") << xdim.SetValue(span_lldf.L) << rptNewLine;
      (*pPara) << _T("Moment of Inertia: I = ") << inertia.SetValue(span_lldf.I) << rptNewLine;
      (*pPara) << _T("St. Venant torsional inertia constant: J = ") << inertia.SetValue(span_lldf.J) << rptNewLine;
      (*pPara) << _T("Beam Width: b = ") << xdim2.SetValue(span_lldf.b) << rptNewLine;
      (*pPara) << _T("Beam Depth: d = ") << xdim2.SetValue(span_lldf.d) << rptNewLine;
      Float64 de = span_lldf.Side==DfSide::Left ? span_lldf.leftDe:span_lldf.rightDe;
      (*pPara) << _T("Distance from exterior web of exterior beam to curb line: d") << Sub(_T("e")) << _T(" = ") << xdim.SetValue(de) << rptNewLine;
      (*pPara) << _T("Poisson Ratio: ") << symbol(mu) << _T(" = ") << span_lldf.PoissonRatio << rptNewLine;
   //   (*pPara) << _T("Skew Angle at start: ") << symbol(theta) << _T(" = ") << angle.SetValue(fabs(span_lldf.skew1)) << rptNewLine;
   //   (*pPara) << _T("Skew Angle at end: ") << symbol(theta) << _T(" = ") << angle.SetValue(fabs(span_lldf.skew2)) << rptNewLine;
      GET_IFACE2(GetBroker(), ISpecification, pSpec);
      GET_IFACE2(GetBroker(), ILibrary, pLibrary);
      const auto* pSpecEntry = pLibrary->GetSpecEntry(pSpec->GetSpecification().c_str());
      const auto& live_load_distribution_criteria = pSpecEntry->GetLiveLoadDistributionCriteria();
      if (live_load_distribution_criteria.bIgnoreSkewReductionForMoment)
      {
         (*pPara) << _T("Skew reduction for moment distribution factors has been ignored (LRFD 4.6.2.2.2e)") << rptNewLine;
      }

      if (pBridgeDesc->GetDistributionFactorMethod() != pgsTypes::LeverRule)
      {
         pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
         (*pPara) << _T("St. Venant torsional inertia constant");
         (*pChapter) << pPara;
         pPara = new rptParagraph;
         (*pChapter) << pPara;

         if ( span_lldf.nVoids == 0 )
         {
            (*pPara) << _T("Polar Moment of Inertia: I") << Sub(_T("p")) << _T(" = ") << inertia.SetValue(span_lldf.Jsolid.Ip) << rptNewLine;
            (*pPara) << _T("Area: A") << _T(" = ") << area.SetValue(span_lldf.Jsolid.A) << rptNewLine;
            (*pPara) << _T("Torsional Constant: ") << rptRcImage(strImagePath + _T("J.png")) << rptTab
                     << _T("J") << _T(" = ") << inertia.SetValue(span_lldf.J) << rptNewLine;
         }
         else
         {
            (*pPara) << rptRcImage(strImagePath + _T("J_closed_thin_wall.png")) << rptNewLine;
            (*pPara) << rptRcImage(strImagePath + _T("VoidedSlab_TorsionalConstant.png")) << rptNewLine;
            (*pPara) << _T("Area enclosed by centerlines of elements: ") << Sub2(_T("A"),_T("o")) << _T(" = ") << area.SetValue(span_lldf.Jvoid.Ao) << rptNewLine;

            rptRcTable* p_table = rptStyleManager::CreateDefaultTable(3,_T(""));
            (*pPara) << p_table;

            (*p_table)(0,0) << _T("Element");
            (*p_table)(0,1) << _T("s");
            (*p_table)(0,2) << _T("t");

            RowIndexType row = p_table->GetNumberOfHeaderRows();
            std::vector<VOIDEDSLAB_J_VOID::Element>::iterator iter;
            for ( iter = span_lldf.Jvoid.Elements.begin(); iter != span_lldf.Jvoid.Elements.end(); iter++ )
            {
               VOIDEDSLAB_J_VOID::Element& element = *iter;
               (*p_table)(row,0) << row;
               (*p_table)(row,1) << xdim2.SetValue(element.first);
               (*p_table)(row,2) << xdim2.SetValue(element.second);

               row++;
            }
            (*pPara) << symbol(SUM) << _T("s/t = ") << span_lldf.Jvoid.S_over_T << rptNewLine;
            (*pPara) << _T("Torsional Constant: J = ") << inertia.SetValue(span_lldf.J) << rptNewLine;
         }
      }

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
      if ( bContinuousAtStart || bContinuousAtEnd || bIntegralAtStart || bIntegralAtEnd )
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
            (*pPara) << _T("g") << superscript << Sub(_T("Fatigue")) << _T(" = ") << _T("mg") << superscript << Sub(_T("1")) << _T("/m =") << scalar.SetValue(pier1_lldf.gM1.mg) << _T("/") << scalar3.SetValue(mpf) << _T(" = ") << scalar2.SetValue(pier1_lldf.gM1.mg / mpf);
         }

         // Positive moment DF
         pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
         (*pChapter) << pPara;
         if ( bContinuousAtStart || bContinuousAtEnd || bIntegralAtStart || bIntegralAtEnd )
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
         (*pPara) << _T("g") << superscript << Sub(_T("Fatigue")) << _T(" = ") << _T("mg") << superscript << Sub(_T("1")) << _T("/m =") << scalar.SetValue(span_lldf.gM1.mg) << _T("/") << scalar3.SetValue(mpf) << _T(" = ") << scalar2.SetValue(span_lldf.gM1.mg / mpf);

         if ( bContinuousAtEnd || bIntegralAtEnd )
         {
            pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
            (*pChapter) << pPara;
            (*pPara) << _T("Distribution Factor for Negative Moment over Pier ") << LABEL_PIER(pier2) << rptNewLine;
            pPara = new rptParagraph;
            (*pChapter) << pPara;

            superscript = (pier2_lldf.bExteriorGirder ? _T("ME") : _T("MI"));
            mpf = pier2_lldf.gM1.GetMultiplePresenceFactor();
            (*pPara) << _T("g") << superscript << Sub(_T("Fatigue")) << _T(" = ") << _T("mg") << superscript << Sub(_T("1")) << _T("/m =") << scalar.SetValue(pier2_lldf.gM1.mg) << _T("/") << scalar3.SetValue(mpf) << _T(" = ") << scalar2.SetValue(pier2_lldf.gM1.mg / mpf);
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
         (*pPara) << _T("g") << superscript << Sub(_T("Fatigue")) << _T(" = ") << _T("mg") << superscript << Sub(_T("1")) << _T("/m =") << scalar.SetValue(span_lldf.gV1.mg) << _T("/") << scalar3.SetValue(mpf) << _T(" = ") << scalar2.SetValue(span_lldf.gV1.mg / mpf);
      }
   } // next span
}

WBFL::LRFD::LiveLoadDistributionFactorBase* VoidedSlab2DistFactorEngineer::GetLLDFParameters(IndexType spanOrPierIdx,GirderIndexType gdrIdx,DFParam dfType,VOIDEDSLAB_LLDFDETAILS* plldf,const GDRCONFIG* pConfig)
{
   GET_IFACE2(GetBroker(), ISectionProperties, pSectProp);
   GET_IFACE2(GetBroker(), IGirder,            pGirder);
   GET_IFACE2(GetBroker(), IBridge,            pBridge);
   GET_IFACE2(GetBroker(), IBarriers,          pBarriers);

   // Determine span/pier index... This is the index of a pier and the next span.
   // If this is the last pier, span index is for the last span
   SpanIndexType span = INVALID_INDEX;
   PierIndexType pier = INVALID_INDEX;
   SpanIndexType prev_span = INVALID_INDEX;
   SpanIndexType next_span = INVALID_INDEX;
   PierIndexType prev_pier = INVALID_INDEX;
   PierIndexType next_pier = INVALID_INDEX;
   GetIndicies(spanOrPierIdx,dfType,span,pier,prev_span,next_span,prev_pier,next_pier);

   GET_IFACE2(GetBroker(), IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   ATLASSERT( pBridgeDesc->GetDistributionFactorMethod() != pgsTypes::DirectlyInput );

   const CDeckDescription2* pDeck = pBridgeDesc->GetDeckDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(span);

   GirderIndexType nGirders = pGroup->GetGirderCount();

   if ( nGirders <= gdrIdx )
   {
      ATLASSERT(false);
      gdrIdx = nGirders-1;
   }

   CSpanKey spanKey(span,gdrIdx);

   const GirderLibraryEntry* pGirderEntry = pGroup->GetGirder(gdrIdx)->GetGirderLibraryEntry();

   // determine girder spacing
   pgsTypes::SupportedBeamSpacing spacingType = pBridgeDesc->GetGirderSpacingType();

   // determine overhang, spacing base data, and controlling poi
   pgsPointOfInterest poi;
   GetGirderSpacingAndOverhang(spanKey, dfType, plldf, &poi);

   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   // Throws exception if fails requirement (no need to catch it)
   GET_IFACE2(GetBroker(), ILiveLoadDistributionFactors, pDistFactors);
   Int32 roaVal = pDistFactors->VerifyDistributionFactorRequirements(poi);

   plldf->bExteriorGirder = pBridge->IsExteriorGirder(segmentKey);

   plldf->bExteriorGirder = pBridge->IsExteriorGirder(segmentKey);

   Float64 Height       = pGirderEntry->GetDimension(_T("H"));
   Float64 Width = pGirderEntry->GetDimension(_T("W"));
   Float64 c1 = pGirderEntry->GetDimension(_T("C1"));
   Float64 c3 = pGirderEntry->GetDimension(_T("C3"));
   if (plldf->bExteriorGirder)
   {
      Width -= 2 * c3;
   }
   else
   {
      Width -= 2 * Max(c1, c3);
   }
   Float64 ExtVoidSpacing = pGirderEntry->GetDimension(_T("S1"));
   Float64 IntVoidSpacing = pGirderEntry->GetDimension(_T("S2"));
   Float64 ExtVoidDiameter = pGirderEntry->GetDimension(_T("D1"));
   Float64 IntVoidDiameter = pGirderEntry->GetDimension(_T("D2"));
   Float64 ExtVoidCenter = pGirderEntry->GetDimension(_T("H1"));
   Float64 IntVoidCenter = pGirderEntry->GetDimension(_T("H2"));
   Int16   nVoids       = (Int16)pGirderEntry->GetDimension(_T("Number_of_Voids"));
   Int16   nExtVoids = 2;
   Int16   nIntVoids = nVoids-nExtVoids;
   if ( nIntVoids < 0 )
   {
      nIntVoids = 0;
      nExtVoids = nVoids;
   }

   plldf->b            = Width;
   plldf->d            = Height;

   GET_IFACE2(GetBroker(), IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType llIntervalIdx = pIntervals->GetLiveLoadInterval();
   plldf->I = pSectProp->GetIxx(pgsTypes::sptGross,llIntervalIdx,poi,pConfig);

   plldf->PoissonRatio = 0.2;
   plldf->nVoids = nVoids; // need to make WBFL::LRFD consistent
   plldf->TransverseConnectivity = pDeck->TransverseConnectivity;

   pgsTypes::TrafficBarrierOrientation side = pBarriers->GetNearestBarrier(segmentKey);
   Float64 curb_offset = pBarriers->GetInterfaceWidth(side);

   // compute de (inside edge of barrier to CL of exterior web)
   Float64 wd = pGirder->GetCL2ExteriorWebDistance(poi); // cl beam to cl web
   ATLASSERT(wd>=0.0);

   // Note this is not exactly correct because opposite exterior beam might be different, but we won't be using this data set for that beam
   plldf->leftDe  = plldf->leftCurbOverhang - wd;  
   plldf->rightDe = plldf->rightCurbOverhang - wd; 

   plldf->L = GetEffectiveSpanLength(spanOrPierIdx,gdrIdx,dfType);

   WBFL::LRFD::LiveLoadDistributionFactorBase* pLLDF;

   if ( nVoids == 0 )
   {
      // solid slab

      auto Ix = pSectProp->GetIxx(pgsTypes::sptGross,llIntervalIdx,poi,pConfig);
      auto Iy = pSectProp->GetIyy(pgsTypes::sptGross,llIntervalIdx,poi,pConfig);
      auto A  = pSectProp->GetAg(pgsTypes::sptGross,llIntervalIdx,poi,pConfig);
      auto Ip = Ix + Iy;

      VOIDEDSLAB_J_SOLID Jsolid;
      Jsolid.A = A;
      Jsolid.Ip = Ip;

      plldf->Jsolid = Jsolid;
      plldf->J = A*A*A*A/(40.0*Ip);
   }
   else
   {
      // voided slab

      // Determine J

      VOIDEDSLAB_J_VOID Jvoid;

      Float64 Sum_s_over_t = 0;

      // thickness of exterior "web" (edge of beam to first void)
      Float64 t_ext;
      if ( nIntVoids == 0 )
      {
         t_ext = (Width - (nExtVoids-1)*ExtVoidSpacing - ExtVoidDiameter)/2;
      }
      else
      {
         t_ext = (Width - (nIntVoids-1)*IntVoidSpacing - 2*ExtVoidSpacing - ExtVoidDiameter)/2;
      }

      // thickness of interior "web" (between interior voids)
      Float64 t_int;
      if ( nIntVoids == 0 )
      {
         t_int = 0;
      }
      else
      {
         t_int = IntVoidSpacing - IntVoidDiameter;
      }

      // thickness of "web" between interior and exterior voids)
      Float64 t_ext_int;
      if ( nIntVoids == 0 )
      {
         t_ext_int = ExtVoidSpacing - ExtVoidDiameter;
      }
      else
      {
         t_ext_int = ExtVoidSpacing - ExtVoidDiameter/2 - IntVoidDiameter/2;
      }

      // s and t for top and bottom
      Float64 s_top = Width - t_ext;
      Float64 s_bot = Width - t_ext;

      Float64 t_top, t_bot;
      if (nIntVoids == 0)
      {
         t_bot = ExtVoidCenter - ExtVoidDiameter / 2;
         t_top = Height - (ExtVoidCenter + ExtVoidDiameter / 2);
      }
      else
      {
         t_bot = Min(ExtVoidCenter - ExtVoidDiameter / 2, IntVoidCenter - IntVoidDiameter / 2);
         t_top = Height - Max((ExtVoidCenter + ExtVoidDiameter / 2), (IntVoidCenter + IntVoidDiameter / 2));
      }

      Float64 slab_depth = pBridge->GetStructuralSlabDepth(poi);
      t_top += slab_depth;

      // length of internal, vertical elements between voids
      Float64 s_int = Height + slab_depth - t_top / 2 - t_bot / 2;

      Jvoid.Elements.push_back(VOIDEDSLAB_J_VOID::Element(s_top,t_top)); // top
      Jvoid.Elements.push_back(VOIDEDSLAB_J_VOID::Element(s_bot,t_bot)); // bottom
      Sum_s_over_t += (s_top / t_top) + (s_bot / t_bot);

      Jvoid.Elements.push_back(VOIDEDSLAB_J_VOID::Element(s_int,t_ext)); // left edge
      Sum_s_over_t += (s_int/t_ext);

      // between voids
      if ( nIntVoids == 0 )
      {
         if ( nExtVoids == 2 )
         {
            Jvoid.Elements.push_back(VOIDEDSLAB_J_VOID::Element(s_int,t_ext_int));
            Sum_s_over_t += (s_int/t_ext_int);
         }
      }
      else
      {
         // "web" between left exterior and first interior void
         Jvoid.Elements.push_back(VOIDEDSLAB_J_VOID::Element(s_int,t_ext_int));
         Sum_s_over_t += (s_int/t_ext_int);

         // between all interior voids
         for ( Int16 i = 1; i < nIntVoids; i++ )
         {
            Jvoid.Elements.push_back(VOIDEDSLAB_J_VOID::Element(s_int,t_int));
            Sum_s_over_t += (s_int/t_int);
         }

         // "web" between last interior void and right exterior void
         Jvoid.Elements.push_back(VOIDEDSLAB_J_VOID::Element(s_int,t_ext_int));
         Sum_s_over_t += (s_int/t_ext_int);
      }

      Jvoid.Elements.push_back(VOIDEDSLAB_J_VOID::Element(s_int,t_ext)); // right edge
      Sum_s_over_t += (s_int/t_ext);

      Jvoid.S_over_T = Sum_s_over_t;

      Float64 Ao = s_top * s_int;

      Jvoid.Ao = Ao;

      Float64 J = 4.0*Ao*Ao/Sum_s_over_t;

      plldf->Jvoid = Jvoid;
      plldf->J = J;
   }

   // WSDOT deviation doesn't apply to this type of cross section because it isn't slab on girder construction
   if (plldf->Method == pgsTypes::LiveLoadDistributionFactorMethod::TxDOT)
   {
         pLLDF = new WBFL::LRFD::TxdotVoidedSlab(plldf->gdrNum,
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
                                         plldf->b,
                                         plldf->d,
                                         plldf->leftDe,
                                         plldf->rightDe,
                                         plldf->PoissonRatio,
                                         plldf->skew1, 
                                         plldf->skew2);
   }
   else
   {
      GET_IFACE2(GetBroker(), ISpecification, pSpec);
      GET_IFACE2(GetBroker(), ILibrary, pLibrary);
      const auto* pSpecEntry = pLibrary->GetSpecEntry(pSpec->GetSpecification().c_str());
      const auto& live_load_distribution_criteria = pSpecEntry->GetLiveLoadDistributionCriteria();
      bool bSkew = !(IsZero(plldf->skew1) && IsZero(plldf->skew2));
      bool bSkewMoment = live_load_distribution_criteria.bIgnoreSkewReductionForMoment ? false : bSkew;
      bool bSkewShear = bSkew;

      if ( WBFL::LRFD::BDSManager::Edition::SeventhEdition2014 <= WBFL::LRFD::BDSManager::GetEdition() )
      {
         // Starting with LRFD 7th Edition, 2014, skew correction is only applied from
         // the obtuse corner to mid-span of exterior and first interior girders.
         // Use the IsObtuseCorner method to determine if there is an obtuse corner for
         // this girder. If so, apply the skew correction
         if ( dfType == DFParam::Reaction )
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

      if ( pDeck->TransverseConnectivity == pgsTypes::atcConnectedAsUnit || 
           WBFL::LRFD::BDSManager::Edition::SeventhEdition2014 <= WBFL::LRFD::BDSManager::GetEdition()  // sufficiently connected as unit was removed in LRFD 7th Edition 2014
         )
      {
         pLLDF = new WBFL::LRFD::LldfTypeF(plldf->gdrNum,
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
                                   plldf->b,
                                   plldf->d,
                                   plldf->leftDe,
                                   plldf->rightDe,
                                   plldf->PoissonRatio,
                                   plldf->skew1, 
                                   plldf->skew2,
                                   bSkewMoment,
                                   bSkewShear);
      }
      else
      {

         pLLDF = new WBFL::LRFD::LldfTypeG(plldf->gdrNum,
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
                            plldf->b,
                            plldf->d,
                            plldf->leftDe,
                            plldf->rightDe,
                            plldf->PoissonRatio,
                            plldf->skew1, 
                            plldf->skew2,
                            bSkewMoment,
                            bSkewShear);
            
            
      }
   }

   GET_IFACE2(GetBroker(), ILiveLoads,pLiveLoads);
   pLLDF->SetRangeOfApplicability( pLiveLoads->GetRangeOfApplicabilityAction(), roaVal );

   plldf->bExteriorGirder = pBridge->IsExteriorGirder(segmentKey);

   return pLLDF;
}

void VoidedSlab2DistFactorEngineer::ReportMoment(rptParagraph* pPara,VOIDEDSLAB_LLDFDETAILS& lldf,WBFL::LRFD::ILiveLoadDistributionFactor::DFResult& gM1,WBFL::LRFD::ILiveLoadDistributionFactor::DFResult& gM2,Float64 gM,bool bSIUnits,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits)
{
   std::_tstring strImagePath(rptStyleManager::GetImagePath());

   INIT_UV_PROTOTYPE( rptLengthUnitValue,    xdim,     pDisplayUnits->GetSpanLengthUnit(),      true );

   INIT_SCALAR_PROTOTYPE(rptRcScalar, scalar, pDisplayUnits->GetScalarFormat());

   GET_IFACE2(GetBroker(), ILibrary, pLib);
   GET_IFACE2(GetBroker(), ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

   if ( lldf.bExteriorGirder )
   {
      if (gM1.LeverRuleData.bWasUsed)
      {
         (*pPara) << Bold(_T("1 Loaded Lane: Lever Rule")) << rptNewLine;
         REPORT_LLDF_INTOVERRIDE(gM1);
         ReportLeverRule(pPara, true, 1.0, gM1.LeverRuleData, GetBroker(), pDisplayUnits);
      }

      if (gM1.EqnData.bWasUsed)
      {
         if (gM1.ControllingMethod & WBFL::LRFD::INTERIOR_OVERRIDE)
         {
            (*pPara) << Bold(_T("1 Loaded Lane: Exterior factor may not be less than that for interior")) << rptNewLine;
            (*pPara) << _T("mg") << Super(_T("ME")) << Sub(_T("1")) << _T(" = ") << _T("mg") << Super(_T("MI")) << Sub(_T("1")) << _T(" = ") << scalar.SetValue(gM1.mg) << rptNewLine;
         }
         else
         {

            (*pPara) << Bold(_T("1 Loaded Lane: Equation")) << rptNewLine;
            if (lldf.Method == pgsTypes::LiveLoadDistributionFactorMethod::TxDOT && !(gM1.ControllingMethod & WBFL::LRFD::LEVER_RULE))
            {
               (*pPara) << _T("For TxDOT Method, Use ") << _T("mg") << Super(_T("MI")) << Sub(_T("1")) << _T(". And,do not apply skew correction factor.") << rptNewLine;

               (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("mg_1_MI_Type_G_SI.png") : _T("mg_1_MI_Type_G_US.png"))) << rptNewLine;
               ATLASSERT(gM1.ControllingMethod & WBFL::LRFD::S_OVER_D_METHOD);
               (*pPara) << _T("K = ") << gM1.EqnData.K << rptNewLine;
               (*pPara) << _T("C = ") << gM1.EqnData.C << rptNewLine;
               (*pPara) << _T("D = ") << xdim.SetValue(gM1.EqnData.D) << rptNewLine;
               (*pPara) << rptNewLine;

               (*pPara) << _T("mg") << Super(_T("ME")) << Sub(_T("1")) << _T(" = ") << scalar.SetValue(gM1.mg) << rptNewLine;

            }
            else
            {
               (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("mg_1_ME_Type_G_SI.png") : _T("mg_1_ME_Type_G_US.png"))) << rptNewLine;

               if (lldf.TransverseConnectivity == pgsTypes::atcConnectedAsUnit || WBFL::LRFD::BDSManager::Edition::SeventhEdition2014 <= WBFL::LRFD::BDSManager::GetEdition())
               {
                  (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("mg_1_MI_Type_F_SI.png") : _T("mg_1_MI_Type_F_US.png"))) << rptNewLine;
               }
               else
               {
                  (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("mg_1_MI_Type_G_SI.png") : _T("mg_1_MI_Type_G_US.png"))) << rptNewLine;
                  ATLASSERT(gM1.ControllingMethod & WBFL::LRFD::S_OVER_D_METHOD);
                  (*pPara) << _T("K = ") << gM1.EqnData.K << rptNewLine;
                  (*pPara) << _T("C = ") << gM1.EqnData.C << rptNewLine;
                  (*pPara) << _T("D = ") << xdim.SetValue(gM1.EqnData.D) << rptNewLine;
                  (*pPara) << rptNewLine;
               }

               (*pPara) << _T("mg") << Super(_T("MI")) << Sub(_T("1")) << _T(" = ") << scalar.SetValue(gM1.EqnData.mg) << rptNewLine;
               (*pPara) << _T("e = ") << gM1.EqnData.e << rptNewLine;
               (*pPara) << _T("mg") << Super(_T("ME")) << Sub(_T("1")) << _T(" = ") << scalar.SetValue(gM1.EqnData.mg * gM1.EqnData.e) << rptNewLine;
            }
         }
      }

      if (gM1.LanesBeamsData.bWasUsed)
      {
         (*pPara) << Bold(_T("1 Loaded Lane: Number of Lanes over Number of Beams - Factor cannot be less than this")) << rptNewLine;
         (*pPara) << _T("Skew correction is not applied to Lanes/Beams method") << rptNewLine;
         ReportLanesBeamsMethod(pPara, gM1.LanesBeamsData, GetBroker(), pDisplayUnits);
      }
       
      if (2 <= lldf.Nl)
      {
         if (gM2.LeverRuleData.bWasUsed)
         {
            ATLASSERT(gM2.ControllingMethod & WBFL::LRFD::LEVER_RULE);
            (*pPara) << rptNewLine;
            (*pPara) << Bold(_T("2+ Loaded Lanes: Lever Rule")) << rptNewLine;
            REPORT_LLDF_INTOVERRIDE(gM2);
            ReportLeverRule(pPara, true, 1.0, gM2.LeverRuleData, GetBroker(), pDisplayUnits);
         }

         if (gM2.ControllingMethod & WBFL::LRFD::INTERIOR_OVERRIDE)
         {
            (*pPara) << Bold(_T("1 Loaded Lane: Exterior factor may not be less than that for interior")) << rptNewLine;
            (*pPara) << _T("mg") << Super(_T("ME")) << Sub(_T("2")) << _T(" = ") << _T("mg") << Super(_T("MI")) << Sub(_T("2")) << _T(" = ") << scalar.SetValue(gM2.mg) << rptNewLine;
         }
         else
         {

            if (gM2.EqnData.bWasUsed)
            {
               if (lldf.Method == pgsTypes::LiveLoadDistributionFactorMethod::TxDOT && !(gM2.ControllingMethod & WBFL::LRFD::LEVER_RULE))
               {
                  (*pPara) << Bold(_T("2+ Loaded Lanes: Equation Method")) << rptNewLine;
                  (*pPara) << _T("Same as for 1 Loaded Lane") << rptNewLine;
                  (*pPara) << _T("mg") << Super(_T("ME")) << Sub(_T("2")) << _T(" = ") << scalar.SetValue(gM2.EqnData.mg) << rptNewLine;
               }
               else
               {
                  (*pPara) << Bold(_T("2+ Loaded Lane")) << rptNewLine;
                  (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("mg_2_ME_Type_G_SI.png") : _T("mg_2_ME_Type_G_US.png"))) << rptNewLine;

                  if (lldf.TransverseConnectivity == pgsTypes::atcConnectedAsUnit || WBFL::LRFD::BDSManager::Edition::SeventhEdition2014 <= WBFL::LRFD::BDSManager::GetEdition())
                  {
                     (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("mg_2_MI_Type_F_SI.png") : _T("mg_2_MI_Type_F_US.png"))) << rptNewLine;
                  }
                  else
                  {
                     (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("mg_2_MI_Type_G_SI.png") : _T("mg_2_MI_Type_G_US.png"))) << rptNewLine;
                     ATLASSERT(gM2.ControllingMethod & WBFL::LRFD::S_OVER_D_METHOD);
                     (*pPara) << _T("K = ") << gM2.EqnData.K << rptNewLine;
                     (*pPara) << _T("C = ") << gM2.EqnData.C << rptNewLine;
                     (*pPara) << _T("D = ") << xdim.SetValue(gM2.EqnData.D) << rptNewLine;
                     (*pPara) << rptNewLine;
                  }

                  (*pPara) << _T("mg") << Super(_T("MI")) << Sub(_T("2+")) << _T(" = ") << scalar.SetValue(gM2.EqnData.mg) << rptNewLine;
                  (*pPara) << _T("e = ") << gM2.EqnData.e << rptNewLine;
                  (*pPara) << _T("mg") << Super(_T("ME")) << Sub(_T("2+")) << _T(" = ") << scalar.SetValue(gM2.EqnData.mg * gM2.EqnData.e) << rptNewLine;
               }
            }
         }

         if (gM2.LanesBeamsData.bWasUsed)
         {
            (*pPara) << Bold(_T("2+ Loaded Lane: Number of Lanes over Number of Beams - Factor cannot be less than this")) << rptNewLine;
            (*pPara) << _T("Skew correction is not applied to Lanes/Beams method") << rptNewLine;
            ReportLanesBeamsMethod(pPara, gM2.LanesBeamsData, GetBroker(), pDisplayUnits);
         }

         (*pPara) << rptNewLine;

         if (gM1.ControllingMethod & WBFL::LRFD::MOMENT_SKEW_CORRECTION_APPLIED)
         {
            (*pPara) << Bold(_T("Skew Correction")) << rptNewLine;
            if (lldf.Method != pgsTypes::LiveLoadDistributionFactorMethod::TxDOT)
            {
               Float64 skew_delta_max = WBFL::Units::ConvertToSysUnits(10.0, WBFL::Units::Measure::Degree);
               if (fabs(lldf.skew1 - lldf.skew2) < skew_delta_max)
               {
                  (*pPara) << rptRcImage(strImagePath + _T("SkewCorrection_Moment_TypeC.png")) << rptNewLine;
               }
            }

            (*pPara) << _T("Skew Correction Factor: = ") << scalar.SetValue(gM1.SkewCorrectionFactor) << rptNewLine;
            (*pPara) << _T("Skew Corrected Factor: mg") << Super(_T("ME")) << Sub(_T("1")) << _T(" = ") << scalar.SetValue(gM1.mg);
            (lldf.Nl == 1 || gM1.mg >= gM2.mg) ? (*pPara) << Bold(_T(" < Controls")) << rptNewLine : (*pPara) << rptNewLine;
            if (lldf.Nl >= 2)
            {
               (*pPara) << _T("Skew Corrected Factor: mg") << Super(_T("ME")) << Sub(_T("2+")) << _T(" = ") << scalar.SetValue(gM2.mg);
               (gM2.mg > gM1.mg) ? (*pPara) << Bold(_T(" < Controls")) << rptNewLine : (*pPara) << rptNewLine;
            }
         }
      }
   }
   else
   {
      // Interior Girder
      if ( gM1.LeverRuleData.bWasUsed )
      {
         (*pPara) << Bold(_T("1 Loaded Lane: Lever Rule")) << rptNewLine;
         ReportLeverRule(pPara,true,1.0,gM1.LeverRuleData,GetBroker(),pDisplayUnits);
      }

      if (gM1.EqnData.bWasUsed)
      {
         (*pPara) << Bold(_T("1 Loaded Lane: Equation")) << rptNewLine;
         if (lldf.Method == pgsTypes::LiveLoadDistributionFactorMethod::TxDOT && !(gM1.ControllingMethod & WBFL::LRFD::LEVER_RULE))
         {
            (*pPara) << _T("For TxDOT Method, do not apply skew correction factor.")<< rptNewLine;

            (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("mg_1_MI_Type_G_SI.png") : _T("mg_1_MI_Type_G_US.png"))) << rptNewLine;
            ATLASSERT(gM1.ControllingMethod & WBFL::LRFD::S_OVER_D_METHOD);
            (*pPara)<< _T("K = ")<< gM1.EqnData.K << rptNewLine;
            (*pPara)<< _T("C = ")<< gM1.EqnData.C << rptNewLine;
            (*pPara)<< _T("D = ")<< xdim.SetValue(gM1.EqnData.D) << rptNewLine;
            (*pPara) << rptNewLine;

            (*pPara) << _T("mg") << Super(_T("MI")) << Sub(_T("1")) << _T(" = ") << scalar.SetValue(gM1.mg) << rptNewLine;
         }
         else
         {
            if ( lldf.TransverseConnectivity == pgsTypes::atcConnectedAsUnit || WBFL::LRFD::BDSManager::Edition::SeventhEdition2014 <= WBFL::LRFD::BDSManager::GetEdition() )
            {
               (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("mg_1_MI_Type_F_SI.png") : _T("mg_1_MI_Type_F_US.png"))) << rptNewLine;
            }
            else
            {
               (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("mg_1_MI_Type_G_SI.png") : _T("mg_1_MI_Type_G_US.png"))) << rptNewLine;
               ATLASSERT(gM1.ControllingMethod & WBFL::LRFD::S_OVER_D_METHOD);
               (*pPara)<< _T("K = ")<< gM1.EqnData.K << rptNewLine;
               (*pPara)<< _T("C = ")<< gM1.EqnData.C << rptNewLine;
               (*pPara)<< _T("D = ")<< xdim.SetValue(gM1.EqnData.D) << rptNewLine;
               (*pPara) << rptNewLine;
            }

            (*pPara) << _T("mg") << Super(_T("MI")) << Sub(_T("1")) << _T(" = ") << scalar.SetValue(gM1.EqnData.mg) << rptNewLine;
         }
      }

      if ( gM1.LanesBeamsData.bWasUsed )
      {
         (*pPara) << Bold(_T("1 Loaded Lane: Number of Lanes over Number of Beams - Factor cannot be less than this")) << rptNewLine;
         (*pPara) << _T("Skew correction is not applied to Lanes/Beams method")<< rptNewLine;
         ReportLanesBeamsMethod(pPara,gM1.LanesBeamsData,GetBroker(),pDisplayUnits);
      }

      if ( 2 <= lldf.Nl )
      {
         if ( gM2.LeverRuleData.bWasUsed )
         {
            (*pPara) << rptNewLine;
            (*pPara) << Bold(_T("2+ Loaded Lanes")) << rptNewLine;
            (*pPara) << Bold(_T("Lever Rule")) << rptNewLine;
            ReportLeverRule(pPara,true,1.0,gM2.LeverRuleData,GetBroker(),pDisplayUnits);
         }

         if (gM2.EqnData.bWasUsed)
         {
            if (lldf.Method == pgsTypes::LiveLoadDistributionFactorMethod::TxDOT && !(gM2.ControllingMethod & WBFL::LRFD::LEVER_RULE))
            {
               (*pPara) << rptNewLine;

               (*pPara) << Bold(_T("2+ Loaded Lanes: Equation Method")) << rptNewLine;
               (*pPara) << _T("Same as for 1 Loaded Lane") << rptNewLine;
               (*pPara) << _T("mg") << Super(_T("MI")) << Sub(_T("2")) << _T(" = ") << scalar.SetValue(gM2.EqnData.mg) << rptNewLine;
            }
            else
            {
               (*pPara) << rptNewLine;

               (*pPara) << Bold(_T("2+ Loaded Lanes")) << rptNewLine;

               if ( lldf.TransverseConnectivity == pgsTypes::atcConnectedAsUnit || WBFL::LRFD::BDSManager::Edition::SeventhEdition2014 <= WBFL::LRFD::BDSManager::GetEdition() )
               {
                  (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("mg_2_MI_Type_F_SI.png") : _T("mg_2_MI_Type_F_US.png"))) << rptNewLine;
               }
               else
               {
                  (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("mg_2_MI_Type_G_SI.png") : _T("mg_2_MI_Type_G_US.png"))) << rptNewLine;
                  ATLASSERT(gM2.ControllingMethod & WBFL::LRFD::S_OVER_D_METHOD);
                  (*pPara)<< _T("K = ")<< gM2.EqnData.K << rptNewLine;
                  (*pPara)<< _T("C = ")<< gM2.EqnData.C << rptNewLine;
                  (*pPara)<< _T("D = ")<< xdim.SetValue(gM2.EqnData.D) << rptNewLine;
                  (*pPara) << rptNewLine;
               }

               (*pPara) << _T("mg") << Super(_T("MI")) << Sub(_T("2+")) << _T(" = ") << scalar.SetValue(gM2.EqnData.mg) << rptNewLine;
            }
         }

         if ( gM2.LanesBeamsData.bWasUsed )
         {
            (*pPara) << Bold(_T("2+ Loaded Lane: Number of Lanes over Number of Beams - Factor cannot be less than this")) << rptNewLine;
            (*pPara) << _T("Skew correction is not applied to Lanes/Beams method")<< rptNewLine;
            ReportLanesBeamsMethod(pPara,gM2.LanesBeamsData,GetBroker(),pDisplayUnits);
         }
      }

      (*pPara) << rptNewLine;

      if ( gM1.ControllingMethod & WBFL::LRFD::MOMENT_SKEW_CORRECTION_APPLIED )
      {
         (*pPara) << Bold(_T("Skew Correction")) << rptNewLine;
         if(lldf.Method != pgsTypes::LiveLoadDistributionFactorMethod::TxDOT)
         {
            Float64 skew_delta_max = WBFL::Units::ConvertToSysUnits( 10.0, WBFL::Units::Measure::Degree );
            if ( fabs(lldf.skew1 - lldf.skew2) < skew_delta_max )
            {
               (*pPara) << rptRcImage(strImagePath + _T("SkewCorrection_Moment_TypeC.png")) << rptNewLine;
            }
         }
   
         (*pPara) << _T("Skew Correction Factor: = ") << scalar.SetValue(gM1.SkewCorrectionFactor) << rptNewLine;
         (*pPara) << _T("Skew Corrected Factor: mg") << Super(_T("MI")) << Sub(_T("1")) << _T(" = ") << scalar.SetValue(gM1.mg);
         (lldf.Nl == 1 || gM1.mg >= gM2.mg) ? (*pPara) << Bold(_T(" < Controls")) << rptNewLine : (*pPara) << rptNewLine;
         if ( 2 <= lldf.Nl )
         {
            (*pPara) << _T("Skew Corrected Factor: mg") << Super(_T("MI")) << Sub(_T("2+")) << _T(" = ") << scalar.SetValue(gM2.mg);
            (gM2.mg > gM1.mg) ? (*pPara) << Bold(_T(" < Controls")) << rptNewLine : (*pPara) << rptNewLine;
         }
      }
   }
}

void VoidedSlab2DistFactorEngineer::ReportShear(rptParagraph* pPara,VOIDEDSLAB_LLDFDETAILS& lldf,WBFL::LRFD::ILiveLoadDistributionFactor::DFResult& gV1,WBFL::LRFD::ILiveLoadDistributionFactor::DFResult& gV2,Float64 gV,bool bSIUnits,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits)
{
   std::_tstring strImagePath(rptStyleManager::GetImagePath());

   INIT_UV_PROTOTYPE( rptLengthUnitValue,    xdim,     pDisplayUnits->GetSpanLengthUnit(),      true );

   INIT_SCALAR_PROTOTYPE(rptRcScalar, scalar, pDisplayUnits->GetScalarFormat());

   GET_IFACE2(GetBroker(), ILibrary, pLib);
   GET_IFACE2(GetBroker(), ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

   if ( lldf.bExteriorGirder )
   {
      if (gV1.LeverRuleData.bWasUsed)
      {
         (*pPara) << Bold(_T("1 Loaded Lane: Lever Rule")) << rptNewLine;
         REPORT_LLDF_INTOVERRIDE(gV1);
         ReportLeverRule(pPara, false, 1.0, gV1.LeverRuleData, GetBroker(), pDisplayUnits);
      }

      if (gV1.EqnData.bWasUsed)
      {
         if (gV1.ControllingMethod & WBFL::LRFD::INTERIOR_OVERRIDE)
         {
            (*pPara) << Bold(_T("1 Loaded Lane: Exterior factor may not be less than that for interior")) << rptNewLine;
            (*pPara) << _T("mg") << Super(_T("VE")) << Sub(_T("1")) << _T(" = ") << _T("mg") << Super(_T("VI")) << Sub(_T("1")) << _T(" = ") << scalar.SetValue(gV1.mg) << rptNewLine;
         }
         else
         {

            (*pPara) << Bold(_T("1 Loaded Lane: Equation")) << rptNewLine;
            if (lldf.Method == pgsTypes::LiveLoadDistributionFactorMethod::TxDOT)
            {
               (*pPara) << _T("For TxDOT Method, Use ") << _T("mg") << Super(_T("MI")) << Sub(_T("1")) << _T(". And,do not apply skew correction factor.") << rptNewLine;

               (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("mg_1_MI_Type_G_SI.png") : _T("mg_1_MI_Type_G_US.png"))) << rptNewLine;
               ATLASSERT(gV1.ControllingMethod & WBFL::LRFD::S_OVER_D_METHOD);
               (*pPara) << _T("K = ") << gV1.EqnData.K << rptNewLine;
               (*pPara) << _T("C = ") << gV1.EqnData.C << rptNewLine;
               (*pPara) << _T("D = ") << xdim.SetValue(gV1.EqnData.D) << rptNewLine;
               (*pPara) << rptNewLine;

               (*pPara) << _T("mg") << Super(_T("VE")) << Sub(_T("1")) << _T(" = ") << scalar.SetValue(gV1.mg) << rptNewLine;
            }
            else
            {

               if (gV1.ControllingMethod & WBFL::LRFD::MOMENT_OVERRIDE)
               {
                  (*pPara) << _T("Overridden by moment factor because J or I was out of range for shear equation") << rptNewLine;
                  (*pPara) << _T("e = ") << gV1.EqnData.e << rptNewLine;
                  (*pPara) << _T("mg") << Super(_T("VE")) << Sub(_T("1")) << _T(" = ") << _T("(e)(mg") << Super(_T("MI")) << Sub(_T("1")) << _T(") = ") << scalar.SetValue(gV1.EqnData.mg * gV1.EqnData.e) << rptNewLine;
               }
               else
               {
                  (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("mg_1_VE_Type_G_SI.png") : _T("mg_1_VE_Type_G_US.png"))) << rptNewLine;
                  (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("mg_1_VI_Type_G_SI.png") : _T("mg_1_VI_Type_G_US.png"))) << rptNewLine;
                  (*pPara) << _T("mg") << Super(_T("VI")) << Sub(_T("1")) << _T(" = ") << scalar.SetValue(gV1.EqnData.mg) << rptNewLine;
                  (*pPara) << _T("e = ") << gV1.EqnData.e << rptNewLine;
                  (*pPara) << _T("mg") << Super(_T("VE")) << Sub(_T("1")) << _T(" = ") << scalar.SetValue(gV1.EqnData.mg * gV1.EqnData.e) << rptNewLine;
               }
            }
         }

         if (gV1.LanesBeamsData.bWasUsed)
         {
            (*pPara) << Bold(_T("1 Loaded Lane: Number of Lanes over Number of Beams - Factor cannot be less than this")) << rptNewLine;
            (*pPara) << _T("Skew correction is not applied to Lanes/Beams method") << rptNewLine;
            ReportLanesBeamsMethod(pPara, gV1.LanesBeamsData, GetBroker(), pDisplayUnits);
         }
      }

      if ( 2 <= lldf.Nl )
      {
         (*pPara) << rptNewLine;
         if (gV2.ControllingMethod & WBFL::LRFD::LEVER_RULE)
         {
            (*pPara) << Bold(_T("2+ Loaded Lane: Lever Rule")) << rptNewLine;
            REPORT_LLDF_INTOVERRIDE(gV2);
            ReportLeverRule(pPara, false, 1.0, gV2.LeverRuleData, GetBroker(), pDisplayUnits);
         }

         if (gV2.ControllingMethod & WBFL::LRFD::INTERIOR_OVERRIDE)
         {
            (*pPara) << Bold(_T("2+ Loaded Lanes: Exterior factor may not be less than that for interior")) << rptNewLine;
            (*pPara) << _T("mg") << Super(_T("VE")) << Sub(_T("2")) << _T(" = ") << _T("mg") << Super(_T("VI")) << Sub(_T("2")) << _T(" = ") << scalar.SetValue(gV2.mg) << rptNewLine;
         }
         else
         {
            if (gV1.EqnData.bWasUsed)
            {
               if (lldf.Method == pgsTypes::LiveLoadDistributionFactorMethod::TxDOT)
               {
                  (*pPara) << Bold(_T("2+ Loaded Lane")) << rptNewLine;
                  (*pPara) << _T("Same as for 1 Loaded Lane") << rptNewLine;
                  (*pPara) << _T("mg") << Super(_T("VE")) << Sub(_T("2")) << _T(" = ") << scalar.SetValue(gV2.mg) << rptNewLine;
               }
               else
               {
                  (*pPara) << Bold(_T("2+ Loaded Lane: Equation")) << rptNewLine;

                  if (gV2.ControllingMethod & WBFL::LRFD::MOMENT_OVERRIDE)
                  {
                     (*pPara) << _T("Overridden by moment factor because J or I was out of range for shear equation") << rptNewLine;
                     (*pPara) << _T("e = ") << gV2.EqnData.e << rptNewLine;
                     (*pPara) << _T("mg") << Super(_T("VE")) << Sub(_T("2")) << _T(" = ") << _T("(e)(mg") << Super(_T("ME")) << Sub(_T("2")) << _T(") = ") << scalar.SetValue(gV2.EqnData.mg * gV2.EqnData.e) << rptNewLine;
                  }
                  else
                  {
                     (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("mg_2_VE_Type_G_SI.png") : _T("mg_2_VE_Type_G_US.png"))) << rptNewLine;
                     (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("mg_2_VI_Type_G_SI.png") : _T("mg_2_VI_Type_G_US.png"))) << rptNewLine;
                     (*pPara) << _T("mg") << Super(_T("VI")) << Sub(_T("2+")) << _T(" = ") << scalar.SetValue(gV2.EqnData.mg) << rptNewLine;
                     (*pPara) << _T("e = ") << gV2.EqnData.e << rptNewLine;
                     (*pPara) << _T("mg") << Super(_T("VE")) << Sub(_T("2+")) << _T(" = ") << scalar.SetValue(gV2.EqnData.mg * gV2.EqnData.e) << rptNewLine;
                  }
               }
            }

            if (gV2.LanesBeamsData.bWasUsed)
            {
               (*pPara) << Bold(_T("2+ Loaded Lane: Number of Lanes over Number of Beams - Factor cannot be less than this")) << rptNewLine;
               (*pPara) << _T("Skew correction is not applied to Lanes/Beams method") << rptNewLine;
               ReportLanesBeamsMethod(pPara, gV2.LanesBeamsData, GetBroker(), pDisplayUnits);
            }
         }

         (*pPara) << rptNewLine;

         if ( gV1.ControllingMethod & WBFL::LRFD::SHEAR_SKEW_CORRECTION_APPLIED )
         {
            (*pPara) << Bold(_T("Skew Correction")) << rptNewLine;
            if(lldf.Method != pgsTypes::LiveLoadDistributionFactorMethod::TxDOT)
            {
               (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("SkewCorrection_Shear_TypeF_SI.png") : _T("SkewCorrection_Shear_TypeF_US.png"))) << rptNewLine;
            }

            (*pPara) << _T("Skew Correction Factor: = ") << scalar.SetValue(gV1.SkewCorrectionFactor) << rptNewLine;
            (*pPara) << _T("Skew Corrected Factor: mg") << Super(_T("VE")) << Sub(_T("1")) << _T(" = ") << scalar.SetValue(gV1.mg);
            (lldf.Nl == 1 || gV1.mg >= gV2.mg) ? (*pPara) << Bold(_T(" < Controls")) << rptNewLine : (*pPara) << rptNewLine;
            if ( 2 <= lldf.Nl )
            {
               (*pPara) << _T("Skew Corrected Factor: mg") << Super(_T("VE")) << Sub(_T("2+")) << _T(" = ") << scalar.SetValue(gV2.mg);
               (gV2.mg > gV1.mg) ? (*pPara) << Bold(_T(" < Controls")) << rptNewLine : (*pPara) << rptNewLine;
            }
         }
      }
   }
   else
   {
      // Interior Girder
      //
      // Shear
      //

      if ( gV1.EqnData.bWasUsed )
      {
         (*pPara) << Bold(_T("1 Loaded Lane: Equation")) << rptNewLine;
         if( lldf.Method== pgsTypes::LiveLoadDistributionFactorMethod::TxDOT)
         {
            (*pPara) << _T("For TxDOT Method, Use ")<<_T("mg") << Super(_T("MI")) << Sub(_T("1"))<<_T(". And,do not apply shear correction factor.")<< rptNewLine;

            (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("mg_1_MI_Type_G_SI.png") : _T("mg_1_MI_Type_G_US.png"))) << rptNewLine;
            ATLASSERT(gV1.ControllingMethod &   WBFL::LRFD::S_OVER_D_METHOD);
            (*pPara)<< _T("K = ")<< gV1.EqnData.K << rptNewLine;
            (*pPara)<< _T("C = ")<< gV1.EqnData.C << rptNewLine;
            (*pPara)<< _T("D = ")<< xdim.SetValue(gV1.EqnData.D) << rptNewLine;
            (*pPara) << rptNewLine;

            (*pPara) << _T("mg") << Super(_T("VI")) << Sub(_T("1")) << _T(" = ") << scalar.SetValue(gV1.mg) << rptNewLine;
         }
         else 
         {
            if (gV1.ControllingMethod & WBFL::LRFD::MOMENT_OVERRIDE)
            {
               (*pPara) << _T("Overridden by moment factor because J or I was out of range for shear equation")<<rptNewLine;
               (*pPara) << _T("mg") << Super(_T("VI")) << Sub(_T("1")) << _T(" = ") << _T("mg") << Super(_T("MI")) << Sub(_T("1")) << _T(" = ") << scalar.SetValue(gV1.mg) << rptNewLine;
            }
            else
            {
               (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("mg_1_VI_Type_G_SI.png") : _T("mg_1_VI_Type_G_US.png"))) << rptNewLine;
               (*pPara) << _T("mg") << Super(_T("VI")) << Sub(_T("1")) << _T(" = ") << scalar.SetValue(gV1.EqnData.mg) << rptNewLine;
            }
         }
      }

      if ( gV1.LeverRuleData.bWasUsed )
      {
         (*pPara) << Bold(_T("Lever Rule")) << rptNewLine;
         ReportLeverRule(pPara,false,1.0,gV1.LeverRuleData,GetBroker(),pDisplayUnits);
      }

      if ( gV1.LanesBeamsData.bWasUsed )
      {
         (*pPara) << Bold(_T("1 Loaded Lane: Number of Lanes over Number of Beams - Factor cannot be less than this")) << rptNewLine;
         (*pPara) << _T("Skew correction is not applied to Lanes/Beams method")<< rptNewLine;
         ReportLanesBeamsMethod(pPara,gV1.LanesBeamsData,GetBroker(),pDisplayUnits);
      }

      if ( 2 <= lldf.Nl )
      {
         if ( gV2.EqnData.bWasUsed )
         {
            (*pPara) << Bold(_T("2+ Loaded Lane: Equation")) << rptNewLine;
            if( lldf.Method== pgsTypes::LiveLoadDistributionFactorMethod::TxDOT)
            {
               (*pPara) << Bold(_T("2+ Loaded Lane")) << rptNewLine;
               (*pPara) << _T("Same as for 1 Loaded Lane") << rptNewLine;
               (*pPara) << _T("mg") << Super(_T("VI")) << Sub(_T("2")) << _T(" = ") << scalar.SetValue(gV2.mg) << rptNewLine;
            }
            else
            {
               if (gV2.ControllingMethod & WBFL::LRFD::MOMENT_OVERRIDE)
               {
                  (*pPara) << _T("Overridden by moment factor because J or I was out of range for shear equation")<<rptNewLine;
                  (*pPara) << _T("mg") << Super(_T("VI")) << Sub(_T("2")) << _T(" = ") << _T("mg") << Super(_T("MI")) << Sub(_T("2")) << _T(" = ") << scalar.SetValue(gV2.mg) << rptNewLine;
               }
               else
               {
                  (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("mg_2_VI_Type_G_SI.png") : _T("mg_2_VI_Type_G_US.png"))) << rptNewLine;
                  (*pPara) << _T("mg") << Super(_T("VI")) << Sub(_T("2+")) << _T(" = ") << scalar.SetValue(gV2.EqnData.mg) << rptNewLine;
               }
            }
         }

         if ( gV2.LeverRuleData.bWasUsed)
         {
            (*pPara) << Bold(_T("2+ Loaded Lane: Lever Rule")) << rptNewLine;
            ReportLeverRule(pPara,false,1.0,gV2.LeverRuleData,GetBroker(),pDisplayUnits);
         }

         if ( gV2.LanesBeamsData.bWasUsed )
         {
            (*pPara) << Bold(_T("2+ Loaded Lane: Number of Lanes over Number of Beams - Factor cannot be less than this")) << rptNewLine;
            (*pPara) << _T("Skew correction is not applied to Lanes/Beams method")<< rptNewLine;
            ReportLanesBeamsMethod(pPara,gV2.LanesBeamsData,GetBroker(),pDisplayUnits);
         }
      }

      (*pPara) << rptNewLine;

      if ( gV1.ControllingMethod & WBFL::LRFD::SHEAR_SKEW_CORRECTION_APPLIED )
      {
         (*pPara) << Bold(_T("Skew Correction")) << rptNewLine;
         (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("SkewCorrection_Shear_TypeF_SI.png") : _T("SkewCorrection_Shear_TypeF_US.png"))) << rptNewLine;
         (*pPara) << _T("Skew Correction Factor: = ") << scalar.SetValue(gV1.SkewCorrectionFactor) << rptNewLine;
         (*pPara) << rptNewLine;
         (*pPara) << _T("Skew Corrected Factor: mg") << Super(_T("VI")) << Sub(_T("1")) << _T(" = ") << scalar.SetValue(gV1.mg);
         (lldf.Nl == 1 || gV1.mg >= gV2.mg) ? (*pPara) << Bold(_T(" < Controls")) << rptNewLine : (*pPara) << rptNewLine;
         if ( 2 <= lldf.Nl )
         {
            (*pPara) << _T("Skew Corrected Factor: mg") << Super(_T("VI")) << Sub(_T("2+")) << _T(" = ") << scalar.SetValue(gV2.mg);
            (gV2.mg > gV1.mg) ? (*pPara) << Bold(_T(" < Controls")) << rptNewLine : (*pPara) << rptNewLine;
         }
      }
   }
}

std::_tstring VoidedSlab2DistFactorEngineer::GetComputationDescription(const CGirderKey& girderKey,const std::_tstring& libraryEntryName,pgsTypes::SupportedDeckType decktype, pgsTypes::AdjacentTransverseConnectivity connect)
{
   GET_IFACE2(GetBroker(), ILibrary, pLib);
   GET_IFACE2(GetBroker(), ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   const auto& live_load_distribution_criteria = pSpecEntry->GetLiveLoadDistributionCriteria();

   auto lldfMethod = live_load_distribution_criteria.LldfMethod;

   std::_tstring descr;
   if ( lldfMethod == pgsTypes::LiveLoadDistributionFactorMethod::TxDOT)
   {
      descr += std::_tstring(_T("TxDOT per TxDOT Bridge Design Manual - LRFD"));
   }
   else if ( lldfMethod == pgsTypes::LiveLoadDistributionFactorMethod::LRFD || lldfMethod == pgsTypes::LiveLoadDistributionFactorMethod::WSDOT)
   {
      if (decktype == pgsTypes::sdtCompositeCIP || decktype == pgsTypes::sdtCompositeOverlay)
      {
         descr += std::_tstring(_T("AASHTO Type (f) using AASHTO LRFD Method per Article 4.6.2.2"));
      }
      else
      {
         descr += std::_tstring(_T("AASHTO Type (g) using AASHTO LRFD Method per Article 4.6.2.2 with determination of transverse connectivity."));
      }
   }
   else
   {
      ATLASSERT(false);
   }

   // Special text if ROA is ignored
   GET_IFACE2(GetBroker(), ILiveLoads,pLiveLoads);
   std::_tstring strAction( pLiveLoads->GetLLDFSpecialActionText() );
   if ( !strAction.empty() )
   {
      descr += strAction;
   }

   return descr;
}
