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

// ElasticGainDueToSIDLTable.cpp : Implementation of CElasticGainDueToSIDLTable
#include "stdafx.h"
#include "ElasticGainDueToSIDLTable.h"
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\Intervals.h>
#include <PsgLib\SpecLibraryEntry.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CElasticGainDueToSIDLTable::CElasticGainDueToSIDLTable(ColumnIndexType NumColumns, IEAFDisplayUnits* pDisplayUnits) :
rptRcTable(NumColumns,0)
{
   DEFINE_UV_PROTOTYPE( spanloc,     pDisplayUnits->GetSpanLengthUnit(),      false );
   DEFINE_UV_PROTOTYPE( gdrloc,      pDisplayUnits->GetSpanLengthUnit(),      false );
   DEFINE_UV_PROTOTYPE( cg,          pDisplayUnits->GetComponentDimUnit(),    false );
   DEFINE_UV_PROTOTYPE( mod_e,       pDisplayUnits->GetModEUnit(),            false );
   DEFINE_UV_PROTOTYPE( force,       pDisplayUnits->GetGeneralForceUnit(),    false );
   DEFINE_UV_PROTOTYPE( area,        pDisplayUnits->GetAreaUnit(),            false );
   DEFINE_UV_PROTOTYPE( mom_inertia, pDisplayUnits->GetMomentOfInertiaUnit(), false );
   DEFINE_UV_PROTOTYPE( ecc,         pDisplayUnits->GetComponentDimUnit(),    false );
   DEFINE_UV_PROTOTYPE( moment,      pDisplayUnits->GetMomentUnit(),          false );
   DEFINE_UV_PROTOTYPE( stress,      pDisplayUnits->GetStressUnit(),          false );
}

CElasticGainDueToSIDLTable* CElasticGainDueToSIDLTable::PrepareTable(rptChapter* pChapter,IBroker* pBroker,const CSegmentKey& segmentKey,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   // Create and configure the table
   GET_IFACE2(pBroker,IUserDefinedLoads,pUDL);
   bool bHasUserLoads = pUDL->DoUserLoadsExist(segmentKey);

   GET_IFACE2(pBroker,IBridge,pBridge);
   bool bHasOverlay = (pBridge->IsFutureOverlay() == true ? false : true); // only include overlay if it is NOT a future overlay

   GET_IFACE2(pBroker,IProductLoads,pLoad);
   bool bHasSidewalk = pLoad->HasSidewalkLoad(segmentKey);

   ColumnIndexType numColumns = 9;

   if ( bHasUserLoads )
      numColumns += 2;

   if ( bHasSidewalk )
      numColumns++;

   if ( bHasOverlay )
      numColumns++;

   CElasticGainDueToSIDLTable* table = new CElasticGainDueToSIDLTable( numColumns, pDisplayUnits);
   pgsReportStyleHolder::ConfigureTable(table);

   table->m_bHasUserLoads = bHasUserLoads;
   table->m_bHasSidewalk  = bHasSidewalk;
   table->m_bHasOverlay   = bHasOverlay;

   table->scalar.SetFormat(sysNumericFormatTool::Fixed);
   table->scalar.SetWidth(5);
   table->scalar.SetPrecision(2);

   std::_tstring strImagePath(pgsReportStyleHolder::GetImagePath());

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType railingSystemIntervalIdx = pIntervals->GetInstallRailingSystemInterval();

   GET_IFACE2(pBroker,ISectionProperties,pSectProp);
   pgsTypes::SectionPropertyMode spMode = pSectProp->GetSectionPropertiesMode();

   GET_IFACE2(pBroker,IMaterials,pMaterials);
   Float64 Ec = pMaterials->GetSegmentEc(segmentKey,railingSystemIntervalIdx);
   Float64 Ep = pMaterials->GetStrandMaterial(segmentKey,pgsTypes::Permanent)->GetE();

   rptParagraph* pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;
   *pParagraph << _T("Elastic Gain Due to Superimposed Dead Load [5.9.5.2.3a]") << rptNewLine;

   pParagraph = new rptParagraph;
   *pChapter << pParagraph;

   *pParagraph << _T("Change in strand stress due to loads applied to the composite girder") << rptNewLine;
   *pParagraph << rptNewLine;
   *pParagraph << rptRcImage(strImagePath + _T("Msidl.png"));
   if ( bHasSidewalk )
      *pParagraph << rptRcImage(strImagePath + _T("Msw.png"));
   if ( bHasOverlay )
      *pParagraph << rptRcImage(strImagePath + _T("Mo.png"));
   if ( bHasUserLoads )
      *pParagraph << rptRcImage(strImagePath + _T("Muser.png"));
   *pParagraph << rptNewLine;

   if ( spMode == pgsTypes::spmGross )
      *pParagraph << rptRcImage(strImagePath + _T("DeltaFcd2_Gross.png")) << rptNewLine;
   else
      *pParagraph << rptRcImage(strImagePath + _T("DeltaFcd2_Transformed.png")) << rptNewLine;

   *pParagraph << rptRcImage(strImagePath + _T("ElasticGain2.png")) << rptNewLine;

   table->mod_e.ShowUnitTag(true);
   *pParagraph << Sub2(_T("E"),_T("p")) << _T(" = ") << table->mod_e.SetValue( Ep ) << rptNewLine;
   *pParagraph << Sub2(_T("E"),_T("c")) << _T(" = ") << table->mod_e.SetValue( Ec ) << rptNewLine;
   table->mod_e.ShowUnitTag(false);

   GET_IFACE2(pBroker,ISpecification,pSpec);
   GET_IFACE2(pBroker,ILibrary,pLibrary);
   const SpecLibraryEntry* pSpecEntry = pLibrary->GetSpecEntry(pSpec->GetSpecification().c_str());

   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   table->m_BAT = (analysisType == pgsTypes::Simple     ? pgsTypes::SimpleSpan : 
                   analysisType == pgsTypes::Continuous ? pgsTypes::ContinuousSpan : pgsTypes::MaxSimpleContinuousEnvelope);


   *pParagraph << rptNewLine;
   *pParagraph << _T("Railing System: ") << Sub2(_T("K"),_T("r")) << _T(" = ") << table->scalar.SetValue(pSpecEntry->GetRailingSystemElasticGain())      << rptNewLine;

   if ( bHasOverlay )
   {
      *pParagraph << _T("Overlay: " )    << Sub2(_T("K"),_T("o")) << _T(" = ") << table->scalar.SetValue(pSpecEntry->GetOverlayElasticGain())   << rptNewLine;
   }

   if ( bHasUserLoads )
   {
      *pParagraph << _T("User DC: ") << Sub2(_T("K"),_T("dc")) << _T(" = ") << table->scalar.SetValue(pSpecEntry->GetUserLoadAfterDeckDCElasticGain()) << rptNewLine;
      *pParagraph << _T("User DW: ") << Sub2(_T("K"),_T("dw")) << _T(" = ") << table->scalar.SetValue(pSpecEntry->GetUserLoadAfterDeckDWElasticGain()) << rptNewLine;
   }

   *pParagraph << table << rptNewLine;

   ColumnIndexType col = 0;
   (*table)(0,col++) << COLHDR(_T("Location from")<<rptNewLine<<_T("Left Support"),rptLengthUnitTag,  pDisplayUnits->GetSpanLengthUnit() );
   (*table)(0,col++) << COLHDR(Sub2(_T("M"),_T("Barrer")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   if ( bHasSidewalk )
   {
      (*table)(0,col++) << COLHDR(Sub2(_T("M"),_T("Sidewalk")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   }
   if ( bHasOverlay )
   {
      (*table)(0,col++) << COLHDR(Sub2(_T("M"),_T("Overlay")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   }
   if ( bHasUserLoads )
   {
      (*table)(0,col++) << COLHDR(Sub2(_T("M"),_T("UserDC")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
      (*table)(0,col++) << COLHDR(Sub2(_T("M"),_T("UserDW")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   }
   (*table)(0,col++) << COLHDR(Sub2(_T("M"),_T("sidl")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   if ( spMode == pgsTypes::spmGross )
   {
      (*table)(0,col++) << COLHDR(Sub2(_T("e"),_T("p")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
      (*table)(0,col++) << COLHDR(Sub2(_T("I"),_T("c")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit() );
      (*table)(0,col++) << COLHDR(Sub2(_T("Y"),_T("bc")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
      (*table)(0,col++) << COLHDR(Sub2(_T("Y"),_T("bg")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   }
   else
   {
      (*table)(0,col++) << COLHDR(Sub2(_T("e"),_T("pt")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
      (*table)(0,col++) << COLHDR(Sub2(_T("I"),_T("ct")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit() );
      (*table)(0,col++) << COLHDR(Sub2(_T("Y"),_T("bct")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
      (*table)(0,col++) << COLHDR(Sub2(_T("Y"),_T("bgt")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   }
   (*table)(0,col++) << COLHDR(symbol(DELTA) << italic(ON) << Sub2(_T("f''"),_T("cd")) << italic(OFF), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pSIDL")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   
   return table;
}

void CElasticGainDueToSIDLTable::AddRow(rptChapter* pChapter,IBroker* pBroker,const pgsPointOfInterest& poi,RowIndexType row,const LOSSDETAILS* pDetails,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   GET_IFACE2(pBroker,IProductForces,pProdForces);
   ColumnIndexType col = 1;

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType railingSystemIntervalIdx = pIntervals->GetInstallRailingSystemInterval();
   IntervalIndexType overlayIntervalIdx       = pIntervals->GetOverlayInterval();
   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval();

   if ( m_bHasSidewalk )
   {
      (*this)(row,col++) << moment.SetValue(pProdForces->GetMoment( railingSystemIntervalIdx, pftSidewalk, poi, m_BAT ));
   }

   (*this)(row,col++) << moment.SetValue(pProdForces->GetMoment( railingSystemIntervalIdx, pftTrafficBarrier, poi, m_BAT ));

   if ( m_bHasOverlay )
   {
      (*this)(row,col++) << moment.SetValue(pProdForces->GetMoment( overlayIntervalIdx, pftOverlay, poi, m_BAT ));
   }

   if ( m_bHasUserLoads )
   {
      (*this)(row,col++) << moment.SetValue( pProdForces->GetMoment( compositeDeckIntervalIdx, pftUserDC,      poi, m_BAT ) );
      (*this)(row,col++) << moment.SetValue( pProdForces->GetMoment( compositeDeckIntervalIdx, pftUserDW,      poi, m_BAT ) );
   }

   (*this)(row,col++) << moment.SetValue( pDetails->pLosses->GetSidlMoment() );
   (*this)(row,col++) << ecc.SetValue( pDetails->pLosses->GetEccPermanentFinal() );
   (*this)(row,col++) << mom_inertia.SetValue( pDetails->pLosses->GetIc() );
   (*this)(row,col++) << cg.SetValue( pDetails->pLosses->GetYbc() );
   (*this)(row,col++) << cg.SetValue( pDetails->pLosses->GetYbg() );
   (*this)(row,col++) << stress.SetValue( pDetails->pLosses->GetDeltaFcd2() );
   (*this)(row,col++) << stress.SetValue( pDetails->pLosses->ElasticGainDueToSIDL() );
}
