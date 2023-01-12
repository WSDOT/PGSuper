///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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
                                      ReactionTableType tableType, IntervalIndexType intervalIdx,IEAFDisplayUnits* pDisplayUnits) const
{
   // Build table
   INIT_UV_PROTOTYPE( rptLengthUnitValue, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptForceSectionValue, reaction, pDisplayUnits->GetShearUnit(), false );

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetFirstCompositeDeckInterval();

   CString strTitle;
   if ( tableType == PierReactionsTable )
   {
      strTitle.Format(_T("Girder Line Pier Reactions due to User Defined Loads in Interval %d: %s"),LABEL_INTERVAL(intervalIdx),pIntervals->GetDescription(intervalIdx));
   }
   else
   {
      strTitle.Format(_T("Girder Bearing Reactions due to User Defined Loads in Interval %d: %s"),LABEL_INTERVAL(intervalIdx),pIntervals->GetDescription(intervalIdx));
   }

   rptRcTable* p_table = CreateUserLoadHeading<rptForceUnitTag,unitmgtForceData>( strTitle.GetBuffer(),
                                                                                  true,analysisType,intervalIdx,pDisplayUnits,pDisplayUnits->GetShearUnit());

   p_table->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   p_table->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   GET_IFACE2(pBroker,IBridge,pBridge);
   PierIndexType nPiers = pBridge->GetPierCount();

   GET_IFACE2(pBroker,IProductForces,pProductForces);
   pgsTypes::BridgeAnalysisType maxBAT = pProductForces->GetBridgeAnalysisType(analysisType,pgsTypes::Maximize);
   pgsTypes::BridgeAnalysisType minBAT = pProductForces->GetBridgeAnalysisType(analysisType,pgsTypes::Minimize);

   // Fill up the table
   RowIndexType row = p_table->GetNumberOfHeaderRows();

   std::unique_ptr<IProductReactionAdapter> pForces;
   if( tableType == PierReactionsTable )
   {
      GET_IFACE2(pBroker,IReactions,pReactions);
      pForces = std::make_unique<ProductForcesReactionAdapter>(pReactions,girderKey);
   }
   else
   {
      GET_IFACE2(pBroker,IBearingDesign,pBearingDesign);
      pForces = std::make_unique<BearingDesignProductReactionAdapter>(pBearingDesign, compositeDeckIntervalIdx, girderKey);
   }

   // User iterator to walk locations
   ReactionLocationIter iter = pForces->GetReactionLocations(pBridge);

   for (iter.First(); !iter.IsDone(); iter.Next())
   {
      ColumnIndexType col = 0;

      const ReactionLocation& reactionLocation( iter.CurrentItem() );

      const CGirderKey& thisGirderKey(reactionLocation.GirderKey);

      (*p_table)(row,col++) << reactionLocation.PierLabel;

      // Use reaction decider tool to determine when to report
      ReactionDecider reactionDecider(tableType, reactionLocation, thisGirderKey, pBridge, pIntervals);

      if ( analysisType == pgsTypes::Envelope )
      {
         if (reactionDecider.DoReport(intervalIdx))
         {
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( intervalIdx, reactionLocation, pgsTypes::pftUserDC,       maxBAT ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( intervalIdx, reactionLocation, pgsTypes::pftUserDC,       minBAT ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( intervalIdx, reactionLocation, pgsTypes::pftUserDW,       maxBAT ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( intervalIdx, reactionLocation, pgsTypes::pftUserDW,       minBAT ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( intervalIdx, reactionLocation, pgsTypes::pftUserLLIM,    maxBAT ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( intervalIdx, reactionLocation, pgsTypes::pftUserLLIM,    minBAT ) );
         }
         else
         {
            (*p_table)(row,col++) << RPT_NA;
            (*p_table)(row,col++) << RPT_NA;
            (*p_table)(row,col++) << RPT_NA;
            (*p_table)(row,col++) << RPT_NA;
            (*p_table)(row,col++) << RPT_NA;
            (*p_table)(row,col++) << RPT_NA;
         }
      }
      else
      {
         if (reactionDecider.DoReport(intervalIdx))
         {
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( intervalIdx, reactionLocation, pgsTypes::pftUserDC,  maxBAT ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( intervalIdx, reactionLocation, pgsTypes::pftUserDW,  maxBAT ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( intervalIdx, reactionLocation, pgsTypes::pftUserLLIM,   maxBAT ) );
         }
         else
         {
            (*p_table)(row,col++) << RPT_NA;
            (*p_table)(row,col++) << RPT_NA;
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
