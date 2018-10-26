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

#include <IFace\Bridge.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\AnalysisResults.h>

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
rptRcTable* CUserReactionTable::Build(IBroker* pBroker,SpanIndexType span,GirderIndexType girder,pgsTypes::AnalysisType analysisType,
                                      ReactionTableType tableType, IEAFDisplayUnits* pDisplayUnits) const
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

   // TRICKY: use adapter class to get correct reaction interfaces
   std::auto_ptr<IProductReactionAdapter> pForces;
   if( tableType==PierReactionsTable )
   {
      pForces =  std::auto_ptr<ProductForcesReactionAdapter>(new ProductForcesReactionAdapter(pProductForces,span, girder));
   }
   else
   {
      pForces =  std::auto_ptr<BearingDesignProductReactionAdapter>(new BearingDesignProductReactionAdapter(pBearingDesign, pgsTypes::GirderPlacement, span, girder) );
   }

   // Fill up the table
   RowIndexType row = p_table->GetNumberOfHeaderRows();

   // User iterator to walk locations
   ReactionLocationIter iter = pForces->GetReactionLocations(pBridge);

   for (iter.First(); !iter.IsDone(); iter.Next())
   {
      ColumnIndexType col = 0;

      const ReactionLocation& rct_locn = iter.CurrentItem();

      (*p_table)(row,col++) << rct_locn.PierLabel;

      // Use reaction decider tool to determine when to report stages
      ReactionDecider rctdr(tableType, rct_locn, pBridge);

      if ( analysisType == pgsTypes::Envelope )
      {
         if (rctdr.DoReport(pgsTypes::BridgeSite1 ))
         {
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( pgsTypes::BridgeSite1, rct_locn, pftUserDC,       MaxSimpleContinuousEnvelope ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( pgsTypes::BridgeSite1, rct_locn, pftUserDC,       MinSimpleContinuousEnvelope ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( pgsTypes::BridgeSite1, rct_locn, pftUserDW,       MaxSimpleContinuousEnvelope ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( pgsTypes::BridgeSite1, rct_locn, pftUserDW,       MinSimpleContinuousEnvelope ) );
         }
         else
         {
            (*p_table)(row,col++) << RPT_NA;
            (*p_table)(row,col++) << RPT_NA;
            (*p_table)(row,col++) << RPT_NA;
            (*p_table)(row,col++) << RPT_NA;
         }

         if (rctdr.DoReport(pgsTypes::BridgeSite2 ))
         {
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( pgsTypes::BridgeSite2, rct_locn, pftUserDC,       MaxSimpleContinuousEnvelope ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( pgsTypes::BridgeSite2, rct_locn, pftUserDC,       MinSimpleContinuousEnvelope ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( pgsTypes::BridgeSite2, rct_locn, pftUserDW,       MaxSimpleContinuousEnvelope ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( pgsTypes::BridgeSite2, rct_locn, pftUserDW,       MinSimpleContinuousEnvelope ) );
         }
         else
         {
            (*p_table)(row,col++) << RPT_NA;
            (*p_table)(row,col++) << RPT_NA;
            (*p_table)(row,col++) << RPT_NA;
            (*p_table)(row,col++) << RPT_NA;
         }
         
         if (rctdr.DoReport(pgsTypes::BridgeSite3 ))
         {
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( pgsTypes::BridgeSite3, rct_locn, pftUserLLIM,    MaxSimpleContinuousEnvelope ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( pgsTypes::BridgeSite3, rct_locn, pftUserLLIM,    MinSimpleContinuousEnvelope ) );
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
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( pgsTypes::BridgeSite1, rct_locn, pftUserDC,       analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( pgsTypes::BridgeSite1, rct_locn, pftUserDW,       analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan ) );
         }
         else
         {
            (*p_table)(row,col++) << RPT_NA;
            (*p_table)(row,col++) << RPT_NA;
         }

         if (rctdr.DoReport(pgsTypes::BridgeSite3 ))
         {
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( pgsTypes::BridgeSite2, rct_locn, pftUserDC,       analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( pgsTypes::BridgeSite2, rct_locn, pftUserDW,       analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan ) );
         }
         else
         {
            (*p_table)(row,col++) << RPT_NA;
            (*p_table)(row,col++) << RPT_NA;
         }

         if (rctdr.DoReport(pgsTypes::BridgeSite3 ))
         {
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( pgsTypes::BridgeSite3, rct_locn, pftUserLLIM,     analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan ) );
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
