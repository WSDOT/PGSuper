///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2020  Washington State Department of Transportation
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
#include <Reporting\BasicCamberChapterBuilder.h>
#include <Reporting\CamberTable.h>

#include <WBFLTools.h> // not sure why, but this is needed to compile

#include <IFace\AnalysisResults.h>
#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\Intervals.h>

#include <PsgLib\SpecLibraryEntry.h>

#include <PgsExt\StrandData.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#define YCR(_details_) symbol(psi) << _T("(") << time1.SetValue(_details_.t) << _T(",") << time2.SetValue(_details_.ti) << _T(")")
#define DEFL(_f_) Sub2(symbol(DELTA),_f_)
#define SCL(_s_) scalar.SetValue(_s_)

/****************************************************************************
CLASS
   CBasicCamberChapterBuilder
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CBasicCamberChapterBuilder::CBasicCamberChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CBasicCamberChapterBuilder::GetName() const
{
   return TEXT("Camber Details");
}

rptChapter* CBasicCamberChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CGirderReportSpecification* pGirderRptSpec = dynamic_cast<CGirderReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pGirderRptSpec->GetBroker(&pBroker);
   const CGirderKey& girderKey(pGirderRptSpec->GetGirderKey());

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker,ISegmentData,pSegmentData);
   GET_IFACE2(pBroker,IBridge,pBridge);
   INIT_SCALAR_PROTOTYPE(rptRcScalar, scalar, pDisplayUnits->GetScalarFormat());

   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      CSegmentKey segmentKey(girderKey,segIdx);

      const CStrandData* pStrands = pSegmentData->GetStrandData(segmentKey);
      bool bTempStrands = ( 0 < pStrands->GetStrandCount(pgsTypes::Temporary) && pStrands->GetTemporaryStrandUsage() != pgsTypes::ttsPTBeforeShipping );

      switch( deckType )
      {
      case pgsTypes::sdtCompositeCIP:
      case pgsTypes::sdtCompositeOverlay:
      case pgsTypes::sdtCompositeSIP:
         Build_Deck(pChapter,pRptSpec,pBroker,segmentKey,bTempStrands,pDisplayUnits,level);
         break;

      case pgsTypes::sdtNone:
      case pgsTypes::sdtNonstructuralOverlay:
         Build_NoDeck(pChapter,pRptSpec,pBroker,segmentKey,bTempStrands,pDisplayUnits,level);
         break;

      default:
         ATLASSERT(false);
      }
   }

   return pChapter;
}

CChapterBuilder* CBasicCamberChapterBuilder::Clone() const
{
   return new CBasicCamberChapterBuilder;
}

void CBasicCamberChapterBuilder::Build_Deck(rptChapter* pChapter,CReportSpecification* pRptSpec,IBroker* pBroker,const CSegmentKey& segmentKey, bool bTempStrands, IEAFDisplayUnits* pDisplayUnits,Uint16 level) const
{
   GET_IFACE2(pBroker,ICamber,pCamber);

   GET_IFACE2(pBroker,IBridge,pBridge);
   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();
   bool bDeckPanels = (deckType == pgsTypes::sdtCompositeSIP ? true : false);

   GET_IFACE2(pBroker,IProductLoads,pProductLoads);
   bool bSidewalk = pProductLoads->HasSidewalkLoad(segmentKey);
   bool bShearKey = pProductLoads->HasShearKeyLoad(segmentKey);
   bool bConstruction = pProductLoads->HasConstructionLoad(segmentKey);
   bool bOverlay  = pBridge->HasOverlay() && !pBridge->IsFutureOverlay();

   INIT_UV_PROTOTYPE( rptTimeUnitValue, time1, pDisplayUnits->GetWholeDaysUnit(), false );
   INIT_UV_PROTOTYPE( rptTimeUnitValue, time2, pDisplayUnits->GetWholeDaysUnit(), false );

   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   if (bDeckPanels)
   {
      // SIP
      if (bTempStrands)
      {
         if ( pBridge->IsFutureOverlay() )
         {
            *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("Camber_SIP_TempStrands_FutureOverlay.gif")) << rptNewLine;
         }
         else
         {
            *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("Camber_SIP_TempStrands.gif")) << rptNewLine;
         }
      }
      else
      {
         if (pBridge->IsFutureOverlay() )
         {
            *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("Camber_SIP_FutureOverlay.gif")) << rptNewLine;
         }
         else
         {
            *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("Camber_SIP.gif")) << rptNewLine;
         }
      }
   }
   else
   {
      // CIP
      if (bTempStrands)
      {
         if ( pBridge->IsFutureOverlay() )
         {
            *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("Camber_CIP_TempStrands_FutureOverlay.gif")) << rptNewLine;
         }
         else
         {
            *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("Camber_CIP_TempStrands.gif")) << rptNewLine;
         }
      }
      else
      {
         if ( pBridge->IsFutureOverlay() )
         {
            *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("Camber_CIP_FutureOverlay.gif")) << rptNewLine;
         }
         else
         {
            *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("Camber_CIP.gif")) << rptNewLine;
         }
      }
   }

   // Camber multipliers for table 3
   CamberMultipliers cm = pCamber->GetCamberMultipliers(segmentKey);

   Float64 precamber = pCamber->GetPrecamber(segmentKey);

   for ( Int16 i = CREEP_MINTIME; i <= CREEP_MAXTIME; i++ )
   {
      pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
      *pChapter << pPara;

      *pPara << (i == CREEP_MINTIME ? _T("Minimum Timing") : _T("Maximum Timing")) << rptNewLine;

      pPara = new rptParagraph;
      *pChapter << pPara;

      CREEPCOEFFICIENTDETAILS details[3];
      details[0] = pCamber->GetCreepCoefficientDetails(segmentKey,ICamber::cpReleaseToDiaphragm,i);
      details[1] = pCamber->GetCreepCoefficientDetails(segmentKey,ICamber::cpDiaphragmToDeck,i);
      details[2] = pCamber->GetCreepCoefficientDetails(segmentKey,ICamber::cpReleaseToDeck,i);

      CCamberTable tbl;
      rptRcTable* pTable1, *pTable2, *pTable3;
      tbl.Build_Deck(pBroker,segmentKey,bTempStrands,bSidewalk,bShearKey,bConstruction,bOverlay,bDeckPanels,pDisplayUnits,i,cm,&pTable1,&pTable2,&pTable3);
      *pPara << pTable1 << rptNewLine;

      // footnotes to release and storage tables
      if (IsZero(precamber))
      {
         (*pTable1)(0, 0) << DEFL(_T("i")) << _T(" = ") << DEFL(_T("girder")) << _T(" + ") << DEFL(_T("ps")) << _T(" = Camber immediately after prestress release") << rptNewLine;
      }
      else
      {
         (*pTable1)(0, 0) << DEFL(_T("i")) << _T(" = ") << DEFL(_T("girder")) << _T(" + ") << DEFL(_T("ps")) << _T(" + ") << DEFL(_T("precamber")) << _T(" = Camber immediately after prestress release") << rptNewLine;
      }
      (*pTable1)(0,1) << DEFL(_T("creep1")) << _T(" = ") << YCR(details[0]) << _T("(") << DEFL(_T("girder Storage")) << _T(" + ") << DEFL(_T("ps Storage")) << _T(")") << rptNewLine;
      if (IsZero(precamber))
      {
         (*pTable1)(0, 1) << DEFL(_T("es")) << _T(" = ") << DEFL(_T("girder")) << _T(" + ") << DEFL(_T("ps"))<< _T(" + ") << DEFL(_T("creep1")) << _T(" = Camber at end of storage = Camber at shipping") << rptNewLine;
      }
      else
      {
         (*pTable1)(0, 1) << DEFL(_T("es")) << _T(" = ") << DEFL(_T("girder")) << _T(" + ") << DEFL(_T("ps")) << _T(" + ") << DEFL(_T("precamber"))  << _T(" + ") << DEFL(_T("creep1")) << _T(" = Camber at end of storage = Camber at shipping") << rptNewLine;
      }
      (*pTable1)(0,1) << _T("Rows with ") << Bold(_T("bold text")) << _T(" are at the support locations after erection") << rptNewLine;
      GET_IFACE2(pBroker, IIntervals, pIntervals);
      IntervalIndexType storageIntervalIdx = pIntervals->GetStorageInterval(segmentKey);
      Float64 start = pIntervals->GetTime(storageIntervalIdx, pgsTypes::Start);
      Float64 duration = ::ConvertFromSysUnits(details[0].t, unitMeasure::Day);
      Float64 end = start + duration;
      (*pTable1)(0, 1) << _T("Storage Duration: Start ") << start << _T(" day, End ") << end << _T(" day, Duration ") << duration << _T(" days") << rptNewLine;;

      pPara = new rptParagraph;
      *pChapter << pPara;
      *pPara << pTable2 << rptNewLine;

      pPara = new rptParagraph(rptStyleManager::GetFootnoteStyle());
      *pChapter << pPara;
      *pPara << DEFL(_T("creep1")) << _T(" = ") << YCR(details[0]) << _T("[(") << DEFL(_T("girder Storage")) << _T(" + ") << DEFL(_T("ps Storage")) << _T(") - (") << DEFL(_T("girder Storage")) << _T(" + ") << DEFL(_T("ps Storage")) << _T(" at location of erected segment supports)] ") << rptNewLine;
      *pPara << DEFL(_T("creep2")) << _T(" = ") << _T("[") << YCR(details[2]) << _T(" - ") << YCR(details[0]) << _T("][(") << DEFL(_T("girder Storage")) << _T(" + ") << DEFL(_T("ps Storage")) << _T(") - ") << _T("(") << DEFL(_T("girder Storage")) << _T(" + ") << DEFL(_T("ps Storage")) << _T(" at location of erected segment supports)] + ")
             << YCR(details[1]) << _T("(") << Sub2(symbol(delta),_T("girder"))  << _T(" + ") << DEFL(_T("diaphragm"));

      if (bConstruction)
      {
         *pPara << _T(" + ") << DEFL(_T("construction"));
      }

      if (bTempStrands)
      {
         *pPara << _T(" + ") << DEFL(_T("tpsr"));
      }

      if (bShearKey)
      {
         *pPara << _T(" + ") << DEFL(_T("shear key"));
      }
      *pPara << _T(")") << rptNewLine;

      *pPara << Sub2(symbol(delta),_T("girder")) << _T(" = girder deflection associated with change in dead load moment due to support location change between storage and erection") << rptNewLine;

      pPara = new rptParagraph;
      *pChapter << pPara;
      *pPara << pTable3 << rptNewLine;

      pPara = new rptParagraph(rptStyleManager::GetFootnoteStyle());
      *pChapter << pPara;

      // build table 3 footnotes
      *pPara << DEFL(_T("1")) << _T(" = ") << SCL(cm.ErectionFactor) << _T(" * (") << DEFL(_T("girder Erected")) << _T(" + ") << DEFL(_T("ps Erected")) << _T(")");
      if (!IsZero(precamber))
      {
         *pPara << _T(" + ") << DEFL(_T("precamber Erected"));
      }
      *pPara << rptNewLine;
      *pPara << DEFL(_T("2")) << _T(" = ") << DEFL(_T("1")) << _T(" + ") << SCL(cm.CreepFactor) << _T(" * ") << DEFL(_T("creep1")) << rptNewLine;

      *pPara << DEFL(_T("3")) << _T(" = ") << DEFL(_T("2")) << _T(" + ") << SCL(cm.DiaphragmFactor) << _T(" * (") << DEFL(_T("diaphragm"));
       if (bConstruction)
       {
          *pPara << _T(" + ") << DEFL(_T("construction"));
       }

       if (bShearKey)
       {
          *pPara << _T(" + ") << DEFL(_T("shear key"));
       }
      *pPara << _T(")");

       if (bTempStrands)
       {
          *pPara << _T(" + ") << SCL(cm.ErectionFactor) << _T(" * ") << DEFL(_T("tpsr"));
       }      
      
       *pPara << rptNewLine;

      *pPara << DEFL(_T("4")) << _T(" = ") << DEFL(_T("3")) << _T(" + ") << SCL(cm.CreepFactor) << _T(" * ") << DEFL(_T("creep2")) << rptNewLine;
      *pPara << DEFL(_T("5")) << _T(" = ") << DEFL(_T("4"));

       if (bDeckPanels)
       {
          *pPara << _T(" + ") << SCL(cm.ErectionFactor) << _T(" * ") << DEFL(_T("panels"));
       }

      *pPara << _T(" + ") << SCL(cm.SlabUser1Factor) << _T(" * (") << DEFL(_T("slab")) << _T(" + ") << DEFL(_T("user1")) << _T(")");
      *pPara << _T(" + ") << SCL(cm.SlabPadLoadFactor) << _T(" * ") << DEFL(_T("haunch")) << rptNewLine;

      *pPara << DEFL(_T("6")) << _T(" = ") << DEFL(_T("5"));

      *pPara << _T(" + ") << SCL(cm.BarrierSwOverlayUser2Factor) << _T(" * (") << DEFL(_T("barrier"));

      if ( bSidewalk )
      {
         *pPara << _T(" + ") << DEFL(_T("sidewalk"));
      }

      if ( bOverlay )
      {
         *pPara << _T(" + ") << DEFL(_T("overlay"));
      }

      *pPara << _T(" + ") << DEFL(_T("user2")) << _T(")");
      *pPara << _T(" = ") << Sub2(symbol(DELTA),_T("excess")) << _T(" = Computed Excess Camber");

      *pPara << rptNewLine;
   }
}

void CBasicCamberChapterBuilder::Build_NoDeck(rptChapter* pChapter,CReportSpecification* pRptSpec,IBroker* pBroker,const CSegmentKey& segmentKey,bool bTempStrands,IEAFDisplayUnits* pDisplayUnits,Uint16 level) const
{
   GET_IFACE2(pBroker,ICamber,pCamber);

   GET_IFACE2(pBroker,IBridge,pBridge);
   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();

   GET_IFACE2(pBroker,IProductLoads,pProductLoads);
   bool bSidewalk = pProductLoads->HasSidewalkLoad(segmentKey);
   bool bShearKey = pProductLoads->HasShearKeyLoad(segmentKey);
   bool bConstruction = pProductLoads->HasConstructionLoad(segmentKey);
   bool bOverlay  = pBridge->HasOverlay() && !pBridge->IsFutureOverlay();

   INIT_UV_PROTOTYPE( rptTimeUnitValue, time1, pDisplayUnits->GetWholeDaysUnit(), false );
   INIT_UV_PROTOTYPE( rptTimeUnitValue, time2, pDisplayUnits->GetWholeDaysUnit(), false );

   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   if (bTempStrands)
   {
      if ( pBridge->IsFutureOverlay() )
      {
         *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("Camber_NoDeck_TempStrands_FutureOverlay.gif")) << rptNewLine;
      }
      else
      {
         *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("Camber_NoDeck_TempStrands.gif")) << rptNewLine;
      }
   }
   else
   {
      if ( pBridge->IsFutureOverlay() )
      {
         *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("Camber_NoDeck_FutureOverlay.gif")) << rptNewLine;
      }
      else
      {
         *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("Camber_NoDeck.gif")) << rptNewLine;
      }
   }

   // Camber multipliers for table 3
   CamberMultipliers cm = pCamber->GetCamberMultipliers(segmentKey);
   
   Float64 precamber = pCamber->GetPrecamber(segmentKey);

   for (Int16 i = CREEP_MINTIME; i <= CREEP_MAXTIME; i++)
   {
      pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
      *pChapter << pPara;

      *pPara << (i == CREEP_MINTIME ? _T("Minimum Timing") : _T("Maximum Timing")) << rptNewLine;

      pPara = new rptParagraph;
      *pChapter << pPara;

      CREEPCOEFFICIENTDETAILS details[6];
      details[0] = pCamber->GetCreepCoefficientDetails(segmentKey, ICamber::cpReleaseToDiaphragm, i);
      details[1] = pCamber->GetCreepCoefficientDetails(segmentKey, ICamber::cpReleaseToDeck, i);
      details[2] = pCamber->GetCreepCoefficientDetails(segmentKey, ICamber::cpReleaseToFinal, i);
      details[3] = pCamber->GetCreepCoefficientDetails(segmentKey, ICamber::cpDiaphragmToDeck, i);
      details[4] = pCamber->GetCreepCoefficientDetails(segmentKey, ICamber::cpDiaphragmToFinal, i);
      details[5] = pCamber->GetCreepCoefficientDetails(segmentKey, ICamber::cpDeckToFinal, i);

      CCamberTable tbl;
      rptRcTable* pTable1, *pTable2, *pTable3;
      tbl.Build_NoDeck(pBroker, segmentKey, bTempStrands, bSidewalk, bShearKey, bConstruction, bOverlay, pDisplayUnits, i, cm, &pTable1, &pTable2, &pTable3);
      *pPara << pTable1 << rptNewLine;

      // footnotes to release and storage tables
      if (IsZero(precamber))
      {
         (*pTable1)(0, 0) << DEFL(_T("i")) << _T(" = ") << DEFL(_T("girder")) << _T(" + ") << DEFL(_T("ps")) << _T(" = Camber immediately after prestress release") << rptNewLine;
      }
      else
      {
         (*pTable1)(0, 0) << DEFL(_T("i")) << _T(" = ") << DEFL(_T("girder")) << _T(" + ") << DEFL(_T("ps")) << _T(" + ") << DEFL(_T("precamber")) << _T(" = Camber immediately after prestress release") << rptNewLine;
      }
      (*pTable1)(0, 1) << DEFL(_T("creep1")) << _T(" = ") << YCR(details[0]) << _T("(") << DEFL(_T("girder Storage")) << _T(" + ") << DEFL(_T("ps Storage")) << _T(")") << rptNewLine;
      if (IsZero(precamber))
      {
         (*pTable1)(0, 1) << DEFL(_T("es")) << _T(" = ") << DEFL(_T("girder")) << _T(" + ") << DEFL(_T("ps")) << _T(" + ") << DEFL(_T("creep1")) << _T(" = Camber at end of storage = Camber at shipping") << rptNewLine;
      }
      else
      {
         (*pTable1)(0, 1) << DEFL(_T("es")) << _T(" = ") << DEFL(_T("girder")) << _T(" + ") << DEFL(_T("ps")) << _T(" + ") << DEFL(_T("precamber")) << _T(" + ") << DEFL(_T("creep1")) << _T(" = Camber at end of storage = Camber at shipping") << rptNewLine;
      }
      (*pTable1)(0, 1) << _T("Rows with ") << Bold(_T("bold text")) << _T(" are at the support locations after erection") << rptNewLine;
      GET_IFACE2(pBroker, IIntervals, pIntervals);
      IntervalIndexType storageIntervalIdx = pIntervals->GetStorageInterval(segmentKey);
      Float64 start = pIntervals->GetTime(storageIntervalIdx, pgsTypes::Start);
      Float64 duration = ::ConvertFromSysUnits(details[0].t, unitMeasure::Day);
      Float64 end = start + duration;
      (*pTable1)(0, 1) << _T("Storage Duration: Start ") << start << _T(" day, End ") << end << _T(" day, Duration ") << duration << _T(" days") << rptNewLine;;

      pPara = new rptParagraph;
      *pChapter << pPara;
      *pPara << pTable2 << rptNewLine;

      pPara = new rptParagraph(rptStyleManager::GetFootnoteStyle());
      *pChapter << pPara;
      *pPara << DEFL(_T("creep1")) << _T(" = ") << YCR(details[0]) << _T("[(") << DEFL(_T("girder Storage")) << _T(" + ") << DEFL(_T("ps Storage")) << _T(") - (") << DEFL(_T("girder Storage")) << _T(" + ") << DEFL(_T("ps Storage")) << _T(" at location of erected segment supports)]") << rptNewLine;

      *pPara << DEFL(_T("creep2")) << _T(" = ") << _T("[") << YCR(details[1]);
      *pPara << _T(" - ") << YCR(details[0]) << _T("]") << _T("[(") << DEFL(_T("girder Storage")) << _T(" + ") << DEFL(_T("ps Storage")) << _T(") - (") << DEFL(_T("girder Storage")) << _T(" + ") << DEFL(_T("ps Storage")) << _T(" at location of erected segment supports)]");
      *pPara << _T(" + ") << YCR(details[3]) << _T("(") << Sub2(symbol(delta), _T("girder")) << _T(" + ") << DEFL(_T("diaphragm")) << _T(" + ") << DEFL(_T("user1"));
      if (bConstruction)
      {
         *pPara << _T(" + ") << DEFL(_T("construction"));
      }

      if (bTempStrands)
      {
         *pPara << _T(" + ") << DEFL(_T("tpsr"));
      }

      if (bShearKey)
      {
         *pPara << _T(" + ") << DEFL(_T("shear key"));
      }

      *pPara << _T(")") << rptNewLine;

      *pPara << DEFL(_T("creep3")) << _T(" = ") << _T("[") << YCR(details[2]);
      *pPara << _T(" - ") << YCR(details[1]) << _T("]") << _T("[(") << DEFL(_T("girder Storage")) << _T(" + ") << DEFL(_T("ps Storage")) << _T(") - (") << DEFL(_T("girder Storage")) << _T(" + ") << DEFL(_T("ps Storage")) << _T(" at location of erected segment supports)]");
      *pPara << _T(" + [") << YCR(details[4]);
      *pPara << _T(" - ") << YCR(details[3]) << _T("](") << Sub2(symbol(delta), _T("girder")) << _T(" + ") << DEFL(_T("diaphragm")) << _T(" + ") << DEFL(_T("user1"));
      if (bConstruction)
      {
         *pPara << _T(" + ") << DEFL(_T("construction"));
      }

      if (bTempStrands)
      {
         *pPara << _T(" + ") << DEFL(_T("tpsr"));
      }

      if (bShearKey)
      {
         *pPara << _T(" + ") << DEFL(_T("shear key"));
      }
      *pPara << _T(") + ") << YCR(details[5]) << _T("(") << DEFL(_T("barrier"));
      if (deckType == pgsTypes::sdtNonstructuralOverlay)
      {
         *pPara << _T(" + ") << DEFL(_T("slab")) << _T(" + ") << DEFL(_T("haunch"));
      }
      if (bSidewalk)
      {
         *pPara << _T(" + ") << DEFL(_T("sidewalk"));
      }
      if (bOverlay)
      {
         *pPara << _T(" + ") << DEFL(_T("overlay"));
      }
      *pPara << _T(" + ") << DEFL(_T("user2")) << _T(")") << rptNewLine;

      *pPara << Sub2(symbol(delta), _T("girder")) << _T(" = girder deflection associated with change in dead load moment due to support location change between storage and erection") << rptNewLine;

      pPara = new rptParagraph;
      *pChapter << pPara;
      *pPara << pTable3 << rptNewLine;

      pPara = new rptParagraph(rptStyleManager::GetFootnoteStyle());
      *pChapter << pPara;

      // build table 3 footnotes
      *pPara << DEFL(_T("1")) << _T(" = ") << SCL(cm.ErectionFactor) << _T(" * (") << DEFL(_T("girder Erected")) << _T(" + ") << DEFL(_T("ps Erected")) << _T(")");
      if (!IsZero(precamber))
      {
         *pPara << _T(" + ") << DEFL(_T("precamber Erected"));
      }
      *pPara << rptNewLine;
      *pPara << DEFL(_T("2")) << _T(" = ") << DEFL(_T("1")) << _T(" + ") << SCL(cm.CreepFactor) << _T(" * ") << DEFL(_T("creep1")) << rptNewLine;

      *pPara << DEFL(_T("3")) << _T(" = ") << DEFL(_T("2")) << _T(" + ") << SCL(cm.DiaphragmFactor) << _T(" * (") << DEFL(_T("diaphragm"));
      if (bConstruction)
      {
         *pPara << _T(" + ") << DEFL(_T("construction"));
      }

      if (bShearKey)
      {
         *pPara << _T(" + ") << DEFL(_T("shear key"));
      }

      *pPara << _T(")") << _T(" + ") << SCL(cm.SlabUser1Factor) << _T(" * ") << DEFL(_T("user1"));

      if (bTempStrands)
      {
         *pPara << _T(" + ") << SCL(cm.ErectionFactor) << _T(" * ") << DEFL(_T("tpsr"));
      }

      *pPara << rptNewLine;

      *pPara << DEFL(_T("4")) << _T(" = ") << DEFL(_T("3")) << _T(" + ") << SCL(cm.CreepFactor) << _T(" * ") << DEFL(_T("creep2")) << rptNewLine;

      *pPara << DEFL(_T("5")) << _T(" = ") << DEFL(_T("4"));
      *pPara << _T(" + ") << SCL(cm.BarrierSwOverlayUser2Factor) << _T(" * (") << DEFL(_T("barrier"));

      if (deckType == pgsTypes::sdtNonstructuralOverlay)
      {
         *pPara << _T(" + ") << DEFL(_T("slab")) << _T(" + ") << DEFL(_T("haunch"));
      }

      if ( bSidewalk )
      {
         *pPara << _T(" + ") << DEFL(_T("sidewalk"));
      }

      if ( bOverlay )
      {
         *pPara << _T(" + ") << DEFL(_T("overlay"));
      }

      *pPara << _T(" + ") << DEFL(_T("user2")) << _T(")") << rptNewLine;

      *pPara << DEFL(_T("6")) << _T(" = ") << DEFL(_T("5")) << _T(" + ") << SCL(cm.CreepFactor) << _T(" * ") << DEFL(_T("creep3"));
      *pPara << _T(" = ") << Sub2(symbol(DELTA),_T("excess")) << _T(" = Computed Excess Camber") << rptNewLine;

      *pPara << rptNewLine;
   }
}
