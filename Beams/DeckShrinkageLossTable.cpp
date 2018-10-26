///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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

// ElasticGainDueToDeckShrinkageTable.cpp : Implementation of CElasticGainDueToDeckShrinkageTable
#include "stdafx.h"
#include "DeckShrinkageLossTable.h"
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\AnalysisResults.h>
#include <PsgLib\SpecLibraryEntry.h>
#include <PgsExt\GirderData.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CElasticGainDueToDeckShrinkageTable::CElasticGainDueToDeckShrinkageTable(ColumnIndexType NumColumns, IEAFDisplayUnits* pDisplayUnits) :
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
   DEFINE_UV_PROTOTYPE( time,        pDisplayUnits->GetLongTimeUnit(),        false );

   scalar.SetFormat( sysNumericFormatTool::Automatic );
   scalar.SetWidth(6);
   scalar.SetPrecision(3);

   strain.SetFormat( sysNumericFormatTool::Automatic );
   strain.SetWidth(6);
   strain.SetPrecision(3);
}

CElasticGainDueToDeckShrinkageTable* CElasticGainDueToDeckShrinkageTable::PrepareTable(rptChapter* pChapter,IBroker* pBroker,SpanIndexType span,GirderIndexType gdr,LOSSDETAILS& details,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   GET_IFACE2(pBroker,ISpecification,pSpec);
   std::_tstring strSpecName = pSpec->GetSpecification();

   GET_IFACE2(pBroker,ILibrary,pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( strSpecName.c_str() );

   // Create and configure the table
   ColumnIndexType numColumns = 10;
   CElasticGainDueToDeckShrinkageTable* table = new CElasticGainDueToDeckShrinkageTable( numColumns, pDisplayUnits );
   pgsReportStyleHolder::ConfigureTable(table);

   std::_tstring strImagePath(pgsReportStyleHolder::GetImagePath());
   
   rptParagraph* pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;

   *pParagraph << _T("[5.9.5.4.3d] Shrinkage of Deck Concrete : ") << symbol(DELTA) << RPT_STRESS(_T("pSS")) << rptNewLine;

   pParagraph = new rptParagraph;
   *pChapter << pParagraph;

   *pParagraph << _T("Change in strand stress due to shrinkage of the deck concrete") << rptNewLine;
   *pParagraph << rptNewLine;

   pParagraph = new rptParagraph;
   *pChapter << pParagraph;

   *pParagraph << rptRcImage(strImagePath + _T("Delta_FpSS.png")) << rptNewLine;

   if ( pSpecEntry->GetSpecificationType() <= lrfdVersionMgr::ThirdEditionWith2005Interims )
   {
      if ( IS_SI_UNITS(pDisplayUnits) )
         *pParagraph << rptRcImage(strImagePath + _T("KvsEqn-SI.png")) << rptNewLine;
      else
         *pParagraph << rptRcImage(strImagePath + _T("KvsEqn-US.png")) << rptNewLine;
   }
   else if ( pSpecEntry->GetSpecificationType() == lrfdVersionMgr::ThirdEditionWith2006Interims )
   {
      if ( IS_SI_UNITS(pDisplayUnits) )
         *pParagraph << rptRcImage(strImagePath + _T("KvsEqn2006-SI.png")) << rptNewLine;
      else
         *pParagraph << rptRcImage(strImagePath + _T("KvsEqn2006-US.png")) << rptNewLine;
   }
   else
   {
      if ( IS_SI_UNITS(pDisplayUnits) )
         *pParagraph << rptRcImage(strImagePath + _T("KvsEqn2007-SI.png")) << rptNewLine;
      else
         *pParagraph << rptRcImage(strImagePath + _T("KvsEqn2007-US.png")) << rptNewLine;
   }

   *pParagraph << rptRcImage(strImagePath + _T("HumidityFactor.png")) << rptNewLine;

   if ( IS_SI_UNITS(pDisplayUnits) )
      *pParagraph << rptRcImage(strImagePath + _T("ConcreteFactors_Deck_SI.png")) << rptNewLine;
   else
      *pParagraph << rptRcImage(strImagePath + _T("ConcreteFactors_Deck_US.png")) << rptNewLine;

   *pParagraph << _T("Girder stresses due to slab shrinkage") << rptNewLine;
   *pParagraph << rptRcImage(strImagePath + _T("SlabShrinkageStress_Ftop.png")) << rptNewLine;
   *pParagraph << rptRcImage(strImagePath + _T("SlabShrinkageStress_Fbot.png")) << rptNewLine;

   rptRcTable* pParamTable = pgsReportStyleHolder::CreateDefaultTable(9,_T(""));
   *pParagraph << pParamTable << rptNewLine;

   (*pParamTable)(0,0) << COLHDR(_T("V/S") << rptNewLine << _T("deck"),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );

   if ( lrfdVersionMgr::FourthEdition2007 <= pSpecEntry->GetSpecificationType() )
     (*pParamTable)(0,1) << Sub2(_T("k"),_T("s")) << rptNewLine << _T("deck");
   else
     (*pParamTable)(0,1) << Sub2(_T("k"),_T("vs")) << rptNewLine << _T("deck");


   (*pParamTable)(0,2) << Sub2(_T("k"),_T("hs"));
   (*pParamTable)(0,3) << COLHDR(RPT_FC << rptNewLine << _T("deck"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*pParamTable)(0,4) << Sub2(_T("k"),_T("f"));
   (*pParamTable)(0,5) << Sub2(_T("k"),_T("td")) << rptNewLine << _T("t = ") << Sub2(_T("t"),_T("f")) << _T(" - ") << Sub2(_T("t"),_T("d"));
   (*pParamTable)(0,6) << Sub2(_T("K"),_T("1"));
   (*pParamTable)(0,7) << Sub2(_T("K"),_T("2"));
   (*pParamTable)(0,8) << Sub2(symbol(epsilon),_T("ddf")) << _T("x 1000");

   if ( IsZero(details.RefinedLosses2005.GetVolumeSlab()) || IsZero(details.RefinedLosses2005.GetSurfaceAreaSlab()) )
      (*pParamTable)(1,0) << table->ecc.SetValue(0.0);
   else
      (*pParamTable)(1,0) << table->ecc.SetValue(details.RefinedLosses2005.GetVolumeSlab()/details.RefinedLosses2005.GetSurfaceAreaSlab());

   (*pParamTable)(1,1) << table->scalar.SetValue(details.RefinedLosses2005.GetCreepDeck().GetKvs());
   (*pParamTable)(1,2) << table->scalar.SetValue(details.RefinedLosses2005.Getkhs());
   (*pParamTable)(1,3) << table->stress.SetValue(details.RefinedLosses2005.GetFcSlab() );
   (*pParamTable)(1,4) << table->scalar.SetValue(details.RefinedLosses2005.GetCreepDeck().GetKf());
   (*pParamTable)(1,5) << table->scalar.SetValue(details.RefinedLosses2005.GetCreepDeck().GetKtd());
   (*pParamTable)(1,6) << details.RefinedLosses2005.GetDeckK1Shrinkage();
   (*pParamTable)(1,7) << details.RefinedLosses2005.GetDeckK2Shrinkage();
   (*pParamTable)(1,8) << table->strain.SetValue(details.RefinedLosses2005.Get_eddf() * 1000);

   pParamTable = pgsReportStyleHolder::CreateDefaultTable(8,_T(""));
   *pParagraph << pParamTable << rptNewLine;
   (*pParamTable)(0,0) << COLHDR( Sub2(_T("E"),_T("p")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*pParamTable)(0,1) << COLHDR( Sub2(_T("E"),_T("c")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*pParamTable)(0,2) << COLHDR( Sub2(_T("E"),_T("cd")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*pParamTable)(0,3) << Sub2(_T("K"),_T("sh"));
   (*pParamTable)(0,4) << Sub2(_T("K"),_T("1"));
   (*pParamTable)(0,5) << Sub2(_T("K"),_T("2"));
   (*pParamTable)(0,6) << Sub2(symbol(psi),_T("b")) << _T("(") << Sub2(_T("t"),_T("f")) << _T(",") << Sub2(_T("t"),_T("d")) << _T(")");
   (*pParamTable)(0,7) << Sub2(symbol(psi),_T("bd")) << _T("(") << Sub2(_T("t"),_T("f")) << _T(",") << Sub2(_T("t"),_T("d")) << _T(")");

   (*pParamTable)(1,0) << table->mod_e.SetValue( details.RefinedLosses2005.GetEp() );
   (*pParamTable)(1,1) << table->mod_e.SetValue( details.RefinedLosses2005.GetEc() );
   (*pParamTable)(1,2) << table->mod_e.SetValue( details.RefinedLosses2005.GetEcd() );
   (*pParamTable)(1,3) << pSpecEntry->GetDeckShrinkageElasticGain();
   (*pParamTable)(1,4) << details.RefinedLosses2005.GetDeckK1Creep();
   (*pParamTable)(1,5) << details.RefinedLosses2005.GetDeckK2Creep();
   (*pParamTable)(1,6) << table->scalar.SetValue(details.RefinedLosses2005.GetCreepDeckToFinal().GetCreepCoefficient());
   (*pParamTable)(1,7) << table->scalar.SetValue(details.RefinedLosses2005.GetCreepDeck().GetCreepCoefficient());

   *pParagraph << table << rptNewLine;
   (*table)(0,0) << COLHDR(_T("Location from")<<rptNewLine<<_T("Left Support"),rptLengthUnitTag,  pDisplayUnits->GetSpanLengthUnit() );
   (*table)(0,1) << COLHDR( Sub2(_T("A"),_T("d")), rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );
   (*table)(0,2) << COLHDR( Sub2(_T("e"),_T("pc")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,3) << COLHDR( Sub2(_T("e"),_T("d")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,4) << COLHDR( Sub2(_T("A"),_T("c")), rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );
   (*table)(0,5) << COLHDR( Sub2(_T("I"),_T("c")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit() );
   (*table)(0,6) << COLHDR( symbol(DELTA) << RPT_STRESS(_T("cdf")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,7) << COLHDR( symbol(DELTA) << RPT_STRESS(_T("pSS")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,8) << COLHDR(RPT_FTOP << rptNewLine << _T("Girder"),rptStressUnitTag,pDisplayUnits->GetStressUnit());
   (*table)(0,9) << COLHDR(RPT_FBOT << rptNewLine << _T("Girder"),rptStressUnitTag,pDisplayUnits->GetStressUnit());

   table->m_Sign =  ( pSpecEntry->GetSpecificationType() < lrfdVersionMgr::FourthEdition2007 ) ? 1 : -1;

   return table;
}

void CElasticGainDueToDeckShrinkageTable::AddRow(rptChapter* pChapter,IBroker* pBroker,const pgsPointOfInterest& poi,RowIndexType row,LOSSDETAILS& details,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   GET_IFACE2(pBroker,IProductForces,pProductForces);
   Float64 fTop,fBot;
   pProductForces->GetDeckShrinkageStresses(poi,&fTop,&fBot);

   (*this)(row,1) << area.SetValue( details.RefinedLosses2005.GetAd() );
   (*this)(row,2) << ecc.SetValue( details.RefinedLosses2005.GetEccpc() );
   (*this)(row,3) << ecc.SetValue( m_Sign*details.RefinedLosses2005.GetDeckEccentricity() );
   (*this)(row,4) << area.SetValue( details.pLosses->GetAc() );
   (*this)(row,5) << mom_inertia.SetValue( details.pLosses->GetIc() );
   (*this)(row,6) << stress.SetValue( details.RefinedLosses2005.GetDeltaFcdf() );
   (*this)(row,7) << stress.SetValue( details.RefinedLosses2005.ElasticGainDueToDeckShrinkage() );
   (*this)(row,8) << stress.SetValue( fTop );
   (*this)(row,9) << stress.SetValue( fBot );
}
