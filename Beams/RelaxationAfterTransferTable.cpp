///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2021  Washington State Department of Transportation
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

// RelaxationAfterTransferTable.cpp : Implementation of CRelaxationAfterTransferTable
#include "stdafx.h"
#include "RelaxationAfterTransferTable.h"
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <PsgLib\SpecLibraryEntry.h>
#include <Reporting\ReportNotes.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CRelaxationAfterTransferTable::CRelaxationAfterTransferTable(ColumnIndexType NumColumns, IEAFDisplayUnits* pDisplayUnits) :
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

CRelaxationAfterTransferTable* CRelaxationAfterTransferTable::PrepareTable(rptChapter* pChapter,IBroker* pBroker,const CSegmentKey& segmentKey,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   // Create and configure the table
   ColumnIndexType numColumns = 5;

   CRelaxationAfterTransferTable* table = new CRelaxationAfterTransferTable( numColumns, pDisplayUnits );
   rptStyleManager::ConfigureTable(table);

   std::_tstring strImagePath(rptStyleManager::GetImagePath());


   GET_IFACE2(pBroker, ISegmentData, pSegmentData );
   // strand is used for general type and coating, not size, so it's ok to use either Straight or Harped since they are the same
   const matPsStrand* pStrand = pSegmentData->GetStrandMaterial(segmentKey,pgsTypes::Straight);
   ATLASSERT(pStrand);
   
   rptParagraph* pParagraph = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pParagraph;
   *pParagraph << _T("Losses due to Relaxation After Transfer") << rptNewLine;
   if ( pStrand->GetType() == matPsStrand::LowRelaxation )
   {
      if ( IS_SI_UNITS(pDisplayUnits) )
      {
         *pParagraph << rptRcImage(strImagePath + _T("Delta_FpR2_LR_SI.png")) << rptNewLine;
      }
      else
      {
         *pParagraph << rptRcImage(strImagePath + _T("Delta_FpR2_LR_US.png")) << rptNewLine;
      }
   }
   else
   {
      if ( IS_SI_UNITS(pDisplayUnits) )
      {
         *pParagraph << rptRcImage(strImagePath + _T("Delta_FpR2_SR_SI.png")) << rptNewLine;
      }
      else
      {
         *pParagraph << rptRcImage(strImagePath + _T("Delta_FpR2_SR_US.png")) << rptNewLine;
      }
   }

   if ( pStrand->GetCoating() != matPsStrand::None )
   {
      *pParagraph << EPOXY_RELAXATION_NOTE << rptNewLine;
   }

   *pParagraph << table << rptNewLine;

   ColumnIndexType col = 0;
   (*table)(0,col++) << COLHDR(_T("Location from")<<rptNewLine<<_T("Left Support"),rptLengthUnitTag,  pDisplayUnits->GetSpanLengthUnit() );
   (*table)(0,col++) << COLHDR( symbol(DELTA) << _T("f") << Sub(_T("pES")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,col++) << COLHDR( symbol(DELTA) << _T("f") << Sub(_T("pSR")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,col++) << COLHDR( symbol(DELTA) << _T("f") << Sub(_T("pCR")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,col++) << COLHDR( symbol(DELTA) << _T("f") << Sub(_T("pR2")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   
   return table;
}

void CRelaxationAfterTransferTable::AddRow(rptChapter* pChapter,IBroker* pBroker,const pgsPointOfInterest& poi,RowIndexType row,const LOSSDETAILS* pDetails,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
  // Typecast to our known type (eating own doggy food)
   std::shared_ptr<const lrfdRefinedLosses> ptl = std::dynamic_pointer_cast<const lrfdRefinedLosses>(pDetails->pLosses);
   if (!ptl)
   {
      ATLASSERT(false); // made a bad cast? Bail...
      return;
   }

   ColumnIndexType col = 1;
   RowIndexType rowOffset = GetNumberOfHeaderRows() - 1;

   (*this)(row+rowOffset,col++) << stress.SetValue( pDetails->pLosses->PermanentStrand_ElasticShorteningLosses() );
   (*this)(row+rowOffset,col++) << stress.SetValue( ptl->ShrinkageLosses() );
   (*this)(row+rowOffset,col++) << stress.SetValue( ptl->CreepLosses() );
   (*this)(row+rowOffset,col++) << stress.SetValue( ptl->RelaxationLossesAfterXfer() );
}
