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

// TimeDependentLossesAtDeckPlacementTable.cpp : Implementation of CTimeDependentLossesAtDeckPlacementTable
#include "stdafx.h"
#include "TimeDependentLossesAtDeckPlacementTable.h"
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <PsgLib\SpecLibraryEntry.h>
#include <PgsExt\GirderData.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CTimeDependentLossesAtDeckPlacementTable::CTimeDependentLossesAtDeckPlacementTable(ColumnIndexType NumColumns, IDisplayUnits* pDisplayUnits) :
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

CTimeDependentLossesAtDeckPlacementTable* CTimeDependentLossesAtDeckPlacementTable::PrepareTable(rptChapter* pChapter,IBroker* pBroker,SpanIndexType span,GirderIndexType gdr,IDisplayUnits* pDisplayUnits,Uint16 level)
{
   std::string strImagePath(pgsReportStyleHolder::GetImagePath());

   GET_IFACE2(pBroker,IBridge,pBridge);


   // Create and configure the table
   ColumnIndexType numColumns = 5;

   CTimeDependentLossesAtDeckPlacementTable* table = new CTimeDependentLossesAtDeckPlacementTable( numColumns, pDisplayUnits );
   pgsReportStyleHolder::ConfigureTable(table);

   rptParagraph* pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;
   if ( pBridge->IsCompositeDeck() )
       *pParagraph << "Time Dependent Losses Before Deck Placement" << rptNewLine;
   else
       *pParagraph << "Time Dependent Losses Before Installation of Precast Members" << rptNewLine;

   pParagraph = new rptParagraph;
   *pChapter << pParagraph;

   *pParagraph << table << rptNewLine;
   (*table)(0,0) << COLHDR("Location from"<<rptNewLine<<"Left Support",rptLengthUnitTag,  pDisplayUnits->GetSpanLengthUnit() );
   (*table)(0,1) << COLHDR(symbol(DELTA) << Sub2("f","pSR"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,2) << COLHDR(symbol(DELTA) << Sub2("f","pCR"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,3) << COLHDR(symbol(DELTA) << Sub2("f","pR1"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,4) << COLHDR(symbol(DELTA) << "f" << subscript(ON) << "pLT" << subscript(ON) << "id" << subscript(OFF) << subscript(OFF), rptStressUnitTag, pDisplayUnits->GetStressUnit() );

   *pParagraph << symbol(DELTA) << "f" << subscript(ON) << "pLT" << subscript(ON) << "id" << subscript(OFF) << subscript(OFF) << " = " << symbol(DELTA) << Sub2("f","pSR") << " + " << symbol(DELTA) << Sub2("f","pCR") << " + " << symbol(DELTA) << Sub2("f","pR1") << rptNewLine;

   return table;
}

void CTimeDependentLossesAtDeckPlacementTable::AddRow(rptChapter* pChapter,IBroker* pBroker,RowIndexType row,LOSSDETAILS& details,IDisplayUnits* pDisplayUnits,Uint16 level)
{
   (*this)(row,1) << stress.SetValue(details.RefinedLosses2005.ShrinkageLossBeforeDeckPlacement());
   (*this)(row,2) << stress.SetValue(details.RefinedLosses2005.CreepLossBeforeDeckPlacement());
   (*this)(row,3) << stress.SetValue(details.RefinedLosses2005.RelaxationLossBeforeDeckPlacement());
   (*this)(row,4) << stress.SetValue(details.RefinedLosses2005.TimeDependentLossesBeforeDeck());
}
