///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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

// PostTensionInteractionTable.cpp : Implementation of CPostTensionInteractionTable
#include "stdafx.h"
#include "PostTensionInteractionTable.h"
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

CPostTensionInteractionTable::CPostTensionInteractionTable(ColumnIndexType NumColumns, IEAFDisplayUnits* pDisplayUnits) :
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

CPostTensionInteractionTable* CPostTensionInteractionTable::PrepareTable(rptChapter* pChapter,IBroker* pBroker,const CSegmentKey& segmentKey,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   // Create and configure table
   ColumnIndexType numColumns = 9;
   CPostTensionInteractionTable* table = new CPostTensionInteractionTable( numColumns, pDisplayUnits );
   pgsReportStyleHolder::ConfigureTable(table);

   std::_tstring strImagePath(pgsReportStyleHolder::GetImagePath());
   
   GET_IFACE2(pBroker,ISegmentData,pSegmentData);
   const CStrandData* pStrands = pSegmentData->GetStrandData(segmentKey);
   pgsTypes::TTSUsage tempStrandUsage = pStrands->GetTemporaryStrandUsage();

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType tsInstallIntervalIdx = pIntervals->GetTemporaryStrandInstallationInterval(segmentKey);

   // gather some data
   GET_IFACE2(pBroker,IMaterials,pMaterials);
   Float64 Eci = pMaterials->GetSegmentEc(segmentKey,tsInstallIntervalIdx);

   GET_IFACE2(pBroker,IMaterials,pMaterial);
   Float64 Ep  = pMaterial->GetStrandMaterial(segmentKey,pgsTypes::Temporary)->GetE();

   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);
   Float64 nEffectiveStrands;
   Float64 ept = pStrandGeom->GetEccentricity( tsInstallIntervalIdx, pgsPointOfInterest(segmentKey,0), pgsTypes::Temporary, &nEffectiveStrands);
   Float64 Apt = pStrandGeom->GetStrandArea(segmentKey,tsInstallIntervalIdx,pgsTypes::Temporary);
   StrandIndexType Npt = pStrandGeom->GetStrandCount(segmentKey,pgsTypes::Temporary);

   rptParagraph* pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;
   *pParagraph << _T("Effect of strand jacking on previously stressed strands") << rptNewLine;

   pParagraph = new rptParagraph;
   *pChapter << pParagraph;

   if ( tempStrandUsage == pgsTypes::ttsPTBeforeShipping )
      *pParagraph << rptRcImage(strImagePath + _T("Delta_Fpt_BeforeShipping.png")) << rptNewLine;
   else
      *pParagraph << rptRcImage(strImagePath + _T("Delta_Fpt.png")) << rptNewLine;

   table->mod_e.ShowUnitTag(true);
   table->area.ShowUnitTag(true);
   table->ecc.ShowUnitTag(true);
   
   *pParagraph << Sub2(_T("E"),_T("p")) << _T(" = ") << table->mod_e.SetValue(Ep) << rptNewLine;

   if ( tempStrandUsage == pgsTypes::ttsPTBeforeShipping )
      *pParagraph << Sub2(_T("E"),_T("c")) << _T(" = ") << table->mod_e.SetValue(Eci) << rptNewLine;
   else
      *pParagraph << Sub2(_T("E"),_T("ci")) << _T(" = ") << table->mod_e.SetValue(Eci) << rptNewLine;

   *pParagraph << Sub2(_T("e"),_T("pt")) << _T(" = ") << table->ecc.SetValue(ept) << rptNewLine;
   *pParagraph << Sub2(_T("A"),_T("pt")) << _T(" = ") << table->area.SetValue(Apt) << rptNewLine;

   table->mod_e.ShowUnitTag(false);
   table->area.ShowUnitTag(false);
   table->ecc.ShowUnitTag(false);

   *pParagraph << table << rptNewLine;
   (*table)(0,0) << COLHDR(_T("Location from")<<rptNewLine<<_T("End of Girder"),rptLengthUnitTag,  pDisplayUnits->GetSpanLengthUnit() );
   (*table)(0,1) << COLHDR(_T("Location from")<<rptNewLine<<_T("Left Support"),rptLengthUnitTag,  pDisplayUnits->GetSpanLengthUnit() );
   (*table)(0,2) << COLHDR(_T("x"),rptLengthUnitTag,pDisplayUnits->GetSpanLengthUnit());
   (*table)(0,3) << COLHDR(RPT_STRESS(_T("pt max")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,4) << COLHDR(Sub2(_T("P"),_T("pt")), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit() );
   (*table)(0,5) << COLHDR(Sub2(_T("A"),_T("g")), rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );
   (*table)(0,6) << COLHDR(Sub2(_T("I"),_T("g")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
   (*table)(0,7) << COLHDR(RPT_STRESS(_T("cgpt")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,8) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pt")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );

   return table;
}

void CPostTensionInteractionTable::AddRow(rptChapter* pChapter,IBroker* pBroker,const pgsPointOfInterest& poi,RowIndexType row,const LOSSDETAILS* pDetails,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   (*this)(row,2) << offset.SetValue( pDetails->pLosses->GetLocation() );
   (*this)(row,3) << stress.SetValue( pDetails->pLosses->GetFptMax() );
   (*this)(row,4) << force.SetValue( pDetails->pLosses->GetPptMax() );
   (*this)(row,5) << area.SetValue( pDetails->pLosses->GetAg() );
   (*this)(row,6) << mom_inertia.SetValue( pDetails->pLosses->GetIg() );
   (*this)(row,7) << stress.SetValue( pDetails->pLosses->GetFcgpt() );
   (*this)(row,8) << stress.SetValue( pDetails->pLosses->GetDeltaFpt() );
}
