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

// BoxBeamDistFactorEngineer.cpp : Implementation of CBoxBeamDistFactorEngineer
#include "stdafx.h"
#include "BoxBeamDistFactorEngineer.h"
#include <WBFLCore.h>
#include <Units\Convert.h>
#include <PsgLib\TrafficBarrierEntry.h>
#include <PsgLib\SpecLibraryEntry.h>
#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\StatusItem.h>
#include <PgsExt\GirderLabel.h>
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\DistributionFactors.h>
#include <IFace\StatusCenter.h>
#include <IFace\Intervals.h>
#include <WBFLCogo.h>
#include <LRFD\LldfTypeG.h>
#include <Beams\Helper.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBoxBeamDistFactorEngineer
HRESULT CBoxBeamDistFactorEngineer::FinalConstruct()
{
   return S_OK;
}

void CBoxBeamDistFactorEngineer::BuildReport(const CGirderKey& girderKey,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits)
{
   // Grab the interfaces that are needed
   GET_IFACE(IBridge,pBridge);

   // Do some initial se
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

      SPANDETAILS span_lldf = GetSpanDF(spanKey,pgsTypes::StrengthI);

      PierIndexType pier1 = spanIdx;
      PierIndexType pier2 = spanIdx+1;

      auto pier1_lldf = GetPierDF(pier1, gdrIdx, pgsTypes::StrengthI, pgsTypes::Ahead);
      auto pier2_lldf = GetPierDF(pier2, gdrIdx, pgsTypes::StrengthI, pgsTypes::Back);

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

      rptParagraph* pPara;

      const CSpanData2* pSpan = pBridgeDesc->GetSpan(spanIdx);
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(pSpan);

      pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
      (*pPara) << _T("Method of Computation:")<<rptNewLine;
      (*pChapter) << pPara;
      pPara = new rptParagraph;
      (*pChapter) << pPara;
      std::_tstring strGirderName = pGroup->GetGirder(gdrIdx)->GetGirderName();
      (*pPara) << GetComputationDescription(girderKey,strGirderName,pDeck->GetDeckType(),pDeck->TransverseConnectivity);

      pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
      (*pChapter) << pPara;
      (*pPara) << _T("Distribution Factor Parameters") << rptNewLine;
      pPara = new rptParagraph;
      (*pChapter) << pPara;

      Float64 station,offset;
      ATLASSERT(pGroup->GetGirder(gdrIdx)->GetSegmentCount() == 1); 
      CSegmentKey segmentKey(girderKey,0);
      pBridge->GetStationAndOffset(segmentKey,span_lldf.ControllingLocation,&station, &offset);
      Float64 supp_dist = span_lldf.ControllingLocation - pBridge->GetSegmentStartEndDistance(segmentKey);
      (*pPara) << _T("Deck Width, Girder Spacing and Deck Overhang are measured along a line that is normal to the alignment and passing through a point ") << location.SetValue(supp_dist) << _T(" from the left support along the centerline of girder. ");
      (*pPara) << _T("The measurement line passes through Station ") << rptRcStation(station, &pDisplayUnits->GetStationFormat() ) << _T(" (") << RPT_OFFSET(offset,offsetFormatter) << _T(")") << rptNewLine;
      (*pPara) << _T("Bridge Width: W = ") << xdim.SetValue(span_lldf.W) << rptNewLine;
      (*pPara) << _T("Roadway Width: w = ") << xdim.SetValue(span_lldf.wCurbToCurb) << rptNewLine;
      (*pPara) << _T("Number of Design Lanes: N") << Sub(_T("L")) << _T(" = ") << span_lldf.Nl << rptNewLine;
      (*pPara) << _T("Lane Width: wLane = ") << xdim.SetValue(span_lldf.wLane) << rptNewLine;
      (*pPara) << _T("Number of Girders: N") << Sub(_T("b")) << _T(" = ") << span_lldf.Nb << rptNewLine;
      (*pPara) << _T("Girder Spacing: ") << Sub2(_T("S"),_T("avg")) << _T(" = ") << xdim.SetValue(span_lldf.Savg) << rptNewLine;
      Float64 de = span_lldf.Side==dfLeft ? span_lldf.leftDe:span_lldf.rightDe;
      (*pPara) << _T("Distance from exterior web of exterior girder to curb line: d") << Sub(_T("e")) << _T(" = ") << xdim.SetValue(de) << rptNewLine;
      (*pPara) << _T("Moment of Inertia: I = ") << inertia.SetValue(span_lldf.I) << rptNewLine;
      (*pPara) << _T("Beam Width: b = ") << xdim2.SetValue(span_lldf.b) << rptNewLine;
      (*pPara) << _T("Beam Depth: d = ") << xdim2.SetValue(span_lldf.d) << rptNewLine;
      (*pPara) << _T("Possion Ratio: ") << symbol(mu) << _T(" = ") << span_lldf.PossionRatio << rptNewLine;
      (*pPara) << _T("Skew Angle at start: ") << symbol(theta) << _T(" = ") << angle.SetValue(fabs(span_lldf.skew1)) << rptNewLine;
      (*pPara) << _T("Skew Angle at end: ") << symbol(theta) << _T(" = ") << angle.SetValue(fabs(span_lldf.skew2)) << rptNewLine;

      GET_IFACE(ISpecification, pSpec);
      GET_IFACE(ILibrary, pLibrary);
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

         (*pPara) << _T("St. Venant torsional inertia constant: J = ") << inertia.SetValue(span_lldf.J) << rptNewLine;

         (*pPara) << rptRcImage(strImagePath + _T("J_closed_thin_wall.png")) << rptNewLine;
         (*pPara) << rptRcImage(strImagePath + _T("BoxBeam_TorsionalConstant.gif")) << rptNewLine;
         (*pPara) << _T("Area enclosed by centerlines of elements: ") << Sub2(_T("A"),_T("o")) << _T(" = ") << area.SetValue(span_lldf.Jvoid.Ao) << rptNewLine;

         rptRcTable* p_table = rptStyleManager::CreateDefaultTable(3,_T(""));
         (*pPara) << p_table;

         (*p_table)(0,0) << _T("Element");
         (*p_table)(0,1) << _T("s");
         (*p_table)(0,2) << _T("t");

         RowIndexType row = p_table->GetNumberOfHeaderRows();
         std::vector<BOXBEAM_J_VOID::Element>::iterator iter;
         for ( iter = span_lldf.Jvoid.Elements.begin(); iter != span_lldf.Jvoid.Elements.end(); iter++ )
         {
            BOXBEAM_J_VOID::Element& element = *iter;
            (*p_table)(row,0) << row;
            (*p_table)(row,1) << xdim2.SetValue(element.first);
            (*p_table)(row,2) << xdim2.SetValue(element.second);

            row++;
         }
         (*pPara) << symbol(SUM) << _T("s/t = ") << span_lldf.Jvoid.S_over_T << rptNewLine;
         (*pPara) << _T("Torsional Constant: J = ") << inertia.SetValue(span_lldf.J) << rptNewLine;
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

      // Distribution factor for exterior girder
      if ( bContinuousAtStart || bIntegralAtStart )
      {
         pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
         (*pChapter) << pPara;
         (*pPara) << _T("Distribution Factor for Negative Moment over ") << LABEL_PIER(pier1) << rptNewLine;
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
         (*pPara) << _T("Distribution Factor for Positive and Negative Moment in Span ") << LABEL_SPAN(spanIdx) << rptNewLine;
      else
         (*pPara) << _T("Distribution Factor for Positive Moment in Span ") << LABEL_SPAN(spanIdx) << rptNewLine;
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
         (*pPara) << _T("Distribution Factor for Negative Moment over ") << LABEL_PIER(pier2) << rptNewLine;
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

   } // next span
}

void CBoxBeamDistFactorEngineer::ReportMoment(rptParagraph* pPara,BOXBEAM_LLDFDETAILS& lldf,WBFL::LRFD::ILiveLoadDistributionFactor::DFResult& gM1,WBFL::LRFD::ILiveLoadDistributionFactor::DFResult& gM2,Float64 gM,bool bSIUnits,IEAFDisplayUnits* pDisplayUnits)
{
   std::_tstring strImagePath(rptStyleManager::GetImagePath());

   INIT_UV_PROTOTYPE( rptLengthUnitValue,    xdim,     pDisplayUnits->GetSpanLengthUnit(),      true );
   INIT_SCALAR_PROTOTYPE(rptRcScalar, scalar, pDisplayUnits->GetScalarFormat());

   GET_IFACE(ILibrary, pLib);
   GET_IFACE(ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

   if ( lldf.bExteriorGirder )
   {
      // Distribution factor for exterior girder
      if( gM1.EqnData.bWasUsed )
      {
         (*pPara) << Bold(_T("1 Loaded Lane - Equation")) << rptNewLine;
         (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("mg_1_ME_Type_G_SI.png") : _T("mg_1_ME_Type_G_US.png"))) << rptNewLine;
 
         if ( lldf.connectedAsUnit || WBFL::LRFD::BDSManager::Edition::SeventhEdition2014 <= WBFL::LRFD::BDSManager::GetEdition() )
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
         (*pPara) << _T("e = ") << scalar.SetValue(gM1.EqnData.e) << rptNewLine;
         (*pPara) << _T("mg") << Super(_T("ME")) << Sub(_T("1")) << _T(" = ") << scalar.SetValue(gM1.EqnData.mg*gM1.EqnData.e) << rptNewLine;

         if (gM1.ControllingMethod & WBFL::LRFD::INTERIOR_OVERRIDE)
         {
            (*pPara) << LLDF_INTOVERRIDE_STR << rptNewLine;
            (*pPara) << _T("mg") << Super(_T("ME")) << Sub(_T("1")) << _T(" = ") << scalar.SetValue(gM1.mg/gM1.SkewCorrectionFactor) << rptNewLine;
         }
      }

      if (gM1.LeverRuleData.bWasUsed)
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


      if ( 2 <= lldf.Nl )
      {
         (*pPara) << rptNewLine;

         if( gM2.EqnData.bWasUsed )
         {
            (*pPara) << Bold(_T("2+ Loaded Lane: Equation")) << rptNewLine;
            (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("mg_2_ME_Type_G_SI.png") : _T("mg_2_ME_Type_G_US.png"))) << rptNewLine;

            if ( lldf.connectedAsUnit || WBFL::LRFD::BDSManager::Edition::SeventhEdition2014 <= WBFL::LRFD::BDSManager::GetEdition() )
            {
               (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("mg_2_MI_Type_F_SI.png") : _T("mg_2_MI_Type_F_US.png"))) << rptNewLine;
            }
            else
            {
               (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("mg_2_MI_Type_G_SI.png") : _T("mg_2_MI_Type_G_US.png"))) << rptNewLine;
               ATLASSERT(gM2.ControllingMethod&WBFL::LRFD::S_OVER_D_METHOD);
               (*pPara)<< _T("K = ")<< gM2.EqnData.K << rptNewLine;
               (*pPara)<< _T("C = ")<< gM2.EqnData.C << rptNewLine;
               (*pPara)<< _T("D = ")<< xdim.SetValue(gM2.EqnData.D) << rptNewLine;
               (*pPara) << rptNewLine;
            }

            (*pPara) << _T("mg") << Super(_T("MI")) << Sub(_T("2+")) << _T(" = ") << scalar.SetValue(gM2.EqnData.mg) << rptNewLine;
            (*pPara) << _T("e = ") << scalar.SetValue(gM2.EqnData.e) << rptNewLine;
            (*pPara) << _T("mg") << Super(_T("ME")) << Sub(_T("2+")) << _T(" = ") << scalar.SetValue(gM2.EqnData.mg*gM2.EqnData.e) << rptNewLine;

            if (gM2.ControllingMethod & WBFL::LRFD::INTERIOR_OVERRIDE)
            {
               (*pPara) << LLDF_INTOVERRIDE_STR << rptNewLine;
               (*pPara) << _T("mg") << Super(_T("ME")) << Sub(_T("2+")) << _T(" = ") << scalar.SetValue(gM2.mg/gM2.SkewCorrectionFactor) << rptNewLine;
            }
         }

         if (gM2.LeverRuleData.bWasUsed)
         {
            (*pPara) << Bold(_T("2+ Loaded Lanes: Lever Rule")) << rptNewLine;
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
         if (lldf.Method== pgsTypes::LiveLoadDistributionFactorMethod::TxDOT)
         {
            (*pPara) << _T("For TxDOT specification, we ignore skew correction, so:") << rptNewLine;
         }
         else
         {
            Float64 skew_delta_max = WBFL::Units::ConvertToSysUnits( 10.0, WBFL::Units::Measure::Degree );
            if ( fabs(lldf.skew1 - lldf.skew2) < skew_delta_max )
               (*pPara) << rptRcImage(strImagePath + _T("SkewCorrection_Moment_TypeC.png")) << rptNewLine;
         }
         (*pPara) << _T("Skew Correction Factor: = ") << scalar.SetValue(gM1.SkewCorrectionFactor) << rptNewLine;
         (*pPara) << rptNewLine;
         (*pPara) << _T("Skew Corrected Factor: mg") << Super(_T("ME")) << Sub(_T("1")) << _T(" = ") << scalar.SetValue(gM1.mg);
         (lldf.Nl == 1 || gM1.mg >= gM2.mg) ? (*pPara) << Bold(_T(" < Controls")) << rptNewLine : (*pPara) << rptNewLine;
         if ( lldf.Nl >= 2 )
         {
            (*pPara) << _T("Skew Corrected Factor: mg") << Super(_T("ME")) << Sub(_T("2+")) << _T(" = ") << scalar.SetValue(gM2.mg);
            (gM2.mg > gM1.mg) ? (*pPara) << Bold(_T(" < Controls")) << rptNewLine : (*pPara) << rptNewLine;
         }
      }
   }
   else
   {
      // Interior Girder
      if ( gM1.EqnData.bWasUsed )
      {
         (*pPara) << Bold(_T("1 Loaded Lane: Equations")) << rptNewLine;
         if ( lldf.connectedAsUnit || WBFL::LRFD::BDSManager::Edition::SeventhEdition2014 <= WBFL::LRFD::BDSManager::GetEdition() )
         {
            (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("mg_1_MI_Type_F_SI.png") : _T("mg_1_MI_Type_F_US.png"))) << rptNewLine;
         }
         else
         {
            (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("mg_1_MI_Type_G_SI.png") : _T("mg_1_MI_Type_G_US.png"))) << rptNewLine;
            ATLASSERT(gM1.ControllingMethod&WBFL::LRFD::S_OVER_D_METHOD);
            (*pPara)<< _T("K = ")<< gM1.EqnData.K << rptNewLine;
            (*pPara)<< _T("C = ")<< gM1.EqnData.C << rptNewLine;
            (*pPara)<< _T("D = ")<< xdim.SetValue(gM1.EqnData.D) << rptNewLine;
            (*pPara) << rptNewLine;
         }

         (*pPara) << _T("mg") << Super(_T("MI")) << Sub(_T("1")) << _T(" = ") << scalar.SetValue(gM1.EqnData.mg) << rptNewLine;

      }

      if (gM1.LeverRuleData.bWasUsed)
      {
         (*pPara) << Bold(_T("1 Loaded Lane: Lever Rule")) << rptNewLine;
         ReportLeverRule(pPara,true,1.0,gM1.LeverRuleData,m_pBroker,pDisplayUnits);
      }

      if ( gM1.LanesBeamsData.bWasUsed )
      {
         (*pPara) << Bold(_T("1 Loaded Lane: Number of Lanes over Number of Beams - Factor cannot be less than this")) << rptNewLine;
         (*pPara) << _T("Skew correction is not applied to Lanes/Beams method")<< rptNewLine;
         ReportLanesBeamsMethod(pPara,gM1.LanesBeamsData,m_pBroker,pDisplayUnits);
      }

      if ( 2 <= lldf.Nl )
      {
         (*pPara) << rptNewLine;

         if ( gM2.EqnData.bWasUsed )
         {
            (*pPara) << Bold(_T("2+ Loaded Lanes: Equation")) << rptNewLine;
            if ( lldf.connectedAsUnit || WBFL::LRFD::BDSManager::Edition::SeventhEdition2014 <= WBFL::LRFD::BDSManager::GetEdition() )
            {
               (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("mg_2_MI_Type_F_SI.png") : _T("mg_2_MI_Type_F_US.png"))) << rptNewLine;
            }
            else
            {
               (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("mg_2_MI_Type_G_SI.png") : _T("mg_2_MI_Type_G_US.png"))) << rptNewLine;
               ATLASSERT(gM2.ControllingMethod&WBFL::LRFD::S_OVER_D_METHOD);
               (*pPara)<< _T("K = ")<< gM2.EqnData.K << rptNewLine;
               (*pPara)<< _T("C = ")<< gM2.EqnData.C << rptNewLine;
               (*pPara)<< _T("D = ")<< xdim.SetValue(gM2.EqnData.D) << rptNewLine;
               (*pPara) << rptNewLine;
            }

            (*pPara) << _T("mg") << Super(_T("MI")) << Sub(_T("2+")) << _T(" = ") << scalar.SetValue(gM2.EqnData.mg) << rptNewLine;
         }

         if (gM2.LeverRuleData.bWasUsed)
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
      }

      (*pPara) << rptNewLine;

      if ( gM1.ControllingMethod & WBFL::LRFD::MOMENT_SKEW_CORRECTION_APPLIED )
      {
         (*pPara) << Bold(_T("Skew Correction")) << rptNewLine;
         if (lldf.Method== pgsTypes::LiveLoadDistributionFactorMethod::TxDOT)
         {
            (*pPara) << _T("For TxDOT specification, we ignore skew correction, so:") << rptNewLine;
         }
         else
         {
            Float64 skew_delta_max = WBFL::Units::ConvertToSysUnits( 10.0, WBFL::Units::Measure::Degree );
            if ( fabs(lldf.skew1 - lldf.skew2) < skew_delta_max )
               (*pPara) << rptRcImage(strImagePath + _T("SkewCorrection_Moment_TypeC.png")) << rptNewLine;
         }
         (*pPara) << _T("Skew Correction Factor: = ") << scalar.SetValue(gM1.SkewCorrectionFactor) << rptNewLine;
         (*pPara) << rptNewLine;
         (*pPara) << _T("Skew Corrected Factor: mg") << Super(_T("MI")) << Sub(_T("1")) << _T(" = ") << scalar.SetValue(gM1.mg);
         (lldf.Nl == 1 || gM1.mg >= gM2.mg) ? (*pPara) << Bold(_T(" < Controls")) << rptNewLine : (*pPara) << rptNewLine;
         if ( lldf.Nl >= 2 )
         {
            (*pPara) << _T("Skew Corrected Factor: mg") << Super(_T("MI")) << Sub(_T("2+")) << _T(" = ") << scalar.SetValue(gM2.mg);
            (gM2.mg > gM1.mg) ? (*pPara) << Bold(_T(" < Controls")) << rptNewLine : (*pPara) << rptNewLine;
         }
      }
   }
}

void CBoxBeamDistFactorEngineer::ReportShear(rptParagraph* pPara,BOXBEAM_LLDFDETAILS& lldf,WBFL::LRFD::ILiveLoadDistributionFactor::DFResult& gV1,WBFL::LRFD::ILiveLoadDistributionFactor::DFResult& gV2,Float64 gV,bool bSIUnits,IEAFDisplayUnits* pDisplayUnits)
{
   std::_tstring strImagePath(rptStyleManager::GetImagePath());

   INIT_SCALAR_PROTOTYPE(rptRcScalar, scalar, pDisplayUnits->GetScalarFormat());

   GET_IFACE(ILibrary, pLib);
   GET_IFACE(ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   
   if ( lldf.bExteriorGirder ) 
   {
      //
      // Shear
      //
      if ( gV1.LeverRuleData.bWasUsed )
      {
         (*pPara) << Bold(_T("1 Loaded Lane: Lever Rule")) << rptNewLine;
         ReportLeverRule(pPara,false,1.0,gV1.LeverRuleData,m_pBroker,pDisplayUnits);
      }

      if ( gV1.EqnData.bWasUsed  )
      {
         (*pPara) << Bold(_T("1 Loaded Lane: Equation")) << rptNewLine;
         if (!(gV1.ControllingMethod & WBFL::LRFD::MOMENT_OVERRIDE))
         {
            (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("mg_1_VE_Type_G_SI.png") : _T("mg_1_VE_Type_G_US.png"))) << rptNewLine;
            (*pPara) << _T("mg") << Super(_T("VI")) << Sub(_T("1")) << _T(" = ") << scalar.SetValue(gV1.EqnData.mg) << rptNewLine;
            (*pPara) << _T("e = ") << scalar.SetValue(gV1.EqnData.e) << rptNewLine;
            (*pPara) << _T("mg") << Super(_T("VE")) << Sub(_T("1")) << _T(" = ") << scalar.SetValue(gV1.EqnData.mg*gV1.EqnData.e) << rptNewLine;
         }
         else
         {
            (*pPara) << _T(" I or J was outside of range of applicability - Use moment equation results for shear")<<rptNewLine;
            (*pPara) << _T("mg") << Super(_T("VE")) << Sub(_T("1")) << _T(" = mg") << Super(_T("ME")) << Sub(_T("1")) <<_T(" = ") << scalar.SetValue(gV1.EqnData.mg*gV1.EqnData.e) << rptNewLine;
         }
      }

      if (gV1.ControllingMethod & WBFL::LRFD::INTERIOR_OVERRIDE)
      {
         (*pPara)<< rptNewLine << LLDF_INTOVERRIDE_STR << rptNewLine;
         (*pPara) << _T("mg") << Super(_T("VE")) << Sub(_T("1")) << _T(" = ") << scalar.SetValue(gV1.mg/gV1.SkewCorrectionFactor) << rptNewLine;
      }

      if ( gV1.LanesBeamsData.bWasUsed )
      {
         (*pPara) << Bold(_T("1 Loaded Lane: Number of Lanes over Number of Beams - Factor cannot be less than this")) << rptNewLine;
         (*pPara) << _T("Skew correction is not applied to Lanes/Beams method")<< rptNewLine;
         ReportLanesBeamsMethod(pPara,gV1.LanesBeamsData,m_pBroker,pDisplayUnits);
      }

      if ( 2 <= lldf.Nl )
      {
         (*pPara) << rptNewLine;

         if ( gV1.LeverRuleData.bWasUsed )
         {
            (*pPara) << Bold(_T("2+ Loaded Lanes: Lever Rule")) << rptNewLine;
            ReportLeverRule(pPara,false,1.0,gV2.LeverRuleData,m_pBroker,pDisplayUnits);
         }

         if ( gV2.EqnData.bWasUsed )
         {
            (*pPara) << Bold(_T("2+ Loaded Lanes: Equation")) << rptNewLine;

            if (!(gV2.ControllingMethod & WBFL::LRFD::MOMENT_OVERRIDE))
            {
               (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("mg_2_VE_Type_G_SI.png") : _T("mg_2_VE_Type_G_US.png"))) << rptNewLine;
               (*pPara) << _T("mg") << Super(_T("VI")) << Sub(_T("2+")) << _T(" = ") << scalar.SetValue(gV2.EqnData.mg) << rptNewLine;

               // have to play games here with oddball 48/b value
               ATLASSERT(gV2.EqnData.K>0.0 && gV2.EqnData.K<=1.0);
               if (bSIUnits)
                  (*pPara) << _T("1200/b = ") << scalar.SetValue(gV2.EqnData.K) << rptNewLine;
               else
                  (*pPara) << _T("48/b = ") << scalar.SetValue(gV2.EqnData.K) << rptNewLine;

               (*pPara) << _T("e = ") << scalar.SetValue(gV2.EqnData.e/gV2.EqnData.K) << rptNewLine;

               (*pPara) << _T("mg") << Super(_T("VE")) << Sub(_T("2+")) << _T(" = ") << scalar.SetValue(gV2.EqnData.mg*gV2.EqnData.e) << rptNewLine;
            }
            else
            {
               (*pPara) << _T(" I or J was outside of range of applicability - Use moment equation results for shear")<<rptNewLine;
               (*pPara) << _T("mg") << Super(_T("VE")) << Sub(_T("2+")) << _T(" = mg") << Super(_T("ME")) << Sub(_T("2+")) <<_T(" = ") << scalar.SetValue(gV2.EqnData.mg*gV2.EqnData.e) << rptNewLine;
            }
         }

         if (gV2.ControllingMethod & WBFL::LRFD::INTERIOR_OVERRIDE)
         {
            (*pPara) << rptNewLine << LLDF_INTOVERRIDE_STR << rptNewLine;
            (*pPara) << _T("mg") << Super(_T("VE")) << Sub(_T("2")) << _T(" = ") << scalar.SetValue(gV2.mg/gV2.SkewCorrectionFactor) << rptNewLine;
         }

         if ( gV2.LanesBeamsData.bWasUsed )
         {
            (*pPara) << Bold(_T("2+ Loaded Lane: Number of Lanes over Number of Beams - Factor cannot be less than this")) << rptNewLine;
            (*pPara) << _T("Skew correction is not applied to Lanes/Beams method")<< rptNewLine;
            ReportLanesBeamsMethod(pPara,gV2.LanesBeamsData,m_pBroker,pDisplayUnits);
         }
      }

      (*pPara) << rptNewLine;

      if ( gV1.ControllingMethod & WBFL::LRFD::SHEAR_SKEW_CORRECTION_APPLIED )
      {
         (*pPara) << Bold(_T("Skew Correction")) << rptNewLine;
         (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("SkewCorrection_Shear_TypeF_SI.png") : _T("SkewCorrection_Shear_TypeF_US.png"))) << rptNewLine;

         (*pPara) << _T("Skew Correction Factor: = ") << scalar.SetValue(gV1.SkewCorrectionFactor) << rptNewLine;
         (*pPara) << rptNewLine;
         (*pPara) << _T("Skew Corrected Factor: mg") << Super(_T("VE")) << Sub(_T("1")) << _T(" = ") << scalar.SetValue(gV1.mg);
         (lldf.Nl == 1 || gV1.mg >= gV2.mg) ? (*pPara) << Bold(_T(" < Controls")) << rptNewLine : (*pPara) << rptNewLine;
         if ( lldf.Nl >= 2 )
         {
            (*pPara) << _T("Skew Corrected Factor: mg") << Super(_T("VE")) << Sub(_T("2+")) << _T(" = ") << scalar.SetValue(gV2.mg);
            (gV2.mg > gV1.mg) ? (*pPara) << Bold(_T(" < Controls")) << rptNewLine : (*pPara) << rptNewLine;
         }
      }
   }
   else
   {
      // Interior Girder
      //
      // Shear
      //
      if ( gV1.ControllingMethod &  WBFL::LRFD::MOMENT_OVERRIDE  )
      {
         (*pPara) << _T("I or J do not comply with the limitations in LRFD Table 4.6.2.2.3a-1. The shear distribution factor is taken as that for moment. [LRFD 4.6.2.2.3a]") << rptNewLine;
         (*pPara) << Bold(_T("1 Loaded Lane:")) << rptNewLine;
         (*pPara) << _T("mg") << Super(_T("VI")) << Sub(_T("1")) << _T(" = ") << scalar.SetValue(gV1.mg/gV1.SkewCorrectionFactor) << rptNewLine;

         if ( 2 <= lldf.Nl )
         {
            (*pPara) << Bold(_T("2+ Loaded Lanes:")) << rptNewLine;
            (*pPara) << _T("mg") << Super(_T("VI")) << Sub(_T("2+")) << _T(" = ") << scalar.SetValue(gV2.mg/gV2.SkewCorrectionFactor) << rptNewLine;
         }
      }
      else
      {

         if ( gV1.LeverRuleData.bWasUsed )
         {
            (*pPara) << Bold(_T("1 Loaded Lane: Lever Rule")) << rptNewLine;
            ReportLeverRule(pPara,false,1.0,gV1.LeverRuleData,m_pBroker,pDisplayUnits);
         }

         if ( gV1.EqnData.bWasUsed )
         {
            (*pPara) << Bold(_T("1 Loaded Lane: Equation")) << rptNewLine;
            (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("mg_1_VI_Type_G_SI.png") : _T("mg_1_VI_Type_G_US.png"))) << rptNewLine;
            (*pPara) << _T("mg") << Super(_T("VI")) << Sub(_T("1")) << _T(" = ") << scalar.SetValue(gV1.EqnData.mg) << rptNewLine;
         }

         if ( 2 <= lldf.Nl )
         {
            (*pPara) << rptNewLine;

            if ( gV2.LeverRuleData.bWasUsed )
            {
               (*pPara) << Bold(_T("2+ Loaded Lanes: Lever Rule")) << rptNewLine;
               (*pPara) << Bold(_T("Lever Rule")) << rptNewLine;
               ReportLeverRule(pPara,false,1.0,gV2.LeverRuleData,m_pBroker,pDisplayUnits);
            }

            if ( gV2.EqnData.bWasUsed )
            {
               (*pPara) << Bold(_T("2+ Loaded Lanes: Equation")) << rptNewLine;
               (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("mg_2_VI_Type_G_SI.png") : _T("mg_2_VI_Type_G_US.png"))) << rptNewLine;
               (*pPara) << _T("mg") << Super(_T("VI")) << Sub(_T("2+")) << _T(" = ") << scalar.SetValue(gV2.EqnData.mg) << rptNewLine;
            }

            if ( gV2.LanesBeamsData.bWasUsed )
            {
               (*pPara) << Bold(_T("2+ Loaded Lanes: Number of Lanes over Number of Beams - Factor cannot be less than this")) << rptNewLine;
               (*pPara) << _T("Skew correction is not applied to Lanes/Beams method")<< rptNewLine;
               ReportLanesBeamsMethod(pPara,gV1.LanesBeamsData,m_pBroker,pDisplayUnits);
            }
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
         if ( lldf.Nl >= 2 )
         {
            (*pPara) << _T("Skew Corrected Factor: mg") << Super(_T("VI")) << Sub(_T("2+")) << _T(" = ") << scalar.SetValue(gV2.mg);
            (gV2.mg > gV1.mg) ? (*pPara) << Bold(_T(" < Controls")) << rptNewLine : (*pPara) << rptNewLine;
         }
      }
   }
}

WBFL::LRFD::LiveLoadDistributionFactorBase* CBoxBeamDistFactorEngineer::GetLLDFParameters(IndexType spanOrPierIdx,GirderIndexType gdrIdx,DFParam dfType,BOXBEAM_LLDFDETAILS* plldf,const GDRCONFIG* pConfig)
{
   GET_IFACE(ISectionProperties, pSectProp);
   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IGirder,pGirder);
   GET_IFACE(ILiveLoads,pLiveLoads);

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CDeckDescription2* pDeck = pBridgeDesc->GetDeckDescription();

   ATLASSERT(pDeck->GetDeckType()==pgsTypes::sdtCompositeOverlay || pDeck->GetDeckType()==pgsTypes::sdtNone);
   ATLASSERT(pBridgeDesc->GetDistributionFactorMethod() != pgsTypes::DirectlyInput);

   // Determine span/pier index... This is the index of a pier and the next span.
   // If this is the last pier, span index is for the last span
   SpanIndexType span = INVALID_INDEX;
   PierIndexType pier = INVALID_INDEX;
   SpanIndexType prev_span = INVALID_INDEX;
   SpanIndexType next_span = INVALID_INDEX;
   PierIndexType prev_pier = INVALID_INDEX;
   PierIndexType next_pier = INVALID_INDEX;
   GetIndicies(spanOrPierIdx,dfType,span,pier,prev_span,next_span,prev_pier,next_pier);

   CSpanKey spanKey(span,gdrIdx);
   
   const CSpanData2* pSpan = pBridgeDesc->GetSpan(span);
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(pSpan);
   GirderIndexType nGirders = pGroup->GetGirderCount();

   if ( nGirders <= gdrIdx )
   {
      ATLASSERT(false);
      gdrIdx = nGirders-1;
   }

   ///////////////////////////////////////////////////////////////////////////
   // determine overhang, spacing base data, and controlling poi
   pgsPointOfInterest poi;
   GetGirderSpacingAndOverhang(spanKey, dfType, plldf, &poi);

   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   // Throws exception if fails requirement (no need to catch it)
   GET_IFACE(ILiveLoadDistributionFactors, pDistFactors);
   Int32 roaVal = pDistFactors->VerifyDistributionFactorRequirements(poi);

   // Get some controlling dimensions
   // get the maximum width of this girder
   Float64 top_width = pGirder->GetTopWidth(poi);
   Float64 bot_width = pGirder->GetBottomWidth(poi);

   Float64 width = Max(top_width,bot_width);

   Float64 height = pGirder->GetHeight(poi);

   Float64 top_flg_thk = pGirder->GetTopFlangeThickness(poi,0);
   Float64 bot_flg_thk = pGirder->GetBottomFlangeThickness(poi,0);

   Float64 web_thk = pGirder->GetWebThickness(poi,0);
   Float64 web_spc = pGirder->GetWebSpacing(poi,0);

   Float64 inner_height = height - top_flg_thk - bot_flg_thk;
   Float64 inner_width  = web_spc - web_thk;

   Float64 sspc=0;
   for (std::vector<Float64>::const_iterator its=plldf->gdrSpacings.begin(); its!=plldf->gdrSpacings.end(); its++)
   {
      sspc += *its;
   }

   plldf->d            = height;
   plldf->b            = width;
   plldf->PossionRatio = 0.2;

   plldf->L = GetEffectiveSpanLength(spanOrPierIdx,gdrIdx,dfType);

   // compute de (inside edge of barrier to CL of exterior web)
   Float64 wd = pGirder->GetCL2ExteriorWebDistance(poi); // cl beam to cl web
   ATLASSERT(0.0 < wd);

   // Note this is not exactly correct because opposite exterior beam might be different, but we won't be using this data set for that beam
   plldf->leftDe  = plldf->leftCurbOverhang  - wd;  
   plldf->rightDe = plldf->rightCurbOverhang - wd; 

   // Determine composite moment of inertia and J
   BOXBEAM_J_VOID Jvoid;

   bool is_composite = pBridge->IsCompositeDeck();
   plldf->connectedAsUnit = (pDeck->TransverseConnectivity == pgsTypes::atcConnectedAsUnit ? true : false);

   // thickness of exterior vertical elements
   Float64 t_ext = web_thk;

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType lastIntervalIdx = pIntervals->GetIntervalCount()-1;
   if (is_composite)
   {
      // We have a composite section. 
      Float64 eff_wid = pSectProp->GetEffectiveFlangeWidth(poi);
      plldf->I  = pSectProp->GetIxx(pgsTypes::sptGross,lastIntervalIdx,poi,pConfig);

      Float64 t_top = top_flg_thk;
      Float64 slab_depth = pBridge->GetStructuralSlabDepth(poi);

      t_top += slab_depth;

      Float64 t_bot = bot_flg_thk;

      // s 
      Float64 s_top  = inner_width + t_ext;
      Float64 s_side = inner_height + (t_top + t_bot)/2;

      Jvoid.Elements.emplace_back(s_top,t_top); // top
      Jvoid.Elements.emplace_back(s_top,t_bot); // bottom
      Jvoid.Elements.emplace_back(s_side,t_ext); // left
      Jvoid.Elements.emplace_back(s_side,t_ext); // right

      Float64 Sum_s_over_t = 0;
      std::vector<BOXBEAM_J_VOID::Element>::iterator iter;
      for ( iter = Jvoid.Elements.begin(); iter != Jvoid.Elements.end(); iter++ )
      {
         BOXBEAM_J_VOID::Element& e = *iter;
         Sum_s_over_t += e.first/e.second;
      }

      Jvoid.S_over_T = Sum_s_over_t;

      Float64 Ao = s_top * s_side;

      Jvoid.Ao = Ao;

      Float64 J = 4.0*Ao*Ao/Sum_s_over_t;

      plldf->Jvoid = Jvoid;
      plldf->J = J;
   }
   else
   {
      // No deck: base I and J on bare beam properties
      plldf->I  = pSectProp->GetIxx(pgsTypes::sptGross,releaseIntervalIdx,poi);

      Float64 t_top = top_flg_thk;
      Float64 t_bot = bot_flg_thk;

      // s 
      Float64 s_top  = inner_width + t_ext;
      Float64 s_side = inner_height + (t_top + t_bot)/2;

      Jvoid.Elements.emplace_back(s_top,t_top); // top
      Jvoid.Elements.emplace_back(s_top,t_bot); // bottom
      Jvoid.Elements.emplace_back(s_side,t_ext); // left
      Jvoid.Elements.emplace_back(s_side,t_ext); // right

      Float64 Sum_s_over_t = 0;
      std::vector<BOXBEAM_J_VOID::Element>::iterator iter;
      for ( iter = Jvoid.Elements.begin(); iter != Jvoid.Elements.end(); iter++ )
      {
         BOXBEAM_J_VOID::Element& e = *iter;
         Sum_s_over_t += e.first/e.second;
      }

      Jvoid.S_over_T = Sum_s_over_t;

      Float64 Ao = s_top * s_side;

      Jvoid.Ao = Ao;

      Float64 J = 4.0*Ao*Ao/Sum_s_over_t;

      plldf->Jvoid = Jvoid;
      plldf->J = J;
   }
   
   // WSDOT deviation doesn't apply to this type of cross section because it isn't slab on girder construction
   WBFL::LRFD::LiveLoadDistributionFactorBase* pLLDF;

   if (plldf->Method== pgsTypes::LiveLoadDistributionFactorMethod::TxDOT)
   {
      plldf->connectedAsUnit = true;

      WBFL::LRFD::TxdotLldfAdjacentBox* pTypeF = new WBFL::LRFD::TxdotLldfAdjacentBox(
                            plldf->gdrNum, // to fix this warning, clean up the LLDF data types
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
                            plldf->PossionRatio,
                            plldf->skew1, 
                            plldf->skew2);

      pLLDF = pTypeF;
   }
   else
   {
      GET_IFACE(ISpecification, pSpec);
      GET_IFACE(ILibrary, pLibrary);
      const auto* pSpecEntry = pLibrary->GetSpecEntry(pSpec->GetSpecification().c_str());
      const auto& live_load_distribution_criteria = pSpecEntry->GetLiveLoadDistributionCriteria();
      bool bSkew = !( IsZero(plldf->skew1) && IsZero(plldf->skew2) );
      bool bSkewMoment = live_load_distribution_criteria.bIgnoreSkewReductionForMoment ? false : bSkew;
      bool bSkewShear = bSkew;

      if ( WBFL::LRFD::BDSManager::Edition::SeventhEdition2014 <= WBFL::LRFD::BDSManager::GetEdition() )
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

      if ( plldf->connectedAsUnit || 
           WBFL::LRFD::BDSManager::Edition::SeventhEdition2014 <= WBFL::LRFD::BDSManager::GetEdition()  // sufficiently connected as unit was removed in LRFD 7th Edition 2014
         )
      {
         WBFL::LRFD::LldfTypeF* pTypeF = new WBFL::LRFD::LldfTypeF(
                               plldf->gdrNum, // to fix this warning, clean up the LLDF data types
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
                               plldf->PossionRatio,
                               plldf->skew1, 
                               plldf->skew2,
                               bSkewMoment, bSkewShear);

         pLLDF = pTypeF;
      }
      else
      {
         WBFL::LRFD::LldfTypeG* pTypeG = new WBFL::LRFD::LldfTypeG(
                               plldf->gdrNum,  // to fix this warning, clean up the LLDF data types
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
                               plldf->PossionRatio,
                               plldf->skew1, 
                               plldf->skew2,
                               bSkewMoment, bSkewShear);

         pLLDF = pTypeG;
      }
   }

   pLLDF->SetRangeOfApplicability( pLiveLoads->GetRangeOfApplicabilityAction(), roaVal);

   return pLLDF;
}

std::_tstring CBoxBeamDistFactorEngineer::GetComputationDescription(const CGirderKey& girderKey,const std::_tstring& libraryEntryName,pgsTypes::SupportedDeckType decktype, pgsTypes::AdjacentTransverseConnectivity connect)
{
   GET_IFACE(ILibrary, pLib);
   GET_IFACE(ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   const auto& live_load_distribution_criteria = pSpecEntry->GetLiveLoadDistributionCriteria();

   auto lldfMethod = live_load_distribution_criteria.LldfMethod;

   std::_tstring descr;

   if ( lldfMethod == pgsTypes::LiveLoadDistributionFactorMethod::TxDOT)
   {
      descr = _T("TxDOT modifications. Treat as AASHTO Type (f,g) connected transversely sufficiently to act as a unit, regardless of deck or connectivity input. Also, do not apply skew correction factor for moment.");
   }
   else
   {
      descr = _T("AASHTO LRFD Method per Article 4.6.2.2.");

      if (decktype == pgsTypes::sdtCompositeOverlay)
      {
         descr += std::_tstring(_T(" type (f) cross section"));
      }
      else if (decktype == pgsTypes::sdtNone)
      {
         descr += std::_tstring(_T(" type (g) cross section"));
      }
      else
      {
         ATLASSERT(false);
      }

      if (connect == pgsTypes::atcConnectedAsUnit)
      {
         descr += std::_tstring(_T(" connected transversely sufficiently to act as a unit."));
      }
      else
      {
         descr += std::_tstring(_T(" connected transversely only enough to prevent relative vertical displacement along interface."));
      }
   }

   return descr;
}

