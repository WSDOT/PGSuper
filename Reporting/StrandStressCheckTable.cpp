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
#include <Reporting\StrandStressCheckTable.h>

#include <PgsExt\StrandStressArtifact.h>
#include <PgsExt\GirderData.h>
#include <PgsExt\CapacityToDemand.h>

#include <IFace\DisplayUnits.h>
#include <IFace\Project.h>
#include <IFace\Bridge.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/****************************************************************************
CLASS
   CStrandStressCheckTable
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CStrandStressCheckTable::CStrandStressCheckTable()
{
}

CStrandStressCheckTable::CStrandStressCheckTable(const CStrandStressCheckTable& rOther)
{
   MakeCopy(rOther);
}

CStrandStressCheckTable::~CStrandStressCheckTable()
{
}

//======================== OPERATORS  =======================================
CStrandStressCheckTable& CStrandStressCheckTable::operator= (const CStrandStressCheckTable& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
rptRcTable* CStrandStressCheckTable::Build(IBroker* pBroker,const pgsStrandStressArtifact* pArtifact,
                                           IDisplayUnits* pDisplayUnits) const
{
   // Get strand types that are checked
   pgsPointOfInterest poi = pArtifact->GetPointOfInterest();

   GET_IFACE2(pBroker,IGirderData,pGirderData);
   CGirderData girderData = pGirderData->GetGirderData(poi.GetSpan(),poi.GetGirder());

   std::vector<pgsTypes::StrandType> strandTypes;
   if ( girderData.NumPermStrandsType == NPS_TOTAL_NUMBER )
   {
      strandTypes.push_back(pgsTypes::Permanent);
   }
   else
   {
      strandTypes.push_back(pgsTypes::Straight);
      strandTypes.push_back(pgsTypes::Harped);
   }

   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);
   long Nt = pStrandGeom->GetNumStrands(poi.GetSpan(),poi.GetGirder(),pgsTypes::Temporary);
   if ( 0 < Nt )
   {
      strandTypes.push_back(pgsTypes::Temporary);
   }

   // Build table
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), false );


   rptCapacityToDemand cap_demand;

   int nColumns = 2 + 2*strandTypes.size();

   rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(nColumns,"Strand Stresses [5.9.3]");
   p_table->SetNumberOfHeaderRows(2);
   p_table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
   p_table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   // Label columns
   int col1 = 0;
   int col2 = 0;
   p_table->SetRowSpan(0,col1,2);
   (*p_table)(0,col1++) << "Loss Stage";
   p_table->SetRowSpan(1,col2++,-1);

   p_table->SetRowSpan(0,col1,2);
   (*p_table)(0,col1++) << COLHDR("Allowable" << rptNewLine << "Stress", rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   p_table->SetRowSpan(1,col2++,-1);

   std::vector<pgsTypes::StrandType>::iterator iter;
   for ( iter = strandTypes.begin(); iter != strandTypes.end(); iter++ )
   {
      pgsTypes::StrandType strandType = *iter;
      p_table->SetColumnSpan(0,col1,2);
      switch(strandType)
      {
      case pgsTypes::Straight:
         (*p_table)(0,col1++) << "Straight";
         break;

      case pgsTypes::Harped:
         (*p_table)(0,col1++) << "Harped";
         break;

      case pgsTypes::Permanent:
         (*p_table)(0,col1++) << "Permanent";
         break;

      case pgsTypes::Temporary:
         (*p_table)(0,col1++) << "Temporary";
         break;
      }

      (*p_table)(1,col2++) << COLHDR("Strand" << rptNewLine << "Stress", rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*p_table)(1,col2++) << "Status" << rptNewLine << "(C/D)";
   }

   for ( int i = col1; i < nColumns; i++ )
      p_table->SetColumnSpan(0,i,-1);

   Float64 demand, capacity;
   bool bPassed;

   for ( iter = strandTypes.begin(); iter != strandTypes.end(); iter++ )
   {
      pgsTypes::StrandType strandType = *iter;

      Uint16 row = 2;
      int startColumn = 2*(iter - strandTypes.begin()) + 2;
      if ( strandType == strandTypes.front() )
         startColumn -= 2;

      int col = startColumn;
      if ( pArtifact->IsCheckAtJackingApplicable(strandType) )
      {
	      pArtifact->GetCheckAtJacking( strandType, &demand, &capacity, &bPassed );

         if ( strandType == strandTypes.front() )
         {
	         (*p_table)(row,col++) << "At Jacking";
	         (*p_table)(row,col++) << stress.SetValue( capacity );
         }
	      (*p_table)(row,col++) << stress.SetValue( demand );

	      if ( bPassed )
		      (*p_table)(row,col) << RPT_PASS;
	      else
		      (*p_table)(row,col) << RPT_FAIL;

         (*p_table)(row,col++) << rptNewLine << "(" << cap_demand.SetValue(capacity,demand,bPassed) << ")";

         row++;
      }

      col = startColumn;
      if ( pArtifact->IsCheckBeforeXferApplicable(strandType) )
      {
	      pArtifact->GetCheckBeforeXfer( strandType, &demand, &capacity, &bPassed );
         if ( strandType == strandTypes.front() )
         {
   	      (*p_table)(row,col++) << "Before Prestress Transfer";
	         (*p_table)(row,col++) << stress.SetValue( capacity );
         }
	      (*p_table)(row,col++) << stress.SetValue( demand );

	      if ( bPassed )
		      (*p_table)(row,col) << RPT_PASS;
	      else
		      (*p_table)(row,col) << RPT_FAIL;

         (*p_table)(row,col++) << rptNewLine << "(" << cap_demand.SetValue(capacity,demand,bPassed) << ")";

         row++;
      }

      col = startColumn;
      if ( pArtifact->IsCheckAfterXferApplicable(strandType) )
      {
	      pArtifact->GetCheckAfterXfer( strandType, &demand, &capacity, &bPassed );
         if ( strandType == strandTypes.front() )
         {
   	      (*p_table)(row,col++) << "After Prestress Transfer";
	         (*p_table)(row,col++) << stress.SetValue( capacity );
         }
	      (*p_table)(row,col++) << stress.SetValue( demand );

	      if ( bPassed )
		      (*p_table)(row,col) << RPT_PASS;
	      else
		      (*p_table)(row,col) << RPT_FAIL;

         (*p_table)(row,col++) << rptNewLine << "(" << cap_demand.SetValue(capacity,demand,bPassed) << ")";

         row++;
      }

      col = startColumn;
      if ( pArtifact->IsCheckAfterLossesApplicable(strandType) )
      {
	      pArtifact->GetCheckAfterLosses( strandType, &demand, &capacity, &bPassed );
         if ( strandType == strandTypes.front() )
         {
   	      (*p_table)(row,col++) << "After All Losses";
	         (*p_table)(row,col++) << stress.SetValue( capacity );
         }
	      (*p_table)(row,col++) << stress.SetValue( demand );

	      if ( bPassed )
		      (*p_table)(row,col) << RPT_PASS;
	      else
		      (*p_table)(row,col) << RPT_FAIL;

         (*p_table)(row,col++) << rptNewLine << "(" << cap_demand.SetValue(capacity,demand,bPassed) << ")";

         row++;
      }
      else
      {
         (*p_table)(row,col++) << "";
         (*p_table)(row,col++) << "";
      }
   }

   return p_table;
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void CStrandStressCheckTable::MakeCopy(const CStrandStressCheckTable& rOther)
{
   // Add copy code here...
}

void CStrandStressCheckTable::MakeAssignment(const CStrandStressCheckTable& rOther)
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
bool CStrandStressCheckTable::AssertValid() const
{
   return true;
}

void CStrandStressCheckTable::Dump(dbgDumpContext& os) const
{
   os << "Dump for CStrandStressCheckTable" << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool CStrandStressCheckTable::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("CStrandStressCheckTable");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for CStrandStressCheckTable");

   TESTME_EPILOG("CStrandStressCheckTable");
}
#endif // _UNITTEST
