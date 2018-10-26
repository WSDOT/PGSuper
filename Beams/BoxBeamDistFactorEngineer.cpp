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

// BoxBeamDistFactorEngineer.cpp : Implementation of CBoxBeamDistFactorEngineer
#include "stdafx.h"
#include "BoxBeamDistFactorEngineer.h"
#include <WBFLCore.h>
#include <Units\SysUnits.h>
#include <PsgLib\TrafficBarrierEntry.h>
#include <PsgLib\SpecLibraryEntry.h>
#include <PgsExt\BridgeDescription.h>
#include <Reporting\ReportStyleHolder.h>
#include <PgsExt\StatusItem.h>
#include <PgsExt\GirderLabel.h>
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\DistributionFactors.h>
#include <IFace\StatusCenter.h>
#include <WBFLCogo.h>
#include <LRFD\LldfTypeG.h>
#include "helper.h"

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

void CBoxBeamDistFactorEngineer::BuildReport(SpanIndexType span,GirderIndexType gdr,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits)
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

   rptParagraph* pPara;

   bool bSIUnits = IS_SI_UNITS(pDisplayUnits);
   std::_tstring strImagePath(pgsReportStyleHolder::GetImagePath());

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

   pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
   (*pPara) << _T("Method of Computation:")<<rptNewLine;
   (*pChapter) << pPara;
   pPara = new rptParagraph;
   (*pChapter) << pPara;
   std::_tstring strGirderName = pSpan->GetGirderTypes()->GetGirderName(gdr);
   (*pPara) << GetComputationDescription(span,gdr, 
                                         strGirderName, 
                                         pDeck->DeckType,
                                         pDeck->TransverseConnectivity);

   pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
   (*pChapter) << pPara;
   (*pPara) << _T("Distribution Factor Parameters") << rptNewLine;
   pPara = new rptParagraph;
   (*pChapter) << pPara;

   (*pPara) << _T("Girder Spacing: ") << Sub2(_T("S"),_T("avg")) << _T(" = ") << xdim.SetValue(span_lldf.Savg) << rptNewLine;
   Float64 station,offset;
   pBridge->GetStationAndOffset(pgsPointOfInterest(span,gdr,span_lldf.ControllingLocation),&station, &offset);
   Float64 supp_dist = span_lldf.ControllingLocation - pBridge->GetGirderStartConnectionLength(span,gdr);
   (*pPara) << _T("Measurement of Girder Spacing taken at ") << location.SetValue(supp_dist)<< _T(" from left support, measured along girder, or station = ")<< rptRcStation(station, &pDisplayUnits->GetStationFormat() ) << rptNewLine;
   (*pPara) << _T("Deck Width: W = ") << xdim.SetValue(span_lldf.W) << rptNewLine;
   (*pPara) << _T("Moment of Inertia: I = ") << inertia.SetValue(span_lldf.I) << rptNewLine;
   (*pPara) << _T("Lane Width: wLane = ") << xdim.SetValue(span_lldf.wLane) << rptNewLine;
   (*pPara) << _T("Beam Width: b = ") << xdim2.SetValue(span_lldf.b) << rptNewLine;
   (*pPara) << _T("Beam Depth: d = ") << xdim2.SetValue(span_lldf.d) << rptNewLine;
   Float64 de = span_lldf.Side==dfLeft ? span_lldf.leftDe:span_lldf.rightDe;
   (*pPara) << _T("Distance from exterior web of exterior beam to curb line: d") << Sub(_T("e")) << _T(" = ") << xdim.SetValue(de) << rptNewLine;
   (*pPara) << _T("Possion Ratio: ") << symbol(mu) << _T(" = ") << span_lldf.PossionRatio << rptNewLine;
   (*pPara) << _T("Skew Angle at start: ") << symbol(theta) << _T(" = ") << angle.SetValue(fabs(span_lldf.skew1)) << rptNewLine;
   (*pPara) << _T("Skew Angle at end: ") << symbol(theta) << _T(" = ") << angle.SetValue(fabs(span_lldf.skew2)) << rptNewLine;
   (*pPara) << _T("Number of Design Lanes: N") << Sub(_T("L")) << _T(" = ") << span_lldf.Nl << rptNewLine;
   (*pPara) << _T("Number of Girders: N") << Sub(_T("b")) << _T(" = ") << span_lldf.Nb << rptNewLine;

   if (pBridgeDesc->GetDistributionFactorMethod() != pgsTypes::LeverRule)
   {
      pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
      (*pPara) << _T("St. Venant torsional inertia constant");
      (*pChapter) << pPara;
      pPara = new rptParagraph;
      (*pChapter) << pPara;

      (*pPara) << _T("St. Venant torsional inertia constant: J = ") << inertia.SetValue(span_lldf.J) << rptNewLine;

      (*pPara) << rptRcImage(strImagePath + _T("J_closed_thin_wall.png")) << rptNewLine;
      (*pPara) << rptRcImage(strImagePath + _T("BoxBeam_TorsionalConstant.gif")) << rptNewLine;
      (*pPara) << _T("Area enclosed by centerlines of elements: ") << Sub2(_T("A"),_T("o")) << _T(" = ") << area.SetValue(span_lldf.Jvoid.Ao) << rptNewLine;

      rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(3,_T(""));
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

void CBoxBeamDistFactorEngineer::ReportMoment(rptParagraph* pPara,BOXBEAM_LLDFDETAILS& lldf,lrfdILiveLoadDistributionFactor::DFResult& gM1,lrfdILiveLoadDistributionFactor::DFResult& gM2,double gM,bool bSIUnits,IEAFDisplayUnits* pDisplayUnits)
{
   GET_IFACE(IBridge,pBridge);
   std::_tstring strImagePath(pgsReportStyleHolder::GetImagePath());

   INIT_UV_PROTOTYPE( rptLengthUnitValue,    xdim,     pDisplayUnits->GetSpanLengthUnit(),      true );

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
      // Distribution factor for exterior girder
      if( gM1.EqnData.bWasUsed )
      {
         (*pPara) << Bold(_T("1 Loaded Lane - Equation")) << rptNewLine;
         (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("mg_1_ME_Type_G_SI.png") : _T("mg_1_ME_Type_G_US.png"))) << rptNewLine;
 
         if ( lldf.connectedAsUnit )
         {
            (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("mg_1_MI_Type_F_SI.png") : _T("mg_1_MI_Type_F_US.png"))) << rptNewLine;
         }
         else
         {
            (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("mg_1_MI_Type_G_SI.png") : _T("mg_1_MI_Type_G_US.png"))) << rptNewLine;
            ATLASSERT(gM1.ControllingMethod&S_OVER_D_METHOD);
            (*pPara)<< _T("K = ")<< gM1.EqnData.K << rptNewLine;
            (*pPara)<< _T("C = ")<< gM1.EqnData.C << rptNewLine;
            (*pPara)<< _T("D = ")<< xdim.SetValue(gM1.EqnData.D) << rptNewLine;
            (*pPara) << rptNewLine;
         }

         (*pPara) << _T("mg") << Super(_T("MI")) << Sub(_T("1")) << _T(" = ") << scalar.SetValue(gM1.EqnData.mg) << rptNewLine;
         (*pPara) << _T("e = ") << scalar.SetValue(gM1.EqnData.e) << rptNewLine;
         (*pPara) << _T("mg") << Super(_T("ME")) << Sub(_T("1")) << _T(" = ") << scalar.SetValue(gM1.EqnData.mg*gM1.EqnData.e) << rptNewLine;

         if (gM1.ControllingMethod & INTERIOR_OVERRIDE)
         {
            (*pPara) << _T("For TxDOT method, exterior mg cannot be less than interior - Interior mg Controls")<< rptNewLine;
            (*pPara) << _T("mg") << Super(_T("ME")) << Sub(_T("1")) << _T(" = ") << scalar.SetValue(gM1.mg/gM1.SkewCorrectionFactor) << rptNewLine;
         }
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

         if( gM2.EqnData.bWasUsed )
         {
            (*pPara) << Bold(_T("2+ Loaded Lane: Equation")) << rptNewLine;
            (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("mg_2_ME_Type_G_SI.png") : _T("mg_2_ME_Type_G_US.png"))) << rptNewLine;

            if ( lldf.connectedAsUnit )
            {
               (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("mg_2_MI_Type_F_SI.png") : _T("mg_2_MI_Type_F_US.png"))) << rptNewLine;
            }
            else
            {
               (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("mg_2_MI_Type_G_SI.png") : _T("mg_2_MI_Type_G_US.png"))) << rptNewLine;
               ATLASSERT(gM2.ControllingMethod&S_OVER_D_METHOD);
               (*pPara)<< _T("K = ")<< gM2.EqnData.K << rptNewLine;
               (*pPara)<< _T("C = ")<< gM2.EqnData.C << rptNewLine;
               (*pPara)<< _T("D = ")<< xdim.SetValue(gM2.EqnData.D) << rptNewLine;
               (*pPara) << rptNewLine;
            }

            (*pPara) << _T("mg") << Super(_T("MI")) << Sub(_T("2+")) << _T(" = ") << scalar.SetValue(gM2.EqnData.mg) << rptNewLine;
            (*pPara) << _T("e = ") << scalar.SetValue(gM2.EqnData.e) << rptNewLine;
            (*pPara) << _T("mg") << Super(_T("ME")) << Sub(_T("2+")) << _T(" = ") << scalar.SetValue(gM2.EqnData.mg*gM2.EqnData.e) << rptNewLine;

            if (gM2.ControllingMethod & INTERIOR_OVERRIDE)
            {
               (*pPara) << _T("For TxDOT method, exterior mg cannot be less than interior - Interior mg Controls")<< rptNewLine;
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

      (*pPara) << Bold(_T("Skew Correction")) << rptNewLine;
      if (lldf.Method==LLDF_TXDOT)
      {
         (*pPara) << _T("For TxDOT specification, we ignore skew correction, so:") << rptNewLine;
      }
      else
      {
         Float64 skew_delta_max = ::ConvertToSysUnits( 10.0, unitMeasure::Degree );
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
   else
   {
      // Interior Girder
      if ( gM1.EqnData.bWasUsed )
      {
         (*pPara) << Bold(_T("1 Loaded Lane: Equations")) << rptNewLine;
         if ( lldf.connectedAsUnit )
         {
            (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("mg_1_MI_Type_F_SI.png") : _T("mg_1_MI_Type_F_US.png"))) << rptNewLine;
         }
         else
         {
            (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("mg_1_MI_Type_G_SI.png") : _T("mg_1_MI_Type_G_US.png"))) << rptNewLine;
            ATLASSERT(gM1.ControllingMethod&S_OVER_D_METHOD);
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
            if ( lldf.connectedAsUnit )
            {
               (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("mg_2_MI_Type_F_SI.png") : _T("mg_2_MI_Type_F_US.png"))) << rptNewLine;
            }
            else
            {
               (*pPara) << rptRcImage(strImagePath + (bSIUnits ? _T("mg_2_MI_Type_G_SI.png") : _T("mg_2_MI_Type_G_US.png"))) << rptNewLine;
               ATLASSERT(gM2.ControllingMethod&S_OVER_D_METHOD);
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

      (*pPara) << Bold(_T("Skew Correction")) << rptNewLine;
      if (lldf.Method==LLDF_TXDOT)
      {
         (*pPara) << _T("For TxDOT specification, we ignore skew correction, so:") << rptNewLine;
      }
      else
      {
         Float64 skew_delta_max = ::ConvertToSysUnits( 10.0, unitMeasure::Degree );
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

void CBoxBeamDistFactorEngineer::ReportShear(rptParagraph* pPara,BOXBEAM_LLDFDETAILS& lldf,lrfdILiveLoadDistributionFactor::DFResult& gV1,lrfdILiveLoadDistributionFactor::DFResult& gV2,double gV,bool bSIUnits,IEAFDisplayUnits* pDisplayUnits)
{
   GET_IFACE(IBridge,pBridge);
   std::_tstring strImagePath(pgsReportStyleHolder::GetImagePath());

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
         if (!(gV1.ControllingMethod & MOMENT_OVERRIDE))
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

      if (gV1.ControllingMethod & INTERIOR_OVERRIDE)
      {
         (*pPara)<< rptNewLine << _T("For TxDOT method, exterior mg cannot be less than interior - Interior mg Controls")<< rptNewLine;
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

            if (!(gV2.ControllingMethod & MOMENT_OVERRIDE))
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

         if (gV2.ControllingMethod & INTERIOR_OVERRIDE)
         {
            (*pPara) << rptNewLine << _T("For TxDOT method, exterior mg cannot be less than interior - Interior mg Controls")<< rptNewLine;
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
   else
   {
      // Interior Girder
      //
      // Shear
      //
      if ( gV1.ControllingMethod &  MOMENT_OVERRIDE  )
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

lrfdLiveLoadDistributionFactorBase* CBoxBeamDistFactorEngineer::GetLLDFParameters(SpanIndexType spanOrPier,GirderIndexType gdr,DFParam dfType,Float64 fcgdr,BOXBEAM_LLDFDETAILS* plldf)
{
   GET_IFACE(ISectProp2, pSectProp2);
   GET_IFACE(ILibrary, pLib);
   GET_IFACE(ISpecification, pSpec);
   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IGirder,pGirder);
   GET_IFACE(IEAFStatusCenter,pStatusCenter);
   GET_IFACE(IBarriers,pBarriers);
   GET_IFACE(IPointOfInterest,pPOI);
   GET_IFACE(ILiveLoads,pLiveLoads);

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CDeckDescription* pDeck = pBridgeDesc->GetDeckDescription();

   ATLASSERT(pDeck->DeckType==pgsTypes::sdtCompositeOverlay ||pDeck->DeckType==pgsTypes::sdtNone);
   ATLASSERT(pBridgeDesc->GetDistributionFactorMethod() != pgsTypes::DirectlyInput);

   // Determine span/pier index... This is the index of a pier and the next span.
   // If this is the last pier, span index is for the last span
   SpanIndexType span = INVALID_INDEX;
   PierIndexType pier = INVALID_INDEX;
   SpanIndexType prev_span = INVALID_INDEX;
   SpanIndexType next_span = INVALID_INDEX;
   PierIndexType prev_pier = INVALID_INDEX;
   PierIndexType next_pier = INVALID_INDEX;
   GetIndicies(spanOrPier,dfType,span,pier,prev_span,next_span,prev_pier,next_pier);
   
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

   // Get some controlling dimensions
   // get the maximum width of this girder
   Float64 top_width = pGirder->GetTopWidth(poi);
   Float64 bot_width = pGirder->GetBottomWidth(poi);

   Float64 width = _cpp_max(top_width,bot_width);

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

   plldf->L = GetEffectiveSpanLength(spanOrPier,gdr,dfType);

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

   if (is_composite)
   {
      // We have a composite section. 
      Float64 eff_wid = pSectProp2->GetEffectiveFlangeWidth(poi);
      if ( fcgdr < 0 )
      {
         plldf->I  = pSectProp2->GetIx(pgsTypes::BridgeSite3,poi);
      }
      else
      {
         plldf->I  = pSectProp2->GetIx(pgsTypes::BridgeSite3,poi,fcgdr);
      }

      Float64 t_top = top_flg_thk;
      Float64 slab_depth = pBridge->GetStructuralSlabDepth(poi);

      t_top += slab_depth;

      Float64 t_bot = bot_flg_thk;

      // s 
      Float64 s_top  = inner_width + t_ext;
      Float64 s_side = inner_height + (t_top + t_bot)/2;

      Jvoid.Elements.push_back(BOXBEAM_J_VOID::Element(s_top,t_top)); // top
      Jvoid.Elements.push_back(BOXBEAM_J_VOID::Element(s_top,t_bot)); // bottom
      Jvoid.Elements.push_back(BOXBEAM_J_VOID::Element(s_side,t_ext)); // left
      Jvoid.Elements.push_back(BOXBEAM_J_VOID::Element(s_side,t_ext)); // right

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
      plldf->I  = pSectProp2->GetIx(pgsTypes::BridgeSite3,poi);

      Float64 t_top = top_flg_thk;
      Float64 t_bot = bot_flg_thk;

      // s 
      Float64 s_top  = inner_width + t_ext;
      Float64 s_side = inner_height + (t_top + t_bot)/2;

      Jvoid.Elements.push_back(BOXBEAM_J_VOID::Element(s_top,t_top)); // top
      Jvoid.Elements.push_back(BOXBEAM_J_VOID::Element(s_top,t_bot)); // bottom
      Jvoid.Elements.push_back(BOXBEAM_J_VOID::Element(s_side,t_ext)); // left
      Jvoid.Elements.push_back(BOXBEAM_J_VOID::Element(s_side,t_ext)); // right

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
   lrfdLiveLoadDistributionFactorBase* pLLDF;

   if (plldf->Method==LLDF_TXDOT)
   {
      plldf->connectedAsUnit = true;

      lrfdTxdotLldfAdjacentBox* pTypeF = new lrfdTxdotLldfAdjacentBox(
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
   else if ( plldf->connectedAsUnit)
   {
      lrfdLldfTypeF* pTypeF = new lrfdLldfTypeF(
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
                            false,
                            plldf->skew1, 
                            plldf->skew2);

      pLLDF = pTypeF;
   }
   else
   {
      lrfdLldfTypeG* pTypeG = new lrfdLldfTypeG(
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
                            false,
                            plldf->skew1, 
                            plldf->skew2);

      pLLDF = pTypeG;
   }

   pLLDF->SetRangeOfApplicabilityAction( pLiveLoads->GetLldfRangeOfApplicabilityAction() );

   return pLLDF;
}

std::_tstring CBoxBeamDistFactorEngineer::GetComputationDescription(SpanIndexType span,GirderIndexType gdr,const std::_tstring& libraryEntryName,pgsTypes::SupportedDeckType decktype, pgsTypes::AdjacentTransverseConnectivity connect)
{
   GET_IFACE(ILibrary, pLib);
   GET_IFACE(ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

   Int16 lldfMethod = pSpecEntry->GetLiveLoadDistributionMethod();

   std::_tstring descr;

   if ( lldfMethod == LLDF_TXDOT )
   {
      descr = _T("TxDOT modifications. Treat as AASHTO Type (f,g) connected transversely sufficiently to act as a unit, regardless of deck or connectivity input. Also, do not apply skew correction factor for moment.");
   }
   else
   {
      descr = _T("AASHTO LRFD Method per Article 4.6.2.2.");

      if ( decktype == pgsTypes::sdtCompositeOverlay)
         descr += std::_tstring(_T(" type (f) cross section"));
      else if (decktype == pgsTypes::sdtNone)
         descr += std::_tstring(_T(" type (g) cross section"));
      else
         ATLASSERT(0);

      if (connect == pgsTypes::atcConnectedAsUnit)
         descr += std::_tstring(_T(" connected transversely sufficiently to act as a unit."));
      else
         descr += std::_tstring(_T(" connected transversely only enough to prevent relative vertical displacement along interface."));

   }

   return descr;
}

