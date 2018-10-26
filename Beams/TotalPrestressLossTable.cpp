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

// TotalPrestressLossTable.cpp : Implementation of CTotalPrestressLossTable
#include "stdafx.h"
#include "TotalPrestressLossTable.h"
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <PsgLib\SpecLibraryEntry.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CTotalPrestressLossTable::CTotalPrestressLossTable(ColumnIndexType NumColumns, IDisplayUnits* pDispUnit) :
rptRcTable(NumColumns,0)
{
   DEFINE_UV_PROTOTYPE( spanloc,     pDispUnit->GetSpanLengthUnit(),      false );
   DEFINE_UV_PROTOTYPE( gdrloc,      pDispUnit->GetSpanLengthUnit(),      false );
   DEFINE_UV_PROTOTYPE( offset,      pDispUnit->GetSpanLengthUnit(),      false );
   DEFINE_UV_PROTOTYPE( mod_e,       pDispUnit->GetModEUnit(),            false );
   DEFINE_UV_PROTOTYPE( force,       pDispUnit->GetGeneralForceUnit(),    false );
   DEFINE_UV_PROTOTYPE( area,        pDispUnit->GetAreaUnit(),            false );
   DEFINE_UV_PROTOTYPE( mom_inertia, pDispUnit->GetMomentOfInertiaUnit(), false );
   DEFINE_UV_PROTOTYPE( ecc,         pDispUnit->GetComponentDimUnit(),    false );
   DEFINE_UV_PROTOTYPE( moment,      pDispUnit->GetMomentUnit(),          false );
   DEFINE_UV_PROTOTYPE( stress,      pDispUnit->GetStressUnit(),          false );

   scalar.SetFormat( sysNumericFormatTool::Fixed );
   scalar.SetWidth(5); // -99.9
   scalar.SetPrecision(1);
   scalar.SetTolerance(1.0e-6);
}

CTotalPrestressLossTable* CTotalPrestressLossTable::PrepareTable(rptChapter* pChapter,IBroker* pBroker,SpanIndexType span,GirderIndexType gdr,IDisplayUnits* pDispUnit,Uint16 level)
{
   GET_IFACE2(pBroker,IGirderData,pGirderData);
   CGirderData girderData = pGirderData->GetGirderData(span,gdr);

   GET_IFACE2(pBroker,ISpecification,pSpec);
   std::string strSpecName = pSpec->GetSpecification();

   GET_IFACE2(pBroker,ILibrary,pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( strSpecName.c_str() );

   int method = pSpecEntry->GetLossMethod();
   bool bIgnoreInitialRelaxation = ( method == LOSSES_WSDOT_REFINED || method == LOSSES_WSDOT_LUMPSUM ) ? false : true;

   bool bIgnoreElasticGain = ( pSpecEntry->GetSpecificationType() <= lrfdVersionMgr::ThirdEdition2004 ) ? true : false;

   ColumnIndexType numColumns = 5;
   if ( girderData.TempStrandUsage == pgsTypes::ttsPretensioned ) 
   {
      if ( bIgnoreInitialRelaxation )
      {
         numColumns = 6;
      }
      else
      {
         numColumns = 7;
      }
   }
   else
   {
      if ( bIgnoreInitialRelaxation )
      {
         numColumns = 7;
      }
      else
      {
         numColumns = 8;
      }
   }

   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);
   StrandIndexType NtMax = pStrandGeom->GetMaxStrands(span,gdr,pgsTypes::Temporary);
   if ( 0 == NtMax  )
      numColumns--;

   if ( bIgnoreElasticGain )
      numColumns--;


   // add one column for % Loss
   numColumns += 1;

   // Create and configure the table
   CTotalPrestressLossTable* table = new CTotalPrestressLossTable( numColumns, pDispUnit );
   pgsReportStyleHolder::ConfigureTable(table);

   std::string strImagePath(pgsReportStyleHolder::GetImagePath());
   
   rptParagraph* pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;
   *pParagraph << "Total Prestress Loss" << rptNewLine;

   pParagraph = new rptParagraph;
   *pChapter << pParagraph;

   std::string strYear = (bIgnoreElasticGain ? "" : "_2005");
   if ( 0 < NtMax )
   {
      if ( girderData.TempStrandUsage == pgsTypes::ttsPretensioned ) 
      {
         if ( bIgnoreInitialRelaxation )
         {
            *pParagraph << rptRcImage(strImagePath + "TotalPrestressLossWithPS_LRFD" + strYear + ".gif") << rptNewLine;
         }
         else
         {
            *pParagraph << rptRcImage(strImagePath + "TotalPrestressLossWithPS_WSDOT" + strYear + ".gif") << rptNewLine;
         }
      }
      else
      {
         if ( bIgnoreInitialRelaxation )
         {
            *pParagraph << rptRcImage(strImagePath + "TotalPrestressLossWithPT_LRFD" + strYear + ".gif") << rptNewLine;
         }
         else
         {
            *pParagraph << rptRcImage(strImagePath + "TotalPrestressLossWithPT_WSDOT" + strYear + ".gif") << rptNewLine;
         }
      }
   }
   else
   {
      if ( bIgnoreInitialRelaxation )
      {
         *pParagraph << rptRcImage(strImagePath + "TotalPrestressLoss_LRFD" + strYear + ".gif") << rptNewLine;
      }
      else
      {
         *pParagraph << rptRcImage(strImagePath + "TotalPrestressLoss_WSDOT" + strYear + ".gif") << rptNewLine;
      }
   }

   *pParagraph << table << rptNewLine;

   int col = 0;
   (*table)(0,col++) << COLHDR("Location from"<<rptNewLine<<"Left Support",rptLengthUnitTag,  pDispUnit->GetSpanLengthUnit() );

   if ( !bIgnoreInitialRelaxation )
   {
      (*table)(0,col++) << COLHDR(symbol(DELTA) << Sub2("f","pR0"), rptStressUnitTag, pDispUnit->GetStressUnit() );
   }

   (*table)(0,col++) << COLHDR(symbol(DELTA) << Sub2("f","pES"), rptStressUnitTag, pDispUnit->GetStressUnit() );

   if ( 0 < NtMax && girderData.TempStrandUsage != pgsTypes::ttsPretensioned ) 
   {
      (*table)(0,col++) << COLHDR(symbol(DELTA) << Sub2("f","pp"), rptStressUnitTag, pDispUnit->GetStressUnit() );
   }

   if ( 0 < NtMax )
      (*table)(0,col++) << COLHDR(symbol(DELTA) << Sub2("f","ptr"), rptStressUnitTag, pDispUnit->GetStressUnit() );

   if ( !bIgnoreElasticGain )
      (*table)(0,col++) << COLHDR(symbol(DELTA) << Sub2("f","pED"), rptStressUnitTag, pDispUnit->GetStressUnit() );


   (*table)(0,col++) << COLHDR(symbol(DELTA) << Sub2("f","pLT"), rptStressUnitTag, pDispUnit->GetStressUnit() );
   (*table)(0,col++) << COLHDR(symbol(DELTA) << Sub2("f","pT"), rptStressUnitTag, pDispUnit->GetStressUnit() );
   (*table)(0,col++) << "% Loss";

   table->m_NtMax = NtMax;
   table->m_GirderData = girderData;
   table->m_bIgnoreElasticGain = bIgnoreElasticGain;

   return table;
}

void CTotalPrestressLossTable::AddRow(rptChapter* pChapter,IBroker* pBroker,RowIndexType row,LOSSDETAILS& details,IDisplayUnits* pDispUnit,Uint16 level)
{
   ColumnIndexType col = 1;
   if ( !details.pLosses->IgnoreInitialRelaxation() )
   {
      (*this)(row,col++) << stress.SetValue(details.pLosses->PermanentStrand_RelaxationLossesBeforeTransfer());
   }

   (*this)(row,col++) << stress.SetValue(details.pLosses->PermanentStrand_ElasticShorteningLosses());
   
   if ( 0 < m_NtMax && m_GirderData.TempStrandUsage != pgsTypes::ttsPretensioned ) 
   {
      (*this)(row,col++) << stress.SetValue(details.pLosses->GetDeltaFpp());
   }


   if ( 0 < m_NtMax )
      (*this)(row,col++) << stress.SetValue(details.pLosses->GetDeltaFptr());

   if ( !m_bIgnoreElasticGain )
      (*this)(row,col++) << stress.SetValue(details.pLosses->ElasticGainDueToDeckPlacement());


   (*this)(row,col++) << stress.SetValue(details.pLosses->TimeDependentLosses());
   (*this)(row,col++) << stress.SetValue(details.pLosses->PermanentStrand_Final());

   double fpj = details.pLosses->GetFpjPermanent();
   double fp  = fpj - details.pLosses->PermanentStrand_Final();
   (*this)(row,col++) << scalar.SetValue( -1*PercentChange(fpj,fp) );
}
