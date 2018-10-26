///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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

// TxDOT2013ChangeOfConcreteStressTable.cpp : Implementation of CTxDOT2013ChangeOfConcreteStressTable
#include "stdafx.h"
#include "TxDOT2013ChangeOfConcreteStressTable.h"
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <PsgLib\SpecLibraryEntry.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CTxDOT2013ChangeOfConcreteStressTable::CTxDOT2013ChangeOfConcreteStressTable(ColumnIndexType NumColumns, IEAFDisplayUnits* pDisplayUnits) :
rptRcTable(NumColumns,0)
{
   DEFINE_UV_PROTOTYPE( spanloc,     pDisplayUnits->GetSpanLengthUnit(),      false );
   DEFINE_UV_PROTOTYPE( gdrloc,      pDisplayUnits->GetSpanLengthUnit(),      false );
   DEFINE_UV_PROTOTYPE( mom_inertia, pDisplayUnits->GetMomentOfInertiaUnit(), false );
   DEFINE_UV_PROTOTYPE( dim,         pDisplayUnits->GetComponentDimUnit(),    false );
   DEFINE_UV_PROTOTYPE( moment,      pDisplayUnits->GetMomentUnit(),          false );
   DEFINE_UV_PROTOTYPE( stress,      pDisplayUnits->GetStressUnit(),          false );
}

CTxDOT2013ChangeOfConcreteStressTable* CTxDOT2013ChangeOfConcreteStressTable::PrepareTable(rptChapter* pChapter,IBroker* pBroker,const CSegmentKey& segmentKey,const LOSSDETAILS* pDetails,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   // If the 0.7fpu method is used for elastic shortening, we only need to compute mid-span values so no table is needed.
   lrfdElasticShortening::FcgpComputationMethod fcgpMethod = pDetails->pLosses->ElasticShortening().GetFcgpComputationMethod();

   std::_tstring strImagePath(rptStyleManager::GetImagePath());

   rptParagraph* pParagraph = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pParagraph;
   *pParagraph << _T("Change in concrete stress at centroid of prestressing strands due to deck weight and superimposed loads [TxDOT Research Report 0-6374-2]") << rptNewLine;

   pParagraph = new rptParagraph;
   *pChapter << pParagraph;

   if (lrfdElasticShortening::fcgp07Fpu==fcgpMethod)
   {
      *pParagraph << rptRcImage(strImagePath + _T("TxDOT_Delta_Fcd_07.png")) << rptNewLine;
      *pParagraph << _T("Note: Elastic values are considered constant along girder length. All parameters taken at mid-span of girder.") << rptNewLine << rptNewLine;

       boost::shared_ptr<const lrfdRefinedLossesTxDOT2013> ptl = boost::dynamic_pointer_cast<const lrfdRefinedLossesTxDOT2013>(pDetails->pLosses);
      if (!ptl)
      {
         ATLASSERT(false); // made a bad cast? Bail...
         return NULL;
      }

      INIT_UV_PROTOTYPE( rptMomentUnitValue, moment,       pDisplayUnits->GetMomentUnit(),          true );
      INIT_UV_PROTOTYPE( rptStressUnitValue, stress,       pDisplayUnits->GetStressUnit(),          true );
      INIT_UV_PROTOTYPE( rptLength4UnitValue,mom_inertia,  pDisplayUnits->GetMomentOfInertiaUnit(), true );
      INIT_UV_PROTOTYPE( rptLengthUnitValue,  ecc,         pDisplayUnits->GetComponentDimUnit(),    true );

      *pParagraph << Sub2(_T("M"),_T("sd")) << _T(" = ") << moment.SetValue( ptl->GetSdMoment())<< _T(" = Moment at mid-girder due to deck weight and other superimposed dead loads")  << rptNewLine;
      *pParagraph << Sub2(_T("e"),_T("m")) << _T(" = ") <<ecc.SetValue( pDetails->pLosses->GetEccPermanentFinal()) << rptNewLine;
      *pParagraph << Sub2(_T("I"),_T("g")) << _T(" = ") << mom_inertia.SetValue(pDetails->pLosses->GetIg()) << rptNewLine << rptNewLine;
      *pParagraph << Sub2(_T("f"),_T("cdp")) << _T(" = ") << stress.SetValue( pDetails->pLosses->GetDeltaFcd1() ) << rptNewLine << rptNewLine;

      return NULL; // no table needed for 0.7fpu case
   }
   else if (lrfdElasticShortening::fcgpIterative==fcgpMethod)
   {
      *pParagraph << rptRcImage(strImagePath + _T("TxDOT_Delta_Fcd.png")) << rptNewLine;

      // Create and configure the table
      ColumnIndexType numColumns = 5;

      CTxDOT2013ChangeOfConcreteStressTable* table = new CTxDOT2013ChangeOfConcreteStressTable( numColumns, pDisplayUnits );
      rptStyleManager::ConfigureTable(table);

      *pParagraph << table << rptNewLine;

      (*table)(0,0) << COLHDR(_T("Location from")<<rptNewLine<<_T("Left Support"),rptLengthUnitTag,  pDisplayUnits->GetSpanLengthUnit() );
      (*table)(0,1) << COLHDR(Sub2(_T("M"),_T("sd")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
      (*table)(0,2) << COLHDR(Sub2(_T("e"),_T("p")),   rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
      (*table)(0,3) << COLHDR(Sub2(_T("I"),_T("g")),    rptLength4UnitTag,pDisplayUnits->GetMomentOfInertiaUnit() );
      (*table)(0,4) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("cdp")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );

      pParagraph = new rptParagraph(rptStyleManager::GetFootnoteStyle());
      *pChapter << pParagraph;
      *pParagraph << Sub2(_T("M"),_T("sd")) << _T(" = Moments due to deck weight and other superimposed dead loads") << rptNewLine;

      return table;
   }
   else
   {
      ATLASSERT(false);
      return NULL;
   }
}

void CTxDOT2013ChangeOfConcreteStressTable::AddRow(rptChapter* pChapter,IBroker* pBroker,const pgsPointOfInterest& poi,RowIndexType row,const LOSSDETAILS* pDetails,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
    boost::shared_ptr<const lrfdRefinedLossesTxDOT2013> ptl = boost::dynamic_pointer_cast<const lrfdRefinedLossesTxDOT2013>(pDetails->pLosses);
   if (!ptl)
   {
      ATLASSERT(false); // made a bad cast? Bail...
      return;
   }

   (*this)(row,1) << moment.SetValue( ptl->GetSdMoment() );
   (*this)(row,2) << dim.SetValue( pDetails->pLosses->GetEccPermanentFinal() );
   (*this)(row,3) << mom_inertia.SetValue( pDetails->pLosses->GetIg() );
   (*this)(row,4) << stress.SetValue( pDetails->pLosses->GetDeltaFcd1() );
}
