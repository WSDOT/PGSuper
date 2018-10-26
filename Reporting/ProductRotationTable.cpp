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
#include <Reporting\ProductRotationTable.h>
#include <Reporting\ProductMomentsTable.h>
#include <Reporting\ReactionInterfaceAdapters.h>

#include <IFace\Bridge.h>
#include <EAF\EAFDisplayUnits.h>
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
rptRcTable* CProductRotationTable::Build(IBroker* pBroker,SpanIndexType span,GirderIndexType gdr,pgsTypes::AnalysisType analysisType,
                                         bool bIncludeImpact, bool bIncludeLLDF,bool bDesign,bool bRating,bool bIndicateControllingLoad,IEAFDisplayUnits* pDisplayUnits) const
{
   // Build table
   INIT_UV_PROTOTYPE( rptLengthUnitValue, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptAngleUnitValue,  rotation, pDisplayUnits->GetRadAngleUnit(), false );

   GET_IFACE2(pBroker,IBridge,pBridge);
   bool bFutureOverlay = pBridge->IsFutureOverlay();
   pgsTypes::Stage overlay_stage = pgsTypes::BridgeSite2;

   bool bConstruction, bDeckPanels, bPedLoading, bSidewalk, bShearKey, bPermit;
   SpanIndexType startSpan, nSpans;
   pgsTypes::Stage continuity_stage;

   GET_IFACE2(pBroker, IRatingSpecification, pRatingSpec);

   ColumnIndexType nCols = GetProductLoadTableColumnCount(pBroker,span,gdr,analysisType,bDesign,bRating,&bConstruction,&bDeckPanels,&bSidewalk,&bShearKey,&bPedLoading,&bPermit,&continuity_stage,&startSpan,&nSpans);

   rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(nCols,_T("Rotations"));
   RowIndexType row = ConfigureProductLoadTableHeading<rptAngleUnitTag,unitmgtAngleData>(p_table,true,false,bConstruction,bDeckPanels,bSidewalk,bShearKey,bFutureOverlay,bDesign,bPedLoading,bPermit,bRating,analysisType,continuity_stage,pRatingSpec,pDisplayUnits,pDisplayUnits->GetRadAngleUnit());

   GET_IFACE2(pBroker,IPointOfInterest,pPOI);
   std::vector<pgsPointOfInterest> vPoi;

   GET_IFACE2(pBroker,IProductLoads,pLoads);
   pgsTypes::Stage girderLoadStage = pLoads->GetGirderDeadLoadStage(gdr);

   GET_IFACE2(pBroker,IBearingDesign,pBearingDesign);
   std::auto_ptr<IProductReactionAdapter> pForces = std::auto_ptr<BearingDesignProductReactionAdapter>(new BearingDesignProductReactionAdapter(pBearingDesign, pgsTypes::GirderPlacement, span, gdr) );

   // Fill up the table
   GET_IFACE2(pBroker,IProductForces,pProductForces);

   // Use iterator to walk locations
   ReactionLocationIter iter = pForces->GetReactionLocations(pBridge);
   for (iter.First(); !iter.IsDone(); iter.Next())
   {
      ColumnIndexType col = 0;

      const ReactionLocation& rct_locn = iter.CurrentItem();
      ATLASSERT(rct_locn.Face!=rftMid); // this table not built for pier reactions

      (*p_table)(row,col++) << rct_locn.PierLabel;

      // Use 1/10 point end pois to get rotation at ends of beam
      SpanIndexType currSpan  = rct_locn.Face==rftBack ? rct_locn.Pier-1 : rct_locn.Pier;
      PoiAttributeType poiAtt = rct_locn.Face==rftBack ? POI_10L : POI_0L;

      std::vector<pgsPointOfInterest> vPois ( pPOI->GetPointsOfInterest(currSpan,gdr,pgsTypes::BridgeSite3, poiAtt,POIFIND_OR) );
      pgsPointOfInterest& poi = vPois.front();

      (*p_table)(row,col++) << rotation.SetValue( pProductForces->GetRotation(girderLoadStage, pftGirder, poi, SimpleSpan) );
      (*p_table)(row,col++) << rotation.SetValue( pProductForces->GetRotation(pgsTypes::BridgeSite1, pftDiaphragm, poi, SimpleSpan) );

      // Use reaction decider tool to determine when to report stages
      ReactionDecider rctdr(BearingReactionsTable, rct_locn, pBridge);

      if ( bShearKey )
      {
         if ( analysisType == pgsTypes::Envelope )
         {
            if (rctdr.DoReport(pgsTypes::BridgeSite1 ))
            {
               (*p_table)(row,col++) << rotation.SetValue( pProductForces->GetRotation( pgsTypes::BridgeSite1, pftShearKey, poi, MaxSimpleContinuousEnvelope ) );
               (*p_table)(row,col++) << rotation.SetValue( pProductForces->GetRotation( pgsTypes::BridgeSite1, pftShearKey, poi, MinSimpleContinuousEnvelope ) );
            }
            else
            {
               (*p_table)(row,col++) << RPT_NA;
               (*p_table)(row,col++) << RPT_NA;
            }
         }
         else
         {
            if (rctdr.DoReport(pgsTypes::BridgeSite1 ))
            {
               (*p_table)(row,col++) << rotation.SetValue( pProductForces->GetRotation( pgsTypes::BridgeSite1, pftShearKey, poi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan ) );
            }
            else
            {
               (*p_table)(row,col++) << RPT_NA;
            }
         }
      }


      if ( bConstruction )
      {
         if ( analysisType == pgsTypes::Envelope && continuity_stage == pgsTypes::BridgeSite1 )
         {
            if (rctdr.DoReport(pgsTypes::BridgeSite1 ))
            {
               (*p_table)(row,col++) << rotation.SetValue( pProductForces->GetRotation( pgsTypes::BridgeSite1, pftConstruction,   poi, MaxSimpleContinuousEnvelope ) );
               (*p_table)(row,col++) << rotation.SetValue( pProductForces->GetRotation( pgsTypes::BridgeSite1, pftConstruction,   poi, MinSimpleContinuousEnvelope ) );
            }
            else
            {
               (*p_table)(row,col++) << RPT_NA;
               (*p_table)(row,col++) << RPT_NA;
            }
         }
         else
         {
            if (rctdr.DoReport(pgsTypes::BridgeSite1 ))
            {
               (*p_table)(row,col++) << rotation.SetValue( pProductForces->GetRotation( pgsTypes::BridgeSite1, pftConstruction,   poi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan ) );
            }
            else
            {
               (*p_table)(row,col++) << RPT_NA;
            }
         }
      }

      if ( analysisType == pgsTypes::Envelope && continuity_stage == pgsTypes::BridgeSite1 )
      {
         if (rctdr.DoReport(pgsTypes::BridgeSite1 ))
         {
            (*p_table)(row,col++) << rotation.SetValue( pProductForces->GetRotation( pgsTypes::BridgeSite1, pftSlab,  poi, MaxSimpleContinuousEnvelope ) );
            (*p_table)(row,col++) << rotation.SetValue( pProductForces->GetRotation( pgsTypes::BridgeSite1, pftSlab,  poi, MinSimpleContinuousEnvelope ) );

            (*p_table)(row,col++) << rotation.SetValue( pProductForces->GetRotation( pgsTypes::BridgeSite1, pftSlabPad,  poi, MaxSimpleContinuousEnvelope ) );
            (*p_table)(row,col++) << rotation.SetValue( pProductForces->GetRotation( pgsTypes::BridgeSite1, pftSlabPad,  poi, MinSimpleContinuousEnvelope ) );
         }
         else
         {
            (*p_table)(row,col++) << RPT_NA;
            (*p_table)(row,col++) << RPT_NA;
            (*p_table)(row,col++) << RPT_NA;
            (*p_table)(row,col++) << RPT_NA;
         }
      }
      else
      {
         if (rctdr.DoReport(pgsTypes::BridgeSite1 ))
         {
            (*p_table)(row,col++) << rotation.SetValue( pProductForces->GetRotation( pgsTypes::BridgeSite1, pftSlab,  poi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan  ) );
            (*p_table)(row,col++) << rotation.SetValue( pProductForces->GetRotation( pgsTypes::BridgeSite1, pftSlabPad,  poi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan  ) );
         }
         else
         {
            (*p_table)(row,col++) << RPT_NA;
            (*p_table)(row,col++) << RPT_NA;
         }
      }

      if ( bDeckPanels )
      {
         if ( analysisType == pgsTypes::Envelope && continuity_stage == pgsTypes::BridgeSite1 )
         {
            if (rctdr.DoReport(pgsTypes::BridgeSite1 ))
            {
               (*p_table)(row,col++) << rotation.SetValue( pProductForces->GetRotation( pgsTypes::BridgeSite1, pftSlabPanel,   poi, MaxSimpleContinuousEnvelope ) );
               (*p_table)(row,col++) << rotation.SetValue( pProductForces->GetRotation( pgsTypes::BridgeSite1, pftSlabPanel,   poi, MinSimpleContinuousEnvelope ) );
            }
            else
            {
               (*p_table)(row,col++) << RPT_NA;
               (*p_table)(row,col++) << RPT_NA;
            }
         }
         else
         {
            if (rctdr.DoReport(pgsTypes::BridgeSite1 ))
            {
               (*p_table)(row,col++) << rotation.SetValue( pProductForces->GetRotation( pgsTypes::BridgeSite1, pftSlabPanel,   poi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan ) );
            }
            else
            {
               (*p_table)(row,col++) << RPT_NA;
            }
         }
      }

      if ( analysisType == pgsTypes::Envelope )
      {
         if ( bSidewalk )
         {
            if (rctdr.DoReport(pgsTypes::BridgeSite2 ))
            {
               (*p_table)(row,col++) << rotation.SetValue( pProductForces->GetRotation( pgsTypes::BridgeSite2, pftSidewalk, poi, MaxSimpleContinuousEnvelope ) );
               (*p_table)(row,col++) << rotation.SetValue( pProductForces->GetRotation( pgsTypes::BridgeSite2, pftSidewalk, poi, MinSimpleContinuousEnvelope ) );
            }
            else
            {
               (*p_table)(row,col++) << RPT_NA;
               (*p_table)(row,col++) << RPT_NA;
            }
         }

         if (rctdr.DoReport(pgsTypes::BridgeSite2 ))
         {
            (*p_table)(row,col++) << rotation.SetValue( pProductForces->GetRotation( pgsTypes::BridgeSite2, pftTrafficBarrier, poi, MaxSimpleContinuousEnvelope ) );
            (*p_table)(row,col++) << rotation.SetValue( pProductForces->GetRotation( pgsTypes::BridgeSite2, pftTrafficBarrier, poi, MinSimpleContinuousEnvelope ) );
         }
         else
         {
            (*p_table)(row,col++) << RPT_NA;
            (*p_table)(row,col++) << RPT_NA;
         }

         if (rctdr.DoReport(overlay_stage ))
         {
            (*p_table)(row,col++) << rotation.SetValue( pProductForces->GetRotation( overlay_stage, !bDesign ? pftOverlayRating : pftOverlay,        poi, MaxSimpleContinuousEnvelope ) );
            (*p_table)(row,col++) << rotation.SetValue( pProductForces->GetRotation( overlay_stage, !bDesign ? pftOverlayRating : pftOverlay,        poi, MinSimpleContinuousEnvelope ) );
         }
         else
         {
            (*p_table)(row,col++) << RPT_NA;
            (*p_table)(row,col++) << RPT_NA;
         }

         Float64 min, max;
         VehicleIndexType minConfig, maxConfig;
         if ( bDesign )
         {
            if ( bPedLoading )
            {
               if (rctdr.DoReport(pgsTypes::BridgeSite3 ))
               {
                  pProductForces->GetLiveLoadRotation( pgsTypes::lltPedestrian, pgsTypes::BridgeSite3, poi, MaxSimpleContinuousEnvelope, bIncludeImpact, true, &min, &max );
                  (*p_table)(row,col++) << rotation.SetValue( max );

                  pProductForces->GetLiveLoadRotation( pgsTypes::lltPedestrian, pgsTypes::BridgeSite3, poi,  MinSimpleContinuousEnvelope, bIncludeImpact, true,&min, &max );
                  (*p_table)(row,col++) << rotation.SetValue( min );
               }
               else
               {
                  (*p_table)(row,col++) << RPT_NA;
                  (*p_table)(row,col++) << RPT_NA;
               }
            }

            if (rctdr.DoReport(pgsTypes::BridgeSite3 ))
            {
               pProductForces->GetLiveLoadRotation( pgsTypes::lltDesign, pgsTypes::BridgeSite3, poi, MaxSimpleContinuousEnvelope, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
               (*p_table)(row,col) << rotation.SetValue( max );
               if ( bIndicateControllingLoad && 0 <= maxConfig )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << maxConfig << _T(")");
               }
            }
            else
            {
               (*p_table)(row,col) << RPT_NA;
            }

            col++;

            if (rctdr.DoReport(pgsTypes::BridgeSite3 ))
            {
               pProductForces->GetLiveLoadRotation( pgsTypes::lltDesign, pgsTypes::BridgeSite3, poi, MinSimpleContinuousEnvelope, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
               (*p_table)(row,col) << rotation.SetValue( min );
               if ( bIndicateControllingLoad && 0 <= minConfig )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << minConfig << _T(")");
               }
            }
            else
            {
               (*p_table)(row,col) << RPT_NA;
            }

            col++;

            if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
            {
               if (rctdr.DoReport(pgsTypes::BridgeSite3 ))
               {
                  pProductForces->GetLiveLoadRotation( pgsTypes::lltFatigue, pgsTypes::BridgeSite3, poi, MaxSimpleContinuousEnvelope, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
                  (*p_table)(row,col) << rotation.SetValue( max );
                  if ( bIndicateControllingLoad && 0 <= maxConfig )
                  {
                     (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltFatigue) << maxConfig << _T(")");
                  }
               }
               else
               {
                  (*p_table)(row,col) << RPT_NA;
               }

               col++;

               if (rctdr.DoReport(pgsTypes::BridgeSite3 ))
               {
                  pProductForces->GetLiveLoadRotation( pgsTypes::lltFatigue, pgsTypes::BridgeSite3, poi, MinSimpleContinuousEnvelope, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
                  (*p_table)(row,col) << rotation.SetValue( min );
                  if ( bIndicateControllingLoad && 0 <= minConfig )
                  {
                     (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltFatigue) << minConfig << _T(")");
                  }
               }
               else
               {
                  (*p_table)(row,col) << RPT_NA;
               }

               col++;
            }

            if ( bPermit )
            {
               if (rctdr.DoReport(pgsTypes::BridgeSite3 ))
               {
                  pProductForces->GetLiveLoadRotation( pgsTypes::lltPermit, pgsTypes::BridgeSite3, poi, MaxSimpleContinuousEnvelope, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
                  (*p_table)(row,col) << rotation.SetValue( max );
                  if ( bIndicateControllingLoad && 0 <= maxConfig )
                  {
                     (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermit) << maxConfig << _T(")");
                  }
               }
               else
               {
                  (*p_table)(row,col) << RPT_NA;
               }

               col++;

               if (rctdr.DoReport(pgsTypes::BridgeSite3 ))
               {
                  pProductForces->GetLiveLoadRotation( pgsTypes::lltPermit, pgsTypes::BridgeSite3, poi, MinSimpleContinuousEnvelope, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
                  (*p_table)(row,col) << rotation.SetValue( min );
                  if ( bIndicateControllingLoad && 0 <= minConfig )
                  {
                     (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermit) << minConfig << _T(")");
                  }
               }
               else
               {
                  (*p_table)(row,col) << RPT_NA;
               }

               col++;
            }
         }

         if ( bRating )
         {
            if ( !bDesign && (pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) || pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating)) )
            {
               if (rctdr.DoReport(pgsTypes::BridgeSite3 ))
               {
                  pProductForces->GetLiveLoadRotation( pgsTypes::lltDesign, pgsTypes::BridgeSite3, poi, MaxSimpleContinuousEnvelope, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
                  (*p_table)(row,col) << rotation.SetValue( max );
                  if ( bIndicateControllingLoad && 0 <= maxConfig )
                  {
                     (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << maxConfig << _T(")");
                  }
               }
               else
               {
                  (*p_table)(row,col) << RPT_NA;
               }

               col++;

               if (rctdr.DoReport(pgsTypes::BridgeSite3 ))
               {
                  pProductForces->GetLiveLoadRotation( pgsTypes::lltDesign, pgsTypes::BridgeSite3, poi, MinSimpleContinuousEnvelope, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
                  (*p_table)(row,col) << rotation.SetValue( min );
                  if ( bIndicateControllingLoad && 0 <= minConfig )
                  {
                     (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << minConfig << _T(")");
                  }
               }
               else
               {
                  (*p_table)(row,col) << RPT_NA;
               }

               col++;
            }

            // Legal Rating - Routine
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
            {
               if (rctdr.DoReport(pgsTypes::BridgeSite3 ))
               {
                  pProductForces->GetLiveLoadRotation( pgsTypes::lltLegalRating_Routine, pgsTypes::BridgeSite3, poi, MaxSimpleContinuousEnvelope, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
                  (*p_table)(row,col) << rotation.SetValue( max );
                  if ( bIndicateControllingLoad && 0 <= maxConfig )
                  {
                     (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Routine) << maxConfig << _T(")");
                  }
               }
               else
               {
                  (*p_table)(row,col) << RPT_NA;
               }

               col++;

               if (rctdr.DoReport(pgsTypes::BridgeSite3 ))
               {
                  pProductForces->GetLiveLoadRotation( pgsTypes::lltLegalRating_Routine, pgsTypes::BridgeSite3, poi, MinSimpleContinuousEnvelope, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
                  (*p_table)(row,col) << rotation.SetValue( min );
                  if ( bIndicateControllingLoad && 0 <= minConfig )
                  {
                     (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Routine) << minConfig << _T(")");
                  }
               }
               else
               {
                  (*p_table)(row,col) << RPT_NA;
               }

               col++;
            }

            // Legal Rating - Special
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
            {
               if (rctdr.DoReport(pgsTypes::BridgeSite3 ))
               {
                  pProductForces->GetLiveLoadRotation( pgsTypes::lltLegalRating_Special, pgsTypes::BridgeSite3, poi, MaxSimpleContinuousEnvelope, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
                  (*p_table)(row,col) << rotation.SetValue( max );
                  if ( bIndicateControllingLoad && 0 <= maxConfig )
                  {
                     (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Special) << maxConfig << _T(")");
                  }
               }
               else
               {
                  (*p_table)(row,col) << RPT_NA;
               }

               col++;

               if (rctdr.DoReport(pgsTypes::BridgeSite3 ))
               {
                  pProductForces->GetLiveLoadRotation( pgsTypes::lltLegalRating_Special, pgsTypes::BridgeSite3, poi, MinSimpleContinuousEnvelope, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
                  (*p_table)(row,col) << rotation.SetValue( min );
                  if ( bIndicateControllingLoad && 0 <= minConfig )
                  {
                     (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Special) << minConfig << _T(")");
                  }
               }
               else
               {
                  (*p_table)(row,col) << RPT_NA;
               }

               col++;
            }

            // Permit Rating - Routine
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
            {
               if (rctdr.DoReport(pgsTypes::BridgeSite3 ))
               {
                  pProductForces->GetLiveLoadRotation( pgsTypes::lltPermitRating_Routine, pgsTypes::BridgeSite3, poi, MaxSimpleContinuousEnvelope, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
                  (*p_table)(row,col) << rotation.SetValue( max );
                  if ( bIndicateControllingLoad && 0 <= maxConfig )
                  {
                     (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermitRating_Routine) << maxConfig << _T(")");
                  }
               }
               else
               {
                  (*p_table)(row,col) << RPT_NA;
               }

               col++;

               if (rctdr.DoReport(pgsTypes::BridgeSite3 ))
               {
                  pProductForces->GetLiveLoadRotation( pgsTypes::lltPermitRating_Routine, pgsTypes::BridgeSite3, poi, MinSimpleContinuousEnvelope, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
                  (*p_table)(row,col) << rotation.SetValue( min );
                  if ( bIndicateControllingLoad && 0 <= minConfig )
                  {
                     (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermitRating_Routine) << minConfig << _T(")");
                  }
               }
               else
               {
                  (*p_table)(row,col) << RPT_NA;
               }

               col++;
            }

            // Permit Rating - Special
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
            {
               if (rctdr.DoReport(pgsTypes::BridgeSite3 ))
               {
                  pProductForces->GetLiveLoadRotation( pgsTypes::lltPermitRating_Special, pgsTypes::BridgeSite3, poi, MaxSimpleContinuousEnvelope, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
                  (*p_table)(row,col) << rotation.SetValue( max );
                  if ( bIndicateControllingLoad && 0 <= maxConfig )
                  {
                     (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermitRating_Special) << maxConfig << _T(")");
                  }
               }
               else
               {
                  (*p_table)(row,col) << RPT_NA;
               }

               col++;

               if (rctdr.DoReport(pgsTypes::BridgeSite3 ))
               {
                  pProductForces->GetLiveLoadRotation( pgsTypes::lltPermitRating_Special, pgsTypes::BridgeSite3, poi, MinSimpleContinuousEnvelope, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
                  (*p_table)(row,col) << rotation.SetValue( min );
                  if ( bIndicateControllingLoad && 0 <= minConfig )
                  {
                     (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermitRating_Special) << minConfig << _T(")");
                  }
               }
               else
               {
                  (*p_table)(row,col) << RPT_NA;
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
            if (rctdr.DoReport(pgsTypes::BridgeSite2 ))
            {
               (*p_table)(row,col++) << rotation.SetValue( pProductForces->GetRotation( pgsTypes::BridgeSite2, pftSidewalk, poi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan ) );
            }
            else
            {
               (*p_table)(row,col++) << RPT_NA;
            }
         }

         if (rctdr.DoReport(pgsTypes::BridgeSite2 ))
         {
            (*p_table)(row,col++) << rotation.SetValue( pProductForces->GetRotation( pgsTypes::BridgeSite2, pftTrafficBarrier, poi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan ) );
            (*p_table)(row,col++) << rotation.SetValue( pProductForces->GetRotation( overlay_stage, !bDesign ? pftOverlayRating : pftOverlay,        poi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan ) );
         }
         else
         {
            (*p_table)(row,col++) << RPT_NA;
            (*p_table)(row,col++) << RPT_NA;
         }

         if ( bPedLoading )
         {
            if (rctdr.DoReport(pgsTypes::BridgeSite3 ))
            {
               pProductForces->GetLiveLoadRotation( pgsTypes::lltPedestrian, pgsTypes::BridgeSite3, poi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, bIncludeImpact, true, &min, &max );
               (*p_table)(row,col++) << rotation.SetValue( max );
               (*p_table)(row,col++) << rotation.SetValue( min );
            }
            else
            {
               (*p_table)(row,col++) << RPT_NA;
               (*p_table)(row,col++) << RPT_NA;
            }
         }

         if ( bDesign )
         {
            if (rctdr.DoReport(pgsTypes::BridgeSite3 ))
            {
               pProductForces->GetLiveLoadRotation( pgsTypes::lltDesign, pgsTypes::BridgeSite3, poi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
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
            else
            {
               (*p_table)(row,col++) << RPT_NA;
               (*p_table)(row,col++) << RPT_NA;
            }

            if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
            {
               if (rctdr.DoReport(pgsTypes::BridgeSite3 ))
               {
                  pProductForces->GetLiveLoadRotation( pgsTypes::lltFatigue, pgsTypes::BridgeSite3, poi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
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
               else
               {
                  (*p_table)(row,col++) << RPT_NA;
                  (*p_table)(row,col++) << RPT_NA;
               }
            }

            if ( bPermit )
            {
               if (rctdr.DoReport(pgsTypes::BridgeSite3 ))
               {
                  pProductForces->GetLiveLoadRotation( pgsTypes::lltPermit, pgsTypes::BridgeSite3, poi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
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
               else
               {
                  (*p_table)(row,col++) << RPT_NA;
                  (*p_table)(row,col++) << RPT_NA;
               }
            }
         }

         if ( bRating )
         {
            if ( !bDesign && (pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) || pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating)) )
            {
               if (rctdr.DoReport(pgsTypes::BridgeSite3 ))
               {
                  pProductForces->GetLiveLoadRotation( pgsTypes::lltDesign, pgsTypes::BridgeSite3, poi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
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
               else
               {
                  (*p_table)(row,col++) << RPT_NA;
                  (*p_table)(row,col++) << RPT_NA;
               }
            }

            // Legal Rating - Routine
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
            {
               if (rctdr.DoReport(pgsTypes::BridgeSite3 ))
               {
                  pProductForces->GetLiveLoadRotation( pgsTypes::lltLegalRating_Routine, pgsTypes::BridgeSite3, poi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
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
               else
               {
                  (*p_table)(row,col++) << RPT_NA;
                  (*p_table)(row,col++) << RPT_NA;
               }
            }

            // Legal Rating - Special
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
            {
               if (rctdr.DoReport(pgsTypes::BridgeSite3 ))
               {
                  pProductForces->GetLiveLoadRotation( pgsTypes::lltLegalRating_Special, pgsTypes::BridgeSite3, poi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
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
               else
               {
                  (*p_table)(row,col++) << RPT_NA;
                  (*p_table)(row,col++) << RPT_NA;
               }
            }

            // Permit Rating - Routine
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
            {
               if (rctdr.DoReport(pgsTypes::BridgeSite3 ))
               {
                  pProductForces->GetLiveLoadRotation( pgsTypes::lltPermitRating_Routine, pgsTypes::BridgeSite3, poi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
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
               else
               {
                  (*p_table)(row,col++) << RPT_NA;
                  (*p_table)(row,col++) << RPT_NA;
               }
            }

            // Permit Rating - Special
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
            {
               if (rctdr.DoReport(pgsTypes::BridgeSite3 ))
               {
                  pProductForces->GetLiveLoadRotation( pgsTypes::lltPermitRating_Special, pgsTypes::BridgeSite3, poi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
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
               else
               {
                  (*p_table)(row,col++) << RPT_NA;
                  (*p_table)(row,col++) << RPT_NA;
               }
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
