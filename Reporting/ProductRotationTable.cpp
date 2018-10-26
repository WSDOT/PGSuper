///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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
#include <IFace\DisplayUnits.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Project.h>

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
                                         bool bIncludeImpact, bool bIncludeLLDF,bool bIndicateControllingLoad,IDisplayUnits* pDisplayUnits) const
{
   // Build table
   INIT_UV_PROTOTYPE( rptLengthUnitValue, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptAngleUnitValue,  rotation, pDisplayUnits->GetRadAngleUnit(), false );

   GET_IFACE2(pBroker,IBridge,pBridge);
   bool bDeckPanels = (pBridge->GetDeckType() == pgsTypes::sdtCompositeSIP ? true : false);
   pgsTypes::Stage overlay_stage = pBridge->IsFutureOverlay() ? pgsTypes::BridgeSite3 : pgsTypes::BridgeSite2;

   PierIndexType nPiers = pBridge->GetPierCount();

   PierIndexType startPier = (span == ALL_SPANS ? 0 : span);
   PierIndexType endPier   = (span == ALL_SPANS ? nPiers : startPier+2 );

   GET_IFACE2(pBroker,ILiveLoads,pLiveLoads);
   bool bPermit = pLiveLoads->IsLiveLoadDefined(pgsTypes::lltPermit);

   GET_IFACE2(pBroker,IProductLoads,pLoads);
   bool bPedLoading = pLoads->HasPedestrianLoad(startPier,gdr);
   bool bSidewalk = pLoads->HasSidewalkLoad(startPier,gdr);
   bool bShearKey = pLoads->HasShearKeyLoad(startPier,gdr);

   pgsTypes::Stage continuity_stage = pgsTypes::BridgeSite2;
   PierIndexType pier;
   for (pier = startPier; pier < endPier; pier++ )
   {
      pgsTypes::Stage left_stage, right_stage;
      pBridge->GetContinuityStage(pier,&left_stage,&right_stage);
      continuity_stage = _cpp_min(continuity_stage,left_stage);
      continuity_stage = _cpp_min(continuity_stage,right_stage);
   }

   ColumnIndexType nCols = 8;

   if ( bDeckPanels )
   {
      if ( analysisType == pgsTypes::Envelope && continuity_stage == pgsTypes::BridgeSite1)
         nCols += 2;
      else
         nCols++;
   }

   if ( analysisType == pgsTypes::Envelope && continuity_stage == pgsTypes::BridgeSite1 )
      nCols++; // add on more column for min/max slab

   if ( analysisType == pgsTypes::Envelope )
      nCols += 2; // add one more each for min/max overlay and min/max traffic barrier

   if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
      nCols += 2; // fatigue live load

   if ( bPermit )
      nCols += 2;

   if ( bPedLoading )
      nCols += 2;

   if ( bSidewalk )
   {
      if (analysisType == pgsTypes::Envelope )
      {
         nCols += 2;
      }
      else
      {
         nCols++;
      }
   }

   if ( bShearKey )
   {
      if (analysisType == pgsTypes::Envelope )
      {
         nCols += 2;
      }
      else
      {
         nCols++;
      }
   }

   rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(nCols,"Rotations");
   RowIndexType row = ConfigureProductLoadTableHeading<rptAngleUnitTag,unitmgtAngleData>(p_table,true,bDeckPanels,bSidewalk,bShearKey,bPedLoading,bPermit,analysisType,continuity_stage,pDisplayUnits,pDisplayUnits->GetRadAngleUnit());

   GET_IFACE2(pBroker,IPointOfInterest,pPOI);
   std::vector<pgsPointOfInterest> vPoi;

   SpanIndexType nSpans = pBridge->GetSpanCount();
   SpanIndexType startSpan = (span == ALL_SPANS ? 0 : span);
   SpanIndexType endSpan   = (span == ALL_SPANS ? nSpans : startSpan+1);
   for ( SpanIndexType spanIdx = startSpan; spanIdx < endSpan; spanIdx++ )
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(spanIdx);
      GirderIndexType gdrIdx = min(gdr,nGirders-1);
      std::vector<pgsPointOfInterest> vTenthPoints = pPOI->GetTenthPointPOIs(pgsTypes::BridgeSite3,spanIdx,gdrIdx);
      vPoi.push_back(*vTenthPoints.begin());
      vPoi.push_back(*(vTenthPoints.end()-1));
   }

   // Fill up the table
   GET_IFACE2(pBroker,IProductForces,pForces);

   for ( pier = startPier; pier < endPier; pier++ )
   {
      int col = 0;

      pgsTypes::Stage girderLoadStage = pLoads->GetGirderDeadLoadStage(gdr);

      if ( pier == 0 || pier == nPiers-1 )
         (*p_table)(row,col++) << "Abutment " << (Int32)(pier+1);
      else
         (*p_table)(row,col++) << "Pier " << (Int32)(pier+1);
   
      
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
         (*p_table)(row,col++) << rotation.SetValue( pForces->GetRotation( overlay_stage, pftOverlay,        poi, MaxSimpleContinuousEnvelope ) );
         (*p_table)(row,col++) << rotation.SetValue( pForces->GetRotation( overlay_stage, pftOverlay,        poi, MinSimpleContinuousEnvelope ) );

         Float64 min, max;
         long minConfig, maxConfig;
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
            (*p_table)(row,col) << rptNewLine << "(D" << maxConfig << ")";
         }
         col++;

         pForces->GetLiveLoadRotation( pgsTypes::lltDesign, pgsTypes::BridgeSite3, poi, MinSimpleContinuousEnvelope, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
         (*p_table)(row,col) << rotation.SetValue( min );
         if ( bIndicateControllingLoad && 0 <= minConfig )
         {
            (*p_table)(row,col) << rptNewLine << "(D" << minConfig << ")";
         }
         col++;

         if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
         {
            pForces->GetLiveLoadRotation( pgsTypes::lltFatigue, pgsTypes::BridgeSite3, poi, MaxSimpleContinuousEnvelope, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
            (*p_table)(row,col) << rotation.SetValue( max );
            if ( bIndicateControllingLoad && 0 <= maxConfig )
            {
               (*p_table)(row,col) << rptNewLine << "(F" << maxConfig << ")";
            }
            col++;

            pForces->GetLiveLoadRotation( pgsTypes::lltFatigue, pgsTypes::BridgeSite3, poi, MinSimpleContinuousEnvelope, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
            (*p_table)(row,col) << rotation.SetValue( min );
            if ( bIndicateControllingLoad && 0 <= minConfig )
            {
               (*p_table)(row,col) << rptNewLine << "(F" << minConfig << ")";
            }
            col++;
         }

         if ( bPermit )
         {
            pForces->GetLiveLoadRotation( pgsTypes::lltPermit, pgsTypes::BridgeSite3, poi, MaxSimpleContinuousEnvelope, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
            (*p_table)(row,col) << rotation.SetValue( max );
            if ( bIndicateControllingLoad && 0 <= maxConfig )
            {
               (*p_table)(row,col) << rptNewLine << "(P" << maxConfig << ")";
            }
            col++;

            pForces->GetLiveLoadRotation( pgsTypes::lltPermit, pgsTypes::BridgeSite3, poi, MinSimpleContinuousEnvelope, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
            (*p_table)(row,col) << rotation.SetValue( min );
            if ( bIndicateControllingLoad && 0 <= minConfig )
            {
               (*p_table)(row,col) << rptNewLine << "(P" << minConfig << ")";
            }
            col++;
         }
      }
      else
      {
         if ( bSidewalk )
         {
            (*p_table)(row,col++) << rotation.SetValue( pForces->GetRotation( pgsTypes::BridgeSite2, pftSidewalk, poi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan ) );
         }

         (*p_table)(row,col++) << rotation.SetValue( pForces->GetRotation( pgsTypes::BridgeSite2, pftTrafficBarrier, poi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan ) );
         (*p_table)(row,col++) << rotation.SetValue( pForces->GetRotation( overlay_stage, pftOverlay,        poi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan ) );

         Float64 min, max;
         long minConfig, maxConfig;
         if ( bPedLoading )
         {
            pForces->GetLiveLoadRotation( pgsTypes::lltPedestrian, pgsTypes::BridgeSite3, poi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, bIncludeImpact, bIncludeLLDF, &min, &max );
            (*p_table)(row,col++) << rotation.SetValue( max );
            (*p_table)(row,col++) << rotation.SetValue( min );
         }

         pForces->GetLiveLoadRotation( pgsTypes::lltDesign, pgsTypes::BridgeSite3, poi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
         (*p_table)(row,col) << rotation.SetValue( max );
         if ( bIndicateControllingLoad && 0 <= maxConfig )
         {
            (*p_table)(row,col) << rptNewLine << "(D" << maxConfig << ")";
         }
         col++;

         (*p_table)(row,col) << rotation.SetValue( min );
         if ( bIndicateControllingLoad && 0 <= minConfig )
         {
            (*p_table)(row,col) << rptNewLine << "(D" << minConfig << ")";
         }
         col++;

         if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
         {
            pForces->GetLiveLoadRotation( pgsTypes::lltFatigue, pgsTypes::BridgeSite3, poi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
            (*p_table)(row,col) << rotation.SetValue( max );
            if ( bIndicateControllingLoad && 0 <= maxConfig )
            {
               (*p_table)(row,col) << rptNewLine << "(F" << maxConfig << ")";
            }
            col++;

            (*p_table)(row,col) << rotation.SetValue( min );
            if ( bIndicateControllingLoad && 0 <= minConfig )
            {
               (*p_table)(row,col) << rptNewLine << "(F" << minConfig << ")";
            }
            col++;
         }

         if ( bPermit )
         {
            pForces->GetLiveLoadRotation( pgsTypes::lltPermit, pgsTypes::BridgeSite3, poi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
            (*p_table)(row,col) << rotation.SetValue( max );
            if ( bIndicateControllingLoad && 0 <= maxConfig )
            {
               (*p_table)(row,col) << rptNewLine << "(P" << maxConfig << ")";
            }
            col++;

            (*p_table)(row,col) << rotation.SetValue( min );
            if ( bIndicateControllingLoad && 0 <= minConfig )
            {
               (*p_table)(row,col) << rptNewLine << "(P" << minConfig << ")";
            }
            col++;
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
   os << "Dump for CProductRotationTable" << endl;
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
