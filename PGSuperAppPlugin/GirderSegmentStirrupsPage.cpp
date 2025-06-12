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

// GirderSegmentStirrupsPage.cpp : implementation file
//

#include "stdafx.h"
#include "PGSuperPluginApp.h"
#include "GirderSegmentStirrupsPage.h"
#include "GirderSegmentDlg.h"
#include <IFace\Project.h>
#include <EAF\EAFDisplayUnits.h>
#include <LRFD\RebarPool.h>



// CGirderSegmentStirrupsPage dialog

IMPLEMENT_DYNAMIC(CGirderSegmentStirrupsPage, CShearSteelPage)

CGirderSegmentStirrupsPage::CGirderSegmentStirrupsPage()
{
	//{{AFX_DATA_INIT(CGirderSegmentStirrupsPage)
	//}}AFX_DATA_INIT
}

CGirderSegmentStirrupsPage::~CGirderSegmentStirrupsPage()
{
}


BEGIN_MESSAGE_MAP(CGirderSegmentStirrupsPage, CShearSteelPage)
	//{{AFX_MSG_MAP(CGirderSegmentStirrupsPage)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGirderSegmentStirrupsPage message handlers

BOOL CGirderSegmentStirrupsPage::OnInitDialog() 
{
	m_bCanCopyFromLibrary = false; // copying of seed values not supported in pgsplice
	m_bIsDisplayedInProject = true;

	CShearSteelPage::OnInitDialog();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CGirderSegmentStirrupsPage::GetLastZoneName(CString& strSymmetric, CString& strEnd)
{
   strSymmetric = _T("mid-segment");
   strEnd = _T("end segment");
}
