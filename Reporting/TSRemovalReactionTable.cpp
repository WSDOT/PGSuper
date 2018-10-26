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
#include <Reporting\TSRemovalReactionTable.h>
#include <Reporting\ReportNotes.h>
#include <Reporting\ProductReactionTable.h>
#include <Reporting\ProductMomentsTable.h>
#include <Reporting\ReactionInterfaceAdapters.h>

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
   CTSRemovalReactionTable
****************************************************************************/
CTSRemovalReactionTable::CTSRemovalReactionTable()
{
}

CTSRemovalReactionTable::CTSRemovalReactionTable(const CTSRemovalReactionTable& rOther)
{
   MakeCopy(rOther);
}

CTSRemovalReactionTable::~CTSRemovalReactionTable()
{
}

CTSRemovalReactionTable& CTSRemovalReactionTable::operator= (const CTSRemovalReactionTable& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}


void CTSRemovalReactionTable::Build(rptChapter* pChapter,IBroker* pBroker,const CGirderKey& girderKey,pgsTypes::AnalysisType analysisType,ReactionTableType tableType,IEAFDisplayUnits* pDisplayUnits) const
{
   // Build table
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptForceUnitValue,  reaction, pDisplayUnits->GetShearUnit(), false );

   bool bConstruction, bDeckPanels, bPedLoading, bSidewalk, bShearKey, bPermit;
   IntervalIndexType continuityIntervalIdx;
   GET_IFACE2(pBroker,IBridge,pBridge);
   bool bIsFutureOverlay = pBridge->IsFutureOverlay();

   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   GroupIndexType startGroup = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
   GroupIndexType endGroup   = (girderKey.groupIndex == ALL_GROUPS ? nGroups-1 : startGroup);

   GET_IFACE2(pBroker, IIntervals, pIntervals);
   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval(girderKey);
   IntervalIndexType overlayIntervalIdx  = pIntervals->GetOverlayInterval(girderKey);

   // Get the results
   GET_IFACE2_NOCHECK(pBroker, IRatingSpecification, pRatingSpec); // only used if there are temporary supports to be removed
   GET_IFACE2_NOCHECK(pBroker, IUserDefinedLoads, pUDL);           // only used if there are temporary supports to be removed
   GET_IFACE2_NOCHECK(pBroker,IPointOfInterest,pIPoi);             // only used if there are temporary supports to be removed
   GET_IFACE2_NOCHECK(pBroker,IProductForces2,pForces2);           // only used if there are temporary supports to be removed

   GET_IFACE2(pBroker,IProductForces,pProdForces);
   pgsTypes::BridgeAnalysisType maxBAT = pProdForces->GetBridgeAnalysisType(analysisType,pgsTypes::Maximize);
   pgsTypes::BridgeAnalysisType minBAT = pProdForces->GetBridgeAnalysisType(analysisType,pgsTypes::Minimize);

   PierIndexType startPierIdx = pBridge->GetGirderGroupStartPier(startGroup);
   PierIndexType endPierIdx   = pBridge->GetGirderGroupEndPier(endGroup);

   PierIndexType nPiers = pBridge->GetPierCount();

   GET_IFACE2_NOCHECK(pBroker,IReactions,pReactions);         // not always used. usage depends on reactions mode
   GET_IFACE2_NOCHECK(pBroker,IBearingDesign,pBearingDesign); // not always used. usage depends on reactions mode

   for ( GroupIndexType grpIdx = startGroup; grpIdx <= endGroup; grpIdx++ )
   {
      // Get the intervals when temporary supports are removed for this group
      std::vector<IntervalIndexType> tsrIntervals(pIntervals->GetTemporarySupportRemovalIntervals(girderKey));

      IntervalIndexType overlayIntervalIdx = pIntervals->GetOverlayInterval(girderKey);

      GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
      GirderIndexType gdrIdx = Min(girderKey.girderIndex,nGirders-1);

      if ( tsrIntervals.size() == 0 )
      {
         continue; // next group
      }

      // determine if any user defined loads where applied before the first temporary
      // support removal interval
      bool bAreThereUserLoads = false;
      IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(CSegmentKey(grpIdx,gdrIdx,0));
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
         ColumnIndexType nCols = GetProductLoadTableColumnCount(pBroker,girderKey,analysisType,false,false,&bConstruction,&bDeckPanels,&bSidewalk,&bShearKey,&bPedLoading,&bPermit,&continuityIntervalIdx,&startGroupIdx,&endGroupIdx);
         bPedLoading = false;
         bPermit     = false;

         // are there user defined loads in this interval?
         if ( pUDL->DoUserLoadsExist(girderKey,tsrIntervalIdx) )
         {
            bAreThereUserLoads = true;
         }

         std::auto_ptr<IProductReactionAdapter> pForces;
         if( tableType == PierReactionsTable )
         {
            pForces =  std::auto_ptr<ProductForcesReactionAdapter>(new ProductForcesReactionAdapter(pReactions,girderKey));
         }
         else
         {
            pForces =  std::auto_ptr<BearingDesignProductReactionAdapter>(new BearingDesignProductReactionAdapter(pBearingDesign, continuityIntervalIdx, girderKey) );
         }

         CString strLabel;
         strLabel.Format(_T("Reactions due to removal of temporary supports in Interval %d"),LABEL_INTERVAL(tsrIntervalIdx));
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

         RowIndexType row = ConfigureProductLoadTableHeading<rptForceUnitTag,unitmgtForceData>(pBroker,p_table,true,false,bConstruction,bDeckPanels,bSidewalk,bShearKey,overlayIntervalIdx != INVALID_INDEX,bIsFutureOverlay,false,bPedLoading,
                                                                                               bPermit,false,analysisType,continuityIntervalIdx,castDeckIntervalIdx,
                                                                                               pRatingSpec,pDisplayUnits,pDisplayUnits->GetShearUnit());

         if ( bAreThereUserLoads )
         {
            nCols += 3;
            p_table->SetNumberOfColumns(nCols);
            p_table->SetRowSpan(0,nCols-3,2);
            p_table->SetRowSpan(1,nCols-3,SKIP_CELL);
            (*p_table)(0,nCols-3) << COLHDR(_T("User DC"), rptForceUnitTag, pDisplayUnits->GetShearUnit() );

            p_table->SetRowSpan(0,nCols-2,2);
            p_table->SetRowSpan(1,nCols-2,SKIP_CELL);
            (*p_table)(0,nCols-2) << COLHDR(_T("User DW"), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );

            p_table->SetRowSpan(0,nCols-1,2);
            p_table->SetRowSpan(1,nCols-1,SKIP_CELL);
            (*p_table)(0,nCols-1) << COLHDR(_T("User LLIM"), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
         }

         // write out the results
         IndexType index = 0;
         for ( PierIndexType pierIdx = startPierIdx; pierIdx <= endPierIdx; pierIdx++, index++)
         {
            if (!pForces->DoReportAtPier(pierIdx, girderKey))
            {
               // Don't report pier if information is not available
               continue;
            }

            ColumnIndexType col = 0;

            if ( pierIdx == 0 || pierIdx == nPiers-1 )
            {
               (*p_table)(row,col++) << _T("Abutment ") << LABEL_PIER(pierIdx);
            }
            else
            {
               (*p_table)(row,col++) << _T("Pier ") << LABEL_PIER(pierIdx);
            }

            (*p_table)(row,col++) << reaction.SetValue( pReactions->GetReaction(girderKey,pierIdx,pgsTypes::stPier,tsrIntervalIdx,pftGirder,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, rtIncremental) );
            (*p_table)(row,col++) << reaction.SetValue( pReactions->GetReaction(girderKey,pierIdx,pgsTypes::stPier,tsrIntervalIdx,pftDiaphragm,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, rtIncremental) );

            if ( bShearKey )
            {
               (*p_table)(row,col++) << reaction.SetValue( pReactions->GetReaction(girderKey,pierIdx,pgsTypes::stPier,tsrIntervalIdx,pftShearKey,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, rtIncremental) );
            }

            if ( bConstruction )
            {
               (*p_table)(row,col++) << reaction.SetValue( pReactions->GetReaction(girderKey,pierIdx,pgsTypes::stPier,tsrIntervalIdx,pftConstruction,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, rtIncremental) );
            }

            (*p_table)(row,col++) << reaction.SetValue( pReactions->GetReaction(girderKey,pierIdx,pgsTypes::stPier,tsrIntervalIdx,pftSlab,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, rtIncremental) );
            (*p_table)(row,col++) << reaction.SetValue( pReactions->GetReaction(girderKey,pierIdx,pgsTypes::stPier,tsrIntervalIdx,pftSlabPad,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, rtIncremental) );

            if ( bDeckPanels )
            {
               (*p_table)(row,col++) << reaction.SetValue( pReactions->GetReaction(girderKey,pierIdx,pgsTypes::stPier,tsrIntervalIdx,pftSlabPanel,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, rtIncremental) );
            }


            if ( bSidewalk )
            {
               (*p_table)(row,col++) << reaction.SetValue( pReactions->GetReaction(girderKey,pierIdx,pgsTypes::stPier,tsrIntervalIdx,pftSidewalk,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, rtIncremental) );
            }

            (*p_table)(row,col++) << reaction.SetValue( pReactions->GetReaction(girderKey,pierIdx,pgsTypes::stPier,tsrIntervalIdx,pftTrafficBarrier,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, rtIncremental) );

            if ( overlayIntervalIdx != INVALID_INDEX )
            {
               (*p_table)(row,col++) << reaction.SetValue( pReactions->GetReaction(girderKey,pierIdx,pgsTypes::stPier,tsrIntervalIdx,pftOverlay,       analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, rtIncremental) );
            }

            if ( bAreThereUserLoads )
            {
               (*p_table)(row,col++) << reaction.SetValue( pReactions->GetReaction(girderKey,pierIdx,pgsTypes::stPier,tsrIntervalIdx,pftUserDC,  analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, rtIncremental) );
               (*p_table)(row,col++) << reaction.SetValue( pReactions->GetReaction(girderKey,pierIdx,pgsTypes::stPier,tsrIntervalIdx,pftUserDW,  analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, rtIncremental) );
               (*p_table)(row,col++) << reaction.SetValue( pReactions->GetReaction(girderKey,pierIdx,pgsTypes::stPier,tsrIntervalIdx,pftUserLLIM,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, rtIncremental) );
            }

            row++;
         } // next pier
      } // next interval
   } // next group
}

void CTSRemovalReactionTable::MakeCopy(const CTSRemovalReactionTable& rOther)
{
   // Add copy code here...
}

void CTSRemovalReactionTable::MakeAssignment(const CTSRemovalReactionTable& rOther)
{
   MakeCopy( rOther );
}

