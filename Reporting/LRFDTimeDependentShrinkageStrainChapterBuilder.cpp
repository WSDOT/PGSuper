///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2018  Washington State Department of Transportation
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
#include <Reporting\LRFDTimeDependentShrinkageStrainChapterBuilder.h>

#include <IFace\Bridge.h>
#include <IFace\Intervals.h>
#include <Material\ConcreteBase.h>
#include <Lrfd\LRFDTimeDependentConcrete.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CLRFDTimeDependentShrinkageStrainChapterBuilder
****************************************************************************/

CLRFDTimeDependentShrinkageStrainChapterBuilder::CLRFDTimeDependentShrinkageStrainChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

LPCTSTR CLRFDTimeDependentShrinkageStrainChapterBuilder::GetName() const
{
   return TEXT("Shrinkage Strain Details");
}

rptChapter* CLRFDTimeDependentShrinkageStrainChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
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

   std::_tstring strImagePath(rptStyleManager::GetImagePath());

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);
   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   bool bSI = IS_SI_UNITS(pDisplayUnits);
   if ( lrfdVersionMgr::SeventhEditionWith2015Interims <= lrfdVersionMgr::GetVersion() )
   {
      *pPara << rptRcImage(strImagePath + _T("LRFD_Shrinkage_2015.png")) << rptNewLine;
   }
   else if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() && lrfdVersionMgr::GetVersion() < lrfdVersionMgr::SeventhEditionWith2015Interims )
   {
      *pPara << rptRcImage(strImagePath + (bSI ? _T("LRFD_Shrinkage_2005_SI.png") : _T("LRFD_Shrinkage_2005_US.png"))) << rptNewLine;
   }
   else
   {
      rptRcTable* pTable = rptStyleManager::CreateDefaultTable(2);

      *pPara << pTable << rptNewLine;
      (*pTable)(0,0) << _T("Moist Curing");
      (*pTable)(0,1) << _T("Steam Curing");
      (*pTable)(1,0) << rptRcImage(strImagePath + (bSI ? _T("LRFD_Shrinkage_MoistCured_SI.png") : _T("LRFD_Shrinkage_MoistCured_US.png"))) << rptNewLine;
      (*pTable)(1,1) << rptRcImage(strImagePath + (bSI ? _T("LRFD_Shrinkage_SteamCured_SI.png") : _T("LRFD_Shrinkage_SteamCured_US.png"))) << rptNewLine;
   }

   ColumnIndexType nColumns = 5;
   if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
   {
      nColumns += 5; // K1, K2, ks, f'ci, kf
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
      (*pTable)(rowIdx,colIdx++) << Sub2(_T("k"),_T("s"));
   }

   (*pTable)(rowIdx,colIdx++) << _T("H (%)");
   (*pTable)(rowIdx,colIdx++) << Sub2(_T("k"),_T("hs"));
   if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
   {
      (*pTable)(rowIdx,colIdx++) << COLHDR(RPT_FCI,rptStressUnitTag,pDisplayUnits->GetStressUnit());
      (*pTable)(rowIdx,colIdx++) << Sub2(_T("k"),_T("f"));
   }


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

      Float64 t = pLRFDConcrete->GetTimeAtCasting() + pLRFDConcrete->GetCureTime();

      if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
      {
         Float64 K1, K2;
         pLRFDConcrete->GetShrinkageCorrectionFactors(&K1,&K2);
         (*pTable)(rowIdx,colIdx++) << K1;
         (*pTable)(rowIdx,colIdx++) << K2;
      }

      (*pTable)(rowIdx,colIdx++) << vsRatio.SetValue(pConcrete->GetVSRatio());

      if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
      {
         (*pTable)(rowIdx,colIdx++) << pLRFDConcrete->GetSizeFactorShrinkage(-99999); // not a function of time so use any bogus value
      }

      (*pTable)(rowIdx,colIdx++) << pConcrete->GetRelativeHumidity();
      (*pTable)(rowIdx,colIdx++) << pLRFDConcrete->GetRelativeHumidityFactorShrinkage();

      if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
      {
         Float64 fci = pLRFDConcrete->GetFc(t);
         (*pTable)(rowIdx,colIdx++) << stress.SetValue(fci);
         (*pTable)(rowIdx,colIdx++) << pLRFDConcrete->GetConcreteStrengthFactor();
      }

      rowIdx++;

      if ( segIdx != nSegments-1 )
      {
         CClosureKey closureKey(segmentKey);
         const matConcreteBase* pConcrete = pMaterials->GetClosureJointConcrete(closureKey);
         const lrfdLRFDTimeDependentConcrete* pACIConcrete = dynamic_cast<const lrfdLRFDTimeDependentConcrete*>(pConcrete);
   
         colIdx = 0;

         (*pTable)(rowIdx,colIdx++) << _T("Closure Joint");
         (*pTable)(rowIdx,colIdx++) << strCuring[pConcrete->GetCureMethod()];

         Float64 t = pLRFDConcrete->GetTimeAtCasting() + pLRFDConcrete->GetCureTime();

         if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
         {
            Float64 K1, K2;
            pLRFDConcrete->GetShrinkageCorrectionFactors(&K1,&K2);
            (*pTable)(rowIdx,colIdx++) << K1;
            (*pTable)(rowIdx,colIdx++) << K2;
         }

         (*pTable)(rowIdx,colIdx++) << vsRatio.SetValue(pConcrete->GetVSRatio());

         if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
         {
            (*pTable)(rowIdx,colIdx++) << pLRFDConcrete->GetSizeFactorShrinkage(-99999); // not a function of time so use any bogus value
         }

         (*pTable)(rowIdx,colIdx++) << pConcrete->GetRelativeHumidity();
         (*pTable)(rowIdx,colIdx++) << pLRFDConcrete->GetRelativeHumidityFactorShrinkage();

         if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
         {
            Float64 fci = pLRFDConcrete->GetFc(t);
            (*pTable)(rowIdx,colIdx++) << stress.SetValue(fci);
            (*pTable)(rowIdx,colIdx++) << pLRFDConcrete->GetConcreteStrengthFactor();
         }

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

      Float64 t = pLRFDConcrete->GetTimeAtCasting() + pLRFDConcrete->GetCureTime();

      if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
      {
         Float64 K1, K2;
         pLRFDConcrete->GetShrinkageCorrectionFactors(&K1,&K2);
         (*pTable)(rowIdx,colIdx++) << K1;
         (*pTable)(rowIdx,colIdx++) << K2;
      }

      (*pTable)(rowIdx,colIdx++) << vsRatio.SetValue(pConcrete->GetVSRatio());

      if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
      {
         (*pTable)(rowIdx,colIdx++) << pLRFDConcrete->GetSizeFactorShrinkage(-99999); // not a function of time so use any bogus value
      }

      (*pTable)(rowIdx,colIdx++) << pConcrete->GetRelativeHumidity();
      (*pTable)(rowIdx,colIdx++) << pLRFDConcrete->GetRelativeHumidityFactorShrinkage();

      if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
      {
         Float64 fci = pLRFDConcrete->GetFc(t);
         (*pTable)(rowIdx,colIdx++) << stress.SetValue(fci);
         (*pTable)(rowIdx,colIdx++) << pLRFDConcrete->GetConcreteStrengthFactor();
      }

      rowIdx++;
   }

#if defined _DEBUG || defined _BETA_VERSION
   nColumns = 6*nSegments+1;

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

   ColumnIndexType colSpan = 3;
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
      (*pTable)(rowIdx+1,colIdx++) << _T("t") << rptNewLine << _T("(days)");

      if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::ThirdEditionWith2005Interims )
      {
         (*pTable)(rowIdx+1,colIdx++) << Sub2(_T("k"),_T("s"));
      }

      (*pTable)(rowIdx+1,colIdx++) << symbol(DELTA) << Sub2(symbol(epsilon),_T("sh")) << _T("x10") << Super(_T("6"));
      (*pTable)(rowIdx+1,colIdx++) << Sub2(symbol(epsilon),_T("sh")) << _T("x10") << Super(_T("6"));
      if ( segIdx != nSegments-1 )
      {
         pTable->SetColumnSpan(rowIdx,colIdx,colSpan);
         (*pTable)(rowIdx,colIdx) << _T("Closure Joint ") << LABEL_SEGMENT(segIdx);
         (*pTable)(rowIdx+1,colIdx++) << _T("t") << rptNewLine << _T("(days)");

         if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::ThirdEditionWith2005Interims )
         {
            (*pTable)(rowIdx+1,colIdx++) << Sub2(_T("k"),_T("s"));
         }

         (*pTable)(rowIdx+1,colIdx++) << symbol(DELTA) << Sub2(symbol(epsilon),_T("sh")) << _T("x10") << Super(_T("6"));
         (*pTable)(rowIdx+1,colIdx++) << Sub2(symbol(epsilon),_T("sh")) << _T("x10") << Super(_T("6"));
      }
   }
   pTable->SetColumnSpan(rowIdx,colIdx,colSpan);
   (*pTable)(rowIdx,colIdx) << _T("Deck");
   (*pTable)(rowIdx+1,colIdx++) << _T("t") << rptNewLine << _T("(days)");

   if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::ThirdEditionWith2005Interims )
   {
      (*pTable)(rowIdx+1,colIdx++) << Sub2(_T("k"),_T("s"));
   }

   (*pTable)(rowIdx+1,colIdx++) << symbol(DELTA) << Sub2(symbol(epsilon),_T("sh")) << _T("x10") << Super(_T("6"));
   (*pTable)(rowIdx+1,colIdx++) << Sub2(symbol(epsilon),_T("sh")) << _T("x10") << Super(_T("6"));

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
            Float64 t  = pMaterials->GetSegmentConcreteAge(segmentKey,intervalIdx,pgsTypes::End);
            const matConcreteBase* pConcrete = pMaterials->GetSegmentConcrete(segmentKey);
            const lrfdLRFDTimeDependentConcrete* pLRFDConcrete = dynamic_cast<const lrfdLRFDTimeDependentConcrete*>(pConcrete);
            Float64 cure = pLRFDConcrete->GetCureTime();
            (*pTable)(rowIdx,colIdx++) << t - cure;

            if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::ThirdEditionWith2005Interims )
            {
               (*pTable)(rowIdx,colIdx++) << pLRFDConcrete->GetSizeFactorShrinkage(t);
            }

            (*pTable)(rowIdx,colIdx++) << 1E6*pMaterials->GetIncrementalSegmentFreeShrinkageStrain(segmentKey,intervalIdx);
            (*pTable)(rowIdx,colIdx++) << 1E6*pMaterials->GetTotalSegmentFreeShrinkageStrain(segmentKey,intervalIdx,pgsTypes::End);
         }
         else
         {
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
               Float64 t  = pMaterials->GetClosureJointConcreteAge(closureKey,intervalIdx,pgsTypes::End);
               const matConcreteBase* pConcrete = pMaterials->GetClosureJointConcrete(closureKey);
               const lrfdLRFDTimeDependentConcrete* pLRFDConcrete = dynamic_cast<const lrfdLRFDTimeDependentConcrete*>(pConcrete);
               Float64 cure = pLRFDConcrete->GetCureTime();
               (*pTable)(rowIdx,colIdx++) << t - cure;

               if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::ThirdEditionWith2005Interims )
               {
                  (*pTable)(rowIdx,colIdx++) << pLRFDConcrete->GetSizeFactorShrinkage(t);
               }

               (*pTable)(rowIdx,colIdx++) << 1E6*pMaterials->GetIncrementalClosureJointFreeShrinkageStrain(closureKey,intervalIdx);
               (*pTable)(rowIdx,colIdx++) << 1E6*pMaterials->GetTotalClosureJointFreeShrinkageStrain(closureKey,intervalIdx,pgsTypes::End);
            }
            else
            {
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
         Float64 t  = pMaterials->GetDeckConcreteAge(intervalIdx,pgsTypes::End);
         const matConcreteBase* pConcrete = pMaterials->GetDeckConcrete();
         const lrfdLRFDTimeDependentConcrete* pLRFDConcrete = dynamic_cast<const lrfdLRFDTimeDependentConcrete*>(pConcrete);
         Float64 cure = pLRFDConcrete->GetCureTime();
         (*pTable)(rowIdx,colIdx++) << t - cure;

         if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::ThirdEditionWith2005Interims )
         {
            (*pTable)(rowIdx,colIdx++) << pLRFDConcrete->GetSizeFactorShrinkage(t);
         }

         (*pTable)(rowIdx,colIdx++) << 1E6*pMaterials->GetIncrementalDeckFreeShrinkageStrain(intervalIdx);
         (*pTable)(rowIdx,colIdx++) << 1E6*pMaterials->GetTotalDeckFreeShrinkageStrain(intervalIdx,pgsTypes::End);
      }
      else
      {
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

CChapterBuilder* CLRFDTimeDependentShrinkageStrainChapterBuilder::Clone() const
{
   return new CLRFDTimeDependentShrinkageStrainChapterBuilder;
}
