///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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

// DeckShrinkageLossTable.cpp : Implementation of CDeckShrinkageLossTable
#include "stdafx.h"
#include "DeckShrinkageLossTable.h"
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <PsgLib\SpecLibraryEntry.h>
#include <PgsExt\GirderData.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CDeckShrinkageLossTable::CDeckShrinkageLossTable(ColumnIndexType NumColumns, IDisplayUnits* pDisplayUnits) :
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
   scalar.SetPrecision(2);

   strain.SetFormat( sysNumericFormatTool::Automatic );
   strain.SetWidth(6);
   strain.SetPrecision(3);
}

CDeckShrinkageLossTable* CDeckShrinkageLossTable::PrepareTable(rptChapter* pChapter,IBroker* pBroker,SpanIndexType span,GirderIndexType gdr,LOSSDETAILS& details,IDisplayUnits* pDisplayUnits,Uint16 level)
{
   GET_IFACE2(pBroker,ISpecification,pSpec);
   std::string strSpecName = pSpec->GetSpecification();

   GET_IFACE2(pBroker,ILibrary,pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( strSpecName.c_str() );

   // Create and configure the table
   ColumnIndexType numColumns = 8;
   CDeckShrinkageLossTable* table = new CDeckShrinkageLossTable( numColumns, pDisplayUnits );
   pgsReportStyleHolder::ConfigureTable(table);

   std::string strImagePath(pgsReportStyleHolder::GetImagePath());
   
   rptParagraph* pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;

   *pParagraph << "[5.9.5.4.3d] Shrinkage of Deck Concrete : " << symbol(DELTA) << Sub2("f","pSS") << rptNewLine;

   pParagraph = new rptParagraph;
   *pChapter << pParagraph;

#if defined IGNORE_2007_CHANGES
   if ( pDisplayUnits->GetUnitDisplayMode() == pgsTypes::umSI )
      *pParagraph << rptRcImage(strImagePath + "Delta_FpSS_SI_2006.gif") << rptNewLine;
   else
      *pParagraph << rptRcImage(strImagePath + "Delta_FpSS_US_2006.gif") << rptNewLine;
#else
   if ( pSpecEntry->GetSpecificationType() < lrfdVersionMgr::FourthEdition2007 )
   {
      if ( pDisplayUnits->GetUnitDisplayMode() == pgsTypes::umSI )
         *pParagraph << rptRcImage(strImagePath + "Delta_FpSS_SI_2006.gif") << rptNewLine;
      else
         *pParagraph << rptRcImage(strImagePath + "Delta_FpSS_US_2006.gif") << rptNewLine;
   }
   else
   {
      if ( pDisplayUnits->GetUnitDisplayMode() == pgsTypes::umSI )
         *pParagraph << rptRcImage(strImagePath + "Delta_FpSS_SI_2007.gif") << rptNewLine;
      else
         *pParagraph << rptRcImage(strImagePath + "Delta_FpSS_US_2007.gif") << rptNewLine;
   }
#endif // IGNORE_2007_CHANGES
//
   rptRcTable* pParamTable = pgsReportStyleHolder::CreateDefaultTable(7,"");
   *pParagraph << pParamTable << rptNewLine;
   (*pParamTable)(0,0) << COLHDR("V/S" << rptNewLine << "deck",rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
#if defined IGNORE_2007_CHANGES
   (*pParamTable)(0,1) << Sub2("k","vs") << rptNewLine << "deck";
#else
   if ( pSpecEntry->GetSpecificationType() <= lrfdVersionMgr::ThirdEditionWith2006Interims )
      (*pParamTable)(0,1) << Sub2("k","vs") << rptNewLine << "deck";
   else
      (*pParamTable)(0,1) << Sub2("k","s") << rptNewLine << "deck";
#endif // IGNORE_2007_CHANGES

   (*pParamTable)(0,2) << Sub2("k","hs");
   (*pParamTable)(0,3) << COLHDR(RPT_FC << rptNewLine << "deck", rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*pParamTable)(0,4) << Sub2("k","f");
   (*pParamTable)(0,5) << Sub2("k","td") << rptNewLine << "t = " << Sub2("t","f") << " - " << Sub2("t","d");
   (*pParamTable)(0,6) << Sub2(symbol(epsilon),"ddf") << "x 1000";

   (*pParamTable)(1,0) << table->ecc.SetValue(details.RefinedLosses2005.GetVolumeSlab()/details.RefinedLosses2005.GetSurfaceAreaSlab());
   (*pParamTable)(1,1) << table->scalar.SetValue(details.RefinedLosses2005.GetCreepDeck().GetKvs());
   (*pParamTable)(1,2) << table->scalar.SetValue(details.RefinedLosses2005.Getkhs());
   (*pParamTable)(1,3) << table->stress.SetValue(details.RefinedLosses2005.GetFcSlab() );
   (*pParamTable)(1,4) << table->scalar.SetValue(details.RefinedLosses2005.GetCreepDeck().GetKf());
   (*pParamTable)(1,5) << table->scalar.SetValue(details.RefinedLosses2005.GetCreepDeck().GetKtd());
   (*pParamTable)(1,6) << table->strain.SetValue(details.RefinedLosses2005.Get_eddf() * 1000);

   pParamTable = pgsReportStyleHolder::CreateDefaultTable(4,"");
   *pParagraph << pParamTable << rptNewLine;
   (*pParamTable)(0,0) << COLHDR( Sub2("E","p"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*pParamTable)(0,1) << COLHDR( Sub2("E","c"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*pParamTable)(0,2) << COLHDR( Sub2("E","cd"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*pParamTable)(0,3) << Sub2(symbol(psi),"b") << "(" << Sub2("t","f") << "," << Sub2("t","d") << ")";

   (*pParamTable)(1,0) << table->mod_e.SetValue( details.RefinedLosses2005.GetEp() );
   (*pParamTable)(1,1) << table->mod_e.SetValue( details.RefinedLosses2005.GetEc() );
   (*pParamTable)(1,2) << table->mod_e.SetValue( details.RefinedLosses2005.GetEcd() );
   (*pParamTable)(1,3) << table->scalar.SetValue(details.RefinedLosses2005.GetCreepDeckToFinal().GetCreepCoefficient());

   *pParagraph << table << rptNewLine;
   (*table)(0,0) << COLHDR("Location from"<<rptNewLine<<"Left Support",rptLengthUnitTag,  pDisplayUnits->GetSpanLengthUnit() );
   (*table)(0,1) << COLHDR( Sub2("A","d"), rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );
   (*table)(0,2) << COLHDR( Sub2("e","pc"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,3) << COLHDR( Sub2("e","d"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,4) << COLHDR( Sub2("A","c"), rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );
   (*table)(0,5) << COLHDR( Sub2("I","c"), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit() );
   (*table)(0,6) << COLHDR( symbol(DELTA) << Sub2("f","cdf"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,7) << COLHDR( symbol(DELTA) << Sub2("f","pSS"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   
#if defined IGNORE_2007_CHANGES
   table->m_Sign = -1;
#else
   table->m_Sign =  ( pSpecEntry->GetSpecificationType() < lrfdVersionMgr::FourthEdition2007 ) ? 1 : -1;
#endif // IGNORE_2007_CHANGES

   return table;
}

void CDeckShrinkageLossTable::AddRow(rptChapter* pChapter,IBroker* pBroker,RowIndexType row,LOSSDETAILS& details,IDisplayUnits* pDisplayUnits,Uint16 level)
{
   (*this)(row,1) << area.SetValue( details.RefinedLosses2005.GetAd() );
   (*this)(row,2) << ecc.SetValue( details.RefinedLosses2005.GetEccpc() );
   (*this)(row,3) << ecc.SetValue( m_Sign*details.RefinedLosses2005.GetDeckEccentricity() );
   (*this)(row,4) << area.SetValue( details.pLosses->GetAc() );
   (*this)(row,5) << mom_inertia.SetValue( details.pLosses->GetIc() );
   (*this)(row,6) << stress.SetValue( details.RefinedLosses2005.GetDeltaFcdf() );
   (*this)(row,7) << stress.SetValue( details.RefinedLosses2005.DeckShrinkageLoss() );
}
