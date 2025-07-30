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

// 
// Do not include stdafx.h here - this control is to be shared among multiple dll projects
#include <WBFLVersion.h>

#include <afxwin.h>
#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdisp.h>
#endif // _AFX_NO_OLE_SUPPORT

#include "SharedCTrls\MultiGirderSelectGrid.h" 
#include <PsgLib\GirderLabel.h>
#include <PsgLib\Keys.h>

#include <EAF\EAFUtilities.h>
#include <IFace\Tools.h>
#include <IFace\DocumentType.h>

#if defined _DEBUG
#include <IFace\Bridge.h>
#endif


//GRID_IMPLEMENT_REGISTER(CMultiGirderSelectGrid, CS_DBLCLKS, 0, 0, 0);






/////////////////////////////////////////////////////////////////////////////
// CMultiGirderSelectGrid

CMultiGirderSelectGrid::CMultiGirderSelectGrid()
{
//   RegisterClass();
}


void CMultiGirderSelectGrid::CustomInit(const GroupGirderOnCollection& groupGirderCollection, std::_tstring(*pGetGirderLabel)(GirderIndexType))
{
// Initialize the grid. For CWnd based grids this call is // 
// essential. For view based grids this initialization is done 
// in OnInitialUpdate. 

	Initialize( );

	GetParam( )->EnableUndo(FALSE);

   GroupIndexType nGroups = groupGirderCollection.size();


   auto pBroker = EAFGetBroker();

#if defined _DEBUG
   GET_IFACE2(pBroker,IBridge,pBridge);
   ATLASSERT(pBridge->GetGirderGroupCount() == nGroups);
#endif

   // Determine max number of girders
   GirderIndexType max_gdrs = 0;
   for (GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++)
   {
      max_gdrs = Max( max_gdrs, (GirderIndexType)groupGirderCollection[grpIdx].size());
   }

   const ROWCOL num_rows = (ROWCOL)max_gdrs;
   const ROWCOL num_cols = (ROWCOL)nGroups;

	SetRowCount(num_rows);
	SetColCount(num_cols);

   // mimic excel selection behavior
   GetParam()->SetExcelLikeSelectionFrame(TRUE);

   // no row or column moving
	GetParam()->EnableMoveRows(FALSE);
	GetParam()->EnableMoveCols(FALSE);

   // disable left side
	SetStyleRange(CGXRange(0,0,num_rows,0), CGXStyle()
			.SetControl(GX_IDS_CTRL_HEADER)
         .SetHorizontalAlignment(DT_CENTER)
			.SetEnabled(FALSE)          // disables usage as current cell
		);

   // disable top row
	SetStyleRange(CGXRange(0,0,0,num_cols), CGXStyle()
         .SetWrapText(TRUE)
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetEnabled(FALSE)          // disables usage as current cell
		);

   // top row labels
   GET_IFACE2(pBroker,IDocumentType,pDocType);
   bool bIsPGSuper = pDocType->IsPGSuperDocument();

   for (GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++)
   {
      CString lbl;
      if (bIsPGSuper)
      {
         lbl.Format(_T("Span %s"), LABEL_SPAN(grpIdx));
      }
      else
      {
         lbl.Format(_T("Group %d"), LABEL_GROUP(grpIdx));
      }

	   SetStyleRange(CGXRange(0,ROWCOL(grpIdx+1)), CGXStyle()
            .SetWrapText(TRUE)
			   .SetEnabled(FALSE)          // disables usage as current cell
            .SetHorizontalAlignment(DT_CENTER)
            .SetVerticalAlignment(DT_VCENTER)
			   .SetValue(lbl)
		   );
   }

   // left column labels
   for (GirderIndexType ig=0; ig<max_gdrs; ig++)
   {
      CString lbl;
      lbl.Format(_T("Girder %s"), LABEL_GIRDER(ig));

	   SetStyleRange(CGXRange(ROWCOL(ig+1),0), CGXStyle()
            .SetWrapText(TRUE)
			   .SetEnabled(FALSE)          // disables usage as current cell
            .SetHorizontalAlignment(DT_CENTER)
            .SetVerticalAlignment(DT_VCENTER)
			   .SetValue(lbl)
		   );
   }

   // fill controls for girders
   for (GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++)
   {
      GirderIndexType nGirders = groupGirderCollection[grpIdx].size();

      for (GirderIndexType gdrIdx = 0; gdrIdx < max_gdrs; gdrIdx++)
      {
         ROWCOL nCol = ROWCOL(grpIdx+1);
         ROWCOL nRow = ROWCOL(gdrIdx+1);

         if (gdrIdx < nGirders)
         {

            UINT enabled = groupGirderCollection[grpIdx][gdrIdx] ? 1 : 0;
            // unchecked check box for girder
            SetStyleRange(CGXRange(nRow,nCol), CGXStyle()
			         .SetControl(GX_IDS_CTRL_CHECKBOX3D)  //
			         .SetValue(enabled)
                  .SetHorizontalAlignment(DT_CENTER)
                  );
         }
         else
         {
            // no girder here - disable cell
            SetStyleRange(CGXRange(nRow,nCol), CGXStyle()
			      .SetEnabled(FALSE)
               );
         }
      }
   }

   // Make it so that text fits correctly in header rows/cols
   ResizeColWidthsToFit(CGXRange(0,0,num_rows,num_cols));

   // don't allow users to resize grids
//   GetParam( )->EnableTrackColWidth(0); 
//   GetParam( )->EnableTrackRowHeight(0); 

	EnableIntelliMouse();

   // Get grid started in dialog navigation:
	SetFocus();
   SetCurrentCell(1,1);

	GetParam( )->EnableUndo(TRUE);
}


std::vector<CGirderKey> CMultiGirderSelectGrid::GetData()
{
    std::vector<CGirderKey> data;

    ROWCOL nRows = GetRowCount();
    ROWCOL nCols = GetColCount();

    for (ROWCOL col = 0; col < nCols; col++)
    {
        for (ROWCOL row = 0; row < nRows; row++)
        {
            bool bDat = GetCellValue(row + 1, col + 1);

            if (bDat)
            {
                CGirderKey girderKey(col, row);
                data.push_back(girderKey);
            }
        }
    }

    return data;
}

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

