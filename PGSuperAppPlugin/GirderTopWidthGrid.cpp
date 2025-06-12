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

// GirderSpacingGrid.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "GirderTopWidthGrid.h"
#include "GirderLayoutPage.h"

#include <IFace/Tools.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\BeamFactory.h>
//
#include <PsgLib\GirderGroupData.h>
#include <PsgLib\GirderLibraryEntry.h>
#include <PsgLib\Helpers.h>


#define LEFT 0
#define RIGHT 1

GRID_IMPLEMENT_REGISTER(CGirderTopWidthGrid, CS_DBLCLKS, 0, 0, 0);

void DDV_TopWidthGrid(CDataExchange* pDX,int nIDC, CGirderTopWidthGrid* pGrid)
{
   if (!pDX->m_bSaveAndValidate )
      return;

   pDX->PrepareCtrl(nIDC);
   if ( !pGrid->ValidateGirderTopWidth() )
   {
      pDX->Fail();
   }
}


class CTopWidthGroupData : public CGXAbstractUserAttribute
{
public:
   CTopWidthGroupData(GirderIndexType firstGdrIdx,GirderIndexType lastGdrIdx)
   {
      m_FirstGirderIdx = firstGdrIdx;
      m_LastGirderIdx  = lastGdrIdx;
   }

   virtual CGXAbstractUserAttribute* Clone() const
   {
      return new CTopWidthGroupData(m_FirstGirderIdx,m_LastGirderIdx);
   }

   GirderIndexType m_FirstGirderIdx;
   GirderIndexType m_LastGirderIdx;
};

/////////////////////////////////////////////////////////////////////////////
// CGirderTopWidthGrid

CGirderTopWidthGrid::CGirderTopWidthGrid()
{
//   RegisterClass();
   m_pGirderGroup = nullptr;
   m_bEnabled = TRUE;
}

CGirderTopWidthGrid::~CGirderTopWidthGrid()
{
}

BEGIN_MESSAGE_MAP(CGirderTopWidthGrid, CGXGridWnd)
	//{{AFX_MSG_MAP(CGirderTopWidthGrid)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	ON_COMMAND(IDC_EXPAND, OnExpand)
	ON_COMMAND(IDC_JOIN, OnJoin)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CGirderTopWidthGrid message handlers
void CGirderTopWidthGrid::CustomInit(CGirderGroupData *pGirderGroup)
{
   m_pGirderGroup = pGirderGroup;

	Initialize( );

   GetParam()->EnableUndo(FALSE);

		// Turn off selecting whole columns when clicking on a column header
	GetParam()->EnableSelection((WORD) (GX_SELFULL & ~GX_SELTABLE & ~GX_SELROW));

   // no row moving
	GetParam()->EnableMoveRows(FALSE);
   GetParam()->EnableMoveCols(FALSE);

   UpdateGrid();

   // don't allow users to resize grids
   GetParam( )->EnableTrackColWidth(0); 
   GetParam( )->EnableTrackRowHeight(0); 

	EnableIntelliMouse();

   EnableGridToolTips();
   SetStyleRange(CGXRange(0,0,GetRowCount(),GetColCount()),
      CGXStyle()
      .SetWrapText(TRUE)
      .SetAutoSize(TRUE)
      .SetUserAttribute(GX_IDS_UA_TOOLTIPTEXT,_T("To regroup top widths, select column headings, right click over the grid, and select Expand or Join from the menu"))
      );
	
   GetParam()->EnableUndo(TRUE);

	SetFocus();
}

void CGirderTopWidthGrid::UpdateGrid()
{
   if ( GetSafeHwnd() == nullptr )
      return; // grid isn't ready for filling

   if ( m_pGirderGroup == nullptr )
      return; // not associated with a group

	GetParam()->EnableUndo(FALSE);
   GetParam()->SetLockReadOnly(FALSE);

   
   auto pBroker = EAFGetBroker();
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   GroupIndexType nTopWidthGroups = m_pGirderGroup->GetGirderTopWidthGroupCount();
   GirderIndexType nGirders = m_pGirderGroup->GetGirderCount();

   const ROWCOL num_rows = 6;
   const ROWCOL num_cols = 2;

   SetRowCount(num_rows);
	SetColCount(num_cols);

   // we want to merge cells
   SetMergeCellsMode(gxnMergeDelayEval);
   SetFrozenCols(1, 1); // column 1 is frozen and is a row header column

   // disable left side
	SetStyleRange(CGXRange(0,0,num_rows,1), CGXStyle()
			.SetControl(GX_IDS_CTRL_HEADER)
			.SetEnabled(FALSE)          // disables usage as current cell
		);

   // This loop fills the grid "group" column headings and with the top width data
   m_MinGirderTopWidth[LEFT].clear();
   m_MaxGirderTopWidth[LEFT].clear();
   m_MinGirderTopWidth[RIGHT].clear();
   m_MaxGirderTopWidth[RIGHT].clear();
   for (GroupIndexType groupIdx = 0; groupIdx < nTopWidthGroups; groupIdx++)
   {
      ROWCOL col = (ROWCOL)(groupIdx + 2);
      if (groupIdx != 0)
      {
         InsertCols(col, 1);
      }

      pgsTypes::TopWidthType topWidthType;
      Float64 leftStart, rightStart, leftEnd, rightEnd;
      GirderIndexType firstGdrIdx, lastGdrIdx;

      m_pGirderGroup->GetGirderTopWidthGroup(groupIdx, &firstGdrIdx, &lastGdrIdx, &topWidthType, &leftStart, &rightStart, &leftEnd, &rightEnd);

      const CSplicedGirderData* pGirder = m_pGirderGroup->GetGirder(firstGdrIdx);
      const GirderLibraryEntry* pGdrEntry = pGirder->GetGirderLibraryEntry();
      auto factory = pGdrEntry->GetBeamFactory();


      CString strTopWidthTypes;
      auto supportedTypes = factory->GetSupportedTopWidthTypes();
      for (auto type : supportedTypes)
      {
         strTopWidthTypes += GetTopWidthType(type) + _T("\n");
      }
      const GirderLibraryEntry::Dimensions& dimensions = pGdrEntry->GetDimensions();

      Float64 Wmin[2], Wmax[2];
      factory->GetAllowableTopWidthRange(topWidthType,dimensions,&Wmin[LEFT], &Wmax[LEFT],&Wmin[RIGHT],&Wmax[RIGHT]);
      m_MinGirderTopWidth[LEFT].push_back(Wmin[LEFT]);
      m_MaxGirderTopWidth[LEFT].push_back(Wmax[LEFT]);
      m_MinGirderTopWidth[RIGHT].push_back(Wmin[RIGHT]);
      m_MaxGirderTopWidth[RIGHT].push_back(Wmax[RIGHT]);

      CString strHeading;
      if (firstGdrIdx == lastGdrIdx)
      {
         strHeading.Format(_T("%s"), LABEL_GIRDER(firstGdrIdx));
      }
      else
      {
         strHeading.Format(_T("%s-%s"), LABEL_GIRDER(firstGdrIdx), LABEL_GIRDER(lastGdrIdx));
      }

      CTopWidthGroupData groupData(firstGdrIdx, lastGdrIdx);
      SetStyleRange(CGXRange(0,col), CGXStyle()
         .SetHorizontalAlignment(DT_CENTER)
         .SetEnabled(FALSE)
         .SetValue(strHeading)
         .SetUserAttribute(0, groupData)
      );

      SetStyleRange(CGXRange(1, col), CGXStyle()
         .SetHorizontalAlignment(DT_RIGHT)
         .SetEnabled(TRUE)
         .SetReadOnly(FALSE)
         .SetControl(GX_IDS_CTRL_CBS_DROPDOWNLIST)
         .SetChoiceList(strTopWidthTypes)
         .SetValue(GetTopWidthType(topWidthType))
      );

      CString strTopWidth;
      strTopWidth.Format(_T("%s"),FormatDimension(leftStart, pDisplayUnits->GetXSectionDimUnit(),false));
      SetStyleRange(CGXRange(2,col), CGXStyle()
         .SetHorizontalAlignment(DT_RIGHT)
         .SetEnabled(TRUE)
         .SetReadOnly(FALSE)
         .SetInterior(::GetSysColor(COLOR_WINDOW))
         .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT))
         .SetValue(strTopWidth)
         );

      if (topWidthType == pgsTypes::twtAsymmetric)
      {
         strTopWidth.Format(_T("%s"), FormatDimension(rightStart, pDisplayUnits->GetXSectionDimUnit(), false));
         SetStyleRange(CGXRange(3, col), CGXStyle()
            .SetHorizontalAlignment(DT_RIGHT)
            .SetEnabled(TRUE)
            .SetReadOnly(FALSE)
            .SetInterior(::GetSysColor(COLOR_WINDOW))
            .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT))
            .SetValue(strTopWidth)
         );
      }
      else
      {
         SetStyleRange(CGXRange(3, col), CGXStyle()
            .SetHorizontalAlignment(DT_RIGHT)
            .SetEnabled(FALSE)
            .SetReadOnly(TRUE)
            .SetInterior(::GetSysColor(COLOR_BTNFACE))
            .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT))
         );
      }

      strTopWidth.Format(_T("%s"), FormatDimension(leftEnd, pDisplayUnits->GetXSectionDimUnit(), false));
      SetStyleRange(CGXRange(4, col), CGXStyle()
         .SetHorizontalAlignment(DT_RIGHT)
         .SetEnabled(TRUE)
         .SetReadOnly(FALSE)
         .SetInterior(::GetSysColor(COLOR_WINDOW))
         .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT))
         .SetValue(strTopWidth)
      );

      if (topWidthType == pgsTypes::twtAsymmetric)
      {
         strTopWidth.Format(_T("%s"), FormatDimension(rightEnd, pDisplayUnits->GetXSectionDimUnit(), false));
         SetStyleRange(CGXRange(5, col), CGXStyle()
            .SetHorizontalAlignment(DT_RIGHT)
            .SetEnabled(TRUE)
            .SetReadOnly(FALSE)
            .SetInterior(::GetSysColor(COLOR_WINDOW))
            .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT))
            .SetValue(strTopWidth)
         );
      }
      else
      {
         SetStyleRange(CGXRange(5, col), CGXStyle()
            .SetHorizontalAlignment(DT_RIGHT)
            .SetEnabled(FALSE)
            .SetReadOnly(TRUE)
            .SetInterior(::GetSysColor(COLOR_BTNFACE))
            .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT))
         );
      }

      CString strAllowable;
      if (topWidthType == pgsTypes::twtAsymmetric)
      {
         strAllowable.Format(_T("Left %s - %s\nRight %s - %s"),
            FormatDimension(Wmin[LEFT], pDisplayUnits->GetXSectionDimUnit(), false),
            FormatDimension(Wmax[LEFT], pDisplayUnits->GetXSectionDimUnit(), false),
            FormatDimension(Wmin[RIGHT], pDisplayUnits->GetXSectionDimUnit(), false),
            FormatDimension(Wmax[RIGHT], pDisplayUnits->GetXSectionDimUnit(), false));
      }
      else
      {
         strAllowable.Format(_T("%s - %s"),
            FormatDimension(Wmin[LEFT], pDisplayUnits->GetXSectionDimUnit(), false),
            FormatDimension(Wmax[LEFT], pDisplayUnits->GetXSectionDimUnit(), false));
      }

      SetStyleRange(CGXRange(6, col), CGXStyle()
         .SetHorizontalAlignment(DT_LEFT)
         .SetEnabled(FALSE)
         .SetReadOnly(TRUE)
         .SetInterior(::GetSysColor(COLOR_BTNFACE))
         .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT))
         .SetWrapText(TRUE) // enables multiline text... also see ResizeRowHeightsToFit below
         .SetValue( strAllowable )
         );
   } // girder group loop

   // The rest of this function sets up the labels down the left side of the grid
   SetStyleRange(CGXRange(0,0,0,1), CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetEnabled(FALSE)
      .SetVerticalAlignment(DT_VCENTER)
      .SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE)
      .SetValue(_T("Girder"))
      );

   SetStyleRange(CGXRange(1, 0, 1, 1), CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetVerticalAlignment(DT_VCENTER)
      .SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE)
      .SetEnabled(FALSE)
      .SetValue(_T("Type"))
   );

   CGXFont font;
   font.SetOrientation(900); // vertical text 

   SetStyleRange(CGXRange(2, 0,3,0), CGXStyle()
      .SetEnabled(FALSE)
      .SetHorizontalAlignment(DT_CENTER)
      .SetVerticalAlignment(DT_VCENTER)
      .SetMergeCell(GX_MERGE_VERTICAL | GX_MERGE_COMPVALUE)
      .SetFont(font)
      .SetWrapText(TRUE)
     .SetValue(_T("Start"))
   );

   CString strLabel;
   strLabel.Format(_T("Top Width/Left (%s)"), pDisplayUnits->GetXSectionDimUnit().UnitOfMeasure.UnitTag().c_str());
   SetStyleRange(CGXRange(2,1), CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetEnabled(FALSE)
      .SetValue(strLabel)
      );

   strLabel.Format(_T("Right (%s)"), pDisplayUnits->GetXSectionDimUnit().UnitOfMeasure.UnitTag().c_str());
   SetStyleRange(CGXRange(3, 1), CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetEnabled(FALSE)
      .SetValue(strLabel)
   );

   SetStyleRange(CGXRange(4, 0,5,0), CGXStyle()
      .SetEnabled(FALSE)
      .SetHorizontalAlignment(DT_CENTER)
      .SetVerticalAlignment(DT_VCENTER)
      .SetMergeCell(GX_MERGE_VERTICAL | GX_MERGE_COMPVALUE)
      .SetFont(font)
      .SetWrapText(TRUE)
      .SetValue(_T("End"))
   );

   strLabel.Format(_T("Top Width/Left (%s)"), pDisplayUnits->GetXSectionDimUnit().UnitOfMeasure.UnitTag().c_str());
   SetStyleRange(CGXRange(4, 1), CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetEnabled(FALSE)
      .SetValue(strLabel)
   );

   strLabel.Format(_T("Right (%s)"), pDisplayUnits->GetXSectionDimUnit().UnitOfMeasure.UnitTag().c_str());
   SetStyleRange(CGXRange(5, 1), CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetEnabled(FALSE)
      .SetValue(strLabel)
   );

   strLabel.Format(_T("Allowable (%s)"), pDisplayUnits->GetXSectionDimUnit().UnitOfMeasure.UnitTag().c_str());
   SetStyleRange(CGXRange(6, 0, 6, 1), CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetVerticalAlignment(DT_VCENTER)
      .SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE)
      .SetEnabled(FALSE)
      .SetValue(strLabel)
      );

   CSpanGirderLayoutPage* pParent = (CSpanGirderLayoutPage*)GetParent();
   auto factory = pParent->GetBeamFactory();
   if (!factory->CanTopWidthVary())
   {
      HideRows(4, 5);

      CGXFont font2;
      font2.SetOrientation(0);

      strLabel.Format(_T("Top Width/Left (%s)"), pDisplayUnits->GetXSectionDimUnit().UnitOfMeasure.UnitTag().c_str());
      SetStyleRange(CGXRange(2, 0, 2, 1), CGXStyle()
         .SetEnabled(FALSE)
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
         .SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE)
         .SetWrapText(TRUE)
         .SetFont(font2)
         .SetValue(strLabel)
      );
      strLabel.Format(_T("Right (%s)"), pDisplayUnits->GetXSectionDimUnit().UnitOfMeasure.UnitTag().c_str());
      SetStyleRange(CGXRange(3, 0, 3, 1), CGXStyle()
         .SetEnabled(FALSE)
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
         .SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE)
         .SetWrapText(TRUE)
         .SetFont(font2)
         .SetValue(strLabel)
      );
   }

   // make it so that text fits correctly in header row
   ResizeColWidthsToFit(CGXRange(0, 0, GetRowCount(), GetColCount()), FALSE);
   ResizeRowHeightsToFit(CGXRange(0, 0, GetRowCount(), GetColCount()));

   // this will get the styles of all the cells to be correct if the size of the grid changed
   Enable(m_bEnabled);

   GetParam()->SetLockReadOnly(TRUE);
	GetParam()->EnableUndo(TRUE);
}

BOOL CGirderTopWidthGrid::OnRButtonHitRowCol(ROWCOL nHitRow,ROWCOL nHitCol,ROWCOL nDragRow,ROWCOL nDragCol,CPoint point,UINT nFlags,WORD nHitState)
{
   if ( nHitState & GX_HITSTART )
      return TRUE;

   if ( 2 <= nHitCol )
   {
      CRowColArray selCols;
      ROWCOL nSelected = GetSelectedCols(selCols);

      if ( 0 < nSelected && m_bEnabled )
      {
         CMenu menu;
         VERIFY( menu.LoadMenu(IDR_GIRDER_GRID_CONTEXT) );
         ClientToScreen(&point);
         menu.GetSubMenu(0)->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this);

         return TRUE;
      }
   }

   return FALSE;
}

void CGirderTopWidthGrid::OnExpand()
{
   CRowColArray selCols;
   ROWCOL nSelCols = GetSelectedCols(selCols);

   if ( nSelCols == 0 )
      return;

   if ( nSelCols == GetColCount() )
   {
      m_pGirderGroup->ExpandAllGirderTopWidthGroups();
   }
   else
   {
      for ( int i = nSelCols-1; 0 <= i; i-- )
      {
         ROWCOL col = selCols[i];
         GroupIndexType grpIdx = GroupIndexType(col-2);

         m_pGirderGroup->ExpandGirderTopWidthGroup(grpIdx);
      }
   }

   UpdateGrid();
}

void CGirderTopWidthGrid::OnJoin()
{
   CRowColArray selLeftCols,selRightCols;
   GetSelectedCols(selLeftCols,selRightCols,FALSE,FALSE);
   ROWCOL nSelRanges = (ROWCOL)selLeftCols.GetSize();
   if ( nSelRanges == 0 )
      return;

   for (ROWCOL i = 0; i < nSelRanges; i++ )
   {
      ROWCOL firstCol = selLeftCols[i];
      ROWCOL lastCol  = selRightCols[i];

      CGXStyle firstColStyle, lastColStyle;
      GetStyleRowCol(0,firstCol,firstColStyle);
      GetStyleRowCol(0,lastCol, lastColStyle);

      const CTopWidthGroupData& firstColUserData = dynamic_cast<const CTopWidthGroupData&>(firstColStyle.GetUserAttribute(0));
      const CTopWidthGroupData& lastColUserData  = dynamic_cast<const CTopWidthGroupData&>(lastColStyle.GetUserAttribute(0));

      GirderIndexType firstGdrIdx = firstColUserData.m_FirstGirderIdx;
      GirderIndexType lastGdrIdx  = lastColUserData.m_LastGirderIdx;

      m_pGirderGroup->JoinGirderTopWidthGroup(firstGdrIdx, lastGdrIdx, firstGdrIdx);
   }

   UpdateGrid();
}

void CGirderTopWidthGrid::Enable(BOOL bEnable)
{
   m_bEnabled = bEnable;

	GetParam()->EnableUndo(FALSE);
   GetParam()->SetLockReadOnly(FALSE);

   CGXStyle style;
   CGXRange range;
   if ( bEnable )
   {
      style.SetEnabled(TRUE)
           .SetReadOnly(FALSE)
           .SetInterior(::GetSysColor(COLOR_WINDOW))
           .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT));

      ROWCOL nCols = GetColCount();
      if ( 0 < nCols )
      {
         for (ROWCOL col = 2; col <= nCols; col++)
         {
            pgsTypes::TopWidthType type = GetTopWidthTypeFromCell(col);
            if (type == pgsTypes::twtAsymmetric)
            {
               // apply style to all input cells
               range = CGXRange(1, col, 5, col); // Set style for Type through End/Right rows
               SetStyleRange(range, style);
            }
            else
            {
               // "right" values are be disabled and blanked out when not used
               
               range = CGXRange(1, col, 2, col); // Set style for Type and Start/Left rows
               SetStyleRange(range, style);

               range = CGXRange(4, col); // Set style for End/Left row
               SetStyleRange(range, style);

               // disable and blank out Start/Right row
               SetStyleRange(CGXRange(3, col), CGXStyle()
                  .SetEnabled(FALSE)
                  .SetReadOnly(TRUE)
                  .SetInterior(::GetSysColor(COLOR_BTNFACE))
                  .SetValue(_T(""))
               );

               // disable and blank out End/Right row
               SetStyleRange(CGXRange(5, col), CGXStyle()
                  .SetEnabled(FALSE)
                  .SetReadOnly(TRUE)
                  .SetInterior(::GetSysColor(COLOR_BTNFACE))
                  .SetValue(_T(""))
               );
            }
         }
      }

      // Column Headings
      style.SetInterior(::GetSysColor(COLOR_BTNFACE))
           .SetEnabled(FALSE)
           .SetReadOnly(TRUE);

      // across the top
      range = CGXRange(0,0,0,nCols);
      SetStyleRange(range,style);

      // the "type" box
      range = CGXRange(1, 0, 1, 0);
      SetStyleRange(range, style);

      // the Start box
      range = CGXRange(2, 0, 3, 0);
      SetStyleRange(range, style);

      // the start "Left" box
      range = CGXRange(2, 1);
      SetStyleRange(range, style);

      // the start "Right" box
      range = CGXRange(3, 1);
      SetStyleRange(range, style);

      // the End box
      range = CGXRange(4, 0, 5, 0);
      SetStyleRange(range, style);

      // the end "Left" box
      range = CGXRange(4, 1);
      SetStyleRange(range, style);

      // the end "Right" box
      range = CGXRange(5, 1);
      SetStyleRange(range, style);

      // the "Allowable" box + the actual allowable spacing cells
      range = CGXRange(6,0,6,nCols);
      SetStyleRange(range,style);
   }
   else
   {
      // Disable everything
      style.SetEnabled(FALSE)
           .SetReadOnly(TRUE)
           .SetInterior(::GetSysColor(COLOR_BTNFACE))
           .SetTextColor(::GetSysColor(COLOR_GRAYTEXT));

      range = CGXRange(0,0,GetRowCount(),GetColCount());
      SetStyleRange(range,style);
   }


   GetParam()->SetLockReadOnly(TRUE);
   GetParam()->EnableUndo(FALSE);
}

CString CGirderTopWidthGrid::GetCellValue(ROWCOL nRow, ROWCOL nCol)
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

pgsTypes::TopWidthType CGirderTopWidthGrid::GetTopWidthTypeFromCell(ROWCOL col)
{
   CString strType = GetCellValue(1, col);
   pgsTypes::TopWidthType type;
   if (strType == GetTopWidthType(pgsTypes::twtSymmetric))
   {
      type = pgsTypes::twtSymmetric;
   }
   else if (strType == GetTopWidthType(pgsTypes::twtCenteredCG))
   {
      type = pgsTypes::twtCenteredCG;
   }
   else if (strType == GetTopWidthType(pgsTypes::twtAsymmetric))
   {
      type = pgsTypes::twtAsymmetric;
   }
   else
   {
      ATLASSERT(false); // is there a new type
   }
   return type;
}

BOOL CGirderTopWidthGrid::ValidateGirderTopWidth()
{
   
   auto pBroker = EAFGetBroker();
   GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);

   ROWCOL nCols = GetColCount();
   for (int i = 0; i < 2; i++) // start/end
   {
      for (ROWCOL col = 2; col <= nCols; col++)
      {
         CString strLeft = GetCellValue(2+i*2, col);
         CString strRight = GetCellValue(3+i*2, col);
         Float64 left = _tstof(strLeft);
         Float64 right = _tstof(strRight);

         left = WBFL::Units::ConvertToSysUnits(left, pDisplayUnits->GetXSectionDimUnit().UnitOfMeasure);
         right = WBFL::Units::ConvertToSysUnits(right, pDisplayUnits->GetXSectionDimUnit().UnitOfMeasure);

         pgsTypes::TopWidthType topWidthType = GetTopWidthTypeFromCell(col);

         Float64 minGirderTopWidth[2] = { m_MinGirderTopWidth[LEFT][col - 2], m_MinGirderTopWidth[RIGHT][col - 2] };
         Float64 maxGirderTopWidth[2] = { m_MaxGirderTopWidth[LEFT][col - 2], m_MaxGirderTopWidth[RIGHT][col - 2] };
         if (topWidthType == pgsTypes::twtAsymmetric)
         {
            if (left < 0 || IsLT(left, minGirderTopWidth[LEFT]) || IsLT(maxGirderTopWidth[LEFT], left))
            {
               if (i == 0)
               {
                  SetWarningText(_T("Start left top width is out of range"));
               }
               else
               {
                  SetWarningText(_T("End left top width is out of range"));
               }
               DisplayWarningText();
               return FALSE;
            }

            if (right < 0 || IsLT(left, minGirderTopWidth[RIGHT]) || IsLT(maxGirderTopWidth[RIGHT], right))
            {
               if (i == 0)
               {
                  SetWarningText(_T("Start right top width is out of range"));
               }
               else
               {
                  SetWarningText(_T("End right top width is out of range"));
               }
               DisplayWarningText();
               return FALSE;
            }
         }
         else
         {
            if (left < 0 || IsLT(left, minGirderTopWidth[LEFT]) || IsLT(maxGirderTopWidth[LEFT], left))
            {
               if (i == 0)
               {
                  SetWarningText(_T("Start top width is out of range"));
               }
               else
               {
                  SetWarningText(_T("End top width is out of range"));
               }
               DisplayWarningText();
               return FALSE;
            }
         }
      }
   }

   return TRUE;
}

void CGirderTopWidthGrid::OnModifyCell(ROWCOL nRow, ROWCOL nCol)
{
   if (nRow == 1 && 2 <= nCol)
   {
      pgsTypes::TopWidthType type = GetTopWidthTypeFromCell(nCol);

      GroupIndexType topWidthGroupIdx = (GroupIndexType)(nCol - 2);

      CGirderTopWidthGroup group = m_pGirderGroup->GetGirderTopWidthGroup(topWidthGroupIdx);
      m_pGirderGroup->SetGirderTopWidth(topWidthGroupIdx, type, group.left[pgsTypes::metStart], group.right[pgsTypes::metStart], group.left[pgsTypes::metEnd], group.right[pgsTypes::metEnd]);

      UpdateGrid();
   }
   else
   {
      __super::OnModifyCell(nRow, nCol);
   }
}

BOOL CGirderTopWidthGrid::OnEndEditing(ROWCOL nRow, ROWCOL nCol)
{
   if ( (nRow == 2 || nRow == 4) && 2 <= nCol)
   {
      
      auto pBroker = EAFGetBroker();

      GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);

      CString strValue = GetCellValue(nRow, nCol);

      Float64 left = _tstof(strValue);
      left = WBFL::Units::ConvertToSysUnits(left, pDisplayUnits->GetXSectionDimUnit().UnitOfMeasure);

      GroupIndexType topWidthGroupIdx = (GroupIndexType)(nCol - 2);

      CGirderTopWidthGroup group = m_pGirderGroup->GetGirderTopWidthGroup(topWidthGroupIdx);
      if (nRow == 2)
      {
         // changed left at start of span
         m_pGirderGroup->SetGirderTopWidth(topWidthGroupIdx, group.type, left, group.right[pgsTypes::metStart],group.left[pgsTypes::metEnd],group.right[pgsTypes::metEnd]);
      }
      else
      {
         // changed left at end of span
         m_pGirderGroup->SetGirderTopWidth(topWidthGroupIdx, group.type, group.left[pgsTypes::metStart], group.right[pgsTypes::metStart], left, group.right[pgsTypes::metEnd]);
      }

      UpdateGrid();
   }
   else if ( (nRow == 3 || nRow == 5) && 2 <= nCol)
   {
      
      auto pBroker = EAFGetBroker();

      GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);

      CString strValue = GetCellValue(nRow, nCol);

      Float64 right = _tstof(strValue);
      right = WBFL::Units::ConvertToSysUnits(right, pDisplayUnits->GetXSectionDimUnit().UnitOfMeasure);

      GroupIndexType topWidthGroupIdx = (GroupIndexType)(nCol - 2);

      CGirderTopWidthGroup group = m_pGirderGroup->GetGirderTopWidthGroup(topWidthGroupIdx);
      if (nRow == 3)
      {
         // changed right at start of span
         m_pGirderGroup->SetGirderTopWidth(topWidthGroupIdx, group.type, group.left[pgsTypes::metStart], right, group.left[pgsTypes::metEnd],group.right[pgsTypes::metEnd]);
      }
      else
      {
         // changed right at end of span
         m_pGirderGroup->SetGirderTopWidth(topWidthGroupIdx, group.type, group.left[pgsTypes::metStart], group.right[pgsTypes::metStart], group.left[pgsTypes::metEnd], right);
      }

      UpdateGrid();
   }

   return CGXGridWnd::OnEndEditing(nRow, nCol);
}
