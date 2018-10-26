///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright (C) 2002  Washington State Department of Transportation
//                     Bridge and Structures Office
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

CElasticShorteningTable::CElasticShorteningTable(ColumnIndexType NumColumns, IDisplayUnits* pDispUnit) :
rptRcTable(NumColumns,0)
{
   DEFINE_UV_PROTOTYPE( spanloc,     pDispUnit->GetSpanLengthUnit(),      false );
   DEFINE_UV_PROTOTYPE( gdrloc,      pDispUnit->GetSpanLengthUnit(),      false );
   DEFINE_UV_PROTOTYPE( mod_e,       pDispUnit->GetModEUnit(),            false );
   DEFINE_UV_PROTOTYPE( force,       pDispUnit->GetGeneralForceUnit(),    false );
   DEFINE_UV_PROTOTYPE( area,        pDispUnit->GetAreaUnit(),            false );
   DEFINE_UV_PROTOTYPE( mom_inertia, pDispUnit->GetMomentOfInertiaUnit(), false );
   DEFINE_UV_PROTOTYPE( ecc,         pDispUnit->GetComponentDimUnit(),    false );
   DEFINE_UV_PROTOTYPE( moment,      pDispUnit->GetMomentUnit(),          false );
   DEFINE_UV_PROTOTYPE( stress,      pDispUnit->GetStressUnit(),          false );
}

CElasticShorteningTable* CElasticShorteningTable::PrepareTable(rptChapter* pChapter,IBroker* pBroker,SpanIndexType span,GirderIndexType gdr,bool bTemporaryStrands,IDisplayUnits* pDispUnit,Uint16 level)
{
   // create and configure the table
   ColumnIndexType numColumns = 10;
   if ( bTemporaryStrands )
      numColumns += 3;

   GET_IFACE2(pBroker,IGirder,pGirder);
   bool bIsPrismatic = pGirder->IsPrismatic(pgsTypes::CastingYard,span,gdr);

   if ( bIsPrismatic )
      numColumns -= 2;

   CElasticShorteningTable* table = new CElasticShorteningTable( numColumns, pDispUnit );
   pgsReportStyleHolder::ConfigureTable(table);

   table->m_bTemporaryStrands = bTemporaryStrands;
   table->m_bIsPrismatic      = bIsPrismatic;
   
   std::string strImagePath(pgsReportStyleHolder::GetImagePath());

   GET_IFACE2(pBroker,IBridgeMaterial,pMaterial);
   double Eci = pMaterial->GetEciGdr(span,gdr);
   double Ep  = pMaterial->GetStrand(span,gdr)->GetE();

   rptParagraph* pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;
   *pParagraph << "Prestress loss due to Elastic Shortening [5.9.5.2.3a]" << rptNewLine;

   pParagraph = new rptParagraph;
   *pChapter << pParagraph;

   *pParagraph << rptRcImage(strImagePath + "Delta_FpES.gif") << rptNewLine;

   table->mod_e.ShowUnitTag(true);
   table->area.ShowUnitTag(true);
   table->mom_inertia.ShowUnitTag(true);
   if ( bIsPrismatic )
   {
      double Ag, Ig;
      GET_IFACE2(pBroker,ISectProp2,pSectProp);
      Ag = pSectProp->GetAg(pgsTypes::CastingYard,pgsPointOfInterest(span,gdr,0.0));
      Ig = pSectProp->GetIx(pgsTypes::CastingYard,pgsPointOfInterest(span,gdr,0.0));
      *pParagraph << Sub2("A","g") << " = " << table->area.SetValue(Ag) << rptNewLine;
      *pParagraph << Sub2("I","g") << " = " << table->mom_inertia.SetValue(Ig) << rptNewLine;
   }

   *pParagraph << Sub2("E","p") << " = " << table->mod_e.SetValue(Ep) << rptNewLine;
   *pParagraph << Sub2("E","ci") << " = " << table->mod_e.SetValue(Eci) << rptNewLine;
   table->mod_e.ShowUnitTag(false);
   table->area.ShowUnitTag(false);
   table->mom_inertia.ShowUnitTag(false);

   *pParagraph << table << rptNewLine;

   ColumnIndexType col = 0;
   (*table)(0,col++) << COLHDR("Location from"<<rptNewLine<<"End of Girder",rptLengthUnitTag,  pDispUnit->GetSpanLengthUnit() );
   (*table)(0,col++) << COLHDR("Location from"<<rptNewLine<<"Left Support",rptLengthUnitTag,  pDispUnit->GetSpanLengthUnit() );
   (*table)(0,col++) << COLHDR("P", rptForceUnitTag, pDispUnit->GetGeneralForceUnit() );

   if ( !bIsPrismatic )
   {
      (*table)(0,col++) << COLHDR(Sub2("A","g"), rptAreaUnitTag, pDispUnit->GetAreaUnit() );
      (*table)(0,col++) << COLHDR(Sub2("I","g"), rptLength4UnitTag, pDispUnit->GetMomentOfInertiaUnit() );
   }

   (*table)(0,col++) << COLHDR(Sub2("e","ps"), rptLengthUnitTag, pDispUnit->GetComponentDimUnit() );
   (*table)(0,col++) << COLHDR(Sub2("M","g"), rptMomentUnitTag, pDispUnit->GetMomentUnit() );

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

      if ( !bIsPrismatic )
      {
         table->SetRowSpan(0,col,2);
         table->SetRowSpan(1,col++,-1);

         table->SetRowSpan(0,col,2);
         table->SetRowSpan(1,col++,-1);
      }

      table->SetRowSpan(0,col,2);
      table->SetRowSpan(1,col++,-1);

      table->SetRowSpan(0,col,2);
      table->SetRowSpan(1,col++,-1);

      table->SetColumnSpan(0,col,3);
      (*table)(0,col++) << "Permanent Strands";

      table->SetColumnSpan(0,col,3);
      (*table)(0,col++) << "Temporary Strands";

      for ( ColumnIndexType i = col; i < numColumns; i++ )
         table->SetColumnSpan(0,i,-1);

      // perm
      col -= 2;
      (*table)(1,col++) << COLHDR(Sub2("e","p"), rptLengthUnitTag, pDispUnit->GetComponentDimUnit() );
      (*table)(1,col++) << COLHDR(Sub2("f","cgp"), rptStressUnitTag, pDispUnit->GetStressUnit() );
      (*table)(1,col++) << COLHDR(symbol(DELTA) << Sub2("f","pES"), rptStressUnitTag, pDispUnit->GetStressUnit() );

      // temp
      (*table)(1,col++) << COLHDR(Sub2("e","t"), rptLengthUnitTag, pDispUnit->GetComponentDimUnit() );
      (*table)(1,col++) << COLHDR(Sub2("f","cgp"), rptStressUnitTag, pDispUnit->GetStressUnit() );
      (*table)(1,col++) << COLHDR(symbol(DELTA) << Sub2("f","pES"), rptStressUnitTag, pDispUnit->GetStressUnit() );
   }
   else
   {
      (*table)(0,col++) << COLHDR(Sub2("e","p"), rptLengthUnitTag, pDispUnit->GetComponentDimUnit() );
      (*table)(0,col++) << COLHDR(Sub2("f","cgp"), rptStressUnitTag, pDispUnit->GetStressUnit() );
      (*table)(0,col++) << COLHDR(symbol(DELTA) << Sub2("f","pES"), rptStressUnitTag, pDispUnit->GetStressUnit() );
   }

   GET_IFACE2(pBroker,ISpecification,pSpec);
   std::string strSpecName = pSpec->GetSpecification();

   GET_IFACE2(pBroker,ILibrary,pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( strSpecName.c_str() );

   int method = pSpecEntry->GetLossMethod();
   if ( method == LOSSES_WSDOT_REFINED || method == LOSSES_WSDOT_LUMPSUM )
   {
      pParagraph = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
      *pChapter << pParagraph;

      if ( bTemporaryStrands )
      {
         *pParagraph << "P is the prestressing force at transfer" << " : "
                     << "P = " << Sub2("A","p") << "(" << Sub2("f","pjp") << " - " << symbol(DELTA) << Sub2("f","pR0p") << " - " << symbol(DELTA) << Sub2("f","pESp") << ")"
                     << "  + " << Sub2("A","t") << "(" << Sub2("f","pjt") << " - " << symbol(DELTA) << Sub2("f","pR0t") << " - " << symbol(DELTA) << Sub2("f","pESt") << ")" << rptNewLine;
      }
      else
      {
         *pParagraph << "P is the prestressing force at transfer" << " : "
                     << "P = " << Sub2("A","ps") << "(" << Sub2("f","pj") << " - " << symbol(DELTA) << Sub2("f","pR0") << " - " << symbol(DELTA) << Sub2("f","pES") << ")" << rptNewLine;
      }
      *pParagraph << rptNewLine;
   }
   else
   {
      pParagraph = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
      *pChapter << pParagraph;

      if ( bTemporaryStrands )
      {
         *pParagraph << "P is the prestressing force at transfer" << " : "
                     << "P = " << Sub2("A","p") << "(" << Sub2("f","pjp") << " - " << symbol(DELTA) << Sub2("f","pESp") << ")"
                     << "  + " << Sub2("A","t") << "(" << Sub2("f","pjt") << " - " << symbol(DELTA) << Sub2("f","pESt") << ")" << rptNewLine;
      }
      else
      {
         *pParagraph << "P is the prestressing force at transfer" << " : "
                     << "P = " << Sub2("A","ps") << "(" << Sub2("f","pj") << " - " << symbol(DELTA) << Sub2("f","pES") << ")" << rptNewLine;
      }
      *pParagraph << rptNewLine;
   }

   if ( bTemporaryStrands )
   {
      *pParagraph << Sub2("A","p") << " = area of permanent prestressing strands" << rptNewLine;
      *pParagraph << Sub2("A","t") << " = area of temporary prestressing strands" << rptNewLine;
   }
   else
   {
      *pParagraph << Sub2("A","ps") << " = area of prestressing strands" << rptNewLine;
   }

   *pParagraph << Sub2("e","p") << " = eccentricty of permanent prestressing strands" << rptNewLine;
   if ( bTemporaryStrands )
      *pParagraph << Sub2("e","t") << " = eccentricty of temporary prestressing strands" << rptNewLine;

   *pParagraph << Sub2("e","ps") << " = eccentricty of all prestressing strands" << rptNewLine;
      
   *pParagraph << rptNewLine;

   return table;
}

void CElasticShorteningTable::AddRow(rptChapter* pChapter,IBroker* pBroker,RowIndexType row,LOSSDETAILS& details,IDisplayUnits* pDispUnit,Uint16 level)
{
   ColumnIndexType col = 2;
   int rowOffset = GetNumberOfHeaderRows()-1;

   const lrfdElasticShortening& es = details.pLosses->ElasticShortening();

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
