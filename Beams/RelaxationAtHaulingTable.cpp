///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2018  Washington State Department of Transportation
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
#include <Reporting\ReportNotes.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CRelaxationAtHaulingTable::CRelaxationAtHaulingTable(ColumnIndexType NumColumns, IEAFDisplayUnits* pDisplayUnits) :
rptRcTable(NumColumns,0)
{
   DEFINE_UV_PROTOTYPE( stress,      pDisplayUnits->GetStressUnit(),          false );
   DEFINE_UV_PROTOTYPE( time,        pDisplayUnits->GetWholeDaysUnit(),       false );

   scalar.SetFormat( sysNumericFormatTool::Automatic );
   scalar.SetWidth(6);
   scalar.SetPrecision(2);
}

CRelaxationAtHaulingTable* CRelaxationAtHaulingTable::PrepareTable(rptChapter* pChapter,IBroker* pBroker,const CSegmentKey& segmentKey,bool bTemporaryStrands,const LOSSDETAILS* pDetails,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   GET_IFACE2(pBroker,ISpecification,pSpec);
   std::_tstring strSpecName = pSpec->GetSpecification();

   GET_IFACE2(pBroker,ILibrary,pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( strSpecName.c_str() );

  // Typecast to our known type (eating own doggy food)
   std::shared_ptr<const lrfdRefinedLosses2005> ptl = std::dynamic_pointer_cast<const lrfdRefinedLosses2005>(pDetails->pLosses);
   if (!ptl)
   {
      ATLASSERT(false); // made a bad cast? Bail...
      return nullptr;
   }

   // Create and configure the table
   ColumnIndexType numColumns = 3;
   if ( ptl->GetRelaxationLossMethod() == lrfdRefinedLosses2005::Simplified )
   {
      numColumns++;
   }
   else if (ptl->GetRelaxationLossMethod() == lrfdRefinedLosses2005::Refined )
   {
      numColumns += 4;
   }

   if ( bTemporaryStrands )
   {
      numColumns++;

      if ( ptl->GetRelaxationLossMethod() == lrfdRefinedLosses2005::Simplified )
      {
         numColumns++;
      }
      else if (ptl->GetRelaxationLossMethod() == lrfdRefinedLosses2005::Refined )
      {
         numColumns += 4;
      }
   }

   CRelaxationAtHaulingTable* table = new CRelaxationAtHaulingTable( numColumns, pDisplayUnits );
   table->m_bTemporaryStrands = bTemporaryStrands;
   table->stress.ShowUnitTag(true);
   table->time.ShowUnitTag(true);

   rptStyleManager::ConfigureTable(table);

   std::_tstring strImagePath(rptStyleManager::GetImagePath());
   
   rptParagraph* pParagraph = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pParagraph;
   *pParagraph << _T("[") << LrfdCw8th(_T("5.9.5.4.2c"),_T("5.9.3.4.2c")) << _T("] Relaxation of Prestressing Strands : ") << symbol(DELTA) << RPT_STRESS(_T("pR1H")) << rptNewLine;

   pParagraph = new rptParagraph(rptStyleManager::GetSubheadingStyle());
   *pChapter << pParagraph;
   *pParagraph << _T("Permanent Strands") << rptNewLine;
   pParagraph = new rptParagraph;
   *pChapter << pParagraph;
   switch(ptl->GetRelaxationLossMethod() )
   {
   case lrfdRefinedLosses2005::Simplified:
      *pParagraph << rptRcImage(strImagePath + _T("Delta_FpR1H_Simplified.png")) << rptNewLine;
      *pParagraph << RPT_FY << _T(" = ") << table->stress.SetValue(ptl->GetFpyPermanent())              << rptNewLine;
      *pParagraph << Sub2(_T("K"),_T("L")) << _T(" = ") << ptl->GetPermanentStrandKL()                        << rptNewLine;
      break;

   case lrfdRefinedLosses2005::Refined:
      *pParagraph << rptRcImage(strImagePath + _T("Delta_FpR1H.png")) << rptNewLine;
      *pParagraph << RPT_FY << _T(" = ") << table->stress.SetValue(ptl->GetFpyPermanent())                              << rptNewLine;
      *pParagraph << Sub2(_T("K'"),_T("L")) << _T(" = ") << ptl->GetPermanentStrandKL()                                       << rptNewLine;
      *pParagraph << Sub2(_T("t"),_T("i"))  << _T(" = ") << table->time.SetValue(ptl->GetInitialAge())         << rptNewLine;
      *pParagraph << Sub2(_T("t"),_T("h"))  << _T(" = ") << table->time.SetValue(ptl->GetAgeAtHauling()) << rptNewLine;
      break;

   case lrfdRefinedLosses2005::LumpSum:
      break;

   default:
      ATLASSERT(false); // should never get here
      break;
   }

   if ( ptl->GetPermanentStrandCoating() != matPsStrand::None )
   {
      *pParagraph << EPOXY_RELAXATION_NOTE << rptNewLine;
   }

   if ( bTemporaryStrands )
   {
      pParagraph = new rptParagraph(rptStyleManager::GetSubheadingStyle());
      *pChapter << pParagraph;
      *pParagraph << _T("Temporary Strands") << rptNewLine;
      pParagraph = new rptParagraph;
      *pChapter << pParagraph;
      switch(ptl->GetRelaxationLossMethod() )
      {
      case lrfdRefinedLosses2005::Simplified:
         *pParagraph << rptRcImage(strImagePath + _T("Delta_FpR1H_Simplified.png")) << rptNewLine;
         *pParagraph << RPT_FY << _T(" = ") << table->stress.SetValue(ptl->GetFpyTemporary())              << rptNewLine;
         *pParagraph << Sub2(_T("K"),_T("L")) << _T(" = ") << ptl->GetTemporaryStrandKL()                        << rptNewLine;
         break;

      case lrfdRefinedLosses2005::Refined:
         *pParagraph << rptRcImage(strImagePath + _T("Delta_FpR1H.png")) << rptNewLine;
         *pParagraph << RPT_FY << _T(" = ") << table->stress.SetValue(ptl->GetFpyTemporary())                              << rptNewLine;
         *pParagraph << Sub2(_T("K'"),_T("L")) << _T(" = ") << ptl->GetTemporaryStrandKL()                                       << rptNewLine;
         *pParagraph << Sub2(_T("t"),_T("i"))  << _T(" = ") << table->time.SetValue(ptl->GetInitialAge())         << rptNewLine;
         *pParagraph << Sub2(_T("t"),_T("h"))  << _T(" = ") << table->time.SetValue(ptl->GetAgeAtHauling()) << rptNewLine;
         break;

      case lrfdRefinedLosses2005::LumpSum:
         break;

      default:
         ATLASSERT(false); // should never get here
         break;
      }

      if ( ptl->GetTemporaryStrandCoating() != matPsStrand::None )
      {
         *pParagraph << EPOXY_RELAXATION_NOTE << rptNewLine;
      }
   }


   table->stress.ShowUnitTag(false);
   table->time.ShowUnitTag(false);

   ColumnIndexType col = 0;

   *pParagraph << table << rptNewLine;
   (*table)(0,0) << COLHDR(_T("Location from")<<rptNewLine<<_T("End of Girder"),rptLengthUnitTag,  pDisplayUnits->GetSpanLengthUnit() );
   (*table)(0,1) << COLHDR(_T("Location from")<<rptNewLine<<_T("Left Support"),rptLengthUnitTag,  pDisplayUnits->GetSpanLengthUnit() );

   if ( bTemporaryStrands )
   {
      table->m_RowOffset = 1;
      table->SetNumberOfHeaderRows(table->m_RowOffset+1);

      table->SetRowSpan(0,col,2);
      table->SetRowSpan(1,col++,SKIP_CELL);
      table->SetRowSpan(0,col,2);
      table->SetRowSpan(1,col++,SKIP_CELL);

      if ( ptl->GetRelaxationLossMethod() == lrfdRefinedLosses2005::Simplified )
      {
         table->SetColumnSpan(0,col,2);
         (*table)(0,col++) << _T("Permanent Strands");

         table->SetColumnSpan(0,col,2);
         (*table)(0,col++) << _T("Temporary Strands");

         table->SetColumnSpan(0,col++,SKIP_CELL);
         table->SetColumnSpan(0,col++,SKIP_CELL);
      }
      else if ( ptl->GetRelaxationLossMethod() == lrfdRefinedLosses2005::Refined )
      {
         table->SetColumnSpan(0,col,5);
         (*table)(0,col++) << _T("Permanent Strands");

         table->SetColumnSpan(0,col,5);
         (*table)(0,col++) << _T("Temporary Strands");

         table->SetColumnSpan(0,col++,SKIP_CELL);
         table->SetColumnSpan(0,col++,SKIP_CELL);
         table->SetColumnSpan(0,col++,SKIP_CELL);
         table->SetColumnSpan(0,col++,SKIP_CELL);
         table->SetColumnSpan(0,col++,SKIP_CELL);
         table->SetColumnSpan(0,col++,SKIP_CELL);
         table->SetColumnSpan(0,col++,SKIP_CELL);
         table->SetColumnSpan(0,col++,SKIP_CELL);
      }
      else if ( ptl->GetRelaxationLossMethod() == lrfdRefinedLosses2005::LumpSum )
      {
         (*table)(0,col++) << _T("Permanent Strands");
         (*table)(0,col++) << _T("Temporary Strands");
      }
      else
      {
         ATLASSERT(false); // should never get here
      }

      col=2;
      if ( ptl->GetRelaxationLossMethod() == lrfdRefinedLosses2005::Simplified )
      {
         (*table)(1,col++) << COLHDR(RPT_STRESS(_T("pt")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
         (*table)(1,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pR1H")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );

         (*table)(1,col++) << COLHDR(RPT_STRESS(_T("pt")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
         (*table)(1,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pR1H")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      }
      else if ( ptl->GetRelaxationLossMethod() == lrfdRefinedLosses2005::Refined )
      {
         (*table)(1,col++) << COLHDR(RPT_STRESS(_T("pt")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
         (*table)(1,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pSRH")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
         (*table)(1,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pCRH")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
         (*table)(1,col++) << Sub2(_T("K"),_T("ih"));
         (*table)(1,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pR1H")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );

         (*table)(1,col++) << COLHDR(RPT_STRESS(_T("pt")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
         (*table)(1,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pSRH")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
         (*table)(1,col++) << COLHDR(symbol(DELTA) <<RPT_STRESS(_T("pCRH")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
         (*table)(1,col++) << Sub2(_T("K"),_T("ih"));
         (*table)(1,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pR1H")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      }
      else if ( ptl->GetRelaxationLossMethod() == lrfdRefinedLosses2005::LumpSum )
      {
         (*table)(1,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pR1H")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
         (*table)(1,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pR1H")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      }
      else
      {
         ATLASSERT(false); // should never get here
      }
   }
   else
   {
      table->m_RowOffset = 0;
      col=2;
      if ( ptl->GetRelaxationLossMethod() == lrfdRefinedLosses2005::Simplified )
      {
         (*table)(0,col++) << COLHDR(RPT_STRESS(_T("pt")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
         (*table)(0,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pR1H")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      }
      else if ( ptl->GetRelaxationLossMethod() == lrfdRefinedLosses2005::Refined )
      {
         (*table)(0,col++) << COLHDR(RPT_STRESS(_T("pt")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
         (*table)(0,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pSRH")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
         (*table)(0,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pCRH")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
         (*table)(0,col++) << Sub2(_T("K"),_T("ih"));
         (*table)(0,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pR1H")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      }
      else if ( ptl->GetRelaxationLossMethod() == lrfdRefinedLosses2005::LumpSum )
      {
         (*table)(0,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pR1H")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      }
      else
      {
         ATLASSERT(false);
      }

   }

   return table;
}

void CRelaxationAtHaulingTable::AddRow(rptChapter* pChapter,IBroker* pBroker,const pgsPointOfInterest& poi,RowIndexType row,const LOSSDETAILS* pDetails,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   ColumnIndexType col = 2;

  // Typecast to our known type (eating own doggy food)
   std::shared_ptr<const lrfdRefinedLosses2005> ptl = std::dynamic_pointer_cast<const lrfdRefinedLosses2005>(pDetails->pLosses);
   if (!ptl)
   {
      ATLASSERT(false); // made a bad cast? Bail...
      return;
   }

   if ( ptl->GetRelaxationLossMethod() == lrfdRefinedLosses2005::Simplified )
   {
      (*this)(row+m_RowOffset,col++) << stress.SetValue(ptl->GetPermanentStrandFpt());
      (*this)(row+m_RowOffset,col++) << stress.SetValue(ptl->PermanentStrand_RelaxationLossAtShipping());
   }
   else if ( ptl->GetRelaxationLossMethod() == lrfdRefinedLosses2005::Refined )
   {
      (*this)(row+m_RowOffset,col++) << stress.SetValue(ptl->GetPermanentStrandFpt());
      (*this)(row+m_RowOffset,col++) << stress.SetValue(ptl->PermanentStrand_ShrinkageLossAtShipping());
      (*this)(row+m_RowOffset,col++) << stress.SetValue(ptl->PermanentStrand_CreepLossAtShipping());
      (*this)(row+m_RowOffset,col++) << scalar.SetValue(ptl->GetPermanentStrandKih());
      (*this)(row+m_RowOffset,col++) << stress.SetValue(ptl->PermanentStrand_RelaxationLossAtShipping());
   }
   else if ( ptl->GetRelaxationLossMethod() == lrfdRefinedLosses2005::LumpSum )
   {
      (*this)(row+m_RowOffset,col++) << stress.SetValue(ptl->PermanentStrand_RelaxationLossAtShipping());
   }
   else
   {
      ATLASSERT(false);
   }

   if ( m_bTemporaryStrands )
   {
      if ( ptl->GetRelaxationLossMethod() == lrfdRefinedLosses2005::Simplified )
      {
         (*this)(row+m_RowOffset,col++) << stress.SetValue(ptl->GetTemporaryStrandFpt());
         (*this)(row+m_RowOffset,col++) << stress.SetValue(ptl->TemporaryStrand_RelaxationLossAtShipping());
      }
      else if ( ptl->GetRelaxationLossMethod() == lrfdRefinedLosses2005::Refined )
      {
         (*this)(row+m_RowOffset,col++) << stress.SetValue(ptl->GetTemporaryStrandFpt());
         (*this)(row+m_RowOffset,col++) << stress.SetValue(ptl->TemporaryStrand_ShrinkageLossAtShipping());
         (*this)(row+m_RowOffset,col++) << stress.SetValue(ptl->TemporaryStrand_CreepLossAtShipping());
         (*this)(row+m_RowOffset,col++) << scalar.SetValue(ptl->GetTemporaryStrandKih());
         (*this)(row+m_RowOffset,col++) << stress.SetValue(ptl->TemporaryStrand_RelaxationLossAtShipping());
      }
      else if ( ptl->GetRelaxationLossMethod() == lrfdRefinedLosses2005::LumpSum )
      {
         (*this)(row+m_RowOffset,col++) << stress.SetValue(ptl->TemporaryStrand_RelaxationLossAtShipping());
      }
      else
      {
         ATLASSERT(false);
      }
   }
}
