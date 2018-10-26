///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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
#include <PgsExt\BridgeDescription.h>
#include <PgsExt\StatusItem.h>
#include <PgsExt\GirderLabel.h>
#include <Reporting\ReportStyleHolder.h>
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\DisplayUnits.h>
#include <IFace\DistributionFactors.h>
#include <IFace\StatusCenter.h>
#include "helper.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CIBeamFactory
void CUBeamDistFactorEngineer::Init(bool bTypeB)
{
   m_bTypeB = bTypeB;
}

HRESULT CUBeamDistFactorEngineer::FinalConstruct()
{
   return S_OK;
}

void CUBeamDistFactorEngineer::BuildReport(SpanIndexType span,GirderIndexType gdr,rptChapter* pChapter,IDisplayUnits* pDisplayUnits)
{
   SPANDETAILS span_lldf;
   GetSpanDF(span,gdr,pgsTypes::StrengthI,-1,&span_lldf);

   PierIndexType pier1 = span;
   PierIndexType pier2 = span+1;
   PIERDETAILS pier1_lldf, pier2_lldf;
   GetPierDF(pier1, gdr, pgsTypes::StrengthI, pgsTypes::Ahead, -1, &pier1_lldf);
   GetPierDF(pier2, gdr, pgsTypes::StrengthI, pgsTypes::Back,  -1, &pier2_lldf);

   REACTIONDETAILS reaction1_lldf, reaction2_lldf;
   GetPierReactionDF(pier1, gdr, pgsTypes::StrengthI, -1, &reaction1_lldf);
   GetPierReactionDF(pier2, gdr, pgsTypes::StrengthI, -1, &reaction2_lldf);

   // do a sanity check to make sure the fundimental values are correct
   ATLASSERT(span_lldf.Method  == pier1_lldf.Method);
   ATLASSERT(span_lldf.Method  == pier2_lldf.Method);
   ATLASSERT(pier1_lldf.Method == pier2_lldf.Method);

   ATLASSERT(span_lldf.bExteriorGirder  == pier1_lldf.bExteriorGirder);
   ATLASSERT(span_lldf.bExteriorGirder  == pier2_lldf.bExteriorGirder);
   ATLASSERT(pier1_lldf.bExteriorGirder == pier2_lldf.bExteriorGirder);

   // Grab the interfaces that are needed
   GET_IFACE(IBridge,pBridge);
   GET_IFACE(ILiveLoads,pLiveLoads);

   // determine continuity
   bool bContinuous, bContinuousAtStart, bContinuousAtEnd;
   pBridge->IsContinuousAtPier(pier1,&bContinuous,&bContinuousAtStart);
   pBridge->IsContinuousAtPier(pier2,&bContinuousAtEnd,&bContinuous);

   bool bIntegral, bIntegralAtStart, bIntegralAtEnd;
   pBridge->IsIntegralAtPier(pier1,&bIntegral,&bIntegralAtStart);
   pBridge->IsIntegralAtPier(pier2,&bIntegralAtEnd,&bIntegral);

   // get to work building the report
   rptParagraph* pPara;

   bool bSIUnits = (pDisplayUnits->GetUnitDisplayMode() == pgsTypes::umSI);
   std::string strImagePath(pgsReportStyleHolder::GetImagePath());

   INIT_UV_PROTOTYPE( rptLengthUnitValue,    location, pDisplayUnits->GetSpanLengthUnit(),      true );
   INIT_UV_PROTOTYPE( rptAreaUnitValue,      area,     pDisplayUnits->GetAreaUnit(),            true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,    xdim,     pDisplayUnits->GetSpanLengthUnit(),      true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,    xdim2,    pDisplayUnits->GetComponentDimUnit(),    true );
   INIT_UV_PROTOTYPE( rptLength4UnitValue,   inertia,  pDisplayUnits->GetMomentOfInertiaUnit(), true );
   INIT_UV_PROTOTYPE( rptAngleUnitValue,     angle,    pDisplayUnits->GetAngleUnit(),           true );

   rptRcScalar scalar;
   scalar.SetFormat( sysNumericFormatTool::Fixed );
   scalar.SetWidth(6);
   scalar.SetPrecision(3);
   scalar.SetTolerance(1.0e-6);

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CDeckDescription* pDeck = pBridgeDesc->GetDeckDescription();
   const CSpanData* pSpan = pBridgeDesc->GetSpan(span);

   std::string strGirderName = pSpan->GetGirderTypes()->GetGirderName(gdr);

   pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
   (*pPara) << "Method of Computation:"<<rptNewLine;
   (*pChapter) << pPara;
   pPara = new rptParagraph;
   (*pChapter) << pPara;
   (*pPara) << GetComputationDescription(span,gdr,
                                         strGirderName,
                                         pDeck->DeckType,
                                         pDeck->TransverseConnectivity);
   
   pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
   (*pChapter) << pPara;
   (*pPara) << "Distribution Factor Parameters" << rptNewLine;
   pPara = new rptParagraph;
   (*pChapter) << pPara;

   (*pPara) << "Girder Spacing: " << Sub2("S","avg") << " = " << xdim.SetValue(span_lldf.Savg) << rptNewLine;
   Float64 station,offset;
   pBridge->GetStationAndOffset(pgsPointOfInterest(span,gdr,span_lldf.ControllingLocation),&station, &offset);
   Float64 supp_dist = span_lldf.ControllingLocation - pBridge->GetGirderStartConnectionLength(span,gdr);
   (*pPara) << "Measurement of Girder Spacing taken at " << location.SetValue(supp_dist)<< " from left support, measured along girder, or station = "<< rptRcStation(station, &pDisplayUnits->GetStationFormat() ) << rptNewLine;
//   (*pPara) << "Span Length: L = " << xdim.SetValue(span_lldf.L) << rptNewLine;
   (*pPara) << "Girder Depth: d = " << xdim2.SetValue(span_lldf.d) << rptNewLine;
   Float64 de = span_lldf.Side==dfLeft ? span_lldf.leftDe:span_lldf.rightDe;
   (*pPara) << "Distance from exterior web of exterior beam to curb line: d" << Sub("e") << " = " << xdim.SetValue(de) << rptNewLine;
//   (*pPara) << "Average Skew Angle: " << symbol(theta) << " = " << angle.SetValue(fabs((span_lldf.skew1 + span_lldf.skew2)/2)) << rptNewLine;
   (*pPara) << "Number of Design Lanes: N" << Sub("L") << " = " << span_lldf.Nl << rptNewLine;
   (*pPara) << "Lane Width: wLane = " << xdim.SetValue(span_lldf.wLane) << rptNewLine;
   (*pPara) << "Number of Beams: N" << Sub("b") << " = " << span_lldf.Nb << rptNewLine;

   if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
   {
      pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
      (*pPara) << "Strength and Service Limit States";
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
      pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
      (*pChapter) << pPara;
      (*pPara) << "Distribution Factor for Negative Moment over Pier " << long(pier1+1) << rptNewLine;
      pPara = new rptParagraph;
      (*pChapter) << pPara;

      (*pPara) << "Average Skew Angle: " << symbol(theta) << " = " << angle.SetValue(fabs((pier1_lldf.skew1 + pier1_lldf.skew2)/2)) << rptNewLine;
      (*pPara) << "Span Length: L = " << xdim.SetValue(pier1_lldf.L) << rptNewLine << rptNewLine;

      // Negative moment DF from pier1_lldf
      ReportMoment(span, pPara,
                   pier1_lldf,
                   pier1_lldf.gM1,
                   pier1_lldf.gM2,
                   pier1_lldf.gM,
                   bSIUnits,pDisplayUnits);
   }

   // Positive moment DF
   pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
   (*pChapter) << pPara;
   if ( bContinuousAtStart || bContinuousAtEnd || bIntegralAtStart || bIntegralAtEnd )
      (*pPara) << "Distribution Factor for Positive and Negative Moment in Span " << LABEL_SPAN(span) << rptNewLine;
   else
      (*pPara) << "Distribution Factor for Positive Moment in Span " << LABEL_SPAN(span) << rptNewLine;
   pPara = new rptParagraph;
   (*pChapter) << pPara;

   (*pPara) << "Average Skew Angle: " << symbol(theta) << " = " << angle.SetValue(fabs((span_lldf.skew1 + span_lldf.skew2)/2)) << rptNewLine;
   (*pPara) << "Span Length: L = " << xdim.SetValue(span_lldf.L) << rptNewLine << rptNewLine;

   ReportMoment(span, pPara,
                span_lldf,
                span_lldf.gM1,
                span_lldf.gM2,
                span_lldf.gM,
                bSIUnits,pDisplayUnits);

   if ( bContinuousAtEnd || bIntegralAtEnd )
   {
      pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
      (*pChapter) << pPara;
      (*pPara) << "Distribution Factor for Negative Moment over Pier " << long(pier2+1) << rptNewLine;
      pPara = new rptParagraph;
      (*pChapter) << pPara;

      (*pPara) << "Average Skew Angle: " << symbol(theta) << " = " << angle.SetValue(fabs((pier2_lldf.skew1 + pier2_lldf.skew2)/2)) << rptNewLine;
      (*pPara) << "Span Length: L = " << xdim.SetValue(pier2_lldf.L) << rptNewLine << rptNewLine;

      // Negative moment DF from pier2_lldf
      ReportMoment(span, pPara,
                   pier2_lldf,
                   pier2_lldf.gM1,
                   pier2_lldf.gM2,
                   pier2_lldf.gM,
                   bSIUnits,pDisplayUnits);
   }
   

   //////////////////////////////////////////////////////////////
   // Shears
   //////////////////////////////////////////////////////////////
   pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
   (*pChapter) << pPara;
   (*pPara) << "Distribution Factor for Shear in Span " << LABEL_SPAN(span) << rptNewLine;
   pPara = new rptParagraph;
   (*pChapter) << pPara;

   (*pPara) << "Average Skew Angle: " << symbol(theta) << " = " << angle.SetValue(fabs((span_lldf.skew1 + span_lldf.skew2)/2)) << rptNewLine;
   (*pPara) << "Span Length: L = " << xdim.SetValue(span_lldf.L) << rptNewLine << rptNewLine;

   ReportShear(span, pPara,
               span_lldf,
               span_lldf.gV1,
               span_lldf.gV2,
               span_lldf.gV,
               bSIUnits,pDisplayUnits);

   //////////////////////////////////////////////////////////////
   // Reactions
   //////////////////////////////////////////////////////////////
   pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
   (*pChapter) << pPara;
   (*pPara) << "Distribution Factor for Reaction at Pier " << long(pier1+1) << rptNewLine;
   pPara = new rptParagraph;
   (*pChapter) << pPara;

   (*pPara) << "Average Skew Angle: " << symbol(theta) << " = " << angle.SetValue(fabs((reaction1_lldf.skew1 + reaction1_lldf.skew2)/2)) << rptNewLine;
   (*pPara) << "Span Length: L = " << xdim.SetValue(reaction1_lldf.L) << rptNewLine << rptNewLine;

   ReportShear(span, pPara,
               reaction1_lldf,
               reaction1_lldf.gR1,
               reaction1_lldf.gR2,
               reaction1_lldf.gR,
               bSIUnits,pDisplayUnits);

     ///////

   pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
   (*pChapter) << pPara;
   (*pPara) << "Distribution Factor for Reaction at Pier " << long(pier2+1) << rptNewLine;
   pPara = new rptParagraph;
   (*pChapter) << pPara;

   (*pPara) << "Average Skew Angle: " << symbol(theta) << " = " << angle.SetValue(fabs((reaction2_lldf.skew1 + reaction2_lldf.skew2)/2)) << rptNewLine;
   (*pPara) << "Span Length: L = " << xdim.SetValue(reaction2_lldf.L) << rptNewLine << rptNewLine;

   ReportShear(span, pPara,
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
      pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
      (*pPara) << "Fatigue Limit States";
      (*pChapter) << pPara;
      pPara = new rptParagraph;
      (*pChapter) << pPara;

      std::string superscript;

      rptRcScalar scalar2 = scalar;

      //////////////////////////////////////////////////////////////
      // Moments
      //////////////////////////////////////////////////////////////
      if ( bContinuousAtEnd || bIntegralAtEnd )
      {
         pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
         (*pChapter) << pPara;
         (*pPara) << "Distribution Factor for Negative Moment over Pier " << LABEL_PIER(pier1) << rptNewLine;
         pPara = new rptParagraph;
         (*pChapter) << pPara;

         superscript = (pier1_lldf.bExteriorGirder ? "ME" : "MI");
         (*pPara) << "g" << superscript << Sub("Fatigue") << " = " << "mg" << superscript << Sub("1") << "/m =" << scalar.SetValue(pier1_lldf.gM1.mg) << "/1.2 = " << scalar2.SetValue(pier1_lldf.gM1.mg/1.2);
      }

      // Positive moment DF
      pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
      (*pChapter) << pPara;
      if ( bContinuousAtStart || bContinuousAtEnd || bIntegralAtStart || bIntegralAtEnd )
         (*pPara) << "Distribution Factor for Positive and Negative Moment in Span " << LABEL_SPAN(span) << rptNewLine;
      else
         (*pPara) << "Distribution Factor for Positive Moment in Span " << LABEL_SPAN(span) << rptNewLine;
      pPara = new rptParagraph;
      (*pChapter) << pPara;

      superscript = (span_lldf.bExteriorGirder ? "ME" : "MI");
      (*pPara) << "g" << superscript << Sub("Fatigue") << " = " << "mg" << superscript << Sub("1") << "/m =" << scalar.SetValue(span_lldf.gM1.mg) << "/1.2 = " << scalar2.SetValue(span_lldf.gM1.mg/1.2);

      if ( bContinuousAtEnd || bIntegralAtEnd )
      {
         pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
         (*pChapter) << pPara;
         (*pPara) << "Distribution Factor for Negative Moment over Pier " << LABEL_PIER(pier2) << rptNewLine;
         pPara = new rptParagraph;
         (*pChapter) << pPara;

         superscript = (pier2_lldf.bExteriorGirder ? "ME" : "MI");
         (*pPara) << "g" << superscript << Sub("Fatigue") << " = " << "mg" << superscript << Sub("1") << "/m =" << scalar.SetValue(pier2_lldf.gM1.mg) << "/1.2 = " << scalar2.SetValue(pier2_lldf.gM1.mg/1.2);
      }

      //////////////////////////////////////////////////////////////
      // Shears
      //////////////////////////////////////////////////////////////
      pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
      (*pChapter) << pPara;
      (*pPara) << "Distribution Factor for Shear in Span " << LABEL_SPAN(span) << rptNewLine;
      pPara = new rptParagraph;
      (*pChapter) << pPara;

      superscript = (span_lldf.bExteriorGirder ? "VE" : "VI");
      (*pPara) << "g" << superscript << Sub("Fatigue") << " = " << "mg" << superscript << Sub("1") << "/m =" << scalar.SetValue(span_lldf.gV1.mg) << "/1.2 = " << scalar2.SetValue(span_lldf.gV1.mg/1.2);

      //////////////////////////////////////////////////////////////
      // Reactions
      //////////////////////////////////////////////////////////////
      pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
      (*pChapter) << pPara;
      (*pPara) << "Distribution Factor for Reaction at Pier " << LABEL_PIER(pier1) << rptNewLine;
      pPara = new rptParagraph;
      (*pChapter) << pPara;

      superscript = (reaction1_lldf.bExteriorGirder ? "VE" : "VI");
      (*pPara) << "g" << superscript << Sub("Fatigue") << " = " << "mg" << superscript << Sub("1") << "/m =" << scalar.SetValue(reaction1_lldf.gR1.mg) << "/1.2 = " << scalar2.SetValue(reaction1_lldf.gR1.mg/1.2);

        ///////

      pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
      (*pChapter) << pPara;
      (*pPara) << "Distribution Factor for Reaction at Pier " << LABEL_PIER(pier2) << rptNewLine;
      pPara = new rptParagraph;
      (*pChapter) << pPara;

      superscript = (reaction2_lldf.bExteriorGirder ? "VE" : "VI");
      (*pPara) << "g" << superscript << Sub("Fatigue") << " = " << "mg" << superscript << Sub("1") << "/m =" << scalar.SetValue(reaction2_lldf.gR1.mg) << "/1.2 = " << scalar2.SetValue(reaction2_lldf.gR1.mg/1.2);
   }
}


lrfdLiveLoadDistributionFactorBase* CUBeamDistFactorEngineer::GetLLDFParameters(SpanIndexType spanOrPier,GirderIndexType gdr,DFParam dfType,Float64 fcgdr,UBEAM_LLDFDETAILS* plldf)
{
   GET_IFACE(IGirder, pGdr);
   GET_IFACE(IBridge,pBridge);
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
   GetIndicies(spanOrPier,dfType,span,pier,prev_span,next_span,prev_pier,next_pier);

   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   ATLASSERT( pBridgeDesc->GetDistributionFactorMethod() != pgsTypes::DirectlyInput );

   const CSpanData* pSpan = pBridgeDesc->GetSpan(span);

   GirderIndexType nGirders = pSpan->GetGirderCount();

   if ( nGirders <= gdr )
   {
      ATLASSERT(0);
      gdr = nGirders-1;
   }

   ///////////////////////////////////////////////////////////////////////////
   // Determine overhang and spacing information
   GetGirderSpacingAndOverhang(span,gdr,dfType, plldf);

   // put a poi at controlling location from spacing comp
   pgsPointOfInterest poi(span,gdr,plldf->ControllingLocation);

   // Throws exception if fails requirement (no need to catch it)
   GET_IFACE(ILiveLoadDistributionFactors, pDistFactors);
   pDistFactors->VerifyDistributionFactorRequirements(poi);

   pgsTypes::TrafficBarrierOrientation side = pBarriers->GetNearestBarrier(span,gdr);
   double curb_offset = pBarriers->GetInterfaceWidth(side);

   // compute de (inside edge of barrier to CL of exterior web)
   Float64 wd = pGdr->GetCL2ExteriorWebDistance(poi); // cl beam to cl web
   ATLASSERT(wd>0.0);

   // Note this is not exactly correct because opposite exterior beam might be different, but we won't be using this data set for that beam
   plldf->leftDe  = plldf->leftCurbOverhang - wd;  
   plldf->rightDe = plldf->rightCurbOverhang - wd; 

   plldf->d     = pGdr->GetHeight(poi);
   plldf->L = GetEffectiveSpanLength(spanOrPier,gdr,dfType);

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
                                 plldf->skew2);
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
                                      plldf->skew2);
   }
   else if ( plldf->Method == LLDF_TXDOT )
   {
      pLLDF = new lrfdTxDotLldfTypeBC(plldf->gdrNum, // to fix this warning, clean up the WBFL data types
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
                                      plldf->wCurbToCurb,
                                      plldf->skew1, 
                                      plldf->skew2);
   }
   else
   {
      ATLASSERT(0); // big problemo
   }

   GET_IFACE(ILiveLoads,pLiveLoads);
   pLLDF->SetRangeOfApplicabilityAction( pLiveLoads->GetLldfRangeOfApplicabilityAction() );

   return pLLDF;
}

void CUBeamDistFactorEngineer::ReportMoment(Uint32 spanOrPier, rptParagraph* pPara,UBEAM_LLDFDETAILS& lldf,lrfdILiveLoadDistributionFactor::DFResult& gM1,lrfdILiveLoadDistributionFactor::DFResult& gM2,double gM,bool bSIUnits,IDisplayUnits* pDisplayUnits)
{
   std::string strImagePath(pgsReportStyleHolder::GetImagePath());

   rptRcScalar scalar;
   scalar.SetFormat( sysNumericFormatTool::Fixed );
   scalar.SetWidth(6);
   scalar.SetPrecision(3);
   scalar.SetTolerance(1.0e-6);

   GET_IFACE(ILibrary, pLib);
   GET_IFACE(ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

   if ( lldf.bExteriorGirder )
   {
      if ( gM1.ControllingMethod & INTERIOR_OVERRIDE )
      {
         (*pPara) << Bold("1 Loaded Lane: Exterior factor may not be less than that for interior") << rptNewLine;
         (*pPara) << "mg" << Super("ME") << Sub("1") << " = " << "mg" << Super("MI") << Sub("1") << " = " << scalar.SetValue(gM1.mg) << rptNewLine;
      }
      else
      {
         GET_IFACE(IBridge,pBridge);

         // if roadway width is > 20ft and TxDOT, and nb>=3, use multiple lane method
         GirderIndexType nb = pBridge->GetGirderCount(spanOrPier);
         double w20 = ::ConvertToSysUnits(20.0, unitMeasure::Feet);
         if (pSpecEntry->GetLiveLoadDistributionMethod()==LLDF_TXDOT && lldf.wCurbToCurb>=w20 && nb>=3)
         {
            // Using TxDOT spec and w >= 20.0
            (*pPara) << Bold("1 Loaded Lane: TxDOT method and roadway width is >= 20.0 ft, use multiple lane method") << rptNewLine;
            (*pPara) << "mg" << Super("ME") << Sub("1") << " = " << "mg" << Super("ME") << Sub("2") << " = " << scalar.SetValue(gM1.mg) << rptNewLine;
         }
         else
         {

            if (gM1.EqnData.bWasUsed )
            {
               // Using WSDOT spec's, and the slab overhang is <= half the girder spacing
               (*pPara) << Bold("1 Loaded Lane: Spec Equations") << rptNewLine;
               (*pPara) << rptRcImage(strImagePath + (bSIUnits ? "mg 1 MI Type C SI.gif" : "mg 1 MI Type C US.gif")) << rptNewLine;
               (*pPara) << "mg" << Super("ME") << Sub("1") << " = " << "mg" << Super("MI") << Sub("1") << " = " << scalar.SetValue(gM1.EqnData.mg) << rptNewLine;
            }

            if ( gM1.LeverRuleData.bWasUsed )
            {
               (*pPara) << Bold("1 Loaded Lane: Lever Rule") << rptNewLine;
               Float64 factor = 1.0;
               if (pSpecEntry->GetLiveLoadDistributionMethod() == LLDF_TXDOT)
               {
                  (*pPara) <<"  For TxDOT method, do not apply multiple presence factor and multiply lever rule result by 0.9"<< rptNewLine;
                  factor = 0.9;
               }

               ReportLeverRule(pPara,true,factor,gM1.LeverRuleData,m_pBroker,pDisplayUnits);
            }

            if ( gM1.RigidData.bWasUsed )
            {
               (*pPara) << Bold("1 Loaded Lane: Rigid Method") << rptNewLine;
               ReportRigidMethod(pPara,gM1.RigidData,m_pBroker,pDisplayUnits);
            }

            if ( gM1.LanesBeamsData.bWasUsed )
            {
               (*pPara) << Bold("1 Loaded Lane: Number of Lanes over Number of Beams - Factor cannot be less than this") << rptNewLine;
               (*pPara) << "Skew correction is not applied to Lanes/Beams method"<< rptNewLine;
               ReportLanesBeamsMethod(pPara,gM1.LanesBeamsData,m_pBroker,pDisplayUnits);
            }

         }
      }

      (*pPara) << rptNewLine;

      if ( lldf.Nl >= 2 )
      {
         if ( gM2.ControllingMethod & INTERIOR_OVERRIDE )
         {
            (*pPara) << Bold("2+ Loaded Lanes: Exterior factor may not be less than that for interior") << rptNewLine;
            (*pPara) << "mg" << Super("ME") << Sub("2") << " = " << "mg" << Super("MI") << Sub("2") << " = " << scalar.SetValue(gM2.mg) << rptNewLine;
         }
         else
         {
            if (gM2.EqnData.bWasUsed )
            {
               (*pPara) << Bold("2+ Loaded Lanes: Spec Equation") << rptNewLine;

               Float64 so = lldf.Side==dfLeft ? lldf.leftSlabOverhang: lldf.rightSlabOverhang;
               if ( pSpecEntry->GetLiveLoadDistributionMethod() == LLDF_WSDOT && so <= lldf.Savg/2 )
               {
                  (*pPara) << rptRcImage(strImagePath + (bSIUnits ? "mg 2 MI Type C SI.gif" : "mg 2 MI Type C US.gif")) << rptNewLine;
                  (*pPara) << "mg" << Super("ME") << Sub("2+") << " = " << "mg" << Super("MI") << Sub("2+") << " = " << scalar.SetValue(gM2.EqnData.mg) << rptNewLine;
               }
               else
               {
                  (*pPara) << rptRcImage(strImagePath + (bSIUnits ? "mg 2 ME Type C SI.gif" : "mg 2 ME Type C US.gif")) << rptNewLine;
                  (*pPara) << rptRcImage(strImagePath + (bSIUnits ? "mg 2 MI Type C SI.gif" : "mg 2 MI Type C US.gif")) << rptNewLine;
                  (*pPara) << "mg" << Super("MI") << Sub("2+") << " = " << scalar.SetValue(gM2.EqnData.mg) << rptNewLine;
                  (*pPara) << "e = " << gM2.EqnData.e << rptNewLine;
                  (*pPara) << "mg" << Super("ME") << Sub("2+") << " = " << scalar.SetValue(gM2.EqnData.mg * gM2.EqnData.e) << rptNewLine;
               }
            }

            if ( gM2.LeverRuleData.bWasUsed )
            {
               (*pPara) << Bold("2+ Loaded Lanes: Lever Rule") << rptNewLine;
               Float64 factor = 1.0;
               if (pSpecEntry->GetLiveLoadDistributionMethod() == LLDF_TXDOT)
               {
                  (*pPara) << "  For TxDOT method, multiply lever rule result by 0.9"<< rptNewLine;
                  factor = 0.9;
               }

               ReportLeverRule(pPara,true,factor,gM2.LeverRuleData,m_pBroker,pDisplayUnits);
            }

            if ( gM2.RigidData.bWasUsed )
            {
               (*pPara) << Bold("2+ Loaded Lanes: Rigid Method") << rptNewLine;
               ReportRigidMethod(pPara,gM2.RigidData,m_pBroker,pDisplayUnits);
            }

            if ( gM2.LanesBeamsData.bWasUsed )
            {
               (*pPara) << Bold("2+ Loaded Lanes: Number of Lanes over Number of Beams") << rptNewLine;
               (*pPara) << "Skew correction is not applied to Lanes/Beams method"<< rptNewLine;
               ReportLanesBeamsMethod(pPara,gM2.LanesBeamsData,m_pBroker,pDisplayUnits);
            }
         }
         (*pPara) << rptNewLine;
      }

      if ( gM2.LanesBeamsData.bWasUsed && gM1.LanesBeamsData.bWasUsed )
      {
         (*pPara) << "Skew Correction not applied to N"<<Sub("l")<<"/N"<<Sub("b")<<" method"<< rptNewLine;
         if ( lldf.Nl >= 2 )
         {
            (*pPara) << "mg" << Super("ME") << " = " << ((gM2.mg>gM1.mg) ? scalar.SetValue(gM2.mg):scalar.SetValue(gM1.mg)) << Bold(" < Controls") << rptNewLine ;
         }
         else
         {
            (*pPara) << "mg" << Super("ME") << " = " << scalar.SetValue(gM1.mg)<< Bold(" < Controls") << rptNewLine;
         }
      }
      else
      {
         (*pPara) << Bold("Skew Correction") << rptNewLine;
         (*pPara) << rptRcImage(strImagePath + "Skew Correction for Moment Type C.gif") << rptNewLine;
         (*pPara) << "Skew Correction Factor: = " << scalar.SetValue(gM1.SkewCorrectionFactor) << rptNewLine;
         (*pPara) << rptNewLine;
         (*pPara) << "Skew Corrected Factor: mg" << Super("ME") << Sub("1") << " = " << scalar.SetValue(gM1.mg);
         (lldf.Nl == 1 || gM1.mg >= gM2.mg) ? (*pPara) << Bold(" < Controls") << rptNewLine : (*pPara) << rptNewLine;
         if ( lldf.Nl >= 2 )
         {
            (*pPara) << "Skew Corrected Factor: mg" << Super("ME") << Sub("2+") << " = " << scalar.SetValue(gM2.mg);
            (gM2.mg > gM1.mg) ? (*pPara) << Bold(" < Controls") << rptNewLine : (*pPara) << rptNewLine;
         }
      }
   }
   else
   {
      // Distribution factor for interior girder
      if (gM1.EqnData.bWasUsed )
      {
         (*pPara) << Bold("1 Loaded Lane: Spec Equations") << rptNewLine;
         (*pPara) << rptRcImage(strImagePath + (bSIUnits ? "mg 1 MI Type C SI.gif" : "mg 1 MI Type C US.gif")) << rptNewLine;
         (*pPara) << "mg" << Super("MI") << Sub("1") << " = " << scalar.SetValue(gM1.EqnData.mg) << rptNewLine;
      }

      if ( gM1.LeverRuleData.bWasUsed )
      {
         (*pPara) << Bold("1 Loaded Lane: Lever Rule") << rptNewLine;
         ReportLeverRule(pPara,true,1.0,gM1.LeverRuleData,m_pBroker,pDisplayUnits);
      }

      if ( gM1.LanesBeamsData.bWasUsed )
      {
         (*pPara) << Bold("1 Loaded Lane: Number of Lanes over Number of Beams - Factor cannot be less than this") << rptNewLine;
         (*pPara) << "Skew correction is not applied to Lanes/Beams method"<< rptNewLine;
         ReportLanesBeamsMethod(pPara,gM1.LanesBeamsData,m_pBroker,pDisplayUnits);
      }

      (*pPara) << rptNewLine;


      if ( lldf.Nl >= 2 )
      {
         if (gM2.EqnData.bWasUsed )
         {
            (*pPara) << Bold("2+ Loaded Lanes: Spec Equations") << rptNewLine;
            (*pPara) << rptRcImage(strImagePath + (bSIUnits ? "mg 2 MI Type C SI.gif" : "mg 2 MI Type C US.gif")) << rptNewLine;
            (*pPara) << "mg" << Super("MI") << Sub("2+") << " = " << scalar.SetValue(gM2.EqnData.mg) << rptNewLine;
         }

         if ( gM2.LeverRuleData.bWasUsed )
         {
            (*pPara) << Bold("2+ Loaded Lanes: Lever Rule") << rptNewLine;
            ReportLeverRule(pPara,true,1.0,gM2.LeverRuleData,m_pBroker,pDisplayUnits);
         }

         if ( gM2.LanesBeamsData.bWasUsed )
         {
            (*pPara) << Bold("2+ Loaded Lanes: Number of Lanes over Number of Beams") << rptNewLine;
            (*pPara) << "Skew correction is not applied to Lanes/Beams method"<< rptNewLine;
            ReportLanesBeamsMethod(pPara,gM2.LanesBeamsData,m_pBroker,pDisplayUnits);
         }

         (*pPara) << rptNewLine;
      }

      if ( gM2.LanesBeamsData.bWasUsed && gM1.LanesBeamsData.bWasUsed )
      {
         (*pPara) << "Skew Correction not applied to N"<<Sub("l")<<"/N"<<Sub("b")<<" method"<< rptNewLine;
         if ( lldf.Nl >= 2 )
         {
            (*pPara) << "mg" << Super("ME") << " = " << ((gM2.mg>gM1.mg) ? scalar.SetValue(gM2.mg):scalar.SetValue(gM1.mg)) << Bold(" < Controls") <<rptNewLine ;
         }
         else
         {
            (*pPara) << "mg" << Super("ME") << " = " << scalar.SetValue(gM1.mg) << Bold(" < Controls") << rptNewLine;
         }
      }
      else
      {
         (*pPara) << Bold("Skew Correction") << rptNewLine;
         (*pPara) << rptRcImage(strImagePath + "Skew Correction for Moment Type C.gif") << rptNewLine;
         (*pPara) << "Skew Correction Factor: = " << scalar.SetValue(gM1.SkewCorrectionFactor) << rptNewLine;
         (*pPara) << rptNewLine;
         (*pPara) << "Skew Corrected Factor: mg" << Super("MI") << Sub("1") << " = " << scalar.SetValue(gM1.mg);
         (lldf.Nl == 1 || gM1.mg >= gM2.mg) ? (*pPara) << Bold(" < Controls") << rptNewLine: (*pPara) << rptNewLine;
         if ( lldf.Nl >= 2 )
         {
            (*pPara) << "Skew Corrected Factor: mg" << Super("MI") << Sub("2+") << " = " << scalar.SetValue(gM2.mg);
            (gM2.mg > gM1.mg) ? (*pPara) << Bold(" < Controls") << rptNewLine : (*pPara) << rptNewLine;
         }
      }
   }
}

void CUBeamDistFactorEngineer::ReportShear(Uint32 spanOrPier,rptParagraph* pPara,UBEAM_LLDFDETAILS& lldf,lrfdILiveLoadDistributionFactor::DFResult& gV1,lrfdILiveLoadDistributionFactor::DFResult& gV2,double gV,bool bSIUnits,IDisplayUnits* pDisplayUnits)
{
   std::string strImagePath(pgsReportStyleHolder::GetImagePath());

   rptRcScalar scalar;
   scalar.SetFormat( sysNumericFormatTool::Fixed );
   scalar.SetWidth(6);
   scalar.SetPrecision(3);
   scalar.SetTolerance(1.0e-6);

   GET_IFACE(ILibrary, pLib);
   GET_IFACE(ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

   if ( lldf.bExteriorGirder )
   {
      if ( gV1.ControllingMethod & INTERIOR_OVERRIDE )
      {
         (*pPara) << Bold("1 Loaded Lane: Exterior factor may not be less than that for interior") << rptNewLine;
         (*pPara) << "mg" << Super("VE") << Sub("1") << " = " << "mg" << Super("VI") << Sub("1") << " = " << scalar.SetValue(gV1.mg) << rptNewLine;
      }
      else
      {
         GET_IFACE(IBridge,pBridge);

         // if roadway width is > 20ft and TxDOT, and nb>=3, use multiple lane method
         GirderIndexType nb = pBridge->GetGirderCount(spanOrPier);
         Float64 roadwayWidth = pBridge->GetCurbToCurbWidth(0.00);
         double w20 = ::ConvertToSysUnits(20.0, unitMeasure::Feet);
         if (pSpecEntry->GetLiveLoadDistributionMethod()==LLDF_TXDOT && roadwayWidth>=w20 && nb>=3)
         {
            // Using TxDOT spec and w >= 20.0
            (*pPara) << Bold("1 Loaded Lane: TxDOT method and roadway width is >= 20.0 ft, use multiple lane method") << rptNewLine;
            (*pPara) << "mg" << Super("VE") << Sub("1") << " = " << "mg" << Super("VE") << Sub("2") << " = " << scalar.SetValue(gV1.mg) << rptNewLine;
         }
         else
         {
            if ( gV1.EqnData.bWasUsed )
            {
               (*pPara) << Bold("1 Loaded Lane: Spec Equations") << rptNewLine;
               (*pPara) << rptRcImage(strImagePath + (bSIUnits ? "mg 1 VI Type C SI.gif" : "mg 1 VI Type C US.gif")) << rptNewLine;
               (*pPara) << "mg" << Super("VE") << Sub("1") << " = " << "mg" << Super("VI") << Sub("1") << " = " << scalar.SetValue(gV1.EqnData.mg) << rptNewLine;
            }

            if ( gV1.LeverRuleData.bWasUsed )
            {
               (*pPara) << Bold("1 Loaded Lane: Lever Rule") << rptNewLine;
               Float64 factor = 1.0;
               if (pSpecEntry->GetLiveLoadDistributionMethod() == LLDF_TXDOT)
               {
                  (*pPara) << "  For TxDOT method, do not apply multiple presence factor and multiply lever rule result by 0.9" << rptNewLine;
                  factor = 0.9;
               }

               ReportLeverRule(pPara,false,factor,gV1.LeverRuleData,m_pBroker,pDisplayUnits);
            }

            if ( gV1.RigidData.bWasUsed )
            {
               (*pPara) << Bold("1 Loaded Lane: Rigid Method") << rptNewLine;
               (*pPara) << "mg" << Super("VE") << Sub("1") << " = " << scalar.SetValue(gV1.RigidData.mg) << rptNewLine;
               (*pPara) << "See Moment for details" << rptNewLine;
            }

            if ( gV1.LanesBeamsData.bWasUsed )
            {
               (*pPara) << Bold("1 Loaded Lane: Number of Lanes over Number of Beams - Factor cannot be less than this") << rptNewLine;
               (*pPara) << "Skew correction is not applied to Lanes/Beams method"<< rptNewLine;
               ReportLanesBeamsMethod(pPara,gV1.LanesBeamsData,m_pBroker,pDisplayUnits);
            }
         }
      }

      (*pPara) << rptNewLine;

      if ( lldf.Nl >= 2 )
      {
         if ( gV2.ControllingMethod & INTERIOR_OVERRIDE )
         {
            (*pPara) << Bold("2+ Loaded Lanes: Exterior factor may not be less than that for interior") << rptNewLine;
            (*pPara) << "mg" << Super("VE") << Sub("2") << " = " << "mg" << Super("VI") << Sub("2") << " = " << scalar.SetValue(gV2.mg) << rptNewLine;
         }
         else
         {
            if ( gV2.EqnData.bWasUsed )
            {
               (*pPara) << Bold("2+ Loaded Lanes: Spec Equation") << rptNewLine;
               Float64 so = lldf.Side==dfLeft ? lldf.leftSlabOverhang: lldf.rightSlabOverhang;
               if ( pSpecEntry->GetLiveLoadDistributionMethod() == LLDF_WSDOT && so <= lldf.Savg/2 )
               {
                  (*pPara) << rptRcImage(strImagePath + (bSIUnits ? "mg 2 VI Type C SI.gif" : "mg 2 VI Type C US.gif")) << rptNewLine;
                  (*pPara) << "mg" << Super("VE") << Sub("2+") << " = " << "mg" << Super("VI") << Sub("2+") << " = " << scalar.SetValue(gV2.EqnData.mg) << rptNewLine;
               }
               else
               {
                  (*pPara) << rptRcImage(strImagePath + (bSIUnits ? "mg 2 VE Type C SI.gif" : "mg 2 VE Type C US.gif")) << rptNewLine;
                  (*pPara) << rptRcImage(strImagePath + (bSIUnits ? "mg 2 VI Type C SI.gif" : "mg 2 VI Type C US.gif")) << rptNewLine;
                  (*pPara) << "mg" << Super("VI") << Sub("2+") << " = " << scalar.SetValue(gV2.EqnData.mg) << rptNewLine;
                  (*pPara) << "e = " << gV2.EqnData.e << rptNewLine;
                  (*pPara) << "mg" << Super("VE") << Sub("2+") << " = " << scalar.SetValue(gV2.EqnData.mg * gV2.EqnData.e) << rptNewLine;
               }
            }

            if ( gV2.LeverRuleData.bWasUsed )
            {
               (*pPara) << Bold("2+ Loaded Lanes: Lever Rule") << rptNewLine;
               Float64 factor = 1.0;
               if (pSpecEntry->GetLiveLoadDistributionMethod() == LLDF_TXDOT)
               {
                  (*pPara) << "  For TxDOT method, multiply lever rule result by 0.9" << rptNewLine;
                  factor = 0.9;
               }

               ReportLeverRule(pPara,false,factor,gV2.LeverRuleData,m_pBroker,pDisplayUnits);
            }

            if ( gV2.RigidData.bWasUsed )
            {
               (*pPara) << Bold("2+ Loaded Lanes: Rigid Method") << rptNewLine;
               (*pPara) << "mg" << Super("VE") << Sub("2+") << " = " << scalar.SetValue(gV2.RigidData.mg) << rptNewLine;
               (*pPara) << "See Moment for details" << rptNewLine;
            }

            if ( gV2.LanesBeamsData.bWasUsed )
            {
               (*pPara) << Bold("2+ Loaded Lanes: Number of Lanes over Number of Beams") << rptNewLine;
               (*pPara) << "Skew correction is not applied to Lanes/Beams method"<< rptNewLine;
               ReportLanesBeamsMethod(pPara,gV2.LanesBeamsData,m_pBroker,pDisplayUnits);
            }
         }
      }
      (*pPara) << rptNewLine;

      if ( gV2.LanesBeamsData.bWasUsed && gV1.LanesBeamsData.bWasUsed )
      {
         (*pPara) << "Skew Correction not applied to N"<<Sub("l")<<"/N"<<Sub("b")<<" method"<< rptNewLine;
         if ( lldf.Nl >= 2 )
         {
            (*pPara) << "mg" << Super("VE") << " = " << ((gV2.mg>gV1.mg) ? scalar.SetValue(gV2.mg):scalar.SetValue(gV1.mg)) << Bold(" < Controls") << rptNewLine ;
         }
         else
         {
            (*pPara) << "mg" << Super("VE") << " = " << scalar.SetValue(gV1.mg)<< Bold(" < Controls") << rptNewLine;
         }
      }
      else
      {
         (*pPara) << Bold("Skew Correction") << rptNewLine;
         (*pPara) << rptRcImage(strImagePath + (bSIUnits ? "Skew Correction for Shear Type C SI.gif" : "Skew Correction for Shear Type C US.gif")) << rptNewLine;
         (*pPara) << "Skew Correction Factor: = " << scalar.SetValue(gV1.SkewCorrectionFactor) << rptNewLine;
         (*pPara) << rptNewLine;
         (*pPara) << "Skew Corrected Factor: mg" << Super("VE") << Sub("1") << " = " << scalar.SetValue(gV1.mg);
         (lldf.Nl == 1 || gV1.mg >= gV2.mg) ? (*pPara) << Bold(" < Controls") << rptNewLine : (*pPara) << rptNewLine;
         if ( lldf.Nl >= 2 )
         {
            (*pPara) << "Skew Corrected Factor: mg" << Super("VE") << Sub("2+") << " = " << scalar.SetValue(gV2.mg);
            (gV2.mg > gV1.mg) ? (*pPara) << Bold(" < Controls") << rptNewLine : (*pPara)  << rptNewLine;
         }
      }
   }
   else
   {
      // interior girder
      if ( gV1.EqnData.bWasUsed )
      {
         (*pPara) << Bold("1 Loaded Lane: Spec Equations") << rptNewLine;
         (*pPara) << rptRcImage(strImagePath + (bSIUnits ? "mg 1 VI Type C SI.gif" : "mg 1 VI Type C US.gif")) << rptNewLine;
         (*pPara) << "mg" << Super("VI") << Sub("1") << " = " << scalar.SetValue(gV1.EqnData.mg) << rptNewLine;
      }

      if ( gV1.LeverRuleData.bWasUsed )
      {
         (*pPara) << Bold("1 Loaded Lane: Lever Rule") << rptNewLine;
         ReportLeverRule(pPara,false,1.0,gV1.LeverRuleData,m_pBroker,pDisplayUnits);
      }

      if ( gV1.LanesBeamsData.bWasUsed )
      {
         (*pPara) << Bold("1 Loaded Lane: Number of Lanes over Number of Beams - Factor cannot be less than this") << rptNewLine;
         (*pPara) << "Skew correction is not applied to Lanes/Beams method"<< rptNewLine;
         ReportLanesBeamsMethod(pPara,gV1.LanesBeamsData,m_pBroker,pDisplayUnits);
      }

      (*pPara) << rptNewLine;

      if ( lldf.Nl >= 2 )
      {
         if ( gV2.EqnData.bWasUsed )
         {
            (*pPara) << Bold("2+ Loaded Lanes: Spec Equations") << rptNewLine;
            (*pPara) << rptRcImage(strImagePath + (bSIUnits ? "mg 2 VI Type C SI.gif" : "mg 2 VI Type C US.gif")) << rptNewLine;
            (*pPara) << "mg" << Super("VI") << Sub("2+") << " = " << scalar.SetValue(gV2.EqnData.mg) << rptNewLine;
         }

         if ( gV2.LeverRuleData.bWasUsed )
         {
            (*pPara) << Bold("2+ Loaded Lanes: Lever Rule") << rptNewLine;
            ReportLeverRule(pPara,false,1.0,gV2.LeverRuleData,m_pBroker,pDisplayUnits);
         }

         if ( gV2.LanesBeamsData.bWasUsed )
         {
            (*pPara) << Bold("2+ Loaded Lanes: Number of Lanes over Number of Beams") << rptNewLine;
            (*pPara) << "Skew correction is not applied to Lanes/Beams method"<< rptNewLine;
            ReportLanesBeamsMethod(pPara,gV2.LanesBeamsData,m_pBroker,pDisplayUnits);
         }

         (*pPara) << rptNewLine;
      }

      if ( gV2.LanesBeamsData.bWasUsed && gV1.LanesBeamsData.bWasUsed )
      {
         (*pPara) << "Skew Correction not applied to N"<<Sub("l")<<"/N"<<Sub("b")<<" method"<< rptNewLine;
         if ( lldf.Nl >= 2 )
         {
            (*pPara) << "mg" << Super("VE") << " = " << ((gV2.mg>gV1.mg) ? scalar.SetValue(gV2.mg):scalar.SetValue(gV1.mg)) << Bold(" < Controls") <<rptNewLine ;
         }
         else
         {
            (*pPara) << "mg" << Super("VE") << " = " << scalar.SetValue(gV1.mg) << Bold(" < Controls") << rptNewLine;
         }
      }
      else
      {
         (*pPara) << Bold("Skew Correction") << rptNewLine << rptRcImage(strImagePath + (bSIUnits ? "Skew Correction for Shear Type C SI.gif" : "Skew Correction for Shear Type C US.gif")) << rptNewLine;
         (*pPara) << "Skew Correction Factor: = " << scalar.SetValue(gV1.SkewCorrectionFactor) << rptNewLine;
         (*pPara) << rptNewLine;
         (*pPara) << "Skew Corrected Factor: mg" << Super("VI") << Sub("1") << " = " << scalar.SetValue(gV1.mg);
         (lldf.Nl == 1 || gV1.mg >= gV2.mg) ? (*pPara) << Bold(" < Controls") << rptNewLine : (*pPara) << rptNewLine;
         if ( lldf.Nl >= 2 )
         {
            (*pPara) << "Skew Corrected Factor: mg" << Super("VI") << Sub("2+") << " = " << scalar.SetValue(gV2.mg);
            (gV2.mg > gV1.mg) ? (*pPara) << Bold(" < Controls") << rptNewLine : (*pPara) << rptNewLine;
         }
      }
   }
}

std::string CUBeamDistFactorEngineer::GetComputationDescription(SpanIndexType span,GirderIndexType gdr,const std::string& libraryEntryName,pgsTypes::SupportedDeckType decktype, pgsTypes::AdjacentTransverseConnectivity connect)
{
   GET_IFACE(ILibrary, pLib);
   GET_IFACE(ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

   Int16 lldfMethod = pSpecEntry->GetLiveLoadDistributionMethod();

   std::string descr;

   if ( m_bTypeB )
      descr = "Type (b) cross section. With ";
   else
      descr = "Type (c) cross section. With ";


   if ( lldfMethod == LLDF_WSDOT )
   {
      descr += std::string("WSDOT Method per Design Memorandum 2-1999 Dated February 22, 1999");
   }
   else if ( lldfMethod == LLDF_TXDOT )
   {
      descr += std::string("TxDOT Method per Section 3 of the TxDOT LRFD Bridge Design Manual, Revised April, 2007");
   }
   else if ( lldfMethod == LLDF_LRFD )
   {
      descr += std::string("AASHTO LRFD Method per Article 4.6.2.2");
   }
   else
   {
      ATLASSERT(0);
   }

   // Special text if ROA is ignored
   GET_IFACE(ILiveLoads,pLiveLoads);
   std::string straction = pLiveLoads->GetLLDFSpecialActionText();
   if ( !straction.empty() )
   {
      descr += straction;
   }

   return descr;
}
