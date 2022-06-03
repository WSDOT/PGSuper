///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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

// ElasticGainDueToLiveLoadTable.cpp : Implementation of CElasticGainDueToLiveLoadTable
#include "stdafx.h"
#include "ElasticGainDueToLiveLoadTable.h"
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\Intervals.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CElasticGainDueToLiveLoadTable::CElasticGainDueToLiveLoadTable(ColumnIndexType NumColumns, IEAFDisplayUnits* pDisplayUnits) :
rptRcTable(NumColumns,0)
{
   DEFINE_UV_PROTOTYPE( spanloc,     pDisplayUnits->GetSpanLengthUnit(),      false );
   DEFINE_UV_PROTOTYPE( gdrloc,      pDisplayUnits->GetSpanLengthUnit(),      false );
   DEFINE_UV_PROTOTYPE( cg,          pDisplayUnits->GetComponentDimUnit(),    false );
   DEFINE_UV_PROTOTYPE( mod_e,       pDisplayUnits->GetModEUnit(),            false );
   DEFINE_UV_PROTOTYPE( force,       pDisplayUnits->GetGeneralForceUnit(),    false );
   DEFINE_UV_PROTOTYPE( area,        pDisplayUnits->GetAreaUnit(),            false );
   DEFINE_UV_PROTOTYPE( mom_inertia, pDisplayUnits->GetMomentOfInertiaUnit(), false );
   DEFINE_UV_PROTOTYPE( ecc,         pDisplayUnits->GetComponentDimUnit(),    false );
   DEFINE_UV_PROTOTYPE( moment,      pDisplayUnits->GetMomentUnit(),          false );
   DEFINE_UV_PROTOTYPE( stress,      pDisplayUnits->GetStressUnit(),          false );
}

CElasticGainDueToLiveLoadTable* CElasticGainDueToLiveLoadTable::PrepareTable(rptChapter* pChapter,IBroker* pBroker,const CSegmentKey& segmentKey,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   // Create and configure the table
   ColumnIndexType numColumns = 8;
   if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion())
   {
      numColumns += 3;
   }

   CElasticGainDueToLiveLoadTable* table = new CElasticGainDueToLiveLoadTable( numColumns, pDisplayUnits);
   rptStyleManager::ConfigureTable(table);

   table->scalar.SetFormat(sysNumericFormatTool::Fixed);
   table->scalar.SetWidth(5);
   table->scalar.SetPrecision(2);

   std::_tstring strImagePath(rptStyleManager::GetImagePath());

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   table->m_LiveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

   GET_IFACE2(pBroker,ISectionProperties,pSectProp);
   pgsTypes::SectionPropertyMode spMode = pSectProp->GetSectionPropertiesMode();

   GET_IFACE2(pBroker,IMaterials,pMaterials);
   Float64 Ec = pMaterials->GetSegmentEc(segmentKey, table->m_LiveLoadIntervalIdx);
   Float64 Ep = pMaterials->GetStrandMaterial(segmentKey, pgsTypes::Straight)->GetE(); // Ok to use straight since we just want E

   rptParagraph* pParagraph = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pParagraph;
   pParagraph->SetName(_T("Elastic Gain Due to Live Load"));
   *pParagraph << pParagraph->GetName() << rptNewLine;

   pParagraph = new rptParagraph;
   *pChapter << pParagraph;

   *pParagraph << _T("Change in strand stress due to live load applied to the composite girder") << rptNewLine;
   *pParagraph << rptNewLine;
   if (spMode == pgsTypes::spmGross)
   {
      *pParagraph << rptRcImage(strImagePath + _T("DeltaFcdLL_Gross.png")) << rptNewLine;
   }
   else
   {
      *pParagraph << rptRcImage(strImagePath + _T("DeltaFcdLL_Transformed.png")) << rptNewLine;
   }

   *pParagraph << rptRcImage(strImagePath + _T("Delta_FpLL.png")) << rptNewLine;

   table->mod_e.ShowUnitTag(true);
   *pParagraph << Sub2(_T("E"),_T("p")) << _T(" = ") << table->mod_e.SetValue( Ep ) << rptNewLine;
   *pParagraph << Sub2(_T("E"),_T("c")) << _T(" = ") << table->mod_e.SetValue( Ec ) << rptNewLine;
   table->mod_e.ShowUnitTag(false);

   GET_IFACE2(pBroker,ISpecification,pSpec);
   GET_IFACE2(pBroker,ILibrary,pLibrary);
   const SpecLibraryEntry* pSpecEntry = pLibrary->GetSpecEntry(pSpec->GetSpecification().c_str());

   table->m_Kliveload = pSpecEntry->GetLiveLoadElasticGain();
   if (spMode == pgsTypes::spmGross)
   {
      *pParagraph << Sub2(_T("K"), _T("llim")) << _T(" = ") << table->scalar.SetValue(table->m_Kliveload) << rptNewLine;
   }

   GET_IFACE2(pBroker, IProductForces, pProductForces);
   table->m_BAT = pProductForces->GetBridgeAnalysisType(pgsTypes::Maximize);

   *pParagraph << table << rptNewLine;

   ColumnIndexType col = 0;
   (*table)(0,col++) << COLHDR(_T("Location from")<<rptNewLine<<_T("Left Support"),rptLengthUnitTag,  pDisplayUnits->GetSpanLengthUnit() );
   if (lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims)
   {
      (*table)(0, col++) << COLHDR(Sub2(_T("M"), _T("llim")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit());
   }
   else
   {
      (*table)(0, col++) << COLHDR(Sub2(_T("M"), _T("llim")) << rptNewLine << _T("Design"), rptMomentUnitTag, pDisplayUnits->GetMomentUnit());
      (*table)(0, col++) << COLHDR(Sub2(_T("M"), _T("llim")) << rptNewLine << _T("Fatigue"), rptMomentUnitTag, pDisplayUnits->GetMomentUnit());
   }

   if ( spMode == pgsTypes::spmGross )
   {
      (*table)(0,col++) << COLHDR(Sub2(_T("e"),_T("p")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
      (*table)(0,col++) << COLHDR(Sub2(_T("I"),_T("c")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit() );
      (*table)(0,col++) << COLHDR(Sub2(_T("Y"),_T("bc")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
      (*table)(0,col++) << COLHDR(Sub2(_T("Y"),_T("bg")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   }
   else
   {
      (*table)(0,col++) << COLHDR(Sub2(_T("e"),_T("pt")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
      (*table)(0,col++) << COLHDR(Sub2(_T("I"),_T("ct")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit() );
      (*table)(0,col++) << COLHDR(Sub2(_T("Y"),_T("bct")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
      (*table)(0,col++) << COLHDR(Sub2(_T("Y"),_T("bgt")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   }

   if (lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims)
   {
      (*table)(0, col++) << COLHDR(symbol(DELTA) << italic(ON) << Sub2(_T("f'''"), _T("cd")) << italic(OFF), rptStressUnitTag, pDisplayUnits->GetStressUnit());
      (*table)(0, col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pLL")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
   }
   else
   {
      (*table)(0, col++) << COLHDR(symbol(DELTA) << italic(ON) << Sub2(_T("f'''"), _T("cd")) << italic(OFF) << rptNewLine << _T("Design"), rptStressUnitTag, pDisplayUnits->GetStressUnit());
      (*table)(0, col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pLL")) << rptNewLine << _T("Design"), rptStressUnitTag, pDisplayUnits->GetStressUnit());

      (*table)(0, col++) << COLHDR(symbol(DELTA) << italic(ON) << Sub2(_T("f'''"), _T("cd")) << italic(OFF) << rptNewLine << _T("Fatigue"), rptStressUnitTag, pDisplayUnits->GetStressUnit());
      (*table)(0, col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pLL")) << rptNewLine << _T("Fatigue"), rptStressUnitTag, pDisplayUnits->GetStressUnit());
   }

   return table;
}

void CElasticGainDueToLiveLoadTable::AddRow(rptChapter* pChapter, IBroker* pBroker, const pgsPointOfInterest& poi, RowIndexType row, const LOSSDETAILS* pDetails, IEAFDisplayUnits* pDisplayUnits, Uint16 level)
{
   ColumnIndexType col = 1;
   RowIndexType rowOffset = GetNumberOfHeaderRows() - 1;

   Float64 MmaxDesign(0), MmaxFatigue(0);
   if (!IsZero(m_Kliveload))
   {
      GET_IFACE2(pBroker, IProductForces, pProductForces);
      Float64 Mmin;
      pProductForces->GetLiveLoadMoment(m_LiveLoadIntervalIdx, pgsTypes::lltDesign, poi, m_BAT, true/*include impact*/, true/*include LLDF*/, &Mmin, &MmaxDesign);

      if (lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims)
      {
         MmaxFatigue = MmaxDesign;
      }
      else
      {
         pProductForces->GetLiveLoadMoment(m_LiveLoadIntervalIdx, pgsTypes::lltFatigue, poi, m_BAT, true/*include impact*/, true/*include LLDF*/, &Mmin, &MmaxFatigue);
      }
   }


   Float64 M_Design  = m_Kliveload*MmaxDesign;
   Float64 M_Fatigue = m_Kliveload*MmaxFatigue;

   if (lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims)
   {
      (*this)(row+rowOffset, col++) << moment.SetValue(M_Design);
   }
   else
   {
      (*this)(row+rowOffset, col++) << moment.SetValue(M_Design);
      (*this)(row+rowOffset, col++) << moment.SetValue(M_Fatigue);
   }

   Float64 Ag, Ybg, Ixx, Iyy, Ixy;
   pDetails->pLosses->GetNoncompositeProperties(&Ag, &Ybg, &Ixx, &Iyy, &Ixy);
   Float64 Ac, Ybc, Ic;
   pDetails->pLosses->GetCompositeProperties2(&Ac, &Ybc, &Ic);

   (*this)(row+rowOffset,col++) << ecc.SetValue( pDetails->pLosses->GetEccPermanentFinal().Y() );
   (*this)(row+rowOffset,col++) << mom_inertia.SetValue( Ic );
   (*this)(row+rowOffset,col++) << cg.SetValue( Ybc );
   (*this)(row+rowOffset,col++) << cg.SetValue( Ybg );

   if (lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims)
   {
      (*this)(row+rowOffset, col++) << stress.SetValue(pDetails->pLosses->GetDeltaFcdLL(M_Design));
      (*this)(row+rowOffset, col++) << stress.SetValue(pDetails->pLosses->ElasticGainDueToLiveLoad(M_Design));
   }
   else
   {
      (*this)(row+rowOffset, col++) << stress.SetValue(pDetails->pLosses->GetDeltaFcdLL(M_Design));
      (*this)(row+rowOffset, col++) << stress.SetValue(pDetails->pLosses->ElasticGainDueToLiveLoad(M_Design));

      (*this)(row+rowOffset, col++) << stress.SetValue(pDetails->pLosses->GetDeltaFcdLL(M_Fatigue));
      (*this)(row+rowOffset, col++) << stress.SetValue(pDetails->pLosses->ElasticGainDueToLiveLoad(M_Fatigue));
   }
}
