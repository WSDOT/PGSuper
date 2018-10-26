///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright (C) 1999  Washington State Department of Transportation
//                     Bridge and Structures Office
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
#include <Reporting\CamberChapterBuilder.h>
#include <Reporting\CamberTable.h>

#include <WBFLTools.h> // not sure why, but this is needed to compile

#include <IFace\DisplayUnits.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Project.h>
#include <IFace\Bridge.h>

#include <PsgLib\SpecLibraryEntry.h>

#include <PgsExt\GirderData.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CCamberChapterBuilder
****************************************************************************/


#define YCR(_details_) symbol(PSI) << "(" << time1.SetValue(_details_.t) << "," << time2.SetValue(_details_.ti) << ")"
#define DEFL(_f_) Sub2(symbol(DELTA),_f_)

////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CCamberChapterBuilder::CCamberChapterBuilder()
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CCamberChapterBuilder::GetName() const
{
   return TEXT("Camber Details");
}

rptChapter* CCamberChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CSpanGirderReportSpecification* pSGRptSpec = dynamic_cast<CSpanGirderReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pSGRptSpec->GetBroker(&pBroker);
   SpanIndexType span = pSGRptSpec->GetSpan();
   GirderIndexType gdr = pSGRptSpec->GetGirder();

   GET_IFACE2(pBroker,IDisplayUnits,pDispUnits);
   GET_IFACE2(pBroker,IGirderData,pGirderData);
   CGirderData girderData = pGirderData->GetGirderData(span,gdr);
   bool bTempStrands = ( 0 < girderData.Nstrands[pgsTypes::Temporary] && girderData.TempStrandUsage != pgsTypes::ttsPTBeforeShipping );

   GET_IFACE2(pBroker,IBridge,pBridge);
   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();

   rptChapter* pChapter;

   switch( deckType )
   {
   case pgsTypes::sdtCompositeCIP:
   case pgsTypes::sdtCompositeOverlay:
      pChapter = (bTempStrands ? Build_CIP_TempStrands(pRptSpec,pBroker,span,gdr,pDispUnits,level) : Build_CIP(pRptSpec,pBroker,span,gdr,pDispUnits,level));
      break;

   case pgsTypes::sdtCompositeSIP:
      pChapter = (bTempStrands ? Build_SIP_TempStrands(pRptSpec,pBroker,span,gdr,pDispUnits,level) : Build_SIP(pRptSpec,pBroker,span,gdr,pDispUnits,level));
      break;

   case pgsTypes::sdtNone:
      pChapter = (bTempStrands ? Build_NoDeck_TempStrands(pRptSpec,pBroker,span,gdr,pDispUnits,level) : Build_NoDeck(pRptSpec,pBroker,span,gdr,pDispUnits,level));
      break;

   default:
      ATLASSERT(false);
   }

   return pChapter;
}

CChapterBuilder* CCamberChapterBuilder::Clone() const
{
   return new CCamberChapterBuilder;
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
rptChapter* CCamberChapterBuilder::Build_CIP_TempStrands(CReportSpecification* pRptSpec,IBroker* pBroker,SpanIndexType span,GirderIndexType gdr,IDisplayUnits* pDispUnits,Uint16 level) const
{
   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);
   GET_IFACE2(pBroker,ICamber,pCamber);

   GET_IFACE2(pBroker,IBridge,pBridge);
   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();
   bool bDeckPanels = (deckType == pgsTypes::sdtCompositeSIP ? true : false);

   GET_IFACE2(pBroker,IProductForces,pProductForces);
   bool bSidewalk = pProductForces->HasSidewalkLoad(span,gdr);

   INIT_UV_PROTOTYPE( rptTimeUnitValue, time1, pDispUnits->GetLongTimeUnit(), false );
   INIT_UV_PROTOTYPE( rptTimeUnitValue, time2, pDispUnits->GetLongTimeUnit(), false );

   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;
   if ( pBridge->IsFutureOverlay() )
      *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + "Camber_CIP_TempStrands_FutureOverlay.gif") << rptNewLine;
   else
      *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + "Camber_CIP_TempStrands.gif") << rptNewLine;

   for ( Int16 i = CREEP_MINTIME; i <= CREEP_MAXTIME; i++ )
   {
      pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      *pChapter << pPara;

      *pPara << (i == CREEP_MINTIME ? "Minimum Timing" : "Maximum Timing") << rptNewLine;

      pPara = new rptParagraph;
      *pChapter << pPara;

      CREEPCOEFFICIENTDETAILS details[3];
      details[0] = pCamber->GetCreepCoefficientDetails(span,gdr,ICamber::cpReleaseToDiaphragm,i);
      details[1] = pCamber->GetCreepCoefficientDetails(span,gdr,ICamber::cpReleaseToDeck,i);
      details[2] = pCamber->GetCreepCoefficientDetails(span,gdr,ICamber::cpDiaphragmToDeck,i);

      CCamberTable tbl;
      rptRcTable* pTable1, *pTable2, *pTable3;
      tbl.Build_CIP_TempStrands(pBroker,span,gdr,pDispUnits,i,&pTable1,&pTable2,&pTable3);
      *pPara << pTable1 << rptNewLine;

      pPara = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
      *pChapter << pPara;
      *pPara << Sub2(symbol(DELTA),"ps") << " " << Sub2("L","g") << " is based the girder length and measured relative to the end of the girder" << rptNewLine;
      *pPara << Sub2(symbol(DELTA),"ps") << " " << Sub2("L","s") << " is based the final span length and measured relative to the final bearing location" << rptNewLine;
      *pPara << Sub2(symbol(DELTA),"girder") << " is based on final span length and " << RPT_ECI << rptNewLine;
      *pPara << DEFL("creep1") << " = " << YCR(details[0]) << "(" << DEFL("girder") << " + " << DEFL("ps") << " " << Sub2("L","s") << " + " << DEFL("tpsi") << ")" << rptNewLine;

      *pPara << pTable2 << rptNewLine;
      *pPara << DEFL("creep2") << " = " << "[" << YCR(details[1]);
      *pPara << " - " << YCR(details[0]) << "]" << "(" << DEFL("girder") << " + " << DEFL("ps") << " " << Sub2("L","s") << " + " << DEFL("tpsi") << ")";
      *pPara << " + " << YCR(details[2]) << "(" << DEFL("diaphragm") << " + " << DEFL("tpsr") << ")" << rptNewLine;

      *pPara << pTable3 << rptNewLine;
      *pPara << DEFL("1") << " = " << DEFL("girder") << " + " << DEFL("ps") << " " << Sub2("L","s") << " + " << DEFL("tpsi") << rptNewLine;
      *pPara << DEFL("2") << " = " << DEFL("1") << " + " << DEFL("creep1") << rptNewLine;
      *pPara << DEFL("3") << " = " << DEFL("2") << " + " << DEFL("diaphragm") << " + " << DEFL("tpsr") << rptNewLine;
      *pPara << DEFL("4") << " = " << DEFL("3") << " + " << DEFL("creep2") << rptNewLine;
      *pPara << DEFL("5") << " = " << DEFL("4") << " + " << DEFL("deck") << " + " << DEFL("user1") << rptNewLine;

      *pPara << DEFL("6") << " = " << DEFL("5");
      if ( bSidewalk )
         *pPara << " + " << DEFL("sidewalk");

      *pPara << " + " << DEFL("barrier");

      if ( !pBridge->IsFutureOverlay() )
         *pPara << " + " << DEFL("overlay");

      *pPara << " + " << DEFL("user2") << rptNewLine;

      *pPara << rptNewLine;
   }

   return pChapter;
}

rptChapter* CCamberChapterBuilder::Build_CIP(CReportSpecification* pRptSpec,IBroker* pBroker,SpanIndexType span,GirderIndexType gdr,IDisplayUnits* pDispUnits,Uint16 level) const
{
   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);
   GET_IFACE2(pBroker,ICamber,pCamber);

   GET_IFACE2(pBroker,IBridge,pBridge);
   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();
   bool bDeckPanels = (deckType == pgsTypes::sdtCompositeSIP ? true : false);

   GET_IFACE2(pBroker,IProductForces,pProductForces);
   bool bSidewalk = pProductForces->HasSidewalkLoad(span,gdr);

   INIT_UV_PROTOTYPE( rptTimeUnitValue, time1, pDispUnits->GetLongTimeUnit(), false );
   INIT_UV_PROTOTYPE( rptTimeUnitValue, time2, pDispUnits->GetLongTimeUnit(), false );

   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   if ( pBridge->IsFutureOverlay() )
      *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + "Camber_CIP_FutureOverlay.gif") << rptNewLine;
   else
      *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + "Camber_CIP.gif") << rptNewLine;

   for ( Int16 i = CREEP_MINTIME; i <= CREEP_MAXTIME; i++ )
   {
      pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      *pChapter << pPara;

      *pPara << (i == CREEP_MINTIME ? "Minimum Timing" : "Maximum Timing") << rptNewLine;

      pPara = new rptParagraph;
      *pChapter << pPara;

      CREEPCOEFFICIENTDETAILS details[2];
      details[0] = pCamber->GetCreepCoefficientDetails(span,gdr,ICamber::cpReleaseToDeck,i);
      details[1] = pCamber->GetCreepCoefficientDetails(span,gdr,ICamber::cpDiaphragmToDeck,i);

      CCamberTable tbl;
      rptRcTable* pTable1, *pTable2, *pTable3;
      tbl.Build_CIP(pBroker,span,gdr,pDispUnits,i,&pTable1,&pTable2,&pTable3);
      *pPara << pTable1 << rptNewLine;

      pPara = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
      *pChapter << pPara;
      *pPara << Sub2(symbol(DELTA),"ps") << " " << Sub2("L","g") << " is based the girder length and measured relative to the end of the girder" << rptNewLine;
      *pPara << Sub2(symbol(DELTA),"ps") << " " << Sub2("L","s") << " is based the final span length and measured relative to the final bearing location" << rptNewLine;
      *pPara << Sub2(symbol(DELTA),"girder") << " is based on final span length and " << RPT_ECI << rptNewLine;
      *pPara << DEFL("creep") << " = " << YCR(details[0]) << "(" << DEFL("girder") << " + " << DEFL("ps") << " " << Sub2("L","s") << ")" << rptNewLine;

      *pPara << pTable2 << rptNewLine;

      *pPara << pTable3 << rptNewLine;
      *pPara << DEFL("1") << " = " << DEFL("girder") << " + " << DEFL("ps") << " " << Sub2("L","s") << rptNewLine;
      *pPara << DEFL("2") << " = " << DEFL("1") << " + " << DEFL("creep") << rptNewLine;
      *pPara << DEFL("3") << " = " << DEFL("2") << " + " << DEFL("diaphragm")<< " + " << DEFL("deck") << " + " << DEFL("user1") << rptNewLine;
      *pPara << DEFL("4") << " = " << DEFL("3");

      if ( bSidewalk )
         *pPara << " + " << DEFL("sidewalk");

      *pPara << " + " << DEFL("barrier");

      if ( !pBridge->IsFutureOverlay() )
         *pPara << " + " << DEFL("overlay");
      
      *pPara << " + " << DEFL("user2") << rptNewLine;


      *pPara << rptNewLine;
   }

   return pChapter;
}

rptChapter* CCamberChapterBuilder::Build_SIP_TempStrands(CReportSpecification* pRptSpec,IBroker* pBroker,SpanIndexType span,GirderIndexType gdr,IDisplayUnits* pDispUnits,Uint16 level) const
{
   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);
   GET_IFACE2(pBroker,ICamber,pCamber);

   GET_IFACE2(pBroker,IBridge,pBridge);
   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();
   bool bDeckPanels = (deckType == pgsTypes::sdtCompositeSIP ? true : false);

   GET_IFACE2(pBroker,IProductForces,pProductForces);
   bool bSidewalk = pProductForces->HasSidewalkLoad(span,gdr);

   INIT_UV_PROTOTYPE( rptTimeUnitValue, time1, pDispUnits->GetLongTimeUnit(), false );
   INIT_UV_PROTOTYPE( rptTimeUnitValue, time2, pDispUnits->GetLongTimeUnit(), false );

   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   if ( pBridge->IsFutureOverlay() )
      *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + "Camber_SIP_TempStrands_FutureOverlay.gif") << rptNewLine;
   else
      *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + "Camber_SIP_TempStrands.gif") << rptNewLine;

   for ( Int16 i = CREEP_MINTIME; i <= CREEP_MAXTIME; i++ )
   {
      pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      *pChapter << pPara;

      *pPara << (i == CREEP_MINTIME ? "Minimum Timing" : "Maximum Timing") << rptNewLine;

      pPara = new rptParagraph;
      *pChapter << pPara;

      CREEPCOEFFICIENTDETAILS details[3];
      details[0] = pCamber->GetCreepCoefficientDetails(span,gdr,ICamber::cpReleaseToDiaphragm,i);
      details[1] = pCamber->GetCreepCoefficientDetails(span,gdr,ICamber::cpReleaseToDeck,i);
      details[2] = pCamber->GetCreepCoefficientDetails(span,gdr,ICamber::cpDiaphragmToDeck,i);

      CCamberTable tbl;
      rptRcTable* pTable1, *pTable2, *pTable3;
      tbl.Build_SIP_TempStrands(pBroker,span,gdr,pDispUnits,i,&pTable1,&pTable2,&pTable3);
      *pPara << pTable1 << rptNewLine;

      pPara = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
      *pChapter << pPara;
      *pPara << Sub2(symbol(DELTA),"ps") << " " << Sub2("L","g") << " is based the girder length and measured relative to the end of the girder" << rptNewLine;
      *pPara << Sub2(symbol(DELTA),"ps") << " " << Sub2("L","s") << " is based the final span length and measured relative to the final bearing location" << rptNewLine;
      *pPara << Sub2(symbol(DELTA),"girder") << " is based on final span length and " << RPT_ECI << rptNewLine;
      *pPara << DEFL("creep1") << " = " << YCR(details[0]) << "(" << DEFL("girder") << " + " << DEFL("ps") << " " << Sub2("L","s") << " + " << DEFL("tpsi") << ")" << rptNewLine;

      *pPara << pTable2 << rptNewLine;
      *pPara << DEFL("creep2") << " = " << "[" << YCR(details[1]);
      *pPara << " - " << YCR(details[0]) << "]" << "(" << DEFL("girder") << " + " << DEFL("ps") << " " << Sub2("L","s") << " + " << DEFL("tpsi") << ")";
      *pPara << " + " << YCR(details[2]) << "(" << DEFL("diaphragm") << " + " << DEFL("tpsr") << ")" << rptNewLine;

      *pPara << pTable3 << rptNewLine;
      *pPara << DEFL("1") << " = " << DEFL("girder") << " + " << DEFL("ps") << " " << Sub2("L","s") << " + " << DEFL("tpsi") << rptNewLine;
      *pPara << DEFL("2") << " = " << DEFL("1") << " + " << DEFL("creep1") << rptNewLine;
      *pPara << DEFL("3") << " = " << DEFL("2") << " + " << DEFL("diaphragm") << " + " << DEFL("tpsr") << " + " << DEFL("panels")  << rptNewLine;
      *pPara << DEFL("4") << " = " << DEFL("3") << " + " << DEFL("creep2") << rptNewLine;
      *pPara << DEFL("5") << " = " << DEFL("4")<< " + " << DEFL("deck") << " + " << DEFL("user1") << rptNewLine;
      *pPara << DEFL("6") << " = " << DEFL("5");

      if ( bSidewalk )
         *pPara << " + " << DEFL("sidewalk");
      
      *pPara << " + " << DEFL("barrier");
   
      if ( !pBridge->IsFutureOverlay() )
         *pPara << " + " << DEFL("overlay") << rptNewLine;

      *pPara << " + " << DEFL("user2") << rptNewLine;

      *pPara << rptNewLine;
   }

   return pChapter;
}

rptChapter* CCamberChapterBuilder::Build_SIP(CReportSpecification* pRptSpec,IBroker* pBroker,SpanIndexType span,GirderIndexType gdr,IDisplayUnits* pDispUnits,Uint16 level) const
{
   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);
   GET_IFACE2(pBroker,ICamber,pCamber);

   GET_IFACE2(pBroker,IBridge,pBridge);
   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();
   bool bDeckPanels = (deckType == pgsTypes::sdtCompositeSIP ? true : false);

   GET_IFACE2(pBroker,IProductForces,pProductForces);
   bool bSidewalk = pProductForces->HasSidewalkLoad(span,gdr);

   INIT_UV_PROTOTYPE( rptTimeUnitValue, time1, pDispUnits->GetLongTimeUnit(), false );
   INIT_UV_PROTOTYPE( rptTimeUnitValue, time2, pDispUnits->GetLongTimeUnit(), false );

   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;
   if (pBridge->IsFutureOverlay() )
      *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + "Camber_SIP.gif") << rptNewLine;
   else
      *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + "Camber_SIP_FutureOverlay.gif") << rptNewLine;

   for ( Int16 i = CREEP_MINTIME; i <= CREEP_MAXTIME; i++ )
   {
      pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      *pChapter << pPara;

      *pPara << (i == CREEP_MINTIME ? "Minimum Timing" : "Maximum Timing") << rptNewLine;

      pPara = new rptParagraph;
      *pChapter << pPara;

      CREEPCOEFFICIENTDETAILS details[2];
      details[0] = pCamber->GetCreepCoefficientDetails(span,gdr,ICamber::cpReleaseToDeck,i);
      details[1] = pCamber->GetCreepCoefficientDetails(span,gdr,ICamber::cpDiaphragmToDeck,i);

      CCamberTable tbl;
      rptRcTable* pTable1, *pTable2, *pTable3;
      tbl.Build_SIP(pBroker,span,gdr,pDispUnits,i,&pTable1,&pTable2,&pTable3);
      *pPara << pTable1 << rptNewLine;

      pPara = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
      *pChapter << pPara;
      *pPara << Sub2(symbol(DELTA),"ps") << " " << Sub2("L","g") << " is based the girder length and measured relative to the end of the girder" << rptNewLine;
      *pPara << Sub2(symbol(DELTA),"ps") << " " << Sub2("L","s") << " is based the final span length and measured relative to the final bearing location" << rptNewLine;
      *pPara << Sub2(symbol(DELTA),"girder") << " is based on final span length and " << RPT_ECI << rptNewLine;
      *pPara << DEFL("creep") << " = " << YCR(details[0]) << "(" << DEFL("girder") << " + " << DEFL("ps") << " " << Sub2("L","s") << ")" << rptNewLine;

      *pPara << pTable2 << rptNewLine;

      *pPara << pTable3 << rptNewLine;
      *pPara << DEFL("1") << " = " << DEFL("girder") << " + " << DEFL("ps") << " " << Sub2("L","s") << rptNewLine;
      *pPara << DEFL("2") << " = " << DEFL("1") << " + " << DEFL("creep") << rptNewLine;
      *pPara << DEFL("3") << " = " << DEFL("2") << " + " << DEFL("diaphragm") << " + " << DEFL("panel") << rptNewLine;
      *pPara << DEFL("4") << " = " << DEFL("3") << " + " << DEFL("deck") << " + " << DEFL("user1");
      
      if ( bSidewalk )
         *pPara << " + " << DEFL("sidewalk");

      *pPara << " + " << DEFL("barrier");

      if ( !pBridge->IsFutureOverlay() )
         *pPara << " + " << DEFL("overlay") << rptNewLine;

      *pPara << " + " << DEFL("user2") << rptNewLine;

      *pPara << rptNewLine;
   }

   return pChapter;
}

rptChapter* CCamberChapterBuilder::Build_NoDeck_TempStrands(CReportSpecification* pRptSpec,IBroker* pBroker,SpanIndexType span,GirderIndexType gdr,IDisplayUnits* pDispUnits,Uint16 level) const
{
   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);
   GET_IFACE2(pBroker,ICamber,pCamber);

   GET_IFACE2(pBroker,IBridge,pBridge);
   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();
   bool bDeckPanels = (deckType == pgsTypes::sdtCompositeSIP ? true : false);

   GET_IFACE2(pBroker,IProductForces,pProductForces);
   bool bSidewalk = pProductForces->HasSidewalkLoad(span,gdr);

   INIT_UV_PROTOTYPE( rptTimeUnitValue, time1, pDispUnits->GetLongTimeUnit(), false );
   INIT_UV_PROTOTYPE( rptTimeUnitValue, time2, pDispUnits->GetLongTimeUnit(), false );

   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;
   if ( pBridge->IsFutureOverlay() )
      *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + "Camber_NoDeck_TempStrands_FutureOverlay.gif") << rptNewLine;
   else
      *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + "Camber_NoDeck_TempStrands.gif") << rptNewLine;

   for ( Int16 i = CREEP_MINTIME; i <= CREEP_MAXTIME; i++ )
   {
      pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      *pChapter << pPara;

      *pPara << (i == CREEP_MINTIME ? "Minimum Timing" : "Maximum Timing") << rptNewLine;

      pPara = new rptParagraph;
      *pChapter << pPara;

      CREEPCOEFFICIENTDETAILS details[6];
      details[0] = pCamber->GetCreepCoefficientDetails(span,gdr,ICamber::cpReleaseToDiaphragm,i);
      details[1] = pCamber->GetCreepCoefficientDetails(span,gdr,ICamber::cpReleaseToDeck,     i);
      details[2] = pCamber->GetCreepCoefficientDetails(span,gdr,ICamber::cpReleaseToFinal,    i);
      details[3] = pCamber->GetCreepCoefficientDetails(span,gdr,ICamber::cpDiaphragmToDeck,   i);
      details[4] = pCamber->GetCreepCoefficientDetails(span,gdr,ICamber::cpDiaphragmToFinal,  i);
      details[5] = pCamber->GetCreepCoefficientDetails(span,gdr,ICamber::cpDeckToFinal,       i);

      CCamberTable tbl;
      rptRcTable* pTable1, *pTable2, *pTable3;
      tbl.Build_NoDeck_TempStrands(pBroker,span,gdr,pDispUnits,i,&pTable1,&pTable2,&pTable3);
      *pPara << pTable1 << rptNewLine;

      pPara = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
      *pChapter << pPara;
      *pPara << Sub2(symbol(DELTA),"ps") << " " << Sub2("L","g") << " is based the girder length and measured relative to the end of the girder" << rptNewLine;
      *pPara << Sub2(symbol(DELTA),"ps") << " " << Sub2("L","s") << " is based the final span length and measured relative to the final bearing location" << rptNewLine;
      *pPara << Sub2(symbol(DELTA),"girder") << " is based on final span length and " << RPT_ECI << rptNewLine;
      *pPara << DEFL("creep1") << " = " << YCR(details[0]) << "(" << DEFL("girder") << " + " << DEFL("ps") << " " << Sub2("L","s") << " + " << DEFL("tpsi") << ")" << rptNewLine;

      *pPara << pTable2 << rptNewLine;
      *pPara << DEFL("creep2") << " = " << "[" << YCR(details[1]);
      *pPara << " - " << YCR(details[0]) << "]" << "(" << DEFL("girder") << " + " << DEFL("ps") << " " << Sub2("L","s") << ")";
      *pPara << " + " << YCR(details[3]) << "(" << DEFL("diaphragm") << " + " << DEFL("tpsr") << ")" << rptNewLine;
      
      *pPara << rptNewLine;

      *pPara << DEFL("creep3") << " = " << "[" << YCR(details[2]);
      *pPara << " - " << YCR(details[1]) << "]" << "(" << DEFL("girder") << " + " << DEFL("ps") << " " << Sub2("L","s") << ")";
      *pPara << " + [" << YCR(details[4]);
      *pPara << " - " << YCR(details[3]) << "](" << DEFL("diaphragm") << " + " << DEFL("tpsr") << ")";
      *pPara << " + " << YCR(details[5]) << "(" << DEFL("barrier") << " + " << DEFL("overlay") << " + " << DEFL("user2") << ")" << rptNewLine;

      *pPara << pTable3 << rptNewLine;
      *pPara << DEFL("1") << " = " << DEFL("girder") << " + " << DEFL("ps") << " " << Sub2("L","s") << " + " << DEFL("tpsi") << rptNewLine;
      *pPara << DEFL("2") << " = " << DEFL("1") << " + " << DEFL("creep1") << rptNewLine;
      *pPara << DEFL("3") << " = " << DEFL("2") << " + " << DEFL("diaphragm") << " + " << DEFL("tpsr")  << " + " << DEFL("user1") << rptNewLine;
      *pPara << DEFL("4") << " = " << DEFL("3") << " + " << DEFL("creep2") << rptNewLine;
      *pPara << DEFL("5") << " = " << DEFL("4");
      
      if ( bSidewalk )
         *pPara << " + " << DEFL("sidewalk");

      *pPara << " + " << DEFL("barrier");

      if ( !pBridge->IsFutureOverlay() )
         *pPara << " + " << DEFL("overlay");

      *pPara << " + " << DEFL("user2") << rptNewLine;

      *pPara << DEFL("6") << " = " << DEFL("5") << " + " << DEFL("creep3") << rptNewLine;

      *pPara << rptNewLine;
   }

   return pChapter;
}

rptChapter* CCamberChapterBuilder::Build_NoDeck(CReportSpecification* pRptSpec,IBroker* pBroker,SpanIndexType span,GirderIndexType gdr,IDisplayUnits* pDispUnits,Uint16 level) const
{
   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);
   GET_IFACE2(pBroker,ICamber,pCamber);

   GET_IFACE2(pBroker,IBridge,pBridge);
   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();
   bool bDeckPanels = (deckType == pgsTypes::sdtCompositeSIP ? true : false);

   INIT_UV_PROTOTYPE( rptTimeUnitValue, time1, pDispUnits->GetLongTimeUnit(), false );
   INIT_UV_PROTOTYPE( rptTimeUnitValue, time2, pDispUnits->GetLongTimeUnit(), false );

   GET_IFACE2(pBroker,IProductForces,pProductForces);
   bool bSidewalk = pProductForces->HasSidewalkLoad(span,gdr);

   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   if ( pBridge->IsFutureOverlay() )
      *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + "Camber_NoDeck_FutureOverlay.gif") << rptNewLine;
   else
      *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + "Camber_NoDeck.gif") << rptNewLine;

   for ( Int16 i = CREEP_MINTIME; i <= CREEP_MAXTIME; i++ )
   {
      pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      *pChapter << pPara;

      *pPara << (i == CREEP_MINTIME ? "Minimum Timing" : "Maximum Timing") << rptNewLine;

      pPara = new rptParagraph;
      *pChapter << pPara;

      CREEPCOEFFICIENTDETAILS details[6];
      details[0] = pCamber->GetCreepCoefficientDetails(span,gdr,ICamber::cpReleaseToDiaphragm,i);
      details[1] = pCamber->GetCreepCoefficientDetails(span,gdr,ICamber::cpReleaseToDeck,     i);
      details[2] = pCamber->GetCreepCoefficientDetails(span,gdr,ICamber::cpReleaseToFinal,    i);
      details[3] = pCamber->GetCreepCoefficientDetails(span,gdr,ICamber::cpDiaphragmToDeck,   i);
      details[4] = pCamber->GetCreepCoefficientDetails(span,gdr,ICamber::cpDiaphragmToFinal,  i);
      details[5] = pCamber->GetCreepCoefficientDetails(span,gdr,ICamber::cpDeckToFinal,       i);

      CCamberTable tbl;
      rptRcTable* pTable1, *pTable2, *pTable3;
      tbl.Build_NoDeck(pBroker,span,gdr,pDispUnits,i,&pTable1,&pTable2,&pTable3);
      *pPara << pTable1 << rptNewLine;

      pPara = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
      *pChapter << pPara;
      *pPara << Sub2(symbol(DELTA),"ps") << " " << Sub2("L","g") << " is based the girder length and measured relative to the end of the girder" << rptNewLine;
      *pPara << Sub2(symbol(DELTA),"ps") << " " << Sub2("L","s") << " is based the final span length and measured relative to the final bearing location" << rptNewLine;
      *pPara << Sub2(symbol(DELTA),"girder") << " is based on final span length and " << RPT_ECI << rptNewLine;
      *pPara << DEFL("creep1") << " = " << YCR(details[0]) << "(" << DEFL("girder") << " + " << DEFL("ps") << " " << Sub2("L","s") << ")" << rptNewLine;

      *pPara << pTable2 << rptNewLine;
      *pPara << DEFL("creep2") << " = " << "[" << YCR(details[1]);
      *pPara << " - " << YCR(details[0]) << "]" << "(" << DEFL("girder") << " + " << DEFL("ps") << " " << Sub2("L","s") << ")";
      *pPara << " + " << YCR(details[3]) << "(" << DEFL("diaphragm") << " + " << DEFL("user1") << ")" << rptNewLine;
      
      *pPara << rptNewLine;

      *pPara << DEFL("creep3") << " = " << "[" << YCR(details[2]);
      *pPara << " - " << YCR(details[1]) << "]" << "(" << DEFL("girder") << " + " << DEFL("ps") << " " << Sub2("L","s") << ")";
      *pPara << " + [" << YCR(details[4]);
      *pPara << " - " << YCR(details[3]) << "](" << DEFL("diaphragm") << " + " << DEFL("user1") << ")";
      *pPara << " + " << YCR(details[5]) << "(" << DEFL("barrier") << " + " << DEFL("overlay") << " + " << DEFL("user2") << ")" << rptNewLine;

      *pPara << pTable3 << rptNewLine;
      *pPara << DEFL("1") << " = " << DEFL("girder") << " + " << DEFL("ps") << " " << Sub2("L","s") << rptNewLine;
      *pPara << DEFL("2") << " = " << DEFL("1") << " + " << DEFL("creep1") << rptNewLine;
      *pPara << DEFL("3") << " = " << DEFL("2") << " + " << DEFL("diaphragm") << " + " << DEFL("user1") << rptNewLine;
      *pPara << DEFL("4") << " = " << DEFL("3") << " + " << DEFL("creep2") << rptNewLine;
      *pPara << DEFL("5") << " = " << DEFL("4");

      if ( bSidewalk )
         *pPara << " + " << DEFL("sidewalk");

      *pPara << " + " << DEFL("barrier");

      if ( !pBridge->IsFutureOverlay() )
         *pPara << " + " << DEFL("overlay");

      *pPara << " + " << DEFL("user2") << rptNewLine;

      *pPara << DEFL("6") << " = " << DEFL("5") << " + " << DEFL("creep3") << rptNewLine;

      *pPara << rptNewLine;
   }

   return pChapter;
}
