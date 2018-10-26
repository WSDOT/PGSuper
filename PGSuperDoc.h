///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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

// PGSuperDoc.h : interface of the CPGSuperDoc class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_PGSUPERDOC_H__59D503F2_265C_11D2_8EB0_006097DF3C68__INCLUDED_)
#define AFX_PGSUPERDOC_H__59D503F2_265C_11D2_8EB0_006097DF3C68__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "PGSuperDocBase.h"
#include "PGSuperPluginMgr.h"
#include "PGSuperCatCom.h"

/*--------------------------------------------------------------------*/
class CPGSuperDoc : public CPGSuperDocBase
{
protected: // create from serialization only
	CPGSuperDoc();
	DECLARE_DYNCREATE(CPGSuperDoc)

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPGSuperDoc)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CPGSuperDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

   void DesignGirder(bool bPrompt,bool bDesignSlabOffset,SpanIndexType spanIdx,GirderIndexType gdrIdx);

   bool EditGirderDescription(const CGirderKey& girderKey,int nPage);
   bool EditGirderSegmentDescription(const CSegmentKey& segmentKey,int nPage);

   // flags for design dialog
   bool IsDesignFlexureEnabled() const;
   void EnableDesignFlexure( bool bEnable );

   bool IsDesignShearEnabled() const;
   void EnableDesignShear( bool bEnable );

   bool IsDesignStirrupsFromScratchEnabled() const;
   void EnableDesignStirrupsFromScratch( bool bEnable );

   virtual UINT GetStandardToolbarResourceID();

// Generated message map functions
protected:
	//{{AFX_MSG(CPGSuperDoc)
	afx_msg void OnEditBridgeDescription();
	afx_msg void OnEditGirder();
	afx_msg void OnProjectDesignGirder();
	afx_msg void OnProjectDesignGirderDirect();
   afx_msg void OnUpdateProjectDesignGirderDirect(CCmdUI* pCmdUI);
	afx_msg void OnProjectDesignGirderDirectHoldSlabOffset();
   afx_msg void OnUpdateProjectDesignGirderDirectHoldSlabOffset(CCmdUI* pCmdUI);
   afx_msg void OnProjectAnalysis();
   //}}AFX_MSG

   bool m_bDesignFlexureEnabled;
   bool m_bDesignShearEnabled;
   bool m_bDesignStirrupsFromScratchEnabled;

   void DoDesignGirder(const std::vector<CGirderKey>& girderKeys, bool doDesignADim);

   virtual CPGSuperPluginMgrBase* CreatePluginManager() { return new CPGSuperPluginMgr(); }

   virtual void LoadDocumentSettings();
   virtual void SaveDocumentSettings();

   virtual CATID GetAgentCategoryID() { return CATID_PGSuperAgent; }
   virtual CATID GetExtensionAgentCategoryID() { return CATID_PGSuperExtensionAgent; }
   virtual CATID GetBeamFamilyCategoryID() { return CATID_PGSuperBeamFamily; }

   virtual LPCTSTR GetTemplateExtension();

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PGSUPERDOC_H__59D503F2_265C_11D2_8EB0_006097DF3C68__INCLUDED_)
