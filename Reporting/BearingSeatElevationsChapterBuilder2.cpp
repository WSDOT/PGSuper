///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2019  Washington State Department of Transportation
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
#include "StdAfx.h"
#include "Reporting\BearingSeatElevationsChapterBuilder2.h"


#include <IReportManager.h>
#include <EAF\EAFDisplayUnits.h>

#include <IFace\Bridge.h>
#include <IFace\Project.h>


#include <PgsExt\GirderLabel.h>
#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\DeckDescription2.h>

#include <PGSuperUnits.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CBearingSeatElevationsChapterBuilderBase::CBearingSeatElevationsChapterBuilderBase(TableType type, bool bSelect) :
CPGSuperChapterBuilder(bSelect),
m_TableType(type)
{
}

CBearingSeatElevationsChapterBuilderBase::~CBearingSeatElevationsChapterBuilderBase(void)
{
}

rptChapter* CBearingSeatElevationsChapterBuilderBase::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   GET_IFACE2(pBroker,IBridge,pBridge);

   PierIndexType nPiers = pBridge->GetPierCount();
   for ( PierIndexType pierIdx = 0; pierIdx < nPiers; pierIdx++ )
   {
      enum lpierType { leftAbut, interPier, interBoundaryPier, rightAbut } prtype;
      if (pBridge->IsInteriorPier(pierIdx))
      {
         prtype = interPier;
      }
      else if (pBridge->IsAbutment(pierIdx))
      {
         if (pierIdx == 0)
         {
            prtype = leftAbut;
         }
         else
         {
            prtype = rightAbut;
         }
      }
      else
      {
         prtype = interBoundaryPier;
      }

      CString strLabel;
      strLabel.Format(_T("%s %d, "), (leftAbut==prtype || rightAbut==prtype) ? _T("Abutment") : _T("Pier"),LABEL_PIER(pierIdx));

      // Create table(s) for pier
      if (interPier==prtype)
      {
         strLabel += _T("C.L.");
         (*pPara) << BuildTable(strLabel, pierIdx,  pgsTypes::Back) << rptNewLine;

      }
      else if (leftAbut==prtype)
      {
         strLabel += _T("Ahead");
         (*pPara) << BuildTable(strLabel, pierIdx,  pgsTypes::Ahead) << rptNewLine;
      }
      else if (rightAbut==prtype)
      {
         strLabel += _T("Back");
         (*pPara) << BuildTable(strLabel, pierIdx,  pgsTypes::Back) << rptNewLine;
      }
      else
      {
         // boundary piers have two faces - put into an invisible layout table
         rptRcTable* pLayoutTable = rptStyleManager::CreateLayoutTable(3);

         CString strNewLabel = strLabel + _T("Back");
         (*pLayoutTable)(0,0) << BuildTable(strNewLabel, pierIdx,  pgsTypes::Back);

         (*pLayoutTable)(0, 1) << _T("&nbsp;") << _T("&nbsp;");

         strLabel += _T("Ahead");
         (*pLayoutTable)(0, 2) << BuildTable(strLabel, pierIdx, pgsTypes::Ahead);

         (*pPara) << pLayoutTable << rptNewLine;
      }
   }

   return pChapter;
}

// Macro we can use to write a newline before our table txt value
#define WRITE_NEWLINE_BEFORE(doWriteNewLineRow, row, col, txt) if (doWriteNewLineRow) { (*pTable)(row, col) << rptNewLine << txt;} else { (*pTable)(row, col) << txt;}


rptRcTable* CBearingSeatElevationsChapterBuilderBase::BuildTable(const CString& strLabel,PierIndexType pierIdx,  pgsTypes::PierFaceType face) const
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CDeckDescription2* pDeck = pBridgeDesc->GetDeckDescription();

   INIT_UV_PROTOTYPE( rptLengthSectionValue, dist, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthSectionValue, dim,  pDisplayUnits->GetComponentDimUnit(), false );

   ColumnIndexType ncols = 4;
   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(ncols,strLabel);

   (*pTable)(0,0) << _T("Girder");
   (*pTable)(0,1) << _T("Bearing");

   if (m_TableType == ttBearingDeduct)
   {
      (*pTable)(0, 2) << COLHDR(_T("Bearing Deduct"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
   }
   else
   {
      (*pTable)(0, 2) << COLHDR(_T("Top Brg Elev"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   }

   (*pTable)(0,3) << COLHDR(_T("Brg Seat Elev"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

   // first columns
   RowIndexType row = pTable->GetNumberOfHeaderRows();

   std::vector<BearingElevationDetails> vElevDetails = pBridge->GetBearingElevationDetails(pierIdx, face);
   std::vector<BearingElevationDetails>::iterator iter(vElevDetails.begin());
   std::vector<BearingElevationDetails>::iterator iend(vElevDetails.end());
   GirderIndexType lastGdrIdx = INVALID_INDEX;
   for (; iter != iend; iter++)
   {
      BearingElevationDetails& elevDetails = *iter;

      // put multiple bearings for same girder in same row
      bool newRow = (lastGdrIdx == INVALID_INDEX) || (lastGdrIdx != elevDetails.GirderKey.girderIndex);
      if (newRow)
      {
         row++;
      }

      bool writeNewLineBefore = !newRow;

      lastGdrIdx = elevDetails.GirderKey.girderIndex;

      if (newRow)
      {
         WRITE_NEWLINE_BEFORE(writeNewLineBefore, row, 0, LABEL_GIRDER(elevDetails.GirderKey.girderIndex))
      }

      WRITE_NEWLINE_BEFORE(writeNewLineBefore, row, 1, elevDetails.BearingIdx+1)

      if (m_TableType == ttBearingDeduct)
      {
         WRITE_NEWLINE_BEFORE(writeNewLineBefore, row, 2, dim.SetValue(elevDetails.BearingDeduct))
      }
      else
      {
         WRITE_NEWLINE_BEFORE(writeNewLineBefore, row, 2, dist.SetValue(elevDetails.TopBrgElevation))
      }

      WRITE_NEWLINE_BEFORE(writeNewLineBefore, row, 3, dist.SetValue(elevDetails.BrgSeatElevation))
   }

   return pTable;
}

////////////////////////////////////////////////////////////////////
CBearingSeatElevationsChapterBuilder2::CBearingSeatElevationsChapterBuilder2(bool bSelect) :
CBearingSeatElevationsChapterBuilderBase(CBearingSeatElevationsChapterBuilderBase::ttBearingElevations, bSelect)
{
}

CBearingSeatElevationsChapterBuilder2::~CBearingSeatElevationsChapterBuilder2(void)
{
}

LPCTSTR CBearingSeatElevationsChapterBuilder2::GetName() const
{
   return TEXT("Bearing Seat Elevations");
}

CChapterBuilder* CBearingSeatElevationsChapterBuilder2::Clone() const
{
   return new CBearingSeatElevationsChapterBuilder2(*this);
}

////////////////////////////////////////////////////////////////////
CBearingDeductChapterBuilder::CBearingDeductChapterBuilder(bool bSelect) :
CBearingSeatElevationsChapterBuilderBase(CBearingSeatElevationsChapterBuilderBase::ttBearingDeduct, bSelect)
{
}

CBearingDeductChapterBuilder::~CBearingDeductChapterBuilder(void)
{
}

LPCTSTR CBearingDeductChapterBuilder::GetName() const
{
   return TEXT("Bearing Seat Elevations");
}

CChapterBuilder* CBearingDeductChapterBuilder::Clone() const
{
   return new CBearingDeductChapterBuilder(*this);
}
