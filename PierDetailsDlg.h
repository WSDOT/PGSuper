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

#if !defined(AFX_PIERDETAILSDLG_H__43875824_6EA0_4E3A_BF7A_B8D20B90BE96__INCLUDED_)
#define AFX_PIERDETAILSDLG_H__43875824_6EA0_4E3A_BF7A_B8D20B90BE96__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PierDetailsDlg.h : header file
//

#include "PierLayoutPage.h"
#include "PierConnectionsPage.h"
#include "PierGirderSpacingPage.h"
#include "PGSuperAppPlugin\ClosureJointGeometryPage.h"
#include "PGSuperAppPlugin\GirderSegmentSpacingPage.h"
#include <PgsExt\BridgeDescription2.h>
#include "EditPier.h"

/////////////////////////////////////////////////////////////////////////////
// CPierDetailsDlg

class CPierDetailsDlg : public CPropertySheet
{
	DECLARE_DYNAMIC(CPierDetailsDlg)

// Construction
public:
	CPierDetailsDlg(const CBridgeDescription2* pBridgeDesc,PierIndexType pierIdx,CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	virtual ~CPierDetailsDlg();
   
   CBridgeDescription2* GetBridgeDescription();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPierDetailsDlg)
	//}}AFX_VIRTUAL

// Implementation
public:

	// Generated message map functions
protected:
	//{{AFX_MSG(CPierDetailsDlg)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   void InitPages();
   void Init(const CBridgeDescription2* pBridge,PierIndexType pierIdx);

   CBridgeDescription2 m_BridgeDesc; // this is the bridge we are operating on
   CSpanData2* m_pPrevSpan; // pointers to objects within our private bridge model just to make life easier
   CPierData2* m_pPierData;
   CSpanData2* m_pNextSpan;

private:
   friend CPierLayoutPage;
   friend CPierGirderSpacingPage;
   friend CGirderSegmentSpacingPage;
   friend CClosureJointGeometryPage;

   // General layout page
   CPierLayoutPage            m_PierLayoutPage;

   // These two pages are used when the pier is at a boundary between girder groups
   CPierConnectionsPage       m_PierConnectionsPage;
   CPierGirderSpacingPage     m_PierGirderSpacingPage;

   // These two pages are used when the pier is interior to a girder group
   CClosureJointGeometryPage  m_ClosureJointGeometryPage;
   CGirderSegmentSpacingPage  m_GirderSegmentSpacingPage;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PIERDETAILSDLG_H__43875824_6EA0_4E3A_BF7A_B8D20B90BE96__INCLUDED_)
