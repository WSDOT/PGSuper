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

// ElasticGainDueToDeckPlacementTable.cpp : Implementation of CElasticGainDueToDeckPlacementTable
#include "stdafx.h"
#include "ElasticGainDueToDeckPlacementTable.h"
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <PsgLib\SpecLibraryEntry.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CElasticGainDueToDeckPlacementTable::CElasticGainDueToDeckPlacementTable(ColumnIndexType NumColumns, IEAFDisplayUnits* pDisplayUnits) :
rptRcTable(NumColumns,0)
{
   DEFINE_UV_PROTOTYPE( spanloc,     pDisplayUnits->GetSpanLengthUnit(),      false );
   DEFINE_UV_PROTOTYPE( gdrloc,      pDisplayUnits->GetSpanLengthUnit(),      false );
   DEFINE_UV_PROTOTYPE( cg,          pDisplayUnits->GetComponentDimUnit(),    false );
   DEFINE_UV_PROTOTYPE( mod_e,       pDisplayUnits->GetModEUnit(),            false );
   DEFINE_UV_PROTOTYPE( force,       pDisplayUnits->GetGeneralForceUnit(),    false );
   DEFINE_UV_PROTOTYPE( area,        pDisplayUnits->GetAreaUnit(),            false );
   DEFINE_UV_PROTOTYPE( mom_inertia, pDisplayUnits->GetMomentOfInertiaUnit(), false );
   DEFINE_UV_PROTOTYPE( ecc,         pDisplayUnits->GetComponentDimUnit(),    false );
   DEFINE_UV_PROTOTYPE( moment,      pDisplayUnits->GetMomentUnit(),          false );
   DEFINE_UV_PROTOTYPE( stress,      pDisplayUnits->GetStressUnit(),          false );
}

CElasticGainDueToDeckPlacementTable* CElasticGainDueToDeckPlacementTable::PrepareTable(rptChapter* pChapter,IBroker* pBroker,SpanIndexType span,GirderIndexType gdr,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   // Create and configure the table
   ColumnIndexType numColumns = 6;

   CElasticGainDueToDeckPlacementTable* table = new CElasticGainDueToDeckPlacementTable( numColumns, pDisplayUnits);
   pgsReportStyleHolder::ConfigureTable(table);

   std::_tstring strImagePath(pgsReportStyleHolder::GetImagePath());

   GET_IFACE2(pBroker,IBridgeMaterial,pMaterial);
   double Ec = pMaterial->GetEcGdr(span,gdr);
   double Ep = pMaterial->GetStrand(span,gdr,pgsTypes::Permanent)->GetE();

   rptParagraph* pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;
   *pParagraph << _T("Elastic Gain Due to Deck Placement [5.9.5.2.3a]") << rptNewLine;

   pParagraph = new rptParagraph;
   *pChapter << pParagraph;

   *pParagraph << rptRcImage(strImagePath + _T("DeltaFcd.png")) << rptNewLine;
   *pParagraph << rptRcImage(strImagePath + _T("ElasticGain.png")) << rptNewLine;

   table->mod_e.ShowUnitTag(true);
   *pParagraph << Sub2(_T("E"),_T("p")) << _T(" = ") << table->mod_e.SetValue( Ep ) << rptNewLine;
   *pParagraph << Sub2(_T("E"),_T("c")) << _T(" = ") << table->mod_e.SetValue( Ec ) << rptNewLine;
   table->mod_e.ShowUnitTag(false);

   *pParagraph << table << rptNewLine;

   (*table)(0,0) << COLHDR(_T("Location from")<<rptNewLine<<_T("Left Support"),rptLengthUnitTag,  pDisplayUnits->GetSpanLengthUnit() );
   (*table)(0,1) << COLHDR(Sub2(_T("M"),_T("Slab")) << _T(" + ") << Sub2(_T("M"),_T("Dia")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   (*table)(0,2) << COLHDR(Sub2(_T("e"),_T("p")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,3) << COLHDR(Sub2(_T("I"),_T("g")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit() );
   (*table)(0,4) << COLHDR(symbol(DELTA) << italic(ON) << Sub2(_T("f'"),_T("cd")) << italic(OFF), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,5) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pED")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   
   return table;
}

void CElasticGainDueToDeckPlacementTable::AddRow(rptChapter* pChapter,IBroker* pBroker,RowIndexType row,LOSSDETAILS& details,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   (*this)(row,1) << moment.SetValue( details.pLosses->GetAddlGdrMoment() );
   (*this)(row,2) << ecc.SetValue( details.pLosses->GetEccPermanent() );
   (*this)(row,3) << mom_inertia.SetValue( details.pLosses->GetIg() );
   (*this)(row,4) << stress.SetValue( details.pLosses->GetDeltaFcd1() );
   (*this)(row,5) << stress.SetValue( details.pLosses->ElasticGainDueToDeckPlacement() );
}
