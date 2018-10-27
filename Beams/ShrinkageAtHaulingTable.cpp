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

// ShrinkageAtHaulingTable.cpp : Implementation of CShrinkageAtHaulingTable
#include "stdafx.h"
#include "ShrinkageAtHaulingTable.h"
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\Intervals.h>
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
   DEFINE_UV_PROTOTYPE( time,        pDisplayUnits->GetWholeDaysUnit(),        false );

   scalar.SetFormat( sysNumericFormatTool::Automatic );
   scalar.SetWidth(6);
   scalar.SetPrecision(3);

   strain.SetFormat( sysNumericFormatTool::Automatic );
   strain.SetWidth(6);
   strain.SetPrecision(3);
}

CShrinkageAtHaulingTable* CShrinkageAtHaulingTable::PrepareTable(rptChapter* pChapter,IBroker* pBroker,const CSegmentKey& segmentKey,bool bTemporaryStrands,const LOSSDETAILS* pDetails,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   GET_IFACE2(pBroker,ISpecification,pSpec);
   std::_tstring strSpecName = pSpec->GetSpecification();

   GET_IFACE2(pBroker,ILibrary,pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( strSpecName.c_str() );

   GET_IFACE2(pBroker,ISectionProperties,pSectProp);
   pgsTypes::SectionPropertyMode spMode = pSectProp->GetSectionPropertiesMode();

   // Create and configure the table
   ColumnIndexType numColumns = 9;
   if ( bTemporaryStrands )
   {
      numColumns += 3;
   }

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);

   GET_IFACE2(pBroker,IGirder,pGirder);
   bool bIsPrismatic = pGirder->IsPrismatic(releaseIntervalIdx,segmentKey);

   if ( bIsPrismatic )
   {
      numColumns -= 2;
   }

   CShrinkageAtHaulingTable* table = new CShrinkageAtHaulingTable( numColumns, pDisplayUnits );
   rptStyleManager::ConfigureTable(table);

   table->m_bTemporaryStrands = bTemporaryStrands;
   table->m_bIsPrismatic = bIsPrismatic;


   std::_tstring strImagePath(rptStyleManager::GetImagePath());
   
   rptParagraph* pParagraph = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pParagraph;
   *pParagraph << _T("[") << LrfdCw8th(_T("5.9.5.4.2a"),_T("5.9.3.4.2a")) << _T("] Shrinkage of Girder Concrete : ") << symbol(DELTA) << RPT_STRESS(_T("pSRH")) << rptNewLine;

   pParagraph = new rptParagraph;
   *pChapter << pParagraph;

   if ( spMode == pgsTypes::spmGross )
   {
      *pParagraph << rptRcImage(strImagePath + _T("Delta_FpSRH_Gross.png")) << rptNewLine;
   }
   else
   {
      *pParagraph << rptRcImage(strImagePath + _T("Delta_FpSRH_Transformed.png")) << rptNewLine;
   }

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
      return table; // so we don't crash
   }

   if ( ptl->AdjustShrinkageStrain() )
   {
      // LRFD 5.4.2.3.3
      // If the concrete is exposed to drying before 5 days of curing have elapsed,
      // the shrinkage as determined in Eq 5.4.2.3.3-1 should be increased by 20%
      *pParagraph << _T("Girder is exposed to drying before 5 days of curing have elapsed, the shrinkage strain has been increased by 20% (LRFD 5.4.2.3.3)") << rptNewLine;
   }


   // parameters for calculations (two tables to keep the width printable)
   rptRcTable* paraTable = rptStyleManager::CreateDefaultTable(6,_T(""));
   *pParagraph << paraTable << rptNewLine;
   (*paraTable)(0,0) << _T("H") << rptNewLine << _T("(%)");
   (*paraTable)(0,1) << COLHDR(_T("V/S"),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*paraTable)(0,2) << COLHDR(RPT_FCI,rptStressUnitTag,pDisplayUnits->GetStressUnit());
   (*paraTable)(0,3) << COLHDR(Sub2(_T("t"),_T("i")),rptTimeUnitTag,pDisplayUnits->GetWholeDaysUnit());
   (*paraTable)(0,4) << COLHDR(Sub2(_T("t"),_T("h")),rptTimeUnitTag,pDisplayUnits->GetWholeDaysUnit());
   (*paraTable)(0,5) << COLHDR(Sub2(_T("t"),_T("f")),rptTimeUnitTag,pDisplayUnits->GetWholeDaysUnit());

   (*paraTable)(1,0) << ptl->GetRelHumidity();
   (*paraTable)(1,1) << table->ecc.SetValue(ptl->GetVolume()/ptl->GetSurfaceArea());
   (*paraTable)(1,2) << table->stress.SetValue(ptl->GetFci());
   (*paraTable)(1,3) << table->time.SetValue(ptl->GetInitialAge());
   (*paraTable)(1,4) << table->time.SetValue(ptl->GetAgeAtHauling());
   (*paraTable)(1,5) << table->time.SetValue(ptl->GetFinalAge());

   paraTable = rptStyleManager::CreateDefaultTable(8,_T(""));
   *pParagraph << paraTable << rptNewLine;
   paraTable->SetNumberOfHeaderRows(2);
   paraTable->SetRowSpan(0,0,2);
   paraTable->SetRowSpan(1,0,SKIP_CELL);
   (*paraTable)(0,0) << COLHDR(Sub2(_T("E"),_T("p")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
   paraTable->SetRowSpan(0,1,2);
   paraTable->SetRowSpan(1,1,SKIP_CELL);
   (*paraTable)(0,1) << COLHDR(Sub2(_T("E"),_T("ci")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
   paraTable->SetColumnSpan(0,2,3);
   paraTable->SetColumnSpan(0,3,SKIP_CELL);
   paraTable->SetColumnSpan(0,4,SKIP_CELL);
   (*paraTable)(0,2) << _T("Shrinkage");
   (*paraTable)(1,2) << Sub2(_T("K"),_T("1"));
   (*paraTable)(1,3) << Sub2(_T("K"),_T("2"));
   (*paraTable)(1,4) << Sub2(symbol(epsilon),_T("bih")) << _T("x 1000");
   paraTable->SetColumnSpan(0,5,3);
   paraTable->SetColumnSpan(0,6,SKIP_CELL);
   paraTable->SetColumnSpan(0,7,SKIP_CELL);
   (*paraTable)(0,5) << _T("Creep");
   (*paraTable)(1,5) << Sub2(_T("K"),_T("1"));
   (*paraTable)(1,6) << Sub2(_T("K"),_T("2"));
   (*paraTable)(1,7) << Sub2(symbol(psi),_T("b")) << _T("(") << Sub2(_T("t"),_T("f")) << _T(",") << Sub2(_T("t"),_T("i")) << _T(")");

   (*paraTable)(2,0) << table->mod_e.SetValue(ptl->GetEp());
   (*paraTable)(2,1) << table->mod_e.SetValue(ptl->GetEci());
   (*paraTable)(2,2) << ptl->GetGdrK1Shrinkage();
   (*paraTable)(2,3) << ptl->GetGdrK2Shrinkage();
   (*paraTable)(2,4) << table->strain.SetValue(ptl->Get_ebih() * 1000);
   (*paraTable)(2,5) << ptl->GetGdrK1Creep();
   (*paraTable)(2,6) << ptl->GetGdrK2Creep();
   (*paraTable)(2,7) << table->scalar.SetValue(ptl->GetCreepInitialToFinal().GetCreepCoefficient());


   // intermediate results
   paraTable = rptStyleManager::CreateDefaultTable(6,_T(""));
   *pParagraph << paraTable << rptNewLine;

    if ( lrfdVersionMgr::FourthEdition2007 <= pSpecEntry->GetSpecificationType() )
      (*paraTable)(0,0) << Sub2(_T("k"),_T("s"));
    else
      (*paraTable)(0,0) << Sub2(_T("k"),_T("vs"));

   (*paraTable)(0,1) << Sub2(_T("k"),_T("hs"));
   (*paraTable)(0,2) << Sub2(_T("k"),_T("hc"));
   (*paraTable)(0,3) << Sub2(_T("k"),_T("f"));

   table->time.ShowUnitTag(true);
   (*paraTable)(0,4) << Sub2(_T("k"),_T("td")) << rptNewLine << _T("Initial to Hauling") << rptNewLine << _T("t = ") << table->time.SetValue(ptl->GetCreepInitialToShipping().GetMaturity());
   (*paraTable)(0,5) << Sub2(_T("k"),_T("td")) << rptNewLine << _T("Initial to Final") << rptNewLine << _T("t = ") << table->time.SetValue(ptl->GetCreepInitialToFinal().GetMaturity());
   table->time.ShowUnitTag(false);

   (*paraTable)(1,0) << table->scalar.SetValue(ptl->GetCreepInitialToFinal().GetKvs());
   (*paraTable)(1,1) << table->scalar.SetValue(ptl->Getkhs());
   (*paraTable)(1,2) << table->scalar.SetValue(ptl->GetCreepInitialToFinal().GetKhc());
   (*paraTable)(1,3) << table->scalar.SetValue(ptl->GetCreepInitialToFinal().GetKf());
   (*paraTable)(1,4) << table->scalar.SetValue(ptl->GetCreepInitialToShipping().GetKtd());
   (*paraTable)(1,5) << table->scalar.SetValue(ptl->GetCreepInitialToFinal().GetKtd());

   // shrinkage loss   
   *pParagraph << table << rptNewLine;

   ColumnIndexType col = 0;
   (*table)(0,col++) << COLHDR(_T("Location from")<<rptNewLine<<_T("End of Girder"),rptLengthUnitTag,  pDisplayUnits->GetSpanLengthUnit() );
   (*table)(0,col++) << COLHDR(_T("Location from")<<rptNewLine<<_T("Left Support"),rptLengthUnitTag,  pDisplayUnits->GetSpanLengthUnit() );
   (*table)(0,col++) << COLHDR(Sub2(_T("A"),_T("ps")), rptAreaUnitTag, pDisplayUnits->GetAreaUnit());

   if ( !bIsPrismatic )
   {
      if (spMode == pgsTypes::spmGross )
      {
         (*table)(0,col++) << COLHDR(Sub2(_T("A"),_T("g")), rptAreaUnitTag, pDisplayUnits->GetAreaUnit());
         (*table)(0,col++) << COLHDR(Sub2(_T("I"),_T("g")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
      }
      else
      {
         (*table)(0,col++) << COLHDR(Sub2(_T("A"),_T("n")), rptAreaUnitTag, pDisplayUnits->GetAreaUnit());
         (*table)(0,col++) << COLHDR(Sub2(_T("I"),_T("n")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
      }
   }

   if ( spMode == pgsTypes::spmGross )
      (*table)(0,col++) << COLHDR(Sub2(_T("e"),_T("ps")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
   else
      (*table)(0,col++) << COLHDR(Sub2(_T("e"),_T("psn")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());

   if ( bTemporaryStrands )
   {
      table->SetNumberOfHeaderRows(2);

      col = 0;
      table->SetRowSpan(0,col,2);
      table->SetRowSpan(1,col++,SKIP_CELL);

      table->SetRowSpan(0,col,2);
      table->SetRowSpan(1,col++,SKIP_CELL);

      table->SetRowSpan(0,col,2);
      table->SetRowSpan(1,col++,SKIP_CELL);

      table->SetRowSpan(0,col,2);
      table->SetRowSpan(1,col++,SKIP_CELL);

      if ( !bIsPrismatic )
      {
         table->SetRowSpan(0,col,2);
         table->SetRowSpan(1,col++,SKIP_CELL);

         table->SetRowSpan(0,col,2);
         table->SetRowSpan(1,col++,SKIP_CELL);
      }

      table->SetColumnSpan(0,col,3);
      (*table)(0,col++) << _T("Permanent Strands");

      table->SetColumnSpan(0,col,3);
      (*table)(0,col++) << _T("Temporary Strands");

      for ( ColumnIndexType i = col; i < numColumns; i++ )
         table->SetColumnSpan(0,i,SKIP_CELL);

      // perm
      col -= 2;
      if ( spMode == pgsTypes::spmGross )
         (*table)(1,col++) << COLHDR(Sub2(_T("e"),_T("p")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      else
         (*table)(1,col++) << COLHDR(Sub2(_T("e"),_T("pn")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());

      (*table)(1,col++) << Sub2(_T("K"),_T("ih"));
      (*table)(1,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pSRH")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );

      // temp
      if ( spMode == pgsTypes::spmGross )
         (*table)(1,col++) << COLHDR(Sub2(_T("e"),_T("t")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      else
         (*table)(1,col++) << COLHDR(Sub2(_T("e"),_T("tn")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());

      (*table)(1,col++) << Sub2(_T("K"),_T("ih"));
      (*table)(1,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pSRH")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   }
   else
   {
      if ( spMode == pgsTypes::spmGross )
         (*table)(0,col++) << COLHDR(Sub2(_T("e"),_T("p")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      else
         (*table)(0,col++) << COLHDR(Sub2(_T("e"),_T("pn")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());

      (*table)(0,col++) << Sub2(_T("K"),_T("ih"));
      (*table)(0,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pSRH")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   }

   return table;
}

void CShrinkageAtHaulingTable::AddRow(rptChapter* pChapter,IBroker* pBroker,const pgsPointOfInterest& poi,RowIndexType row,const LOSSDETAILS* pDetails,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   RowIndexType rowOffset = GetNumberOfHeaderRows()-1;

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

   ColumnIndexType col = 2;
   (*this)(row+rowOffset,col++) << area.SetValue(Aps);

   if ( !m_bIsPrismatic )
   {
      (*this)(row+rowOffset,col++) << area.SetValue(pDetails->pLosses->GetAn());
      (*this)(row+rowOffset,col++) << mom_inertia.SetValue(pDetails->pLosses->GetIn());
   }

   (*this)(row+rowOffset,col++) << ecc.SetValue(pDetails->pLosses->GetEccpgFinal());

   (*this)(row+rowOffset,col++) << ecc.SetValue(pDetails->pLosses->GetEccPermanentFinal());
   (*this)(row+rowOffset,col++) << scalar.SetValue(ptl->GetPermanentStrandKih());
   (*this)(row+rowOffset,col++) << stress.SetValue( ptl->PermanentStrand_ShrinkageLossAtShipping() );
   
   if (m_bTemporaryStrands )
   {
      (*this)(row+rowOffset,col++) << ecc.SetValue(pDetails->pLosses->GetEccTemporary());
      (*this)(row+rowOffset,col++) << scalar.SetValue(ptl->GetTemporaryStrandKih());
      (*this)(row+rowOffset,col++) << stress.SetValue( ptl->TemporaryStrand_ShrinkageLossAtShipping() );
   }
}
