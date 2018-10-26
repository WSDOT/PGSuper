///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2017  Washington State Department of Transportation
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

// PostTensionTimeDependentLossesAtShippingTable.cpp : Implementation of CPostTensionTimeDependentLossesAtShippingTable
#include "stdafx.h"
#include "PostTensionTimeDependentLossesAtShippingTable.h"
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <PsgLib\SpecLibraryEntry.h>
#include <PgsExt\StrandData.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CPostTensionTimeDependentLossesAtShippingTable::CPostTensionTimeDependentLossesAtShippingTable(ColumnIndexType NumColumns, IEAFDisplayUnits* pDisplayUnits) :
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
}

CPostTensionTimeDependentLossesAtShippingTable* CPostTensionTimeDependentLossesAtShippingTable::PrepareTable(rptChapter* pChapter,IBroker* pBroker,const CSegmentKey& segmentKey,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   std::_tstring strImagePath(rptStyleManager::GetImagePath());

   CPostTensionTimeDependentLossesAtShippingTable* table = NULL;

   GET_IFACE2(pBroker,ISegmentData,pSegmentData);
   const CStrandData* pStrands = pSegmentData->GetStrandData(segmentKey);

   if ( pStrands->GetTemporaryStrandUsage() == pgsTypes::ttsPTBeforeLifting ||
        pStrands->GetTemporaryStrandUsage() == pgsTypes::ttsPTAfterLifting 
      ) 
   {
      ColumnIndexType numColumns = 7;
      table = new CPostTensionTimeDependentLossesAtShippingTable( numColumns, pDisplayUnits );
      rptStyleManager::ConfigureTable(table);

      rptParagraph* pParagraph = new rptParagraph(rptStyleManager::GetHeadingStyle());
      *pChapter << pParagraph;

      *pParagraph << _T("Post-Tensioned Temporary Strands") << rptNewLine;
      
      pParagraph = new rptParagraph;
      *pChapter << pParagraph;

      *pParagraph << rptRcImage(strImagePath + _T("PTLossAtHauling.png")) << rptNewLine;

      *pParagraph << table << rptNewLine;

      (*table)(0,0) << COLHDR(_T("Location from")<<rptNewLine<<_T("End of Girder"),rptLengthUnitTag,  pDisplayUnits->GetSpanLengthUnit() );
      (*table)(0,1) << COLHDR(_T("Location from")<<rptNewLine<<_T("Left Support"),rptLengthUnitTag,  pDisplayUnits->GetSpanLengthUnit() );
      (*table)(0,2) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pF")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*table)(0,3) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pA")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*table)(0,4) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pt avg")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*table)(0,5) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pLTH")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*table)(0,6) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("ptH")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   }

   return table;
}

void CPostTensionTimeDependentLossesAtShippingTable::AddRow(rptChapter* pChapter,IBroker* pBroker,const pgsPointOfInterest& poi,RowIndexType row,const LOSSDETAILS* pDetails,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   (*this)(row,2) << stress.SetValue(pDetails->pLosses->FrictionLoss());
   (*this)(row,3) << stress.SetValue(pDetails->pLosses->AnchorSetLoss());
   (*this)(row,4) << stress.SetValue(pDetails->pLosses->GetDeltaFptAvg());
   (*this)(row,5) << stress.SetValue(pDetails->pLosses->TemporaryStrand_TimeDependentLossesAtShipping() );
   (*this)(row,6) << stress.SetValue(pDetails->pLosses->TemporaryStrand_AtShipping());
}
