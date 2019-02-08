///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2019  Washington State Department of Transportation
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
#include <IFace\PrestressForce.h>
#include <PsgLib\SpecLibraryEntry.h>

#if defined _DEBUG
#include <IFace\Intervals.h>
#include <IFace\PrestressForce.h>
#endif

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
   scalar.SetWidth(6); // -99.9
   scalar.SetPrecision(1);
   scalar.SetTolerance(1.0e-6);
}

CTotalPrestressLossTable* CTotalPrestressLossTable::PrepareTable(rptChapter* pChapter,IBroker* pBroker,const CSegmentKey& segmentKey,const LOSSDETAILS* pDetails,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   GET_IFACE2(pBroker,ISegmentData,pSegmentData);
   const CStrandData* pStrands = pSegmentData->GetStrandData(segmentKey);

   bool bIgnoreInitialRelaxation = pDetails->pLosses->IgnoreInitialRelaxation();

   ColumnIndexType numColumns = 9;
   if ( !bIgnoreInitialRelaxation )
   {
      numColumns++; // fpR0
   }

   if ( pStrands->GetTemporaryStrandUsage() != pgsTypes::ttsPretensioned )
   {
      numColumns++; // fpp
   }

   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);
   StrandIndexType NtMax = pStrandGeom->GetMaxStrands(segmentKey,pgsTypes::Temporary);
   if ( 0 == NtMax  )
   {
      numColumns--; // omit fptr
   }

   // Create and configure the table
   CTotalPrestressLossTable* table = new CTotalPrestressLossTable( numColumns, pDisplayUnits );
   rptStyleManager::ConfigureTable(table);

   std::_tstring strImagePath(rptStyleManager::GetImagePath());
   
   rptParagraph* pParagraph = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pParagraph;
   pParagraph->SetName(_T("Total Prestress Loss"));
   *pParagraph << pParagraph->GetName() << rptNewLine;

   pParagraph = new rptParagraph;
   *pChapter << pParagraph;

   *pParagraph << _T("% Loss Initial = (");
   if ( !bIgnoreInitialRelaxation )
   {
      *pParagraph << symbol(DELTA) << RPT_STRESS(_T("pR0")) << _T(" + ");
   }

   *pParagraph << symbol(DELTA) << RPT_STRESS(_T("pES")) << _T(")/") << RPT_STRESS(_T("pj")) << rptNewLine;

   *pParagraph << _T("% Loss Final (Time dependent losses only) = (");
   if ( !bIgnoreInitialRelaxation )
   {
      *pParagraph << symbol(DELTA) << RPT_STRESS(_T("pR0")) << _T(" + ");
   }
   *pParagraph << symbol(DELTA) << RPT_STRESS(_T("pLT")) << _T(")/") << RPT_STRESS(_T("pj")) << rptNewLine;

   *pParagraph << _T("% Loss Total (Time dependent losses and instantaneous effects) = (");
   *pParagraph << RPT_STRESS(_T("pj")) << _T(" - ");
   *pParagraph << RPT_STRESS(_T("pe")) << _T(")/") << RPT_STRESS(_T("pj")) << rptNewLine;

   *pParagraph << table << rptNewLine;

   ColumnIndexType col = 0;
   (*table)(0,col++) << COLHDR(_T("Location from")<<rptNewLine<<_T("Left Support"),rptLengthUnitTag,  pDisplayUnits->GetSpanLengthUnit() );

   if ( !bIgnoreInitialRelaxation )
   {
      (*table)(0,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pR0")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   }

   (*table)(0,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pES")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,col++) << _T("% Loss") << rptNewLine << _T("Initial");

   if ( 0 < NtMax && pStrands->GetTemporaryStrandUsage() != pgsTypes::ttsPretensioned ) 
   {
      (*table)(0,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pp")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   }

   if ( 0 < NtMax )
   {
      (*table)(0,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("ptr")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   }

   (*table)(0,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pLT")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,col++) << _T("% Loss") << rptNewLine << _T("Final");
   (*table)(0,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pT")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,col++) << COLHDR(RPT_STRESS(_T("pe")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,col++) << _T("% Loss") << rptNewLine << _T("Total");

   table->m_NtMax = NtMax;
   table->m_pStrands = pStrands;
   table->m_bIgnoreInitialRelaxation = bIgnoreInitialRelaxation;

   return table;
}

void CTotalPrestressLossTable::AddRow(rptChapter* pChapter,IBroker* pBroker,const pgsPointOfInterest& poi,RowIndexType row,const LOSSDETAILS* pDetails,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   ColumnIndexType col = 1;
   RowIndexType rowOffset = GetNumberOfHeaderRows() - 1;

   Float64 dfpR0 = 0;
   if ( !m_bIgnoreInitialRelaxation )
   {
      dfpR0 = pDetails->pLosses->PermanentStrand_RelaxationLossesBeforeTransfer();
      (*this)(row+rowOffset,col++) << stress.SetValue(dfpR0);
   }

   Float64 dfpES = pDetails->pLosses->PermanentStrand_ElasticShorteningLosses();
   (*this)(row+rowOffset,col++) << stress.SetValue(dfpES);

   Float64 fpj = pDetails->pLosses->GetFpjPermanent();

   GET_IFACE2(pBroker,IPretensionForce,pPrestressForce);
   Float64 adj = pPrestressForce->GetXferLengthAdjustment(poi,pgsTypes::Permanent);

   (*this)(row+rowOffset,col++) << scalar.SetValue( 100*(fpj - adj*(fpj - (dfpR0+dfpES)))/fpj );
   
   Float64 dfpp = 0;
   if ( 0 < m_NtMax && m_pStrands->GetTemporaryStrandUsage() != pgsTypes::ttsPretensioned ) 
   {
      dfpp = pDetails->pLosses->GetDeltaFpp();
      (*this)(row+rowOffset,col++) << stress.SetValue(dfpp);
   }

   Float64 dfptr = 0;
   if ( 0 < m_NtMax )
   {
      dfptr = pDetails->pLosses->GetDeltaFptr();
      (*this)(row+rowOffset,col++) << stress.SetValue(dfptr);
   }


   Float64 dfpLT = pDetails->pLosses->TimeDependentLosses();
   (*this)(row+rowOffset,col++) << stress.SetValue(dfpLT);
   (*this)(row+rowOffset,col++) << scalar.SetValue( 100*(fpj - adj*(fpj - (dfpR0+dfpLT)))/fpj );

   Float64 dfpT = pDetails->pLosses->PermanentStrand_Final(); 

   Float64 dfpED   = pDetails->pLosses->ElasticGainDueToDeckPlacement();
   Float64 dfpSIDL = pDetails->pLosses->ElasticGainDueToSIDL();

   dfpT += dfpES + dfpp + dfptr - dfpED - dfpSIDL;
   (*this)(row+rowOffset,col++) << stress.SetValue(dfpT);

   Float64 fpe = fpj - dfpT;
   fpe *= adj;

#if defined _DEBUG
   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType intervalIdx = pIntervals->GetIntervalCount()-1;
   Float64 _fpe_ = pPrestressForce->GetEffectivePrestress(poi,pgsTypes::Permanent,intervalIdx,pgsTypes::End);
   ATLASSERT(IsEqual(fpe,_fpe_));
#endif

   (*this)(row+rowOffset,col++) << stress.SetValue(fpe);

   (*this)(row+rowOffset,col++) << scalar.SetValue( 100*(fpj-fpe)/fpj );
}
