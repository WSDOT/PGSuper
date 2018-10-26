///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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

// TotalPrestressLossTable.cpp : Implementation of CTotalPrestressLossTable
#include "stdafx.h"
#include "TotalPrestressLossTable.h"
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <PsgLib\SpecLibraryEntry.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CTotalPrestressLossTable::CTotalPrestressLossTable(ColumnIndexType NumColumns, IEAFDisplayUnits* pDisplayUnits) :
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

   scalar.SetFormat( sysNumericFormatTool::Fixed );
   scalar.SetWidth(5); // -99.9
   scalar.SetPrecision(1);
   scalar.SetTolerance(1.0e-6);
}

CTotalPrestressLossTable* CTotalPrestressLossTable::PrepareTable(rptChapter* pChapter,IBroker* pBroker,SpanIndexType span,GirderIndexType gdr,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   GET_IFACE2(pBroker,IGirderData,pGirderData);
   const CGirderData* pgirderData = pGirderData->GetGirderData(span,gdr);

   GET_IFACE2(pBroker,ISpecification,pSpec);
   std::_tstring strSpecName = pSpec->GetSpecification();

   GET_IFACE2(pBroker,ILibrary,pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( strSpecName.c_str() );

   int method = pSpecEntry->GetLossMethod();
   bool bIgnoreInitialRelaxation = ( method == LOSSES_WSDOT_REFINED || method == LOSSES_WSDOT_LUMPSUM ) ? false : true;

   //bool bIgnoreElasticGain = ( pSpecEntry->GetSpecificationType() <= lrfdVersionMgr::ThirdEdition2004 ) ? true : false;

   ColumnIndexType numColumns = 4;
   if ( pgirderData->PrestressData.TempStrandUsage == pgsTypes::ttsPretensioned ) 
   {
      if ( bIgnoreInitialRelaxation )
      {
         numColumns++;
      }
      else
      {
         numColumns += 2;
      }
   }
   else
   {
      if ( bIgnoreInitialRelaxation )
      {
         numColumns += 2;
      }
      else
      {
         numColumns += 3;
      }
   }

   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);
   StrandIndexType NtMax = pStrandGeom->GetMaxStrands(span,gdr,pgsTypes::Temporary);
   if ( 0 == NtMax  )
      numColumns--;


   // add one column for % Loss
   numColumns += 1;

   // Create and configure the table
   CTotalPrestressLossTable* table = new CTotalPrestressLossTable( numColumns, pDisplayUnits );
   pgsReportStyleHolder::ConfigureTable(table);

   std::_tstring strImagePath(pgsReportStyleHolder::GetImagePath());
   
   rptParagraph* pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;
   *pParagraph << _T("Total Prestress Loss") << rptNewLine;

   pParagraph = new rptParagraph;
   *pChapter << pParagraph;

   if ( 0 < NtMax )
   {
      if ( pgirderData->PrestressData.TempStrandUsage == pgsTypes::ttsPretensioned ) 
      {
         if ( bIgnoreInitialRelaxation )
         {
            *pParagraph << rptRcImage(strImagePath + _T("TotalPrestressLossWithPS_LRFD.png")) << rptNewLine;
         }
         else
         {
            *pParagraph << rptRcImage(strImagePath + _T("TotalPrestressLossWithPS_WSDOT.png")) << rptNewLine;
         }
      }
      else
      {
         if ( bIgnoreInitialRelaxation )
         {
            *pParagraph << rptRcImage(strImagePath + _T("TotalPrestressLossWithPT_LRFD.png")) << rptNewLine;
         }
         else
         {
            *pParagraph << rptRcImage(strImagePath + _T("TotalPrestressLossWithPT_WSDOT.png")) << rptNewLine;
         }
      }
   }
   else
   {
      if ( bIgnoreInitialRelaxation )
      {
         *pParagraph << rptRcImage(strImagePath + _T("TotalPrestressLoss_LRFD.png")) << rptNewLine;
      }
      else
      {
         *pParagraph << rptRcImage(strImagePath + _T("TotalPrestressLoss_WSDOT.png")) << rptNewLine;
      }
   }

   *pParagraph << table << rptNewLine;

   ColumnIndexType col = 0;
   (*table)(0,col++) << COLHDR(_T("Location from")<<rptNewLine<<_T("Left Support"),rptLengthUnitTag,  pDisplayUnits->GetSpanLengthUnit() );

   if ( !bIgnoreInitialRelaxation )
   {
      (*table)(0,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pR0")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   }

   (*table)(0,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pES")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );

   if ( 0 < NtMax && pgirderData->PrestressData.TempStrandUsage != pgsTypes::ttsPretensioned ) 
   {
      (*table)(0,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pp")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   }

   if ( 0 < NtMax )
   {
      (*table)(0,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("ptr")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   }

   (*table)(0,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pLT")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pT")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,col++) << _T("% Loss");

   table->m_NtMax = NtMax;
   table->m_pGirderData = pgirderData;

   return table;
}

void CTotalPrestressLossTable::AddRow(rptChapter* pChapter,IBroker* pBroker,const pgsPointOfInterest& poi,RowIndexType row,LOSSDETAILS& details,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   ColumnIndexType col = 1;
   if ( !details.pLosses->IgnoreInitialRelaxation() )
   {
      (*this)(row,col++) << stress.SetValue(details.pLosses->PermanentStrand_RelaxationLossesBeforeTransfer());
   }

   (*this)(row,col++) << stress.SetValue(details.pLosses->PermanentStrand_ElasticShorteningLosses());
   
   if ( 0 < m_NtMax && m_pGirderData->PrestressData.TempStrandUsage != pgsTypes::ttsPretensioned ) 
   {
      (*this)(row,col++) << stress.SetValue(details.pLosses->GetDeltaFpp());
   }


   if ( 0 < m_NtMax )
   {
      (*this)(row,col++) << stress.SetValue(details.pLosses->GetDeltaFptr());
   }


   (*this)(row,col++) << stress.SetValue(details.pLosses->TimeDependentLosses());

   Float64 fpj = details.pLosses->GetFpjPermanent();
   Float64 fpt = details.pLosses->PermanentStrand_Final(); // includes elastic gains due to permanent loads

   // remove gains so we have only losses
   fpt += details.pLosses->ElasticGainDueToDeckPlacement();
   fpt += details.pLosses->ElasticGainDueToSIDL();
   fpt += details.pLosses->ElasticGainDueToDeckShrinkage();

   Float64 fpe  = fpj - fpt;

   (*this)(row,col++) << stress.SetValue(fpt);
   (*this)(row,col++) << scalar.SetValue( -1*PercentChange(fpj,fpe) );
}
