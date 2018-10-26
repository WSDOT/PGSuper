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

// TimeDependentLossFinalTable.cpp : Implementation of CTimeDependentLossFinalTable
#include "stdafx.h"
#include "TimeDependentLossFinalTable.h"
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <PsgLib\SpecLibraryEntry.h>
#include <PgsExt\GirderData.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CTimeDependentLossFinalTable::CTimeDependentLossFinalTable(ColumnIndexType NumColumns, IEAFDisplayUnits* pDisplayUnits) :
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
   DEFINE_UV_PROTOTYPE( time,        pDisplayUnits->GetLongTimeUnit(),        false );

   scalar.SetFormat( sysNumericFormatTool::Automatic );
   scalar.SetWidth(6);
   scalar.SetPrecision(2);
}

CTimeDependentLossFinalTable* CTimeDependentLossFinalTable::PrepareTable(rptChapter* pChapter,IBroker* pBroker,SpanIndexType span,GirderIndexType gdr,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   GET_IFACE2(pBroker,IGirderData,pGirderData);
   CGirderData girderData = pGirderData->GetGirderData(span,gdr);

   GET_IFACE2(pBroker,ISpecification,pSpec);
   std::string strSpecName = pSpec->GetSpecification();

   GET_IFACE2(pBroker,ILibrary,pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( strSpecName.c_str() );

   // Create and configure the table
   ColumnIndexType numColumns = 6;
   CTimeDependentLossFinalTable* table = new CTimeDependentLossFinalTable( numColumns, pDisplayUnits );
   pgsReportStyleHolder::ConfigureTable(table);

   std::string strImagePath(pgsReportStyleHolder::GetImagePath());
   
   rptParagraph* pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;

   *pParagraph << "Time Dependent Losses After Deck Placement" << rptNewLine;

   pParagraph = new rptParagraph;
   *pChapter << pParagraph;

   *pParagraph << table << rptNewLine;
   (*table)(0,0) << COLHDR("Location from"<<rptNewLine<<"Left Support",rptLengthUnitTag,  pDisplayUnits->GetSpanLengthUnit() );
   (*table)(0,1) << COLHDR(symbol(DELTA) << Sub2("f","pSD"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,2) << COLHDR(symbol(DELTA) << Sub2("f","pCD"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,3) << COLHDR(symbol(DELTA) << Sub2("f","pR2"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,4) << COLHDR(symbol(DELTA) << Sub2("f","pSS"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,5) << COLHDR(symbol(DELTA) << "f" << subscript(ON) << "pLT" << subscript(ON) << "df" << subscript(OFF) << subscript(OFF), rptStressUnitTag, pDisplayUnits->GetStressUnit() );

   *pParagraph << symbol(DELTA) << "f" << subscript(ON) << "pLT" << subscript(ON) << "df" << subscript(OFF) << subscript(OFF) << " = " << symbol(DELTA) << Sub2("f","pSD") << " + " << symbol(DELTA) << Sub2("f","pCD") << " + " << symbol(DELTA) << Sub2("f","pR2") << " - " << symbol(DELTA) << Sub2("f","pSS") << rptNewLine;

   return table;
}

void CTimeDependentLossFinalTable::AddRow(rptChapter* pChapter,IBroker* pBroker,RowIndexType row,LOSSDETAILS& details,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   (*this)(row,1) << stress.SetValue(details.RefinedLosses2005.ShrinkageLossAfterDeckPlacement());
   (*this)(row,2) << stress.SetValue(details.RefinedLosses2005.CreepLossAfterDeckPlacement());
   (*this)(row,3) << stress.SetValue(details.RefinedLosses2005.RelaxationLossAfterDeckPlacement());
   (*this)(row,4) << stress.SetValue(details.RefinedLosses2005.DeckShrinkageLoss());
   (*this)(row,5) << stress.SetValue(details.RefinedLosses2005.TimeDependentLossesAfterDeck());
}
