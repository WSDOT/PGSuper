///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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

// BridgeDescFramingGrid.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "BridgeDescFramingGrid.h"
#include "PGSuperDoc.h"
#include "PGSuperUnits.h"
#include "BridgeDescDlg.h"
#include "BridgeDescFramingPage.h"

#include <Units\Measure.h>
#include <EAF\EAFDisplayUnits.h>

#include <algorithm>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static int SPAN_BUTTON = 1;
static int PIER_BUTTON = 2;

GRID_IMPLEMENT_REGISTER(CBridgeDescFramingGrid, CS_DBLCLKS, 0, 0, 0);

/////////////////////////////////////////////////////////////////////////////
// CBridgeDescFramingGrid

CBridgeDescFramingGrid::CBridgeDescFramingGrid()
{
//   RegisterClass();
   HRESULT hr = m_objStation.CoCreateInstance(CLSID_Station);
}

CBridgeDescFramingGrid::~CBridgeDescFramingGrid()
{
}

BEGIN_MESSAGE_MAP(CBridgeDescFramingGrid, CGXGridWnd)
	//{{AFX_MSG_MAP(BridgeDescFramingGrid)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

std::vector<Float64> CBridgeDescFramingGrid::GetSpanLengths()
{
   std::vector<Float64> spanLengths;

   ROWCOL nRows = GetRowCount();
   for (ROWCOL row = 3; row <= nRows; row += 2)
   {
      CGXStyle style;

      CPierData* pPier2 = GetPierRowData(row);

      CPierData* pPier1 = GetPierRowData(row-2);

      Float64 L = pPier2->GetStation() - pPier1->GetStation();
      spanLengths.push_back(L);
   }

   ASSERT(spanLengths.size() == GetPierCount()-1);
   return spanLengths;
}

void CBridgeDescFramingGrid::SetSpanLengths(const std::vector<Float64>& spanLengths,PierIndexType fixedPierIdx)
{
   ROWCOL row = GetPierRow(fixedPierIdx);
   CPierData* pFixedPierData = GetPierRowData(row);
   Float64 station = pFixedPierData->GetStation();

   // work backwards to the first pier
   PierIndexType idx = fixedPierIdx-1;
   while (0 <= idx && idx != ALL_PIERS)
   {
      station -= spanLengths[idx];
      idx--;
   }

   CBridgeDescFramingPage* pParent = (CBridgeDescFramingPage*)GetParent();
   ASSERT( pParent->IsKindOf(RUNTIME_CLASS(CBridgeDescFramingPage) ) );

   CBridgeDescDlg* pDlg = (CBridgeDescDlg*)(pParent->GetParent());
   ASSERT( pDlg->IsKindOf(RUNTIME_CLASS(CBridgeDescDlg) ) );

   // holding first pier constant
   CPierData* pPrevPier = pDlg->m_BridgeDesc.GetPier(0);
   pPrevPier->SetStation( station );
   FillPierRow(GetPierRow(pPrevPier->GetPierIndex()),*pPrevPier);

   ASSERT(spanLengths.size() == pDlg->m_BridgeDesc.GetSpanCount());

   std::vector<Float64>::const_iterator iter;
   for ( iter = spanLengths.begin(); iter != spanLengths.end(); iter++ )
   {
      Float64 L = *iter;

      CSpanData* pNextSpan = pPrevPier->GetNextSpan();
      ASSERT(pNextSpan);

      CPierData* pNextPier = pNextSpan->GetNextPier();
      pNextPier->SetStation( pPrevPier->GetStation() + L );

      FillSpanRow(GetSpanRow(pNextSpan->GetSpanIndex()),*pNextSpan);
      FillPierRow(GetPierRow(pNextPier->GetPierIndex()),*pNextPier);

      pPrevPier = pNextPier;
   }
}

/////////////////////////////////////////////////////////////////////////////
// CBridgeDescFramingGrid message handlers
int CBridgeDescFramingGrid::GetColWidth(ROWCOL nCol)
{
	CRect rect;
   GetClientRect(&rect);

   int grid_width = rect.Width();
   int A = 28; // abut/pier/span column
   int B = 28; // station
   int C = 28; // orientation
   int W = 160;

   switch (nCol)
   {
   case 0: // abut/pier column
      return A*grid_width/W;

   case 1: // station
      return B*grid_width/W;

   case 2: // orientation
      return C*grid_width/W;

   case 3: // button
      return (W-(A+B+C))*grid_width/W;

   default:
      return CGXGridWnd::GetColWidth(nCol);
   }
}

BOOL CBridgeDescFramingGrid::OnRButtonClickedRowCol(ROWCOL nRow, ROWCOL nCol, UINT nFlags, CPoint pt)
{
	 // unreferenced parameters
	 nRow, nCol, nFlags;

	CMenu menu;
	VERIFY(menu.LoadMenu(IDR_FRAMING_GRID_CONTEXT));

	CMenu* pPopup = menu.GetSubMenu( 0 );
	ASSERT( pPopup != NULL );

   // deal with disabling delete since update stuff doesn't seem to work right
   UINT bCanDelete = EnableItemDelete() ? MF_ENABLED|MF_BYCOMMAND : MF_GRAYED|MF_BYCOMMAND;
   pPopup->EnableMenuItem(IDC_REMOVE_SPAN, bCanDelete);

   CBridgeDescFramingPage* pParent = (CBridgeDescFramingPage*)GetParent();
   ASSERT( pParent->IsKindOf(RUNTIME_CLASS(CBridgeDescFramingPage) ) );

	// display the menu
	ClientToScreen(&pt);
	pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, pParent);

	// we processed the message
	return TRUE;
}

void CBridgeDescFramingGrid::OnChangedSelection(const CGXRange* pChangedRect,BOOL bIsDragging, BOOL bKey)
{
   CBridgeDescFramingPage* pParent = (CBridgeDescFramingPage*)GetParent();
   ASSERT( pParent->IsKindOf(RUNTIME_CLASS(CBridgeDescFramingPage) ) );
   pParent->EnableRemove(EnableItemDelete());
}

void CBridgeDescFramingGrid::InsertRow()
{
	ROWCOL nRow = 0;
   nRow = GetRowCount()+1;
	nRow = max(1, nRow);

	InsertRows(nRow, 1);

   ScrollCellInView(nRow+1, GetLeftCol());
}

void CBridgeDescFramingGrid::OnAddSpan()
{
   CBridgeDescFramingPage* pParent = (CBridgeDescFramingPage*)GetParent();
   ASSERT( pParent->IsKindOf(RUNTIME_CLASS(CBridgeDescFramingPage) ) );

   CBridgeDescDlg* pDlg = (CBridgeDescDlg*)(pParent->GetParent());
   ASSERT( pDlg->IsKindOf(RUNTIME_CLASS(CBridgeDescDlg) ) );

   CRowColArray sel_rows;
   ROWCOL nSelRows = GetSelectedRows(sel_rows);
   if ( nSelRows == 0 )
   {
      nSelRows = 1;
      sel_rows.Add(GetRowCount());
   }

   // Default length for new spans
   Float64 new_span_length = ::ConvertToSysUnits(100.0,unitMeasure::Feet);

   for ( int r = nSelRows-1; r >= 0; r-- )
   {
      ROWCOL selRow = sel_rows[r];
      PierIndexType pierIdx = GetPierIndex(selRow);
      if ( pierIdx != ALL_PIERS)
      {
         pDlg->m_BridgeDesc.InsertSpan(pierIdx,pgsTypes::Ahead,new_span_length);
      }
      else
      {
         SpanIndexType spanIdx = GetSpanIndex(selRow);
         if ( spanIdx != ALL_SPANS)
         {
            pDlg->m_BridgeDesc.InsertSpan(spanIdx+1,pgsTypes::Back,new_span_length);
         }
      }
   }

   FillGrid(pDlg->m_BridgeDesc);
}

void CBridgeDescFramingGrid::OnRemoveSpan()
{
   CBridgeDescFramingPage* pParent = (CBridgeDescFramingPage*)GetParent();
   ASSERT( pParent->IsKindOf(RUNTIME_CLASS(CBridgeDescFramingPage) ) );

   CBridgeDescDlg* pDlg = (CBridgeDescDlg*)(pParent->GetParent());
   ASSERT( pDlg->IsKindOf(RUNTIME_CLASS(CBridgeDescDlg) ) );

   CRowColArray sel_rows;
   ROWCOL nSelRows = GetSelectedRows(sel_rows);
   for ( int r = nSelRows-1; r >= 0; r-- )
   {
      ROWCOL selRow = sel_rows[r];
      SpanIndexType spanIdx = GetSpanIndex(selRow);
      if ( spanIdx != ALL_SPANS)
      {
         pDlg->m_BridgeDesc.RemoveSpan(spanIdx,pgsTypes::NextPier);
      }
   }

   FillGrid(pDlg->m_BridgeDesc);
}

bool CBridgeDescFramingGrid::EnableItemDelete()
{
	if (GetParam() == NULL)
		return false;

   if ( GetPierCount() == 2 )
      return false; // can't delete the last span

   CRowColArray selRows;
   ROWCOL nSelRows  = GetSelectedRows(selRows);
   if ( nSelRows != 1 )
      return false; // only one selection at a time

   ROWCOL selRow = selRows.GetAt(0);
   SpanIndexType spanIdx = GetSpanIndex(selRow);

   if ( spanIdx == ALL_SPANS )
      return false;

   return true;
}


void CBridgeDescFramingGrid::CustomInit()
{
// Initialize the grid. For CWnd based grids this call is // 
// essential. For view based grids this initialization is done 
// in OnInitialUpdate.
	Initialize( );

	GetParam( )->EnableUndo(FALSE);
   GetParam()->SetLockReadOnly(FALSE);

   SetMergeCellsMode(gxnMergeDelayEval);

   const int num_rows = 0;
   const int num_cols = 3;

	SetRowCount(num_rows);
	SetColCount(num_cols);

		// Turn off selecting whole columns when clicking on a column header
	GetParam()->EnableSelection((WORD) (GX_SELFULL & ~GX_SELCOL & ~GX_SELTABLE & ~GX_SELMULTIPLE & ~GX_SELSHIFT & ~GX_SELKEYBOARD));

   // no row moving
	GetParam()->EnableMoveRows(FALSE);

   // disable left side
	SetStyleRange(CGXRange(0,0,num_rows,0), CGXStyle()
			.SetControl(GX_IDS_CTRL_HEADER)
			.SetEnabled(FALSE)          // disables usage as current cell
		);

// set text along top row
	SetStyleRange(CGXRange(0,0), CGXStyle()
         .SetWrapText(TRUE)
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetEnabled(FALSE)          // disables usage as current cell
		);

   CString cv = "Station";
	SetStyleRange(CGXRange(0,1), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(cv)
		);

   cv = "Orientation";
	SetStyleRange(CGXRange(0,2), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(cv)
		);

	SetStyleRange(CGXRange(0,3), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue("Connection, Boundary Condition, and Girder Spacing Details")
		);

   // make it so that text fits correctly in header row
	ResizeRowHeightsToFit(CGXRange(0,0,0,num_cols));

   // don't allow users to resize grids
   GetParam( )->EnableTrackColWidth(0); 
   GetParam( )->EnableTrackRowHeight(0); 

	EnableIntelliMouse();
	SetFocus();

   GetParam()->SetLockReadOnly(TRUE);
	GetParam( )->EnableUndo(TRUE);
}

void CBridgeDescFramingGrid::SetPierRowStyle(ROWCOL nRow,const CPierData& pierData)
{
	GetParam()->EnableUndo(FALSE);
   GetParam()->SetLockReadOnly(FALSE);

   // left column header
   SetStyleRange(CGXRange(nRow,0), CGXStyle()
      .SetHorizontalAlignment(DT_LEFT)
   );

   // station
   SetStyleRange(CGXRange(nRow,1), CGXStyle()
      .SetHorizontalAlignment(DT_RIGHT)
   );

   // orientation
   CGXStyle style;
   style.SetHorizontalAlignment(DT_RIGHT);
   style.SetInterior(::GetSysColor(COLOR_WINDOW))
        .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT))
        .SetReadOnly(FALSE)
        .SetEnabled(TRUE);
   
   SetStyleRange(CGXRange(nRow,2), style );

   // edit button
   PierIndexType pierIdx = GetPierIndex(nRow);
   CString strButton;
   if ( pierData.GetPrevSpan() == NULL || pierData.GetNextSpan() == NULL )
      strButton.Format(_T("Edit Abutment %d Details..."),pierIdx+1);
   else
      strButton.Format(_T("Edit Pier %d Details..."),pierIdx+1);

   ASSERT( nRow % 2 != 0 );
   SetStyleRange(CGXRange(nRow,3), CGXStyle()
			.SetControl(GX_IDS_CTRL_PUSHBTN)
			.SetChoiceList(strButton)
         .SetHorizontalAlignment(DT_LEFT)
         .SetEnabled(TRUE)
         .SetItemDataPtr( &PIER_BUTTON )
         );

   GetParam()->SetLockReadOnly(TRUE);
   GetParam()->EnableUndo(TRUE);
}

void CBridgeDescFramingGrid::SetSpanRowStyle(ROWCOL nRow,const CSpanData& spanData)
{
	GetParam()->EnableUndo(FALSE);
   GetParam()->SetLockReadOnly(FALSE);

   // left column header
   SetStyleRange(CGXRange(nRow,0), CGXStyle()
      .SetHorizontalAlignment(DT_RIGHT)
   );

   // edit button
   ASSERT( nRow % 2 == 0 );
   SpanIndexType spanIdx = GetSpanIndex(nRow);
   CString strButton;
   strButton.Format(_T("Edit Span %d Details..."),LABEL_SPAN(spanIdx));
   SetStyleRange(CGXRange(nRow,3), CGXStyle()
			.SetControl(GX_IDS_CTRL_PUSHBTN)
			.SetChoiceList(strButton)
         .SetHorizontalAlignment(DT_RIGHT)
         .SetEnabled(TRUE)
         .SetItemDataPtr( &SPAN_BUTTON )
         );

   GetParam()->SetLockReadOnly(TRUE);
   GetParam()->EnableUndo(TRUE);
}

CString CBridgeDescFramingGrid::GetCellValue(ROWCOL nRow, ROWCOL nCol)
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

CPierData* CBridgeDescFramingGrid::GetPierRowData(ROWCOL nRow)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   CBridgeDescFramingPage* pParent = (CBridgeDescFramingPage*)GetParent();
   ASSERT( pParent->IsKindOf(RUNTIME_CLASS(CBridgeDescFramingPage) ) );

   CBridgeDescDlg* pDlg = (CBridgeDescDlg*)(pParent->GetParent());
   ASSERT( pDlg->IsKindOf(RUNTIME_CLASS(CBridgeDescDlg) ) );

   PierIndexType pierIdx = GetPierIndex(nRow);
   ASSERT( pierIdx != ALL_PIERS );

   CPierData* pPier = pDlg->m_BridgeDesc.GetPier(pierIdx);

   // update for the data in the grid
   // Station
   CString strStation = GetCellValue(nRow,1);
   UnitModeType unitMode = (UnitModeType)(pDisplayUnits->GetUnitMode());
   m_objStation->FromString(CComBSTR(strStation),unitMode);
   Float64 station;
   m_objStation->get_Value(&station);
   pPier->SetStation( ::ConvertToSysUnits(station,pDisplayUnits->GetAlignmentLengthUnit().UnitOfMeasure) );

   // Orientation
   CString strOrientation = GetCellValue(nRow,2);
   strOrientation.MakeUpper();
   strOrientation.TrimLeft();
   strOrientation.TrimRight();
   if ( strOrientation.GetLength() == 1 && strOrientation.GetAt(0) == _T('N'))
   {
      // if user input N for normal, fill out the whole word
      strOrientation = _T("NORMAL");
   }
   pPier->SetOrientation( strOrientation );

   return pPier;
}

void CBridgeDescFramingGrid::FillGrid(const CBridgeDescription& bridgeDesc)
{
	GetParam()->EnableUndo(FALSE);
   GetParam()->SetLockReadOnly(FALSE);

   // remove all rows
   ROWCOL rows = GetRowCount();
   if ( 1 < rows )
      RemoveRows(1, rows);

   const CPierData* pFirstPier = bridgeDesc.GetPier(0);

   // create all the rows
   InsertRow(); // pier row
   
   const CSpanData* pSpanData = pFirstPier->GetNextSpan();
   const CPierData* pPierData = pSpanData->GetNextPier();
   while(pSpanData)
   {
      pPierData = pSpanData->GetNextPier();

      InsertRow(); // span row
      InsertRow(); // pier row

      pSpanData = pPierData->GetNextSpan();
   }
   
   // fill up the rows
   ROWCOL row = 1;
   SetPierRowStyle(row,*pFirstPier);
   FillPierRow(row++,*pFirstPier);

   pSpanData = pFirstPier->GetNextSpan();
   pPierData = pSpanData->GetNextPier();
   while(pSpanData)
   {
      pPierData = pSpanData->GetNextPier();

      SetSpanRowStyle(row,*pSpanData);
      FillSpanRow(row++,*pSpanData);

      SetPierRowStyle(row,*pPierData);
      FillPierRow(row++,*pPierData);

      pSpanData = pPierData->GetNextSpan();
   }

   GetParam()->SetLockReadOnly(TRUE);
	GetParam()->EnableUndo(TRUE);
}

void CBridgeDescFramingGrid::FillPierRow(ROWCOL row,const CPierData& pierData)
{
	GetParam()->EnableUndo(FALSE);
   GetParam()->SetLockReadOnly(FALSE);


   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   CString strStation = FormatStation(pDisplayUnits->GetStationFormat(),pierData.GetStation());

   CString strPierLabel;
   if ( pierData.GetPrevSpan() == NULL || pierData.GetNextSpan() == NULL )
   {
      strPierLabel.Format(_T("Abut %d"),pierData.GetPierIndex()+1);
   }
   else
   {
      strPierLabel.Format(_T("Pier %d"),pierData.GetPierIndex()+1);
   }

   SetValueRange(CGXRange(row,0),strPierLabel);
   SetValueRange(CGXRange(row,1),strStation);
   SetValueRange(CGXRange(row,2),pierData.GetOrientation());

   GetParam()->SetLockReadOnly(TRUE);
	GetParam()->EnableUndo(TRUE);
}

void CBridgeDescFramingGrid::FillSpanRow(ROWCOL row,const CSpanData& spanData)
{
	GetParam()->EnableUndo(FALSE);
   GetParam()->SetLockReadOnly(FALSE);

	GetParam()->EnableUndo(FALSE);
   GetParam()->SetLockReadOnly(FALSE);

   CString strSpanLabel;
   strSpanLabel.Format(_T("Span %d"),LABEL_SPAN(spanData.GetSpanIndex()));
   SetValueRange(CGXRange(row,0),strSpanLabel);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   CString strSpanLength;
   Float64 spanLength = spanData.GetSpanLength();
   strSpanLength.Format(_T("%s"),FormatDimension(spanLength,pDisplayUnits->GetSpanLengthUnit()));

   SetStyleRange(CGXRange(row,1,row,2), CGXStyle()
      .SetMergeCell(GX_MERGE_HORIZONTAL)
      .SetHorizontalAlignment(DT_CENTER)
      .SetInterior(::GetSysColor(COLOR_BTNFACE)) // dialog face color
      .SetTextColor(spanLength <= 0 ? RGB(255,0,0) : ::GetSysColor(COLOR_GRAYTEXT))
      .SetReadOnly(TRUE)
      .SetEnabled(FALSE)
      .SetValue(strSpanLength)
      );

   GetParam()->SetLockReadOnly(TRUE);
	GetParam()->EnableUndo(TRUE);

   GetParam()->SetLockReadOnly(TRUE);
	GetParam()->EnableUndo(TRUE);
}

void CBridgeDescFramingGrid::GetGridData()
{
   ROWCOL nRows = GetRowCount();
   for ( ROWCOL row = 1; row <= nRows; row++ )
   {
      PierIndexType pierIdx = GetPierIndex(row);

      if ( pierIdx != ALL_PIERS )
      {
         CPierData* pPier = GetPierRowData(row);
      }
   }
}

void CBridgeDescFramingGrid::OnClickedButtonRowCol(ROWCOL nRow,ROWCOL nCol)
{
   if ( nCol != 3 )
      return;
 
   CBridgeDescFramingPage* pParent = (CBridgeDescFramingPage*)GetParent();
   ASSERT( pParent->IsKindOf(RUNTIME_CLASS(CBridgeDescFramingPage) ) );
   pParent->UpdateData();

   CGXStyle style;
   GetStyleRowCol(nRow,3,style);

   if ( *(int*)(style.GetItemDataPtr()) == SPAN_BUTTON )
   {
      SpanIndexType spanIdx = GetSpanIndex(nRow);
      EditSpan(spanIdx);
   }
   else
   {
      PierIndexType pierIdx = GetPierIndex(nRow);
      EditPier(pierIdx);
   }
}

// validate input
BOOL CBridgeDescFramingGrid::OnValidateCell(ROWCOL nRow, ROWCOL nCol)
{
	CString s;
	CGXControl* pControl = GetControl(nRow, nCol);
	pControl->GetCurrentText(s);

	if (nCol==1)
	{
      // station
      CComPtr<IStation> objStation;
      HRESULT hr = objStation.CoCreateInstance(CLSID_Station);
      ASSERT(SUCCEEDED(hr));

      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);
      GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

      hr = objStation->FromString( CComBSTR(s), (UnitModeType)(pDisplayUnits->GetUnitMode()));
      if ( FAILED(hr) )
      {
			SetWarningText (_T("Invalid Station Value"));
         return FALSE;
      }
      else
      {
         return TRUE;
      }
   }
	else if (nCol==2)
	{
         // orientation
      CString strOrientation(s);
      strOrientation.MakeUpper(); // do comparisons in upper case

      if (strOrientation.IsEmpty())
      {
			SetWarningText (_T("Orientation must not be blank"));
         return FALSE;
      }

      if ( strOrientation == _T("NORMAL") || (strOrientation.GetLength() == 1 && strOrientation[0] == _T('N')) )
         return TRUE;

      CComPtr<IAngle> angle;
      angle.CoCreateInstance(CLSID_Angle);
      HRESULT hr_angle = angle->FromString(CComBSTR(strOrientation));
      if ( SUCCEEDED(hr_angle) )
      {
         Float64 value;
         angle->get_Value(&value);
         if ( value < -MAX_SKEW_ANGLE || MAX_SKEW_ANGLE < value )
         {
   		   SetWarningText (_T("Skew angle must be less than 88 deg"));
            return FALSE;
         }
         else
         {
            return TRUE;
         }
      }

      CComPtr<IDirection> direction;
      direction.CoCreateInstance(CLSID_Direction);
      HRESULT hr_direction = direction->FromString(CComBSTR( strOrientation ));
      if ( SUCCEEDED(hr_direction) )
      {
         return TRUE;
      }
      else
      {
 		   SetWarningText (_T("Orientation is invalid"));
         return FALSE;
      }
   }

	return CGXGridWnd::OnValidateCell(nRow, nCol);
}

BOOL CBridgeDescFramingGrid::OnEndEditing(ROWCOL nRow,ROWCOL nCol)
{
   if ( nCol == 1 )
   {
      // Station changed... update span lengths
      CPierData* pPier = GetPierRowData(nRow);

      if ( pPier->GetPrevSpan() )
         FillSpanRow(nRow-1,*pPier->GetPrevSpan());

      if ( pPier->GetNextSpan() )
         FillSpanRow(nRow+1,*pPier->GetNextSpan());
   }
   else if ( nCol == 2 )
   {
      // pier orientation is changed... if there are target piers and their orientation is linked... update the orientation in the grid
      CPierData* pPier = GetPierRowData(nRow);
   }
   return CGXGridWnd::OnEndEditing(nRow,nCol);
}

PierIndexType CBridgeDescFramingGrid::GetPierCount()
{
   ROWCOL nRows = GetRowCount();
   PierIndexType nPiers = PierIndexType(nRows/2 + 1);
   return nPiers;
}

BOOL CBridgeDescFramingGrid::CanActivateGrid(BOOL bActivate)
{
   if ( !bActivate )
   {
      // make sure all the grid data is active
      std::vector<Float64> spanLengths = GetSpanLengths();
      std::vector<Float64>::iterator iter;
      for ( iter = spanLengths.begin(); iter != spanLengths.end(); iter++ )
      {
         Float64 L = *iter;
         if ( L <= 0 )
         {
   			AfxMessageBox(_T("Invalid Span Length"));
            return FALSE;
         }
      }
   }

   return CGXGridWnd::CanActivateGrid(bActivate);
}

void CBridgeDescFramingGrid::EditSpan(SpanIndexType spanIdx)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CBridgeDescFramingPage* pParent = (CBridgeDescFramingPage*)GetParent();
   ASSERT( pParent->IsKindOf(RUNTIME_CLASS(CBridgeDescFramingPage) ) );

   CBridgeDescDlg* pDlg = (CBridgeDescDlg*)(pParent->GetParent());
   ASSERT( pDlg->IsKindOf(RUNTIME_CLASS(CBridgeDescDlg) ) );

   CSpanData* pSpanData = pDlg->m_BridgeDesc.GetSpan(spanIdx);
   CPierData* pPrevPier = pSpanData->GetPrevPier();
   CPierData* pNextPier = pSpanData->GetNextPier();

   CSpanDetailsDlg dlg(pSpanData);

   if ( dlg.DoModal() == IDOK )
   {
      pDlg->m_BridgeDesc.UseSameGirderForEntireBridge( dlg.UseSameGirderType() );
      pDlg->m_BridgeDesc.UseSameNumberOfGirdersInAllSpans( dlg.UseSameNumGirders() );
      
      pDlg->m_BridgeDesc.SetGirderSpacingType( dlg.GetGirderSpacingType() );
      pDlg->m_BridgeDesc.SetMeasurementLocation( dlg.GetMeasurementLocation() );


      if ( pPrevPier->GetPrevSpan() )
         GetConnectionData(pPrevPier,pgsTypes::Back,dlg);

      GetConnectionData(pPrevPier,pgsTypes::Ahead,dlg);
      GetConnectionData(pNextPier,pgsTypes::Back,dlg);

      if ( pNextPier->GetNextSpan() )
         GetConnectionData(pNextPier,pgsTypes::Ahead,dlg);



      pPrevPier->SetConnectionType( dlg.GetConnectionType(pgsTypes::Ahead) );
      pNextPier->SetConnectionType( dlg.GetConnectionType(pgsTypes::Back)  );



      if ( IsGirderSpacing(dlg.GetGirderSpacingType()) && 1 < dlg.GetGirderCount() )
      {
         pDlg->m_BridgeDesc.SetGirderSpacing( dlg.GetGirderSpacing(pgsTypes::metStart).GetGirderSpacing(0) );
      }
      *pSpanData->GetGirderSpacing(pgsTypes::metStart) = dlg.GetGirderSpacing(pgsTypes::metStart);
      *pSpanData->GetGirderSpacing(pgsTypes::metEnd)   = dlg.GetGirderSpacing(pgsTypes::metEnd);
      
      if ( dlg.UseSameGirderType() )
      {
         pDlg->m_BridgeDesc.SetGirderName( dlg.GetGirderTypes().GetGirderName(0) );
      }
      pSpanData->SetGirderTypes( dlg.GetGirderTypes() );

      if ( dlg.UseSameNumGirders() )
      {
         // set girder count for entire bridge
         pDlg->m_BridgeDesc.SetGirderCount( dlg.GetGirderCount() );
      }
      // set girder count for this span
      pSpanData->SetGirderCount( dlg.GetGirderCount() );

      pDlg->m_BridgeDesc.SetSpanLength(spanIdx,dlg.GetSpanLength());

      pDlg->m_BridgeDesc.SetSlabOffsetType( dlg.GetSlabOffsetType() );
      pSpanData->SetSlabOffset(pgsTypes::metStart,dlg.GetSlabOffset(pgsTypes::metStart));
      pSpanData->SetSlabOffset(pgsTypes::metEnd,  dlg.GetSlabOffset(pgsTypes::metEnd));

      FillGrid(pDlg->m_BridgeDesc);
   }
}

void CBridgeDescFramingGrid::GetConnectionData(CPierData* pPier,pgsTypes::PierFaceType pierFace,CSpanDetailsDlg& dlg)
{
   pgsTypes::MemberEndType end = (pgsTypes::MemberEndType)(pierFace);
   pPier->SetConnectionType(dlg.GetConnectionType(end));
   pPier->SetDiaphragmHeight(pierFace,dlg.GetDiaphragmHeight(end));
   pPier->SetDiaphragmWidth(pierFace,dlg.GetDiaphragmWidth(end));
   pPier->SetDiaphragmLoadType(pierFace,dlg.GetDiaphragmLoadType(end));
   pPier->SetDiaphragmLoadLocation(pierFace,dlg.GetDiaphragmLoadLocation(end));
   pPier->SetGirderEndDistance(pierFace,dlg.GetEndDistance(end,pierFace),dlg.GetEndDistanceMeasurementType(end,pierFace));
   pPier->SetBearingOffset(pierFace,dlg.GetBearingOffset(end,pierFace),dlg.GetBearingOffsetMeasurementType(end,pierFace));
   pPier->SetSupportWidth(pierFace,dlg.GetSupportWidth(end,pierFace));
}

void CBridgeDescFramingGrid::EditPier(PierIndexType pierIdx)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   ROWCOL nRow = GetPierRow(pierIdx);

   CBridgeDescFramingPage* pParent = (CBridgeDescFramingPage*)GetParent();
   ASSERT( pParent->IsKindOf(RUNTIME_CLASS(CBridgeDescFramingPage) ) );

   CBridgeDescDlg* pDlg = (CBridgeDescDlg*)(pParent->GetParent());
   ASSERT( pDlg->IsKindOf(RUNTIME_CLASS(CBridgeDescDlg) ) );

   CPierData* pPierData = pDlg->m_BridgeDesc.GetPier(pierIdx);

   CPierDetailsDlg dlg(pPierData);

   Float64 oldStation = pPierData->GetStation();

   if ( dlg.DoModal() == IDOK )
   {
      pPierData->SetOrientation( dlg.GetOrientation() );
      pPierData->SetConnectionType( dlg.GetConnectionType()  );

      if ( pPierData->GetPrevSpan() )
      {
         pPierData->SetGirderEndDistance(pgsTypes::Back,dlg.GetEndDistance(pgsTypes::Back),dlg.GetEndDistanceMeasurementType(pgsTypes::Back));
         pPierData->SetBearingOffset(pgsTypes::Back,dlg.GetBearingOffset(pgsTypes::Back),dlg.GetBearingOffsetMeasurementType(pgsTypes::Back));
         pPierData->SetSupportWidth(pgsTypes::Back,dlg.GetSupportWidth(pgsTypes::Back));
         pPierData->SetDiaphragmHeight(pgsTypes::Back,dlg.GetDiaphragmHeight(pgsTypes::Back));
         pPierData->SetDiaphragmWidth(pgsTypes::Back,dlg.GetDiaphragmHeight(pgsTypes::Back));
         pPierData->SetDiaphragmLoadType(pgsTypes::Back,dlg.GetDiaphragmLoadType(pgsTypes::Back));
         pPierData->SetDiaphragmLoadLocation(pgsTypes::Back,dlg.GetDiaphragmLoadLocation(pgsTypes::Back));
      }

      if ( pPierData->GetNextSpan() )
      {
         pPierData->SetGirderEndDistance(pgsTypes::Ahead,dlg.GetEndDistance(pgsTypes::Ahead),dlg.GetEndDistanceMeasurementType(pgsTypes::Ahead));
         pPierData->SetBearingOffset(pgsTypes::Ahead,dlg.GetBearingOffset(pgsTypes::Ahead),dlg.GetBearingOffsetMeasurementType(pgsTypes::Ahead));
         pPierData->SetSupportWidth(pgsTypes::Ahead,dlg.GetSupportWidth(pgsTypes::Ahead));
         pPierData->SetDiaphragmHeight(pgsTypes::Ahead,dlg.GetDiaphragmHeight(pgsTypes::Ahead));
         pPierData->SetDiaphragmWidth(pgsTypes::Ahead,dlg.GetDiaphragmHeight(pgsTypes::Ahead));
         pPierData->SetDiaphragmLoadType(pgsTypes::Ahead,dlg.GetDiaphragmLoadType(pgsTypes::Ahead));
         pPierData->SetDiaphragmLoadLocation(pgsTypes::Ahead,dlg.GetDiaphragmLoadLocation(pgsTypes::Ahead));
      }

      GirderIndexType nGirders = 9999;

      if ( dlg.UseSameNumberOfGirdersInAllSpans() )
      {
         if ( pPierData->GetPrevSpan() )
            nGirders = dlg.GetGirderCount(pgsTypes::Back);
         else
            nGirders = dlg.GetGirderCount(pgsTypes::Ahead);

         pDlg->m_BridgeDesc.SetGirderCount( nGirders );
      }
      else
      {
         if ( pPierData->GetPrevSpan() )
         {
            nGirders = 9999;
            nGirders = _cpp_min(nGirders,dlg.GetGirderCount(pgsTypes::Back));
            pPierData->GetPrevSpan()->SetGirderCount( nGirders );
         }

         if ( pPierData->GetNextSpan() )
         {
            nGirders = 9999;
            nGirders = _cpp_min(nGirders,dlg.GetGirderCount(pgsTypes::Ahead));
            pPierData->GetNextSpan()->SetGirderCount( nGirders );
         }
      }

      pDlg->m_BridgeDesc.UseSameNumberOfGirdersInAllSpans( dlg.UseSameNumberOfGirdersInAllSpans() );

      if ( 2 <= nGirders )
      {
         if ( IsSpanSpacing(dlg.GetSpacingType()) )
         {
            // spacing is span by span
            if ( pPierData->GetPrevSpan() )
            {
               CGirderSpacing girderSpacing( dlg.GetSpacing(pgsTypes::Back) );
               *pPierData->GetPrevSpan()->GetGirderSpacing(pgsTypes::metEnd) = girderSpacing;
            }

            if ( pPierData->GetNextSpan() )
            {
               CGirderSpacing girderSpacing( dlg.GetSpacing(pgsTypes::Ahead) );
               *pPierData->GetNextSpan()->GetGirderSpacing(pgsTypes::metStart) = girderSpacing;
            }
         }
         else
         {
            // one spacing for the entire bridge
            if ( pPierData->GetPrevSpan() )
               pDlg->m_BridgeDesc.SetGirderSpacing( dlg.GetSpacing(pgsTypes::Back).GetGirderSpacing(0) );
            else
               pDlg->m_BridgeDesc.SetGirderSpacing( dlg.GetSpacing(pgsTypes::Ahead).GetGirderSpacing(0) );
         }
      }

      pDlg->m_BridgeDesc.SetGirderSpacingType( dlg.GetSpacingType() );
      pDlg->m_BridgeDesc.SetMeasurementLocation( dlg.GetMeasurementLocation() );
      pDlg->m_BridgeDesc.MovePier(pierIdx,dlg.GetStation(),dlg.GetMovePierOption());

      FillGrid(pDlg->m_BridgeDesc);
   }
}

ROWCOL CBridgeDescFramingGrid::GetSpanRow(SpanIndexType spanIdx)
{
   return ROWCOL(spanIdx*2 + 2);
}

ROWCOL CBridgeDescFramingGrid::GetPierRow(PierIndexType pierIdx)
{
   return ROWCOL(pierIdx*2 + 1);
}

SpanIndexType CBridgeDescFramingGrid::GetSpanIndex(ROWCOL nRow)
{
   if ( nRow % 2 != 0 )
      return ALL_SPANS; 

   return SpanIndexType(nRow/2 - 1);
}

PierIndexType CBridgeDescFramingGrid::GetPierIndex(ROWCOL nRow)
{
   if ( nRow % 2 == 0 )
      return ALL_PIERS; 

   return PierIndexType(nRow/2);
}
