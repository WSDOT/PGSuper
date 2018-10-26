///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2012  Washington State Department of Transportation
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

// GirderNameGrid.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperDoc.h"
#include "PGSuperAppPlugin\resource.h"
#include "GirderNameGrid.h"

#include <PgsExt\BridgeDescription.h>
#include <IFace\Project.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

GRID_IMPLEMENT_REGISTER(CGirderNameGrid, CS_DBLCLKS, 0, 0, 0);

/////////////////////////////////////////////////////////////////////////////
// CGirderNameGrid

CGirderNameGrid::CGirderNameGrid()
{
//   RegisterClass();
   m_bEnabled = TRUE;
}

CGirderNameGrid::~CGirderNameGrid()
{
}

BEGIN_MESSAGE_MAP(CGirderNameGrid, CGXGridWnd)
	//{{AFX_MSG_MAP(CGirderNameGrid)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	ON_COMMAND(IDC_EXPAND, OnExpand)
	ON_COMMAND(IDC_JOIN, OnJoin)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CGirderNameGrid message handlers

void CGirderNameGrid::CustomInit(const CSpanData* pSpanData)
{
// Initialize the grid. For CWnd based grids this call is // 
// essential. For view based grids this initialization is done 
// in OnInitialUpdate.
	Initialize( );

   UpdateGirderFamilyList(pSpanData->GetBridgeDescription()->GetGirderFamilyName());

   GetParam()->EnableUndo(FALSE);

		// Turn off selecting whole columns when clicking on a column header
	GetParam()->EnableSelection((WORD) (GX_SELFULL & ~GX_SELTABLE & ~GX_SELROW));

   // no row moving
	GetParam()->EnableMoveRows(FALSE);
   GetParam()->EnableMoveCols(FALSE);

   m_bSameGirderName = pSpanData->GetBridgeDescription()->UseSameGirderForEntireBridge();
   m_bSameNumGirders = pSpanData->GetBridgeDescription()->UseSameNumberOfGirdersInAllSpans();
   m_GirderTypes     = *pSpanData->GetGirderTypes();
//   m_GirderTypes.SetSpan(NULL);

   FillGrid();

   // don't allow users to resize grids
   GetParam( )->EnableTrackColWidth(0); 
   GetParam( )->EnableTrackRowHeight(0); 

	EnableIntelliMouse();

   EnableGridToolTips();
   SetStyleRange(CGXRange(0,0,GetRowCount(),GetColCount()),
      CGXStyle()
      .SetWrapText(TRUE)
      .SetAutoSize(TRUE)
      .SetUserAttribute(GX_IDS_UA_TOOLTIPTEXT,"To regroup girders, select column headings, right click over the grid, and select Expand or Join from the menu")
      );
	
   GetParam()->EnableUndo(TRUE);

	SetFocus();
}


void CGirderNameGrid::FillGrid()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,ILibrary,pLib);

	GetParam()->EnableUndo(FALSE);
   GetParam()->SetLockReadOnly(FALSE);

   GroupIndexType nGirderGroups = m_GirderTypes.GetGirderGroupCount();
   GirderIndexType nGirders     = m_GirderTypes.GetGirderCount();

   const int num_rows = 1;
   const int num_cols = (int)nGirderGroups;

   SetRowCount(num_rows);
	SetColCount(num_cols);

   // disable left side
	SetStyleRange(CGXRange(0,0,num_rows,0), CGXStyle()
			.SetControl(GX_IDS_CTRL_HEADER)
			.SetEnabled(FALSE)          // disables usage as current cell
		);


   for ( GroupIndexType grpIdx = 0; grpIdx < nGirderGroups; grpIdx++ )
   {
      std::_tstring strName;
      GirderIndexType firstGdrIdx, lastGdrIdx;

      m_GirderTypes.GetGirderGroup(grpIdx,&firstGdrIdx,&lastGdrIdx,strName);

      // if this is the last group and the girder names are shared but the girder count is not shared
      // then the last girder index needs to be forced to nGirders-1
      if ( grpIdx == nGirderGroups-1 && m_bSameGirderName && !m_bSameNumGirders )
         lastGdrIdx = nGirders-1;

      CString strHeading;
      if ( firstGdrIdx == lastGdrIdx )
      {
         strHeading.Format(_T("%s"),LABEL_GIRDER(firstGdrIdx));
      }
      else
      {
         strHeading.Format(_T("%s-%s"),LABEL_GIRDER(firstGdrIdx),LABEL_GIRDER(lastGdrIdx));
      }

      UserData* pUserData = new UserData(firstGdrIdx,lastGdrIdx); // the grid will delete this

      SetStyleRange(CGXRange(0,ROWCOL(grpIdx+1)), CGXStyle()
         .SetHorizontalAlignment(DT_CENTER)
         .SetEnabled(FALSE)
         .SetValue(strHeading)
         .SetItemDataPtr((void*)pUserData)
         );

      SetStyleRange(CGXRange(1,ROWCOL(grpIdx+1)), CGXStyle()
         .SetHorizontalAlignment(DT_RIGHT)
         .SetEnabled(TRUE)
         .SetControl(GX_IDS_CTRL_CBS_DROPDOWNLIST)
         .SetChoiceList(m_GirderList)
         );

      const GirderLibraryEntry* pGdrEntry = pLib->GetGirderEntry( strName.c_str() );
      CString strGirderFamilyName = pGdrEntry->GetGirderFamilyName().c_str();

      // of the girder is from the girder family, use it's name, otherwise use the 
      // first name in the girder list
      if ( strGirderFamilyName == m_strGirderFamilyName )
      {
         SetStyleRange(CGXRange(1,ROWCOL(grpIdx+1)), CGXStyle()
            .SetValue(strName.c_str())
            );
      }
      else
      {
         SetStyleRange(CGXRange(1,ROWCOL(grpIdx+1)), CGXStyle()
            .SetValue(m_GirderList.Left(m_GirderList.FindOneOf(_T("\n"))))
            );
      }

   }

   SetStyleRange(CGXRange(0,0), CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetEnabled(FALSE)
      .SetReadOnly(TRUE)
      .SetValue(_T("Girder"))
      );

   SetStyleRange(CGXRange(1,0), CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetEnabled(FALSE)
      .SetReadOnly(TRUE)
      .SetValue(_T("Name"))
      );

   // make it so that text fits correctly in header row
	ResizeRowHeightsToFit(CGXRange(0,0,num_rows,num_cols));
   ResizeColWidthsToFit(CGXRange(0,0,num_rows,num_cols));

   // this will get the styles of all the cells to be correct if the size of the grid changed
   Enable(m_bEnabled);

   GetParam()->SetLockReadOnly(TRUE);
	GetParam()->EnableUndo(TRUE);
}

void CGirderNameGrid::AddGirders(GirderIndexType nGirders)
{
   m_GirderTypes.AddGirders(nGirders);
   FillGrid();
}

void CGirderNameGrid::RemoveGirders(GirderIndexType nGirders)
{
   m_GirderTypes.RemoveGirders(nGirders);
   FillGrid();
}

BOOL CGirderNameGrid::OnRButtonHitRowCol(ROWCOL nHitRow,ROWCOL nHitCol,ROWCOL nDragRow,ROWCOL nDragCol,CPoint point,UINT nFlags,WORD nHitState)
{
   if ( nHitState & GX_HITSTART )
      return TRUE;

   if ( nHitCol != 0 )
   {
      CRowColArray selCols;
      ROWCOL nSelected = GetSelectedCols(selCols);

      if ( 0 < nSelected )
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

void CGirderNameGrid::UseSameGirderName(bool bSame)
{
   m_bSameGirderName = bSame;
   m_GirderTypes.SetSpan(NULL);
   FillGrid();
}

void CGirderNameGrid::UseSameNumGirders(bool bSame)
{
   m_bSameNumGirders = bSame;
   FillGrid();
}

void CGirderNameGrid::OnExpand()
{
   CRowColArray selCols;
   ROWCOL nSelCols = GetSelectedCols(selCols);

   if ( nSelCols == 0 )
      return;

   if ( nSelCols == GetColCount() )
   {
      m_GirderTypes.ExpandAll();
   }
   else
   {
      for ( int i = nSelCols-1; 0 <= i; i-- )
      {
         ROWCOL col = selCols[i];
         GroupIndexType grpIdx = (GroupIndexType)col-1;

         m_GirderTypes.Expand(grpIdx);
      }
   }

   FillGrid();
}

void CGirderNameGrid::OnJoin()
{
   CRowColArray selLeftCols,selRightCols;
   GetSelectedCols(selLeftCols,selRightCols,FALSE,FALSE);
   ROWCOL nSelRanges = (ROWCOL)selLeftCols.GetSize();
   ASSERT( 0 < nSelRanges ); // must be more that one selected column to join

   for (ROWCOL i = 0; i < nSelRanges; i++ )
   {
      ROWCOL firstCol = selLeftCols[i];
      ROWCOL lastCol  = selRightCols[i];

      CGXStyle firstColStyle, lastColStyle;
      GetStyleRowCol(0,firstCol,firstColStyle);
      GetStyleRowCol(0,lastCol, lastColStyle);

      UserData* pFirstColUserData = (UserData*)firstColStyle.GetItemDataPtr();
      UserData* pLastColUserData =  (UserData*)lastColStyle.GetItemDataPtr();

      GirderIndexType firstGdrIdx = pFirstColUserData->first;
      GirderIndexType lastGdrIdx  = pLastColUserData->second;

      m_GirderTypes.Join(firstGdrIdx,lastGdrIdx,firstGdrIdx);
  }

   FillGrid();
}

BOOL CGirderNameGrid::OnEndEditing(ROWCOL nRow,ROWCOL nCol)
{
   if ( nRow != 1 || nCol == 0)
      return CGXGridWnd::OnEndEditing(nRow,nCol);

   GroupIndexType grpIdx = GroupIndexType(nCol - 1);

   CString strNewName;
   GetCurrentCellControl()->GetCurrentText(strNewName);

   ASSERT( !m_bSameGirderName );

   GirderIndexType firstGdrIdx,lastGdrIdx;
   std::_tstring strName;
   m_GirderTypes.GetGirderGroup(grpIdx,&firstGdrIdx,&lastGdrIdx,strName);

   if ( strNewName != CString(strName.c_str()) )
   {
      m_GirderTypes.SetGirderName(grpIdx,strNewName);

      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);
      GET_IFACE2(pBroker,ILibrary,pLib);
      const GirderLibraryEntry* pGdrEntry = pLib->GetGirderEntry( strNewName );
      
      m_GirderTypes.SetGirderLibraryEntry(grpIdx,pGdrEntry);
   }

   return CGXGridWnd::OnEndEditing(nRow,nCol);
}

void CGirderNameGrid::UpdateGirderFamilyList(LPCTSTR strGirderFamily)
{
   // fill the girder list
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2( pBroker, ILibraryNames, pLibNames );
   std::vector<std::_tstring> names;
   std::vector<std::_tstring>::iterator iter;

   m_strGirderFamilyName = strGirderFamily;

   m_GirderList.Empty();
   
   pLibNames->EnumGirderNames(m_strGirderFamilyName, &names );
   for ( iter = names.begin(); iter < names.end(); iter++ )
   {
      std::_tstring& name = *iter;
      m_GirderList += CString(name.c_str());
      m_GirderList += CString(_T("\n"));
   }
}

void CGirderNameGrid::OnGirderFamilyChanged(LPCTSTR strGirderFamily)
{
   UpdateGirderFamilyList(strGirderFamily);
   FillGrid();

   // force the data structure to be updated with the new girder type
   // based on the new girder family
   ROWCOL nCols = GetColCount();
   for (ROWCOL col = 1; col <= nCols; col++ )
   {
      SetCurrentCell(1,col);
      OnEndEditing(1,col);
   }
}

void CGirderNameGrid::Enable(BOOL bEnable)
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

      if ( 0 < GetColCount() )
      {
         range = CGXRange(1,1,1,GetColCount());
         SetStyleRange(range,style);
      }

      style.SetInterior(::GetSysColor(COLOR_BTNFACE));

      range = CGXRange(0,0,0,GetColCount());
      SetStyleRange(range,style);

      range = CGXRange(0,0,_cpp_min(GetRowCount(),(ROWCOL)1),0);
      SetStyleRange(range,style);
   }
   else
   {
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
