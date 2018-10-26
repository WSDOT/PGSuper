///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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

#ifndef INCLUDED_SPLITCHILDFRM_H_
#define INCLUDED_SPLITCHILDFRM_H_

// CSplitChildFrm.h : interface of the CSplitChildFrame class
//
/////////////////////////////////////////////////////////////////////////////
#include "ChildFrm.h"
#include <MfcTools\StaticSplitter.h>

// A pure virtual class to facilitate adding a static splitter.

class CSplitChildFrame : public CChildFrame
{
	DECLARE_DYNAMIC(CSplitChildFrame)
public:
	CSplitChildFrame();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSplitChildFrame)
	protected:
	virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CSplitChildFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

// Generated message map functions
protected:
	//{{AFX_MSG(CSplitChildFrame)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   // pure virtual class to get runtime class of bottom pane
   virtual CRuntimeClass* GetLowerPaneClass() const =0;

   // Get the fraction that the top frame takes of the client window
   virtual Float64 GetTopFrameFraction() const =0;


   BOOL m_bPanesCreated;
   CDynamicSplitter m_SplitterWnd;
};

/////////////////////////////////////////////////////////////////////////////
#endif
