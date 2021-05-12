///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2021  Washington State Department of Transportation
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
#include <Reporting\TendonStressCheckTable.h>

#include <PgsExt\GirderArtifact.h>
#include <PgsExt\TendonStressArtifact.h>
#include <PgsExt\CapacityToDemand.h>

#include <IFace\Bridge.h>
#include <IFace\Allowables.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/****************************************************************************
CLASS
   CTendonStressCheckTable
****************************************************************************/

CTendonStressCheckTable::CTendonStressCheckTable()
{
}

CTendonStressCheckTable::CTendonStressCheckTable(const CTendonStressCheckTable& rOther)
{
   MakeCopy(rOther);
}

CTendonStressCheckTable::~CTendonStressCheckTable()
{
}

CTendonStressCheckTable& CTendonStressCheckTable::operator= (const CTendonStressCheckTable& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
void CTendonStressCheckTable::Build(rptChapter* pChapter,IBroker* pBroker,const pgsGirderArtifact* pGirderArtifact,IEAFDisplayUnits* pDisplayUnits) const
{
   const CGirderKey& girderKey = pGirderArtifact->GetGirderKey();

   GET_IFACE2(pBroker, ISegmentTendonGeometry, pSegmentTendonGeometry);
   DuctIndexType nMaxSegmentDucts = pSegmentTendonGeometry->GetMaxDuctCount(girderKey);

   GET_IFACE2(pBroker,IGirderTendonGeometry,pGirderTendonGeometry);
   DuctIndexType nGirderDucts = pGirderTendonGeometry->GetDuctCount(girderKey);
   if (nMaxSegmentDucts+nGirderDucts == 0 )
   {
      return;
   }

   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), false );

   GET_IFACE2(pBroker,IAllowableTendonStress,pAllowable);
   
   rptCapacityToDemand cap_demand;

   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pPara << _T("Tendon Stresses");
   pPara->SetName(_T("Tendon Stresses"));
   *pChapter << pPara;

   GET_IFACE2(pBroker, IBridge, pBridge);
   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
   for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
   {
      CSegmentKey segmentKey(girderKey, segIdx);
      DuctIndexType nSegmentDucts = pSegmentTendonGeometry->GetDuctCount(segmentKey);

      const auto* pSegmentArtifact = pGirderArtifact->GetSegmentArtifact(segIdx);

      for (DuctIndexType ductIdx = 0; ductIdx < nSegmentDucts; ductIdx++)
      {
         const pgsTendonStressArtifact* pTendonArtifact = pSegmentArtifact->GetTendonStressArtifact(ductIdx);

         CString strTitle;
         strTitle.Format(_T("Segment %d, Tendon %d"), LABEL_SEGMENT(segIdx),LABEL_DUCT(ductIdx));
         rptRcTable* p_table = rptStyleManager::CreateDefaultTable(4, strTitle);
         *pPara << p_table;

         p_table->SetColumnStyle(0, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
         p_table->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

         (*p_table)(0, 0) << _T("");
         if (pTendonArtifact->IsAtJackingApplicable())
         {
            Float64 x = pAllowable->GetSegmentTendonAllowableCoefficientAtJacking(segmentKey);
            (*p_table)(1, 0) << _T("At Jacking, ") << RPT_FPJ << _T(" (") << x << RPT_FPU << _T(")");
         }
         else
         {
            Float64 x = pAllowable->GetSegmentTendonAllowableCoefficientPriorToSeating(segmentKey);
            (*p_table)(1, 0) << _T("Prior to seating - short-term ") << RPT_FPBT << _T(" may be allowed. (") << x << RPT_FPY << _T(")");
         }

         Float64 x = pAllowable->GetSegmentTendonAllowableCoefficientAfterAnchorSetAtAnchorage(segmentKey);
         (*p_table)(2, 0) << _T("At anchorages and couplers immediately after anchor set. (") << x << RPT_FPU << _T(")");

         x = pAllowable->GetSegmentTendonAllowableCoefficientAfterAnchorSet(segmentKey);
         (*p_table)(3, 0) << _T("Elsewhere along length of member away from anchorages and couplers immediately after anchor set. (") << x << RPT_FPU << _T(")");

         x = pAllowable->GetSegmentTendonAllowableCoefficientAfterLosses(segmentKey);
         (*p_table)(4, 0) << _T("At service limit state after losses, ") << RPT_FPE << _T(" (") << x << RPT_FPY << _T(")");

         (*p_table)(0, 1) << COLHDR(_T("Allowable") << rptNewLine << _T("Stress"), rptStressUnitTag, pDisplayUnits->GetStressUnit());
         (*p_table)(0, 2) << COLHDR(_T("Tendon") << rptNewLine << _T("Stress"), rptStressUnitTag, pDisplayUnits->GetStressUnit());
         (*p_table)(0, 3) << _T("Status") << rptNewLine << _T("(C/D)");

         Float64 capacity, demand;
         bool bPassed;
         if (pTendonArtifact->IsAtJackingApplicable())
         {
            pTendonArtifact->GetCheckAtJacking(&capacity, &demand, &bPassed);
         }
         else
         {
            pTendonArtifact->GetCheckPriorToSeating(&capacity, &demand, &bPassed);
         }

         (*p_table)(1, 1) << stress.SetValue(capacity);
         (*p_table)(1, 2) << stress.SetValue(demand);
         if (bPassed)
         {
            (*p_table)(1, 3) << RPT_PASS;
         }
         else
         {
            (*p_table)(1, 3) << RPT_FAIL;
         }

         (*p_table)(1, 3) << rptNewLine << _T("(") << cap_demand.SetValue(capacity, demand, bPassed) << _T(")");

         pTendonArtifact->GetCheckAtAnchoragesAfterSeating(&capacity, &demand, &bPassed);
         (*p_table)(2, 1) << stress.SetValue(capacity);
         (*p_table)(2, 2) << stress.SetValue(demand);
         if (bPassed)
         {
            (*p_table)(2, 3) << RPT_PASS;
         }
         else
         {
            (*p_table)(2, 3) << RPT_FAIL;
         }

         (*p_table)(2, 3) << rptNewLine << _T("(") << cap_demand.SetValue(capacity, demand, bPassed) << _T(")");

         pTendonArtifact->GetCheckAfterSeating(&capacity, &demand, &bPassed);
         (*p_table)(3, 1) << stress.SetValue(capacity);
         (*p_table)(3, 2) << stress.SetValue(demand);
         if (bPassed)
         {
            (*p_table)(3, 3) << RPT_PASS;
         }
         else
         {
            (*p_table)(3, 3) << RPT_FAIL;
         }

         (*p_table)(3, 3) << rptNewLine << _T("(") << cap_demand.SetValue(capacity, demand, bPassed) << _T(")");

         pTendonArtifact->GetCheckAfterLosses(&capacity, &demand, &bPassed);
         (*p_table)(4, 1) << stress.SetValue(capacity);
         (*p_table)(4, 2) << stress.SetValue(demand);
         if (bPassed)
         {
            (*p_table)(4, 3) << RPT_PASS;
         }
         else
         {
            (*p_table)(4, 3) << RPT_FAIL;
         }

         (*p_table)(4, 3) << rptNewLine << _T("(") << cap_demand.SetValue(capacity, demand, bPassed) << _T(")");
      } // next duct
   } // next segment

   for ( DuctIndexType ductIdx = 0; ductIdx < nGirderDucts; ductIdx++ )
   {
      const pgsTendonStressArtifact* pTendonArtifact = pGirderArtifact->GetTendonStressArtifact(ductIdx);

      CString strTitle;
      strTitle.Format(_T("Girder Tendon %d"),LABEL_DUCT(ductIdx));
      rptRcTable* p_table = rptStyleManager::CreateDefaultTable(4,strTitle);
      *pPara << p_table;

      p_table->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
      p_table->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

      (*p_table)(0,0) << _T("");
      if ( pTendonArtifact->IsAtJackingApplicable() )
      {
         Float64 x = pAllowable->GetGirderTendonAllowableCoefficientAtJacking(girderKey);
        (*p_table)(1,0) << _T("At Jacking, ") << RPT_FPJ << _T(" (") << x << RPT_FPU << _T(")");
      }
      else
      {
         Float64 x = pAllowable->GetGirderTendonAllowableCoefficientPriorToSeating(girderKey);
        (*p_table)(1,0) << _T("Prior to seating - short-term ") << RPT_FPBT << _T(" may be allowed. (") << x << RPT_FPY << _T(")");
      }

      Float64 x = pAllowable->GetGirderTendonAllowableCoefficientAfterAnchorSetAtAnchorage(girderKey);
      (*p_table)(2,0) << _T("At anchorages and couplers immediately after anchor set. (") << x << RPT_FPU << _T(")");

      x = pAllowable->GetGirderTendonAllowableCoefficientAfterAnchorSet(girderKey);
      (*p_table)(3,0) << _T("Elsewhere along length of member away from anchorages and couplers immediately after anchor set. (") << x << RPT_FPU << _T(")");

      x = pAllowable->GetGirderTendonAllowableCoefficientAfterLosses(girderKey);
      (*p_table)(4,0) << _T("At service limit state after losses, ") << RPT_FPE << _T(" (") << x << RPT_FPY << _T(")");
   
      (*p_table)(0,1) << COLHDR(_T("Allowable") << rptNewLine << _T("Stress"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*p_table)(0,2) << COLHDR(_T("Tendon") << rptNewLine << _T("Stress"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*p_table)(0,3) << _T("Status") << rptNewLine << _T("(C/D)");

      Float64 capacity,demand;
      bool bPassed;
      if ( pTendonArtifact->IsAtJackingApplicable() )
      {
         pTendonArtifact->GetCheckAtJacking(&capacity,&demand,&bPassed);
      }
      else
      {
         pTendonArtifact->GetCheckPriorToSeating(&capacity,&demand,&bPassed);
      }

      (*p_table)(1,1) << stress.SetValue(capacity);
      (*p_table)(1,2) << stress.SetValue(demand);
	   if ( bPassed )
      {
		   (*p_table)(1,3) << RPT_PASS;
      }
      else
      {
		   (*p_table)(1,3) << RPT_FAIL;
      }

      (*p_table)(1,3) << rptNewLine << _T("(") << cap_demand.SetValue(capacity,demand,bPassed) << _T(")");

      pTendonArtifact->GetCheckAtAnchoragesAfterSeating(&capacity,&demand,&bPassed);
      (*p_table)(2,1) << stress.SetValue(capacity);
      (*p_table)(2,2) << stress.SetValue(demand);
	   if ( bPassed )
      {
		   (*p_table)(2,3) << RPT_PASS;
      }
      else
      {
		   (*p_table)(2,3) << RPT_FAIL;
      }

      (*p_table)(2,3) << rptNewLine << _T("(") << cap_demand.SetValue(capacity,demand,bPassed) << _T(")");

      pTendonArtifact->GetCheckAfterSeating(&capacity,&demand,&bPassed);
      (*p_table)(3,1) << stress.SetValue(capacity);
      (*p_table)(3,2) << stress.SetValue(demand);
	   if ( bPassed )
      {
		   (*p_table)(3,3) << RPT_PASS;
      }
      else
      {
		   (*p_table)(3,3) << RPT_FAIL;
      }

      (*p_table)(3,3) << rptNewLine << _T("(") << cap_demand.SetValue(capacity,demand,bPassed) << _T(")");

      pTendonArtifact->GetCheckAfterLosses(&capacity,&demand,&bPassed);
      (*p_table)(4,1) << stress.SetValue(capacity);
      (*p_table)(4,2) << stress.SetValue(demand);
	   if ( bPassed )
      {
		   (*p_table)(4,3) << RPT_PASS;
      }
      else
      {
		   (*p_table)(4,3) << RPT_FAIL;
      }

      (*p_table)(4,3) << rptNewLine << _T("(") << cap_demand.SetValue(capacity,demand,bPassed) << _T(")");
   } // next duct
}

void CTendonStressCheckTable::MakeCopy(const CTendonStressCheckTable& rOther)
{
   // Add copy code here...
}

void CTendonStressCheckTable::MakeAssignment(const CTendonStressCheckTable& rOther)
{
   MakeCopy( rOther );
}

#if defined _DEBUG
bool CTendonStressCheckTable::AssertValid() const
{
   return true;
}

void CTendonStressCheckTable::Dump(dbgDumpContext& os) const
{
   os << _T("Dump for CTendonStressCheckTable") << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool CTendonStressCheckTable::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("CTendonStressCheckTable");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for CTendonStressCheckTable");

   TESTME_EPILOG("CTendonStressCheckTable");
}
#endif // _UNITTEST
