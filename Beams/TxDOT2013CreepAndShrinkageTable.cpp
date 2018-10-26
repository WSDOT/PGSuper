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

// TxDOT2013CreepAndShrinkageTable.cpp : Implementation of CTxDOT2013CreepAndShrinkageTable
#include "stdafx.h"
#include "TxDOT2013CreepAndShrinkageTable.h"
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <PsgLib\SpecLibraryEntry.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CTxDOT2013CreepAndShrinkageTable::CTxDOT2013CreepAndShrinkageTable(ColumnIndexType NumColumns, IEAFDisplayUnits* pDisplayUnits) :
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

CTxDOT2013CreepAndShrinkageTable* CTxDOT2013CreepAndShrinkageTable::PrepareTable(rptChapter* pChapter,IBroker* pBroker,const CSegmentKey& segmentKey,
                                                                                 const LOSSDETAILS* pDetails, IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   // Create and configure the table
   ColumnIndexType numColumns = 5;

   CTxDOT2013CreepAndShrinkageTable* table = new CTxDOT2013CreepAndShrinkageTable( numColumns, pDisplayUnits );
   pgsReportStyleHolder::ConfigureTable(table);

   std::_tstring strImagePath(pgsReportStyleHolder::GetImagePath());

   rptParagraph* pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;
   *pParagraph << _T("Losses Due to Creep and Shrinkage [TxDOT Research Report 0-6374-2]") << rptNewLine;

   pParagraph = new rptParagraph;
   *pChapter << pParagraph;

   *pParagraph << rptRcImage(strImagePath + _T("Delta_FpSR_TxDOT_2013.png")) << rptNewLine;
   *pParagraph << rptRcImage(strImagePath + _T("Delta_FpCR_TxDOT_2013.png")) << rptNewLine;

   INIT_UV_PROTOTYPE( rptStressUnitValue,  stress, pDisplayUnits->GetStressUnit(), true );

   GET_IFACE2(pBroker,IEnvironment,pEnv);
   *pParagraph << _T("H = ") << pEnv->GetRelHumidity() << _T("%") << rptNewLine;
   *pParagraph << RPT_FCI << stress.SetValue( pDetails->pLosses->GetFci() ) << rptNewLine;
   *pParagraph << RPT_ECI << stress.SetValue( pDetails->pLosses->GetEci() ) << rptNewLine;
   *pParagraph << RPT_EP << stress.SetValue( pDetails->pLosses->GetEp() )   << rptNewLine;

   *pParagraph << table << rptNewLine;

   (*table)(0,0) << COLHDR(_T("Location from")<<rptNewLine<<_T("Left Support"),rptLengthUnitTag,  pDisplayUnits->GetSpanLengthUnit() );
   (*table)(0,1) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pSR")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,2) << COLHDR(RPT_STRESS(_T("cgp")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,3) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("cdp")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,4) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pCR")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   
   return table;
}

void CTxDOT2013CreepAndShrinkageTable::AddRow(rptChapter* pChapter,IBroker* pBroker,const pgsPointOfInterest& poi,RowIndexType row,const LOSSDETAILS* pDetails,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   // Typecast to our known type (eating own doggy food)
   boost::shared_ptr<const lrfdRefinedLossesTxDOT2013> ptl = boost::dynamic_pointer_cast<const lrfdRefinedLossesTxDOT2013>(pDetails->pLosses);
   if (!ptl)
   {
      ATLASSERT(0); // made a bad cast? Bail...
      return;
   }

   (*this)(row,1) << stress.SetValue( ptl->ShrinkageLosses() );
   (*this)(row,2) << stress.SetValue( pDetails->pLosses->ElasticShortening().PermanentStrand_Fcgp() );
   (*this)(row,3) << stress.SetValue( -pDetails->pLosses->GetDeltaFcd1() );
   (*this)(row,4) << stress.SetValue( ptl->CreepLosses() );
}