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
#include <Reporting\PrestressForceChapterBuilder.h>
#include <Reporting\PrestressLossTable.h>

#include <PgsExt\GirderData.h>

#include <IFace\DisplayUnits.h>
#include <IFace\Bridge.h>
#include <IFace\PrestressForce.h>
#include <IFace\Project.h>
#include <IFace\AnalysisResults.h>

#include <Material\PsStrand.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CPrestressForceChapterBuilder
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CPrestressForceChapterBuilder::CPrestressForceChapterBuilder()
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CPrestressForceChapterBuilder::GetName() const
{
   return TEXT("Prestressing Force and Strand Stresses");
}

rptChapter* CPrestressForceChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CSpanGirderReportSpecification* pSGRptSpec = dynamic_cast<CSpanGirderReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pSGRptSpec->GetBroker(&pBroker);
   SpanIndexType span = pSGRptSpec->GetSpan();
   GirderIndexType gdr = pSGRptSpec->GetGirder();

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   GET_IFACE2(pBroker,IDisplayUnits,pDisplayUnits);
   // These are the interfaces we are going to be using
   GET_IFACE2(pBroker,IBridgeMaterial,       pMat);
   GET_IFACE2(pBroker,IStrandGeometry, pStrandGeom);
   GET_IFACE2(pBroker,IPrestressForce, pPrestressForce ); 
   GET_IFACE2(pBroker,IPointOfInterest,pIPOI);
   GET_IFACE2(pBroker,IGirderData,pGirderData);
   GET_IFACE2(pBroker,ILosses,pLosses);
   GET_IFACE2(pBroker,IPrestressStresses,pPrestressStresses);


   std::vector<pgsPointOfInterest> vPoi = pIPOI->GetPointsOfInterest(pgsTypes::BridgeSite3,span,gdr,POI_MIDSPAN);
   pgsPointOfInterest poi = *vPoi.begin();

   CGirderData girderData = pGirderData->GetGirderData(span,gdr);

   // Setup some unit-value prototypes
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(),       true );
   INIT_UV_PROTOTYPE( rptAreaUnitValue,   area,   pDisplayUnits->GetAreaUnit(),         true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, len,    pDisplayUnits->GetComponentDimUnit(), true );
   INIT_UV_PROTOTYPE( rptForceUnitValue,  force,  pDisplayUnits->GetGeneralForceUnit(), true );

   rptParagraph* pPara;

   // Write out what we have for prestressing in this girder
   pPara = new rptParagraph;
   *pChapter << pPara;
   StrandIndexType Ns = pStrandGeom->GetNumStrands(span,gdr,pgsTypes::Straight);
   *pPara << "Number of straight strands (N" << Sub("s") << ") = " << Ns << " " 
      << "(P" << Sub("jack") << " = " << force.SetValue(pStrandGeom->GetPjack(span,gdr,pgsTypes::Straight)) << ")" << rptNewLine;
   *pPara << "Number of harped strands (N" << Sub("h") << ") = " << pStrandGeom->GetNumStrands(span,gdr,pgsTypes::Harped) << " " 
      << "(P" << Sub("jack") << " = " << force.SetValue(pStrandGeom->GetPjack(span,gdr,pgsTypes::Harped)) << ")" << rptNewLine;

   if ( 0 < pStrandGeom->GetMaxStrands(span,gdr,pgsTypes::Temporary ) )
   {
      *pPara << "Number of temporary strands (N" << Sub("t") << ") = " << pStrandGeom->GetNumStrands(span,gdr,pgsTypes::Temporary) << " " 
         << "(P" << Sub("jack") << " = " << force.SetValue(pStrandGeom->GetPjack(span,gdr,pgsTypes::Temporary)) << ")" << rptNewLine;

         
      switch(girderData.TempStrandUsage)
      {
      case pgsTypes::ttsPretensioned:
         *pPara << "Temporary Strands pretensioned with permanent strands" << rptNewLine;
         break;

      case pgsTypes::ttsPTBeforeShipping:
         *pPara << "Temporary Strands post-tensioned immedately before shipping" << rptNewLine;
         break;

      case pgsTypes::ttsPTAfterLifting:
         *pPara << "Temporary Strands post-tensioned immedately after lifting" << rptNewLine;
         break;

      case pgsTypes::ttsPTBeforeLifting:
         *pPara << "Temporary Strands post-tensioned before lifting" << rptNewLine;
         break;
      }

      *pPara << "Permanent Strands: " << RPT_APS << " = " << area.SetValue(pStrandGeom->GetStrandArea(span,gdr,pgsTypes::Permanent)) << rptNewLine;
      *pPara << "Temporary Strands: " << RPT_APS << " = " << area.SetValue(pStrandGeom->GetStrandArea(span,gdr,pgsTypes::Temporary)) << rptNewLine;
      *pPara << "Total Strand Area: " << RPT_APS << " = " << area.SetValue( pStrandGeom->GetAreaPrestressStrands(span,gdr,true)) << rptNewLine;
   }
   else
   {
      *pPara << RPT_APS << " = " << area.SetValue( pStrandGeom->GetAreaPrestressStrands(span,gdr,false)) << rptNewLine;
      *pPara << Sub2("P","jack") << " = " << force.SetValue( pStrandGeom->GetPjack(span,gdr,false)) << rptNewLine;
   }

   *pPara << "Prestress Transfer Length = " << len.SetValue( pPrestressForce->GetXferLength(span,gdr) ) << rptNewLine;
   //*pPara << "Prestress Development Length (bonded strands) = " << len.SetValue( pPrestressForce->GetDevLength(span,gdr,false) ) << rptNewLine;
   //*pPara << "Prestress Development Length (debonded strands) = " << len.SetValue( pPrestressForce->GetDevLength(span,gdr,true) ) << rptNewLine;

   // Write out strand forces and stresses at the various stages of prestress loss
   pPara = new rptParagraph;
   *pChapter << pPara;
   *pPara << CPrestressLossTable().Build(pBroker,span,gdr,pDisplayUnits) << rptNewLine;

   return pChapter;
}

CChapterBuilder* CPrestressForceChapterBuilder::Clone() const
{
   return new CPrestressForceChapterBuilder;
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
