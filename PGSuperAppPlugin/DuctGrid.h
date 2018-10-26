///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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

#pragma once

#include <PgsExt\PTData.h>

//DuctGrid.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDuctGrid window

class CDuctGrid : public CGXGridWnd
{
	GRID_DECLARE_REGISTER()
// Construction
public:
	CDuctGrid();

public:
   // custom stuff for grid
   void CustomInit(CSplicedGirderData* pGirder);

   void AddDuct(EventIndexType stressingEvent);
   void DeleteDuct();

   void UpdatePTData();
   void FillGrid();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDuctGrid)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CDuctGrid();

   void OnStrandChanged(); // called by parent when strand type changes
   void OnInstallationTypeChanged();

	// Generated message map functions
protected:
	//{{AFX_MSG(CDuctGrid)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   CPTData* m_pPTData;
   EventIndexType m_CreateEventIndex;
   EventIndexType m_PrevStressTendonEventIdx;

   void AddDuct(const CDuctData& duct,EventIndexType stressingEvent);

   virtual int GetColWidth(ROWCOL nCol);
   virtual BOOL OnLButtonHitRowCol(ROWCOL nHitRow,ROWCOL nHitCol,ROWCOL nDragRow,ROWCOL nDragCol,CPoint point,UINT flags,WORD nHitState);
   virtual void OnModifyCell(ROWCOL nRow,ROWCOL nCol);

   void SetRowStyle(ROWCOL row);
   void UpdateEventList(ROWCOL row);

   void AddRow(SpanIndexType spanIdx,LPCTSTR lpszGirderName);
   void OnClickedButtonRowCol(ROWCOL nRow,ROWCOL nCol);
   CString GetCellValue(ROWCOL nRow, ROWCOL nCol);
   void RefreshRowHeading(ROWCOL rFrom,ROWCOL rTo);

   bool ComputePjackMax(ROWCOL row);
   void OnCalcPjack(ROWCOL nRow);
   void OnEditDuctGeometry(ROWCOL nRow);
   void OnDuctTypeChanged(ROWCOL nRow);
   void UpdateNumStrandsList(ROWCOL nRow);
   void UpdateMaxPjack(ROWCOL nRow);

   void SetDuctData(ROWCOL row,const CDuctData& duct,EventIndexType stressingEvent);
   void GetDuctData(ROWCOL row,CDuctData& duct,EventIndexType& stressingEvent);

   void OnChangedSelection(const CGXRange* changedRect,BOOL bIsDragging,BOOL bKey);

   EventIndexType CreateEvent();

   virtual BOOL CanActivateGrid(BOOL bActivate);
};
