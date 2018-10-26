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
   std::string strSpecName = pSpec->GetSpecification();

   GET_IFACE2(pBroker,ILibrary,pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( strSpecName.c_str() );

   // Create and configure the table
   ColumnIndexType numColumns = 7;
   CShrinkageAtFinalTable* table = new CShrinkageAtFinalTable( numColumns, pDisplayUnits );
   pgsReportStyleHolder::ConfigureTable(table);


   std::string strImagePath(pgsReportStyleHolder::GetImagePath());
   
   rptParagraph* pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;
   *pParagraph << "[5.9.5.4.3a] Shrinkage of Girder Concrete : " << symbol(DELTA) << Sub2("f","pSD") << rptNewLine;

   
   pParagraph = new rptParagraph;
   *pChapter << pParagraph;

   if ( pSpecEntry->GetSpecificationType() <= lrfdVersionMgr::ThirdEditionWith2005Interims )
   {
      if ( IS_SI_UNITS(pDisplayUnits) )
          *pParagraph << rptRcImage(strImagePath + "Delta_FpSD_SI.gif") << rptNewLine;
      else
          *pParagraph << rptRcImage(strImagePath + "Delta_FpSD_US.gif") << rptNewLine;
   }
#if defined IGNORE_2007_CHANGES
   else
   {
      if ( IS_SI_UNITS(pDisplayUnits) )
          *pParagraph << rptRcImage(strImagePath + "Delta_FpSD_SI_2006.gif") << rptNewLine;
      else
          *pParagraph << rptRcImage(strImagePath + "Delta_FpSD_US_2006.gif") << rptNewLine;
   }
#else
   else if ( pSpecEntry->GetSpecificationType() == lrfdVersionMgr::ThirdEditionWith2006Interims )
   {
      if ( pDisplayUnits->GetUnitDisplayMode() == pgsTypes::umSI )
          *pParagraph << rptRcImage(strImagePath + "Delta_FpSD_SI_2006.gif") << rptNewLine;
      else
          *pParagraph << rptRcImage(strImagePath + "Delta_FpSD_US_2006.gif") << rptNewLine;
   }
   else
   {
      if ( pDisplayUnits->GetUnitDisplayMode() == pgsTypes::umSI )
          *pParagraph << rptRcImage(strImagePath + "Delta_FpSD_SI_2007.gif") << rptNewLine;
      else
          *pParagraph << rptRcImage(strImagePath + "Delta_FpSD_US_2007.gif") << rptNewLine;
   }
#endif // IGNORE_2007_CHANGES

   // parameters for calculations (two tables to keep the width printable)
   rptRcTable* pParamTable = pgsReportStyleHolder::CreateDefaultTable(5,"");
   *pParagraph << pParamTable << rptNewLine;
   (*pParamTable)(0,0) << "H" << rptNewLine << "(%)";
   (*pParamTable)(0,1) << COLHDR("V/S",rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*pParamTable)(0,2) << COLHDR(RPT_FCI,rptStressUnitTag,pDisplayUnits->GetStressUnit());
   (*pParamTable)(0,3) << COLHDR(Sub2("t","i"),rptTimeUnitTag,pDisplayUnits->GetLongTimeUnit());
   (*pParamTable)(0,4) << COLHDR(Sub2("t","f"),rptTimeUnitTag,pDisplayUnits->GetLongTimeUnit());

   (*pParamTable)(1,0) << details.RefinedLosses2005.GetRelHumidity();
   (*pParamTable)(1,1) << table->ecc.SetValue(details.RefinedLosses2005.GetVolume()/details.RefinedLosses2005.GetSurfaceArea());
   (*pParamTable)(1,2) << table->stress.SetValue(details.RefinedLosses2005.GetFci());
   (*pParamTable)(1,3) << table->time.SetValue(details.RefinedLosses2005.GetAdjustedInitialAge());
   (*pParamTable)(1,4) << table->time.SetValue(details.RefinedLosses2005.GetFinalAge());

   pParamTable = pgsReportStyleHolder::CreateDefaultTable(4,"");
   *pParagraph << pParamTable << rptNewLine;
   (*pParamTable)(0,0) << COLHDR(Sub2("E","p"), rptStressUnitTag, pDisplayUnits->GetStressUnit());
   (*pParamTable)(0,1) << COLHDR(Sub2("E","ci"), rptStressUnitTag, pDisplayUnits->GetStressUnit());
   (*pParamTable)(0,2) << Sub2(symbol(epsilon),"bdf") << "x 1000";
   (*pParamTable)(0,3) << Sub2(symbol(psi),"b") << "(" << Sub2("t","f") << "," << Sub2("t","i") << ")";

   (*pParamTable)(1,0) << table->mod_e.SetValue(details.RefinedLosses2005.GetEp());
   (*pParamTable)(1,1) << table->mod_e.SetValue(details.RefinedLosses2005.GetEci());
   (*pParamTable)(1,2) << table->strain.SetValue(details.RefinedLosses2005.Get_ebdf() * 1000);
   (*pParamTable)(1,3) << table->scalar.SetValue(details.RefinedLosses2005.GetCreepInitialToFinal().GetCreepCoefficient());

   // intermediate results
   pParamTable = pgsReportStyleHolder::CreateDefaultTable(5,"");
   *pParagraph << pParamTable << rptNewLine;
#if defined IGNORE_2007_CHANGES
   (*pParamTable)(0,0) << Sub2("k","vs");
#else
   if ( pSpecEntry->GetSpecificationType() <= lrfdVersionMgr::ThirdEditionWith2006Interims )
      (*pParamTable)(0,0) << Sub2("k","vs");
   else
      (*pParamTable)(0,0) << Sub2("k","s");
#endif

   (*pParamTable)(0,1) << Sub2("k","hs");
   (*pParamTable)(0,2) << Sub2("k","hc");
   (*pParamTable)(0,3) << Sub2("k","f");

   table->time.ShowUnitTag(true);
   (*pParamTable)(0,4) << Sub2("k","td") << rptNewLine << "t = " << table->time.SetValue(details.RefinedLosses2005.GetFinalAge());
   table->time.ShowUnitTag(false);

   (*pParamTable)(1,0) << table->scalar.SetValue(details.RefinedLosses2005.GetCreepInitialToFinal().GetKvs());
   (*pParamTable)(1,1) << table->scalar.SetValue(details.RefinedLosses2005.Getkhs());
   (*pParamTable)(1,2) << table->scalar.SetValue(details.RefinedLosses2005.GetCreepInitialToFinal().GetKhc());
   (*pParamTable)(1,3) << table->scalar.SetValue(details.RefinedLosses2005.GetCreepInitialToFinal().GetKf());
   (*pParamTable)(1,4) << table->scalar.SetValue(details.RefinedLosses2005.GetCreepInitialToFinal().GetKtd());

   // shrinkage loss   
   *pParagraph << table << rptNewLine;
   (*table)(0,0) << COLHDR("Location from"<<rptNewLine<<"Left Support",rptLengthUnitTag,  pDisplayUnits->GetSpanLengthUnit() );
   (*table)(0,1) << COLHDR(Sub2("A","ps"), rptAreaUnitTag, pDisplayUnits->GetAreaUnit());
   (*table)(0,2) << COLHDR(Sub2("A","c"), rptAreaUnitTag, pDisplayUnits->GetAreaUnit());
   (*table)(0,3) << COLHDR(Sub2("I","c"), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
   (*table)(0,4) << COLHDR(Sub2("e","pc"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
   (*table)(0,5) << Sub2("K","df");
   (*table)(0,6) << COLHDR(symbol(DELTA) << Sub2("f","pSD"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );

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
