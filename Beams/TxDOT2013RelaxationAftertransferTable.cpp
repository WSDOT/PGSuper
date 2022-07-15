///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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

// TxDOT2013RelaxationAfterTransferTable.cpp : Implementation of CTxDOT2013RelaxationAfterTransferTable
#include "stdafx.h"
#include "TxDOT2013RelaxationAfterTransferTable.h"
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <PsgLib\SpecLibraryEntry.h>
#include <PgsExt\GirderData.h>
#include <Reporting\ReportNotes.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CTxDOT2013RelaxationAfterTransferTable::CTxDOT2013RelaxationAfterTransferTable(ColumnIndexType NumColumns, IEAFDisplayUnits* pDisplayUnits) :
rptRcTable(NumColumns,0)
{
   DEFINE_UV_PROTOTYPE( stress,      pDisplayUnits->GetStressUnit(),          false );
}

CTxDOT2013RelaxationAfterTransferTable* CTxDOT2013RelaxationAfterTransferTable::PrepareTable(rptChapter* pChapter,IBroker* pBroker,const CSegmentKey& segmentKey,const LOSSDETAILS* pDetails,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   std::_tstring strImagePath(rptStyleManager::GetImagePath());

   INIT_UV_PROTOTYPE( rptStressUnitValue,  stress, pDisplayUnits->GetStressUnit(), true );

   rptParagraph* pParagraph = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pParagraph;
   *pParagraph << _T("Losses due to Relaxation") << rptNewLine;
   pParagraph = new rptParagraph;
   *pChapter << pParagraph;
   *pParagraph << symbol(DELTA) << Sub2(_T("f"),_T("fpr1"))<<_T(" = loss from transfer to deck placement. ")
               << symbol(DELTA) << Sub2(_T("f"),_T("fpr2"))<<_T(" = loss from deck placement to final. ")  << rptNewLine;
   *pParagraph << rptRcImage(strImagePath + _T("Delta_Fpr_TxDOT_2013.png")) << rptNewLine;

   const lrfdRefinedLossesTxDOT2013* pLosses = dynamic_cast<const lrfdRefinedLossesTxDOT2013*>(pDetails->pLosses.get());
   if (pLosses==nullptr)
   {
      ATLASSERT(false);
      return nullptr; // we have bigger problems than a memory leak at this point
   }

   *pParagraph << Sub2(_T("K"),_T("L"))<<_T(" = ")<<pLosses->GetKL() << rptNewLine;
   *pParagraph << Sub2(_T("f"),_T("py"))<<_T(" = ")<< stress.SetValue(pLosses->GetFpyPermanent()) << rptNewLine;

   if (pLosses->ElasticShortening().GetFcgpComputationMethod() == lrfdElasticShortening::fcgp07Fpu)
   {
      // fpt is the same along girder - we don't need a table
      *pParagraph << Sub2(_T("f"),_T("pt"))<<_T(" = 0.7 ")<<Sub2(_T("f"),_T("pu"))<<_T(" = ")<< stress.SetValue(pLosses->Getfpt()) << rptNewLine << rptNewLine;

      if ( pLosses->GetPermanentStrandCoating() != WBFL::Materials::PsStrand::Coating::None )
      {
         *pParagraph << EPOXY_RELAXATION_NOTE << rptNewLine;
      }

      *pParagraph << symbol(DELTA) << Sub2(_T("f"),_T("fpr1"))<<_T(" = ")<< symbol(DELTA) << Sub2(_T("f"),_T("fpr2"))<<_T(" = ")<<stress.SetValue(pLosses->RelaxationLossBeforeDeckPlacement()) << rptNewLine;

      return nullptr;
   }
   else
   {
      // Create and configure the table
      ColumnIndexType numColumns = 4;

      CTxDOT2013RelaxationAfterTransferTable* table = new CTxDOT2013RelaxationAfterTransferTable( numColumns, pDisplayUnits );
      rptStyleManager::ConfigureTable(table);


      if ( pLosses->GetPermanentStrandCoating() != WBFL::Materials::PsStrand::Coating::None )
      {
         *pParagraph << EPOXY_RELAXATION_NOTE << rptNewLine;
      }

      *pParagraph << table << rptNewLine;

      (*table)(0,0) << COLHDR(_T("Location from")<<rptNewLine<<_T("Left Support"),rptLengthUnitTag,  pDisplayUnits->GetSpanLengthUnit() );
      (*table)(0,1) << COLHDR( _T("f") << Sub(_T("pt")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*table)(0,2) << COLHDR( symbol(DELTA) << _T("f") << Sub(_T("pR1")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*table)(0,3) << COLHDR( symbol(DELTA) << _T("f") << Sub(_T("pR2")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      
      return table;
   }
}

void CTxDOT2013RelaxationAfterTransferTable::AddRow(rptChapter* pChapter,IBroker* pBroker,const pgsPointOfInterest& poi,RowIndexType row,const LOSSDETAILS* pDetails,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   const lrfdRefinedLossesTxDOT2013* pLosses = dynamic_cast<const lrfdRefinedLossesTxDOT2013*>(pDetails->pLosses.get());

   RowIndexType rowOffset = GetNumberOfHeaderRows() - 1;

   (*this)(row+rowOffset,1) << stress.SetValue( pLosses->Getfpt() );
   (*this)(row+rowOffset,2) << stress.SetValue( pLosses->RelaxationLossBeforeDeckPlacement() );
   (*this)(row+rowOffset,3) << stress.SetValue( pLosses->RelaxationLossAfterDeckPlacement() );
}
