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

// CreepAtFinalTable.cpp : Implementation of CCreepAtFinalTable
#include "stdafx.h"
#include "CreepAtFinalTable.h"
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\Intervals.h>
#include <PsgLib\SpecLibraryEntry.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CCreepAtFinalTable::CCreepAtFinalTable(ColumnIndexType NumColumns, IEAFDisplayUnits* pDisplayUnits) :
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
}

CCreepAtFinalTable* CCreepAtFinalTable::PrepareTable(rptChapter* pChapter,IBroker* pBroker,const CSegmentKey& segmentKey,const LOSSDETAILS* pDetails,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   GET_IFACE2(pBroker,ISegmentData,pSegmentData);
   const CStrandData* pStrands = pSegmentData->GetStrandData(segmentKey);

   GET_IFACE2(pBroker,ISpecification,pSpec);
   std::_tstring strSpecName = pSpec->GetSpecification();

   GET_IFACE2(pBroker,ILibrary,pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( strSpecName.c_str() );

   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);
   StrandIndexType NtMax = pStrandGeom->GetMaxStrands(segmentKey,pgsTypes::Temporary);

   GET_IFACE2(pBroker,ISectionProperties,pSectProp);
   pgsTypes::SectionPropertyMode spMode = pSectProp->GetSectionPropertiesMode();

   GET_IFACE2(pBroker, IIntervals, pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);

   GET_IFACE2(pBroker, IGirder, pGirder);
   bool bIsPrismatic = pGirder->IsPrismatic(releaseIntervalIdx, segmentKey);

   GET_IFACE2(pBroker, IBridge, pBridge);
   bool bIsAsymmetric = pBridge->HasAsymmetricGirders() || pBridge->HasAsymmetricPrestressing() ? true : false;

   // Create and configure the table
   ColumnIndexType numColumns = 8; // Location, Kdf, fcgp, dfpSR, dfpCR, dfpR1, dfcd, dfpCD

   if ( 0 < NtMax )
   {
      if ( pStrands->GetTemporaryStrandUsage() == pgsTypes::ttsPretensioned )
      {
         numColumns++; // dfptr
      }
      else
      {
         numColumns+=2; // dfpp, dfptr
      }
   }

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
         numColumns += 6; // Ag, Ixx, Iyy, Ixy, epsx, epsy
      }
      else
      {
         numColumns += 3; // Ag, Ig, eps
      }
   }

   CCreepAtFinalTable* table = new CCreepAtFinalTable( numColumns, pDisplayUnits );
   rptStyleManager::ConfigureTable(table);

   
   table->m_pStrands = pStrands;
   table->m_NtMax = NtMax;
   table->m_bIsPrismatic = bIsPrismatic;
   table->m_bIsAsymmetric = bIsAsymmetric;

   std::_tstring strImagePath(rptStyleManager::GetImagePath());
   
   rptParagraph* pParagraph = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pParagraph;

   *pParagraph << _T("[") << LrfdCw8th(_T("5.9.5.4.3b"),_T("5.9.3.4.3b")) << _T("] Creep of Girder Concrete : ") << symbol(DELTA) << RPT_STRESS(_T("pCD")) << rptNewLine;

   
   pParagraph = new rptParagraph;
   *pChapter << pParagraph;

   if ( spMode == pgsTypes::spmGross )
   {
      if (bIsAsymmetric)
      {
         *pParagraph << rptRcImage(strImagePath + _T("Delta_fpcd_Gross_Asymmetric.png")) << rptNewLine;
      }
      else
      {
         *pParagraph << rptRcImage(strImagePath + _T("Delta_fpcd_Gross.png")) << rptNewLine;
      }
   }
   else
   {
      if (bIsAsymmetric)
      {
         *pParagraph << rptRcImage(strImagePath + _T("Delta_fpcd_Transformed_Asymmetric.png")) << rptNewLine;
      }
      else
      {
         *pParagraph << rptRcImage(strImagePath + _T("Delta_fpcd_Transformed.png")) << rptNewLine;
      }
   }

   
   if (0 < NtMax)
   {
      if (pStrands->GetTemporaryStrandUsage() != pgsTypes::ttsPretensioned)
      {
         if (pSpecEntry->GetSpecificationType() < lrfdVersionMgr::FourthEdition2007)
         {
            *pParagraph << rptRcImage(strImagePath + _T("Delta_FpCD_PT.png")) << rptNewLine;
         }
         else
         {
            *pParagraph << rptRcImage(strImagePath + _T("Delta_FpCD_PT_2007.png")) << rptNewLine;
         }
      }
      else
      {
         if (pSpecEntry->GetSpecificationType() < lrfdVersionMgr::FourthEdition2007)
         {
            *pParagraph << rptRcImage(strImagePath + _T("Delta_FpCD_PS.png")) << rptNewLine;
         }
         else
         {
            *pParagraph << rptRcImage(strImagePath + _T("Delta_FpCD_PS_2007.png")) << rptNewLine;
         }
      }
   }
   else
   {
      if (pSpecEntry->GetSpecificationType() < lrfdVersionMgr::FourthEdition2007)
      {
         *pParagraph << rptRcImage(strImagePath + _T("Delta_FpCD.png")) << rptNewLine;
      }
      else
      {
         *pParagraph << rptRcImage(strImagePath + _T("Delta_FpCD_2007.png")) << rptNewLine;
      }
   }


   // DELTA Fcd Table
   pParagraph = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pParagraph;

   pParagraph = new rptParagraph;
   *pChapter << pParagraph;

   rptRcTable* pParamTable = rptStyleManager::CreateDefaultTable(5,_T(""));
   *pParagraph << pParamTable << rptNewLine;
   (*pParamTable)(0,0) << COLHDR(Sub2(_T("E"),_T("p")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
   (*pParamTable)(0,1) << COLHDR(Sub2(_T("E"),_T("c")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
   (*pParamTable)(0,2) << COLHDR(Sub2(_T("E"),_T("ci")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
   (*pParamTable)(0,3) << COLHDR(RPT_FC,rptStressUnitTag,pDisplayUnits->GetStressUnit());
   (*pParamTable)(0,4) << Sub2(_T("k"),_T("f"));

  // Typecast to our known type (eating own doggy food)
   std::shared_ptr<const lrfdRefinedLosses2005> ptl = std::dynamic_pointer_cast<const lrfdRefinedLosses2005>(pDetails->pLosses);
   if (!ptl)
   {
      ATLASSERT(false); // made a bad cast? Bail...
      return table;
   }

   (*pParamTable)(1,0) << table->mod_e.SetValue( ptl->GetEp() );
   (*pParamTable)(1,1) << table->mod_e.SetValue( ptl->GetEc() );
   (*pParamTable)(1,2) << table->mod_e.SetValue( ptl->GetEci() );
   (*pParamTable)(1,3) << table->stress.SetValue(ptl->GetFc());
   (*pParamTable)(1,4) << table->scalar.SetValue(ptl->GetCreepDeckToFinal().GetKf());

   pParamTable = rptStyleManager::CreateDefaultTable(5,_T(""));
   *pParagraph << pParamTable << rptNewLine;
   (*pParamTable)(0,0) << COLHDR(Sub2(_T("t"),_T("i")), rptTimeUnitTag, pDisplayUnits->GetWholeDaysUnit());
   (*pParamTable)(0,1) << COLHDR(Sub2(_T("t"),_T("d")), rptTimeUnitTag, pDisplayUnits->GetWholeDaysUnit());
   (*pParamTable)(0,2) << COLHDR(Sub2(_T("t"),_T("f")), rptTimeUnitTag, pDisplayUnits->GetWholeDaysUnit());

   table->time.ShowUnitTag(true);
   (*pParamTable)(0,3) << Sub2(_T("k"),_T("td")) << rptNewLine << _T("Initial to Final") << rptNewLine << _T("t = ") << table->time.SetValue(ptl->GetCreepInitialToFinal().GetMaturity());
   (*pParamTable)(0,4) << Sub2(_T("k"),_T("td")) << rptNewLine << _T("Deck Placement to Final") << rptNewLine << _T("t = ") << table->time.SetValue(ptl->GetCreepDeckToFinal().GetMaturity());
   table->time.ShowUnitTag(false);

   (*pParamTable)(1,0) << table->time.SetValue( ptl->GetInitialAge() );
   (*pParamTable)(1,1) << table->time.SetValue( ptl->GetAgeAtDeckPlacement() );
   (*pParamTable)(1,2) << table->time.SetValue( ptl->GetFinalAge() );
   (*pParamTable)(1,3) << table->scalar.SetValue(ptl->GetCreepInitialToFinal().GetKtd());
   (*pParamTable)(1,4) << table->scalar.SetValue(ptl->GetCreepDeckToFinal().GetKtd());

   pParamTable = rptStyleManager::CreateDefaultTable(3,_T(""));
   *pParagraph << pParamTable << rptNewLine;
   (*pParamTable)(0,0) << Sub2(symbol(psi),_T("b")) << _T("(") << Sub2(_T("t"),_T("f")) << _T(",") << Sub2(_T("t"),_T("i")) << _T(")");
   (*pParamTable)(0,1) << Sub2(symbol(psi),_T("b")) << _T("(") << Sub2(_T("t"),_T("d")) << _T(",") << Sub2(_T("t"),_T("i")) << _T(")");
   (*pParamTable)(0,2) << Sub2(symbol(psi),_T("b")) << _T("(") << Sub2(_T("t"),_T("f")) << _T(",") << Sub2(_T("t"),_T("d")) << _T(")");

   (*pParamTable)(1,0) << table->scalar.SetValue(ptl->GetCreepInitialToFinal().GetCreepCoefficient());
   (*pParamTable)(1,1) << table->scalar.SetValue(ptl->GetCreepInitialToDeck().GetCreepCoefficient());
   (*pParamTable)(1,2) << table->scalar.SetValue(ptl->GetCreepDeckToFinal().GetCreepCoefficient());

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

   
   *pParagraph << table << rptNewLine;

   ColumnIndexType col = 0;

   (*table)(0,col++) << COLHDR(_T("Location from")<<rptNewLine<<_T("Left Support"),rptLengthUnitTag,  pDisplayUnits->GetSpanLengthUnit() );

  if (spMode == pgsTypes::spmGross)
  {
     if (bIsPrismatic)
     {
        if (bIsAsymmetric)
        {
           (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("psx")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
           (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("psy")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
        }
        else
        {
           (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("ps")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
        }
     }
     else
     {
        if (bIsAsymmetric)
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
           (*table)(0, col++) << COLHDR(Sub2(_T("A"), _T("g")), rptAreaUnitTag, pDisplayUnits->GetAreaUnit());
           (*table)(0, col++) << COLHDR(Sub2(_T("I"), _T("g")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
           (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("ps")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
        }
     }
  }
  else
  {
     if (bIsPrismatic)
     {
        if (bIsAsymmetric)
        {
           (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("psxt")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
           (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("psyt")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
        }
        else
        {
           (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("pst")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
        }
     }
     else
     {
        if (bIsAsymmetric)
        {
           (*table)(0, col++) << COLHDR(Sub2(_T("A"), _T("gt")), rptAreaUnitTag, pDisplayUnits->GetAreaUnit());
           (*table)(0, col++) << COLHDR(Sub2(_T("I"), _T("xxt")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
           (*table)(0, col++) << COLHDR(Sub2(_T("I"), _T("yyt")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
           (*table)(0, col++) << COLHDR(Sub2(_T("I"), _T("xyt")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
           (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("psxt")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
           (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("psyt")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
        }
        else
        {
           (*table)(0, col++) << COLHDR(Sub2(_T("A"), _T("gt")), rptAreaUnitTag, pDisplayUnits->GetAreaUnit());
           (*table)(0, col++) << COLHDR(Sub2(_T("I"), _T("gt")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
           (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("pst")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
        }
     }
  }

  (*table)(0, col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pSR")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
  (*table)(0, col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pCR")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
  (*table)(0, col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pR1")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
  (*table)(0, col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("cd")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
   if ( 0 < NtMax &&  pStrands->GetTemporaryStrandUsage() != pgsTypes::ttsPretensioned )
   {
      (*table)(0,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pp")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   }

   if ( 0 < NtMax )
   {
      (*table)(0,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("ptr")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   }

   (*table)(0, col++) << Sub2(_T("K"), _T("df"));
   (*table)(0, col++) << COLHDR(RPT_STRESS(_T("cgp")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
   (*table)(0,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pCD")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );

   return table;
}

void CCreepAtFinalTable::AddRow(rptChapter* pChapter,IBroker* pBroker,const pgsPointOfInterest& poi,RowIndexType row,const LOSSDETAILS* pDetails,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
  // Typecast to our known type (eating own doggy food)
   std::shared_ptr<const lrfdRefinedLosses2005> ptl = std::dynamic_pointer_cast<const lrfdRefinedLosses2005>(pDetails->pLosses);
   if (!ptl)
   {
      ATLASSERT(false); // made a bad cast? Bail...
      return;
   }

   ColumnIndexType col = 1;
   RowIndexType rowOffset = GetNumberOfHeaderRows() - 1;

   if (m_bIsPrismatic)
   {
      if (m_bIsAsymmetric)
      {
         (*this)(row+rowOffset, col++) << ecc.SetValue(pDetails->pLosses->GetEccPermanentFinal().X());
         (*this)(row+rowOffset, col++) << ecc.SetValue(pDetails->pLosses->GetEccPermanentFinal().Y());
      }
      else
      {
         (*this)(row+rowOffset, col++) << ecc.SetValue(pDetails->pLosses->GetEccPermanentFinal().Y());
      }
   }
   else
   {
      Float64 Ag, Ybg, Ixx, Iyy, Ixy;
      pDetails->pLosses->GetNetNoncompositeProperties(&Ag, &Ybg, &Ixx, &Iyy, &Ixy);
      if (m_bIsAsymmetric)
      {
         (*this)(row+rowOffset, col++) << area.SetValue(Ag);
         (*this)(row+rowOffset, col++) << mom_inertia.SetValue(Ixx);
         (*this)(row+rowOffset, col++) << mom_inertia.SetValue(Iyy);
         (*this)(row+rowOffset, col++) << mom_inertia.SetValue(Ixy);
         (*this)(row+rowOffset, col++) << ecc.SetValue(pDetails->pLosses->GetEccPermanentFinal().X());
         (*this)(row+rowOffset, col++) << ecc.SetValue(pDetails->pLosses->GetEccPermanentFinal().Y());
      }
      else
      {
         (*this)(row+rowOffset, col++) << area.SetValue(Ag);
         (*this)(row+rowOffset, col++) << mom_inertia.SetValue(Ixx);
         (*this)(row+rowOffset, col++) << ecc.SetValue(pDetails->pLosses->GetEccPermanentFinal().Y());
      }
   }

   (*this)(row+rowOffset, col++) << stress.SetValue(ptl->ShrinkageLossBeforeDeckPlacement());
   (*this)(row+rowOffset, col++) << stress.SetValue(ptl->CreepLossBeforeDeckPlacement());
   (*this)(row+rowOffset, col++) << stress.SetValue(ptl->RelaxationLossBeforeDeckPlacement());

   (*this)(row+rowOffset, col++) << stress.SetValue(ptl->GetDeltaFcd());

   if ( 0 < m_NtMax && m_pStrands->GetTemporaryStrandUsage() != pgsTypes::ttsPretensioned )
   {
      (*this)(row+rowOffset,col++) << stress.SetValue( ptl->GetDeltaFpp() );
   }

   if ( 0 < m_NtMax )
   {
      (*this)(row+rowOffset,col++) << stress.SetValue( ptl->GetDeltaFptr() );
   }

   (*this)(row+rowOffset, col++) << scalar.SetValue(ptl->GetKdf());
   (*this)(row+rowOffset, col++) << stress.SetValue(pDetails->pLosses->ElasticShortening().PermanentStrand_Fcgp());

   (*this)(row+rowOffset,col++) << stress.SetValue( ptl->CreepLossAfterDeckPlacement() );
}
