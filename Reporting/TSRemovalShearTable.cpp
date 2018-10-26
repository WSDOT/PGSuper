///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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
#include <Reporting\TSRemovalShearTable.h>
#include <Reporting\ReportNotes.h>
#include <Reporting\ProductMomentsTable.h>

#include <PgsExt\GirderPointOfInterest.h>
#include <PgsExt\BridgeDescription2.h>

#include <IFace\Project.h>
#include <IFace\Bridge.h>

#include <IFace\AnalysisResults.h>
#include <IFace\RatingSpecification.h>

#include <IFace\Intervals.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CTSRemovalShearTable
****************************************************************************/
CTSRemovalShearTable::CTSRemovalShearTable()
{
}

CTSRemovalShearTable::CTSRemovalShearTable(const CTSRemovalShearTable& rOther)
{
   MakeCopy(rOther);
}

CTSRemovalShearTable::~CTSRemovalShearTable()
{
}

CTSRemovalShearTable& CTSRemovalShearTable::operator= (const CTSRemovalShearTable& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}


void CTSRemovalShearTable::Build(rptChapter* pChapter,IBroker* pBroker,const CGirderKey& girderKey,pgsTypes::AnalysisType analysisType,IEAFDisplayUnits* pDisplayUnits) const
{
   // Build table
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptForceSectionValue, shear, pDisplayUnits->GetShearUnit(), false );

   bool bConstruction, bDeckPanels, bPedLoading, bSidewalk, bShearKey, bPermit;
   IntervalIndexType continuity_interval;
   GET_IFACE2(pBroker,IBridge,pBridge);
   bool bIsFutureOverlay = pBridge->IsFutureOverlay();

   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   GroupIndexType startGroup = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
   GroupIndexType endGroup   = (girderKey.groupIndex == ALL_GROUPS ? nGroups-1 : startGroup);

   GET_IFACE2(pBroker, IRatingSpecification, pRatingSpec);


   // Get the results
   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);
   GET_IFACE2(pBroker,IProductForces2,pForces2);
   GET_IFACE2(pBroker,IProductLoads,pLoads);
   GET_IFACE2(pBroker,IProductForces,pProdForces);
   pgsTypes::BridgeAnalysisType maxBAT = pProdForces->GetBridgeAnalysisType(analysisType,pgsTypes::Maximize);
   pgsTypes::BridgeAnalysisType minBAT = pProdForces->GetBridgeAnalysisType(analysisType,pgsTypes::Minimize);

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType castDeckIntervalIdx      = pIntervals->GetCastDeckInterval();
   IntervalIndexType railingSystemIntervalIdx = pIntervals->GetInstallRailingSystemInterval();
   IntervalIndexType liveLoadIntervalIdx      = pIntervals->GetLiveLoadInterval();
   IntervalIndexType overlayIntervalIdx       = pIntervals->GetOverlayInterval();
   IntervalIndexType erectSegmentIntervalIdx  = pIntervals->GetFirstErectedSegmentInterval();

   std::set<IntervalIndexType> tsrIntervals;
   for ( GroupIndexType grpIdx = startGroup; grpIdx <= endGroup; grpIdx++ )
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
      GirderIndexType gdrIdx = Min(girderKey.girderIndex,nGirders-1);

      // Get the intervals when temporary supports are removed for this group
      GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
      const CGirderGroupData* pGroup = pIBridgeDesc->GetGirderGroup(grpIdx);
      const CSpanData2* pSpan = pGroup->GetPier(pgsTypes::metStart)->GetNextSpan();
      const CSpanData2* pEndSpan = pGroup->GetPier(pgsTypes::metEnd)->GetPrevSpan();
      for ( SpanIndexType spanIdx = pSpan->GetIndex(); spanIdx <= pEndSpan->GetIndex(); spanIdx++ )
      {
         std::vector<const CTemporarySupportData*> vTS( pSpan->GetTemporarySupports() );
         std::vector<const CTemporarySupportData*>::iterator tsIter(vTS.begin());
         std::vector<const CTemporarySupportData*>::iterator tsIterEnd(vTS.end());
         for ( ; tsIter != tsIterEnd; tsIter++ )
         {
            const CTemporarySupportData* pTS = *tsIter;
            tsrIntervals.insert( pIntervals->GetTemporarySupportRemovalInterval(pTS->GetID()) );
         }

         pSpan = pSpan->GetNextPier()->GetNextSpan();
      }

      if ( tsrIntervals.size() == 0 )
         continue; // next group


      std::set<IntervalIndexType>::iterator iter(tsrIntervals.begin());
      std::set<IntervalIndexType>::iterator end(tsrIntervals.end());
      for ( ; iter != end; iter++ )
      {
         IntervalIndexType tsrIntervalIdx = *iter;

         ColumnIndexType nCols = GetProductLoadTableColumnCount(pBroker,girderKey,analysisType,false,false,&bConstruction,&bDeckPanels,&bSidewalk,&bShearKey,&bPedLoading,&bPermit,&continuity_interval,&startGroup,&nGroups);
         bPedLoading = false;
         bPermit     = false;

         CString strLabel;
         strLabel.Format(_T("Shear due to removal of temporary supports in Interval %d"),LABEL_INTERVAL(tsrIntervalIdx));
         rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(nCols,strLabel);

         rptParagraph* p = new rptParagraph;
         *pChapter << p;
         *p << p_table << rptNewLine;

         if ( girderKey.groupIndex == ALL_GROUPS )
         {
            p_table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
            p_table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
         }

         location.IncludeSpanAndGirder(girderKey.groupIndex == ALL_GROUPS);

         RowIndexType row = ConfigureProductLoadTableHeading<rptForceUnitTag,unitmgtForceData>(pBroker,p_table,false,false,bConstruction,bDeckPanels,bSidewalk,bShearKey,bIsFutureOverlay,false,bPedLoading,
                                                                                                 bPermit,false,analysisType,continuity_interval,
                                                                                                 pRatingSpec,pDisplayUnits,pDisplayUnits->GetShearUnit());


         CSegmentKey allSegmentsKey(grpIdx,gdrIdx,ALL_SEGMENTS);
         std::vector<pgsPointOfInterest> vPoi( pIPoi->GetPointsOfInterest(allSegmentsKey) );

         // Get the results for this span (it is faster to get them as a vector rather than individually)
         std::vector<sysSectionValue> girder    = pForces2->GetShear(tsrIntervalIdx, pftGirder,    vPoi, maxBAT);
         std::vector<sysSectionValue> diaphragm = pForces2->GetShear(tsrIntervalIdx, pftDiaphragm, vPoi, maxBAT);

         std::vector<sysSectionValue> minSlab, maxSlab;
         std::vector<sysSectionValue> minSlabPad, maxSlabPad;
         maxSlab = pForces2->GetShear( tsrIntervalIdx, pftSlab, vPoi, maxBAT );
         minSlab = pForces2->GetShear( tsrIntervalIdx, pftSlab, vPoi, minBAT );

         maxSlabPad = pForces2->GetShear( tsrIntervalIdx, pftSlabPad, vPoi, maxBAT );
         minSlabPad = pForces2->GetShear( tsrIntervalIdx, pftSlabPad, vPoi, minBAT );

         std::vector<sysSectionValue> minDeckPanel, maxDeckPanel;
         if ( bDeckPanels )
         {
            maxDeckPanel = pForces2->GetShear( tsrIntervalIdx, pftSlabPanel, vPoi, maxBAT );
            minDeckPanel = pForces2->GetShear( tsrIntervalIdx, pftSlabPanel, vPoi, minBAT );
         }

         std::vector<sysSectionValue> minConstruction, maxConstruction;
         if ( bConstruction )
         {
            maxConstruction = pForces2->GetShear( tsrIntervalIdx, pftConstruction, vPoi, maxBAT );
            minConstruction = pForces2->GetShear( tsrIntervalIdx, pftConstruction, vPoi, minBAT );
         }

         std::vector<sysSectionValue> minOverlay, maxOverlay;
         std::vector<sysSectionValue> minTrafficBarrier, maxTrafficBarrier;
         std::vector<sysSectionValue> minSidewalk, maxSidewalk;
         std::vector<sysSectionValue> minShearKey, maxShearKey;

         if ( bSidewalk )
         {
            maxSidewalk = pForces2->GetShear( tsrIntervalIdx, pftSidewalk, vPoi, maxBAT );
            minSidewalk = pForces2->GetShear( tsrIntervalIdx, pftSidewalk, vPoi, minBAT );
         }

         if ( bShearKey )
         {
            maxShearKey = pForces2->GetShear( tsrIntervalIdx, pftShearKey, vPoi, maxBAT );
            minShearKey = pForces2->GetShear( tsrIntervalIdx, pftShearKey, vPoi, minBAT );
         }

         maxTrafficBarrier = pForces2->GetShear( tsrIntervalIdx, pftTrafficBarrier, vPoi, maxBAT );
         minTrafficBarrier = pForces2->GetShear( tsrIntervalIdx, pftTrafficBarrier, vPoi, minBAT );
         if ( overlayIntervalIdx != INVALID_INDEX )
         {
            maxOverlay = pForces2->GetShear( tsrIntervalIdx, /*bRating && !bDesign ? pftOverlayRating : */pftOverlay, vPoi, maxBAT );
            minOverlay = pForces2->GetShear( tsrIntervalIdx, /*bRating && !bDesign ? pftOverlayRating : */pftOverlay, vPoi, minBAT );
         }

         // write out the results
         std::vector<pgsPointOfInterest>::const_iterator i(vPoi.begin());
         std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
         long index = 0;
         for ( ; i != end; i++, index++ )
         {
            const pgsPointOfInterest& poi = *i;
            const CSegmentKey& thisSegmentKey = poi.GetSegmentKey();

            ColumnIndexType col = 0;

            Float64 end_size = pBridge->GetSegmentStartEndDistance(thisSegmentKey);

            (*p_table)(row,col++) << location.SetValue( POI_ERECTED_SEGMENT, poi, end_size );
            (*p_table)(row,col++) << shear.SetValue( girder[index] );
            (*p_table)(row,col++) << shear.SetValue( diaphragm[index] );

            if ( bShearKey )
            {
               if ( analysisType == pgsTypes::Envelope )
               {
                  (*p_table)(row,col++) << shear.SetValue( maxShearKey[index] );
                  (*p_table)(row,col++) << shear.SetValue( minShearKey[index] );
               }
               else
               {
                  (*p_table)(row,col++) << shear.SetValue( maxShearKey[index] );
               }
            }

            if ( bConstruction )
            {
               if ( analysisType == pgsTypes::Envelope && continuity_interval == castDeckIntervalIdx )
               {
                  (*p_table)(row,col++) << shear.SetValue( maxConstruction[index] );
                  (*p_table)(row,col++) << shear.SetValue( minConstruction[index] );
               }
               else
               {
                  (*p_table)(row,col++) << shear.SetValue( maxConstruction[index] );
               }
            }

            if ( analysisType == pgsTypes::Envelope && continuity_interval == castDeckIntervalIdx )
            {
               (*p_table)(row,col++) << shear.SetValue( maxSlab[index] );
               (*p_table)(row,col++) << shear.SetValue( minSlab[index] );

               (*p_table)(row,col++) << shear.SetValue( maxSlabPad[index] );
               (*p_table)(row,col++) << shear.SetValue( minSlabPad[index] );
            }
            else
            {
               (*p_table)(row,col++) << shear.SetValue( maxSlab[index] );

               (*p_table)(row,col++) << shear.SetValue( maxSlabPad[index] );
            }

            if ( bDeckPanels )
            {
               if ( analysisType == pgsTypes::Envelope && continuity_interval == castDeckIntervalIdx )
               {
                  (*p_table)(row,col++) << shear.SetValue( maxDeckPanel[index] );
                  (*p_table)(row,col++) << shear.SetValue( minDeckPanel[index] );
               }
               else
               {
                  (*p_table)(row,col++) << shear.SetValue( maxDeckPanel[index] );
               }
            }

            if ( analysisType == pgsTypes::Envelope )
            {
               if ( bSidewalk )
               {
                  (*p_table)(row,col++) << shear.SetValue( maxSidewalk[index] );
                  (*p_table)(row,col++) << shear.SetValue( minSidewalk[index] );
               }

               (*p_table)(row,col++) << shear.SetValue( maxTrafficBarrier[index] );
               (*p_table)(row,col++) << shear.SetValue( minTrafficBarrier[index] );

               (*p_table)(row,col++) << shear.SetValue( maxOverlay[index] );
               (*p_table)(row,col++) << shear.SetValue( minOverlay[index] );
            }
            else
            {
               if ( bSidewalk )
               {
                  (*p_table)(row,col++) << shear.SetValue( maxSidewalk[index] );
               }

               (*p_table)(row,col++) << shear.SetValue( maxTrafficBarrier[index] );

               (*p_table)(row,col++) << shear.SetValue( maxOverlay[index] );
            }

            row++;
         } // next poi
      } // next interval
   } // next group
}

void CTSRemovalShearTable::MakeCopy(const CTSRemovalShearTable& rOther)
{
   // Add copy code here...
}

void CTSRemovalShearTable::MakeAssignment(const CTSRemovalShearTable& rOther)
{
   MakeCopy( rOther );
}

