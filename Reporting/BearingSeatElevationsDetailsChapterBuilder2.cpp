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
#include <Reporting\BearingSeatElevationsDetailsChapterBuilder2.h>

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

inline Float64 GetNetBearingHeight(const BearingElevationDetails& det)
{
   return det.BrgHeight + det.SolePlateHeight - det.BrgRecess;
}

inline rptRcTable* MakeTable(const CString& strLabel, IEAFDisplayUnits* pDisplayUnits, bool bHasOverlay)
{
   ColumnIndexType nCols = 14 + (bHasOverlay ? 1 : 0);

   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(nCols, strLabel);

   std::_tstring strSlopeTag = pDisplayUnits->GetAlignmentLengthUnit().UnitOfMeasure.UnitTag();

   ColumnIndexType col = 0;
   (*pTable)(0, col++) << _T("Girder");
   (*pTable)(0, col++) << _T("Bearing");
   (*pTable)(0, col++) << _T("Station");
   (*pTable)(0, col++) << COLHDR(_T("Offset"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*pTable)(0, col++) << COLHDR(_T("Finish") << rptNewLine << _T("Grade") << rptNewLine << _T("Elev"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*pTable)(0, col++) << _T("Profile") << rptNewLine << _T("Grade") << rptNewLine << _T("(") << strSlopeTag << _T("/") << strSlopeTag << _T(")");
   (*pTable)(0, col++) << _T("Girder") << rptNewLine << _T("Grade") << rptNewLine << _T("(") << strSlopeTag << _T("/") << strSlopeTag << _T(")");
   (*pTable)(0, col++) << _T("Girder") << rptNewLine << _T("Orientation") << rptNewLine << _T("(") << strSlopeTag << _T("/") << strSlopeTag << _T(")");

   if (bHasOverlay)
      (*pTable)(0, col++) << COLHDR(_T("Overlay Depth"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());

   (*pTable)(0, col++) << COLHDR(_T("Slab") << rptNewLine << _T("Offset"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
   (*pTable)(0, col++) << COLHDR(_T("Height") << rptNewLine << _T("of") << rptNewLine << _T("Girder"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
   (*pTable)(0, col++) << COLHDR(_T("Top") << rptNewLine << _T("Bearing") << rptNewLine << _T("Elev"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*pTable)(0, col++) << COLHDR(_T("Net") << rptNewLine << _T("Bearing") << rptNewLine << _T("Height"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
   (*pTable)(0, col++) << COLHDR(_T("Bearing") << rptNewLine << _T("Deduct"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
   (*pTable)(0, col++) << COLHDR(_T("Bearing") << rptNewLine << _T("Seat") << rptNewLine << _T("Elev"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

   return pTable;
}

inline void FillTable(rptRcTable* pTable, IEAFDisplayUnits* pDisplayUnits, 
                      rptLengthSectionValue& elev, rptLengthSectionValue& dim, rptLengthSectionValue& dist, 
                      bool bHasOverlay, const std::vector<BearingElevationDetails>& vElevDetails)
{
   RowIndexType Row = pTable->GetNumberOfHeaderRows();

   std::vector<BearingElevationDetails>::const_iterator Iter(vElevDetails.begin());
   std::vector<BearingElevationDetails>::const_iterator End(vElevDetails.end());
   for (; Iter != End; Iter++, Row++)
   {
      const BearingElevationDetails& ElevDetails = *Iter;

      ColumnIndexType Col = 0;

      (*pTable)(Row, Col++) << LABEL_GIRDER(ElevDetails.GdrIdx);
      (*pTable)(Row, Col++) << ElevDetails.BearingIdx + 1;
      (*pTable)(Row, Col++) << rptRcStation(ElevDetails.Station, &pDisplayUnits->GetStationFormat());
      (*pTable)(Row, Col++) << RPT_OFFSET(ElevDetails.Offset, dist);
      (*pTable)(Row, Col++) << elev.SetValue(ElevDetails.FinishedGradeElevation);
      (*pTable)(Row, Col++) << ElevDetails.ProfileGrade;
      (*pTable)(Row, Col++) << ElevDetails.GirderGrade;
      (*pTable)(Row, Col++) << ElevDetails.GirderOrientation;

      if (bHasOverlay)
         (*pTable)(Row, Col++) << dim.SetValue(ElevDetails.OverlayDepth);

      (*pTable)(Row, Col++) << dim.SetValue(ElevDetails.SlabOffset);
      (*pTable)(Row, Col++) << dim.SetValue(ElevDetails.Hg);
      (*pTable)(Row, Col++) << elev.SetValue(ElevDetails.TopBrgElevation);
      (*pTable)(Row, Col++) << dim.SetValue(GetNetBearingHeight(ElevDetails));
      (*pTable)(Row, Col++) << dim.SetValue(ElevDetails.BearingDeduct);
      (*pTable)(Row, Col++) << elev.SetValue(ElevDetails.BrgSeatElevation);
   }
}

CBearingSeatElevationsDetailsChapterBuilder2::CBearingSeatElevationsDetailsChapterBuilder2(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

CBearingSeatElevationsDetailsChapterBuilder2::~CBearingSeatElevationsDetailsChapterBuilder2(void)
{
}

LPCTSTR CBearingSeatElevationsDetailsChapterBuilder2::GetName() const
{
   return TEXT("Bearing Seat Elevation Details");
}

rptChapter* CBearingSeatElevationsDetailsChapterBuilder2::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   INIT_UV_PROTOTYPE( rptLengthSectionValue, elev, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthSectionValue, dist, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthSectionValue, dim,  pDisplayUnits->GetComponentDimUnit(), false );

   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CDeckDescription2* pDeck = pBridgeDesc->GetDeckDescription();
   if ( pDeck->WearingSurface == pgsTypes::wstOverlay && !pDeck->bInputAsDepthAndDensity )
   {
      Float64 density = ::ConvertToSysUnits(140.0,unitMeasure::LbfPerFeet3);

      *pPara << _T("NOTE: overlay depth estimated based on a unit weight of ") << ::FormatDimension(density,pDisplayUnits->GetDensityUnit(),true) << rptNewLine;
      *pPara << ::FormatDimension(pDeck->OverlayWeight,pDisplayUnits->GetOverlayWeightUnit(),true) << _T("/") << ::FormatDimension(density,pDisplayUnits->GetDensityUnit(),true) << _T(" = ") << ::FormatDimension(pBridge->GetOverlayDepth(),pDisplayUnits->GetComponentDimUnit(),true) << rptNewLine;
   }

   *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("BearingSeatElevation.png")) << rptNewLine;

   *pPara << _T("NOTE: Vertical height dimensions listed in the tables below are adjusted for girder and roadway grade and orientation.") << rptNewLine;

   bool bHasOverlay = ( pBridge->HasOverlay() && !pBridge->IsFutureOverlay() ? true : false );

   PierIndexType nPiers = pBridge->GetPierCount();
   for (PierIndexType pierIdx = 0; pierIdx < nPiers; pierIdx++)
   {
      if (pBridge->IsBoundaryPier(pierIdx))
      {
         CString strBackLabel;
         strBackLabel.Format(_T("%s %d Back"), pierIdx == 0 || pierIdx == nPiers - 1 ? _T("Abutment") : _T("Pier"), LABEL_PIER(pierIdx));
         rptRcTable* pBackTable = nullptr;
         if (pierIdx != 0)
         {
            pBackTable = MakeTable(strBackLabel,pDisplayUnits,bHasOverlay);
            (*pPara) << pBackTable << rptNewLine;
         }

         CString strAheadLabel;
         strAheadLabel.Format(_T("%s %d Ahead"), pierIdx == 0 || pierIdx == nPiers - 1 ? _T("Abutment") : _T("Pier"), LABEL_PIER(pierIdx));
         rptRcTable* pAheadTable = nullptr;
         if (pierIdx != nPiers - 1)
         {
            pAheadTable = MakeTable(strBackLabel,pDisplayUnits,bHasOverlay);
            (*pPara) << pAheadTable << rptNewLine;
         }

         if (pBackTable)
         {

            std::vector<BearingElevationDetails> vBackElevDetails = pBridge->GetBearingElevationDetails(pierIdx, pgsTypes::Back);

            FillTable(pBackTable, pDisplayUnits, elev, dim, dist, bHasOverlay, vBackElevDetails);
         }

         if (pAheadTable)
         {
            std::vector<BearingElevationDetails> vAheadElevDetails = pBridge->GetBearingElevationDetails(pierIdx, pgsTypes::Ahead);

            FillTable(pAheadTable, pDisplayUnits, elev, dim, dist, bHasOverlay, vAheadElevDetails);
         }
      }
      else
      {
         // Interior Pier
         CString strLabel;
         strLabel.Format(_T("Pier %d"), LABEL_PIER(pierIdx));
         rptRcTable* pTable = nullptr;
         pTable = MakeTable(strLabel,pDisplayUnits,bHasOverlay);
         (*pPara) << pTable << rptNewLine;

         std::vector<BearingElevationDetails> vBackElevDetails = pBridge->GetBearingElevationDetails(pierIdx, pgsTypes::Back);

         FillTable(pTable, pDisplayUnits, elev, dim, dist, bHasOverlay, vBackElevDetails);
      }
   }

   return pChapter;
}

CChapterBuilder* CBearingSeatElevationsDetailsChapterBuilder2::Clone() const
{
   return new CBearingSeatElevationsDetailsChapterBuilder2;
}
