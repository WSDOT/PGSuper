///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2011  Washington State Department of Transportation
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

// ShrinkageAtFinalTable.cpp : Implementation of CShrinkageAtFinalTable
#include "stdafx.h"
#include "ShrinkageAtFinalTable.h"
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <PsgLib\SpecLibraryEntry.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CShrinkageAtFinalTable::CShrinkageAtFinalTable(ColumnIndexType NumColumns, IEAFDisplayUnits* pDisplayUnits) :
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

CShrinkageAtFinalTable* CShrinkageAtFinalTable::PrepareTable(rptChapter* pChapter,IBroker* pBroker,SpanIndexType span,GirderIndexType gdr,LOSSDETAILS& details,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   GET_IFACE2(pBroker,ISpecification,pSpec);
   std::_tstring strSpecName = pSpec->GetSpecification();

   GET_IFACE2(pBroker,ILibrary,pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( strSpecName.c_str() );

   // Create and configure the table
   ColumnIndexType numColumns = 7;
   CShrinkageAtFinalTable* table = new CShrinkageAtFinalTable( numColumns, pDisplayUnits );
   pgsReportStyleHolder::ConfigureTable(table);


   std::_tstring strImagePath(pgsReportStyleHolder::GetImagePath());

   rptParagraph* pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;
   *pParagraph << _T("[5.9.5.4.3a] Shrinkage of Girder Concrete : ") << symbol(DELTA) << RPT_STRESS(_T("pSD")) << rptNewLine;

   pParagraph = new rptParagraph;
   *pChapter << pParagraph;

   *pParagraph << rptRcImage(strImagePath + _T("Delta_FpSD.png")) << rptNewLine;

   if ( pSpecEntry->GetSpecificationType() <= lrfdVersionMgr::ThirdEditionWith2005Interims )
   {
      if ( IS_SI_UNITS(pDisplayUnits) )
         *pParagraph << rptRcImage(strImagePath + _T("VSFactor_SI_2005.png")) << rptNewLine;
      else
         *pParagraph << rptRcImage(strImagePath + _T("VSFactor_US_2005.png")) << rptNewLine;
   }
#if defined IGNORE_2007_CHANGES
   else
   {
      if ( IS_SI_UNITS(pDisplayUnits) )
         *pParagraph << rptRcImage(strImagePath + _T("VSFactor_SI_2006.png")) << rptNewLine;
      else
         *pParagraph << rptRcImage(strImagePath + _T("VSFactor_US_2006.png")) << rptNewLine;
   }
#else
   else if ( pSpecEntry->GetSpecificationType() == lrfdVersionMgr::ThirdEditionWith2006Interims )
   {
      if ( IS_SI_UNITS(pDisplayUnits) )
         *pParagraph << rptRcImage(strImagePath + _T("VSFactor_SI_2006.png")) << rptNewLine;
      else
         *pParagraph << rptRcImage(strImagePath + _T("VSFactor_US_2006.png")) << rptNewLine;
   }
   else
   {
      if ( IS_SI_UNITS(pDisplayUnits) )
         *pParagraph << rptRcImage(strImagePath + _T("VSFactor_SI_2007.png")) << rptNewLine;
      else
         *pParagraph << rptRcImage(strImagePath + _T("VSFactor_US_2007.png")) << rptNewLine;
   }
#endif // IGNORE_2007_CHANGES
   *pParagraph << rptRcImage(strImagePath + _T("HumidityFactor.png")) << rptNewLine;
   if ( IS_SI_UNITS(pDisplayUnits) )
      *pParagraph << rptRcImage(strImagePath + _T("ConcreteFactors_SI.png")) << rptNewLine;
   else
      *pParagraph << rptRcImage(strImagePath + _T("ConcreteFactors_US.png")) << rptNewLine;
   

   // parameters for calculations (two tables to keep the width printable)
   rptRcTable* pParamTable = pgsReportStyleHolder::CreateDefaultTable(5,_T(""));
   *pParagraph << pParamTable << rptNewLine;
   (*pParamTable)(0,0) << _T("H") << rptNewLine << _T("(%)");
   (*pParamTable)(0,1) << COLHDR(_T("V/S"),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*pParamTable)(0,2) << COLHDR(RPT_FCI,rptStressUnitTag,pDisplayUnits->GetStressUnit());
   (*pParamTable)(0,3) << COLHDR(Sub2(_T("t"),_T("i")),rptTimeUnitTag,pDisplayUnits->GetLongTimeUnit());
   (*pParamTable)(0,4) << COLHDR(Sub2(_T("t"),_T("f")),rptTimeUnitTag,pDisplayUnits->GetLongTimeUnit());

   (*pParamTable)(1,0) << details.RefinedLosses2005.GetRelHumidity();
   (*pParamTable)(1,1) << table->ecc.SetValue(details.RefinedLosses2005.GetVolume()/details.RefinedLosses2005.GetSurfaceArea());
   (*pParamTable)(1,2) << table->stress.SetValue(details.RefinedLosses2005.GetFci());
   (*pParamTable)(1,3) << table->time.SetValue(details.RefinedLosses2005.GetAdjustedInitialAge());
   (*pParamTable)(1,4) << table->time.SetValue(details.RefinedLosses2005.GetFinalAge());

   pParamTable = pgsReportStyleHolder::CreateDefaultTable(10,_T(""));
   *pParagraph << pParamTable << rptNewLine;
   pParamTable->SetNumberOfHeaderRows(2);
   pParamTable->SetRowSpan(0,0,2);
   pParamTable->SetRowSpan(1,0,-1);
   (*pParamTable)(0,0) << COLHDR(Sub2(_T("E"),_T("p")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
   pParamTable->SetRowSpan(0,1,2);
   pParamTable->SetRowSpan(1,1,-1);
   (*pParamTable)(0,1) << COLHDR(Sub2(_T("E"),_T("ci")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
   pParamTable->SetColumnSpan(0,2,5);
   pParamTable->SetColumnSpan(0,3,-1);
   pParamTable->SetColumnSpan(0,4,-1);
   pParamTable->SetColumnSpan(0,5,-1);
   pParamTable->SetColumnSpan(0,6,-1);
   (*pParamTable)(0,2) << _T("Shrinkage");
   (*pParamTable)(1,2) << Sub2(_T("K"),_T("1"));
   (*pParamTable)(1,3) << Sub2(_T("K"),_T("2"));
   (*pParamTable)(1,4) << Sub2(symbol(epsilon),_T("bif")) << _T("x1000");
   (*pParamTable)(1,5) << Sub2(symbol(epsilon),_T("bid")) << _T("x1000");
   (*pParamTable)(1,6) << Sub2(symbol(epsilon),_T("bdf")) << _T("x1000");
   pParamTable->SetColumnSpan(0,7,3);
   pParamTable->SetColumnSpan(0,8,-1);
   pParamTable->SetColumnSpan(0,9,-1);
   (*pParamTable)(0,7) << _T("Creep");
   (*pParamTable)(1,7) << Sub2(_T("K"),_T("1"));
   (*pParamTable)(1,8) << Sub2(_T("K"),_T("2"));
   (*pParamTable)(1,9) << Sub2(symbol(psi),_T("b")) << _T("(") << Sub2(_T("t"),_T("f")) << _T(",") << Sub2(_T("t"),_T("i")) << _T(")");

   (*pParamTable)(2,0) << table->mod_e.SetValue(details.RefinedLosses2005.GetEp());
   (*pParamTable)(2,1) << table->mod_e.SetValue(details.RefinedLosses2005.GetEci());
   (*pParamTable)(2,2) << details.RefinedLosses2005.GetGdrK1Shrinkage();
   (*pParamTable)(2,3) << details.RefinedLosses2005.GetGdrK2Shrinkage();
   (*pParamTable)(2,4) << table->strain.SetValue(details.RefinedLosses2005.Get_ebif() * 1000);
   (*pParamTable)(2,5) << table->strain.SetValue(details.RefinedLosses2005.Get_ebid() * 1000);
   (*pParamTable)(2,6) << table->strain.SetValue(details.RefinedLosses2005.Get_ebdf() * 1000);
   (*pParamTable)(2,7) << details.RefinedLosses2005.GetGdrK1Creep();
   (*pParamTable)(2,8) << details.RefinedLosses2005.GetGdrK2Creep();
   (*pParamTable)(2,9) << table->scalar.SetValue(details.RefinedLosses2005.GetCreepInitialToFinal().GetCreepCoefficient());

   // intermediate results
   pParamTable = pgsReportStyleHolder::CreateDefaultTable(5,_T(""));
   *pParagraph << pParamTable << rptNewLine;
   (*pParamTable)(0,0) << Sub2(_T("k"),_T("vs"));
   (*pParamTable)(0,1) << Sub2(_T("k"),_T("hs"));
   (*pParamTable)(0,2) << Sub2(_T("k"),_T("hc"));
   (*pParamTable)(0,3) << Sub2(_T("k"),_T("f"));

   table->time.ShowUnitTag(true);
   (*pParamTable)(0,4) << Sub2(_T("k"),_T("td")) << rptNewLine << _T("t = ") << table->time.SetValue(details.RefinedLosses2005.GetFinalAge());
   table->time.ShowUnitTag(false);

   (*pParamTable)(1,0) << table->scalar.SetValue(details.RefinedLosses2005.GetCreepInitialToFinal().GetKvs());
   (*pParamTable)(1,1) << table->scalar.SetValue(details.RefinedLosses2005.Getkhs());
   (*pParamTable)(1,2) << table->scalar.SetValue(details.RefinedLosses2005.GetCreepInitialToFinal().GetKhc());
   (*pParamTable)(1,3) << table->scalar.SetValue(details.RefinedLosses2005.GetCreepInitialToFinal().GetKf());
   (*pParamTable)(1,4) << table->scalar.SetValue(details.RefinedLosses2005.GetCreepInitialToFinal().GetKtd());

   // shrinkage loss   
   *pParagraph << table << rptNewLine;
   (*table)(0,0) << COLHDR(_T("Location from")<<rptNewLine<<_T("Left Support"),rptLengthUnitTag,  pDisplayUnits->GetSpanLengthUnit() );
   (*table)(0,1) << COLHDR(Sub2(_T("A"),_T("ps")), rptAreaUnitTag, pDisplayUnits->GetAreaUnit());
   (*table)(0,2) << COLHDR(Sub2(_T("A"),_T("c")), rptAreaUnitTag, pDisplayUnits->GetAreaUnit());
   (*table)(0,3) << COLHDR(Sub2(_T("I"),_T("c")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
   (*table)(0,4) << COLHDR(Sub2(_T("e"),_T("pc")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
   (*table)(0,5) << Sub2(_T("K"),_T("df"));
   (*table)(0,6) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pSD")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );

   return table;
}

void CShrinkageAtFinalTable::AddRow(rptChapter* pChapter,IBroker* pBroker,RowIndexType row,LOSSDETAILS& details,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   (*this)(row,1) << area.SetValue(details.pLosses->GetApsPermanent());
   (*this)(row,2) << area.SetValue(details.pLosses->GetAc());
   (*this)(row,3) << mom_inertia.SetValue(details.pLosses->GetIc());
   (*this)(row,4) << ecc.SetValue(details.pLosses->GetEccpc());
   (*this)(row,5) << scalar.SetValue(details.RefinedLosses2005.GetKdf());
   (*this)(row,6) << stress.SetValue(details.RefinedLosses2005.ShrinkageLossAfterDeckPlacement());
}
