///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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

// RelaxationAtDeckPlacementTable.cpp : Implementation of CRelaxationAtDeckPlacementTable
#include "stdafx.h"
#include "RelaxationAtDeckPlacementTable.h"
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <PsgLib\SpecLibraryEntry.h>
#include <PgsExt\GirderData.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CRelaxationAtDeckPlacementTable::CRelaxationAtDeckPlacementTable(ColumnIndexType NumColumns, IEAFDisplayUnits* pDisplayUnits) :
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

CRelaxationAtDeckPlacementTable* CRelaxationAtDeckPlacementTable::PrepareTable(rptChapter* pChapter,IBroker* pBroker,SpanIndexType span,GirderIndexType gdr,LOSSDETAILS& details,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   GET_IFACE2(pBroker,ISpecification,pSpec);
   std::_tstring strSpecName = pSpec->GetSpecification();

   GET_IFACE2(pBroker,ILibrary,pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( strSpecName.c_str() );

   // Create and configure the table
   ColumnIndexType numColumns = 2;
   if ( details.RefinedLosses2005.GetRelaxationLossMethod() == lrfdRefinedLosses2005::Simplified )
      numColumns++;
   else if (details.RefinedLosses2005.GetRelaxationLossMethod() == lrfdRefinedLosses2005::Refined )
      numColumns += 4;

   CRelaxationAtDeckPlacementTable* table = new CRelaxationAtDeckPlacementTable( numColumns, pDisplayUnits );
   pgsReportStyleHolder::ConfigureTable(table);

   std::_tstring strImagePath(pgsReportStyleHolder::GetImagePath());
   
   rptParagraph* pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;
   *pParagraph << _T("[5.9.5.4.2c] Relaxation of Prestressing Strands : ") << symbol(DELTA) << RPT_STRESS(_T("pR1")) << rptNewLine;

   pParagraph = new rptParagraph;
   *pChapter << pParagraph;
   table->stress.ShowUnitTag(true);
   table->time.ShowUnitTag(true);

   switch(details.RefinedLosses2005.GetRelaxationLossMethod() )
   {
   case lrfdRefinedLosses2005::Simplified:
      *pParagraph << rptRcImage(strImagePath + _T("Delta_FpR1_Simplified.png")) << rptNewLine;
      *pParagraph << RPT_FY << _T(" = ") << table->stress.SetValue(details.RefinedLosses2005.GetFpy())              << rptNewLine;
      *pParagraph << Sub2(_T("K"),_T("L")) << _T(" = ") << details.RefinedLosses2005.GetKL()                        << rptNewLine;
      break;

   case lrfdRefinedLosses2005::Refined:
      *pParagraph << rptRcImage(strImagePath + _T("Delta_FpR1.png")) << rptNewLine;
      *pParagraph << RPT_FY << _T(" = ") << table->stress.SetValue(details.RefinedLosses2005.GetFpy())                              << rptNewLine;
      *pParagraph << Sub2(_T("K'"),_T("L")) << _T(" = ") << details.RefinedLosses2005.GetKL()                                       << rptNewLine;
      *pParagraph << Sub2(_T("t"),_T("i"))  << _T(" = ") << table->time.SetValue(details.RefinedLosses2005.GetInitialAge())         << rptNewLine;
      *pParagraph << Sub2(_T("t"),_T("d"))  << _T(" = ") << table->time.SetValue(details.RefinedLosses2005.GetAgeAtDeckPlacement()) << rptNewLine;
      break;

   case lrfdRefinedLosses2005::LumpSum:
      break;

   default:
      ATLASSERT(false); // should never get here
      break;
   }


   table->stress.ShowUnitTag(false);
   table->time.ShowUnitTag(false);

   ColumnIndexType col = 0;
   *pParagraph << table << rptNewLine;
   (*table)(0,col++) << COLHDR(_T("Location from")<<rptNewLine<<_T("Left Support"),rptLengthUnitTag,  pDisplayUnits->GetSpanLengthUnit() );

   if (details.RefinedLosses2005.GetRelaxationLossMethod() == lrfdRefinedLosses2005::Simplified )
   {
      (*table)(0,col++) << COLHDR(RPT_STRESS(_T("pt")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   }
   else if (details.RefinedLosses2005.GetRelaxationLossMethod() == lrfdRefinedLosses2005::Refined )
   {
      (*table)(0,col++) << COLHDR(RPT_STRESS(_T("pt")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*table)(0,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pSR")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*table)(0,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pCR")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*table)(0,col++) << Sub2(_T("K"),_T("id"));
   }

   (*table)(0,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pR1")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );

   return table;
}

void CRelaxationAtDeckPlacementTable::AddRow(rptChapter* pChapter,IBroker* pBroker,const pgsPointOfInterest& poi,RowIndexType row,LOSSDETAILS& details,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   ColumnIndexType col = 1;
   if (details.RefinedLosses2005.GetRelaxationLossMethod() == lrfdRefinedLosses2005::Simplified )
   {
      (*this)(row,col++) << stress.SetValue(details.RefinedLosses2005.GetPermanentStrandFpt());
   }
   else if (details.RefinedLosses2005.GetRelaxationLossMethod() == lrfdRefinedLosses2005::Refined )
   {
      (*this)(row,col++) << stress.SetValue(details.RefinedLosses2005.GetPermanentStrandFpt());
      (*this)(row,col++) << stress.SetValue(details.RefinedLosses2005.ShrinkageLossBeforeDeckPlacement());
      (*this)(row,col++) << stress.SetValue(details.RefinedLosses2005.CreepLossBeforeDeckPlacement());
      (*this)(row,col++) << scalar.SetValue(details.RefinedLosses2005.GetKid());
   }
   (*this)(row,col++) << stress.SetValue(details.RefinedLosses2005.RelaxationLossBeforeDeckPlacement());
}
