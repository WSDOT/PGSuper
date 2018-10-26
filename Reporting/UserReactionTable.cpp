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
#include <Reporting\UserReactionTable.h>
#include <Reporting\UserMomentsTable.h>
#include <Reporting\ReactionInterfaceAdapters.h>

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
                                      TableType tableType, IEAFDisplayUnits* pDisplayUnits) const
{
   // Build table
   INIT_UV_PROTOTYPE( rptLengthUnitValue, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptForceSectionValue, reaction, pDisplayUnits->GetShearUnit(), false );

   rptRcTable* p_table = CreateUserLoadHeading<rptForceUnitTag,unitmgtForceData>( (tableType==PierReactionsTable ? 
                                                                                   _T("Total Girderline Reactions at Abutments and Piers - User Defined Loads") :
                                                                                   _T("Girder Bearing Reactions- User Defined Loads") ),
                                                                                  true,analysisType,pDisplayUnits,pDisplayUnits->GetShearUnit());

   GET_IFACE2(pBroker,IProductForces,pProductForces);
   GET_IFACE2(pBroker,IBearingDesign,pBearingDesign);
   GET_IFACE2(pBroker,IBridge,pBridge);

   GET_IFACE2(pBroker,IProductForces,pProdForces);
   pgsTypes::BridgeAnalysisType maxBAT = pProdForces->GetBridgeAnalysisType(pgsTypes::Maximize);
   pgsTypes::BridgeAnalysisType minBAT = pProdForces->GetBridgeAnalysisType(pgsTypes::Minimize);

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType castDeckIntervalIdx      = pIntervals->GetCastDeckInterval();
   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval();
   IntervalIndexType liveLoadIntervalIdx      = pIntervals->GetLiveLoadInterval();

   PierIndexType startPier = pBridge->GetGirderGroupStartPier(girderKey.groupIndex);
   PierIndexType endPier   = pBridge->GetGirderGroupEndPier(girderKey.groupIndex);

   // Fill up the table
   RowIndexType row = p_table->GetNumberOfHeaderRows();
   for ( PierIndexType pier = startPier; pier <= endPier; pier++ )
   {
      std::auto_ptr<IProductReactionAdapter> pForces;
      if( tableType==PierReactionsTable )
      {
         pForces =  std::auto_ptr<ProductForcesReactionAdapter>(new ProductForcesReactionAdapter(pProductForces));
      }
      else
      {
         pForces =  std::auto_ptr<BearingDesignProductReactionAdapter>(new BearingDesignProductReactionAdapter(pBearingDesign, startPier, endPier) );
      }

      if (!pForces->DoReportAtPier(pier, girderKey))
      {
         continue; // don't report if no bearing
      }

      ColumnIndexType col = 0;

      if ( pier == 0 || pier == pBridge->GetPierCount()-1 )
         (*p_table)(row,col++) << _T("Abutment ") << LABEL_PIER(pier);
      else
         (*p_table)(row,col++) << _T("Pier ") << LABEL_PIER(pier);


      if ( analysisType == pgsTypes::Envelope )
      {
         (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( castDeckIntervalIdx, pftUserDC,         pier, girderKey, maxBAT ) );
         (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( castDeckIntervalIdx, pftUserDC,         pier, girderKey, minBAT ) );
         (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( castDeckIntervalIdx, pftUserDW,         pier, girderKey, maxBAT ) );
         (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( castDeckIntervalIdx, pftUserDW,         pier, girderKey, minBAT ) );

         (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( compositeDeckIntervalIdx, pftUserDC,         pier, girderKey, maxBAT ) );
         (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( compositeDeckIntervalIdx, pftUserDC,         pier, girderKey, minBAT ) );
         (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( compositeDeckIntervalIdx, pftUserDW,         pier, girderKey, maxBAT ) );
         (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( compositeDeckIntervalIdx, pftUserDW,         pier, girderKey, minBAT ) );
         
         (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( liveLoadIntervalIdx, pftUserLLIM,      pier, girderKey, maxBAT ) );
         (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( liveLoadIntervalIdx, pftUserLLIM,      pier, girderKey, minBAT ) );
      }
      else
      {
         (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( castDeckIntervalIdx,      pftUserDC,         pier, girderKey, maxBAT ) );
         (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( castDeckIntervalIdx,      pftUserDW,         pier, girderKey, maxBAT ) );
         (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( compositeDeckIntervalIdx, pftUserDC,         pier, girderKey, maxBAT ) );
         (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( compositeDeckIntervalIdx, pftUserDW,         pier, girderKey, maxBAT ) );
         (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( liveLoadIntervalIdx,      pftUserLLIM,       pier, girderKey, maxBAT ) );
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
