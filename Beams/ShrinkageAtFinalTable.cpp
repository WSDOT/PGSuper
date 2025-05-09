///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2025  Washington State Department of Transportation
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

#include <PgsExt\GirderMaterial.h>

#include <psgLib/SpecificationCriteria.h>

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
   DEFINE_UV_PROTOTYPE( time,        pDisplayUnits->GetWholeDaysUnit(),        false );

   scalar.SetFormat( WBFL::System::NumericFormatTool::Format::Automatic );
   scalar.SetWidth(6);
   scalar.SetPrecision(3);

   strain.SetFormat( WBFL::System::NumericFormatTool::Format::Automatic );
   strain.SetWidth(6);
   strain.SetPrecision(3);
}

CShrinkageAtFinalTable* CShrinkageAtFinalTable::PrepareTable(rptChapter* pChapter,IBroker* pBroker,const CSegmentKey& segmentKey,const LOSSDETAILS* pDetails,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   GET_IFACE2(pBroker,ISpecification,pSpec);
   std::_tstring strSpecName = pSpec->GetSpecification();

   GET_IFACE2(pBroker,ILibrary,pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( strSpecName.c_str() );

   GET_IFACE2(pBroker,ISectionProperties,pSectProp);
   pgsTypes::SectionPropertyMode spMode = pSectProp->GetSectionPropertiesMode();

   GET_IFACE2(pBroker, ISegmentData, pSegmentData);

   // Create and configure the table
   ColumnIndexType numColumns = 7;
   CShrinkageAtFinalTable* table = new CShrinkageAtFinalTable( numColumns, pDisplayUnits );
   rptStyleManager::ConfigureTable(table);

   std::_tstring strImagePath(rptStyleManager::GetImagePath());

   rptParagraph* pParagraph = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pParagraph;
   *pParagraph << _T("[") << WBFL::LRFD::LrfdCw8th(_T("5.9.5.4.3a"),_T("5.9.3.4.3a")) << _T("] Shrinkage of Girder Concrete : ") << symbol(DELTA) << RPT_STRESS(_T("pSD")) << rptNewLine;

   pParagraph = new rptParagraph;
   *pChapter << pParagraph;

   *pParagraph << rptRcImage(strImagePath + _T("Delta_fpSD.png")) << rptNewLine;

   if ( spMode == pgsTypes::spmGross )
   {
      *pParagraph << rptRcImage(strImagePath + _T("Kdf_Gross.png")) << rptNewLine;
   }
   else
   {
      *pParagraph << rptRcImage(strImagePath + _T("Kdf_Transformed.png")) << rptNewLine;
   }

   if (pSegmentData->GetSegmentMaterial(segmentKey)->Concrete.Type == pgsTypes::PCI_UHPC)
   {
      if (pSegmentData->GetSegmentMaterial(segmentKey)->Concrete.bPCTT)
         *pParagraph << rptRcImage(strImagePath + _T("CreepShrinkageAtFinal_PCI_UHPC_PCTT.png")) << rptNewLine;
      else
         *pParagraph << rptRcImage(strImagePath + _T("CreepShrinkageAtFinal_PCI_UHPC.png")) << rptNewLine;

      *pParagraph << rptRcImage(strImagePath + _T("PCI_UHPC_Factors.png")) << rptNewLine;
   }
   else if (pSegmentData->GetSegmentMaterial(segmentKey)->Concrete.Type == pgsTypes::UHPC)
   {
      *pParagraph << rptRcImage(strImagePath + _T("CreepShrinkageAtFinal_UHPC.png")) << rptNewLine;
      *pParagraph << rptRcImage(strImagePath + _T("UHPC_Factors.png")) << rptNewLine;
   }
   else
   {
      *pParagraph << rptRcImage(strImagePath + _T("CreepShrinkageAtFinal.png")) << rptNewLine;

      if (pSpecEntry->GetSpecificationCriteria().GetEdition() <= WBFL::LRFD::BDSManager::Edition::ThirdEditionWith2005Interims)
      {
         if (IS_SI_UNITS(pDisplayUnits))
         {
            *pParagraph << rptRcImage(strImagePath + _T("KvsEqn-SI.png")) << rptNewLine;
         }
         else
         {
            *pParagraph << rptRcImage(strImagePath + _T("KvsEqn-US.png")) << rptNewLine;
         }
      }
      else if (pSpecEntry->GetSpecificationCriteria().GetEdition() == WBFL::LRFD::BDSManager::Edition::ThirdEditionWith2006Interims)
      {
         if (IS_SI_UNITS(pDisplayUnits))
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
         if (IS_SI_UNITS(pDisplayUnits))
         {
            *pParagraph << rptRcImage(strImagePath + _T("KvsEqn2007-SI.png")) << rptNewLine;
         }
         else
         {
            *pParagraph << rptRcImage(strImagePath + _T("KvsEqn2007-US.png")) << rptNewLine;
         }
      }

      *pParagraph << rptRcImage(strImagePath + _T("HumidityFactor.png")) << rptNewLine;
      if (IS_SI_UNITS(pDisplayUnits))
      {
         ATLASSERT(pSpecEntry->GetSpecificationCriteria().GetEdition() < WBFL::LRFD::BDSManager::Edition::SeventhEditionWith2015Interims);
         *pParagraph << rptRcImage(strImagePath + _T("ConcreteFactors_SI.png")) << rptNewLine;
      }
      else
      {
         if (pSpecEntry->GetSpecificationCriteria().GetEdition() < WBFL::LRFD::BDSManager::Edition::SeventhEditionWith2015Interims)
         {
            *pParagraph << rptRcImage(strImagePath + _T("ConcreteFactors_US.png")) << rptNewLine;
         }
         else
         {
            *pParagraph << rptRcImage(strImagePath + _T("ConcreteFactors_US2015.png")) << rptNewLine;
         }
      }
   }
   
  // Typecast to our known type (eating own doggy food)
   std::shared_ptr<const WBFL::LRFD::RefinedLosses2005> ptl = std::dynamic_pointer_cast<const WBFL::LRFD::RefinedLosses2005>(pDetails->pLosses);
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
   rptRcTable* pParamTable = rptStyleManager::CreateDefaultTable(5,_T(""));
   *pParagraph << pParamTable << rptNewLine;
   (*pParamTable)(0,0) << _T("H") << rptNewLine << _T("(%)");
   (*pParamTable)(0,1) << COLHDR(_T("V/S"),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*pParamTable)(0,2) << COLHDR(RPT_FCI,rptStressUnitTag,pDisplayUnits->GetStressUnit());
   (*pParamTable)(0,3) << COLHDR(Sub2(_T("t"),_T("i")),rptTimeUnitTag,pDisplayUnits->GetWholeDaysUnit());
   (*pParamTable)(0,4) << COLHDR(Sub2(_T("t"),_T("f")),rptTimeUnitTag,pDisplayUnits->GetWholeDaysUnit());

   (*pParamTable)(1,0) << ptl->GetRelHumidity();
   (*pParamTable)(1,1) << table->ecc.SetValue(ptl->GetGirderCreep()->GetVolume()/ptl->GetGirderCreep()->GetSurfaceArea());
   (*pParamTable)(1,2) << table->stress.SetValue(ptl->GetFci());
   (*pParamTable)(1,3) << table->time.SetValue(ptl->GetInitialAge());
   (*pParamTable)(1,4) << table->time.SetValue(ptl->GetFinalAge());

   pParamTable = rptStyleManager::CreateDefaultTable(10,_T(""));
   *pParagraph << pParamTable << rptNewLine;
   pParamTable->SetNumberOfHeaderRows(2);
   pParamTable->SetRowSpan(0,0,2);
   (*pParamTable)(0,0) << COLHDR(Sub2(_T("E"),_T("p")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
   pParamTable->SetRowSpan(0,1,2);
   (*pParamTable)(0,1) << COLHDR(Sub2(_T("E"),_T("ci")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
   pParamTable->SetColumnSpan(0,2,5);
   (*pParamTable)(0,2) << _T("Shrinkage");
   (*pParamTable)(1,2) << Sub2(_T("K"),_T("1"));
   (*pParamTable)(1,3) << Sub2(_T("K"),_T("2"));
   (*pParamTable)(1,4) << Sub2(symbol(epsilon),_T("bif")) << _T("x1000");
   (*pParamTable)(1,5) << Sub2(symbol(epsilon),_T("bid")) << _T("x1000");
   (*pParamTable)(1,6) << Sub2(symbol(epsilon),_T("bdf")) << _T("x1000");
   pParamTable->SetColumnSpan(0,7,3);
   (*pParamTable)(0,7) << _T("Creep");
   (*pParamTable)(1,7) << Sub2(_T("K"),_T("1"));
   (*pParamTable)(1,8) << Sub2(_T("K"),_T("2"));
   (*pParamTable)(1,9) << Sub2(symbol(psi),_T("b")) << _T("(") << Sub2(_T("t"),_T("f")) << _T(",") << Sub2(_T("t"),_T("i")) << _T(")");

   (*pParamTable)(2,0) << table->mod_e.SetValue(ptl->GetEp());
   (*pParamTable)(2,1) << table->mod_e.SetValue(ptl->GetEci());
   (*pParamTable)(2,2) << ptl->GetGdrK1Shrinkage();
   (*pParamTable)(2,3) << ptl->GetGdrK2Shrinkage();
   (*pParamTable)(2,4) << table->strain.SetValue(ptl->Get_ebif() * 1000);
   (*pParamTable)(2,5) << table->strain.SetValue(ptl->Get_ebid() * 1000);
   (*pParamTable)(2,6) << table->strain.SetValue(ptl->Get_ebdf() * 1000);
   (*pParamTable)(2,7) << ptl->GetGirderCreep()->GetK1();
   (*pParamTable)(2, 8) << ptl->GetGirderCreep()->GetK2();
   (*pParamTable)(2,9) << table->creep.SetValue(ptl->GetGirderCreep()->GetCreepCoefficient(ptl->GetMaturityAtFinal(),ptl->GetInitialAge()));

   // intermediate results
   pParamTable = rptStyleManager::CreateDefaultTable(pSegmentData->GetSegmentMaterial(segmentKey)->Concrete.Type == pgsTypes::UHPC ? 6 : 5,_T(""));
   *pParagraph << pParamTable << rptNewLine;

   if ( WBFL::LRFD::BDSManager::Edition::FourthEdition2007 <= pSpecEntry->GetSpecificationCriteria().GetEdition() )
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
   (*pParamTable)(0,4) << Sub2(_T("k"),_T("td")) << rptNewLine << _T("t = ") << table->time.SetValue(ptl->GetMaturityAtFinal());
   table->time.ShowUnitTag(false);

   if (pSegmentData->GetSegmentMaterial(segmentKey)->Concrete.Type == pgsTypes::UHPC)
   {
      (*pParamTable)(0, 5) << Sub2(_T("k"), _T("l"));
   }

   (*pParamTable)(1,0) << table->scalar.SetValue(ptl->GetGirderCreep()->GetKvs());
   (*pParamTable)(1,1) << table->scalar.SetValue(ptl->Getkhs_Girder());
   (*pParamTable)(1,2) << table->scalar.SetValue(ptl->GetGirderCreep()->GetKhc());
   (*pParamTable)(1,3) << table->scalar.SetValue(ptl->GetGirderCreep()->GetKf());
   (*pParamTable)(1,4) << table->scalar.SetValue(ptl->GetGirderCreep()->GetKtd(ptl->GetMaturityAtFinal()));
   if (pSegmentData->GetSegmentMaterial(segmentKey)->Concrete.Type == pgsTypes::UHPC)
   {
      (*pParamTable)(1, 5) << table->scalar.SetValue(ptl->GetGirderCreep()->GetKl(ptl->GetInitialAge()));
   }
   // shrinkage loss   
   *pParagraph << table << rptNewLine;
   (*table)(0,0) << COLHDR(_T("Location from")<<rptNewLine<<_T("Left Support"),rptLengthUnitTag,  pDisplayUnits->GetSpanLengthUnit() );
   (*table)(0,1) << COLHDR(Sub2(_T("A"),_T("ps")), rptAreaUnitTag, pDisplayUnits->GetAreaUnit());
   if ( spMode == pgsTypes::spmGross )
   {
      (*table)(0,2) << COLHDR(Sub2(_T("A"),_T("c")), rptAreaUnitTag, pDisplayUnits->GetAreaUnit());
      (*table)(0,3) << COLHDR(Sub2(_T("I"),_T("c")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
      (*table)(0,4) << COLHDR(Sub2(_T("e"),_T("pc")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
   }
   else
   {
      (*table)(0,2) << COLHDR(Sub2(_T("A"),_T("cn")), rptAreaUnitTag, pDisplayUnits->GetAreaUnit());
      (*table)(0,3) << COLHDR(Sub2(_T("I"),_T("cn")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
      (*table)(0,4) << COLHDR(Sub2(_T("e"),_T("pcn")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
   }
   (*table)(0,5) << Sub2(_T("K"),_T("df"));
   (*table)(0,6) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pSD")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );

   return table;
}

void CShrinkageAtFinalTable::AddRow(rptChapter* pChapter,IBroker* pBroker,const pgsPointOfInterest& poi,RowIndexType row,const LOSSDETAILS* pDetails,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   // Typecast to our known type (eating own doggy food)
   std::shared_ptr<const WBFL::LRFD::RefinedLosses2005> ptl = std::dynamic_pointer_cast<const WBFL::LRFD::RefinedLosses2005>(pDetails->pLosses);
   if (!ptl)
   {
      ATLASSERT(false); // made a bad cast? Bail...
      return;
   }

   Float64 Acn, Ybcn, Icn;
   pDetails->pLosses->GetNetCompositeProperties(&Acn, &Ybcn, &Icn);

   ColumnIndexType col = 1;
   RowIndexType rowOffset = GetNumberOfHeaderRows() - 1;

   (*this)(row+rowOffset,col++) << area.SetValue(pDetails->pLosses->GetApsPermanent());
   (*this)(row+rowOffset,col++) << area.SetValue(Acn);
   (*this)(row+rowOffset,col++) << mom_inertia.SetValue(Icn);
   (*this)(row+rowOffset,col++) << ecc.SetValue(pDetails->pLosses->GetEccpc());
   (*this)(row+rowOffset,col++) << scalar.SetValue(ptl->GetKdf());
   (*this)(row+rowOffset,col++) << stress.SetValue(ptl->ShrinkageLossAfterDeckPlacement());
}
