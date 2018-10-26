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

CTotalPrestressLossTable* CTotalPrestressLossTable::PrepareTable(rptChapter* pChapter,IBroker* pBroker,const CSegmentKey& segmentKey,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   GET_IFACE2(pBroker,ISegmentData,pSegmentData);
   const CStrandData* pStrands = pSegmentData->GetStrandData(segmentKey);

   GET_IFACE2(pBroker,ILossParameters,pLossParameters);
   pgsTypes::LossMethod loss_method = pLossParameters->GetLossMethod();

   bool bIgnoreInitialRelaxation = ( loss_method == pgsTypes::WSDOT_REFINED || loss_method == pgsTypes::WSDOT_LUMPSUM ) ? false : true;

   GET_IFACE2(pBroker,ISectionProperties,pSectProp);
   bool bUseGrossProperties = pSectProp->GetSectionPropertiesMode() == pgsTypes::spmGross ? true : false;

   ColumnIndexType numColumns = 9;
   if ( !bIgnoreInitialRelaxation )
      numColumns++; // fpR0

   if ( pStrands->TempStrandUsage != pgsTypes::ttsPretensioned )
      numColumns++; // fpp

   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);
   StrandIndexType NtMax = pStrandGeom->GetMaxStrands(segmentKey,pgsTypes::Temporary);
   if ( 0 == NtMax  )
      numColumns--; // omit fptr

   // Create and configure the table
   CTotalPrestressLossTable* table = new CTotalPrestressLossTable( numColumns, pDisplayUnits );
   pgsReportStyleHolder::ConfigureTable(table);

   std::_tstring strImagePath(pgsReportStyleHolder::GetImagePath());
   
   rptParagraph* pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;
   *pParagraph << _T("Total Prestress Loss") << rptNewLine;

   pParagraph = new rptParagraph;
   *pChapter << pParagraph;

   // delta fpT
   *pParagraph << symbol(DELTA) << RPT_STRESS(_T("pT")) << _T(" = ");
   if ( !bIgnoreInitialRelaxation )
      *pParagraph << symbol(DELTA) << RPT_STRESS(_T("pR0")) << _T(" + ");

   *pParagraph << symbol(DELTA) << RPT_STRESS(_T("pES")) << _T(" + ");

   if ( pStrands->TempStrandUsage != pgsTypes::ttsPretensioned )
      *pParagraph << symbol(DELTA) << RPT_STRESS(_T("pp")) << _T(" + ");

   if ( 0 < NtMax )
      *pParagraph << symbol(DELTA) << RPT_STRESS(_T("ptr")) << _T(" + ");

   *pParagraph << symbol(DELTA) << RPT_STRESS(_T("pLT")) << rptNewLine;

   //
   *pParagraph << _T("% Loss Initial = (");
   if ( !bIgnoreInitialRelaxation )
      *pParagraph << symbol(DELTA) << RPT_STRESS(_T("pR0")) << _T(" + ");

   *pParagraph << symbol(DELTA) << RPT_STRESS(_T("pES")) << _T(")/") << RPT_STRESS(_T("pj")) << rptNewLine;

   //
   *pParagraph << _T("% Loss Final = (");
   *pParagraph << symbol(DELTA) << RPT_STRESS(_T("pT")) << _T(" - ");
   *pParagraph << symbol(DELTA) << RPT_STRESS(_T("pED")) << _T(" - ") << symbol(DELTA) << RPT_STRESS(_T("pSIDL")) << _T(")/") << RPT_STRESS(_T("pj")) << rptNewLine;

   *pParagraph << table << rptNewLine;

   ColumnIndexType col = 0;
   (*table)(0,col++) << COLHDR(_T("Location from")<<rptNewLine<<_T("Left Support"),rptLengthUnitTag,  pDisplayUnits->GetSpanLengthUnit() );

   if ( !bIgnoreInitialRelaxation )
   {
      (*table)(0,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pR0")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   }

   (*table)(0,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pES")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,col++) << _T("% Loss") << rptNewLine << _T("Initial");

   if ( 0 < NtMax && pStrands->TempStrandUsage != pgsTypes::ttsPretensioned ) 
   {
      (*table)(0,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pp")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   }

   if ( 0 < NtMax )
   {
      (*table)(0,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("ptr")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   }

   (*table)(0,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pLT")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pED")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pSIDL")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pT")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,col++) << _T("% Loss") << rptNewLine << _T("Final");

   table->m_NtMax = NtMax;
   table->m_pStrands = pStrands;
   table->m_bIgnoreInitialRelaxation = bIgnoreInitialRelaxation;
   table->m_bUseGrossProperties = bUseGrossProperties;

   return table;
}

void CTotalPrestressLossTable::AddRow(rptChapter* pChapter,IBroker* pBroker,const pgsPointOfInterest& poi,RowIndexType row,const LOSSDETAILS* pDetails,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   ColumnIndexType col = 1;
   if ( !m_bIgnoreInitialRelaxation )
   {
      (*this)(row,col++) << stress.SetValue(pDetails->pLosses->PermanentStrand_RelaxationLossesBeforeTransfer());
   }

   Float64 dfpES = pDetails->pLosses->PermanentStrand_ElasticShorteningLosses();
   (*this)(row,col++) << stress.SetValue(dfpES);

   Float64 fpj = pDetails->pLosses->GetFpjPermanent();
   Float64 fpi = pDetails->pLosses->PermanentStrand_AfterTransfer(); 

   if ( !m_bUseGrossProperties )
      fpi += dfpES;

   Float64 fpe  = fpj - fpi;
   (*this)(row,col++) << scalar.SetValue( -1*PercentChange(fpj,fpe) );
   
   if ( 0 < m_NtMax && m_pStrands->TempStrandUsage != pgsTypes::ttsPretensioned ) 
   {
      (*this)(row,col++) << stress.SetValue(pDetails->pLosses->GetDeltaFpp());
   }


   if ( 0 < m_NtMax )
   {
      (*this)(row,col++) << stress.SetValue(pDetails->pLosses->GetDeltaFptr());
   }


   (*this)(row,col++) << stress.SetValue(pDetails->pLosses->TimeDependentLosses());

   (*this)(row,col++) << stress.SetValue(pDetails->pLosses->ElasticGainDueToDeckPlacement());
   (*this)(row,col++) << stress.SetValue(pDetails->pLosses->ElasticGainDueToSIDL());

   Float64 fpt = pDetails->pLosses->PermanentStrand_Final(); 

   if ( !m_bUseGrossProperties )
      fpt += dfpES;

   Float64 fpED   = pDetails->pLosses->ElasticGainDueToDeckPlacement();
   Float64 fpSIDL = pDetails->pLosses->ElasticGainDueToSIDL();
   if ( m_bUseGrossProperties )
   {
      fpt += fpED+fpSIDL;
   }

   fpe  = fpj - fpt + fpED + fpSIDL;

   (*this)(row,col++) << stress.SetValue(fpt);
   (*this)(row,col++) << scalar.SetValue( -1*PercentChange(fpj,fpe) );
}
