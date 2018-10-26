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

#include "StdAfx.h"
#include <Reporting\CreepCoefficientChapterBuilder.h>

#include <PgsExt\GirderData.h>

#include <EAF\EAFDisplayUnits.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Project.h>
#include <IFace\Bridge.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CCreepCoefficientChapterBuilder
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CCreepCoefficientChapterBuilder::CCreepCoefficientChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CCreepCoefficientChapterBuilder::GetName() const
{
   return TEXT("Creep Coefficient Details");
}

rptChapter* CCreepCoefficientChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CSpanGirderReportSpecification* pSGRptSpec = dynamic_cast<CSpanGirderReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pSGRptSpec->GetBroker(&pBroker);
   SpanIndexType span = pSGRptSpec->GetSpan();
   GirderIndexType gdr = pSGRptSpec->GetGirder();

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker,IBridge,pBridge);
   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();

   GET_IFACE2(pBroker,IGirderData,pGirderData);
   CGirderData girderData = pGirderData->GetGirderData(span,gdr);
   bool bTempStrands = (0 < girderData.Nstrands[pgsTypes::Temporary] && girderData.TempStrandUsage != pgsTypes::ttsPTBeforeShipping) ? true : false;

   rptChapter* pChapter;
   switch( deckType )
   {
   case pgsTypes::sdtCompositeCIP:
   case pgsTypes::sdtCompositeOverlay:
      pChapter = (bTempStrands ? Build_CIP_TempStrands(pRptSpec, pBroker,span,gdr,pDisplayUnits,level) : Build_CIP(pRptSpec, pBroker,span,gdr,pDisplayUnits,level));
      break;

   case pgsTypes::sdtCompositeSIP:
      pChapter = (bTempStrands ? Build_SIP_TempStrands(pRptSpec, pBroker,span,gdr,pDisplayUnits,level) : Build_SIP(pRptSpec, pBroker,span,gdr,pDisplayUnits,level));
      break;

   case pgsTypes::sdtNone:
      pChapter = (bTempStrands ? Build_NoDeck_TempStrands(pRptSpec, pBroker,span,gdr,pDisplayUnits,level) : Build_NoDeck(pRptSpec, pBroker,span,gdr,pDisplayUnits,level));
      break;

   default:
      ATLASSERT(false);
   }

   return pChapter;
}

CChapterBuilder* CCreepCoefficientChapterBuilder::Clone() const
{
   return new CCreepCoefficientChapterBuilder;
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

rptChapter* CCreepCoefficientChapterBuilder::Build_CIP_TempStrands(CReportSpecification* pRptSpec,IBroker* pBroker,SpanIndexType span,GirderIndexType gdr,
                                                   IEAFDisplayUnits* pDisplayUnits,
                                                   Uint16 level) const
{
   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);
   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   GET_IFACE2(pBroker,ICamber,pCamber);
   GET_IFACE2(pBroker,IGirderData,pGirderData);

   GET_IFACE2(pBroker,ILibrary,pLib);
   GET_IFACE2(pBroker,ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());

   INIT_UV_PROTOTYPE( rptTimeUnitValue, time, pDisplayUnits->GetLongTimeUnit(), true );
   INIT_UV_PROTOTYPE( rptTimeUnitValue, time2, pDisplayUnits->GetLongTimeUnit(), false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, fc, pDisplayUnits->GetStressUnit(), true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, length, pDisplayUnits->GetComponentDimUnit(), true );

   CREEPCOEFFICIENTDETAILS details;

#if defined IGNORE_2007_CHANGES
   if ( lrfdVersionMgr::FourthEdition2007 == pSpecEntry->GetSpecificationType() )
   {
      pPara = new rptParagraph();
      *pChapter << pPara;
      *pPara << color(Red) << bold(ON) << _T(_T("Changes to LRFD 4th Edition, 2007, Article 5.4.2.3.2 have been ignored.")) << bold(OFF) << color(Black) << rptNewLine;
   }
#endif

   //////////////////////////////
   // Report LRFD method
   //////////////////////////////
   bool bSI = IS_SI_UNITS(pDisplayUnits);

   for ( Int16 i = CREEP_MINTIME; i <= CREEP_MAXTIME; i++ )
   {
      if ( i == CREEP_MINTIME )
      {
         // firs time through loop, report the common information
         details = pCamber->GetCreepCoefficientDetails(span,gdr,ICamber::cpReleaseToDiaphragm,i);

         if ( details.Spec == CREEP_SPEC_PRE_2005 )
         {
            *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("LRFDCreepEqn.png")) << rptNewLine;
            *pPara << Bold(_T("for which:")) << rptNewLine;
            *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + (bSI ? _T("KfEqn-SI.png") : _T("KfEqn-US.png")) ) << rptNewLine;
            *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + (bSI ? _T("KcEqn-SI.png") : _T("KcEqn-US.png")) ) << rptNewLine;

            *pPara << Bold(_T("where:")) << rptNewLine;
            *pPara << _T("H = ") << details.H << _T("%") << rptNewLine;
            *pPara << _T("V/S = ") << length.SetValue(details.VSratio) << rptNewLine;
            *pPara << _T("In determining the maturity of concrete at initial load, t") << Sub(_T("i"))
                   << _T(", one day of accelerated curing may be taken as equal to seven days of normal curing.") << rptNewLine;
            *pPara << _T("Curing Method = ") << (details.CuringMethod == CURING_ACCELERATED ? _T("Accelerated") : _T("Normal")) << rptNewLine;
            *pPara << rptNewLine;
         }
         else
         {
#if defined IGNORE_2007_CHANGES
            *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("LRFDCreepEqn2005.png")) << rptNewLine;
            *pPara << Bold(_T("for which:")) << rptNewLine;
            
            if ( pSpecEntry->GetSpecificationType() <= lrfdVersionMgr::ThirdEditionWith2005Interims )
               *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + (bSI ? _T("KvsEqn-SI.png") : _T("KvsEqn-US.png")) ) << rptNewLine;
            else
               *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + (bSI ? _T("KvsEqn2006-SI.png") : _T("KvsEqn2006-US.png")) ) << rptNewLine;
#else
            if ( lrfdVersionMgr::FourthEdition2007 <= pSpecEntry->GetSpecificationType() )
               *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("LRFDCreepEqn2007.png")) << rptNewLine;
            else
               *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("LRFDCreepEqn2005.png")) << rptNewLine;

            *pPara << Bold(_T("for which:")) << rptNewLine;
            
            if ( pSpecEntry->GetSpecificationType() <= lrfdVersionMgr::ThirdEditionWith2005Interims || lrfdVersionMgr::FourthEdition2007 <= pSpecEntry->GetSpecificationType())
               *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + (bSI ? _T("KvsEqn-SI.png") : _T("KvsEqn-US.png")) ) << rptNewLine;
            else
               *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + (bSI ? _T("KvsEqn2006-SI.png") : _T("KvsEqn2006-US.png")) ) << rptNewLine;
#endif // IGNORE_2007_CHANGES

            *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("KhcEqn.png") ) << rptNewLine;
            *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + (bSI ? _T("KfEqn2005-SI.png") : _T("KfEqn2005-US.png")) ) << rptNewLine;
            *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + (bSI ? _T("KtdEqn-SI.png") : _T("KtdEqn-US.png")) ) << rptNewLine;

            *pPara << Bold(_T("where:")) << rptNewLine;
            *pPara << _T("H = ") << details.H << _T("%") << rptNewLine;
            *pPara << _T("V/S = ") << length.SetValue(details.VSratio) << rptNewLine;
            *pPara << Sub2(_T("K"),_T("1")) << _T(" = ") << details.K1 << rptNewLine;
            *pPara << Sub2(_T("K"),_T("2")) << _T(" = ") << details.K2 << rptNewLine;
#if defined IGNORE_2007_CHANGES
           *pPara << _T("In determining the maturity of concrete at initial load, t") << Sub(_T("i"))
                  << _T(", one day of accelerated curing may be taken as equal to seven days of normal curing.") << rptNewLine;
#else
           if ( pSpecEntry->GetSpecificationType() < lrfdVersionMgr::FourthEdition2007 )
           {
              *pPara << _T("In determining the maturity of concrete at initial load, t") << Sub(_T("i"))
                     << _T(", one day of accelerated curing may be taken as equal to seven days of normal curing.") << rptNewLine;
           }
#endif
            *pPara << _T("Curing Method = ") << (details.CuringMethod == CURING_ACCELERATED ? _T("Accelerated") : _T("Normal")) << rptNewLine;
            *pPara << rptNewLine;
         } // spec
      } // i

      *pPara << Bold((i == CREEP_MINTIME ? _T("Minimum Timing") : _T("Maximum Timing"))) << rptNewLine;

      details = pCamber->GetCreepCoefficientDetails(span,gdr,ICamber::cpReleaseToDiaphragm,i);

      if ( details.Spec == CREEP_SPEC_PRE_2005 )
      {
         *pPara << rptNewLine;
         *pPara << Bold(_T("Prestress release until temporary strand removal and diaphragm casting")) <<rptNewLine;
         *pPara << RPT_FCI << _T(" = ") << fc.SetValue( details.Fc ) << rptNewLine;
         *pPara << _T("t") << Sub(_T("i")) << _T(" (Adjusted) = ") << time.SetValue(details.ti) << rptNewLine;
         *pPara << _T("t = ")<< time.SetValue(details.t) << rptNewLine;
         *pPara << _T("k") << Sub(_T("f")) << _T(" = ") << details.kf << rptNewLine;
         *pPara << _T("k") << Sub(_T("c")) << _T(" = ") << details.kc << rptNewLine;
         *pPara << symbol(psi) << _T("(")<<time2.SetValue( details.t);
         *pPara <<_T(",")<<time2.SetValue(details.ti)<<_T(") = ")<< details.Ct << rptNewLine;

         *pPara << rptNewLine;

         details = pCamber->GetCreepCoefficientDetails(span,gdr,ICamber::cpReleaseToDeck,i);
         *pPara << Bold(_T("Prestress release until deck casting")) << rptNewLine;
         *pPara << RPT_FCI << _T(" = ") << fc.SetValue( details.Fc ) << rptNewLine;
         *pPara << _T("t") << Sub(_T("i")) << _T(" = ") << time.SetValue( details.ti) << rptNewLine;
         *pPara << _T("t = ")<< time.SetValue( details.t ) << rptNewLine;
         *pPara << _T("k") << Sub(_T("f")) << _T(" = ") << details.kf << rptNewLine;
         *pPara << _T("k") << Sub(_T("c")) << _T(" = ") << details.kc << rptNewLine;
         *pPara << symbol(psi) << _T("(")<<time2.SetValue( details.t);
         *pPara <<_T(",")<<time2.SetValue(details.ti)<<_T(") = ")<< details.Ct << rptNewLine;

         *pPara << rptNewLine;

         details = pCamber->GetCreepCoefficientDetails(span,gdr,ICamber::cpDiaphragmToDeck,i);
         *pPara << Bold(_T("Temporary strand removal and diaphragm casting to deck casting and application of superimposed dead loads")) << rptNewLine;
         *pPara << RPT_FCI << _T(" = ") << fc.SetValue( details.Fc ) << rptNewLine;
         *pPara << _T("t") << Sub(_T("i")) << _T(" = ") << time.SetValue( details.ti) << rptNewLine;
         *pPara << _T("t = ")<< time.SetValue( details.t ) << rptNewLine;
         *pPara << _T("k") << Sub(_T("f")) << _T(" = ") << details.kf << rptNewLine;
         *pPara << _T("k") << Sub(_T("c")) << _T(" = ") << details.kc << rptNewLine;
         *pPara << symbol(psi) << _T("(")<<time2.SetValue( details.t);
         *pPara <<_T(",")<<time2.SetValue(details.ti)<<_T(") = ")<< details.Ct << rptNewLine;
      }
      else
      {
         // 2005 and later
         *pPara << rptNewLine;
         *pPara << Bold(_T("Prestress release until temporary strand removal and diaphragm casting")) <<rptNewLine;
         *pPara << RPT_FCI << _T(" = ") << fc.SetValue( details.Fc ) << rptNewLine;
         *pPara << _T("t") << Sub(_T("i")) << _T(" (Adjusted) = ") << time.SetValue(details.ti) << rptNewLine;
         *pPara << _T("t = ")<< time.SetValue(details.t) << rptNewLine;
#if defined IGNORE_2007_CHANGES
         *pPara << _T("k") << Sub(_T("vs")) << _T(" = ") << details.kvs << rptNewLine;
#else
         if ( lrfdVersionMgr::FourthEdition2007 <= pSpecEntry->GetSpecificationType() )
            *pPara << _T("k") << Sub(_T("s")) << _T(" = ") << details.kvs << rptNewLine;
         else
            *pPara << _T("k") << Sub(_T("vs")) << _T(" = ") << details.kvs << rptNewLine;
#endif
         *pPara << _T("k") << Sub(_T("hc")) << _T(" = ") << details.khc << rptNewLine;
         *pPara << _T("k") << Sub(_T("f")) << _T(" = ") << details.kf << rptNewLine;
         *pPara << _T("k") << Sub(_T("td")) << _T(" = ") << details.ktd << rptNewLine;
         *pPara << symbol(psi) << _T("(")<<time2.SetValue(details.t);
         *pPara <<_T(",")<<time2.SetValue(details.ti)<<_T(") = ")<< details.Ct << rptNewLine;

         *pPara << rptNewLine;

         details = pCamber->GetCreepCoefficientDetails(span,gdr,ICamber::cpReleaseToDeck,i);
         *pPara << Bold(_T("Prestress release until deck casting")) << rptNewLine;
         *pPara << RPT_FCI << _T(" = ") << fc.SetValue( details.Fc ) << rptNewLine;
         *pPara << _T("t") << Sub(_T("i")) << _T(" = ") << time.SetValue( details.ti) << rptNewLine;
         *pPara << _T("t = ")<< time.SetValue(details.t) << rptNewLine;
#if defined IGNORE_2007_CHANGES
         *pPara << _T("k") << Sub(_T("vs")) << _T(" = ") << details.kvs << rptNewLine;
#else
         if ( lrfdVersionMgr::FourthEdition2007 <= pSpecEntry->GetSpecificationType() )
            *pPara << _T("k") << Sub(_T("s")) << _T(" = ") << details.kvs << rptNewLine;
         else
            *pPara << _T("k") << Sub(_T("vs")) << _T(" = ") << details.kvs << rptNewLine;
#endif
         *pPara << _T("k") << Sub(_T("hc")) << _T(" = ") << details.khc << rptNewLine;
         *pPara << _T("k") << Sub(_T("f")) << _T(" = ") << details.kf << rptNewLine;
         *pPara << _T("k") << Sub(_T("td")) << _T(" = ") << details.ktd << rptNewLine;
         *pPara << symbol(psi) << _T("(")<<time2.SetValue(details.t);
         *pPara <<_T(",")<<time2.SetValue(details.ti)<<_T(") = ")<< details.Ct << rptNewLine;

         *pPara << rptNewLine;

         details = pCamber->GetCreepCoefficientDetails(span,gdr,ICamber::cpDiaphragmToDeck,i);
         *pPara << Bold(_T("Temporary strand removal and diaphragm casting to deck casting and application of superimposed dead loads")) << rptNewLine;
         *pPara << RPT_FCI << _T(" = ") << fc.SetValue( details.Fc ) << rptNewLine;
         *pPara << _T("t") << Sub(_T("i")) << _T(" = ") << time.SetValue( details.ti) << rptNewLine;
         *pPara << _T("t = ")<< time.SetValue(details.t) << rptNewLine;
#if defined IGNORE_2007_CHANGES
         *pPara << _T("k") << Sub(_T("vs")) << _T(" = ") << details.kvs << rptNewLine;
#else
         if ( lrfdVersionMgr::FourthEdition2007 <= pSpecEntry->GetSpecificationType() )
            *pPara << _T("k") << Sub(_T("s")) << _T(" = ") << details.kvs << rptNewLine;
         else
            *pPara << _T("k") << Sub(_T("vs")) << _T(" = ") << details.kvs << rptNewLine;
#endif
         *pPara << _T("k") << Sub(_T("hc")) << _T(" = ") << details.khc << rptNewLine;
         *pPara << _T("k") << Sub(_T("f")) << _T(" = ") << details.kf << rptNewLine;
         *pPara << _T("k") << Sub(_T("td")) << _T(" = ") << details.ktd << rptNewLine;
         *pPara << symbol(psi) << _T("(")<<time2.SetValue(details.t);
         *pPara <<_T(",")<<time2.SetValue(details.ti)<<_T(") = ")<< details.Ct << rptNewLine;
      } // spec type

   *pPara << rptNewLine;
   }// loop on i


   return pChapter;
}

rptChapter* CCreepCoefficientChapterBuilder::Build_CIP(CReportSpecification* pRptSpec,IBroker* pBroker,SpanIndexType span,GirderIndexType gdr,
                                                       IEAFDisplayUnits* pDisplayUnits,
                                                       Uint16 level) const
{
   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);
   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   GET_IFACE2(pBroker,ICamber,pCamber);
   GET_IFACE2(pBroker,IGirderData,pGirderData);

   GET_IFACE2(pBroker,ILibrary,pLib);
   GET_IFACE2(pBroker,ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());

   INIT_UV_PROTOTYPE( rptTimeUnitValue, time, pDisplayUnits->GetLongTimeUnit(), true );
   INIT_UV_PROTOTYPE( rptTimeUnitValue, time2, pDisplayUnits->GetLongTimeUnit(), false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, fc, pDisplayUnits->GetStressUnit(), true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, length, pDisplayUnits->GetComponentDimUnit(), true );

   CREEPCOEFFICIENTDETAILS details;

#if defined IGNORE_2007_CHANGES
   if ( lrfdVersionMgr::FourthEdition2007 == pSpecEntry->GetSpecificationType() )
   {
      pPara = new rptParagraph();
      *pChapter << pPara;
      *pPara << color(Red) << bold(ON) << _T("Changes to LRFD 4th Edition, 2007, Article 5.4.2.3.2 have been ignored.") << bold(OFF) << color(Black) << rptNewLine;
   }
#endif

   //////////////////////////////
   // Report LRFD method
   //////////////////////////////
   bool bSI = IS_SI_UNITS(pDisplayUnits);

   for ( Int16 i = CREEP_MINTIME; i <= CREEP_MAXTIME; i++ )
   {
      details = pCamber->GetCreepCoefficientDetails(span,gdr,ICamber::cpReleaseToDeck,i);
      if ( i == CREEP_MINTIME )
      {
         // first time through loop, report the common information

         if ( details.Spec == CREEP_SPEC_PRE_2005 )
         {
            *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("LRFDCreepEqn.png")) << rptNewLine;
            *pPara << Bold(_T("for which:")) << rptNewLine;
            *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + (bSI ? _T("KfEqn-SI.png") : _T("KfEqn-US.png")) ) << rptNewLine;
            *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + (bSI ? _T("KcEqn-SI.png") : _T("KcEqn-US.png")) ) << rptNewLine;

            *pPara << Bold(_T("where:")) << rptNewLine;
            *pPara << _T("H = ") << details.H << _T("%") << rptNewLine;
            *pPara << _T("V/S = ") << length.SetValue(details.VSratio) << rptNewLine;
            *pPara << _T("In determining the maturity of concrete at initial load, t") << Sub(_T("i"))
                   << _T(", one day of accelerated curing may be taken as equal to seven days of normal curing.") << rptNewLine;
            *pPara << _T("Curing Method = ") << (details.CuringMethod == CURING_ACCELERATED ? _T("Accelerated") : _T("Normal")) << rptNewLine;
            *pPara << rptNewLine;
         }
         else
         {
#if defined IGNORE_2007_CHANGES
            *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("LRFDCreepEqn2005.png")) << rptNewLine;
            *pPara << Bold(_T("for which:")) << rptNewLine;
            
            if ( pSpecEntry->GetSpecificationType() <= lrfdVersionMgr::ThirdEditionWith2005Interims )
               *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + (bSI ? _T("KvsEqn-SI.png") : _T("KvsEqn-US.png")) ) << rptNewLine;
            else
               *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + (bSI ? _T("KvsEqn2006-SI.png") : _T("KvsEqn2006-US.png")) ) << rptNewLine;
#else
            if ( lrfdVersionMgr::FourthEdition2007 <= pSpecEntry->GetSpecificationType() )
               *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("LRFDCreepEqn2007.png")) << rptNewLine;
            else
               *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("LRFDCreepEqn2005.png")) << rptNewLine;
            *pPara << Bold(_T("for which:")) << rptNewLine;
            
            if ( pSpecEntry->GetSpecificationType() <= lrfdVersionMgr::ThirdEditionWith2005Interims || lrfdVersionMgr::FourthEdition2007 <= pSpecEntry->GetSpecificationType())
               *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + (bSI ? _T("KvsEqn-SI.png") : _T("KvsEqn-US.png")) ) << rptNewLine;
            else
               *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + (bSI ? _T("KvsEqn2006-SI.png") : _T("KvsEqn2006-US.png")) ) << rptNewLine;
#endif // IGNORE_2007_CHANGES

            *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("KhcEqn.png") ) << rptNewLine;
            *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + (bSI ? _T("KfEqn2005-SI.png") : _T("KfEqn2005-US.png")) ) << rptNewLine;
            *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + (bSI ? _T("KtdEqn-SI.png") : _T("KtdEqn-US.png")) ) << rptNewLine;

            *pPara << Bold(_T("where:")) << rptNewLine;
            *pPara << _T("H = ") << details.H << _T("%") << rptNewLine;
            *pPara << _T("V/S = ") << length.SetValue(details.VSratio) << rptNewLine;
            *pPara << Sub2(_T("K"),_T("1")) << _T(" = ") << details.K1 << rptNewLine;
            *pPara << Sub2(_T("K"),_T("2")) << _T(" = ") << details.K2 << rptNewLine;
#if defined IGNORE_2007_CHANGES
           *pPara << _T("In determining the maturity of concrete at initial load, t") << Sub(_T("i"))
                  << _T(", one day of accelerated curing may be taken as equal to seven days of normal curing.") << rptNewLine;
#else
           if ( pSpecEntry->GetSpecificationType() < lrfdVersionMgr::FourthEdition2007 )
           {
              *pPara << _T("In determining the maturity of concrete at initial load, t") << Sub(_T("i"))
                     << _T(", one day of accelerated curing may be taken as equal to seven days of normal curing.") << rptNewLine;
           }
#endif
            *pPara << _T("Curing Method = ") << (details.CuringMethod == CURING_ACCELERATED ? _T("Accelerated") : _T("Normal")) << rptNewLine;
            *pPara << rptNewLine;
         } // spec
      } // i

      *pPara << Bold((i == CREEP_MINTIME ? _T("Minimum Timing") : _T("Maximum Timing"))) << rptNewLine;

      if ( details.Spec == CREEP_SPEC_PRE_2005 )
      {
         *pPara << rptNewLine;
         *pPara << Bold(_T("Prestress release until deck casting")) <<rptNewLine;
         *pPara << RPT_FCI << _T(" = ") << fc.SetValue( details.Fc ) << rptNewLine;
         *pPara << _T("t") << Sub(_T("i")) << _T(" (Adjusted) = ") << time.SetValue(details.ti) << rptNewLine;
         *pPara << _T("t = ")<< time.SetValue(details.t) << rptNewLine;
         *pPara << _T("k") << Sub(_T("f")) << _T(" = ") << details.kf << rptNewLine;
         *pPara << _T("k") << Sub(_T("c")) << _T(" = ") << details.kc << rptNewLine;
         *pPara << symbol(psi) << _T("(")<<time2.SetValue( details.t);
         *pPara <<_T(",")<<time2.SetValue(details.ti)<<_T(") = ")<< details.Ct << rptNewLine;
      }
      else
      {
         // 2005 and later
         *pPara << rptNewLine;
         *pPara << Bold(_T("Prestress release until deck casting")) <<rptNewLine;
         *pPara << RPT_FCI << _T(" = ") << fc.SetValue( details.Fc ) << rptNewLine;
         *pPara << _T("t") << Sub(_T("i")) << _T(" (Adjusted) = ") << time.SetValue(details.ti) << rptNewLine;
         *pPara << _T("t = ")<< time.SetValue(details.t) << rptNewLine;
#if defined IGNORE_2007_CHANGES
         *pPara << _T("k") << Sub(_T("vs")) << _T(" = ") << details.kvs << rptNewLine;
#else
         if ( lrfdVersionMgr::FourthEdition2007 <= pSpecEntry->GetSpecificationType() )
            *pPara << _T("k") << Sub(_T("s")) << _T(" = ") << details.kvs << rptNewLine;
         else
            *pPara << _T("k") << Sub(_T("vs")) << _T(" = ") << details.kvs << rptNewLine;
#endif
         *pPara << _T("k") << Sub(_T("hc")) << _T(" = ") << details.khc << rptNewLine;
         *pPara << _T("k") << Sub(_T("f")) << _T(" = ") << details.kf << rptNewLine;
         *pPara << _T("k") << Sub(_T("td")) << _T(" = ") << details.ktd << rptNewLine;
         *pPara << symbol(psi) << _T("(")<<time2.SetValue(details.t);
         *pPara <<_T(",")<<time2.SetValue(details.ti)<<_T(") = ")<< details.Ct << rptNewLine;
      } // spec type

      *pPara << rptNewLine;
   }// loop on i


   return pChapter;
}

rptChapter* CCreepCoefficientChapterBuilder::Build_SIP_TempStrands(CReportSpecification* pRptSpec,IBroker* pBroker,SpanIndexType span,GirderIndexType gdr,
                                                                   IEAFDisplayUnits* pDisplayUnits,
                                                                   Uint16 level) const
{
   return Build_CIP_TempStrands(pRptSpec, pBroker, span, gdr, pDisplayUnits, level);
}

rptChapter* CCreepCoefficientChapterBuilder::Build_SIP(CReportSpecification* pRptSpec,IBroker* pBroker,SpanIndexType span,GirderIndexType gdr,
                                                       IEAFDisplayUnits* pDisplayUnits,
                                                       Uint16 level) const
{
   return Build_SIP_TempStrands(pRptSpec, pBroker, span, gdr, pDisplayUnits, level);
}

rptChapter* CCreepCoefficientChapterBuilder::Build_NoDeck_TempStrands(CReportSpecification* pRptSpec,IBroker* pBroker,SpanIndexType span,GirderIndexType gdr,
                                                                      IEAFDisplayUnits* pDisplayUnits,
                                                                      Uint16 level) const
{
   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);
   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   GET_IFACE2(pBroker,ICamber,pCamber);
   GET_IFACE2(pBroker,IGirderData,pGirderData);

   GET_IFACE2(pBroker,ILibrary,pLib);
   GET_IFACE2(pBroker,ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());

   INIT_UV_PROTOTYPE( rptTimeUnitValue, time, pDisplayUnits->GetLongTimeUnit(), true );
   INIT_UV_PROTOTYPE( rptTimeUnitValue, time2, pDisplayUnits->GetLongTimeUnit(), false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, fc, pDisplayUnits->GetStressUnit(), true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, length, pDisplayUnits->GetComponentDimUnit(), true );

   CREEPCOEFFICIENTDETAILS details;

#if defined IGNORE_2007_CHANGES
   if ( lrfdVersionMgr::FourthEdition2007 == pSpecEntry->GetSpecificationType() )
   {
      pPara = new rptParagraph();
      *pChapter << pPara;
      *pPara << color(Red) << bold(ON) << _T("Changes to LRFD 4th Edition, 2007, Article 5.4.2.3.2 have been ignored.") << bold(OFF) << color(Black) << rptNewLine;
   }
#endif

   //////////////////////////////
   // Report LRFD method
   //////////////////////////////
   bool bSI = IS_SI_UNITS(pDisplayUnits);

   for ( Int16 i = CREEP_MINTIME; i <= CREEP_MAXTIME; i++ )
   {
     details = pCamber->GetCreepCoefficientDetails(span,gdr,ICamber::cpReleaseToDiaphragm,i);
     if ( i == CREEP_MINTIME )
     {
        // first time through loop, report the common information

        if ( details.Spec == CREEP_SPEC_PRE_2005 )
        {
           *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("LRFDCreepEqn.png")) << rptNewLine;
           *pPara << Bold(_T("for which:")) << rptNewLine;
           *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + (bSI ? _T("KfEqn-SI.png") : _T("KfEqn-US.png")) ) << rptNewLine;
           *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + (bSI ? _T("KcEqn-SI.png") : _T("KcEqn-US.png")) ) << rptNewLine;

           *pPara << Bold(_T("where:")) << rptNewLine;
           *pPara << _T("H = ") << details.H << _T("%") << rptNewLine;
           *pPara << _T("V/S = ") << length.SetValue(details.VSratio) << rptNewLine;
           *pPara << _T("In determining the maturity of concrete at initial load, t") << Sub(_T("i"))
                  << _T(", one day of accelerated curing may be taken as equal to seven days of normal curing.") << rptNewLine;
           *pPara << _T("Curing Method = ") << (details.CuringMethod == CURING_ACCELERATED ? _T("Accelerated") : _T("Normal")) << rptNewLine;
           *pPara << rptNewLine;
        }
        else
        {
#if defined IGNORE_2007_CHANGES
            *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("LRFDCreepEqn2005.png")) << rptNewLine;
            *pPara << Bold(_T("for which:")) << rptNewLine;
            
            if ( pSpecEntry->GetSpecificationType() <= lrfdVersionMgr::ThirdEditionWith2005Interims )
               *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + (bSI ? _T("KvsEqn-SI.png") : _T("KvsEqn-US.png")) ) << rptNewLine;
            else
               *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + (bSI ? _T("KvsEqn2006-SI.png") : _T("KvsEqn2006-US.png")) ) << rptNewLine;
#else
            if ( lrfdVersionMgr::FourthEdition2007 <= pSpecEntry->GetSpecificationType() )
               *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("LRFDCreepEqn2007.png")) << rptNewLine;
            else
               *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("LRFDCreepEqn2005.png")) << rptNewLine;
            *pPara << Bold(_T("for which:")) << rptNewLine;
            
            if ( pSpecEntry->GetSpecificationType() <= lrfdVersionMgr::ThirdEditionWith2005Interims || lrfdVersionMgr::FourthEdition2007 <= pSpecEntry->GetSpecificationType())
               *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + (bSI ? _T("KvsEqn-SI.png") : _T("KvsEqn-US.png")) ) << rptNewLine;
            else
               *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + (bSI ? _T("KvsEqn2006-SI.png") : _T("KvsEqn2006-US.png")) ) << rptNewLine;
#endif // IGNORE_2007_CHANGES
           *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("KhcEqn.png") ) << rptNewLine;
           *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + (bSI ? _T("KfEqn2005-SI.png") : _T("KfEqn2005-US.png")) ) << rptNewLine;
           *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + (bSI ? _T("KtdEqn-SI.png") : _T("KtdEqn-US.png")) ) << rptNewLine;

           *pPara << Bold(_T("where:")) << rptNewLine;
           *pPara << _T("H = ") << details.H << _T("%") << rptNewLine;
           *pPara << _T("V/S = ") << length.SetValue(details.VSratio) << rptNewLine;
            *pPara << Sub2(_T("K"),_T("1")) << _T(" = ") << details.K1 << rptNewLine;
            *pPara << Sub2(_T("K"),_T("2")) << _T(" = ") << details.K2 << rptNewLine;
#if defined IGNORE_2007_CHANGES
           *pPara << _T("In determining the maturity of concrete at initial load, t") << Sub(_T("i"))
                  << _T(", one day of accelerated curing may be taken as equal to seven days of normal curing.") << rptNewLine;
#else
           if ( pSpecEntry->GetSpecificationType() < lrfdVersionMgr::FourthEdition2007 )
           {
              *pPara << _T("In determining the maturity of concrete at initial load, t") << Sub(_T("i"))
                     << _T(", one day of accelerated curing may be taken as equal to seven days of normal curing.") << rptNewLine;
           }
#endif
           *pPara << _T("Curing Method = ") << (details.CuringMethod == CURING_ACCELERATED ? _T("Accelerated") : _T("Normal")) << rptNewLine;
           *pPara << rptNewLine;
        } // spec
     } // i
     
     *pPara << Bold((i == CREEP_MINTIME ? _T("Minimum Timing") : _T("Maximum Timing"))) << rptNewLine;


      if ( details.Spec == CREEP_SPEC_PRE_2005 )
      {
         *pPara << rptNewLine;
         *pPara << Bold(_T("Prestress release until temporary strand removal and diaphragm casting")) <<rptNewLine;
         *pPara << RPT_FCI << _T(" = ") << fc.SetValue( details.Fc ) << rptNewLine;
         *pPara << _T("t") << Sub(_T("i")) << _T(" (Adjusted) = ") << time.SetValue(details.ti) << rptNewLine;
         *pPara << _T("t = ")<< time.SetValue(details.t) << rptNewLine;
         *pPara << _T("k") << Sub(_T("f")) << _T(" = ") << details.kf << rptNewLine;
         *pPara << _T("k") << Sub(_T("c")) << _T(" = ") << details.kc << rptNewLine;
         *pPara << symbol(psi) << _T("(")<<time2.SetValue( details.t);
         *pPara <<_T(",")<<time2.SetValue(details.ti)<<_T(") = ")<< details.Ct << rptNewLine;

         *pPara << rptNewLine;

         details = pCamber->GetCreepCoefficientDetails(span,gdr,ICamber::cpReleaseToDeck,i);
         *pPara << Bold(_T("Prestress release until application of superimposed dead loads")) << rptNewLine;
         *pPara << RPT_FCI << _T(" = ") << fc.SetValue( details.Fc ) << rptNewLine;
         *pPara << _T("t") << Sub(_T("i")) << _T(" = ") << time.SetValue( details.ti) << rptNewLine;
         *pPara << _T("t = ")<< time.SetValue( details.t ) << rptNewLine;
         *pPara << _T("k") << Sub(_T("f")) << _T(" = ") << details.kf << rptNewLine;
         *pPara << _T("k") << Sub(_T("c")) << _T(" = ") << details.kc << rptNewLine;
         *pPara << symbol(psi) << _T("(")<<time2.SetValue( details.t);
         *pPara <<_T(",")<<time2.SetValue(details.ti)<<_T(") = ")<< details.Ct << rptNewLine;

         *pPara << rptNewLine;

         details = pCamber->GetCreepCoefficientDetails(span,gdr,ICamber::cpReleaseToFinal,i);
         *pPara << Bold(_T("Prestress release to final")) << rptNewLine;
         *pPara << RPT_FCI << _T(" = ") << fc.SetValue( details.Fc ) << rptNewLine;
         *pPara << _T("t") << Sub(_T("i")) << _T(" = ") << time.SetValue( details.ti) << rptNewLine;
         *pPara << _T("t = ")<< time.SetValue( details.t ) << rptNewLine;
         *pPara << _T("k") << Sub(_T("f")) << _T(" = ") << details.kf << rptNewLine;
         *pPara << _T("k") << Sub(_T("c")) << _T(" = ") << details.kc << rptNewLine;
         *pPara << symbol(psi) << _T("(")<<time2.SetValue( details.t);
         *pPara <<_T(",")<<time2.SetValue(details.ti)<<_T(") = ")<< details.Ct << rptNewLine;

         *pPara << rptNewLine;

         details = pCamber->GetCreepCoefficientDetails(span,gdr,ICamber::cpDiaphragmToDeck,i);
         *pPara << Bold(_T("Temporary strand removal and diaphragm casting to application of superimposed dead loads")) << rptNewLine;
         *pPara << RPT_FCI << _T(" = ") << fc.SetValue( details.Fc ) << rptNewLine;
         *pPara << _T("t") << Sub(_T("i")) << _T(" = ") << time.SetValue( details.ti) << rptNewLine;
         *pPara << _T("t = ")<< time.SetValue( details.t ) << rptNewLine;
         *pPara << _T("k") << Sub(_T("f")) << _T(" = ") << details.kf << rptNewLine;
         *pPara << _T("k") << Sub(_T("c")) << _T(" = ") << details.kc << rptNewLine;
         *pPara << symbol(psi) << _T("(")<<time2.SetValue( details.t);
         *pPara <<_T(",")<<time2.SetValue(details.ti)<<_T(") = ")<< details.Ct << rptNewLine;

         *pPara << rptNewLine;

         details = pCamber->GetCreepCoefficientDetails(span,gdr,ICamber::cpDeckToFinal,i);
         *pPara << Bold(_T("Application of superimposed dead loads to final")) << rptNewLine;
         *pPara << RPT_FCI << _T(" = ") << fc.SetValue( details.Fc ) << rptNewLine;
         *pPara << _T("t") << Sub(_T("i")) << _T(" = ") << time.SetValue( details.ti) << rptNewLine;
         *pPara << _T("t = ")<< time.SetValue( details.t ) << rptNewLine;
         *pPara << _T("k") << Sub(_T("f")) << _T(" = ") << details.kf << rptNewLine;
         *pPara << _T("k") << Sub(_T("c")) << _T(" = ") << details.kc << rptNewLine;
         *pPara << symbol(psi) << _T("(")<<time2.SetValue( details.t);
         *pPara <<_T(",")<<time2.SetValue(details.ti)<<_T(") = ")<< details.Ct << rptNewLine;
      }
      else
      {
         // 2005 and later
         *pPara << rptNewLine;
         *pPara << Bold(_T("Prestress release until temporary strand removal and diaphragm casting")) <<rptNewLine;
         *pPara << RPT_FCI << _T(" = ") << fc.SetValue( details.Fc ) << rptNewLine;
         *pPara << _T("t") << Sub(_T("i")) << _T(" (Adjusted) = ") << time.SetValue(details.ti) << rptNewLine;
         *pPara << _T("t = ")<< time.SetValue(details.t) << rptNewLine;
#if defined IGNORE_2007_CHANGES
         *pPara << _T("k") << Sub(_T("vs")) << _T(" = ") << details.kvs << rptNewLine;
#else
         if ( lrfdVersionMgr::FourthEdition2007 <= pSpecEntry->GetSpecificationType() )
            *pPara << _T("k") << Sub(_T("s")) << _T(" = ") << details.kvs << rptNewLine;
         else
            *pPara << _T("k") << Sub(_T("vs")) << _T(" = ") << details.kvs << rptNewLine;
#endif
         *pPara << _T("k") << Sub(_T("hc")) << _T(" = ") << details.khc << rptNewLine;
         *pPara << _T("k") << Sub(_T("f")) << _T(" = ") << details.kf << rptNewLine;
         *pPara << _T("k") << Sub(_T("td")) << _T(" = ") << details.ktd << rptNewLine;
         *pPara << symbol(psi) << _T("(")<<time2.SetValue(details.t);
         *pPara <<_T(",")<<time2.SetValue(details.ti)<<_T(") = ")<< details.Ct << rptNewLine;

         *pPara << rptNewLine;

         details = pCamber->GetCreepCoefficientDetails(span,gdr,ICamber::cpReleaseToDeck,i);
         *pPara << Bold(_T("Prestress release until application of superimposed dead loads")) << rptNewLine;
         *pPara << RPT_FCI << _T(" = ") << fc.SetValue( details.Fc ) << rptNewLine;
         *pPara << _T("t") << Sub(_T("i")) << _T(" = ") << time.SetValue( details.ti) << rptNewLine;
         *pPara << _T("t = ")<< time.SetValue(details.t) << rptNewLine;
#if defined IGNORE_2007_CHANGES
         *pPara << _T("k") << Sub(_T("vs")) << _T(" = ") << details.kvs << rptNewLine;
#else
         if ( lrfdVersionMgr::FourthEdition2007 <= pSpecEntry->GetSpecificationType() )
            *pPara << _T("k") << Sub(_T("s")) << _T(" = ") << details.kvs << rptNewLine;
         else
            *pPara << _T("k") << Sub(_T("vs")) << _T(" = ") << details.kvs << rptNewLine;
#endif
         *pPara << _T("k") << Sub(_T("hc")) << _T(" = ") << details.khc << rptNewLine;
         *pPara << _T("k") << Sub(_T("f")) << _T(" = ") << details.kf << rptNewLine;
         *pPara << _T("k") << Sub(_T("td")) << _T(" = ") << details.ktd << rptNewLine;
         *pPara << symbol(psi) << _T("(")<<time2.SetValue(details.t);
         *pPara <<_T(",")<<time2.SetValue(details.ti)<<_T(") = ")<< details.Ct << rptNewLine;

         *pPara << rptNewLine;

         details = pCamber->GetCreepCoefficientDetails(span,gdr,ICamber::cpReleaseToFinal,i);
         *pPara << Bold(_T("Prestress release to final")) << rptNewLine;
         *pPara << RPT_FCI << _T(" = ") << fc.SetValue( details.Fc ) << rptNewLine;
         *pPara << _T("t") << Sub(_T("i")) << _T(" = ") << time.SetValue( details.ti) << rptNewLine;
         *pPara << _T("t = ")<< time.SetValue(details.t) << rptNewLine;
#if defined IGNORE_2007_CHANGES
         *pPara << _T("k") << Sub(_T("vs")) << _T(" = ") << details.kvs << rptNewLine;
#else
         if ( lrfdVersionMgr::FourthEdition2007 <= pSpecEntry->GetSpecificationType() )
            *pPara << _T("k") << Sub(_T("s")) << _T(" = ") << details.kvs << rptNewLine;
         else
            *pPara << _T("k") << Sub(_T("vs")) << _T(" = ") << details.kvs << rptNewLine;
#endif
         *pPara << _T("k") << Sub(_T("hc")) << _T(" = ") << details.khc << rptNewLine;
         *pPara << _T("k") << Sub(_T("f")) << _T(" = ") << details.kf << rptNewLine;
         *pPara << _T("k") << Sub(_T("td")) << _T(" = ") << details.ktd << rptNewLine;
         *pPara << symbol(psi) << _T("(")<<time2.SetValue(details.t);
         *pPara <<_T(",")<<time2.SetValue(details.ti)<<_T(") = ")<< details.Ct << rptNewLine;

         *pPara << rptNewLine;

         details = pCamber->GetCreepCoefficientDetails(span,gdr,ICamber::cpDiaphragmToDeck,i);
         *pPara << Bold(_T("Temporary strand removal and diaphragm casting to application of superimposed dead loads")) << rptNewLine;
         *pPara << RPT_FCI << _T(" = ") << fc.SetValue( details.Fc ) << rptNewLine;
         *pPara << _T("t") << Sub(_T("i")) << _T(" = ") << time.SetValue( details.ti) << rptNewLine;
         *pPara << _T("t = ")<< time.SetValue(details.t) << rptNewLine;
#if defined IGNORE_2007_CHANGES
         *pPara << _T("k") << Sub(_T("vs")) << _T(" = ") << details.kvs << rptNewLine;
#else
         if ( lrfdVersionMgr::FourthEdition2007 <= pSpecEntry->GetSpecificationType() )
            *pPara << _T("k") << Sub(_T("s")) << _T(" = ") << details.kvs << rptNewLine;
         else
            *pPara << _T("k") << Sub(_T("vs")) << _T(" = ") << details.kvs << rptNewLine;
#endif
         *pPara << _T("k") << Sub(_T("hc")) << _T(" = ") << details.khc << rptNewLine;
         *pPara << _T("k") << Sub(_T("f")) << _T(" = ") << details.kf << rptNewLine;
         *pPara << _T("k") << Sub(_T("td")) << _T(" = ") << details.ktd << rptNewLine;
         *pPara << symbol(psi) << _T("(")<<time2.SetValue(details.t);
         *pPara <<_T(",")<<time2.SetValue(details.ti)<<_T(") = ")<< details.Ct << rptNewLine;

         *pPara << rptNewLine;

         details = pCamber->GetCreepCoefficientDetails(span,gdr,ICamber::cpDiaphragmToFinal,i);
         *pPara << Bold(_T("Temporary strand removal and diaphragm casting to final")) << rptNewLine;
         *pPara << RPT_FCI << _T(" = ") << fc.SetValue( details.Fc ) << rptNewLine;
         *pPara << _T("t") << Sub(_T("i")) << _T(" = ") << time.SetValue( details.ti) << rptNewLine;
         *pPara << _T("t = ")<< time.SetValue(details.t) << rptNewLine;
#if defined IGNORE_2007_CHANGES
         *pPara << _T("k") << Sub(_T("vs")) << _T(" = ") << details.kvs << rptNewLine;
#else
         if ( lrfdVersionMgr::FourthEdition2007 <= pSpecEntry->GetSpecificationType() )
            *pPara << _T("k") << Sub(_T("s")) << _T(" = ") << details.kvs << rptNewLine;
         else
            *pPara << _T("k") << Sub(_T("vs")) << _T(" = ") << details.kvs << rptNewLine;
#endif
         *pPara << _T("k") << Sub(_T("hc")) << _T(" = ") << details.khc << rptNewLine;
         *pPara << _T("k") << Sub(_T("f")) << _T(" = ") << details.kf << rptNewLine;
         *pPara << _T("k") << Sub(_T("td")) << _T(" = ") << details.ktd << rptNewLine;
         *pPara << symbol(psi) << _T("(")<<time2.SetValue(details.t);
         *pPara <<_T(",")<<time2.SetValue(details.ti)<<_T(") = ")<< details.Ct << rptNewLine;

         *pPara << rptNewLine;

         details = pCamber->GetCreepCoefficientDetails(span,gdr,ICamber::cpDeckToFinal,i);
         *pPara << Bold(_T("Application of superimposed dead loads to final")) << rptNewLine;
         *pPara << RPT_FCI << _T(" = ") << fc.SetValue( details.Fc ) << rptNewLine;
         *pPara << _T("t") << Sub(_T("i")) << _T(" = ") << time.SetValue( details.ti) << rptNewLine;
         *pPara << _T("t = ")<< time.SetValue(details.t) << rptNewLine;
#if defined IGNORE_2007_CHANGES
         *pPara << _T("k") << Sub(_T("vs")) << _T(" = ") << details.kvs << rptNewLine;
#else
         if ( lrfdVersionMgr::FourthEdition2007 <= pSpecEntry->GetSpecificationType() )
            *pPara << _T("k") << Sub(_T("s")) << _T(" = ") << details.kvs << rptNewLine;
         else
            *pPara << _T("k") << Sub(_T("vs")) << _T(" = ") << details.kvs << rptNewLine;
#endif
         *pPara << _T("k") << Sub(_T("hc")) << _T(" = ") << details.khc << rptNewLine;
         *pPara << _T("k") << Sub(_T("f")) << _T(" = ") << details.kf << rptNewLine;
         *pPara << _T("k") << Sub(_T("td")) << _T(" = ") << details.ktd << rptNewLine;
         *pPara << symbol(psi) << _T("(")<<time2.SetValue(details.t);
         *pPara <<_T(",")<<time2.SetValue(details.ti)<<_T(") = ")<< details.Ct << rptNewLine;
      } // spec type

      *pPara << rptNewLine;
   }// loop on i


   return pChapter;
}

rptChapter* CCreepCoefficientChapterBuilder::Build_NoDeck(CReportSpecification* pRptSpec,IBroker* pBroker,SpanIndexType span,GirderIndexType gdr,
                                                          IEAFDisplayUnits* pDisplayUnits,
                                                          Uint16 level) const
{
   return Build_NoDeck_TempStrands(pRptSpec, pBroker, span, gdr, pDisplayUnits, level);
}