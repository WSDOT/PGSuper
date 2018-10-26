///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2011  Washington State Department of Transportation
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
   pgsTypes::Stage overlay_stage = pBridge->IsFutureOverlay() ? pgsTypes::BridgeSite3 : pgsTypes::BridgeSite2;

   bool bConstruction, bDeckPanels, bPedLoading, bSidewalk, bShearKey, bPermit;
   SpanIndexType startSpan, nSpans;
   pgsTypes::Stage continuity_stage;

   GET_IFACE2(pBroker, IRatingSpecification, pRatingSpec);

   ColumnIndexType nCols = GetProductLoadTableColumnCount(pBroker,span,gdr,analysisType,bDesign,bRating,&bConstruction,&bDeckPanels,&bSidewalk,&bShearKey,&bPedLoading,&bPermit,&continuity_stage,&startSpan,&nSpans);

   PierIndexType nPiers = nSpans+1;
   PierIndexType startPier = startSpan;
   PierIndexType endPier   = (span == ALL_SPANS ? nPiers : startPier+2 );

   rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(nCols,_T("Rotations"));
   RowIndexType row = ConfigureProductLoadTableHeading<rptAngleUnitTag,unitmgtAngleData>(p_table,true,bConstruction,bDeckPanels,bSidewalk,bShearKey,bDesign,bPedLoading,bPermit,bRating,analysisType,continuity_stage,pRatingSpec,pDisplayUnits,pDisplayUnits->GetRadAngleUnit());

   GET_IFACE2(pBroker,IPointOfInterest,pPOI);
   std::vector<pgsPointOfInterest> vPoi;

   SpanIndexType endSpan   = (span == ALL_SPANS ? nSpans : startSpan+1);
   for ( SpanIndexType spanIdx = startSpan; spanIdx < endSpan; spanIdx++ )
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(spanIdx);
      GirderIndexType gdrIdx = min(gdr,nGirders-1);
      std::vector<pgsPointOfInterest> vTenthPoints = pPOI->GetTenthPointPOIs(pgsTypes::BridgeSite3,spanIdx,gdrIdx);
      vPoi.push_back(*vTenthPoints.begin());
      vPoi.push_back(*(vTenthPoints.end()-1));
   }

   GET_IFACE2(pBroker,IProductLoads,pLoads);
   pgsTypes::Stage girderLoadStage = pLoads->GetGirderDeadLoadStage(gdr);

   // Fill up the table
   GET_IFACE2(pBroker,IProductForces,pForces);
   for ( PierIndexType pier = startPier; pier < endPier; pier++ )
   {
      ColumnIndexType col = 0;

      if ( pier == 0 || pier == pBridge->GetPierCount()-1 )
         (*p_table)(row,col++) << _T("Abutment ") << LABEL_PIER(pier);
      else
         (*p_table)(row,col++) << _T("Pier ") << LABEL_PIER(pier);
   
      
      pgsPointOfInterest& poi = vPoi[pier-startPier];
      (*p_table)(row,col++) << rotation.SetValue( pForces->GetRotation(girderLoadStage, pftGirder, poi, SimpleSpan) );
      (*p_table)(row,col++) << rotation.SetValue( pForces->GetRotation(pgsTypes::BridgeSite1, pftDiaphragm, poi, SimpleSpan) );

      if ( bShearKey )
      {
         if ( analysisType == pgsTypes::Envelope )
         {
            (*p_table)(row,col++) << rotation.SetValue( pForces->GetRotation( pgsTypes::BridgeSite1, pftShearKey, poi, MaxSimpleContinuousEnvelope ) );
            (*p_table)(row,col++) << rotation.SetValue( pForces->GetRotation( pgsTypes::BridgeSite1, pftShearKey, poi, MinSimpleContinuousEnvelope ) );
         }
         else
         {
            (*p_table)(row,col++) << rotation.SetValue( pForces->GetRotation( pgsTypes::BridgeSite1, pftShearKey, poi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan ) );
         }
      }


      if ( bConstruction )
      {
         if ( analysisType == pgsTypes::Envelope && continuity_stage == pgsTypes::BridgeSite1 )
         {
            (*p_table)(row,col++) << rotation.SetValue( pForces->GetRotation( pgsTypes::BridgeSite1, pftConstruction,   poi, MaxSimpleContinuousEnvelope ) );
            (*p_table)(row,col++) << rotation.SetValue( pForces->GetRotation( pgsTypes::BridgeSite1, pftConstruction,   poi, MinSimpleContinuousEnvelope ) );
         }
         else
         {
            (*p_table)(row,col++) << rotation.SetValue( pForces->GetRotation( pgsTypes::BridgeSite1, pftConstruction,   poi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan ) );
         }
      }

      if ( analysisType == pgsTypes::Envelope && continuity_stage == pgsTypes::BridgeSite1 )
      {
         (*p_table)(row,col++) << rotation.SetValue( pForces->GetRotation( pgsTypes::BridgeSite1, pftSlab,  poi, MaxSimpleContinuousEnvelope ) );
         (*p_table)(row,col++) << rotation.SetValue( pForces->GetRotation( pgsTypes::BridgeSite1, pftSlab,  poi, MinSimpleContinuousEnvelope ) );
      }
      else
      {
         (*p_table)(row,col++) << rotation.SetValue( pForces->GetRotation( pgsTypes::BridgeSite1, pftSlab,  poi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan  ) );
      }

      if ( bDeckPanels )
      {
         if ( analysisType == pgsTypes::Envelope && continuity_stage == pgsTypes::BridgeSite1 )
         {
            (*p_table)(row,col++) << rotation.SetValue( pForces->GetRotation( pgsTypes::BridgeSite1, pftSlabPanel,   poi, MaxSimpleContinuousEnvelope ) );
            (*p_table)(row,col++) << rotation.SetValue( pForces->GetRotation( pgsTypes::BridgeSite1, pftSlabPanel,   poi, MinSimpleContinuousEnvelope ) );
         }
         else
         {
            (*p_table)(row,col++) << rotation.SetValue( pForces->GetRotation( pgsTypes::BridgeSite1, pftSlabPanel,   poi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan ) );
         }
      }

      if ( analysisType == pgsTypes::Envelope )
      {
         if ( bSidewalk )
         {
            (*p_table)(row,col++) << rotation.SetValue( pForces->GetRotation( pgsTypes::BridgeSite2, pftSidewalk, poi, MaxSimpleContinuousEnvelope ) );
            (*p_table)(row,col++) << rotation.SetValue( pForces->GetRotation( pgsTypes::BridgeSite2, pftSidewalk, poi, MinSimpleContinuousEnvelope ) );
         }

         (*p_table)(row,col++) << rotation.SetValue( pForces->GetRotation( pgsTypes::BridgeSite2, pftTrafficBarrier, poi, MaxSimpleContinuousEnvelope ) );
         (*p_table)(row,col++) << rotation.SetValue( pForces->GetRotation( pgsTypes::BridgeSite2, pftTrafficBarrier, poi, MinSimpleContinuousEnvelope ) );
         (*p_table)(row,col++) << rotation.SetValue( pForces->GetRotation( overlay_stage, bRating ? pftOverlayRating : pftOverlay,        poi, MaxSimpleContinuousEnvelope ) );
         (*p_table)(row,col++) << rotation.SetValue( pForces->GetRotation( overlay_stage, bRating ? pftOverlayRating : pftOverlay,        poi, MinSimpleContinuousEnvelope ) );

         Float64 min, max;
         long minConfig, maxConfig;
         if ( bDesign )
         {
            if ( bPedLoading )
            {
               pForces->GetLiveLoadRotation( pgsTypes::lltPedestrian, pgsTypes::BridgeSite3, poi, MaxSimpleContinuousEnvelope, bIncludeImpact, bIncludeLLDF, &min, &max );
               (*p_table)(row,col++) << rotation.SetValue( max );

               pForces->GetLiveLoadRotation( pgsTypes::lltPedestrian, pgsTypes::BridgeSite3, poi,  MinSimpleContinuousEnvelope, bIncludeImpact, bIncludeLLDF,&min, &max );
               (*p_table)(row,col++) << rotation.SetValue( min );
            }

            pForces->GetLiveLoadRotation( pgsTypes::lltDesign, pgsTypes::BridgeSite3, poi, MaxSimpleContinuousEnvelope, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
            (*p_table)(row,col) << rotation.SetValue( max );
            if ( bIndicateControllingLoad && 0 <= maxConfig )
            {
               (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << maxConfig << _T(")");
            }
            col++;

            pForces->GetLiveLoadRotation( pgsTypes::lltDesign, pgsTypes::BridgeSite3, poi, MinSimpleContinuousEnvelope, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
            (*p_table)(row,col) << rotation.SetValue( min );
            if ( bIndicateControllingLoad && 0 <= minConfig )
            {
               (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << minConfig << _T(")");
            }
            col++;

            if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
            {
               pForces->GetLiveLoadRotation( pgsTypes::lltFatigue, pgsTypes::BridgeSite3, poi, MaxSimpleContinuousEnvelope, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
               (*p_table)(row,col) << rotation.SetValue( max );
               if ( bIndicateControllingLoad && 0 <= maxConfig )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltFatigue) << maxConfig << _T(")");
               }
               col++;

               pForces->GetLiveLoadRotation( pgsTypes::lltFatigue, pgsTypes::BridgeSite3, poi, MinSimpleContinuousEnvelope, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
               (*p_table)(row,col) << rotation.SetValue( min );
               if ( bIndicateControllingLoad && 0 <= minConfig )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltFatigue) << minConfig << _T(")");
               }
               col++;
            }

            if ( bPermit )
            {
               pForces->GetLiveLoadRotation( pgsTypes::lltPermit, pgsTypes::BridgeSite3, poi, MaxSimpleContinuousEnvelope, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
               (*p_table)(row,col) << rotation.SetValue( max );
               if ( bIndicateControllingLoad && 0 <= maxConfig )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermit) << maxConfig << _T(")");
               }
               col++;

               pForces->GetLiveLoadRotation( pgsTypes::lltPermit, pgsTypes::BridgeSite3, poi, MinSimpleContinuousEnvelope, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
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
               pForces->GetLiveLoadRotation( pgsTypes::lltDesign, pgsTypes::BridgeSite3, poi, MaxSimpleContinuousEnvelope, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
               (*p_table)(row,col) << rotation.SetValue( max );
               if ( bIndicateControllingLoad && 0 <= maxConfig )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << maxConfig << _T(")");
               }
               col++;

               pForces->GetLiveLoadRotation( pgsTypes::lltDesign, pgsTypes::BridgeSite3, poi, MinSimpleContinuousEnvelope, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
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
               pForces->GetLiveLoadRotation( pgsTypes::lltLegalRating_Routine, pgsTypes::BridgeSite3, poi, MaxSimpleContinuousEnvelope, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
               (*p_table)(row,col) << rotation.SetValue( max );
               if ( bIndicateControllingLoad && 0 <= maxConfig )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Routine) << maxConfig << _T(")");
               }
               col++;

               pForces->GetLiveLoadRotation( pgsTypes::lltLegalRating_Routine, pgsTypes::BridgeSite3, poi, MinSimpleContinuousEnvelope, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
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
               pForces->GetLiveLoadRotation( pgsTypes::lltLegalRating_Special, pgsTypes::BridgeSite3, poi, MaxSimpleContinuousEnvelope, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
               (*p_table)(row,col) << rotation.SetValue( max );
               if ( bIndicateControllingLoad && 0 <= maxConfig )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Special) << maxConfig << _T(")");
               }
               col++;

               pForces->GetLiveLoadRotation( pgsTypes::lltLegalRating_Special, pgsTypes::BridgeSite3, poi, MinSimpleContinuousEnvelope, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
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
               pForces->GetLiveLoadRotation( pgsTypes::lltPermitRating_Routine, pgsTypes::BridgeSite3, poi, MaxSimpleContinuousEnvelope, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
               (*p_table)(row,col) << rotation.SetValue( max );
               if ( bIndicateControllingLoad && 0 <= maxConfig )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermitRating_Routine) << maxConfig << _T(")");
               }
               col++;

               pForces->GetLiveLoadRotation( pgsTypes::lltPermitRating_Routine, pgsTypes::BridgeSite3, poi, MinSimpleContinuousEnvelope, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
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
               pForces->GetLiveLoadRotation( pgsTypes::lltPermitRating_Special, pgsTypes::BridgeSite3, poi, MaxSimpleContinuousEnvelope, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
               (*p_table)(row,col) << rotation.SetValue( max );
               if ( bIndicateControllingLoad && 0 <= maxConfig )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermitRating_Special) << maxConfig << _T(")");
               }
               col++;

               pForces->GetLiveLoadRotation( pgsTypes::lltPermitRating_Special, pgsTypes::BridgeSite3, poi, MinSimpleContinuousEnvelope, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
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
         long minConfig, maxConfig;
         if ( bSidewalk )
         {
            (*p_table)(row,col++) << rotation.SetValue( pForces->GetRotation( pgsTypes::BridgeSite2, pftSidewalk, poi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan ) );
         }

         (*p_table)(row,col++) << rotation.SetValue( pForces->GetRotation( pgsTypes::BridgeSite2, pftTrafficBarrier, poi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan ) );
         (*p_table)(row,col++) << rotation.SetValue( pForces->GetRotation( overlay_stage, bRating ? pftOverlayRating : pftOverlay,        poi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan ) );

         if ( bPedLoading )
         {
            pForces->GetLiveLoadRotation( pgsTypes::lltPedestrian, pgsTypes::BridgeSite3, poi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, bIncludeImpact, bIncludeLLDF, &min, &max );
            (*p_table)(row,col++) << rotation.SetValue( max );
            (*p_table)(row,col++) << rotation.SetValue( min );
         }

         if ( bDesign )
         {
            pForces->GetLiveLoadRotation( pgsTypes::lltDesign, pgsTypes::BridgeSite3, poi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
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
               pForces->GetLiveLoadRotation( pgsTypes::lltFatigue, pgsTypes::BridgeSite3, poi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
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
               pForces->GetLiveLoadRotation( pgsTypes::lltPermit, pgsTypes::BridgeSite3, poi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
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
               pForces->GetLiveLoadRotation( pgsTypes::lltDesign, pgsTypes::BridgeSite3, poi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
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
               pForces->GetLiveLoadRotation( pgsTypes::lltLegalRating_Routine, pgsTypes::BridgeSite3, poi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
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
               pForces->GetLiveLoadRotation( pgsTypes::lltLegalRating_Special, pgsTypes::BridgeSite3, poi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
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
               pForces->GetLiveLoadRotation( pgsTypes::lltPermitRating_Routine, pgsTypes::BridgeSite3, poi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
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
               pForces->GetLiveLoadRotation( pgsTypes::lltPermitRating_Special, pgsTypes::BridgeSite3, poi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
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
