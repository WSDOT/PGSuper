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
#include <Reporting\TSRemovalStressTable.h>
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
   CTSRemovalStressTable
****************************************************************************/
CTSRemovalStressTable::CTSRemovalStressTable()
{
}

CTSRemovalStressTable::CTSRemovalStressTable(const CTSRemovalStressTable& rOther)
{
   MakeCopy(rOther);
}

CTSRemovalStressTable::~CTSRemovalStressTable()
{
}

CTSRemovalStressTable& CTSRemovalStressTable::operator= (const CTSRemovalStressTable& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}


void CTSRemovalStressTable::Build(rptChapter* pChapter,IBroker* pBroker,const CGirderKey& girderKey,pgsTypes::AnalysisType analysisType,IEAFDisplayUnits* pDisplayUnits,bool bGirderStresses) const
{
   pgsTypes::StressLocation topLocation = (bGirderStresses ? pgsTypes::TopGirder    : pgsTypes::TopDeck);
   pgsTypes::StressLocation botLocation = (bGirderStresses ? pgsTypes::BottomGirder : pgsTypes::BottomDeck);

   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), false );

   location.IncludeSpanAndGirder(girderKey.groupIndex == ALL_GROUPS ? true : false);

   bool bConstruction, bDeckPanels, bPedLoading, bSidewalk, bShearKey, bPermit;
   bool bContinuousBeforeDeckCasting;
   GET_IFACE2(pBroker,IBridge,pBridge);
   bool bHasOverlay      = pBridge->HasOverlay();
   bool bIsFutureOverlay = pBridge->IsFutureOverlay();

   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   GroupIndexType startGroup = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
   GroupIndexType endGroup   = (girderKey.groupIndex == ALL_GROUPS ? nGroups-1 : startGroup);

   GET_IFACE2(pBroker, IIntervals, pIntervals);

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
      GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
      GirderIndexType gdrIdx = Min(girderKey.girderIndex,nGirders-1);
      CGirderKey thisGirderKey(grpIdx,gdrIdx);

      IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval();
      IntervalIndexType overlayIntervalIdx  = pIntervals->GetOverlayInterval();

      // Get the intervals when temporary supports are removed for this group
      std::vector<IntervalIndexType> tsrIntervals(pIntervals->GetTemporarySupportRemovalIntervals(grpIdx));

      // if we are writing out stresses in the deck, don't report on any interval
      // that occurs before the deck is composite
      if ( !bGirderStresses )
      {
         IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval();
         tsrIntervals.erase(std::remove_if(tsrIntervals.begin(),tsrIntervals.end(),std::bind2nd(std::less<IntervalIndexType>(),compositeDeckIntervalIdx)),tsrIntervals.end());
      }

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
         ColumnIndexType nCols = GetProductLoadTableColumnCount(pBroker,girderKey,analysisType,false,false,false,&bConstruction,&bDeckPanels,&bSidewalk,&bShearKey,&bPedLoading,&bPermit,&bContinuousBeforeDeckCasting,&startGroupIdx,&endGroupIdx);
         bPedLoading = false;
         bPermit     = false;

         // are there user defined loads in this interval?
         if ( pUDL->DoUserLoadsExist(girderKey,tsrIntervalIdx) )
         {
            bAreThereUserLoads = true;
         }

         CString strLabel;
         strLabel.Format(_T("%s Stresses due to removal of temporary supports in Interval %d"),(bGirderStresses ? _T("Girder") : _T("Deck")),LABEL_INTERVAL(tsrIntervalIdx));
         rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(nCols,strLabel);

         rptParagraph* p = new rptParagraph;
         *pChapter << p;
         *p << p_table << rptNewLine;

         if ( girderKey.groupIndex == ALL_GROUPS )
         {
            p_table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
            p_table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
         }

         RowIndexType row = ConfigureProductLoadTableHeading<rptStressUnitTag,unitmgtStressData>(pBroker,p_table,false,false,bConstruction,bDeckPanels,bSidewalk,bShearKey,bHasOverlay,bIsFutureOverlay,false,bPedLoading,
                                                                                                 bPermit,false,analysisType,bContinuousBeforeDeckCasting,
                                                                                                 pRatingSpec,pDisplayUnits,pDisplayUnits->GetStressUnit());


         if ( bAreThereUserLoads )
         {
            nCols += 3;
            p_table->SetNumberOfColumns(nCols);
            p_table->SetRowSpan(0,nCols-3,2);
            p_table->SetRowSpan(1,nCols-3,SKIP_CELL);
            (*p_table)(0,nCols-3) << COLHDR(_T("User DC"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );

            p_table->SetRowSpan(0,nCols-2,2);
            p_table->SetRowSpan(1,nCols-2,SKIP_CELL);
            (*p_table)(0,nCols-2) << COLHDR(_T("User DW"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );

            p_table->SetRowSpan(0,nCols-1,2);
            p_table->SetRowSpan(1,nCols-1,SKIP_CELL);
            (*p_table)(0,nCols-1) << COLHDR(_T("User LLIM"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
         }

         CSegmentKey allSegmentsKey(grpIdx,gdrIdx,ALL_SEGMENTS);
         std::vector<pgsPointOfInterest> vPoi( pIPoi->GetPointsOfInterest(allSegmentsKey,POI_ERECTED_SEGMENT) );

         // Get the results for this span (it is faster to get them as a vector rather than individually)
         std::vector<Float64> fTopGirder, fBotGirder;
         std::vector<Float64> fTopDiaphragm, fBotDiaphragm;
         pForces2->GetStress(tsrIntervalIdx, pftGirder,    vPoi, maxBAT, rtIncremental, topLocation, botLocation, &fTopGirder, &fBotGirder);
         pForces2->GetStress(tsrIntervalIdx, pftDiaphragm, vPoi, maxBAT, rtIncremental, topLocation, botLocation, &fTopDiaphragm, &fBotDiaphragm);

         std::vector<Float64> fTopMaxSlab, fTopMinSlab, fBotMaxSlab, fBotMinSlab;
         pForces2->GetStress( tsrIntervalIdx, pftSlab, vPoi, maxBAT, rtIncremental, topLocation, botLocation, &fTopMaxSlab, &fBotMaxSlab );
         pForces2->GetStress( tsrIntervalIdx, pftSlab, vPoi, minBAT, rtIncremental, topLocation, botLocation, &fTopMinSlab, &fBotMinSlab );

         std::vector<Float64> fTopMaxSlabPad, fTopMinSlabPad, fBotMaxSlabPad, fBotMinSlabPad;
         pForces2->GetStress( tsrIntervalIdx, pftSlabPad, vPoi, maxBAT, rtIncremental, topLocation, botLocation, &fTopMaxSlabPad, &fBotMaxSlabPad );
         pForces2->GetStress( tsrIntervalIdx, pftSlabPad, vPoi, minBAT, rtIncremental, topLocation, botLocation, &fTopMinSlabPad, &fBotMinSlabPad );

         std::vector<Float64> fTopMaxSlabPanel, fTopMinSlabPanel, fBotMaxSlabPanel, fBotMinSlabPanel;
         if ( bDeckPanels )
         {
            pForces2->GetStress( tsrIntervalIdx, pftSlabPanel, vPoi, maxBAT, rtIncremental, topLocation, botLocation, &fTopMaxSlabPanel, &fBotMaxSlabPanel );
            pForces2->GetStress( tsrIntervalIdx, pftSlabPanel, vPoi, minBAT, rtIncremental, topLocation, botLocation, &fTopMinSlabPanel, &fBotMinSlabPanel );
         }

         std::vector<Float64> fTopMaxConstruction, fTopMinConstruction, fBotMaxConstruction, fBotMinConstruction;
         if ( bConstruction )
         {
            pForces2->GetStress( tsrIntervalIdx, pftConstruction, vPoi, maxBAT, rtIncremental, topLocation, botLocation, &fTopMaxConstruction, &fBotMaxConstruction );
            pForces2->GetStress( tsrIntervalIdx, pftConstruction, vPoi, minBAT, rtIncremental, topLocation, botLocation, &fTopMinConstruction, &fBotMinConstruction );
         }

         std::vector<Float64> fTopMaxSidewalk, fTopMinSidewalk, fBotMaxSidewalk, fBotMinSidewalk;
         if ( bSidewalk )
         {
            pForces2->GetStress( tsrIntervalIdx, pftSidewalk, vPoi, maxBAT, rtIncremental, topLocation, botLocation, &fTopMaxSidewalk, &fBotMaxSidewalk );
            pForces2->GetStress( tsrIntervalIdx, pftSidewalk, vPoi, minBAT, rtIncremental, topLocation, botLocation, &fTopMinSidewalk, &fBotMinSidewalk );
         }

         std::vector<Float64> fTopMaxShearKey, fTopMinShearKey, fBotMaxShearKey, fBotMinShearKey;
         if ( bShearKey )
         {
            pForces2->GetStress( tsrIntervalIdx, pftShearKey, vPoi, maxBAT, rtIncremental, topLocation, botLocation, &fTopMaxShearKey, &fBotMaxShearKey );
            pForces2->GetStress( tsrIntervalIdx, pftShearKey, vPoi, minBAT, rtIncremental, topLocation, botLocation, &fTopMinShearKey, &fBotMinShearKey );
         }


         std::vector<Float64> fTopMaxTrafficBarrier, fTopMinTrafficBarrier, fBotMaxTrafficBarrier, fBotMinTrafficBarrier;
         pForces2->GetStress( tsrIntervalIdx, pftTrafficBarrier, vPoi, maxBAT, rtIncremental, topLocation, botLocation, &fTopMaxTrafficBarrier, &fBotMaxTrafficBarrier );
         pForces2->GetStress( tsrIntervalIdx, pftTrafficBarrier, vPoi, minBAT, rtIncremental, topLocation, botLocation, &fTopMinTrafficBarrier, &fBotMinTrafficBarrier );

         std::vector<Float64> fTopMaxOverlay, fTopMinOverlay, fBotMaxOverlay, fBotMinOverlay;
         if ( bHasOverlay )
         {
            pForces2->GetStress( tsrIntervalIdx, /*bRating && !bDesign ? pftOverlayRating : */pftOverlay, vPoi, maxBAT, rtIncremental, topLocation, botLocation, &fTopMaxOverlay, &fBotMaxOverlay );
            pForces2->GetStress( tsrIntervalIdx, /*bRating && !bDesign ? pftOverlayRating : */pftOverlay, vPoi, minBAT, rtIncremental, topLocation, botLocation, &fTopMinOverlay, &fBotMinOverlay );
         }

         std::vector<Float64> fTopUserDC, fTopUserDW, fTopUserLLIM;
         std::vector<Float64> fBotUserDC, fBotUserDW, fBotUserLLIM;
         if ( bAreThereUserLoads )
         {
            pForces2->GetStress( tsrIntervalIdx, pftUserDC, vPoi, maxBAT, rtIncremental, topLocation, botLocation, &fTopUserDC, &fBotUserDC );
            pForces2->GetStress( tsrIntervalIdx, pftUserDW, vPoi, maxBAT, rtIncremental, topLocation, botLocation, &fTopUserDW, &fBotUserDW );
            pForces2->GetStress( tsrIntervalIdx, pftUserLLIM, vPoi, maxBAT, rtIncremental, topLocation, botLocation, &fTopUserLLIM, &fBotUserLLIM );
         }

         // write out the results
         std::vector<pgsPointOfInterest>::const_iterator i(vPoi.begin());
         std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
         IndexType index = 0;
         for ( ; i != end; i++, index++ )
         {
            const pgsPointOfInterest& poi = *i;
            const CSegmentKey& thisSegmentKey = poi.GetSegmentKey();

            ColumnIndexType col = 0;

            (*p_table)(row,col++) << location.SetValue( POI_ERECTED_SEGMENT, poi );

            (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopGirder[index]) << rptNewLine;
            (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotGirder[index]);
            col++;

            (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopDiaphragm[index]) << rptNewLine;
            (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotDiaphragm[index]);
            col++;

            if ( bShearKey )
            {
               if ( analysisType == pgsTypes::Envelope && bContinuousBeforeDeckCasting )
               {
                  (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMaxShearKey[index]) << rptNewLine;
                  (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMaxShearKey[index]);
                  col++;

                  (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMinShearKey[index]) << rptNewLine;
                  (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMinShearKey[index]);
                  col++;
               }
               else
               {
                  (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMaxShearKey[index]) << rptNewLine;
                  (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMaxShearKey[index]);
                  col++;
               }
            }

            if ( bConstruction )
            {
               if ( analysisType == pgsTypes::Envelope && bContinuousBeforeDeckCasting )
               {
                  (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMaxConstruction[index]) << rptNewLine;
                  (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMaxConstruction[index]);
                  col++;

                  (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMinConstruction[index]) << rptNewLine;
                  (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMinConstruction[index]);
                  col++;
               }
               else
               {
                  (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMaxConstruction[index]) << rptNewLine;
                  (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMaxConstruction[index]);
                  col++;
               }
            }

            if ( analysisType == pgsTypes::Envelope && bContinuousBeforeDeckCasting )
            {
               (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMaxSlab[index]) << rptNewLine;
               (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMaxSlab[index]);
               col++;

               (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMinSlab[index]) << rptNewLine;
               (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMinSlab[index]);
               col++;

               (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMaxSlabPad[index]) << rptNewLine;
               (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMaxSlabPad[index]);
               col++;

               (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMinSlabPad[index]) << rptNewLine;
               (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMinSlabPad[index]);
               col++;
            }
            else
            {
               (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMaxSlab[index]) << rptNewLine;
               (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMaxSlab[index]);
               col++;

               (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMaxSlabPad[index]) << rptNewLine;
               (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMaxSlabPad[index]);
               col++;
            }

            if ( bDeckPanels )
            {
               if ( analysisType == pgsTypes::Envelope && bContinuousBeforeDeckCasting )
               {
                  (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMaxSlabPanel[index]) << rptNewLine;
                  (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMaxSlabPanel[index]);
                  col++;

                  (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMinSlabPanel[index]) << rptNewLine;
                  (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMinSlabPanel[index]);
                  col++;
               }
               else
               {
                  (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMaxSlabPanel[index]) << rptNewLine;
                  (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMaxSlabPanel[index]);
                  col++;
               }
            }

            if ( analysisType == pgsTypes::Envelope )
            {
               if ( bSidewalk )
               {
                  (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMaxSidewalk[index]) << rptNewLine;
                  (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMaxSidewalk[index]);
                  col++;

                  (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMinSidewalk[index]) << rptNewLine;
                  (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMinSidewalk[index]);
                  col++;
               }

               (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMaxTrafficBarrier[index]) << rptNewLine;
               (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMaxTrafficBarrier[index]);
               col++;

               (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMinTrafficBarrier[index]) << rptNewLine;
               (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMinTrafficBarrier[index]);
               col++;

               if ( overlayIntervalIdx != INVALID_INDEX )
               {
                  (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMaxOverlay[index]) << rptNewLine;
                  (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMaxOverlay[index]);
                  col++;

                  (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMinOverlay[index]) << rptNewLine;
                  (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMinOverlay[index]);
                  col++;
               }
            }
            else
            {
               if ( bSidewalk )
               {
                  (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMaxSidewalk[index]) << rptNewLine;
                  (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMaxSidewalk[index]);
                  col++;
               }

               (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMaxTrafficBarrier[index]) << rptNewLine;
               (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMaxTrafficBarrier[index]);
               col++;

               if ( overlayIntervalIdx != INVALID_INDEX )
               {
                  (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMaxOverlay[index]) << rptNewLine;
                  (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMaxOverlay[index]);
                  col++;
               }
            }

            if ( bAreThereUserLoads )
            {
               (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue( fTopUserDC[index] ) << rptNewLine;
               (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue( fTopUserDC[index] );
               col++;

               (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue( fTopUserDW[index] ) << rptNewLine;
               (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue( fTopUserDW[index] );
               col++;

               (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue( fTopUserLLIM[index] ) << rptNewLine;
               (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue( fTopUserLLIM[index] );
               col++;
            }

            row++;
         } // next poi
      } // next interval
   } // next group
}

void CTSRemovalStressTable::MakeCopy(const CTSRemovalStressTable& rOther)
{
   // Add copy code here...
}

void CTSRemovalStressTable::MakeAssignment(const CTSRemovalStressTable& rOther)
{
   MakeCopy( rOther );
}

