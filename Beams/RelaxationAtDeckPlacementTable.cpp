///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright (C) 2002  Washington State Department of Transportation
//                     Bridge and Structures Office
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

CRelaxationAtDeckPlacementTable::CRelaxationAtDeckPlacementTable(ColumnIndexType NumColumns, IDisplayUnits* pDispUnit) :
rptRcTable(NumColumns,0)
{
   DEFINE_UV_PROTOTYPE( spanloc,     pDispUnit->GetSpanLengthUnit(),      false );
   DEFINE_UV_PROTOTYPE( gdrloc,      pDispUnit->GetSpanLengthUnit(),      false );
   DEFINE_UV_PROTOTYPE( offset,      pDispUnit->GetSpanLengthUnit(),      false );
   DEFINE_UV_PROTOTYPE( mod_e,       pDispUnit->GetModEUnit(),            false );
   DEFINE_UV_PROTOTYPE( force,       pDispUnit->GetGeneralForceUnit(),    false );
   DEFINE_UV_PROTOTYPE( area,        pDispUnit->GetAreaUnit(),            false );
   DEFINE_UV_PROTOTYPE( mom_inertia, pDispUnit->GetMomentOfInertiaUnit(), false );
   DEFINE_UV_PROTOTYPE( ecc,         pDispUnit->GetComponentDimUnit(),    false );
   DEFINE_UV_PROTOTYPE( moment,      pDispUnit->GetMomentUnit(),          false );
   DEFINE_UV_PROTOTYPE( stress,      pDispUnit->GetStressUnit(),          false );
   DEFINE_UV_PROTOTYPE( time,        pDispUnit->GetLongTimeUnit(),        false );

   scalar.SetFormat( sysNumericFormatTool::Automatic );
   scalar.SetWidth(6);
   scalar.SetPrecision(2);
}

CRelaxationAtDeckPlacementTable* CRelaxationAtDeckPlacementTable::PrepareTable(rptChapter* pChapter,IBroker* pBroker,SpanIndexType span,GirderIndexType gdr,LOSSDETAILS& details,IDisplayUnits* pDispUnit,Uint16 level)
{
   GET_IFACE2(pBroker,IGirderData,pGirderData);
   CGirderData girderData = pGirderData->GetGirderData(span,gdr);

   GET_IFACE2(pBroker,ISpecification,pSpec);
   std::string strSpecName = pSpec->GetSpecification();

   GET_IFACE2(pBroker,ILibrary,pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( strSpecName.c_str() );

   // Create and configure the table
   ColumnIndexType numColumns = 6;
   CRelaxationAtDeckPlacementTable* table = new CRelaxationAtDeckPlacementTable( numColumns, pDispUnit );
   pgsReportStyleHolder::ConfigureTable(table);

   std::string strImagePath(pgsReportStyleHolder::GetImagePath());
   
   rptParagraph* pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;
   *pParagraph << "[5.9.5.4.2c] Relaxation of Prestressing Strands : " << symbol(DELTA) << Sub2("f","pR1") << rptNewLine;

   pParagraph = new rptParagraph;
   *pChapter << pParagraph;
   *pParagraph << rptRcImage(strImagePath + "Delta_FpR1.gif") << rptNewLine;

   table->stress.ShowUnitTag(true);
   table->time.ShowUnitTag(true);

   *pParagraph << Sub2("f","py") << " = " << table->stress.SetValue(details.RefinedLosses2005.GetFpy())              << rptNewLine;
   *pParagraph << Sub2("K'","L") << " = " << details.RefinedLosses2005.GetKL()                                       << rptNewLine;
   *pParagraph << Sub2("t","i")  << " = " << table->time.SetValue(details.RefinedLosses2005.GetInitialAge())         << rptNewLine;
   *pParagraph << Sub2("t","d")  << " = " << table->time.SetValue(details.RefinedLosses2005.GetAgeAtDeckPlacement()) << rptNewLine;

   table->stress.ShowUnitTag(false);
   table->time.ShowUnitTag(false);

   *pParagraph << table << rptNewLine;
   (*table)(0,0) << COLHDR("Location from"<<rptNewLine<<"Left Support",rptLengthUnitTag,  pDispUnit->GetSpanLengthUnit() );
   (*table)(0,1) << COLHDR(Sub2("f","pt"), rptStressUnitTag, pDispUnit->GetStressUnit() );
   (*table)(0,2) << COLHDR(symbol(DELTA) << Sub2("f","pSR"), rptStressUnitTag, pDispUnit->GetStressUnit() );
   (*table)(0,3) << COLHDR(symbol(DELTA) << Sub2("f","pCR"), rptStressUnitTag, pDispUnit->GetStressUnit() );
   (*table)(0,4) << Sub2("K","id");
   (*table)(0,5) << COLHDR(symbol(DELTA) << Sub2("f","pR1"), rptStressUnitTag, pDispUnit->GetStressUnit() );

   return table;
}

void CRelaxationAtDeckPlacementTable::AddRow(rptChapter* pChapter,IBroker* pBroker,RowIndexType row,LOSSDETAILS& details,IDisplayUnits* pDispUnit,Uint16 level)
{
   (*this)(row,1) << stress.SetValue(details.RefinedLosses2005.GetPermanentStrandFpt());
   (*this)(row,2) << stress.SetValue(details.RefinedLosses2005.ShrinkageLossBeforeDeckPlacement());
   (*this)(row,3) << stress.SetValue(details.RefinedLosses2005.CreepLossBeforeDeckPlacement());
   (*this)(row,4) << scalar.SetValue(details.RefinedLosses2005.GetKid());
   (*this)(row,5) << stress.SetValue(details.RefinedLosses2005.RelaxationLossBeforeDeckPlacement());
}
