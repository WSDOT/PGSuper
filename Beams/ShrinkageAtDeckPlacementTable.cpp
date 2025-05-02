///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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
#include "Beams.h"
#include "ShrinkageAtDeckPlacementTable.h"
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\Intervals.h>
#include <IFace/PointOfInterest.h>

#include <PsgLib\SpecLibraryEntry.h>
#include <psgLib/SpecificationCriteria.h>
#include <PsgLib\GirderMaterial.h>


CShrinkageAtDeckPlacementTable::CShrinkageAtDeckPlacementTable(ColumnIndexType NumColumns, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) :
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
   scalar.SetWidth(7);
   scalar.SetPrecision(3);

   strain.SetFormat( WBFL::System::NumericFormatTool::Format::Automatic );
   strain.SetWidth(6);
   strain.SetPrecision(3);
}

CShrinkageAtDeckPlacementTable* CShrinkageAtDeckPlacementTable::PrepareTable(rptChapter* pChapter,std::shared_ptr<WBFL::EAF::Broker> pBroker,const CSegmentKey& segmentKey,const LOSSDETAILS* pDetails,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits,Uint16 level)
{
   GET_IFACE2(pBroker,ISpecification,pSpec);
   std::_tstring strSpecName = pSpec->GetSpecification();

   GET_IFACE2(pBroker,ILibrary,pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( strSpecName.c_str() );

   GET_IFACE2(pBroker,ISectionProperties,pSectProp);
   pgsTypes::SectionPropertyMode spMode = pSectProp->GetSectionPropertiesMode();

   GET_IFACE2(pBroker, IIntervals, pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);

   GET_IFACE2(pBroker, IGirder, pGirder);
   bool bIsPrismatic = pGirder->IsPrismatic(releaseIntervalIdx, segmentKey);

   GET_IFACE2(pBroker, IBridge, pBridge);
   bool bIsAsymmetric = pBridge->HasAsymmetricGirders() || pBridge->HasAsymmetricPrestressing() ? true : false;

   GET_IFACE2(pBroker, ISegmentData, pSegmentData);

   // Create and configure the table
   ColumnIndexType numColumns = 2; // location, Aps
   if (bIsPrismatic)
   {
      if (bIsAsymmetric)
      {
         numColumns += 2; // epsx, epsy
      }
      else
      {
         numColumns++; // eps
      }
   }
   else
   {
      if (bIsAsymmetric)
      {
         numColumns += 6; // Ag, Ixx, Ixy, Ixy, epsx, epsy
      }
      else
      {
         numColumns += 3; // Ag, Ig, eps
      }
   }

   // permanent strands
   if (bIsAsymmetric)
   {
      numColumns += 4; // epx, epy, Kih, dfpSR
   }
   else
   {
      numColumns += 3; // ep, Kih, dfpSR
   }

   // Create and configure the table
   CShrinkageAtDeckPlacementTable* table = new CShrinkageAtDeckPlacementTable( numColumns, pDisplayUnits );
   rptStyleManager::ConfigureTable(table);

   table->m_bIsPrismatic = bIsPrismatic;
   table->m_bIsAsymmetric = bIsAsymmetric;


   std::_tstring strImagePath(rptStyleManager::GetImagePath());
   
   rptParagraph* pParagraph = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pParagraph;

   *pParagraph << _T("[") << WBFL::LRFD::LrfdCw8th(_T("5.9.5.4.2a"),_T("5.9.3.4.2a")) << _T("] Shrinkage of Girder Concrete : ") << symbol(DELTA) << RPT_STRESS(_T("pSR")) << rptNewLine;

   pParagraph = new rptParagraph;
   *pChapter << pParagraph;

   *pParagraph << rptRcImage(strImagePath + _T("Delta_fpSR.png")) << rptNewLine;

   if (bIsAsymmetric)
   {
      if (spMode == pgsTypes::spmGross)
      {
         *pParagraph << rptRcImage(strImagePath + _T("Kid_Gross_Asymmetric.png")) << rptNewLine;
      }
      else
      {
         *pParagraph << rptRcImage(strImagePath + _T("Kid_Transformed_Asymmetric.png")) << rptNewLine;
      }
   }
   else
   {
      if (spMode == pgsTypes::spmGross)
      {
         *pParagraph << rptRcImage(strImagePath + _T("Kid_Gross.png")) << rptNewLine;
      }
      else
      {
         *pParagraph << rptRcImage(strImagePath + _T("Kid_Transformed.png")) << rptNewLine;
      }
   }

   if (pSegmentData->GetSegmentMaterial(segmentKey)->Concrete.Type == pgsTypes::PCI_UHPC)
   {
      if (pSegmentData->GetSegmentMaterial(segmentKey)->Concrete.bPCTT)
         *pParagraph << rptRcImage(strImagePath + _T("CreepShrinkageAtDeckPlacement_PCI_UHPC_PCTT.png")) << rptNewLine;
      else
         *pParagraph << rptRcImage(strImagePath + _T("CreepShrinkageAtDeckPlacement_PCI_UHPC.png")) << rptNewLine;

      *pParagraph << rptRcImage(strImagePath + _T("PCI_UHPC_Factors.png")) << rptNewLine;
   }
   else if (pSegmentData->GetSegmentMaterial(segmentKey)->Concrete.Type == pgsTypes::UHPC)
   {
      *pParagraph << rptRcImage(strImagePath + _T("CreepShrinkageAtDeckPlacement_UHPC.png")) << rptNewLine;
      *pParagraph << rptRcImage(strImagePath + _T("UHPC_Factors.png")) << rptNewLine;
   }
   else
   {
      *pParagraph << rptRcImage(strImagePath + _T("CreepShrinkageAtDeckPlacement.png")) << rptNewLine;

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
   rptRcTable* pParamTable = rptStyleManager::CreateDefaultTable(6,_T(""));
   *pParagraph << pParamTable << rptNewLine;
   (*pParamTable)(0,0) << _T("H") << rptNewLine << _T("(%)");
   (*pParamTable)(0,1) << COLHDR(_T("V/S"),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*pParamTable)(0,2) << COLHDR(RPT_FCI,rptStressUnitTag,pDisplayUnits->GetStressUnit());
   (*pParamTable)(0,3) << COLHDR(Sub2(_T("t"),_T("i")),rptTimeUnitTag,pDisplayUnits->GetWholeDaysUnit());
   (*pParamTable)(0,4) << COLHDR(Sub2(_T("t"),_T("d")),rptTimeUnitTag,pDisplayUnits->GetWholeDaysUnit());
   (*pParamTable)(0,5) << COLHDR(Sub2(_T("t"),_T("f")),rptTimeUnitTag,pDisplayUnits->GetWholeDaysUnit());

   (*pParamTable)(1,0) << ptl->GetRelHumidity();
   (*pParamTable)(1,1) << table->ecc.SetValue(ptl->GetGirderCreep()->GetVolume()/ptl->GetGirderCreep()->GetSurfaceArea());
   (*pParamTable)(1,2) << table->stress.SetValue(ptl->GetFci());
   (*pParamTable)(1,3) << table->time.SetValue(ptl->GetInitialAge());
   (*pParamTable)(1,4) << table->time.SetValue(ptl->GetAgeAtDeckPlacement());
   (*pParamTable)(1,5) << table->time.SetValue(ptl->GetFinalAge());

   // intermediate results
   pParamTable = rptStyleManager::CreateDefaultTable(pSegmentData->GetSegmentMaterial(segmentKey)->Concrete.Type == pgsTypes::UHPC ? 7 : 6,_T(""));
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
   (*pParamTable)(0,4) << Sub2(_T("k"),_T("td")) << rptNewLine << _T("Initial to Deck Placement") << rptNewLine << _T("t = ") << table->time.SetValue(ptl->GetMaturityAtDeckPlacement());
   (*pParamTable)(0,5) << Sub2(_T("k"),_T("td")) << rptNewLine << _T("Initial to Final") << rptNewLine << _T("t = ") << table->time.SetValue(ptl->GetMaturityAtFinal());
   table->time.ShowUnitTag(false);

   if (pSegmentData->GetSegmentMaterial(segmentKey)->Concrete.Type == pgsTypes::UHPC)
   {
      (*pParamTable)(0, 6) << Sub2(_T("k"), _T("l"));
   }

   (*pParamTable)(1,0) << table->scalar.SetValue(ptl->GetGirderCreep()->GetKvs());
   (*pParamTable)(1,1) << table->scalar.SetValue(ptl->Getkhs_Girder());
   (*pParamTable)(1,2) << table->scalar.SetValue(ptl->GetGirderCreep()->GetKhc());
   (*pParamTable)(1,3) << table->scalar.SetValue(ptl->GetGirderCreep()->GetKf());
   (*pParamTable)(1,4) << table->scalar.SetValue(ptl->GetGirderCreep()->GetKtd(ptl->GetMaturityAtDeckPlacement()));
   (*pParamTable)(1,5) << table->scalar.SetValue(ptl->GetGirderCreep()->GetKtd(ptl->GetMaturityAtFinal()));

   if (pSegmentData->GetSegmentMaterial(segmentKey)->Concrete.Type == pgsTypes::UHPC)
   {
      (*pParamTable)(1, 6) << table->scalar.SetValue(ptl->GetGirderCreep()->GetKl(ptl->GetInitialAge()));
   }

   pParamTable = rptStyleManager::CreateDefaultTable(8,_T(""));
   *pParagraph << pParamTable << rptNewLine;
   pParamTable->SetNumberOfHeaderRows(2);
   pParamTable->SetRowSpan(0,0,2);

   (*pParamTable)(0,0) << COLHDR(Sub2(_T("E"),_T("p")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
   pParamTable->SetRowSpan(0,1,2);
   (*pParamTable)(0,1) << COLHDR(Sub2(_T("E"),_T("ci")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
   pParamTable->SetColumnSpan(0,2,3);
   (*pParamTable)(0,2) << _T("Shrinkage");
   (*pParamTable)(1,2) << Sub2(_T("K"),_T("1"));
   (*pParamTable)(1,3) << Sub2(_T("K"),_T("2"));
   (*pParamTable)(1,4) << Sub2(symbol(epsilon),_T("bid")) << _T("x 1000");
   pParamTable->SetColumnSpan(0,5,3);
   (*pParamTable)(0,5) << _T("Creep");
   (*pParamTable)(1,5) << Sub2(_T("K"),_T("1"));
   (*pParamTable)(1,6) << Sub2(_T("K"),_T("2"));
   (*pParamTable)(1,7) << Sub2(symbol(psi),_T("b")) << _T("(") << Sub2(_T("t"),_T("f")) << _T(",") << Sub2(_T("t"),_T("i")) << _T(")");

   (*pParamTable)(2,0) << table->mod_e.SetValue(ptl->GetEp());
   (*pParamTable)(2,1) << table->mod_e.SetValue(ptl->GetEci());
   (*pParamTable)(2,2) << ptl->GetGdrK1Shrinkage();
   (*pParamTable)(2,3) << ptl->GetGdrK2Shrinkage();
   (*pParamTable)(2,4) << table->strain.SetValue(ptl->Get_ebid() * 1000);
   (*pParamTable)(2,5) << ptl->GetGirderCreep()->GetK1();
   (*pParamTable)(2,6) << ptl->GetGirderCreep()->GetK2();
   (*pParamTable)(2, 7) << table->creep.SetValue(ptl->GetGirderCreep()->GetCreepCoefficient(ptl->GetMaturityAtFinal(),ptl->GetInitialAge()));

   if (bIsPrismatic)
   {
      rptRcTable* sectPropTable = rptStyleManager::CreateDefaultTable(bIsAsymmetric ? 4 : 2, _T(""));
      *pParagraph << sectPropTable << rptNewLine;

      GET_IFACE2(pBroker, IPointOfInterest, pPoi);
      PoiList vPoi;
      pPoi->GetPointsOfInterest(segmentKey, POI_5L | POI_RELEASED_SEGMENT, &vPoi);
      ATLASSERT(vPoi.size() == 1);
      const pgsPointOfInterest& poi(vPoi.front());
      Float64 Ag = pSectProp->GetAg(releaseIntervalIdx, poi);
      Float64 Ixx = pSectProp->GetIxx(releaseIntervalIdx, poi);

      if (bIsAsymmetric)
      {
         Float64 Iyy = pSectProp->GetIyy(releaseIntervalIdx, poi);
         Float64 Ixy = pSectProp->GetIxy(releaseIntervalIdx, poi);
         if (spMode == pgsTypes::spmGross)
         {
            (*sectPropTable)(0, 0) << COLHDR(Sub2(_T("A"), _T("g")), rptAreaUnitTag, pDisplayUnits->GetAreaUnit());
            (*sectPropTable)(0, 1) << COLHDR(Sub2(_T("I"), _T("xx")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
            (*sectPropTable)(0, 2) << COLHDR(Sub2(_T("I"), _T("yy")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
            (*sectPropTable)(0, 3) << COLHDR(Sub2(_T("I"), _T("xy")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
         }
         else
         {
            (*sectPropTable)(0, 0) << COLHDR(Sub2(_T("A"), _T("gt")), rptAreaUnitTag, pDisplayUnits->GetAreaUnit());
            (*sectPropTable)(0, 1) << COLHDR(Sub2(_T("I"), _T("xxt")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
            (*sectPropTable)(0, 2) << COLHDR(Sub2(_T("I"), _T("yyt")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
            (*sectPropTable)(0, 3) << COLHDR(Sub2(_T("I"), _T("xyt")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
         }

         (*sectPropTable)(1, 0) << table->area.SetValue(Ag);
         (*sectPropTable)(1, 1) << table->mom_inertia.SetValue(Ixx);
         (*sectPropTable)(1, 2) << table->mom_inertia.SetValue(Iyy);
         (*sectPropTable)(1, 3) << table->mom_inertia.SetValue(Ixy);
      }
      else
      {
         if (spMode == pgsTypes::spmGross)
         {
            (*sectPropTable)(0, 0) << COLHDR(Sub2(_T("A"), _T("g")), rptAreaUnitTag, pDisplayUnits->GetAreaUnit());
            (*sectPropTable)(0, 1) << COLHDR(Sub2(_T("I"), _T("g")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
         }
         else
         {
            (*sectPropTable)(0, 0) << COLHDR(Sub2(_T("A"), _T("gt")), rptAreaUnitTag, pDisplayUnits->GetAreaUnit());
            (*sectPropTable)(0, 1) << COLHDR(Sub2(_T("I"), _T("gt")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
         }

         (*sectPropTable)(1, 0) << table->area.SetValue(Ag);
         (*sectPropTable)(1, 1) << table->mom_inertia.SetValue(Ixx);
      }
   }

   // shrinkage loss   
   *pParagraph << table << rptNewLine;
   ColumnIndexType col = 0;
   (*table)(0,col++) << COLHDR(_T("Location from")<<rptNewLine<<_T("Left Support"),rptLengthUnitTag,  pDisplayUnits->GetSpanLengthUnit() );
   (*table)(0,col++) << COLHDR(Sub2(_T("A"),_T("ps")), rptAreaUnitTag, pDisplayUnits->GetAreaUnit());
   if (bIsPrismatic)
   {
      if (bIsAsymmetric)
      {
         if (spMode == pgsTypes::spmGross)
         {
            (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("psx")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
            (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("psy")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
         }
         else
         {
            (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("psxn")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
            (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("psyn")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
         }
      }
      else
      {
         if (spMode == pgsTypes::spmGross)
         {
            (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("ps")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
         }
         else
         {
            (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("psn")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
         }
      }
   }
   else
   {
      if (bIsAsymmetric)
      {
         if (spMode == pgsTypes::spmGross)
         {
            (*table)(0, col++) << COLHDR(Sub2(_T("A"), _T("g")), rptAreaUnitTag, pDisplayUnits->GetAreaUnit());
            (*table)(0, col++) << COLHDR(Sub2(_T("I"), _T("xx")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
            (*table)(0, col++) << COLHDR(Sub2(_T("I"), _T("yy")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
            (*table)(0, col++) << COLHDR(Sub2(_T("I"), _T("xy")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
            (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("psx")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
            (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("psy")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
         }
         else
         {
            (*table)(0, col++) << COLHDR(Sub2(_T("A"), _T("n")), rptAreaUnitTag, pDisplayUnits->GetAreaUnit());
            (*table)(0, col++) << COLHDR(Sub2(_T("I"), _T("xxn")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
            (*table)(0, col++) << COLHDR(Sub2(_T("I"), _T("yyn")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
            (*table)(0, col++) << COLHDR(Sub2(_T("I"), _T("xyn")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
            (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("psxn")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
            (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("psyn")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
         }
      }
      else
      {
         if (spMode == pgsTypes::spmGross)
         {
            (*table)(0, col++) << COLHDR(Sub2(_T("A"), _T("g")), rptAreaUnitTag, pDisplayUnits->GetAreaUnit());
            (*table)(0, col++) << COLHDR(Sub2(_T("I"), _T("g")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
            (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("ps")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
         }
         else
         {
            (*table)(0, col++) << COLHDR(Sub2(_T("A"), _T("n")), rptAreaUnitTag, pDisplayUnits->GetAreaUnit());
            (*table)(0, col++) << COLHDR(Sub2(_T("I"), _T("n")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
            (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("psn")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
         }
      }
   }

   if (bIsAsymmetric)
   {
      if (spMode == pgsTypes::spmGross)
      {
         (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("px")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
         (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("py")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      }
      else
      {
         (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("pxn")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
         (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("pyn")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      }
   }
   else
   {
      if (spMode == pgsTypes::spmGross)
      {
         (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("p")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      }
      else
      {
         (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("pn")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      }
   }

   (*table)(0, col++) << Sub2(_T("K"), _T("id"));
   (*table)(0, col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pSR")), rptStressUnitTag, pDisplayUnits->GetStressUnit());


   return table;
}

void CShrinkageAtDeckPlacementTable::AddRow(rptChapter* pChapter,std::shared_ptr<WBFL::EAF::Broker> pBroker,const pgsPointOfInterest& poi,RowIndexType row,const LOSSDETAILS* pDetails,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits,Uint16 level)
{
   // Typecast to our known type (eating own doggy food)
   std::shared_ptr<const WBFL::LRFD::RefinedLosses2005> ptl = std::dynamic_pointer_cast<const WBFL::LRFD::RefinedLosses2005>(pDetails->pLosses);
   if (!ptl)
   {
      ATLASSERT(false); // made a bad cast? Bail...
      return;
   }

   Float64 Aps = pDetails->pLosses->GetApsPermanent();
   if ( pDetails->pLosses->GetTempStrandUsage() == WBFL::LRFD::Losses::TempStrandUsage::Pretensioned ||
        pDetails->pLosses->GetTempStrandUsage() == WBFL::LRFD::Losses::TempStrandUsage::PTBeforeLifting )
   {
      Aps += pDetails->pLosses->GetApsTemporary();
   }

   WBFL::Geometry::Point2d e  = pDetails->pLosses->GetEccPermanentFinal();
   WBFL::Geometry::Point2d eps = e;
   if ( pDetails->pLosses->GetTempStrandUsage() == WBFL::LRFD::Losses::TempStrandUsage::Pretensioned )
   {
      eps = pDetails->pLosses->GetEccpgFinal();
   }

   Float64 An, Ybn, Ixxn, Iyyn, Ixyn;
   pDetails->pLosses->GetNetNoncompositeProperties(&An, &Ybn, &Ixxn, &Iyyn, &Ixyn);

   ColumnIndexType col = 1;
   RowIndexType rowOffset = GetNumberOfHeaderRows() - 1;

   (*this)(row+rowOffset, col++) << area.SetValue(Aps);


   if (m_bIsPrismatic)
   {
      if (m_bIsAsymmetric)
      {
         (*this)(row+rowOffset, col++) << ecc.SetValue(eps.X());
         (*this)(row+rowOffset, col++) << ecc.SetValue(eps.Y());
      }
      else
      {
         (*this)(row+rowOffset, col++) << ecc.SetValue(eps.Y());
      }
   }
   else
   {
      if (m_bIsAsymmetric)
      {
         (*this)(row+rowOffset, col++) << area.SetValue(An);
         (*this)(row+rowOffset, col++) << mom_inertia.SetValue(Ixxn);
         (*this)(row+rowOffset, col++) << mom_inertia.SetValue(Iyyn);
         (*this)(row+rowOffset, col++) << mom_inertia.SetValue(Ixyn);
         (*this)(row+rowOffset, col++) << ecc.SetValue(eps.X());
         (*this)(row+rowOffset, col++) << ecc.SetValue(eps.Y());
      }
      else
      {
         (*this)(row+rowOffset, col++) << area.SetValue(An);
         (*this)(row+rowOffset, col++) << mom_inertia.SetValue(Ixxn);
         (*this)(row+rowOffset, col++) << ecc.SetValue(eps.Y());
      }
   }

   if (m_bIsAsymmetric)
   {
      (*this)(row+rowOffset, col++) << ecc.SetValue(e.X());
   }
   (*this)(row+rowOffset, col++) << ecc.SetValue(e.Y());
   (*this)(row+rowOffset, col++) << scalar.SetValue(ptl->GetKid());
   (*this)(row+rowOffset, col++) << stress.SetValue(ptl->ShrinkageLossBeforeDeckPlacement());
}
