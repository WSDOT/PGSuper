///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2011  Washington State Department of Transportation
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

CFrictionLossTable::CFrictionLossTable(ColumnIndexType NumColumns, IEAFDisplayUnits* pDisplayUnits) :
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
   DEFINE_UV_PROTOTYPE( wobble,      pDisplayUnits->GetPerLengthUnit(),       true  );
}

CFrictionLossTable* CFrictionLossTable::PrepareTable(rptChapter* pChapter,IBroker* pBroker,SpanIndexType span,GirderIndexType gdr,LOSSDETAILS& details,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   // Create and configure the table
   ColumnIndexType numColumns = 5;
   CFrictionLossTable* table = new CFrictionLossTable( numColumns, pDisplayUnits );
   pgsReportStyleHolder::ConfigureTable(table);

   std::_tstring strImagePath(pgsReportStyleHolder::GetImagePath());
   
   rptParagraph* pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;
   *pParagraph << _T("Friction and Anchor Set loss in post-tensioned temporary strands [5.9.5.2.1, 5.9.5.2.2b]") << rptNewLine;

   pParagraph = new rptParagraph;
   *pChapter << pParagraph;
   *pParagraph << rptRcImage(strImagePath + _T("FrictionAndAnchorSetLosses.gif")) << rptNewLine << rptNewLine;
   *pParagraph << rptRcImage(strImagePath + _T("Delta_FpF.png")) << rptNewLine << rptNewLine;
   *pParagraph << rptRcImage(strImagePath + _T("Delta_FpA.png")) << rptNewLine;

   *pParagraph << _T("Wobble Friction: K = ") << table->wobble.SetValue(details.pLosses->GetWobbleFrictionCoefficient()) << rptNewLine;

   table->ecc.ShowUnitTag(true);
   *pParagraph << _T("Anchor Set: ") << symbol(DELTA) << _T("L = ") << table->ecc.SetValue(details.pLosses->GetAnchorSet()) << rptNewLine;
   table->ecc.ShowUnitTag(false);

   table->offset.ShowUnitTag(true);
   *pParagraph << Sub2(_T("L"),_T("set")) << _T(" = ") << table->offset.SetValue(details.pLosses->AnchorSetZone()) << rptNewLine;
   *pParagraph << Sub2(_T("L"),_T("g")) << _T(" = ") << table->offset.SetValue(details.pLosses->GetGirderLength() ) << rptNewLine;
   table->offset.ShowUnitTag(false);

   table->stress.ShowUnitTag(true);
   *pParagraph << symbol(DELTA) << RPT_STRESS(_T("pFT")) << _T(" = ") << table->stress.SetValue(details.pLosses->TotalFrictionLoss() ) << rptNewLine;
   *pParagraph << symbol(DELTA) << RPT_STRESS(_T("pAT")) << _T(" = ") << table->stress.SetValue(details.pLosses->TotalAnchorSetLoss() ) << rptNewLine;
   table->stress.ShowUnitTag(false);

   *pParagraph << table << rptNewLine;


   (*table)(0,0) << COLHDR(_T("Location from")<<rptNewLine<<_T("End of Girder"),rptLengthUnitTag,  pDisplayUnits->GetSpanLengthUnit() );
   (*table)(0,1) << COLHDR(_T("Location from")<<rptNewLine<<_T("Left Support"),rptLengthUnitTag,  pDisplayUnits->GetSpanLengthUnit() );
   (*table)(0,2) << COLHDR(_T("x"),rptLengthUnitTag,pDisplayUnits->GetSpanLengthUnit());
   (*table)(0,3) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pF")) << _T(" @ x") , rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,4) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pA")) << _T(" @ x") , rptStressUnitTag, pDisplayUnits->GetStressUnit() );

   return table;
}

void CFrictionLossTable::AddRow(rptChapter* pChapter,IBroker* pBroker,RowIndexType row,LOSSDETAILS& details,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   (*this)(row,2) << offset.SetValue(details.pLosses->GetLocation()   );
   (*this)(row,3) << stress.SetValue(details.pLosses->FrictionLoss()  );
   (*this)(row,4) << stress.SetValue(details.pLosses->AnchorSetLoss() );
}
