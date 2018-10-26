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

// ActivityGrid.h : header file
//

#include <PgsExt\TimelineEvent.h>

/////////////////////////////////////////////////////////////////////////////
// CActivityGrid window

class CActivityGrid : public CGXGridWnd
{
	GRID_DECLARE_REGISTER()
// Construction
public:
	CActivityGrid();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CActivityGrid)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CActivityGrid();

public:
   // custom stuff for grid
   void CustomInit();
   void Refresh();
   void RemoveActivity();

	// Generated message map functions
protected:
	//{{AFX_MSG(CActivityGrid)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

   void AddActivity(LPCTSTR strName,int activityKey);
   
   virtual void OnClickedButtonRowCol(ROWCOL nRow,ROWCOL nCol);

   DECLARE_MESSAGE_MAP()
};

