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
#include <Reporting\LRFDTimeDependentCreepCoefficientChapterBuilder.h>

#include <IFace\AnalysisResults.h>
#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\Intervals.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CLRFDTimeDependentCreepCoefficientChapterBuilder
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CLRFDTimeDependentCreepCoefficientChapterBuilder::CLRFDTimeDependentCreepCoefficientChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CLRFDTimeDependentCreepCoefficientChapterBuilder::GetName() const
{
   return TEXT("Creep Coefficient Details");
}

rptChapter* CLRFDTimeDependentCreepCoefficientChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CGirderReportSpecification* pGirderRptSpec = dynamic_cast<CGirderReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pGirderRptSpec->GetBroker(&pBroker);
   const CGirderKey& girderKey(pGirderRptSpec->GetGirderKey());

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IMaterials,pMaterials);

   INIT_UV_PROTOTYPE( rptLengthUnitValue, vsRatio, pDisplayUnits->GetComponentDimUnit(), false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), false);

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   bool bSI = IS_SI_UNITS(pDisplayUnits);

   if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::ThirdEditionWith2005Interims )
   {
      *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("LRFDCreepEqn.png")) << rptNewLine;
      *pPara << Bold(_T("for which:")) << rptNewLine;
      *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + (bSI ? _T("KfEqn-SI.png") : _T("KfEqn-US.png")) ) << rptNewLine;
      *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + (bSI ? _T("KcEqn-SI.png") : _T("KcEqn-US.png")) ) << rptNewLine;
      *pPara << rptNewLine;
   }
   else
   {
      if ( lrfdVersionMgr::FourthEdition2007 <= lrfdVersionMgr::GetVersion() )
      {
         *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("LRFDCreepEqn2007.png")) << rptNewLine;
      }
      else
      {
         *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("LRFDCreepEqn2005.png")) << rptNewLine;
      }
      
      *pPara << Bold(_T("for which:")) << rptNewLine;

      if ( lrfdVersionMgr::GetVersion() <= lrfdVersionMgr::ThirdEditionWith2005Interims )
      {
         *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + (bSI ? _T("KvsEqn-SI.png") : _T("KvsEqn-US.png")) ) << rptNewLine;
      }
      else if ( lrfdVersionMgr::ThirdEditionWith2006Interims == lrfdVersionMgr::GetVersion() )
      {
         *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + (bSI ? _T("KvsEqn2006-SI.png") : _T("KvsEqn2006-US.png")) ) << rptNewLine;
      }
      else
      {
         *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + (bSI ? _T("KvsEqn2007-SI.png") : _T("KvsEqn2007-US.png")) ) << rptNewLine;
      }

      *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("KhcEqn.png") ) << rptNewLine;
      *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + (bSI ? _T("KfEqn2005-SI.png") : _T("KfEqn2005-US.png")) ) << rptNewLine;

      if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::SeventhEditionWith2015Interims )
      {
         *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + (bSI ? _T("KtdEqn-SI.png") : _T("KtdEqn-US.png")) ) << rptNewLine;
      }
      else
      {
         ATLASSERT(!bSI);
         *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("KtdEqn-US2015.png")) << rptNewLine;
      }

      *pPara << rptNewLine;
   } // spec

   ColumnIndexType nColumns = 6;
   if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
   {
      nColumns += 4; // K1, K2, ks, khc
   }
   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(nColumns);
   *pPara << pTable << rptNewLine;

   pTable->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   pTable->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));


   RowIndexType rowIdx = 0;
   ColumnIndexType colIdx = 0;
   (*pTable)(rowIdx,colIdx++) << _T("Element");
   (*pTable)(rowIdx,colIdx++) << _T("Curing") << rptNewLine << _T("Method");
   if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
   {
      (*pTable)(rowIdx,colIdx++) << Sub2(_T("K"),_T("1"));
      (*pTable)(rowIdx,colIdx++) << Sub2(_T("K"),_T("2"));
   }
   (*pTable)(rowIdx,colIdx++) << COLHDR(_T("V/S"),rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit());

   if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
   {
      if ( lrfdVersionMgr::FourthEdition2007 <= lrfdVersionMgr::GetVersion() )
      {
         (*pTable)(rowIdx,colIdx++) << Sub2(_T("k"),_T("s"));
      }
      else
      {
         (*pTable)(rowIdx,colIdx++) << Sub2(_T("k"),_T("vs"));
      }
   }

   (*pTable)(rowIdx,colIdx++) << _T("H (%)");
   if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
   {
      (*pTable)(rowIdx,colIdx++) << Sub2(_T("k"),_T("hc"));
   }
   (*pTable)(rowIdx,colIdx++) << COLHDR(RPT_FCI,rptStressUnitTag,pDisplayUnits->GetStressUnit());
   (*pTable)(rowIdx,colIdx++) << Sub2(_T("k"),_T("f"));


   std::_tstring strCuring[] = { _T("Moist"), _T("Steam") };

   rowIdx = pTable->GetNumberOfHeaderRows();
   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      ColumnIndexType colIdx = 0;

      CSegmentKey segmentKey(girderKey,segIdx);
      const matConcreteBase* pConcrete = pMaterials->GetSegmentConcrete(segmentKey);
      const lrfdLRFDTimeDependentConcrete* pLRFDConcrete = dynamic_cast<const lrfdLRFDTimeDependentConcrete*>(pConcrete);

      (*pTable)(rowIdx,colIdx++) << _T("Segment ") << LABEL_SEGMENT(segIdx);
      (*pTable)(rowIdx,colIdx++) << strCuring[pConcrete->GetCureMethod()];
      if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
      {
         Float64 K1, K2;
         pLRFDConcrete->GetCreepCorrectionFactors(&K1,&K2);
         (*pTable)(rowIdx,colIdx++) << K1;
         (*pTable)(rowIdx,colIdx++) << K2;
      }

      Float64 t = pLRFDConcrete->GetTimeAtCasting() + pLRFDConcrete->GetCureTime();

      (*pTable)(rowIdx,colIdx++) << vsRatio.SetValue(pConcrete->GetVSRatio());

      if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
      {
         (*pTable)(rowIdx,colIdx++) << pLRFDConcrete->GetSizeFactorCreep(-99999,-99999); // 2005 and later, doesn't rely on time
      }
      (*pTable)(rowIdx,colIdx++) << pConcrete->GetRelativeHumidity();
      if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
      {
         (*pTable)(rowIdx,colIdx++) << pLRFDConcrete->GetRelativeHumidityFactorCreep();
      }

      Float64 fci = pLRFDConcrete->GetFc(t);
      (*pTable)(rowIdx,colIdx++) << stress.SetValue(fci);

      (*pTable)(rowIdx,colIdx++) << pLRFDConcrete->GetConcreteStrengthFactor();

      rowIdx++;

      if ( segIdx != nSegments-1 )
      {
         CClosureKey closureKey(segmentKey);
         const matConcreteBase* pConcrete = pMaterials->GetClosureJointConcrete(closureKey);
         const lrfdLRFDTimeDependentConcrete* pLRFDConcrete = dynamic_cast<const lrfdLRFDTimeDependentConcrete*>(pConcrete);
   
         colIdx = 0;

         (*pTable)(rowIdx,colIdx++) << _T("Closure Joint");
         (*pTable)(rowIdx,colIdx++) << strCuring[pConcrete->GetCureMethod()];
         if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
         {
            Float64 K1, K2;
            pLRFDConcrete->GetCreepCorrectionFactors(&K1,&K2);
            (*pTable)(rowIdx,colIdx++) << K1;
            (*pTable)(rowIdx,colIdx++) << K2;
         }

         Float64 t = pLRFDConcrete->GetTimeAtCasting() + pLRFDConcrete->GetCureTime();

         (*pTable)(rowIdx,colIdx++) << vsRatio.SetValue(pConcrete->GetVSRatio());

         if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
         {
            (*pTable)(rowIdx,colIdx++) << pLRFDConcrete->GetSizeFactorCreep(-99999,-99999); // 2005 and later, doesn't rely on time
         }

         (*pTable)(rowIdx,colIdx++) << pConcrete->GetRelativeHumidity();
         if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
         {
            (*pTable)(rowIdx,colIdx++) << pLRFDConcrete->GetRelativeHumidityFactorCreep();
         }

         Float64 fci = pLRFDConcrete->GetFc(t);
         (*pTable)(rowIdx,colIdx++) << stress.SetValue(fci);
         (*pTable)(rowIdx,colIdx++) << pLRFDConcrete->GetConcreteStrengthFactor();

         rowIdx++;
      }
   }

   if ( pBridge->GetDeckType() != pgsTypes::sdtNone )
   {
      const matConcreteBase* pConcrete = pMaterials->GetDeckConcrete();
      const lrfdLRFDTimeDependentConcrete* pLRFDConcrete = dynamic_cast<const lrfdLRFDTimeDependentConcrete*>(pConcrete);

      ColumnIndexType colIdx = 0;

      (*pTable)(rowIdx,colIdx++) << _T("Deck");
      (*pTable)(rowIdx,colIdx++) << strCuring[pConcrete->GetCureMethod()];
      if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
      {
         Float64 K1, K2;
         pLRFDConcrete->GetCreepCorrectionFactors(&K1,&K2);
         (*pTable)(rowIdx,colIdx++) << K1;
         (*pTable)(rowIdx,colIdx++) << K2;
      }

      Float64 t = pLRFDConcrete->GetTimeAtCasting() + pLRFDConcrete->GetCureTime();

      (*pTable)(rowIdx,colIdx++) << vsRatio.SetValue(pConcrete->GetVSRatio());

      if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
      {
         (*pTable)(rowIdx,colIdx++) << pLRFDConcrete->GetSizeFactorCreep(-99999,-99999); // 2005 and later, doesn't rely on time
      }

      (*pTable)(rowIdx,colIdx++) << pConcrete->GetRelativeHumidity();
      if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
      {
         (*pTable)(rowIdx,colIdx++) << pLRFDConcrete->GetRelativeHumidityFactorCreep();
      }

      Float64 fci = pLRFDConcrete->GetFc(t);
      (*pTable)(rowIdx,colIdx++) << stress.SetValue(fci);
      (*pTable)(rowIdx,colIdx++) << pLRFDConcrete->GetConcreteStrengthFactor();

      rowIdx++;
   }

#if defined _DEBUG || defined _BETA_VERSION
   nColumns = 8*nSegments + 1; // four for each segment + four for the deck + 1 for the interval

   if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::ThirdEditionWith2005Interims )
   {
      nColumns += 2*nSegments + 1;
   }

   pTable = rptStyleManager::CreateDefaultTable(nColumns);
   *pPara << pTable << rptNewLine;

   pTable->SetNumberOfHeaderRows(2);
   pTable->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   pTable->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   rowIdx = 0;
   colIdx = 0;

   ColumnIndexType colSpan = 4;
   if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::ThirdEditionWith2005Interims )
   {
      colSpan++;
   }

   pTable->SetRowSpan(rowIdx,colIdx,2);
   (*pTable)(rowIdx,colIdx++) << _T("Interval");
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      pTable->SetColumnSpan(rowIdx,colIdx,colSpan);
      (*pTable)(rowIdx,colIdx) << _T("Segment ") << LABEL_SEGMENT(segIdx);
      (*pTable)(rowIdx+1,colIdx++) << Sub2(_T("t"),_T("i")) << rptNewLine << _T("(days)");
      (*pTable)(rowIdx+1,colIdx++) << _T("t") << rptNewLine << _T("(days)");

      if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::ThirdEditionWith2005Interims )
      {
         (*pTable)(rowIdx+1,colIdx++) << Sub2(_T("k"),_T("c"));
      }

      (*pTable)(rowIdx+1,colIdx++) << Sub2(_T("k"),_T("td"));
      (*pTable)(rowIdx+1,colIdx++) << symbol(psi) << _T("(t,") << Sub2(_T("t"),_T("i")) << _T(")");

      if ( segIdx != nSegments-1 )
      {
         pTable->SetColumnSpan(rowIdx,colIdx,colSpan);
         (*pTable)(rowIdx,colIdx) << _T("Closure Joint ") << LABEL_SEGMENT(segIdx);
         (*pTable)(rowIdx+1,colIdx++) << Sub2(_T("t"),_T("i")) << rptNewLine << _T("(days)");
         (*pTable)(rowIdx+1,colIdx++) << _T("t") << rptNewLine << _T("(days)");

         if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::ThirdEditionWith2005Interims )
         {
            (*pTable)(rowIdx+1,colIdx++) << Sub2(_T("k"),_T("c"));
         }

         (*pTable)(rowIdx+1,colIdx++) << Sub2(_T("k"),_T("td"));
         (*pTable)(rowIdx+1,colIdx++) << symbol(psi) << _T("(t,") << Sub2(_T("t"),_T("i")) << _T(")");
      }
   }
   pTable->SetColumnSpan(rowIdx,colIdx,colSpan);
   (*pTable)(rowIdx,colIdx) << _T("Deck");
   (*pTable)(rowIdx+1,colIdx++) << Sub2(_T("t"),_T("i")) << rptNewLine << _T("(days)");
   (*pTable)(rowIdx+1,colIdx++) << _T("t") << rptNewLine << _T("(days)");

   if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::ThirdEditionWith2005Interims )
   {
      (*pTable)(rowIdx+1,colIdx++) << Sub2(_T("k"),_T("c"));
   }

   (*pTable)(rowIdx+1,colIdx++) << Sub2(_T("k"),_T("td"));
   (*pTable)(rowIdx+1,colIdx++) << symbol(psi) << _T("(t,") << Sub2(_T("t"),_T("i")) << _T(")");

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType nIntervals = pIntervals->GetIntervalCount();

   rowIdx = pTable->GetNumberOfHeaderRows();
   for ( IntervalIndexType intervalIdx = 0; intervalIdx < nIntervals; intervalIdx++, rowIdx++ )
   {
      colIdx = 0;
      (*pTable)(rowIdx,colIdx++) << LABEL_INTERVAL(intervalIdx);

      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         CSegmentKey segmentKey(girderKey,segIdx);

         IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
         if ( releaseIntervalIdx <= intervalIdx )
         {
            Float64 ti = pMaterials->GetSegmentConcreteAge(segmentKey,releaseIntervalIdx,pgsTypes::Middle);
            Float64 t  = pMaterials->GetSegmentConcreteAge(segmentKey,intervalIdx,pgsTypes::End);
            const matConcreteBase* pConcrete = pMaterials->GetSegmentConcrete(segmentKey);
            const lrfdLRFDTimeDependentConcrete* pLRFDConcrete = dynamic_cast<const lrfdLRFDTimeDependentConcrete*>(pConcrete);
            (*pTable)(rowIdx,colIdx++) << ti;
            if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::ThirdEditionWith2005Interims )
            {
               (*pTable)(rowIdx,colIdx++) << t;
            }
            else
            {
               (*pTable)(rowIdx,colIdx++) << t - ti;
            }
            if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::ThirdEditionWith2005Interims )
            {
               (*pTable)(rowIdx,colIdx++) << pLRFDConcrete->GetSizeFactorCreep(t,ti);
            }

            std::shared_ptr<matConcreteBaseCreepDetails> pDetails = pMaterials->GetSegmentCreepCoefficientDetails(segmentKey,releaseIntervalIdx,pgsTypes::Middle,intervalIdx,pgsTypes::End);
            lrfdLRFDTimeDependentConcreteCreepDetails* pLRFDDetails = static_cast<lrfdLRFDTimeDependentConcreteCreepDetails*>(pDetails.get());

            (*pTable)(rowIdx,colIdx++) << pLRFDDetails->ktd;
            (*pTable)(rowIdx,colIdx++) << pDetails->Ct;
         }
         else
         {
            (*pTable)(rowIdx,colIdx++) << _T("");
            (*pTable)(rowIdx,colIdx++) << _T("");
            if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::ThirdEditionWith2005Interims )
            {
               (*pTable)(rowIdx,colIdx++) << _T("");
            }
            (*pTable)(rowIdx,colIdx++) << _T("");
            (*pTable)(rowIdx,colIdx++) << _T("");
         }

         if ( segIdx < nSegments-1 )
         {
            CClosureKey closureKey(segmentKey);
            IntervalIndexType compositeClosureIntervalIdx = pIntervals->GetCompositeClosureJointInterval(closureKey);
            if ( compositeClosureIntervalIdx <= intervalIdx )
            {
               Float64 ti = pMaterials->GetClosureJointConcreteAge(segmentKey,compositeClosureIntervalIdx,pgsTypes::Middle);
               Float64 t  = pMaterials->GetClosureJointConcreteAge(segmentKey,intervalIdx,pgsTypes::End);
               const matConcreteBase* pConcrete = pMaterials->GetClosureJointConcrete(closureKey);
               const lrfdLRFDTimeDependentConcrete* pLRFDConcrete = dynamic_cast<const lrfdLRFDTimeDependentConcrete*>(pConcrete);
               (*pTable)(rowIdx,colIdx++) << ti;
               if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::ThirdEditionWith2005Interims )
               {
                  (*pTable)(rowIdx,colIdx++) << t;
               }
               else
               {
                  (*pTable)(rowIdx,colIdx++) << t - ti;
               }
               if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::ThirdEditionWith2005Interims )
               {
                  (*pTable)(rowIdx,colIdx++) << pLRFDConcrete->GetSizeFactorCreep(t,ti);
               }

               std::shared_ptr<matConcreteBaseCreepDetails> pDetails = pMaterials->GetClosureJointCreepCoefficientDetails(closureKey,compositeClosureIntervalIdx,pgsTypes::Middle,intervalIdx,pgsTypes::End);
               lrfdLRFDTimeDependentConcreteCreepDetails* pLRFDDetails = static_cast<lrfdLRFDTimeDependentConcreteCreepDetails*>(pDetails.get());

               (*pTable)(rowIdx,colIdx++) << pLRFDDetails->ktd;
               (*pTable)(rowIdx,colIdx++) << pDetails->Ct;
            }
            else
            {
               (*pTable)(rowIdx,colIdx++) << _T("");
               (*pTable)(rowIdx,colIdx++) << _T("");
               if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::ThirdEditionWith2005Interims )
               {
                  (*pTable)(rowIdx,colIdx++) << _T("");
               }
               (*pTable)(rowIdx,colIdx++) << _T("");
               (*pTable)(rowIdx,colIdx++) << _T("");
            }
         }
      }

      IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval();
      if ( compositeDeckIntervalIdx <= intervalIdx )
      {
         Float64 ti = pMaterials->GetDeckConcreteAge(compositeDeckIntervalIdx,pgsTypes::Middle);
         Float64 t  = pMaterials->GetDeckConcreteAge(intervalIdx,pgsTypes::End);
         const matConcreteBase* pConcrete = pMaterials->GetDeckConcrete();
         const lrfdLRFDTimeDependentConcrete* pLRFDConcrete = dynamic_cast<const lrfdLRFDTimeDependentConcrete*>(pConcrete);
         (*pTable)(rowIdx,colIdx++) << ti;
         if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::ThirdEditionWith2005Interims )
         {
            (*pTable)(rowIdx,colIdx++) << t;
         }
         else
         {
            (*pTable)(rowIdx,colIdx++) << t - ti;
         }
         if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::ThirdEditionWith2005Interims )
         {
            (*pTable)(rowIdx,colIdx++) << pLRFDConcrete->GetSizeFactorCreep(t,ti);
         }

         std::shared_ptr<matConcreteBaseCreepDetails> pDetails = pMaterials->GetDeckCreepCoefficientDetails(compositeDeckIntervalIdx,pgsTypes::Middle,intervalIdx,pgsTypes::End);
         lrfdLRFDTimeDependentConcreteCreepDetails* pLRFDDetails = static_cast<lrfdLRFDTimeDependentConcreteCreepDetails*>(pDetails.get());

         (*pTable)(rowIdx,colIdx++) << pLRFDDetails->ktd;
         (*pTable)(rowIdx,colIdx++) << pDetails->Ct;
      }
      else
      {
         (*pTable)(rowIdx,colIdx++) << _T("");
         (*pTable)(rowIdx,colIdx++) << _T("");
         if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::ThirdEditionWith2005Interims )
         {
            (*pTable)(rowIdx,colIdx++) << _T("");
         }
         (*pTable)(rowIdx,colIdx++) << _T("");
         (*pTable)(rowIdx,colIdx++) << _T("");
      }
   }
#endif //#if defined _DEBUG || defined _BETA_VERSION

   return pChapter;
}

CChapterBuilder* CLRFDTimeDependentCreepCoefficientChapterBuilder::Clone() const
{
   return new CLRFDTimeDependentCreepCoefficientChapterBuilder;
}
