///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2018  Washington State Department of Transportation
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

// ShrinkageAtDeckPlacementTable.cpp : Implementation of CShrinkageAtDeckPlacementTable
#include "stdafx.h"
#include "ShrinkageAtDeckPlacementTable.h"
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <PsgLib\SpecLibraryEntry.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CShrinkageAtDeckPlacementTable::CShrinkageAtDeckPlacementTable(ColumnIndexType NumColumns, IEAFDisplayUnits* pDisplayUnits) :
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
   DEFINE_UV_PROTOTYPE( time,        pDisplayUnits->GetWholeDaysUnit(),        false );

   scalar.SetFormat( sysNumericFormatTool::Automatic );
   scalar.SetWidth(7);
   scalar.SetPrecision(3);

   strain.SetFormat( sysNumericFormatTool::Automatic );
   strain.SetWidth(6);
   strain.SetPrecision(3);
}

CShrinkageAtDeckPlacementTable* CShrinkageAtDeckPlacementTable::PrepareTable(rptChapter* pChapter,IBroker* pBroker,const CSegmentKey& segmentKey,const LOSSDETAILS* pDetails,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   GET_IFACE2(pBroker,ISpecification,pSpec);
   std::_tstring strSpecName = pSpec->GetSpecification();

   GET_IFACE2(pBroker,ILibrary,pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( strSpecName.c_str() );

   GET_IFACE2(pBroker,ISectionProperties,pSectProp);
   pgsTypes::SectionPropertyMode spMode = pSectProp->GetSectionPropertiesMode();

   // Create and configure the table
   ColumnIndexType numColumns = 8;
   CShrinkageAtDeckPlacementTable* table = new CShrinkageAtDeckPlacementTable( numColumns, pDisplayUnits );
   rptStyleManager::ConfigureTable(table);


   std::_tstring strImagePath(rptStyleManager::GetImagePath());
   
   rptParagraph* pParagraph = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pParagraph;

   *pParagraph << _T("[") << LrfdCw8th(_T("5.9.5.4.2a"),_T("5.9.3.4.2a")) << _T("] Shrinkage of Girder Concrete : ") << symbol(DELTA) << RPT_STRESS(_T("pSR")) << rptNewLine;

   if ( spMode == pgsTypes::spmGross )
   {
      *pParagraph << rptRcImage(strImagePath + _T("Delta_FpSR_Gross.png")) << rptNewLine;
   }
   else
   {
      *pParagraph << rptRcImage(strImagePath + _T("Delta_FpSR_Transformed.png")) << rptNewLine;
   }

   if ( pSpecEntry->GetSpecificationType() <= lrfdVersionMgr::ThirdEditionWith2005Interims )
   {
      if ( IS_SI_UNITS(pDisplayUnits) )
      {
         *pParagraph << rptRcImage(strImagePath + _T("KvsEqn-SI.png")) << rptNewLine;
      }
      else
      {
         *pParagraph << rptRcImage(strImagePath + _T("KvsEqn-US.png")) << rptNewLine;
      }
   }
   else if ( pSpecEntry->GetSpecificationType() == lrfdVersionMgr::ThirdEditionWith2006Interims )
   {
      if ( IS_SI_UNITS(pDisplayUnits) )
      {
         *pParagraph << rptRcImage(strImagePath + _T("KvsEqn2006-SI.png")) << rptNewLine;
      }
      else
      {
         *pParagraph << rptRcImage(strImagePath + _T("KvsEqn2006-US.png")) << rptNewLine;
      }
   }
   else
   {
      if ( IS_SI_UNITS(pDisplayUnits) )
      {
         *pParagraph << rptRcImage(strImagePath + _T("KvsEqn2007-SI.png")) << rptNewLine;
      }
      else
      {
         *pParagraph << rptRcImage(strImagePath + _T("KvsEqn2007-US.png")) << rptNewLine;
      }
   }

   *pParagraph << rptRcImage(strImagePath + _T("HumidityFactor.png")) << rptNewLine;
   if ( IS_SI_UNITS(pDisplayUnits) )
   {
      ATLASSERT( pSpecEntry->GetSpecificationType() < lrfdVersionMgr::SeventhEditionWith2015Interims );
      *pParagraph << rptRcImage(strImagePath + _T("ConcreteFactors_SI.png")) << rptNewLine;
   }
   else
   {
      if ( pSpecEntry->GetSpecificationType() < lrfdVersionMgr::SeventhEditionWith2015Interims )
      {
         *pParagraph << rptRcImage(strImagePath + _T("ConcreteFactors_US.png")) << rptNewLine;
      }
      else
      {
         *pParagraph << rptRcImage(strImagePath + _T("ConcreteFactors_US2015.png")) << rptNewLine;
      }
   }

  // Typecast to our known type (eating own doggy food)
   std::shared_ptr<const lrfdRefinedLosses2005> ptl = std::dynamic_pointer_cast<const lrfdRefinedLosses2005>(pDetails->pLosses);
   if (!ptl)
   {
      ATLASSERT(false); // made a bad cast? Bail...
      return table;
   }

   if ( ptl->AdjustShrinkageStrain() )
   {
      // LRFD 5.4.2.3.3
      // If the concrete is exposed to drying before 5 days of curing have elapsed,
      // the shrinkage as determined in Eq 5.4.2.3.3-1 should be increased by 20%
      *pParagraph << _T("Girder is exposed to drying before 5 days of curing have elapsed, the shrinkage strain has been increased by 20% (LRFD 5.4.2.3.3)") << rptNewLine;
   }

   // parameters for calculations (two tables to keep the width printable)
   rptRcTable* pParamTable = rptStyleManager::CreateDefaultTable(6,_T(""));
   *pParagraph << pParamTable << rptNewLine;
   (*pParamTable)(0,0) << _T("H") << rptNewLine << _T("(%)");
   (*pParamTable)(0,1) << COLHDR(_T("V/S"),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*pParamTable)(0,2) << COLHDR(RPT_FCI,rptStressUnitTag,pDisplayUnits->GetStressUnit());
   (*pParamTable)(0,3) << COLHDR(Sub2(_T("t"),_T("i")),rptTimeUnitTag,pDisplayUnits->GetWholeDaysUnit());
   (*pParamTable)(0,4) << COLHDR(Sub2(_T("t"),_T("d")),rptTimeUnitTag,pDisplayUnits->GetWholeDaysUnit());
   (*pParamTable)(0,5) << COLHDR(Sub2(_T("t"),_T("f")),rptTimeUnitTag,pDisplayUnits->GetWholeDaysUnit());

   (*pParamTable)(1,0) << ptl->GetRelHumidity();
   (*pParamTable)(1,1) << table->ecc.SetValue(ptl->GetVolume()/ptl->GetSurfaceArea());
   (*pParamTable)(1,2) << table->stress.SetValue(ptl->GetFci());
   (*pParamTable)(1,3) << table->time.SetValue(ptl->GetAdjustedInitialAge());
   (*pParamTable)(1,4) << table->time.SetValue(ptl->GetAgeAtDeckPlacement());
   (*pParamTable)(1,5) << table->time.SetValue(ptl->GetFinalAge());

   // intermediate results
   pParamTable = rptStyleManager::CreateDefaultTable(6,_T(""));
   *pParagraph << pParamTable << rptNewLine;

   if ( lrfdVersionMgr::FourthEdition2007 <= pSpecEntry->GetSpecificationType() )
   {
     (*pParamTable)(0,0) << Sub2(_T("k"),_T("s"));
   }
   else
   {
     (*pParamTable)(0,0) << Sub2(_T("k"),_T("vs"));
   }

    (*pParamTable)(0,1) << Sub2(_T("k"),_T("hs"));
   (*pParamTable)(0,2) << Sub2(_T("k"),_T("hc"));
   (*pParamTable)(0,3) << Sub2(_T("k"),_T("f"));

   table->time.ShowUnitTag(true);
   (*pParamTable)(0,4) << Sub2(_T("k"),_T("td")) << rptNewLine << _T("Initial to Deck Placement") << rptNewLine << _T("t = ") << table->time.SetValue(ptl->GetCreepInitialToDeck().GetMaturity());
   (*pParamTable)(0,5) << Sub2(_T("k"),_T("td")) << rptNewLine << _T("Initial to Final") << rptNewLine << _T("t = ") << table->time.SetValue(ptl->GetCreepInitialToFinal().GetMaturity());
   table->time.ShowUnitTag(false);

   (*pParamTable)(1,0) << table->scalar.SetValue(ptl->GetCreepInitialToFinal().GetKvs());
   (*pParamTable)(1,1) << table->scalar.SetValue(ptl->Getkhs());
   (*pParamTable)(1,2) << table->scalar.SetValue(ptl->GetCreepInitialToFinal().GetKhc());
   (*pParamTable)(1,3) << table->scalar.SetValue(ptl->GetCreepInitialToFinal().GetKf());
   (*pParamTable)(1,4) << table->scalar.SetValue(ptl->GetCreepInitialToDeck().GetKtd());
   (*pParamTable)(1,5) << table->scalar.SetValue(ptl->GetCreepInitialToFinal().GetKtd());

   pParamTable = rptStyleManager::CreateDefaultTable(8,_T(""));
   *pParagraph << pParamTable << rptNewLine;
   pParamTable->SetNumberOfHeaderRows(2);
   pParamTable->SetRowSpan(0,0,2);
   pParamTable->SetRowSpan(1,0,SKIP_CELL);
   (*pParamTable)(0,0) << COLHDR(Sub2(_T("E"),_T("p")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
   pParamTable->SetRowSpan(0,1,2);
   pParamTable->SetRowSpan(1,1,SKIP_CELL);
   (*pParamTable)(0,1) << COLHDR(Sub2(_T("E"),_T("ci")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
   pParamTable->SetColumnSpan(0,2,3);
   pParamTable->SetColumnSpan(0,3,SKIP_CELL);
   pParamTable->SetColumnSpan(0,4,SKIP_CELL);
   (*pParamTable)(0,2) << _T("Shrinkage");
   (*pParamTable)(1,2) << Sub2(_T("K"),_T("1"));
   (*pParamTable)(1,3) << Sub2(_T("K"),_T("2"));
   (*pParamTable)(1,4) << Sub2(symbol(epsilon),_T("bid")) << _T("x 1000");
   pParamTable->SetColumnSpan(0,5,3);
   pParamTable->SetColumnSpan(0,6,SKIP_CELL);
   pParamTable->SetColumnSpan(0,7,SKIP_CELL);
   (*pParamTable)(0,5) << _T("Creep");
   (*pParamTable)(1,5) << Sub2(_T("K"),_T("1"));
   (*pParamTable)(1,6) << Sub2(_T("K"),_T("2"));
   (*pParamTable)(1,7) << Sub2(symbol(psi),_T("b")) << _T("(") << Sub2(_T("t"),_T("f")) << _T(",") << Sub2(_T("t"),_T("i")) << _T(")");

   (*pParamTable)(2,0) << table->mod_e.SetValue(ptl->GetEp());
   (*pParamTable)(2,1) << table->mod_e.SetValue(ptl->GetEci());
   (*pParamTable)(2,2) << ptl->GetGdrK1Shrinkage();
   (*pParamTable)(2,3) << ptl->GetGdrK2Shrinkage();
   (*pParamTable)(2,4) << table->strain.SetValue(ptl->Get_ebid() * 1000);
   (*pParamTable)(2,5) << ptl->GetGdrK1Creep();
   (*pParamTable)(2,6) << ptl->GetGdrK2Creep();
   (*pParamTable)(2,7) << table->scalar.SetValue(ptl->GetCreepInitialToFinal().GetCreepCoefficient());

   // shrinkage loss   
   *pParagraph << table << rptNewLine;
   (*table)(0,0) << COLHDR(_T("Location from")<<rptNewLine<<_T("Left Support"),rptLengthUnitTag,  pDisplayUnits->GetSpanLengthUnit() );
   (*table)(0,1) << COLHDR(Sub2(_T("A"),_T("ps")), rptAreaUnitTag, pDisplayUnits->GetAreaUnit());
   if ( spMode == pgsTypes::spmGross )
   {
      (*table)(0,2) << COLHDR(Sub2(_T("A"),_T("g")), rptAreaUnitTag, pDisplayUnits->GetAreaUnit());
      (*table)(0,3) << COLHDR(Sub2(_T("I"),_T("g")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
      (*table)(0,4) << COLHDR(Sub2(_T("e"),_T("ps")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      (*table)(0,5) << COLHDR(Sub2(_T("e"),_T("p")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
   }
   else
   {
      (*table)(0,2) << COLHDR(Sub2(_T("A"),_T("n")), rptAreaUnitTag, pDisplayUnits->GetAreaUnit());
      (*table)(0,3) << COLHDR(Sub2(_T("I"),_T("n")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
      (*table)(0,4) << COLHDR(Sub2(_T("e"),_T("psn")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      (*table)(0,5) << COLHDR(Sub2(_T("e"),_T("pn")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
   }
   (*table)(0,6) << Sub2(_T("K"),_T("id"));
   (*table)(0,7) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pSR")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );

   return table;
}

void CShrinkageAtDeckPlacementTable::AddRow(rptChapter* pChapter,IBroker* pBroker,const pgsPointOfInterest& poi,RowIndexType row,const LOSSDETAILS* pDetails,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   // Typecast to our known type (eating own doggy food)
   std::shared_ptr<const lrfdRefinedLosses2005> ptl = std::dynamic_pointer_cast<const lrfdRefinedLosses2005>(pDetails->pLosses);
   if (!ptl)
   {
      ATLASSERT(false); // made a bad cast? Bail...
      return;
   }

   Float64 Aps = pDetails->pLosses->GetApsPermanent();
   if ( pDetails->pLosses->GetTempStrandUsage() == lrfdLosses::tsPretensioned ||
        pDetails->pLosses->GetTempStrandUsage() == lrfdLosses::tsPTBeforeLifting )
   {
      Aps += pDetails->pLosses->GetApsTemporary();
   }

   Float64 e  = pDetails->pLosses->GetEccPermanentFinal();
   Float64 eps = e;
   if ( pDetails->pLosses->GetTempStrandUsage() == lrfdLosses::tsPretensioned )
   {
      eps = pDetails->pLosses->GetEccpgFinal();
   }

   (*this)(row,1) << area.SetValue(Aps);
   (*this)(row,2) << area.SetValue(pDetails->pLosses->GetAn());
   (*this)(row,3) << mom_inertia.SetValue(pDetails->pLosses->GetIn());
   (*this)(row,4) << ecc.SetValue(eps);
   (*this)(row,5) << ecc.SetValue(e);
   (*this)(row,6) << scalar.SetValue(ptl->GetKid());
   (*this)(row,7) << stress.SetValue( ptl->ShrinkageLossBeforeDeckPlacement() );
}
