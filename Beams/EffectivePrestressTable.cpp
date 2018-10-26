///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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

// EffectivePrestressTable.cpp : Implementation of CEffectivePrestressTable
#include "stdafx.h"
#include "EffectivePrestressTable.h"
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <PsgLib\SpecLibraryEntry.h>
#include <PgsExt\GirderData.h>
#include <PgsExt\LoadFactors.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CEffectivePrestressTable::CEffectivePrestressTable(ColumnIndexType NumColumns, IEAFDisplayUnits* pDisplayUnits) :
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

   scalar.SetFormat( sysNumericFormatTool::Fixed );
   scalar.SetWidth(5); // -99.9
   scalar.SetPrecision(1);
   scalar.SetTolerance(1.0e-6);
}

CEffectivePrestressTable* CEffectivePrestressTable::PrepareTable(rptChapter* pChapter,IBroker* pBroker,SpanIndexType span,GirderIndexType gdr,LOSSDETAILS& details,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   GET_IFACE2(pBroker,ISpecification,pSpec);
   std::_tstring strSpecName = pSpec->GetSpecification();

   GET_IFACE2(pBroker,ILibrary,pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( strSpecName.c_str() );

   int loss_method = pSpecEntry->GetLossMethod();

   // Create and configure the table
   ColumnIndexType numColumns = 8;
   if ( loss_method == LOSSES_AASHTO_REFINED || loss_method == LOSSES_WSDOT_REFINED )
   {
      numColumns++;
   }

   CEffectivePrestressTable* table = new CEffectivePrestressTable( numColumns, pDisplayUnits );
   pgsReportStyleHolder::ConfigureTable(table);

   table->m_LossMethod = loss_method;

   std::_tstring strImagePath(pgsReportStyleHolder::GetImagePath());
   
   rptParagraph* pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;

   *pParagraph << _T("Effective Prestress") << rptNewLine;

   GET_IFACE2(pBroker,ILoadFactors,pLoadFactors);
   Float64 gLL = pLoadFactors->GetLoadFactors()->LLIMmax[pgsTypes::ServiceIII];

   std::_tostringstream os;
   os << gLL;


   pParagraph = new rptParagraph;
   *pChapter << pParagraph;
   if (  loss_method == LOSSES_AASHTO_REFINED || loss_method == LOSSES_WSDOT_REFINED  )
   {
      *pParagraph << RPT_STRESS(_T("pe")) << _T(" = ") << RPT_STRESS(_T("pj")) << _T(" - ") << symbol(DELTA) << RPT_STRESS(_T("pT")) << _T(" + ")
                  << symbol(DELTA) << RPT_STRESS(_T("pED")) << _T(" + ")
                  << symbol(DELTA) << RPT_STRESS(_T("pSIDL")) << rptNewLine;
      *pParagraph << RPT_STRESS(_T("pe")) << _T(" = ") << RPT_STRESS(_T("pj")) << _T(" - ") << symbol(DELTA) << RPT_STRESS(_T("pT")) << _T(" + ")
                  << symbol(DELTA) << RPT_STRESS(_T("pED")) << _T(" + ")
                  << symbol(DELTA) << RPT_STRESS(_T("pSIDL")) << _T(" + ")
                  << _T("(") << os.str().c_str() << _T(")") << symbol(DELTA) << RPT_STRESS(_T("pLL")) << rptNewLine;
   }
   else
   {
      *pParagraph << rptRcImage(strImagePath + _T("EffectivePrestress_Approx.png")) << rptNewLine;
   }

   ColumnIndexType col = 0;
   *pParagraph << table << rptNewLine;
   (*table)(0,col++) << COLHDR(_T("Location from")<<rptNewLine<<_T("Left Support"),rptLengthUnitTag,  pDisplayUnits->GetSpanLengthUnit() );
   (*table)(0,col++) << COLHDR(RPT_FPJ, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pT")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pED")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pSIDL")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );

   if (  loss_method == LOSSES_AASHTO_REFINED || loss_method == LOSSES_WSDOT_REFINED  )
   {
      (*table)(0,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pSS")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   }

   (*table)(0,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pLL")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,col++) << COLHDR(RPT_STRESS(_T("pe")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,col++) << COLHDR(RPT_STRESS(_T("pe")) << rptNewLine << _T("with Live Load"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );

   return table;
}

void CEffectivePrestressTable::AddRow(rptChapter* pChapter,IBroker* pBroker,const pgsPointOfInterest& poi,RowIndexType row,LOSSDETAILS& details,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   ColumnIndexType col = 1;
   Float64 fpj = details.pLosses->GetFpjPermanent();
   Float64 fpT = details.pLosses->PermanentStrand_Final(); // includes elastic gains due to permanent loads
   Float64 fpED = details.pLosses->ElasticGainDueToDeckPlacement();
   Float64 fpSIDL = details.pLosses->ElasticGainDueToSIDL();
   Float64 fpSS = details.pLosses->ElasticGainDueToDeckShrinkage();
   Float64 fpLL = details.pLosses->ElasticGainDueToLiveLoad();
   (*this)(row,col++) << stress.SetValue(fpj);
   (*this)(row,col++) << stress.SetValue(fpT+fpED+fpSIDL+fpSS);
   (*this)(row,col++) << stress.SetValue(fpED);
   (*this)(row,col++) << stress.SetValue(fpSIDL);
   if ( m_LossMethod == LOSSES_AASHTO_REFINED || m_LossMethod == LOSSES_WSDOT_REFINED )
   {
      (*this)(row,col++) << stress.SetValue(fpSS);
   }

   (*this)(row,col++) << stress.SetValue(fpLL);

   Float64 fpe = fpj - fpT;
   (*this)(row,col++) << stress.SetValue(fpe);

   fpe += fpLL;
   (*this)(row,col++) << stress.SetValue(fpe);
}
