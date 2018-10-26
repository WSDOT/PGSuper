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

// ShrinkageAtHaulingTable.cpp : Implementation of CShrinkageAtHaulingTable
#include "stdafx.h"
#include "ShrinkageAtHaulingTable.h"
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <PsgLib\SpecLibraryEntry.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CShrinkageAtHaulingTable::CShrinkageAtHaulingTable(ColumnIndexType NumColumns, IEAFDisplayUnits* pDisplayUnits) :
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

CShrinkageAtHaulingTable* CShrinkageAtHaulingTable::PrepareTable(rptChapter* pChapter,IBroker* pBroker,SpanIndexType span,GirderIndexType gdr,bool bTemporaryStrands,LOSSDETAILS& details,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   GET_IFACE2(pBroker,ISpecification,pSpec);
   std::string strSpecName = pSpec->GetSpecification();

   GET_IFACE2(pBroker,ILibrary,pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( strSpecName.c_str() );

   // Create and configure the table
   ColumnIndexType numColumns = 9;
   if ( bTemporaryStrands )
      numColumns += 3;

   GET_IFACE2(pBroker,IGirder,pGirder);
   bool bIsPrismatic = pGirder->IsPrismatic(pgsTypes::CastingYard,span,gdr);

   if ( bIsPrismatic )
      numColumns -= 2;

   CShrinkageAtHaulingTable* table = new CShrinkageAtHaulingTable( numColumns, pDisplayUnits );
   pgsReportStyleHolder::ConfigureTable(table);

   table->m_bTemporaryStrands = bTemporaryStrands;
   table->m_bIsPrismatic = bIsPrismatic;


   std::string strImagePath(pgsReportStyleHolder::GetImagePath());
   
   rptParagraph* pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;
   *pParagraph << "[5.9.5.4.2a] Shrinkage of Girder Concrete : " << symbol(DELTA) << Sub2("f","pSRH") << rptNewLine;

   pParagraph = new rptParagraph;
   *pChapter << pParagraph;

   *pParagraph << rptRcImage(strImagePath + "Delta_FpSRH.gif") << rptNewLine;

   if ( pSpecEntry->GetSpecificationType() <= lrfdVersionMgr::ThirdEditionWith2005Interims )
   {
      if ( IS_SI_UNITS(pDisplayUnits) )
         *pParagraph << rptRcImage(strImagePath + "VSFactor_SI_2005.gif") << rptNewLine;
      else
         *pParagraph << rptRcImage(strImagePath + "VSFactor_US_2005.gif") << rptNewLine;
   }
#if defined IGNORE_2007_CHANGES
   else
   {
      if ( IS_SI_UNITS(pDisplayUnits) )
         *pParagraph << rptRcImage(strImagePath + "VSFactor_SI_2006.gif") << rptNewLine;
      else
         *pParagraph << rptRcImage(strImagePath + "VSFactor_US_2006.gif") << rptNewLine;
   }
#else
   else if ( pSpecEntry->GetSpecificationType() == lrfdVersionMgr::ThirdEditionWith2006Interims )
   {
      if ( pDisplayUnits->GetUnitDisplayMode() == pgsTypes::umSI )
         *pParagraph << rptRcImage(strImagePath + "VSFactor_SI_2006.gif") << rptNewLine;
      else
         *pParagraph << rptRcImage(strImagePath + "VSFactor_US_2006.gif") << rptNewLine;
   }
   else
   {
      if ( pDisplayUnits->GetUnitDisplayMode() == pgsTypes::umSI )
         *pParagraph << rptRcImage(strImagePath + "VSFactor_SI_2007.gif") << rptNewLine;
      else
         *pParagraph << rptRcImage(strImagePath + "VSFactor_US_2007.gif") << rptNewLine;
   }
#endif // IGNORE_2007_CHANGES
   *pParagraph << rptRcImage(strImagePath + "HumidityFactor.gif") << rptNewLine;
      if ( IS_SI_UNITS(pDisplayUnits) )
      *pParagraph << rptRcImage(strImagePath + "ConcreteFactors_SI.gif") << rptNewLine;
   else
      *pParagraph << rptRcImage(strImagePath + "ConcreteFactors_US.gif") << rptNewLine;

   // parameters for calculations (two tables to keep the width printable)
   rptRcTable* paraTable = pgsReportStyleHolder::CreateDefaultTable(6,"");
   *pParagraph << paraTable << rptNewLine;
   (*paraTable)(0,0) << "H" << rptNewLine << "(%)";
   (*paraTable)(0,1) << COLHDR("V/S",rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*paraTable)(0,2) << COLHDR(RPT_FCI,rptStressUnitTag,pDisplayUnits->GetStressUnit());
   (*paraTable)(0,3) << COLHDR(Sub2("t","i"),rptTimeUnitTag,pDisplayUnits->GetLongTimeUnit());
   (*paraTable)(0,4) << COLHDR(Sub2("t","h"),rptTimeUnitTag,pDisplayUnits->GetLongTimeUnit());
   (*paraTable)(0,5) << COLHDR(Sub2("t","f"),rptTimeUnitTag,pDisplayUnits->GetLongTimeUnit());

   (*paraTable)(1,0) << details.RefinedLosses2005.GetRelHumidity();
   (*paraTable)(1,1) << table->ecc.SetValue(details.RefinedLosses2005.GetVolume()/details.RefinedLosses2005.GetSurfaceArea());
   (*paraTable)(1,2) << table->stress.SetValue(details.RefinedLosses2005.GetFci());
   (*paraTable)(1,3) << table->time.SetValue(details.RefinedLosses2005.GetAdjustedInitialAge());
   (*paraTable)(1,4) << table->time.SetValue(details.RefinedLosses2005.GetAgeAtHauling());
   (*paraTable)(1,5) << table->time.SetValue(details.RefinedLosses2005.GetFinalAge());

   paraTable = pgsReportStyleHolder::CreateDefaultTable(4,"");
   *pParagraph << paraTable << rptNewLine;
   (*paraTable)(0,0) << COLHDR(Sub2("E","p"), rptStressUnitTag, pDisplayUnits->GetStressUnit());
   (*paraTable)(0,1) << COLHDR(Sub2("E","ci"), rptStressUnitTag, pDisplayUnits->GetStressUnit());
   (*paraTable)(0,2) << Sub2(symbol(epsilon),"bih") << "x 1000";
   (*paraTable)(0,3) << Sub2(symbol(psi),"b") << "(" << Sub2("t","f") << "," << Sub2("t","i") << ")";

   (*paraTable)(1,0) << table->mod_e.SetValue(details.RefinedLosses2005.GetEp());
   (*paraTable)(1,1) << table->mod_e.SetValue(details.RefinedLosses2005.GetEci());
   (*paraTable)(1,2) << table->strain.SetValue(details.RefinedLosses2005.Get_ebih() * 1000);
   (*paraTable)(1,3) << table->scalar.SetValue(details.RefinedLosses2005.GetCreepInitialToFinal().GetCreepCoefficient());


   // intermediate results
   paraTable = pgsReportStyleHolder::CreateDefaultTable(6,"");
   *pParagraph << paraTable << rptNewLine;

#if defined IGNORE_2007_CHANGES
   (*paraTable)(0,0) << Sub2("k","vs");
#else
   if ( pSpecEntry->GetSpecificationType() <= lrfdVersionMgr::ThirdEditionWith2006Interims )
      (*paraTable)(0,0) << Sub2("k","vs");
   else
      (*paraTable)(0,0) << Sub2("k","s");
#endif // IGNORE_2007_CHANGES

   (*paraTable)(0,1) << Sub2("k","hs");
   (*paraTable)(0,2) << Sub2("k","hc");
   (*paraTable)(0,3) << Sub2("k","f");

   table->time.ShowUnitTag(true);
   (*paraTable)(0,4) << Sub2("k","td") << rptNewLine << "t = " << table->time.SetValue(details.RefinedLosses2005.GetAgeAtHauling());
   (*paraTable)(0,5) << Sub2("k","td") << rptNewLine << "t = " << table->time.SetValue(details.RefinedLosses2005.GetFinalAge());
   table->time.ShowUnitTag(false);

   (*paraTable)(1,0) << table->scalar.SetValue(details.RefinedLosses2005.GetCreepInitialToFinal().GetKvs());
   (*paraTable)(1,1) << table->scalar.SetValue(details.RefinedLosses2005.Getkhs());
   (*paraTable)(1,2) << table->scalar.SetValue(details.RefinedLosses2005.GetCreepInitialToFinal().GetKhc());
   (*paraTable)(1,3) << table->scalar.SetValue(details.RefinedLosses2005.GetCreepInitialToFinal().GetKf());
   (*paraTable)(1,4) << table->scalar.SetValue(details.RefinedLosses2005.GetCreepInitialToShipping().GetKtd());
   (*paraTable)(1,5) << table->scalar.SetValue(details.RefinedLosses2005.GetCreepInitialToFinal().GetKtd());

   // shrinkage loss   
   *pParagraph << table << rptNewLine;

   ColumnIndexType col = 0;
   (*table)(0,col++) << COLHDR("Location from"<<rptNewLine<<"End of Girder",rptLengthUnitTag,  pDisplayUnits->GetSpanLengthUnit() );
   (*table)(0,col++) << COLHDR("Location from"<<rptNewLine<<"Left Support",rptLengthUnitTag,  pDisplayUnits->GetSpanLengthUnit() );
   (*table)(0,col++) << COLHDR(Sub2("A","ps"), rptAreaUnitTag, pDisplayUnits->GetAreaUnit());

   if ( !bIsPrismatic )
   {
      (*table)(0,col++) << COLHDR(Sub2("A","g"), rptAreaUnitTag, pDisplayUnits->GetAreaUnit());
      (*table)(0,col++) << COLHDR(Sub2("I","g"), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
   }

   (*table)(0,col++) << COLHDR(Sub2("e","ps"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());

   if ( bTemporaryStrands )
   {
      table->SetNumberOfHeaderRows(2);

      col = 0;
      table->SetRowSpan(0,col,2);
      table->SetRowSpan(1,col++,-1);

      table->SetRowSpan(0,col,2);
      table->SetRowSpan(1,col++,-1);

      table->SetRowSpan(0,col,2);
      table->SetRowSpan(1,col++,-1);

      table->SetRowSpan(0,col,2);
      table->SetRowSpan(1,col++,-1);

      if ( !bIsPrismatic )
      {
         table->SetRowSpan(0,col,2);
         table->SetRowSpan(1,col++,-1);

         table->SetRowSpan(0,col,2);
         table->SetRowSpan(1,col++,-1);
      }

      table->SetColumnSpan(0,col,3);
      (*table)(0,col++) << "Permanent Strands";

      table->SetColumnSpan(0,col,3);
      (*table)(0,col++) << "Temporary Strands";

      for ( ColumnIndexType i = col; i < numColumns; i++ )
         table->SetColumnSpan(0,i,-1);

      // perm
      col -= 2;
      (*table)(1,col++) << COLHDR(Sub2("e","p"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      (*table)(1,col++) << Sub2("K","ih");
      (*table)(1,col++) << COLHDR(symbol(DELTA) << Sub2("f","pSRH"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );

      // temp
      (*table)(1,col++) << COLHDR(Sub2("e","t"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      (*table)(1,col++) << Sub2("K","ih");
      (*table)(1,col++) << COLHDR(symbol(DELTA) << Sub2("f","pSRH"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   }
   else
   {
      (*table)(0,col++) << COLHDR(Sub2("e","p"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      (*table)(0,col++) << Sub2("K","ih");
      (*table)(0,col++) << COLHDR(symbol(DELTA) << Sub2("f","pSRH"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   }

   return table;
}

void CShrinkageAtHaulingTable::AddRow(rptChapter* pChapter,IBroker* pBroker,RowIndexType row,LOSSDETAILS& details,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   RowIndexType rowOffset = GetNumberOfHeaderRows()-1;

   Float64 Aps = details.pLosses->GetApsPermanent();
   if ( details.pLosses->GetTempStrandUsage() == lrfdLosses::tsPretensioned ||
        details.pLosses->GetTempStrandUsage() == lrfdLosses::tsPTBeforeLifting )
   {
      Aps += details.pLosses->GetApsTemporary();
   }

   ColumnIndexType col = 2;
   (*this)(row+rowOffset,col++) << area.SetValue(Aps);

   if ( !m_bIsPrismatic )
   {
      (*this)(row+rowOffset,col++) << area.SetValue(details.pLosses->GetAg());
      (*this)(row+rowOffset,col++) << mom_inertia.SetValue(details.pLosses->GetIg());
   }

   (*this)(row+rowOffset,col++) << ecc.SetValue(details.pLosses->GetEccpg());

   (*this)(row+rowOffset,col++) << ecc.SetValue(details.pLosses->GetEccPermanent());
   (*this)(row+rowOffset,col++) << scalar.SetValue(details.RefinedLosses2005.GetPermanentStrandKih());
   (*this)(row+rowOffset,col++) << stress.SetValue( details.RefinedLosses2005.PermanentStrand_ShrinkageLossAtShipping() );
   
   if (m_bTemporaryStrands )
   {
      (*this)(row+rowOffset,col++) << ecc.SetValue(details.pLosses->GetEccTemporary());
      (*this)(row+rowOffset,col++) << scalar.SetValue(details.RefinedLosses2005.GetTemporaryStrandKih());
      (*this)(row+rowOffset,col++) << stress.SetValue( details.RefinedLosses2005.TemporaryStrand_ShrinkageLossAtShipping() );
   }
}
