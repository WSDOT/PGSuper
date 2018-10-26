///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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
#include <PsgLib\SpecLibraryEntry.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CCreepAtFinalTable::CCreepAtFinalTable(ColumnIndexType NumColumns, IDisplayUnits* pDisplayUnits) :
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

   scalar.SetFormat( sysNumericFormatTool::Automatic );
   scalar.SetWidth(6);
   scalar.SetPrecision(2);
}

CCreepAtFinalTable* CCreepAtFinalTable::PrepareTable(rptChapter* pChapter,IBroker* pBroker,SpanIndexType span,GirderIndexType gdr,LOSSDETAILS& details,IDisplayUnits* pDisplayUnits,Uint16 level)
{
   GET_IFACE2(pBroker,IGirderData,pGirderData);
   CGirderData girderData = pGirderData->GetGirderData(span,gdr);

   GET_IFACE2(pBroker,ISpecification,pSpec);
   std::string strSpecName = pSpec->GetSpecification();

   GET_IFACE2(pBroker,ILibrary,pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( strSpecName.c_str() );

   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);
   StrandIndexType NtMax = pStrandGeom->GetMaxStrands(span,gdr,pgsTypes::Temporary);

   // Create and configure the table
   ColumnIndexType numColumns = 5;

   if ( 0 < NtMax )
   {
      if ( girderData.TempStrandUsage == pgsTypes::ttsPretensioned )
         numColumns++;
      else
         numColumns+=2;
   }

   CCreepAtFinalTable* table = new CCreepAtFinalTable( numColumns, pDisplayUnits );
   pgsReportStyleHolder::ConfigureTable(table);

   
   table->m_GirderData = girderData;
   table->m_NtMax = NtMax;

   std::string strImagePath(pgsReportStyleHolder::GetImagePath());
   
   rptParagraph* pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;

   *pParagraph << "[5.9.5.4.3b] Creep of Girder Concrete : " << symbol(DELTA) << Sub2("f","pCD") << rptNewLine;

   
   pParagraph = new rptParagraph;
   *pChapter << pParagraph;

   if ( 0 < NtMax )
   {
      if ( girderData.TempStrandUsage != pgsTypes::ttsPretensioned )
      {
         if ( pSpecEntry->GetSpecificationType() < lrfdVersionMgr::FourthEdition2007 )
            *pParagraph << rptRcImage(strImagePath + "Delta_FpCD_PT.gif") << rptNewLine;
         else
            *pParagraph << rptRcImage(strImagePath + "Delta_FpCD_2007_PT.gif") << rptNewLine;
      }
      else
      {
         if ( pSpecEntry->GetSpecificationType() < lrfdVersionMgr::FourthEdition2007 )
            *pParagraph << rptRcImage(strImagePath + "Delta_FpCD_PS.gif") << rptNewLine;
         else
            *pParagraph << rptRcImage(strImagePath + "Delta_FpCD_2007_PS.gif") << rptNewLine;
      }
   }
   else
   {
      if ( pSpecEntry->GetSpecificationType() < lrfdVersionMgr::FourthEdition2007 )
         *pParagraph << rptRcImage(strImagePath + "Delta_FpCD.gif") << rptNewLine;
      else
         *pParagraph << rptRcImage(strImagePath + "Delta_FpCD_2007.gif") << rptNewLine;
   }

   // DELTA Fcd Table
   pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;

   pParagraph = new rptParagraph;
   *pChapter << pParagraph;

   rptRcTable* pParamTable = pgsReportStyleHolder::CreateDefaultTable(5,"");
   *pParagraph << pParamTable << rptNewLine;
   (*pParamTable)(0,0) << COLHDR(Sub2("E","p"), rptStressUnitTag, pDisplayUnits->GetStressUnit());
   (*pParamTable)(0,1) << COLHDR(Sub2("E","c"), rptStressUnitTag, pDisplayUnits->GetStressUnit());
   (*pParamTable)(0,2) << COLHDR(Sub2("E","ci"), rptStressUnitTag, pDisplayUnits->GetStressUnit());
   (*pParamTable)(0,3) << COLHDR(RPT_FC,rptStressUnitTag,pDisplayUnits->GetStressUnit());
   (*pParamTable)(0,4) << Sub2("k","f");

   (*pParamTable)(1,0) << table->mod_e.SetValue( details.RefinedLosses2005.GetEp() );
   (*pParamTable)(1,1) << table->mod_e.SetValue( details.RefinedLosses2005.GetEc() );
   (*pParamTable)(1,2) << table->mod_e.SetValue( details.RefinedLosses2005.GetEci() );
   (*pParamTable)(1,3) << table->stress.SetValue(details.RefinedLosses2005.GetFc());
   (*pParamTable)(1,4) << table->scalar.SetValue(details.RefinedLosses2005.GetCreepDeckToFinal().GetKf());

   pParamTable = pgsReportStyleHolder::CreateDefaultTable(5,"");
   *pParagraph << pParamTable << rptNewLine;
   (*pParamTable)(0,0) << COLHDR(Sub2("t","i"), rptTimeUnitTag, pDisplayUnits->GetLongTimeUnit());
   (*pParamTable)(0,1) << COLHDR(Sub2("t","d"), rptTimeUnitTag, pDisplayUnits->GetLongTimeUnit());
   (*pParamTable)(0,2) << COLHDR(Sub2("t","f"), rptTimeUnitTag, pDisplayUnits->GetLongTimeUnit());

   table->time.ShowUnitTag(true);
   (*pParamTable)(0,3) << Sub2("k","td") << rptNewLine << "t = " << table->time.SetValue(details.RefinedLosses2005.GetAdjustedInitialAge());
   (*pParamTable)(0,4) << Sub2("k","td") << rptNewLine << "t = " << table->time.SetValue(details.RefinedLosses2005.GetAgeAtDeckPlacement());
   table->time.ShowUnitTag(false);

   (*pParamTable)(1,0) << table->time.SetValue( details.RefinedLosses2005.GetAdjustedInitialAge() );
   (*pParamTable)(1,1) << table->time.SetValue( details.RefinedLosses2005.GetAgeAtDeckPlacement() );
   (*pParamTable)(1,2) << table->time.SetValue( details.RefinedLosses2005.GetFinalAge() );
   (*pParamTable)(1,3) << table->scalar.SetValue(details.RefinedLosses2005.GetCreepInitialToFinal().GetKtd());
   (*pParamTable)(1,4) << table->scalar.SetValue(details.RefinedLosses2005.GetCreepDeckToFinal().GetKtd());

   pParamTable = pgsReportStyleHolder::CreateDefaultTable(3,"");
   *pParagraph << pParamTable << rptNewLine;
   (*pParamTable)(0,0) << Sub2(symbol(psi),"b") << "(" << Sub2("t","f") << "," << Sub2("t","i") << ")";
   (*pParamTable)(0,1) << Sub2(symbol(psi),"b") << "(" << Sub2("t","d") << "," << Sub2("t","i") << ")";
   (*pParamTable)(0,2) << Sub2(symbol(psi),"b") << "(" << Sub2("t","f") << "," << Sub2("t","d") << ")";

   (*pParamTable)(1,0) << table->scalar.SetValue(details.RefinedLosses2005.GetCreepInitialToFinal().GetCreepCoefficient());
   (*pParamTable)(1,1) << table->scalar.SetValue(details.RefinedLosses2005.GetCreepInitialToDeck().GetCreepCoefficient());
   (*pParamTable)(1,2) << table->scalar.SetValue(details.RefinedLosses2005.GetCreepDeckToFinal().GetCreepCoefficient());


   
   *pParagraph << table << rptNewLine;

   ColumnIndexType col = 0;

   (*table)(0,col++) << COLHDR("Location from"<<rptNewLine<<"Left Support",rptLengthUnitTag,  pDisplayUnits->GetSpanLengthUnit() );
   (*table)(0,col++) << Sub2("K","df");

  (*table)(0,col++) << COLHDR(Sub2("f","cgp"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   if ( 0 < NtMax &&  girderData.TempStrandUsage != pgsTypes::ttsPretensioned )
   {
      (*table)(0,col++) << COLHDR(symbol(DELTA) << Sub2("f","pp"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   }

   if ( 0 < NtMax )
   {
      (*table)(0,col++) << COLHDR(symbol(DELTA) << Sub2("f","ptr"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   }

   (*table)(0,col++) << COLHDR(symbol(DELTA) << Sub2("f","cd"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,col++) << COLHDR(symbol(DELTA) << Sub2("f","pCD"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );

   return table;
}

void CCreepAtFinalTable::AddRow(rptChapter* pChapter,IBroker* pBroker,RowIndexType row,LOSSDETAILS& details,IDisplayUnits* pDisplayUnits,Uint16 level)
{
   ColumnIndexType col = 1;
   (*this)(row,col++) << scalar.SetValue(details.RefinedLosses2005.GetKdf());
   (*this)(row,col++) << stress.SetValue( details.pLosses->ElasticShortening().PermanentStrand_Fcgp() );

   if ( 0 < m_NtMax && m_GirderData.TempStrandUsage != pgsTypes::ttsPretensioned )
      (*this)(row,col++) << stress.SetValue( details.RefinedLosses2005.GetDeltaFpp() );

   if ( 0 < m_NtMax )
      (*this)(row,col++) << stress.SetValue( details.RefinedLosses2005.GetDeltaFptr() );

   (*this)(row,col++) << stress.SetValue( details.RefinedLosses2005.GetDeltaFcd() );
   (*this)(row,col++) << stress.SetValue( details.RefinedLosses2005.CreepLossAfterDeckPlacement() );
}
