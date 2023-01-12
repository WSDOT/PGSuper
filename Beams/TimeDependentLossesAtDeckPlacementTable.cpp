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

// TimeDependentLossesAtDeckPlacementTable.cpp : Implementation of CTimeDependentLossesAtDeckPlacementTable
#include "stdafx.h"
#include "TimeDependentLossesAtDeckPlacementTable.h"
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <PsgLib\SpecLibraryEntry.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CTimeDependentLossesAtDeckPlacementTable::CTimeDependentLossesAtDeckPlacementTable(ColumnIndexType NumColumns, IEAFDisplayUnits* pDisplayUnits) :
rptRcTable(NumColumns,0)
{
   DEFINE_UV_PROTOTYPE( spanloc,     pDisplayUnits->GetSpanLengthUnit(),      false );
   DEFINE_UV_PROTOTYPE( gdrloc,      pDisplayUnits->GetSpanLengthUnit(),      false );
   DEFINE_UV_PROTOTYPE( offset,      pDisplayUnits->GetSpanLengthUnit(),      false );
   DEFINE_UV_PROTOTYPE( mod_e,       pDisplayUnits->GetModEUnit(),            false );
   DEFINE_UV_PROTOTYPE( force,       pDisplayUnits->GetGeneralForceUnit(),    false );
   DEFINE_UV_PROTOTYPE( area,        pDisplayUnits->GetAreaUnit(),            false );
   DEFINE_UV_PROTOTYPE( mom_inertia, pDisplayUnits->GetMomentOfInertiaUnit(), false );
   DEFINE_UV_PROTOTYPE( ecc,         pDisplayUnits->GetComponentDimUnit(),    false );
   DEFINE_UV_PROTOTYPE( moment,      pDisplayUnits->GetMomentUnit(),          false );
   DEFINE_UV_PROTOTYPE( stress,      pDisplayUnits->GetStressUnit(),          false );
}

CTimeDependentLossesAtDeckPlacementTable* CTimeDependentLossesAtDeckPlacementTable::PrepareTable(rptChapter* pChapter,IBroker* pBroker,const CSegmentKey& segmentKey,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   std::_tstring strImagePath(rptStyleManager::GetImagePath());

   GET_IFACE2(pBroker,IBridge,pBridge);


   // Create and configure the table
   ColumnIndexType numColumns = 5;

   CTimeDependentLossesAtDeckPlacementTable* table = new CTimeDependentLossesAtDeckPlacementTable( numColumns, pDisplayUnits );
   rptStyleManager::ConfigureTable(table);

   rptParagraph* pParagraph = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pParagraph;
   if ( pBridge->IsCompositeDeck() )
       *pParagraph << _T("Time Dependent Losses Before Deck Placement") << rptNewLine;
   else
       *pParagraph << _T("Time Dependent Losses Before Installation of Precast Members") << rptNewLine;

   pParagraph = new rptParagraph;
   *pChapter << pParagraph;

   *pParagraph << symbol(DELTA) << italic(ON) << _T("f") << subscript(ON) << _T("pLT") << subscript(ON) << _T("id") << subscript(OFF) << subscript(OFF) << italic(OFF) << _T(" = ") << symbol(DELTA) << RPT_STRESS(_T("pSR")) << _T(" + ") << symbol(DELTA) << RPT_STRESS(_T("pCR")) << _T(" + ") << symbol(DELTA) << RPT_STRESS(_T("pR1")) << rptNewLine;

   *pParagraph << table << rptNewLine;
   (*table)(0,0) << COLHDR(_T("Location from")<<rptNewLine<<_T("Left Support"),rptLengthUnitTag,  pDisplayUnits->GetSpanLengthUnit() );
   (*table)(0,1) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pSR")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,2) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pCR")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,3) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pR1")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,4) << COLHDR(symbol(DELTA) << italic(ON) << _T("f") << subscript(ON) << _T("pLT") << subscript(ON) << _T("id") << subscript(OFF) << subscript(OFF) << italic(OFF), rptStressUnitTag, pDisplayUnits->GetStressUnit() );

   return table;
}

void CTimeDependentLossesAtDeckPlacementTable::AddRow(rptChapter* pChapter,IBroker* pBroker,const pgsPointOfInterest& poi,RowIndexType row,const LOSSDETAILS* pDetails,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   // Typecast to our known type (eating own doggy food)
   std::shared_ptr<const lrfdRefinedLosses2005> ptl = std::dynamic_pointer_cast<const lrfdRefinedLosses2005>(pDetails->pLosses);
   if (!ptl)
   {
      ATLASSERT(false); // made a bad cast? Bail...
      return;
   }

   RowIndexType rowOffset = GetNumberOfHeaderRows() - 1;

   (*this)(row+rowOffset,1) << stress.SetValue(ptl->ShrinkageLossBeforeDeckPlacement());
   (*this)(row+rowOffset,2) << stress.SetValue(ptl->CreepLossBeforeDeckPlacement());
   (*this)(row+rowOffset,3) << stress.SetValue(ptl->RelaxationLossBeforeDeckPlacement());
   (*this)(row+rowOffset,4) << stress.SetValue(ptl->TimeDependentLossesBeforeDeck());
}
