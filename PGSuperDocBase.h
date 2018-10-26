///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2017  Washington State Department of Transportation
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

// PGSuperDocBase.h : interface of the CPGSDocBase class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include <EAF\EAFBrokerDocument.h>
#include <EAF\EAFAutoCalcDoc.h>

#include <PsgLib\ISupportLibraryManager.h>
#include <PsgLib\LibraryManager.h>
#include <IReportManager.h>
#include <IGraphManager.h>

#include <IFace\ViewEvents.h> 
#include <IFace\Selection.h> 
#include <IFace\ExtendUI.h>

#include <WBFLUnitServer.h>

#include "Hints.h"
#include "pgsExt\BaseCommandLineInfo.h"
#include "PGSuperPluginMgr.h"

#include "CopyGirderPropertiesCallbacks.h"

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
#define VS_BRIDGE_ALIGNMENT   2
#define VS_BRIDGE_PROFILE     3

class CCopyGirderDlg;
class pgsSegmentDesignArtifact;
class CPGSuperDocProxyAgent;

/*--------------------------------------------------------------------*/
class CPGSDocBase : public CEAFBrokerDocument, 
                    public CEAFAutoCalcDocMixin,  // mix-in class for auto-calc capabilities
                    public libISupportLibraryManager
{
protected: // create from serialization only
	CPGSDocBase();

// CBrokerDocument over-rides
protected:
   virtual BOOL Init();
   virtual BOOL LoadSpecialAgents(IBrokerInitEx2* pBrokerInit); 
   virtual void OnChangedFavoriteReports(BOOL bIsFavorites, BOOL bFromMenu);
   virtual void ShowCustomReportHelp(eafTypes::CustomReportHelp helpType);
   virtual void ShowCustomReportDefinitionHelp();

// CEAFAutoCalcDocMixin over-rides
public:
   virtual bool IsAutoCalcEnabled() const;
   virtual void EnableAutoCalc(bool bEnable);

// Operations
public:
   CSelection GetSelection();
   void SetSelection(const CSelection& selection,BOOL bNotify=TRUE);
   void SelectPier(PierIndexType pierIdx,BOOL bNotify=TRUE);
   void SelectSpan(SpanIndexType spanIdx,BOOL bNotify=TRUE);
   void SelectGirder(const CGirderKey& girderKey,BOOL bNotify=TRUE);
   void SelectSegment(const CSegmentKey& segmentKey,BOOL bNotify=TRUE);
   void SelectClosureJoint(const CClosureKey& closureKey,BOOL bNotify=TRUE);
   void SelectTemporarySupport(SupportIDType tsID,BOOL bNotify=TRUE);
   void SelectDeck(BOOL bNotify=TRUE);
   void SelectAlignment(BOOL bNotify=TRUE);
   void ClearSelection(BOOL bNotify=TRUE);

   void OnLibMgrChanged(psgLibraryManager* pNewLibMgr);

   void PopulateReportMenu();
   void PopulateGraphMenu();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPGSDocBase)
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
	virtual ~CPGSDocBase();
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
   bool EditSpanDescription(SpanIndexType spanIdx, int nPage);
   bool EditPierDescription(PierIndexType pierIdx, int nPage);
   bool EditDirectSelectionPrestressing(const CSegmentKey& segmentKey);
   bool EditDirectInputPrestressing(const CSegmentKey& segmentKey);

   virtual bool EditGirderDescription(const CGirderKey& girderKey,int nPage) = 0;
   virtual bool EditGirderSegmentDescription(const CSegmentKey& segmentKey,int nPage) = 0;
   virtual bool EditClosureJointDescription(const CClosureKey& closureKey,int nPage) = 0;

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


   void GetDocUnitSystem(IDocUnitSystem** ppDocUnitSystem);

   void DeletePier(SpanIndexType pierIdx);
   void DeletePier(SpanIndexType pierIdx,pgsTypes::PierFaceType face);
   void DeleteSpan(SpanIndexType spanIdx);
   void DeleteSpan(SpanIndexType spanIdx,pgsTypes::RemovePierType pierRemoveType);
   void InsertSpan(PierIndexType refPierIdx,pgsTypes::PierFaceType pierFace,Float64 spanLength,bool bCreateNewGroup,IndexType eventIdx);


   // set/get view settings for bridge model editor
   UINT GetBridgeEditorSettings() const;
   void SetBridgeEditorSettings(UINT settings);

   // set/get view settings for alignment/profile editor
   UINT GetAlignmentEditorSettings() const;
   void SetAlignmentEditorSettings(UINT settings);

   // set/get view settings for Girder model editor
   UINT GetGirderEditorSettings() const;
   void SetGirderEditorSettings(UINT settings);

   // called when the UI Hints have been reset
   virtual void ResetUIHints();

   bool ShowProjectPropertiesOnNewProject();
   void ShowProjectPropertiesOnNewProject(bool bShow);

   BOOL UpdateTemplates();

   IDType RegisterBridgePlanViewCallback(IBridgePlanViewEventCallback* pCallback);
   bool UnregisterBridgePlanViewCallback(IDType ID);
   const std::map<IDType,IBridgePlanViewEventCallback*>& GetBridgePlanViewCallbacks();

   IDType RegisterBridgeSectionViewCallback(IBridgeSectionViewEventCallback* pCallback);
   bool UnregisterBridgeSectionViewCallback(IDType ID);
   const std::map<IDType,IBridgeSectionViewEventCallback*>& GetBridgeSectionViewCallbacks();

   IDType RegisterAlignmentPlanViewCallback(IAlignmentPlanViewEventCallback* pCallback);
   bool UnregisterAlignmentPlanViewCallback(IDType ID);
   const std::map<IDType,IAlignmentPlanViewEventCallback*>& GetAlignmentPlanViewCallbacks();

   IDType RegisterAlignmentProfileViewCallback(IAlignmentProfileViewEventCallback* pCallback);
   bool UnregisterAlignmentProfileViewCallback(IDType ID);
   const std::map<IDType,IAlignmentProfileViewEventCallback*>& GetAlignmentProfileViewCallbacks();

   IDType RegisterGirderElevationViewCallback(IGirderElevationViewEventCallback* pCallback);
   bool UnregisterGirderElevationViewCallback(IDType ID);
   const std::map<IDType,IGirderElevationViewEventCallback*>& GetGirderElevationViewCallbacks();

   IDType RegisterGirderSectionViewCallback(IGirderSectionViewEventCallback* pCallback);
   bool UnregisterGirderSectionViewCallback(IDType ID);
   const std::map<IDType,IGirderSectionViewEventCallback*>& GetGirderSectionViewCallbacks();

   IDType RegisterEditPierCallback(IEditPierCallback* pCallback);
   bool UnregisterEditPierCallback(IDType ID);
   const std::map<IDType,IEditPierCallback*>& GetEditPierCallbacks();

   IDType RegisterEditTemporarySupportCallback(IEditTemporarySupportCallback* pCallback);
   bool UnregisterEditTemporarySupportCallback(IDType ID);
   const std::map<IDType,IEditTemporarySupportCallback*>& GetEditTemporarySupportCallbacks();

   IDType RegisterEditSpanCallback(IEditSpanCallback* pCallback);
   bool UnregisterEditSpanCallback(IDType ID);
   const std::map<IDType,IEditSpanCallback*>& GetEditSpanCallbacks();

   IDType RegisterEditGirderCallback(IEditGirderCallback* pCallback,ICopyGirderPropertiesCallback* pCopyCallback);
   bool UnregisterEditGirderCallback(IDType ID);
   const std::map<IDType,IEditGirderCallback*>& GetEditGirderCallbacks();
   const std::map<IDType,ICopyGirderPropertiesCallback*>& GetCopyGirderPropertiesCallbacks();

   IDType RegisterEditSplicedGirderCallback(IEditSplicedGirderCallback* pCallback,ICopyGirderPropertiesCallback* pCopyCallback);
   bool UnregisterEditSplicedGirderCallback(IDType ID);
   const std::map<IDType,IEditSplicedGirderCallback*>& GetEditSplicedGirderCallbacks();
   const std::map<IDType,ICopyGirderPropertiesCallback*>& GetCopySplicedGirderPropertiesCallbacks();

   IDType RegisterEditSegmentCallback(IEditSegmentCallback* pCallback);
   bool UnregisterEditSegmentCallback(IDType ID);
   const std::map<IDType,IEditSegmentCallback*>& GetEditSegmentCallbacks();

   IDType RegisterEditClosureJointCallback(IEditClosureJointCallback* pCallback);
   bool UnregisterEditClosureJointCallback(IDType ID);
   const std::map<IDType,IEditClosureJointCallback*>& GetEditClosureJointCallbacks();

   IDType RegisterEditBridgeCallback(IEditBridgeCallback* pCallback);
   bool UnregisterEditBridgeCallback(IDType ID);
   const std::map<IDType,IEditBridgeCallback*>& GetEditBridgeCallbacks();

   IDType RegisterEditLoadRatingOptionsCallback(IEditLoadRatingOptionsCallback* pCallback);
   bool UnregisterEditLoadRatingOptionsCallback(IDType ID);
   const std::map<IDType,IEditLoadRatingOptionsCallback*>& GetEditLoadRatingOptionsCallbacks();

   virtual UINT GetStandardToolbarResourceID() = 0;

   long GetReportViewKey();

protected:

   CPGSuperDocProxyAgent* m_pPGSuperDocProxyAgent;

   bool m_bSelectingGirder;
   bool m_bSelectingSegment;
   bool m_bClearingSelection;

   IDType m_CallbackID;
   // View Notification Callbacks
   std::map<IDType,IBridgePlanViewEventCallback*>       m_BridgePlanViewCallbacks;
   std::map<IDType,IBridgeSectionViewEventCallback*>    m_BridgeSectionViewCallbacks;
   std::map<IDType,IAlignmentPlanViewEventCallback*>    m_AlignmentPlanViewCallbacks;
   std::map<IDType,IAlignmentProfileViewEventCallback*> m_AlignmentProfileViewCallbacks;
   std::map<IDType,IGirderElevationViewEventCallback*>  m_GirderElevationViewCallbacks;
   std::map<IDType,IGirderSectionViewEventCallback*>    m_GirderSectionViewCallbacks;

   // UI/Dialog Extension Callbacks
   std::map<IDType,IEditPierCallback*>              m_EditPierCallbacks;
   std::map<IDType,IEditTemporarySupportCallback*>  m_EditTemporarySupportCallbacks;
   std::map<IDType,IEditSpanCallback*>              m_EditSpanCallbacks;
   std::map<IDType,IEditGirderCallback*>            m_EditGirderCallbacks;
   std::map<IDType,ICopyGirderPropertiesCallback*>  m_CopyGirderPropertiesCallbacks;
   std::map<IDType,IEditSplicedGirderCallback*>     m_EditSplicedGirderCallbacks;
   std::map<IDType,ICopyGirderPropertiesCallback*>  m_CopySplicedGirderPropertiesCallbacks;
   std::map<IDType,IEditSegmentCallback*>           m_EditSegmentCallbacks;
   std::map<IDType,IEditClosureJointCallback*>      m_EditClosureJointCallbacks;
   std::map<IDType,IEditBridgeCallback*>            m_EditBridgeCallbacks;
   std::map<IDType,IEditLoadRatingOptionsCallback*> m_EditLoadRatingOptionsCallbacks;

   // these are the standard copy girder callbacks
   CCopyGirderType         m_CopyGirderType;
   CCopyGirderStirrups     m_CopyGirderStirrups;
   CCopyGirderPrestressing m_CopyGirderPrestressing;
   CCopyGirderHandling     m_CopyGirderHandling;
   CCopyGirderMaterial     m_CopyGirderMaterials;
   CCopyGirderRebar        m_CopyGirderRebar;
   CCopyGirderSlabOffset   m_CopyGirderSlabOffset;


   psgLibraryManager m_LibMgr;

   bool m_bAutoCalcEnabled;
   UINT m_BridgeModelEditorSettings;
   UINT m_AlignmentEditorSettings;
   UINT m_GirderModelEditorSettings;
   bool m_bShowProjectProperties;

   CComPtr<IDocUnitSystem> m_DocUnitSystem;
   
   virtual CPGSuperPluginMgrBase* CreatePluginManager() = 0;
   CPGSuperPluginMgrBase* m_pPluginMgr; // manages data importer and exporter plugins

   CSelection m_Selection;

   arSlabOffsetDesignType m_DesignSlabOffset;

   // callback IDs for any status callbacks we register
   StatusCallbackIDType m_scidInformationalError;
   StatusGroupIDType m_StatusGroupID;

   virtual void LoadToolbarState();
   virtual void SaveToolbarState();

   virtual void OnCreateInitialize();
   virtual void OnCreateFinalize();

   virtual LPCTSTR GetTemplateExtension() = 0;
   virtual CATID GetComponentInfoCategoryID() = 0;

   virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
   virtual BOOL OpenTheDocument(LPCTSTR lpszPathName);

   virtual void HandleOpenDocumentError( HRESULT hr, LPCTSTR lpszPathName );
   virtual void HandleSaveDocumentError( HRESULT hr, LPCTSTR lpszPathName );

   virtual HRESULT ConvertTheDocument(LPCTSTR lpszPathName, CString* realFileName);
   virtual void HandleConvertDocumentError( HRESULT hr, LPCTSTR lpszPathName );

   virtual CString GetRootNodeName();
   virtual Float64 GetRootNodeVersion();

   virtual HRESULT LoadTheDocument(IStructuredLoad* pStrLoad);
   virtual HRESULT WriteTheDocument(IStructuredSave* pStrSave);

   virtual void OnErrorDeletingBadSave(LPCTSTR lpszPathName,LPCTSTR lpszBackup);
   virtual void OnErrorRemaningSaveBackup(LPCTSTR lpszPathName,LPCTSTR lpszBackup);

   virtual void LoadDocumentSettings();
   virtual void SaveDocumentSettings();

   virtual void LoadDocumentationMap();
   virtual CString GetDocumentationRootLocation();
   virtual eafTypes::HelpResult GetDocumentLocation(LPCTSTR lpszDocSetName,UINT nHID,CString& strURL);

   virtual void OnLogFileOpened(); // called when the log file is first opened

   virtual void BrokerShutDown();
   virtual void OnStatusChanged();

   virtual BOOL CreateBroker();
   virtual HINSTANCE GetResourceInstance();

   BOOL UpdateTemplates(IProgress* pProgress,LPCTSTR lpszDir);

   virtual CString GetToolbarSectionName();

   virtual void CreateReportView(CollectionIndexType rptIdx,BOOL bPrompt);
   virtual void CreateGraphView(CollectionIndexType graphIdx);

   virtual void DeleteContents();

   virtual CATID GetBeamFamilyCategoryID() = 0;

   virtual BOOL LoadAgents();

// Generated message map functions
protected:
	//{{AFX_MSG(CPGSDocBase)
	afx_msg void OnFileProjectProperties();
	afx_msg void OnProjectEnvironment();
   afx_msg void OnRatingSpec();
	afx_msg void OnExportToTemplateFile();
	afx_msg void OnViewsettingsBridgemodelEditor();
	afx_msg void OnLoadsLoadModifiers();
   afx_msg void OnLoadsLoadFactors();
	afx_msg void OnViewsettingsGirderEditor();
	afx_msg void OnCopyGirderProps();
	afx_msg void OnImportProjectLibrary();
	afx_msg void OnAddPointload();
	afx_msg void OnAddDistributedLoad();
	afx_msg void OnAddMomentLoad();
   afx_msg void OnConstructionLoads();
	afx_msg void OnProjectAlignment();
   afx_msg void OnProjectProfile();
	afx_msg void OnEditPier();
	afx_msg void OnEditSpan();
	afx_msg void OnDeleteSelection();
	afx_msg void OnUpdateDeleteSelection(CCmdUI* pCmdUI);
	afx_msg void OnInsert();
   afx_msg void OnOptionsLabels();
   afx_msg void OnUpdateNow();
	afx_msg void OnUpdateUpdateNow(CCmdUI* pCmdUI);
   afx_msg void OnLosses();
   afx_msg void OnEditTimeline();
   //}}AFX_MSG
   afx_msg void OnViewStatusCenter(UINT nID);
   afx_msg void OnUpdateViewGraphs(CCmdUI* pCmdUI);
   afx_msg BOOL OnViewGraphs(NMHDR* pnmtb,LRESULT* plr);
   afx_msg void OnUpdateViewReports(CCmdUI* pCmdUI);
   afx_msg BOOL OnViewReports(NMHDR* pnmtb,LRESULT* plr);
   afx_msg void OnImport(UINT nID);
   afx_msg void OnExport(UINT nID);
   afx_msg void OnImportMenu(CCmdUI* pCmdUI);
   afx_msg void OnExportMenu(CCmdUI* pCmdUI);
   afx_msg void OnAutoCalc();
   afx_msg void OnUpdateAutoCalc(CCmdUI* pCmdUI);

   afx_msg void OnHelpFinder();
   afx_msg void OnAbout();

public:
	afx_msg void OnViewBridgeModelEditor();
	afx_msg void OnViewGirderEditor();
   afx_msg void OnEditUserLoads();
	afx_msg void OnViewLibraryEditor();
	afx_msg void OnEffectiveFlangeWidth();
	afx_msg void OnProjectSpec();
   afx_msg void OnEditHaunch();
   afx_msg void OnUpdateEditHaunch(CCmdUI* pCmdUI);

   bool LoadMasterLibrary();
   bool DoLoadMasterLibrary(const CString& rPath);

   void InitProjectProperties();

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

