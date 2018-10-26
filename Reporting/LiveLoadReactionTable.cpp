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
#include <Reporting\LiveLoadReactionTable.h>
#include <Reporting\ReportNotes.h>

#include <IFace\Project.h>
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
   CLiveLoadReactionTable
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CLiveLoadReactionTable::CLiveLoadReactionTable()
{
}

CLiveLoadReactionTable::CLiveLoadReactionTable(const CLiveLoadReactionTable& rOther)
{
   MakeCopy(rOther);
}

CLiveLoadReactionTable::~CLiveLoadReactionTable()
{
}

//======================== OPERATORS  =======================================
CLiveLoadReactionTable& CLiveLoadReactionTable::operator= (const CLiveLoadReactionTable& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
void CLiveLoadReactionTable::Build(IBroker* pBroker, rptChapter* pChapter,
                                          SpanIndexType span,GirderIndexType girder,
                                          IDisplayUnits* pDisplayUnits,
                                          pgsTypes::Stage stage, pgsTypes::AnalysisType analysisType) const
{
   // Build table
   INIT_UV_PROTOTYPE( rptLengthUnitValue, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptForceSectionValue, reaction, pDisplayUnits->GetShearUnit(), false );

   GET_IFACE2(pBroker,IBridge,pBridge);

   bool bPermit = false;

   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   rptRcTable* p_table=0;

   PierIndexType nPiers = pBridge->GetPierCount();

   PierIndexType startPier = (span == ALL_SPANS ? 0 : span);
   PierIndexType endPier   = (span == ALL_SPANS ? nPiers : startPier+2 );

   ColumnIndexType nCols;

   GET_IFACE2(pBroker,ILiveLoads,pLiveLoads);
   bPermit = pLiveLoads->IsLiveLoadDefined(pgsTypes::lltPermit);

   GET_IFACE2(pBroker,IProductLoads,pProductLoads);
   bool bPedLoading = pProductLoads->HasPedestrianLoad(startPier,girder);

	if ( analysisType == pgsTypes::Envelope )
		nCols = 9;
	else
		nCols = 7;

   if ( bPermit )
      nCols += 4;

   if ( bPedLoading )
      nCols += 2;

 	p_table = pgsReportStyleHolder::CreateDefaultTable(nCols,"Reactions without Impact");
   p_table->SetNumberOfHeaderRows(2);

   p_table->SetRowSpan(0,0,2);
   p_table->SetRowSpan(1,0,-1);
   (*p_table)(0,0) << "";

 	ColumnIndexType col1 = 1;
   ColumnIndexType col2 = 1;
	if ( analysisType == pgsTypes::Envelope )
	{
      p_table->SetColumnSpan(0,col1,2);
      (*p_table)(0,col1++) << symbol(SUM) << "DC";
      (*p_table)(1,col2++) << COLHDR("Max", rptForceUnitTag, pDisplayUnits->GetShearUnit() );
      (*p_table)(1,col2++) << COLHDR("Min", rptForceUnitTag, pDisplayUnits->GetShearUnit() );

      p_table->SetColumnSpan(0,col1,2);
      (*p_table)(0,col1++) << symbol(SUM) << "DW";
      (*p_table)(1,col2++) << COLHDR("Max", rptForceUnitTag, pDisplayUnits->GetShearUnit() );
      (*p_table)(1,col2++) << COLHDR("Min", rptForceUnitTag, pDisplayUnits->GetShearUnit() );
	}
	else
	{
      p_table->SetRowSpan(0,col1,2);
		(*p_table)(0,col1++) << COLHDR(symbol(SUM) << "DC",          rptForceUnitTag, pDisplayUnits->GetShearUnit() );

      p_table->SetRowSpan(0,col1,2);
		(*p_table)(0,col1++) << COLHDR(symbol(SUM) << "DW",          rptForceUnitTag, pDisplayUnits->GetShearUnit() );

      p_table->SetRowSpan(1,col2++,-1);
      p_table->SetRowSpan(1,col2++,-1);
	}

   if ( bPedLoading )
   {
      p_table->SetColumnSpan(0,col1,2);
      (*p_table)(0,col1++) << "* PL";
		(*p_table)(1,col2++) << COLHDR("Max",       rptForceUnitTag, pDisplayUnits->GetShearUnit() );
		(*p_table)(1,col2++) << COLHDR("Min",       rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   }

   p_table->SetColumnSpan(0,col1,2);
   (*p_table)(0,col1++) << "* LL Design";
	(*p_table)(1,col2++) << COLHDR("Max",       rptForceUnitTag, pDisplayUnits->GetShearUnit() );
	(*p_table)(1,col2++) << COLHDR("Min",       rptForceUnitTag, pDisplayUnits->GetShearUnit() );

	if ( bPermit )
	{
      p_table->SetColumnSpan(0,col1,2);
      (*p_table)(0,col1++) << "* LL Permit";
		(*p_table)(1,col2++) << COLHDR("Max",       rptForceUnitTag, pDisplayUnits->GetShearUnit() );
		(*p_table)(1,col2++) << COLHDR("Min",       rptForceUnitTag, pDisplayUnits->GetShearUnit() );
	}

   p_table->SetColumnSpan(0,col1,2);
   (*p_table)(0,col1++) << "Strength I";
	(*p_table)(1,col2++) << COLHDR("Max", rptForceUnitTag, pDisplayUnits->GetShearUnit() );
	(*p_table)(1,col2++) << COLHDR("Min", rptForceUnitTag, pDisplayUnits->GetShearUnit() );

	if ( bPermit )
	{
      p_table->SetColumnSpan(0,col1,2);
		(*p_table)(0,col1++) << "Strength II";
		(*p_table)(1,col2++) << COLHDR("Max", rptForceUnitTag, pDisplayUnits->GetShearUnit() );
		(*p_table)(1,col2++) << COLHDR("Min", rptForceUnitTag, pDisplayUnits->GetShearUnit() );
	}

   for ( ColumnIndexType i = col1; i < nCols; i++ )
      p_table->SetColumnSpan(0,i,-1);

   *p << p_table;
   *p << LIVELOAD_PER_GIRDER_NO_IMPACT << rptNewLine;

   // Get the interface pointers we need
   GET_IFACE2(pBroker,ICombinedForces,pForces);
   GET_IFACE2(pBroker,ILimitStateForces,pLsForces);

   (*p_table)(0,0) << "";

   // Fill up the table
   Float64 min, max;
   RowIndexType row = 2;
   for ( PierIndexType pier = startPier; pier < endPier; pier++ )
   {
      if (pier == 0 || pier == nPiers-1 )
         (*p_table)(row,0) << "Abutment " << (Int32)(pier+1);
      else
         (*p_table)(row,0) << "Pier " << (Int32)(pier+1);

     ColumnIndexType col = 1;

     if ( analysisType == pgsTypes::Envelope )
     {
        (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDC, stage, pier, girder, ctCummulative, MaxSimpleContinuousEnvelope ) );
        (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDC, stage, pier, girder, ctCummulative, MinSimpleContinuousEnvelope ) );
        (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDW, stage, pier, girder, ctCummulative, MaxSimpleContinuousEnvelope ) );
        (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDW, stage, pier, girder, ctCummulative, MinSimpleContinuousEnvelope ) );

        if ( bPedLoading )
        {
           pForces->GetCombinedLiveLoadReaction( pgsTypes::lltPedestrian, pgsTypes::BridgeSite3, pier, girder, MaxSimpleContinuousEnvelope, false, &min, &max );
           (*p_table)(row,col++) << reaction.SetValue( max );

           pForces->GetCombinedLiveLoadReaction( pgsTypes::lltPedestrian, pgsTypes::BridgeSite3, pier, girder, MinSimpleContinuousEnvelope, false, &min, &max );
           (*p_table)(row,col++) << reaction.SetValue( min );
        }

        pForces->GetCombinedLiveLoadReaction( pgsTypes::lltDesign, pgsTypes::BridgeSite3, pier, girder, MaxSimpleContinuousEnvelope, false, &min, &max );
        (*p_table)(row,col++) << reaction.SetValue( max );

        pForces->GetCombinedLiveLoadReaction( pgsTypes::lltDesign, pgsTypes::BridgeSite3, pier, girder, MinSimpleContinuousEnvelope, false, &min, &max );
        (*p_table)(row,col++) << reaction.SetValue( min );

        if ( bPermit )
        {
           pForces->GetCombinedLiveLoadReaction( pgsTypes::lltPermit, pgsTypes::BridgeSite3, pier, girder, MaxSimpleContinuousEnvelope, false, &min, &max );
           (*p_table)(row,col++) << reaction.SetValue( max );

           pForces->GetCombinedLiveLoadReaction( pgsTypes::lltPermit, pgsTypes::BridgeSite3, pier, girder, MinSimpleContinuousEnvelope, false, &min, &max );
           (*p_table)(row,col++) << reaction.SetValue( min );
        }

        pLsForces->GetReaction( pgsTypes::StrengthI, stage, pier, girder, MaxSimpleContinuousEnvelope, false, &min, &max );
        (*p_table)(row,col++) << reaction.SetValue( max );

        pLsForces->GetReaction( pgsTypes::StrengthI, stage, pier, girder, MinSimpleContinuousEnvelope, false, &min, &max );
        (*p_table)(row,col++) << reaction.SetValue( min );

        if ( bPermit )
        {
           pLsForces->GetReaction( pgsTypes::StrengthII, stage, pier, girder, MaxSimpleContinuousEnvelope, false, &min, &max );
           (*p_table)(row,col++) << reaction.SetValue( max );

           pLsForces->GetReaction( pgsTypes::StrengthII, stage, pier, girder, MinSimpleContinuousEnvelope, false, &min, &max );
           (*p_table)(row,col++) << reaction.SetValue( min );
        }
     }
     else
     {
        BridgeAnalysisType bat = (analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan);
        (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDC, stage, pier, girder, ctCummulative, bat ) );
        (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDW, stage, pier, girder, ctCummulative, bat ) );

        if ( bPedLoading )
        {
           pForces->GetCombinedLiveLoadReaction( pgsTypes::lltPedestrian, pgsTypes::BridgeSite3, pier, girder, bat, false, &min, &max );
           (*p_table)(row,col++) << reaction.SetValue( max );
           (*p_table)(row,col++) << reaction.SetValue( min );
        }

        pForces->GetCombinedLiveLoadReaction( pgsTypes::lltDesign, pgsTypes::BridgeSite3, pier, girder, bat, false, &min, &max );
        (*p_table)(row,col++) << reaction.SetValue( max );
        (*p_table)(row,col++) << reaction.SetValue( min );

        if ( bPermit )
        {
           pForces->GetCombinedLiveLoadReaction( pgsTypes::lltPermit, pgsTypes::BridgeSite3, pier, girder, bat, false, &min, &max );
           (*p_table)(row,col++) << reaction.SetValue( max );
           (*p_table)(row,col++) << reaction.SetValue( min );
        }

        pLsForces->GetReaction( pgsTypes::StrengthI, stage, pier, girder, bat, false, &min, &max );
        (*p_table)(row,col++) << reaction.SetValue( max );
        (*p_table)(row,col++) << reaction.SetValue( min );

        if ( bPermit )
        {
           pLsForces->GetReaction( pgsTypes::StrengthII, stage, pier, girder, bat, false, &min, &max );
           (*p_table)(row,col++) << reaction.SetValue( max );
           (*p_table)(row,col++) << reaction.SetValue( min );
        }
     }

      row++;
   }
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void CLiveLoadReactionTable::MakeCopy(const CLiveLoadReactionTable& rOther)
{
   // Add copy code here...
}

void CLiveLoadReactionTable::MakeAssignment(const CLiveLoadReactionTable& rOther)
{
   MakeCopy( rOther );
}
