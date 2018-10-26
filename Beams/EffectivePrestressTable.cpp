///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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
#include <IFace\PrestressForce.h>
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
   DEFINE_UV_PROTOTYPE( time,        pDisplayUnits->GetWholeDaysUnit(),        false );

   scalar.SetFormat( sysNumericFormatTool::Fixed );
   scalar.SetWidth(5); // -99.9
   scalar.SetPrecision(1);
   scalar.SetTolerance(1.0e-6);
}

CEffectivePrestressTable* CEffectivePrestressTable::PrepareTable(rptChapter* pChapter,IBroker* pBroker,const CSegmentKey& segmentKey,const LOSSDETAILS* pDetails,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   GET_IFACE2(pBroker,ILossParameters,pLossParameters);

   pgsTypes::LossMethod loss_method = pLossParameters->GetLossMethod();

   GET_IFACE2(pBroker,ISegmentData,pSegmentData);
   const CStrandData* pStrands = pSegmentData->GetStrandData(segmentKey);

   bool bIgnoreInitialRelaxation = pDetails->pLosses->IgnoreInitialRelaxation();

   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);
   StrandIndexType NtMax = pStrandGeom->GetMaxStrands(segmentKey,pgsTypes::Temporary);

   GET_IFACE2(pBroker,ISectionProperties,pSectProp);
   bool bUseGrossProperties = pSectProp->GetSectionPropertiesMode() == pgsTypes::spmGross ? true : false;

   GET_IFACE2(pBroker,ILosses, pLosses);
   bool bDeckShrinkage = pLosses->IsDeckShrinkageApplicable();

   // Create and configure the table
   ColumnIndexType numColumns = 9;
   if ( bUseGrossProperties )
   {
      numColumns++;
   }

   if ( !bIgnoreInitialRelaxation )
   {
      numColumns++;
   }

   if ( bDeckShrinkage )
   {
      numColumns++;
   }

   CEffectivePrestressTable* table = new CEffectivePrestressTable( numColumns, pDisplayUnits );
   pgsReportStyleHolder::ConfigureTable(table);

   table->m_bIsDeckShinkageApplied = bDeckShrinkage;
   table->m_bUseGrossProperties = bUseGrossProperties;
   table->m_bIgnoreInitialRelaxation = bIgnoreInitialRelaxation;

   std::_tstring strImagePath(pgsReportStyleHolder::GetImagePath());
   
   rptParagraph* pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;

   *pParagraph << _T("Effective Prestress") << rptNewLine;


   GET_IFACE2(pBroker,ILoadFactors,pLoadFactors);
   table->m_gLL = pLoadFactors->GetLoadFactors()->LLIMmax[pgsTypes::ServiceIII];

   std::_tostringstream os;
   os << table->m_gLL;

   pParagraph = new rptParagraph;
   *pChapter << pParagraph;
   if (  loss_method == pgsTypes::AASHTO_REFINED || loss_method == pgsTypes::WSDOT_REFINED  )
   {
      // delta fpT
      *pParagraph << symbol(DELTA) << RPT_STRESS(_T("pT")) << _T(" = ");
      if ( !bIgnoreInitialRelaxation )
      {
         *pParagraph << symbol(DELTA) << RPT_STRESS(_T("pR0")) << _T(" + ");
      }

      *pParagraph << symbol(DELTA) << RPT_STRESS(_T("pES")) << _T(" + ");

      if ( pStrands->GetTemporaryStrandUsage() != pgsTypes::ttsPretensioned )
      {
         *pParagraph << symbol(DELTA) << RPT_STRESS(_T("pp")) << _T(" + ");
      }

      if ( 0 < NtMax )
      {
         *pParagraph << symbol(DELTA) << RPT_STRESS(_T("ptr")) << _T(" + ");
      }

      *pParagraph << symbol(DELTA) << RPT_STRESS(_T("pLT")) << rptNewLine;


      // fpe
      *pParagraph << RPT_STRESS(_T("pe")) << _T(" = ") << RPT_STRESS(_T("pj")) << _T(" - ") << symbol(DELTA) << RPT_STRESS(_T("pT")) << _T(" + ")
                  << symbol(DELTA) << RPT_STRESS(_T("pED")) << _T(" + ")
                  << symbol(DELTA) << RPT_STRESS(_T("pSIDL"));
      if ( bDeckShrinkage )
      {
         *pParagraph << _T(" + ") << symbol(DELTA) << RPT_STRESS(_T("pSS"));
      }
      *pParagraph << rptNewLine;
      *pParagraph << RPT_STRESS(_T("pe")) << _T(" = ") << RPT_STRESS(_T("pj")) << _T(" - ") << symbol(DELTA) << RPT_STRESS(_T("pT")) << _T(" + ")
                  << symbol(DELTA) << RPT_STRESS(_T("pED")) << _T(" + ")
                  << symbol(DELTA) << RPT_STRESS(_T("pSIDL"));
      if ( bUseGrossProperties )
      {
         *pParagraph << _T(" + ") << symbol(DELTA) << RPT_STRESS(_T("pSS"));
      }
      *pParagraph << _T(" + ") << _T("(") << os.str().c_str() << _T(")") << symbol(DELTA) << RPT_STRESS(_T("pLL")) << rptNewLine;
   }
   else
   {
      *pParagraph << rptRcImage(strImagePath + _T("EffectivePrestress_Approx.png")) << rptNewLine;
   }

   ColumnIndexType col = 0;
   *pParagraph << table << rptNewLine;
   (*table)(0,col++) << COLHDR(_T("Location from")<<rptNewLine<<_T("Left Support"),rptLengthUnitTag,  pDisplayUnits->GetSpanLengthUnit() );
   (*table)(0,col++) << COLHDR(RPT_FPJ, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   if ( !bIgnoreInitialRelaxation )
   {
      (*table)(0,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pR0")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   }
   (*table)(0,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pES")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pLT")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pT")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pED")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pSIDL")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   if ( bDeckShrinkage )
   {
      (*table)(0,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pSS")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   }

   (*table)(0,col++) << COLHDR(RPT_STRESS(_T("pe")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pLL")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,col++) << COLHDR(RPT_STRESS(_T("pe")) << rptNewLine << _T("with Live Load"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );

   return table;
}

void CEffectivePrestressTable::AddRow(rptChapter* pChapter,IBroker* pBroker,const pgsPointOfInterest& poi,RowIndexType row,const LOSSDETAILS* pDetails,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   ColumnIndexType col = 1;
   Float64 fpj = pDetails->pLosses->GetFpjPermanent();

   Float64 fpLT = pDetails->pLosses->TimeDependentLosses();   // LT = SR + CR + R1 + SD + CD + R2
   Float64 fpT  = pDetails->pLosses->PermanentStrand_Final(); // R0 + SR + CR + R1 + SD + CD + R2 = LT + R0

   Float64 fpR0   = pDetails->pLosses->PermanentStrand_RelaxationLossesBeforeTransfer(); // R0
   Float64 fpES   = pDetails->pLosses->PermanentStrand_ElasticShorteningLosses(); // ES
   Float64 fpED   = pDetails->pLosses->ElasticGainDueToDeckPlacement();
   Float64 fpSIDL = pDetails->pLosses->ElasticGainDueToSIDL();
   Float64 fpSS   = pDetails->pLosses->ElasticGainDueToDeckShrinkage();
   Float64 fpLL   = pDetails->pLosses->ElasticGainDueToLiveLoad();

   fpT += fpES; // T = R0 + ES + LT

   // fpT includes elastic gains due to permanent loads
   // elastic gains are subtracted when computing fpT so add them back
   // in so we are left with just time dependent losses (see below)

   (*this)(row,col++) << stress.SetValue(fpj);
   if ( !m_bIgnoreInitialRelaxation )
   {
      (*this)(row,col++) << stress.SetValue(fpR0);
   }
   (*this)(row,col++) << stress.SetValue(fpES);
   (*this)(row,col++) << stress.SetValue(fpLT);
   (*this)(row,col++) << stress.SetValue(fpT);
   (*this)(row,col++) << stress.SetValue(fpED);
   (*this)(row,col++) << stress.SetValue(fpSIDL);
   if ( m_bIsDeckShinkageApplied )
   {
      (*this)(row,col++) << stress.SetValue(fpSS);
   }

   Float64 fpe = fpj - fpT + fpED + fpSIDL;
   if ( m_bUseGrossProperties )
   {
      fpe += fpSS;
   }

   (*this)(row,col++) << stress.SetValue(fpe);

   (*this)(row,col++) << stress.SetValue(fpLL);
   fpe += m_gLL*fpLL; // add elastic gain due to live load
   (*this)(row,col++) << stress.SetValue(fpe);
}
