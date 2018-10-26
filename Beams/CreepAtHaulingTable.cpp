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

// CreepAtHaulingTable.cpp : Implementation of CCreepAtHaulingTable
#include "stdafx.h"
#include "CreepAtHaulingTable.h"
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <PsgLib\SpecLibraryEntry.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CCreepAtHaulingTable::CCreepAtHaulingTable(ColumnIndexType NumColumns, IEAFDisplayUnits* pDisplayUnits) :
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

   strain.SetFormat( sysNumericFormatTool::Automatic );
   strain.SetWidth(6);
   strain.SetPrecision(3);
}

CCreepAtHaulingTable* CCreepAtHaulingTable::PrepareTable(rptChapter* pChapter,IBroker* pBroker,SpanIndexType span,GirderIndexType gdr,bool bTemporaryStrands,LOSSDETAILS& details,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   GET_IFACE2(pBroker,IGirderData,pGirderData);
   const CGirderData* pgirderData = pGirderData->GetGirderData(span,gdr);

   GET_IFACE2(pBroker,ISpecification,pSpec);
   std::_tstring strSpecName = pSpec->GetSpecification();

   GET_IFACE2(pBroker,ILibrary,pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( strSpecName.c_str() );

   // Create and configure the table
   ColumnIndexType numColumns = 5;
   if ( bTemporaryStrands )
      numColumns += 3;

   CCreepAtHaulingTable* table = new CCreepAtHaulingTable( numColumns, pDisplayUnits );
   pgsReportStyleHolder::ConfigureTable(table);

   table->m_bTemporaryStrands = bTemporaryStrands;
   table->m_pGirderData = pgirderData;

   std::_tstring strImagePath(pgsReportStyleHolder::GetImagePath());
   
   rptParagraph* pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;
   
   *pParagraph << _T("[5.9.5.4.2b] Creep of Girder Concrete : ") << symbol(DELTA) << RPT_STRESS(_T("pCRH")) << rptNewLine;

   if ( pgirderData->PrestressData.TempStrandUsage != pgsTypes::ttsPretensioned )
      *pParagraph << rptRcImage(strImagePath + _T("Delta_FpCRH_PT.png")) << rptNewLine;
   else
      *pParagraph << rptRcImage(strImagePath + _T("Delta_FpCRH.png")) << rptNewLine;

   pParagraph = new rptParagraph;
   *pChapter << pParagraph;

   table->time.ShowUnitTag(true);
   *pParagraph << Sub2(_T("k"),_T("td")) << _T(" = ") << table->scalar.SetValue(details.RefinedLosses2005.GetCreepInitialToShipping().GetKtd()) << rptNewLine;
   *pParagraph << Sub2(_T("t"),_T("i"))  << _T(" = ") << table->time.SetValue(details.RefinedLosses2005.GetAdjustedInitialAge())   << rptNewLine;
   *pParagraph << Sub2(_T("t"),_T("h"))  << _T(" = ") << table->time.SetValue(details.RefinedLosses2005.GetAgeAtHauling()) << rptNewLine;
   *pParagraph << Sub2(_T("K"),_T("1"))  << _T(" = ") << details.RefinedLosses2005.GetGdrK1Creep() << rptNewLine;
   *pParagraph << Sub2(_T("K"),_T("2"))  << _T(" = ") << details.RefinedLosses2005.GetGdrK2Creep() << rptNewLine;
   *pParagraph << Sub2(symbol(psi),_T("b")) << _T("(") << Sub2(_T("t"),_T("h")) << _T(",") << Sub2(_T("t"),_T("i")) << _T(")") << _T(" = ") << table->scalar.SetValue(details.RefinedLosses2005.GetCreepInitialToShipping().GetCreepCoefficient()) << rptNewLine;
   table->time.ShowUnitTag(false);

   *pParagraph << table << rptNewLine;

   ColumnIndexType col = 0;
   (*table)(0,col++) << COLHDR(_T("Location from")<<rptNewLine<<_T("End of Girder"),rptLengthUnitTag,  pDisplayUnits->GetSpanLengthUnit() );
   (*table)(0,col++) << COLHDR(_T("Location from")<<rptNewLine<<_T("Left Support"),rptLengthUnitTag,  pDisplayUnits->GetSpanLengthUnit() );

   if ( bTemporaryStrands )
   {
      table->SetNumberOfHeaderRows(2);

      table->SetRowSpan(0,0,2);
      table->SetRowSpan(0,1,2);
      table->SetRowSpan(1,0,SKIP_CELL);
      table->SetRowSpan(1,1,SKIP_CELL);

      table->SetColumnSpan(0,2,3);
      (*table)(0,col++) << _T("Permanent Strands");

      table->SetColumnSpan(0,3,3);
      (*table)(0,col++) << _T("Temporary Strands");

      for ( ColumnIndexType i = col; i < numColumns; i++ )
         table->SetColumnSpan(0,i,SKIP_CELL);


      col = 2;
      if ( pgirderData->PrestressData.TempStrandUsage == pgsTypes::ttsPretensioned )
         (*table)(1,col++) << COLHDR(RPT_STRESS(_T("cgp")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      else
         (*table)(1,col++) << COLHDR(RPT_STRESS(_T("cgp")) << _T(" + ") << symbol(DELTA) << RPT_STRESS(_T("pp")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );


      (*table)(1,col++) << Sub2(_T("K"),_T("ih"));
      (*table)(1,col++) << COLHDR(symbol(DELTA) <<RPT_STRESS(_T("pCRH")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );

      if ( bTemporaryStrands )
      {
         (*table)(1,col++) << COLHDR(RPT_STRESS(_T("cgp")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
         (*table)(1,col++) << Sub2(_T("K"),_T("ih"));
         (*table)(1,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pCRH")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      }
   }
   else
   {
      if ( pgirderData->PrestressData.TempStrandUsage == pgsTypes::ttsPretensioned )
         (*table)(0,col++) << COLHDR(RPT_STRESS(_T("cgp")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      else
         (*table)(0,col++) << COLHDR(RPT_STRESS(_T("cgp")) << _T(" + ") << symbol(DELTA) << RPT_STRESS(_T("pp")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );

      (*table)(0,col++) << Sub2(_T("K"),_T("ih"));
      (*table)(0,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pCRH")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   }

   return table;
}

void CCreepAtHaulingTable::AddRow(rptChapter* pChapter,IBroker* pBroker,const pgsPointOfInterest& poi,RowIndexType row,LOSSDETAILS& details,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   ColumnIndexType col = 2;
   RowIndexType rowOffset = GetNumberOfHeaderRows()-1;

   if ( m_pGirderData->PrestressData.TempStrandUsage == pgsTypes::ttsPretensioned )
      (*this)(row+rowOffset,col++) << stress.SetValue(details.pLosses->ElasticShortening().PermanentStrand_Fcgp());
   else
      (*this)(row+rowOffset,col++) << stress.SetValue(details.pLosses->ElasticShortening().PermanentStrand_Fcgp() + details.pLosses->GetDeltaFpp());

   (*this)(row+rowOffset,col++) << scalar.SetValue(details.RefinedLosses2005.GetPermanentStrandKih());
   (*this)(row+rowOffset,col++) << stress.SetValue(details.RefinedLosses2005.PermanentStrand_CreepLossAtShipping());

   if ( m_bTemporaryStrands )
   {
      (*this)(row+rowOffset,col++) << stress.SetValue(details.pLosses->ElasticShortening().TemporaryStrand_Fcgp());
      (*this)(row+rowOffset,col++) << scalar.SetValue(details.RefinedLosses2005.GetTemporaryStrandKih());
      (*this)(row+rowOffset,col++) << stress.SetValue(details.RefinedLosses2005.TemporaryStrand_CreepLossAtShipping());
   }
}
