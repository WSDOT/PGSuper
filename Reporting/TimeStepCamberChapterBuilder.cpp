///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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

   GET_IFACE2(pBroker, IIntervals, pIntervals);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

   const CGirderKey& girderKey(pGirderRptSpec->GetGirderKey());

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);
   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   *pPara << CreateStorageDeflectionTable(pBroker, girderKey) << rptNewLine;
   *pPara << CreateHandlingDeflectionTable(pBroker, girderKey) << rptNewLine;
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
   *pPara << Sub2(symbol(DELTA), _T("service")) << _T(" = deflection at service (open to traffic), Interval ") << LABEL_INTERVAL(liveLoadIntervalIdx) << rptNewLine;
   *pPara << Sub2(symbol(DELTA), _T("final")) << _T(" = final deflection = sum of the individual deflections given in this table") << rptNewLine;

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
   
   ColumnIndexType colIdx = 0;
   (*pLayoutTable)(0, colIdx++) << _T("Prestress Release");
   (*pLayoutTable)(0, colIdx++) << _T("Begin Storage");
   (*pLayoutTable)(0, colIdx++) << _T("End Storage");

   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);

   RowIndexType rowIdx = 1;
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++, rowIdx++ )
   {
      CSegmentKey segmentKey(girderKey,segIdx);
      

      IntervalIndexType releaseIntervalIdx  = pIntervals->GetPrestressReleaseInterval(segmentKey);
      IntervalIndexType storageIntervalIdx  = pIntervals->GetStorageInterval(segmentKey);
      IntervalIndexType haulingIntervalIdx  = pIntervals->GetHaulSegmentInterval(segmentKey);
      IntervalIndexType stressingIntervalIdx = pIntervals->GetStressSegmentTendonInterval(segmentKey);

      if ( 1 < nSegments )
      {
         colIdx = 0;
         (*pLayoutTable)(rowIdx, colIdx++).SetStyleName(rptStyleManager::GetTableCellStyle(CB_NONE | CJ_CENTER));
         (*pLayoutTable)(rowIdx, colIdx++).SetStyleName(rptStyleManager::GetTableCellStyle(CB_NONE | CJ_CENTER));
         (*pLayoutTable)(rowIdx, colIdx++).SetStyleName(rptStyleManager::GetTableCellStyle(CB_NONE | CJ_CENTER));


         colIdx = 0;
         (*pLayoutTable)(rowIdx, colIdx++) << bold(ON) << _T("Segment ") << LABEL_SEGMENT(segIdx) << bold(OFF);
         (*pLayoutTable)(rowIdx, colIdx++) << bold(ON) << _T("Segment ") << LABEL_SEGMENT(segIdx) << bold(OFF);
         (*pLayoutTable)(rowIdx, colIdx++) << bold(ON) << _T("Segment ") << LABEL_SEGMENT(segIdx) << bold(OFF);
         rowIdx++;
      }

      colIdx = 0;

      rptRcTable* pTable = CreateTable(pBroker,segmentKey, stressingIntervalIdx == INVALID_INDEX ? releaseIntervalIdx : stressingIntervalIdx);
      (*pLayoutTable)(rowIdx,colIdx).SetStyleName(rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
      (*pLayoutTable)(rowIdx,colIdx++) << pTable;

      pTable = CreateTable(pBroker,segmentKey,storageIntervalIdx);
      (*pLayoutTable)(rowIdx, colIdx).SetStyleName(rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
      (*pLayoutTable)(rowIdx,colIdx++) << pTable;

      pTable = CreateTable(pBroker,segmentKey,haulingIntervalIdx-1); // storage ends the interval before hauling
      (*pLayoutTable)(rowIdx, colIdx).SetStyleName(rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
      (*pLayoutTable)(rowIdx,colIdx++) << pTable;
   }

   return pLayoutTable;
}

rptRcTable* CTimeStepCamberChapterBuilder::CreateHandlingDeflectionTable(IBroker* pBroker, const CGirderKey& girderKey) const
{
   GET_IFACE2(pBroker, IIntervals, pIntervals);
   GET_IFACE2(pBroker, IBridge, pBridge);

   ColumnIndexType nCols = 2;
   rptRcTable* pLayoutTable = rptStyleManager::CreateDefaultTable(nCols, _T("Deflections during Handling"));

   ColumnIndexType colIdx = 0;
   (*pLayoutTable)(0, colIdx++) << _T("Initial Lifting");
   (*pLayoutTable)(0, colIdx++) << _T("Hauling");

   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);

   RowIndexType rowIdx = 1;
   for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++, rowIdx++)
   {
      CSegmentKey segmentKey(girderKey, segIdx);


      IntervalIndexType liftingIntervalIdx = pIntervals->GetLiftSegmentInterval(segmentKey);
      IntervalIndexType haulingIntervalIdx = pIntervals->GetHaulSegmentInterval(segmentKey);

      if (1 < nSegments)
      {
         colIdx = 0;
         (*pLayoutTable)(rowIdx, colIdx++).SetStyleName(rptStyleManager::GetTableCellStyle(CB_NONE | CJ_CENTER));
         (*pLayoutTable)(rowIdx, colIdx++).SetStyleName(rptStyleManager::GetTableCellStyle(CB_NONE | CJ_CENTER));


         colIdx = 0;
         (*pLayoutTable)(rowIdx, colIdx++) << bold(ON) << _T("Segment ") << LABEL_SEGMENT(segIdx) << bold(OFF);
         (*pLayoutTable)(rowIdx, colIdx++) << bold(ON) << _T("Segment ") << LABEL_SEGMENT(segIdx) << bold(OFF);
         rowIdx++;
      }

      colIdx = 0;

      rptRcTable* pTable = CreateTable(pBroker, segmentKey, liftingIntervalIdx);
      (*pLayoutTable)(rowIdx, colIdx).SetStyleName(rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
      (*pLayoutTable)(rowIdx, colIdx++) << pTable;

      pTable = CreateTable(pBroker, segmentKey, haulingIntervalIdx);
      (*pLayoutTable)(rowIdx, colIdx).SetStyleName(rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
      (*pLayoutTable)(rowIdx, colIdx++) << pTable;
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
            (*pLayoutTable)(rowIdx, layoutTableColIdx).SetStyleName(rptStyleManager::GetTableCellStyle(CB_NONE | CJ_CENTER));
            (*pLayoutTable)(rowIdx, layoutTableColIdx) << bold(ON) << _T("Segment ") << LABEL_SEGMENT(segIdx) << bold(OFF);
            rowIdx++;
         }

         IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
         if ( erectionIntervalIdx <= intervalIdx )
         {
            rptRcTable* pTable = CreateTable(pBroker,segmentKey,intervalIdx);
            (*pLayoutTable)(rowIdx, layoutTableColIdx).SetStyleName(rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
            (*pLayoutTable)(rowIdx,layoutTableColIdx) << pTable;
         }
         else
         {
            (*pLayoutTable)(rowIdx, layoutTableColIdx).SetStyleName(rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
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

   GET_IFACE2(pBroker, ICamber, pCamber);
   bool bHasPrecamber = pCamber->HasPrecamber(segmentKey);
   
   GET_IFACE2(pBroker, IIntervals, pIntervals);
   IntervalIndexType liftingIntervalIdx = pIntervals->GetLiftSegmentInterval(segmentKey);
   IntervalIndexType storageIntervalIdx = pIntervals->GetStorageInterval(segmentKey);
   IntervalIndexType haulingIntervalIdx = pIntervals->GetHaulSegmentInterval(segmentKey);
   IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
   IntervalIndexType stressingIntervalIdx = pIntervals->GetStressSegmentTendonInterval(segmentKey);

   pgsTypes::PrestressDeflectionDatum datum;
   PoiAttributeType poiReference = 0;
   if (intervalIdx < liftingIntervalIdx)
   {
      poiReference = POI_RELEASED_SEGMENT;
      datum = pgsTypes::pddRelease;
   }
   else if (intervalIdx < storageIntervalIdx)
   {
      poiReference = POI_LIFT_SEGMENT;
      datum = pgsTypes::pddLifting;
   }
   else if (intervalIdx < haulingIntervalIdx)
   {
      poiReference = POI_STORAGE_SEGMENT;
      datum = pgsTypes::pddStorage;
   }
   else if (intervalIdx < erectionIntervalIdx)
   {
      poiReference = POI_HAUL_SEGMENT;
      datum = pgsTypes::pddHauling;
   }
   else
   {
      poiReference = POI_ERECTED_SEGMENT;
      datum = pgsTypes::pddErected;
   }


   GET_IFACE2(pBroker, IPointOfInterest, pIPoi);
   PoiList vPoi;
   pIPoi->GetPointsOfInterest(segmentKey, poiReference, &vPoi);

   if (poiReference != POI_RELEASED_SEGMENT)
   {
      PoiList vPoiEnds;
      pIPoi->GetPointsOfInterest(segmentKey, POI_START_FACE | POI_END_FACE, &vPoiEnds);
      ATLASSERT(vPoiEnds.size() == 2);
      pIPoi->MergePoiLists(vPoi, vPoiEnds, &vPoi);
   }

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
   //vProductForces.push_back(pgsTypes::pftLongitudinalJoint);
   //vProductForces.push_back(pgsTypes::pftSecondaryEffects);
   if (stressingIntervalIdx != INVALID_INDEX)
   {
      vProductForces.push_back(pgsTypes::pftPostTensioning);
   }

   if (storageIntervalIdx < intervalIdx)
   {
      vProductForces.push_back(pgsTypes::pftCreep);
      vProductForces.push_back(pgsTypes::pftShrinkage);
      vProductForces.push_back(pgsTypes::pftRelaxation);
   }

   INIT_UV_PROTOTYPE(rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false);
   INIT_UV_PROTOTYPE(rptLengthUnitValue, deflection, pDisplayUnits->GetDeflectionUnit(), false);

   ColumnIndexType nCols = vProductForces.size() + 2;
   if (bHasPrecamber)
   {
      nCols++;
   }

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

      pResults[i] = pForces->GetDeflection(intervalIdx,pfType,vPoi,bat,rtCumulative,false,false);
   }

   if(bHasPrecamber)
   {
      (*pTable)(0, col++) << COLHDR(_T("Precamber"), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   }

   if (erectionIntervalIdx <= intervalIdx)
   {
      (*pTable)(0, col++) << COLHDR(_T("Elevation") << rptNewLine << _T("Adjustment"), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   }

   (*pTable)(0, col++) << COLHDR(symbol(DELTA), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());

   RowIndexType row = pTable->GetNumberOfHeaderRows();

   i = 0;
   for (const pgsPointOfInterest& poi : vPoi)
   {
      col = 0;
      (*pTable)(row,col++) << location.SetValue( poiReference, poi );

      std::vector<pgsTypes::ProductForceType>::iterator pfIter(vProductForces.begin());
      std::vector<pgsTypes::ProductForceType>::iterator pfIterEnd(vProductForces.end());
      int k = 0;
      Float64 total = 0;
      for ( ; pfIter != pfIterEnd; pfIter++, k++ )
      {
         (*pTable)(row,col++) << deflection.SetValue( pResults[k].at(i) );
         total += pResults[k].at(i);
      }
      i++;

      if (bHasPrecamber)
      {
         Float64 precamber = pCamber->GetPrecamber(poi, datum);
         total += precamber;
         (*pTable)(row, col++) << deflection.SetValue(precamber);
      }

      if (erectionIntervalIdx <= intervalIdx)
      {
         Float64 elev_adj = pBridge->GetElevationAdjustment(intervalIdx, poi);
         total += elev_adj;
         (*pTable)(row, col++) << deflection.SetValue(elev_adj);
      }

      (*pTable)(row, col++) << deflection.SetValue(total);

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


   GET_IFACE2(pBroker, ICamber, pCamber);
   bool bHasPrecamber = pCamber->HasPrecamber(girderKey);
   pgsTypes::PrestressDeflectionDatum datum = pgsTypes::pddErected;

   GET_IFACE2(pBroker, ISegmentTendonGeometry, pSegmentTendonGeometry);
   DuctIndexType nMaxSegmentDucts = pSegmentTendonGeometry->GetMaxDuctCount(girderKey);

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType castDeckIntervalIdx = pIntervals->GetFirstCastDeckInterval();
   IntervalIndexType firstTendonStressingIntervalIdx = pIntervals->GetFirstGirderTendonStressingInterval(girderKey);

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

   vProductForces.push_back(pgsTypes::pftPretension);

   vProductForces.push_back(pgsTypes::pftDiaphragm);

   if (pProductLoads->HasShearKeyLoad(girderKey))
   {
      vProductForces.push_back(pgsTypes::pftShearKey);
   }

   if (!IsZero(pUserLoadData->GetConstructionLoad()))
   {
      vProductForces.push_back(pgsTypes::pftConstruction);
   }

   //vProductForces.push_back(pgsTypes::pftSlab);
   //vProductForces.push_back(pgsTypes::pftSlabPad);
   //vProductForces.push_back(pgsTypes::pftSlabPanel);
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

   // LLIM is not applicable.... that's why it is commented out
   //if ( pUserLoads->DoUserLoadsExist(girderKey,0,castDeckIntervalIdx-1,IUserDefinedLoads::userLL_IM) )
   //{
   //   vProductForces.push_back(pgsTypes::pftUserLLIM);
   //}

   if ( firstTendonStressingIntervalIdx < castDeckIntervalIdx || 0 < nMaxSegmentDucts)
   {
      vProductForces.push_back(pgsTypes::pftPostTensioning);
      
      if (firstTendonStressingIntervalIdx < castDeckIntervalIdx)
      {
         vProductForces.push_back(pgsTypes::pftSecondaryEffects);
      }
   }

   vProductForces.push_back(pgsTypes::pftCreep);
   vProductForces.push_back(pgsTypes::pftShrinkage);
   vProductForces.push_back(pgsTypes::pftRelaxation);

   ColumnIndexType nCols = vProductForces.size() + 3;
   if (bHasPrecamber)
   {
      nCols++;
   }

   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(nCols,_T("Deflections immediately prior to deck casting"));

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
   if (bHasPrecamber)
   {
      (*pTable)(0, col++) << COLHDR(_T("Precamber"), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   }
   (*pTable)(0, col++) << COLHDR(_T("Elevation") << rptNewLine << _T("Adjustment"), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   (*pTable)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("D")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

   RowIndexType row = pTable->GetNumberOfHeaderRows();

#if defined _DEBUG
   GET_IFACE2(pBroker,ILimitStateForces2,pLimitStateForces);
   std::vector<Float64> vDmin,vDmax;
   pLimitStateForces->GetDeflection(castDeckIntervalIdx-1,pgsTypes::ServiceI,vPoi,bat,true,false,true,true,true,&vDmin,&vDmax);
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

      if (bHasPrecamber)
      {
         Float64 precamber = pCamber->GetPrecamber(poi, datum);
         D += precamber;
         (*pTable)(row, col++) << deflection.SetValue(precamber);
      }

      Float64 elev_adj = pBridge->GetElevationAdjustment(castDeckIntervalIdx - 1, poi);
      (*pTable)(row, col++) << deflection.SetValue(elev_adj);
      D += elev_adj;

#if defined _DEBUG
      Float64 Dmin, Dmax;
      GET_IFACE2(pBroker, ILimitStateForces, pLSF);
      pLSF->GetDeflection(castDeckIntervalIdx - 1, pgsTypes::ServiceI, poi, bat, true, false, true, true, true, &Dmin, &Dmax);
      ATLASSERT(IsEqual(vDmin[i], Dmin));
      ATLASSERT(IsEqual(vDmax[i], Dmax));
      ATLASSERT(IsEqual(Dmin,D));
      ATLASSERT(IsEqual(Dmax,D));
#endif
      
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
   auto castDeckIntervalIdx = pIntervals->GetFirstCastDeckInterval();
   if (pBridge->GetDeckType() == pgsTypes::sdtNone)
   {
      intervalIdx = pIntervals->GetLiveLoadInterval();
   }
   else
   {
      intervalIdx = castDeckIntervalIdx;
   }
   intervalIdx--; // we actually want one interval before so when we compute D(live load interval) - D(interval) we get the increment of deflection
                  // between interval and service
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

   auto firstPTIntervalIdx = pIntervals->GetFirstGirderTendonStressingInterval(girderKey);
   auto lastPTIntervalIdx = pIntervals->GetLastGirderTendonStressingInterval(girderKey);
   const int CAST_DECK_BEFORE_PT = 1;
   const int CAST_DECK_AFTER_PT = 2;
   const int TWO_STAGE_PT = 3;
   int pt_to_deck = (castDeckIntervalIdx < firstPTIntervalIdx) ? CAST_DECK_BEFORE_PT : (lastPTIntervalIdx < castDeckIntervalIdx) ? CAST_DECK_AFTER_PT : TWO_STAGE_PT;

   GET_IFACE2(pBroker, IGirderTendonGeometry, pTendonGeom);
   DuctIndexType nDucts = pTendonGeom->GetDuctCount(girderKey);

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
   vProductForces.push_back(pgsTypes::pftPretension);

   vProductForces.push_back(pgsTypes::pftDiaphragm);

   if (pProductLoads->HasShearKeyLoad(girderKey))
   {
      vProductForces.push_back(pgsTypes::pftShearKey);
   }

   if ( !IsZero(pUserLoadData->GetConstructionLoad()) )
   {
      vProductForces.push_back(pgsTypes::pftConstruction);
   }

   // don't report this here... this is only deflections after deck casting
   // pt is before deck casting (e.g deck casting is after pt)
   //if ((pt_to_deck == CAST_DECK_AFTER_PT || pt_to_deck == TWO_STAGE_PT) && 0 < nDucts)
   //{
   //   vProductForces.push_back(pgsTypes::pftPostTensioning);
   //   vProductForces.push_back(pgsTypes::pftSecondaryEffects);
   //}

   if ( pBridge->GetDeckType() != pgsTypes::sdtNone )
   {
      vProductForces.push_back(pgsTypes::pftSlab);
      vProductForces.push_back(pgsTypes::pftSlabPad);
   }

   if ( pBridge->GetDeckType() == pgsTypes::sdtCompositeSIP )
   {
      vProductForces.push_back(pgsTypes::pftSlabPanel);
   }

   if ((pt_to_deck == CAST_DECK_BEFORE_PT || pt_to_deck == TWO_STAGE_PT) && 0 < nDucts)
   {
      vProductForces.push_back(pgsTypes::pftPostTensioning);
      //vProductForces.push_back(pgsTypes::pftSecondaryEffects); secondary effects don't cause deflections or rotations, only restraining forces
   }

   if ( pProductLoads->HasSidewalkLoad(girderKey) )
   {
      vProductForces.push_back(pgsTypes::pftSidewalk);
   }

   vProductForces.push_back(pgsTypes::pftTrafficBarrier);

   if (pIntervals->GetOverlayInterval() <= liveLoadIntervalIdx)
   {
      vProductForces.push_back(pgsTypes::pftOverlay);
   }

   if ( pUserLoads->DoUserLoadsExist(girderKey,0,liveLoadIntervalIdx,IUserDefinedLoads::userDC) )
   {
      vProductForces.push_back(pgsTypes::pftUserDC);
   }

   if ( pUserLoads->DoUserLoadsExist(girderKey,0,liveLoadIntervalIdx,IUserDefinedLoads::userDW) )
   {
      vProductForces.push_back(pgsTypes::pftUserDW);
   }

   //if ( pUserLoads->DoUserLoadsExist(girderKey,0,liveLoadIntervalIdx,IUserDefinedLoads::userLL_IM) )
   //{
   //   vProductForces.push_back(pgsTypes::pftUserLLIM);
   //}

   vProductForces.push_back(pgsTypes::pftCreep);
   vProductForces.push_back(pgsTypes::pftShrinkage);
   vProductForces.push_back(pgsTypes::pftRelaxation);

   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(vProductForces.size()+2,_T("Deflections immediately prior to deck casting to service (Screed Camber)"));

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
   pLimitStateForces->GetDeflection(intervalIdx,pgsTypes::ServiceI,vPoi,bat,true,false,true,true,true,&vDmin1,&vDmax1);
   std::vector<Float64> vDmin2,vDmax2;
   pLimitStateForces->GetDeflection(liveLoadIntervalIdx,pgsTypes::ServiceI,vPoi,bat,true,false,true,true,true,&vDmin2,&vDmax2);
   std::vector<Float64> vC;
   std::transform(vDmin2.cbegin(),vDmin2.cend(),vDmin1.cbegin(),std::back_inserter(vC),[](const auto& a, const auto& b) {return a - b;});

   GET_IFACE2(pBroker, ICamber, pCamber);
#endif


   i = 0;
   for (const pgsPointOfInterest& poi : vPoi)
   {
      col = 0;

      (*pTable)(row, col++) << location.SetValue(POI_ERECTED_SEGMENT, poi);

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
#if defined _DEBUG
      Float64 _d_, _c_;
      pCamber->GetExcessCamberEx(poi, CREEP_MAXTIME, &_d_, &_c_);
      ATLASSERT(IsEqual(_c_, -C));
#endif

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
      IntervalIndexType castDeckIntervalIdx = pIntervals->GetFirstCastDeckInterval();
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
   pLimitStateForces->GetDeflection(intervalIdx,pgsTypes::ServiceI,vPoi,bat,true,false,true,true,true,&vDmin,&vDmax);
   
   std::vector<Float64> vExcessMin,vExcessMax;
   pLimitStateForces->GetDeflection(liveLoadIntervalIdx,pgsTypes::ServiceI,vPoi,bat,true,false,true,true,true,&vExcessMin,&vExcessMax);
   
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

   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);
   PoiList vPoi;
   pIPoi->GetPointsOfInterest(CSegmentKey(girderKey, ALL_SEGMENTS), POI_ERECTED_SEGMENT, &vPoi);

   INIT_UV_PROTOTYPE( rptPointOfInterest, location,     pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, deflection,   pDisplayUnits->GetDeflectionUnit(), false );

   location.IncludeSpanAndGirder(true);


   GET_IFACE2(pBroker,IUserDefinedLoads,pUserLoads);
   GET_IFACE2(pBroker,IUserDefinedLoadData,pUserLoadData);
   GET_IFACE2(pBroker,IGirderTendonGeometry,pTendonGeom);

   GET_IFACE2(pBroker, IIntervals, pIntervals);
   auto firstCastDeckIntervalIdx = pIntervals->GetFirstCastDeckInterval();
   auto lastCastDeckIntervalIdx = pIntervals->GetLastCastDeckInterval();
   auto firstPTIntervalIdx = pIntervals->GetFirstGirderTendonStressingInterval(girderKey);
   auto lastPTIntervalIdx = pIntervals->GetLastGirderTendonStressingInterval(girderKey);
   const int CAST_DECK_BEFORE_PT = 1; // All PT after deck
   const int CAST_DECK_AFTER_PT = 2;  // All PT before deck
   const int TWO_STAGE_PT = 3;        // Some PT before deck and some PT after deck
   int pt_to_deck = (lastCastDeckIntervalIdx < firstPTIntervalIdx) ? CAST_DECK_BEFORE_PT : (lastPTIntervalIdx < firstCastDeckIntervalIdx) ? CAST_DECK_AFTER_PT : TWO_STAGE_PT;

   auto overlayIntervalIdx = pIntervals->GetOverlayInterval();
   auto liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();
   IntervalIndexType lastIntervalIdx = pIntervals->GetIntervalCount() - 1;

   DuctIndexType nDucts = pTendonGeom->GetDuctCount(girderKey);

   GET_IFACE2(pBroker, ICamber, pCamber);
   bool bHasPrecamber = pCamber->HasPrecamber(girderKey);
   pgsTypes::PrestressDeflectionDatum datum = pgsTypes::pddErected;

   std::vector<pgsTypes::ProductForceType> vProductForces;
   vProductForces.push_back(pgsTypes::pftGirder);

   vProductForces.push_back(pgsTypes::pftPretension);

   vProductForces.push_back(pgsTypes::pftDiaphragm);

   if (pProductLoads->HasShearKeyLoad(girderKey))
   {
      vProductForces.push_back(pgsTypes::pftShearKey);
   }

   if ( !IsZero(pUserLoadData->GetConstructionLoad()) )
   {
      vProductForces.push_back(pgsTypes::pftConstruction);
   }

   if ((pt_to_deck == CAST_DECK_AFTER_PT || pt_to_deck == TWO_STAGE_PT) && 0 < nDucts)
   {
      vProductForces.push_back(pgsTypes::pftPostTensioning);
      vProductForces.push_back(pgsTypes::pftSecondaryEffects);
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

   if (pt_to_deck == CAST_DECK_BEFORE_PT && 0 < nDucts)
   {
      vProductForces.push_back(pgsTypes::pftPostTensioning);
      vProductForces.push_back(pgsTypes::pftSecondaryEffects);
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

   //if ( pUserLoads->DoUserLoadsExist(girderKey,IUserDefinedLoads::userLL_IM) )
   //{
   //   vProductForces.push_back(pgsTypes::pftUserLLIM);
   //}

   if (overlayIntervalIdx != INVALID_INDEX)
   {
      vProductForces.push_back(pgsTypes::pftOverlay);
   }

   vProductForces.push_back(pgsTypes::pftCreep);
   vProductForces.push_back(pgsTypes::pftShrinkage);
   vProductForces.push_back(pgsTypes::pftRelaxation);

   ColumnIndexType nCols = vProductForces.size() + 4;
   if (bHasPrecamber)
   {
      nCols++;
   }

   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(nCols,_T("Final Deflections"));

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
      pResults[i] = pForces->GetDeflection(lastIntervalIdx,pfType,vPoi,bat,rtCumulative,false,false);
   }
   if (bHasPrecamber)
   {
      (*pTable)(0, col++) << COLHDR(_T("Precamber"), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   }
   (*pTable)(0, col++) << COLHDR(_T("Elevation") << rptNewLine << _T("Adjustment"), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());

   (*pTable)(0, col++) << COLHDR(Sub2(symbol(DELTA),_T("service")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());
   (*pTable)(0, col++) << COLHDR(Sub2(symbol(DELTA),_T("final")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit());

   RowIndexType row = pTable->GetNumberOfHeaderRows();

   GET_IFACE2(pBroker,ILimitStateForces2,pLimitStateForces);
   std::vector<Float64> vDmin, vDmax; // deflection at time bridge is open to traffic
   pLimitStateForces->GetDeflection(liveLoadIntervalIdx, pgsTypes::ServiceI, vPoi, bat, true, false, true, true, true, &vDmin, &vDmax);

   std::vector<Float64> vFmin, vFmax; // final deflection
   pLimitStateForces->GetDeflection(lastIntervalIdx, pgsTypes::ServiceI, vPoi, bat, true, false, true, true, true, &vFmin, &vFmax);

   i = 0;
   for (const pgsPointOfInterest& poi : vPoi)
   {
      col = 0;

      (*pTable)(row,col++) << location.SetValue(POI_ERECTED_SEGMENT, poi );

      std::vector<pgsTypes::ProductForceType>::iterator pfIter(vProductForces.begin());
      std::vector<pgsTypes::ProductForceType>::iterator pfIterEnd(vProductForces.end());
      int k = 0;
      for ( ; pfIter != pfIterEnd; pfIter++, k++ )
      {
         Float64 d = pResults[k].at(i);
         (*pTable)(row,col++) << deflection.SetValue( d );
      }

      if (bHasPrecamber)
      {
         Float64 precamber = pCamber->GetPrecamber(poi, datum);
         (*pTable)(row, col++) << deflection.SetValue(precamber);
      }

      Float64 elev_adj = pBridge->GetElevationAdjustment(lastIntervalIdx, poi);
      (*pTable)(row, col++) << deflection.SetValue(elev_adj);
      
      (*pTable)(row, col++) << deflection.SetValue(vDmin[i]);
      (*pTable)(row, col++) << deflection.SetValue(vFmin[i]);

      i++;
      row++;
   }

   delete[] pResults;

   return pTable;
}

