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

// EffectivePrestressForceTable.cpp : Implementation of CEffectivePrestressForceTable
#include "stdafx.h"
#include "EffectivePrestressForceTable.h"
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\PrestressForce.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Intervals.h>
#include <PsgLib\SpecLibraryEntry.h>
#include <PgsExt\GirderData.h>
#include <PgsExt\LoadFactors.h>
#include <PgsExt\PrecastSegmentData.h>
#include <PgsExt\GirderLabel.h>
#include <IFace\PrestressForce.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CEffectivePrestressForceTable::CEffectivePrestressForceTable(ColumnIndexType NumColumns, IEAFDisplayUnits* pDisplayUnits) :
rptRcTable(NumColumns,0)
{
   DEFINE_UV_PROTOTYPE( force,       pDisplayUnits->GetGeneralForceUnit(),    false );
   DEFINE_UV_PROTOTYPE( area,        pDisplayUnits->GetAreaUnit(),            false );
   DEFINE_UV_PROTOTYPE( stress,      pDisplayUnits->GetStressUnit(),          false );

   scalar.SetFormat( WBFL::System::NumericFormatTool::Format::Fixed );
   scalar.SetWidth(6); // -99.9
   scalar.SetPrecision(3);
   scalar.SetTolerance(1.0e-6);
}

CEffectivePrestressForceTable* CEffectivePrestressForceTable::PrepareTable(rptChapter* pChapter, IBroker* pBroker, const CSegmentKey& segmentKey, IEAFDisplayUnits* pDisplayUnits, Uint16 level)
{
   // Create and configure the table
   ColumnIndexType numColumns = 13; // location, (xfer, Aps)*2, fpe, Ppe, fpe_ServiceI, Ppe_ServiceI, fpe_ServiceIII, Ppe_ServiceIII, fpe_FatigueI, Ppe_FatigueI

   CEffectivePrestressForceTable* table = new CEffectivePrestressForceTable( numColumns, pDisplayUnits );
   rptStyleManager::ConfigureTable(table);

   table->SetNumberOfHeaderRows(2);
   
   rptParagraph* pParagraph = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pParagraph;

   pParagraph->SetName(_T("Effective Prestress Force"));
   *pParagraph << pParagraph->GetName() << rptNewLine;

   pParagraph = new rptParagraph;
   *pChapter << pParagraph;
   *pParagraph << _T("In determining the resistance of pretensioned concrete components in their end zones, the gradual buildup of the strand force in the transfer and development lengths shall be taken into account. (5.9.4.3.1)") << rptNewLine;
   *pParagraph << Sub2(_T("P"), _T("pe")) << _T(" = ") << RPT_FPE << _T("[") << symbol(SUM) << _T("(") << symbol(zeta) << RPT_APS << _T(")]") << rptNewLine;
   *pParagraph << symbol(zeta) << _T(" = Prestress Transfer Length Reduction Factor");

   ColumnIndexType col = 0;
   *pParagraph << table << rptNewLine;
   table->SetRowSpan(0, col, 2);
   (*table)(0,col++) << COLHDR(_T("Location from")<<rptNewLine<<_T("Left Support"),rptLengthUnitTag,  pDisplayUnits->GetSpanLengthUnit() );

   table->SetColumnSpan(0, col, 2);
   (*table)(0, col) << _T("Straight") << rptNewLine << _T("Strands");
   (*table)(1, col++) << symbol(zeta);
   (*table)(1, col++) << COLHDR(RPT_APS, rptAreaUnitTag, pDisplayUnits->GetAreaUnit());

   GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
   const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(segmentKey);
   pgsTypes::AdjustableStrandType adj_type = pSegment->Strands.GetAdjustableStrandType();
   std::_tstring strAdj(pgsTypes::asHarped == adj_type ? _T("Harped") : _T("Adj. Straight"));
   table->SetColumnSpan(0, col, 2);
   (*table)(0, col) << strAdj << rptNewLine << _T("Strands");
   (*table)(1, col++) << symbol(zeta);
   (*table)(1, col++) << COLHDR(RPT_APS, rptAreaUnitTag, pDisplayUnits->GetAreaUnit());

   table->SetColumnSpan(0, col, 2);
   (*table)(0, col) << _T("Without Live load");
   (*table)(1, col++) << COLHDR(RPT_FPE, rptStressUnitTag, pDisplayUnits->GetStressUnit());
   (*table)(1, col++) << COLHDR(Sub2(_T("P"), _T("pe")), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());

   table->SetColumnSpan(0, col, 2);
   (*table)(0, col) << GetLimitStateString(pgsTypes::ServiceI) << rptNewLine << _T("with Live load");
   (*table)(1, col++) << COLHDR(RPT_FPE, rptStressUnitTag, pDisplayUnits->GetStressUnit());
   (*table)(1, col++) << COLHDR(Sub2(_T("P"), _T("pe")), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());

   table->SetColumnSpan(0, col, 2);
   (*table)(0, col) << GetLimitStateString(pgsTypes::ServiceIII) << rptNewLine << _T("with Live load");
   (*table)(1, col++) << COLHDR(RPT_FPE, rptStressUnitTag, pDisplayUnits->GetStressUnit());
   (*table)(1, col++) << COLHDR(Sub2(_T("P"), _T("pe")), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());

   table->SetColumnSpan(0, col, 2);
   (*table)(0, col) << GetLimitStateString(lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims ? pgsTypes::ServiceIA : pgsTypes::FatigueI) << rptNewLine << _T("with Live load");
   (*table)(1, col++) << COLHDR(RPT_FPE, rptStressUnitTag, pDisplayUnits->GetStressUnit());
   (*table)(1, col++) << COLHDR(Sub2(_T("P"), _T("pe")), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());

   GET_IFACE2(pBroker, IIntervals, pIntervals);
   table->m_LiveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

   return table;
}

void CEffectivePrestressForceTable::AddRow(rptChapter* pChapter,IBroker* pBroker,const pgsPointOfInterest& poi,RowIndexType row,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   ColumnIndexType col = 1;
   RowIndexType rowOffset = GetNumberOfHeaderRows() - 1;

   GET_IFACE2(pBroker, IPretensionForce, pPrestressForce);

   Float64 fpe = pPrestressForce->GetEffectivePrestress(poi, pgsTypes::Permanent, m_LiveLoadIntervalIdx, pgsTypes::End);

   std::array<Float64, 2> adj{ pPrestressForce->GetTransferLengthAdjustment(poi,pgsTypes::Straight),  pPrestressForce->GetTransferLengthAdjustment(poi,pgsTypes::Harped) };

   GET_IFACE2(pBroker, IStrandGeometry, pStrandGeom);
   std::array<Float64, 2> Aps{ pStrandGeom->GetStrandArea(poi,m_LiveLoadIntervalIdx,pgsTypes::Straight), pStrandGeom->GetStrandArea(poi,m_LiveLoadIntervalIdx,pgsTypes::Harped) };

   Float64 Ppe = pPrestressForce->GetPrestressForce(poi, pgsTypes::Permanent, m_LiveLoadIntervalIdx, pgsTypes::End, true/*include elastic effects*/);
   ATLASSERT(IsEqual(Ppe, pPrestressForce->GetPrestressForce(poi, pgsTypes::Straight, m_LiveLoadIntervalIdx, pgsTypes::End, true/*include elastic effects*/) + pPrestressForce->GetPrestressForce(poi, pgsTypes::Harped, m_LiveLoadIntervalIdx, pgsTypes::End, true/*include elastic effects*/)));

   std::array<pgsTypes::LimitState, 3> vLimitStates{ pgsTypes::ServiceI, pgsTypes::ServiceIII, lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims ? pgsTypes::ServiceIA : pgsTypes::FatigueI };
   std::array<Float64, 3> fpe_with_liveload;
   std::array<Float64, 3> Ppe_with_liveload;
   int i = 0;
   for (auto limitState : vLimitStates)
   {
      // NOTE: can't use limitState as an array index key because the values are not sequential. that is why we are using "i"
      fpe_with_liveload[i] = pPrestressForce->GetEffectivePrestressWithLiveLoad(poi, pgsTypes::Permanent, limitState, true/*include elastic effects*/, true/*apply elastic gain reductions*/, INVALID_INDEX);
      Ppe_with_liveload[i] = pPrestressForce->GetPrestressForceWithLiveLoad(poi, pgsTypes::Permanent, limitState, true/*include elastic effects*/, INVALID_INDEX);
      i++;
   }


   // Fill up the table row+rowOffset
   for (int i = 0; i < 2; i++)
   {
      pgsTypes::StrandType strandType = (pgsTypes::StrandType)i;
      (*this)(row + rowOffset, col++) << scalar.SetValue(adj[strandType]);
      (*this)(row + rowOffset, col++) << area.SetValue(Aps[strandType]);
   }

   (*this)(row + rowOffset, col++) << stress.SetValue(fpe);
   (*this)(row + rowOffset, col++) << force.SetValue(Ppe);

   int j = 0;
   for (auto limitState : vLimitStates)
   {
      (*this)(row + rowOffset, col++) << stress.SetValue(fpe_with_liveload[j]);
      (*this)(row + rowOffset, col++) << force.SetValue(Ppe_with_liveload[j]);
      j++;
   }
}
