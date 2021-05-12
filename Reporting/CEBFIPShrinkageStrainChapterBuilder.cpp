///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2021  Washington State Department of Transportation
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
#include <Reporting\CEBFIPShrinkageStrainChapterBuilder.h>

#include <IFace\Bridge.h>
#include <IFace\Intervals.h>
#include <IFace\Project.h>
#include <Material\ConcreteBase.h>
#include <Material\CEBFIPConcrete.h>
#include <PgsExt\TimelineEvent.h>
#include <PgsExt\CastDeckActivity.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CCEBFIPShrinkageStrainChapterBuilder
****************************************************************************/

CCEBFIPShrinkageStrainChapterBuilder::CCEBFIPShrinkageStrainChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

LPCTSTR CCEBFIPShrinkageStrainChapterBuilder::GetName() const
{
   return TEXT("Shrinkage Strain Details");
}

rptChapter* CCEBFIPShrinkageStrainChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CGirderReportSpecification* pGirderRptSpec = dynamic_cast<CGirderReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pGirderRptSpec->GetBroker(&pBroker);
   const CGirderKey& girderKey(pGirderRptSpec->GetGirderKey());

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IMaterials,pMaterials);

   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), false);
   INIT_UV_PROTOTYPE( rptLengthUnitValue, hDim, pDisplayUnits->GetComponentDimUnit(), false );

   std::_tstring strImagePath(rptStyleManager::GetImagePath());

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);
   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   *pPara << _T("Taken from \"CEB-FIP Model Code 1990\"") << rptNewLine;
   *pPara << rptNewLine;

   *pPara << rptRcImage(strImagePath + _T("CEBFIP_Shrinkage.png")) << rptNewLine;
   *pPara << rptNewLine;

   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(8);
   *pPara << pTable << rptNewLine;

   pTable->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   pTable->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));


   RowIndexType rowIdx = 0;
   ColumnIndexType colIdx = 0;
   (*pTable)(rowIdx,colIdx++) << _T("Element");
   (*pTable)(rowIdx,colIdx++) << COLHDR(RPT_FC << Sub(_T("28")),rptStressUnitTag,pDisplayUnits->GetStressUnit());
   (*pTable)(rowIdx,colIdx++) << _T("RH (%)");
   (*pTable)(rowIdx,colIdx++) << Sub2(symbol(beta),_T("RH"));
   (*pTable)(rowIdx,colIdx++) << Sub2(symbol(beta),_T("SC"));
   (*pTable)(rowIdx,colIdx++) << COLHDR(_T("h"),rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit());
   (*pTable)(rowIdx,colIdx++) << Sub2(symbol(epsilon),_T("s")) << _T("(") << RPT_FC << Sub(_T("28")) << _T(")") << _T("x10") << Super(_T("6"));
   (*pTable)(rowIdx,colIdx++) << Sub2(symbol(epsilon),_T("cso")) << _T("x10") << Super(_T("6"));

   rowIdx = pTable->GetNumberOfHeaderRows();
   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      ColumnIndexType colIdx = 0;

      CSegmentKey segmentKey(girderKey,segIdx);
      const matConcreteBase* pConcrete = pMaterials->GetSegmentConcrete(segmentKey);
      const matCEBFIPConcrete* pCEBFIPConcrete = dynamic_cast<const matCEBFIPConcrete*>(pConcrete);

      (*pTable)(rowIdx,colIdx++) << _T("Segment ") << LABEL_SEGMENT(segIdx);
      (*pTable)(rowIdx,colIdx++) << stress.SetValue(pCEBFIPConcrete->GetFc28());
      (*pTable)(rowIdx,colIdx++) << pConcrete->GetRelativeHumidity();
      (*pTable)(rowIdx,colIdx++) << pCEBFIPConcrete->GetBetaRH();
      (*pTable)(rowIdx,colIdx++) << pCEBFIPConcrete->GetBetaSc();
      (*pTable)(rowIdx,colIdx++) << hDim.SetValue(pCEBFIPConcrete->GetH());
      (*pTable)(rowIdx,colIdx++) << 1E6*pCEBFIPConcrete->GetEpsilonS();
      (*pTable)(rowIdx,colIdx++) << 1E6*pCEBFIPConcrete->GetNotionalShrinkageCoefficient();

      rowIdx++;

      if ( segIdx != nSegments-1 )
      {
         CClosureKey closureKey(segmentKey);
         const matConcreteBase* pConcrete = pMaterials->GetClosureJointConcrete(closureKey);
         const matCEBFIPConcrete* pCEBFIPConcrete = dynamic_cast<const matCEBFIPConcrete*>(pConcrete);
   
         colIdx = 0;

         (*pTable)(rowIdx,colIdx++) << _T("Closure Joint");
         (*pTable)(rowIdx,colIdx++) << stress.SetValue(pCEBFIPConcrete->GetFc28());
         (*pTable)(rowIdx,colIdx++) << pConcrete->GetRelativeHumidity();
         (*pTable)(rowIdx,colIdx++) << pCEBFIPConcrete->GetBetaRH();
         (*pTable)(rowIdx,colIdx++) << pCEBFIPConcrete->GetBetaSc();
         (*pTable)(rowIdx,colIdx++) << hDim.SetValue(pCEBFIPConcrete->GetH());
         (*pTable)(rowIdx,colIdx++) << 1E6*pCEBFIPConcrete->GetEpsilonS();
         (*pTable)(rowIdx,colIdx++) << 1E6*pCEBFIPConcrete->GetNotionalShrinkageCoefficient();

         rowIdx++;
      }
   }

   if (IsStructuralDeck(pBridge->GetDeckType()))
   {
      GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
      EventIndexType castDeckEventIdx = pIBridgeDesc->GetCastDeckEventIndex();
      const auto& castDeckActivity = pIBridgeDesc->GetEventByIndex(castDeckEventIdx)->GetCastDeckActivity();
      IndexType nCastings = castDeckActivity.GetCastingCount();
      for (IndexType castingIdx = 0; castingIdx < nCastings; castingIdx++)
      {
         std::vector<IndexType> vRegions = castDeckActivity.GetRegions(castingIdx);
         IndexType deckCastingRegionIdx = vRegions.front();
         const matConcreteBase* pConcrete = pMaterials->GetDeckConcrete(deckCastingRegionIdx);
         const matCEBFIPConcrete* pCEBFIPConcrete = dynamic_cast<const matCEBFIPConcrete*>(pConcrete);

         ColumnIndexType colIdx = 0;

         CString strTitle(_T("Deck"));
         if (1 < nCastings)
         {
            strTitle += _T(" Regions ");
            auto begin = std::begin(vRegions);
            auto iter = begin;
            auto end = std::end(vRegions);
            for (; iter != end; iter++)
            {
               if (iter != begin)
               {
                  strTitle += _T(", ");
               }
               CString strRegion;
               strRegion.Format(_T("%d"), LABEL_INDEX(*iter));
               strTitle += strRegion;
            }
         }


         (*pTable)(rowIdx, colIdx++) << strTitle;
         (*pTable)(rowIdx, colIdx++) << stress.SetValue(pCEBFIPConcrete->GetFc28());
         (*pTable)(rowIdx, colIdx++) << pConcrete->GetRelativeHumidity();
         (*pTable)(rowIdx, colIdx++) << pCEBFIPConcrete->GetBetaRH();
         (*pTable)(rowIdx, colIdx++) << pCEBFIPConcrete->GetBetaSc();
         (*pTable)(rowIdx, colIdx++) << hDim.SetValue(pCEBFIPConcrete->GetH());
         (*pTable)(rowIdx, colIdx++) << 1E6*pCEBFIPConcrete->GetEpsilonS();
         (*pTable)(rowIdx, colIdx++) << 1E6*pCEBFIPConcrete->GetNotionalShrinkageCoefficient();

         rowIdx++;
      } // next casting stage
   }

#if defined _DEBUG || defined _BETA_VERSION
   GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
   EventIndexType castDeckIndex = pIBridgeDesc->GetCastDeckEventIndex();
   const auto* pEvent = pIBridgeDesc->GetEventByIndex(castDeckIndex);
   const auto& castDeckActivity = pEvent->GetCastDeckActivity();
   IndexType nCastings = castDeckActivity.GetCastingCount();
   IndexType nClosures = nSegments - 1;

   pTable = rptStyleManager::CreateDefaultTable(4 * (nSegments + nClosures + nCastings) + 1, _T("Debugging Table"));
   *pPara << pTable << rptNewLine;

   pTable->SetNumberOfHeaderRows(2);
   pTable->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   pTable->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   rowIdx = 0;
   colIdx = 0;

   pTable->SetRowSpan(rowIdx,colIdx,2);
   (*pTable)(rowIdx,colIdx++) << _T("Interval");
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      pTable->SetColumnSpan(rowIdx,colIdx,4);
      (*pTable)(rowIdx,colIdx) << _T("Segment ") << LABEL_SEGMENT(segIdx);
      (*pTable)(rowIdx+1,colIdx++) << _T("t") << rptNewLine << _T("(days)");
      (*pTable)(rowIdx+1,colIdx++) << Sub2(_T("t"),_T("s")) << rptNewLine << _T("(days)");
      (*pTable)(rowIdx+1,colIdx++) << symbol(DELTA) << Sub2(symbol(epsilon),_T("sh")) << _T("x10") << Super(_T("6"));
      (*pTable)(rowIdx+1,colIdx++) << Sub2(symbol(epsilon),_T("sh")) << _T("x10") << Super(_T("6"));
      if ( segIdx != nSegments-1 )
      {
         pTable->SetColumnSpan(rowIdx,colIdx,4);
         (*pTable)(rowIdx,colIdx) << _T("Closure Joint ") << LABEL_SEGMENT(segIdx);
         (*pTable)(rowIdx+1,colIdx++) << _T("t") << rptNewLine << _T("(days)");
         (*pTable)(rowIdx+1,colIdx++) << Sub2(_T("t"),_T("s")) << rptNewLine << _T("(days)");
         (*pTable)(rowIdx+1,colIdx++) << symbol(DELTA) << Sub2(symbol(epsilon),_T("sh")) << _T("x10") << Super(_T("6"));
         (*pTable)(rowIdx+1,colIdx++) << Sub2(symbol(epsilon),_T("sh")) << _T("x10") << Super(_T("6"));
      }
   }


   for (IndexType castingIdx = 0; castingIdx < nCastings; castingIdx++)
   {
      pTable->SetColumnSpan(rowIdx, colIdx, 4);

      CString strTitle(_T("Deck"));
      if (1 < nCastings)
      {
         std::vector<IndexType> vRegions = castDeckActivity.GetRegions(castingIdx);

         strTitle += _T(" Regions ");
         auto begin = std::begin(vRegions);
         auto iter = begin;
         auto end = std::end(vRegions);
         for (; iter != end; iter++)
         {
            if (iter != begin)
            {
               strTitle += _T(", ");
            }
            CString strRegion;
            strRegion.Format(_T("%d"), LABEL_INDEX(*iter));
            strTitle += strRegion;
         }
      }

      (*pTable)(rowIdx, colIdx) << strTitle;
      (*pTable)(rowIdx + 1, colIdx++) << _T("t") << rptNewLine << _T("(days)");
      (*pTable)(rowIdx + 1, colIdx++) << Sub2(_T("t"), _T("s")) << rptNewLine << _T("(days)");
      (*pTable)(rowIdx + 1, colIdx++) << symbol(DELTA) << Sub2(symbol(epsilon), _T("sh")) << _T("x10") << Super(_T("6"));
      (*pTable)(rowIdx + 1, colIdx++) << Sub2(symbol(epsilon), _T("sh")) << _T("x10") << Super(_T("6"));
   }

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
            const matCEBFIPConcrete* pCEBFIPConcrete = dynamic_cast<const matCEBFIPConcrete*>(pConcrete);
            Float64 cure = pCEBFIPConcrete->GetCureTime();
            (*pTable)(rowIdx,colIdx++) << t;
            (*pTable)(rowIdx,colIdx++) << cure;
            (*pTable)(rowIdx,colIdx++) << 1E6*pMaterials->GetIncrementalSegmentFreeShrinkageStrain(segmentKey,intervalIdx);
            (*pTable)(rowIdx,colIdx++) << 1E6*pMaterials->GetTotalSegmentFreeShrinkageStrain(segmentKey,intervalIdx,pgsTypes::End);
         }
         else
         {
            (*pTable)(rowIdx,colIdx++) << _T("");
            (*pTable)(rowIdx,colIdx++) << _T("");
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
               const matCEBFIPConcrete* pCEBFIPConcrete = dynamic_cast<const matCEBFIPConcrete*>(pConcrete);
               Float64 cure = pCEBFIPConcrete->GetCureTime();
               (*pTable)(rowIdx,colIdx++) << t;
               (*pTable)(rowIdx,colIdx++) << cure;
               (*pTable)(rowIdx,colIdx++) << 1E6*pMaterials->GetIncrementalClosureJointFreeShrinkageStrain(closureKey,intervalIdx);
               (*pTable)(rowIdx,colIdx++) << 1E6*pMaterials->GetTotalClosureJointFreeShrinkageStrain(closureKey,intervalIdx,pgsTypes::End);
            }
            else
            {
               (*pTable)(rowIdx,colIdx++) << _T("");
               (*pTable)(rowIdx,colIdx++) << _T("");
               (*pTable)(rowIdx,colIdx++) << _T("");
               (*pTable)(rowIdx,colIdx++) << _T("");
            }
         }
      }

      for (IndexType castingIdx = 0; castingIdx < nCastings; castingIdx++)
      {
         std::vector<IndexType> vRegions = castDeckActivity.GetRegions(castingIdx);
         IndexType deckCastingRegionIdx = vRegions.front();
         IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval(deckCastingRegionIdx);
         if ( compositeDeckIntervalIdx <= intervalIdx )
         {
            Float64 t  = pMaterials->GetDeckConcreteAge(deckCastingRegionIdx,intervalIdx,pgsTypes::End);
            const matConcreteBase* pConcrete = pMaterials->GetDeckConcrete(deckCastingRegionIdx);
            const matCEBFIPConcrete* pCEBFIPConcrete = dynamic_cast<const matCEBFIPConcrete*>(pConcrete);
            Float64 cure = pCEBFIPConcrete->GetCureTime();
            (*pTable)(rowIdx,colIdx++) << t;
            (*pTable)(rowIdx,colIdx++) << cure;
            (*pTable)(rowIdx,colIdx++) << 1E6*pMaterials->GetIncrementalDeckFreeShrinkageStrain(deckCastingRegionIdx,intervalIdx);
            (*pTable)(rowIdx,colIdx++) << 1E6*pMaterials->GetTotalDeckFreeShrinkageStrain(deckCastingRegionIdx,intervalIdx,pgsTypes::End);
         }
         else
         {
            (*pTable)(rowIdx,colIdx++) << _T("");
            (*pTable)(rowIdx,colIdx++) << _T("");
            (*pTable)(rowIdx,colIdx++) << _T("");
            (*pTable)(rowIdx,colIdx++) << _T("");
         }
      }
   }
#endif //#if defined _DEBUG || defined _BETA_VERSION


   return pChapter;
}

CChapterBuilder* CCEBFIPShrinkageStrainChapterBuilder::Clone() const
{
   return new CCEBFIPShrinkageStrainChapterBuilder;
}
