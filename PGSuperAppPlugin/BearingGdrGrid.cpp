///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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

// BearingGdrGrid.cpp : implementation file
//

#include "stdafx.h"
#include "BearingGdrGrid.h"


#include "resource.h"
#include "BearingGdrByGdrDlg.h"

#include "IFace\Project.h"

#include <System\Tokenizer.h>
#include "PGSuperUnits.h"
#include <Units\Measure.h>
#include <EAF\EAFDisplayUnits.h>
#include <PsgLib\GirderLabel.h>
#include <PsgLib\BearingCriteria.h>
#include <PsgLib\SpecLibraryEntry.h>
#include <AgentTools.h>


const ROWCOL _STARTCOL = 1;
const ROWCOL _STARTNCOLS = 13;

GRID_IMPLEMENT_REGISTER(CBearingGdrGrid, CS_DBLCLKS, 0, 0, 0);

/////////////////////////////////////////////////////////////////////////////
// CBearingGdrGrid
CBearingGdrGrid::CBearingGdrGrid():
m_pBearingInputData(nullptr),
m_SpanIdx(INVALID_INDEX)
{
//   RegisterClass();
}

CBearingGdrGrid::CBearingGdrGrid(BearingInputData* pData):
m_pBearingInputData(pData),
m_SpanIdx(INVALID_INDEX)
{
//   RegisterClass();
}

CBearingGdrGrid::~CBearingGdrGrid()
{
}

BEGIN_MESSAGE_MAP(CBearingGdrGrid, CGXGridWnd)
	//{{AFX_MSG_MAP(CBearingGdrGrid)
		// NOTE - the ClassWizard will add and remove mapping macros here.
   ON_MESSAGE(WM_GX_NEEDCHANGETAB, ChangeTabName) 

	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CBearingGdrGrid message handlers

int CBearingGdrGrid::GetColWidth(ROWCOL nCol)
{
    if (IsColHidden(nCol))
        return CGXGridWnd::GetColWidth(nCol);

    ROWCOL nspc = _STARTNCOLS + 3;

    CRect rect = GetGridRect();

    switch (nCol)
    {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
        return (int)(rect.Width() * (Float64)2 / (nspc + 2));
    default:
        return (int)(rect.Width() / nspc);
    }
}

void CBearingGdrGrid::CustomInit(SpanIndexType ispan)
{
   m_SpanIdx = ispan;

   // initialize units
   auto pBroker = EAFGetBroker();
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   m_pCompUnit = &(pDisplayUnits->GetComponentDimUnit());

// Initialize the grid. For CWnd based grids this call is // 
// essential. For view based grids this initialization is done 
// in OnInitialUpdate.
	Initialize( );

	GetParam( )->EnableUndo(FALSE);

   const int num_rows=0;
   const int num_cols=_STARTNCOLS;

	SetRowCount(num_rows);
	SetColCount(num_cols);

   // Turn off selecting 
	GetParam()->EnableSelection((WORD) (GX_SELFULL & ~GX_SELCOL & ~GX_SELTABLE));

   // disable left side
	this->SetStyleRange(CGXRange(0,0,num_rows,0), CGXStyle()
			.SetControl(GX_IDS_CTRL_HEADER)
			.SetEnabled(FALSE)          // disables usage as current cell
		);

   // no row moving
   GetParam()->EnableMoveRows(FALSE);

   // don't allow users to resize grids
   GetParam( )->EnableTrackColWidth(0); 
   GetParam( )->EnableTrackRowHeight(0); 

   // Header
   ROWCOL col=1;
	SetStyleRange(CGXRange(0,col,0,col+1), CGXStyle()
         .SetWrapText(TRUE)
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_TOP)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE)
			.SetValue(_T("\nLocation\n")) // two rows so we expand properly
		);

   col+=2;
   m_DGetter.m_BearingDefTypeCol = col;
	SetStyleRange(CGXRange(0,col), CGXStyle()
      .SetWrapText(TRUE)
      .SetHorizontalAlignment(DT_CENTER)
      .SetVerticalAlignment(DT_TOP)
		.SetEnabled(FALSE)          // disables usage as current cell
		.SetValue(_T("\nType\n "))
		);

    col++;
    m_DGetter.m_BearingShapeCol = col;
    SetStyleRange(CGXRange(0, col), CGXStyle()
        .SetWrapText(TRUE)
        .SetHorizontalAlignment(DT_CENTER)
        .SetVerticalAlignment(DT_TOP)
        .SetEnabled(FALSE)          // disables usage as current cell
        .SetValue(_T("\nShape\n "))
    );

   col++;
   m_DGetter.m_BearingCountCol = col;
	SetStyleRange(CGXRange(0,col), CGXStyle()
      .SetWrapText(TRUE)
      .SetHorizontalAlignment(DT_CENTER)
      .SetVerticalAlignment(DT_TOP)
		.SetEnabled(FALSE)          // disables usage as current cell
      .SetValue(_T("# of\nBearings"))
		);

   col++;
   m_DGetter.m_BearingSpacingCol = col;
   CString strLabel;
   strLabel.Format(_T("\nSpacing\n(%s)"),pDisplayUnits->GetComponentDimUnit().UnitOfMeasure.UnitTag().c_str());
	SetStyleRange(CGXRange(0,col), CGXStyle()
      .SetWrapText(TRUE)
      .SetHorizontalAlignment(DT_CENTER)
      .SetVerticalAlignment(DT_TOP)
		.SetEnabled(FALSE)          // disables usage as current cell
		.SetValue(strLabel)
		);

   col++;
   m_DGetter.m_BearingLengthCol = col;
   strLabel.Format(_T("Length or\nDiameter\n(%s)"),pDisplayUnits->GetComponentDimUnit().UnitOfMeasure.UnitTag().c_str());
	SetStyleRange(CGXRange(0,col), CGXStyle()
      .SetWrapText(TRUE)
      .SetHorizontalAlignment(DT_CENTER)
      .SetVerticalAlignment(DT_TOP)
		.SetEnabled(FALSE)          // disables usage as current cell
		.SetValue(strLabel)
		);

   col++;
   m_DGetter.m_BearingWidthCol = col;
   strLabel.Format(_T("\nWidth\n(%s)"),pDisplayUnits->GetComponentDimUnit().UnitOfMeasure.UnitTag().c_str());
	SetStyleRange(CGXRange(0,col), CGXStyle()
      .SetWrapText(TRUE)
      .SetHorizontalAlignment(DT_CENTER)
      .SetVerticalAlignment(DT_TOP)
		.SetEnabled(FALSE)          // disables usage as current cell
		.SetValue(strLabel)
		);

   col++;
   m_DGetter.m_BearingHeightCol = col;
   strLabel.Format(_T("\nHeight\n(%s)"),pDisplayUnits->GetComponentDimUnit().UnitOfMeasure.UnitTag().c_str());
	SetStyleRange(CGXRange(0,col), CGXStyle()
      .SetWrapText(TRUE)
      .SetHorizontalAlignment(DT_CENTER)
      .SetVerticalAlignment(DT_TOP)
		.SetEnabled(FALSE)          // disables usage as current cell
		.SetValue(strLabel)
		);

   col++;
   m_DGetter.m_BearingRecessHeightCol = col;
   strLabel.Format(_T("Recess\nHeight\n(%s)"),pDisplayUnits->GetComponentDimUnit().UnitOfMeasure.UnitTag().c_str());
	SetStyleRange(CGXRange(0,col), CGXStyle()
      .SetWrapText(TRUE)
      .SetHorizontalAlignment(DT_CENTER)
      .SetVerticalAlignment(DT_TOP)
		.SetEnabled(FALSE)          // disables usage as current cell
		.SetValue(strLabel)
		);

   col++;
   m_DGetter.m_BearingRecessLengthCol = col;
   strLabel.Format(_T("Recess\nLength\n(%s)"),pDisplayUnits->GetComponentDimUnit().UnitOfMeasure.UnitTag().c_str());
	SetStyleRange(CGXRange(0,col), CGXStyle()
      .SetWrapText(TRUE)
      .SetHorizontalAlignment(DT_CENTER)
      .SetVerticalAlignment(DT_TOP)
		.SetEnabled(FALSE)          // disables usage as current cell
		.SetValue(strLabel)
		);

   col++;
   m_DGetter.m_BearingSolePlateCol = col;
   strLabel.Format(_T("Sole\nPlate\nHeight (%s)"), pDisplayUnits->GetComponentDimUnit().UnitOfMeasure.UnitTag().c_str());
	SetStyleRange(CGXRange(0,col), CGXStyle()
      .SetWrapText(TRUE)
      .SetHorizontalAlignment(DT_CENTER)
      .SetVerticalAlignment(DT_TOP)
		.SetEnabled(FALSE)          // disables usage as current cell
		.SetValue(strLabel)
		);

    col++;
    m_DGetter.m_BearingDetailButtonCol = col;
    strLabel = _T("Detail");
    SetStyleRange(CGXRange(0, col), CGXStyle()
        .SetWrapText(TRUE)
        .SetHorizontalAlignment(DT_CENTER)
        .SetVerticalAlignment(DT_VCENTER)
        .SetEnabled(FALSE)          // disables usage as current cell
        .SetValue(strLabel)
    );

   // make it so that text fits correctly in header row
	this->ResizeRowHeightsToFit(CGXRange(0,0,0, GetColCount()));

   // Hide the row header column
   HideCols(0, 0);

   SetScrollBarMode(SB_HORZ,gxnDisabled);
   SetScrollBarMode(SB_VERT,gxnAutomatic | gxnEnhanced);

	EnableIntelliMouse();
	SetFocus();

	GetParam( )->EnableUndo(TRUE);
}

void CBearingGdrGrid::SetRowStyle(ROWCOL nRow)
{
   ROWCOL col = _STARTCOL;
	SetStyleRange(CGXRange(nRow,_STARTCOL,nRow,_STARTCOL+1), CGXStyle()
 			.SetEnabled(FALSE)          // disables usage as current cell
			.SetInterior(GXSYSCOLOR(COLOR_BTNFACE))
			.SetHorizontalAlignment(DT_CENTER)
			.SetVerticalAlignment(DT_VCENTER)
		);

    this->SetStyleRange(CGXRange(nRow, _STARTCOL + 2), CGXStyle()
        .SetControl(GX_IDS_CTRL_CBS_DROPDOWNLIST)
        .SetChoiceList(_T("Basic\nDetailed"))
        .SetValue(_T("Basic"))
        .SetHorizontalAlignment(DT_RIGHT)
        );

	this->SetStyleRange(CGXRange(nRow,_STARTCOL + 3), CGXStyle()
			.SetControl(GX_IDS_CTRL_CBS_DROPDOWNLIST)
			.SetChoiceList(_T("Rectangular\nRound"))
			.SetValue(_T("Rectangular"))
         .SetHorizontalAlignment(DT_RIGHT)
         );

    this->SetStyleRange(CGXRange(nRow, _STARTCOL + 12), CGXStyle()
        .SetControl(GX_IDS_CTRL_PUSHBTN)
        .SetChoiceList(_T("Edit"))
        .SetHorizontalAlignment(DT_LEFT)
        .SetEnabled(TRUE)
    );

   // available number of bearings
   CString choicelist;
   for (int i=1; i<=MAX_BEARING_CNT; i++)
   {
      CString choice;
      choice.Format(_T("%d\n"), i);
      choicelist += choice;
   }

	this->SetStyleRange(CGXRange(nRow,_STARTCOL+4), CGXStyle()
			.SetControl(GX_IDS_CTRL_CBS_DROPDOWNLIST)
			.SetChoiceList(choicelist)
			.SetValue(_T("1"))
         .SetHorizontalAlignment(DT_RIGHT)
         );
}

void CBearingGdrGrid::FillGrid()
{
   GetParam()->EnableUndo(FALSE);
   GetParam()->SetLockReadOnly(FALSE);
 
   ROWCOL rows = GetRowCount();
   if (0 < rows)
	   RemoveRows(0, rows);

   // we want to merge cells
   SetMergeCellsMode(gxnMergeEvalOnDisplay);

   // Find back index for this grid's span
   PierIndexType backBlIdx = this->GetBackBearingIdx();
   if (backBlIdx == INVALID_INDEX)
   {
      return;
   }

   const BearingPierData& rBack = m_pBearingInputData->m_Bearings[backBlIdx];
   const BearingPierData& rAhead = m_pBearingInputData->m_Bearings[backBlIdx+1];

   // Better have same number of girders at both ends of span
   ATLASSERT(rBack.m_BearingsForGirders.size() == rAhead.m_BearingsForGirders.size());

   // Two rows for each girder
   GirderIndexType numGdrs = rBack.m_BearingsForGirders.size();
   ROWCOL numRows = (ROWCOL)numGdrs * 2;
   InsertRows(1, numRows);

   ROWCOL BackRow = 1;
   ROWCOL AheadRow = 2;

   for (GirderIndexType gdrIdx=0; gdrIdx<numGdrs; gdrIdx++  )
   {
      SetRowStyle(BackRow);
      SetRowStyle(AheadRow);

      ROWCOL col = _STARTCOL;

      // row index
      CString crow;
      crow.Format(_T("Girder %s"), LABEL_GIRDER(gdrIdx));

	   SetStyleRange(CGXRange(BackRow,col,AheadRow,col), CGXStyle()
            .SetWrapText(TRUE)
            .SetHorizontalAlignment(DT_CENTER)
            .SetVerticalAlignment(DT_TOP)
			   .SetEnabled(FALSE)          // disables usage as current cell
            .SetMergeCell(GX_MERGE_VERTICAL | GX_MERGE_COMPVALUE)
            .SetValue(crow));

      col++;

      SetStyleRange(CGXRange(BackRow,col), CGXStyle().SetValue(_T("Back")));
      SetStyleRange(CGXRange(AheadRow,col++), CGXStyle().SetValue(_T("Ahead")));

      WriteBearingRow(BackRow,  rBack.m_BearingsForGirders[gdrIdx]);
      WriteBearingRow(AheadRow, rAhead.m_BearingsForGirders[gdrIdx]);

      m_girderBearingDetailData.emplace_back(rBack.m_BearingsForGirders[gdrIdx]);
      m_girderBearingDetailData.emplace_back(rAhead.m_BearingsForGirders[gdrIdx]);


      BackRow += 2;
      AheadRow += 2;
   }

	ResizeRowHeightsToFit(CGXRange(0,0,numRows,_STARTNCOLS-1));

   GetParam()->SetLockReadOnly(TRUE);
	GetParam()->EnableUndo(TRUE);

	ScrollCellInView(1, GetLeftCol());
}

void CBearingGdrGrid::WriteBearingRow(ROWCOL row, const CBearingData2& bearingData)
{


   // We use slot (girder) zero for piers

    CString strType = (bearingData.DefinitionType == btBasic) ? _T("Basic") : _T("Detailed");

    auto pBroker = EAFGetBroker();

    GET_IFACE2(pBroker, ILibrary, pLib);
    GET_IFACE2(pBroker, ISpecification, pSpec);
    const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());
    const BearingCriteria& criteria = pSpecEntry->GetBearingCriteria();

    //if (criteria.bCheck)
    //{
        SetStyleRange(CGXRange(row, m_DGetter.m_BearingDefTypeCol), CGXStyle()
            .SetReadOnly(FALSE)
            .SetEnabled(TRUE)
            .SetValue(strType)
        );
    //}
    //else
    //{
    //    SetStyleRange(CGXRange(row, m_DGetter.m_BearingDefTypeCol), CGXStyle()
    //        .SetInterior(GXSYSCOLOR(COLOR_BTNFACE))
    //        .SetReadOnly(TRUE)
    //        .SetEnabled(FALSE)
    //        .SetValue(strType)
    //    );
    //}

   CString strshape = (bearingData.Shape == bsRectangular) ? _T("Rectangular") : _T("Round");

   SetStyleRange(CGXRange(row,m_DGetter.m_BearingShapeCol), CGXStyle()
      .SetReadOnly(FALSE)
      .SetEnabled(TRUE)
      .SetValue(strshape)
      );

   CString strcnt;
   strcnt.Format(_T("%d"), bearingData.BearingCount);

   SetStyleRange(CGXRange(row,m_DGetter.m_BearingCountCol), CGXStyle()
      .SetReadOnly(FALSE)
      .SetEnabled(TRUE)
      .SetValue(strcnt) 
      );

   SetStyleRange(CGXRange(row,m_DGetter.m_BearingSpacingCol), CGXStyle()
      .SetReadOnly(FALSE)
      .SetEnabled(TRUE)
      .SetHorizontalAlignment(DT_RIGHT)
      .SetVerticalAlignment(DT_TOP)
      .SetValue(FormatDimension(bearingData.Spacing,*m_pCompUnit, false))
      );

   SetStyleRange(CGXRange(row,m_DGetter.m_BearingLengthCol), CGXStyle()
      .SetReadOnly(FALSE)
      .SetEnabled(TRUE)
      .SetHorizontalAlignment(DT_RIGHT)
      .SetVerticalAlignment(DT_TOP)
      .SetValue(FormatDimension(bearingData.Length,*m_pCompUnit, false))
      );

   SetStyleRange(CGXRange(row,m_DGetter.m_BearingWidthCol), CGXStyle()
      .SetReadOnly(FALSE)
      .SetEnabled(TRUE)
      .SetHorizontalAlignment(DT_RIGHT)
      .SetVerticalAlignment(DT_TOP)
      .SetValue(FormatDimension(bearingData.Width,*m_pCompUnit, false))
      );

   if (bearingData.DefinitionType == btDetailed)
   {
       SetStyleRange(CGXRange(row, m_DGetter.m_BearingHeightCol), CGXStyle()
           .SetEnabled(FALSE)
           .SetHorizontalAlignment(DT_RIGHT)
           .SetVerticalAlignment(DT_TOP)
           .SetInterior(::GetSysColor(COLOR_BTNFACE))
           .SetTextColor(::GetSysColor(COLOR_GRAYTEXT))
           .SetValue(FormatDimension(bearingData.Height, *m_pCompUnit, false))
       );
   }
   else
   {
       SetStyleRange(CGXRange(row, m_DGetter.m_BearingHeightCol), CGXStyle()
           .SetReadOnly(FALSE)
           .SetEnabled(TRUE)
           .SetHorizontalAlignment(DT_RIGHT)
           .SetVerticalAlignment(DT_TOP)
           .SetValue(FormatDimension(bearingData.Height, *m_pCompUnit, false))
       );
   }

   SetStyleRange(CGXRange(row,m_DGetter.m_BearingRecessHeightCol), CGXStyle()
      .SetReadOnly(FALSE)
      .SetEnabled(TRUE)
      .SetHorizontalAlignment(DT_RIGHT)
      .SetVerticalAlignment(DT_TOP)
      .SetValue(FormatDimension(bearingData.RecessHeight,*m_pCompUnit, false))
      );

   SetStyleRange(CGXRange(row,m_DGetter.m_BearingRecessLengthCol), CGXStyle()
      .SetReadOnly(FALSE)
      .SetEnabled(TRUE)
      .SetHorizontalAlignment(DT_RIGHT)
      .SetVerticalAlignment(DT_TOP)
      .SetValue(FormatDimension(bearingData.RecessLength,*m_pCompUnit, false))
      );

   SetStyleRange(CGXRange(row,m_DGetter.m_BearingSolePlateCol), CGXStyle()
      .SetReadOnly(FALSE)
      .SetEnabled(TRUE)
      .SetHorizontalAlignment(DT_RIGHT)
      .SetVerticalAlignment(DT_TOP)
      .SetValue(FormatDimension(bearingData.SolePlateHeight,*m_pCompUnit, false))
      );

   // Disable columns if needed
   OnModifyCell(row, m_DGetter.m_BearingDefTypeCol);
   OnModifyCell(row, m_DGetter.m_BearingShapeCol);
   OnModifyCell(row, m_DGetter.m_BearingCountCol);
}

CString CBearingGdrGrid::GetCellValue(ROWCOL nRow, ROWCOL nCol)
{
    if (IsCurrentCell(nRow, nCol) && IsActiveCurrentCell())
    {
        CString s;
        CGXControl* pControl = GetControl(nRow, nCol);
        pControl->GetValue(s);
        return s;
    }
    else
    {
        return GetValueRowCol(nRow, nCol);
    }
}

PierIndexType CBearingGdrGrid::GetBackBearingIdx()
{
   // Find back index for this grid's span
   BearingPierDataConstIter BegBliter = m_pBearingInputData->m_Bearings.begin();
   BegBliter++; // jump past first bearing line
   SpanIndexType spanidx = 0;
   PierIndexType backBlIdx = 0;
   BearingPierDataConstIter bliter = BegBliter;
   for (; bliter != m_pBearingInputData->m_Bearings.end(); bliter++)
   {
      if (spanidx == m_SpanIdx)
         break;

      backBlIdx++;

      const BearingPierData& rdata = *bliter;
      if (rdata.m_BPDType == BearingPierData::bpdAhead || rdata.m_BPDType == BearingPierData::bpdCL)
      {
         spanidx++;
      }
   }

   if (bliter == m_pBearingInputData->m_Bearings.end())
   {
      AfxMessageBox(_T("Programming error. could not create grid"));
      return INVALID_INDEX;
   }

   return backBlIdx;
}

void CBearingGdrGrid::GetData(CDataExchange* pDX)
{
   // make a local copy of data and only overwrite if we are successful
   BearingInputData localBearingData(*m_pBearingInputData);

   // Find back index for this grid's span
   PierIndexType backBlIdx = this->GetBackBearingIdx();
   if (backBlIdx == INVALID_INDEX)
   {
      return;
   }

   BearingPierData& rBack = localBearingData.m_Bearings[backBlIdx];
   BearingPierData& rAhead = localBearingData.m_Bearings[backBlIdx+1];

   // clear out bearings for girders
   rBack.m_BearingsForGirders.clear();
   rAhead.m_BearingsForGirders.clear();

   ROWCOL nRows = this->GetRowCount();

   ROWCOL BackRow  = 1;
   ROWCOL AheadRow = 2;

   while (AheadRow <= nRows)
   {
      // use utility class to get bearing data
      CBearingData2 bbd = m_DGetter.GetBrgData(this, BackRow, m_pCompUnit, pDX);
      bbd.NumIntLayers = m_girderBearingDetailData[BackRow - 1].NumIntLayers;
      bbd.ElastomerThickness = m_girderBearingDetailData[BackRow - 1].ElastomerThickness;
      bbd.CoverThickness = m_girderBearingDetailData[BackRow - 1].CoverThickness;
      bbd.ShimThickness = m_girderBearingDetailData[BackRow - 1].ShimThickness;
      bbd.ShearDeformationOverride = m_girderBearingDetailData[BackRow - 1].ShearDeformationOverride;
      bbd.UseExtPlates = m_girderBearingDetailData[BackRow - 1].UseExtPlates;
      bbd.FixedX = m_girderBearingDetailData[BackRow - 1].FixedX;
      bbd.FixedY = m_girderBearingDetailData[BackRow - 1].FixedY;

      CBearingData2 abd = m_DGetter.GetBrgData(this, AheadRow, m_pCompUnit, pDX);
      abd.NumIntLayers = m_girderBearingDetailData[AheadRow - 1].NumIntLayers;
      abd.ElastomerThickness = m_girderBearingDetailData[AheadRow - 1].ElastomerThickness;
      abd.CoverThickness = m_girderBearingDetailData[AheadRow - 1].CoverThickness;
      abd.ShimThickness = m_girderBearingDetailData[AheadRow - 1].ShimThickness;
      abd.ShearDeformationOverride = m_girderBearingDetailData[AheadRow - 1].ShearDeformationOverride;
      abd.UseExtPlates = m_girderBearingDetailData[AheadRow - 1].UseExtPlates;
      abd.FixedX = m_girderBearingDetailData[AheadRow - 1].FixedX;
      abd.FixedY = m_girderBearingDetailData[AheadRow - 1].FixedY;

      // save bearing data
      rBack.m_BearingsForGirders.push_back(bbd);
      rAhead.m_BearingsForGirders.push_back(abd);

      BackRow += 2;
      AheadRow += 2;
   }

   // save data
   *m_pBearingInputData = localBearingData;
   m_pBearingInputData->m_BearingType = pgsTypes::brtGirder;
}

void CBearingGdrGrid::OnModifyCell(ROWCOL nRow,ROWCOL nCol)
{
   GetParam()->EnableUndo(FALSE);
   GetParam()->SetLockReadOnly(FALSE);

   if ( nCol == m_DGetter.m_BearingCountCol )
   {
      CString strSel = GetCellValue(nRow,nCol);
      if (strSel == _T("1"))
      {
         // have one bearing - don't need to display spacing
         SetStyleRange(CGXRange(nRow, m_DGetter.m_BearingSpacingCol), CGXStyle()
            .SetValue(_T("")) // erase the current value
            .SetEnabled(FALSE)
            .SetReadOnly(TRUE)
            .SetInterior(::GetSysColor(COLOR_BTNFACE))
            .SetTextColor(::GetSysColor(COLOR_GRAYTEXT))
         );
      }
      else
      {
         SetStyleRange(CGXRange(nRow,m_DGetter.m_BearingSpacingCol),CGXStyle()
            .SetEnabled(TRUE)
            .SetReadOnly(FALSE)
            .SetInterior(::GetSysColor(COLOR_WINDOW))
            .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT))
            );
      }
   }
   else if ( nCol == m_DGetter.m_BearingShapeCol )
   {
      CString strShp = GetCellValue(nRow,nCol);
      if (strShp == _T("Round"))
      {
         // round bearing - don't need to display width
       SetStyleRange(CGXRange(nRow, m_DGetter.m_BearingWidthCol), CGXStyle()
            .SetValue(_T("")) // erase the current value
            .SetEnabled(FALSE)
            .SetReadOnly(TRUE)
            .SetInterior(::GetSysColor(COLOR_BTNFACE))
            .SetTextColor(::GetSysColor(COLOR_GRAYTEXT))
         );
      }
      else
      {
         SetStyleRange(CGXRange(nRow,m_DGetter.m_BearingWidthCol),CGXStyle()
            .SetEnabled(TRUE)
            .SetReadOnly(FALSE)
            .SetInterior(::GetSysColor(COLOR_WINDOW))
            .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT))
            );
      }
   }

   else if (nCol == m_DGetter.m_BearingDefTypeCol)
   {
       CString strDef = GetCellValue(nRow, nCol);
       if (strDef == _T("Detailed"))
       {
           SetStyleRange(CGXRange(nRow, m_DGetter.m_BearingDetailButtonCol), CGXStyle()
               .SetEnabled(TRUE)
               .SetReadOnly(FALSE)
               .SetInterior(::GetSysColor(COLOR_WINDOW))
               .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT))
           );
       }
       else
       {
           SetStyleRange(CGXRange(nRow, m_DGetter.m_BearingDetailButtonCol), CGXStyle()
               .SetEnabled(FALSE)
               .SetReadOnly(TRUE)
               .SetInterior(::GetSysColor(COLOR_BTNFACE))
               .SetTextColor(::GetSysColor(COLOR_GRAYTEXT))
           );

           SetStyleRange(CGXRange(nRow, m_DGetter.m_BearingHeightCol), CGXStyle()
               .SetEnabled(TRUE)
               .SetReadOnly(FALSE)
               .SetInterior(::GetSysColor(COLOR_WINDOW))
               .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT))
           );
       }

   }

   GetParam()->SetLockReadOnly(TRUE);
   GetParam()->EnableUndo(TRUE);
}

BOOL CBearingGdrGrid::OnValidateCell(ROWCOL nRow, ROWCOL nCol)
{
   if ( nCol == m_DGetter.m_BearingSpacingCol )
   {
      CString bcnt =  GetCellValue(nRow,m_DGetter.m_BearingCountCol);
      if (bcnt != _T("1")) // no need to check spacing if only one bearing
      {
         CString strSpacing = GetCellValue(nRow, nCol);
         if (strSpacing.IsEmpty())
         {
            SetWarningText(_T("Bearing spacing must not be blank"));
            return false;
         }
         else
         {
            Float64 spacing = _tstof(strSpacing);
            if (IsLE(spacing, 0.0))
            {
               SetWarningText(_T("Bearing spacing must be zero or greater"));
               return false;
            }
         }
      }
   }
   else if ( nCol == m_DGetter.m_BearingLengthCol )
   {
      CString strVal = GetCellValue(nRow, nCol);
      if (strVal.IsEmpty())
      {
         SetWarningText(_T("Bearing length must not be blank"));
         return false;
      }
      else
      {
         Float64 val = _tstof(strVal);
         if (IsLT(val, 0.0))
         {
            SetWarningText(_T("Bearing length must be zero or greater"));
            return false;
         }
      }
   }
   else if ( nCol == m_DGetter.m_BearingWidthCol )
   {
      CString strShape = GetCellValue(nRow, m_DGetter.m_BearingShapeCol);
      if (strShape != _T("Round"))
      {
         CString strVal = GetCellValue(nRow, nCol);
         if (strVal.IsEmpty())
         {
            SetWarningText(_T("Bearing width must not be blank"));
            return false;
         }
         else
         {
            Float64 val = _tstof(strVal);
            if (IsLT(val, 0.0))
            {
               SetWarningText(_T("Bearing width must be zero or greater"));
               return false;
            }
         }
      }
   }
   else if ( nCol == m_DGetter.m_BearingHeightCol )
   {
      CString strVal = GetCellValue(nRow, nCol);
      if (strVal.IsEmpty())
      {
         SetWarningText(_T("Bearing Height must not be blank"));
         return false;
      }
      else
      {
         Float64 val = _tstof(strVal);
         if (IsLT(val, 0.0))
         {
            SetWarningText(_T("Bearing Height must be zero or greater"));
            return false;
         }
      }
   }
   else if ( nCol == m_DGetter.m_BearingRecessHeightCol )
   {
      CString strVal = GetCellValue(nRow, nCol);
      if (strVal.IsEmpty())
      {
         SetWarningText(_T("Bearing Recess Height must not be blank"));
         return false;
      }
      else
      {
         Float64 val = _tstof(strVal);
         if (IsLT(val, 0.0))
         {
            SetWarningText(_T("Bearing Recess Height must be zero or greater"));
            return false;
         }
      }
   }
   else if ( nCol == m_DGetter.m_BearingRecessLengthCol )
   {
      CString strVal = GetCellValue(nRow, nCol);
      if (strVal.IsEmpty())
      {
         SetWarningText(_T("Bearing Recess Length must not be blank"));
         return false;
      }
      else
      {
         Float64 val = _tstof(strVal);
         if (IsLT(val, 0.0))
         {
            SetWarningText(_T("Bearing Recess Length must be zero or greater"));
            return false;
         }
      }
   }
   else if ( nCol == m_DGetter.m_BearingSolePlateCol )
   {
      CString strVal = GetCellValue(nRow, nCol);
      if (strVal.IsEmpty())
      {
         SetWarningText(_T("Bearing Sole Plate thickness must not be blank"));
         return false;
      }
      else
      {
         Float64 val = _tstof(strVal);
         if (IsLT(val, 0.0))
         {
            SetWarningText(_T("Bearing Sole Plate thickness must be zero or greater"));
            return false;
         }
      }
   }

	return CGXGridWnd::OnValidateCell(nRow, nCol);
}

LRESULT CBearingGdrGrid::ChangeTabName( WPARAM wParam, LPARAM lParam ) 
{
   // don't allow userss to change tab names
   return FALSE;
}

void CBearingGdrGrid::OnClickedButtonRowCol(ROWCOL nRow, ROWCOL nCol)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    CDataExchange dx(this, TRUE);
    DoDataExchange(&dx);

    CBearingData2 bd = m_DGetter.GetBrgData(this, nRow, m_pCompUnit, &dx);

    if (nCol == 13)
    {

        bd.ElastomerThickness = m_girderBearingDetailData[nRow - 1].ElastomerThickness;
        bd.ElastomerThickness = m_girderBearingDetailData[nRow - 1].ElastomerThickness;
        bd.CoverThickness = m_girderBearingDetailData[nRow - 1].CoverThickness;
        bd.CoverThickness = m_girderBearingDetailData[nRow - 1].CoverThickness;
        bd.ShimThickness = m_girderBearingDetailData[nRow - 1].ShimThickness;
        bd.ShimThickness = m_girderBearingDetailData[nRow - 1].ShimThickness;
        bd.NumIntLayers = m_girderBearingDetailData[nRow - 1].NumIntLayers;
        bd.NumIntLayers = m_girderBearingDetailData[nRow - 1].NumIntLayers;
        bd.FixedX = m_girderBearingDetailData[nRow - 1].FixedX;
        bd.FixedX = m_girderBearingDetailData[nRow - 1].FixedX;
        bd.FixedY = m_girderBearingDetailData[nRow - 1].FixedY;
        bd.FixedY = m_girderBearingDetailData[nRow - 1].FixedY;
        bd.UseExtPlates = m_girderBearingDetailData[nRow - 1].UseExtPlates;
        bd.UseExtPlates = m_girderBearingDetailData[nRow - 1].UseExtPlates;
        bd.ShearDeformationOverride = m_girderBearingDetailData[nRow - 1].ShearDeformationOverride;
        bd.ShearDeformationOverride = m_girderBearingDetailData[nRow - 1].ShearDeformationOverride;

        m_details_dlg.SetBearingDetailDlg(bd);

        

        //bool bRepeatDialog = true;

        //while (bRepeatDialog)
        //{
            if (m_details_dlg.DoModal() == IDOK)
            {
                const auto& computed_height = m_details_dlg.GetComputedHeight();

                const auto& brg_details = m_details_dlg.GetBearingDetails();

                //if (!IsEqual(computed_height, bd.Height))
                //{
                //    CString msg;
                //    msg.Format(_T("The computed height and the initial input are different. Do you want to override the input?"));

                //    if (AfxMessageBox(msg, MB_YESNO | MB_ICONWARNING) == IDYES)
                //    {

                        SetStyleRange(CGXRange(nRow, m_DGetter.m_BearingHeightCol), CGXStyle()
                            .SetEnabled(FALSE)
                            .SetHorizontalAlignment(DT_RIGHT)
                            .SetVerticalAlignment(DT_TOP)
                            .SetInterior(::GetSysColor(COLOR_BTNFACE))
                            .SetTextColor(::GetSysColor(COLOR_GRAYTEXT))
                            .SetValue(FormatDimension(computed_height, *m_pCompUnit, false))
                        );

                //        bRepeatDialog = false; // Exit loop
                //    }
                //    else
                //    {
                //        // Loop will repeat
                //    }

                //}
                //else
                //{
                    bd.Height = computed_height;
                    //bRepeatDialog = false;
                //}

                SetStyleRange(CGXRange(nRow, m_DGetter.m_BearingLengthCol), CGXStyle()
                    .SetReadOnly(FALSE)
                    .SetEnabled(TRUE)
                    .SetHorizontalAlignment(DT_RIGHT)
                    .SetVerticalAlignment(DT_TOP)
                    .SetValue(FormatDimension(brg_details.Length, *m_pCompUnit, false))
                );

                SetStyleRange(CGXRange(nRow, m_DGetter.m_BearingWidthCol), CGXStyle()
                    .SetReadOnly(FALSE)
                    .SetEnabled(TRUE)
                    .SetHorizontalAlignment(DT_RIGHT)
                    .SetVerticalAlignment(DT_TOP)
                    .SetInterior(::GetSysColor(COLOR_WINDOW))
                    .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT))
                    .SetValue(FormatDimension(brg_details.Width, *m_pCompUnit, false))
                );

                // Find back index for this grid's span
                PierIndexType backBlIdx = this->GetBackBearingIdx();
                if (backBlIdx == INVALID_INDEX)
                {
                    return;
                }


                
                
                GirderIndexType gdrIdx = (int)((nRow-1) / 2 + 1);
                if ((nRow-1) % 2 == 0)
                {
                    gdrIdx = (nRow-1) / 2;
                }
                m_pBearingInputData->m_Bearings[backBlIdx].m_BearingsForGirders[gdrIdx].NumIntLayers = m_girderBearingDetailData[nRow - 1].NumIntLayers = brg_details.NumIntLayers;
                m_pBearingInputData->m_Bearings[backBlIdx].m_BearingsForGirders[gdrIdx].ElastomerThickness = m_girderBearingDetailData[nRow - 1].ElastomerThickness = brg_details.ElastomerThickness;
                m_pBearingInputData->m_Bearings[backBlIdx].m_BearingsForGirders[gdrIdx].CoverThickness = m_girderBearingDetailData[nRow - 1].CoverThickness = brg_details.CoverThickness;
                m_pBearingInputData->m_Bearings[backBlIdx].m_BearingsForGirders[gdrIdx].ShimThickness = m_girderBearingDetailData[nRow - 1].ShimThickness = brg_details.ShimThickness;
                m_pBearingInputData->m_Bearings[backBlIdx].m_BearingsForGirders[gdrIdx].UseExtPlates = m_girderBearingDetailData[nRow - 1].UseExtPlates = brg_details.UseExtPlates;
                m_pBearingInputData->m_Bearings[backBlIdx].m_BearingsForGirders[gdrIdx].FixedX = m_girderBearingDetailData[nRow - 1].FixedX = brg_details.FixedX;
                m_pBearingInputData->m_Bearings[backBlIdx].m_BearingsForGirders[gdrIdx].FixedY = m_girderBearingDetailData[nRow - 1].FixedY = brg_details.FixedY;
                m_pBearingInputData->m_Bearings[backBlIdx].m_BearingsForGirders[gdrIdx].ShearDeformationOverride = m_girderBearingDetailData[nRow - 1].ShearDeformationOverride = brg_details.ShearDeformationOverride;

            }
            //else
            //{
            //    //bRepeatDialog = false; // Dialog cancelled
            //}
        //}
    }

}



BOOL CBearingGdrGrid::OnEndEditing(ROWCOL nRow, ROWCOL nCol)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    CDataExchange dx(this, TRUE);
    DoDataExchange(&dx);

    //CBearingData2 bd = m_DGetter.GetBrgData(this, nRow, m_pCompUnit, &dx);

    // make a local copy of data and only overwrite if we are successful
    BearingInputData localBearingData(*m_pBearingInputData);

    //// Find back index for this grid's span
    PierIndexType backBlIdx = this->GetBackBearingIdx();
    if (backBlIdx == INVALID_INDEX)
    {
        return TRUE;
    }

    BearingPierData& rBack = localBearingData.m_Bearings[backBlIdx];
    BearingPierData& rAhead = localBearingData.m_Bearings[backBlIdx + 1];

    if (rBack.m_BPDType == BearingPierData::bpdCL)
    {
        ROWCOL nRows = this->GetRowCount();

        ROWCOL BackRow = 1;

        while (BackRow <= nRows)
        {

            if (nRow == BackRow)
            {
                CWnd* pWnd;
                pWnd = GetParent();
                CBearingGdrByGdrDlg* dlg;

                while (pWnd)
                {
                    dlg = dynamic_cast<CBearingGdrByGdrDlg*>(pWnd);
                    if (dlg)
                        break;
                    pWnd = pWnd->GetParent();
                }

                for (size_t i = 0; i < dlg->m_GirderGrids.size(); ++i)
                {
                    if (dlg->m_GirderGrids[i].get() == this)
                    {
                        if (i - 1 >= 0)
                        {
                            auto prevGrid = dlg->m_GirderGrids[i - 1];

                            prevGrid->SetStyleRange(CGXRange(nRow + 1, nCol), CGXStyle()
                                .SetValue(GetCellValue(nRow, nCol))
                            );

                        }
                        break;
                    }
                }

            }

            BackRow += 2;
        }

    }


    if (rAhead.m_BPDType == BearingPierData::bpdCL)
    {
        ROWCOL nRows = this->GetRowCount();

        ROWCOL AheadRow = 2;

        while (AheadRow <= nRows)
        {
            if (nRow == AheadRow)
            {
                CWnd* pWnd;
                pWnd = GetParent();
                CBearingGdrByGdrDlg* dlg;

                while (pWnd)
                {
                    dlg = dynamic_cast<CBearingGdrByGdrDlg*>(pWnd);
                    if (dlg)
                        break;
                    pWnd = pWnd->GetParent();
                }

                for (size_t i = 0; i < dlg->m_GirderGrids.size(); ++i)
                {
                    if (dlg->m_GirderGrids[i].get() == this)
                    {
                        if (i + 1 < dlg->m_GirderGrids.size())
                        {
                            auto nextGrid = dlg->m_GirderGrids[i + 1];

                            nextGrid->SetStyleRange(CGXRange(nRow - 1, nCol), CGXStyle()
                                .SetValue(GetCellValue(nRow, nCol))
                            );

                        }
                        break;
                    }
                }

            }

            AheadRow += 2;
        }
    }

    return TRUE;

}