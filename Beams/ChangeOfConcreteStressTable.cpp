///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2024  Washington State Department of Transportation
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

// ChangeOfConcreteStressTable.cpp : Implementation of CChangeOfConcreteStressTable
#include "stdafx.h"
#include "ChangeOfConcreteStressTable.h"
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <PsgLib\SpecLibraryEntry.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CChangeOfConcreteStressTable::CChangeOfConcreteStressTable(ColumnIndexType NumColumns, IEAFDisplayUnits* pDisplayUnits) :
rptRcTable(NumColumns,0)
{
   DEFINE_UV_PROTOTYPE( spanloc,     pDisplayUnits->GetSpanLengthUnit(),      false );
   DEFINE_UV_PROTOTYPE( gdrloc,      pDisplayUnits->GetSpanLengthUnit(),      false );
   DEFINE_UV_PROTOTYPE( mom_inertia, pDisplayUnits->GetMomentOfInertiaUnit(), false );
   DEFINE_UV_PROTOTYPE( dim,         pDisplayUnits->GetComponentDimUnit(),    false );
   DEFINE_UV_PROTOTYPE( moment,      pDisplayUnits->GetMomentUnit(),          false );
   DEFINE_UV_PROTOTYPE( stress,      pDisplayUnits->GetStressUnit(),          false );
}

CChangeOfConcreteStressTable* CChangeOfConcreteStressTable::PrepareTable(rptChapter* pChapter,IBroker* pBroker,const CSegmentKey& segmentKey,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   GET_IFACE2_NOCHECK(pBroker, IBridge, pBridge);
   GET_IFACE2(pBroker, IGirder, pGirder);
   bool bHasDeckLoads = pGirder->HasStructuralLongitudinalJoints() && pBridge->GetDeckType() != pgsTypes::sdtNone ? true : false; // if longitudinal joints are structural and there is a deck, the deck dead loads go on the composite section
   bool bIs2StageComposite = pGirder->HasStructuralLongitudinalJoints() && IsStructuralDeck(pBridge->GetDeckType()) ? true : false;

   // Create and configure the table
   ColumnIndexType numColumns = 9;

   if (bHasDeckLoads)
   {
      numColumns += 1;
   }

   if (bIs2StageComposite)
   {
      numColumns += 2;
   }

   CChangeOfConcreteStressTable* table = new CChangeOfConcreteStressTable( numColumns, pDisplayUnits );
   rptStyleManager::ConfigureTable(table);

   table->m_bHasDeckLoads = bHasDeckLoads;
   table->m_bIs2StageComposite = bIs2StageComposite;


   std::_tstring strImagePath(rptStyleManager::GetImagePath());


   rptParagraph* pParagraph = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pParagraph;
   ATLASSERT(WBFL::LRFD::BDSManager::GetEdition() < WBFL::LRFD::BDSManager::Edition::EighthEdition2017); // code reference below is invalid
   pParagraph->SetName(_T("Change in Concrete Stress at Level of Prestressing"));
   *pParagraph << pParagraph->GetName() << _T(" [5.9.5.4.3]") << rptNewLine;

   pParagraph = new rptParagraph;
   *pChapter << pParagraph;

   if (bIs2StageComposite)
   {
      *pParagraph << rptRcImage(strImagePath + _T("Delta_Fcdp_with_Deck.png")) << rptNewLine;
   }
   else
   {
      *pParagraph << rptRcImage(strImagePath + _T("Delta_Fcdp.png")) << rptNewLine;
   }

   *pParagraph << table << rptNewLine;

   ColumnIndexType col = 0;
   (*table)(0, col++) << COLHDR(_T("Location from")<<rptNewLine<<_T("Left Support"),rptLengthUnitTag,  pDisplayUnits->GetSpanLengthUnit() );
   (*table)(0, col++) << COLHDR(Sub2(_T("M"),_T("adl")),  rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   if (bIs2StageComposite)
   {
      (*table)(0, col++) << COLHDR(Sub2(_T("M"), _T("sidl1")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit());
      (*table)(0, col++) << COLHDR(Sub2(_T("M"), _T("sidl2")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit());
   }
   else
   {
      (*table)(0, col++) << COLHDR(Sub2(_T("M"), _T("sidl")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit());
   }
   (*table)(0, col++) << COLHDR(Sub2(_T("e"),_T("ps")),   rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0, col++) << COLHDR(Sub2(_T("Y"),_T("bg")),   rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0, col++) << COLHDR(Sub2(_T("I"), _T("g")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
   if (bIs2StageComposite)
   {
      (*table)(0, col++) << COLHDR(Sub2(_T("Y"), _T("bc1")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      (*table)(0, col++) << COLHDR(Sub2(_T("I"), _T("c1")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
      (*table)(0, col++) << COLHDR(Sub2(_T("Y"), _T("bc2")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      (*table)(0, col++) << COLHDR(Sub2(_T("I"), _T("c2")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
   }
   else
   {
      (*table)(0, col++) << COLHDR(Sub2(_T("Y"), _T("bc")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      (*table)(0, col++) << COLHDR(Sub2(_T("I"), _T("c")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
   }
   (*table)(0, col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("cdp")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );

   pParagraph = new rptParagraph(rptStyleManager::GetFootnoteStyle());
   *pChapter << pParagraph;
   *pParagraph << Sub2(_T("M"),_T("adl")) << _T(" = Moment due to permanent loads applied to the noncomposite girder section after the prestressing force is applied") << rptNewLine;
   if (bIs2StageComposite)
   {
      *pParagraph << Sub2(_T("M"), _T("sidl1")) << _T(" = Moment due to permanent loads applied after the longitudinal joints become composite after the prestressing force is applied") << rptNewLine;
      *pParagraph << Sub2(_T("M"), _T("sidl2")) << _T(" = Moment due to permanent loads applied to the final composite girder section after the prestressing force is applied") << rptNewLine;
   }
   else
   {
      *pParagraph << Sub2(_T("M"), _T("sidl")) << _T(" = Moment due to permanent loads applied to the composite girder section after the prestressing force is applied") << rptNewLine;
   }
   
   return table;
}

void CChangeOfConcreteStressTable::AddRow(rptChapter* pChapter,IBroker* pBroker,const pgsPointOfInterest& poi,RowIndexType row,const LOSSDETAILS* pDetails,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   ColumnIndexType col = 1;
   RowIndexType rowOffset = GetNumberOfHeaderRows() - 1;

   Float64 Ag, Ybg, Ixx, Iyy, Ixy;
   pDetails->pLosses->GetNoncompositeProperties(&Ag, &Ybg, &Ixx, &Iyy, &Ixy);
   Float64 Ac1, Ybc1, Ic1;
   pDetails->pLosses->GetCompositeProperties1(&Ac1, &Ybc1, &Ic1);
   Float64 Ac2, Ybc2, Ic2;
   pDetails->pLosses->GetCompositeProperties2(&Ac2, &Ybc2, &Ic2);

   (*this)(row+rowOffset, col++) << moment.SetValue( pDetails->pLosses->GetAddlGdrMoment() );
   if (m_bIs2StageComposite)
   {
      (*this)(row + rowOffset, col++) << moment.SetValue(pDetails->pLosses->GetSidlMoment1());
      (*this)(row + rowOffset, col++) << moment.SetValue(pDetails->pLosses->GetSidlMoment2());
   }
   else
   {
      (*this)(row + rowOffset, col++) << moment.SetValue(pDetails->pLosses->GetSidlMoment1() + pDetails->pLosses->GetSidlMoment2());
   }
   (*this)(row+rowOffset, col++) << dim.SetValue( pDetails->pLosses->GetEccPermanentFinal().Y() );
   (*this)(row+rowOffset, col++) << dim.SetValue( Ybg );
   (*this)(row + rowOffset, col++) << mom_inertia.SetValue(Ixx);

   if (m_bIs2StageComposite)
   {
      (*this)(row + rowOffset, col++) << dim.SetValue(Ybc1);
      (*this)(row + rowOffset, col++) << mom_inertia.SetValue(Ic1);
      (*this)(row + rowOffset, col++) << dim.SetValue(Ybc2);
      (*this)(row + rowOffset, col++) << mom_inertia.SetValue(Ic2);
   }
   else
   {
      (*this)(row + rowOffset, col++) << dim.SetValue(Ybc2);
      (*this)(row + rowOffset, col++) << mom_inertia.SetValue(Ic2);
   }
   (*this)(row+rowOffset, col++) << stress.SetValue( -pDetails->pLosses->GetDeltaFcd1(true/*apply elastic gains reduction*/) );
}
