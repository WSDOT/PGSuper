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
#include <Reporting\TimeStepCamberChapterBuilder.h>

#include <IFace\Intervals.h>
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\AnalysisResults.h>


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
   *pPara << CreateBeforeSlabCastingDeflectionTable(pBroker,girderKey) << rptNewLine;
   *pPara << CreateScreedCamberDeflectionTable(pBroker,girderKey)      << rptNewLine;
   *pPara << CreateExcessCamberTable(pBroker,girderKey)                << rptNewLine;
   *pPara << CreateFinalDeflectionTable(pBroker,girderKey)             << rptNewLine;

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
   rptRcTable* pLayoutTable = pgsReportStyleHolder::CreateDefaultTable(nCols,_T("Deflections during Storage"));
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

      pLayoutTable->SetColumnSpan(rowIdx,0,nCols);
      for ( ColumnIndexType c = 1; c < nCols; c++ )
      {
         pLayoutTable->SetColumnSpan(rowIdx,c,SKIP_CELL);
      }
      (*pLayoutTable)(rowIdx++,0) << _T("Segment ") << LABEL_SEGMENT(segIdx);

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
   rptRcTable* pLayoutTable = pgsReportStyleHolder::CreateDefaultTable(nCols,_T("Deflections after Erection"));

   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);

   ColumnIndexType layoutTableColIdx = 0;
   std::vector<IntervalIndexType>::iterator intervalIter(vIntervals.begin());
   std::vector<IntervalIndexType>::iterator intervalIterEnd(vIntervals.end());
   for ( ; intervalIter != intervalIterEnd; intervalIter++, layoutTableColIdx++ )
   {
      RowIndexType rowIdx = 0;
      IntervalIndexType intervalIdx = *intervalIter;
      (*pLayoutTable)(rowIdx++,layoutTableColIdx) << _T("Interval ") << LABEL_INTERVAL(intervalIdx) << rptNewLine << pIntervals->GetDescription(girderKey,intervalIdx);

      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++, rowIdx++ )
      {
         CSegmentKey segmentKey(girderKey,segIdx);

         if ( layoutTableColIdx == 0 )
         {
            // put segment name in segment label row
            pLayoutTable->SetColumnSpan(rowIdx,0,nCols);
            for ( ColumnIndexType c = 1; c < nCols; c++ )
            {
               pLayoutTable->SetColumnSpan(rowIdx,c,SKIP_CELL);
            }
            (*pLayoutTable)(rowIdx++,layoutTableColIdx) << _T("Segment ") << LABEL_SEGMENT(segIdx);
         }
         else
         {
            // skip over the segment label row
            rowIdx++;
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

rptRcTable* CTimeStepCamberChapterBuilder::CreateTable(IBroker* pBroker,const CSegmentKey& segmentKey,IntervalIndexType intervalIdx) const
{
   ASSERT_SEGMENT_KEY(segmentKey);

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker,IProductForces2,pForces);

   GET_IFACE2(pBroker,IProductForces,pProduct);
   pgsTypes::BridgeAnalysisType bat = pProduct->GetBridgeAnalysisType(pgsTypes::Minimize);

   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 end_size = pBridge->GetSegmentStartEndDistance(segmentKey);

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType storageIntervalIdx  = pIntervals->GetStorageInterval(segmentKey);
   IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
   PoiAttributeType poiReference = 0;
   if ( intervalIdx < storageIntervalIdx )
   {
      poiReference = POI_RELEASED_SEGMENT;
   }
   else if ( intervalIdx < erectionIntervalIdx )
   {
      poiReference = POI_STORAGE_SEGMENT;
   }
   else
   {
      poiReference = POI_ERECTED_SEGMENT;
   }


   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);
   std::vector<pgsPointOfInterest> vPoi( pIPoi->GetPointsOfInterest( segmentKey, poiReference ) );

   GET_IFACE2(pBroker,IUserDefinedLoads,pUserLoads);

   std::vector<ProductForceType> vProductForces;
   vProductForces.push_back(pftGirder);
   //vProductForces.push_back(pftConstruction);
   //vProductForces.push_back(pftSlab);
   //vProductForces.push_back(pftSlabPad);
   //vProductForces.push_back(pftSlabPanel);
   //vProductForces.push_back(pftDiaphragm); 
   //vProductForces.push_back(pftOverlay);
   //vProductForces.push_back(pftSidewalk);
   //vProductForces.push_back(pftTrafficBarrier);
   if ( pUserLoads->DoUserLoadsExist(segmentKey,0,intervalIdx,IUserDefinedLoads::userDC) )
   {
      vProductForces.push_back(pftUserDC);
   }

   if ( pUserLoads->DoUserLoadsExist(segmentKey,0,intervalIdx,IUserDefinedLoads::userDW) )
   {
      vProductForces.push_back(pftUserDW);
   }

   if ( pUserLoads->DoUserLoadsExist(segmentKey,0,intervalIdx,IUserDefinedLoads::userLL_IM) )
   {
      vProductForces.push_back(pftUserLLIM);
   }
   //vProductForces.push_back(pftShearKey);
   //vProductForces.push_back(pftSecondaryEffects);
   //vProductForces.push_back(pftPrimaryPostTensioning);
   vProductForces.push_back(pftPretension);

   if ( storageIntervalIdx < intervalIdx )
   {
      vProductForces.push_back(pftCreep);
      vProductForces.push_back(pftShrinkage);
      vProductForces.push_back(pftRelaxation);
   }

   INIT_UV_PROTOTYPE( rptPointOfInterest, location,     pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, deflection,   pDisplayUnits->GetDeflectionUnit(), false );

   rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(vProductForces.size()+1);

   std::vector<Float64>* pResults = new std::vector<Float64>[vProductForces.size()];

   GET_IFACE2(pBroker,IProductLoads,pProductLoads);

   ColumnIndexType col = 0;
   (*pTable)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   std::vector<ProductForceType>::iterator pfIter(vProductForces.begin());
   std::vector<ProductForceType>::iterator pfIterEnd(vProductForces.end());
   int i = 0;
   for ( ; pfIter != pfIterEnd; pfIter++, i++ )
   {
      ProductForceType pfType = *pfIter;
      std::_tstring strName = pProductLoads->GetProductLoadName(pfType);

      (*pTable)(0,col++) << COLHDR(Sub2(symbol(DELTA),strName.c_str()),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

      pResults[i] = pForces->GetDeflection(intervalIdx,pfType,vPoi,bat,rtCumulative,true);
   }

   RowIndexType row = pTable->GetNumberOfHeaderRows();

   std::vector<pgsPointOfInterest>::iterator poiIter(vPoi.begin());
   std::vector<pgsPointOfInterest>::iterator poiIterEnd(vPoi.end());
   i = 0;
   for ( ; poiIter != poiIterEnd; poiIter++, i++, row++ )
   {
      col = 0;
      pgsPointOfInterest& poi(*poiIter);

      (*pTable)(row,col++) << location.SetValue( poiReference, poi, end_size );

      std::vector<ProductForceType>::iterator pfIter(vProductForces.begin());
      std::vector<ProductForceType>::iterator pfIterEnd(vProductForces.end());
      int k = 0;
      for ( ; pfIter != pfIterEnd; pfIter++, k++ )
      {
         (*pTable)(row,col++) << deflection.SetValue( pResults[k].at(i) );
      }
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
   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval(girderKey);
   IntervalIndexType firstTendonStressingIntervalIdx = pIntervals->GetFirstTendonStressingInterval(girderKey);

   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);
   std::vector<pgsPointOfInterest> vPoi( pIPoi->GetPointsOfInterest( CSegmentKey(girderKey,ALL_SEGMENTS), POI_ERECTED_SEGMENT ) );

   INIT_UV_PROTOTYPE( rptPointOfInterest, location,     pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, deflection,   pDisplayUnits->GetDeflectionUnit(), false );

   location.IncludeSpanAndGirder(true);

   GET_IFACE2(pBroker,IUserDefinedLoads,pUserLoads);
   GET_IFACE2(pBroker,IUserDefinedLoadData,pUserLoadData);

   std::vector<ProductForceType> vProductForces;
   vProductForces.push_back(pftGirder);
   if ( !IsZero(pUserLoadData->GetConstructionLoad()) )
   {
      vProductForces.push_back(pftConstruction);
   }
   //vProductForces.push_back(pftSlab);
   //vProductForces.push_back(pftSlabPad);
   //vProductForces.push_back(pftSlabPanel);
   vProductForces.push_back(pftDiaphragm); 
   //vProductForces.push_back(pftOverlay);
   //vProductForces.push_back(pftSidewalk);
   //vProductForces.push_back(pftTrafficBarrier);
   if ( pUserLoads->DoUserLoadsExist(girderKey,0,castDeckIntervalIdx-1,IUserDefinedLoads::userDC) )
   {
      vProductForces.push_back(pftUserDC);
   }

   if ( pUserLoads->DoUserLoadsExist(girderKey,0,castDeckIntervalIdx-1,IUserDefinedLoads::userDW) )
   {
      vProductForces.push_back(pftUserDW);
   }

   if ( pUserLoads->DoUserLoadsExist(girderKey,0,castDeckIntervalIdx-1,IUserDefinedLoads::userLL_IM) )
   {
      vProductForces.push_back(pftUserLLIM);
   }
   //vProductForces.push_back(pftShearKey);

   if ( firstTendonStressingIntervalIdx < castDeckIntervalIdx )
   {
      vProductForces.push_back(pftSecondaryEffects);
      vProductForces.push_back(pftPrimaryPostTensioning);
   }

   vProductForces.push_back(pftPretension);
   vProductForces.push_back(pftCreep);
   vProductForces.push_back(pftShrinkage);
   vProductForces.push_back(pftRelaxation);

   rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(vProductForces.size()+2,_T("Deflections prior to slab casting"));

   std::vector<Float64>* pResults = new std::vector<Float64>[vProductForces.size()];

   ColumnIndexType col = 0;
   (*pTable)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   std::vector<ProductForceType>::iterator pfIter(vProductForces.begin());
   std::vector<ProductForceType>::iterator pfIterEnd(vProductForces.end());
   int i = 0;
   for ( ; pfIter != pfIterEnd; pfIter++, i++ )
   {
      ProductForceType pfType = *pfIter;
      std::_tstring strName = pProductLoads->GetProductLoadName(pfType);

      (*pTable)(0,col++) << COLHDR(Sub2(symbol(DELTA),strName.c_str()),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

      // NOTE: Minus one because we want the interval immediately prior to deck placement
      // This is what the "D" dimension is.
      pResults[i] = pForces->GetDeflection(castDeckIntervalIdx-1,pfType,vPoi,bat,rtCumulative,true);
   }
  (*pTable)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("D")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

   RowIndexType row = pTable->GetNumberOfHeaderRows();

#if defined _DEBUG
   GET_IFACE2(pBroker,ILimitStateForces2,pLimitStateForces);
   std::vector<Float64> vDmin,vDmax;
   pLimitStateForces->GetDeflection(castDeckIntervalIdx-1,pgsTypes::ServiceI,vPoi,bat,true,false,true,&vDmin,&vDmax);
#endif

   std::vector<pgsPointOfInterest>::iterator poiIter(vPoi.begin());
   std::vector<pgsPointOfInterest>::iterator poiIterEnd(vPoi.end());
   i = 0;
   for ( ; poiIter != poiIterEnd; poiIter++, i++, row++ )
   {
      col = 0;
      pgsPointOfInterest& poi(*poiIter);

      (*pTable)(row,col++) << location.SetValue( POI_ERECTED_SEGMENT, poi, end_size );

      Float64 D = 0;
      std::vector<ProductForceType>::iterator pfIter(vProductForces.begin());
      std::vector<ProductForceType>::iterator pfIterEnd(vProductForces.end());
      int k = 0;
      for ( ; pfIter != pfIterEnd; pfIter++, k++ )
      {
         Float64 d = pResults[k].at(i);
         (*pTable)(row,col++) << deflection.SetValue( d );
         D += d;
      }
      
      ATLASSERT(IsEqual(vDmin[i],D));
      ATLASSERT(IsEqual(vDmax[i],D));

      (*pTable)(row,col++) << deflection.SetValue(D);
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
   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval(girderKey);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval(girderKey);

   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);
   std::vector<pgsPointOfInterest> vPoi( pIPoi->GetPointsOfInterest( CSegmentKey(girderKey,ALL_SEGMENTS), POI_ERECTED_SEGMENT ) );

   INIT_UV_PROTOTYPE( rptPointOfInterest, location,     pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, deflection,   pDisplayUnits->GetDeflectionUnit(), false );

   location.IncludeSpanAndGirder(true);

   GET_IFACE2(pBroker,IUserDefinedLoads,pUserLoads);
   GET_IFACE2(pBroker,IUserDefinedLoadData,pUserLoadData);
   GET_IFACE2(pBroker,ITendonGeometry,pTendonGeom);

   std::vector<ProductForceType> vProductForces;
   vProductForces.push_back(pftGirder);

   if ( !IsZero(pUserLoadData->GetConstructionLoad()) )
   {
      vProductForces.push_back(pftConstruction);
   }

   if ( pBridge->GetDeckType() != pgsTypes::sdtNone )
   {
      vProductForces.push_back(pftSlab);
      vProductForces.push_back(pftSlabPad);
   }

   if ( pBridge->GetDeckType() == pgsTypes::sdtCompositeSIP )
   {
      vProductForces.push_back(pftSlabPanel);
   }

   vProductForces.push_back(pftDiaphragm); 

   if ( pIntervals->GetOverlayInterval(girderKey) <= liveLoadIntervalIdx )
   {
      vProductForces.push_back(pftOverlay);
   }

   if ( pProductLoads->HasSidewalkLoad(girderKey) )
   {
      vProductForces.push_back(pftSidewalk);
   }

   vProductForces.push_back(pftTrafficBarrier);

   if ( pUserLoads->DoUserLoadsExist(girderKey,0,liveLoadIntervalIdx,IUserDefinedLoads::userDC) )
   {
      vProductForces.push_back(pftUserDC);
   }

   if ( pUserLoads->DoUserLoadsExist(girderKey,0,liveLoadIntervalIdx,IUserDefinedLoads::userDW) )
   {
      vProductForces.push_back(pftUserDW);
   }

   if ( pUserLoads->DoUserLoadsExist(girderKey,0,liveLoadIntervalIdx,IUserDefinedLoads::userLL_IM) )
   {
      vProductForces.push_back(pftUserLLIM);
   }

   if ( pProductLoads->HasShearKeyLoad(girderKey) )
   {
      vProductForces.push_back(pftShearKey);
   }

   if ( 0 < pTendonGeom->GetDuctCount(girderKey) )
   {
      vProductForces.push_back(pftSecondaryEffects);
      vProductForces.push_back(pftPrimaryPostTensioning);
   }

   vProductForces.push_back(pftPretension);
   vProductForces.push_back(pftCreep);
   vProductForces.push_back(pftShrinkage);
   vProductForces.push_back(pftRelaxation);

   rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(vProductForces.size()+2,_T("Deflections from slab casting to service (Screed Camber)"));

   std::vector<Float64>* pResults1 = new std::vector<Float64>[vProductForces.size()];
   std::vector<Float64>* pResults2 = new std::vector<Float64>[vProductForces.size()];

   ColumnIndexType col = 0;
   (*pTable)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   std::vector<ProductForceType>::iterator pfIter(vProductForces.begin());
   std::vector<ProductForceType>::iterator pfIterEnd(vProductForces.end());
   int i = 0;
   for ( ; pfIter != pfIterEnd; pfIter++, i++ )
   {
      ProductForceType pfType = *pfIter;
      std::_tstring strName = pProductLoads->GetProductLoadName(pfType);

      (*pTable)(0,col++) << COLHDR(Sub2(symbol(DELTA),strName.c_str()),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

      pResults1[i] = pForces->GetDeflection(castDeckIntervalIdx-1,pfType,vPoi,bat,rtCumulative,true);
      pResults2[i] = pForces->GetDeflection(liveLoadIntervalIdx,  pfType,vPoi,bat,rtCumulative,true);
   }
  (*pTable)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("C")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

   RowIndexType row = pTable->GetNumberOfHeaderRows();


#if defined _DEBUG
   GET_IFACE2(pBroker,ILimitStateForces2,pLimitStateForces);
   std::vector<Float64> vDmin1,vDmax1;
   pLimitStateForces->GetDeflection(castDeckIntervalIdx-1,pgsTypes::ServiceI,vPoi,bat,true,false,true,&vDmin1,&vDmax1);
   std::vector<Float64> vDmin2,vDmax2;
   pLimitStateForces->GetDeflection(liveLoadIntervalIdx,pgsTypes::ServiceI,vPoi,bat,true,false,true,&vDmin2,&vDmax2);
   std::vector<Float64> vC;
   std::transform(vDmax2.begin(),vDmax2.end(),vDmax1.begin(),std::back_inserter(vC),std::minus<Float64>());
#endif

   std::vector<pgsPointOfInterest>::iterator poiIter(vPoi.begin());
   std::vector<pgsPointOfInterest>::iterator poiIterEnd(vPoi.end());
   i = 0;
   for ( ; poiIter != poiIterEnd; poiIter++, i++, row++ )
   {
      col = 0;
      pgsPointOfInterest& poi(*poiIter);

      (*pTable)(row,col++) << location.SetValue( POI_ERECTED_SEGMENT, poi, end_size );

      Float64 C = 0;
      std::vector<ProductForceType>::iterator pfIter(vProductForces.begin());
      std::vector<ProductForceType>::iterator pfIterEnd(vProductForces.end());
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

      (*pTable)(row,col++) << deflection.SetValue(C);
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
   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval(girderKey);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval(girderKey);

   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);
   std::vector<pgsPointOfInterest> vPoi( pIPoi->GetPointsOfInterest( CSegmentKey(girderKey,ALL_SEGMENTS), POI_ERECTED_SEGMENT ) );

   INIT_UV_PROTOTYPE( rptPointOfInterest, location,     pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, deflection,   pDisplayUnits->GetDeflectionUnit(), false );

   location.IncludeSpanAndGirder(true);

   rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(4,_T("Excess Camber"));

   ColumnIndexType col = 0;
   (*pTable)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*pTable)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("D")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*pTable)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("C")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*pTable)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("Excess")), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

   RowIndexType row = pTable->GetNumberOfHeaderRows();

   GET_IFACE2(pBroker,ILimitStateForces2,pLimitStateForces);
   std::vector<Float64> vDmin,vDmax;
   pLimitStateForces->GetDeflection(castDeckIntervalIdx-1,pgsTypes::ServiceI,vPoi,bat,true,false,true,&vDmin,&vDmax);
   std::vector<Float64> vExcessMin,vExcessMax;
   pLimitStateForces->GetDeflection(liveLoadIntervalIdx,pgsTypes::ServiceI,vPoi,bat,true,false,true,&vExcessMin,&vExcessMax);
   std::vector<Float64> vC;
   std::transform(vExcessMax.begin(),vExcessMax.end(),vDmax.begin(),std::back_inserter(vC),std::minus<Float64>());

   std::vector<pgsPointOfInterest>::iterator poiIter(vPoi.begin());
   std::vector<pgsPointOfInterest>::iterator poiIterEnd(vPoi.end());
   int i = 0;
   for ( ; poiIter != poiIterEnd; poiIter++, i++, row++ )
   {
      col = 0;
      pgsPointOfInterest& poi(*poiIter);

      (*pTable)(row,col++) << location.SetValue( POI_ERECTED_SEGMENT, poi, end_size );
      (*pTable)(row,col++) << deflection.SetValue( vDmax[i] );
      (*pTable)(row,col++) << deflection.SetValue( vC[i] );
      (*pTable)(row,col++) << deflection.SetValue( vExcessMax[i] );
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
   IntervalIndexType lastIntervalIdx = pIntervals->GetIntervalCount(girderKey)-1;

   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);
   std::vector<pgsPointOfInterest> vPoi( pIPoi->GetPointsOfInterest( CSegmentKey(girderKey,ALL_SEGMENTS), POI_ERECTED_SEGMENT ) );

   INIT_UV_PROTOTYPE( rptPointOfInterest, location,     pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, deflection,   pDisplayUnits->GetDeflectionUnit(), false );

   location.IncludeSpanAndGirder(true);


   GET_IFACE2(pBroker,IUserDefinedLoads,pUserLoads);
   GET_IFACE2(pBroker,IUserDefinedLoadData,pUserLoadData);
   GET_IFACE2(pBroker,ITendonGeometry,pTendonGeom);

   std::vector<ProductForceType> vProductForces;
   vProductForces.push_back(pftGirder);

   if ( !IsZero(pUserLoadData->GetConstructionLoad()) )
   {
      vProductForces.push_back(pftConstruction);
   }

   if ( pBridge->GetDeckType() != pgsTypes::sdtNone )
   {
      vProductForces.push_back(pftSlab);
      vProductForces.push_back(pftSlabPad);
   }

   if ( pBridge->GetDeckType() == pgsTypes::sdtCompositeSIP )
   {
      vProductForces.push_back(pftSlabPanel);
   }

   vProductForces.push_back(pftDiaphragm); 

   if ( pIntervals->GetOverlayInterval(girderKey) != INVALID_INDEX )
   {
      vProductForces.push_back(pftOverlay);
   }

   if ( pProductLoads->HasSidewalkLoad(girderKey) )
   {
      vProductForces.push_back(pftSidewalk);
   }

   vProductForces.push_back(pftTrafficBarrier);

   if ( pUserLoads->DoUserLoadsExist(girderKey,IUserDefinedLoads::userDC) )
   {
      vProductForces.push_back(pftUserDC);
   }

   if ( pUserLoads->DoUserLoadsExist(girderKey,IUserDefinedLoads::userDW) )
   {
      vProductForces.push_back(pftUserDW);
   }

   if ( pUserLoads->DoUserLoadsExist(girderKey,IUserDefinedLoads::userLL_IM) )
   {
      vProductForces.push_back(pftUserLLIM);
   }

   if ( pProductLoads->HasShearKeyLoad(girderKey) )
   {
      vProductForces.push_back(pftShearKey);
   }

   if ( 0 < pTendonGeom->GetDuctCount(girderKey) )
   {
      vProductForces.push_back(pftSecondaryEffects);
      vProductForces.push_back(pftPrimaryPostTensioning);
   }

   vProductForces.push_back(pftPretension);
   vProductForces.push_back(pftCreep);
   vProductForces.push_back(pftShrinkage);
   vProductForces.push_back(pftRelaxation);

   rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(vProductForces.size()+2,_T("Final Deflections"));

   std::vector<Float64>* pResults = new std::vector<Float64>[vProductForces.size()];

   ColumnIndexType col = 0;
   (*pTable)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   std::vector<ProductForceType>::iterator pfIter(vProductForces.begin());
   std::vector<ProductForceType>::iterator pfIterEnd(vProductForces.end());
   int i = 0;
   for ( ; pfIter != pfIterEnd; pfIter++, i++ )
   {
      ProductForceType pfType = *pfIter;
      std::_tstring strName = pProductLoads->GetProductLoadName(pfType);

      (*pTable)(0,col++) << COLHDR(Sub2(symbol(DELTA),strName.c_str()),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

      // NOTE: Minus one because we want the interval immediately prior to deck placement
      // This is what the "D" dimension is.
      pResults[i] = pForces->GetDeflection(lastIntervalIdx,pfType,vPoi,bat,rtCumulative,true);
   }
  (*pTable)(0,col++) << COLHDR(symbol(DELTA), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

   RowIndexType row = pTable->GetNumberOfHeaderRows();

   GET_IFACE2(pBroker,ILimitStateForces2,pLimitStateForces);
   std::vector<Float64> vDmin,vDmax;
   pLimitStateForces->GetDeflection(lastIntervalIdx,pgsTypes::ServiceI,vPoi,bat,true,false,true,&vDmin,&vDmax);

   std::vector<pgsPointOfInterest>::iterator poiIter(vPoi.begin());
   std::vector<pgsPointOfInterest>::iterator poiIterEnd(vPoi.end());
   i = 0;
   for ( ; poiIter != poiIterEnd; poiIter++, i++, row++ )
   {
      col = 0;
      pgsPointOfInterest& poi(*poiIter);

      (*pTable)(row,col++) << location.SetValue( POI_ERECTED_SEGMENT, poi, end_size );

      std::vector<ProductForceType>::iterator pfIter(vProductForces.begin());
      std::vector<ProductForceType>::iterator pfIterEnd(vProductForces.end());
      int k = 0;
      for ( ; pfIter != pfIterEnd; pfIter++, k++ )
      {
         Float64 d = pResults[k].at(i);
         (*pTable)(row,col++) << deflection.SetValue( d );
      }
      
      (*pTable)(row,col++) << deflection.SetValue(vDmax[i]);
   }

   delete[] pResults;

   return pTable;
}
