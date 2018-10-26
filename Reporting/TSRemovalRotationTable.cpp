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
#include <Reporting\TSRemovalRotationTable.h>
#include <Reporting\ReportNotes.h>
#include <Reporting\ProductRotationTable.h>
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
   CTSRemovalRotationTable
****************************************************************************/
CTSRemovalRotationTable::CTSRemovalRotationTable()
{
}

CTSRemovalRotationTable::CTSRemovalRotationTable(const CTSRemovalRotationTable& rOther)
{
   MakeCopy(rOther);
}

CTSRemovalRotationTable::~CTSRemovalRotationTable()
{
}

CTSRemovalRotationTable& CTSRemovalRotationTable::operator= (const CTSRemovalRotationTable& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}


void CTSRemovalRotationTable::Build(rptChapter* pChapter,IBroker* pBroker,const CGirderKey& girderKey,pgsTypes::AnalysisType analysisType,IEAFDisplayUnits* pDisplayUnits) const
{
   // Build table
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptAngleUnitValue,  rotation, pDisplayUnits->GetRadAngleUnit(), false );

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

   PierIndexType startPier = pBridge->GetGirderGroupStartPier(startGroup);
   PierIndexType endPier   = pBridge->GetGirderGroupEndPier(endGroup);


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
         strLabel.Format(_T("Rotations due to removal of temporary supports in Interval %d"),LABEL_INTERVAL(tsrIntervalIdx));
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

         RowIndexType row = ConfigureProductLoadTableHeading<rptAngleUnitTag,unitmgtAngleData>(pBroker,p_table,true,false,bConstruction,bDeckPanels,bSidewalk,bShearKey,bHasOverlay,bIsFutureOverlay,false,bPedLoading,
                                                                                               bPermit,false,analysisType,bContinuousBeforeDeckCasting,
                                                                                               pRatingSpec,pDisplayUnits,pDisplayUnits->GetAngleUnit());


         if ( bAreThereUserLoads )
         {
            nCols += 3;
            p_table->SetNumberOfColumns(nCols);
            p_table->SetRowSpan(0,nCols-3,2);
            p_table->SetRowSpan(1,nCols-3,SKIP_CELL);
            (*p_table)(0,nCols-3) << COLHDR(_T("User DC"), rptAngleUnitTag, pDisplayUnits->GetAngleUnit() );

            p_table->SetRowSpan(0,nCols-2,2);
            p_table->SetRowSpan(1,nCols-2,SKIP_CELL);
            (*p_table)(0,nCols-2) << COLHDR(_T("User DW"), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );

            p_table->SetRowSpan(0,nCols-1,2);
            p_table->SetRowSpan(1,nCols-1,SKIP_CELL);
            (*p_table)(0,nCols-1) << COLHDR(_T("User LLIM"), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
         }

         // get poi at start and end of each segment in the girder
         std::vector<pgsPointOfInterest> vPoi;
         for ( GroupIndexType grpIdx = startGroup; grpIdx <= endGroup; grpIdx++ )
         {
            SegmentIndexType nSegments = pBridge->GetSegmentCount(CGirderKey(grpIdx,girderKey.girderIndex));
            for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
            {
               CSegmentKey segmentKey(grpIdx,girderKey.girderIndex,segIdx);
               std::vector<pgsPointOfInterest> segPoi1(pIPoi->GetPointsOfInterest(segmentKey,POI_ERECTED_SEGMENT | POI_0L, POIFIND_AND));
               std::vector<pgsPointOfInterest> segPoi2(pIPoi->GetPointsOfInterest(segmentKey,POI_ERECTED_SEGMENT | POI_10L,POIFIND_AND));
               ATLASSERT(segPoi1.size() == 1);
               ATLASSERT(segPoi2.size() == 1);
               vPoi.push_back(segPoi1.front());
               vPoi.push_back(segPoi2.front());
            }
         }

         // Get the results for this span (it is faster to get them as a vector rather than individually)
         std::vector<Float64> girder    = pForces2->GetRotation(tsrIntervalIdx, pftGirder,    vPoi, maxBAT, rtIncremental, false);
         std::vector<Float64> diaphragm = pForces2->GetRotation(tsrIntervalIdx, pftDiaphragm, vPoi, maxBAT, rtIncremental, false);

         std::vector<Float64> minSlab, maxSlab;
         std::vector<Float64> minSlabPad, maxSlabPad;
         maxSlab = pForces2->GetRotation( tsrIntervalIdx, pftSlab, vPoi, maxBAT, rtIncremental, false );
         minSlab = pForces2->GetRotation( tsrIntervalIdx, pftSlab, vPoi, minBAT, rtIncremental, false );

         maxSlabPad = pForces2->GetRotation( tsrIntervalIdx, pftSlabPad, vPoi, maxBAT, rtIncremental, false );
         minSlabPad = pForces2->GetRotation( tsrIntervalIdx, pftSlabPad, vPoi, minBAT, rtIncremental, false );

         std::vector<Float64> minDeckPanel, maxDeckPanel;
         if ( bDeckPanels )
         {
            maxDeckPanel = pForces2->GetRotation( tsrIntervalIdx, pftSlabPanel, vPoi, maxBAT, rtIncremental, false );
            minDeckPanel = pForces2->GetRotation( tsrIntervalIdx, pftSlabPanel, vPoi, minBAT, rtIncremental, false );
         }

         std::vector<Float64> minConstruction, maxConstruction;
         if ( bConstruction )
         {
            maxConstruction = pForces2->GetRotation( tsrIntervalIdx, pftConstruction, vPoi, maxBAT, rtIncremental, false );
            minConstruction = pForces2->GetRotation( tsrIntervalIdx, pftConstruction, vPoi, minBAT, rtIncremental, false );
         }

         std::vector<Float64> minOverlay, maxOverlay;
         std::vector<Float64> minTrafficBarrier, maxTrafficBarrier;
         std::vector<Float64> minSidewalk, maxSidewalk;
         std::vector<Float64> minShearKey, maxShearKey;

         if ( bSidewalk )
         {
            maxSidewalk = pForces2->GetRotation( tsrIntervalIdx, pftSidewalk, vPoi, maxBAT, rtIncremental, false );
            minSidewalk = pForces2->GetRotation( tsrIntervalIdx, pftSidewalk, vPoi, minBAT, rtIncremental, false );
         }

         if ( bShearKey )
         {
            maxShearKey = pForces2->GetRotation( tsrIntervalIdx, pftShearKey, vPoi, maxBAT, rtIncremental, false );
            minShearKey = pForces2->GetRotation( tsrIntervalIdx, pftShearKey, vPoi, minBAT, rtIncremental, false );
         }

         maxTrafficBarrier = pForces2->GetRotation( tsrIntervalIdx, pftTrafficBarrier, vPoi, maxBAT, rtIncremental, false );
         minTrafficBarrier = pForces2->GetRotation( tsrIntervalIdx, pftTrafficBarrier, vPoi, minBAT, rtIncremental, false );
         if ( overlayIntervalIdx != INVALID_INDEX )
         {
            maxOverlay = pForces2->GetRotation( tsrIntervalIdx, /*bRating && !bDesign ? pftOverlayRating : */pftOverlay, vPoi, maxBAT, rtIncremental, false );
            minOverlay = pForces2->GetRotation( tsrIntervalIdx, /*bRating && !bDesign ? pftOverlayRating : */pftOverlay, vPoi, minBAT, rtIncremental, false );
         }

         std::vector<Float64> userDC, userDW, userLLIM;
         if ( bAreThereUserLoads )
         {
            userDC   = pForces2->GetRotation(tsrIntervalIdx, pftUserDC,   vPoi, maxBAT, rtIncremental, false);
            userDW   = pForces2->GetRotation(tsrIntervalIdx, pftUserDW,   vPoi, maxBAT, rtIncremental, false);
            userLLIM = pForces2->GetRotation(tsrIntervalIdx, pftUserLLIM, vPoi, maxBAT, rtIncremental, false);
         }

         // write out the results
         IndexType index = 0;
         for ( PierIndexType pier = startPier; pier <= endPier; pier++, index++)
         {
            ColumnIndexType col = 0;

            if ( pier == 0 || pier == pBridge->GetPierCount()-1 )
            {
               (*p_table)(row,col++) << _T("Abutment ") << LABEL_PIER(pier);
            }
            else
            {
               (*p_table)(row,col++) << _T("Pier ") << LABEL_PIER(pier);
            }
         
            
            const pgsPointOfInterest& poi = vPoi[index];

            const CSegmentKey& thisSegmentKey = poi.GetSegmentKey();

            Float64 end_size = pBridge->GetSegmentStartEndDistance(thisSegmentKey);

            (*p_table)(row,col++) << rotation.SetValue( girder[index] );
            (*p_table)(row,col++) << rotation.SetValue( diaphragm[index] );

            if ( bShearKey && bContinuousBeforeDeckCasting )
            {
               if ( analysisType == pgsTypes::Envelope )
               {
                  (*p_table)(row,col++) << rotation.SetValue( maxShearKey[index] );
                  (*p_table)(row,col++) << rotation.SetValue( minShearKey[index] );
               }
               else
               {
                  (*p_table)(row,col++) << rotation.SetValue( maxShearKey[index] );
               }
            }

            if ( bConstruction )
            {
               if ( analysisType == pgsTypes::Envelope && bContinuousBeforeDeckCasting )
               {
                  (*p_table)(row,col++) << rotation.SetValue( maxConstruction[index] );
                  (*p_table)(row,col++) << rotation.SetValue( minConstruction[index] );
               }
               else
               {
                  (*p_table)(row,col++) << rotation.SetValue( maxConstruction[index] );
               }
            }

            if ( analysisType == pgsTypes::Envelope && bContinuousBeforeDeckCasting )
            {
               (*p_table)(row,col++) << rotation.SetValue( maxSlab[index] );
               (*p_table)(row,col++) << rotation.SetValue( minSlab[index] );

               (*p_table)(row,col++) << rotation.SetValue( maxSlabPad[index] );
               (*p_table)(row,col++) << rotation.SetValue( minSlabPad[index] );
            }
            else
            {
               (*p_table)(row,col++) << rotation.SetValue( maxSlab[index] );

               (*p_table)(row,col++) << rotation.SetValue( maxSlabPad[index] );
            }

            if ( bDeckPanels )
            {
               if ( analysisType == pgsTypes::Envelope && bContinuousBeforeDeckCasting )
               {
                  (*p_table)(row,col++) << rotation.SetValue( maxDeckPanel[index] );
                  (*p_table)(row,col++) << rotation.SetValue( minDeckPanel[index] );
               }
               else
               {
                  (*p_table)(row,col++) << rotation.SetValue( maxDeckPanel[index] );
               }
            }

            if ( analysisType == pgsTypes::Envelope )
            {
               if ( bSidewalk )
               {
                  (*p_table)(row,col++) << rotation.SetValue( maxSidewalk[index] );
                  (*p_table)(row,col++) << rotation.SetValue( minSidewalk[index] );
               }

               (*p_table)(row,col++) << rotation.SetValue( maxTrafficBarrier[index] );
               (*p_table)(row,col++) << rotation.SetValue( minTrafficBarrier[index] );

               if ( overlayIntervalIdx != INVALID_INDEX )
               {
                  (*p_table)(row,col++) << rotation.SetValue( maxOverlay[index] );
                  (*p_table)(row,col++) << rotation.SetValue( minOverlay[index] );
               }
            }
            else
            {
               if ( bSidewalk )
               {
                  (*p_table)(row,col++) << rotation.SetValue( maxSidewalk[index] );
               }

               (*p_table)(row,col++) << rotation.SetValue( maxTrafficBarrier[index] );

               if ( overlayIntervalIdx != INVALID_INDEX )
               {
                  (*p_table)(row,col++) << rotation.SetValue( maxOverlay[index] );
               }
            }

            if ( bAreThereUserLoads )
            {
               (*p_table)(row,col++) << rotation.SetValue( userDC[index] );
               (*p_table)(row,col++) << rotation.SetValue( userDW[index] );
               (*p_table)(row,col++) << rotation.SetValue( userLLIM[index] );
            }

            row++;
         } // next poi
      } // next interval
   } // next group
}

void CTSRemovalRotationTable::MakeCopy(const CTSRemovalRotationTable& rOther)
{
   // Add copy code here...
}

void CTSRemovalRotationTable::MakeAssignment(const CTSRemovalRotationTable& rOther)
{
   MakeCopy( rOther );
}

