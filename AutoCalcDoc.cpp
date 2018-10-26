///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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

// AutoCalcDoc.cpp : implementation file
//

#include "stdafx.h"
#include "AutoCalcDoc.h"
#include "AutoCalcView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAutoCalcDoc
CAutoCalcDoc::CAutoCalcDoc()
{
}

CAutoCalcDoc::~CAutoCalcDoc()
{
}


BEGIN_MESSAGE_MAP(CAutoCalcDoc, CDocument)
	//{{AFX_MSG_MAP(CAutoCalcDoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAutoCalcDoc diagnostics

#ifdef _DEBUG
void CAutoCalcDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CAutoCalcDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CAutoCalcDoc serialization

/////////////////////////////////////////////////////////////////////////////
// CAutoCalcDoc commands
void CAutoCalcDoc::OnUpdateNow()
{
   POSITION pos = GetFirstViewPosition();
   while (pos != NULL)   
   {
      CView* pView = GetNextView(pos);
      if ( pView->IsKindOf(RUNTIME_CLASS(CAutoCalcView)) )
      {
         CAutoCalcView* pAutoCalcView = (CAutoCalcView*)pView;
         pAutoCalcView->OnUpdateNow();
      }
   }
}

void CAutoCalcDoc::OnUpdateUpdateNow(CCmdUI* pCmdUI)
{
   if ( IsAutoCalcEnabled() )
      pCmdUI->Enable( FALSE );
   else
      pCmdUI->Enable( TRUE );
}

