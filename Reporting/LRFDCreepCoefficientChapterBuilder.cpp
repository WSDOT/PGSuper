///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2024  Washington State Department of Transportation
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
#include <Reporting\CreepCoefficientChapterBuilder.h>

#include <IFace\AnalysisResults.h>
#include <IFace\Project.h>
#include <IFace\Bridge.h>

#include <PgsExt\StrandData.h>
#include <PgsExt\GirderMaterial.h>

#include <psgLib/CreepCriteria.h>
#include <psgLib/SpecificationCriteria.h>

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

rptChapter* CLRFDCreepCoefficientChapterBuilder::Build(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,Uint16 level) const
{
   auto pGirderRptSpec = std::dynamic_pointer_cast<const CGirderReportSpecification>(pRptSpec);
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
      case pgsTypes::sdtNonstructuralOverlay:
         pPara = (bTempStrands ? Build_NoDeck_TempStrands(pRptSpec, pBroker, segmentKey, pDisplayUnits,level) : Build_NoDeck(pRptSpec, pBroker, segmentKey, pDisplayUnits,level));
         break;

      default:
         ATLASSERT(false);
      }
   }

   *pChapter << pPara;
   return pChapter;
}

std::unique_ptr<WBFL::Reporting::ChapterBuilder> CLRFDCreepCoefficientChapterBuilder::Clone() const
{
   return std::make_unique<CLRFDCreepCoefficientChapterBuilder>();
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

rptParagraph* CLRFDCreepCoefficientChapterBuilder::Build_CIP_TempStrands(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,IBroker* pBroker,const CSegmentKey& segmentKey,
                                                   IEAFDisplayUnits* pDisplayUnits,
                                                   Uint16 level) const
{
   rptParagraph* pPara = new rptParagraph;

   GET_IFACE2(pBroker,ICamber,pCamber);

   GET_IFACE2(pBroker,ILibrary,pLib);
   GET_IFACE2(pBroker,ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());

   const auto& creep_criteria = pSpecEntry->GetCreepCriteria();

   GET_IFACE2(pBroker, ISegmentData, pSegmentData);

   INIT_UV_PROTOTYPE( rptTimeUnitValue, time, pDisplayUnits->GetWholeDaysUnit(), true );
   INIT_UV_PROTOTYPE( rptTimeUnitValue, time2, pDisplayUnits->GetWholeDaysUnit(), false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, fc, pDisplayUnits->GetStressUnit(), true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, length, pDisplayUnits->GetComponentDimUnit(), true );

   rptCreepCoefficient creep;

   CREEPCOEFFICIENTDETAILS details;

   //////////////////////////////
   // Report LRFD method
   //////////////////////////////
   std::vector<std::pair<ICamber::CreepPeriod, std::_tstring>> strHeading;
   strHeading.emplace_back(ICamber::cpReleaseToDiaphragm, _T("Prestress release until girder erection"));
   strHeading.emplace_back(ICamber::cpDiaphragmToDeck, _T("Girder erection until deck casting"));
   strHeading.emplace_back(ICamber::cpReleaseToDeck, _T("Prestress release until deck casting"));
   strHeading.emplace_back(ICamber::cpDeckToFinal, _T("Deck casting to final"));
   strHeading.emplace_back(ICamber::cpReleaseToFinal, _T("Prestress release to final"));

   bool bSI = IS_SI_UNITS(pDisplayUnits);

   for(const auto creep_time : pgsTypes::enum_range<pgsTypes::CreepTime>(pgsTypes::CreepTime::Min,pgsTypes::CreepTime::Max))
   {
      int j = 0;
      for(const auto& item : strHeading)
      {
         ICamber::CreepPeriod creepPeriod = item.first;

         details = pCamber->GetCreepCoefficientDetails(segmentKey,creepPeriod,creep_time);

         if ( creep_time == pgsTypes::CreepTime::Min && j == 0 )
         {
            // first time through loop, report the common information
            if (pSegmentData->GetSegmentMaterial(segmentKey)->Concrete.Type == pgsTypes::PCI_UHPC)
            {
               if (pSegmentData->GetSegmentMaterial(segmentKey)->Concrete.bPCTT)
                  *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("Creep_PCI_UHPC_PCTT.png")) << rptNewLine;
               else
                  *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("Creep_PCI_UHPC.png")) << rptNewLine;

               *pPara << Bold(_T("where:")) << rptNewLine;
               *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("PCI_UHPC_Factors.png")) << rptNewLine;
            }
            else if (pSegmentData->GetSegmentMaterial(segmentKey)->Concrete.Type == pgsTypes::UHPC)
            {
               *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("Creep_UHPC.png")) << rptNewLine;
               *pPara << Bold(_T("where:")) << rptNewLine;
               *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("UHPC_Factors.png")) << rptNewLine;
            }
            else if ( details.Spec == pgsTypes::CreepSpecification::LRFDPre2005 )
            {
               *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("LRFDCreepEqn.png")) << rptNewLine;
               *pPara << Bold(_T("for which:")) << rptNewLine;
               *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + (bSI ? _T("KfEqn-SI.png") : _T("KfEqn-US.png")) ) << rptNewLine;
               *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + (bSI ? _T("KcEqn-SI.png") : _T("KcEqn-US.png")) ) << rptNewLine;

               *pPara << Bold(_T("where:")) << rptNewLine;
               *pPara << _T("H = ") << details.H << _T("%") << _T(", ");
               *pPara << _T("V/S = ") << length.SetValue(details.VSratio) << rptNewLine;
               *pPara << _T("In determining the maturity of concrete at initial load, t") << Sub(_T("i"))
                      << _T(", 1 day of accelerated curing may be taken as equal to ") << creep_criteria.CuringMethodTimeAdjustmentFactor << _T(" days of normal curing.") << rptNewLine;
               *pPara << _T("Curing Method = ") << (details.CuringMethod == pgsTypes::CuringMethod::Accelerated ? _T("Accelerated") : _T("Normal")) << rptNewLine;
               *pPara << rptNewLine;
            }
            else
            {
               if ( WBFL::LRFD::BDSManager::Edition::FourthEdition2007 <= pSpecEntry->GetSpecificationCriteria().GetEdition() )
               {
                  *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("LRFDCreepEqn2007.png")) << rptNewLine;
               }
               else
               {
                  *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("LRFDCreepEqn2005.png")) << rptNewLine;
               }

               *pPara << Bold(_T("for which:")) << rptNewLine;
               
               if ( pSpecEntry->GetSpecificationCriteria().GetEdition() <= WBFL::LRFD::BDSManager::Edition::ThirdEditionWith2005Interims )
               {
                  *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + (bSI ? _T("KvsEqn-SI.png") : _T("KvsEqn-US.png")) ) << rptNewLine;
               }
               else if ( WBFL::LRFD::BDSManager::Edition::ThirdEditionWith2006Interims == pSpecEntry->GetSpecificationCriteria().GetEdition())
               {
                  *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + (bSI ? _T("KvsEqn2006-SI.png") : _T("KvsEqn2006-US.png")) ) << rptNewLine;
               }
               else
               {
                  *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + (bSI ? _T("KvsEqn2007-SI.png") : _T("KvsEqn2007-US.png")) ) << rptNewLine;
               }

               *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("KhcEqn.png") ) << rptNewLine;
               *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + (bSI ? _T("KfEqn2005-SI.png") : _T("KfEqn2005-US.png")) ) << rptNewLine;
               if ( pSpecEntry->GetSpecificationCriteria().GetEdition() < WBFL::LRFD::BDSManager::Edition::SeventhEditionWith2015Interims )
               {
                  *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + (bSI ? _T("KtdEqn-SI.png") : _T("KtdEqn-US.png")) ) << rptNewLine;
               }
               else
               {
                  ATLASSERT(!bSI);
                  *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("KtdEqn-US2015.png")) << rptNewLine;
               }

               *pPara << Bold(_T("where:")) << rptNewLine;
               *pPara << _T("H = ") << details.H << _T("%") << _T(", ");
               *pPara << _T("V/S = ") << length.SetValue(details.VSratio) << _T(", ");
               *pPara << Sub2(_T("K"),_T("1")) << _T(" = ") << details.K1 << _T(", ");
               *pPara << Sub2(_T("K"),_T("2")) << _T(" = ") << details.K2 << rptNewLine;
               if ( pSpecEntry->GetSpecificationCriteria().GetEdition() < WBFL::LRFD::BDSManager::Edition::FourthEdition2007 )
               {
                  *pPara << _T("In determining the maturity of concrete at initial load, t") << Sub(_T("i"))
                         << _T(", 1 day of accelerated curing may be taken as equal to ") << creep_criteria.CuringMethodTimeAdjustmentFactor << _T(" days of normal curing.") << rptNewLine;
               }
               *pPara << _T("Curing Method = ") << (details.CuringMethod == pgsTypes::CuringMethod::Accelerated ? _T("Accelerated") : _T("Normal")) << rptNewLine;
               *pPara << rptNewLine;
            } // spec
         } // i


         if ( j == 0 )
         {
            *pPara << Bold((creep_time == pgsTypes::CreepTime::Min ? _T("Minimum Timing") : _T("Maximum Timing"))) << rptNewLine;
         }

         if ( details.Spec == pgsTypes::CreepSpecification::LRFDPre2005)
         {
            *pPara << rptNewLine;
            *pPara << Bold(item.second) << rptNewLine;
            *pPara << RPT_FCI << _T(" = ") << fc.SetValue( details.Fc ) << _T(", ");
            *pPara << _T("t") << Sub(_T("i")) << _T(" (Adjusted) = ") << time.SetValue(details.ti) << _T(", ");
            *pPara << _T("t = ")<< time.SetValue(details.t) << _T(", ");
            *pPara << _T("k") << Sub(_T("f")) << _T(" = ") << details.kf << _T(", ");
            *pPara << _T("k") << Sub(_T("c")) << _T(" = ") << details.ktd << rptNewLine;
            *pPara << symbol(psi) << _T("(")<<time2.SetValue( details.t);
            *pPara <<_T(",")<<time2.SetValue(details.ti)<<_T(") = ")<< creep.SetValue(details.Ct) << rptNewLine;
         }
         else
         {
            // 2005 and later
            *pPara << rptNewLine;
            *pPara << Bold(item.second) << rptNewLine;
            *pPara << RPT_FCI << _T(" = ") << fc.SetValue( details.Fc ) << _T(", ");
            if (pSpecEntry->GetSpecificationCriteria().GetEdition() < WBFL::LRFD::BDSManager::Edition::FourthEdition2007)
            {
               *pPara << _T("t") << Sub(_T("i")) << _T(" (Adjusted) = ") << time.SetValue(details.ti) << _T(", ");
            }
            else
            {
               *pPara << _T("t") << Sub(_T("i")) << _T(" = ") << time.SetValue(details.ti) << _T(", ");
            }
            *pPara << _T("t = ")<< time.SetValue(details.t) << _T(", ");

            if ( WBFL::LRFD::BDSManager::Edition::FourthEdition2007 <= pSpecEntry->GetSpecificationCriteria().GetEdition() )
            {
               *pPara << _T("k") << Sub(_T("s")) << _T(" = ") << details.kvs << _T(", ");
            }
            else
            {
               *pPara << _T("k") << Sub(_T("vs")) << _T(" = ") << details.kvs << _T(", ");
            }

            *pPara << _T("k") << Sub(_T("hc")) << _T(" = ") << details.khc << _T(", ");
            *pPara << _T("k") << Sub(_T("f")) << _T(" = ") << details.kf << _T(", ");
            *pPara << _T("k") << Sub(_T("td")) << _T(" = ") << details.ktd;

            if (pSegmentData->GetSegmentMaterial(segmentKey)->Concrete.Type == pgsTypes::UHPC)
            {
               *pPara << _T(", ") << Sub2(_T("k"), _T("l")) << _T(" = ") << details.kl << _T(", ");
            }

            *pPara << rptNewLine;

            *pPara << symbol(psi) << _T("(")<<time2.SetValue(details.t);
            *pPara <<_T(",")<<time2.SetValue(details.ti)<<_T(") = ")<< creep.SetValue(details.Ct) << rptNewLine;
         } // spec type

         *pPara << rptNewLine;
         j++;
      } // loop on j
   }// loop on i


   return pPara;
}

rptParagraph* CLRFDCreepCoefficientChapterBuilder::Build_CIP(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,IBroker* pBroker,const CSegmentKey& segmentKey,
                                                       IEAFDisplayUnits* pDisplayUnits,
                                                       Uint16 level) const
{
   rptParagraph* pPara = new rptParagraph;

   GET_IFACE2(pBroker,ICamber,pCamber);

   GET_IFACE2(pBroker,ILibrary,pLib);
   GET_IFACE2(pBroker,ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());

   const auto& creep_criteria = pSpecEntry->GetCreepCriteria();

   GET_IFACE2(pBroker, ISegmentData, pSegmentData);

   INIT_UV_PROTOTYPE( rptTimeUnitValue, time, pDisplayUnits->GetWholeDaysUnit(), true );
   INIT_UV_PROTOTYPE( rptTimeUnitValue, time2, pDisplayUnits->GetWholeDaysUnit(), false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, fc, pDisplayUnits->GetStressUnit(), true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, length, pDisplayUnits->GetComponentDimUnit(), true );
   rptCreepCoefficient creep;

   CREEPCOEFFICIENTDETAILS details;

   //////////////////////////////
   // Report LRFD method
   //////////////////////////////
   bool bSI = IS_SI_UNITS(pDisplayUnits);

   std::vector<std::pair<ICamber::CreepPeriod, std::_tstring>> strHeading;
   strHeading.emplace_back(ICamber::cpReleaseToDiaphragm, _T("Prestress release until girder erection"));
   strHeading.emplace_back(ICamber::cpDiaphragmToDeck, _T("Girder erection until deck casting"));
   strHeading.emplace_back(ICamber::cpReleaseToDeck, _T("Prestress release until deck casting"));
   strHeading.emplace_back(ICamber::cpDeckToFinal, _T("Deck casting to final"));
   strHeading.emplace_back(ICamber::cpReleaseToFinal, _T("Prestress release to final"));

   for(const auto creep_time : pgsTypes::enum_range<pgsTypes::CreepTime>(pgsTypes::CreepTime::Min,pgsTypes::CreepTime::Max))
   {
      int j = 0;
      for(const auto& item : strHeading)
      {
         ICamber::CreepPeriod creepPeriod = item.first;

         details = pCamber->GetCreepCoefficientDetails(segmentKey,creepPeriod,creep_time);

         if ( creep_time == pgsTypes::CreepTime::Min && j == 0 )
         {
            // first time through loop, report the common information
            if (pSegmentData->GetSegmentMaterial(segmentKey)->Concrete.Type == pgsTypes::PCI_UHPC)
            {
               if (pSegmentData->GetSegmentMaterial(segmentKey)->Concrete.bPCTT)
                  *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("Creep_PCI_UHPC_PCTT.png")) << rptNewLine;
               else
                  *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("Creep_PCI_UHPC.png")) << rptNewLine;

               *pPara << Bold(_T("where:")) << rptNewLine;
               *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("PCI_UHPC_Factors.png")) << rptNewLine;
            }
            else if (pSegmentData->GetSegmentMaterial(segmentKey)->Concrete.Type == pgsTypes::UHPC)
            {
               *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("Creep_UHPC.png")) << rptNewLine;
               *pPara << Bold(_T("where:")) << rptNewLine;
               *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("UHPC_Factors.png")) << rptNewLine;
            }
            else if (details.Spec == pgsTypes::CreepSpecification::LRFDPre2005)
            {
               *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("LRFDCreepEqn.png")) << rptNewLine;
               *pPara << Bold(_T("for which:")) << rptNewLine;
               *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + (bSI ? _T("KfEqn-SI.png") : _T("KfEqn-US.png")) ) << rptNewLine;
               *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + (bSI ? _T("KcEqn-SI.png") : _T("KcEqn-US.png")) ) << rptNewLine;

               *pPara << Bold(_T("where:")) << rptNewLine;
               *pPara << _T("H = ") << details.H << _T("%") << _T(", ");
               *pPara << _T("V/S = ") << length.SetValue(details.VSratio) << rptNewLine;
               *pPara << _T("In determining the maturity of concrete at initial load, t") << Sub(_T("i"))
                      << _T(", 1 day of accelerated curing may be taken as equal to ") << creep_criteria.CuringMethodTimeAdjustmentFactor << _T(" days of normal curing.") << rptNewLine;
               *pPara << _T("Curing Method = ") << (details.CuringMethod == pgsTypes::CuringMethod::Accelerated ? _T("Accelerated") : _T("Normal")) << rptNewLine;
               *pPara << rptNewLine;
            }
            else
            {
               if ( WBFL::LRFD::BDSManager::Edition::FourthEdition2007 <= pSpecEntry->GetSpecificationCriteria().GetEdition() )
               {
                  *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("LRFDCreepEqn2007.png")) << rptNewLine;
               }
               else
               {
                  *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("LRFDCreepEqn2005.png")) << rptNewLine;
               }
               *pPara << Bold(_T("for which:")) << rptNewLine;

               if ( pSpecEntry->GetSpecificationCriteria().GetEdition() <= WBFL::LRFD::BDSManager::Edition::ThirdEditionWith2005Interims )
               {
                  *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + (bSI ? _T("KvsEqn-SI.png") : _T("KvsEqn-US.png")) ) << rptNewLine;
               }
               else if ( WBFL::LRFD::BDSManager::Edition::ThirdEditionWith2006Interims == pSpecEntry->GetSpecificationCriteria().GetEdition())
               {
                  *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + (bSI ? _T("KvsEqn2006-SI.png") : _T("KvsEqn2006-US.png")) ) << rptNewLine;
               }
               else
               {
                  *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + (bSI ? _T("KvsEqn2007-SI.png") : _T("KvsEqn2007-US.png")) ) << rptNewLine;
               }

               *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("KhcEqn.png") ) << rptNewLine;
               *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + (bSI ? _T("KfEqn2005-SI.png") : _T("KfEqn2005-US.png")) ) << rptNewLine;
               if ( pSpecEntry->GetSpecificationCriteria().GetEdition() < WBFL::LRFD::BDSManager::Edition::SeventhEditionWith2015Interims )
               {
                  *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + (bSI ? _T("KtdEqn-SI.png") : _T("KtdEqn-US.png")) ) << rptNewLine;
               }
               else
               {
                  ATLASSERT(!bSI);
                  *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("KtdEqn-US2015.png")) << rptNewLine;
               }

               *pPara << Bold(_T("where:")) << rptNewLine;
               *pPara << _T("H = ") << details.H << _T("%") << _T(", ");
               *pPara << _T("V/S = ") << length.SetValue(details.VSratio) << _T(", ");
               *pPara << Sub2(_T("K"),_T("1")) << _T(" = ") << details.K1 << _T(", ");
               *pPara << Sub2(_T("K"),_T("2")) << _T(" = ") << details.K2 << rptNewLine;

               if ( pSpecEntry->GetSpecificationCriteria().GetEdition() < WBFL::LRFD::BDSManager::Edition::FourthEdition2007 )
               {
                 *pPara << _T("In determining the maturity of concrete at initial load, t") << Sub(_T("i"))
                        << _T(", 1 day of accelerated curing may be taken as equal to ") << creep_criteria.CuringMethodTimeAdjustmentFactor << _T(" days of normal curing.") << rptNewLine;
               }
               *pPara << _T("Curing Method = ") << (details.CuringMethod == pgsTypes::CuringMethod::Accelerated ? _T("Accelerated") : _T("Normal")) << rptNewLine;
               *pPara << rptNewLine;
            } // spec
         } // if

         if ( j == 0 )
         {
            *pPara << Bold((creep_time == pgsTypes::CreepTime::Min ? _T("Minimum Timing") : _T("Maximum Timing"))) << rptNewLine;
         }

         if ( details.Spec == pgsTypes::CreepSpecification::LRFDPre2005)
         {
            *pPara << rptNewLine;
            *pPara << Bold(item.second) << rptNewLine;
            *pPara << RPT_FCI << _T(" = ") << fc.SetValue( details.Fc ) << _T(", ");
            *pPara << _T("t") << Sub(_T("i")) << _T(" (Adjusted) = ") << time.SetValue(details.ti) << _T(", ");
            *pPara << _T("t = ")<< time.SetValue(details.t) << _T(", ");
            *pPara << _T("k") << Sub(_T("f")) << _T(" = ") << details.kf << _T(", ");
            *pPara << _T("k") << Sub(_T("c")) << _T(" = ") << details.ktd << rptNewLine;
            *pPara << symbol(psi) << _T("(")<<time2.SetValue( details.t);
            *pPara <<_T(",")<<time2.SetValue(details.ti)<<_T(") = ")<< creep.SetValue(details.Ct) << rptNewLine;
         }
         else
         {
            // 2005 and later
            *pPara << rptNewLine;
            *pPara << Bold(item.second) << rptNewLine;
            *pPara << RPT_FCI << _T(" = ") << fc.SetValue( details.Fc ) << _T(", ");
            if (pSpecEntry->GetSpecificationCriteria().GetEdition() < WBFL::LRFD::BDSManager::Edition::FourthEdition2007)
            {
               *pPara << _T("t") << Sub(_T("i")) << _T(" (Adjusted) = ") << time.SetValue(details.ti) << _T(", ");
            }
            else
            {
               *pPara << _T("t") << Sub(_T("i")) << _T(" = ") << time.SetValue(details.ti) << _T(", ");
            }
            *pPara << _T("t = ")<< time.SetValue(details.t) << _T(", ");

            if ( WBFL::LRFD::BDSManager::Edition::FourthEdition2007 <= pSpecEntry->GetSpecificationCriteria().GetEdition() )
            {
               *pPara << _T("k") << Sub(_T("s")) << _T(" = ") << details.kvs << _T(", ");
            }
            else
            {
               *pPara << _T("k") << Sub(_T("vs")) << _T(" = ") << details.kvs << _T(", ");
            }

            *pPara << _T("k") << Sub(_T("hc")) << _T(" = ") << details.khc << _T(", ");
            *pPara << _T("k") << Sub(_T("f")) << _T(" = ") << details.kf << _T(", ");
            *pPara << _T("k") << Sub(_T("td")) << _T(" = ") << details.ktd;

            if (pSegmentData->GetSegmentMaterial(segmentKey)->Concrete.Type == pgsTypes::UHPC)
            {
               *pPara << _T(", ") << Sub2(_T("k"), _T("l")) << _T(" = ") << details.kl << _T(", ");
            }

            *pPara << rptNewLine;

            *pPara << symbol(psi) << _T("(")<<time2.SetValue(details.t);
            *pPara <<_T(",")<<time2.SetValue(details.ti)<<_T(") = ")<< creep.SetValue(details.Ct) << rptNewLine;
         } // spec type

         *pPara << rptNewLine;
         j++;
      } // loop on j
   }// loop on i


   return pPara;
}

rptParagraph* CLRFDCreepCoefficientChapterBuilder::Build_SIP_TempStrands(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,IBroker* pBroker,const CSegmentKey& segmentKey,
                                                                   IEAFDisplayUnits* pDisplayUnits,
                                                                   Uint16 level) const
{
   return Build_CIP_TempStrands(pRptSpec, pBroker, segmentKey, pDisplayUnits, level);
}

rptParagraph* CLRFDCreepCoefficientChapterBuilder::Build_SIP(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,IBroker* pBroker,const CSegmentKey& segmentKey,
                                                       IEAFDisplayUnits* pDisplayUnits,
                                                       Uint16 level) const
{
   return Build_SIP_TempStrands(pRptSpec, pBroker, segmentKey, pDisplayUnits, level);
}

rptParagraph* CLRFDCreepCoefficientChapterBuilder::Build_NoDeck_TempStrands(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,IBroker* pBroker,const CSegmentKey& segmentKey,
                                                                      IEAFDisplayUnits* pDisplayUnits,
                                                                      Uint16 level) const
{
   rptParagraph* pPara = new rptParagraph;

   GET_IFACE2(pBroker,ICamber,pCamber);

   GET_IFACE2(pBroker,ILibrary,pLib);
   GET_IFACE2(pBroker,ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());

   const auto& creep_criteria = pSpecEntry->GetCreepCriteria();

   GET_IFACE2(pBroker, ISegmentData, pSegmentData);

   INIT_UV_PROTOTYPE( rptTimeUnitValue, time, pDisplayUnits->GetWholeDaysUnit(), true );
   INIT_UV_PROTOTYPE( rptTimeUnitValue, time2, pDisplayUnits->GetWholeDaysUnit(), false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, fc, pDisplayUnits->GetStressUnit(), true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, length, pDisplayUnits->GetComponentDimUnit(), true );

   rptCreepCoefficient creep;

   CREEPCOEFFICIENTDETAILS details;

   //////////////////////////////
   // Report LRFD method
   //////////////////////////////
   bool bSI = IS_SI_UNITS(pDisplayUnits);

   for( const auto creep_time : pgsTypes::enum_range<pgsTypes::CreepTime>(pgsTypes::CreepTime::Min,pgsTypes::CreepTime::Max))
   {
     details = pCamber->GetCreepCoefficientDetails(segmentKey,ICamber::cpReleaseToDiaphragm,creep_time);
     if ( creep_time == pgsTypes::CreepTime::Min )
     {
        // first time through loop, report the common information
        if (pSegmentData->GetSegmentMaterial(segmentKey)->Concrete.Type == pgsTypes::PCI_UHPC)
        {
           if (pSegmentData->GetSegmentMaterial(segmentKey)->Concrete.bPCTT)
              *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("Creep_PCI_UHPC_PCTT.png")) << rptNewLine;
           else
              *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("Creep_PCI_UHPC.png")) << rptNewLine;

           *pPara << Bold(_T("where:")) << rptNewLine;
           *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("PCI_UHPC_Factors.png")) << rptNewLine;
        }
        else if (pSegmentData->GetSegmentMaterial(segmentKey)->Concrete.Type == pgsTypes::UHPC)
        {
           *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("Creep_UHPC.png")) << rptNewLine;
           *pPara << Bold(_T("where:")) << rptNewLine;
           *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("UHPC_Factors.png")) << rptNewLine;
        }
        else if (details.Spec == pgsTypes::CreepSpecification::LRFDPre2005)
        {
           *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("LRFDCreepEqn.png")) << rptNewLine;
           *pPara << Bold(_T("for which:")) << rptNewLine;
           *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + (bSI ? _T("KfEqn-SI.png") : _T("KfEqn-US.png")) ) << rptNewLine;
           *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + (bSI ? _T("KcEqn-SI.png") : _T("KcEqn-US.png")) ) << rptNewLine;

           *pPara << Bold(_T("where:")) << rptNewLine;
           *pPara << _T("H = ") << details.H << _T("%") << _T(", ");
           *pPara << _T("V/S = ") << length.SetValue(details.VSratio) << rptNewLine;
           *pPara << _T("In determining the maturity of concrete at initial load, t") << Sub(_T("i"))
                  << _T(", 1 day of accelerated curing may be taken as equal to ") << creep_criteria.CuringMethodTimeAdjustmentFactor << _T(" days of normal curing.") << rptNewLine;
           *pPara << _T("Curing Method = ") << (details.CuringMethod == pgsTypes::CuringMethod::Accelerated ? _T("Accelerated") : _T("Normal")) << rptNewLine;
           *pPara << rptNewLine;
        }
        else
        {
            if ( WBFL::LRFD::BDSManager::Edition::FourthEdition2007 <= pSpecEntry->GetSpecificationCriteria().GetEdition() )
            {
               *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("LRFDCreepEqn2007.png")) << rptNewLine;
            }
            else
            {
               *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("LRFDCreepEqn2005.png")) << rptNewLine;
            }
            *pPara << Bold(_T("for which:")) << rptNewLine;
            
            if ( pSpecEntry->GetSpecificationCriteria().GetEdition() <= WBFL::LRFD::BDSManager::Edition::ThirdEditionWith2005Interims )
            {
               *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + (bSI ? _T("KvsEqn-SI.png") : _T("KvsEqn-US.png")) ) << rptNewLine;
            }
            else if ( WBFL::LRFD::BDSManager::Edition::ThirdEditionWith2006Interims == pSpecEntry->GetSpecificationCriteria().GetEdition())
            {
               *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + (bSI ? _T("KvsEqn2006-SI.png") : _T("KvsEqn2006-US.png")) ) << rptNewLine;
            }
            else
            {
               *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + (bSI ? _T("KvsEqn2007-SI.png") : _T("KvsEqn2007-US.png")) ) << rptNewLine;
            }

           *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("KhcEqn.png") ) << rptNewLine;
           *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + (bSI ? _T("KfEqn2005-SI.png") : _T("KfEqn2005-US.png")) ) << rptNewLine;
            if ( pSpecEntry->GetSpecificationCriteria().GetEdition() < WBFL::LRFD::BDSManager::Edition::SeventhEditionWith2015Interims )
            {
               *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + (bSI ? _T("KtdEqn-SI.png") : _T("KtdEqn-US.png")) ) << rptNewLine;
            }
            else
            {
               ATLASSERT(!bSI);
               *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("KtdEqn-US2015.png")) << rptNewLine;
            }

           *pPara << Bold(_T("where:")) << rptNewLine;
           *pPara << _T("H = ") << details.H << _T("%") << _T(", ");
           *pPara << _T("V/S = ") << length.SetValue(details.VSratio) << _T(", ");
           *pPara << Sub2(_T("K"),_T("1")) << _T(" = ") << details.K1 << _T(", ");
           *pPara << Sub2(_T("K"),_T("2")) << _T(" = ") << details.K2 << rptNewLine;

           if ( pSpecEntry->GetSpecificationCriteria().GetEdition() < WBFL::LRFD::BDSManager::Edition::FourthEdition2007 )
           {
              *pPara << _T("In determining the maturity of concrete at initial load, t") << Sub(_T("i"))
                     << _T(", 1 day of accelerated curing may be taken as equal to ") << creep_criteria.CuringMethodTimeAdjustmentFactor << _T(" days of normal curing.") << rptNewLine;
           }
           *pPara << _T("Curing Method = ") << (details.CuringMethod == pgsTypes::CuringMethod::Accelerated ? _T("Accelerated") : _T("Normal")) << rptNewLine;
           *pPara << rptNewLine;
        } // spec
     } // i
     
     *pPara << Bold((creep_time == pgsTypes::CreepTime::Min ? _T("Minimum Timing") : _T("Maximum Timing"))) << rptNewLine;


      if ( details.Spec == pgsTypes::CreepSpecification::LRFDPre2005)
      {
         *pPara << rptNewLine;
         *pPara << Bold(_T("Prestress release until temporary strand removal and diaphragm casting")) <<rptNewLine;
         *pPara << RPT_FCI << _T(" = ") << fc.SetValue( details.Fc ) << _T(", ");
         *pPara << _T("t") << Sub(_T("i")) << _T(" (Adjusted) = ") << time.SetValue(details.ti) << _T(", ");
         *pPara << _T("t = ")<< time.SetValue(details.t) << _T(", ");
         *pPara << _T("k") << Sub(_T("f")) << _T(" = ") << details.kf << _T(", ");
         *pPara << _T("k") << Sub(_T("c")) << _T(" = ") << details.ktd << rptNewLine;
         *pPara << symbol(psi) << _T("(")<<time2.SetValue( details.t);
         *pPara <<_T(",")<<time2.SetValue(details.ti)<<_T(") = ")<< creep.SetValue(details.Ct) << rptNewLine;

         *pPara << rptNewLine;

         details = pCamber->GetCreepCoefficientDetails(segmentKey,ICamber::cpReleaseToDeck,creep_time);
         *pPara << Bold(_T("Prestress release until application of superimposed dead loads")) << rptNewLine;
         *pPara << RPT_FCI << _T(" = ") << fc.SetValue( details.Fc ) << _T(", ");
         *pPara << _T("t") << Sub(_T("i")) << _T(" = ") << time.SetValue( details.ti) << _T(", ");
         *pPara << _T("t = ")<< time.SetValue( details.t ) << _T(", ");
         *pPara << _T("k") << Sub(_T("f")) << _T(" = ") << details.kf << _T(", ");
         *pPara << _T("k") << Sub(_T("c")) << _T(" = ") << details.ktd << rptNewLine;
         *pPara << symbol(psi) << _T("(")<<time2.SetValue( details.t);
         *pPara <<_T(",")<<time2.SetValue(details.ti)<<_T(") = ")<< creep.SetValue(details.Ct) << rptNewLine;

         *pPara << rptNewLine;

         details = pCamber->GetCreepCoefficientDetails(segmentKey,ICamber::cpReleaseToFinal,creep_time);
         *pPara << Bold(_T("Prestress release to final")) << rptNewLine;
         *pPara << RPT_FCI << _T(" = ") << fc.SetValue( details.Fc ) << _T(", ");
         *pPara << _T("t") << Sub(_T("i")) << _T(" = ") << time.SetValue( details.ti) << _T(", ");
         *pPara << _T("t = ")<< time.SetValue( details.t ) << _T(", ");
         *pPara << _T("k") << Sub(_T("f")) << _T(" = ") << details.kf << _T(", ");
         *pPara << _T("k") << Sub(_T("c")) << _T(" = ") << details.ktd << rptNewLine;
         *pPara << symbol(psi) << _T("(")<<time2.SetValue( details.t);
         *pPara <<_T(",")<<time2.SetValue(details.ti)<<_T(") = ")<< creep.SetValue(details.Ct) << rptNewLine;

         *pPara << rptNewLine;

         details = pCamber->GetCreepCoefficientDetails(segmentKey,ICamber::cpDiaphragmToDeck, creep_time);
         *pPara << Bold(_T("Temporary strand removal and diaphragm casting to application of superimposed dead loads")) << rptNewLine;
         *pPara << RPT_FCI << _T(" = ") << fc.SetValue( details.Fc ) << _T(", ");
         *pPara << _T("t") << Sub(_T("i")) << _T(" = ") << time.SetValue( details.ti) << _T(", ");
         *pPara << _T("t = ")<< time.SetValue( details.t ) << _T(", ");
         *pPara << _T("k") << Sub(_T("f")) << _T(" = ") << details.kf << _T(", ");
         *pPara << _T("k") << Sub(_T("c")) << _T(" = ") << details.ktd << rptNewLine;
         *pPara << symbol(psi) << _T("(")<<time2.SetValue( details.t);
         *pPara <<_T(",")<<time2.SetValue(details.ti)<<_T(") = ")<< creep.SetValue(details.Ct) << rptNewLine;

         *pPara << rptNewLine;

         details = pCamber->GetCreepCoefficientDetails(segmentKey,ICamber::cpDeckToFinal, creep_time);
         *pPara << Bold(_T("Application of superimposed dead loads to final")) << rptNewLine;
         *pPara << RPT_FCI << _T(" = ") << fc.SetValue( details.Fc ) << _T(", ");
         *pPara << _T("t") << Sub(_T("i")) << _T(" = ") << time.SetValue( details.ti) << _T(", ");
         *pPara << _T("t = ")<< time.SetValue( details.t ) << _T(", ");
         *pPara << _T("k") << Sub(_T("f")) << _T(" = ") << details.kf << _T(", ");
         *pPara << _T("k") << Sub(_T("c")) << _T(" = ") << details.ktd << rptNewLine;
         *pPara << symbol(psi) << _T("(")<<time2.SetValue( details.t);
         *pPara <<_T(",")<<time2.SetValue(details.ti)<<_T(") = ")<< creep.SetValue(details.Ct) << rptNewLine;
      }
      else
      {
         // 2005 and later
         *pPara << rptNewLine;
         *pPara << Bold(_T("Prestress release until temporary strand removal and diaphragm casting")) <<rptNewLine;
         *pPara << RPT_FCI << _T(" = ") << fc.SetValue( details.Fc ) << _T(", ");
         if (pSpecEntry->GetSpecificationCriteria().GetEdition() < WBFL::LRFD::BDSManager::Edition::FourthEdition2007)
         {
            *pPara << _T("t") << Sub(_T("i")) << _T(" (Adjusted) = ") << time.SetValue(details.ti) << _T(", ");
         }
         else
         {
            *pPara << _T("t") << Sub(_T("i")) << _T(" = ") << time.SetValue(details.ti) << _T(", ");
         }
         *pPara << _T("t = ")<< time.SetValue(details.t) << _T(", ");

         if ( WBFL::LRFD::BDSManager::Edition::FourthEdition2007 <= pSpecEntry->GetSpecificationCriteria().GetEdition() )
         {
            *pPara << _T("k") << Sub(_T("s")) << _T(" = ") << details.kvs << _T(", ");
         }
         else
         {
            *pPara << _T("k") << Sub(_T("vs")) << _T(" = ") << details.kvs << _T(", ");
         }

         *pPara << _T("k") << Sub(_T("hc")) << _T(" = ") << details.khc << _T(", ");
         *pPara << _T("k") << Sub(_T("f")) << _T(" = ") << details.kf << _T(", ");
         *pPara << _T("k") << Sub(_T("td")) << _T(" = ") << details.ktd;

         if (pSegmentData->GetSegmentMaterial(segmentKey)->Concrete.Type == pgsTypes::UHPC)
         {
            *pPara << _T(", ") << Sub2(_T("k"), _T("l")) << _T(" = ") << details.kl << _T(", ");
         }

         *pPara << rptNewLine;
         *pPara << symbol(psi) << _T("(")<<time2.SetValue(details.t);
         *pPara <<_T(",")<<time2.SetValue(details.ti)<<_T(") = ")<< creep.SetValue(details.Ct) << rptNewLine;

         *pPara << rptNewLine;

         details = pCamber->GetCreepCoefficientDetails(segmentKey,ICamber::cpReleaseToDeck, creep_time);
         *pPara << Bold(_T("Prestress release until application of superimposed dead loads")) << rptNewLine;
         *pPara << RPT_FCI << _T(" = ") << fc.SetValue( details.Fc ) << _T(", ");
         *pPara << _T("t") << Sub(_T("i")) << _T(" = ") << time.SetValue( details.ti) << _T(", ");
         *pPara << _T("t = ")<< time.SetValue(details.t) << _T(", ");

         if ( WBFL::LRFD::BDSManager::Edition::FourthEdition2007 <= pSpecEntry->GetSpecificationCriteria().GetEdition() )
         {
            *pPara << _T("k") << Sub(_T("s")) << _T(" = ") << details.kvs << _T(", ");
         }
         else
         {
            *pPara << _T("k") << Sub(_T("vs")) << _T(" = ") << details.kvs << _T(", ");
         }

         *pPara << _T("k") << Sub(_T("hc")) << _T(" = ") << details.khc << _T(", ");
         *pPara << _T("k") << Sub(_T("f")) << _T(" = ") << details.kf << _T(", ");
         *pPara << _T("k") << Sub(_T("td")) << _T(" = ") << details.ktd;

         if (pSegmentData->GetSegmentMaterial(segmentKey)->Concrete.Type == pgsTypes::UHPC)
         {
            *pPara << _T(", ") << Sub2(_T("k"), _T("l")) << _T(" = ") << details.kl << _T(", ");
         }

         *pPara << rptNewLine;

         *pPara << symbol(psi) << _T("(")<<time2.SetValue(details.t);
         *pPara <<_T(",")<<time2.SetValue(details.ti)<<_T(") = ")<< creep.SetValue(details.Ct) << rptNewLine;

         *pPara << rptNewLine;

         details = pCamber->GetCreepCoefficientDetails(segmentKey,ICamber::cpReleaseToFinal, creep_time);
         *pPara << Bold(_T("Prestress release to final")) << rptNewLine;
         *pPara << RPT_FCI << _T(" = ") << fc.SetValue( details.Fc ) << _T(", ");
         *pPara << _T("t") << Sub(_T("i")) << _T(" = ") << time.SetValue( details.ti) << _T(", ");
         *pPara << _T("t = ")<< time.SetValue(details.t) << _T(", ");

         if ( WBFL::LRFD::BDSManager::Edition::FourthEdition2007 <= pSpecEntry->GetSpecificationCriteria().GetEdition() )
         {
            *pPara << _T("k") << Sub(_T("s")) << _T(" = ") << details.kvs << _T(", ");
         }
         else
         {
            *pPara << _T("k") << Sub(_T("vs")) << _T(" = ") << details.kvs << _T(", ");
         }

         *pPara << _T("k") << Sub(_T("hc")) << _T(" = ") << details.khc << _T(", ");
         *pPara << _T("k") << Sub(_T("f")) << _T(" = ") << details.kf << _T(", ");
         *pPara << _T("k") << Sub(_T("td")) << _T(" = ") << details.ktd;

         if (pSegmentData->GetSegmentMaterial(segmentKey)->Concrete.Type == pgsTypes::UHPC)
         {
            *pPara << _T(", ") << Sub2(_T("k"), _T("l")) << _T(" = ") << details.kl << _T(", ");
         }

         *pPara << rptNewLine;

         *pPara << symbol(psi) << _T("(")<<time2.SetValue(details.t);
         *pPara <<_T(",")<<time2.SetValue(details.ti)<<_T(") = ")<< creep.SetValue(details.Ct) << rptNewLine;

         *pPara << rptNewLine;

         details = pCamber->GetCreepCoefficientDetails(segmentKey,ICamber::cpDiaphragmToDeck, creep_time);
         *pPara << Bold(_T("Temporary strand removal and diaphragm casting to application of superimposed dead loads")) << rptNewLine;
         *pPara << RPT_FCI << _T(" = ") << fc.SetValue( details.Fc ) << _T(", ");
         *pPara << _T("t") << Sub(_T("i")) << _T(" = ") << time.SetValue( details.ti) << _T(", ");
         *pPara << _T("t = ")<< time.SetValue(details.t) << _T(", ");

         if ( WBFL::LRFD::BDSManager::Edition::FourthEdition2007 <= pSpecEntry->GetSpecificationCriteria().GetEdition() )
         {
            *pPara << _T("k") << Sub(_T("s")) << _T(" = ") << details.kvs << _T(", ");
         }
         else
         {
            *pPara << _T("k") << Sub(_T("vs")) << _T(" = ") << details.kvs << _T(", ");
         }

         *pPara << _T("k") << Sub(_T("hc")) << _T(" = ") << details.khc << _T(", ");
         *pPara << _T("k") << Sub(_T("f")) << _T(" = ") << details.kf << _T(", ");
         *pPara << _T("k") << Sub(_T("td")) << _T(" = ") << details.ktd;
         
         if (pSegmentData->GetSegmentMaterial(segmentKey)->Concrete.Type == pgsTypes::UHPC)
         {
            *pPara << _T(", ") << Sub2(_T("k"), _T("l")) << _T(" = ") << details.kl << _T(", ");
         }

         *pPara << rptNewLine;

         *pPara << symbol(psi) << _T("(")<<time2.SetValue(details.t);
         *pPara <<_T(",")<<time2.SetValue(details.ti)<<_T(") = ")<< creep.SetValue(details.Ct) << rptNewLine;

         *pPara << rptNewLine;

         details = pCamber->GetCreepCoefficientDetails(segmentKey,ICamber::cpDiaphragmToFinal, creep_time);
         *pPara << Bold(_T("Temporary strand removal and diaphragm casting to final")) << rptNewLine;
         *pPara << RPT_FCI << _T(" = ") << fc.SetValue( details.Fc ) << _T(", ");
         *pPara << _T("t") << Sub(_T("i")) << _T(" = ") << time.SetValue( details.ti) << _T(", ");
         *pPara << _T("t = ")<< time.SetValue(details.t) << _T(", ");

         if ( WBFL::LRFD::BDSManager::Edition::FourthEdition2007 <= pSpecEntry->GetSpecificationCriteria().GetEdition() )
         {
            *pPara << _T("k") << Sub(_T("s")) << _T(" = ") << details.kvs << _T(", ");
         }
         else
         {
            *pPara << _T("k") << Sub(_T("vs")) << _T(" = ") << details.kvs << _T(", ");
         }

         *pPara << _T("k") << Sub(_T("hc")) << _T(" = ") << details.khc << _T(", ");
         *pPara << _T("k") << Sub(_T("f")) << _T(" = ") << details.kf << _T(", ");
         *pPara << _T("k") << Sub(_T("td")) << _T(" = ") << details.ktd;

         if (pSegmentData->GetSegmentMaterial(segmentKey)->Concrete.Type == pgsTypes::UHPC)
         {
            *pPara << _T(", ") << Sub2(_T("k"), _T("l")) << _T(" = ") << details.kl << _T(", ");
         }

         *pPara << rptNewLine;

         *pPara << symbol(psi) << _T("(")<<time2.SetValue(details.t);
         *pPara <<_T(",")<<time2.SetValue(details.ti)<<_T(") = ")<< creep.SetValue(details.Ct) << rptNewLine;

         *pPara << rptNewLine;

         details = pCamber->GetCreepCoefficientDetails(segmentKey,ICamber::cpDeckToFinal, creep_time);
         *pPara << Bold(_T("Application of superimposed dead loads to final")) << rptNewLine;
         *pPara << RPT_FCI << _T(" = ") << fc.SetValue( details.Fc ) << _T(", ");
         *pPara << _T("t") << Sub(_T("i")) << _T(" = ") << time.SetValue( details.ti) << _T(", ");
         *pPara << _T("t = ")<< time.SetValue(details.t) << _T(", ");

         if ( WBFL::LRFD::BDSManager::Edition::FourthEdition2007 <= pSpecEntry->GetSpecificationCriteria().GetEdition() )
         {
            *pPara << _T("k") << Sub(_T("s")) << _T(" = ") << details.kvs << _T(", ");
         }
         else
         {
            *pPara << _T("k") << Sub(_T("vs")) << _T(" = ") << details.kvs << _T(", ");
         }

         *pPara << _T("k") << Sub(_T("hc")) << _T(" = ") << details.khc << _T(", ");
         *pPara << _T("k") << Sub(_T("f")) << _T(" = ") << details.kf << _T(", ");
         *pPara << _T("k") << Sub(_T("td")) << _T(" = ") << details.ktd;

         if (pSegmentData->GetSegmentMaterial(segmentKey)->Concrete.Type == pgsTypes::UHPC)
         {
            *pPara << _T(", ") << Sub2(_T("k"), _T("l")) << _T(" = ") << details.kl << _T(", ");
         }

         *pPara << rptNewLine;

         *pPara << symbol(psi) << _T("(")<<time2.SetValue(details.t);
         *pPara <<_T(",")<<time2.SetValue(details.ti)<<_T(") = ")<< creep.SetValue(details.Ct) << rptNewLine;
      } // spec type

      *pPara << rptNewLine;
   }// loop on i


   return pPara;
}

rptParagraph* CLRFDCreepCoefficientChapterBuilder::Build_NoDeck(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,IBroker* pBroker,const CSegmentKey& segmentKey,
                                                          IEAFDisplayUnits* pDisplayUnits,
                                                          Uint16 level) const
{
   return Build_NoDeck_TempStrands(pRptSpec, pBroker, segmentKey, pDisplayUnits, level);
}