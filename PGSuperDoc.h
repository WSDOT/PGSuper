///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2012  Washington State Department of Transportation
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

#include <EAF\EAFBrokerDocument.h>
#include <EAF\EAFAutoCalcDoc.h>

#include <PsgLib\ISupportLibraryManager.h>
#include <PsgLib\LibraryManager.h>
#include <IReportManager.h>

#include <IFace\ViewEvents.h> 
#include <IFace\Selection.h> 

#include "Hints.h"
#include "PGSuperCommandLineInfo.h"
#include "PGSuperPluginMgr.h"

#define EPD_GENERAL        0
#define EPD_CONNECTION     1
#define EPD_SPACING        2

#define ESD_GENERAL        0
#define ESD_CONNECTION     1
#define ESD_SPACING        2

#define VS_GIRDER_ELEVATION   0
#define VS_GIRDER_SECTION     1

#define VS_BRIDGE_PLAN        0
#define VS_BRIDGE_SECTION     1

class CCopyGirderDlg;
class pgsDesignArtifact;
class CPGSuperDocProxyAgent;

/*--------------------------------------------------------------------*/
class CPGSuperDoc : public CEAFBrokerDocument, 
                    public CEAFAutoCalcDocMixin,  // mix-in class for auto-calc capabilities
                    public libISupportLibraryManager
{
protected: // create from serialization only
	CPGSuperDoc();
	DECLARE_DYNCREATE(CPGSuperDoc)

// CBrokerDocument over-rides
public:
   virtual CATID GetAgentCategoryID();
   virtual CATID GetExtensionAgentCategoryID();
   virtual BOOL Init();
   virtual BOOL LoadSpecialAgents(IBrokerInitEx2* pBrokerInit); 

// CEAFAutoCalcDocMixin over-rides
public:
   virtual bool IsAutoCalcEnabled() const;
   virtual void EnableAutoCalc(bool bEnable);

// Operations
public:
   CSelection GetSelection();
   void SelectPier(PierIndexType pierIdx);
   void SelectSpan(SpanIndexType spanIdx);
   void SelectGirder(SpanIndexType spanIdx,GirderIndexType gdrIdx);
   void SelectDeck();
   void SelectAlignment();
   void ClearSelection();

   void OnLibMgrChanged(psgLibraryManager* pNewLibMgr);

   void PopulateReportMenu();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPGSuperDoc)
	public:
	virtual BOOL OnNewDocumentFromTemplate(LPCTSTR lpszPathName);
   virtual void OnCloseDocument();
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	//}}AFX_VIRTUAL

   void UpdateAnalysisTypeStatusIndicator();
   void OnLoadsLldf();
   void OnLoadsLldf(pgsTypes::DistributionFactorMethod method,LldfRangeOfApplicabilityAction roaAction);
   void OnLiveLoads();

   virtual BOOL GetStatusBarMessageString(UINT nID,CString& rMessage) const;
   virtual BOOL GetToolTipMessageString(UINT nID, CString& rMessage) const;

// Implementation
public:
	virtual ~CPGSuperDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

   // ISupportLibraryManager
   virtual CollectionIndexType GetNumberOfLibraryManagers() const;
   virtual libLibraryManager* GetLibraryManager(CollectionIndexType num);
   virtual libLibraryManager* GetTargetLibraryManager();

   void EditBridgeDescription(int nPage);
   void EditAlignmentDescription(int nPage);
   bool EditGirderDescription(SpanIndexType span,GirderIndexType girder, int nPage);
   bool EditSpanDescription(SpanIndexType spanIdx, int nPage);
   bool EditPierDescription(PierIndexType pierIdx, int nPage);

   void AddPointLoad(const CPointLoadData& loadData);
   bool EditPointLoad(CollectionIndexType loadIdx);
   void DeletePointLoad(CollectionIndexType loadIdx);

   void AddDistributedLoad(const CDistributedLoadData& loadData);
   bool EditDistributedLoad(CollectionIndexType loadIdx);
   void DeleteDistributedLoad(CollectionIndexType loadIdx);

   void AddMomentLoad(const CMomentLoadData& loadData);
   bool EditMomentLoad(CollectionIndexType loadIdx);
   void DeleteMomentLoad(CollectionIndexType loadIdx);


   void EditGirderViewSettings(int nPage);
   void EditBridgeViewSettings(int nPage);

   void DesignGirder(bool bPrompt,bool bDesignSlabOffset,SpanIndexType spanIdx,GirderIndexType gdrIdx);


   void GetDocUnitSystem(IDocUnitSystem** ppDocUnitSystem);

   void DeletePier(SpanIndexType pierIdx);
   void DeletePier(SpanIndexType pierIdx,pgsTypes::PierFaceType face);
   void DeleteSpan(SpanIndexType spanIdx);
   void DeleteSpan(SpanIndexType spanIdx,pgsTypes::RemovePierType pierRemoveType);
   void InsertSpan(PierIndexType refPierIdx,pgsTypes::PierFaceType pierFace);


   // set/get view settings for bridge model editor
   UINT GetBridgeEditorSettings() const;
   void SetBridgeEditorSettings(UINT settings);

   // set/get view settings for Girder model editor
   UINT GetGirderEditorSettings() const;
   void SetGirderEditorSettings(UINT settings);

   // set/get settings for UI hint dialogs
   UINT GetUIHintSettings() const;
   void SetUIHintSettings(UINT settings);

   // flags for design dialog
   bool IsDesignFlexureEnabled() const;
   void EnableDesignFlexure( bool bEnable );

   bool IsDesignShearEnabled() const;
   void EnableDesignShear( bool bEnable );

   bool IsDesignStirrupsFromScratchEnabled() const;
   void EnableDesignStirrupsFromScratch( bool bEnable );

   bool ShowProjectPropertiesOnNewProject();
   void ShowProjectPropertiesOnNewProject(bool bShow);

   BOOL UpdateTemplates();

   IDType RegisterBridgePlanViewCallback(IBridgePlanViewEventCallback* pCallback);
   bool UnregisterBridgePlanViewCallback(IDType ID);
   std::map<IDType,IBridgePlanViewEventCallback*> GetBridgePlanViewCallbacks();

   IDType RegisterBridgeSectionViewCallback(IBridgeSectionViewEventCallback* pCallback);
   bool UnregisterBridgeSectionViewCallback(IDType ID);
   std::map<IDType,IBridgeSectionViewEventCallback*> GetBridgeSectionViewCallbacks();

   IDType RegisterGirderElevationViewCallback(IGirderElevationViewEventCallback* pCallback);
   bool UnregisterGirderElevationViewCallback(IDType ID);
   std::map<IDType,IGirderElevationViewEventCallback*> GetGirderElevationViewCallbacks();

   IDType RegisterGirderSectionViewCallback(IGirderSectionViewEventCallback* pCallback);
   bool UnregisterGirderSectionViewCallback(IDType ID);
   std::map<IDType,IGirderSectionViewEventCallback*> GetGirderSectionViewCallbacks();


protected:
   CPGSuperDocProxyAgent* m_pPGSuperDocProxyAgent;

   IDType m_ViewCallbackID;
   std::map<IDType,IBridgePlanViewEventCallback*> m_BridgePlanViewCallbacks;
   std::map<IDType,IBridgeSectionViewEventCallback*> m_BridgeSectionViewCallbacks;
   std::map<IDType,IGirderElevationViewEventCallback*> m_GirderElevationViewCallbacks;
   std::map<IDType,IGirderSectionViewEventCallback*> m_GirderSectionViewCallbacks;

   psgLibraryManager m_LibMgr;

   bool m_bAutoCalcEnabled;
   UINT m_BridgeModelEditorSettings;
   UINT m_GirderModelEditorSettings;
   UINT m_UIHintSettings;
   bool m_bDesignFlexureEnabled;
   bool m_bDesignShearEnabled;
   bool m_bDesignStirrupsFromScratchEnabled;
   bool m_bShowProjectProperties;

   CComPtr<IDocUnitSystem> m_DocUnitSystem;
   
   CPGSuperPluginMgr m_PluginMgr; // manages data importer and exporter plugins

   CSelection m_Selection;

   bool m_bDesignSlabOffset;

   friend CCopyGirderDlg;
   void OnApplyCopyGirder(SpanGirderHashType fromHash,std::vector<SpanGirderHashType> toHash,BOOL bGirder,BOOL bTransverse,BOOL bLongitudinalRebar,BOOL bPrestress,BOOL bHandling, BOOL bMaterial, BOOL bSlabOffset);

   virtual void OnCreateInitialize();
   virtual void OnCreateFinalize();

   virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
   virtual BOOL OpenTheDocument(LPCTSTR lpszPathName);

   virtual void HandleOpenDocumentError( HRESULT hr, LPCTSTR lpszPathName );
   virtual void HandleSaveDocumentError( HRESULT hr, LPCTSTR lpszPathName );

   virtual HRESULT ConvertTheDocument(LPCTSTR lpszPathName, CString* realFileName);
   virtual void HandleConvertDocumentError( HRESULT hr, LPCTSTR lpszPathName );

   virtual CString GetRootNodeName();
   virtual Float64 GetRootNodeVersion();

   virtual HRESULT OpenDocumentRootNode(IStructuredSave* pStrSave);
   virtual HRESULT OpenDocumentRootNode(IStructuredLoad* pStrLoad);

   virtual void OnErrorDeletingBadSave(LPCTSTR lpszPathName,LPCTSTR lpszBackup);
   virtual void OnErrorRemaningSaveBackup(LPCTSTR lpszPathName,LPCTSTR lpszBackup);

   virtual void LoadDocumentSettings();
   virtual void SaveDocumentSettings();

   virtual void OnLogFileOpened(); // called when the log file is first opened

   virtual void BrokerShutDown();
   virtual void OnStatusChanged();

   BOOL UpdateTemplates(IProgress* pProgress,LPCTSTR lpszDir);

   virtual CString GetToolbarSectionName();

   virtual void CreateReportView(CollectionIndexType rptIdx,bool bPrompt);
   void DoDesignGirder(const std::vector<SpanGirderHashType>& girderList, bool doDesignADim);

// Generated message map functions
protected:
	//{{AFX_MSG(CPGSuperDoc)
	afx_msg void OnFileProjectProperties();
	afx_msg void OnProjectEnvironment();
   afx_msg void OnProjectBridgeDesc();
	afx_msg void OnProjectSpec();
   afx_msg void OnRatingSpec();
	afx_msg void OnProjectAutoCalc();
	afx_msg void OnUpdateProjectAutoCalc(CCmdUI* pCmdUI);
	afx_msg void OnExportToTemplateFile();
	afx_msg void OnViewsettingsBridgemodelEditor();
	afx_msg void OnLoadsLoadModifiers();
	afx_msg void OnViewsettingsGirderEditor();
	afx_msg void OnProjectDesignGirder();
	afx_msg void OnProjectDesignGirderDirect();
   afx_msg void OnUpdateProjectDesignGirderDirect(CCmdUI* pCmdUI);
	afx_msg void OnProjectDesignGirderDirectHoldSlabOffset();
   afx_msg void OnUpdateProjectDesignGirderDirectHoldSlabOffset(CCmdUI* pCmdUI);
	afx_msg void OnEditGirder();
	afx_msg void OnCopyGirderProps();
	afx_msg void OnImportProjectLibrary();
	afx_msg void OnAddPointload();
	afx_msg void OnAddDistributedLoad();
	afx_msg void OnAddMomentLoad();
   afx_msg void OnConstructionLoads();
	afx_msg void OnProjectAlignment();
   afx_msg void OnProjectAnalysis();
	afx_msg void OnEditPier();
	afx_msg void OnEditSpan();
	afx_msg void OnDeleteSelection();
	afx_msg void OnUpdateDeleteSelection(CCmdUI* pCmdUI);
	afx_msg void OnInsert();
	afx_msg void OnOptionsHints();
   afx_msg void OnOptionsLabels();
   afx_msg void OnUpdateNow();
	afx_msg void OnUpdateUpdateNow(CCmdUI* pCmdUI);
   //}}AFX_MSG
   afx_msg void OnViewStatusCenter(UINT nID);
   afx_msg void OnUpdateViewReports(CCmdUI* pCmdUI);
   afx_msg BOOL OnViewReports(NMHDR* pnmtb,LRESULT* plr);
   afx_msg void OnImport(UINT nID);
   afx_msg void OnExport(UINT nID);
   afx_msg void OnImportMenu(CCmdUI* pCmdUI);
   afx_msg void OnExportMenu(CCmdUI* pCmdUI);

public:
	afx_msg void OnViewBridgeModelEditor();
	afx_msg void OnViewGirderEditor();
	afx_msg void OnViewAnalysisResults();
   afx_msg void OnViewStability();
   afx_msg void OnEditUserLoads();
	afx_msg void OnViewLibraryEditor();
	afx_msg void OnEffectiveFlangeWidth();

   bool LoadMasterLibrary();
   bool DoLoadMasterLibrary(const CString& rPath);

   void InitProjectProperties();

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PGSUPERDOC_H__59D503F2_265C_11D2_8EB0_006097DF3C68__INCLUDED_)
