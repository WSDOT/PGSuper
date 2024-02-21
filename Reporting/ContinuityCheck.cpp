///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2024  Washington State Department of Transportation
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
#include <Reporting\ContinuityCheck.h>

#include <IFace\AnalysisResults.h>
#include <IFace\Bridge.h>
#include <IFace\Project.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CContinuityCheck
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CContinuityCheck::CContinuityCheck()
{
}

CContinuityCheck::CContinuityCheck(const CContinuityCheck& rOther)
{
   MakeCopy(rOther);
}

CContinuityCheck::~CContinuityCheck()
{
}

//======================== OPERATORS  =======================================
CContinuityCheck& CContinuityCheck::operator= (const CContinuityCheck& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
void CContinuityCheck::Build(rptChapter* pChapter,
                              IBroker* pBroker,const CGirderKey& girderKey,
                              IEAFDisplayUnits* pDisplayUnits) const
{
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,ISpecification,pSpec);

   SpanIndexType nSpans = pBridge->GetSpanCount();

   // if there is only one span or if this is simple span analysis, get the heck outta here
   pgsTypes::AnalysisType analysis_type = pSpec->GetAnalysisType();
   if ( nSpans == 1 || analysis_type == pgsTypes::Simple )
   {
      return;
   }

   INIT_UV_PROTOTYPE( rptPressureSectionValue, stress, pDisplayUnits->GetStressUnit(), false );

   rptParagraph* pTitle = new rptParagraph( rptStyleManager::GetHeadingStyle() );
   *pChapter << pTitle;
   *pTitle << _T("Continuity [") << WBFL::LRFD::LrfdCw8th(_T("5.14.1.4.5"),_T("5.12.3.3.5")) << _T("]");

   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(4);
   pTable->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   pTable->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   *pPara << pTable;

   (*pTable)(0,0) << _T("");
   (*pTable)(0,1) << COLHDR(RPT_FBOT, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*pTable)(0,2) << _T("Boundary Condition");
   (*pTable)(0,3) << _T("Is Compressive?");

   GET_IFACE2(pBroker,IContinuity,pContinuity);

   PierIndexType nPiers = pBridge->GetPierCount();
   RowIndexType row = 1;
   for ( PierIndexType pierIdx = 0; pierIdx < nPiers; pierIdx++, row++ )
   {
      ColumnIndexType col = 0;
      bool isAbut = (pierIdx == 0 || pierIdx == nPiers - 1);
      (*pTable)(row,col++) << LABEL_PIER_EX(isAbut,pierIdx);

      if ( pBridge->IsInteriorPier(pierIdx) )
      {
         (*pTable)(row,col++) << _T("");
         (*pTable)(row,col++) << _T("");
         (*pTable)(row,col++) << _T("");
      }
      else
      {
         Float64 fBottom = pContinuity->GetContinuityStressLevel(pierIdx,girderKey);
         (*pTable)(row,col++) << stress.SetValue(fBottom);

         bool bContinuousLeft, bContinuousRight;
         pBridge->IsContinuousAtPier(pierIdx,&bContinuousLeft,&bContinuousRight);
         
         bool bIntegralLeft, bIntegralRight;
         pBridge->IsIntegralAtPier(pierIdx,&bIntegralLeft,&bIntegralRight);

         if ( bContinuousLeft || bContinuousRight )
         {
            (*pTable)(row,col++) << _T("Continuous");
         }
         else if ( bIntegralLeft || bIntegralRight )
         {
            (*pTable)(row,col++) << _T("Integral");
         }
         else
         {
            (*pTable)(row,col++) << _T("Hinged");
         }

         fBottom = IsZero(fBottom) ? 0 : fBottom;
         (*pTable)(row,col++) << (fBottom < 0 ? _T("Yes") : _T("No"));
      }
   }

   pPara = new rptParagraph;
   *pPara << RPT_FBOT << _T(" is the calcuated stress at the bottom of the continuity diaphragm for the combination of superimposed permanent loads and 50% live load") << rptNewLine;
   *pChapter << pPara;

   bool bEffective = pContinuity->IsContinuityFullyEffective(girderKey);
   if ( bEffective )
   {
      *pPara << _T("Continuous connections are fully effective.") << rptNewLine;
      *pPara << _T("Continuity is accounted for in Service and Strength Limit States.") << rptNewLine;
   }
   else
   {
      *pPara << _T("Continuous connections are not fully effective.") << rptNewLine;
      *pPara << _T("Continuity is accounted for only in Strength Limit States.") << rptNewLine;
   }
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void CContinuityCheck::MakeCopy(const CContinuityCheck& rOther)
{
   // Add copy code here...
}

void CContinuityCheck::MakeAssignment(const CContinuityCheck& rOther)
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
