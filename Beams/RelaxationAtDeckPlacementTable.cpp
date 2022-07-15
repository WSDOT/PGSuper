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

// RelaxationAtDeckPlacementTable.cpp : Implementation of CRelaxationAtDeckPlacementTable
#include "stdafx.h"
#include "RelaxationAtDeckPlacementTable.h"
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <PsgLib\SpecLibraryEntry.h>
#include <Reporting\ReportNotes.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CRelaxationAtDeckPlacementTable::CRelaxationAtDeckPlacementTable(ColumnIndexType NumColumns, IEAFDisplayUnits* pDisplayUnits) :
rptRcTable(NumColumns,0)
{
   DEFINE_UV_PROTOTYPE( stress,      pDisplayUnits->GetStressUnit(),          false );
   DEFINE_UV_PROTOTYPE( time,        pDisplayUnits->GetWholeDaysUnit(),       false );

   scalar.SetFormat( WBFL::System::NumericFormatTool::Format::Automatic );
   scalar.SetWidth(6);
   scalar.SetPrecision(3);
}

CRelaxationAtDeckPlacementTable* CRelaxationAtDeckPlacementTable::PrepareTable(rptChapter* pChapter,IBroker* pBroker,const CSegmentKey& segmentKey,const LOSSDETAILS* pDetails,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   GET_IFACE2(pBroker,ISpecification,pSpec);
   std::_tstring strSpecName = pSpec->GetSpecification();

   GET_IFACE2(pBroker,ILibrary,pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( strSpecName.c_str() );

  // Typecast to our known type (eating own doggy food)
   std::shared_ptr<const lrfdRefinedLosses2005> ptl = std::dynamic_pointer_cast<const lrfdRefinedLosses2005>(pDetails->pLosses);
   if (!ptl)
   {
      ATLASSERT(false); // made a bad cast? Bail...
      return nullptr;
   }

   // Create and configure the table
   ColumnIndexType numColumns = 2;
   if ( ptl->GetRelaxationLossMethod() == lrfdRefinedLosses2005::Simplified )
   {
      numColumns++;
   }
   else if (ptl->GetRelaxationLossMethod() == lrfdRefinedLosses2005::Refined )
   {
      numColumns += 4;
   }

   CRelaxationAtDeckPlacementTable* table = new CRelaxationAtDeckPlacementTable( numColumns, pDisplayUnits );
   rptStyleManager::ConfigureTable(table);

   std::_tstring strImagePath(rptStyleManager::GetImagePath());
   
   rptParagraph* pParagraph = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pParagraph;
   *pParagraph << _T("[") << LrfdCw8th(_T("5.9.5.4.2c"),_T("5.9.3.4.2c")) << _T("] Relaxation of Prestressing Strands : ") << symbol(DELTA) << RPT_STRESS(_T("pR1")) << rptNewLine;

   pParagraph = new rptParagraph;
   *pChapter << pParagraph;
   table->stress.ShowUnitTag(true);
   table->time.ShowUnitTag(true);

   switch(ptl->GetRelaxationLossMethod() )
   {
   case lrfdRefinedLosses2005::Simplified:
      *pParagraph << rptRcImage(strImagePath + _T("Delta_FpR1_Simplified.png")) << rptNewLine;
      *pParagraph << RPT_FY << _T(" = ") << table->stress.SetValue(ptl->GetFpyPermanent())              << rptNewLine;
      *pParagraph << Sub2(_T("K"),_T("L")) << _T(" = ") << ptl->GetPermanentStrandKL()                        << rptNewLine;
      break;

   case lrfdRefinedLosses2005::Refined:
      *pParagraph << rptRcImage(strImagePath + _T("Delta_FpR1.png")) << rptNewLine;
      *pParagraph << RPT_FY << _T(" = ") << table->stress.SetValue(ptl->GetFpyPermanent())                              << rptNewLine;
      *pParagraph << Sub2(_T("K'"),_T("L")) << _T(" = ") << ptl->GetPermanentStrandKL()                                       << rptNewLine;
      *pParagraph << Sub2(_T("t"),_T("i"))  << _T(" = ") << table->time.SetValue(ptl->GetInitialAge())         << rptNewLine;
      *pParagraph << Sub2(_T("t"),_T("d"))  << _T(" = ") << table->time.SetValue(ptl->GetAgeAtDeckPlacement()) << rptNewLine;
      break;

   case lrfdRefinedLosses2005::LumpSum:
      break;

   default:
      ATLASSERT(false); // should never get here
      break;
   }


   if ( ptl->GetPermanentStrandCoating() != WBFL::Materials::PsStrand::Coating::None )
   {
      *pParagraph << EPOXY_RELAXATION_NOTE << rptNewLine;
   }

   table->stress.ShowUnitTag(false);
   table->time.ShowUnitTag(false);

   ColumnIndexType col = 0;
   *pParagraph << table << rptNewLine;
   (*table)(0,col++) << COLHDR(_T("Location from")<<rptNewLine<<_T("Left Support"),rptLengthUnitTag,  pDisplayUnits->GetSpanLengthUnit() );

   if (ptl->GetRelaxationLossMethod() == lrfdRefinedLosses2005::Simplified )
   {
      (*table)(0,col++) << COLHDR(RPT_STRESS(_T("pt")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   }
   else if (ptl->GetRelaxationLossMethod() == lrfdRefinedLosses2005::Refined )
   {
      (*table)(0,col++) << COLHDR(RPT_STRESS(_T("pt")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*table)(0,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pSR")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*table)(0,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pCR")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*table)(0,col++) << Sub2(_T("K"),_T("id"));
   }

   (*table)(0,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pR1")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );

   return table;
}

void CRelaxationAtDeckPlacementTable::AddRow(rptChapter* pChapter,IBroker* pBroker,const pgsPointOfInterest& poi,RowIndexType row,const LOSSDETAILS* pDetails,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   ColumnIndexType col = 1;
   RowIndexType rowOffset = GetNumberOfHeaderRows() - 1;

  // Typecast to our known type (eating own doggy food)
   std::shared_ptr<const lrfdRefinedLosses2005> ptl = std::dynamic_pointer_cast<const lrfdRefinedLosses2005>(pDetails->pLosses);
   if (!ptl)
   {
      ATLASSERT(false); // made a bad cast? Bail...
      return;
   }

   if (ptl->GetRelaxationLossMethod() == lrfdRefinedLosses2005::Simplified )
   {
      (*this)(row+rowOffset,col++) << stress.SetValue(ptl->GetPermanentStrandFpt());
   }
   else if (ptl->GetRelaxationLossMethod() == lrfdRefinedLosses2005::Refined )
   {
      (*this)(row+rowOffset,col++) << stress.SetValue(ptl->GetPermanentStrandFpt());
      (*this)(row+rowOffset,col++) << stress.SetValue(ptl->ShrinkageLossBeforeDeckPlacement());
      (*this)(row+rowOffset,col++) << stress.SetValue(ptl->CreepLossBeforeDeckPlacement());
      (*this)(row+rowOffset,col++) << scalar.SetValue(ptl->GetKid());
   }
   (*this)(row+rowOffset,col++) << stress.SetValue(ptl->RelaxationLossBeforeDeckPlacement());
}
