///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2025  Washington State Department of Transportation
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

// BridgeDescFramingGrid.h : header file
//

#include <PgsExt\BridgeDescription2.h>
#include <EAF\EAFMacroTxn.h>

/////////////////////////////////////////////////////////////////////////////
// CBridgeDescFramingGrid window

class CBridgeDescFramingGrid : public CGXGridWnd
{
	GRID_DECLARE_REGISTER()
// Construction
public:
	CBridgeDescFramingGrid();
	virtual ~CBridgeDescFramingGrid();

// Attributes
public:

// Operations
public:
   std::vector<Float64> GetSpanLengths();
   void SetSpanLengths(const std::vector<Float64>& spanLengths,PierIndexType fixedPierIdx);
   void SetPierOrientation(LPCTSTR strOrientation);
   std::vector<Float64> GetSegmentLengths();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBridgeDescFramingGrid)
	//}}AFX_VIRTUAL

// Implementation
public:
   SpanIndexType GetSelectedSpan();
   PierIndexType GetSelectedPier();
   PierIndexType GetPierCount();

	// Generated message map functions
public:
	//{{AFX_MSG(CBridgeDescFramingGrid)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   // virtual overrides for grid
   BOOL OnRButtonClickedRowCol(ROWCOL nRow, ROWCOL nCol, UINT nFlags, CPoint pt);
   void OnChangedSelection(const CGXRange* pChangedRect,BOOL bIsDragging, BOOL bKey);
   void OnClickedButtonRowCol(ROWCOL nRow,ROWCOL nCol);
   
   BOOL CanActivateGrid(BOOL bActivate); // called by DDV_GXGridWnd
   BOOL OnValidateCell(ROWCOL nRow, ROWCOL nCol);
   BOOL OnEndEditing(ROWCOL nRow,ROWCOL nCol);

public:
   // custom stuff for grid
   void CustomInit();

   // get a cell value whether is is selected or not
   CString GetCellValue(ROWCOL nRow, ROWCOL nCol);

   // fill grid with data
   void FillGrid(const CBridgeDescription2& bridgeDesc);

   CPierData2* GetPierRowData(ROWCOL nRow);
   CTemporarySupportData GetTemporarySupportRowData(ROWCOL nRow);

	void OnAddPier();
	void OnRemovePier();
   bool EnableRemovePierBtn();

   void OnAddTemporarySupport();
   void OnRemoveTemporarySupport();
   bool EnableRemoveTemporarySupportBtn();

   void GetTransactions(CEAFMacroTxn& macro);

   // Tricky workaround here to avoid cell validation if user hits the dailog Cancel button or "X". Value
   // below will be set in OnKillFocus (for Cancel button) and CBridgeDescFramingPage::OnCancel() (for "X" menu item)
   void SetDoNotValidateCells();

private:
   void InsertRow();

   void FillPierRow(ROWCOL row,const CPierData2* pPierData);
   //void FillSpanRow(ROWCOL row,const CSpanData* pSpanData);
   void FillTemporarySupportRow(ROWCOL row,const CTemporarySupportData* pTSData);
   void FillSegmentRow(ROWCOL row);
   void FillSegmentColumn();
   void FillSpanColumn();

   void EditPier(PierIndexType pierIdx);
   void EditSpan(SpanIndexType spanIdx);
   void EditTemporarySupport(SupportIndexType tsIdx);

   ROWCOL GetPierRow(PierIndexType pierIdx);
   PierIndexType GetPierIndex(ROWCOL nRow);

   SpanIndexType GetSpanIndex(ROWCOL nRow);

   ROWCOL GetTemporarySupportRow(SupportIndexType tsIdx);
   SupportIndexType GetTemporarySupportIndex(ROWCOL nRow);

   void SavePierTransaction(PierIndexType pierIdx,std::unique_ptr<CEAFTransaction>&& pTxn);
   std::map<PierIndexType,std::vector<std::unique_ptr<CEAFTransaction>>> m_PierTransactions;

   void SaveSpanTransaction(SpanIndexType spanIdx, std::unique_ptr<CEAFTransaction>&& pTxn);
   std::map<SpanIndexType, std::vector<std::unique_ptr<CEAFTransaction>>> m_SpanTransactions;

   void SaveTemporarySupportTransaction(SupportIndexType tsIdx, std::unique_ptr<CEAFTransaction>&&  pTxn);
   std::map<SupportIndexType, std::vector<std::unique_ptr<CEAFTransaction>>> m_TempSupportTransactions;

   bool m_bDoValidate;

public:
   afx_msg void OnKillFocus(CWnd* pNewWnd);

};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.
