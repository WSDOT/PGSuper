///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright (C) 1999  Washington State Department of Transportation
//                     Bridge and Structures Office
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
#include <IFace\DisplayUnits.h>
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
                                      IDisplayUnits* pDispUnits) const
{
   // Build table
   INIT_UV_PROTOTYPE( rptLengthUnitValue, location, pDispUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptForceSectionValue, reaction, pDispUnits->GetShearUnit(), false );

   rptRcTable* p_table = CreateUserLoadHeading<rptForceUnitTag,unitmgtForceData>("Reactions - User Defined Loads",true,analysisType,pDispUnits,pDispUnits->GetShearUnit());

   GET_IFACE2(pBroker,IProductForces,pForces);
   GET_IFACE2(pBroker,IBridge,pBridge);

   PierIndexType nPiers = pBridge->GetPierCount();

   PierIndexType startPier = (span == ALL_SPANS ? 0 : span);
   PierIndexType endPier   = (span == ALL_SPANS ? nPiers : startPier+2 );

   // Fill up the table
   RowIndexType row = p_table->GetNumberOfHeaderRows();
   for ( PierIndexType pier = startPier; pier < endPier; pier++ )
   {
      ColumnIndexType col = 0;

      if ( pier == 0 || pier == nPiers-1 )
         (*p_table)(row,col++) << "Abutment " << (Int32)(pier+1);
      else
         (*p_table)(row,col++) << "Pier " << (Int32)(pier+1);


      if ( analysisType == pgsTypes::Envelope )
      {
         (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( pgsTypes::BridgeSite1, pftUserDC,         pier, girder, MaxSimpleContinuousEnvelope ) );
         (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( pgsTypes::BridgeSite1, pftUserDC,         pier, girder, MinSimpleContinuousEnvelope ) );
         (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( pgsTypes::BridgeSite1, pftUserDW,         pier, girder, MaxSimpleContinuousEnvelope ) );
         (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( pgsTypes::BridgeSite1, pftUserDW,         pier, girder, MinSimpleContinuousEnvelope ) );

         (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( pgsTypes::BridgeSite2, pftUserDC,         pier, girder, MaxSimpleContinuousEnvelope ) );
         (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( pgsTypes::BridgeSite2, pftUserDC,         pier, girder, MinSimpleContinuousEnvelope ) );
         (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( pgsTypes::BridgeSite2, pftUserDW,         pier, girder, MaxSimpleContinuousEnvelope ) );
         (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( pgsTypes::BridgeSite2, pftUserDW,         pier, girder, MinSimpleContinuousEnvelope ) );
         
         (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( pgsTypes::BridgeSite3, pftUserLLIM,      pier, girder, MaxSimpleContinuousEnvelope ) );
         (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( pgsTypes::BridgeSite3, pftUserLLIM,      pier, girder, MinSimpleContinuousEnvelope ) );
      }
      else
      {
         (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( pgsTypes::BridgeSite1, pftUserDC,         pier, girder, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan ) );
         (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( pgsTypes::BridgeSite1, pftUserDW,         pier, girder, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan ) );
         (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( pgsTypes::BridgeSite2, pftUserDC,         pier, girder, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan ) );
         (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( pgsTypes::BridgeSite2, pftUserDW,         pier, girder, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan ) );
         (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( pgsTypes::BridgeSite3, pftUserLLIM,      pier, girder, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan ) );
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
   os << "Dump for CUserReactionTable" << endl;
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
