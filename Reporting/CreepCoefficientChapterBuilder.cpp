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
      *pPara << color(Red) << bold(ON) << "Changes to LRFD 4th Edition, 2007, Article 5.4.2.3.2 have been ignored." << bold(OFF) << color(Black) << rptNewLine;
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
            *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + "LRFDCreepEqn.png") << rptNewLine;
            *pPara << Bold("for which:") << rptNewLine;
            *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + (bSI ? "KfEqn-SI.png" : "KfEqn-US.png") ) << rptNewLine;
            *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + (bSI ? "KcEqn-SI.png" : "KcEqn-US.png") ) << rptNewLine;

            *pPara << Bold("where:") << rptNewLine;
            *pPara << "H = " << details.H << "%" << rptNewLine;
            *pPara << "V/S = " << length.SetValue(details.VSratio) << rptNewLine;
            *pPara << "In determining the maturity of concrete at initial load, t" << Sub("i")
                   << ", one day of accelerated curing may be taken as equal to seven days of normal curing." << rptNewLine;
            *pPara << "Curing Method = " << (details.CuringMethod == CURING_ACCELERATED ? "Accelerated" : "Normal") << rptNewLine;
            *pPara << rptNewLine;
         }
         else
         {
#if defined IGNORE_2007_CHANGES
            *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + "LRFDCreepEqn2005.png") << rptNewLine;
            *pPara << Bold("for which:") << rptNewLine;
            
            if ( pSpecEntry->GetSpecificationType() <= lrfdVersionMgr::ThirdEditionWith2005Interims )
               *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + (bSI ? "KvsEqn-SI.png" : "KvsEqn-US.png") ) << rptNewLine;
            else
               *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + (bSI ? "KvsEqn2006-SI.png" : "KvsEqn2006-US.png") ) << rptNewLine;
#else
            if ( lrfdVersionMgr::FourthEdition2007 <= pSpecEntry->GetSpecificationType() )
               *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + "LRFDCreepEqn2007.png") << rptNewLine;
            else
               *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + "LRFDCreepEqn2005.png") << rptNewLine;

            *pPara << Bold("for which:") << rptNewLine;
            
            if ( pSpecEntry->GetSpecificationType() <= lrfdVersionMgr::ThirdEditionWith2005Interims || lrfdVersionMgr::FourthEdition2007 <= pSpecEntry->GetSpecificationType())
               *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + (bSI ? "KvsEqn-SI.png" : "KvsEqn-US.png") ) << rptNewLine;
            else
               *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + (bSI ? "KvsEqn2006-SI.png" : "KvsEqn2006-US.png") ) << rptNewLine;
#endif // IGNORE_2007_CHANGES

            *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + "KhcEqn.png" ) << rptNewLine;
            *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + (bSI ? "KfEqn2005-SI.png" : "KfEqn2005-US.png") ) << rptNewLine;
            *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + (bSI ? "KtdEqn-SI.png" : "KtdEqn-US.png") ) << rptNewLine;

            *pPara << Bold("where:") << rptNewLine;
            *pPara << "H = " << details.H << "%" << rptNewLine;
            *pPara << "V/S = " << length.SetValue(details.VSratio) << rptNewLine;
            *pPara << Sub2("K","1") << " = " << details.K1 << rptNewLine;
            *pPara << Sub2("K","2") << " = " << details.K2 << rptNewLine;
#if defined IGNORE_2007_CHANGES
           *pPara << "In determining the maturity of concrete at initial load, t" << Sub("i")
                  << ", one day of accelerated curing may be taken as equal to seven days of normal curing." << rptNewLine;
#else
           if ( pSpecEntry->GetSpecificationType() < lrfdVersionMgr::FourthEdition2007 )
           {
              *pPara << "In determining the maturity of concrete at initial load, t" << Sub("i")
                     << ", one day of accelerated curing may be taken as equal to seven days of normal curing." << rptNewLine;
           }
#endif
            *pPara << "Curing Method = " << (details.CuringMethod == CURING_ACCELERATED ? "Accelerated" : "Normal") << rptNewLine;
            *pPara << rptNewLine;
         } // spec
      } // i

      *pPara << Bold((i == CREEP_MINTIME ? "Minimum Timing" : "Maximum Timing")) << rptNewLine;

      details = pCamber->GetCreepCoefficientDetails(span,gdr,ICamber::cpReleaseToDiaphragm,i);

      if ( details.Spec == CREEP_SPEC_PRE_2005 )
      {
         *pPara << rptNewLine;
         *pPara << Bold("Prestress release until temporary strand removal and diaphragm casting") <<rptNewLine;
         *pPara << RPT_FC << " = " << fc.SetValue( details.Fc ) << rptNewLine;
         *pPara << "t" << Sub("i") << " (Adjusted) = " << time.SetValue(details.ti) << rptNewLine;
         *pPara << "t = "<< time.SetValue(details.t) << rptNewLine;
         *pPara << "k" << Sub("f") << " = " << details.kf << rptNewLine;
         *pPara << "k" << Sub("c") << " = " << details.kc << rptNewLine;
         *pPara << symbol(psi) << "("<<time2.SetValue( details.t);
         *pPara <<","<<time2.SetValue(details.ti)<<") = "<< details.Ct << rptNewLine;

         *pPara << rptNewLine;

         details = pCamber->GetCreepCoefficientDetails(span,gdr,ICamber::cpReleaseToDeck,i);
         *pPara << Bold("Prestress release until deck casting") << rptNewLine;
         *pPara << RPT_FC << " = " << fc.SetValue( details.Fc ) << rptNewLine;
         *pPara << "t" << Sub("i") << " = " << time.SetValue( details.ti) << rptNewLine;
         *pPara << "t = "<< time.SetValue( details.t ) << rptNewLine;
         *pPara << "k" << Sub("f") << " = " << details.kf << rptNewLine;
         *pPara << "k" << Sub("c") << " = " << details.kc << rptNewLine;
         *pPara << symbol(psi) << "("<<time2.SetValue( details.t);
         *pPara <<","<<time2.SetValue(details.ti)<<") = "<< details.Ct << rptNewLine;

         *pPara << rptNewLine;

         details = pCamber->GetCreepCoefficientDetails(span,gdr,ICamber::cpDiaphragmToDeck,i);
         *pPara << Bold("Temporary strand removal and diaphragm casting to deck casting and application of superimposed dead loads") << rptNewLine;
         *pPara << RPT_FC << " = " << fc.SetValue( details.Fc ) << rptNewLine;
         *pPara << "t" << Sub("i") << " = " << time.SetValue( details.ti) << rptNewLine;
         *pPara << "t = "<< time.SetValue( details.t ) << rptNewLine;
         *pPara << "k" << Sub("f") << " = " << details.kf << rptNewLine;
         *pPara << "k" << Sub("c") << " = " << details.kc << rptNewLine;
         *pPara << symbol(psi) << "("<<time2.SetValue( details.t);
         *pPara <<","<<time2.SetValue(details.ti)<<") = "<< details.Ct << rptNewLine;
      }
      else
      {
         // 2005 and later
         *pPara << rptNewLine;
         *pPara << Bold("Prestress release until temporary strand removal and diaphragm casting") <<rptNewLine;
         *pPara << RPT_FC << " = " << fc.SetValue( details.Fc ) << rptNewLine;
         *pPara << "t" << Sub("i") << " (Adjusted) = " << time.SetValue(details.ti) << rptNewLine;
         *pPara << "t = "<< time.SetValue(details.t) << rptNewLine;
#if defined IGNORE_2007_CHANGES
         *pPara << "k" << Sub("vs") << " = " << details.kvs << rptNewLine;
#else
         if ( lrfdVersionMgr::FourthEdition2007 <= pSpecEntry->GetSpecificationType() )
            *pPara << "k" << Sub("s") << " = " << details.kvs << rptNewLine;
         else
            *pPara << "k" << Sub("vs") << " = " << details.kvs << rptNewLine;
#endif
         *pPara << "k" << Sub("hc") << " = " << details.khc << rptNewLine;
         *pPara << "k" << Sub("f") << " = " << details.kf << rptNewLine;
         *pPara << "k" << Sub("td") << " = " << details.ktd << rptNewLine;
         *pPara << symbol(psi) << "("<<time2.SetValue(details.t);
         *pPara <<","<<time2.SetValue(details.ti)<<") = "<< details.Ct << rptNewLine;

         *pPara << rptNewLine;

         details = pCamber->GetCreepCoefficientDetails(span,gdr,ICamber::cpReleaseToDeck,i);
         *pPara << Bold("Prestress release until deck casting") << rptNewLine;
         *pPara << RPT_FC << " = " << fc.SetValue( details.Fc ) << rptNewLine;
         *pPara << "t" << Sub("i") << " = " << time.SetValue( details.ti) << rptNewLine;
         *pPara << "t = "<< time.SetValue(details.t) << rptNewLine;
#if defined IGNORE_2007_CHANGES
         *pPara << "k" << Sub("vs") << " = " << details.kvs << rptNewLine;
#else
         if ( lrfdVersionMgr::FourthEdition2007 <= pSpecEntry->GetSpecificationType() )
            *pPara << "k" << Sub("s") << " = " << details.kvs << rptNewLine;
         else
            *pPara << "k" << Sub("vs") << " = " << details.kvs << rptNewLine;
#endif
         *pPara << "k" << Sub("hc") << " = " << details.khc << rptNewLine;
         *pPara << "k" << Sub("f") << " = " << details.kf << rptNewLine;
         *pPara << "k" << Sub("td") << " = " << details.ktd << rptNewLine;
         *pPara << symbol(psi) << "("<<time2.SetValue(details.t);
         *pPara <<","<<time2.SetValue(details.ti)<<") = "<< details.Ct << rptNewLine;

         *pPara << rptNewLine;

         details = pCamber->GetCreepCoefficientDetails(span,gdr,ICamber::cpDiaphragmToDeck,i);
         *pPara << Bold("Temporary strand removal and diaphragm casting to deck casting and application of superimposed dead loads") << rptNewLine;
         *pPara << RPT_FC << " = " << fc.SetValue( details.Fc ) << rptNewLine;
         *pPara << "t" << Sub("i") << " = " << time.SetValue( details.ti) << rptNewLine;
         *pPara << "t = "<< time.SetValue(details.t) << rptNewLine;
#if defined IGNORE_2007_CHANGES
         *pPara << "k" << Sub("vs") << " = " << details.kvs << rptNewLine;
#else
         if ( lrfdVersionMgr::FourthEdition2007 <= pSpecEntry->GetSpecificationType() )
            *pPara << "k" << Sub("s") << " = " << details.kvs << rptNewLine;
         else
            *pPara << "k" << Sub("vs") << " = " << details.kvs << rptNewLine;
#endif
         *pPara << "k" << Sub("hc") << " = " << details.khc << rptNewLine;
         *pPara << "k" << Sub("f") << " = " << details.kf << rptNewLine;
         *pPara << "k" << Sub("td") << " = " << details.ktd << rptNewLine;
         *pPara << symbol(psi) << "("<<time2.SetValue(details.t);
         *pPara <<","<<time2.SetValue(details.ti)<<") = "<< details.Ct << rptNewLine;
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
      *pPara << color(Red) << bold(ON) << "Changes to LRFD 4th Edition, 2007, Article 5.4.2.3.2 have been ignored." << bold(OFF) << color(Black) << rptNewLine;
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
            *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + "LRFDCreepEqn.png") << rptNewLine;
            *pPara << Bold("for which:") << rptNewLine;
            *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + (bSI ? "KfEqn-SI.png" : "KfEqn-US.png") ) << rptNewLine;
            *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + (bSI ? "KcEqn-SI.png" : "KcEqn-US.png") ) << rptNewLine;

            *pPara << Bold("where:") << rptNewLine;
            *pPara << "H = " << details.H << "%" << rptNewLine;
            *pPara << "V/S = " << length.SetValue(details.VSratio) << rptNewLine;
            *pPara << "In determining the maturity of concrete at initial load, t" << Sub("i")
                   << ", one day of accelerated curing may be taken as equal to seven days of normal curing." << rptNewLine;
            *pPara << "Curing Method = " << (details.CuringMethod == CURING_ACCELERATED ? "Accelerated" : "Normal") << rptNewLine;
            *pPara << rptNewLine;
         }
         else
         {
#if defined IGNORE_2007_CHANGES
            *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + "LRFDCreepEqn2005.png") << rptNewLine;
            *pPara << Bold("for which:") << rptNewLine;
            
            if ( pSpecEntry->GetSpecificationType() <= lrfdVersionMgr::ThirdEditionWith2005Interims )
               *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + (bSI ? "KvsEqn-SI.png" : "KvsEqn-US.png") ) << rptNewLine;
            else
               *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + (bSI ? "KvsEqn2006-SI.png" : "KvsEqn2006-US.png") ) << rptNewLine;
#else
            if ( lrfdVersionMgr::FourthEdition2007 <= pSpecEntry->GetSpecificationType() )
               *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + "LRFDCreepEqn2007.png") << rptNewLine;
            else
               *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + "LRFDCreepEqn2005.png") << rptNewLine;
            *pPara << Bold("for which:") << rptNewLine;
            
            if ( pSpecEntry->GetSpecificationType() <= lrfdVersionMgr::ThirdEditionWith2005Interims || lrfdVersionMgr::FourthEdition2007 <= pSpecEntry->GetSpecificationType())
               *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + (bSI ? "KvsEqn-SI.png" : "KvsEqn-US.png") ) << rptNewLine;
            else
               *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + (bSI ? "KvsEqn2006-SI.png" : "KvsEqn2006-US.png") ) << rptNewLine;
#endif // IGNORE_2007_CHANGES

            *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + "KhcEqn.png" ) << rptNewLine;
            *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + (bSI ? "KfEqn2005-SI.png" : "KfEqn2005-US.png") ) << rptNewLine;
            *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + (bSI ? "KtdEqn-SI.png" : "KtdEqn-US.png") ) << rptNewLine;

            *pPara << Bold("where:") << rptNewLine;
            *pPara << "H = " << details.H << "%" << rptNewLine;
            *pPara << "V/S = " << length.SetValue(details.VSratio) << rptNewLine;
            *pPara << Sub2("K","1") << " = " << details.K1 << rptNewLine;
            *pPara << Sub2("K","2") << " = " << details.K2 << rptNewLine;
#if defined IGNORE_2007_CHANGES
           *pPara << "In determining the maturity of concrete at initial load, t" << Sub("i")
                  << ", one day of accelerated curing may be taken as equal to seven days of normal curing." << rptNewLine;
#else
           if ( pSpecEntry->GetSpecificationType() < lrfdVersionMgr::FourthEdition2007 )
           {
              *pPara << "In determining the maturity of concrete at initial load, t" << Sub("i")
                     << ", one day of accelerated curing may be taken as equal to seven days of normal curing." << rptNewLine;
           }
#endif
            *pPara << "Curing Method = " << (details.CuringMethod == CURING_ACCELERATED ? "Accelerated" : "Normal") << rptNewLine;
            *pPara << rptNewLine;
         } // spec
      } // i

      *pPara << Bold((i == CREEP_MINTIME ? "Minimum Timing" : "Maximum Timing")) << rptNewLine;

      if ( details.Spec == CREEP_SPEC_PRE_2005 )
      {
         *pPara << rptNewLine;
         *pPara << Bold("Prestress release until deck casting") <<rptNewLine;
         *pPara << RPT_FC << " = " << fc.SetValue( details.Fc ) << rptNewLine;
         *pPara << "t" << Sub("i") << " (Adjusted) = " << time.SetValue(details.ti) << rptNewLine;
         *pPara << "t = "<< time.SetValue(details.t) << rptNewLine;
         *pPara << "k" << Sub("f") << " = " << details.kf << rptNewLine;
         *pPara << "k" << Sub("c") << " = " << details.kc << rptNewLine;
         *pPara << symbol(psi) << "("<<time2.SetValue( details.t);
         *pPara <<","<<time2.SetValue(details.ti)<<") = "<< details.Ct << rptNewLine;
      }
      else
      {
         // 2005 and later
         *pPara << rptNewLine;
         *pPara << Bold("Prestress release until deck casting") <<rptNewLine;
         *pPara << RPT_FC << " = " << fc.SetValue( details.Fc ) << rptNewLine;
         *pPara << "t" << Sub("i") << " (Adjusted) = " << time.SetValue(details.ti) << rptNewLine;
         *pPara << "t = "<< time.SetValue(details.t) << rptNewLine;
#if defined IGNORE_2007_CHANGES
         *pPara << "k" << Sub("vs") << " = " << details.kvs << rptNewLine;
#else
         if ( lrfdVersionMgr::FourthEdition2007 <= pSpecEntry->GetSpecificationType() )
            *pPara << "k" << Sub("s") << " = " << details.kvs << rptNewLine;
         else
            *pPara << "k" << Sub("vs") << " = " << details.kvs << rptNewLine;
#endif
         *pPara << "k" << Sub("hc") << " = " << details.khc << rptNewLine;
         *pPara << "k" << Sub("f") << " = " << details.kf << rptNewLine;
         *pPara << "k" << Sub("td") << " = " << details.ktd << rptNewLine;
         *pPara << symbol(psi) << "("<<time2.SetValue(details.t);
         *pPara <<","<<time2.SetValue(details.ti)<<") = "<< details.Ct << rptNewLine;
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
      *pPara << color(Red) << bold(ON) << "Changes to LRFD 4th Edition, 2007, Article 5.4.2.3.2 have been ignored." << bold(OFF) << color(Black) << rptNewLine;
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
           *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + "LRFDCreepEqn.png") << rptNewLine;
           *pPara << Bold("for which:") << rptNewLine;
           *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + (bSI ? "KfEqn-SI.png" : "KfEqn-US.png") ) << rptNewLine;
           *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + (bSI ? "KcEqn-SI.png" : "KcEqn-US.png") ) << rptNewLine;

           *pPara << Bold("where:") << rptNewLine;
           *pPara << "H = " << details.H << "%" << rptNewLine;
           *pPara << "V/S = " << length.SetValue(details.VSratio) << rptNewLine;
           *pPara << "In determining the maturity of concrete at initial load, t" << Sub("i")
                  << ", one day of accelerated curing may be taken as equal to seven days of normal curing." << rptNewLine;
           *pPara << "Curing Method = " << (details.CuringMethod == CURING_ACCELERATED ? "Accelerated" : "Normal") << rptNewLine;
           *pPara << rptNewLine;
        }
        else
        {
#if defined IGNORE_2007_CHANGES
            *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + "LRFDCreepEqn2005.png") << rptNewLine;
            *pPara << Bold("for which:") << rptNewLine;
            
            if ( pSpecEntry->GetSpecificationType() <= lrfdVersionMgr::ThirdEditionWith2005Interims )
               *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + (bSI ? "KvsEqn-SI.png" : "KvsEqn-US.png") ) << rptNewLine;
            else
               *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + (bSI ? "KvsEqn2006-SI.png" : "KvsEqn2006-US.png") ) << rptNewLine;
#else
            if ( lrfdVersionMgr::FourthEdition2007 <= pSpecEntry->GetSpecificationType() )
               *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + "LRFDCreepEqn2007.png") << rptNewLine;
            else
               *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + "LRFDCreepEqn2005.png") << rptNewLine;
            *pPara << Bold("for which:") << rptNewLine;
            
            if ( pSpecEntry->GetSpecificationType() <= lrfdVersionMgr::ThirdEditionWith2005Interims || lrfdVersionMgr::FourthEdition2007 <= pSpecEntry->GetSpecificationType())
               *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + (bSI ? "KvsEqn-SI.png" : "KvsEqn-US.png") ) << rptNewLine;
            else
               *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + (bSI ? "KvsEqn2006-SI.png" : "KvsEqn2006-US.png") ) << rptNewLine;
#endif // IGNORE_2007_CHANGES
           *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + "KhcEqn.png" ) << rptNewLine;
           *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + (bSI ? "KfEqn2005-SI.png" : "KfEqn2005-US.png") ) << rptNewLine;
           *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + (bSI ? "KtdEqn-SI.png" : "KtdEqn-US.png") ) << rptNewLine;

           *pPara << Bold("where:") << rptNewLine;
           *pPara << "H = " << details.H << "%" << rptNewLine;
           *pPara << "V/S = " << length.SetValue(details.VSratio) << rptNewLine;
            *pPara << Sub2("K","1") << " = " << details.K1 << rptNewLine;
            *pPara << Sub2("K","2") << " = " << details.K2 << rptNewLine;
#if defined IGNORE_2007_CHANGES
           *pPara << "In determining the maturity of concrete at initial load, t" << Sub("i")
                  << ", one day of accelerated curing may be taken as equal to seven days of normal curing." << rptNewLine;
#else
           if ( pSpecEntry->GetSpecificationType() < lrfdVersionMgr::FourthEdition2007 )
           {
              *pPara << "In determining the maturity of concrete at initial load, t" << Sub("i")
                     << ", one day of accelerated curing may be taken as equal to seven days of normal curing." << rptNewLine;
           }
#endif
           *pPara << "Curing Method = " << (details.CuringMethod == CURING_ACCELERATED ? "Accelerated" : "Normal") << rptNewLine;
           *pPara << rptNewLine;
        } // spec
     } // i
     
     *pPara << Bold((i == CREEP_MINTIME ? "Minimum Timing" : "Maximum Timing")) << rptNewLine;


      if ( details.Spec == CREEP_SPEC_PRE_2005 )
      {
         *pPara << rptNewLine;
         *pPara << Bold("Prestress release until temporary strand removal and diaphragm casting") <<rptNewLine;
         *pPara << RPT_FC << " = " << fc.SetValue( details.Fc ) << rptNewLine;
         *pPara << "t" << Sub("i") << " (Adjusted) = " << time.SetValue(details.ti) << rptNewLine;
         *pPara << "t = "<< time.SetValue(details.t) << rptNewLine;
         *pPara << "k" << Sub("f") << " = " << details.kf << rptNewLine;
         *pPara << "k" << Sub("c") << " = " << details.kc << rptNewLine;
         *pPara << symbol(psi) << "("<<time2.SetValue( details.t);
         *pPara <<","<<time2.SetValue(details.ti)<<") = "<< details.Ct << rptNewLine;

         *pPara << rptNewLine;

         details = pCamber->GetCreepCoefficientDetails(span,gdr,ICamber::cpReleaseToDeck,i);
         *pPara << Bold("Prestress release until application of superimposed dead loads") << rptNewLine;
         *pPara << RPT_FC << " = " << fc.SetValue( details.Fc ) << rptNewLine;
         *pPara << "t" << Sub("i") << " = " << time.SetValue( details.ti) << rptNewLine;
         *pPara << "t = "<< time.SetValue( details.t ) << rptNewLine;
         *pPara << "k" << Sub("f") << " = " << details.kf << rptNewLine;
         *pPara << "k" << Sub("c") << " = " << details.kc << rptNewLine;
         *pPara << symbol(psi) << "("<<time2.SetValue( details.t);
         *pPara <<","<<time2.SetValue(details.ti)<<") = "<< details.Ct << rptNewLine;

         *pPara << rptNewLine;

         details = pCamber->GetCreepCoefficientDetails(span,gdr,ICamber::cpReleaseToFinal,i);
         *pPara << Bold("Prestress release to final") << rptNewLine;
         *pPara << RPT_FC << " = " << fc.SetValue( details.Fc ) << rptNewLine;
         *pPara << "t" << Sub("i") << " = " << time.SetValue( details.ti) << rptNewLine;
         *pPara << "t = "<< time.SetValue( details.t ) << rptNewLine;
         *pPara << "k" << Sub("f") << " = " << details.kf << rptNewLine;
         *pPara << "k" << Sub("c") << " = " << details.kc << rptNewLine;
         *pPara << symbol(psi) << "("<<time2.SetValue( details.t);
         *pPara <<","<<time2.SetValue(details.ti)<<") = "<< details.Ct << rptNewLine;

         *pPara << rptNewLine;

         details = pCamber->GetCreepCoefficientDetails(span,gdr,ICamber::cpDiaphragmToDeck,i);
         *pPara << Bold("Temporary strand removal and diaphragm casting to application of superimposed dead loads") << rptNewLine;
         *pPara << RPT_FC << " = " << fc.SetValue( details.Fc ) << rptNewLine;
         *pPara << "t" << Sub("i") << " = " << time.SetValue( details.ti) << rptNewLine;
         *pPara << "t = "<< time.SetValue( details.t ) << rptNewLine;
         *pPara << "k" << Sub("f") << " = " << details.kf << rptNewLine;
         *pPara << "k" << Sub("c") << " = " << details.kc << rptNewLine;
         *pPara << symbol(psi) << "("<<time2.SetValue( details.t);
         *pPara <<","<<time2.SetValue(details.ti)<<") = "<< details.Ct << rptNewLine;

         *pPara << rptNewLine;

         details = pCamber->GetCreepCoefficientDetails(span,gdr,ICamber::cpDeckToFinal,i);
         *pPara << Bold("Application of superimposed dead loads to final") << rptNewLine;
         *pPara << RPT_FC << " = " << fc.SetValue( details.Fc ) << rptNewLine;
         *pPara << "t" << Sub("i") << " = " << time.SetValue( details.ti) << rptNewLine;
         *pPara << "t = "<< time.SetValue( details.t ) << rptNewLine;
         *pPara << "k" << Sub("f") << " = " << details.kf << rptNewLine;
         *pPara << "k" << Sub("c") << " = " << details.kc << rptNewLine;
         *pPara << symbol(psi) << "("<<time2.SetValue( details.t);
         *pPara <<","<<time2.SetValue(details.ti)<<") = "<< details.Ct << rptNewLine;
      }
      else
      {
         // 2005 and later
         *pPara << rptNewLine;
         *pPara << Bold("Prestress release until temporary strand removal and diaphragm casting") <<rptNewLine;
         *pPara << RPT_FC << " = " << fc.SetValue( details.Fc ) << rptNewLine;
         *pPara << "t" << Sub("i") << " (Adjusted) = " << time.SetValue(details.ti) << rptNewLine;
         *pPara << "t = "<< time.SetValue(details.t) << rptNewLine;
#if defined IGNORE_2007_CHANGES
         *pPara << "k" << Sub("vs") << " = " << details.kvs << rptNewLine;
#else
         if ( lrfdVersionMgr::FourthEdition2007 <= pSpecEntry->GetSpecificationType() )
            *pPara << "k" << Sub("s") << " = " << details.kvs << rptNewLine;
         else
            *pPara << "k" << Sub("vs") << " = " << details.kvs << rptNewLine;
#endif
         *pPara << "k" << Sub("hc") << " = " << details.khc << rptNewLine;
         *pPara << "k" << Sub("f") << " = " << details.kf << rptNewLine;
         *pPara << "k" << Sub("td") << " = " << details.ktd << rptNewLine;
         *pPara << symbol(psi) << "("<<time2.SetValue(details.t);
         *pPara <<","<<time2.SetValue(details.ti)<<") = "<< details.Ct << rptNewLine;

         *pPara << rptNewLine;

         details = pCamber->GetCreepCoefficientDetails(span,gdr,ICamber::cpReleaseToDeck,i);
         *pPara << Bold("Prestress release until application of superimposed dead loads") << rptNewLine;
         *pPara << RPT_FC << " = " << fc.SetValue( details.Fc ) << rptNewLine;
         *pPara << "t" << Sub("i") << " = " << time.SetValue( details.ti) << rptNewLine;
         *pPara << "t = "<< time.SetValue(details.t) << rptNewLine;
#if defined IGNORE_2007_CHANGES
         *pPara << "k" << Sub("vs") << " = " << details.kvs << rptNewLine;
#else
         if ( lrfdVersionMgr::FourthEdition2007 <= pSpecEntry->GetSpecificationType() )
            *pPara << "k" << Sub("s") << " = " << details.kvs << rptNewLine;
         else
            *pPara << "k" << Sub("vs") << " = " << details.kvs << rptNewLine;
#endif
         *pPara << "k" << Sub("hc") << " = " << details.khc << rptNewLine;
         *pPara << "k" << Sub("f") << " = " << details.kf << rptNewLine;
         *pPara << "k" << Sub("td") << " = " << details.ktd << rptNewLine;
         *pPara << symbol(psi) << "("<<time2.SetValue(details.t);
         *pPara <<","<<time2.SetValue(details.ti)<<") = "<< details.Ct << rptNewLine;

         *pPara << rptNewLine;

         details = pCamber->GetCreepCoefficientDetails(span,gdr,ICamber::cpReleaseToFinal,i);
         *pPara << Bold("Prestress release to final") << rptNewLine;
         *pPara << RPT_FC << " = " << fc.SetValue( details.Fc ) << rptNewLine;
         *pPara << "t" << Sub("i") << " = " << time.SetValue( details.ti) << rptNewLine;
         *pPara << "t = "<< time.SetValue(details.t) << rptNewLine;
#if defined IGNORE_2007_CHANGES
         *pPara << "k" << Sub("vs") << " = " << details.kvs << rptNewLine;
#else
         if ( lrfdVersionMgr::FourthEdition2007 <= pSpecEntry->GetSpecificationType() )
            *pPara << "k" << Sub("s") << " = " << details.kvs << rptNewLine;
         else
            *pPara << "k" << Sub("vs") << " = " << details.kvs << rptNewLine;
#endif
         *pPara << "k" << Sub("hc") << " = " << details.khc << rptNewLine;
         *pPara << "k" << Sub("f") << " = " << details.kf << rptNewLine;
         *pPara << "k" << Sub("td") << " = " << details.ktd << rptNewLine;
         *pPara << symbol(psi) << "("<<time2.SetValue(details.t);
         *pPara <<","<<time2.SetValue(details.ti)<<") = "<< details.Ct << rptNewLine;

         *pPara << rptNewLine;

         details = pCamber->GetCreepCoefficientDetails(span,gdr,ICamber::cpDiaphragmToDeck,i);
         *pPara << Bold("Temporary strand removal and diaphragm casting to application of superimposed dead loads") << rptNewLine;
         *pPara << RPT_FC << " = " << fc.SetValue( details.Fc ) << rptNewLine;
         *pPara << "t" << Sub("i") << " = " << time.SetValue( details.ti) << rptNewLine;
         *pPara << "t = "<< time.SetValue(details.t) << rptNewLine;
#if defined IGNORE_2007_CHANGES
         *pPara << "k" << Sub("vs") << " = " << details.kvs << rptNewLine;
#else
         if ( lrfdVersionMgr::FourthEdition2007 <= pSpecEntry->GetSpecificationType() )
            *pPara << "k" << Sub("s") << " = " << details.kvs << rptNewLine;
         else
            *pPara << "k" << Sub("vs") << " = " << details.kvs << rptNewLine;
#endif
         *pPara << "k" << Sub("hc") << " = " << details.khc << rptNewLine;
         *pPara << "k" << Sub("f") << " = " << details.kf << rptNewLine;
         *pPara << "k" << Sub("td") << " = " << details.ktd << rptNewLine;
         *pPara << symbol(psi) << "("<<time2.SetValue(details.t);
         *pPara <<","<<time2.SetValue(details.ti)<<") = "<< details.Ct << rptNewLine;

         *pPara << rptNewLine;

         details = pCamber->GetCreepCoefficientDetails(span,gdr,ICamber::cpDiaphragmToFinal,i);
         *pPara << Bold("Temporary strand removal and diaphragm casting to final") << rptNewLine;
         *pPara << RPT_FC << " = " << fc.SetValue( details.Fc ) << rptNewLine;
         *pPara << "t" << Sub("i") << " = " << time.SetValue( details.ti) << rptNewLine;
         *pPara << "t = "<< time.SetValue(details.t) << rptNewLine;
#if defined IGNORE_2007_CHANGES
         *pPara << "k" << Sub("vs") << " = " << details.kvs << rptNewLine;
#else
         if ( lrfdVersionMgr::FourthEdition2007 <= pSpecEntry->GetSpecificationType() )
            *pPara << "k" << Sub("s") << " = " << details.kvs << rptNewLine;
         else
            *pPara << "k" << Sub("vs") << " = " << details.kvs << rptNewLine;
#endif
         *pPara << "k" << Sub("hc") << " = " << details.khc << rptNewLine;
         *pPara << "k" << Sub("f") << " = " << details.kf << rptNewLine;
         *pPara << "k" << Sub("td") << " = " << details.ktd << rptNewLine;
         *pPara << symbol(psi) << "("<<time2.SetValue(details.t);
         *pPara <<","<<time2.SetValue(details.ti)<<") = "<< details.Ct << rptNewLine;

         *pPara << rptNewLine;

         details = pCamber->GetCreepCoefficientDetails(span,gdr,ICamber::cpDeckToFinal,i);
         *pPara << Bold("Application of superimposed dead loads to final") << rptNewLine;
         *pPara << RPT_FC << " = " << fc.SetValue( details.Fc ) << rptNewLine;
         *pPara << "t" << Sub("i") << " = " << time.SetValue( details.ti) << rptNewLine;
         *pPara << "t = "<< time.SetValue(details.t) << rptNewLine;
#if defined IGNORE_2007_CHANGES
         *pPara << "k" << Sub("vs") << " = " << details.kvs << rptNewLine;
#else
         if ( lrfdVersionMgr::FourthEdition2007 <= pSpecEntry->GetSpecificationType() )
            *pPara << "k" << Sub("s") << " = " << details.kvs << rptNewLine;
         else
            *pPara << "k" << Sub("vs") << " = " << details.kvs << rptNewLine;
#endif
         *pPara << "k" << Sub("hc") << " = " << details.khc << rptNewLine;
         *pPara << "k" << Sub("f") << " = " << details.kf << rptNewLine;
         *pPara << "k" << Sub("td") << " = " << details.ktd << rptNewLine;
         *pPara << symbol(psi) << "("<<time2.SetValue(details.t);
         *pPara <<","<<time2.SetValue(details.ti)<<") = "<< details.Ct << rptNewLine;
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