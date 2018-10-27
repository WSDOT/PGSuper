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

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CDeckDescription2* pDeck = pBridgeDesc->GetDeckDescription();

   INIT_UV_PROTOTYPE( rptLengthSectionValue, dist, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthSectionValue, dim,  pDisplayUnits->GetComponentDimUnit(), false );

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
      strLabel.Format(_T("%s %d"), (leftAbut==prtype || rightAbut==prtype) ? _T("Abutment") : _T("Pier"),LABEL_PIER(pierIdx));

      ColumnIndexType ncols = interBoundaryPier==prtype ? 5 : 3;
      rptRcTable* pTable = rptStyleManager::CreateDefaultTable(ncols,strLabel);
      (*pPara) << pTable << rptNewLine;

      pTable->SetNumberOfHeaderRows(2);

      pTable->SetRowSpan(0,0,2);
      pTable->SetRowSpan(1,0,SKIP_CELL);
      (*pTable)(0,0) << _T("Girder");

      // first bearing line always printed
      if (interPier==prtype)
      {
         strLabel = _T("C.L.");
      }
      else if (rightAbut==prtype)
      {
         strLabel = _T("Back");
      }
      else
      {
         strLabel = _T("Ahead");// left abut or boundary pier
      }

      pTable->SetColumnSpan(0,1,2);
      (*pTable)(0, 1) << strLabel;
      pTable->SetColumnSpan(0,2,SKIP_CELL);
      if (m_TableType == ttBearingDeduct)
      {
         (*pTable)(1, 1) << COLHDR(_T("Bearing Deduct"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      }
      else
      {
         (*pTable)(1, 1) << COLHDR(_T("Top Brg Elev"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
      }

      (*pTable)(1,2) << COLHDR(_T("Brg Seat Elev"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

      // first columns
      RowIndexType row = pTable->GetNumberOfHeaderRows();

      pgsTypes::PierFaceType face = (rightAbut == prtype) ? pgsTypes::Back : pgsTypes::Ahead;
      std::vector<BearingElevationDetails> vElevDetails = pBridge->GetBearingElevationDetails(pierIdx, face);
      std::vector<BearingElevationDetails>::iterator iter(vElevDetails.begin());
      std::vector<BearingElevationDetails>::iterator iend(vElevDetails.end());
      for ( ; iter != iend;  iter++, row++ )
      {
         BearingElevationDetails& elevDetails = *iter;
         (*pTable)(row, 0) << LABEL_GIRDER(elevDetails.GdrIdx);

         if (m_TableType == ttBearingDeduct)
         {
            (*pTable)(row, 1) << dim.SetValue(elevDetails.BearingDeduct);
         }
         else
         {
            (*pTable)(row, 1) << dist.SetValue(elevDetails.TopBrgElevation);
         }

         (*pTable)(row,2) << dist.SetValue(elevDetails.BrgSeatElevation);
      }

      if (interBoundaryPier == prtype) // boundary piers are only type with two bearing lines
      {
         pTable->SetColumnSpan(0, 3, 2);
         (*pTable)(0, 3) << _T("Back");
         pTable->SetColumnSpan(0, 4, SKIP_CELL);

         if (m_TableType == ttBearingDeduct)
         {
            (*pTable)(1, 3) << COLHDR(_T("Bearing Deduct"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
         }
         else
         {
            (*pTable)(1, 3) << COLHDR(_T("Top Brg Elev"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
         }

         (*pTable)(1, 4) << COLHDR(_T("Brg Seat Elev"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

         row = pTable->GetNumberOfHeaderRows();

         vElevDetails = pBridge->GetBearingElevationDetails(pierIdx, pgsTypes::Back);
         iter= vElevDetails.begin();
         iend = vElevDetails.end();
         for ( ; iter != iend;  iter++, row++ )
         {
            BearingElevationDetails& elevDetails = *iter;
            if ((*pTable)(row, 0).IsEmpty())
            {
               (*pTable)(row, 0) << LABEL_GIRDER(elevDetails.GdrIdx);
            }

            if (m_TableType == ttBearingDeduct)
            {
               (*pTable)(row, 3) << dim.SetValue(elevDetails.BearingDeduct);
            }
            else
            {
               (*pTable)(row, 3) << dist.SetValue(elevDetails.TopBrgElevation);
            }

            (*pTable)(row,4) << dist.SetValue(elevDetails.BrgSeatElevation);
         }
      }
   }

   return pChapter;
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
