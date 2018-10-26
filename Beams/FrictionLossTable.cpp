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

// FrictionLossTable.cpp : Implementation of CFrictionLossTable
#include "stdafx.h"
#include "FrictionLossTable.h"
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <PsgLib\SpecLibraryEntry.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CFrictionLossTable::CFrictionLossTable(ColumnIndexType NumColumns, IDisplayUnits* pDispUnit) :
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
   DEFINE_UV_PROTOTYPE( wobble,      pDispUnit->GetPerLengthUnit(),       true  );
}

CFrictionLossTable* CFrictionLossTable::PrepareTable(rptChapter* pChapter,IBroker* pBroker,SpanIndexType span,GirderIndexType gdr,LOSSDETAILS& details,IDisplayUnits* pDispUnit,Uint16 level)
{
   // Create and configure the table
   ColumnIndexType numColumns = 5;
   CFrictionLossTable* table = new CFrictionLossTable( numColumns, pDispUnit );
   pgsReportStyleHolder::ConfigureTable(table);

   std::string strImagePath(pgsReportStyleHolder::GetImagePath());
   
   rptParagraph* pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;
   *pParagraph << "Friction and Anchor Set loss in post-tensioned temporary strands [5.9.5.2.1, 5.9.5.2.2b]" << rptNewLine;

   pParagraph = new rptParagraph;
   *pChapter << pParagraph;
   *pParagraph << rptRcImage(strImagePath + "FrictionAndAnchorSetLosses.gif") << rptNewLine << rptNewLine;
   *pParagraph << rptRcImage(strImagePath + "Delta_FpF.gif") << rptNewLine << rptNewLine;
   *pParagraph << rptRcImage(strImagePath + "Delta_FpA.gif") << rptNewLine;

   *pParagraph << "Wobble Friction: K = " << table->wobble.SetValue(details.pLosses->GetWobbleFrictionCoefficient()) << rptNewLine;

   table->ecc.ShowUnitTag(true);
   *pParagraph << "Anchor Set: " << symbol(DELTA) << "L = " << table->ecc.SetValue(details.pLosses->GetAnchorSet()) << rptNewLine;
   table->ecc.ShowUnitTag(false);

   table->offset.ShowUnitTag(true);
   *pParagraph << Sub2("L","set") << " = " << table->offset.SetValue(details.pLosses->AnchorSetZone()) << rptNewLine;
   *pParagraph << Sub2("L","g") << " = " << table->offset.SetValue(details.pLosses->GetGirderLength() ) << rptNewLine;
   table->offset.ShowUnitTag(false);

   table->stress.ShowUnitTag(true);
   *pParagraph << symbol(DELTA) << Sub2("f","pFT") << " = " << table->stress.SetValue(details.pLosses->TotalFrictionLoss() ) << rptNewLine;
   *pParagraph << symbol(DELTA) << Sub2("f","pAT") << " = " << table->stress.SetValue(details.pLosses->TotalAnchorSetLoss() ) << rptNewLine;
   table->stress.ShowUnitTag(false);

   *pParagraph << table << rptNewLine;


   (*table)(0,0) << COLHDR("Location from"<<rptNewLine<<"End of Girder",rptLengthUnitTag,  pDispUnit->GetSpanLengthUnit() );
   (*table)(0,1) << COLHDR("Location from"<<rptNewLine<<"Left Support",rptLengthUnitTag,  pDispUnit->GetSpanLengthUnit() );
   (*table)(0,2) << COLHDR("x",rptLengthUnitTag,pDispUnit->GetSpanLengthUnit());
   (*table)(0,3) << COLHDR(symbol(DELTA) << Sub2("f","pF") << " @ x" , rptStressUnitTag, pDispUnit->GetStressUnit() );
   (*table)(0,4) << COLHDR(symbol(DELTA) << Sub2("f","pA") << " @ x" , rptStressUnitTag, pDispUnit->GetStressUnit() );

   return table;
}

void CFrictionLossTable::AddRow(rptChapter* pChapter,IBroker* pBroker,RowIndexType row,LOSSDETAILS& details,IDisplayUnits* pDispUnit,Uint16 level)
{
   (*this)(row,2) << offset.SetValue(details.pLosses->GetLocation()   );
   (*this)(row,3) << stress.SetValue(details.pLosses->FrictionLoss()  );
   (*this)(row,4) << stress.SetValue(details.pLosses->AnchorSetLoss() );
}
