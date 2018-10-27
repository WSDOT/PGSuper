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
#include <Reporting\TimeStepCamberChapterBuilder.h>

#include <IFace\Intervals.h>
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\AnalysisResults.h>

#include <iterator>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CTimeStepCamberChapterBuilder::CTimeStepCamberChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

LPCTSTR CTimeStepCamberChapterBuilder::GetName() const
{
   return TEXT("Camber Details");
}

rptChapter* CTimeStepCamberChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CGirderReportSpecification* pGirderRptSpec = dynamic_cast<CGirderReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pGirderRptSpec->GetBroker(&pBroker);

   const CGirderKey& girderKey(pGirderRptSpec->GetGirderKey());

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);
   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   *pPara << CreateStorageDeflectionTable(pBroker,girderKey)           << rptNewLine;
   *pPara << CreateAfterErectionDeflectionTable(pBroker,girderKey)     << rptNewLine;

   GET_IFACE2(pBroker, IBridge, pBridge);
   if (pBridge->GetDeckType() != pgsTypes::sdtNone)
   {
      *pPara << CreateBeforeSlabCastingDeflectionTable(pBroker, girderKey) << rptNewLine;

      pPara = new rptParagraph(rptStyleManager::GetFootnoteStyle());
      *pChapter << pPara;
      *pPara << Sub2(symbol(DELTA), _T("D")) << _T(" = sum of the individual deflections given in this table") << rptNewLine;

      pPara = new rptParagraph;
      *pChapter << pPara;
   }

   *pPara << CreateScreedCamberDeflectionTable(pBroker,girderKey)      << rptNewLine;
   pPara = new rptParagraph(rptStyleManager::GetFootnoteStyle());
   *pChapter << pPara;
   *pPara << Sub2(symbol(DELTA), _T("C")) << _T(" = -1 times the sum of the individual deflections given in this table") << rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;
   *pPara << CreateExcessCamberTable(pBroker,girderKey)                << rptNewLine;
   pPara = new rptParagraph(rptStyleManager::GetFootnoteStyle());
   *pChapter << pPara;
   *pPara << Sub2(symbol(DELTA), _T("Excess")) << _T(" = ") << Sub2(symbol(DELTA), _T("D")) << _T(" - ") << Sub2(symbol(DELTA), _T("C")) << rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;
   *pPara << CreateFinalDeflectionTable(pBroker,girderKey)             << rptNewLine;
   pPara = new rptParagraph(rptStyleManager::GetFootnoteStyle());
   *pChapter << pPara;
   *pPara << symbol(DELTA) << _T(" = sum of the individual deflections given in this table") << rptNewLine;

   return pChapter;
}

CChapterBuilder* CTimeStepCamberChapterBuilder::Clone() const
{
   return new CTimeStepCamberChapterBuilder;
}

rptRcTable* CTimeStepCamberChapterBuilder::CreateStorageDeflectionTable(IBroker* pBroker,const CGirderKey& girderKey) const
{
   GET_IFACE2(pBroker,IIntervals,pIntervals);
   GET_IFACE2(pBroker,IBridge,pBridge);

   ColumnIndexType nCols = 3;
   rptRcTable* pLayoutTable = rptStyleManager::CreateDefaultTable(nCols,_T("Deflections during Storage"));
   (*pLayoutTable)(0,0) << _T("Prestress Release");
   (*pLayoutTable)(0,1) << _T("Begin Storage");
   (*pLayoutTable)(0,2) << _T("End Storage");

   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);

   RowIndexType rowIdx = 1;
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++, rowIdx++ )
   {
      CSegmentKey segmentKey(girderKey,segIdx);

      IntervalIndexType releaseIntervalIdx  = pIntervals->GetPrestressReleaseInterval(segmentKey);
      IntervalIndexType storageIntervalIdx  = pIntervals->GetStorageInterval(segmentKey);
      IntervalIndexType haulingIntervalIdx  = pIntervals->GetHaulSegmentInterval(segmentKey);

      if ( 1 < nSegments )
      {
         pLayoutTable->SetColumnSpan(rowIdx,0,nCols);
         (*pLayoutTable)(rowIdx++,0) << _T("Segment ") << LABEL_SEGMENT(segIdx);
      }

      ColumnIndexType colIdx = 0;

      rptRcTable* pTable = CreateTable(pBroker,segmentKey,releaseIntervalIdx);
      (*pLayoutTable)(rowIdx,colIdx++) << pTable;

      pTable = CreateTable(pBroker,segmentKey,storageIntervalIdx);
      (*pLayoutTable)(rowIdx,colIdx++) << pTable;

      pTable = CreateTable(pBroker,segmentKey,haulingIntervalIdx-1); // storage ends the interval before hauling
      (*pLayoutTable)(rowIdx,colIdx++) << pTable;
   }

   return pLayoutTable;
}

rptRcTable* CTimeStepCamberChapterBuilder::CreateAfterErectionDeflectionTable(IBroker* pBroker,const CGirderKey& girderKey) const
{
   GET_IFACE2(pBroker,IIntervals,pIntervals);
   GET_IFACE2(pBroker,IBridge,pBridge);

   IntervalIndexType firstErectionIntervalIdx = pIntervals->GetFirstSegmentErectionInterval(girderKey);
   IntervalIndexType lastErectionIntervalIdx  = pIntervals->GetLastSegmentErectionInterval(girderKey);

   // not all intervals in the range first-last are segment erection intervals
   // create a vector of just the intervals when segments are erected
   std::vector<IntervalIndexType> vIntervals;
   vIntervals.reserve(lastErectionIntervalIdx-firstErectionIntervalIdx+1);
   for ( IntervalIndexType intervalIdx = firstErectionIntervalIdx; intervalIdx <= lastErectionIntervalIdx; intervalIdx++ )
   {
      if ( pIntervals->IsSegmentErectionInterval(girderKey,intervalIdx) )
      {
         vIntervals.push_back(intervalIdx);
      }
   }

   ColumnIndexType nCols = vIntervals.size();
   rptRcTable* pLayoutTable = rptStyleManager::CreateDefaultTable(nCols,_T("Deflections after Erection"));

   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);

   ColumnIndexType layoutTableColIdx = 0;
   std::vector<IntervalIndexType>::iterator intervalIter(vIntervals.begin());
   std::vector<IntervalIndexType>::iterator intervalIterEnd(vIntervals.end());
   for ( ; intervalIter != intervalIterEnd; intervalIter++, layoutTableColIdx++ )
   {
      RowIndexType rowIdx = 0;
      IntervalIndexType intervalIdx = *intervalIter;
      (*pLayoutTable)(rowIdx++,layoutTableColIdx) << _T("Interval ") << LABEL_INTERVAL(intervalIdx) << rptNewLine << pIntervals->GetDescription(intervalIdx);

      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++, rowIdx++ )
      {
         CSegmentKey segmentKey(girderKey,segIdx);

         if ( 1 < nSegments )
         {
            if ( layoutTableColIdx == 0 )
            {
               // put segment name in segment label row
               pLayoutTable->SetColumnSpan(rowIdx,0,nCols);
               (*pLayoutTable)(rowIdx++,layoutTableColIdx) << _T("Segment ") << LABEL_SEGMENT(segIdx);
            }
            else
            {
               // skip over the segment label row
               rowIdx++;
            }
         }

         IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
         if ( erectionIntervalIdx <= intervalIdx )
         {
            rptRcTable* pTable = CreateTable(pBroker,segmentKey,intervalIdx);
            (*pLayoutTable)(rowIdx,layoutTableColIdx) << pTable;
         }
         else
         {
            (*pLayoutTable)(rowIdx,layoutTableColIdx) << _T("Not erected");
         }
      }
   }

   return pLayoutTable;
}

rptRcTable* CTimeStepCamberChapterBuilder::CreateTable(IBroker* pBroker, const CSegmentKey& segmentKey, IntervalIndexType intervalIdx) const
{
   ASSERT_SEGMENT_KEY(segmentKey);

   GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);
   GET_IFACE2(pBroker, IProductForces2, pForces);
   GET_IFACE2_NOCHECK(pBroker, IBridge, pBridge);

   GET_IFACE2(pBroker, IProductForces, pProduct);
   pgsTypes::BridgeAnalysisType bat = pProduct->GetBridgeAnalysisType(pgsTypes::Minimize);

   GET_IFACE2(pBroker, IIntervals, pIntervals);
   IntervalIndexType storageIntervalIdx = pIntervals->GetStorageInterval(segmentKey);
   IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
   PoiAttributeType poiReference = 0;
   if (intervalIdx < storageIntervalIdx)
   {
      poiReference = POI_RELEASED_SEGMENT;
   }
   else if (intervalIdx < erectionIntervalIdx)
   {
      poiReference = POI_STORAGE_SEGMENT;
   }
   else
   {
      poiReference = POI_ERECTED_SEGMENT;
   }


   GET_IFACE2(pBroker, IPointOfInterest, pIPoi);
   PoiList vPoi;
   pIPoi->GetPointsOfInterest(segmentKey, poiReference, &vPoi);

   GET_IFACE2(pBroker, IUserDefinedLoads, pUserLoads);

   std::vector<pgsTypes::ProductForceType> vProductForces;
   vProductForces.push_back(pgsTypes::pftGirder);
   vProductForces.push_back(pgsTypes::pftPretension);
   //vProductForces.push_back(pgsTypes::pftConstruction);
   //vProductForces.push_back(pgsTypes::pftSlab);
   //vProductForces.push_back(pgsTypes::pftSlabPad);
   //vProductForces.push_back(pgsTypes::pftSlabPanel);
   if (erectionIntervalIdx <= intervalIdx)
   {
      vProductForces.push_back(pgsTypes::pftDiaphragm);
   }
   //vProductForces.push_back(pgsTypes::pftOverlay);
   //vProductForces.push_back(pgsTypes::pftSidewalk);
   //vProductForces.push_back(pgsTypes::pftTrafficBarrier);
   if (pUserLoads->DoUserLoadsExist(segmentKey, 0, intervalIdx, IUserDefinedLoads::userDC))
   {
      vProductForces.push_back(pgsTypes::pftUserDC);
   }

   if (pUserLoads->DoUserLoadsExist(segmentKey, 0, intervalIdx, IUserDefinedLoads::userDW))
   {
      vProductForces.push_back(pgsTypes::pftUserDW);
   }

   if (pUserLoads->DoUserLoadsExist(segmentKey, 0, intervalIdx, IUserDefinedLoads::userLL_IM))
   {
      vProductForces.push_back(pgsTypes::pftUserLLIM);
   }
   //vProductForces.push_back(pgsTypes::pftShearKey);
   //vProductForces.push_back(pgsTypes::pftSecondaryEffects);
   //vProductForces.push_back(pgsTypes::pftPostTensioning);

   if (storageIntervalIdx < intervalIdx)
   {
      vProductForces.push_back(pgsTypes::pftCreep);
      vProductForces.push_back(pgsTypes::pftShrinkage);
      vProductForces.push_back(pgsTypes::pftRelaxation);
   }

   INIT_UV_PROTOTYPE(rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false);
   INIT_UV_PROTOTYPE(rptLengthUnitValue, deflection, pDisplayUnits->GetDeflectionUnit(), false);

   ColumnIndexType nCols = vProductForces.size() + 1;
   if (erectionIntervalIdx <= intervalIdx)
   {
      nCols++;
   }

   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(nCols);

   std::vector<Float64>* pResults = new std::vector<Float64>[vProductForces.size()];

   GET_IFACE2(pBroker,IProductLoads,pProductLoads);

   ColumnIndexType col = 0;
   (*pTable)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   std::vector<pgsTypes::ProductForceType>::iterator pfIter(vProductForces.begin());
   std::vector<pgsTypes::ProductForceType>::iterator pfIterEnd(vProductForces.end());
   int i = 0;
   for ( ; pfIter != pfIterEnd; pfIter++, i++ )
   {
      pgsTypes::ProductForceType pfType = *pfIter;
      LPCTSTR strName = pProductLoads->GetProductLoadName(pfType);

      (*pTable)(0,col++) << COLHDR(Sub2(symbol(DELTA),strName),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

      pResults[i] = pForces->GetDeflection(intervalIdx,pfType,vPoi,bat,rtCumulative,false);
   }

   if (erectionIntervalIdx <= intervalIdx)
   {
      (*pTable)(0, col++) << COLHDR(_T("Elevation") << rptNewLine << _T("Adjustment"), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   }


   RowIndexType row = pTable->GetNumberOfHeaderRows();

   i = 0;
   for (const pgsPointOfInterest& poi : vPoi)
   {
      col = 0;
      (*pTable)(row,col++) << location.SetValue( poiReference, poi );

      std::vector<pgsTypes::ProductForceType>::iterator pfIter(vProductForces.begin());
      std::vector<pgsTypes::ProductForceType>::iterator pfIterEnd(vProductForces.end());
      int k = 0;
      for ( ; pfIter != pfIterEnd; pfIter++, k++ )
      {
         (*pTable)(row,col++) << deflection.SetValue( pResults[k].at(i) );
      }
      i++;

      if (erectionIntervalIdx <= intervalIdx)
      {
         Float64 elev_adj = pBridge->GetElevationAdjustment(intervalIdx, poi);
         (*pTable)(row, col++) << deflection.SetValue(elev_adj);
      }

      row++;
   }

   delete[] pResults;

   return pTable;
}

rptRcTable* CTimeStepCamberChapterBuilder::CreateBeforeSlabCastingDeflectionTable(IBroker* pBroker,const CGirderKey& girderKey) const
{
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker,IProductLoads,pProductLoads);
   GET_IFACE2(pBroker,IProductForces2,pForces);

   GET_IFACE2(pBroker,IProductForces,pProduct);
   pgsTypes::BridgeAnalysisType bat = pProduct->GetBridgeAnalysisType(pgsTypes::Minimize);

   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 end_size = pBridge->GetSegmentStartEndDistance(CSegmentKey(girderKey,0));

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval();
   IntervalIndexType firstTendonStressingIntervalIdx = pIntervals->GetFirstTendonStressingInterval(girderKey);

   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);
   PoiList vPoi;
   pIPoi->GetPointsOfInterest(CSegmentKey(girderKey, ALL_SEGMENTS), POI_ERECTED_SEGMENT, &vPoi);

   INIT_UV_PROTOTYPE( rptPointOfInterest, location,     pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, deflection,   pDisplayUnits->GetDeflectionUnit(), false );

   location.IncludeSpanAndGirder(true);

   GET_IFACE2(pBroker,IUserDefinedLoads,pUserLoads);
   GET_IFACE2(pBroker,IUserDefinedLoadData,pUserLoadData);

   std::vector<pgsTypes::ProductForceType> vProductForces;
   vProductForces.push_back(pgsTypes::pftGirder);
   if ( !IsZero(pUserLoadData->GetConstructionLoad()) )
   {
      vProductForces.push_back(pgsTypes::pftConstruction);
   }
   //vProductForces.push_back(pgsTypes::pftSlab);
   //vProductForces.push_back(pgsTypes::pftSlabPad);
   //vProductForces.push_back(pgsTypes::pftSlabPanel);
   vProductForces.push_back(pgsTypes::pftDiaphragm); 
   //vProductForces.push_back(pgsTypes::pftOverlay);
   //vProductForces.push_back(pgsTypes::pftSidewalk);
   //vProductForces.push_back(pgsTypes::pftTrafficBarrier);
   if ( pUserLoads->DoUserLoadsExist(girderKey,0,castDeckIntervalIdx-1,IUserDefinedLoads::userDC) )
   {
      vProductForces.push_back(pgsTypes::pftUserDC);
   }

   if ( pUserLoads->DoUserLoadsExist(girderKey,0,castDeckIntervalIdx-1,IUserDefinedLoads::userDW) )
   {
      vProductForces.push_back(pgsTypes::pftUserDW);
   }

   if ( pUserLoads->DoUserLoadsExist(girderKey,0,castDeckIntervalIdx-1,IUserDefinedLoads::userLL_IM) )
   {
      vProductForces.push_back(pgsTypes::pftUserLLIM);
   }
   //vProductForces.push_back(pgsTypes::pftShearKey);

   vProductForces.push_back(pgsTypes::pftPretension);

   if ( firstTendonStressingIntervalIdx < castDeckIntervalIdx )
   {
      vProductForces.push_back(pgsTypes::pftSecondaryEffects);
      vProductForces.push_back(pgsTypes::pftPostTensioning);
   }

   vProductForces.push_back(pgsTypes::pftCreep);
   vProductForces.push_back(pgsTypes::pftShrinkage);
   vProductForces.push_back(pgsTypes::pftRelaxation);

   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(vProductForces.size()+3,_T("Deflections immediately prior to deck casting"));

   std::vector<Float64>* pResults = new std::vector<Float64>[vProductForces.size()];

   ColumnIndexType col = 0;
   (*pTable)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   std::vector<pgsTypes::ProductForceType>::iterator pfIter(vProductForces.begin());
   std::vector<pgsTypes::ProductForceType>::iterator pfIterEnd(vProductForces.end());
   int i = 0;
   for ( ; pfIter != pfIterEnd; pfIter++, i++ )
   {
      pgsTypes::ProductForceType pfType = *pfIter;
      LPCTSTR strName = pProductLoads->GetProductLoadName(pfType);

      (*pTable)(0,col++) << COLHDR(Sub2(symbol(DELTA),strName),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

      // NOTE: Minus one because we want the interval immediately prior to deck placement
      // This is what the "D" dimension is.
      pResults[i] = pForces->GetDeflection(castDeckIntervalIdx-1,pfType,vPoi,bat,rtCumulative,false);
   }
   (*pTable)(0, col++) << COLHDR(_T("Elevation") << rptNewLine << _T("Adjustment"), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   (*pTable)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("D")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

   RowIndexType row = pTable->GetNumberOfHeaderRows();

#if defined _DEBUG
   GET_IFACE2(pBroker,ILimitStateForces2,pLimitStateForces);
   std::vector<Float64> vDmin,vDmax;
   pLimitStateForces->GetDeflection(castDeckIntervalIdx-1,pgsTypes::ServiceI,vPoi,bat,true,false,true,true,&vDmin,&vDmax);
#endif

   i = 0;
   for (const pgsPointOfInterest& poi : vPoi)
   {
      col = 0;

      (*pTable)(row,col++) << location.SetValue( POI_ERECTED_SEGMENT, poi );

      Float64 D = 0;
      std::vector<pgsTypes::ProductForceType>::iterator pfIter(vProductForces.begin());
      std::vector<pgsTypes::ProductForceType>::iterator pfIterEnd(vProductForces.end());
      int k = 0;
      for ( ; pfIter != pfIterEnd; pfIter++, k++ )
      {
         Float64 d = pResults[k].at(i);
         (*pTable)(row,col++) << deflection.SetValue( d );
         D += d;
      }

      Float64 elev_adj = pBridge->GetElevationAdjustment(castDeckIntervalIdx - 1, poi);
      (*pTable)(row, col++) << deflection.SetValue(elev_adj);
      D += elev_adj;
      
      ATLASSERT(IsEqual(vDmin[i],D));
      ATLASSERT(IsEqual(vDmax[i],D));

      (*pTable)(row,col++) << deflection.SetValue(D);

      i++;
      row++;
   }

   delete[] pResults;

   return pTable;
}

rptRcTable* CTimeStepCamberChapterBuilder::CreateScreedCamberDeflectionTable(IBroker* pBroker,const CGirderKey& girderKey) const
{
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker,IProductLoads,pProductLoads);
   GET_IFACE2(pBroker,IProductForces2,pForces);

   GET_IFACE2(pBroker,IProductForces,pProduct);
   pgsTypes::BridgeAnalysisType bat = pProduct->GetBridgeAnalysisType(pgsTypes::Minimize);

   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 end_size = pBridge->GetSegmentStartEndDistance(CSegmentKey(girderKey,0));

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType intervalIdx;
   if (pBridge->GetDeckType() == pgsTypes::sdtNone)
   {
      intervalIdx = pIntervals->GetIntervalCount() - 1;
   }
   else
   {
      IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval();
      intervalIdx = castDeckIntervalIdx - 1;
   }
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);
   PoiList vPoi;
   pIPoi->GetPointsOfInterest(CSegmentKey(girderKey, ALL_SEGMENTS), POI_ERECTED_SEGMENT, &vPoi);

   INIT_UV_PROTOTYPE( rptPointOfInterest, location,     pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, deflection,   pDisplayUnits->GetDeflectionUnit(), false );

   location.IncludeSpanAndGirder(true);

   GET_IFACE2(pBroker,IUserDefinedLoads,pUserLoads);
   GET_IFACE2(pBroker,IUserDefinedLoadData,pUserLoadData);
   GET_IFACE2(pBroker,ITendonGeometry,pTendonGeom);

   std::vector<pgsTypes::ProductForceType> vProductForces;
   vProductForces.push_back(pgsTypes::pftGirder);
   vProductForces.push_back(pgsTypes::pftPretension);

   if ( !IsZero(pUserLoadData->GetConstructionLoad()) )
   {
      vProductForces.push_back(pgsTypes::pftConstruction);
   }

   if ( pBridge->GetDeckType() != pgsTypes::sdtNone )
   {
      vProductForces.push_back(pgsTypes::pftSlab);
      vProductForces.push_back(pgsTypes::pftSlabPad);
   }

   if ( pBridge->GetDeckType() == pgsTypes::sdtCompositeSIP )
   {
      vProductForces.push_back(pgsTypes::pftSlabPanel);
   }

   vProductForces.push_back(pgsTypes::pftDiaphragm); 

   if ( pIntervals->GetOverlayInterval() <= liveLoadIntervalIdx )
   {
      vProductForces.push_back(pgsTypes::pftOverlay);
   }

   if ( pProductLoads->HasSidewalkLoad(girderKey) )
   {
      vProductForces.push_back(pgsTypes::pftSidewalk);
   }

   vProductForces.push_back(pgsTypes::pftTrafficBarrier);

   if ( pUserLoads->DoUserLoadsExist(girderKey,0,liveLoadIntervalIdx,IUserDefinedLoads::userDC) )
   {
      vProductForces.push_back(pgsTypes::pftUserDC);
   }

   if ( pUserLoads->DoUserLoadsExist(girderKey,0,liveLoadIntervalIdx,IUserDefinedLoads::userDW) )
   {
      vProductForces.push_back(pgsTypes::pftUserDW);
   }

   if ( pUserLoads->DoUserLoadsExist(girderKey,0,liveLoadIntervalIdx,IUserDefinedLoads::userLL_IM) )
   {
      vProductForces.push_back(pgsTypes::pftUserLLIM);
   }

   if ( pProductLoads->HasShearKeyLoad(girderKey) )
   {
      vProductForces.push_back(pgsTypes::pftShearKey);
   }

   if ( 0 < pTendonGeom->GetDuctCount(girderKey) )
   {
      vProductForces.push_back(pgsTypes::pftPostTensioning);
      vProductForces.push_back(pgsTypes::pftSecondaryEffects);
   }

   vProductForces.push_back(pgsTypes::pftCreep);
   vProductForces.push_back(pgsTypes::pftShrinkage);
   vProductForces.push_back(pgsTypes::pftRelaxation);

   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(vProductForces.size()+2,_T("Deflections from deck casting to service (Screed Camber)"));

   std::vector<Float64>* pResults1 = new std::vector<Float64>[vProductForces.size()];
   std::vector<Float64>* pResults2 = new std::vector<Float64>[vProductForces.size()];

   ColumnIndexType col = 0;
   (*pTable)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   std::vector<pgsTypes::ProductForceType>::iterator pfIter(vProductForces.begin());
   std::vector<pgsTypes::ProductForceType>::iterator pfIterEnd(vProductForces.end());
   int i = 0;
   for ( ; pfIter != pfIterEnd; pfIter++, i++ )
   {
      pgsTypes::ProductForceType pfType = *pfIter;
      LPCTSTR strName = pProductLoads->GetProductLoadName(pfType);

      (*pTable)(0,col++) << COLHDR(Sub2(symbol(DELTA),strName),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

      pResults1[i] = pForces->GetDeflection(intervalIdx,pfType,vPoi,bat,rtCumulative,true);
      pResults2[i] = pForces->GetDeflection(liveLoadIntervalIdx,  pfType,vPoi,bat,rtCumulative,true);
   }
  (*pTable)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("C")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

   RowIndexType row = pTable->GetNumberOfHeaderRows();


#if defined _DEBUG
   GET_IFACE2(pBroker,ILimitStateForces2,pLimitStateForces);
   std::vector<Float64> vDmin1,vDmax1;
   pLimitStateForces->GetDeflection(intervalIdx,pgsTypes::ServiceI,vPoi,bat,true,false,true,true,&vDmin1,&vDmax1);
   std::vector<Float64> vDmin2,vDmax2;
   pLimitStateForces->GetDeflection(liveLoadIntervalIdx,pgsTypes::ServiceI,vPoi,bat,true,false,true,true,&vDmin2,&vDmax2);
   std::vector<Float64> vC;
   std::transform(vDmin2.cbegin(),vDmin2.cend(),vDmin1.cbegin(),std::back_inserter(vC),[](const auto& a, const auto& b) {return a - b;});
#endif

   i = 0;
   for (const pgsPointOfInterest& poi : vPoi)
   {
      col = 0;

      (*pTable)(row,col++) << location.SetValue( POI_ERECTED_SEGMENT, poi );

      Float64 C = 0;
      std::vector<pgsTypes::ProductForceType>::iterator pfIter(vProductForces.begin());
      std::vector<pgsTypes::ProductForceType>::iterator pfIterEnd(vProductForces.end());
      int k = 0;
      for ( ; pfIter != pfIterEnd; pfIter++, k++ )
      {
         Float64 d1 = pResults1[k].at(i);
         Float64 d2 = pResults2[k].at(i);
         Float64 c = d2-d1;
         (*pTable)(row,col++) << deflection.SetValue( c );
         C += c;
      }

      ATLASSERT(IsEqual(C,vC[i]));

      (*pTable)(row,col++) << deflection.SetValue(-C); // minus because screed camber is equal and opposite the deflections

      i++;
      row++;
   }

   delete[] pResults1;
   delete[] pResults2;

   return pTable;
}

rptRcTable* CTimeStepCamberChapterBuilder::CreateExcessCamberTable(IBroker* pBroker,const CGirderKey& girderKey) const
{
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   GET_IFACE2(pBroker,IProductForces,pProduct);
   pgsTypes::BridgeAnalysisType bat = pProduct->GetBridgeAnalysisType(pgsTypes::Minimize);

   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 end_size = pBridge->GetSegmentStartEndDistance(CSegmentKey(girderKey,0));

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType intervalIdx;
   if (pBridge->GetDeckType() == pgsTypes::sdtNone)
   {
      intervalIdx = pIntervals->GetIntervalCount() - 1;
   }
   else
   {
      IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval();
      intervalIdx = castDeckIntervalIdx - 1;
   }
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);
   PoiList vPoi;
   pIPoi->GetPointsOfInterest(CSegmentKey(girderKey, ALL_SEGMENTS), POI_ERECTED_SEGMENT, &vPoi);

   INIT_UV_PROTOTYPE( rptPointOfInterest, location,     pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, deflection,   pDisplayUnits->GetDeflectionUnit(), false );

   location.IncludeSpanAndGirder(true);

   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(4,_T("Excess Camber"));

   ColumnIndexType col = 0;
   (*pTable)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*pTable)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("D")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*pTable)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("C")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*pTable)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("Excess")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

   RowIndexType row = pTable->GetNumberOfHeaderRows();

   GET_IFACE2(pBroker,ILimitStateForces2,pLimitStateForces);
   
   std::vector<Float64> vDmin,vDmax;
   pLimitStateForces->GetDeflection(intervalIdx,pgsTypes::ServiceI,vPoi,bat,true,false,true,true,&vDmin,&vDmax);
   
   std::vector<Float64> vExcessMin,vExcessMax;
   pLimitStateForces->GetDeflection(liveLoadIntervalIdx,pgsTypes::ServiceI,vPoi,bat,true,false,true,true,&vExcessMin,&vExcessMax);
   
   std::vector<Float64> vC;
   std::transform(vExcessMin.cbegin(),vExcessMin.cend(),vDmin.cbegin(),std::back_inserter(vC),[](const auto& a, const auto& b) {return a - b;});

   int i = 0;
   for (const pgsPointOfInterest& poi : vPoi)
   {
      col = 0;

      (*pTable)(row,col++) << location.SetValue( POI_ERECTED_SEGMENT, poi );
      (*pTable)(row,col++) << deflection.SetValue( vDmin[i] );
      (*pTable)(row,col++) << deflection.SetValue( -vC[i] ); // minus because screed camber is equal and opposite the deflections
      (*pTable)(row,col++) << deflection.SetValue( vExcessMin[i] );

      i++;
      row++;
   }

   return pTable;
}

rptRcTable* CTimeStepCamberChapterBuilder::CreateFinalDeflectionTable(IBroker* pBroker,const CGirderKey& girderKey) const
{
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker,IProductLoads,pProductLoads);
   GET_IFACE2(pBroker,IProductForces2,pForces);

   GET_IFACE2(pBroker,IProductForces,pProduct);
   pgsTypes::BridgeAnalysisType bat = pProduct->GetBridgeAnalysisType(pgsTypes::Minimize);

   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 end_size = pBridge->GetSegmentStartEndDistance(CSegmentKey(girderKey,0));

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType lastIntervalIdx = pIntervals->GetIntervalCount()-1;

   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);
   PoiList vPoi;
   pIPoi->GetPointsOfInterest(CSegmentKey(girderKey, ALL_SEGMENTS), POI_SPAN, &vPoi);

   INIT_UV_PROTOTYPE( rptPointOfInterest, location,     pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, deflection,   pDisplayUnits->GetDeflectionUnit(), false );

   location.IncludeSpanAndGirder(true);


   GET_IFACE2(pBroker,IUserDefinedLoads,pUserLoads);
   GET_IFACE2(pBroker,IUserDefinedLoadData,pUserLoadData);
   GET_IFACE2(pBroker,ITendonGeometry,pTendonGeom);

   std::vector<pgsTypes::ProductForceType> vProductForces;
   vProductForces.push_back(pgsTypes::pftGirder);

   if ( !IsZero(pUserLoadData->GetConstructionLoad()) )
   {
      vProductForces.push_back(pgsTypes::pftConstruction);
   }

   if ( pBridge->GetDeckType() != pgsTypes::sdtNone )
   {
      vProductForces.push_back(pgsTypes::pftSlab);
      vProductForces.push_back(pgsTypes::pftSlabPad);
   }

   if ( pBridge->GetDeckType() == pgsTypes::sdtCompositeSIP )
   {
      vProductForces.push_back(pgsTypes::pftSlabPanel);
   }

   vProductForces.push_back(pgsTypes::pftDiaphragm); 

   if ( pIntervals->GetOverlayInterval() != INVALID_INDEX )
   {
      vProductForces.push_back(pgsTypes::pftOverlay);
   }

   if ( pProductLoads->HasSidewalkLoad(girderKey) )
   {
      vProductForces.push_back(pgsTypes::pftSidewalk);
   }

   vProductForces.push_back(pgsTypes::pftTrafficBarrier);

   if ( pUserLoads->DoUserLoadsExist(girderKey,IUserDefinedLoads::userDC) )
   {
      vProductForces.push_back(pgsTypes::pftUserDC);
   }

   if ( pUserLoads->DoUserLoadsExist(girderKey,IUserDefinedLoads::userDW) )
   {
      vProductForces.push_back(pgsTypes::pftUserDW);
   }

   if ( pUserLoads->DoUserLoadsExist(girderKey,IUserDefinedLoads::userLL_IM) )
   {
      vProductForces.push_back(pgsTypes::pftUserLLIM);
   }

   if ( pProductLoads->HasShearKeyLoad(girderKey) )
   {
      vProductForces.push_back(pgsTypes::pftShearKey);
   }

   vProductForces.push_back(pgsTypes::pftPretension);

   if ( 0 < pTendonGeom->GetDuctCount(girderKey) )
   {
      vProductForces.push_back(pgsTypes::pftPostTensioning);
      vProductForces.push_back(pgsTypes::pftSecondaryEffects);
   }

   vProductForces.push_back(pgsTypes::pftCreep);
   vProductForces.push_back(pgsTypes::pftShrinkage);
   vProductForces.push_back(pgsTypes::pftRelaxation);

   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(vProductForces.size()+3,_T("Final Deflections"));

   std::vector<Float64>* pResults = new std::vector<Float64>[vProductForces.size()];

   ColumnIndexType col = 0;
   (*pTable)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   std::vector<pgsTypes::ProductForceType>::iterator pfIter(vProductForces.begin());
   std::vector<pgsTypes::ProductForceType>::iterator pfIterEnd(vProductForces.end());
   int i = 0;
   for ( ; pfIter != pfIterEnd; pfIter++, i++ )
   {
      pgsTypes::ProductForceType pfType = *pfIter;
      LPCTSTR strName = pProductLoads->GetProductLoadName(pfType);

      (*pTable)(0,col++) << COLHDR(Sub2(symbol(DELTA),strName),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
      pResults[i] = pForces->GetDeflection(lastIntervalIdx,pfType,vPoi,bat,rtCumulative,false);
   }
   (*pTable)(0, col++) << COLHDR(_T("Elevation") << rptNewLine << _T("Adjustment"), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   (*pTable)(0, col++) << COLHDR(symbol(DELTA), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());

   RowIndexType row = pTable->GetNumberOfHeaderRows();

   GET_IFACE2(pBroker,ILimitStateForces2,pLimitStateForces);
   std::vector<Float64> vDmin,vDmax;
   pLimitStateForces->GetDeflection(lastIntervalIdx,pgsTypes::ServiceI,vPoi,bat,true,false,true,true,&vDmin,&vDmax);

   i = 0;
   for (const pgsPointOfInterest& poi : vPoi)
   {
      col = 0;

      (*pTable)(row,col++) << location.SetValue( POI_SPAN, poi );

      std::vector<pgsTypes::ProductForceType>::iterator pfIter(vProductForces.begin());
      std::vector<pgsTypes::ProductForceType>::iterator pfIterEnd(vProductForces.end());
      int k = 0;
      for ( ; pfIter != pfIterEnd; pfIter++, k++ )
      {
         Float64 d = pResults[k].at(i);
         (*pTable)(row,col++) << deflection.SetValue( d );
      }

      Float64 elev_adj = pBridge->GetElevationAdjustment(lastIntervalIdx, poi);
      (*pTable)(row, col++) << deflection.SetValue(elev_adj);

      
      (*pTable)(row,col++) << deflection.SetValue(vDmin[i]);

      i++;
      row++;
   }

   delete[] pResults;

   return pTable;
}

