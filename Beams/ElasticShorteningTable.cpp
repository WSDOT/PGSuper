///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2021  Washington State Department of Transportation
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

// ElasticShorteningTable.cpp : Implementation of CElasticShorteningTable
#include "stdafx.h"
#include "ElasticShorteningTable.h"
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\Intervals.h>
#include <PsgLib\SpecLibraryEntry.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CElasticShorteningTable::CElasticShorteningTable(ColumnIndexType NumColumns, IEAFDisplayUnits* pDisplayUnits) :
rptRcTable(NumColumns,0)
{
   DEFINE_UV_PROTOTYPE( spanloc,     pDisplayUnits->GetSpanLengthUnit(),      false );
   DEFINE_UV_PROTOTYPE( gdrloc,      pDisplayUnits->GetSpanLengthUnit(),      false );
   DEFINE_UV_PROTOTYPE( mod_e,       pDisplayUnits->GetModEUnit(),            false );
   DEFINE_UV_PROTOTYPE( force,       pDisplayUnits->GetGeneralForceUnit(),    false );
   DEFINE_UV_PROTOTYPE( area,        pDisplayUnits->GetAreaUnit(),            false );
   DEFINE_UV_PROTOTYPE( mom_inertia, pDisplayUnits->GetMomentOfInertiaUnit(), false );
   DEFINE_UV_PROTOTYPE( ecc,         pDisplayUnits->GetComponentDimUnit(),    false );
   DEFINE_UV_PROTOTYPE( moment,      pDisplayUnits->GetMomentUnit(),          false );
   DEFINE_UV_PROTOTYPE( stress,      pDisplayUnits->GetStressUnit(),          false );
}

CElasticShorteningTable* CElasticShorteningTable::PrepareTable(rptChapter* pChapter,IBroker* pBroker,const CSegmentKey& segmentKey,bool bTemporaryStrands,const LOSSDETAILS* pDetails,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   std::_tstring strImagePath(rptStyleManager::GetImagePath());

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);

   lrfdElasticShortening::FcgpComputationMethod fcgpMethod = pDetails->pLosses->ElasticShortening().GetFcgpComputationMethod();

   GET_IFACE2(pBroker,IMaterials,pMaterials);
   Float64 Eci = pMaterials->GetSegmentEc(segmentKey,releaseIntervalIdx);
   Float64 Epp = pMaterials->GetStrandMaterial(segmentKey,pgsTypes::Permanent)->GetE();
   Float64 Ept = pMaterials->GetStrandMaterial(segmentKey,pgsTypes::Temporary)->GetE();

   rptParagraph* pParagraph = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pParagraph;
   pParagraph->SetName(_T("Elastic Shortening"));
   if (fcgpMethod == lrfdElasticShortening::fcgpIterative)
   {
      *pParagraph << pParagraph->GetName() << _T(" - LRFD ") << LrfdCw8th(_T("5.9.5.2.3a"), _T("5.9.3.2.3a")) << rptNewLine;
   }
   else if (fcgpMethod == lrfdElasticShortening::fcgp07Fpu)
   {
      *pParagraph << pParagraph->GetName() << _T(" [TxDOT Research Report 0-6374-2]") << rptNewLine;
   }
   else
   {
      ATLASSERT(false); // new method?
   }

   pParagraph = new rptParagraph;
   *pChapter << pParagraph;

   GET_IFACE2(pBroker,ISectionProperties,pSectProp);
   pgsTypes::SectionPropertyMode spMode = pSectProp->GetSectionPropertiesMode();

   if (fcgpMethod == lrfdElasticShortening::fcgp07Fpu)
   {
      // For 0.7fpu method, all values are constant along the girder - we don't need a table
      INIT_UV_PROTOTYPE( rptMomentUnitValue, moment,       pDisplayUnits->GetMomentUnit(),          true );
      INIT_UV_PROTOTYPE( rptStressUnitValue, stress,       pDisplayUnits->GetStressUnit(),          true );
      INIT_UV_PROTOTYPE( rptStressUnitValue, mod_e,        pDisplayUnits->GetModEUnit(),            true );
      INIT_UV_PROTOTYPE( rptForceUnitValue,  force,        pDisplayUnits->GetShearUnit(),           true );
      INIT_UV_PROTOTYPE( rptAreaUnitValue,   area,         pDisplayUnits->GetAreaUnit(),            true );
      INIT_UV_PROTOTYPE( rptLength4UnitValue,mom_inertia,  pDisplayUnits->GetMomentOfInertiaUnit(), true );
      INIT_UV_PROTOTYPE( rptLengthUnitValue,  ecc,         pDisplayUnits->GetComponentDimUnit(),    true );

      *pParagraph << rptRcImage(strImagePath + _T("Delta_FpES_TxDOTPerm.png")) << rptNewLine;

      *pParagraph << _T("Note: Elastic Shortening considered constant along girder length. All parameters taken at mid-span of girder.") << rptNewLine << rptNewLine;
      *pParagraph << Sub2(_T("E"),_T("p")) << _T(" = ") << mod_e.SetValue(Epp) << rptNewLine;
      *pParagraph << Sub2(_T("E"),_T("ci")) << _T(" = ") << mod_e.SetValue(Eci) << rptNewLine;

      Float64 Ag, Ybg, Ixx, Iyy, Ixy;
      pDetails->pLosses->GetNoncompositeProperties(&Ag, &Ybg, &Ixx, &Iyy, &Ixy);

      *pParagraph << Sub2(_T("A"),_T("g")) << _T(" = ") << area.SetValue(Ag) << rptNewLine;
      *pParagraph << Sub2(_T("I"),_T("g")) << _T(" = ") << mom_inertia.SetValue(Ixx) << rptNewLine;
      *pParagraph << Sub2(_T("M"),_T("gm")) << _T(" = ") << moment.SetValue( pDetails->pLosses->GetGdrMoment()) << rptNewLine;
      *pParagraph << Sub2(_T("e"),_T("m")) << _T(" = ") <<ecc.SetValue( pDetails->pLosses->GetEccPermanentRelease().Y()) << rptNewLine;

      Float64 Fpu = lrfdPsStrand::GetUltimateStrength( pDetails->pLosses->GetPermanentStrandGrade() );
      Float64 Aps = pDetails->pLosses->GetApsPermanent();
      Float64 P   = pDetails->pLosses->ElasticShortening().P();

      *pParagraph << Sub2(_T("0.7 f"),_T("pu")) << Sub2(_T(" A"),_T("ps")) << _T(" = 0.7(") 
                  << stress.SetValue(Fpu) << _T(")(") << area.SetValue(Aps) 
                  <<  _T(") = ") << force.SetValue(-P) << rptNewLine << rptNewLine;

      *pParagraph << Sub2(_T("f"),_T("cgp")) << _T(" = ") << stress.SetValue( pDetails->pLosses->ElasticShortening().PermanentStrand_Fcgp() ) << rptNewLine << rptNewLine;
      *pParagraph << symbol(DELTA) << Sub2(_T("f"),_T("pES")) << _T(" = ") << stress.SetValue( pDetails->pLosses->PermanentStrand_ElasticShorteningLosses() ) << rptNewLine;

      return nullptr;
   }
   else
   {
      // create and configure the table
      GET_IFACE2(pBroker, IGirder, pGirder);
      bool bIsPrismatic = pGirder->IsPrismatic(releaseIntervalIdx, segmentKey);

      GET_IFACE2(pBroker, IBridge, pBridge);
      bool bIsAsymmetric = pBridge->HasAsymmetricGirders() || pBridge->HasAsymmetricPrestressing() ? true : false;

      ColumnIndexType numColumns = 4; // location, location, P, M

      // columns for section properties and total ps eccentricty
      if (bIsPrismatic)
      {
         if (bIsAsymmetric)
         {
            numColumns += 2; // epsx, epsy
         }
         else
         {
            numColumns += 1; // eps
         }
      }
      else
      {
         // for prismatic girders, properties are the same along the length of the girder so they are reported
         // above the table... for non-prismatic girders, section properties change at each location
         if (bIsAsymmetric)
         {
            numColumns += 6; // A, Ixx, Iyy, Ixy, epsx, epsy
         }
         else
         {
            numColumns += 3; // A, Ixx, eps
         }
      }

      // permanent strands
      if (bIsAsymmetric)
      {
         numColumns += 4; // epx, epy, fcgp, dfpES
      }
      else
      {
         numColumns += 3; // ep, fcgp, dfpES
      }

      if ( bTemporaryStrands )
      {
         if (bIsAsymmetric)
         {
            numColumns += 4; // etx, ety, fcgp, dfpES
         }
         else
         {
            numColumns += 3; // et, fcgp, dfpES
         }
      }
    
      CElasticShorteningTable* table = new CElasticShorteningTable( numColumns, pDisplayUnits );
      rptStyleManager::ConfigureTable(table);
   
      table->m_bTemporaryStrands = bTemporaryStrands;
      table->m_bIsPrismatic      = bIsPrismatic;
      table->m_bIsAsymmetric = bIsAsymmetric;

      if (bIsAsymmetric)
      {
         if (spMode == pgsTypes::spmGross)
         {
            *pParagraph << rptRcImage(strImagePath + _T("Delta_FpES_Gross_Asymmetric.png")) << rptNewLine;
         }
         else
         {
            *pParagraph << rptRcImage(strImagePath + _T("Delta_FpES_Transformed_Asymmetric.png")) << rptNewLine;
         }
      }
      else
      {
         if (spMode == pgsTypes::spmGross)
         {
            *pParagraph << rptRcImage(strImagePath + _T("Delta_FpES_Gross.png")) << rptNewLine;
         }
         else
         {
            *pParagraph << rptRcImage(strImagePath + _T("Delta_FpES_Transformed.png")) << rptNewLine;
         }
      }
   
      table->mod_e.ShowUnitTag(true);
      table->area.ShowUnitTag(true);
      table->mom_inertia.ShowUnitTag(true);
      table->stress.ShowUnitTag(true);
      table->force.ShowUnitTag(true);
      if ( bIsPrismatic )
      {
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
               *pParagraph << Sub2(_T("A"), _T("g")) << _T(" = ") << table->area.SetValue(Ag) << rptNewLine;
               *pParagraph << Sub2(_T("I"), _T("xx")) << _T(" = ") << table->mom_inertia.SetValue(Ixx) << rptNewLine;
               *pParagraph << Sub2(_T("I"), _T("yy")) << _T(" = ") << table->mom_inertia.SetValue(Iyy) << rptNewLine;
               *pParagraph << Sub2(_T("I"), _T("xy")) << _T(" = ") << table->mom_inertia.SetValue(Ixy) << rptNewLine;
            }
            else
            {
               *pParagraph << Sub2(_T("A"), _T("gt")) << _T(" = ") << table->area.SetValue(Ag) << rptNewLine;
               *pParagraph << Sub2(_T("I"), _T("xxt")) << _T(" = ") << table->mom_inertia.SetValue(Ixx) << rptNewLine;
               *pParagraph << Sub2(_T("I"), _T("yyt")) << _T(" = ") << table->mom_inertia.SetValue(Iyy) << rptNewLine;
               *pParagraph << Sub2(_T("I"), _T("xyt")) << _T(" = ") << table->mom_inertia.SetValue(Ixy) << rptNewLine;
            }

         }
         else
         {
            if (spMode == pgsTypes::spmGross)
            {
               *pParagraph << Sub2(_T("A"), _T("g")) << _T(" = ") << table->area.SetValue(Ag) << rptNewLine;
               *pParagraph << Sub2(_T("I"), _T("g")) << _T(" = ") << table->mom_inertia.SetValue(Ixx) << rptNewLine;
            }
            else
            {
               *pParagraph << Sub2(_T("A"), _T("gt")) << _T(" = ") << table->area.SetValue(Ag) << rptNewLine;
               *pParagraph << Sub2(_T("I"), _T("gt")) << _T(" = ") << table->mom_inertia.SetValue(Ixx) << rptNewLine;
            }
         }
      }

      if ( bTemporaryStrands )
      {
         *pParagraph << Sub2(_T("E"),_T("p")) << _T(" (Permanent) = ") << table->mod_e.SetValue(Epp) << rptNewLine;
         *pParagraph << Sub2(_T("E"),_T("p")) << _T(" (Temporary) = ") << table->mod_e.SetValue(Ept) << rptNewLine;
      }
      else
      {
         *pParagraph << Sub2(_T("E"),_T("p")) << _T(" = ") << table->mod_e.SetValue(Epp) << rptNewLine;
      }
      *pParagraph << Sub2(_T("E"),_T("ci")) << _T(" = ") << table->mod_e.SetValue(Eci) << rptNewLine;

      table->mod_e.ShowUnitTag(false);
      table->area.ShowUnitTag(false);
      table->mom_inertia.ShowUnitTag(false);
      table->stress.ShowUnitTag(false);
      table->force.ShowUnitTag(false);
   
      *pParagraph << table << rptNewLine;
   
      ColumnIndexType col = 0;
      (*table)(0,col++) << COLHDR(_T("Location from")<<rptNewLine<<_T("End of Girder"),rptLengthUnitTag,  pDisplayUnits->GetSpanLengthUnit() );
      (*table)(0,col++) << COLHDR(_T("Location from")<<rptNewLine<<_T("Left Support"),rptLengthUnitTag,  pDisplayUnits->GetSpanLengthUnit() );
   
      if (fcgpMethod != lrfdElasticShortening::fcgp07Fpu)
      {
         (*table)(0,col++) << COLHDR(_T("P"), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit() );
      }

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
               (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("psxt")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
               (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("psyt")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
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
               (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("pst")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
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
               (*table)(0, col++) << COLHDR(Sub2(_T("A"), _T("gt")), rptAreaUnitTag, pDisplayUnits->GetAreaUnit());
               (*table)(0, col++) << COLHDR(Sub2(_T("I"), _T("xxt")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
               (*table)(0, col++) << COLHDR(Sub2(_T("I"), _T("yyt")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
               (*table)(0, col++) << COLHDR(Sub2(_T("I"), _T("xyt")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
               (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("psxt")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
               (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("psyt")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
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
               (*table)(0, col++) << COLHDR(Sub2(_T("A"), _T("gt")), rptAreaUnitTag, pDisplayUnits->GetAreaUnit());
               (*table)(0, col++) << COLHDR(Sub2(_T("I"), _T("gt")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
               (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("pst")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
            }
         }
      }
   
   
      (*table)(0,col++) << COLHDR(Sub2(_T("M"),_T("g")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
      
   
      if ( bTemporaryStrands )
      {
         // we have temporary strands so we need two header rows
         table->SetNumberOfHeaderRows(2);
   
         // all preceding columns need to span 2 rows
         for (ColumnIndexType c = 0; c < col; c++)
         {
            table->SetRowSpan(0, c, 2);
         }
   
         table->SetColumnSpan(0,col,bIsAsymmetric ? 4 : 3);
         (*table)(0,col) << _T("Permanent Strands");
   
         if (bIsAsymmetric)
         {
            if (spMode == pgsTypes::spmGross)
            {
               (*table)(1, col++) << COLHDR(Sub2(_T("e"), _T("px")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
               (*table)(1, col++) << COLHDR(Sub2(_T("e"), _T("py")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
            }
            else
            {
               (*table)(1, col++) << COLHDR(Sub2(_T("e"), _T("pxt")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
               (*table)(1, col++) << COLHDR(Sub2(_T("e"), _T("pyt")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
            }
         }
         else
         {
            if (spMode == pgsTypes::spmGross)
            {
               (*table)(1, col++) << COLHDR(Sub2(_T("e"), _T("p")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
            }
            else
            {
               (*table)(1, col++) << COLHDR(Sub2(_T("e"), _T("pt")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
            }
         }
   
         (*table)(1,col++) << COLHDR(RPT_STRESS(_T("cgp")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
         (*table)(1,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pES")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   
         // temp
         table->SetColumnSpan(0, col, bIsAsymmetric ? 4 : 3);
         (*table)(0, col) << _T("Temporary Strands");
         if (bIsAsymmetric)
         {
            if (spMode == pgsTypes::spmGross)
            {
               (*table)(1, col++) << COLHDR(Sub2(_T("e"), _T("tx")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
               (*table)(1, col++) << COLHDR(Sub2(_T("e"), _T("ty")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
            }
            else
            {
               (*table)(1, col++) << COLHDR(Sub2(_T("e"), _T("txt")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
               (*table)(1, col++) << COLHDR(Sub2(_T("e"), _T("tyt")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
            }
         }
         else
         {
            if (spMode == pgsTypes::spmGross)
            {
               (*table)(1, col++) << COLHDR(Sub2(_T("e"), _T("t")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
            }
            else
            {
               (*table)(1, col++) << COLHDR(Sub2(_T("e"), _T("tt")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
            }
         }
   
         (*table)(1,col++) << COLHDR(RPT_STRESS(_T("cgp")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
         (*table)(1,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pES")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      }
      else
      {
         if (bIsAsymmetric)
         {
            if (spMode == pgsTypes::spmGross)
            {
               (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("px")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
               (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("py")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
            }
            else
            {
               (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("pxt")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
               (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("pyt")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
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
               (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("pt")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
            }
         }
   
   
         (*table)(0,col++) << COLHDR(RPT_STRESS(_T("cgp")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
         (*table)(0,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pES")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      }
   
      GET_IFACE2(pBroker,ILossParameters,pLossParameters);
      pgsTypes::LossMethod loss_method = pLossParameters->GetLossMethod();
   
      if ( loss_method == pgsTypes::WSDOT_REFINED || loss_method == pgsTypes::WSDOT_LUMPSUM )
      {
         pParagraph = new rptParagraph(rptStyleManager::GetFootnoteStyle());
         *pChapter << pParagraph;
   
         if ( spMode == pgsTypes::spmGross )
         {
            if ( bTemporaryStrands )
            {
               *pParagraph << _T("P is the prestressing force after transfer") << _T(" : ")
                           << _T("P = ") << Sub2(_T("A"),_T("p")) << _T("(") << RPT_STRESS(_T("pjp")) << _T(" - ") << symbol(DELTA) << RPT_STRESS(_T("pR0p")) << _T(" - ") << symbol(DELTA) << RPT_STRESS(_T("pESp")) << _T(")")
                           << _T("  + ") << Sub2(_T("A"),_T("t")) << _T("(") << RPT_STRESS(_T("pjt")) << _T(" - ") << symbol(DELTA) << RPT_STRESS(_T("pR0t")) << _T(" - ") << symbol(DELTA) << RPT_STRESS(_T("pESt")) << _T(")") << rptNewLine;
            }
            else
            {
               *pParagraph << _T("P is the prestressing force after transfer") << _T(" : ")
                           << _T("P = ") << Sub2(_T("A"),_T("ps")) << _T("(") << RPT_STRESS(_T("pj")) << _T(" - ") << symbol(DELTA) << RPT_STRESS(_T("pR0")) << _T(" - ") << symbol(DELTA) << RPT_STRESS(_T("pES")) << _T(")") << rptNewLine;
            }
         }
         else
         {
            if ( bTemporaryStrands )
            {
               *pParagraph << _T("P is the prestressing force before transfer") << _T(" : ")
                           << _T("P = ") << Sub2(_T("A"),_T("p")) << _T("(") << RPT_STRESS(_T("pjp")) << _T(" - ") << symbol(DELTA) << RPT_STRESS(_T("pR0p")) << _T(")")
                           << _T("  + ") << Sub2(_T("A"),_T("t")) << _T("(") << RPT_STRESS(_T("pjt")) << _T(" - ") << symbol(DELTA) << RPT_STRESS(_T("pR0t")) << _T(")") << rptNewLine;
            }
            else
            {
               *pParagraph << _T("P is the prestressing force before transfer") << _T(" : ")
                           << _T("P = ") << Sub2(_T("A"),_T("ps")) << _T("(") << RPT_STRESS(_T("pj")) << _T(" - ") << symbol(DELTA) << RPT_STRESS(_T("pR0")) << _T(")") << rptNewLine;
            }
         }
         *pParagraph << rptNewLine;
      }
      else
      {
         pParagraph = new rptParagraph(rptStyleManager::GetFootnoteStyle());
         *pChapter << pParagraph;
   
         if ( spMode == pgsTypes::spmGross )
         {
            if ( bTemporaryStrands )
            {
               *pParagraph << _T("P is the prestressing force after transfer") << _T(" : ")
                           << _T("P = ") << Sub2(_T("A"),_T("p")) << _T("(") << RPT_STRESS(_T("pjp")) << _T(" - ") << symbol(DELTA) << RPT_STRESS(_T("pESp")) << _T(")")
                           << _T("  + ") << Sub2(_T("A"),_T("t")) << _T("(") << RPT_STRESS(_T("pjt")) << _T(" - ") << symbol(DELTA) << RPT_STRESS(_T("pESt")) << _T(")") << rptNewLine;
            }
            else
            {
               *pParagraph << _T("P is the prestressing force after transfer") << _T(" : ")
                           << _T("P = ") << Sub2(_T("A"),_T("ps")) << _T("(") << RPT_STRESS(_T("pj")) << _T(" - ") << symbol(DELTA) << RPT_STRESS(_T("pES")) << _T(")") << rptNewLine;
            }
         }
         else
         {
            if ( bTemporaryStrands )
            {
               *pParagraph << _T("P is the prestressing force before transfer") << _T(" : ")
                           << _T("P = ") << Sub2(_T("A"),_T("p")) << _T("(") << RPT_STRESS(_T("pjp")) << _T(")")
                           << _T("  + ") << Sub2(_T("A"),_T("t")) << _T("(") << RPT_STRESS(_T("pjt")) << _T(")") << rptNewLine;
            }
            else
            {
               *pParagraph << _T("P is the prestressing force before transfer") << _T(" : ")
                           << _T("P = ") << Sub2(_T("A"),_T("ps")) << _T("(") << RPT_STRESS(_T("pj")) << _T(")") << rptNewLine;
            }
         }
         *pParagraph << rptNewLine;
      }
   
      if ( bTemporaryStrands )
      {
         *pParagraph << Sub2(_T("A"),_T("p")) << _T(" = area of permanent prestressing strands") << rptNewLine;
         *pParagraph << Sub2(_T("A"),_T("t")) << _T(" = area of temporary prestressing strands") << rptNewLine;
      }
      else
      {
         *pParagraph << Sub2(_T("A"),_T("ps")) << _T(" = area of prestressing strands") << rptNewLine;
      }
   
      if (bIsAsymmetric)
      {
         if (spMode == pgsTypes::spmGross)
         {
            *pParagraph << Sub2(_T("e"), _T("px")) << _T(" = horizontal eccentricty of permanent prestressing strands") << rptNewLine;
            *pParagraph << Sub2(_T("e"), _T("py")) << _T(" = vertical eccentricty of permanent prestressing strands") << rptNewLine;
            if (bTemporaryStrands)
            {
               *pParagraph << Sub2(_T("e"), _T("tx")) << _T(" = horizontal eccentricty of temporary prestressing strands") << rptNewLine;
               *pParagraph << Sub2(_T("e"), _T("ty")) << _T(" = vertical eccentricty of temporary prestressing strands") << rptNewLine;
            }

            *pParagraph << Sub2(_T("e"), _T("psx")) << _T(" = horizontal eccentricty of all prestressing strands") << rptNewLine;
            *pParagraph << Sub2(_T("e"), _T("psy")) << _T(" = vertical eccentricty of all prestressing strands") << rptNewLine;
         }
         else
         {
            *pParagraph << Sub2(_T("e"), _T("pxt")) << _T(" = horizontal eccentricty of permanent prestressing strands") << rptNewLine;
            *pParagraph << Sub2(_T("e"), _T("pyt")) << _T(" = vertical eccentricty of permanent prestressing strands") << rptNewLine;
            if (bTemporaryStrands)
            {
               *pParagraph << Sub2(_T("e"), _T("txt")) << _T(" = horizontal eccentricty of temporary prestressing strands") << rptNewLine;
               *pParagraph << Sub2(_T("e"), _T("tyt")) << _T(" = vertical eccentricty of temporary prestressing strands") << rptNewLine;
            }

            *pParagraph << Sub2(_T("e"), _T("psxt")) << _T(" = horizontal eccentricty of all prestressing strands") << rptNewLine;
            *pParagraph << Sub2(_T("e"), _T("psyt")) << _T(" = vertical eccentricty of all prestressing strands") << rptNewLine;
         }
      }
      else
      {
         if (spMode == pgsTypes::spmGross)
         {
            *pParagraph << Sub2(_T("e"), _T("p")) << _T(" = eccentricty of permanent prestressing strands") << rptNewLine;
            if (bTemporaryStrands)
            {
               *pParagraph << Sub2(_T("e"), _T("t")) << _T(" = eccentricty of temporary prestressing strands") << rptNewLine;
            }

            *pParagraph << Sub2(_T("e"), _T("ps")) << _T(" = eccentricty of all prestressing strands") << rptNewLine;
         }
         else
         {
            *pParagraph << Sub2(_T("e"), _T("pt")) << _T(" = eccentricty of permanent prestressing strands") << rptNewLine;
            if (bTemporaryStrands)
            {
               *pParagraph << Sub2(_T("e"), _T("tt")) << _T(" = eccentricty of temporary prestressing strands") << rptNewLine;
            }

            *pParagraph << Sub2(_T("e"), _T("pst")) << _T(" = eccentricty of all prestressing strands") << rptNewLine;
         }
      }
         
      *pParagraph << rptNewLine;
   
      return table;
   }
}

void CElasticShorteningTable::AddRow(rptChapter* pChapter,IBroker* pBroker,const pgsPointOfInterest& poi,RowIndexType row,const LOSSDETAILS* pDetails,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   ColumnIndexType col = 2;
   RowIndexType rowOffset = GetNumberOfHeaderRows()-1;

   const lrfdElasticShortening& es = pDetails->pLosses->ElasticShortening();

   if (es.GetFcgpComputationMethod() != lrfdElasticShortening::fcgp07Fpu)
   {
      (*this)(row+rowOffset,col++) << force.SetValue( -es.P() );
   }

   Float64 Ag, Ybg, Ixx, Iyy, Ixy;
   pDetails->pLosses->GetNoncompositeProperties(&Ag, &Ybg, &Ixx, &Iyy, &Ixy);

   if (m_bIsPrismatic)
   {
      if (m_bIsAsymmetric)
      {
         (*this)(row + rowOffset, col++) << ecc.SetValue(pDetails->pLosses->GetEccpgRelease().X());
         (*this)(row + rowOffset, col++) << ecc.SetValue(pDetails->pLosses->GetEccpgRelease().Y());
      }
      else
      {
         (*this)(row + rowOffset, col++) << ecc.SetValue(pDetails->pLosses->GetEccpgRelease().Y());
      }
   }
   else
   {
      if (m_bIsAsymmetric)
      {
         (*this)(row + rowOffset, col++) << area.SetValue(Ag);
         (*this)(row + rowOffset, col++) << mom_inertia.SetValue(Ixx);
         (*this)(row + rowOffset, col++) << mom_inertia.SetValue(Iyy);
         (*this)(row + rowOffset, col++) << mom_inertia.SetValue(Ixy);
         (*this)(row + rowOffset, col++) << ecc.SetValue(pDetails->pLosses->GetEccpgRelease().X());
         (*this)(row + rowOffset, col++) << ecc.SetValue(pDetails->pLosses->GetEccpgRelease().Y());
      }
      else
      {
         (*this)(row + rowOffset, col++) << area.SetValue(Ag);
         (*this)(row + rowOffset, col++) << mom_inertia.SetValue(Ixx);
         (*this)(row + rowOffset, col++) << ecc.SetValue(pDetails->pLosses->GetEccpgRelease().Y());
      }
   }


   (*this)(row+rowOffset,col++) << moment.SetValue( pDetails->pLosses->GetGdrMoment() );

   if (m_bIsAsymmetric)
   {
      (*this)(row + rowOffset, col++) << ecc.SetValue(pDetails->pLosses->GetEccPermanentRelease().X());
   }
   (*this)(row+rowOffset,col++) << ecc.SetValue( pDetails->pLosses->GetEccPermanentRelease().Y() );
   (*this)(row+rowOffset,col++) << stress.SetValue( es.PermanentStrand_Fcgp() );
   (*this)(row+rowOffset,col++) << stress.SetValue( pDetails->pLosses->PermanentStrand_ElasticShorteningLosses() );

   if ( m_bTemporaryStrands )
   {
      if (m_bIsAsymmetric)
      {
         (*this)(row + rowOffset, col++) << ecc.SetValue(pDetails->pLosses->GetEccTemporary().X());
      }
      (*this)(row + rowOffset, col++) << ecc.SetValue(pDetails->pLosses->GetEccTemporary().Y());
      (*this)(row+rowOffset,col++) << stress.SetValue( es.TemporaryStrand_Fcgp() );
      (*this)(row+rowOffset,col++) << stress.SetValue( pDetails->pLosses->TemporaryStrand_ElasticShorteningLosses() );
   }
}
