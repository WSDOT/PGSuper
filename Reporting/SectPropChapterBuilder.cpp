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
#include <IFace\DisplayUnits.h>

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
CSectPropChapterBuilder::CSectPropChapterBuilder()
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
   CComPtr<IBroker> pBroker;
   pSGRptSpec->GetBroker(&pBroker);
   SpanIndexType span = pSGRptSpec->GetSpan();
   GirderIndexType girder = pSGRptSpec->GetGirder();

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   GET_IFACE2(pBroker,IDisplayUnits,pDisplayUnits);
   

   rptParagraph* pPara = new rptParagraph();
   *pChapter << pPara;

   GET_IFACE2(pBroker,ISectProp2,pSectProp2);
   GET_IFACE2(pBroker,IGirder,pGirder);
   GET_IFACE2(pBroker,IBarriers,pBarriers);
   GET_IFACE2(pBroker,IBridge,pBridge);

   INIT_UV_PROTOTYPE( rptAreaUnitValue, l2, pDisplayUnits->GetAreaUnit(), false );
   INIT_UV_PROTOTYPE( rptLength4UnitValue, ui, pDisplayUnits->GetMomentOfInertiaUnit(), true );
   INIT_UV_PROTOTYPE( rptForceLength2UnitValue, uei, pDisplayUnits->GetStiffnessUnit(), true );
   INIT_UV_PROTOTYPE( rptStressUnitValue, modE, pDisplayUnits->GetModEUnit(), true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,    dim,    pDisplayUnits->GetComponentDimUnit(),    true );
   INIT_UV_PROTOTYPE( rptForcePerLengthUnitValue, fpl, pDisplayUnits->GetForcePerLengthUnit(), true );

   GET_IFACE2( pBroker, IBridgeMaterial, pMaterial );
   (*pPara) << "Girder " << RPT_EC << " = " << modE.SetValue( pMaterial->GetEcGdr(span,girder) ) << rptNewLine;
   (*pPara) << "Girder " << RPT_ECI << " = " << modE.SetValue( pMaterial->GetEciGdr(span,girder) ) << rptNewLine;

   if ( pBridge->IsCompositeDeck() )
     (*pPara) << "Slab   " << RPT_EC << " = " << modE.SetValue( pMaterial->GetEcSlab() ) << rptNewLine;

   *pPara << rptNewLine;

   // Write out traffic barrier properties
   pPara = new rptParagraph();
   *pChapter << pPara;
   l2.ShowUnitTag( true );
   (*pPara) << "Left Traffic Barrier Area = " << l2.SetValue( pBarriers->GetAtb(pgsTypes::tboLeft) ) << rptNewLine;
   (*pPara) << "Left Traffic Barrier " << Sub2("I","yy") << " = " << ui.SetValue( pBarriers->GetItb(pgsTypes::tboLeft) ) << rptNewLine;
   (*pPara) << "Left Traffic Barrier " << Sub2("Y","b") << " = " << dim.SetValue( pBarriers->GetYbtb(pgsTypes::tboLeft) ) << rptNewLine;

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   if ( pBridgeDesc->GetLeftRailingSystem()->GetExteriorRailing()->GetWeightMethod() == TrafficBarrierEntry::Compute )
      (*pPara) << "Left Traffic Barrier Weight (computed from area) = "<<fpl.SetValue( pBarriers->GetBarrierWeight(pgsTypes::tboLeft) ) << rptNewLine;
   else
      (*pPara) << "Left Traffic Barrier Weight = "<<fpl.SetValue( pBarriers->GetBarrierWeight(pgsTypes::tboLeft) ) << rptNewLine;

   *pPara << rptNewLine;

   pPara = new rptParagraph();
   *pChapter << pPara;
   l2.ShowUnitTag( true );
   (*pPara) << "Right Traffic Barrier Area = " << l2.SetValue( pBarriers->GetAtb(pgsTypes::tboRight) ) << rptNewLine;
   (*pPara) << "Right Traffic Barrier " << Sub2("I","yy") << " = " << ui.SetValue( pBarriers->GetItb(pgsTypes::tboRight) ) << rptNewLine;
   (*pPara) << "Right Traffic Barrier " << Sub2("Y","b") << " = " << dim.SetValue( pBarriers->GetYbtb(pgsTypes::tboRight) ) << rptNewLine;

   if ( pBridgeDesc->GetRightRailingSystem()->GetExteriorRailing()->GetWeightMethod() == TrafficBarrierEntry::Compute )
      (*pPara) << "Right Traffic Barrier Weight (computed from area) = "<<fpl.SetValue( pBarriers->GetBarrierWeight(pgsTypes::tboRight) ) << rptNewLine;
   else
      (*pPara) << "Right Traffic Barrier Weight = "<<fpl.SetValue( pBarriers->GetBarrierWeight(pgsTypes::tboRight) ) << rptNewLine;

   *pPara << rptNewLine;

   double span_length = pBridge->GetSpanLength(span,girder);
   (*pPara) << "Bending Stiffness of Entire Bridge Section at mid-span" << rptNewLine;
   (*pPara) << Sub2("EI","xx") << " = " << uei.SetValue( pSectProp2->GetBridgeEIxx(span_length/2) ) << " (used to compute Live Load Deflections per LRFD 3.6.1.3.2)" << rptNewLine;
   (*pPara) << Sub2("EI","yy") << " = " << uei.SetValue( pSectProp2->GetBridgeEIyy(span_length/2) ) << rptNewLine;
   *pPara << rptNewLine;


   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();

   bool bIsPrismatic_CastingYard = pGirder->IsPrismatic(pgsTypes::CastingYard,span,girder);
   bool bIsPrismatic_Final       = pGirder->IsPrismatic(pgsTypes::BridgeSite3,span,girder);

   bool bComposite = (deckType != pgsTypes::sdtNone);

   if ( bIsPrismatic_CastingYard && bIsPrismatic_Final )
   {
      // simple table
      rptRcTable* pTable = CSectionPropertiesTable().Build(pBroker,span,girder,bComposite,pDisplayUnits);
      *pPara << pTable << rptNewLine;
   }
   else if ( bIsPrismatic_CastingYard && !bIsPrismatic_Final )
   {
      // simple table for bare girder (don't report composite)
      rptRcTable* pTable = CSectionPropertiesTable().Build(pBroker,span,girder,false,pDisplayUnits);
      *pPara << pTable << rptNewLine;

      if ( bComposite )
      {
         // there is a deck so we have composite, non-prismatic results
         pTable = CSectionPropertiesTable2().Build(pBroker,span,girder,pgsTypes::BridgeSite3,pDisplayUnits);
         *pPara << pTable << rptNewLine;
      }
   }
   else if ( !bIsPrismatic_CastingYard && !bIsPrismatic_Final )
   {
      rptRcTable* pTable = CSectionPropertiesTable2().Build(pBroker,span,girder,pgsTypes::CastingYard,pDisplayUnits);
      *pPara << pTable << rptNewLine;

      if ( pBridge->GetDeckType() != pgsTypes::sdtNone ) // if there isn't a deck, no need to report duplicate properties
      {
         pTable = CSectionPropertiesTable2().Build(pBroker,span,girder,pgsTypes::BridgeSite3,pDisplayUnits);
         *pPara << pTable << rptNewLine;
      }
   }
   else if ( !bIsPrismatic_CastingYard && bIsPrismatic_Final )
   {
      ATLASSERT(false); // this is an impossible case
   }

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
