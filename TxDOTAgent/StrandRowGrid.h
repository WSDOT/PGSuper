///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2019  Washington State Department of Transportation
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

#if !defined(AFX_BRIDGECStrandRowGrid_H__8D165F54_32B9_11D2_9D40_00609710E6CE__INCLUDED_)
#define AFX_BRIDGECStrandRowGrid_H__8D165F54_32B9_11D2_9D40_00609710E6CE__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// StrandRowGrid.h : header file
//

#include "TxDOTOptionalDesignGirderData.h"
#include "TxDOTOptionalDesignBrokerRetreiver.h"

class StrandRowGridEventHandler
{
public:
   virtual void OnGridDataChanged() = 0;
};

/////////////////////////////////////////////////////////////////////////////
// CStrandRowGrid window

class CStrandRowGrid : public CGXGridWnd
{
	GRID_DECLARE_REGISTER()
// Construction
public:
	CStrandRowGrid();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CStrandRowGrid)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CStrandRowGrid();

	// Generated message map functions
protected:
	//{{AFX_MSG(CStrandRowGrid)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   // virtual overrides for grid
   virtual int GetColWidth(ROWCOL nCol) override;
   virtual void OnModifyCell (ROWCOL nRow, ROWCOL nCol) override;

public:
   // custom stuff for grid
   void CustomInit(StrandRowGridEventHandler* pHandler);


   void FillGrid(const CTxDOTOptionalDesignGirderData::AvailableStrandsInRowContainer& availStrands, 
                 const CTxDOTOptionalDesignGirderData::StrandRowContainer& container);
   CTxDOTOptionalDesignGirderData::StrandRowContainer GetData();

   void ComputeStrands(StrandIndexType* pNum, Float64* pCg);

private:
   StrandRowGridEventHandler* m_pHandler;

   // set up styles for interior rows
   void SetRowStyle(ROWCOL nRow);
   CString GetStrandRowList(const std::vector<CTxDOTOptionalDesignGirderData::StrandIncrement>& rowData);
   // get a cell value whether is is selected or not
   CString GetCellValue(ROWCOL nRow, ROWCOL nCol);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_BRIDGECStrandRowGrid_H__8D165F54_32B9_11D2_9D40_00609710E6CE__INCLUDED_)
