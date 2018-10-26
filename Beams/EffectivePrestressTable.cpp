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

#if defined _DEBUG
#include <IFace\Intervals.h>
#include <IFace\PrestressForce.h>
#endif

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
   scalar.SetWidth(6); // -99.9
   scalar.SetPrecision(3);
   scalar.SetTolerance(1.0e-6);
}

CEffectivePrestressTable* CEffectivePrestressTable::PrepareTable(rptChapter* pChapter,IBroker* pBroker,const CSegmentKey& segmentKey,const LOSSDETAILS* pDetails,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   GET_IFACE2(pBroker,ILossParameters,pLossParameters);

   pgsTypes::LossMethod loss_method = pLossParameters->GetLossMethod();

   GET_IFACE2(pBroker,ISegmentData,pSegmentData);
   const CStrandData* pStrands = pSegmentData->GetStrandData(segmentKey);
   bool bPTTempStrand = pStrands->GetTemporaryStrandUsage() != pgsTypes::ttsPretensioned ? true : false;

   bool bIgnoreInitialRelaxation = pDetails->pLosses->IgnoreInitialRelaxation();

   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);
   StrandIndexType NtMax = pStrandGeom->GetMaxStrands(segmentKey,pgsTypes::Temporary);
   bool bTempStrands = (0 < NtMax ? true : false);

   // Create and configure the table
   ColumnIndexType numColumns = 12;

   if ( !bIgnoreInitialRelaxation )
   {
      numColumns++;
   }

   if ( bPTTempStrand )
   {
      numColumns++;
   }

   if ( bTempStrands )
   {
      numColumns++;
   }

   CEffectivePrestressTable* table = new CEffectivePrestressTable( numColumns, pDisplayUnits );
   pgsReportStyleHolder::ConfigureTable(table);

   table->m_bPTTempStrand = bPTTempStrand;
   table->m_bTempStrands = bTempStrands;
   table->m_bIgnoreInitialRelaxation = bIgnoreInitialRelaxation;

   std::_tstring strImagePath(pgsReportStyleHolder::GetImagePath());
   
   rptParagraph* pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;

   *pParagraph << _T("Effective Prestress") << rptNewLine;


   GET_IFACE2(pBroker,ILoadFactors,pLoadFactors);
   table->m_gLL_ServiceI   = pLoadFactors->GetLoadFactors()->LLIMmax[pgsTypes::ServiceI];
   table->m_gLL_ServiceIII = pLoadFactors->GetLoadFactors()->LLIMmax[pgsTypes::ServiceIII];

   std::_tostringstream os1;
   os1 << table->m_gLL_ServiceI;

   std::_tostringstream os3;
   os3 << table->m_gLL_ServiceIII;

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

      if ( bPTTempStrand )
      {
         *pParagraph << symbol(DELTA) << RPT_STRESS(_T("pp")) << _T(" + ");
      }

      if ( bTempStrands )
      {
         *pParagraph << symbol(DELTA) << RPT_STRESS(_T("ptr")) << _T(" + ");
      }

      *pParagraph << symbol(DELTA) << RPT_STRESS(_T("pLT"));
      *pParagraph << _T(" - ") << symbol(DELTA) << RPT_STRESS(_T("pED"));
      *pParagraph << _T(" - ") << symbol(DELTA) << RPT_STRESS(_T("pSIDL"));

      *pParagraph << rptNewLine;

      // fpe
      *pParagraph << RPT_FPE << _T(" = ") << RPT_FPJ << _T(" - ") << symbol(DELTA) << RPT_STRESS(_T("pT"));
      *pParagraph << rptNewLine;

      // fpe with live load service I
      *pParagraph << RPT_FPE << _T(" = ") << RPT_FPJ << _T(" - ") << symbol(DELTA) << RPT_STRESS(_T("pT"));
      *pParagraph << _T(" + ") << _T("(") << os1.str().c_str() << _T(")") << symbol(DELTA) << RPT_STRESS(_T("pLL")) << _T(" (Service I)") << rptNewLine;

      // fpe with live load service III
      *pParagraph << RPT_FPE << _T(" = ") << RPT_FPJ << _T(" - ") << symbol(DELTA) << RPT_STRESS(_T("pT"));
      *pParagraph << _T(" + ") << _T("(") << os3.str().c_str() << _T(")") << symbol(DELTA) << RPT_STRESS(_T("pLL")) << _T(" (Service III)") << rptNewLine;
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

   if ( bPTTempStrand )
   {
      (*table)(0,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pp")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   }

   if ( bTempStrands )
   {
      (*table)(0,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("ptr")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   }

   (*table)(0,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pLT")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pED")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pSIDL")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );

   (*table)(0,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pT")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,col++) << _T("Xfer") << rptNewLine << _T("Length") << rptNewLine << _T("Factor");
   (*table)(0,col++) << COLHDR(RPT_FPE, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pLL")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,col++) << COLHDR(RPT_FPE << rptNewLine << _T("with Live Load") << rptNewLine << _T("Service I"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,col++) << COLHDR(RPT_FPE << rptNewLine << _T("with Live Load") << rptNewLine << _T("Service III"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );

   return table;
}

void CEffectivePrestressTable::AddRow(rptChapter* pChapter,IBroker* pBroker,const pgsPointOfInterest& poi,RowIndexType row,const LOSSDETAILS* pDetails,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   ColumnIndexType col = 1;
   Float64 fpj = pDetails->pLosses->GetFpjPermanent();

   // Long Term Time Dependent Losses
   Float64 fpLT = pDetails->pLosses->TimeDependentLosses();   // LT = SR + CR + R1 + SD + CD + R2

   // Total Time Dependent Losses
   // Later, we will added elastic effects to this to get the total loss (delta_fpT from LRFD 5.9.5.1)
   Float64 fpT  = pDetails->pLosses->PermanentStrand_Final(); // R0 + SR + CR + R1 + SD + CD + R2 = LT + R0

   Float64 fpR0   = pDetails->pLosses->PermanentStrand_RelaxationLossesBeforeTransfer(); // R0
   Float64 fpES   = pDetails->pLosses->PermanentStrand_ElasticShorteningLosses(); // ES
   Float64 fpED   = pDetails->pLosses->ElasticGainDueToDeckPlacement(); // ED
   Float64 fpSIDL = pDetails->pLosses->ElasticGainDueToSIDL(); // SIDL
   Float64 fpLL   = pDetails->pLosses->ElasticGainDueToLiveLoad(); // LL

   Float64 fpp = 0;
   if ( m_bPTTempStrand )
   {
      fpp = pDetails->pLosses->GetDeltaFpp();
   }

   Float64 fptr = 0;
   if ( m_bTempStrands )
   {
      fptr = pDetails->pLosses->GetDeltaFptr();
   }


   // Add in elastic effects - this is now delta_fpT from LRFD 5.9.5.1
   fpT += fpES + fptr + fpp - fpED - fpSIDL;

   // Effective prestress is jacking minus total loss (which is total change in prestress)
   Float64 fpe = fpj - fpT;

   GET_IFACE2(pBroker,IPretensionForce,pPrestressForce);
   Float64 adj = pPrestressForce->GetXferLengthAdjustment(poi,pgsTypes::Permanent);
   fpe *= adj;

#if defined _DEBUG
   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType intervalIdx = pIntervals->GetIntervalCount()-1;
   Float64 _fpe_ = pPrestressForce->GetEffectivePrestress(poi,pgsTypes::Permanent,intervalIdx,pgsTypes::End);
   ATLASSERT(IsEqual(fpe,_fpe_));
#endif

   // Fill up the table row
   (*this)(row,col++) << stress.SetValue(fpj);
   if ( !m_bIgnoreInitialRelaxation )
   {
      (*this)(row,col++) << stress.SetValue(fpR0);
   }
   
   (*this)(row,col++) << stress.SetValue(fpES);
   
   if ( m_bPTTempStrand )
   {
      (*this)(row,col++) << stress.SetValue(fpp);
   }
   
   if ( m_bTempStrands )
   {
      (*this)(row,col++) << stress.SetValue(fptr);
   }

   (*this)(row,col++) << stress.SetValue(fpLT);
   (*this)(row,col++) << stress.SetValue(fpED);
   (*this)(row,col++) << stress.SetValue(fpSIDL);

   (*this)(row,col++) << stress.SetValue(fpT);
   (*this)(row,col++) << scalar.SetValue(adj);
   (*this)(row,col++) << stress.SetValue(fpe);

   (*this)(row,col++) << stress.SetValue(fpLL);
   (*this)(row,col++) << stress.SetValue(fpe + m_gLL_ServiceI*fpLL);
   (*this)(row,col++) << stress.SetValue(fpe + m_gLL_ServiceIII*fpLL);
}
