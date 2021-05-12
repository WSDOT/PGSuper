///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2021  Washington State Department of Transportation
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

// TxDOTOptionalDesignDoc.h : interface of the CTxDOTOptionalDesignDoc class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include <EAF\EAFBrokerDocument.h>
#include "TxDOTOptionalDesignData.h"
#include "TxDOTOptionalDesignBrokerRetreiver.h"

#include <PsgLib\LibraryManager.h>

class CTxDOTOptionalDesignDocProxyAgent;
class CBridgeDescription2;
class CGirderGroupData;

// Hints for views
// in the OnUpdate method.
#define HINT_GIRDERVIEWSETTINGSCHANGED    1
#define HINT_GIRDERVIEWSECTIONCUTCHANGED  3
#define HINT_GIRDERSELECTIONCHANGED       5
#define HINT_GIRDERCHANGED                7
#include <EAF\EAFHints.h>

// Girder Model Editor
#define VS_GIRDER_ELEVATION   0
#define VS_GIRDER_SECTION     1

// ======================================
// all possible settings for girder model editor;
//       section view
#define IDG_SV_SHOW_STRANDS     ((DWORD)0x00000001)
#define IDG_SV_SHOW_PS_CG       ((DWORD)0x00000002)
#define IDG_SV_SHOW_DIMENSIONS  ((DWORD)0x00000004)
#define IDG_SV_DRAW_ISOTROPIC   ((DWORD)0x00000010)
#define IDG_SV_DRAW_TO_SCALE    ((DWORD)0x00000020)
#define IDG_SV_SHOW_LONG_REINF  ((DWORD)0x00000040)

// elevation view
#define IDG_EV_SHOW_STRANDS     ((DWORD)0x00000100)
#define IDG_EV_SHOW_PS_CG       ((DWORD)0x00000200)
#define IDG_EV_SHOW_DIMENSIONS  ((DWORD)0x00000400)
#define IDG_EV_DRAW_ISOTROPIC   ((DWORD)0x00000800)
#define IDG_EV_DRAW_TO_SCALE    ((DWORD)0x00001000)
#define IDG_EV_SHOW_STIRRUPS    ((DWORD)0x00002000)
#define IDG_EV_SHOW_LONG_REINF  ((DWORD)0x00004000)

// show everything by default
const UINT DEF_GV =  IDG_SV_SHOW_STRANDS     |
                     IDG_SV_SHOW_STRANDS     |
                     IDG_SV_SHOW_PS_CG       |
                     IDG_SV_SHOW_DIMENSIONS  |
                     IDG_SV_DRAW_ISOTROPIC   |
                     IDG_SV_DRAW_TO_SCALE    |
                     IDG_SV_SHOW_LONG_REINF  |
                     IDG_EV_SHOW_STRANDS     |
                     IDG_EV_SHOW_PS_CG       |
                     IDG_EV_SHOW_DIMENSIONS  |
                     IDG_EV_DRAW_ISOTROPIC   |
                     IDG_EV_DRAW_TO_SCALE    |
                     IDG_EV_SHOW_STIRRUPS    |
                     IDG_EV_SHOW_LONG_REINF;


/*--------------------------------------------------------------------*/
class CTxDOTOptionalDesignDoc : public CEAFBrokerDocument, public ITxDataObserver, public ITxDOTBrokerRetriever
{
protected: // create from serialization only
	CTxDOTOptionalDesignDoc();
	DECLARE_DYNCREATE(CTxDOTOptionalDesignDoc)

// CEAFBrokerDocument overrides
public:
   virtual CATID GetAgentCategoryID() override;
   virtual CATID GetExtensionAgentCategoryID() override;
   virtual BOOL Init() override;
   virtual BOOL LoadSpecialAgents(IBrokerInitEx2* pBrokerInit) override; 
   virtual CString GetToolbarSectionName() override;

   virtual void LoadToolbarState() override;
   virtual void SaveToolbarState() override;

   virtual void DoIntegrateWithUI(BOOL bIntegrate) override;

   virtual void LoadDocumentSettings() override;
   virtual void SaveDocumentSettings() override;

	virtual BOOL OnNewDocumentFromTemplate(LPCTSTR lpszPathName) override;
   virtual BOOL OpenTheDocument(LPCTSTR lpszPathName) override;
   virtual BOOL SaveTheDocument(LPCTSTR lpszPathName) override;
   virtual HRESULT WriteTheDocument(IStructuredSave* pStrSave) override;
   virtual HRESULT LoadTheDocument(IStructuredLoad* pStrLoad) override;

   virtual CString GetDocumentationSetName() override;
   virtual CString GetDocumentationRootLocation() override;

   virtual BOOL GetStatusBarMessageString(UINT nID,CString& rMessage) const override;
   virtual BOOL GetToolTipMessageString(UINT nID, CString& rMessage) const override;

protected:
   virtual void OnCreateInitialize() override;
   virtual void OnCreateFinalize() override;

   virtual CString GetRootNodeName() override;
   virtual Float64 GetRootNodeVersion() override;
   virtual HRESULT OpenDocumentRootNode(IStructuredSave* pStrSave) override;

public:
   // ITxDOTBrokerRetriever
   virtual IBroker* GetUpdatedBroker() override;
   virtual IBroker* GetClassicBroker() override;
   virtual GirderLibrary* GetGirderLibrary() override;
   virtual ConnectionLibrary* GetConnectionLibrary() override;
   virtual SpecLibrary* GetSpecLibrary() override;

// Operations
   // listen to data events
   virtual void OnTxDotDataChanged(int change) override;


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTxDOTOptionalDesignDoc)
	public:
//	virtual BOOL OnNewDocumentFromTemplate(LPCTSTR lpszPathName) override;
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CTxDOTOptionalDesignDoc();
#ifdef _DEBUG
	virtual void AssertValid() const override;
	virtual void Dump(CDumpContext& dc) const override;
#endif

// Generated message map functions
protected:
	//{{AFX_MSG(CTxDOTOptionalDesignDoc)
   //}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   virtual BOOL CreateBroker() override;
   virtual HINSTANCE GetResourceInstance() override;

   // our data
public:
   CTxDOTOptionalDesignData m_ProjectData;

private:
   CEAFToolBar* m_pMyToolBar;

   // Has our data changed?
   int m_ChangeStatus;

   // Is first time for the broker
   bool m_VirginBroker;

   CTxDOTOptionalDesignDocProxyAgent* m_pTxDOTOptionalDesignDocProxyAgent;

   // need a local library manager for master library
   psgLibraryManager m_LibMgr;

   // Flag that's true if we are exporting to PGSuper
   bool m_InExportMode;

   UINT m_GirderModelEditorSettings;

private:
   // Implementation
   void InitializeLibraryManager();
   void MarryPGSuperTemplateWithBroker(LPCTSTR lpszPathName);
   void PreprocessTemplateData();
   void UpdatePgsuperModelWithData();
   void VerifyPgsuperTemplateData(CBridgeDescription2& bridgeDesc);
   void SetGirderData(CTxDOTOptionalDesignGirderData* pOdGirderData, GirderIndexType gdr,
                      LPCTSTR gdrName, const GirderLibraryEntry* pGdrEntry, Float64 EcBeam, Float64 weightDensity,
                      CGirderGroupData* pGroup);

   void RecreateBroker();

   BOOL ParseTemplateFile(bool isNewFileFromTemplate);
   BOOL ParseTemplateFile(LPCTSTR lpszPathName, bool isNewFileFromTemplate);

   HRESULT LoadThePGSuperDocument(IStructuredLoad* pStrLoad);
   void HandleOpenDocumentError( HRESULT hr, LPCTSTR lpszPathName );

   BOOL UpdateCurrentViewInputData();

public:
   afx_msg void OnFileSave();
   afx_msg void OnFileSaveas();
   afx_msg void OnFileExportPgsuperModel();

   // set/get view settings for Girder model editor
   UINT GetGirderEditorSettings() const;
   void SetGirderEditorSettings(UINT settings);

   void EditGirderViewSettings(int nPage);
   afx_msg void OnViewGirderviewsettings();
   afx_msg void OnStatuscenterView();

   virtual void ShowCustomReportHelp(eafTypes::CustomReportHelp helpType) override;
   virtual void ShowCustomReportDefinitionHelp() override;

};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.
