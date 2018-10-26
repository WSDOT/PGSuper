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

// TemporaryStrandRemovalTable.cpp : Implementation of CTemporaryStrandRemovalTable
#include "stdafx.h"
#include "TemporaryStrandRemovalTable.h"
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\Intervals.h>
#include <PsgLib\SpecLibraryEntry.h>
#include <PgsExt\StrandData.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CTemporaryStrandRemovalTable::CTemporaryStrandRemovalTable(ColumnIndexType NumColumns, IEAFDisplayUnits* pDisplayUnits) :
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

CTemporaryStrandRemovalTable* CTemporaryStrandRemovalTable::PrepareTable(rptChapter* pChapter,IBroker* pBroker,const CSegmentKey& segmentKey,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType tsInstallIntervalIdx = pIntervals->GetTemporaryStrandInstallationInterval(segmentKey);
   IntervalIndexType tsRemovalIntervalIdx = pIntervals->GetTemporaryStrandRemovalInterval(segmentKey);

   GET_IFACE2(pBroker,IMaterials,pMaterials);
   Float64 Ec  = pMaterials->GetSegmentEc(segmentKey,tsRemovalIntervalIdx);
   Float64 Ep  = pMaterials->GetStrandMaterial(segmentKey,pgsTypes::Temporary)->GetE();

   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);
   Float64 nEffectiveStrands;
   Float64 ept = pStrandGeom->GetEccentricity( tsInstallIntervalIdx, pgsPointOfInterest(segmentKey,0), pgsTypes::Temporary, &nEffectiveStrands);
   Float64 Apt = pStrandGeom->GetStrandArea(segmentKey,tsInstallIntervalIdx,pgsTypes::Temporary);

   GET_IFACE2(pBroker,ISegmentData,pSegmentData);
   const CStrandData* pStrands = pSegmentData->GetStrandData(segmentKey);

   std::_tstring strImagePath(rptStyleManager::GetImagePath());

   ///////////////////////////////////////////////////////////////////////////////////////
   // Change in stress due to removal of temporary strands
   rptParagraph* pParagraph = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pParagraph;
   *pParagraph << _T("Effect of temporary strand removal on permanent strands") << rptNewLine;

   pParagraph = new rptParagraph;
   *pChapter << pParagraph;

   GET_IFACE2(pBroker,ILossParameters,pLossParameters);
   pgsTypes::LossMethod loss_method = pLossParameters->GetLossMethod();

   bool bIgnoreInitialRelaxation = ( loss_method == pgsTypes::WSDOT_REFINED || loss_method == pgsTypes::WSDOT_LUMPSUM ) ? false : true;

   // Create and configure the table
   ColumnIndexType numColumns = 9;

   CTemporaryStrandRemovalTable* table = new CTemporaryStrandRemovalTable( numColumns, pDisplayUnits );
   rptStyleManager::ConfigureTable(table);


   *pParagraph << Sub2(_T("P"),_T("tr")) << _T(" = ") << Sub2(_T("A"),_T("t")) << _T("(") << Sub2(_T("f"),_T("pj")) << _T(" - ");

   if ( pStrands->GetTemporaryStrandUsage() != pgsTypes::ttsPretensioned )
   {
      *pParagraph << symbol(DELTA) << Sub2(_T("f"),_T("pF")) << _T(" - ");
      *pParagraph << symbol(DELTA) << Sub2(_T("f"),_T("pA")) << _T(" - ");
      *pParagraph << symbol(DELTA) << Sub2(_T("f"),_T("pt")) << _T(" - ");
   }

   *pParagraph << symbol(DELTA) << Sub2(_T("f"),_T("pH")) << _T(")") << rptNewLine;

   *pParagraph << rptRcImage(strImagePath + _T("Delta_Fptr.png")) << rptNewLine;

   table->mod_e.ShowUnitTag(true);
   table->ecc.ShowUnitTag(true);
   table->area.ShowUnitTag(true);

   *pParagraph << Sub2(_T("E"),_T("p")) << _T(" = ") << table->mod_e.SetValue(Ep) << rptNewLine;
   *pParagraph << Sub2(_T("E"),_T("c")) << _T(" = ") << table->mod_e.SetValue(Ec) << rptNewLine;
   *pParagraph << Sub2(_T("A"),_T("t")) << _T(" = ") << table->area.SetValue(Apt) << rptNewLine;
   *pParagraph << Sub2(_T("e"),_T("t")) << _T(" = ") << table->ecc.SetValue(ept)  << rptNewLine;

   table->mod_e.ShowUnitTag(false);
   table->ecc.ShowUnitTag(false);
   table->area.ShowUnitTag(false);

   *pParagraph << table << rptNewLine;

   ColumnIndexType col = 0;
   (*table)(0,col++) << COLHDR(_T("Location from")<<rptNewLine<<_T("Left Support"),rptLengthUnitTag,  pDisplayUnits->GetSpanLengthUnit() );
   (*table)(0,col++) << COLHDR(RPT_STRESS(_T("pj")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pH")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,col++) << COLHDR(Sub2(_T("P"),_T("tr")),rptForceUnitTag,pDisplayUnits->GetGeneralForceUnit());
   (*table)(0,col++) << COLHDR(Sub2(_T("A"),_T("g")), rptAreaUnitTag, pDisplayUnits->GetAreaUnit());
   (*table)(0,col++) << COLHDR(Sub2(_T("I"),_T("g")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
   (*table)(0,col++) << COLHDR(Sub2(_T("e"),_T("p")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
   (*table)(0,col++) << COLHDR(RPT_STRESS(_T("ptr")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("ptr")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );

   return table;
}

void CTemporaryStrandRemovalTable::AddRow(rptChapter* pChapter,IBroker* pBroker,const pgsPointOfInterest& poi,RowIndexType row,const LOSSDETAILS* pDetails,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
//   (*this)(row,0) << spanloc.SetValue(poi,end_size);
   ColumnIndexType col = 1;
   (*this)(row,col++) << stress.SetValue(pDetails->pLosses->GetFpjTemporary());
   (*this)(row,col++) << stress.SetValue(pDetails->pLosses->TemporaryStrand_AtShipping());
   (*this)(row,col++) << force.SetValue(pDetails->pLosses->GetPtr());
   (*this)(row,col++) << area.SetValue(pDetails->pLosses->GetAg());
   (*this)(row,col++) << mom_inertia.SetValue(pDetails->pLosses->GetIg());
   (*this)(row,col++) << ecc.SetValue(pDetails->pLosses->GetEccPermanentFinal());
   (*this)(row,col++) << stress.SetValue( pDetails->pLosses->GetFptr() );
   (*this)(row,col++) << stress.SetValue( pDetails->pLosses->GetDeltaFptr() );
}
