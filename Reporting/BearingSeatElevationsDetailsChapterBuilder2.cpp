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
#include <IFace\AnalysisResults.h>


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

inline rptRcTable* MakeTable(const CString& strLabel, IEAFDisplayUnits* pDisplayUnits, bool bHasOverlay, bool bHasPrecamber)
{
   ColumnIndexType nCols = 14 + (bHasOverlay ? 1 : 0) + (bHasPrecamber ? 2 : 0);

   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(nCols, strLabel);

   std::_tstring strSlopeTag = pDisplayUnits->GetAlignmentLengthUnit().UnitOfMeasure.UnitTag();

   ColumnIndexType col = 0;
   (*pTable)(0, col++) << _T("Girder");
   (*pTable)(0, col++) << _T("Bearing");
   (*pTable)(0, col++) << _T("Station");
   (*pTable)(0, col++) << COLHDR(_T("Offset"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*pTable)(0, col++) << COLHDR(_T("Finish") << rptNewLine << _T("Grade") << rptNewLine << _T("Elev"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*pTable)(0, col++) << _T("Profile") << rptNewLine << _T("Grade") << rptNewLine << _T("(") << strSlopeTag << _T("/") << strSlopeTag << _T(")");
   if (bHasPrecamber)
   {
      (*pTable)(0, col++) << _T("Basic Girder") << rptNewLine << _T("Grade") << rptNewLine << _T("(") << strSlopeTag << _T("/") << strSlopeTag << _T(")");
      (*pTable)(0, col++) << _T("Precamber") << rptNewLine << _T("Slope") << rptNewLine << _T("(") << strSlopeTag << _T("/") << strSlopeTag << _T(")");
   }
   (*pTable)(0, col++) << _T("Girder") << rptNewLine << _T("Grade") << rptNewLine << _T("(") << strSlopeTag << _T("/") << strSlopeTag << _T(")");
   (*pTable)(0, col++) << _T("Girder") << rptNewLine << _T("Orientation") << rptNewLine << _T("(") << strSlopeTag << _T("/") << strSlopeTag << _T(")");

   if (bHasOverlay)
   {
      (*pTable)(0, col++) << COLHDR(_T("Overlay Depth"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
   }

   (*pTable)(0, col++) << COLHDR(_T("Slab") << rptNewLine << _T("Offset"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
   (*pTable)(0, col++) << COLHDR(_T("Height") << rptNewLine << _T("of") << rptNewLine << _T("Girder"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
   (*pTable)(0, col++) << COLHDR(_T("Top") << rptNewLine << _T("Bearing") << rptNewLine << _T("Elev"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*pTable)(0, col++) << COLHDR(_T("Net") << rptNewLine << _T("Bearing") << rptNewLine << _T("Height"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
   (*pTable)(0, col++) << COLHDR(_T("Bearing") << rptNewLine << _T("Deduct"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
   (*pTable)(0, col++) << COLHDR(_T("Bearing") << rptNewLine << _T("Seat") << rptNewLine << _T("Elev"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

   return pTable;
}

// Macro we can use to write a newline before our table txt value
#define WRITE_NEWLINE_BEFORE(doWriteNewLineRow, row, col, txt) if (doWriteNewLineRow) { (*pTable)(row, col) << rptNewLine << txt;} else { (*pTable)(row, col) << txt;}

inline void FillTable(rptRcTable* pTable, IEAFDisplayUnits* pDisplayUnits, 
                      rptLengthSectionValue& elev, rptLengthSectionValue& dim, rptLengthSectionValue& dist, 
                      bool bHasOverlay, bool bHasPrecamber, const std::vector<BearingElevationDetails>& vElevDetails)
{
   RowIndexType Row = pTable->GetNumberOfHeaderRows();

   std::vector<BearingElevationDetails>::const_iterator Iter(vElevDetails.begin());
   std::vector<BearingElevationDetails>::const_iterator End(vElevDetails.end());
   GirderIndexType lastGdrIdx = INVALID_INDEX;
   for (; Iter != End; Iter++)
   {
      const BearingElevationDetails& ElevDetails = *Iter;

      // put multiple bearings for same girder in same row
      bool newRow = (lastGdrIdx == INVALID_INDEX) || (lastGdrIdx != ElevDetails.GirderKey.girderIndex);
      if (newRow)
      {
         Row++;
      }

      lastGdrIdx = ElevDetails.GirderKey.girderIndex;

      ColumnIndexType Col = 0;

      if (newRow)
      {
         (*pTable)(Row, Col++) << LABEL_GIRDER(ElevDetails.GirderKey.girderIndex);
      }
      else
      {
         Col++;
      }

      WRITE_NEWLINE_BEFORE(!newRow, Row, Col++, ElevDetails.BearingIdx + 1);
      WRITE_NEWLINE_BEFORE(!newRow, Row, Col++, rptRcStation(ElevDetails.Station, &pDisplayUnits->GetStationFormat()));

      WRITE_NEWLINE_BEFORE(!newRow, Row, Col++, RPT_OFFSET(ElevDetails.Offset, dist));
      WRITE_NEWLINE_BEFORE(!newRow, Row, Col++, elev.SetValue(ElevDetails.FinishedGradeElevation));
      WRITE_NEWLINE_BEFORE(!newRow, Row, Col++, ElevDetails.ProfileGrade);
      if (bHasPrecamber)
      {
         WRITE_NEWLINE_BEFORE(!newRow, Row, Col++, ElevDetails.BasicGirderGrade);
         WRITE_NEWLINE_BEFORE(!newRow, Row, Col++, ElevDetails.PrecamberSlope);
      }
      WRITE_NEWLINE_BEFORE(!newRow, Row, Col++, ElevDetails.GirderGrade);
      WRITE_NEWLINE_BEFORE(!newRow, Row, Col++, ElevDetails.GirderOrientation);

      if (bHasOverlay)
      {
         WRITE_NEWLINE_BEFORE(!newRow, Row, Col++, dim.SetValue(ElevDetails.OverlayDepth));
      }

      WRITE_NEWLINE_BEFORE(!newRow, Row, Col++, dim.SetValue(ElevDetails.SlabOffset));
      WRITE_NEWLINE_BEFORE(!newRow, Row, Col++, dim.SetValue(ElevDetails.Hg));
      WRITE_NEWLINE_BEFORE(!newRow, Row, Col++, elev.SetValue(ElevDetails.TopBrgElevation));
      WRITE_NEWLINE_BEFORE(!newRow, Row, Col++, dim.SetValue(GetNetBearingHeight(ElevDetails)));
      WRITE_NEWLINE_BEFORE(!newRow, Row, Col++, dim.SetValue(ElevDetails.BearingDeduct));
      WRITE_NEWLINE_BEFORE(!newRow, Row, Col++, elev.SetValue(ElevDetails.BrgSeatElevation));
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

   GET_IFACE2(pBroker, ICamber, pCamber);

   PierIndexType nPiers = pBridge->GetPierCount();
   for (PierIndexType pierIdx = 0; pierIdx < nPiers; pierIdx++)
   {
      GroupIndexType backGroupIdx, aheadGroupIdx;
      pBridge->GetGirderGroupIndex(pierIdx, &backGroupIdx, &aheadGroupIdx);

      bool bHasPrecamberBack = false;
      bool bHasPrecamberAhead = false;
      if (pBridge->IsBoundaryPier(pierIdx))
      {
         rptRcTable* pBackTable = nullptr;
         if (pierIdx != 0)
         {
            // check if we have to report precamber slope
            GirderIndexType nGirders = pBridge->GetGirderCount(backGroupIdx);
            for (GirderIndexType gdrIdx = 0; gdrIdx < nGirders && bHasPrecamberBack == false; gdrIdx++)
            {
               CSegmentKey backSegmentKey, aheadSegmentKey;
               pBridge->GetSegmentsAtPier(pierIdx, gdrIdx, &backSegmentKey, &aheadSegmentKey);
               bHasPrecamberBack = (IsZero(pCamber->GetPrecamber(backSegmentKey)) ? false : true);
            }

            CString strBackLabel;
            strBackLabel.Format(_T("%s %d Back"), pierIdx == 0 || pierIdx == nPiers - 1 ? _T("Abutment") : _T("Pier"), LABEL_PIER(pierIdx));
            pBackTable = MakeTable(strBackLabel,pDisplayUnits,bHasOverlay,bHasPrecamberBack);
            (*pPara) << pBackTable << rptNewLine;
         }

         rptRcTable* pAheadTable = nullptr;
         if (pierIdx != nPiers - 1)
         {
            // check if we have to report precamber slope
            GirderIndexType nGirders = pBridge->GetGirderCount(aheadGroupIdx);
            for (GirderIndexType gdrIdx = 0; gdrIdx < nGirders && bHasPrecamberAhead == false; gdrIdx++)
            {
               CSegmentKey backSegmentKey, aheadSegmentKey;
               pBridge->GetSegmentsAtPier(pierIdx, gdrIdx, &backSegmentKey, &aheadSegmentKey);
               bHasPrecamberAhead = (IsZero(pCamber->GetPrecamber(aheadSegmentKey)) ? false : true);
            }

            CString strAheadLabel;
            strAheadLabel.Format(_T("%s %d Ahead"), pierIdx == 0 || pierIdx == nPiers - 1 ? _T("Abutment") : _T("Pier"), LABEL_PIER(pierIdx));
            pAheadTable = MakeTable(strAheadLabel,pDisplayUnits,bHasOverlay,bHasPrecamberAhead);
            (*pPara) << pAheadTable << rptNewLine;
         }

         if (pBackTable)
         {
            std::vector<BearingElevationDetails> vBackElevDetails = pBridge->GetBearingElevationDetails(pierIdx, pgsTypes::Back);

            FillTable(pBackTable, pDisplayUnits, elev, dim, dist, bHasOverlay, bHasPrecamberBack, vBackElevDetails);
         }

         if (pAheadTable)
         {
            std::vector<BearingElevationDetails> vAheadElevDetails = pBridge->GetBearingElevationDetails(pierIdx, pgsTypes::Ahead);

            FillTable(pAheadTable, pDisplayUnits, elev, dim, dist, bHasOverlay, bHasPrecamberAhead, vAheadElevDetails);
         }
      }
      else
      {
         // Interior Pier
         pgsTypes::PierSegmentConnectionType connectionType = pBridge->GetPierSegmentConnectionType(pierIdx);
         if (connectionType == pgsTypes::psctContinousClosureJoint || connectionType == pgsTypes::psctContinousClosureJoint)
         {
            // check if we have to report precamber slope
            GirderIndexType nGirders = pBridge->GetGirderCount(backGroupIdx);
            for (GirderIndexType gdrIdx = 0; gdrIdx < nGirders && bHasPrecamberBack == false; gdrIdx++)
            {
               CSegmentKey backSegmentKey, aheadSegmentKey;
               pBridge->GetSegmentsAtPier(pierIdx, gdrIdx, &backSegmentKey, &aheadSegmentKey);
               bHasPrecamberBack = (IsZero(pCamber->GetPrecamber(backSegmentKey)) ? false : true);
            }
         }

         CString strLabel;
         strLabel.Format(_T("Pier %d"), LABEL_PIER(pierIdx));
         rptRcTable* pTable = nullptr;
         pTable = MakeTable(strLabel,pDisplayUnits,bHasOverlay,bHasPrecamberBack);
         (*pPara) << pTable << rptNewLine;

         std::vector<BearingElevationDetails> vBackElevDetails = pBridge->GetBearingElevationDetails(pierIdx, pgsTypes::Back);

         FillTable(pTable, pDisplayUnits, elev, dim, dist, bHasOverlay, bHasPrecamberBack, vBackElevDetails);
      }
   }

   return pChapter;
}

CChapterBuilder* CBearingSeatElevationsDetailsChapterBuilder2::Clone() const
{
   return new CBearingSeatElevationsDetailsChapterBuilder2;
}
