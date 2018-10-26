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
#include <Reporting\ProductRotationTable.h>
#include <Reporting\ProductMomentsTable.h>
#include <Reporting\ReactionInterfaceAdapters.h>

#include <IFace\Bridge.h>

#include <IFace\AnalysisResults.h>
#include <IFace\Project.h>
#include <IFace\RatingSpecification.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CProductRotationTable
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CProductRotationTable::CProductRotationTable()
{
}

CProductRotationTable::CProductRotationTable(const CProductRotationTable& rOther)
{
   MakeCopy(rOther);
}

CProductRotationTable::~CProductRotationTable()
{
}

//======================== OPERATORS  =======================================
CProductRotationTable& CProductRotationTable::operator= (const CProductRotationTable& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
rptRcTable* CProductRotationTable::Build(IBroker* pBroker,const CGirderKey& girderKey,pgsTypes::AnalysisType analysisType,
                                         bool bIncludeImpact, bool bIncludeLLDF,bool bDesign,bool bRating,bool bIndicateControllingLoad,IEAFDisplayUnits* pDisplayUnits) const
{
   // Build table
   INIT_UV_PROTOTYPE( rptLengthUnitValue, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptAngleUnitValue,  rotation, pDisplayUnits->GetRadAngleUnit(), false );

   bool bConstruction, bDeckPanels, bPedLoading, bSidewalk, bShearKey, bPermit;
   GroupIndexType startGroup, nGroups;
   IntervalIndexType continityIntervalIdx;

   GET_IFACE2(pBroker, IRatingSpecification, pRatingSpec);
   GET_IFACE2(pBroker,IPointOfInterest,pPOI);
   GET_IFACE2(pBroker,IBridge,pBridge);
   PierIndexType nPiers = pBridge->GetPierCount();

   ColumnIndexType nCols = GetProductLoadTableColumnCount(pBroker,girderKey,analysisType,bDesign,bRating,&bConstruction,&bDeckPanels,&bSidewalk,&bShearKey,&bPedLoading,&bPermit,&continityIntervalIdx,&startGroup,&nGroups);

   GroupIndexType endGroup = startGroup + nGroups - 1;

   PierIndexType startPier = pBridge->GetGirderGroupStartPier(startGroup);
   PierIndexType endPier   = pBridge->GetGirderGroupEndPier(endGroup);

   rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(nCols,_T("Rotations"));
   RowIndexType row = ConfigureProductLoadTableHeading<rptAngleUnitTag,unitmgtAngleData>(pBroker,p_table,true,false,bConstruction,bDeckPanels,bSidewalk,bShearKey,bDesign,bPedLoading,bPermit,bRating,analysisType,continityIntervalIdx,pRatingSpec,pDisplayUnits,pDisplayUnits->GetRadAngleUnit());


   // get poi at start and end of each segment in the girder
   std::vector<pgsPointOfInterest> vPoi;
   for ( GroupIndexType grpIdx = startGroup; grpIdx <= endGroup; grpIdx++ )
   {
      SegmentIndexType nSegments = pBridge->GetSegmentCount(CGirderKey(grpIdx,girderKey.girderIndex));
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         CSegmentKey segmentKey(grpIdx,girderKey.girderIndex,segIdx);
         std::vector<pgsPointOfInterest> segPoi1(pPOI->GetPointsOfInterest(segmentKey,POI_ERECTED_SEGMENT | POI_0L, POIFIND_AND));
         std::vector<pgsPointOfInterest> segPoi2(pPOI->GetPointsOfInterest(segmentKey,POI_ERECTED_SEGMENT | POI_10L,POIFIND_AND));
         ATLASSERT(segPoi1.size() == 1);
         ATLASSERT(segPoi2.size() == 1);
         vPoi.push_back(segPoi1.front());
         vPoi.push_back(segPoi2.front());
      }
   }

   GET_IFACE2(pBroker,IProductLoads,pLoads);
   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType castDeckIntervalIdx      = pIntervals->GetCastDeckInterval();
   IntervalIndexType railingSystemIntervalIdx = pIntervals->GetRailingSystemInterval();
   IntervalIndexType liveLoadIntervalIdx      = pIntervals->GetLiveLoadInterval();
   IntervalIndexType overlayIntervalIdx       = pIntervals->GetOverlayInterval();

   // TRICKY: use adapter class to get correct reaction interfaces
   GET_IFACE2(pBroker,IBearingDesign,pBearingDesign);
   std::auto_ptr<IProductReactionAdapter> pForces = std::auto_ptr<BearingDesignProductReactionAdapter>(new BearingDesignProductReactionAdapter(pBearingDesign, startPier, endPier) );

   // Fill up the table
   GET_IFACE2(pBroker,IProductForces,pProductForces);
   pgsTypes::BridgeAnalysisType maxBAT = pProductForces->GetBridgeAnalysisType(pgsTypes::Maximize);
   pgsTypes::BridgeAnalysisType minBAT = pProductForces->GetBridgeAnalysisType(pgsTypes::Minimize);
   for ( PierIndexType pier = startPier; pier <= endPier; pier++ )
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
   
      
      pgsPointOfInterest& poi = vPoi[pier-startPier];
      IntervalIndexType erectSegmentIntervalIdx = pIntervals->GetErectSegmentInterval(poi.GetSegmentKey());

      (*p_table)(row,col++) << rotation.SetValue( pProductForces->GetRotation(erectSegmentIntervalIdx, pftGirder, poi, maxBAT) );
      (*p_table)(row,col++) << rotation.SetValue( pProductForces->GetRotation(castDeckIntervalIdx, pftDiaphragm, poi, maxBAT) );

      if ( bShearKey )
      {
         if ( analysisType == pgsTypes::Envelope )
         {
            (*p_table)(row,col++) << rotation.SetValue( pProductForces->GetRotation( castDeckIntervalIdx, pftShearKey, poi, maxBAT ) );
            (*p_table)(row,col++) << rotation.SetValue( pProductForces->GetRotation( castDeckIntervalIdx, pftShearKey, poi, minBAT ) );
         }
         else
         {
            (*p_table)(row,col++) << rotation.SetValue( pProductForces->GetRotation( castDeckIntervalIdx, pftShearKey, poi, maxBAT ) );
         }
      }


      if ( bConstruction )
      {
         if ( analysisType == pgsTypes::Envelope && continityIntervalIdx == castDeckIntervalIdx )
         {
            (*p_table)(row,col++) << rotation.SetValue( pProductForces->GetRotation( castDeckIntervalIdx, pftConstruction,   poi, maxBAT ) );
            (*p_table)(row,col++) << rotation.SetValue( pProductForces->GetRotation( castDeckIntervalIdx, pftConstruction,   poi, minBAT ) );
         }
         else
         {
            (*p_table)(row,col++) << rotation.SetValue( pProductForces->GetRotation( castDeckIntervalIdx, pftConstruction,   poi, maxBAT ) );
         }
      }

      if ( analysisType == pgsTypes::Envelope && continityIntervalIdx == castDeckIntervalIdx )
      {
         (*p_table)(row,col++) << rotation.SetValue( pProductForces->GetRotation( castDeckIntervalIdx, pftSlab,  poi, maxBAT ) );
         (*p_table)(row,col++) << rotation.SetValue( pProductForces->GetRotation( castDeckIntervalIdx, pftSlab,  poi, minBAT ) );

         (*p_table)(row,col++) << rotation.SetValue( pProductForces->GetRotation( castDeckIntervalIdx, pftSlabPad,  poi, maxBAT ) );
         (*p_table)(row,col++) << rotation.SetValue( pProductForces->GetRotation( castDeckIntervalIdx, pftSlabPad,  poi, minBAT ) );
      }
      else
      {
         (*p_table)(row,col++) << rotation.SetValue( pProductForces->GetRotation( castDeckIntervalIdx, pftSlab,  poi, maxBAT  ) );

         (*p_table)(row,col++) << rotation.SetValue( pProductForces->GetRotation( castDeckIntervalIdx, pftSlabPad,  poi, maxBAT  ) );
      }

      if ( bDeckPanels )
      {
         if ( analysisType == pgsTypes::Envelope && continityIntervalIdx == castDeckIntervalIdx )
         {
            (*p_table)(row,col++) << rotation.SetValue( pProductForces->GetRotation( castDeckIntervalIdx, pftSlabPanel,   poi, maxBAT ) );
            (*p_table)(row,col++) << rotation.SetValue( pProductForces->GetRotation( castDeckIntervalIdx, pftSlabPanel,   poi, minBAT ) );
         }
         else
         {
            (*p_table)(row,col++) << rotation.SetValue( pProductForces->GetRotation( castDeckIntervalIdx, pftSlabPanel,   poi, maxBAT ) );
         }
      }

      if ( analysisType == pgsTypes::Envelope )
      {
         if ( bSidewalk )
         {
            (*p_table)(row,col++) << rotation.SetValue( pProductForces->GetRotation( railingSystemIntervalIdx, pftSidewalk, poi, maxBAT ) );
            (*p_table)(row,col++) << rotation.SetValue( pProductForces->GetRotation( railingSystemIntervalIdx, pftSidewalk, poi, minBAT ) );
         }

         (*p_table)(row,col++) << rotation.SetValue( pProductForces->GetRotation( railingSystemIntervalIdx, pftTrafficBarrier, poi, maxBAT ) );
         (*p_table)(row,col++) << rotation.SetValue( pProductForces->GetRotation( railingSystemIntervalIdx, pftTrafficBarrier, poi, minBAT ) );
         (*p_table)(row,col++) << rotation.SetValue( pProductForces->GetRotation( overlayIntervalIdx, bRating ? pftOverlayRating : pftOverlay, poi, maxBAT ) );
         (*p_table)(row,col++) << rotation.SetValue( pProductForces->GetRotation( overlayIntervalIdx, bRating ? pftOverlayRating : pftOverlay, poi, minBAT ) );

         Float64 min, max;
         VehicleIndexType minConfig, maxConfig;
         if ( bDesign )
         {
            if ( bPedLoading )
            {
               pProductForces->GetLiveLoadRotation( pgsTypes::lltPedestrian, liveLoadIntervalIdx, poi, maxBAT, bIncludeImpact, true, &min, &max );
               (*p_table)(row,col++) << rotation.SetValue( max );

               pProductForces->GetLiveLoadRotation( pgsTypes::lltPedestrian, liveLoadIntervalIdx, poi,  minBAT, bIncludeImpact, true,&min, &max );
               (*p_table)(row,col++) << rotation.SetValue( min );
            }

            pProductForces->GetLiveLoadRotation( pgsTypes::lltDesign, liveLoadIntervalIdx, poi, maxBAT, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
            (*p_table)(row,col) << rotation.SetValue( max );
            if ( bIndicateControllingLoad && 0 <= maxConfig )
            {
               (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << maxConfig << _T(")");
            }
            col++;

            pProductForces->GetLiveLoadRotation( pgsTypes::lltDesign, liveLoadIntervalIdx, poi, minBAT, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
            (*p_table)(row,col) << rotation.SetValue( min );
            if ( bIndicateControllingLoad && 0 <= minConfig )
            {
               (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << minConfig << _T(")");
            }
            col++;

            if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
            {
               pProductForces->GetLiveLoadRotation( pgsTypes::lltFatigue, liveLoadIntervalIdx, poi, maxBAT, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
               (*p_table)(row,col) << rotation.SetValue( max );
               if ( bIndicateControllingLoad && 0 <= maxConfig )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltFatigue) << maxConfig << _T(")");
               }
               col++;

               pProductForces->GetLiveLoadRotation( pgsTypes::lltFatigue, liveLoadIntervalIdx, poi, minBAT, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
               (*p_table)(row,col) << rotation.SetValue( min );
               if ( bIndicateControllingLoad && 0 <= minConfig )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltFatigue) << minConfig << _T(")");
               }
               col++;
            }

            if ( bPermit )
            {
               pProductForces->GetLiveLoadRotation( pgsTypes::lltPermit, liveLoadIntervalIdx, poi, maxBAT, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
               (*p_table)(row,col) << rotation.SetValue( max );
               if ( bIndicateControllingLoad && 0 <= maxConfig )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermit) << maxConfig << _T(")");
               }
               col++;

               pProductForces->GetLiveLoadRotation( pgsTypes::lltPermit, liveLoadIntervalIdx, poi, minBAT, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
               (*p_table)(row,col) << rotation.SetValue( min );
               if ( bIndicateControllingLoad && 0 <= minConfig )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermit) << minConfig << _T(")");
               }
               col++;
            }
         }

         if ( bRating )
         {
            if ( !bDesign && (pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) || pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating)) )
            {
               pProductForces->GetLiveLoadRotation( pgsTypes::lltDesign, liveLoadIntervalIdx, poi, maxBAT, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
               (*p_table)(row,col) << rotation.SetValue( max );
               if ( bIndicateControllingLoad && 0 <= maxConfig )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << maxConfig << _T(")");
               }
               col++;

               pProductForces->GetLiveLoadRotation( pgsTypes::lltDesign, liveLoadIntervalIdx, poi, minBAT, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
               (*p_table)(row,col) << rotation.SetValue( min );
               if ( bIndicateControllingLoad && 0 <= minConfig )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << minConfig << _T(")");
               }
               col++;
            }

            // Legal Rating - Routine
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
            {
               pProductForces->GetLiveLoadRotation( pgsTypes::lltLegalRating_Routine, liveLoadIntervalIdx, poi, maxBAT, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
               (*p_table)(row,col) << rotation.SetValue( max );
               if ( bIndicateControllingLoad && 0 <= maxConfig )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Routine) << maxConfig << _T(")");
               }
               col++;

               pProductForces->GetLiveLoadRotation( pgsTypes::lltLegalRating_Routine, liveLoadIntervalIdx, poi, minBAT, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
               (*p_table)(row,col) << rotation.SetValue( min );
               if ( bIndicateControllingLoad && 0 <= minConfig )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Routine) << minConfig << _T(")");
               }
               col++;
            }

            // Legal Rating - Special
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
            {
               pProductForces->GetLiveLoadRotation( pgsTypes::lltLegalRating_Special, liveLoadIntervalIdx, poi, maxBAT, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
               (*p_table)(row,col) << rotation.SetValue( max );
               if ( bIndicateControllingLoad && 0 <= maxConfig )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Special) << maxConfig << _T(")");
               }
               col++;

               pProductForces->GetLiveLoadRotation( pgsTypes::lltLegalRating_Special, liveLoadIntervalIdx, poi, minBAT, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
               (*p_table)(row,col) << rotation.SetValue( min );
               if ( bIndicateControllingLoad && 0 <= minConfig )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Special) << minConfig << _T(")");
               }
               col++;
            }

            // Permit Rating - Routine
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
            {
               pProductForces->GetLiveLoadRotation( pgsTypes::lltPermitRating_Routine, liveLoadIntervalIdx, poi, maxBAT, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
               (*p_table)(row,col) << rotation.SetValue( max );
               if ( bIndicateControllingLoad && 0 <= maxConfig )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermitRating_Routine) << maxConfig << _T(")");
               }
               col++;

               pProductForces->GetLiveLoadRotation( pgsTypes::lltPermitRating_Routine, liveLoadIntervalIdx, poi, minBAT, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
               (*p_table)(row,col) << rotation.SetValue( min );
               if ( bIndicateControllingLoad && 0 <= minConfig )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermitRating_Routine) << minConfig << _T(")");
               }
               col++;
            }

            // Permit Rating - Special
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
            {
               pProductForces->GetLiveLoadRotation( pgsTypes::lltPermitRating_Special, liveLoadIntervalIdx, poi, maxBAT, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
               (*p_table)(row,col) << rotation.SetValue( max );
               if ( bIndicateControllingLoad && 0 <= maxConfig )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermitRating_Special) << maxConfig << _T(")");
               }
               col++;

               pProductForces->GetLiveLoadRotation( pgsTypes::lltPermitRating_Special, liveLoadIntervalIdx, poi, minBAT, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
               (*p_table)(row,col) << rotation.SetValue( min );
               if ( bIndicateControllingLoad && 0 <= minConfig )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermitRating_Special) << minConfig << _T(")");
               }
               col++;
            }
         }
      }
      else
      {
         Float64 min, max;
         VehicleIndexType minConfig, maxConfig;
         if ( bSidewalk )
         {
            (*p_table)(row,col++) << rotation.SetValue( pProductForces->GetRotation( railingSystemIntervalIdx, pftSidewalk, poi, maxBAT ) );
         }

         (*p_table)(row,col++) << rotation.SetValue( pProductForces->GetRotation( railingSystemIntervalIdx, pftTrafficBarrier, poi, maxBAT ) );
         (*p_table)(row,col++) << rotation.SetValue( pProductForces->GetRotation( overlayIntervalIdx, bRating ? pftOverlayRating : pftOverlay, poi, maxBAT ) );

         if ( bPedLoading )
         {
            pProductForces->GetLiveLoadRotation( pgsTypes::lltPedestrian, liveLoadIntervalIdx, poi, maxBAT, bIncludeImpact, true, &min, &max );
            (*p_table)(row,col++) << rotation.SetValue( max );
            (*p_table)(row,col++) << rotation.SetValue( min );
         }

         if ( bDesign )
         {
            pProductForces->GetLiveLoadRotation( pgsTypes::lltDesign, liveLoadIntervalIdx, poi, maxBAT, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
            (*p_table)(row,col) << rotation.SetValue( max );
            if ( bIndicateControllingLoad && 0 <= maxConfig )
            {
               (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << maxConfig << _T(")");
            }
            col++;

            (*p_table)(row,col) << rotation.SetValue( min );
            if ( bIndicateControllingLoad && 0 <= minConfig )
            {
               (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << minConfig << _T(")");
            }
            col++;

            if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
            {
               pProductForces->GetLiveLoadRotation( pgsTypes::lltFatigue, liveLoadIntervalIdx, poi, maxBAT, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
               (*p_table)(row,col) << rotation.SetValue( max );
               if ( bIndicateControllingLoad && 0 <= maxConfig )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltFatigue) << maxConfig << _T(")");
               }
               col++;

               (*p_table)(row,col) << rotation.SetValue( min );
               if ( bIndicateControllingLoad && 0 <= minConfig )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltFatigue) << minConfig << _T(")");
               }
               col++;
            }

            if ( bPermit )
            {
               pProductForces->GetLiveLoadRotation( pgsTypes::lltPermit, liveLoadIntervalIdx, poi, maxBAT, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
               (*p_table)(row,col) << rotation.SetValue( max );
               if ( bIndicateControllingLoad && 0 <= maxConfig )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermit) << maxConfig << _T(")");
               }
               col++;

               (*p_table)(row,col) << rotation.SetValue( min );
               if ( bIndicateControllingLoad && 0 <= minConfig )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermit) << minConfig << _T(")");
               }
               col++;
            }
         }

         if ( bRating )
         {
            if ( !bDesign && (pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) || pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating)) )
            {
               pProductForces->GetLiveLoadRotation( pgsTypes::lltDesign, liveLoadIntervalIdx, poi, maxBAT, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
               (*p_table)(row,col) << rotation.SetValue( max );
               if ( bIndicateControllingLoad && 0 <= maxConfig )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << maxConfig << _T(")");
               }
               col++;

               (*p_table)(row,col) << rotation.SetValue( min );
               if ( bIndicateControllingLoad && 0 <= minConfig )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << minConfig << _T(")");
               }
               col++;
            }

            // Legal Rating - Routine
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
            {
               pProductForces->GetLiveLoadRotation( pgsTypes::lltLegalRating_Routine, liveLoadIntervalIdx, poi, maxBAT, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
               (*p_table)(row,col) << rotation.SetValue( max );
               if ( bIndicateControllingLoad && 0 <= maxConfig )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Routine) << maxConfig << _T(")");
               }
               col++;

               (*p_table)(row,col) << rotation.SetValue( min );
               if ( bIndicateControllingLoad && 0 <= minConfig )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Routine) << minConfig << _T(")");
               }
               col++;
            }

            // Legal Rating - Special
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
            {
               pProductForces->GetLiveLoadRotation( pgsTypes::lltLegalRating_Special, liveLoadIntervalIdx, poi, maxBAT, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
               (*p_table)(row,col) << rotation.SetValue( max );
               if ( bIndicateControllingLoad && 0 <= maxConfig )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Special) << maxConfig << _T(")");
               }
               col++;

               (*p_table)(row,col) << rotation.SetValue( min );
               if ( bIndicateControllingLoad && 0 <= minConfig )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Special) << minConfig << _T(")");
               }
               col++;
            }

            // Permit Rating - Routine
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
            {
               pProductForces->GetLiveLoadRotation( pgsTypes::lltPermitRating_Routine, liveLoadIntervalIdx, poi, maxBAT, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
               (*p_table)(row,col) << rotation.SetValue( max );
               if ( bIndicateControllingLoad && 0 <= maxConfig )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermitRating_Routine) << maxConfig << _T(")");
               }
               col++;

               (*p_table)(row,col) << rotation.SetValue( min );
               if ( bIndicateControllingLoad && 0 <= minConfig )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermitRating_Routine) << minConfig << _T(")");
               }
               col++;
            }

            // Permit Rating - Special
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
            {
               pProductForces->GetLiveLoadRotation( pgsTypes::lltPermitRating_Special, liveLoadIntervalIdx, poi, maxBAT, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
               (*p_table)(row,col) << rotation.SetValue( max );
               if ( bIndicateControllingLoad && 0 <= maxConfig )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermitRating_Special) << maxConfig << _T(")");
               }
               col++;

               (*p_table)(row,col) << rotation.SetValue( min );
               if ( bIndicateControllingLoad && 0 <= minConfig )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermitRating_Special) << minConfig << _T(")");
               }
               col++;
            }
         }
      }

      row++;
   }

   return p_table;
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void CProductRotationTable::MakeCopy(const CProductRotationTable& rOther)
{
   // Add copy code here...
}

void CProductRotationTable::MakeAssignment(const CProductRotationTable& rOther)
{
   MakeCopy( rOther );
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PRIVATE    ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
//======================== INQUERY    =======================================

//======================== DEBUG      =======================================
#if defined _DEBUG
bool CProductRotationTable::AssertValid() const
{
   return true;
}

void CProductRotationTable::Dump(dbgDumpContext& os) const
{
   os << _T("Dump for CProductRotationTable") << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool CProductRotationTable::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("CProductRotationTable");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for CProductRotationTable");

   TESTME_EPILOG("CProductRotationTable");
}
#endif // _UNITTEST
