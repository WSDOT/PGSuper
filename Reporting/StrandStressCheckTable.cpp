///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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

#include <PgsExt\GirderArtifact.h>
#include <PgsExt\StrandStressArtifact.h>
#include <PgsExt\CapacityToDemand.h>
#include <PgsExt\StrandData.h>


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
void CStrandStressCheckTable::Build(rptChapter* pChapter,IBroker* pBroker,const pgsGirderArtifact* pGirderArtifact,IEAFDisplayUnits* pDisplayUnits) const
{
   const CGirderKey& girderKey = pGirderArtifact->GetGirderKey();

   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,ISegmentData,pSegmentData);
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);

   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), false );

   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);

   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pPara << _T("Strand Stresses");
   pPara->SetName(_T("Strand Stresses"));
   *pChapter << pPara;

   pPara = new rptParagraph;
   *pChapter << pPara;

   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      // Get strand types that are checked
      CSegmentKey segmentKey(girderKey,segIdx);

      const pgsSegmentArtifact* pSegmentArtifact = pGirderArtifact->GetSegmentArtifact(segIdx);
      const pgsStrandStressArtifact* pArtifact = pSegmentArtifact->GetStrandStressArtifact();

      const CStrandData* pStrands = pSegmentData->GetStrandData(segmentKey);

      std::vector<pgsTypes::StrandType> strandTypes{ pgsTypes::Straight, pgsTypes::Harped };

      StrandIndexType Nt = pStrandGeom->GetStrandCount(segmentKey,pgsTypes::Temporary);
      if ( 0 < Nt )
      {
         strandTypes.push_back(pgsTypes::Temporary);
      }

      // Build table
      rptCapacityToDemand cap_demand;

      ColumnIndexType nColumns = 2 + 2*strandTypes.size();

      CString strTitle;
      if ( 1 < nSegments )
      {
         strTitle.Format(_T("Segment %d"),LABEL_SEGMENT(segIdx));
      }

      rptRcTable* p_table = rptStyleManager::CreateDefaultTable(nColumns,strTitle);
      *pPara << p_table;

      p_table->SetNumberOfHeaderRows(2);
      p_table->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
      p_table->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

      // Label columns
      ColumnIndexType col = 0;
      p_table->SetRowSpan(0,col,2);
      (*p_table)(0,col++) << _T("Loss Stage");

      p_table->SetRowSpan(0,col,2);
      (*p_table)(0,col++) << COLHDR(_T("Stress") << rptNewLine << _T("Limit"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );

      std::vector<pgsTypes::StrandType>::iterator iter;
      for ( iter = strandTypes.begin(); iter != strandTypes.end(); iter++ )
      {
         pgsTypes::StrandType strandType = *iter;
         p_table->SetColumnSpan(0,col,2);
         switch(strandType)
         {
         case pgsTypes::Straight:
            (*p_table)(0,col) << _T("Straight");
            break;

         case pgsTypes::Harped:
            (*p_table)(0,col) << LABEL_HARP_TYPE(pStrandGeom->GetAreHarpedStrandsForcedStraight(segmentKey));
            break;

         case pgsTypes::Temporary:
            (*p_table)(0,col) << _T("Temporary");
            break;

         default:
            ATLASSERT(false); // shouldn't get here
         }

         (*p_table)(1,col++) << COLHDR(_T("Strand") << rptNewLine << _T("Stress"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
         (*p_table)(1,col++) << _T("Status") << rptNewLine << _T("(C/D)");
      }

      Float64 demand, capacity;
      bool bPassed;

      for ( iter = strandTypes.begin(); iter != strandTypes.end(); iter++ )
      {
         pgsTypes::StrandType strandType = *iter;

         RowIndexType row = 2;
         ColumnIndexType startColumn = 2*(iter - strandTypes.begin()) + 2;
         if ( strandType == strandTypes.front() )
         {
            startColumn -= 2;
         }

         ColumnIndexType col = startColumn;
         if ( pArtifact->IsCheckAtJackingApplicable(strandType) )
         {
	         pArtifact->GetCheckAtJacking( strandType, &demand, &capacity, &bPassed );

            if ( strandType == strandTypes.front() )
            {
	            (*p_table)(row,col++) << _T("At Jacking (") << RPT_FPJ << _T(")");
	            (*p_table)(row,col++) << stress.SetValue( capacity );
            }
	         (*p_table)(row,col++) << stress.SetValue( demand );

	         if ( bPassed )
            {
		         (*p_table)(row,col) << RPT_PASS;
            }
	         else
            {
		         (*p_table)(row,col) << RPT_FAIL;
            }

            (*p_table)(row,col++) << rptNewLine << _T("(") << cap_demand.SetValue(capacity,demand,bPassed) << _T(")");

            row++;
         }

         col = startColumn;
         if ( pArtifact->IsCheckBeforeXferApplicable(strandType) )
         {
	         pArtifact->GetCheckBeforeXfer( strandType, &demand, &capacity, &bPassed );
            if ( strandType == strandTypes.front() )
            {
   	         (*p_table)(row,col++) << _T("Before Prestress Transfer (") << RPT_FPBT << _T(")");
	            (*p_table)(row,col++) << stress.SetValue( capacity );
            }
	         (*p_table)(row,col++) << stress.SetValue( demand );

	         if ( bPassed )
            {
		         (*p_table)(row,col) << RPT_PASS;
            }
	         else
            {
		         (*p_table)(row,col) << RPT_FAIL;
            }

            (*p_table)(row,col++) << rptNewLine << _T("(") << cap_demand.SetValue(capacity,demand,bPassed) << _T(")");

            row++;
         }

         col = startColumn;
         if ( pArtifact->IsCheckAfterXferApplicable(strandType) )
         {
	         pArtifact->GetCheckAfterXfer( strandType, &demand, &capacity, &bPassed );
            if ( strandType == strandTypes.front() )
            {
   	         (*p_table)(row,col++) << _T("After Prestress Transfer (") << RPT_FPT << _T(")");
	            (*p_table)(row,col++) << stress.SetValue( capacity );
            }
	         (*p_table)(row,col++) << stress.SetValue( demand );

	         if ( bPassed )
            {
		         (*p_table)(row,col) << RPT_PASS;
            }
	         else
            {
		         (*p_table)(row,col) << RPT_FAIL;
            }

            (*p_table)(row,col++) << rptNewLine << _T("(") << cap_demand.SetValue(capacity,demand,bPassed) << _T(")");

            row++;
         }

         col = startColumn;
         if ( pArtifact->IsCheckAfterLossesApplicable(strandType) )
         {
	         pArtifact->GetCheckAfterLosses( strandType, &demand, &capacity, &bPassed );
            if ( strandType == strandTypes.front() )
            {
   	         (*p_table)(row,col++) << _T("After All Losses and Elastic Gains") << rptNewLine << _T("including Live Load (") << RPT_FPE << _T(")");
	            (*p_table)(row,col++) << stress.SetValue( capacity );
            }
	         (*p_table)(row,col++) << stress.SetValue( demand );

	         if ( bPassed )
            {
		         (*p_table)(row,col) << RPT_PASS;
            }
	         else
            {
		         (*p_table)(row,col) << RPT_FAIL;
            }

            (*p_table)(row,col++) << rptNewLine << _T("(") << cap_demand.SetValue(capacity,demand,bPassed) << _T(")");

            row++;
         }
         else
         {
            (*p_table)(row,col++) << _T("");
            (*p_table)(row,col++) << _T("");
         }
      } // next strand type
   } // next segment
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
