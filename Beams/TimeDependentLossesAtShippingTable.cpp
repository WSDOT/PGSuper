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

// TimeDependentLossesAtShippingTable.cpp : Implementation of CTimeDependentLossesAtShippingTable
#include "stdafx.h"
#include "TimeDependentLossesAtShippingTable.h"
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <PsgLib\SpecLibraryEntry.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CTimeDependentLossesAtShippingTable::CTimeDependentLossesAtShippingTable(ColumnIndexType NumColumns, IEAFDisplayUnits* pDisplayUnits) :
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

   scalar.SetFormat( sysNumericFormatTool::Automatic );
   scalar.SetWidth(6);
   scalar.SetPrecision(2);
}

CTimeDependentLossesAtShippingTable* CTimeDependentLossesAtShippingTable::PrepareTable(rptChapter* pChapter,IBroker* pBroker,SpanIndexType span,GirderIndexType gdr,bool bTemporaryStrands,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   GET_IFACE2(pBroker,ISpecification,pSpec);
   std::_tstring strSpecName = pSpec->GetSpecification();

   GET_IFACE2(pBroker,ILibrary,pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( strSpecName.c_str() );

   GET_IFACE2(pBroker,IGirderData,pGirderData);
   CGirderData girderData = pGirderData->GetGirderData(span,gdr);

   std::_tstring strImagePath(pgsReportStyleHolder::GetImagePath());

   int method = pSpecEntry->GetLossMethod();
   bool bIgnoreInitialRelaxation = ( method == LOSSES_WSDOT_REFINED || method == LOSSES_WSDOT_LUMPSUM ) ? false : true;
   if ((pSpecEntry->GetSpecificationType() <= lrfdVersionMgr::ThirdEdition2004 && method == LOSSES_AASHTO_REFINED) ||
        method == LOSSES_TXDOT_REFINED_2004)
   {
      bIgnoreInitialRelaxation = false;
   }

   // Create and configure the table
   ColumnIndexType numColumns = 6;
   if ( bIgnoreInitialRelaxation ) // for perm strands
      numColumns--;

   if ( girderData.TempStrandUsage != pgsTypes::ttsPretensioned ) 
      numColumns++;

   if ( bTemporaryStrands )
   {
      numColumns += 4;

      if ( bIgnoreInitialRelaxation ) // for temp strands
         numColumns--;
   }

   CTimeDependentLossesAtShippingTable* table = new CTimeDependentLossesAtShippingTable( numColumns, pDisplayUnits );
   pgsReportStyleHolder::ConfigureTable(table);

   table->m_bTemporaryStrands = bTemporaryStrands;

   table->m_GirderData = girderData;

   rptParagraph* pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;
   *pParagraph << _T("Losses at Shipping") << rptNewLine;

   if ( girderData.TempStrandUsage != pgsTypes::ttsPretensioned ) 
   {
      *pParagraph << _T("Prestressed Permanent Strands") << rptNewLine;
   }

   pParagraph = new rptParagraph;
   *pChapter << pParagraph;

   *pParagraph << table << rptNewLine;

   int col = 0;
   (*table)(0,col++) << COLHDR(_T("Location from")<<rptNewLine<<_T("End of Girder"),rptLengthUnitTag,  pDisplayUnits->GetSpanLengthUnit() );
   (*table)(0,col++) << COLHDR(_T("Location from")<<rptNewLine<<_T("Left Support"),rptLengthUnitTag,  pDisplayUnits->GetSpanLengthUnit() );

   if ( bTemporaryStrands )
   {
      table->SetNumberOfHeaderRows(2);

      table->SetRowSpan(0,0,2);
      table->SetRowSpan(0,1,2);
      table->SetRowSpan(1,0,SKIP_CELL);
      table->SetRowSpan(1,1,SKIP_CELL);

      int colspan = 4;
      if ( bIgnoreInitialRelaxation )
         colspan--;

      table->SetColumnSpan(0,2,colspan + (girderData.TempStrandUsage != pgsTypes::ttsPretensioned ? 1 : 0));
      (*table)(0,2) << _T("Permanent Strands");

      table->SetColumnSpan(0,3,colspan);
      (*table)(0,3) << _T("Temporary Strands");

      for ( ColumnIndexType i = 4; i < numColumns; i++ )
         table->SetColumnSpan(0,i,SKIP_CELL);

      // permanent
      col = 2;
      if ( !bIgnoreInitialRelaxation )
         (*table)(1,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pR0")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );

      (*table)(1,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pES")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*table)(1,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pLTH")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );

      if ( girderData.TempStrandUsage != pgsTypes::ttsPretensioned ) 
      {
         (*table)(1,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pp")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      }

      (*table)(1,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pH")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );


      // temporary
      if ( !bIgnoreInitialRelaxation )
         (*table)(1,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pR0")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );

      (*table)(1,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pES")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*table)(1,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pLTH")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*table)(1,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pH")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );

      if ( method == LOSSES_WSDOT_REFINED || method == LOSSES_AASHTO_REFINED || method == LOSSES_TXDOT_REFINED_2004 )
      {
         *pParagraph << symbol(DELTA) << RPT_STRESS(_T("pLTH")) << _T(" = ") << symbol(DELTA) << RPT_STRESS(_T("pSRH")) << _T(" + ") << symbol(DELTA) << RPT_STRESS(_T("pCRH")) << _T(" + ") << symbol(DELTA) << RPT_STRESS(_T("pR1H")) << rptNewLine;
      }
   }
   else
   {
      if ( !bIgnoreInitialRelaxation )
         (*table)(0,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pR0")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );

      (*table)(0,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pES")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*table)(0,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pLTH")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );

      if ( girderData.TempStrandUsage != pgsTypes::ttsPretensioned ) 
      {
         (*table)(0,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pp")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      }

      (*table)(0,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pH")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );

      if ( method == LOSSES_WSDOT_REFINED || method == LOSSES_AASHTO_REFINED || method == LOSSES_TXDOT_REFINED_2004 )
      {
         *pParagraph << symbol(DELTA) << RPT_STRESS(_T("pLTH")) << _T(" = ") << symbol(DELTA) << RPT_STRESS(_T("pSRH")) << _T(" + ") << symbol(DELTA) << RPT_STRESS(_T("pCRH")) << _T(" + ") << symbol(DELTA) << RPT_STRESS(_T("pR1H")) << rptNewLine;
      }
   }


   if ( girderData.TempStrandUsage != pgsTypes::ttsPretensioned ) 
   {
      if ( bIgnoreInitialRelaxation )
         *pParagraph << rptRcImage(strImagePath + _T("PrestressLossAtHaulingWithPT_LRFD.png")) << rptNewLine;
      else
         *pParagraph << rptRcImage(strImagePath + _T("PrestressLossAtHaulingWithPT_WSDOT.png")) << rptNewLine;
   }
   else
   {
      if ( bIgnoreInitialRelaxation )
         *pParagraph << rptRcImage(strImagePath + _T("PrestressLossAtHauling_LRFD.png")) << rptNewLine;
      else
         *pParagraph << rptRcImage(strImagePath + _T("PrestressLossAtHauling_WSDOT.png")) << rptNewLine;
   }


   return table;
}

void CTimeDependentLossesAtShippingTable::AddRow(rptChapter* pChapter,IBroker* pBroker,const pgsPointOfInterest& poi,RowIndexType row,LOSSDETAILS& details,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   ColumnIndexType col = 2;
   RowIndexType rowOffset = GetNumberOfHeaderRows() - 1;

   if ( !details.pLosses->IgnoreInitialRelaxation() )
         (*this)(row+rowOffset,col++) << stress.SetValue(details.pLosses->PermanentStrand_RelaxationLossesBeforeTransfer());

   (*this)(row+rowOffset,col++) << stress.SetValue(details.pLosses->PermanentStrand_ElasticShorteningLosses());
   (*this)(row+rowOffset,col++) << stress.SetValue(details.pLosses->PermanentStrand_TimeDependentLossesAtShipping());
   if ( m_GirderData.TempStrandUsage != pgsTypes::ttsPretensioned ) 
   {
      (*this)(row+rowOffset,col++) << stress.SetValue(details.pLosses->GetDeltaFpp());
   }
   (*this)(row+rowOffset,col++) << stress.SetValue(details.pLosses->PermanentStrand_AtShipping());

   if ( m_bTemporaryStrands )
   {
      if ( !details.pLosses->IgnoreInitialRelaxation() )
         (*this)(row+rowOffset,col++) << stress.SetValue(details.pLosses->TemporaryStrand_RelaxationLossesBeforeTransfer());

      (*this)(row+rowOffset,col++) << stress.SetValue(details.pLosses->TemporaryStrand_ElasticShorteningLosses());
      (*this)(row+rowOffset,col++) << stress.SetValue(details.pLosses->TemporaryStrand_TimeDependentLossesAtShipping());
      (*this)(row+rowOffset,col++) << stress.SetValue(details.pLosses->TemporaryStrand_AtShipping());
   }
}
