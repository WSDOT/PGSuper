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

// FinalPrestressLossTable.cpp : Implementation of CFinalPrestressLossTable
#include "stdafx.h"
#include "FinalPrestressLossTable.h"
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <PsgLib\SpecLibraryEntry.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CFinalPrestressLossTable::CFinalPrestressLossTable(ColumnIndexType NumColumns, IEAFDisplayUnits* pDisplayUnits) :
rptRcTable(NumColumns,0)
{
   DEFINE_UV_PROTOTYPE( spanloc,     pDisplayUnits->GetSpanLengthUnit(),      false );
   DEFINE_UV_PROTOTYPE( gdrloc,      pDisplayUnits->GetSpanLengthUnit(),      false );
   DEFINE_UV_PROTOTYPE( cg,          pDisplayUnits->GetSpanLengthUnit(),      false );
   DEFINE_UV_PROTOTYPE( mod_e,       pDisplayUnits->GetModEUnit(),            false );
   DEFINE_UV_PROTOTYPE( force,       pDisplayUnits->GetGeneralForceUnit(),    false );
   DEFINE_UV_PROTOTYPE( area,        pDisplayUnits->GetAreaUnit(),            false );
   DEFINE_UV_PROTOTYPE( mom_inertia, pDisplayUnits->GetMomentOfInertiaUnit(), false );
   DEFINE_UV_PROTOTYPE( ecc,         pDisplayUnits->GetComponentDimUnit(),    false );
   DEFINE_UV_PROTOTYPE( moment,      pDisplayUnits->GetMomentUnit(),          false );
   DEFINE_UV_PROTOTYPE( stress,      pDisplayUnits->GetStressUnit(),          false );
}

CFinalPrestressLossTable* CFinalPrestressLossTable::PrepareTable(rptChapter* pChapter,IBroker* pBroker,const CSegmentKey& segmentKey,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   GET_IFACE2(pBroker,ISpecification,pSpec);
   std::_tstring strSpecName = pSpec->GetSpecification();

   GET_IFACE2(pBroker,ILibrary,pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( strSpecName.c_str() );

   bool bIgnoreElasticGain = ( pSpecEntry->GetSpecificationType() <= lrfdVersionMgr::ThirdEdition2004 ) ? true : false;
   
   // Create and configure the table
   ColumnIndexType numColumns = 9;

   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);
   StrandIndexType NtMax = pStrandGeom->GetMaxStrands(segmentKey,pgsTypes::Temporary);

   GET_IFACE2(pBroker,ISegmentData,pSegmentData);
   const CStrandData* pStrands = pSegmentData->GetStrandData(segmentKey);
   if ( 0 < NtMax )
   {
      if ( pStrands->GetTemporaryStrandUsage() == pgsTypes::ttsPretensioned ) 
      {
         numColumns++;
      }
      else
      {
         numColumns += 2;
      }
   }

   if ( bIgnoreElasticGain )
   {
      numColumns -= 2;
   }

   CFinalPrestressLossTable* table = new CFinalPrestressLossTable( numColumns, pDisplayUnits );
   pgsReportStyleHolder::ConfigureTable(table);

   std::_tstring strImagePath(pgsReportStyleHolder::GetImagePath());

   rptParagraph* pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;

   std::_tstring strYear = (bIgnoreElasticGain ? _T("") : _T("_2005"));
   if ( 0 < NtMax )
   {
      if ( pStrands->GetTemporaryStrandUsage() == pgsTypes::ttsPretensioned ) 
      {
         *pParagraph << _T("Total Prestress Loss ") << rptNewLine << rptRcImage(strImagePath + _T("DeltaFpt_PS_TTS") + strYear + _T(".png")) << rptNewLine;
      }
      else
      {
         *pParagraph << _T("Total Prestress Loss ") << rptNewLine << rptRcImage(strImagePath + _T("DeltaFpt_PT_TTS") + strYear + _T(".png")) << rptNewLine;
      }
   }
   else
   {
      *pParagraph << _T("Total Prestress Loss ") << rptNewLine << rptRcImage(strImagePath + _T("DeltaFpt") + strYear + _T(".png")) << rptNewLine;
   }
      

   *pParagraph << table << rptNewLine;

   ColumnIndexType col = 0;
   (*table)(0,col++) << COLHDR(_T("Location from")<<rptNewLine<<_T("Left Support"),rptLengthUnitTag,  pDisplayUnits->GetSpanLengthUnit() );
   (*table)(0,col++) << COLHDR(symbol(DELTA) << _T("f") << Sub(_T("pR1")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,col++) << COLHDR(symbol(DELTA) << _T("f") << Sub(_T("pES")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   
   if ( 0 < NtMax && pStrands->GetTemporaryStrandUsage() != pgsTypes::ttsPretensioned ) 
   {
      (*table)(0,col++) << COLHDR(symbol(DELTA) << _T("f") << Sub(_T("pp")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   }

   (*table)(0,col++) << COLHDR(symbol(DELTA) << _T("f") << Sub(_T("pSR")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,col++) << COLHDR(symbol(DELTA) << _T("f") << Sub(_T("pCR")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,col++) << COLHDR(symbol(DELTA) << _T("f") << Sub(_T("pR2")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );

   if ( 0 < NtMax ) 
   {
      (*table)(0,col++) << COLHDR(symbol(DELTA) << _T("f") << Sub(_T("ptr")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   }

   if ( !bIgnoreElasticGain )
   {
      (*table)(0,col++) << COLHDR(symbol(DELTA) << _T("f") << Sub(_T("pED")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*table)(0,col++) << COLHDR(symbol(DELTA) << _T("f") << Sub(_T("pSIDL")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   }


   (*table)(0,col++) << COLHDR(symbol(DELTA) << _T("f") << Sub(_T("pT")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );

   table->m_pStrands = pStrands;
   table->m_NtMax = NtMax;
   table->m_bIgnoreElasticGain = bIgnoreElasticGain;
   
   return table;
}

void CFinalPrestressLossTable::AddRow(rptChapter* pChapter,IBroker* pBroker,const pgsPointOfInterest& poi,RowIndexType row,const LOSSDETAILS* pDetails,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   ColumnIndexType col = 1;

  // Typecast to our known type (eating own doggy food)
   boost::shared_ptr<const lrfdRefinedLosses> ptl = boost::dynamic_pointer_cast<const lrfdRefinedLosses>(pDetails->pLosses);
   if (!ptl)
   {
      ATLASSERT(false); // made a bad cast? Bail...
      return;
   }

   (*this)(row,col++) << stress.SetValue( ptl->PermanentStrand_RelaxationLossesAtXfer() );
   (*this)(row,col++) << stress.SetValue( pDetails->pLosses->PermanentStrand_ElasticShorteningLosses() );
   
   if ( 0 < m_NtMax && m_pStrands->GetTemporaryStrandUsage() != pgsTypes::ttsPretensioned ) 
   {
      (*this)(row,col++) << stress.SetValue(pDetails->pLosses->GetDeltaFpp());
   }

   (*this)(row,col++) << stress.SetValue( ptl->ShrinkageLosses() );
   (*this)(row,col++) << stress.SetValue( ptl->CreepLosses() );
   (*this)(row,col++) << stress.SetValue( ptl->RelaxationLossesAfterXfer() );

   if ( 0 < m_NtMax ) 
   {
      (*this)(row,col++) << stress.SetValue( pDetails->pLosses->GetDeltaFptr() );
   }

   if ( !m_bIgnoreElasticGain )
   {
      (*this)(row,col++) << stress.SetValue( pDetails->pLosses->ElasticGainDueToDeckPlacement() );
      (*this)(row,col++) << stress.SetValue( pDetails->pLosses->ElasticGainDueToSIDL() );
   }


   Float64 dFpT = pDetails->pLosses->PermanentStrand_Final(); // this is the time-dependent loss only... it does not include elastic effects

   if ( m_bIgnoreElasticGain )
   {
      // we ignore elastic gains for LRFD versions before 2005. For LRFD 2004 and earlier, the total
      // loss included the elastic shortening. Add it in here so the results table matches earlier
      // versions of PGSuper
      dFpT += pDetails->pLosses->PermanentStrand_ElasticShorteningLosses();
   }

   (*this)(row,col++) << stress.SetValue( dFpT );
}
