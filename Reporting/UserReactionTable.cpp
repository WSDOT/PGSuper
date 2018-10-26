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
#include <Reporting\UserReactionTable.h>
#include <Reporting\UserMomentsTable.h>

#include <IFace\Bridge.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Intervals.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CUserReactionTable
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CUserReactionTable::CUserReactionTable()
{
}

CUserReactionTable::CUserReactionTable(const CUserReactionTable& rOther)
{
   MakeCopy(rOther);
}

CUserReactionTable::~CUserReactionTable()
{
}

//======================== OPERATORS  =======================================
CUserReactionTable& CUserReactionTable::operator= (const CUserReactionTable& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
rptRcTable* CUserReactionTable::Build(IBroker* pBroker,const CGirderKey& girderKey,pgsTypes::AnalysisType analysisType,
                                      ReactionTableType tableType, IEAFDisplayUnits* pDisplayUnits) const
{
   // Build table
   INIT_UV_PROTOTYPE( rptLengthUnitValue, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptForceSectionValue, reaction, pDisplayUnits->GetShearUnit(), false );

   rptRcTable* p_table = CreateUserLoadHeading<rptForceUnitTag,unitmgtForceData>( (tableType==PierReactionsTable ? 
                                                                                   _T("Total Girderline Reactions at Abutments and Piers - User Defined Loads") :
                                                                                   _T("Girder Bearing Reactions - User Defined Loads") ),
                                                                                  true,analysisType,pDisplayUnits,pDisplayUnits->GetShearUnit());

   GET_IFACE2(pBroker,IProductForces,pProductForces);
   GET_IFACE2(pBroker,IBearingDesign,pBearingDesign);
   GET_IFACE2(pBroker,IBridge,pBridge);
   PierIndexType nPiers = pBridge->GetPierCount();

   GET_IFACE2(pBroker,IProductForces,pProdForces);
   pgsTypes::BridgeAnalysisType maxBAT = pProdForces->GetBridgeAnalysisType(analysisType,pgsTypes::Maximize);
   pgsTypes::BridgeAnalysisType minBAT = pProdForces->GetBridgeAnalysisType(analysisType,pgsTypes::Minimize);

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType castDeckIntervalIdx      = pIntervals->GetCastDeckInterval();
   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval();
   IntervalIndexType railingSystemIntervalIdx = pIntervals->GetInstallRailingSystemInterval();
   IntervalIndexType liveLoadIntervalIdx      = pIntervals->GetLiveLoadInterval();

   // Fill up the table
   RowIndexType row = p_table->GetNumberOfHeaderRows();

   std::auto_ptr<IProductReactionAdapter> pForces;
   if( tableType == PierReactionsTable )
   {
      pForces = std::auto_ptr<ProductForcesReactionAdapter>(new ProductForcesReactionAdapter(pProductForces,girderKey));
   }
   else
   {
      pForces = std::auto_ptr<BearingDesignProductReactionAdapter>(new BearingDesignProductReactionAdapter(pBearingDesign, compositeDeckIntervalIdx, girderKey) );
   }

   // User iterator to walk locations
   ReactionLocationIter iter = pForces->GetReactionLocations(pBridge);

   for (iter.First(); !iter.IsDone(); iter.Next())
   {
      ColumnIndexType col = 0;

      const ReactionLocation& reactionLocation( iter.CurrentItem() );

      (*p_table)(row,col++) << reactionLocation.PierLabel;

      // Use reaction decider tool to determine when to report
      ReactionDecider rctdr(tableType, reactionLocation, pBridge, pIntervals);

      if ( analysisType == pgsTypes::Envelope )
      {
         if (rctdr.DoReport(castDeckIntervalIdx))
         {
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( castDeckIntervalIdx, reactionLocation, pftUserDC,       maxBAT ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( castDeckIntervalIdx, reactionLocation, pftUserDC,       minBAT ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( castDeckIntervalIdx, reactionLocation, pftUserDW,       maxBAT ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( castDeckIntervalIdx, reactionLocation, pftUserDW,       minBAT ) );
         }
         else
         {
            (*p_table)(row,col++) << RPT_NA;
            (*p_table)(row,col++) << RPT_NA;
            (*p_table)(row,col++) << RPT_NA;
            (*p_table)(row,col++) << RPT_NA;
         }

         if (rctdr.DoReport(railingSystemIntervalIdx))
         {
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( railingSystemIntervalIdx, reactionLocation, pftUserDC,       maxBAT ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( railingSystemIntervalIdx, reactionLocation, pftUserDC,       minBAT ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( railingSystemIntervalIdx, reactionLocation, pftUserDW,       maxBAT ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( railingSystemIntervalIdx, reactionLocation, pftUserDW,       minBAT ) );
         }
         else
         {
            (*p_table)(row,col++) << RPT_NA;
            (*p_table)(row,col++) << RPT_NA;
            (*p_table)(row,col++) << RPT_NA;
            (*p_table)(row,col++) << RPT_NA;
         }
         
         if (rctdr.DoReport(liveLoadIntervalIdx))
         {
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( liveLoadIntervalIdx, reactionLocation, pftUserLLIM,    maxBAT ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( liveLoadIntervalIdx, reactionLocation, pftUserLLIM,    minBAT ) );
         }
         else
         {
            (*p_table)(row,col++) << RPT_NA;
            (*p_table)(row,col++) << RPT_NA;
         }
      }
      else
      {
         if (rctdr.DoReport(castDeckIntervalIdx))
         {
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( castDeckIntervalIdx, reactionLocation, pftUserDC,  maxBAT ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( castDeckIntervalIdx, reactionLocation, pftUserDW,  maxBAT ) );
         }
         else
         {
            (*p_table)(row,col++) << RPT_NA;
            (*p_table)(row,col++) << RPT_NA;
         }

         if (rctdr.DoReport(railingSystemIntervalIdx))
         {
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( railingSystemIntervalIdx, reactionLocation, pftUserDC,  maxBAT ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( railingSystemIntervalIdx, reactionLocation, pftUserDW,  maxBAT ) );
         }
         else
         {
            (*p_table)(row,col++) << RPT_NA;
            (*p_table)(row,col++) << RPT_NA;
         }

         if (rctdr.DoReport(liveLoadIntervalIdx))
         {
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( liveLoadIntervalIdx, reactionLocation, pftUserLLIM,   maxBAT ) );
         }
         else
         {
            (*p_table)(row,col++) << RPT_NA;
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
void CUserReactionTable::MakeCopy(const CUserReactionTable& rOther)
{
   // Add copy code here...
}

void CUserReactionTable::MakeAssignment(const CUserReactionTable& rOther)
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
bool CUserReactionTable::AssertValid() const
{
   return true;
}

void CUserReactionTable::Dump(dbgDumpContext& os) const
{
   os << _T("Dump for CUserReactionTable") << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool CUserReactionTable::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("CUserReactionTable");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for CUserReactionTable");

   TESTME_EPILOG("CUserReactionTable");
}
#endif // _UNITTEST
