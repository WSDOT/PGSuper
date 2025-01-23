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

#if !defined(AFX_PIERDETAILSDLG_H__43875824_6EA0_4E3A_BF7A_B8D20B90BE96__INCLUDED_)
#define AFX_PIERDETAILSDLG_H__43875824_6EA0_4E3A_BF7A_B8D20B90BE96__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PierDetailsDlg.h : header file
//

#include "PierLocationPage.h"
#include "PierLayoutPage.h"
#include "PierConnectionsPage.h"
#include "AbutmentConnectionsPage.h"
#include "PierGirderSpacingPage.h"
#include "ClosureJointGeometryPage.h"
#include "GirderSegmentSpacingPage.h"
#include "PierDetailsBearingsPage.h"
#include <PgsExt\BridgeDescription2.h>
#include "EditPier.h"
#include <IFace\ExtendUI.h>
#include <EAF\EAFMacroTxn.h>

/////////////////////////////////////////////////////////////////////////////
// CPierDetailsDlg

class CPierDetailsDlg : public CPropertySheet, public IEditPierData
{
	DECLARE_DYNAMIC(CPierDetailsDlg)

// Construction
public:
	CPierDetailsDlg(const CBridgeDescription2* pBridgeDesc,PierIndexType pierIdx,CWnd* pParentWnd = nullptr, UINT iSelectPage = 0);
	CPierDetailsDlg(const CBridgeDescription2* pBridgeDesc,PierIndexType pierIdx,const std::vector<EditBridgeExtension>& editBridgeExtensions,CWnd* pParentWnd = nullptr, UINT iSelectPage = 0);
	virtual ~CPierDetailsDlg();
   
   CBridgeDescription2* GetBridgeDescription();

// interface IEditPierData
   virtual CPierData2* GetPierData() override {return m_pPier;}
   virtual PierIndexType GetPierCount() override { return m_BridgeDesc.GetPierCount(); }
   virtual PierIndexType GetPier() override { return m_pPier->GetIndex(); }
   virtual pgsTypes::BoundaryConditionType GetConnectionType() override;
   virtual GirderIndexType GetGirderCount(pgsTypes::PierFaceType face) override;

// Attributes
public:

// Operations
public:
   // Returns a macro transaction object that contains editing transactions
   // for all the extension pages. The caller is responsble for deleting this object
   std::unique_ptr<CEAFTransaction> GetExtensionPageTransaction();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPierDetailsDlg)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual INT_PTR DoModal() override;

	// Generated message map functions
protected:
	//{{AFX_MSG(CPierDetailsDlg)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
	afx_msg LRESULT OnKickIdle(WPARAM, LPARAM);

   void CommonInitPages();
   void InitPages();
   void InitPages(const std::vector<EditBridgeExtension>& editBridgeExtensions);
   void Init(const CBridgeDescription2* pBridge,PierIndexType pierIdx);

   void CreateExtensionPages();
   void CreateExtensionPages(const std::vector<EditBridgeExtension>& editBridgeExtensions);
   void DestroyExtensionPages();

   CBridgeDescription2 m_BridgeDesc; // this is the bridge we are operating on
   // pointers to objects within our private bridge model just to make life easier
   CSpanData2* m_pSpan[2];
   CPierData2* m_pPier;

private:
   friend CPierLocationPage;
   friend CPierGirderSpacingPage;
   friend CGirderSegmentSpacingPage;
   friend CClosureJointGeometryPage;
   friend CPierDetailsBearingsPage;

   // General layout page
   CPierLocationPage          m_PierLocationPage;

   // These pages are used when the pier is at a boundary between girder groups
   CAbutmentConnectionsPage   m_AbutmentConnectionsPage; // used at abutments (boundary condition and connection geometry)
   CPierLayoutPage            m_PierLayoutPage;          // used at piers (boundary condition and bent geometry)
   CPierConnectionsPage       m_PierConnectionsPage;     // used at piers (boundary condition and connection geometry)
   CPierGirderSpacingPage     m_PierGirderSpacingPage;   // used at piers and abutments (girder spacing)
   CPierDetailsBearingsPage   m_PierDetailsBearingsPage;

   // These two pages are used when the pier is interior to a girder group
   CClosureJointGeometryPage  m_ClosureJointGeometryPage; // Boundary condition and bent geometry
   CGirderSegmentSpacingPage  m_GirderSegmentSpacingPage; // girder spacing

   CEAFMacroTxn m_Macro;
   std::vector<std::pair<IEditPierCallback*,CPropertyPage*>> m_ExtensionPages;
   std::vector<EditBridgeExtension> m_BridgeExtensionPages;
   void NotifyExtensionPages();
   void NotifyBridgeExtensionPages();
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PIERDETAILSDLG_H__43875824_6EA0_4E3A_BF7A_B8D20B90BE96__INCLUDED_)
