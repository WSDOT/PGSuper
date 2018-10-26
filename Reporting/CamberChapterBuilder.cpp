///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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
#include <Reporting\CamberChapterBuilder.h>
#include <Reporting\CamberTable.h>

#include <WBFLTools.h> // not sure why, but this is needed to compile

#include <IFace\AnalysisResults.h>
#include <IFace\Project.h>
#include <IFace\Bridge.h>

#include <PsgLib\SpecLibraryEntry.h>

#include <PgsExt\StrandData.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CCamberChapterBuilder
****************************************************************************/


#define YCR(_details_) symbol(PSI) << _T("(") << time1.SetValue(_details_.t) << _T(",") << time2.SetValue(_details_.ti) << _T(")")
#define DEFL(_f_) Sub2(symbol(DELTA),_f_)

////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CCamberChapterBuilder::CCamberChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
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
   CGirderReportSpecification* pGirderRptSpec = dynamic_cast<CGirderReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pGirderRptSpec->GetBroker(&pBroker);
   const CGirderKey& girderKey(pGirderRptSpec->GetGirderKey());

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker,ISegmentData,pSegmentData);
   GET_IFACE2(pBroker,IBridge,pBridge);
   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      CSegmentKey segmentKey(girderKey,segIdx);

      const CStrandData* pStrands = pSegmentData->GetStrandData(segmentKey);
      bool bTempStrands = ( 0 < pStrands->Nstrands[pgsTypes::Temporary] && pStrands->TempStrandUsage != pgsTypes::ttsPTBeforeShipping );

      switch( deckType )
      {
      case pgsTypes::sdtCompositeCIP:
      case pgsTypes::sdtCompositeOverlay:
         (bTempStrands ? Build_CIP_TempStrands(pChapter,pRptSpec,pBroker,segmentKey,pDisplayUnits,level) : Build_CIP(pChapter,pRptSpec,pBroker,segmentKey,pDisplayUnits,level));
         break;

      case pgsTypes::sdtCompositeSIP:
         (bTempStrands ? Build_SIP_TempStrands(pChapter,pRptSpec,pBroker,segmentKey,pDisplayUnits,level) : Build_SIP(pChapter,pRptSpec,pBroker,segmentKey,pDisplayUnits,level));
         break;

      case pgsTypes::sdtNone:
         (bTempStrands ? Build_NoDeck_TempStrands(pChapter,pRptSpec,pBroker,segmentKey,pDisplayUnits,level) : Build_NoDeck(pChapter,pRptSpec,pBroker,segmentKey,pDisplayUnits,level));
         break;

      default:
         ATLASSERT(false);
      }
   }

   return pChapter;
}

CChapterBuilder* CCamberChapterBuilder::Clone() const
{
   return new CCamberChapterBuilder;
}

void CCamberChapterBuilder::Build_CIP_TempStrands(rptChapter* pChapter,CReportSpecification* pRptSpec,IBroker* pBroker,const CSegmentKey& segmentKey,IEAFDisplayUnits* pDisplayUnits,Uint16 level) const
{
   GET_IFACE2(pBroker,ICamber,pCamber);

   GET_IFACE2(pBroker,IBridge,pBridge);
   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();
   bool bDeckPanels = (deckType == pgsTypes::sdtCompositeSIP ? true : false);

   GET_IFACE2(pBroker,IProductLoads,pProductLoads);
   bool bSidewalk = pProductLoads->HasSidewalkLoad(segmentKey);

   INIT_UV_PROTOTYPE( rptTimeUnitValue, time1, pDisplayUnits->GetLongTimeUnit(), false );
   INIT_UV_PROTOTYPE( rptTimeUnitValue, time2, pDisplayUnits->GetLongTimeUnit(), false );

   rptParagraph* pPara = new rptParagraph;
   if ( pBridge->IsFutureOverlay() )
      *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("Camber_CIP_TempStrands_FutureOverlay.gif")) << rptNewLine;
   else
      *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("Camber_CIP_TempStrands.gif")) << rptNewLine;

   for ( Int16 i = CREEP_MINTIME; i <= CREEP_MAXTIME; i++ )
   {
      pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      *pChapter << pPara;

      *pPara << (i == CREEP_MINTIME ? _T("Minimum Timing") : _T("Maximum Timing")) << rptNewLine;

      pPara = new rptParagraph;
      *pChapter << pPara;

      CREEPCOEFFICIENTDETAILS details[3];
      details[0] = pCamber->GetCreepCoefficientDetails(segmentKey,ICamber::cpReleaseToDiaphragm,i);
      details[1] = pCamber->GetCreepCoefficientDetails(segmentKey,ICamber::cpReleaseToDeck,i);
      details[2] = pCamber->GetCreepCoefficientDetails(segmentKey,ICamber::cpDiaphragmToDeck,i);

      CCamberTable tbl;
      rptRcTable* pTable1, *pTable2, *pTable3;
      tbl.Build_CIP_TempStrands(pBroker,segmentKey,pDisplayUnits,i,&pTable1,&pTable2,&pTable3);
      *pPara << pTable1 << rptNewLine;

      pPara = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
      *pChapter << pPara;
      *pPara << Sub2(symbol(DELTA),_T("ps")) << _T(" ") << Sub2(_T("L"),_T("g")) << _T(" is based the girder length and measured relative to the end of the girder") << rptNewLine;
      *pPara << Sub2(symbol(DELTA),_T("ps")) << _T(" ") << Sub2(_T("L"),_T("s")) << _T(" is based the final span length and measured relative to the final bearing location") << rptNewLine;
      *pPara << Sub2(symbol(DELTA),_T("girder")) << _T(" is based on final span length and ") << RPT_ECI << rptNewLine;
      *pPara << DEFL(_T("creep1")) << _T(" = ") << YCR(details[0]) << _T("(") << DEFL(_T("girder")) << _T(" + ") << DEFL(_T("ps")) << _T(" ") << Sub2(_T("L"),_T("s")) << _T(" + ") << DEFL(_T("tpsi")) << _T(")") << rptNewLine;

      *pPara << pTable2 << rptNewLine;
      *pPara << DEFL(_T("creep2")) << _T(" = ") << _T("[") << YCR(details[1]);
      *pPara << _T(" - ") << YCR(details[0]) << _T("]") << _T("(") << DEFL(_T("girder")) << _T(" + ") << DEFL(_T("ps")) << _T(" ") << Sub2(_T("L"),_T("s")) << _T(" + ") << DEFL(_T("tpsi")) << _T(")");
      *pPara << _T(" + ") << YCR(details[2]) << _T("(") << DEFL(_T("diaphragm")) << _T(" + ") << DEFL(_T("tpsr")) << _T(")") << rptNewLine;

      *pPara << pTable3 << rptNewLine;
      *pPara << DEFL(_T("1")) << _T(" = ") << DEFL(_T("girder")) << _T(" + ") << DEFL(_T("ps")) << _T(" ") << Sub2(_T("L"),_T("s")) << _T(" + ") << DEFL(_T("tpsi")) << rptNewLine;
      *pPara << DEFL(_T("2")) << _T(" = ") << DEFL(_T("1")) << _T(" + ") << DEFL(_T("creep1")) << rptNewLine;
      *pPara << DEFL(_T("3")) << _T(" = ") << DEFL(_T("2")) << _T(" + ") << DEFL(_T("diaphragm")) << _T(" + ") << DEFL(_T("tpsr")) << rptNewLine;
      *pPara << DEFL(_T("4")) << _T(" = ") << DEFL(_T("3")) << _T(" + ") << DEFL(_T("creep2")) << rptNewLine;
      *pPara << DEFL(_T("5")) << _T(" = ") << DEFL(_T("4")) << _T(" + ") << DEFL(_T("deck")) << _T(" + ") << DEFL(_T("user1")) << rptNewLine;

      *pPara << DEFL(_T("6")) << _T(" = ") << DEFL(_T("5"));
      if ( bSidewalk )
         *pPara << _T(" + ") << DEFL(_T("sidewalk"));

      *pPara << _T(" + ") << DEFL(_T("barrier"));

      if ( !pBridge->IsFutureOverlay() )
         *pPara << _T(" + ") << DEFL(_T("overlay"));

      *pPara << _T(" + ") << DEFL(_T("user2")) << rptNewLine;

      *pPara << rptNewLine;
   }
}

void CCamberChapterBuilder::Build_CIP(rptChapter* pChapter,CReportSpecification* pRptSpec,IBroker* pBroker,const CSegmentKey& segmentKey,IEAFDisplayUnits* pDisplayUnits,Uint16 level) const
{
   GET_IFACE2(pBroker,ICamber,pCamber);

   GET_IFACE2(pBroker,IBridge,pBridge);
   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();
   bool bDeckPanels = (deckType == pgsTypes::sdtCompositeSIP ? true : false);

   GET_IFACE2(pBroker,IProductLoads,pProductLoads);
   bool bSidewalk = pProductLoads->HasSidewalkLoad(segmentKey);

   INIT_UV_PROTOTYPE( rptTimeUnitValue, time1, pDisplayUnits->GetLongTimeUnit(), false );
   INIT_UV_PROTOTYPE( rptTimeUnitValue, time2, pDisplayUnits->GetLongTimeUnit(), false );

   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   if ( pBridge->IsFutureOverlay() )
      *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("Camber_CIP_FutureOverlay.gif")) << rptNewLine;
   else
      *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("Camber_CIP.gif")) << rptNewLine;

   for ( Int16 i = CREEP_MINTIME; i <= CREEP_MAXTIME; i++ )
   {
      pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      *pChapter << pPara;

      *pPara << (i == CREEP_MINTIME ? _T("Minimum Timing") : _T("Maximum Timing")) << rptNewLine;

      pPara = new rptParagraph;
      *pChapter << pPara;

      CREEPCOEFFICIENTDETAILS details[2];
      details[0] = pCamber->GetCreepCoefficientDetails(segmentKey,ICamber::cpReleaseToDeck,i);
      details[1] = pCamber->GetCreepCoefficientDetails(segmentKey,ICamber::cpDiaphragmToDeck,i);

      CCamberTable tbl;
      rptRcTable* pTable1, *pTable2, *pTable3;
      tbl.Build_CIP(pBroker,segmentKey,pDisplayUnits,i,&pTable1,&pTable2,&pTable3);
      *pPara << pTable1 << rptNewLine;

      pPara = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
      *pChapter << pPara;
      *pPara << Sub2(symbol(DELTA),_T("ps")) << _T(" ") << Sub2(_T("L"),_T("g")) << _T(" is based the girder length and measured relative to the end of the girder") << rptNewLine;
      *pPara << Sub2(symbol(DELTA),_T("ps")) << _T(" ") << Sub2(_T("L"),_T("s")) << _T(" is based the final span length and measured relative to the final bearing location") << rptNewLine;
      *pPara << Sub2(symbol(DELTA),_T("girder")) << _T(" is based on final span length and ") << RPT_ECI << rptNewLine;
      *pPara << DEFL(_T("creep")) << _T(" = ") << YCR(details[0]) << _T("(") << DEFL(_T("girder")) << _T(" + ") << DEFL(_T("ps")) << _T(" ") << Sub2(_T("L"),_T("s")) << _T(")") << rptNewLine;

      *pPara << pTable2 << rptNewLine;

      *pPara << pTable3 << rptNewLine;
      *pPara << DEFL(_T("1")) << _T(" = ") << DEFL(_T("girder")) << _T(" + ") << DEFL(_T("ps")) << _T(" ") << Sub2(_T("L"),_T("s")) << rptNewLine;
      *pPara << DEFL(_T("2")) << _T(" = ") << DEFL(_T("1")) << _T(" + ") << DEFL(_T("creep")) << rptNewLine;
      *pPara << DEFL(_T("3")) << _T(" = ") << DEFL(_T("2")) << _T(" + ") << DEFL(_T("diaphragm"))<< _T(" + ") << DEFL(_T("deck")) << _T(" + ") << DEFL(_T("user1")) << rptNewLine;
      *pPara << DEFL(_T("4")) << _T(" = ") << DEFL(_T("3"));

      if ( bSidewalk )
         *pPara << _T(" + ") << DEFL(_T("sidewalk"));

      *pPara << _T(" + ") << DEFL(_T("barrier"));

      if ( !pBridge->IsFutureOverlay() )
         *pPara << _T(" + ") << DEFL(_T("overlay"));
      
      *pPara << _T(" + ") << DEFL(_T("user2")) << rptNewLine;


      *pPara << rptNewLine;
   }
}

void CCamberChapterBuilder::Build_SIP_TempStrands(rptChapter* pChapter,CReportSpecification* pRptSpec,IBroker* pBroker,const CSegmentKey& segmentKey,IEAFDisplayUnits* pDisplayUnits,Uint16 level) const
{
   GET_IFACE2(pBroker,ICamber,pCamber);

   GET_IFACE2(pBroker,IBridge,pBridge);
   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();
   bool bDeckPanels = (deckType == pgsTypes::sdtCompositeSIP ? true : false);

   GET_IFACE2(pBroker,IProductLoads,pProductLoads);
   bool bSidewalk = pProductLoads->HasSidewalkLoad(segmentKey);

   INIT_UV_PROTOTYPE( rptTimeUnitValue, time1, pDisplayUnits->GetLongTimeUnit(), false );
   INIT_UV_PROTOTYPE( rptTimeUnitValue, time2, pDisplayUnits->GetLongTimeUnit(), false );

   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   if ( pBridge->IsFutureOverlay() )
      *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("Camber_SIP_TempStrands_FutureOverlay.gif")) << rptNewLine;
   else
      *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("Camber_SIP_TempStrands.gif")) << rptNewLine;

   for ( Int16 i = CREEP_MINTIME; i <= CREEP_MAXTIME; i++ )
   {
      pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      *pChapter << pPara;

      *pPara << (i == CREEP_MINTIME ? _T("Minimum Timing") : _T("Maximum Timing")) << rptNewLine;

      pPara = new rptParagraph;
      *pChapter << pPara;

      CREEPCOEFFICIENTDETAILS details[3];
      details[0] = pCamber->GetCreepCoefficientDetails(segmentKey,ICamber::cpReleaseToDiaphragm,i);
      details[1] = pCamber->GetCreepCoefficientDetails(segmentKey,ICamber::cpReleaseToDeck,i);
      details[2] = pCamber->GetCreepCoefficientDetails(segmentKey,ICamber::cpDiaphragmToDeck,i);

      CCamberTable tbl;
      rptRcTable* pTable1, *pTable2, *pTable3;
      tbl.Build_SIP_TempStrands(pBroker,segmentKey,pDisplayUnits,i,&pTable1,&pTable2,&pTable3);
      *pPara << pTable1 << rptNewLine;

      pPara = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
      *pChapter << pPara;
      *pPara << Sub2(symbol(DELTA),_T("ps")) << _T(" ") << Sub2(_T("L"),_T("g")) << _T(" is based the girder length and measured relative to the end of the girder") << rptNewLine;
      *pPara << Sub2(symbol(DELTA),_T("ps")) << _T(" ") << Sub2(_T("L"),_T("s")) << _T(" is based the final span length and measured relative to the final bearing location") << rptNewLine;
      *pPara << Sub2(symbol(DELTA),_T("girder")) << _T(" is based on final span length and ") << RPT_ECI << rptNewLine;
      *pPara << DEFL(_T("creep1")) << _T(" = ") << YCR(details[0]) << _T("(") << DEFL(_T("girder")) << _T(" + ") << DEFL(_T("ps")) << _T(" ") << Sub2(_T("L"),_T("s")) << _T(" + ") << DEFL(_T("tpsi")) << _T(")") << rptNewLine;

      *pPara << pTable2 << rptNewLine;
      *pPara << DEFL(_T("creep2")) << _T(" = ") << _T("[") << YCR(details[1]);
      *pPara << _T(" - ") << YCR(details[0]) << _T("]") << _T("(") << DEFL(_T("girder")) << _T(" + ") << DEFL(_T("ps")) << _T(" ") << Sub2(_T("L"),_T("s")) << _T(" + ") << DEFL(_T("tpsi")) << _T(")");
      *pPara << _T(" + ") << YCR(details[2]) << _T("(") << DEFL(_T("diaphragm")) << _T(" + ") << DEFL(_T("tpsr")) << _T(")") << rptNewLine;

      *pPara << pTable3 << rptNewLine;
      *pPara << DEFL(_T("1")) << _T(" = ") << DEFL(_T("girder")) << _T(" + ") << DEFL(_T("ps")) << _T(" ") << Sub2(_T("L"),_T("s")) << _T(" + ") << DEFL(_T("tpsi")) << rptNewLine;
      *pPara << DEFL(_T("2")) << _T(" = ") << DEFL(_T("1")) << _T(" + ") << DEFL(_T("creep1")) << rptNewLine;
      *pPara << DEFL(_T("3")) << _T(" = ") << DEFL(_T("2")) << _T(" + ") << DEFL(_T("diaphragm")) << _T(" + ") << DEFL(_T("tpsr")) << _T(" + ") << DEFL(_T("panels"))  << rptNewLine;
      *pPara << DEFL(_T("4")) << _T(" = ") << DEFL(_T("3")) << _T(" + ") << DEFL(_T("creep2")) << rptNewLine;
      *pPara << DEFL(_T("5")) << _T(" = ") << DEFL(_T("4"))<< _T(" + ") << DEFL(_T("deck")) << _T(" + ") << DEFL(_T("user1")) << rptNewLine;
      *pPara << DEFL(_T("6")) << _T(" = ") << DEFL(_T("5"));

      if ( bSidewalk )
         *pPara << _T(" + ") << DEFL(_T("sidewalk"));
      
      *pPara << _T(" + ") << DEFL(_T("barrier"));
   
      if ( !pBridge->IsFutureOverlay() )
         *pPara << _T(" + ") << DEFL(_T("overlay")) << rptNewLine;

      *pPara << _T(" + ") << DEFL(_T("user2")) << rptNewLine;

      *pPara << rptNewLine;
   }
}

void CCamberChapterBuilder::Build_SIP(rptChapter* pChapter,CReportSpecification* pRptSpec,IBroker* pBroker,const CSegmentKey& segmentKey,IEAFDisplayUnits* pDisplayUnits,Uint16 level) const
{
   GET_IFACE2(pBroker,ICamber,pCamber);

   GET_IFACE2(pBroker,IBridge,pBridge);
   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();
   bool bDeckPanels = (deckType == pgsTypes::sdtCompositeSIP ? true : false);

   GET_IFACE2(pBroker,IProductLoads,pProductLoads);
   bool bSidewalk = pProductLoads->HasSidewalkLoad(segmentKey);

   INIT_UV_PROTOTYPE( rptTimeUnitValue, time1, pDisplayUnits->GetLongTimeUnit(), false );
   INIT_UV_PROTOTYPE( rptTimeUnitValue, time2, pDisplayUnits->GetLongTimeUnit(), false );

   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;
   if (pBridge->IsFutureOverlay() )
      *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("Camber_SIP.gif")) << rptNewLine;
   else
      *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("Camber_SIP_FutureOverlay.gif")) << rptNewLine;

   for ( Int16 i = CREEP_MINTIME; i <= CREEP_MAXTIME; i++ )
   {
      pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      *pChapter << pPara;

      *pPara << (i == CREEP_MINTIME ? _T("Minimum Timing") : _T("Maximum Timing")) << rptNewLine;

      pPara = new rptParagraph;
      *pChapter << pPara;

      CREEPCOEFFICIENTDETAILS details[2];
      details[0] = pCamber->GetCreepCoefficientDetails(segmentKey,ICamber::cpReleaseToDeck,i);
      details[1] = pCamber->GetCreepCoefficientDetails(segmentKey,ICamber::cpDiaphragmToDeck,i);

      CCamberTable tbl;
      rptRcTable* pTable1, *pTable2, *pTable3;
      tbl.Build_SIP(pBroker,segmentKey,pDisplayUnits,i,&pTable1,&pTable2,&pTable3);
      *pPara << pTable1 << rptNewLine;

      pPara = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
      *pChapter << pPara;
      *pPara << Sub2(symbol(DELTA),_T("ps")) << _T(" ") << Sub2(_T("L"),_T("g")) << _T(" is based the girder length and measured relative to the end of the girder") << rptNewLine;
      *pPara << Sub2(symbol(DELTA),_T("ps")) << _T(" ") << Sub2(_T("L"),_T("s")) << _T(" is based the final span length and measured relative to the final bearing location") << rptNewLine;
      *pPara << Sub2(symbol(DELTA),_T("girder")) << _T(" is based on final span length and ") << RPT_ECI << rptNewLine;
      *pPara << DEFL(_T("creep")) << _T(" = ") << YCR(details[0]) << _T("(") << DEFL(_T("girder")) << _T(" + ") << DEFL(_T("ps")) << _T(" ") << Sub2(_T("L"),_T("s")) << _T(")") << rptNewLine;

      *pPara << pTable2 << rptNewLine;

      *pPara << pTable3 << rptNewLine;
      *pPara << DEFL(_T("1")) << _T(" = ") << DEFL(_T("girder")) << _T(" + ") << DEFL(_T("ps")) << _T(" ") << Sub2(_T("L"),_T("s")) << rptNewLine;
      *pPara << DEFL(_T("2")) << _T(" = ") << DEFL(_T("1")) << _T(" + ") << DEFL(_T("creep")) << rptNewLine;
      *pPara << DEFL(_T("3")) << _T(" = ") << DEFL(_T("2")) << _T(" + ") << DEFL(_T("diaphragm")) << _T(" + ") << DEFL(_T("panel")) << rptNewLine;
      *pPara << DEFL(_T("4")) << _T(" = ") << DEFL(_T("3")) << _T(" + ") << DEFL(_T("deck")) << _T(" + ") << DEFL(_T("user1"));
      
      if ( bSidewalk )
         *pPara << _T(" + ") << DEFL(_T("sidewalk"));

      *pPara << _T(" + ") << DEFL(_T("barrier"));

      if ( !pBridge->IsFutureOverlay() )
         *pPara << _T(" + ") << DEFL(_T("overlay")) << rptNewLine;

      *pPara << _T(" + ") << DEFL(_T("user2")) << rptNewLine;

      *pPara << rptNewLine;
   }
}

void CCamberChapterBuilder::Build_NoDeck_TempStrands(rptChapter* pChapter,CReportSpecification* pRptSpec,IBroker* pBroker,const CSegmentKey& segmentKey,IEAFDisplayUnits* pDisplayUnits,Uint16 level) const
{
   GET_IFACE2(pBroker,ICamber,pCamber);

   GET_IFACE2(pBroker,IBridge,pBridge);
   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();
   bool bDeckPanels = (deckType == pgsTypes::sdtCompositeSIP ? true : false);

   GET_IFACE2(pBroker,IProductLoads,pProductLoads);
   bool bSidewalk = pProductLoads->HasSidewalkLoad(segmentKey);

   INIT_UV_PROTOTYPE( rptTimeUnitValue, time1, pDisplayUnits->GetLongTimeUnit(), false );
   INIT_UV_PROTOTYPE( rptTimeUnitValue, time2, pDisplayUnits->GetLongTimeUnit(), false );

   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;
   if ( pBridge->IsFutureOverlay() )
      *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("Camber_NoDeck_TempStrands_FutureOverlay.gif")) << rptNewLine;
   else
      *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("Camber_NoDeck_TempStrands.gif")) << rptNewLine;

   for ( Int16 i = CREEP_MINTIME; i <= CREEP_MAXTIME; i++ )
   {
      pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      *pChapter << pPara;

      *pPara << (i == CREEP_MINTIME ? _T("Minimum Timing") : _T("Maximum Timing")) << rptNewLine;

      pPara = new rptParagraph;
      *pChapter << pPara;

      CREEPCOEFFICIENTDETAILS details[6];
      details[0] = pCamber->GetCreepCoefficientDetails(segmentKey,ICamber::cpReleaseToDiaphragm,i);
      details[1] = pCamber->GetCreepCoefficientDetails(segmentKey,ICamber::cpReleaseToDeck,     i);
      details[2] = pCamber->GetCreepCoefficientDetails(segmentKey,ICamber::cpReleaseToFinal,    i);
      details[3] = pCamber->GetCreepCoefficientDetails(segmentKey,ICamber::cpDiaphragmToDeck,   i);
      details[4] = pCamber->GetCreepCoefficientDetails(segmentKey,ICamber::cpDiaphragmToFinal,  i);
      details[5] = pCamber->GetCreepCoefficientDetails(segmentKey,ICamber::cpDeckToFinal,       i);

      CCamberTable tbl;
      rptRcTable* pTable1, *pTable2, *pTable3;
      tbl.Build_NoDeck_TempStrands(pBroker,segmentKey,pDisplayUnits,i,&pTable1,&pTable2,&pTable3);
      *pPara << pTable1 << rptNewLine;

      pPara = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
      *pChapter << pPara;
      *pPara << Sub2(symbol(DELTA),_T("ps")) << _T(" ") << Sub2(_T("L"),_T("g")) << _T(" is based the girder length and measured relative to the end of the girder") << rptNewLine;
      *pPara << Sub2(symbol(DELTA),_T("ps")) << _T(" ") << Sub2(_T("L"),_T("s")) << _T(" is based the final span length and measured relative to the final bearing location") << rptNewLine;
      *pPara << Sub2(symbol(DELTA),_T("girder")) << _T(" is based on final span length and ") << RPT_ECI << rptNewLine;
      *pPara << DEFL(_T("creep1")) << _T(" = ") << YCR(details[0]) << _T("(") << DEFL(_T("girder")) << _T(" + ") << DEFL(_T("ps")) << _T(" ") << Sub2(_T("L"),_T("s")) << _T(" + ") << DEFL(_T("tpsi")) << _T(")") << rptNewLine;

      *pPara << pTable2 << rptNewLine;
      *pPara << DEFL(_T("creep2")) << _T(" = ") << _T("[") << YCR(details[1]);
      *pPara << _T(" - ") << YCR(details[0]) << _T("]") << _T("(") << DEFL(_T("girder")) << _T(" + ") << DEFL(_T("ps")) << _T(" ") << Sub2(_T("L"),_T("s")) << _T(")");
      *pPara << _T(" + ") << YCR(details[3]) << _T("(") << DEFL(_T("diaphragm")) << _T(" + ") << DEFL(_T("tpsr")) << _T(")") << rptNewLine;
      
      *pPara << rptNewLine;

      *pPara << DEFL(_T("creep3")) << _T(" = ") << _T("[") << YCR(details[2]);
      *pPara << _T(" - ") << YCR(details[1]) << _T("]") << _T("(") << DEFL(_T("girder")) << _T(" + ") << DEFL(_T("ps")) << _T(" ") << Sub2(_T("L"),_T("s")) << _T(")");
      *pPara << _T(" + [") << YCR(details[4]);
      *pPara << _T(" - ") << YCR(details[3]) << _T("](") << DEFL(_T("diaphragm")) << _T(" + ") << DEFL(_T("tpsr")) << _T(")");
      *pPara << _T(" + ") << YCR(details[5]) << _T("(") << DEFL(_T("barrier")) << _T(" + ") << DEFL(_T("overlay")) << _T(" + ") << DEFL(_T("user2")) << _T(")") << rptNewLine;

      *pPara << pTable3 << rptNewLine;
      *pPara << DEFL(_T("1")) << _T(" = ") << DEFL(_T("girder")) << _T(" + ") << DEFL(_T("ps")) << _T(" ") << Sub2(_T("L"),_T("s")) << _T(" + ") << DEFL(_T("tpsi")) << rptNewLine;
      *pPara << DEFL(_T("2")) << _T(" = ") << DEFL(_T("1")) << _T(" + ") << DEFL(_T("creep1")) << rptNewLine;
      *pPara << DEFL(_T("3")) << _T(" = ") << DEFL(_T("2")) << _T(" + ") << DEFL(_T("diaphragm")) << _T(" + ") << DEFL(_T("tpsr"))  << _T(" + ") << DEFL(_T("user1")) << rptNewLine;
      *pPara << DEFL(_T("4")) << _T(" = ") << DEFL(_T("3")) << _T(" + ") << DEFL(_T("creep2")) << rptNewLine;
      *pPara << DEFL(_T("5")) << _T(" = ") << DEFL(_T("4"));
      
      if ( bSidewalk )
         *pPara << _T(" + ") << DEFL(_T("sidewalk"));

      *pPara << _T(" + ") << DEFL(_T("barrier"));

      if ( !pBridge->IsFutureOverlay() )
         *pPara << _T(" + ") << DEFL(_T("overlay"));

      *pPara << _T(" + ") << DEFL(_T("user2")) << rptNewLine;

      *pPara << DEFL(_T("6")) << _T(" = ") << DEFL(_T("5")) << _T(" + ") << DEFL(_T("creep3")) << rptNewLine;

      *pPara << rptNewLine;
   }
}

void CCamberChapterBuilder::Build_NoDeck(rptChapter* pChapter,CReportSpecification* pRptSpec,IBroker* pBroker,const CSegmentKey& segmentKey,IEAFDisplayUnits* pDisplayUnits,Uint16 level) const
{
   GET_IFACE2(pBroker,ICamber,pCamber);

   GET_IFACE2(pBroker,IBridge,pBridge);
   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();
   bool bDeckPanels = (deckType == pgsTypes::sdtCompositeSIP ? true : false);

   INIT_UV_PROTOTYPE( rptTimeUnitValue, time1, pDisplayUnits->GetLongTimeUnit(), false );
   INIT_UV_PROTOTYPE( rptTimeUnitValue, time2, pDisplayUnits->GetLongTimeUnit(), false );

   GET_IFACE2(pBroker,IProductLoads,pProductLoads);
   bool bSidewalk = pProductLoads->HasSidewalkLoad(segmentKey);

   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   if ( pBridge->IsFutureOverlay() )
      *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("Camber_NoDeck_FutureOverlay.gif")) << rptNewLine;
   else
      *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("Camber_NoDeck.gif")) << rptNewLine;

   for ( Int16 i = CREEP_MINTIME; i <= CREEP_MAXTIME; i++ )
   {
      pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      *pChapter << pPara;

      *pPara << (i == CREEP_MINTIME ? _T("Minimum Timing") : _T("Maximum Timing")) << rptNewLine;

      pPara = new rptParagraph;
      *pChapter << pPara;

      CREEPCOEFFICIENTDETAILS details[6];
      details[0] = pCamber->GetCreepCoefficientDetails(segmentKey,ICamber::cpReleaseToDiaphragm,i);
      details[1] = pCamber->GetCreepCoefficientDetails(segmentKey,ICamber::cpReleaseToDeck,     i);
      details[2] = pCamber->GetCreepCoefficientDetails(segmentKey,ICamber::cpReleaseToFinal,    i);
      details[3] = pCamber->GetCreepCoefficientDetails(segmentKey,ICamber::cpDiaphragmToDeck,   i);
      details[4] = pCamber->GetCreepCoefficientDetails(segmentKey,ICamber::cpDiaphragmToFinal,  i);
      details[5] = pCamber->GetCreepCoefficientDetails(segmentKey,ICamber::cpDeckToFinal,       i);

      CCamberTable tbl;
      rptRcTable* pTable1, *pTable2, *pTable3;
      tbl.Build_NoDeck(pBroker,segmentKey,pDisplayUnits,i,&pTable1,&pTable2,&pTable3);
      *pPara << pTable1 << rptNewLine;

      pPara = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
      *pChapter << pPara;
      *pPara << Sub2(symbol(DELTA),_T("ps")) << _T(" ") << Sub2(_T("L"),_T("g")) << _T(" is based the girder length and measured relative to the end of the girder") << rptNewLine;
      *pPara << Sub2(symbol(DELTA),_T("ps")) << _T(" ") << Sub2(_T("L"),_T("s")) << _T(" is based the final span length and measured relative to the final bearing location") << rptNewLine;
      *pPara << Sub2(symbol(DELTA),_T("girder")) << _T(" is based on final span length and ") << RPT_ECI << rptNewLine;
      *pPara << DEFL(_T("creep1")) << _T(" = ") << YCR(details[0]) << _T("(") << DEFL(_T("girder")) << _T(" + ") << DEFL(_T("ps")) << _T(" ") << Sub2(_T("L"),_T("s")) << _T(")") << rptNewLine;

      *pPara << pTable2 << rptNewLine;
      *pPara << DEFL(_T("creep2")) << _T(" = ") << _T("[") << YCR(details[1]);
      *pPara << _T(" - ") << YCR(details[0]) << _T("]") << _T("(") << DEFL(_T("girder")) << _T(" + ") << DEFL(_T("ps")) << _T(" ") << Sub2(_T("L"),_T("s")) << _T(")");
      *pPara << _T(" + ") << YCR(details[3]) << _T("(") << DEFL(_T("diaphragm")) << _T(" + ") << DEFL(_T("user1")) << _T(")") << rptNewLine;
      
      *pPara << rptNewLine;

      *pPara << DEFL(_T("creep3")) << _T(" = ") << _T("[") << YCR(details[2]);
      *pPara << _T(" - ") << YCR(details[1]) << _T("]") << _T("(") << DEFL(_T("girder")) << _T(" + ") << DEFL(_T("ps")) << _T(" ") << Sub2(_T("L"),_T("s")) << _T(")");
      *pPara << _T(" + [") << YCR(details[4]);
      *pPara << _T(" - ") << YCR(details[3]) << _T("](") << DEFL(_T("diaphragm")) << _T(" + ") << DEFL(_T("user1")) << _T(")");
      *pPara << _T(" + ") << YCR(details[5]) << _T("(") << DEFL(_T("barrier")) << _T(" + ") << DEFL(_T("overlay")) << _T(" + ") << DEFL(_T("user2")) << _T(")") << rptNewLine;

      *pPara << pTable3 << rptNewLine;
      *pPara << DEFL(_T("1")) << _T(" = ") << DEFL(_T("girder")) << _T(" + ") << DEFL(_T("ps")) << _T(" ") << Sub2(_T("L"),_T("s")) << rptNewLine;
      *pPara << DEFL(_T("2")) << _T(" = ") << DEFL(_T("1")) << _T(" + ") << DEFL(_T("creep1")) << rptNewLine;
      *pPara << DEFL(_T("3")) << _T(" = ") << DEFL(_T("2")) << _T(" + ") << DEFL(_T("diaphragm")) << _T(" + ") << DEFL(_T("user1")) << rptNewLine;
      *pPara << DEFL(_T("4")) << _T(" = ") << DEFL(_T("3")) << _T(" + ") << DEFL(_T("creep2")) << rptNewLine;
      *pPara << DEFL(_T("5")) << _T(" = ") << DEFL(_T("4"));

      if ( bSidewalk )
         *pPara << _T(" + ") << DEFL(_T("sidewalk"));

      *pPara << _T(" + ") << DEFL(_T("barrier"));

      if ( !pBridge->IsFutureOverlay() )
         *pPara << _T(" + ") << DEFL(_T("overlay"));

      *pPara << _T(" + ") << DEFL(_T("user2")) << rptNewLine;

      *pPara << DEFL(_T("6")) << _T(" = ") << DEFL(_T("5")) << _T(" + ") << DEFL(_T("creep3")) << rptNewLine;

      *pPara << rptNewLine;
   }
}
