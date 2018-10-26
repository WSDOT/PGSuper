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

// LibraryEditor.cpp : implementation file
//

#include "stdafx.h"
#include "PGSuper.h"
#include "LibraryEditor.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLibraryEditor

IMPLEMENT_DYNCREATE(CLibraryEditor, CView)

CLibraryEditor::CLibraryEditor()
{
}

CLibraryEditor::~CLibraryEditor()
{
}


BEGIN_MESSAGE_MAP(CLibraryEditor, CView)
	//{{AFX_MSG_MAP(CLibraryEditor)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLibraryEditor drawing

void CLibraryEditor::OnDraw(CDC* pDC)
{
	CDocument* pDoc = GetDocument();
	// TODO: add draw code here
   pDC->TextOut(0,0,"Library Editor");
}

/////////////////////////////////////////////////////////////////////////////
// CLibraryEditor diagnostics

#ifdef _DEBUG
void CLibraryEditor::AssertValid() const
{
	CView::AssertValid();
}

void CLibraryEditor::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CLibraryEditor message handlers
