///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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
#include <Reporting\TSRemovalMomentsTable.h>
#include <Reporting\ReportNotes.h>
#include <Reporting\ProductMomentsTable.h>

#include <PgsExt\ReportPointOfInterest.h>
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
   CTSRemovalMomentsTable
****************************************************************************/
CTSRemovalMomentsTable::CTSRemovalMomentsTable()
{
}

CTSRemovalMomentsTable::CTSRemovalMomentsTable(const CTSRemovalMomentsTable& rOther)
{
   MakeCopy(rOther);
}

CTSRemovalMomentsTable::~CTSRemovalMomentsTable()
{
}

CTSRemovalMomentsTable& CTSRemovalMomentsTable::operator= (const CTSRemovalMomentsTable& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}


void CTSRemovalMomentsTable::Build(rptChapter* pChapter,IBroker* pBroker,const CGirderKey& girderKey,pgsTypes::AnalysisType analysisType,IEAFDisplayUnits* pDisplayUnits) const
{
   // Build table
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptMomentSectionValue,     moment,   pDisplayUnits->GetMomentUnit(),     false );

   bool bConstruction, bDeckPanels, bPedLoading, bSidewalk, bShearKey, bPermit;
   bool bContinuousBeforeDeckCasting;
   GET_IFACE2(pBroker,IBridge,pBridge);
   bool bHasOverlay      = pBridge->HasOverlay();
   bool bIsFutureOverlay = pBridge->IsFutureOverlay();

   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   GroupIndexType startGroup = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
   GroupIndexType endGroup   = (girderKey.groupIndex == ALL_GROUPS ? nGroups-1 : startGroup);

   GET_IFACE2(pBroker, IIntervals, pIntervals);
   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval(girderKey);

   // Get the results
   GET_IFACE2_NOCHECK(pBroker, IRatingSpecification, pRatingSpec); // only used if there are temporary supports to be removed
   GET_IFACE2_NOCHECK(pBroker, IUserDefinedLoads, pUDL);           // only used if there are temporary supports to be removed
   GET_IFACE2_NOCHECK(pBroker,IPointOfInterest,pIPoi);             // only used if there are temporary supports to be removed
   GET_IFACE2_NOCHECK(pBroker,IProductForces2,pForces2);           // only used if there are temporary supports to be removed

   GET_IFACE2(pBroker,IProductForces,pProdForces);
   pgsTypes::BridgeAnalysisType maxBAT = pProdForces->GetBridgeAnalysisType(analysisType,pgsTypes::Maximize);
   pgsTypes::BridgeAnalysisType minBAT = pProdForces->GetBridgeAnalysisType(analysisType,pgsTypes::Minimize);

   for ( GroupIndexType grpIdx = startGroup; grpIdx <= endGroup; grpIdx++ )
   {
      // Get the intervals when temporary supports are removed for this group
      std::vector<IntervalIndexType> tsrIntervals(pIntervals->GetTemporarySupportRemovalIntervals(girderKey));

      GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
      GirderIndexType gdrIdx = Min(girderKey.girderIndex,nGirders-1);

      if ( tsrIntervals.size() == 0 )
      {
         continue; // next group
      }

      CGirderKey thisGirderKey(grpIdx,gdrIdx);

      IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval(thisGirderKey);
      IntervalIndexType overlayIntervalIdx  = pIntervals->GetOverlayInterval(thisGirderKey);

      // determine if any user defined loads where applied before the first temporary
      // support removal interval
      bool bAreThereUserLoads = false;
      IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(CSegmentKey(thisGirderKey,0));
      for ( IntervalIndexType intervalIdx = releaseIntervalIdx; intervalIdx < tsrIntervals.front(); intervalIdx++ )
      {
         if ( pUDL->DoUserLoadsExist(girderKey,intervalIdx) )
         {
            bAreThereUserLoads = true;
            break; // just need to find one instance
         }
      }

      std::vector<IntervalIndexType>::iterator iter(tsrIntervals.begin());
      std::vector<IntervalIndexType>::iterator end(tsrIntervals.end());
      for ( ; iter != end; iter++ )
      {
         IntervalIndexType tsrIntervalIdx = *iter;

         GroupIndexType startGroupIdx, endGroupIdx; // use these so we don't mess up the loop parameters
         ColumnIndexType nCols = GetProductLoadTableColumnCount(pBroker,girderKey,analysisType,false,false,false,&bConstruction,&bDeckPanels,&bSidewalk,&bShearKey,&bPedLoading,&bPermit,&bContinuousBeforeDeckCasting,&startGroupIdx,&endGroupIdx);
         bPedLoading = false;
         bPermit     = false;

         // are there user defined loads in this interval?
         if ( pUDL->DoUserLoadsExist(girderKey,tsrIntervalIdx) )
         {
            bAreThereUserLoads = true;
         }

         CString strLabel;
         strLabel.Format(_T("Moment due to removal of temporary supports in Interval %d"),LABEL_INTERVAL(tsrIntervalIdx));
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

         RowIndexType row = ConfigureProductLoadTableHeading<rptMomentUnitTag,unitmgtMomentData>(pBroker,p_table,false,false,bConstruction,bDeckPanels,bSidewalk,bShearKey,bHasOverlay,bIsFutureOverlay,false,bPedLoading,
                                                                                                 bPermit,false,analysisType,bContinuousBeforeDeckCasting,
                                                                                                 pRatingSpec,pDisplayUnits,pDisplayUnits->GetMomentUnit());

         if ( bAreThereUserLoads )
         {
            nCols += 3;
            p_table->SetNumberOfColumns(nCols);
            p_table->SetRowSpan(0,nCols-3,2);
            p_table->SetRowSpan(1,nCols-3,SKIP_CELL);
            (*p_table)(0,nCols-3) << COLHDR(_T("User DC"), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );

            p_table->SetRowSpan(0,nCols-2,2);
            p_table->SetRowSpan(1,nCols-2,SKIP_CELL);
            (*p_table)(0,nCols-2) << COLHDR(_T("User DW"), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );

            p_table->SetRowSpan(0,nCols-1,2);
            p_table->SetRowSpan(1,nCols-1,SKIP_CELL);
            (*p_table)(0,nCols-1) << COLHDR(_T("User LLIM"), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
         }

         CSegmentKey allSegmentsKey(grpIdx,gdrIdx,ALL_SEGMENTS);
         std::vector<pgsPointOfInterest> vPoi( pIPoi->GetPointsOfInterest(allSegmentsKey,POI_ERECTED_SEGMENT) );

         // Get the results for this span (it is faster to get them as a vector rather than individually)
         std::vector<Float64> girder    = pForces2->GetMoment(tsrIntervalIdx, pftGirder,    vPoi, maxBAT, rtIncremental);
         std::vector<Float64> diaphragm = pForces2->GetMoment(tsrIntervalIdx, pftDiaphragm, vPoi, maxBAT, rtIncremental);

         std::vector<Float64> minSlab, maxSlab;
         std::vector<Float64> minSlabPad, maxSlabPad;
         maxSlab = pForces2->GetMoment( tsrIntervalIdx, pftSlab, vPoi, maxBAT, rtIncremental );
         minSlab = pForces2->GetMoment( tsrIntervalIdx, pftSlab, vPoi, minBAT, rtIncremental );

         maxSlabPad = pForces2->GetMoment( tsrIntervalIdx, pftSlabPad, vPoi, maxBAT, rtIncremental );
         minSlabPad = pForces2->GetMoment( tsrIntervalIdx, pftSlabPad, vPoi, minBAT, rtIncremental );

         std::vector<Float64> minDeckPanel, maxDeckPanel;
         if ( bDeckPanels )
         {
            maxDeckPanel = pForces2->GetMoment( tsrIntervalIdx, pftSlabPanel, vPoi, maxBAT, rtIncremental );
            minDeckPanel = pForces2->GetMoment( tsrIntervalIdx, pftSlabPanel, vPoi, minBAT, rtIncremental );
         }

         std::vector<Float64> minConstruction, maxConstruction;
         if ( bConstruction )
         {
            maxConstruction = pForces2->GetMoment( tsrIntervalIdx, pftConstruction, vPoi, maxBAT, rtIncremental );
            minConstruction = pForces2->GetMoment( tsrIntervalIdx, pftConstruction, vPoi, minBAT, rtIncremental );
         }

         std::vector<Float64> minOverlay, maxOverlay;
         std::vector<Float64> minTrafficBarrier, maxTrafficBarrier;
         std::vector<Float64> minSidewalk, maxSidewalk;
         std::vector<Float64> minShearKey, maxShearKey;

         if ( bSidewalk )
         {
            maxSidewalk = pForces2->GetMoment( tsrIntervalIdx, pftSidewalk, vPoi, maxBAT, rtIncremental );
            minSidewalk = pForces2->GetMoment( tsrIntervalIdx, pftSidewalk, vPoi, minBAT, rtIncremental );
         }

         if ( bShearKey )
         {
            maxShearKey = pForces2->GetMoment( tsrIntervalIdx, pftShearKey, vPoi, maxBAT, rtIncremental );
            minShearKey = pForces2->GetMoment( tsrIntervalIdx, pftShearKey, vPoi, minBAT, rtIncremental );
         }

         maxTrafficBarrier = pForces2->GetMoment( tsrIntervalIdx, pftTrafficBarrier, vPoi, maxBAT, rtIncremental );
         minTrafficBarrier = pForces2->GetMoment( tsrIntervalIdx, pftTrafficBarrier, vPoi, minBAT, rtIncremental );
         if ( bHasOverlay )
         {
            maxOverlay = pForces2->GetMoment( tsrIntervalIdx, /*bRating && !bDesign ? pftOverlayRating : */pftOverlay, vPoi, maxBAT, rtIncremental );
            minOverlay = pForces2->GetMoment( tsrIntervalIdx, /*bRating && !bDesign ? pftOverlayRating : */pftOverlay, vPoi, minBAT, rtIncremental );
         }

         std::vector<Float64> userDC, userDW, userLLIM;
         if ( bAreThereUserLoads )
         {
            userDC   = pForces2->GetMoment(tsrIntervalIdx, pftUserDC,   vPoi, maxBAT, rtIncremental);
            userDW   = pForces2->GetMoment(tsrIntervalIdx, pftUserDW,   vPoi, maxBAT, rtIncremental);
            userLLIM = pForces2->GetMoment(tsrIntervalIdx, pftUserLLIM, vPoi, maxBAT, rtIncremental);
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

            (*p_table)(row,col++) << location.SetValue( POI_ERECTED_SEGMENT, poi );
            (*p_table)(row,col++) << moment.SetValue( girder[index] );
            (*p_table)(row,col++) << moment.SetValue( diaphragm[index] );

            if ( bShearKey )
            {
               if ( analysisType == pgsTypes::Envelope && bContinuousBeforeDeckCasting )
               {
                  (*p_table)(row,col++) << moment.SetValue( maxShearKey[index] );
                  (*p_table)(row,col++) << moment.SetValue( minShearKey[index] );
               }
               else
               {
                  (*p_table)(row,col++) << moment.SetValue( maxShearKey[index] );
               }
            }

            if ( bConstruction )
            {
               if ( analysisType == pgsTypes::Envelope && bContinuousBeforeDeckCasting )
               {
                  (*p_table)(row,col++) << moment.SetValue( maxConstruction[index] );
                  (*p_table)(row,col++) << moment.SetValue( minConstruction[index] );
               }
               else
               {
                  (*p_table)(row,col++) << moment.SetValue( maxConstruction[index] );
               }
            }

            if ( analysisType == pgsTypes::Envelope && bContinuousBeforeDeckCasting )
            {
               (*p_table)(row,col++) << moment.SetValue( maxSlab[index] );
               (*p_table)(row,col++) << moment.SetValue( minSlab[index] );

               (*p_table)(row,col++) << moment.SetValue( maxSlabPad[index] );
               (*p_table)(row,col++) << moment.SetValue( minSlabPad[index] );
            }
            else
            {
               (*p_table)(row,col++) << moment.SetValue( maxSlab[index] );

               (*p_table)(row,col++) << moment.SetValue( maxSlabPad[index] );
            }

            if ( bDeckPanels )
            {
               if ( analysisType == pgsTypes::Envelope && bContinuousBeforeDeckCasting )
               {
                  (*p_table)(row,col++) << moment.SetValue( maxDeckPanel[index] );
                  (*p_table)(row,col++) << moment.SetValue( minDeckPanel[index] );
               }
               else
               {
                  (*p_table)(row,col++) << moment.SetValue( maxDeckPanel[index] );
               }
            }

            if ( analysisType == pgsTypes::Envelope )
            {
               if ( bSidewalk )
               {
                  (*p_table)(row,col++) << moment.SetValue( maxSidewalk[index] );
                  (*p_table)(row,col++) << moment.SetValue( minSidewalk[index] );
               }

               (*p_table)(row,col++) << moment.SetValue( maxTrafficBarrier[index] );
               (*p_table)(row,col++) << moment.SetValue( minTrafficBarrier[index] );

               if ( overlayIntervalIdx != INVALID_INDEX )
               {
                  (*p_table)(row,col++) << moment.SetValue( maxOverlay[index] );
                  (*p_table)(row,col++) << moment.SetValue( minOverlay[index] );
               }
            }
            else
            {
               if ( bSidewalk )
               {
                  (*p_table)(row,col++) << moment.SetValue( maxSidewalk[index] );
               }

               (*p_table)(row,col++) << moment.SetValue( maxTrafficBarrier[index] );

               if ( overlayIntervalIdx != INVALID_INDEX )
               {
                  (*p_table)(row,col++) << moment.SetValue( maxOverlay[index] );
               }
            }

            if ( bAreThereUserLoads )
            {
               (*p_table)(row,col++) << moment.SetValue( userDC[index] );
               (*p_table)(row,col++) << moment.SetValue( userDW[index] );
               (*p_table)(row,col++) << moment.SetValue( userLLIM[index] );
            }


            row++;
         } // next poi
      } // next interval
   } // next group
}

void CTSRemovalMomentsTable::MakeCopy(const CTSRemovalMomentsTable& rOther)
{
   // Add copy code here...
}

void CTSRemovalMomentsTable::MakeAssignment(const CTSRemovalMomentsTable& rOther)
{
   MakeCopy( rOther );
}

