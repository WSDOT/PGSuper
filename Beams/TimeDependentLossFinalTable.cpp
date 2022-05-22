///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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
#include <IFace\PrestressForce.h>
#include <PsgLib\SpecLibraryEntry.h>

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
   DEFINE_UV_PROTOTYPE( time,        pDisplayUnits->GetWholeDaysUnit(),        false );

   scalar.SetFormat( WBFL::System::NumericFormatTool::Format::Automatic );
   scalar.SetWidth(6);
   scalar.SetPrecision(2);
}

CTimeDependentLossFinalTable* CTimeDependentLossFinalTable::PrepareTable(rptChapter* pChapter,IBroker* pBroker,const CSegmentKey& segmentKey,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   GET_IFACE2(pBroker,ISpecification,pSpec);
   std::_tstring strSpecName = pSpec->GetSpecification();

   GET_IFACE2(pBroker,ILibrary,pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( strSpecName.c_str() );

   // Create and configure the table
   ColumnIndexType numColumns = 6;

   CTimeDependentLossFinalTable* table = new CTimeDependentLossFinalTable( numColumns, pDisplayUnits );
   rptStyleManager::ConfigureTable(table);

   std::_tstring strImagePath(rptStyleManager::GetImagePath());
   
   rptParagraph* pParagraph = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pParagraph;

   GET_IFACE2(pBroker, IBridge, pBridge);
   CString strTitle;
   if (pBridge->GetDeckType() == pgsTypes::sdtNonstructuralOverlay)
   {
      pParagraph->SetName(_T("Time Dependent Losses After Nonstructural Overlay Installation"));
   }
   else
   {
      pParagraph->SetName(_T("Time Dependent Losses After Deck Placement"));
   }
   *pParagraph << pParagraph->GetName() << rptNewLine;

   pParagraph = new rptParagraph;
   *pChapter << pParagraph;

   *pParagraph << symbol(DELTA) << italic(ON) << _T("f") << subscript(ON) << _T("pLT") << subscript(ON) << _T("df") << subscript(OFF) << subscript(OFF) << italic(OFF) << _T(" = ") << symbol(DELTA) << RPT_STRESS(_T("pSD")) << _T(" + ") << symbol(DELTA) << RPT_STRESS(_T("pCD")) << _T(" + ") << symbol(DELTA) << RPT_STRESS(_T("pR2")) << _T(" - ") << symbol(DELTA) << RPT_STRESS(_T("pSS"));
   *pParagraph << rptNewLine;

   ColumnIndexType colIdx = 0;
   *pParagraph << table << rptNewLine;
   (*table)(0,colIdx++) << COLHDR(_T("Location from")<<rptNewLine<<_T("Left Support"),rptLengthUnitTag,  pDisplayUnits->GetSpanLengthUnit() );
   (*table)(0,colIdx++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pSD")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,colIdx++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pCD")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,colIdx++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pR2")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,colIdx++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pSS")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,colIdx++) << COLHDR(symbol(DELTA) << italic(ON) << _T("f") << subscript(ON) << _T("pLT") << subscript(ON) << _T("df") << subscript(OFF) << subscript(OFF) << italic(OFF), rptStressUnitTag, pDisplayUnits->GetStressUnit() );


   return table;
}

void CTimeDependentLossFinalTable::AddRow(rptChapter* pChapter,IBroker* pBroker,const pgsPointOfInterest& poi,RowIndexType row,const LOSSDETAILS* pDetails,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   // Typecast to our known type (eating own doggy food)
   std::shared_ptr<const lrfdRefinedLosses2005> ptl = std::dynamic_pointer_cast<const lrfdRefinedLosses2005>(pDetails->pLosses);
   if (!ptl)
   {
      ATLASSERT(false); // made a bad cast? Bail...
      return;
   }

   ColumnIndexType colIdx = 1;
   RowIndexType rowOffset = GetNumberOfHeaderRows() - 1;

   (*this)(row+rowOffset,colIdx++) << stress.SetValue(ptl->ShrinkageLossAfterDeckPlacement());
   (*this)(row+rowOffset,colIdx++) << stress.SetValue(ptl->CreepLossAfterDeckPlacement());
   (*this)(row+rowOffset,colIdx++) << stress.SetValue(ptl->RelaxationLossAfterDeckPlacement());
   (*this)(row+rowOffset,colIdx++) << stress.SetValue(ptl->ElasticGainDueToDeckShrinkage());
   (*this)(row+rowOffset,colIdx++) << stress.SetValue(ptl->TimeDependentLossesAfterDeck());
}
