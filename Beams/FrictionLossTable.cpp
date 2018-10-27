///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2018  Washington State Department of Transportation
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
   DEFINE_UV_PROTOTYPE(angle, pDisplayUnits->GetRadAngleUnit(), false);
}

CFrictionLossTable* CFrictionLossTable::PrepareTable(rptChapter* pChapter,IBroker* pBroker,const CSegmentKey& segmentKey,const LOSSDETAILS* pDetails,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   // Create and configure the table
   ColumnIndexType numColumns = 5;
   CFrictionLossTable* table = new CFrictionLossTable( numColumns, pDisplayUnits );
   rptStyleManager::ConfigureTable(table);

   std::_tstring strImagePath(rptStyleManager::GetImagePath());
   
   rptParagraph* pParagraph = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pParagraph;
   pParagraph->SetName(_T("Friction and Anchor Set loss in post-tensioned temporary strands"));
   *pParagraph << pParagraph->GetName() << _T(" [") << LrfdCw8th(_T("5.9.5.2.1, "), _T("5.9.3.2.1, ")) << LrfdCw8th(_T("5.9.5.2.2b"), _T("5.9.3.2.2b")) <<_T("]") << rptNewLine;

   pParagraph = new rptParagraph;
   *pChapter << pParagraph;
   *pParagraph << rptRcImage(strImagePath + _T("FrictionAndAnchorSetLosses.gif")) << rptNewLine << rptNewLine;
   *pParagraph << rptRcImage(strImagePath + _T("Delta_FpF.png")) << rptNewLine << rptNewLine;
   *pParagraph << rptRcImage(strImagePath + _T("Delta_FpA.png")) << rptNewLine;

   *pParagraph << _T("Wobble Friction: K = ") << table->wobble.SetValue(pDetails->pLosses->GetWobbleFrictionCoefficient()) << rptNewLine;

   *pParagraph << _T("Coefficient of Friction: ") << symbol(mu) << _T(" = ") << pDetails->pLosses->GetCoefficientOfFriction() << rptNewLine;

   table->ecc.ShowUnitTag(true);
   *pParagraph << _T("Anchor Set: ") << symbol(DELTA) << _T("L = ") << table->ecc.SetValue(pDetails->pLosses->GetAnchorSet()) << rptNewLine;
   table->ecc.ShowUnitTag(false);

   *pParagraph << _T("Angular Change: ") << symbol(alpha) << _T(" = ") << table->angle.SetValue(pDetails->pLosses->GetTendonAngleChange()) << rptNewLine;

   table->offset.ShowUnitTag(true);
   *pParagraph << Sub2(_T("L"),_T("set")) << _T(" = ") << table->offset.SetValue(pDetails->pLosses->AnchorSetZone()) << rptNewLine;
   *pParagraph << Sub2(_T("L"),_T("g")) << _T(" = ") << table->offset.SetValue(pDetails->pLosses->GetGirderLength() ) << rptNewLine;
   table->offset.ShowUnitTag(false);

   table->stress.ShowUnitTag(true);
   *pParagraph << symbol(DELTA) << RPT_STRESS(_T("pFT")) << _T(" = ") << table->stress.SetValue(pDetails->pLosses->TotalFrictionLoss() ) << rptNewLine;
   *pParagraph << symbol(DELTA) << RPT_STRESS(_T("pAT")) << _T(" = ") << table->stress.SetValue(pDetails->pLosses->TotalAnchorSetLoss() ) << rptNewLine;
   table->stress.ShowUnitTag(false);

   *pParagraph << table << rptNewLine;

   ColumnIndexType col = 0;
   (*table)(0,col++) << COLHDR(_T("Location from")<<rptNewLine<<_T("End of Girder"),rptLengthUnitTag,  pDisplayUnits->GetSpanLengthUnit() );
   (*table)(0,col++) << COLHDR(_T("Location from")<<rptNewLine<<_T("Left Support"),rptLengthUnitTag,  pDisplayUnits->GetSpanLengthUnit() );
   (*table)(0,col++) << COLHDR(_T("x"),rptLengthUnitTag,pDisplayUnits->GetSpanLengthUnit());
   (*table)(0,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pF")) << _T(" @ x") , rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pA")) << _T(" @ x") , rptStressUnitTag, pDisplayUnits->GetStressUnit() );

   return table;
}

void CFrictionLossTable::AddRow(rptChapter* pChapter,IBroker* pBroker,const pgsPointOfInterest& poi,RowIndexType row,const LOSSDETAILS* pDetails,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   ColumnIndexType col = 2;
   RowIndexType rowOffset = GetNumberOfHeaderRows() - 1;
   (*this)(row+rowOffset,col++) << offset.SetValue(pDetails->pLosses->GetLocation()   );
   (*this)(row+rowOffset,col++) << stress.SetValue(pDetails->pLosses->FrictionLoss()  );
   (*this)(row+rowOffset,col++) << stress.SetValue(pDetails->pLosses->AnchorSetLoss() );
}
