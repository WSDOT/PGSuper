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

CElasticShorteningTable* CElasticShorteningTable::PrepareTable(rptChapter* pChapter,IBroker* pBroker,SpanIndexType span,GirderIndexType gdr,
                                                               bool bTemporaryStrands, const LOSSDETAILS& details,
                                                               IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   lrfdElasticShortening::FcgpComputationMethod fcgpMethod = details.pLosses->ElasticShortening().GetFcgpComputationMethod();

   std::_tstring strImagePath(pgsReportStyleHolder::GetImagePath());

   GET_IFACE2(pBroker,IBridgeMaterial,pMaterial);
   Float64 Eci = pMaterial->GetEciGdr(span,gdr);
   Float64 Epp = pMaterial->GetStrand(span,gdr,pgsTypes::Permanent)->GetE();
   Float64 Ept = pMaterial->GetStrand(span,gdr,pgsTypes::Temporary)->GetE();

   rptParagraph* pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;
   if (lrfdElasticShortening::fcgpIterative==fcgpMethod)
   {
      *pParagraph << _T("Prestress loss due to Elastic Shortening - LRFD [5.9.5.2.3a]") << rptNewLine;
   }
   else if (lrfdElasticShortening::fcgp07Fpu==fcgpMethod)
   {
      *pParagraph << _T("Prestress loss due to Elastic Shortening [TxDOT Research Report 0-6374-2]") << rptNewLine;
   }
   else
   {
      ATLASSERT(0); // new method?
   }

   pParagraph = new rptParagraph;
   *pChapter << pParagraph;

   if (lrfdElasticShortening::fcgp07Fpu==fcgpMethod)
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

      *pParagraph << Sub2(_T("A"),_T("g")) << _T(" = ") << area.SetValue(details.pLosses->GetAg()) << rptNewLine;
      *pParagraph << Sub2(_T("I"),_T("g")) << _T(" = ") << mom_inertia.SetValue(details.pLosses->GetIg()) << rptNewLine;
      *pParagraph << Sub2(_T("M"),_T("gm")) << _T(" = ") << moment.SetValue( details.pLosses->GetGdrMoment()) << rptNewLine;
      *pParagraph << Sub2(_T("e"),_T("m")) << _T(" = ") <<ecc.SetValue( details.pLosses->GetEccPermanent()) << rptNewLine;

      Float64 Fpu = lrfdPsStrand::GetUltimateStrength( details.pLosses->GetStrandGrade() );
      Float64 Aps = details.pLosses->GetApsPermanent();
      Float64 P   = details.pLosses->ElasticShortening().P();

      *pParagraph << Sub2(_T("0.7 f"),_T("pu")) << Sub2(_T(" A"),_T("ps")) << _T(" = 0.7(") 
                  << stress.SetValue(Fpu) << _T(")(") << area.SetValue(Aps) 
                  <<  _T(") = ") << force.SetValue(-P) << rptNewLine << rptNewLine;

      *pParagraph << Sub2(_T("f"),_T("cgp")) << _T(" = ") << stress.SetValue( details.pLosses->ElasticShortening().PermanentStrand_Fcgp() ) << rptNewLine << rptNewLine;
      *pParagraph << symbol(DELTA) << Sub2(_T("f"),_T("pes")) << _T(" = ") << stress.SetValue( details.pLosses->PermanentStrand_ElasticShorteningLosses() ) << rptNewLine;

      return NULL;
   }
   else
   {
      // create and configure the table
      ColumnIndexType numColumns = 10;
      if ( bTemporaryStrands )
         numColumns += 3;

      GET_IFACE2(pBroker,IGirder,pGirder);
      bool bIsPrismatic = pGirder->IsPrismatic(pgsTypes::CastingYard,span,gdr);

      if ( bIsPrismatic )
         numColumns -= 2;

      CElasticShorteningTable* table = new CElasticShorteningTable( numColumns, pDisplayUnits );
      pgsReportStyleHolder::ConfigureTable(table);

      table->m_bTemporaryStrands = bTemporaryStrands;
      table->m_bIsPrismatic      = bIsPrismatic;

      *pParagraph << rptRcImage(strImagePath + _T("Delta_FpES.png")) << rptNewLine;

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

      if ( bIsPrismatic )
      {
         Float64 Ag, Ig;
         GET_IFACE2(pBroker,ISectProp2,pSectProp);
         Ag = pSectProp->GetAg(pgsTypes::CastingYard,pgsPointOfInterest(span,gdr,0.0));
         Ig = pSectProp->GetIx(pgsTypes::CastingYard,pgsPointOfInterest(span,gdr,0.0));
         *pParagraph << Sub2(_T("A"),_T("g")) << _T(" = ") << table->area.SetValue(Ag) << rptNewLine;
         *pParagraph << Sub2(_T("I"),_T("g")) << _T(" = ") << table->mom_inertia.SetValue(Ig) << rptNewLine;
      }


      table->mod_e.ShowUnitTag(false);
      table->area.ShowUnitTag(false);
      table->mom_inertia.ShowUnitTag(false);
      table->stress.ShowUnitTag(false);
      table->force.ShowUnitTag(false);

      *pParagraph << table << rptNewLine;

      ColumnIndexType col = 0;
      (*table)(0,col++) << COLHDR(_T("Location from")<<rptNewLine<<_T("End of Girder"),rptLengthUnitTag,  pDisplayUnits->GetSpanLengthUnit() );
      (*table)(0,col++) << COLHDR(_T("Location from")<<rptNewLine<<_T("Left Support"),rptLengthUnitTag,  pDisplayUnits->GetSpanLengthUnit() );

      if (lrfdElasticShortening::fcgp07Fpu!=fcgpMethod)
         (*table)(0,col++) << COLHDR(_T("P"), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit() );

      if ( !bIsPrismatic )
      {
         (*table)(0,col++) << COLHDR(Sub2(_T("A"),_T("g")), rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );
         (*table)(0,col++) << COLHDR(Sub2(_T("I"),_T("g")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit() );
      }

      (*table)(0,col++) << COLHDR(Sub2(_T("e"),_T("ps")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
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
         (*table)(1,col++) << COLHDR(Sub2(_T("e"),_T("p")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
         (*table)(1,col++) << COLHDR(RPT_STRESS(_T("cgp")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
         (*table)(1,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pES")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );

         // temp
         (*table)(1,col++) << COLHDR(Sub2(_T("e"),_T("t")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
         (*table)(1,col++) << COLHDR(RPT_STRESS(_T("cgp")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
         (*table)(1,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pES")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      }
      else
      {
         (*table)(0,col++) << COLHDR(Sub2(_T("e"),_T("p")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
         (*table)(0,col++) << COLHDR(RPT_STRESS(_T("cgp")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
         (*table)(0,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pES")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      }

      GET_IFACE2(pBroker,ISpecification,pSpec);
      std::_tstring strSpecName = pSpec->GetSpecification();

      GET_IFACE2(pBroker,ILibrary,pLib);
      const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( strSpecName.c_str() );

      int method = pSpecEntry->GetLossMethod();
      if ( method == LOSSES_WSDOT_REFINED || method == LOSSES_WSDOT_LUMPSUM )
      {
         pParagraph = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
         *pChapter << pParagraph;

         if ( bTemporaryStrands )
         {
            *pParagraph << _T("P is the prestressing force at transfer") << _T(" : ")
                        << _T("P = ") << Sub2(_T("A"),_T("p")) << _T("(") << RPT_STRESS(_T("pjp")) << _T(" - ") << symbol(DELTA) << RPT_STRESS(_T("pR0p")) << _T(" - ") << symbol(DELTA) << RPT_STRESS(_T("pESp")) << _T(")")
                        << _T("  + ") << Sub2(_T("A"),_T("t")) << _T("(") << RPT_STRESS(_T("pjt")) << _T(" - ") << symbol(DELTA) << RPT_STRESS(_T("pR0t")) << _T(" - ") << symbol(DELTA) << RPT_STRESS(_T("pESt")) << _T(")") << rptNewLine;
         }
         else
         {
            *pParagraph << _T("P is the prestressing force at transfer") << _T(" : ")
                        << _T("P = ") << Sub2(_T("A"),_T("ps")) << _T("(") << RPT_STRESS(_T("pj")) << _T(" - ") << symbol(DELTA) << RPT_STRESS(_T("pR0")) << _T(" - ") << symbol(DELTA) << RPT_STRESS(_T("pES")) << _T(")") << rptNewLine;
         }
         *pParagraph << rptNewLine;
      }
      else
      {
         pParagraph = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
         *pChapter << pParagraph;

         if ( bTemporaryStrands )
         {
            *pParagraph << _T("P is the prestressing force at transfer") << _T(" : ")
                        << _T("P = ") << Sub2(_T("A"),_T("p")) << _T("(") << RPT_STRESS(_T("pjp")) << _T(" - ") << symbol(DELTA) << RPT_STRESS(_T("pESp")) << _T(")")
                        << _T("  + ") << Sub2(_T("A"),_T("t")) << _T("(") << RPT_STRESS(_T("pjt")) << _T(" - ") << symbol(DELTA) << RPT_STRESS(_T("pESt")) << _T(")") << rptNewLine;
         }
         else
         {
            *pParagraph << _T("P is the prestressing force at transfer") << _T(" : ")
                        << _T("P = ") << Sub2(_T("A"),_T("ps")) << _T("(") << RPT_STRESS(_T("pj")) << _T(" - ") << symbol(DELTA) << RPT_STRESS(_T("pES")) << _T(")") << rptNewLine;
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

      *pParagraph << Sub2(_T("e"),_T("p")) << _T(" = eccentricty of permanent prestressing strands") << rptNewLine;
      if ( bTemporaryStrands )
         *pParagraph << Sub2(_T("e"),_T("t")) << _T(" = eccentricty of temporary prestressing strands") << rptNewLine;

      *pParagraph << Sub2(_T("e"),_T("ps")) << _T(" = eccentricty of all prestressing strands") << rptNewLine;
         
      *pParagraph << rptNewLine;

      return table;
   }
}

void CElasticShorteningTable::AddRow(rptChapter* pChapter,IBroker* pBroker,const pgsPointOfInterest& poi,RowIndexType row,LOSSDETAILS& details,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   ColumnIndexType col = 2;
   RowIndexType rowOffset = GetNumberOfHeaderRows()-1;

   const lrfdElasticShortening& es = details.pLosses->ElasticShortening();

   if (lrfdElasticShortening::fcgp07Fpu != es.GetFcgpComputationMethod())
      (*this)(row+rowOffset,col++) << force.SetValue( -es.P() );

   if ( !m_bIsPrismatic )
   {
      (*this)(row+rowOffset,col++) << area.SetValue( details.pLosses->GetAg() );
      (*this)(row+rowOffset,col++) << mom_inertia.SetValue( details.pLosses->GetIg() );
   }

   (*this)(row+rowOffset,col++) << ecc.SetValue( details.pLosses->GetEccpg() );
   (*this)(row+rowOffset,col++) << moment.SetValue( details.pLosses->GetGdrMoment() );

   (*this)(row+rowOffset,col++) << ecc.SetValue( details.pLosses->GetEccPermanent() );
   (*this)(row+rowOffset,col++) << stress.SetValue( es.PermanentStrand_Fcgp() );
   (*this)(row+rowOffset,col++) << stress.SetValue( details.pLosses->PermanentStrand_ElasticShorteningLosses() );

   if ( m_bTemporaryStrands )
   {
      (*this)(row+rowOffset,col++) << ecc.SetValue( details.pLosses->GetEccTemporary() );
      (*this)(row+rowOffset,col++) << stress.SetValue( es.TemporaryStrand_Fcgp() );
      (*this)(row+rowOffset,col++) << stress.SetValue( details.pLosses->TemporaryStrand_ElasticShorteningLosses() );
   }
}
