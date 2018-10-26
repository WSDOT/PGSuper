///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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
   if (fcgpMethod == lrfdElasticShortening::fcgpIterative)
   {
      *pParagraph << _T("Prestress loss due to Elastic Shortening - LRFD 5.9.5.2.3a") << rptNewLine;
   }
   else if (fcgpMethod == lrfdElasticShortening::fcgp07Fpu)
   {
      *pParagraph << _T("Prestress loss due to Elastic Shortening [TxDOT Research Report 0-6374-2]") << rptNewLine;
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

      if ( spMode == pgsTypes::spmGross )
      {
         *pParagraph << rptRcImage(strImagePath + _T("Delta_FpES_Gross.png")) << rptNewLine;
      }
      else
      {
         *pParagraph << rptRcImage(strImagePath + _T("Delta_FpES_Transformed.png")) << rptNewLine;
      }

      *pParagraph << _T("Note: Elastic Shortening considered constant along girder length. All parameters taken at mid-span of girder.") << rptNewLine << rptNewLine;
      *pParagraph << Sub2(_T("E"),_T("p")) << _T(" = ") << mod_e.SetValue(Epp) << rptNewLine;
      *pParagraph << Sub2(_T("E"),_T("ci")) << _T(" = ") << mod_e.SetValue(Eci) << rptNewLine;

      *pParagraph << Sub2(_T("A"),_T("g")) << _T(" = ") << area.SetValue(pDetails->pLosses->GetAg()) << rptNewLine;
      *pParagraph << Sub2(_T("I"),_T("g")) << _T(" = ") << mom_inertia.SetValue(pDetails->pLosses->GetIg()) << rptNewLine;
      *pParagraph << Sub2(_T("M"),_T("gm")) << _T(" = ") << moment.SetValue( pDetails->pLosses->GetGdrMoment()) << rptNewLine;
      *pParagraph << Sub2(_T("e"),_T("m")) << _T(" = ") <<ecc.SetValue( pDetails->pLosses->GetEccPermanentRelease()) << rptNewLine;

      Float64 Fpu = lrfdPsStrand::GetUltimateStrength( pDetails->pLosses->GetPermanentStrandGrade() );
      Float64 Aps = pDetails->pLosses->GetApsPermanent();
      Float64 P   = pDetails->pLosses->ElasticShortening().P();

      *pParagraph << Sub2(_T("0.7 f"),_T("pu")) << Sub2(_T(" A"),_T("ps")) << _T(" = 0.7(") 
                  << stress.SetValue(Fpu) << _T(")(") << area.SetValue(Aps) 
                  <<  _T(") = ") << force.SetValue(-P) << rptNewLine << rptNewLine;

      *pParagraph << Sub2(_T("f"),_T("cgp")) << _T(" = ") << stress.SetValue( pDetails->pLosses->ElasticShortening().PermanentStrand_Fcgp() ) << rptNewLine << rptNewLine;
      *pParagraph << symbol(DELTA) << Sub2(_T("f"),_T("pes")) << _T(" = ") << stress.SetValue( pDetails->pLosses->PermanentStrand_ElasticShorteningLosses() ) << rptNewLine;

      return NULL;
   }
   else
   {
      // create and configure the table
      ColumnIndexType numColumns = 10;
      if ( bTemporaryStrands )
      {
         numColumns += 3;
      }
      
      GET_IFACE2(pBroker,IGirder,pGirder);
      bool bIsPrismatic = pGirder->IsPrismatic(releaseIntervalIdx,segmentKey);
   
      if ( bIsPrismatic )
      {
         numColumns -= 2;
      }
    
      CElasticShorteningTable* table = new CElasticShorteningTable( numColumns, pDisplayUnits );
      rptStyleManager::ConfigureTable(table);
   
      table->m_bTemporaryStrands = bTemporaryStrands;
      table->m_bIsPrismatic      = bIsPrismatic;

      if ( spMode == pgsTypes::spmGross )
      {
         *pParagraph << rptRcImage(strImagePath + _T("Delta_FpES_Gross.png")) << rptNewLine;
      }
      else
      {
         *pParagraph << rptRcImage(strImagePath + _T("Delta_FpES_Transformed.png")) << rptNewLine;
      }
   
      table->mod_e.ShowUnitTag(true);
      table->area.ShowUnitTag(true);
      table->mom_inertia.ShowUnitTag(true);
      table->stress.ShowUnitTag(true);
      table->force.ShowUnitTag(true);
      if ( bIsPrismatic )
      {
         Float64 Ag, Ig;
         Ag = pSectProp->GetAg(releaseIntervalIdx,pgsPointOfInterest(segmentKey,0.0));
         Ig = pSectProp->GetIx(releaseIntervalIdx,pgsPointOfInterest(segmentKey,0.0));
   
         if ( spMode == pgsTypes::spmGross )
         {
            *pParagraph << Sub2(_T("A"),_T("g")) << _T(" = ") << table->area.SetValue(Ag) << rptNewLine;
            *pParagraph << Sub2(_T("I"),_T("g")) << _T(" = ") << table->mom_inertia.SetValue(Ig) << rptNewLine;
         }
         else
         {
            *pParagraph << Sub2(_T("A"),_T("gt")) << _T(" = ") << table->area.SetValue(Ag) << rptNewLine;
            *pParagraph << Sub2(_T("I"),_T("gt")) << _T(" = ") << table->mom_inertia.SetValue(Ig) << rptNewLine;
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

      if ( !bIsPrismatic )
      {
         if ( spMode == pgsTypes::spmGross )
         {
            (*table)(0,col++) << COLHDR(Sub2(_T("A"),_T("g")), rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );
            (*table)(0,col++) << COLHDR(Sub2(_T("I"),_T("g")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit() );
         }
         else
         {
            (*table)(0,col++) << COLHDR(Sub2(_T("A"),_T("gt")), rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );
            (*table)(0,col++) << COLHDR(Sub2(_T("I"),_T("gt")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit() );
         }
      }
   
      if ( spMode == pgsTypes::spmGross )
      {
         (*table)(0,col++) << COLHDR(Sub2(_T("e"),_T("ps")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
      }
      else
      {
         (*table)(0,col++) << COLHDR(Sub2(_T("e"),_T("pst")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
      }
   
   
      (*table)(0,col++) << COLHDR(Sub2(_T("M"),_T("g")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
      
   
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
   
   
         if ( !bIsPrismatic )
         {
            table->SetRowSpan(0,col,2);
            table->SetRowSpan(1,col++,SKIP_CELL);
   
            table->SetRowSpan(0,col,2);
            table->SetRowSpan(1,col++,SKIP_CELL);
         }
   
         table->SetRowSpan(0,col,2);
         table->SetRowSpan(1,col++,SKIP_CELL);
   
         table->SetRowSpan(0,col,2);
         table->SetRowSpan(1,col++,SKIP_CELL);
   
         table->SetColumnSpan(0,col,3);
         (*table)(0,col++) << _T("Permanent Strands");
   
         table->SetColumnSpan(0,col,3);
         (*table)(0,col++) << _T("Temporary Strands");
   
         for ( ColumnIndexType i = col; i < numColumns; i++ )
            table->SetColumnSpan(0,i,SKIP_CELL);
   
         // perm
         col -= 2;
         if ( spMode == pgsTypes::spmGross )
         {
            (*table)(1,col++) << COLHDR(Sub2(_T("e"),_T("p")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
         }
         else
         {
            (*table)(1,col++) << COLHDR(Sub2(_T("e"),_T("pt")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
         }
   
         (*table)(1,col++) << COLHDR(RPT_STRESS(_T("cgp")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
         (*table)(1,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pES")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   
         // temp
         if ( spMode == pgsTypes::spmGross )
         {
            (*table)(1,col++) << COLHDR(Sub2(_T("e"),_T("t")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
         }
         else
         {
            (*table)(1,col++) << COLHDR(Sub2(_T("e"),_T("tt")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
         }
   
         (*table)(1,col++) << COLHDR(RPT_STRESS(_T("cgp")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
         (*table)(1,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pES")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      }
      else
      {
         if ( spMode == pgsTypes::spmGross )
         {
            (*table)(0,col++) << COLHDR(Sub2(_T("e"),_T("p")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
         }
         else
         {
            (*table)(0,col++) << COLHDR(Sub2(_T("e"),_T("pt")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
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
   
      if ( spMode == pgsTypes::spmGross )
      {
         *pParagraph << Sub2(_T("e"),_T("p")) << _T(" = eccentricty of permanent prestressing strands") << rptNewLine;
         if ( bTemporaryStrands )
         {
            *pParagraph << Sub2(_T("e"),_T("t")) << _T(" = eccentricty of temporary prestressing strands") << rptNewLine;
         }
   
         *pParagraph << Sub2(_T("e"),_T("ps")) << _T(" = eccentricty of all prestressing strands") << rptNewLine;
      }
      else
      {
         *pParagraph << Sub2(_T("e"),_T("pt")) << _T(" = eccentricty of permanent prestressing strands") << rptNewLine;
         if ( bTemporaryStrands )
         {
            *pParagraph << Sub2(_T("e"),_T("tt")) << _T(" = eccentricty of temporary prestressing strands") << rptNewLine;
         }
   
         *pParagraph << Sub2(_T("e"),_T("pst")) << _T(" = eccentricty of all prestressing strands") << rptNewLine;
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


   if ( !m_bIsPrismatic )
   {
      (*this)(row+rowOffset,col++) << area.SetValue( pDetails->pLosses->GetAg() );
      (*this)(row+rowOffset,col++) << mom_inertia.SetValue( pDetails->pLosses->GetIg() );
   }

   (*this)(row+rowOffset,col++) << ecc.SetValue( pDetails->pLosses->GetEccpgRelease() );
   (*this)(row+rowOffset,col++) << moment.SetValue( pDetails->pLosses->GetGdrMoment() );

   (*this)(row+rowOffset,col++) << ecc.SetValue( pDetails->pLosses->GetEccPermanentRelease() );
   (*this)(row+rowOffset,col++) << stress.SetValue( es.PermanentStrand_Fcgp() );
   (*this)(row+rowOffset,col++) << stress.SetValue( pDetails->pLosses->PermanentStrand_ElasticShorteningLosses() );

   if ( m_bTemporaryStrands )
   {
      (*this)(row+rowOffset,col++) << ecc.SetValue( pDetails->pLosses->GetEccTemporary() );
      (*this)(row+rowOffset,col++) << stress.SetValue( es.TemporaryStrand_Fcgp() );
      (*this)(row+rowOffset,col++) << stress.SetValue( pDetails->pLosses->TemporaryStrand_ElasticShorteningLosses() );
   }
}
