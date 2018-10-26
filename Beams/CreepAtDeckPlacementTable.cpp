///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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

// CreepAtDeckPlacementTable.cpp : Implementation of CCreepAtDeckPlacementTable
#include "stdafx.h"
#include "CreepAtDeckPlacementTable.h"
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <PsgLib\SpecLibraryEntry.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CCreepAtDeckPlacementTable::CCreepAtDeckPlacementTable(ColumnIndexType NumColumns, IEAFDisplayUnits* pDisplayUnits) :
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

CCreepAtDeckPlacementTable* CCreepAtDeckPlacementTable::PrepareTable(rptChapter* pChapter,IBroker* pBroker,SpanIndexType span,GirderIndexType gdr,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   GET_IFACE2(pBroker,IGirderData,pGirderData);
   const CGirderData* pgirderData = pGirderData->GetGirderData(span,gdr);

   std::_tstring strImagePath(pgsReportStyleHolder::GetImagePath());

   // Create and configure the table
   ColumnIndexType numColumns = 8;
   CCreepAtDeckPlacementTable* table = new CCreepAtDeckPlacementTable( numColumns, pDisplayUnits );
   pgsReportStyleHolder::ConfigureTable(table);

   table->m_pGirderData = pgirderData;

   rptParagraph* pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;
   *pParagraph << _T("[5.9.5.4.2b] Creep of Girder Concrete : ") << symbol(DELTA) << RPT_STRESS(_T("pCR")) << rptNewLine;

   if ( pgirderData->PrestressData.TempStrandUsage != pgsTypes::ttsPretensioned )
      *pParagraph << rptRcImage(strImagePath + _T("Delta_FpCR_PT.png")) << rptNewLine;
   else
      *pParagraph << rptRcImage(strImagePath + _T("Delta_FpCR.png")) << rptNewLine;

   // creep loss   
   *pParagraph << table << rptNewLine;
   (*table)(0,0) << COLHDR(_T("Location from")<<rptNewLine<<_T("Left Support"),rptLengthUnitTag,  pDisplayUnits->GetSpanLengthUnit() );

   if ( pgirderData->PrestressData.TempStrandUsage == pgsTypes::ttsPretensioned )
      (*table)(0,1) << COLHDR(RPT_STRESS(_T("cgp")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   else
      (*table)(0,1) << COLHDR(RPT_STRESS(_T("cgp")) << _T(" + ") << symbol(DELTA) << RPT_STRESS(_T("pp")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );

   (*table)(0,2) << Sub2(_T("k"),_T("td"));
   (*table)(0,3) << COLHDR(Sub2(_T("t"),_T("i")),rptTimeUnitTag,pDisplayUnits->GetLongTimeUnit());
   (*table)(0,4) << COLHDR(Sub2(_T("t"),_T("d")),rptTimeUnitTag,pDisplayUnits->GetLongTimeUnit());
   (*table)(0,5) << Sub2(symbol(psi),_T("b")) << _T("(") << Sub2(_T("t"),_T("d")) << _T(",") << Sub2(_T("t"),_T("i")) << _T(")");
   (*table)(0,6) << Sub2(_T("K"),_T("id"));
   (*table)(0,7) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pCR")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );

   return table;
}

void CCreepAtDeckPlacementTable::AddRow(rptChapter* pChapter,IBroker* pBroker,const pgsPointOfInterest& poi,RowIndexType row,LOSSDETAILS& details,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
  // Typecast to our known type (eating own doggy food)
   boost::shared_ptr<const lrfdRefinedLosses2005> ptl = boost::dynamic_pointer_cast<const lrfdRefinedLosses2005>(details.pLosses);
   if (!ptl)
   {
      ATLASSERT(0); // made a bad cast? Bail...
      return;
   }

   if ( m_pGirderData->PrestressData.TempStrandUsage == pgsTypes::ttsPretensioned )
      (*this)(row,1) << stress.SetValue(details.pLosses->ElasticShortening().PermanentStrand_Fcgp());
   else
      (*this)(row,1) << stress.SetValue(details.pLosses->ElasticShortening().PermanentStrand_Fcgp() + ptl->GetDeltaFpp());

   (*this)(row,2) << scalar.SetValue(ptl->GetCreepInitialToDeck().GetKtd());
   (*this)(row,3) << time.SetValue(ptl->GetAdjustedInitialAge());
   (*this)(row,4) << time.SetValue(ptl->GetAgeAtDeckPlacement());
   (*this)(row,5) << scalar.SetValue(ptl->GetCreepInitialToDeck().GetCreepCoefficient());
   (*this)(row,6) << scalar.SetValue(ptl->GetKid());
   (*this)(row,7) << stress.SetValue(ptl->CreepLossBeforeDeckPlacement() );
}
