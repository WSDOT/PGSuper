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

// RelaxationAfterTransferTable.cpp : Implementation of CRelaxationAfterTransferTable
#include "stdafx.h"
#include "RelaxationAfterTransferTable.h"
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <PsgLib\SpecLibraryEntry.h>
#include <PgsExt\GirderData.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CRelaxationAfterTransferTable::CRelaxationAfterTransferTable(ColumnIndexType NumColumns, IDisplayUnits* pDispUnits) :
rptRcTable(NumColumns,0)
{
   DEFINE_UV_PROTOTYPE( spanloc,     pDispUnits->GetSpanLengthUnit(),      false );
   DEFINE_UV_PROTOTYPE( gdrloc,      pDispUnits->GetSpanLengthUnit(),      false );
   DEFINE_UV_PROTOTYPE( cg,          pDispUnits->GetSpanLengthUnit(),      false );
   DEFINE_UV_PROTOTYPE( mod_e,       pDispUnits->GetModEUnit(),            false );
   DEFINE_UV_PROTOTYPE( force,       pDispUnits->GetGeneralForceUnit(),    false );
   DEFINE_UV_PROTOTYPE( area,        pDispUnits->GetAreaUnit(),            false );
   DEFINE_UV_PROTOTYPE( mom_inertia, pDispUnits->GetMomentOfInertiaUnit(), false );
   DEFINE_UV_PROTOTYPE( ecc,         pDispUnits->GetComponentDimUnit(),    false );
   DEFINE_UV_PROTOTYPE( moment,      pDispUnits->GetMomentUnit(),          false );
   DEFINE_UV_PROTOTYPE( stress,      pDispUnits->GetStressUnit(),          false );
}

CRelaxationAfterTransferTable* CRelaxationAfterTransferTable::PrepareTable(rptChapter* pChapter,IBroker* pBroker,SpanIndexType span,GirderIndexType gdr,IDisplayUnits* pDispUnits,Uint16 level)
{
   // Create and configure the table
   ColumnIndexType numColumns = 5;

   CRelaxationAfterTransferTable* table = new CRelaxationAfterTransferTable( numColumns, pDispUnits );
   pgsReportStyleHolder::ConfigureTable(table);

   std::string strImagePath(pgsReportStyleHolder::GetImagePath());


   GET_IFACE2(pBroker, IGirderData,      pGirderData );
   CGirderData girderData = pGirderData->GetGirderData(span,gdr);
   const matPsStrand* pstrand = pGirderData->GetStrandMaterial(span,gdr);
   CHECK(pstrand);
   
   rptParagraph* pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;
   *pParagraph << "Losses due to Relaxation After Transfer" << rptNewLine;
   if ( pstrand->GetType() == matPsStrand::LowRelaxation )
   {
      if ( pDispUnits->GetUnitDisplayMode() == pgsTypes::umSI )
      {
         *pParagraph << rptRcImage(strImagePath + "Delta FpR2 Equation for Low Relax Strands SI.jpg") << rptNewLine;
      }
      else
      {
         *pParagraph << rptRcImage(strImagePath + "Delta FpR2 Equation for Low Relax Strands US.jpg") << rptNewLine;
      }
   }
   else
   {
      if ( pDispUnits->GetUnitDisplayMode() == pgsTypes::umSI )
      {
         *pParagraph << rptRcImage(strImagePath + "Delta FpR2 Equation for Stress Rel Strands SI.jpg") << rptNewLine;
      }
      else
      {
         *pParagraph << rptRcImage(strImagePath + "Delta FpR2 Equation for Stress Rel Strands US.jpg") << rptNewLine;
      }
   }

   *pParagraph << table << rptNewLine;

   (*table)(0,0) << COLHDR("Location from"<<rptNewLine<<"Left Support",rptLengthUnitTag,  pDispUnits->GetSpanLengthUnit() );
   (*table)(0,1) << COLHDR( symbol(DELTA) << "f" << Sub("pES"), rptStressUnitTag, pDispUnits->GetStressUnit() );
   (*table)(0,2) << COLHDR( symbol(DELTA) << "f" << Sub("pSR"), rptStressUnitTag, pDispUnits->GetStressUnit() );
   (*table)(0,3) << COLHDR( symbol(DELTA) << "f" << Sub("pCR"), rptStressUnitTag, pDispUnits->GetStressUnit() );
   (*table)(0,4) << COLHDR( symbol(DELTA) << "f" << Sub("pR2"), rptStressUnitTag, pDispUnits->GetStressUnit() );
   
   return table;
}

void CRelaxationAfterTransferTable::AddRow(rptChapter* pChapter,IBroker* pBroker,RowIndexType row,LOSSDETAILS& details,IDisplayUnits* pDispUnits,Uint16 level)
{
   (*this)(row,1) << stress.SetValue( details.pLosses->PermanentStrand_ElasticShorteningLosses() );
   (*this)(row,2) << stress.SetValue( details.RefinedLosses.ShrinkageLosses() );
   (*this)(row,3) << stress.SetValue( details.RefinedLosses.CreepLosses() );
   (*this)(row,4) << stress.SetValue( details.RefinedLosses.RelaxationLossesAfterXfer() );
}
