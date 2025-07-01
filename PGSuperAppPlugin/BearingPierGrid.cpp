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

// BearingPierGrid.cpp : implementation file
//

#include "stdafx.h"
#include "BearingPierGrid.h"

#include <System\Tokenizer.h>
#include "PGSuperUnits.h"
#include <Units\Measure.h>

#include <IFace/Tools.h>
#include <IFace/Project.h>

#include <EAF\EAFDisplayUnits.h>
#include <psgLib/SpecLibraryEntry.h>
#include <psgLib/BearingCriteria.h>



/////////////////////////////////////////////////////////////////////////////
// BearingInputData
// Functions to copy bearing input data from dialog into the bridge
void BearingInputData::CopyToBridgeDescription(CBridgeDescription2* pBridgeDesc) const
{
   if (m_BearingType == pgsTypes::brtBridge)
   {
      pBridgeDesc->SetBearingType(pgsTypes::brtBridge);
      pBridgeDesc->SetBearingData(m_SingleBearing);
   }
   else if (m_BearingType == pgsTypes::brtPier)
   {
      pBridgeDesc->SetBearingType(pgsTypes::brtPier);

      // loop over each bearing line and set bearing data
      for (BearingPierDataConstIter dit = m_Bearings.begin(); dit != m_Bearings.end(); dit++)
      {
         const BearingPierData& bpd = *dit;

         CPierData2* pPier = pBridgeDesc->GetPier(bpd.m_PierIndex);

         // use  slot[0] for all girders at pier
         CBearingData2 bd = bpd.m_BearingsForGirders[0];

         // Tricky: split bearing length when continuous segments cross over a pier
         if (BearingPierData::bpdAhead == bpd.m_BPDType)
         {
            pPier->SetBearingData(pgsTypes::Ahead, bd);
         }
         else if (BearingPierData::bpdBack == bpd.m_BPDType)
         {
            pPier->SetBearingData(pgsTypes::Back, bd);
         }
         else
         {
            ATLASSERT(BearingPierData::bpdCL == bpd.m_BPDType);

            pPier->SetBearingData(pgsTypes::Ahead, bd);
            pPier->SetBearingData(pgsTypes::Back, bd);
         }
      }
   }
   else if (m_BearingType == pgsTypes::brtGirder)
   {
      pBridgeDesc->SetBearingType(pgsTypes::brtGirder);

      // loop over each bearing line / girder and set A
      // loop over each bearing line and set bearing data
      for (BearingPierDataConstIter dit = m_Bearings.begin(); dit != m_Bearings.end(); dit++)
      {
         const BearingPierData& bpd = *dit;
         CPierData2* pPier = pBridgeDesc->GetPier(bpd.m_PierIndex);

         GirderIndexType ng = bpd.m_BearingsForGirders.size();
         for (GirderIndexType ig = 0; ig < ng; ig++)
         {
            CBearingData2 bd = bpd.m_BearingsForGirders[ig];

            // Tricky: split bearing length when continuous segments cross over a pier
            if (BearingPierData::bpdAhead == bpd.m_BPDType)
            {
               pPier->SetBearingData(ig, pgsTypes::Ahead, bd);
            }
            else if (BearingPierData::bpdBack == bpd.m_BPDType)
            {
               pPier->SetBearingData(ig, pgsTypes::Back, bd);
            }
            else
            {
               ATLASSERT(BearingPierData::bpdCL == bpd.m_BPDType);

               pPier->SetBearingData(ig, pgsTypes::Ahead, bd);
               pPier->SetBearingData(ig, pgsTypes::Back,  bd);
            }
         }
      }
   }
}

void BearingInputData::CopyFromBridgeDescription(const CBridgeDescription2* pBridgeDescr)
{
   // Take data from project and fill our local data structures
   // Fill all slab offset types with values depending on initial type
   m_Bearings.clear();
   m_MaxGirdersPerSpan = 0;

   // Bridge-wide data
   ///////////////////////////////////
   m_BearingType = pBridgeDescr->GetBearingType();

   m_SingleBearing = *(pBridgeDescr->GetBearingData());

   // Pier and girder based A data are treated the same for all types
   PierIndexType npiers = pBridgeDescr->GetPierCount();

   bool bFirst(true);
   for (PierIndexType ipier=0; ipier<npiers; ipier++)
   {
      const CPierData2* pPier = pBridgeDescr->GetPier(ipier);

      // We want to iterate over bearing lines. Determine how many
      std::array<pgsTypes::PierFaceType, 2> pierFaces;
      PierIndexType nbrglines = 1; 
      if (ipier==0)
      {
         ATLASSERT(pPier->IsAbutment());
         nbrglines = 1;
         pierFaces[0] = pgsTypes::Ahead;
      }
      else if (ipier==npiers-1)
      {
         ATLASSERT(pPier->IsAbutment());
         nbrglines = 1;
         pierFaces[0] = pgsTypes::Back;
      }
      else if (pPier->IsBoundaryPier())
      {
         nbrglines = 2;
         pierFaces[0] = pgsTypes::Back;
         pierFaces[1] = pgsTypes::Ahead;
      }
      else
      {
         ATLASSERT(pPier->IsInteriorPier());
         nbrglines = 1;
         pierFaces[0] = pgsTypes::Back;
      }

      for (PierIndexType ibrg=0; ibrg<nbrglines; ibrg++)
      {
         const CGirderGroupData* pGroup = pPier->GetGirderGroup(pierFaces[ibrg]);

         BearingPierData brgData;

         // Save pier and group data to make updating bridge description easier
         brgData.m_PierIndex   = ipier;
         brgData.m_pGroupIndex = pGroup->GetIndex();

         if (!pPier->IsAbutment() && nbrglines==1)
         {
            // continuous interior pier
            brgData.m_BPDType = BearingPierData::bpdCL;
         }
         else
         {
            brgData.m_BPDType = pgsTypes::Back==pierFaces[ibrg] ? BearingPierData::bpdBack :  BearingPierData::bpdAhead;
         }

         GirderIndexType ng = pGroup->GetGirderCount();
         m_MaxGirdersPerSpan = max(m_MaxGirdersPerSpan, ng);

         for (GirderIndexType ig=0; ig<ng; ig++)
         {
            const CBearingData2* pBr = pPier->GetBearingData(ig, pierFaces[ibrg]);
            brgData.m_BearingsForGirders.push_back(*pBr);
         }

         m_Bearings.push_back( brgData );
      }
   }
}

const ROWCOL _STARTCOL = 1;
const ROWCOL _STARTNCOLS = 13;

GRID_IMPLEMENT_REGISTER(CBearingPierGrid, CS_DBLCLKS, 0, 0, 0);

/////////////////////////////////////////////////////////////////////////////
// CBearingPierGrid

CBearingPierGrid::CBearingPierGrid()
{
//   RegisterClass();
}

CBearingPierGrid::~CBearingPierGrid()
{
}

BEGIN_MESSAGE_MAP(CBearingPierGrid, CGXGridWnd)
	//{{AFX_MSG_MAP(CBearingPierGrid)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CBearingPierGrid message handlers

int CBearingPierGrid::GetColWidth(ROWCOL nCol)
{
   if ( IsColHidden(nCol) )
      return CGXGridWnd::GetColWidth(nCol);

   ROWCOL nspc = _STARTNCOLS + 3;

	CRect rect = GetGridRect( );

   switch (nCol)
   {
   case 0:
   case 1:
   case 2:
   case 3:
   case 4:
      return (int)(rect.Width( )*(Float64)2/(nspc + 2));
   default:
      return (int)(rect.Width( )/nspc);
   }
}

void CBearingPierGrid::CustomInit()
{
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

    col += 2;
    m_DGetter.m_BearingDefTypeCol = col;
    SetStyleRange(CGXRange(0, col), CGXStyle()
        .SetWrapText(TRUE)
        .SetHorizontalAlignment(DT_CENTER)
        .SetVerticalAlignment(DT_TOP)
        .SetEnabled(FALSE)          // disables usage as current cell
        .SetValue(_T("\nType\n "))
    );

    col ++;
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
      .SetValue(_T("# of\nBearings\nper Girder"))
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
	this->ResizeRowHeightsToFit(CGXRange(0,0,0,GetColCount()));

   // Hide the row header column
   HideCols(0, 0);

	EnableIntelliMouse();
	SetFocus();

	GetParam( )->EnableUndo(TRUE);
}

void CBearingPierGrid::SetRowStyle(ROWCOL nRow)
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
	this->SetStyleRange(CGXRange(nRow,_STARTCOL+3), CGXStyle()
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

void CBearingPierGrid::FillGrid(const BearingInputData& BearingData)
{
   GetParam()->EnableUndo(FALSE);
   GetParam()->SetLockReadOnly(FALSE);
 
   ROWCOL rows = GetRowCount();
   if (0 < rows)
	   RemoveRows(0, rows);

   // we want to merge cells
   SetMergeCellsMode(gxnMergeEvalOnDisplay);

   // One row for each bearing line
   ROWCOL numRows = (ROWCOL)BearingData.m_Bearings.size();

   ROWCOL row = 1;
   InsertRows(row, numRows);

   PierIndexType PierNo=1;
   BearingPierDataConstIter iter = BearingData.m_Bearings.begin();
   for (; row<=numRows; row++  )
   {
      const BearingPierData& hp = *iter;

      m_BearingPierDetailData.emplace_back(hp.m_BearingsForGirders[0]);

      ATLASSERT(PierNo - 1 == hp.m_PierIndex);

      SetRowStyle(row);

      ROWCOL col = _STARTCOL;

      // row label
      bool isAbut = (row == 1 || row == numRows);
      CString crow = pgsPierLabel::GetPierLabelEx(isAbut, hp.m_PierIndex).c_str();

      if (row == 1 || row == numRows || hp.m_BPDType==BearingPierData::bpdCL) // single row title
      {
         SetStyleRange(CGXRange(row, col), CGXStyle()
            .SetValue(crow));
      }
      else if (hp.m_BPDType==BearingPierData::bpdBack)
      {
         SetStyleRange(CGXRange(row, col, row+1, col), CGXStyle() // title takes two rows
            .SetWrapText(TRUE)
            .SetHorizontalAlignment(DT_CENTER)
            .SetVerticalAlignment(DT_TOP)
            .SetMergeCell(GX_MERGE_VERTICAL | GX_MERGE_COMPVALUE)
            .SetValue(crow));
      }

      col++;

      pgsTypes::PierFaceType face;
      if (hp.m_BPDType == BearingPierData::bpdAhead)
      {
         face = pgsTypes::Ahead;
         crow = _T("Ahead");
      }
      else if (hp.m_BPDType == BearingPierData::bpdBack)
      {
         face = pgsTypes::Back;
         crow = _T("Back");
      }
      else if (hp.m_BPDType == BearingPierData::bpdCL)
      {
         face = pgsTypes::Back;
         crow = _T("C.L.");
      }
      SetStyleRange(CGXRange(row,col++), CGXStyle()
         .SetValue(crow));

      CString strType = (hp.m_BearingsForGirders[0].DefinitionType == btBasic) ? _T("Basic") : _T("Detailed");

      auto pBroker = EAFGetBroker();

      GET_IFACE2(pBroker, ILibrary, pLib);
      GET_IFACE2(pBroker, ISpecification, pSpec);
      const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());
      const BearingCriteria& criteria = pSpecEntry->GetBearingCriteria();

      if (criteria.bCheck)
      {
          SetStyleRange(CGXRange(row, col++), CGXStyle()
              .SetReadOnly(FALSE)
              .SetEnabled(TRUE)
              .SetValue(strType)
          );
      }
      else
      {
          SetStyleRange(CGXRange(row, col++), CGXStyle()
              .SetInterior(GXSYSCOLOR(COLOR_BTNFACE))
              .SetReadOnly(TRUE)
              .SetEnabled(FALSE)
              .SetValue(strType)
          );
      }

      // We use slot (girder) zero for piers
      CString strshape = (hp.m_BearingsForGirders[0].Shape == bsRectangular) ? _T("Rectangular") : _T("Round");

      SetStyleRange(CGXRange(row,col++), CGXStyle()
         .SetReadOnly(FALSE)
         .SetEnabled(TRUE)
         .SetValue(strshape)
         );

      CString strcnt;
      strcnt.Format(_T("%d"), hp.m_BearingsForGirders[0].BearingCount);

      SetStyleRange(CGXRange(row,col++), CGXStyle()
         .SetReadOnly(FALSE)
         .SetEnabled(TRUE)
         .SetValue(strcnt) 
         );

      SetStyleRange(CGXRange(row,col++), CGXStyle()
         .SetReadOnly(FALSE)
         .SetEnabled(TRUE)
         .SetHorizontalAlignment(DT_RIGHT)
         .SetVerticalAlignment(DT_TOP)
         .SetValue(FormatDimension(hp.m_BearingsForGirders[0].Spacing,*m_pCompUnit, false))
         );

      SetStyleRange(CGXRange(row,col++), CGXStyle()
         .SetReadOnly(FALSE)
         .SetEnabled(TRUE)
         .SetHorizontalAlignment(DT_RIGHT)
         .SetVerticalAlignment(DT_TOP)
         .SetValue(FormatDimension(hp.m_BearingsForGirders[0].Length,*m_pCompUnit, false))
         );

      SetStyleRange(CGXRange(row,col++), CGXStyle()
         .SetReadOnly(FALSE)
         .SetEnabled(TRUE)
         .SetHorizontalAlignment(DT_RIGHT)
         .SetVerticalAlignment(DT_TOP)
         .SetValue(FormatDimension(hp.m_BearingsForGirders[0].Width,*m_pCompUnit, false))
         );

      if (hp.m_BearingsForGirders[0].DefinitionType == btDetailed)
      {
          SetStyleRange(CGXRange(row, col++), CGXStyle()
           //   .SetReadOnly(TRUE)
              .SetEnabled(FALSE)
              .SetHorizontalAlignment(DT_RIGHT)
              .SetVerticalAlignment(DT_TOP)
              .SetInterior(::GetSysColor(COLOR_BTNFACE))
              .SetTextColor(::GetSysColor(COLOR_GRAYTEXT))
              .SetValue(FormatDimension(hp.m_BearingsForGirders[0].Height, *m_pCompUnit, false))
          );
      }
      else
      {
          SetStyleRange(CGXRange(row, col++), CGXStyle()
              .SetReadOnly(FALSE)
              .SetEnabled(TRUE)
              .SetHorizontalAlignment(DT_RIGHT)
              .SetVerticalAlignment(DT_TOP)
              .SetInterior(::GetSysColor(COLOR_WINDOW))
              .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT))
              .SetValue(FormatDimension(hp.m_BearingsForGirders[0].Height, *m_pCompUnit, false))
          );
      }

      SetStyleRange(CGXRange(row,col++), CGXStyle()
         .SetReadOnly(FALSE)
         .SetEnabled(TRUE)
         .SetHorizontalAlignment(DT_RIGHT)
         .SetVerticalAlignment(DT_TOP)
         .SetValue(FormatDimension(hp.m_BearingsForGirders[0].RecessHeight,*m_pCompUnit, false))
         );

      SetStyleRange(CGXRange(row,col++), CGXStyle()
         .SetReadOnly(FALSE)
         .SetEnabled(TRUE)
         .SetHorizontalAlignment(DT_RIGHT)
         .SetVerticalAlignment(DT_TOP)
         .SetValue(FormatDimension(hp.m_BearingsForGirders[0].RecessLength,*m_pCompUnit, false))
         );

      SetStyleRange(CGXRange(row,col++), CGXStyle()
         .SetReadOnly(FALSE)
         .SetEnabled(TRUE)
         .SetHorizontalAlignment(DT_RIGHT)
         .SetVerticalAlignment(DT_TOP)
         .SetValue(FormatDimension(hp.m_BearingsForGirders[0].SolePlateHeight,*m_pCompUnit, false))
         );

      // Disable columns if needed
      OnModifyCell(row, m_DGetter.m_BearingDefTypeCol);
      OnModifyCell(row, m_DGetter.m_BearingShapeCol);
      OnModifyCell(row, m_DGetter.m_BearingCountCol);

      if (hp.m_BPDType!=BearingPierData::bpdBack)
         PierNo++;

      iter++;
   }

	ResizeRowHeightsToFit(CGXRange(0,0,numRows,_STARTNCOLS-1));

   GetParam()->SetLockReadOnly(TRUE);
	GetParam()->EnableUndo(TRUE);

	ScrollCellInView(1, GetLeftCol());
}

CString CBearingPierGrid::GetCellValue(ROWCOL nRow, ROWCOL nCol)
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

void CBearingPierGrid::GetData(BearingInputData* pData, CDataExchange* pDX)
{
   // make a local copy of data and only overwrite if we are successful
   BearingPierDataVec localBearings(pData->m_Bearings);
   BearingPierDataIter brgIt = localBearings.begin();

   ROWCOL nRows = this->GetRowCount();
   ATLASSERT(nRows == pData->m_Bearings.size());
   for (ROWCOL row = 1; row <= nRows; row++)
   {
      BearingPierData& hp = *brgIt;

      // use utility class
      CBearingData2 bd = m_DGetter.GetBrgData(this, row, m_pCompUnit, pDX);

      bd.ElastomerThickness = m_BearingPierDetailData[row-1].ElastomerThickness;
      bd.CoverThickness = m_BearingPierDetailData[row-1].CoverThickness;
      bd.ShimThickness = m_BearingPierDetailData[row-1].ShimThickness;
      bd.NumIntLayers = m_BearingPierDetailData[row-1].NumIntLayers;
      bd.UseExtPlates = m_BearingPierDetailData[row-1].UseExtPlates;
      bd.FixedX = m_BearingPierDetailData[row-1].FixedX;
      bd.FixedY = m_BearingPierDetailData[row-1].FixedY;
      bd.FixedY = m_BearingPierDetailData[row-1].FixedY;
      bd.ShearDeformationOverride = m_BearingPierDetailData[row-1].ShearDeformationOverride;

      if (hp.m_BearingsForGirders.empty())
      {
         ATLASSERT(0); // should not happen
         hp.m_BearingsForGirders.push_back(bd);
      }
      else
      {
         hp.m_BearingsForGirders.assign(hp.m_BearingsForGirders.size(), bd);
      }

      brgIt++;
   }

   // save data
   pData->m_BearingType = pgsTypes::brtPier;
   pData->m_Bearings = localBearings;
}


void CBearingPierGrid::OnModifyCell(ROWCOL nRow,ROWCOL nCol)
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

BOOL CBearingPierGrid::OnValidateCell(ROWCOL nRow, ROWCOL nCol)
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
               SetWarningText(_T("Bearing spacing must be greater than zero"));
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
            SetWarningText(_T("Bearing length must be greater than zero"));
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
               SetWarningText(_T("Bearing width must be greater than zero"));
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
            SetWarningText(_T("Bearing Recess Heightmust be zero or greater"));
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
            SetWarningText(_T("Bearing Recess Lengthmust be zero or greater"));
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

void CBearingPierGrid::OnClickedButtonRowCol(ROWCOL nRow, ROWCOL nCol)
{

    if (nCol == 13)
    {
        AFX_MANAGE_STATE(AfxGetStaticModuleState());

        CDataExchange dx(this, TRUE);
        DoDataExchange(&dx);

        CBearingData2 bd = m_DGetter.GetBrgData(this, nRow, m_pCompUnit, &dx);

        bd.ElastomerThickness = m_BearingPierDetailData[nRow - 1].ElastomerThickness;
        bd.ShimThickness = m_BearingPierDetailData[nRow - 1].ShimThickness;
        bd.CoverThickness = m_BearingPierDetailData[nRow - 1].CoverThickness;
        bd.NumIntLayers = m_BearingPierDetailData[nRow - 1].NumIntLayers;
        bd.FixedX = m_BearingPierDetailData[nRow - 1].FixedX;
        bd.FixedY = m_BearingPierDetailData[nRow - 1].FixedY;
        bd.UseExtPlates = m_BearingPierDetailData[nRow - 1].UseExtPlates;
        bd.ShearDeformationOverride = m_BearingPierDetailData[nRow - 1].ShearDeformationOverride;

        m_details_dlg.SetBearingDetailDlg(bd);

        bool bRepeatDialog = true;

        while (bRepeatDialog)
        {
            if (m_details_dlg.DoModal() == IDOK)
            {
                const auto& computed_height = m_details_dlg.GetComputedHeight();

                const auto& brg_details = m_details_dlg.GetBearingDetails();

                if (!IsEqual(computed_height, bd.Height))
                {
                    CString msg;
                    msg.Format(_T("The computed height and the initial input are different. Do you want to override the input?"));

                    if (AfxMessageBox(msg, MB_YESNO | MB_ICONWARNING) == IDYES)
                    {
                        


                        SetStyleRange(CGXRange(nRow, m_DGetter.m_BearingHeightCol), CGXStyle()
                            .SetEnabled(FALSE)
                            .SetHorizontalAlignment(DT_RIGHT)
                            .SetVerticalAlignment(DT_TOP)
                            .SetInterior(::GetSysColor(COLOR_BTNFACE))
                            .SetTextColor(::GetSysColor(COLOR_GRAYTEXT))
                            .SetValue(FormatDimension(computed_height, *m_pCompUnit, false))
                        );

                        


                       bRepeatDialog = false; // Exit loop
                    }
                    else
                    {
                        // Loop will repeat
                    }
                }
                else
                {
                    bd.Height = computed_height;
                    bRepeatDialog = false;
                }

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
                    .SetValue(FormatDimension(brg_details.Width, *m_pCompUnit, false))
                );

                m_BearingPierDetailData[0].ElastomerThickness = brg_details.ElastomerThickness;
                m_BearingPierDetailData[0].CoverThickness = brg_details.CoverThickness;
                m_BearingPierDetailData[0].ShimThickness = brg_details.ShimThickness;
                m_BearingPierDetailData[0].NumIntLayers = brg_details.NumIntLayers;
                m_BearingPierDetailData[0].UseExtPlates = brg_details.UseExtPlates;
                m_BearingPierDetailData[0].FixedX = brg_details.FixedX;
                m_BearingPierDetailData[0].FixedY = brg_details.FixedY;
                m_BearingPierDetailData[0].ShearDeformationOverride = brg_details.ShearDeformationOverride;


            }
            else
            {
                bRepeatDialog = false; // Dialog cancelled
            }
        }
    }

}
