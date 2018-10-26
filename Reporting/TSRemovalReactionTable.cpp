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
#include <Reporting\TSRemovalReactionTable.h>
#include <Reporting\ReportNotes.h>
#include <Reporting\ProductReactionTable.h>
#include <Reporting\ProductMomentsTable.h>
#include <Reporting\ReactionInterfaceAdapters.h>

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

   PierIndexType startPier = pBridge->GetGirderGroupStartPier(startGroup);
   PierIndexType endPier   = pBridge->GetGirderGroupEndPier(endGroup);

   PierIndexType nPiers = pBridge->GetPierCount();

   GET_IFACE2(pBroker,IProductForces,pProductForces);
   GET_IFACE2(pBroker,IBearingDesign,pBearingDesign);

   std::set<IntervalIndexType> tsrIntervals;
   for ( GroupIndexType grpIdx = startGroup; grpIdx <= endGroup; grpIdx++ )
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
      GirderIndexType gdrIdx = min(girderKey.girderIndex,nGirders-1);

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

#pragma Reminder("UPDATE: can this be re-worked to use RDP's ReactionLocationIterator?")

      std::set<IntervalIndexType>::iterator iter(tsrIntervals.begin());
      std::set<IntervalIndexType>::iterator end(tsrIntervals.end());
      for ( ; iter != end; iter++ )
      {
         IntervalIndexType tsrIntervalIdx = *iter;

         ColumnIndexType nCols = GetProductLoadTableColumnCount(pBroker,girderKey,analysisType,false,false,&bConstruction,&bDeckPanels,&bSidewalk,&bShearKey,&bPedLoading,&bPermit,&continuity_interval,&startGroup,&nGroups);
         bPedLoading = false;
         bPermit     = false;

         std::auto_ptr<IProductReactionAdapter> pForces;
         if( tableType == PierReactionsTable )
         {
            pForces =  std::auto_ptr<ProductForcesReactionAdapter>(new ProductForcesReactionAdapter(pProductForces,girderKey));
         }
         else
         {
            pForces =  std::auto_ptr<BearingDesignProductReactionAdapter>(new BearingDesignProductReactionAdapter(pBearingDesign, continuity_interval, girderKey) );
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

         RowIndexType row = ConfigureProductLoadTableHeading<rptForceUnitTag,unitmgtForceData>(pBroker,p_table,true,false,bConstruction,bDeckPanels,bSidewalk,bShearKey,bIsFutureOverlay,false,bPedLoading,
                                                                                               bPermit,false,analysisType,continuity_interval,
                                                                                               pRatingSpec,pDisplayUnits,pDisplayUnits->GetShearUnit());
         // write out the results
         IndexType index = 0;
         for ( PierIndexType pier = startPier; pier <= endPier; pier++, index++)
         {
            if (!pForces->DoReportAtPier(pier, girderKey))
            {
               // Don't report pier if information is not available
               continue;
            }

            ColumnIndexType col = 0;

            if ( pier == 0 || pier == nPiers-1 )
               (*p_table)(row,col++) << _T("Abutment ") << LABEL_PIER(pier);
            else
               (*p_table)(row,col++) << _T("Pier ") << LABEL_PIER(pier);
         
            (*p_table)(row,col++) << reaction.SetValue( pProdForces->GetReaction( tsrIntervalIdx, pftGirder,    pier, girderKey, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan ) );
            (*p_table)(row,col++) << reaction.SetValue( pProdForces->GetReaction( tsrIntervalIdx, pftDiaphragm, pier, girderKey, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan ) );

            if ( bShearKey )
            {
               (*p_table)(row,col++) << reaction.SetValue( pProdForces->GetReaction( tsrIntervalIdx, pftShearKey, pier, girderKey, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan ) );
            }

            if ( bConstruction )
            {
               (*p_table)(row,col++) << reaction.SetValue( pProdForces->GetReaction( tsrIntervalIdx, pftConstruction,   pier, girderKey, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan ) );
            }

            (*p_table)(row,col++) << reaction.SetValue( pProdForces->GetReaction( tsrIntervalIdx, pftSlab, pier, girderKey, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan  ) );
            (*p_table)(row,col++) << reaction.SetValue( pProdForces->GetReaction( tsrIntervalIdx, pftSlabPad, pier, girderKey, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan  ) );

            if ( bDeckPanels )
            {
               (*p_table)(row,col++) << reaction.SetValue( pProdForces->GetReaction( tsrIntervalIdx, pftSlabPanel,   pier, girderKey, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan ) );
            }


            if ( bSidewalk )
            {
               (*p_table)(row,col++) << reaction.SetValue( pProdForces->GetReaction( tsrIntervalIdx, pftSidewalk, pier, girderKey, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan ) );
            }

            (*p_table)(row,col++) << reaction.SetValue( pProdForces->GetReaction( tsrIntervalIdx, pftTrafficBarrier, pier, girderKey, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan ) );
            (*p_table)(row,col++) << reaction.SetValue( pProdForces->GetReaction( tsrIntervalIdx, pftOverlay,        pier, girderKey, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan ) );

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

