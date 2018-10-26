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

// ElasticGainDueToDeckPlacementTable.cpp : Implementation of CElasticGainDueToDeckPlacementTable
#include "stdafx.h"
#include "ElasticGainDueToDeckPlacementTable.h"
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <PsgLib\SpecLibraryEntry.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CElasticGainDueToDeckPlacementTable::CElasticGainDueToDeckPlacementTable(ColumnIndexType NumColumns, IEAFDisplayUnits* pDisplayUnits) :
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

CElasticGainDueToDeckPlacementTable* CElasticGainDueToDeckPlacementTable::PrepareTable(rptChapter* pChapter,IBroker* pBroker,SpanIndexType span,GirderIndexType gdr,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   // Create and configure the table
   GET_IFACE2(pBroker,IUserDefinedLoads,pUDL);
   bool bHasUserLoads = pUDL->DoUserLoadsExist(span,gdr);

   GET_IFACE2(pBroker,IBridge,pBridge);
   bool bHasDeckPanel = pBridge->GetDeckType() == pgsTypes::sdtCompositeSIP ? true : false;

   ColumnIndexType numColumns = 9;

   if ( bHasUserLoads )
      numColumns += 2;

   if ( bHasDeckPanel )
      numColumns++;

   CElasticGainDueToDeckPlacementTable* table = new CElasticGainDueToDeckPlacementTable( numColumns, pDisplayUnits);
   pgsReportStyleHolder::ConfigureTable(table);

   table->m_bHasDeckPanel = bHasDeckPanel;
   table->m_bHasUserLoads = bHasUserLoads;
   table->scalar.SetFormat(sysNumericFormatTool::Fixed);
   table->scalar.SetWidth(5);
   table->scalar.SetPrecision(2);

   std::_tstring strImagePath(pgsReportStyleHolder::GetImagePath());

   GET_IFACE2(pBroker,IBridgeMaterial,pMaterial);
   Float64 Ec = pMaterial->GetEcGdr(span,gdr);
   Float64 Ep = pMaterial->GetStrand(span,gdr,pgsTypes::Permanent)->GetE();

   rptParagraph* pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;
   *pParagraph << _T("Elastic Gain Due to Deck Placement [5.9.5.2.3a]") << rptNewLine;

   pParagraph = new rptParagraph;
   *pChapter << pParagraph;

   *pParagraph << _T("Change in strand stress due to loads applied to the non-composite girder") << rptNewLine;
   *pParagraph << rptNewLine;
   if ( bHasDeckPanel && bHasUserLoads )
   {
      *pParagraph << rptRcImage(strImagePath + _T("Madl4.png"))        << rptNewLine;
   }
   else if ( bHasDeckPanel && !bHasUserLoads )
   {
      *pParagraph << rptRcImage(strImagePath + _T("Madl3.png"))        << rptNewLine;
   }
   else if ( !bHasDeckPanel && bHasUserLoads )
   {
      *pParagraph << rptRcImage(strImagePath + _T("Madl2.png"))        << rptNewLine;
   }
   else
   {
      *pParagraph << rptRcImage(strImagePath + _T("Madl.png"))        << rptNewLine;
   }
   *pParagraph << Sub2(_T("M"),_T("Diaphragm")) << _T(" includes shear key dead load if applicable") << rptNewLine;
   *pParagraph << rptRcImage(strImagePath + _T("DeltaFcd.png"))    << rptNewLine;
   *pParagraph << rptRcImage(strImagePath + _T("ElasticGain.png")) << rptNewLine;

   table->mod_e.ShowUnitTag(true);
   *pParagraph << Sub2(_T("E"),_T("p")) << _T(" = ") << table->mod_e.SetValue( Ep ) << rptNewLine;
   *pParagraph << Sub2(_T("E"),_T("c")) << _T(" = ") << table->mod_e.SetValue( Ec ) << rptNewLine;
   table->mod_e.ShowUnitTag(false);

   GET_IFACE2(pBroker,ISpecification,pSpec);
   GET_IFACE2(pBroker,ILibrary,pLibrary);
   const SpecLibraryEntry* pSpecEntry = pLibrary->GetSpecEntry(pSpec->GetSpecification().c_str());

   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   table->m_BAT = (analysisType == pgsTypes::Simple     ? SimpleSpan : 
                   analysisType == pgsTypes::Continuous ? ContinuousSpan : MaxSimpleContinuousEnvelope);


   *pParagraph << rptNewLine;
   if ( bHasDeckPanel )
      *pParagraph << _T("Slab+Panel: ")       << Sub2(_T("K"),_T("s")) << _T(" = ") << table->scalar.SetValue(pSpecEntry->GetSlabElasticGain())      << rptNewLine;
   else
      *pParagraph << _T("Slab: ")       << Sub2(_T("K"),_T("s")) << _T(" = ") << table->scalar.SetValue(pSpecEntry->GetSlabElasticGain())      << rptNewLine;

   *pParagraph << _T("Haunch: " )    << Sub2(_T("K"),_T("h")) << _T(" = ") << table->scalar.SetValue(pSpecEntry->GetSlabPadElasticGain())   << rptNewLine;
   *pParagraph << _T("Diaphragms: ") << Sub2(_T("K"),_T("d")) << _T(" = ") << table->scalar.SetValue(pSpecEntry->GetDiaphragmElasticGain()) << rptNewLine;

   if ( bHasUserLoads )
   {
      *pParagraph << _T("User DC: ") << Sub2(_T("K"),_T("dc")) << _T(" = ") << table->scalar.SetValue(pSpecEntry->GetUserDCElasticGain(pgsTypes::BridgeSite1)) << rptNewLine;
      *pParagraph << _T("User DW: ") << Sub2(_T("K"),_T("dw")) << _T(" = ") << table->scalar.SetValue(pSpecEntry->GetUserDWElasticGain(pgsTypes::BridgeSite1)) << rptNewLine;
   }

   *pParagraph << table << rptNewLine;

   ColumnIndexType col = 0;
   (*table)(0,col++) << COLHDR(_T("Location from")<<rptNewLine<<_T("Left Support"),rptLengthUnitTag,  pDisplayUnits->GetSpanLengthUnit() );
   (*table)(0,col++) << COLHDR(Sub2(_T("M"),_T("Slab")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   if ( bHasDeckPanel )
   {
      (*table)(0,col++) << COLHDR(Sub2(_T("M"),_T("Panel")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   }
   (*table)(0,col++) << COLHDR(Sub2(_T("M"),_T("Haunch")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   (*table)(0,col++) << COLHDR(Sub2(_T("M"),_T("Diaphragm")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   if ( bHasUserLoads )
   {
      (*table)(0,col++) << COLHDR(Sub2(_T("M"),_T("User DC")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
      (*table)(0,col++) << COLHDR(Sub2(_T("M"),_T("User DW")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   }
   (*table)(0,col++) << COLHDR(Sub2(_T("M"),_T("adl")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   (*table)(0,col++) << COLHDR(Sub2(_T("e"),_T("p")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,col++) << COLHDR(Sub2(_T("I"),_T("g")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit() );
   (*table)(0,col++) << COLHDR(symbol(DELTA) << italic(ON) << Sub2(_T("f'"),_T("cd")) << italic(OFF), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pED")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   
   return table;
}

void CElasticGainDueToDeckPlacementTable::AddRow(rptChapter* pChapter,IBroker* pBroker,const pgsPointOfInterest& poi,RowIndexType row,LOSSDETAILS& details,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   GET_IFACE2(pBroker,IProductForces,pProdForces);
   ColumnIndexType col = 1;
   (*this)(row,col++) << moment.SetValue( pProdForces->GetMoment( pgsTypes::BridgeSite1, pftSlab,      poi, m_BAT ) );
   if ( m_bHasDeckPanel )
   {
      (*this)(row,col++) << moment.SetValue( pProdForces->GetMoment( pgsTypes::BridgeSite1, pftSlabPanel,      poi, m_BAT ) );
   }
   (*this)(row,col++) << moment.SetValue( pProdForces->GetMoment( pgsTypes::BridgeSite1, pftSlabPad,   poi, m_BAT ) );
   (*this)(row,col++) << moment.SetValue( pProdForces->GetMoment( pgsTypes::BridgeSite1, pftDiaphragm, poi, m_BAT ) + 
                                          pProdForces->GetMoment( pgsTypes::BridgeSite1, pftShearKey,  poi, m_BAT ));
   if ( m_bHasUserLoads )
   {
      (*this)(row,col++) << moment.SetValue( pProdForces->GetMoment( pgsTypes::BridgeSite1, pftUserDC,      poi, m_BAT ) );
      (*this)(row,col++) << moment.SetValue( pProdForces->GetMoment( pgsTypes::BridgeSite1, pftUserDW,      poi, m_BAT ) );
   }

   (*this)(row,col++) << moment.SetValue( details.pLosses->GetAddlGdrMoment() );
   (*this)(row,col++) << ecc.SetValue( details.pLosses->GetEccPermanent() );
   (*this)(row,col++) << mom_inertia.SetValue( details.pLosses->GetIg() );
   (*this)(row,col++) << stress.SetValue( details.pLosses->GetDeltaFcd1() );
   (*this)(row,col++) << stress.SetValue( details.pLosses->ElasticGainDueToDeckPlacement() );
}
