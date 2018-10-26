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

// RelaxationAtHaulingTable.cpp : Implementation of CRelaxationAtHaulingTable
#include "stdafx.h"
#include "RelaxationAtHaulingTable.h"
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <PsgLib\SpecLibraryEntry.h>
#include <PgsExt\GirderData.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CRelaxationAtHaulingTable::CRelaxationAtHaulingTable(ColumnIndexType NumColumns, IEAFDisplayUnits* pDisplayUnits) :
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

CRelaxationAtHaulingTable* CRelaxationAtHaulingTable::PrepareTable(rptChapter* pChapter,IBroker* pBroker,SpanIndexType span,GirderIndexType gdr,bool bTemporaryStrands,LOSSDETAILS& details,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   GET_IFACE2(pBroker,IGirderData,pGirderData);
   CGirderData girderData = pGirderData->GetGirderData(span,gdr);

   GET_IFACE2(pBroker,ISpecification,pSpec);
   std::_tstring strSpecName = pSpec->GetSpecification();

   GET_IFACE2(pBroker,ILibrary,pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( strSpecName.c_str() );

   // Create and configure the table
   ColumnIndexType numColumns = 7;
   if ( bTemporaryStrands )
      numColumns += 5;

   CRelaxationAtHaulingTable* table = new CRelaxationAtHaulingTable( numColumns, pDisplayUnits );
   table->m_bTemporaryStrands = bTemporaryStrands;

   pgsReportStyleHolder::ConfigureTable(table);

   std::_tstring strImagePath(pgsReportStyleHolder::GetImagePath());
   
   rptParagraph* pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;
   *pParagraph << _T("[5.9.5.4.2c] Relaxation of Prestressing Strands : ") << symbol(DELTA) << RPT_STRESS(_T("pR1H")) << rptNewLine;

   pParagraph = new rptParagraph;
   *pChapter << pParagraph;
   *pParagraph << rptRcImage(strImagePath + _T("Delta_FpR1H.png")) << rptNewLine;

   table->stress.ShowUnitTag(true);
   table->time.ShowUnitTag(true);

   *pParagraph << RPT_FY << _T(" = ") << table->stress.SetValue(details.RefinedLosses2005.GetFpy())        << rptNewLine;
   *pParagraph << Sub2(_T("K'"),_T("L")) << _T(" = ") << details.RefinedLosses2005.GetKL()                          << rptNewLine;
   *pParagraph << Sub2(_T("t"),_T("i"))  << _T(" = ") << table->time.SetValue(details.RefinedLosses2005.GetInitialAge())   << rptNewLine;
   *pParagraph << Sub2(_T("t"),_T("h"))  << _T(" = ") << table->time.SetValue(details.RefinedLosses2005.GetAgeAtHauling()) << rptNewLine;

   table->stress.ShowUnitTag(false);
   table->time.ShowUnitTag(false);

   *pParagraph << table << rptNewLine;
   (*table)(0,0) << COLHDR(_T("Location from")<<rptNewLine<<_T("End of Girder"),rptLengthUnitTag,  pDisplayUnits->GetSpanLengthUnit() );
   (*table)(0,1) << COLHDR(_T("Location from")<<rptNewLine<<_T("Left Support"),rptLengthUnitTag,  pDisplayUnits->GetSpanLengthUnit() );

   if ( bTemporaryStrands )
   {
      table->m_RowOffset = 1;
      table->SetNumberOfHeaderRows(table->m_RowOffset+1);

      table->SetRowSpan(0,0,2);
      table->SetRowSpan(0,1,2);
      table->SetRowSpan(1,0,-1);
      table->SetRowSpan(1,1,-1);

      table->SetColumnSpan(0,2,5);
      (*table)(0,2) << _T("Permanent Strands");

      table->SetColumnSpan(0,3,5);
      (*table)(0,3) << _T("Temporary Strands");

      table->SetColumnSpan(0,4,-1);
      table->SetColumnSpan(0,5,-1);
      table->SetColumnSpan(0,6,-1);
      table->SetColumnSpan(0,7,-1);
      table->SetColumnSpan(0,8,-1);
      table->SetColumnSpan(0,9,-1);
      table->SetColumnSpan(0,10,-1);
      table->SetColumnSpan(0,11,-1);

      (*table)(1,2) << COLHDR(RPT_STRESS(_T("pt")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*table)(1,3) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pSRH")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*table)(1,4) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pCRH")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*table)(1,5) << Sub2(_T("K"),_T("ih"));
      (*table)(1,6) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pR1H")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );

      (*table)(1,7) << COLHDR(RPT_STRESS(_T("pt")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*table)(1,8) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pSRH")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*table)(1,9) << COLHDR(symbol(DELTA) <<RPT_STRESS(_T("pCRH")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*table)(1,10) << Sub2(_T("K"),_T("ih"));
      (*table)(1,11) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pR1H")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   }
   else
   {
      table->m_RowOffset = 0;
      (*table)(0,2) << COLHDR(RPT_STRESS(_T("pt")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*table)(0,3) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pSRH")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*table)(0,4) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pCRH")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*table)(0,5) << Sub2(_T("K"),_T("ih"));
      (*table)(0,6) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pR1H")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   }

   return table;
}

void CRelaxationAtHaulingTable::AddRow(rptChapter* pChapter,IBroker* pBroker,RowIndexType row,LOSSDETAILS& details,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   ColumnIndexType col = 2;
   (*this)(row+m_RowOffset,col++) << stress.SetValue(details.RefinedLosses2005.GetPermanentStrandFpt());
   (*this)(row+m_RowOffset,col++) << stress.SetValue(details.RefinedLosses2005.PermanentStrand_ShrinkageLossAtShipping());
   (*this)(row+m_RowOffset,col++) << stress.SetValue(details.RefinedLosses2005.PermanentStrand_CreepLossAtShipping());
   (*this)(row+m_RowOffset,col++) << scalar.SetValue(details.RefinedLosses2005.GetPermanentStrandKih());
   (*this)(row+m_RowOffset,col++) << stress.SetValue(details.RefinedLosses2005.PermanentStrand_RelaxationLossAtShipping());

   if ( m_bTemporaryStrands )
   {
      (*this)(row+m_RowOffset,col++) << stress.SetValue(details.RefinedLosses2005.GetTemporaryStrandFpt());
      (*this)(row+m_RowOffset,col++) << stress.SetValue(details.RefinedLosses2005.TemporaryStrand_ShrinkageLossAtShipping());
      (*this)(row+m_RowOffset,col++) << stress.SetValue(details.RefinedLosses2005.TemporaryStrand_CreepLossAtShipping());
      (*this)(row+m_RowOffset,col++) << scalar.SetValue(details.RefinedLosses2005.GetTemporaryStrandKih());
      (*this)(row+m_RowOffset,col++) << stress.SetValue(details.RefinedLosses2005.TemporaryStrand_RelaxationLossAtShipping());
   }
}
