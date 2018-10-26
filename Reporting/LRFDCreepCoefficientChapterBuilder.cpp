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
#include <Reporting\LRFDCreepCoefficientChapterBuilder.h>

#include <IFace\AnalysisResults.h>
#include <IFace\Project.h>
#include <IFace\Bridge.h>

#include <PgsExt\StrandData.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CLRFDCreepCoefficientChapterBuilder
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CLRFDCreepCoefficientChapterBuilder::CLRFDCreepCoefficientChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CLRFDCreepCoefficientChapterBuilder::GetName() const
{
   return TEXT("Creep Coefficient Details");
}

rptChapter* CLRFDCreepCoefficientChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CGirderReportSpecification* pGirderRptSpec = dynamic_cast<CGirderReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pGirderRptSpec->GetBroker(&pBroker);
   const CGirderKey& girderKey(pGirderRptSpec->GetGirderKey());

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,ISegmentData,pSegmentData);

   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);
   rptParagraph* pPara;

   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      CSegmentKey segmentKey(girderKey,segIdx);

      const CStrandData* pStrands = pSegmentData->GetStrandData(segmentKey);
      bool bTempStrands = (0 < pStrands->GetStrandCount(pgsTypes::Temporary) && pStrands->GetTemporaryStrandUsage() != pgsTypes::ttsPTBeforeShipping) ? true : false;

      switch( deckType )
      {
      case pgsTypes::sdtCompositeCIP:
      case pgsTypes::sdtCompositeOverlay:
         pPara = (bTempStrands ? Build_CIP_TempStrands(pRptSpec, pBroker, segmentKey, pDisplayUnits,level) : Build_CIP(pRptSpec, pBroker, segmentKey, pDisplayUnits,level));
         break;

      case pgsTypes::sdtCompositeSIP:
         pPara = (bTempStrands ? Build_SIP_TempStrands(pRptSpec, pBroker, segmentKey, pDisplayUnits,level) : Build_SIP(pRptSpec, pBroker, segmentKey, pDisplayUnits,level));
         break;

      case pgsTypes::sdtNone:
         pPara = (bTempStrands ? Build_NoDeck_TempStrands(pRptSpec, pBroker, segmentKey, pDisplayUnits,level) : Build_NoDeck(pRptSpec, pBroker, segmentKey, pDisplayUnits,level));
         break;

      default:
         ATLASSERT(false);
      }
   }

   *pChapter << pPara;
   return pChapter;
}

CChapterBuilder* CLRFDCreepCoefficientChapterBuilder::Clone() const
{
   return new CLRFDCreepCoefficientChapterBuilder;
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

rptParagraph* CLRFDCreepCoefficientChapterBuilder::Build_CIP_TempStrands(CReportSpecification* pRptSpec,IBroker* pBroker,const CSegmentKey& segmentKey,
                                                   IEAFDisplayUnits* pDisplayUnits,
                                                   Uint16 level) const
{
   rptParagraph* pPara = new rptParagraph;

   GET_IFACE2(pBroker,ICamber,pCamber);

   GET_IFACE2(pBroker,ILibrary,pLib);
   GET_IFACE2(pBroker,ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());

   INIT_UV_PROTOTYPE( rptTimeUnitValue, time, pDisplayUnits->GetWholeDaysUnit(), true );
   INIT_UV_PROTOTYPE( rptTimeUnitValue, time2, pDisplayUnits->GetWholeDaysUnit(), false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, fc, pDisplayUnits->GetStressUnit(), true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, length, pDisplayUnits->GetComponentDimUnit(), true );

   CREEPCOEFFICIENTDETAILS details;

   //////////////////////////////
   // Report LRFD method
   //////////////////////////////
   bool bSI = IS_SI_UNITS(pDisplayUnits);

   for ( Int16 i = CREEP_MINTIME; i <= CREEP_MAXTIME; i++ )
   {
      if ( i == CREEP_MINTIME )
      {
         // firs time through loop, report the common information
         details = pCamber->GetCreepCoefficientDetails(segmentKey,ICamber::cpReleaseToDiaphragm,i);

         if ( details.Spec == CREEP_SPEC_PRE_2005 )
         {
            *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("LRFDCreepEqn.png")) << rptNewLine;
            *pPara << Bold(_T("for which:")) << rptNewLine;
            *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + (bSI ? _T("KfEqn-SI.png") : _T("KfEqn-US.png")) ) << rptNewLine;
            *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + (bSI ? _T("KcEqn-SI.png") : _T("KcEqn-US.png")) ) << rptNewLine;

            *pPara << Bold(_T("where:")) << rptNewLine;
            *pPara << _T("H = ") << details.H << _T("%") << _T(", ");
            *pPara << _T("V/S = ") << length.SetValue(details.VSratio) << rptNewLine;
            *pPara << _T("In determining the maturity of concrete at initial load, t") << Sub(_T("i"))
                   << _T(", 1 day of accelerated curing may be taken as equal to ") << pSpecEntry->GetCuringMethodTimeAdjustmentFactor() << _T(" days of normal curing.") << rptNewLine;
            *pPara << _T("Curing Method = ") << (details.CuringMethod == CURING_ACCELERATED ? _T("Accelerated") : _T("Normal")) << rptNewLine;
            *pPara << rptNewLine;
         }
         else
         {
            if ( lrfdVersionMgr::FourthEdition2007 <= pSpecEntry->GetSpecificationType() )
            {
               *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("LRFDCreepEqn2007.png")) << rptNewLine;
            }
            else
            {
               *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("LRFDCreepEqn2005.png")) << rptNewLine;
            }

            *pPara << Bold(_T("for which:")) << rptNewLine;
            
            if ( pSpecEntry->GetSpecificationType() <= lrfdVersionMgr::ThirdEditionWith2005Interims )
            {
               *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + (bSI ? _T("KvsEqn-SI.png") : _T("KvsEqn-US.png")) ) << rptNewLine;
            }
            else if ( lrfdVersionMgr::ThirdEditionWith2006Interims == pSpecEntry->GetSpecificationType())
            {
               *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + (bSI ? _T("KvsEqn2006-SI.png") : _T("KvsEqn2006-US.png")) ) << rptNewLine;
            }
            else
            {
               *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + (bSI ? _T("KvsEqn2007-SI.png") : _T("KvsEqn2007-US.png")) ) << rptNewLine;
            }

            *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("KhcEqn.png") ) << rptNewLine;
            *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + (bSI ? _T("KfEqn2005-SI.png") : _T("KfEqn2005-US.png")) ) << rptNewLine;
            *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + (bSI ? _T("KtdEqn-SI.png") : _T("KtdEqn-US.png")) ) << rptNewLine;

            *pPara << Bold(_T("where:")) << rptNewLine;
            *pPara << _T("H = ") << details.H << _T("%") << _T(", ");
            *pPara << _T("V/S = ") << length.SetValue(details.VSratio) << _T(", ");
            *pPara << Sub2(_T("K"),_T("1")) << _T(" = ") << details.K1 << _T(", ");
            *pPara << Sub2(_T("K"),_T("2")) << _T(" = ") << details.K2 << rptNewLine;
            if ( pSpecEntry->GetSpecificationType() < lrfdVersionMgr::FourthEdition2007 )
            {
               *pPara << _T("In determining the maturity of concrete at initial load, t") << Sub(_T("i"))
                      << _T(", 1 day of accelerated curing may be taken as equal to ") << pSpecEntry->GetCuringMethodTimeAdjustmentFactor() << _T(" days of normal curing.") << rptNewLine;
            }
            *pPara << _T("Curing Method = ") << (details.CuringMethod == CURING_ACCELERATED ? _T("Accelerated") : _T("Normal")) << rptNewLine;
            *pPara << rptNewLine;
         } // spec
      } // i

      *pPara << Bold((i == CREEP_MINTIME ? _T("Minimum Timing") : _T("Maximum Timing"))) << rptNewLine;

      details = pCamber->GetCreepCoefficientDetails(segmentKey,ICamber::cpReleaseToDiaphragm,i);

      if ( details.Spec == CREEP_SPEC_PRE_2005 )
      {
         *pPara << rptNewLine;
         *pPara << Bold(_T("Prestress release until temporary strand removal and diaphragm casting")) <<rptNewLine;
         *pPara << RPT_FCI << _T(" = ") << fc.SetValue( details.Fc ) << _T(", ");
         *pPara << _T("t") << Sub(_T("i")) << _T(" (Adjusted) = ") << time.SetValue(details.ti) << _T(", ");
         *pPara << _T("t = ")<< time.SetValue(details.t) << _T(", ");
         *pPara << _T("k") << Sub(_T("f")) << _T(" = ") << details.kf << _T(", ");
         *pPara << _T("k") << Sub(_T("c")) << _T(" = ") << details.kc << rptNewLine;
         *pPara << symbol(psi) << _T("(")<<time2.SetValue( details.t);
         *pPara <<_T(",")<<time2.SetValue(details.ti)<<_T(") = ")<< details.Ct << rptNewLine;

         *pPara << rptNewLine;

         details = pCamber->GetCreepCoefficientDetails(segmentKey,ICamber::cpReleaseToDeck,i);
         *pPara << Bold(_T("Prestress release until deck casting")) << rptNewLine;
         *pPara << RPT_FCI << _T(" = ") << fc.SetValue( details.Fc ) << _T(", ");
         *pPara << _T("t") << Sub(_T("i")) << _T(" = ") << time.SetValue( details.ti) << _T(", ");
         *pPara << _T("t = ")<< time.SetValue( details.t ) << _T(", ");
         *pPara << _T("k") << Sub(_T("f")) << _T(" = ") << details.kf << _T(", ");
         *pPara << _T("k") << Sub(_T("c")) << _T(" = ") << details.kc << rptNewLine;
         *pPara << symbol(psi) << _T("(")<<time2.SetValue( details.t);
         *pPara <<_T(",")<<time2.SetValue(details.ti)<<_T(") = ")<< details.Ct << rptNewLine;

         *pPara << rptNewLine;

         details = pCamber->GetCreepCoefficientDetails(segmentKey,ICamber::cpDiaphragmToDeck,i);
         *pPara << Bold(_T("Temporary strand removal and diaphragm casting to deck casting and application of superimposed dead loads")) << rptNewLine;
         *pPara << RPT_FCI << _T(" = ") << fc.SetValue( details.Fc ) << _T(", ");
         *pPara << _T("t") << Sub(_T("i")) << _T(" = ") << time.SetValue( details.ti) << _T(", ");
         *pPara << _T("t = ")<< time.SetValue( details.t ) << _T(", ");
         *pPara << _T("k") << Sub(_T("f")) << _T(" = ") << details.kf << _T(", ");
         *pPara << _T("k") << Sub(_T("c")) << _T(" = ") << details.kc << rptNewLine;
         *pPara << symbol(psi) << _T("(")<<time2.SetValue( details.t);
         *pPara <<_T(",")<<time2.SetValue(details.ti)<<_T(") = ")<< details.Ct << rptNewLine;
      }
      else
      {
         // 2005 and later
         *pPara << rptNewLine;
         *pPara << Bold(_T("Prestress release until temporary strand removal and diaphragm casting")) <<rptNewLine;
         *pPara << RPT_FCI << _T(" = ") << fc.SetValue( details.Fc ) << _T(", ");
         *pPara << _T("t") << Sub(_T("i")) << _T(" (Adjusted) = ") << time.SetValue(details.ti) << _T(", ");
         *pPara << _T("t = ")<< time.SetValue(details.t) << _T(", ");

         if ( lrfdVersionMgr::FourthEdition2007 <= pSpecEntry->GetSpecificationType() )
         {
            *pPara << _T("k") << Sub(_T("s")) << _T(" = ") << details.kvs << _T(", ");
         }
         else
         {
            *pPara << _T("k") << Sub(_T("vs")) << _T(" = ") << details.kvs << _T(", ");
         }

         *pPara << _T("k") << Sub(_T("hc")) << _T(" = ") << details.khc << _T(", ");
         *pPara << _T("k") << Sub(_T("f")) << _T(" = ") << details.kf << _T(", ");
         *pPara << _T("k") << Sub(_T("td")) << _T(" = ") << details.ktd << rptNewLine;
         *pPara << symbol(psi) << _T("(")<<time2.SetValue(details.t);
         *pPara <<_T(",")<<time2.SetValue(details.ti)<<_T(") = ")<< details.Ct << rptNewLine;

         *pPara << rptNewLine;

         details = pCamber->GetCreepCoefficientDetails(segmentKey,ICamber::cpReleaseToDeck,i);
         *pPara << Bold(_T("Prestress release until deck casting")) << rptNewLine;
         *pPara << RPT_FCI << _T(" = ") << fc.SetValue( details.Fc ) << _T(", ");
         *pPara << _T("t") << Sub(_T("i")) << _T(" = ") << time.SetValue( details.ti) << _T(", ");
         *pPara << _T("t = ")<< time.SetValue(details.t) << _T(", ");

         if ( lrfdVersionMgr::FourthEdition2007 <= pSpecEntry->GetSpecificationType() )
         {
            *pPara << _T("k") << Sub(_T("s")) << _T(" = ") << details.kvs << _T(", ");
         }
         else
         {
            *pPara << _T("k") << Sub(_T("vs")) << _T(" = ") << details.kvs << _T(", ");
         }

         *pPara << _T("k") << Sub(_T("hc")) << _T(" = ") << details.khc << _T(", ");
         *pPara << _T("k") << Sub(_T("f")) << _T(" = ") << details.kf << _T(", ");
         *pPara << _T("k") << Sub(_T("td")) << _T(" = ") << details.ktd << rptNewLine;
         *pPara << symbol(psi) << _T("(")<<time2.SetValue(details.t);
         *pPara <<_T(",")<<time2.SetValue(details.ti)<<_T(") = ")<< details.Ct << rptNewLine;

         *pPara << rptNewLine;

         details = pCamber->GetCreepCoefficientDetails(segmentKey,ICamber::cpDiaphragmToDeck,i);
         *pPara << Bold(_T("Temporary strand removal and diaphragm casting to deck casting and application of superimposed dead loads")) << rptNewLine;
         *pPara << RPT_FCI << _T(" = ") << fc.SetValue( details.Fc ) << _T(", ");
         *pPara << _T("t") << Sub(_T("i")) << _T(" = ") << time.SetValue( details.ti) << _T(", ");
         *pPara << _T("t = ")<< time.SetValue(details.t) << _T(", ");

         if ( lrfdVersionMgr::FourthEdition2007 <= pSpecEntry->GetSpecificationType() )
         {
            *pPara << _T("k") << Sub(_T("s")) << _T(" = ") << details.kvs << _T(", ");
         }
         else
         {
            *pPara << _T("k") << Sub(_T("vs")) << _T(" = ") << details.kvs << _T(", ");
         }

         *pPara << _T("k") << Sub(_T("hc")) << _T(" = ") << details.khc << _T(", ");
         *pPara << _T("k") << Sub(_T("f")) << _T(" = ") << details.kf << _T(", ");
         *pPara << _T("k") << Sub(_T("td")) << _T(" = ") << details.ktd << rptNewLine;
         *pPara << symbol(psi) << _T("(")<<time2.SetValue(details.t);
         *pPara <<_T(",")<<time2.SetValue(details.ti)<<_T(") = ")<< details.Ct << rptNewLine;
      } // spec type

   *pPara << rptNewLine;
   }// loop on i


   return pPara;
}

rptParagraph* CLRFDCreepCoefficientChapterBuilder::Build_CIP(CReportSpecification* pRptSpec,IBroker* pBroker,const CSegmentKey& segmentKey,
                                                       IEAFDisplayUnits* pDisplayUnits,
                                                       Uint16 level) const
{
   rptParagraph* pPara = new rptParagraph;

   GET_IFACE2(pBroker,ICamber,pCamber);

   GET_IFACE2(pBroker,ILibrary,pLib);
   GET_IFACE2(pBroker,ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());

   INIT_UV_PROTOTYPE( rptTimeUnitValue, time, pDisplayUnits->GetWholeDaysUnit(), true );
   INIT_UV_PROTOTYPE( rptTimeUnitValue, time2, pDisplayUnits->GetWholeDaysUnit(), false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, fc, pDisplayUnits->GetStressUnit(), true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, length, pDisplayUnits->GetComponentDimUnit(), true );

   CREEPCOEFFICIENTDETAILS details;

   //////////////////////////////
   // Report LRFD method
   //////////////////////////////
   bool bSI = IS_SI_UNITS(pDisplayUnits);

   for ( Int16 i = CREEP_MINTIME; i <= CREEP_MAXTIME; i++ )
   {
      details = pCamber->GetCreepCoefficientDetails(segmentKey,ICamber::cpReleaseToDeck,i);
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
            *pPara << _T("H = ") << details.H << _T("%") << _T(", ");
            *pPara << _T("V/S = ") << length.SetValue(details.VSratio) << rptNewLine;
            *pPara << _T("In determining the maturity of concrete at initial load, t") << Sub(_T("i"))
                   << _T(", 1 day of accelerated curing may be taken as equal to ") << pSpecEntry->GetCuringMethodTimeAdjustmentFactor() << _T(" days of normal curing.") << rptNewLine;
            *pPara << _T("Curing Method = ") << (details.CuringMethod == CURING_ACCELERATED ? _T("Accelerated") : _T("Normal")) << rptNewLine;
            *pPara << rptNewLine;
         }
         else
         {
            if ( lrfdVersionMgr::FourthEdition2007 <= pSpecEntry->GetSpecificationType() )
            {
               *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("LRFDCreepEqn2007.png")) << rptNewLine;
            }
            else
            {
               *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("LRFDCreepEqn2005.png")) << rptNewLine;
            }
            *pPara << Bold(_T("for which:")) << rptNewLine;

            if ( pSpecEntry->GetSpecificationType() <= lrfdVersionMgr::ThirdEditionWith2005Interims )
            {
               *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + (bSI ? _T("KvsEqn-SI.png") : _T("KvsEqn-US.png")) ) << rptNewLine;
            }
            else if ( lrfdVersionMgr::ThirdEditionWith2006Interims == pSpecEntry->GetSpecificationType())
            {
               *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + (bSI ? _T("KvsEqn2006-SI.png") : _T("KvsEqn2006-US.png")) ) << rptNewLine;
            }
            else
            {
               *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + (bSI ? _T("KvsEqn2007-SI.png") : _T("KvsEqn2007-US.png")) ) << rptNewLine;
            }

            *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("KhcEqn.png") ) << rptNewLine;
            *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + (bSI ? _T("KfEqn2005-SI.png") : _T("KfEqn2005-US.png")) ) << rptNewLine;
            *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + (bSI ? _T("KtdEqn-SI.png") : _T("KtdEqn-US.png")) ) << rptNewLine;

            *pPara << Bold(_T("where:")) << rptNewLine;
            *pPara << _T("H = ") << details.H << _T("%") << _T(", ");
            *pPara << _T("V/S = ") << length.SetValue(details.VSratio) << _T(", ");
            *pPara << Sub2(_T("K"),_T("1")) << _T(" = ") << details.K1 << _T(", ");
            *pPara << Sub2(_T("K"),_T("2")) << _T(" = ") << details.K2 << rptNewLine;

            if ( pSpecEntry->GetSpecificationType() < lrfdVersionMgr::FourthEdition2007 )
            {
              *pPara << _T("In determining the maturity of concrete at initial load, t") << Sub(_T("i"))
                     << _T(", 1 day of accelerated curing may be taken as equal to ") << pSpecEntry->GetCuringMethodTimeAdjustmentFactor() << _T(" days of normal curing.") << rptNewLine;
            }
            *pPara << _T("Curing Method = ") << (details.CuringMethod == CURING_ACCELERATED ? _T("Accelerated") : _T("Normal")) << rptNewLine;
            *pPara << rptNewLine;
         } // spec
      } // i

      *pPara << Bold((i == CREEP_MINTIME ? _T("Minimum Timing") : _T("Maximum Timing"))) << rptNewLine;

      if ( details.Spec == CREEP_SPEC_PRE_2005 )
      {
         *pPara << rptNewLine;
         *pPara << Bold(_T("Prestress release until deck casting")) <<rptNewLine;
         *pPara << RPT_FCI << _T(" = ") << fc.SetValue( details.Fc ) << _T(", ");
         *pPara << _T("t") << Sub(_T("i")) << _T(" (Adjusted) = ") << time.SetValue(details.ti) << _T(", ");
         *pPara << _T("t = ")<< time.SetValue(details.t) << _T(", ");
         *pPara << _T("k") << Sub(_T("f")) << _T(" = ") << details.kf << _T(", ");
         *pPara << _T("k") << Sub(_T("c")) << _T(" = ") << details.kc << rptNewLine;
         *pPara << symbol(psi) << _T("(")<<time2.SetValue( details.t);
         *pPara <<_T(",")<<time2.SetValue(details.ti)<<_T(") = ")<< details.Ct << rptNewLine;
      }
      else
      {
         // 2005 and later
         *pPara << rptNewLine;
         *pPara << Bold(_T("Prestress release until deck casting")) <<rptNewLine;
         *pPara << RPT_FCI << _T(" = ") << fc.SetValue( details.Fc ) << _T(", ");
         *pPara << _T("t") << Sub(_T("i")) << _T(" (Adjusted) = ") << time.SetValue(details.ti) << _T(", ");
         *pPara << _T("t = ")<< time.SetValue(details.t) << _T(", ");

         if ( lrfdVersionMgr::FourthEdition2007 <= pSpecEntry->GetSpecificationType() )
         {
            *pPara << _T("k") << Sub(_T("s")) << _T(" = ") << details.kvs << _T(", ");
         }
         else
         {
            *pPara << _T("k") << Sub(_T("vs")) << _T(" = ") << details.kvs << _T(", ");
         }

         *pPara << _T("k") << Sub(_T("hc")) << _T(" = ") << details.khc << _T(", ");
         *pPara << _T("k") << Sub(_T("f")) << _T(" = ") << details.kf << _T(", ");
         *pPara << _T("k") << Sub(_T("td")) << _T(" = ") << details.ktd << rptNewLine;
         *pPara << symbol(psi) << _T("(")<<time2.SetValue(details.t);
         *pPara <<_T(",")<<time2.SetValue(details.ti)<<_T(") = ")<< details.Ct << rptNewLine;
      } // spec type

      *pPara << rptNewLine;
   }// loop on i


   return pPara;
}

rptParagraph* CLRFDCreepCoefficientChapterBuilder::Build_SIP_TempStrands(CReportSpecification* pRptSpec,IBroker* pBroker,const CSegmentKey& segmentKey,
                                                                   IEAFDisplayUnits* pDisplayUnits,
                                                                   Uint16 level) const
{
   return Build_CIP_TempStrands(pRptSpec, pBroker, segmentKey, pDisplayUnits, level);
}

rptParagraph* CLRFDCreepCoefficientChapterBuilder::Build_SIP(CReportSpecification* pRptSpec,IBroker* pBroker,const CSegmentKey& segmentKey,
                                                       IEAFDisplayUnits* pDisplayUnits,
                                                       Uint16 level) const
{
   return Build_SIP_TempStrands(pRptSpec, pBroker, segmentKey, pDisplayUnits, level);
}

rptParagraph* CLRFDCreepCoefficientChapterBuilder::Build_NoDeck_TempStrands(CReportSpecification* pRptSpec,IBroker* pBroker,const CSegmentKey& segmentKey,
                                                                      IEAFDisplayUnits* pDisplayUnits,
                                                                      Uint16 level) const
{
   rptParagraph* pPara = new rptParagraph;

   GET_IFACE2(pBroker,ICamber,pCamber);

   GET_IFACE2(pBroker,ILibrary,pLib);
   GET_IFACE2(pBroker,ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());

   INIT_UV_PROTOTYPE( rptTimeUnitValue, time, pDisplayUnits->GetWholeDaysUnit(), true );
   INIT_UV_PROTOTYPE( rptTimeUnitValue, time2, pDisplayUnits->GetWholeDaysUnit(), false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, fc, pDisplayUnits->GetStressUnit(), true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, length, pDisplayUnits->GetComponentDimUnit(), true );

   CREEPCOEFFICIENTDETAILS details;

   //////////////////////////////
   // Report LRFD method
   //////////////////////////////
   bool bSI = IS_SI_UNITS(pDisplayUnits);

   for ( Int16 i = CREEP_MINTIME; i <= CREEP_MAXTIME; i++ )
   {
     details = pCamber->GetCreepCoefficientDetails(segmentKey,ICamber::cpReleaseToDiaphragm,i);
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
           *pPara << _T("H = ") << details.H << _T("%") << _T(", ");
           *pPara << _T("V/S = ") << length.SetValue(details.VSratio) << rptNewLine;
           *pPara << _T("In determining the maturity of concrete at initial load, t") << Sub(_T("i"))
                  << _T(", 1 day of accelerated curing may be taken as equal to ") << pSpecEntry->GetCuringMethodTimeAdjustmentFactor() << _T(" days of normal curing.") << rptNewLine;
           *pPara << _T("Curing Method = ") << (details.CuringMethod == CURING_ACCELERATED ? _T("Accelerated") : _T("Normal")) << rptNewLine;
           *pPara << rptNewLine;
        }
        else
        {
            if ( lrfdVersionMgr::FourthEdition2007 <= pSpecEntry->GetSpecificationType() )
            {
               *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("LRFDCreepEqn2007.png")) << rptNewLine;
            }
            else
            {
               *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("LRFDCreepEqn2005.png")) << rptNewLine;
            }
            *pPara << Bold(_T("for which:")) << rptNewLine;
            
            if ( pSpecEntry->GetSpecificationType() <= lrfdVersionMgr::ThirdEditionWith2005Interims )
            {
               *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + (bSI ? _T("KvsEqn-SI.png") : _T("KvsEqn-US.png")) ) << rptNewLine;
            }
            else if ( lrfdVersionMgr::ThirdEditionWith2006Interims == pSpecEntry->GetSpecificationType())
            {
               *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + (bSI ? _T("KvsEqn2006-SI.png") : _T("KvsEqn2006-US.png")) ) << rptNewLine;
            }
            else
            {
               *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + (bSI ? _T("KvsEqn2007-SI.png") : _T("KvsEqn2007-US.png")) ) << rptNewLine;
            }

           *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("KhcEqn.png") ) << rptNewLine;
           *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + (bSI ? _T("KfEqn2005-SI.png") : _T("KfEqn2005-US.png")) ) << rptNewLine;
           *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + (bSI ? _T("KtdEqn-SI.png") : _T("KtdEqn-US.png")) ) << rptNewLine;

           *pPara << Bold(_T("where:")) << rptNewLine;
           *pPara << _T("H = ") << details.H << _T("%") << _T(", ");
           *pPara << _T("V/S = ") << length.SetValue(details.VSratio) << _T(", ");
            *pPara << Sub2(_T("K"),_T("1")) << _T(" = ") << details.K1 << _T(", ");
            *pPara << Sub2(_T("K"),_T("2")) << _T(" = ") << details.K2 << rptNewLine;

           if ( pSpecEntry->GetSpecificationType() < lrfdVersionMgr::FourthEdition2007 )
           {
              *pPara << _T("In determining the maturity of concrete at initial load, t") << Sub(_T("i"))
                     << _T(", 1 day of accelerated curing may be taken as equal to ") << pSpecEntry->GetCuringMethodTimeAdjustmentFactor() << _T(" days of normal curing.") << rptNewLine;
           }
           *pPara << _T("Curing Method = ") << (details.CuringMethod == CURING_ACCELERATED ? _T("Accelerated") : _T("Normal")) << rptNewLine;
           *pPara << rptNewLine;
        } // spec
     } // i
     
     *pPara << Bold((i == CREEP_MINTIME ? _T("Minimum Timing") : _T("Maximum Timing"))) << rptNewLine;


      if ( details.Spec == CREEP_SPEC_PRE_2005 )
      {
         *pPara << rptNewLine;
         *pPara << Bold(_T("Prestress release until temporary strand removal and diaphragm casting")) <<rptNewLine;
         *pPara << RPT_FCI << _T(" = ") << fc.SetValue( details.Fc ) << _T(", ");
         *pPara << _T("t") << Sub(_T("i")) << _T(" (Adjusted) = ") << time.SetValue(details.ti) << _T(", ");
         *pPara << _T("t = ")<< time.SetValue(details.t) << _T(", ");
         *pPara << _T("k") << Sub(_T("f")) << _T(" = ") << details.kf << _T(", ");
         *pPara << _T("k") << Sub(_T("c")) << _T(" = ") << details.kc << rptNewLine;
         *pPara << symbol(psi) << _T("(")<<time2.SetValue( details.t);
         *pPara <<_T(",")<<time2.SetValue(details.ti)<<_T(") = ")<< details.Ct << rptNewLine;

         *pPara << rptNewLine;

         details = pCamber->GetCreepCoefficientDetails(segmentKey,ICamber::cpReleaseToDeck,i);
         *pPara << Bold(_T("Prestress release until application of superimposed dead loads")) << rptNewLine;
         *pPara << RPT_FCI << _T(" = ") << fc.SetValue( details.Fc ) << _T(", ");
         *pPara << _T("t") << Sub(_T("i")) << _T(" = ") << time.SetValue( details.ti) << _T(", ");
         *pPara << _T("t = ")<< time.SetValue( details.t ) << _T(", ");
         *pPara << _T("k") << Sub(_T("f")) << _T(" = ") << details.kf << _T(", ");
         *pPara << _T("k") << Sub(_T("c")) << _T(" = ") << details.kc << rptNewLine;
         *pPara << symbol(psi) << _T("(")<<time2.SetValue( details.t);
         *pPara <<_T(",")<<time2.SetValue(details.ti)<<_T(") = ")<< details.Ct << rptNewLine;

         *pPara << rptNewLine;

         details = pCamber->GetCreepCoefficientDetails(segmentKey,ICamber::cpReleaseToFinal,i);
         *pPara << Bold(_T("Prestress release to final")) << rptNewLine;
         *pPara << RPT_FCI << _T(" = ") << fc.SetValue( details.Fc ) << _T(", ");
         *pPara << _T("t") << Sub(_T("i")) << _T(" = ") << time.SetValue( details.ti) << _T(", ");
         *pPara << _T("t = ")<< time.SetValue( details.t ) << _T(", ");
         *pPara << _T("k") << Sub(_T("f")) << _T(" = ") << details.kf << _T(", ");
         *pPara << _T("k") << Sub(_T("c")) << _T(" = ") << details.kc << rptNewLine;
         *pPara << symbol(psi) << _T("(")<<time2.SetValue( details.t);
         *pPara <<_T(",")<<time2.SetValue(details.ti)<<_T(") = ")<< details.Ct << rptNewLine;

         *pPara << rptNewLine;

         details = pCamber->GetCreepCoefficientDetails(segmentKey,ICamber::cpDiaphragmToDeck,i);
         *pPara << Bold(_T("Temporary strand removal and diaphragm casting to application of superimposed dead loads")) << rptNewLine;
         *pPara << RPT_FCI << _T(" = ") << fc.SetValue( details.Fc ) << _T(", ");
         *pPara << _T("t") << Sub(_T("i")) << _T(" = ") << time.SetValue( details.ti) << _T(", ");
         *pPara << _T("t = ")<< time.SetValue( details.t ) << _T(", ");
         *pPara << _T("k") << Sub(_T("f")) << _T(" = ") << details.kf << _T(", ");
         *pPara << _T("k") << Sub(_T("c")) << _T(" = ") << details.kc << rptNewLine;
         *pPara << symbol(psi) << _T("(")<<time2.SetValue( details.t);
         *pPara <<_T(",")<<time2.SetValue(details.ti)<<_T(") = ")<< details.Ct << rptNewLine;

         *pPara << rptNewLine;

         details = pCamber->GetCreepCoefficientDetails(segmentKey,ICamber::cpDeckToFinal,i);
         *pPara << Bold(_T("Application of superimposed dead loads to final")) << rptNewLine;
         *pPara << RPT_FCI << _T(" = ") << fc.SetValue( details.Fc ) << _T(", ");
         *pPara << _T("t") << Sub(_T("i")) << _T(" = ") << time.SetValue( details.ti) << _T(", ");
         *pPara << _T("t = ")<< time.SetValue( details.t ) << _T(", ");
         *pPara << _T("k") << Sub(_T("f")) << _T(" = ") << details.kf << _T(", ");
         *pPara << _T("k") << Sub(_T("c")) << _T(" = ") << details.kc << rptNewLine;
         *pPara << symbol(psi) << _T("(")<<time2.SetValue( details.t);
         *pPara <<_T(",")<<time2.SetValue(details.ti)<<_T(") = ")<< details.Ct << rptNewLine;
      }
      else
      {
         // 2005 and later
         *pPara << rptNewLine;
         *pPara << Bold(_T("Prestress release until temporary strand removal and diaphragm casting")) <<rptNewLine;
         *pPara << RPT_FCI << _T(" = ") << fc.SetValue( details.Fc ) << _T(", ");
         *pPara << _T("t") << Sub(_T("i")) << _T(" (Adjusted) = ") << time.SetValue(details.ti) << _T(", ");
         *pPara << _T("t = ")<< time.SetValue(details.t) << _T(", ");

         if ( lrfdVersionMgr::FourthEdition2007 <= pSpecEntry->GetSpecificationType() )
         {
            *pPara << _T("k") << Sub(_T("s")) << _T(" = ") << details.kvs << _T(", ");
         }
         else
         {
            *pPara << _T("k") << Sub(_T("vs")) << _T(" = ") << details.kvs << _T(", ");
         }

         *pPara << _T("k") << Sub(_T("hc")) << _T(" = ") << details.khc << _T(", ");
         *pPara << _T("k") << Sub(_T("f")) << _T(" = ") << details.kf << _T(", ");
         *pPara << _T("k") << Sub(_T("td")) << _T(" = ") << details.ktd << rptNewLine;
         *pPara << symbol(psi) << _T("(")<<time2.SetValue(details.t);
         *pPara <<_T(",")<<time2.SetValue(details.ti)<<_T(") = ")<< details.Ct << rptNewLine;

         *pPara << rptNewLine;

         details = pCamber->GetCreepCoefficientDetails(segmentKey,ICamber::cpReleaseToDeck,i);
         *pPara << Bold(_T("Prestress release until application of superimposed dead loads")) << rptNewLine;
         *pPara << RPT_FCI << _T(" = ") << fc.SetValue( details.Fc ) << _T(", ");
         *pPara << _T("t") << Sub(_T("i")) << _T(" = ") << time.SetValue( details.ti) << _T(", ");
         *pPara << _T("t = ")<< time.SetValue(details.t) << _T(", ");

         if ( lrfdVersionMgr::FourthEdition2007 <= pSpecEntry->GetSpecificationType() )
         {
            *pPara << _T("k") << Sub(_T("s")) << _T(" = ") << details.kvs << _T(", ");
         }
         else
         {
            *pPara << _T("k") << Sub(_T("vs")) << _T(" = ") << details.kvs << _T(", ");
         }

         *pPara << _T("k") << Sub(_T("hc")) << _T(" = ") << details.khc << _T(", ");
         *pPara << _T("k") << Sub(_T("f")) << _T(" = ") << details.kf << _T(", ");
         *pPara << _T("k") << Sub(_T("td")) << _T(" = ") << details.ktd << rptNewLine;
         *pPara << symbol(psi) << _T("(")<<time2.SetValue(details.t);
         *pPara <<_T(",")<<time2.SetValue(details.ti)<<_T(") = ")<< details.Ct << rptNewLine;

         *pPara << rptNewLine;

         details = pCamber->GetCreepCoefficientDetails(segmentKey,ICamber::cpReleaseToFinal,i);
         *pPara << Bold(_T("Prestress release to final")) << rptNewLine;
         *pPara << RPT_FCI << _T(" = ") << fc.SetValue( details.Fc ) << _T(", ");
         *pPara << _T("t") << Sub(_T("i")) << _T(" = ") << time.SetValue( details.ti) << _T(", ");
         *pPara << _T("t = ")<< time.SetValue(details.t) << _T(", ");

         if ( lrfdVersionMgr::FourthEdition2007 <= pSpecEntry->GetSpecificationType() )
         {
            *pPara << _T("k") << Sub(_T("s")) << _T(" = ") << details.kvs << _T(", ");
         }
         else
         {
            *pPara << _T("k") << Sub(_T("vs")) << _T(" = ") << details.kvs << _T(", ");
         }

         *pPara << _T("k") << Sub(_T("hc")) << _T(" = ") << details.khc << _T(", ");
         *pPara << _T("k") << Sub(_T("f")) << _T(" = ") << details.kf << _T(", ");
         *pPara << _T("k") << Sub(_T("td")) << _T(" = ") << details.ktd << rptNewLine;
         *pPara << symbol(psi) << _T("(")<<time2.SetValue(details.t);
         *pPara <<_T(",")<<time2.SetValue(details.ti)<<_T(") = ")<< details.Ct << rptNewLine;

         *pPara << rptNewLine;

         details = pCamber->GetCreepCoefficientDetails(segmentKey,ICamber::cpDiaphragmToDeck,i);
         *pPara << Bold(_T("Temporary strand removal and diaphragm casting to application of superimposed dead loads")) << rptNewLine;
         *pPara << RPT_FCI << _T(" = ") << fc.SetValue( details.Fc ) << _T(", ");
         *pPara << _T("t") << Sub(_T("i")) << _T(" = ") << time.SetValue( details.ti) << _T(", ");
         *pPara << _T("t = ")<< time.SetValue(details.t) << _T(", ");

         if ( lrfdVersionMgr::FourthEdition2007 <= pSpecEntry->GetSpecificationType() )
         {
            *pPara << _T("k") << Sub(_T("s")) << _T(" = ") << details.kvs << _T(", ");
         }
         else
         {
            *pPara << _T("k") << Sub(_T("vs")) << _T(" = ") << details.kvs << _T(", ");
         }

         *pPara << _T("k") << Sub(_T("hc")) << _T(" = ") << details.khc << _T(", ");
         *pPara << _T("k") << Sub(_T("f")) << _T(" = ") << details.kf << _T(", ");
         *pPara << _T("k") << Sub(_T("td")) << _T(" = ") << details.ktd << rptNewLine;
         *pPara << symbol(psi) << _T("(")<<time2.SetValue(details.t);
         *pPara <<_T(",")<<time2.SetValue(details.ti)<<_T(") = ")<< details.Ct << rptNewLine;

         *pPara << rptNewLine;

         details = pCamber->GetCreepCoefficientDetails(segmentKey,ICamber::cpDiaphragmToFinal,i);
         *pPara << Bold(_T("Temporary strand removal and diaphragm casting to final")) << rptNewLine;
         *pPara << RPT_FCI << _T(" = ") << fc.SetValue( details.Fc ) << _T(", ");
         *pPara << _T("t") << Sub(_T("i")) << _T(" = ") << time.SetValue( details.ti) << _T(", ");
         *pPara << _T("t = ")<< time.SetValue(details.t) << _T(", ");

         if ( lrfdVersionMgr::FourthEdition2007 <= pSpecEntry->GetSpecificationType() )
         {
            *pPara << _T("k") << Sub(_T("s")) << _T(" = ") << details.kvs << _T(", ");
         }
         else
         {
            *pPara << _T("k") << Sub(_T("vs")) << _T(" = ") << details.kvs << _T(", ");
         }

         *pPara << _T("k") << Sub(_T("hc")) << _T(" = ") << details.khc << _T(", ");
         *pPara << _T("k") << Sub(_T("f")) << _T(" = ") << details.kf << _T(", ");
         *pPara << _T("k") << Sub(_T("td")) << _T(" = ") << details.ktd << rptNewLine;
         *pPara << symbol(psi) << _T("(")<<time2.SetValue(details.t);
         *pPara <<_T(",")<<time2.SetValue(details.ti)<<_T(") = ")<< details.Ct << rptNewLine;

         *pPara << rptNewLine;

         details = pCamber->GetCreepCoefficientDetails(segmentKey,ICamber::cpDeckToFinal,i);
         *pPara << Bold(_T("Application of superimposed dead loads to final")) << rptNewLine;
         *pPara << RPT_FCI << _T(" = ") << fc.SetValue( details.Fc ) << _T(", ");
         *pPara << _T("t") << Sub(_T("i")) << _T(" = ") << time.SetValue( details.ti) << _T(", ");
         *pPara << _T("t = ")<< time.SetValue(details.t) << _T(", ");

         if ( lrfdVersionMgr::FourthEdition2007 <= pSpecEntry->GetSpecificationType() )
         {
            *pPara << _T("k") << Sub(_T("s")) << _T(" = ") << details.kvs << _T(", ");
         }
         else
         {
            *pPara << _T("k") << Sub(_T("vs")) << _T(" = ") << details.kvs << _T(", ");
         }

         *pPara << _T("k") << Sub(_T("hc")) << _T(" = ") << details.khc << _T(", ");
         *pPara << _T("k") << Sub(_T("f")) << _T(" = ") << details.kf << rptNewLine;
         *pPara << _T("k") << Sub(_T("td")) << _T(" = ") << details.ktd << rptNewLine;
         *pPara << symbol(psi) << _T("(")<<time2.SetValue(details.t);
         *pPara <<_T(",")<<time2.SetValue(details.ti)<<_T(") = ")<< details.Ct << rptNewLine;
      } // spec type

      *pPara << rptNewLine;
   }// loop on i


   return pPara;
}

rptParagraph* CLRFDCreepCoefficientChapterBuilder::Build_NoDeck(CReportSpecification* pRptSpec,IBroker* pBroker,const CSegmentKey& segmentKey,
                                                          IEAFDisplayUnits* pDisplayUnits,
                                                          Uint16 level) const
{
   return Build_NoDeck_TempStrands(pRptSpec, pBroker, segmentKey, pDisplayUnits, level);
}