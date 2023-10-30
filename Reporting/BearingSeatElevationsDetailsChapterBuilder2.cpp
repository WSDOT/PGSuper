///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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
#include <IFace\Intervals.h>


#include <PgsExt\GirderLabel.h>
#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\DeckDescription2.h>

#include <PGSuperUnits.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

inline void GetStartEndGirderIndex(GirderIndexType girderIdx,IBridge* pBridge,GroupIndexType groupIdx,GirderIndexType* pStartIndex,GirderIndexType* pEndIndex)
{
   // Encapsulate loop logic that is used many times here
   if (girderIdx != ALL_GIRDERS)
   {
      *pStartIndex = girderIdx;
      *pEndIndex = girderIdx;
   }
   else
   {
      *pStartIndex = 0;
      *pEndIndex = pBridge->GetGirderCount(groupIdx) - 1;
   }
}

inline Float64 GetNetBearingHeight(const BearingElevationDetails& det)
{
   return det.BrgHeight + det.SolePlateHeight - det.BrgRecess;
}

inline rptRcTable* MakeTable(const CString& strLabel, IEAFDisplayUnits* pDisplayUnits, bool bHasOverlay, bool bHasPrecamber, bool bIsGirderEdges,bool bIsDirectHaunchInput)
{
   ColumnIndexType nCols = 15 + (bHasOverlay ? 1 : 0) + (bHasPrecamber ? 2 : 0) + (bIsDirectHaunchInput ? 2 : 0);

   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(nCols, strLabel);

   std::_tstring strSlopeTag = pDisplayUnits->GetAlignmentLengthUnit().UnitOfMeasure.UnitTag();

   ColumnIndexType col = 0;
   (*pTable)(0, col++) << _T("Girder");

   if (bIsGirderEdges)
   {
      (*pTable)(0, col++) << _T("Edge") << Super(_T("*"));
   }
   else
   {
      (*pTable)(0, col++) << _T("Bearing") << Super(_T("*")) << rptNewLine << _T("#");
   }

   (*pTable)(0, col++) << _T("Station");
   (*pTable)(0, col++) << COLHDR(_T("Offset"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

   if (bIsDirectHaunchInput)
   {
      (*pTable)(0,col++) << COLHDR(_T("Design") << rptNewLine << _T("Deck") << rptNewLine << _T("Elev"),rptLengthUnitTag,pDisplayUnits->GetSpanLengthUnit());
   }

   (*pTable)(0, col++) << COLHDR(_T("Finish") << rptNewLine << _T("Deck") << rptNewLine << _T("Elev"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*pTable)(0, col++) << _T("Profile") << rptNewLine << _T("Grade") << rptNewLine << _T("(") << strSlopeTag << _T("/") << strSlopeTag << _T(")");
   (*pTable)(0, col++) << _T("Cross") << rptNewLine << _T("Slope") << rptNewLine << _T("(") << strSlopeTag << _T("/") << strSlopeTag << _T(")");
   if (bHasPrecamber)
   {
      (*pTable)(0, col++) << _T("Basic Girder") << rptNewLine << _T("Grade") << rptNewLine << _T("(") << strSlopeTag << _T("/") << strSlopeTag << _T(")");
      (*pTable)(0, col++) << _T("Precamber") << rptNewLine << _T("Slope") << rptNewLine << _T("(") << strSlopeTag << _T("/") << strSlopeTag << _T(")");
   }
   (*pTable)(0, col++) << _T("Girder") << rptNewLine << _T("Grade") << rptNewLine << _T("(") << strSlopeTag << _T("/") << strSlopeTag << _T(")");
   (*pTable)(0, col++) << _T("Girder") << rptNewLine << _T("Orientation") << rptNewLine << _T("(") << strSlopeTag << _T("/") << strSlopeTag << _T(")");

   if (bHasOverlay)
   {
      (*pTable)(0, col++) << COLHDR(_T("Net") << rptNewLine << _T("Overlay") << rptNewLine << _T("Depth"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
   }

   (*pTable)(0, col++) << COLHDR(_T("Slab") << rptNewLine << _T("Offset"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());

   if (bIsDirectHaunchInput)
   {
      (*pTable)(0,col++) << COLHDR(_T("Haunch") << rptNewLine << _T("Depth"),rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit());
   }

   (*pTable)(0, col++) << COLHDR(_T("Net") << rptNewLine << _T("Girder") << rptNewLine <<_T("Height"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
   (*pTable)(0, col++) << COLHDR(_T("Top") << rptNewLine << _T("Bearing") << rptNewLine << _T("Elev"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*pTable)(0, col++) << COLHDR(_T("Net") << rptNewLine << _T("Bearing") << rptNewLine << _T("Height"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
   (*pTable)(0, col++) << COLHDR(_T("Bearing") << rptNewLine << _T("Seat") << rptNewLine << _T("Elev"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*pTable)(0, col++) << COLHDR(_T("Bearing") << rptNewLine << _T("Deduct"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());

   return pTable;
}

// Macro we can use to write a newline before our table txt value
#define WRITE_NEWLINE_BEFORE(doWriteNewLineRow, row, col, txt) if (doWriteNewLineRow) { (*pTable)(row, col) << rptNewLine << txt;} else { (*pTable)(row, col) << txt;}

#define ADD_TABLE_NOTE  *pPara << _T("* Note that reported bearing locations are physical 3D locations of bearings adjusted for girder depth, slope and orientation. \'CL\' is location of bearing seat at intersection of CL girder and bearing line. Roadway elevations at bearing line - work line intersections can be found in other tables in the Bridge Geometry report.")<< rptNewLine;

inline void FillTable(rptRcTable* pTable, IEAFDisplayUnits* pDisplayUnits, 
                      rptLengthSectionValue& elev, rptLengthSectionValue& dim, rptLengthSectionValue& dist, 
                      bool bHasOverlay, bool bHasPrecamber, bool bIsDirectHaunchInput, const std::vector<BearingElevationDetails>& vElevDetails)
{
   RowIndexType Row = pTable->GetNumberOfHeaderRows();

   std::vector<BearingElevationDetails>::const_iterator Iter(vElevDetails.begin());
   std::vector<BearingElevationDetails>::const_iterator End(vElevDetails.end());
   GirderIndexType lastGdrIdx = INVALID_INDEX;
   for (; Iter != End; Iter++)
   {
      const BearingElevationDetails& ElevDetails = *Iter;

      // CL values have preset index
      bool isSingleBearing = ElevDetails.BearingIdx == IBridge::sbiSingleBearingValue;
      bool isCL = ElevDetails.BearingIdx == IBridge::sbiCLValue || isSingleBearing;

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

      if (isSingleBearing)
      {
         WRITE_NEWLINE_BEFORE(!newRow, Row, Col++, _T("1"));
      }
      else if (isCL)
      {
         WRITE_NEWLINE_BEFORE(!newRow, Row, Col++, _T("CL"));
      }
      else
      {
      WRITE_NEWLINE_BEFORE(!newRow, Row, Col++, ElevDetails.BearingIdx + 1);
      }

      WRITE_NEWLINE_BEFORE(!newRow, Row, Col++, rptRcStation(ElevDetails.Station, &pDisplayUnits->GetStationFormat()));

      WRITE_NEWLINE_BEFORE(!newRow, Row, Col++, RPT_OFFSET(ElevDetails.Offset, dist));

      if (bIsDirectHaunchInput)
      {
         WRITE_NEWLINE_BEFORE(!newRow,Row,Col++,elev.SetValue(ElevDetails.DesignGradeElevation));
      }

      WRITE_NEWLINE_BEFORE(!newRow, Row, Col++, elev.SetValue(ElevDetails.FinishedGradeElevation));

      if (isCL)
      {
      WRITE_NEWLINE_BEFORE(!newRow, Row, Col++, ElevDetails.ProfileGrade);
      }
      else
      {
         WRITE_NEWLINE_BEFORE(!newRow, Row, Col++, _T("-"));
      }

      if (isCL)
      {
         WRITE_NEWLINE_BEFORE(!newRow, Row, Col++, ElevDetails.CrossSlope);
      }
      else
      {
         WRITE_NEWLINE_BEFORE(!newRow, Row, Col++, _T("-"));
      }

      if (bHasPrecamber)
      {
         if (isCL)
         {
         WRITE_NEWLINE_BEFORE(!newRow, Row, Col++, ElevDetails.BasicGirderGrade);
         }
         else
         {
            WRITE_NEWLINE_BEFORE(!newRow, Row, Col++, _T("-"));
         }

         if (isCL)
         {
         WRITE_NEWLINE_BEFORE(!newRow, Row, Col++, ElevDetails.PrecamberSlope);
      }
         else
         {
            WRITE_NEWLINE_BEFORE(!newRow, Row, Col++, _T("-"));
         }
      }

      if (isCL)
      {
      WRITE_NEWLINE_BEFORE(!newRow, Row, Col++, ElevDetails.GirderGrade);
      }
      else
      {
         WRITE_NEWLINE_BEFORE(!newRow, Row, Col++, _T("-"));
      }

      if (isCL)
      {
         // convert to right-hand rule convention
         WRITE_NEWLINE_BEFORE(!newRow, Row, Col++, -1.0 * ElevDetails.GirderOrientation);
      }
      else
      {
         WRITE_NEWLINE_BEFORE(!newRow, Row, Col++, _T("-"));
      }

      if (bHasOverlay)
      {
         if (isCL)
         {
         WRITE_NEWLINE_BEFORE(!newRow, Row, Col++, dim.SetValue(ElevDetails.OverlayDepth));
      }
         else
         {
            WRITE_NEWLINE_BEFORE(!newRow, Row, Col++, _T("-"));
         }
      }

      if (isCL)
      {
         WRITE_NEWLINE_BEFORE(!newRow, Row, Col++, dim.SetValue(ElevDetails.SlabOffset));
         if (bIsDirectHaunchInput)
         {
            WRITE_NEWLINE_BEFORE(!newRow,Row,Col++,dim.SetValue(ElevDetails.HaunchDepth));
         }
      }
      else
      {
         WRITE_NEWLINE_BEFORE(!newRow, Row, Col++, _T("-"));
         if (bIsDirectHaunchInput)
         {
            WRITE_NEWLINE_BEFORE(!newRow,Row,Col++,_T("-"));
         }
      }

      if (isCL)
      {
      WRITE_NEWLINE_BEFORE(!newRow, Row, Col++, dim.SetValue(ElevDetails.Hg));
      }
      else
      {
         WRITE_NEWLINE_BEFORE(!newRow, Row, Col++, _T("-"));
      }

      WRITE_NEWLINE_BEFORE(!newRow, Row, Col++, elev.SetValue(ElevDetails.TopBrgElevation));
      WRITE_NEWLINE_BEFORE(!newRow, Row, Col++, dim.SetValue(GetNetBearingHeight(ElevDetails)));
      WRITE_NEWLINE_BEFORE(!newRow, Row, Col++, elev.SetValue(ElevDetails.BrgSeatElevation));
      WRITE_NEWLINE_BEFORE(!newRow, Row, Col++, dim.SetValue(ElevDetails.BearingDeduct));
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

rptChapter* CBearingSeatElevationsDetailsChapterBuilder2::Build(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec, Uint16 level) const
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec, level);

   auto pGdrRptSpec = std::dynamic_pointer_cast<const CGirderReportSpecification>(pRptSpec);
   auto pGdrLineRptSpec = std::dynamic_pointer_cast<const CGirderLineReportSpecification>(pRptSpec);

   GirderIndexType girderIndex;
   if (pGdrRptSpec)
   {
      girderIndex = pGdrRptSpec->GetGirderKey().girderIndex;
   }
   else if (pGdrLineRptSpec)
   {
      girderIndex = pGdrLineRptSpec->GetGirderKey().girderIndex;
   }
   else
   {
      ATLASSERT(false); // not expecting a different kind of report spec
      return pChapter;
   }

   BuildBearingsTables(pBroker, pChapter, girderIndex);

   // Use setting for span 1, girder 1 to determine whether girder edge table is to be printed.
   // Could get more sophisticated here, but all girders in the same DOT family should have the same setting.
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(GroupIndexType(0));
   const CSplicedGirderData* pGirder = pGroup->GetGirder(0);
   const GirderLibraryEntry* pGirderEntry = pGirder->GetGirderLibraryEntry();

   if (pGirderEntry->GetDoReportBearingElevationsAtGirderEdges())
   {
      BuildGirderEdgesTables(pBroker, pChapter, girderIndex);
   }

   return pChapter;
}

void CBearingSeatElevationsDetailsChapterBuilder2::BuildBearingsTables(CComPtr<IBroker> pBroker, rptChapter* pChapter, GirderIndexType girderIndex) const
{
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
      GET_IFACE2(pBroker,IIntervals,pIntervals);
      IntervalIndexType gcInterval = pIntervals->GetGeometryControlInterval();

      Float64 density = WBFL::Units::ConvertToSysUnits(140.0,WBFL::Units::Measure::LbfPerFeet3);

      *pPara << _T("NOTE: overlay depth estimated based on a unit weight of ") << ::FormatDimension(density,pDisplayUnits->GetDensityUnit(),true) << rptNewLine;
      *pPara << ::FormatDimension(pDeck->OverlayWeight,pDisplayUnits->GetOverlayWeightUnit(),true) << _T("/") << ::FormatDimension(density,pDisplayUnits->GetDensityUnit(),true) << _T(" = ") << ::FormatDimension(pBridge->GetOverlayDepth(gcInterval),pDisplayUnits->GetComponentDimUnit(),true) << rptNewLine;
   }

   *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("BearingSeatElevation.png")) << rptNewLine;

   *pPara << _T("NOTE: Vertical height dimensions listed in the tables below are adjusted for girder and roadway grade and orientation.") << rptNewLine;
   *pPara << _T("NOTE: Bearing Deduct is a nominal value rounded to 1/8\"") << rptNewLine;

   bool bHasOverlay = ( pBridge->HasOverlay() && !pBridge->IsFutureOverlay() ? true : false );

   bool bIsDirectHaunchInput = pBridgeDesc->GetHaunchInputDepthType() != pgsTypes::hidACamber;

   GET_IFACE2_NOCHECK(pBroker, ICamber, pCamber);

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
            GirderIndexType startGdrIdx,endGdrIdx;
            GetStartEndGirderIndex(girderIndex,pBridge,backGroupIdx,&startGdrIdx,&endGdrIdx);
            for (GirderIndexType gdrIdx = startGdrIdx; gdrIdx < endGdrIdx && bHasPrecamberBack == false; gdrIdx++)
            {
               CSegmentKey backSegmentKey, aheadSegmentKey;
               pBridge->GetSegmentsAtPier(pierIdx, gdrIdx, &backSegmentKey, &aheadSegmentKey);
               bHasPrecamberBack = (IsZero(pCamber->GetPrecamber(backSegmentKey)) ? false : true);
            }

            CString strBackLabel;
            strBackLabel.Format(_T("%s Back"), LABEL_PIER_EX(pierIdx == 0 || pierIdx == nPiers - 1 , pierIdx));
            pBackTable = MakeTable(strBackLabel,pDisplayUnits,bHasOverlay,bHasPrecamberBack, false, bIsDirectHaunchInput);
            (*pPara) << pBackTable << rptNewLine;
            ADD_TABLE_NOTE;
         }

         rptRcTable* pAheadTable = nullptr;
         if (pierIdx != nPiers - 1)
         {
            // check if we have to report precamber slope
            GirderIndexType startGdrIdx,endGdrIdx;
            GetStartEndGirderIndex(girderIndex,pBridge,aheadGroupIdx,&startGdrIdx,&endGdrIdx);
            for (GirderIndexType gdrIdx = startGdrIdx; gdrIdx < endGdrIdx && bHasPrecamberAhead == false; gdrIdx++)
            {
               CSegmentKey backSegmentKey, aheadSegmentKey;
               pBridge->GetSegmentsAtPier(pierIdx, gdrIdx, &backSegmentKey, &aheadSegmentKey);
               bHasPrecamberAhead = (IsZero(pCamber->GetPrecamber(aheadSegmentKey)) ? false : true);
            }

            CString strAheadLabel;
            strAheadLabel.Format(_T("%s Ahead"), LABEL_PIER_EX(pierIdx == 0 || pierIdx == nPiers - 1 ,pierIdx));
            pAheadTable = MakeTable(strAheadLabel,pDisplayUnits,bHasOverlay,bHasPrecamberAhead,false,bIsDirectHaunchInput);
            (*pPara) << pAheadTable << rptNewLine;
            ADD_TABLE_NOTE;
         }

         if (pBackTable)
         {
            std::vector<BearingElevationDetails> vBackElevDetails = pBridge->GetBearingElevationDetails(pierIdx, pgsTypes::Back, girderIndex, false);

            FillTable(pBackTable, pDisplayUnits, elev, dim, dist, bHasOverlay, bHasPrecamberBack, bIsDirectHaunchInput, vBackElevDetails);
         }

         if (pAheadTable)
         {
            std::vector<BearingElevationDetails> vAheadElevDetails = pBridge->GetBearingElevationDetails(pierIdx, pgsTypes::Ahead, girderIndex, false);

            FillTable(pAheadTable, pDisplayUnits, elev, dim, dist, bHasOverlay, bHasPrecamberAhead,bIsDirectHaunchInput, vAheadElevDetails);
         }
      }
      else
      {
         // Interior Pier
         pgsTypes::PierSegmentConnectionType connectionType = pBridge->GetPierSegmentConnectionType(pierIdx);
         if (connectionType == pgsTypes::psctContinousClosureJoint || connectionType == pgsTypes::psctContinousClosureJoint)
         {
            // check if we have to report precamber slope
            GirderIndexType startGdrIdx,endGdrIdx;
            GetStartEndGirderIndex(girderIndex,pBridge,backGroupIdx,&startGdrIdx,&endGdrIdx);
            for (GirderIndexType gdrIdx = startGdrIdx; gdrIdx < endGdrIdx && bHasPrecamberBack == false; gdrIdx++)
            {
               CSegmentKey backSegmentKey, aheadSegmentKey;
               pBridge->GetSegmentsAtPier(pierIdx, gdrIdx, &backSegmentKey, &aheadSegmentKey);
               bHasPrecamberBack = (IsZero(pCamber->GetPrecamber(backSegmentKey)) ? false : true);
            }
         }

         CString strLabel;
         strLabel.Format(_T("%s"), LABEL_PIER_EX(false, pierIdx));
         rptRcTable* pTable = nullptr;
         pTable = MakeTable(strLabel,pDisplayUnits,bHasOverlay,bHasPrecamberBack,false,bIsDirectHaunchInput);
         (*pPara) << pTable << rptNewLine;
         ADD_TABLE_NOTE;

         std::vector<BearingElevationDetails> vBackElevDetails = pBridge->GetBearingElevationDetails(pierIdx, pgsTypes::Back, girderIndex, false);

         FillTable(pTable, pDisplayUnits, elev, dim, dist, bHasOverlay, bHasPrecamberBack, bIsDirectHaunchInput, vBackElevDetails);
      }
   }
}

inline void FillGirderEdgesTable(rptRcTable* pTable, IEAFDisplayUnits* pDisplayUnits, 
                      rptLengthSectionValue& elev, rptLengthSectionValue& dim, rptLengthSectionValue& dist, 
                      bool bHasOverlay, bool bHasPrecamber,bool bIsDirectHaunchInput, const std::vector<BearingElevationDetails>& vElevDetails)
{
   RowIndexType Row = pTable->GetNumberOfHeaderRows();

   std::vector<BearingElevationDetails>::const_iterator Iter(vElevDetails.begin());
   std::vector<BearingElevationDetails>::const_iterator End(vElevDetails.end());
   GirderIndexType lastGdrIdx = INVALID_INDEX;
   for (; Iter != End; Iter++)
   {
      const BearingElevationDetails& ElevDetails = *Iter;

      bool isCL = ElevDetails.BearingIdx == IBridge::sbiCLValue;

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

      WRITE_NEWLINE_BEFORE(!newRow, Row, Col++, (ElevDetails.BearingIdx==IBridge::sbiCLValue ? _T("CL") : (ElevDetails.BearingIdx==0 ? _T("Left") : _T("Right"))));
      WRITE_NEWLINE_BEFORE(!newRow, Row, Col++, rptRcStation(ElevDetails.Station, &pDisplayUnits->GetStationFormat()));

      WRITE_NEWLINE_BEFORE(!newRow, Row, Col++, RPT_OFFSET(ElevDetails.Offset, dist));

      if (bIsDirectHaunchInput)
      {
         WRITE_NEWLINE_BEFORE(!newRow,Row,Col++,elev.SetValue(ElevDetails.DesignGradeElevation));
      }

      WRITE_NEWLINE_BEFORE(!newRow, Row, Col++, elev.SetValue(ElevDetails.FinishedGradeElevation));

      if (isCL)
      {
         WRITE_NEWLINE_BEFORE(!newRow, Row, Col++, ElevDetails.ProfileGrade);
      }
      else
      {
         WRITE_NEWLINE_BEFORE(!newRow, Row, Col++, _T("-"));
      }

      if (isCL)
      {
         WRITE_NEWLINE_BEFORE(!newRow, Row, Col++, ElevDetails.CrossSlope);
      }
      else
      {
         WRITE_NEWLINE_BEFORE(!newRow, Row, Col++, _T("-"));
      }

      if (bHasPrecamber)
      {
         if (isCL)
         {
            WRITE_NEWLINE_BEFORE(!newRow, Row, Col++, ElevDetails.BasicGirderGrade);
         }
         else
         {
            WRITE_NEWLINE_BEFORE(!newRow, Row, Col++, _T("-"));
         }

         if (isCL)
         {
            WRITE_NEWLINE_BEFORE(!newRow, Row, Col++, ElevDetails.PrecamberSlope);
         }
         else
         {
            WRITE_NEWLINE_BEFORE(!newRow, Row, Col++, _T("-"));
         }
      }

      if (isCL)
      {
         WRITE_NEWLINE_BEFORE(!newRow, Row, Col++, ElevDetails.GirderGrade);
      }
      else
      {
         WRITE_NEWLINE_BEFORE(!newRow, Row, Col++, _T("-"));
      }

      if (isCL)
      {
         WRITE_NEWLINE_BEFORE(!newRow, Row, Col++, ElevDetails.GirderOrientation);
      }
      else
      {
         WRITE_NEWLINE_BEFORE(!newRow, Row, Col++, _T("-"));
      }

      if (bHasOverlay)
      {
         if (isCL)
         {
            WRITE_NEWLINE_BEFORE(!newRow, Row, Col++, dim.SetValue(ElevDetails.OverlayDepth));
         }
         else
         {
            WRITE_NEWLINE_BEFORE(!newRow, Row, Col++, _T("-"));
         }
      }

      if (isCL)
      {
         WRITE_NEWLINE_BEFORE(!newRow,Row,Col++,dim.SetValue(ElevDetails.SlabOffset));
         if (bIsDirectHaunchInput)
         {
            WRITE_NEWLINE_BEFORE(!newRow,Row,Col++,dim.SetValue(ElevDetails.HaunchDepth));
         }
      }
      else
      {
         WRITE_NEWLINE_BEFORE(!newRow,Row,Col++,_T("-"));
         if (bIsDirectHaunchInput)
         {
            WRITE_NEWLINE_BEFORE(!newRow,Row,Col++,_T("-"));
         }
      }

      if (isCL)
      {
         WRITE_NEWLINE_BEFORE(!newRow, Row, Col++, dim.SetValue(ElevDetails.Hg));
      }
      else
      {
         WRITE_NEWLINE_BEFORE(!newRow, Row, Col++, _T("-"));
      }

      WRITE_NEWLINE_BEFORE(!newRow, Row, Col++, elev.SetValue(ElevDetails.TopBrgElevation));
      WRITE_NEWLINE_BEFORE(!newRow, Row, Col++, dim.SetValue(GetNetBearingHeight(ElevDetails)));
      WRITE_NEWLINE_BEFORE(!newRow, Row, Col++, elev.SetValue(ElevDetails.BrgSeatElevation));
      WRITE_NEWLINE_BEFORE(!newRow, Row, Col++, dim.SetValue(ElevDetails.BearingDeduct));
   }
}

void CBearingSeatElevationsDetailsChapterBuilder2::BuildGirderEdgesTables(CComPtr<IBroker> pBroker, rptChapter * pChapter,GirderIndexType girderIndex) const
{
   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pPara;
   *pPara << _T("Bearing Seat Elevation Details at Girder Bottom Edges");

   pPara = new rptParagraph;
   *pChapter << pPara;
   *pPara << _T("The tables below show bearing elevations at the outermost edges of girder bottom flanges.");

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   INIT_UV_PROTOTYPE( rptLengthSectionValue, elev, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthSectionValue, dist, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthSectionValue, dim,  pDisplayUnits->GetComponentDimUnit(), false );

   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CDeckDescription2* pDeck = pBridgeDesc->GetDeckDescription();
   bool bHasOverlay = ( pBridge->HasOverlay() && !pBridge->IsFutureOverlay() ? true : false );
   bool bIsDirectHaunchInput = pBridgeDesc->GetHaunchInputDepthType() != pgsTypes::hidACamber;

   GET_IFACE2_NOCHECK(pBroker, ICamber, pCamber);

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
            GirderIndexType startGdrIdx,endGdrIdx;
            GetStartEndGirderIndex(girderIndex,pBridge,backGroupIdx,&startGdrIdx,&endGdrIdx);
            for (GirderIndexType gdrIdx = startGdrIdx; gdrIdx < endGdrIdx && bHasPrecamberBack == false; gdrIdx++)
            {
               CSegmentKey backSegmentKey, aheadSegmentKey;
               pBridge->GetSegmentsAtPier(pierIdx, gdrIdx, &backSegmentKey, &aheadSegmentKey);
               bHasPrecamberBack = (IsZero(pCamber->GetPrecamber(backSegmentKey)) ? false : true);
            }

            CString strBackLabel;
            strBackLabel.Format(_T("%s Back"), LABEL_PIER_EX(pierIdx == 0 || pierIdx == nPiers - 1, pierIdx));
            pBackTable = MakeTable(strBackLabel,pDisplayUnits,bHasOverlay,bHasPrecamberBack,true,bIsDirectHaunchInput);
            (*pPara) << pBackTable << rptNewLine;
            ADD_TABLE_NOTE;
         }

         rptRcTable* pAheadTable = nullptr;
         if (pierIdx != nPiers - 1)
         {
            // check if we have to report precamber slope
            GirderIndexType startGdrIdx,endGdrIdx;
            GetStartEndGirderIndex(girderIndex,pBridge,aheadGroupIdx,&startGdrIdx,&endGdrIdx);
            for (GirderIndexType gdrIdx = startGdrIdx; gdrIdx < endGdrIdx && bHasPrecamberAhead == false; gdrIdx++)
            {
               CSegmentKey backSegmentKey, aheadSegmentKey;
               pBridge->GetSegmentsAtPier(pierIdx, gdrIdx, &backSegmentKey, &aheadSegmentKey);
               bHasPrecamberAhead = (IsZero(pCamber->GetPrecamber(aheadSegmentKey)) ? false : true);
            }

            CString strAheadLabel;
            strAheadLabel.Format(_T("%s Ahead"), LABEL_PIER_EX(pierIdx == 0 || pierIdx == nPiers - 1, pierIdx));
            pAheadTable = MakeTable(strAheadLabel,pDisplayUnits,bHasOverlay,bHasPrecamberAhead,true,bIsDirectHaunchInput);
            (*pPara) << pAheadTable << rptNewLine;
            ADD_TABLE_NOTE;
         }

         if (pBackTable)
         {
            std::vector<BearingElevationDetails> vBackElevDetails = pBridge->GetBearingElevationDetailsAtGirderEdges(pierIdx, pgsTypes::Back, girderIndex);

            FillGirderEdgesTable(pBackTable, pDisplayUnits, elev, dim, dist, bHasOverlay, bHasPrecamberBack,bIsDirectHaunchInput, vBackElevDetails);
         }

         if (pAheadTable)
         {
            std::vector<BearingElevationDetails> vAheadElevDetails = pBridge->GetBearingElevationDetailsAtGirderEdges(pierIdx, pgsTypes::Ahead, girderIndex);

            FillGirderEdgesTable(pAheadTable, pDisplayUnits, elev, dim, dist, bHasOverlay, bHasPrecamberAhead,bIsDirectHaunchInput, vAheadElevDetails);
         }
      }
      else
      {
         // Interior Pier
         pgsTypes::PierSegmentConnectionType connectionType = pBridge->GetPierSegmentConnectionType(pierIdx);
         if (connectionType == pgsTypes::psctContinousClosureJoint || connectionType == pgsTypes::psctContinousClosureJoint)
         {
            // check if we have to report precamber slope
            GirderIndexType startGdrIdx,endGdrIdx;
            GetStartEndGirderIndex(girderIndex,pBridge,backGroupIdx,&startGdrIdx,&endGdrIdx);
            for (GirderIndexType gdrIdx = startGdrIdx; gdrIdx < endGdrIdx && bHasPrecamberBack == false; gdrIdx++)
            {
               CSegmentKey backSegmentKey, aheadSegmentKey;
               pBridge->GetSegmentsAtPier(pierIdx, gdrIdx, &backSegmentKey, &aheadSegmentKey);
               bHasPrecamberBack = (IsZero(pCamber->GetPrecamber(backSegmentKey)) ? false : true);
            }
         }

         CString strLabel;
         strLabel.Format(_T("%s"), LABEL_PIER_EX(false, pierIdx));
         rptRcTable* pTable = nullptr;
         pTable = MakeTable(strLabel,pDisplayUnits,bHasOverlay,bHasPrecamberBack,true,bIsDirectHaunchInput);
         (*pPara) << pTable << rptNewLine;
         ADD_TABLE_NOTE;

         std::vector<BearingElevationDetails> vBackElevDetails = pBridge->GetBearingElevationDetailsAtGirderEdges(pierIdx, pgsTypes::Back, girderIndex);

         FillGirderEdgesTable(pTable, pDisplayUnits, elev, dim, dist, bHasOverlay, bHasPrecamberBack,bIsDirectHaunchInput, vBackElevDetails);
      }
   }
}

std::unique_ptr<WBFL::Reporting::ChapterBuilder> CBearingSeatElevationsDetailsChapterBuilder2::Clone() const
{
   return std::make_unique<CBearingSeatElevationsDetailsChapterBuilder2>();
}
