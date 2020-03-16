///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2020  Washington State Department of Transportation
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

#include "PGSComponentInfo.h"
typedef CEAFPluginManagerBase<IPGSuperComponentInfo,CPGSuperDoc> CPGSuperComponentInfoManager;

/*--------------------------------------------------------------------*/
class CPGSuperDoc : public CPGSDocBase
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
	virtual void AssertValid() const override;
	virtual void Dump(CDumpContext& dc) const override;
#endif

   virtual bool EditGirderDescription(const CGirderKey& girderKey, int nPage) override;
   virtual bool EditGirderSegmentDescription(const CSegmentKey& segmentKey, int nPage) override;
   virtual bool EditClosureJointDescription(const CClosureKey& closureKey, int nPage) override;
   virtual UINT GetStandardToolbarResourceID() override;

   void DesignGirder(bool bPrompt, arSlabOffsetDesignType designSlabOffset, const CGirderKey& girderKey);

   CPGSuperComponentInfoManager* GetComponentInfoManager() { return &m_ComponentInfoManager; }

// Generated message map functions
protected:
   BOOL Init();

   //{{AFX_MSG(CPGSuperDoc)
	afx_msg void OnEditBridgeDescription();
	afx_msg void OnEditGirder();
	afx_msg void OnProjectDesignGirder();
	afx_msg void OnProjectDesignGirderDirect();
	afx_msg void OnProjectDesignGirderDirectPreserveHaunch();
   afx_msg void OnUpdateProjectDesignGirderDirectPreserveHaunch(CCmdUI* pCmdUI);
   afx_msg void OnProjectAnalysis();
   //}}AFX_MSG

   CPGSuperComponentInfoManager m_ComponentInfoManager;

   void DoDesignGirder(const std::vector<CGirderKey>& girderKeys, bool bDesignFlexure, arSlabOffsetDesignType haunchDesignType, arConcreteDesignType concreteDesignType, arShearDesignType shearDesignType);

   virtual CPGSuperPluginMgrBase* CreatePluginManager() override { return new CPGSuperPluginMgr(); }

   virtual CATID GetAgentCategoryID() override { return CATID_PGSuperAgent; }
   virtual CATID GetExtensionAgentCategoryID() override { return CATID_PGSuperExtensionAgent; }
   virtual CATID GetBeamFamilyCategoryID() override { return CATID_PGSuperBeamFamily; }
   virtual CATID GetComponentInfoCategoryID() override { return CATID_PGSuperComponentInfo; }

   virtual LPCTSTR GetTemplateExtension() override;

   virtual void ModifyTemplate(LPCTSTR strTemplate) override;

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PGSUPERDOC_H__59D503F2_265C_11D2_8EB0_006097DF3C68__INCLUDED_)
