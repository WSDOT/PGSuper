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

#include "StdAfx.h"
#include "CountedMultiDocTemplate.h"
#include "PGSuperDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNAMIC(CCountedMultiDocTemplate,CMultiDocTemplate)

CCountedMultiDocTemplate::CCountedMultiDocTemplate(UINT nIDResource,
                            CRuntimeClass* pDocClass,
                            CRuntimeClass* pFrameClass,
                            CRuntimeClass* pViewClass,
                            int maxViewCount) :
CMultiDocTemplate(nIDResource,pDocClass,pFrameClass,pViewClass)
{
   m_MaxViewCount = maxViewCount;
}

CCountedMultiDocTemplate::~CCountedMultiDocTemplate()
{
   // This prevents the base class virtual destructor from
   // destroying the menu resource (i.e. it checks to make
   // sure the handle isn't NULL)
   //
   // See MSKB Article ID: Q118435, "Sharing Menus Between MDI Child Windows"

   m_hMenuShared = NULL;
}

void CCountedMultiDocTemplate::InitialUpdateFrame(CFrameWnd* pFrame, CDocument* pDoc,BOOL bMakeVisible)
{
   CMultiDocTemplate::InitialUpdateFrame(pFrame,pDoc,bMakeVisible);
}


CPGSuperCountedMultiDocTemplate::CPGSuperCountedMultiDocTemplate(UINT nIDResource,
                            CRuntimeClass* pDocClass,
                            CRuntimeClass* pFrameClass,
                            CRuntimeClass* pViewClass,
                            int maxViewCount) :
CCountedMultiDocTemplate(nIDResource,pDocClass,pFrameClass,pViewClass,maxViewCount)
{
}

void CPGSuperCountedMultiDocTemplate::InitialUpdateFrame(CFrameWnd* pFrame, CDocument* pDoc,BOOL bMakeVisible)
{
   CCountedMultiDocTemplate::InitialUpdateFrame(pFrame,pDoc,bMakeVisible);

   // This is the first point that we can intercept the document/frame/view initialization
   // after the main frame menu is loaded.
   //
   // Have the document populate the Reports sub-menu
   CPGSuperDoc* pPGSuperDoc = (CPGSuperDoc*)pDoc;
   pPGSuperDoc->PopulateReportMenu();
}
