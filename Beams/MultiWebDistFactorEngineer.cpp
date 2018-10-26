///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2011  Washington State Department of Transportation
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
#include "..\PGSuperException.h"
#include <Units\SysUnits.h>
#include <PsgLib\TrafficBarrierEntry.h>
#include <PsgLib\SpecLibraryEntry.h>
#include <PsgLib\GirderLibraryEntry.h>
#include <PgsExt\BridgeDescription.h>
#include <PgsExt\StatusItem.h>
#include <PgsExt\GirderLabel.h>
#include <Reporting\ReportStyleHolder.h>
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\DistributionFactors.h>
#include <IFace\StatusCenter.h>
#include "helper.h"

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

void CMultiWebDistFactorEngineer::BuildReport(SpanIndexType span,GirderIndexType gdr,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits)
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
   GET_IFACE(ILiveLoads, pLiveLoads);

   // determine continuity
   bool bContinuous, bContinuousAtStart, bContinuousAtEnd;
   pBridge->IsContinuousAtPier(pier1,&bContinuous,&bContinuousAtStart);
   pBridge->IsContinuousAtPier(pier2,&bContinuousAtEnd,&bContinuous);

   bool bIntegral, bIntegralAtStart, bIntegralAtEnd;
   pBridge->IsIntegralAtPier(pier1,&bIntegral,&bIntegralAtStart);
   pBridge->IsIntegralAtPier(pier2,&bIntegralAtEnd,&bIntegral);

   rptParagraph* pPara;

   bool bSIUnits = IS_SI_UNITS(pDisplayUnits);
   std::_tstring strImagePath(pgsReportStyleHolder::GetImagePath());

   INIT_UV_PROTOTYPE( rptLengthUnitValue,    location, pDisplayUnits->GetSpanLengthUnit(),      true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,    offsetFormatter, pDisplayUnits->GetSpanLengthUnit(),      false );
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

   std::_tstring strGirderName = pSpan->GetGirderTypes()->GetGirderName(gdr);

   pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
   (*pPara) << _T("Method of Computation:")<<rptNewLine;
   (*pChapter) << pPara;
   pPara = new rptParagraph;
   (*pChapter) << pPara;
   (*pPara) << GetComputationDescription(span,gdr,
                                         strGirderName,
                                         pDeck->DeckType,
                                         pDeck->TransverseConnectivity);

   pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
   (*pChapter) << pPara;
   (*pPara) << _T("Distribution Factor Parameters") << rptNewLine;
   pPara = new rptParagraph;
   (*pChapter) << pPara;

   Float64 station,offset;
   pBridge->GetStationAndOffset(pgsPointOfInterest(span,gdr,span_lldf.ControllingLocation),&station, &offset);
   Float64 supp_dist = span_lldf.ControllingLocation - pBridge->GetGirderStartConnectionLength(span,gdr);
   (*pPara) << _T("Girder Spacing and Slab Overhang are measured along a line that is normal to the alignment and passing through a point ") << location.SetValue(supp_dist) << _T(" from the left support along the centerline of girder.") << rptNewLine;
   (*pPara) << _T("The measurement line passes through Station ") << rptRcStation(station, &pDisplayUnits->GetStationFormat() ) << _T(" (") << RPT_OFFSET(offset,offsetFormatter) << _T(")") << rptNewLine;
   (*pPara) << _T("Girder Spacing: ") << Sub2(_T("S"),_T("avg")) << _T(" = ") << xdim.SetValue(span_lldf.Savg) << rptNewLine;
   (*pPara) << _T("Girder Width: b = ") << xdim.SetValue(span_lldf.b) << rptNewLine;
   (*pPara) << _T("Bridge Width: W = ") << xdim.SetValue(span_lldf.W) << rptNewLine;
   (*pPara) << _T("Lane Width: wLane = ") << xdim.SetValue(span_lldf.wLane) << rptNewLine;
   (*pPara) << _T("Curb Offset = ") << xdim.SetValue(span_lldf.CurbOffset) << rptNewLine;
   Float64 de = span_lldf.Side==dfLeft ? span_lldf.leftDe : span_lldf.rightDe;
   (*pPara) << Sub2(_T("d"),_T("e")) << _T(" = ") << xdim.SetValue(de) << rptNewLine;
   Float64 ro = span_lldf.Side==dfLeft ? span_lldf.leftCurbOverhang : span_lldf.rightCurbOverhang;
   (*pPara) << _T("Roadway overhang = ") << xdim.SetValue(ro) << rptNewLine;
   (*pPara) << _T("Number of Design Lanes: N") << Sub(_T("L")) << _T(" = ") << span_lldf.Nl << rptNewLine;

   if (pBridgeDesc->GetDistributionFactorMethod() != pgsTypes::LeverRule)
   {
      if(span_lldf.Method==LLDF_TXDOT || !span_lldf.connectedAsUnit)
      {
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
      pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
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
      pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
      (*pChapter) << pPara;
      (*pPara) << _T("Distribution Factor for Negative Moment over Pier ") << long(pier1+1) << rptNewLine;
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
   pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
   (*pChapter) << pPara;
   if ( bContinuousAtStart || bContinuousAtEnd || bIntegralAtStart || bIntegralAtEnd )
      (*pPara) << _T("Distribution Factor for Positive and Negative Moment in Span ") << LABEL_SPAN(span) << rptNewLine;
   else
      (*pPara) << _T("Distribution Factor for Positive Moment in Span ") << LABEL_SPAN(span) << rptNewLine;
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
      pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
      (*pChapter) << pPara;
      (*pPara) << _T("Distribution Factor for Negative Moment over Pier ") << long(pier2+1) << rptNewLine;
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
   pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
   (*pChapter) << pPara;
   (*pPara) << _T("Distribution Factor for Shear in Span ") << LABEL_SPAN(span) << rptNewLine;
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

   //////////////////////////////////////////////////////////////
   // Reactions
   //////////////////////////////////////////////////////////////
   pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
   (*pChapter) << pPara;
   (*pPara) << _T("Distribution Factor for Reaction at Pier ") << long(pier1+1) << rptNewLine;
   pPara = new rptParagraph;
   (*pChapter) << pPara;

   (*pPara) << _T("Average Skew Angle: ") << symbol(theta) << _T(" = ") << angle.SetValue(fabs((reaction1_lldf.skew1 + reaction1_lldf.skew2)/2)) << rptNewLine;
   (*pPara) << _T("Span Length: L = ") << xdim.SetValue(reaction1_lldf.L) << rptNewLine << rptNewLine;

   ReportShear(pPara,
               reaction1_lldf,
               reaction1_lldf.gR1,
               reaction1_lldf.gR2,
               reaction1_lldf.gR,
               bSIUnits,pDisplayUnits);

     ///////

   pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
   (*pChapter) << pPara;
   (*pPara) << _T("Distribution Factor for Reaction at Pier ") << long(pier2+1) << rptNewLine;
   pPara = new rptParagraph;
   (*pChapter) << pPara;

   (*pPara) << _T("Average Skew Angle: ") << symbol(theta) << _T(" = ") << angle.SetValue(fabs((reaction2_lldf.skew1 + reaction2_lldf.skew2)/2)) << rptNewLine;
   (*pPara) << _T("Span Length: L = ") << xdim.SetValue(reaction2_lldf.L) << rptNewLine << rptNewLine;

   ReportShear(pPara,
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
      (*pPara) << _T("Fatigue Limit States");
      (*pChapter) << pPara;
      pPara = new rptParagraph;
      (*pChapter) << pPara;

      std::_tstring superscript;

      rptRcScalar scalar2 = scalar;

      //////////////////////////////////////////////////////////////
      // Moments
      //////////////////////////////////////////////////////////////
      if ( bContinuousAtEnd || bIntegralAtEnd )
      {
         pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
         (*pChapter) << pPara;
         (*pPara) << _T("Distribution Factor for Negative Moment over Pier ") << LABEL_PIER(pier1) << rptNewLine;
         pPara = new rptParagraph;
         (*pChapter) << pPara;

         superscript = (pier1_lldf.bExteriorGirder ? _T("ME") : _T("MI"));
         (*pPara) << _T("g") << superscript << Sub(_T("Fatigue")) << _T(" = ") << _T("mg") << superscript << Sub(_T("1")) << _T("/m =") << scalar.SetValue(pier1_lldf.gM1.mg) << _T("/1.2 = ") << scalar2.SetValue(pier1_lldf.gM1.mg/1.2);
      }

      // Positive moment DF
      pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
      (*pChapter) << pPara;
      if ( bContinuousAtStart || bContinuousAtEnd || bIntegralAtStart || bIntegralAtEnd )
         (*pPara) << _T("Distribution Factor for Positive and Negative Moment in Span ") << LABEL_SPAN(span) << rptNewLine;
      else
         (*pPara) << _T("Distribution Factor for Positive Moment in Span ") << LABEL_SPAN(span) << rptNewLine;
      pPara = new rptParagraph;
      (*pChapter) << pPara;

      superscript = (span_lldf.bExteriorGirder ? _T("ME") : _T("MI"));
      (*pPara) << _T("g") << superscript << Sub(_T("Fatigue")) << _T(" = ") << _T("mg") << superscript << Sub(_T("1")) << _T("/m =") << scalar.SetValue(span_lldf.gM1.mg) << _T("/1.2 = ") << scalar2.SetValue(span_lldf.gM1.mg/1.2);

      if ( bContinuousAtEnd || bIntegralAtEnd )
      {
         pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
         (*pChapter) << pPara;
         (*pPara) << _T("Distribution Factor for Negative Moment over Pier ") << LABEL_PIER(pier2) << rptNewLine;
         pPara = new rptParagraph;
         (*pChapter) << pPara;

         superscript = (pier2_lldf.bExteriorGirder ? _T("ME") : _T("MI"));
         (*pPara) << _T("g") << superscript << Sub(_T("Fatigue")) << _T(" = ") << _T("mg") << superscript << Sub(_T("1")) << _T("/m =") << scalar.SetValue(pier2_lldf.gM1.mg) << _T("/1.2 = ") << scalar2.SetValue(pier2_lldf.gM1.mg/1.2);
      }

      //////////////////////////////////////////////////////////////
      // Shears
      //////////////////////////////////////////////////////////////
      pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
      (*pChapter) << pPara;
      (*pPara) << _T("Distribution Factor for Shear in Span ") << LABEL_SPAN(span) << rptNewLine;
      pPara = new rptParagraph;
      (*pChapter) << pPara;

      superscript = (span_lldf.bExteriorGirder ? _T("VE") : _T("VI"));
      (*pPara) << _T("g") << superscript << Sub(_T("Fatigue")) << _T(" = ") << _T("mg") << superscript << Sub(_T("1")) << _T("/m =") << scalar.SetValue(span_lldf.gV1.mg) << _T("/1.2 = ") << scalar2.SetValue(span_lldf.gV1.mg/1.2);

      //////////////////////////////////////////////////////////////
      // Reactions
      //////////////////////////////////////////////////////////////
      pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
      (*pChapter) << pPara;
      (*pPara) << _T("Distribution Factor for Reaction at Pier ") << LABEL_PIER(pier1) << rptNewLine;
      pPara = new rptParagraph;
      (*pChapter) << pPara;

      superscript = (reaction1_lldf.bExteriorGirder ? _T("VE") : _T("VI"));
      (*pPara) << _T("g") << superscript << Sub(_T("Fatigue")) << _T(" = ") << _T("mg") << superscript << Sub(_T("1")) << _T("/m =") << scalar.SetValue(reaction1_lldf.gR1.mg) << _T("/1.2 = ") << scalar2.SetValue(reaction1_lldf.gR1.mg/1.2);

        ///////

      pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
      (*pChapter) << pPara;
      (*pPara) << _T("Distribution Factor for Reaction at Pier ") << LABEL_PIER(pier2) << rptNewLine;
      pPara = new rptParagraph;
      (*pChapter) << pPara;

      superscript = (reaction2_lldf.bExteriorGirder ? _T("VE") : _T("VI"));
      (*pPara) << _T("g") << superscript << Sub(_T("Fatigue")) << _T(" = ") << _T("mg") << superscript << Sub(_T("1")) << _T("/m =") << scalar.SetValue(reaction2_lldf.gR1.mg) << _T("/1.2 = ") << scalar2.SetValue(reaction2_lldf.gR1.mg/1.2);
   }
}

lrfdLiveLoadDistributionFactorBase* CMultiWebDistFactorEngineer::GetLLDFParameters(SpanIndexType spanOrPier,GirderIndexType gdr,DFParam dfType,Float64 fcgdr,MULTIWEB_LLDFDETAILS* plldf)
{
   GET_IFACE(IBridgeMaterial,   pMaterial);
   GET_IFACE(IBridgeMaterialEx, pMaterialEx);
   GET_IFACE(ISectProp2,        pSectProp2);
   GET_IFACE(IGirder,           pGirder);
   GET_IFACE(ILibrary,          pLib);
   GET_IFACE(ISpecification,    pSpec);
   GET_IFACE(IRoadwayData,      pRoadway);
   GET_IFACE(IBridge,           pBridge);
   GET_IFACE(IPointOfInterest,  pPOI);
   GET_IFACE(IBarriers,         pBarriers);
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

   const CDeckDescription* pDeck = pBridgeDesc->GetDeckDescription();
   const CSpanData* pSpan = pBridgeDesc->GetSpan(span);

   GirderIndexType nGirders = pSpan->GetGirderCount();

   if ( nGirders <= gdr )
   {
      ATLASSERT(0);
      gdr = nGirders-1;
   }

   // determine overhang and spacing base data
   GetGirderSpacingAndOverhang(span,gdr,dfType, plldf);

   // put a poi at controlling location from spacing comp
   pgsPointOfInterest poi(span,gdr,plldf->ControllingLocation);

   // Throws exception if fails requirement (no need to catch it)
   GET_IFACE(ILiveLoadDistributionFactors, pDistFactors);
   pDistFactors->VerifyDistributionFactorRequirements(poi);

   plldf->b            = pGirder->GetTopFlangeWidth(poi);

   if (fcgdr>0)
   {
      plldf->I            = pSectProp2->GetIx(pgsTypes::BridgeSite3,poi,fcgdr);
   }
   else
   {
      plldf->I            = pSectProp2->GetIx(pgsTypes::BridgeSite3,poi);
   }

   plldf->PossionRatio = 0.2;

   pgsTypes::TrafficBarrierOrientation side = pBarriers->GetNearestBarrier(span,gdr);
   plldf->CurbOffset = pBarriers->GetInterfaceWidth(side);

   if ( fcgdr < 0 )
   {
      plldf->n     = pMaterial->GetEcGdr(span,gdr) / pMaterial->GetEcSlab();
   }
   else
   {
      Float64 Ecgdr = pMaterialEx->GetEconc(fcgdr,
                                             pMaterial->GetStrDensityGdr(span,gdr),
                                             pMaterialEx->GetEccK1Gdr(span,gdr),
                                             pMaterialEx->GetEccK2Gdr(span,gdr));

      plldf->n     = Ecgdr / pMaterial->GetEcSlab();
   }

   // compute de (inside edge of barrier to CL of exterior web)
   Float64 wd = pGirder->GetCL2ExteriorWebDistance(poi); // cl beam to cl web

   // Note this is not exactly correct because opposite exterior beam might be different, but we won't be using this data set for that beam
   plldf->leftDe  = plldf->leftCurbOverhang - wd;  
   plldf->rightDe = plldf->rightCurbOverhang - wd; 

   plldf->L = GetEffectiveSpanLength(spanOrPier,gdr,dfType);

   Float64 Ix, Iy, A, Ip;
   if (fcgdr>0)
   {
      Ix = pSectProp2->GetIx(pgsTypes::BridgeSite3,poi,fcgdr);
      Iy = pSectProp2->GetIy(pgsTypes::BridgeSite3,poi,fcgdr);
      A  = pSectProp2->GetAg(pgsTypes::BridgeSite3,poi,fcgdr);
   }
   else
   {
      Ix = pSectProp2->GetIx(pgsTypes::BridgeSite3,poi);
      Iy = pSectProp2->GetIy(pgsTypes::BridgeSite3,poi);
      A  = pSectProp2->GetAg(pgsTypes::BridgeSite3,poi);
   }

   plldf->A = A;
   plldf->Yt    = pSectProp2->GetYtGirder(pgsTypes::BridgeSite1,poi);
   Ip = Ix + Iy;
   plldf->Ip = Ip;
   plldf->J = A*A*A*A/(40.0*Ip);

   plldf->Ag = pSectProp2->GetAg(pgsTypes::CastingYard,poi);
   plldf->Ig = pSectProp2->GetIx(pgsTypes::CastingYard,poi);

   // Assume slab thickness includes top flange
   Float64 ts = pBridge->GetStructuralSlabDepth(poi);
   Float64 tf =  pGirder->GetMinTopFlangeThickness(poi);
   plldf->ts = ts + tf;

   // location of cg of combined slab wrt top of girder
   Float64 tscg = (ts*ts/2.0 - tf*tf/2.0)/(ts+tf);
   plldf->eg    = plldf->Yt + tscg;

   WebIndexType nWebs = pGirder->GetNumberOfWebs(span,gdr);
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
      // AASHTO and WSDOT are the same
      if ( plldf->connectedAsUnit )
      {
         bool bSkew = !( IsZero(plldf->skew1) && IsZero(plldf->skew2) );

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
                                       bSkew,bSkew);
            
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
                                     plldf->skew2);

         plldf->Kg = -1.0; // doesnt apply here. set bogus value
      }
   }

   GET_IFACE(ILiveLoads,pLiveLoads);
   pLLDF->SetRangeOfApplicabilityAction( pLiveLoads->GetLldfRangeOfApplicabilityAction() );
   plldf->bExteriorGirder = pBridge->IsExteriorGirder(span,gdr);

   return pLLDF;
}

void CMultiWebDistFactorEngineer::ReportMoment(rptParagraph* pPara,MULTIWEB_LLDFDETAILS& lldf,lrfdILiveLoadDistributionFactor::DFResult& gM1,lrfdILiveLoadDistributionFactor::DFResult& gM2,double gM,bool bSIUnits,IEAFDisplayUnits* pDisplayUnits)
{
   std::_tstring strImagePath(pgsReportStyleHolder::GetImagePath());

   rptRcScalar scalar;
   scalar.SetFormat( sysNumericFormatTool::Fixed );
   scalar.SetWidth(6);
   scalar.SetPrecision(3);
   scalar.SetTolerance(1.0e-6);

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
         (lldf.Nl == 1 || gM1.mg >= gM2.mg) ? (*pPara) << Bold(_T(" < Controls")) << rptNewLine : (*pPara) << rptNewLine;
         if ( lldf.Nl >= 2 )
         {
            (*pPara) << _T("Corrected Factor: mg") << Super(_T("ME")) << Sub(_T("2+")) << _T(" = ") << scalar.SetValue(gM2.mg);
            (gM2.mg > gM1.mg) ? (*pPara) << Bold(_T(" < Controls")) << rptNewLine : (*pPara) << rptNewLine;
         }
      }
      else
      {
         if ( gM1.LeverRuleData.bWasUsed )
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
               ReportLeverRule(pPara,true,1.0,gM2.LeverRuleData,m_pBroker,pDisplayUnits);
            }

            if ( gM2.RigidData.bWasUsed )
            {
               (*pPara) << Bold(_T("2+ Loaded Lanes: Rigid Method")) << rptNewLine;
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
            (*pPara) << Bold(_T("Skew Correction")) << rptNewLine;
            Float64 skew_delta_max = ::ConvertToSysUnits( 10.0, unitMeasure::Degree );
            if ( fabs(lldf.skew1 - lldf.skew2) < skew_delta_max )
               (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("SkewCorrection_Moment_SI.png") : _T("SkewCorrection_Moment_US.png"))) << rptNewLine;

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
         else
         {
            (*pPara) << _T("Controlling mg") << Super(_T("ME"))  << _T(" = ") << scalar.SetValue(max(gM1.mg,gM2.mg)) << rptNewLine;
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
         (lldf.Nl == 1 || gM1.mg >= gM2.mg) ? (*pPara) << Bold(_T(" < Controls")) << rptNewLine : (*pPara) << rptNewLine;
         if ( lldf.Nl >= 2 )
         {
            (*pPara) << _T("Corrected Factor: mg") << Super(_T("ME")) << Sub(_T("2+")) << _T(" = ") << scalar.SetValue(gM2.mg);
            (gM2.mg > gM1.mg) ? (*pPara) << Bold(_T(" < Controls")) << rptNewLine : (*pPara) << rptNewLine;
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

            if ( lldf.Nl >= 2 )
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

            if ( lldf.Nl>=2 && gM2.LanesBeamsData.bWasUsed )
            {
               (*pPara) << Bold(_T("2+ Loaded Lane: Number of Lanes over Number of Beams - Factor cannot be less than this")) << rptNewLine;
               (*pPara) << _T("Skew correction is not applied to Lanes/Beams method")<< rptNewLine;
               ReportLanesBeamsMethod(pPara,gM2.LanesBeamsData,m_pBroker,pDisplayUnits);
            }
         }

         (*pPara) << rptNewLine;

         if (lldf.connectedAsUnit)
         {
            (*pPara) << Bold(_T("Skew Correction")) << rptNewLine;
            Float64 skew_delta_max = ::ConvertToSysUnits( 10.0, unitMeasure::Degree );
            if ( fabs(lldf.skew1 - lldf.skew2) < skew_delta_max )
               (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("SkewCorrection_Moment_SI.png") : _T("SkewCorrection_Moment_US.png"))) << rptNewLine;

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
         else
         {
            (*pPara) << _T("Controlling mg") << Super(_T("MI"))  << _T(" = ") << scalar.SetValue(max(gM1.mg,gM2.mg)) << rptNewLine;
         }
      }

      (*pPara) << rptNewLine;
   }
}

void CMultiWebDistFactorEngineer::ReportShear(rptParagraph* pPara,MULTIWEB_LLDFDETAILS& lldf,lrfdILiveLoadDistributionFactor::DFResult& gV1,lrfdILiveLoadDistributionFactor::DFResult& gV2,double gV,bool bSIUnits,IEAFDisplayUnits* pDisplayUnits)
{
   std::_tstring strImagePath(pgsReportStyleHolder::GetImagePath());

   rptRcScalar scalar;
   scalar.SetFormat( sysNumericFormatTool::Fixed );
   scalar.SetWidth(6);
   scalar.SetPrecision(3);
   scalar.SetTolerance(1.0e-6);

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
         (lldf.Nl == 1 || gV1.mg >= gV2.mg) ? (*pPara) << Bold(_T(" < Controls")) << rptNewLine : (*pPara) << rptNewLine;
         if ( lldf.Nl >= 2 )
         {
            (*pPara) << _T("Corrected Factor: mg") << Super(_T("VE")) << Sub(_T("2+")) << _T(" = ") << scalar.SetValue(gV2.mg);
            (gV2.mg > gV1.mg) ? (*pPara) << Bold(_T(" < Controls")) << rptNewLine : (*pPara) << rptNewLine;
         }

      }
      else
      {
         if ( gV1.LeverRuleData.bWasUsed )
         {
            (*pPara) << Bold(_T("1 Loaded Lane: Lever Rule")) << rptNewLine;
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

         if ( lldf.Nl >= 2 )
         {
            if ( gV2.EqnData.bWasUsed )
            {
               (*pPara) << Bold(_T("2+ Loaded Lanes: Spec Equation")) << rptNewLine;
               (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("mg_2_VE_Type_K_SI.png") : _T("mg_2_VE_Type_K_US.png"))) << rptNewLine;
               (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("mg_2_VI_Type_K_SI.png") : _T("mg_2_VI_Type_K_US.png"))) << rptNewLine;
               (*pPara) << _T("mg") << Super(_T("VI")) << Sub(_T("2+")) << _T(" = ") << scalar.SetValue(gV2.EqnData.mg) << rptNewLine;
               (*pPara) << _T("e = ") << scalar.SetValue(gV2.EqnData.e) << rptNewLine;

               (*pPara) << _T("mg") << Super(_T("VE")) << Sub(_T("2+")) << _T(" = ") << scalar.SetValue(gV2.EqnData.mg*gV2.EqnData.e) << rptNewLine;
            }

            if ( gV2.LeverRuleData.bWasUsed )
            {
               (*pPara) << Bold(_T("2+ Loaded Lanes: Lever Rule")) << rptNewLine;
               ReportLeverRule(pPara,false,1.0,gV2.LeverRuleData,m_pBroker,pDisplayUnits);
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

         (*pPara) << rptNewLine;
         if (lldf.connectedAsUnit)
         {
            (*pPara) << Bold(_T("Skew Correction")) << rptNewLine;
            (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("SkewCorrection_Shear_SI.png") : _T("SkewCorrection_Shear_US.png"))) << rptNewLine;
            (*pPara) << _T("Skew Correction Factor: = ") << scalar.SetValue(gV1.SkewCorrectionFactor) << rptNewLine;
            (*pPara) << rptNewLine;
            (*pPara) << _T("Skew Corrected Factor: mg") << Super(_T("VE")) << Sub(_T("1")) << _T(" = ") << scalar.SetValue(gV1.mg);
            (lldf.Nl == 1 || gV1.mg >= gV2.mg) ? (*pPara) << Bold(_T(" < Controls")) << rptNewLine : (*pPara) << rptNewLine;
            if ( lldf.Nl >= 2 )
            {
               (*pPara) << _T("Skew Corrected Factor: mg") << Super(_T("VE")) << Sub(_T("2+")) << _T(" = ") << scalar.SetValue(gV2.mg);
               (gV2.mg > gV1.mg) ? (*pPara) << Bold(_T(" < Controls")) << rptNewLine : (*pPara)  << rptNewLine;
            }
         }
         else
         {
            // skew makes no sense for unconnected
            (*pPara) << _T("Controlling Factor: mg") << Super(_T("VE")) << _T(" = ") << scalar.SetValue(max(gV1.mg,gV2.mg));
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
         (lldf.Nl == 1 || gV1.mg >= gV2.mg) ? (*pPara) << Bold(_T(" < Controls")) << rptNewLine : (*pPara) << rptNewLine;
         if ( lldf.Nl >= 2 )
         {
            (*pPara) << _T("Corrected Factor: mg") << Super(_T("VE")) << Sub(_T("2+")) << _T(" = ") << scalar.SetValue(gV2.mg);
            (gV2.mg > gV1.mg) ? (*pPara) << Bold(_T(" < Controls")) << rptNewLine : (*pPara) << rptNewLine;
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

         if ( lldf.Nl >= 2 )
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
            (*pPara) << Bold(_T("Skew Correction")) << rptNewLine << rptRcImage(strImagePath + (bSIUnits ? _T("SkewCorrection_Shear_SI.png") : _T("SkewCorrection_Shear_US.png"))) << rptNewLine;
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
         else
         {
            // skew makes no sense for unconnected
            (*pPara) << _T("Controlling Factor: mg") << Super(_T("VE")) << _T(" = ") << scalar.SetValue(max(gV1.mg,gV2.mg));
         }
      }
   }
}

std::_tstring CMultiWebDistFactorEngineer::GetComputationDescription(SpanIndexType span,GirderIndexType gdr,const std::_tstring& libraryEntryName,pgsTypes::SupportedDeckType decktype, pgsTypes::AdjacentTransverseConnectivity connect)
{
   GET_IFACE(ILibrary, pLib);
   GET_IFACE(ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

   Int16 lldfMethod = pSpecEntry->GetLiveLoadDistributionMethod();

   std::_tostringstream osdescr;

   if ( lldfMethod == LLDF_TXDOT )
   {
      osdescr <<_T("TxDOT modifications. Treat as AASHTO Type (i,j) connected only enough to prevent relative vertical displacement, regardless of deck or connectivity input. (K=")<< this->GetTxDOTKfactor() << _T(", S/10 max). Effects of skew will be ignored.");
   }
   else if ( lldfMethod == LLDF_WSDOT || lldfMethod == LLDF_LRFD )
   {
      osdescr << _T("AASHTO LRFD Method per Article 4.6.2.2. Using type (i,j) cross section ");

      if (connect == pgsTypes::atcConnectedAsUnit)
         osdescr << _T("connected transversely sufficiently to act as a unit.");
      else
         osdescr << _T("connected transversely only enough to prevent relative vertical displacement along interface.");
   }
   else
   {
      ATLASSERT(0);
   }

   // Special text if ROA is ignored
   GET_IFACE(ILiveLoads,pLiveLoads);
   std::_tstring straction = pLiveLoads->GetLLDFSpecialActionText();
   if ( !straction.empty() )
   {
      osdescr << straction;
   }

   return osdescr.str();
}
