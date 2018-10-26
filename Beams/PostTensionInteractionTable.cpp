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

// PostTensionInteractionTable.cpp : Implementation of CPostTensionInteractionTable
#include "stdafx.h"
#include "PostTensionInteractionTable.h"
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <PsgLib\SpecLibraryEntry.h>
#include <PgsExt\GirderData.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CPostTensionInteractionTable::CPostTensionInteractionTable(ColumnIndexType NumColumns, IEAFDisplayUnits* pDisplayUnits) :
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
}

CPostTensionInteractionTable* CPostTensionInteractionTable::PrepareTable(rptChapter* pChapter,IBroker* pBroker,SpanIndexType span,GirderIndexType gdr,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   // Create and configure table
   ColumnIndexType numColumns = 9;
   CPostTensionInteractionTable* table = new CPostTensionInteractionTable( numColumns, pDisplayUnits );
   pgsReportStyleHolder::ConfigureTable(table);

   std::string strImagePath(pgsReportStyleHolder::GetImagePath());
   
   GET_IFACE2(pBroker,IGirderData,pGirderData);
   CGirderData girderData = pGirderData->GetGirderData(span,gdr);
   pgsTypes::TTSUsage tempStrandUsage = girderData.TempStrandUsage;

   // gather some data
   GET_IFACE2(pBroker,IBridgeMaterial,pMaterial);
   double Eci = (tempStrandUsage == pgsTypes::ttsPTBeforeShipping ? pMaterial->GetEcGdr(span,gdr) : pMaterial->GetEciGdr(span,gdr));
   double Ep  = pMaterial->GetStrand(span,gdr)->GetE();

   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);
   double nEffectiveStrands;
   double ept = pStrandGeom->GetTempEccentricity( pgsPointOfInterest(span,gdr,0), &nEffectiveStrands);
   double Apt = pStrandGeom->GetStrandArea(span,gdr,pgsTypes::Temporary);
   StrandIndexType Npt = pStrandGeom->GetNumStrands(span,gdr,pgsTypes::Temporary);

   rptParagraph* pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;
   *pParagraph << "Effect of strand jacking on previously stressed strands" << rptNewLine;

   pParagraph = new rptParagraph;
   *pChapter << pParagraph;

   if ( tempStrandUsage == pgsTypes::ttsPTBeforeShipping )
      *pParagraph << rptRcImage(strImagePath + "Delta_Fpt_BeforeShipping.gif") << rptNewLine;
   else
      *pParagraph << rptRcImage(strImagePath + "Delta_Fpt.gif") << rptNewLine;

   table->mod_e.ShowUnitTag(true);
   table->area.ShowUnitTag(true);
   table->ecc.ShowUnitTag(true);
   
   *pParagraph << Sub2("E","p") << " = " << table->mod_e.SetValue(Ep) << rptNewLine;

   if ( tempStrandUsage == pgsTypes::ttsPTBeforeShipping )
      *pParagraph << Sub2("E","c") << " = " << table->mod_e.SetValue(Eci) << rptNewLine;
   else
      *pParagraph << Sub2("E","ci") << " = " << table->mod_e.SetValue(Eci) << rptNewLine;

   *pParagraph << Sub2("e","pt") << " = " << table->ecc.SetValue(ept) << rptNewLine;
   *pParagraph << Sub2("A","pt") << " = " << table->area.SetValue(Apt) << rptNewLine;

   table->mod_e.ShowUnitTag(false);
   table->area.ShowUnitTag(false);
   table->ecc.ShowUnitTag(false);

   *pParagraph << table << rptNewLine;
   (*table)(0,0) << COLHDR("Location from"<<rptNewLine<<"End of Girder",rptLengthUnitTag,  pDisplayUnits->GetSpanLengthUnit() );
   (*table)(0,1) << COLHDR("Location from"<<rptNewLine<<"Left Support",rptLengthUnitTag,  pDisplayUnits->GetSpanLengthUnit() );
   (*table)(0,2) << COLHDR("x",rptLengthUnitTag,pDisplayUnits->GetSpanLengthUnit());
   (*table)(0,3) << COLHDR(Sub2("f","pt max"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,4) << COLHDR(Sub2("P","pt"), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit() );
   (*table)(0,5) << COLHDR(Sub2("A","g"), rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );
   (*table)(0,6) << COLHDR(Sub2("I","g"), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
   (*table)(0,7) << COLHDR(Sub2("f","cgpt"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,8) << COLHDR(symbol(DELTA) << Sub2("f","pt"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );

   return table;
}

void CPostTensionInteractionTable::AddRow(rptChapter* pChapter,IBroker* pBroker,RowIndexType row,LOSSDETAILS& details,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   (*this)(row,2) << offset.SetValue( details.pLosses->GetLocation() );
   (*this)(row,3) << stress.SetValue( details.pLosses->GetFptMax() );
   (*this)(row,4) << force.SetValue( details.pLosses->GetPptMax() );
   (*this)(row,5) << area.SetValue( details.pLosses->GetAg() );
   (*this)(row,6) << mom_inertia.SetValue( details.pLosses->GetIg() );
   (*this)(row,7) << stress.SetValue( details.pLosses->GetFcgpt() );
   (*this)(row,8) << stress.SetValue( details.pLosses->GetDeltaFpt() );
}
