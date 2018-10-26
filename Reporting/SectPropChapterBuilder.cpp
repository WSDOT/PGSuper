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

#include "StdAfx.h"
#include <Reporting\SectPropChapterBuilder.h>
#include <Reporting\SectPropTable.h>
#include <Reporting\SectPropTable2.h>

#include <PgsExt\BridgeDescription.h>
#include <PgsExt\PointOfInterest.h>

#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <EAF\EAFDisplayUnits.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CSectPropChapterBuilder
****************************************************************************/



////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CSectPropChapterBuilder::CSectPropChapterBuilder(bool bSelect,bool simplifiedVersion) :
CPGSuperChapterBuilder(bSelect),
m_SimplifiedVersion(simplifiedVersion)
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CSectPropChapterBuilder::GetName() const
{
   return TEXT("Section Properties");
}

rptChapter* CSectPropChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CSpanGirderReportSpecification* pSGRptSpec = dynamic_cast<CSpanGirderReportSpecification*>(pRptSpec);
   CGirderReportSpecification* pGdrRptSpec    = dynamic_cast<CGirderReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   SpanIndexType span;
   GirderIndexType gdr;

   if ( pSGRptSpec )
   {
      pSGRptSpec->GetBroker(&pBroker);
      span = pSGRptSpec->GetSpan();
      gdr = pSGRptSpec->GetGirder();
   }
   else if ( pGdrRptSpec )
   {
      pGdrRptSpec->GetBroker(&pBroker);
      span = ALL_SPANS;
      gdr = pGdrRptSpec->GetGirder();
   }
   else
   {
      span = ALL_SPANS;
      gdr  = ALL_GIRDERS;
   }

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   

   rptParagraph* pPara = new rptParagraph();
   *pChapter << pPara;

   GET_IFACE2(pBroker,ISectProp2,pSectProp2);
   GET_IFACE2(pBroker,IGirder,pGirder);
   GET_IFACE2(pBroker,IBarriers,pBarriers);
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2( pBroker, IBridgeMaterial, pMaterial );

   INIT_UV_PROTOTYPE( rptAreaUnitValue, l2, pDisplayUnits->GetAreaUnit(), false );
   INIT_UV_PROTOTYPE( rptLength4UnitValue, ui, pDisplayUnits->GetMomentOfInertiaUnit(), true );
   INIT_UV_PROTOTYPE( rptForceLength2UnitValue, uei, pDisplayUnits->GetStiffnessUnit(), true );
   INIT_UV_PROTOTYPE( rptStressUnitValue, modE, pDisplayUnits->GetModEUnit(), true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,    dim,    pDisplayUnits->GetComponentDimUnit(),    true );
   INIT_UV_PROTOTYPE( rptForcePerLengthUnitValue, fpl, pDisplayUnits->GetForcePerLengthUnit(), true );

   bool bComposite = pBridge->IsCompositeDeck();

   if (!m_SimplifiedVersion)
   {
      // Write out traffic barrier properties
      pPara = new rptParagraph();
      *pChapter << pPara;
      l2.ShowUnitTag( true );
      (*pPara) << _T("Left Traffic Barrier Area = ") << l2.SetValue( pBarriers->GetAtb(pgsTypes::tboLeft) ) << rptNewLine;
      (*pPara) << _T("Left Traffic Barrier ") << Sub2(_T("I"),_T("yy")) << _T(" = ") << ui.SetValue( pBarriers->GetItb(pgsTypes::tboLeft) ) << rptNewLine;
      (*pPara) << _T("Left Traffic Barrier ") << Sub2(_T("Y"),_T("b")) << _T(" = ") << dim.SetValue( pBarriers->GetYbtb(pgsTypes::tboLeft) ) << rptNewLine;

      GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
      const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

      if ( pBridgeDesc->GetLeftRailingSystem()->GetExteriorRailing()->GetWeightMethod() == TrafficBarrierEntry::Compute )
         (*pPara) << _T("Left Traffic Barrier Weight (computed from area) = ")<<fpl.SetValue( pBarriers->GetBarrierWeight(pgsTypes::tboLeft) ) << rptNewLine;
      else
         (*pPara) << _T("Left Traffic Barrier Weight = ")<<fpl.SetValue( pBarriers->GetBarrierWeight(pgsTypes::tboLeft) ) << rptNewLine;

      *pPara << rptNewLine;

      pPara = new rptParagraph();
      *pChapter << pPara;
      l2.ShowUnitTag( true );
      (*pPara) << _T("Right Traffic Barrier Area = ") << l2.SetValue( pBarriers->GetAtb(pgsTypes::tboRight) ) << rptNewLine;
      (*pPara) << _T("Right Traffic Barrier ") << Sub2(_T("I"),_T("yy")) << _T(" = ") << ui.SetValue( pBarriers->GetItb(pgsTypes::tboRight) ) << rptNewLine;
      (*pPara) << _T("Right Traffic Barrier ") << Sub2(_T("Y"),_T("b")) << _T(" = ") << dim.SetValue( pBarriers->GetYbtb(pgsTypes::tboRight) ) << rptNewLine;

      if ( pBridgeDesc->GetRightRailingSystem()->GetExteriorRailing()->GetWeightMethod() == TrafficBarrierEntry::Compute )
         (*pPara) << _T("Right Traffic Barrier Weight (computed from area) = ")<<fpl.SetValue( pBarriers->GetBarrierWeight(pgsTypes::tboRight) ) << rptNewLine;
      else
         (*pPara) << _T("Right Traffic Barrier Weight = ")<<fpl.SetValue( pBarriers->GetBarrierWeight(pgsTypes::tboRight) ) << rptNewLine;

      *pPara << rptNewLine;

      if ( bComposite )
        (*pPara) << _T("Slab   ") << RPT_EC << _T(" = ") << modE.SetValue( pMaterial->GetEcSlab() ) << rptNewLine;
   }

   SpanIndexType nSpans = pBridge->GetSpanCount();
   SpanIndexType firstSpanIdx = (span == ALL_SPANS ? 0 : span);
   SpanIndexType lastSpanIdx  = (span == ALL_SPANS ? nSpans : firstSpanIdx+1);

   for ( SpanIndexType spanIdx = firstSpanIdx; spanIdx < lastSpanIdx; spanIdx++ )
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(spanIdx);
      GirderIndexType firstGirderIdx = min(nGirders-1,(gdr == ALL_GIRDERS ? 0 : gdr));
      GirderIndexType lastGirderIdx  = min(nGirders,  (gdr == ALL_GIRDERS ? nGirders : firstGirderIdx + 1));
      
      for ( GirderIndexType gdrIdx = firstGirderIdx; gdrIdx < lastGirderIdx; gdrIdx++ )
      {
         if (!m_SimplifiedVersion)
         {
            pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
            *pChapter << pPara;
            (*pPara) << _T("Span ") << LABEL_SPAN(spanIdx) << _T(" Girder ") << LABEL_GIRDER(gdrIdx) << rptNewLine;
         }

         pPara = new rptParagraph();
         *pChapter << pPara;

         if (!m_SimplifiedVersion)
         {
            (*pPara) << _T("Girder ") << RPT_EC << _T(" = ") << modE.SetValue( pMaterial->GetEcGdr(spanIdx,gdrIdx) ) << rptNewLine;
            (*pPara) << _T("Girder ") << RPT_ECI << _T(" = ") << modE.SetValue( pMaterial->GetEciGdr(spanIdx,gdrIdx) ) << rptNewLine;


            Float64 span_length = pBridge->GetSpanLength(spanIdx,gdrIdx);
            (*pPara) << _T("Bending Stiffness of Entire Bridge Section at mid-span") << rptNewLine;
            (*pPara) << Sub2(_T("EI"),_T("xx")) << _T(" = ") << uei.SetValue( pSectProp2->GetBridgeEIxx(span_length/2) ) << _T(" (used to compute Live Load Deflections per LRFD 3.6.1.3.2)") << rptNewLine;
            (*pPara) << Sub2(_T("EI"),_T("yy")) << _T(" = ") << uei.SetValue( pSectProp2->GetBridgeEIyy(span_length/2) ) << rptNewLine;
            *pPara << rptNewLine;
         }

         bool bIsPrismatic_CastingYard = pGirder->IsPrismatic(pgsTypes::CastingYard,spanIdx,gdrIdx);
         bool bIsPrismatic_Final       = pGirder->IsPrismatic(pgsTypes::BridgeSite3,spanIdx,gdrIdx);


         if ( bIsPrismatic_CastingYard && bIsPrismatic_Final )
         {
            // simple table
            rptRcTable* pTable = CSectionPropertiesTable().Build(pBroker,spanIdx,gdrIdx,bComposite,pDisplayUnits);
            *pPara << pTable << rptNewLine;
         }
         else if ( bIsPrismatic_CastingYard && !bIsPrismatic_Final )
         {
            // simple table for bare girder (don't report composite)
            rptRcTable* pTable = CSectionPropertiesTable().Build(pBroker,spanIdx,gdrIdx,false,pDisplayUnits);
            *pPara << pTable << rptNewLine;

            if ( bComposite )
            {
               // there is a deck so we have composite, non-prismatic results
               pTable = CSectionPropertiesTable2().Build(pBroker,spanIdx,gdrIdx,pgsTypes::BridgeSite3,pDisplayUnits);
               *pPara << pTable << rptNewLine;
            }
         }
         else if ( !bIsPrismatic_CastingYard && !bIsPrismatic_Final )
         {
            rptRcTable* pTable = CSectionPropertiesTable2().Build(pBroker,spanIdx,gdrIdx,pgsTypes::CastingYard,pDisplayUnits);
            *pPara << pTable << rptNewLine;

            if ( pBridge->GetDeckType() != pgsTypes::sdtNone ) // if there isn't a deck, no need to report duplicate properties
            {
               pTable = CSectionPropertiesTable2().Build(pBroker,spanIdx,gdrIdx,pgsTypes::BridgeSite3,pDisplayUnits);
               *pPara << pTable << rptNewLine;
            }
         }
         else if ( !bIsPrismatic_CastingYard && bIsPrismatic_Final )
         {
            ATLASSERT(false); // this is an impossible case
         }
      } // gdrIdx
   } // spanIdx

   return pChapter;
}

CChapterBuilder* CSectPropChapterBuilder::Clone() const
{
   return new CSectPropChapterBuilder;
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
