///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright (C) 1999  Washington State Department of Transportation
//                     Bridge and Structures Office
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

// ChildFrm.h : interface of the COutputChildFrame class
//
/////////////////////////////////////////////////////////////////////////////
#if !defined OutputChildFrame_H_
#define OutputChildFrame_H_

// :TRICKY: rab 11.23.96 : Modifying default behavior
// :FILE: ChildFrm.h
//
// We override CMDIChildWnd to customize the MDI child's title bar.
// By default the title bar shows the document name.  But we want
// it to instead show the text defined as the first string in
// the document template STRINGTABLE resource.  This string is
// the name of the view.  If we didn't customize the title bar, two
// MDI child windows containing differnt view types would
// show MYDOC:1 and MYDOC:2 if the document were named MYDOC.
#include "LicensePlateChildFrm.h"

class COutputChildFrame : public CLicensePlateChildFrame
{
	DECLARE_DYNCREATE(COutputChildFrame)
public:
	COutputChildFrame();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(COutputChildFrame)
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~COutputChildFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

// Generated message map functions
protected:
	//{{AFX_MSG(COutputChildFrame)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   void OnUpdateFrameTitle(BOOL bAddToTitle);
};

/////////////////////////////////////////////////////////////////////////////
#endif // OutputChildFrame_H_