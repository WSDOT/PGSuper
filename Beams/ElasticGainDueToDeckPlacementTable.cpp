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

CElasticGainDueToDeckPlacementTable::CElasticGainDueToDeckPlacementTable(ColumnIndexType NumColumns, IDisplayUnits* pDispUnit) :
rptRcTable(NumColumns,0)
{
   DEFINE_UV_PROTOTYPE( spanloc,     pDispUnit->GetSpanLengthUnit(),      false );
   DEFINE_UV_PROTOTYPE( gdrloc,      pDispUnit->GetSpanLengthUnit(),      false );
   DEFINE_UV_PROTOTYPE( cg,          pDispUnit->GetComponentDimUnit(),    false );
   DEFINE_UV_PROTOTYPE( mod_e,       pDispUnit->GetModEUnit(),            false );
   DEFINE_UV_PROTOTYPE( force,       pDispUnit->GetGeneralForceUnit(),    false );
   DEFINE_UV_PROTOTYPE( area,        pDispUnit->GetAreaUnit(),            false );
   DEFINE_UV_PROTOTYPE( mom_inertia, pDispUnit->GetMomentOfInertiaUnit(), false );
   DEFINE_UV_PROTOTYPE( ecc,         pDispUnit->GetComponentDimUnit(),    false );
   DEFINE_UV_PROTOTYPE( moment,      pDispUnit->GetMomentUnit(),          false );
   DEFINE_UV_PROTOTYPE( stress,      pDispUnit->GetStressUnit(),          false );
}

CElasticGainDueToDeckPlacementTable* CElasticGainDueToDeckPlacementTable::PrepareTable(rptChapter* pChapter,IBroker* pBroker,SpanIndexType span,GirderIndexType gdr,IDisplayUnits* pDispUnit,Uint16 level)
{
   // Create and configure the table
   ColumnIndexType numColumns = 10;

   CElasticGainDueToDeckPlacementTable* table = new CElasticGainDueToDeckPlacementTable( numColumns, pDispUnit);
   pgsReportStyleHolder::ConfigureTable(table);

   std::string strImagePath(pgsReportStyleHolder::GetImagePath());

   GET_IFACE2(pBroker,IBridgeMaterial,pMaterial);
   double Ec = pMaterial->GetEcGdr(span,gdr);
   double Ep = pMaterial->GetStrand(span,gdr)->GetE();

   rptParagraph* pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;
   *pParagraph << "Elastic Gain Due to Deck Placement [5.9.5.2.3a]" << rptNewLine;

   pParagraph = new rptParagraph;
   *pChapter << pParagraph;

   *pParagraph << rptRcImage(strImagePath + "DeltaFcd.gif") << rptNewLine;
   *pParagraph << rptRcImage(strImagePath + "ElasticGain.gif") << rptNewLine;

   table->mod_e.ShowUnitTag(true);
   *pParagraph << Sub2("E","p") << " = " << table->mod_e.SetValue( Ep ) << rptNewLine;
   *pParagraph << Sub2("E","c") << " = " << table->mod_e.SetValue( Ec ) << rptNewLine;
   table->mod_e.ShowUnitTag(false);

   *pParagraph << table << rptNewLine;

   (*table)(0,0) << COLHDR("Location from"<<rptNewLine<<"Left Support",rptLengthUnitTag,  pDispUnit->GetSpanLengthUnit() );
   (*table)(0,1) << COLHDR("M" << Sub("Slab") << " + M" << Sub("Dia"), rptMomentUnitTag, pDispUnit->GetMomentUnit() );
   (*table)(0,2) << COLHDR("M" << Sub("sidl"), rptMomentUnitTag, pDispUnit->GetMomentUnit() );
   (*table)(0,3) << COLHDR("e" << Sub("perm"), rptLengthUnitTag, pDispUnit->GetComponentDimUnit() );
   (*table)(0,4) << COLHDR("I" << Sub("g"), rptLength4UnitTag, pDispUnit->GetMomentOfInertiaUnit() );
   (*table)(0,5) << COLHDR("I" << Sub("c"), rptLength4UnitTag, pDispUnit->GetMomentOfInertiaUnit() );
   (*table)(0,6) << COLHDR("Y" << Sub("bc"), rptLengthUnitTag, pDispUnit->GetComponentDimUnit() );
   (*table)(0,7) << COLHDR("Y" << Sub("bg"), rptLengthUnitTag, pDispUnit->GetComponentDimUnit() );
   (*table)(0,8) << COLHDR(symbol(DELTA) << "f'" << Sub("cd"), rptStressUnitTag, pDispUnit->GetStressUnit() );
   (*table)(0,9) << COLHDR(symbol(DELTA) << "f" << Sub("pED"), rptStressUnitTag, pDispUnit->GetStressUnit() );
   
   return table;
}

void CElasticGainDueToDeckPlacementTable::AddRow(rptChapter* pChapter,IBroker* pBroker,RowIndexType row,LOSSDETAILS& details,IDisplayUnits* pDispUnit,Uint16 level)
{
   (*this)(row,1) << moment.SetValue( details.pLosses->GetAddlGdrMoment() );
   (*this)(row,2) << moment.SetValue( details.pLosses->GetSidlMoment() );
   (*this)(row,3) << ecc.SetValue( details.pLosses->GetEccPermanent() );
   (*this)(row,4) << mom_inertia.SetValue( details.pLosses->GetIg() );
   (*this)(row,5) << mom_inertia.SetValue( details.pLosses->GetIc() );
   (*this)(row,6) << cg.SetValue( details.pLosses->GetYbc() );
   (*this)(row,7) << cg.SetValue( details.pLosses->GetYbg() );
   (*this)(row,8) << stress.SetValue( details.pLosses->GetDeltaFcd1() );
   (*this)(row,9) << stress.SetValue( details.pLosses->ElasticGainDueToDeckPlacement() );
}
