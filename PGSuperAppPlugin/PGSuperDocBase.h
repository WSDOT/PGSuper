///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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

#include <PgsExt\TxnManager.h>

#include <IFace\ViewEvents.h> 
#include <IFace\Selection.h> 
#include <IFace\ExtendUI.h>

#include <WBFLUnitServer.h>

#include "Hints.h"
#include <PgsExt\BaseCommandLineInfo.h>
#include "PGSuperPluginMgr.h"

#include "CopyGirderPropertiesCallbacks.h"
#include "CopyPierPropertiesCallbacks.h"
#include "CopyTempSupportPropertiesCallbacks.h"

#define PGSUPER_DOCUMENT_ROOT_NODE_VERSION 3.0

#define VS_GIRDER_ELEVATION   0
#define VS_GIRDER_SECTION     1

#define VS_BRIDGE_PLAN        0
#define VS_BRIDGE_SECTION     1
#define VS_BRIDGE_ALIGNMENT   2
#define VS_BRIDGE_PROFILE     3

class pgsSegmentDesignArtifact;
class CPGSuperDocProxyAgent;

// A simple class to keep track of state information
// for file compatibility
class CFileCompatibilityState
{
public:
   CFileCompatibilityState() : m_bCreatingFromTemplate(false) { ResetFlags(); }

   // Set/Get version of application that was used to when saving a file (after version 2.1)
   void SetApplicationVersionFromFile(LPCTSTR lpszAppVersion) { m_strAppVersionFromFile = lpszAppVersion; }
   const CString& GetApplicationVersionFromFile() const { return m_strAppVersionFromFile; }

   CString GetApplicationVersion() const;

   // Set this flag if the application used to save this file was version 2.1 or earlier
   void SetPreVersion21Flag() { m_bPreVersion21File = true;  }

   void CreatingFromTemplate() { m_bCreatingFromTemplate = true; }

   // Call when a new file is created
   void NewFileCreated() { ResetFlags(); m_strFilePath.Empty(); m_bCreatingFromTemplate = false; m_bNewFromTemplate = true; }

   // Call when a file was opened. Keeps track of original filename and if the file was created from a template
   void FileOpened(LPCTSTR lpszFilePath) { ResetFlags(); m_strFilePath = lpszFilePath; m_bNewFromTemplate = false;  }

   // Call when a file is saved. Updates the file name and the version of the application when saved
   void FileSaved(LPCTSTR lpszFilePath, LPCTSTR lpszAppVersion) { ResetFlags();  m_strFilePath = lpszFilePath; m_strAppVersionFromFile = lpszAppVersion; }

   // Call at the beginning of the file saving process. Call with true of the file is unnamed (e.g. a new file that hasn't been saved or a Save As)
   void Saving(bool bUnnamed) { m_bUnnamed = bUnnamed; }

   // Returns the file name that will be used when making a copy of the original file
   CString GetCopyFileName() const;


   CString GetFileName() const { return m_strFilePath; }

   CString GetAppVersionForComparison(const CString& strAppVersion) const;
   
   // Returns true if the user should be warned that the file format is going to change
   // lpszPathName is name of file that is going to be saved
   // lpszCurrentAppVersion is the application version of the application right now
   bool PromptToMakeCopy(LPCTSTR lpszPathName, LPCTSTR lpszCurrentAppVersion) const;

private:
   void ResetFlags()
   {
      m_bPreVersion21File = false;
      m_bUnnamed = false;
      m_bNewFromTemplate = false;
   }

   CString m_strFilePath;
   CString m_strAppVersionFromFile;
   bool m_bPreVersion21File; // while was created with Version 2.1 or earlier
   bool m_bUnnamed;
   bool m_bNewFromTemplate;
   bool m_bCreatingFromTemplate;
};

/*--------------------------------------------------------------------*/
class CPGSDocBase : public CEAFBrokerDocument, 
                    public CEAFAutoCalcDocMixin,  // mix-in class for auto-calc capabilities
                    public libISupportLibraryManager
{
protected: // create from serialization only
	CPGSDocBase();

// CBrokerDocument over-rides
protected:
   virtual BOOL Init() override;
   virtual BOOL LoadSpecialAgents(IBrokerInitEx2* pBrokerInit) override; 
   virtual void OnChangedFavoriteReports(BOOL bIsFavorites, BOOL bFromMenu) override;
   virtual void ShowCustomReportHelp(eafTypes::CustomReportHelp helpType) override;
   virtual void ShowCustomReportDefinitionHelp() override;

// CEAFAutoCalcDocMixin over-rides
public:
   virtual bool IsAutoCalcEnabled() const override;
   virtual void EnableAutoCalc(bool bEnable) override;

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
   void SelectTrafficBarrier(pgsTypes::TrafficBarrierOrientation orientation,BOOL bNotify = TRUE);
   void ClearSelection(BOOL bNotify=TRUE);

   void OnLibMgrChanged(psgLibraryManager* pNewLibMgr);

   void PopulateReportMenu();
   void PopulateGraphMenu();
   void PopulateCopyGirderMenu();
   void PopulateCopyPierMenu();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPGSDocBase)
	public:
	virtual BOOL OnNewDocumentFromTemplate(LPCTSTR lpszPathName) override;
   virtual void OnCloseDocument() override;
   virtual BOOL DoSave(LPCTSTR lpszPathName, BOOL bReplace = TRUE) override;
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo) override;
	//}}AFX_VIRTUAL

   void UpdateProjectCriteriaIndicator();
   void UpdateAnalysisTypeStatusIndicator();
   void OnLoadsLldf();
   void OnLoadsLldf(pgsTypes::DistributionFactorMethod method,LldfRangeOfApplicabilityAction roaAction);
   void OnLiveLoads();

   virtual BOOL GetStatusBarMessageString(UINT nID,CString& rMessage) const override;
   virtual BOOL GetToolTipMessageString(UINT nID, CString& rMessage) const override;

// Implementation
public:
	virtual ~CPGSDocBase();
#ifdef _DEBUG
	virtual void AssertValid() const override;
	virtual void Dump(CDumpContext& dc) const override;
#endif

   IDType RegisterBridgePlanViewCallback(IBridgePlanViewEventCallback* pCallback);
   bool UnregisterBridgePlanViewCallback(IDType ID);
   const std::map<IDType, IBridgePlanViewEventCallback*>& GetBridgePlanViewCallbacks();

   IDType RegisterBridgeSectionViewCallback(IBridgeSectionViewEventCallback* pCallback);
   bool UnregisterBridgeSectionViewCallback(IDType ID);
   const std::map<IDType, IBridgeSectionViewEventCallback*>& GetBridgeSectionViewCallbacks();

   IDType RegisterAlignmentPlanViewCallback(IAlignmentPlanViewEventCallback* pCallback);
   bool UnregisterAlignmentPlanViewCallback(IDType ID);
   const std::map<IDType, IAlignmentPlanViewEventCallback*>& GetAlignmentPlanViewCallbacks();

   IDType RegisterAlignmentProfileViewCallback(IAlignmentProfileViewEventCallback* pCallback);
   bool UnregisterAlignmentProfileViewCallback(IDType ID);
   const std::map<IDType, IAlignmentProfileViewEventCallback*>& GetAlignmentProfileViewCallbacks();

   IDType RegisterGirderElevationViewCallback(IGirderElevationViewEventCallback* pCallback);
   bool UnregisterGirderElevationViewCallback(IDType ID);
   const std::map<IDType, IGirderElevationViewEventCallback*>& GetGirderElevationViewCallbacks();

   IDType RegisterGirderSectionViewCallback(IGirderSectionViewEventCallback* pCallback);
   bool UnregisterGirderSectionViewCallback(IDType ID);
   const std::map<IDType, IGirderSectionViewEventCallback*>& GetGirderSectionViewCallbacks();

   IDType RegisterEditPierCallback(IEditPierCallback* pCallback, ICopyPierPropertiesCallback* pCopyCallback);
   bool UnregisterEditPierCallback(IDType ID);
   const std::map<IDType, IEditPierCallback*>& GetEditPierCallbacks();
   const std::map<IDType, ICopyPierPropertiesCallback*>& GetCopyPierPropertiesCallbacks();


   IDType RegisterEditTemporarySupportCallback(IEditTemporarySupportCallback* pCallback, ICopyTemporarySupportPropertiesCallback* pCopyCallBack);
   bool UnregisterEditTemporarySupportCallback(IDType ID);
   const std::map<IDType, IEditTemporarySupportCallback*>& GetEditTemporarySupportCallbacks();

   IDType RegisterEditSpanCallback(IEditSpanCallback* pCallback);
   bool UnregisterEditSpanCallback(IDType ID);
   const std::map<IDType, IEditSpanCallback*>& GetEditSpanCallbacks();

   IDType RegisterEditGirderCallback(IEditGirderCallback* pCallback, ICopyGirderPropertiesCallback* pCopyCallback);
   bool UnregisterEditGirderCallback(IDType ID);
   const std::map<IDType, IEditGirderCallback*>& GetEditGirderCallbacks();
   const std::map<IDType, ICopyGirderPropertiesCallback*>& GetCopyGirderPropertiesCallbacks();

   IDType RegisterEditSplicedGirderCallback(IEditSplicedGirderCallback* pCallback, ICopyGirderPropertiesCallback* pCopyCallback);
   bool UnregisterEditSplicedGirderCallback(IDType ID);
   const std::map<IDType, IEditSplicedGirderCallback*>& GetEditSplicedGirderCallbacks();
   const std::map<IDType, ICopyGirderPropertiesCallback*>& GetCopySplicedGirderPropertiesCallbacks();

   IDType RegisterEditSegmentCallback(IEditSegmentCallback* pCallback);
   bool UnregisterEditSegmentCallback(IDType ID);
   const std::map<IDType, IEditSegmentCallback*>& GetEditSegmentCallbacks();

   IDType RegisterEditClosureJointCallback(IEditClosureJointCallback* pCallback);
   bool UnregisterEditClosureJointCallback(IDType ID);
   const std::map<IDType, IEditClosureJointCallback*>& GetEditClosureJointCallbacks();

   IDType RegisterEditBridgeCallback(IEditBridgeCallback* pCallback);
   bool UnregisterEditBridgeCallback(IDType ID);
   const std::map<IDType, IEditBridgeCallback*>& GetEditBridgeCallbacks();

   IDType RegisterEditLoadRatingOptionsCallback(IEditLoadRatingOptionsCallback* pCallback);
   bool UnregisterEditLoadRatingOptionsCallback(IDType ID);
   const std::map<IDType, IEditLoadRatingOptionsCallback*>& GetEditLoadRatingOptionsCallbacks();

   // ISupportLibraryManager
   virtual CollectionIndexType GetNumberOfLibraryManagers() const override;
   virtual libLibraryManager* GetLibraryManager(CollectionIndexType num) override;
   virtual libLibraryManager* GetTargetLibraryManager() override;

   bool EditBridgeDescription(int nPage);
   bool EditAlignmentDescription(int nPage);
   bool EditSpanDescription(SpanIndexType spanIdx, int nPage);
   bool EditPierDescription(PierIndexType pierIdx, int nPage);
   bool EditTemporarySupportDescription(PierIndexType pierIdx, int nPage);
   bool EditDirectSelectionPrestressing(const CSegmentKey& segmentKey);
   bool EditDirectRowInputPrestressing(const CSegmentKey& segmentKey);
   bool EditDirectStrandInputPrestressing(const CSegmentKey& segmentKey);
   
   bool EditGirderDescription();
   bool EditGirderSegmentDescription();

   // Return true if the edit was completed, otherwise return false (return false if the edit was canceled)
   virtual bool EditGirderDescription(const CGirderKey& girderKey,int nPage) = 0;
   virtual bool EditGirderSegmentDescription(const CSegmentKey& segmentKey,int nPage) = 0;
   virtual bool EditClosureJointDescription(const CClosureKey& closureKey,int nPage) = 0;

   void AddPointLoad(const CPointLoadData& loadData);
   bool EditPointLoad(CollectionIndexType loadIdx);
   bool EditPointLoadByID(LoadIDType loadID);
   void DeletePointLoad(CollectionIndexType loadIdx);
   void DeletePointLoadByID(LoadIDType loadID);

   void AddDistributedLoad(const CDistributedLoadData& loadData);
   bool EditDistributedLoad(CollectionIndexType loadIdx);
   bool EditDistributedLoadByID(LoadIDType loadID);
   void DeleteDistributedLoad(CollectionIndexType loadIdx);
   void DeleteDistributedLoadByID(LoadIDType loadID);

   void AddMomentLoad(const CMomentLoadData& loadData);
   bool EditMomentLoad(CollectionIndexType loadIdx);
   bool EditMomentLoadByID(LoadIDType loadID);
   void DeleteMomentLoad(CollectionIndexType loadIdx);
   void DeleteMomentLoadByID(LoadIDType loadID);

   bool EditTimeline();
   bool EditCastDeckActivity();
   bool EditEffectiveFlangeWidth();
   bool SelectProjectCriteria();

   void EditGirderViewSettings(int nPage);
   void EditBridgeViewSettings(int nPage);


   void GetDocUnitSystem(IDocUnitSystem** ppDocUnitSystem);

   void DeletePier(PierIndexType pierIdx);
   void DeletePier(PierIndexType pierIdx,pgsTypes::PierFaceType face);
   void DeleteSpan(SpanIndexType spanIdx);
   void DeleteSpan(SpanIndexType spanIdx,pgsTypes::RemovePierType pierRemoveType);
   void InsertSpan(PierIndexType refPierIdx, pgsTypes::PierFaceType pierFace, Float64 spanLength, bool bCreateNewGroup, EventIndexType eventIdx);

   // set/get view settings for bridge model editor
   UINT GetBridgeEditorSettings() const;
   void SetBridgeEditorSettings(UINT settings,BOOL bNotify=TRUE);

   // set/get view settings for alignment/profile editor
   UINT GetAlignmentEditorSettings() const;
   void SetAlignmentEditorSettings(UINT settings, BOOL bNotify = TRUE);

   // set/get view settings for Girder model editor
   UINT GetGirderEditorSettings() const;
   void SetGirderEditorSettings(UINT settings, BOOL bNotify = TRUE);


   BOOL UpdateTemplates();

   long GetReportViewKey();

   virtual UINT GetStandardToolbarResourceID() = 0;

protected:
   // Override default behavior
   virtual void ResetUIHints(bool bPrompt = TRUE) override;

   // called when the UI Hints have been reset
   virtual void OnUIHintsReset() override;


   bool ShowProjectPropertiesOnNewProject();
   void ShowProjectPropertiesOnNewProject(bool bShow);

   CPGSuperDocProxyAgent* m_pPGSuperDocProxyAgent;

   bool m_bSelectingGirder;
   bool m_bSelectingSegment;
   bool m_bClearingSelection;

   CFileCompatibilityState m_FileCompatibilityState;

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
   std::map<IDType,ICopyPierPropertiesCallback*>    m_CopyPierPropertiesCallbacks;
   std::map<IDType,IEditTemporarySupportCallback*>  m_EditTemporarySupportCallbacks;
   std::map<IDType,ICopyTemporarySupportPropertiesCallback*> m_CopyTempSupportPropertiesCallbacks;
   std::map<IDType,IEditSpanCallback*>              m_EditSpanCallbacks;
   std::map<IDType,IEditGirderCallback*>            m_EditGirderCallbacks;
   std::map<IDType,ICopyGirderPropertiesCallback*>  m_CopyGirderPropertiesCallbacks;
   std::map<IDType,IEditSplicedGirderCallback*>     m_EditSplicedGirderCallbacks;
   std::map<IDType,ICopyGirderPropertiesCallback*>  m_CopySplicedGirderPropertiesCallbacks;
   std::map<IDType,IEditSegmentCallback*>           m_EditSegmentCallbacks;
   std::map<IDType,IEditClosureJointCallback*>      m_EditClosureJointCallbacks;
   std::map<IDType,IEditBridgeCallback*>            m_EditBridgeCallbacks;
   std::map<IDType,IEditLoadRatingOptionsCallback*> m_EditLoadRatingOptionsCallbacks;

   // map  from menu cmd to callback ID
   std::map<UINT,IDType>  m_CopyGirderPropertiesCallbacksCmdMap;
   std::map<UINT,IDType>  m_CopyPierPropertiesCallbacksCmdMap;
   std::map<UINT,IDType>  m_CopyTempSupportPropertiesCallbacksCmdMap;

   // these are the standard copy pier callbacks
   CCopyPierAllProperties        m_CopyPierAllProperties;
   CCopyPierConnectionProperties m_CopyPierConnectionProperties;
   CCopyPierDiaphragmProperties  m_CopyPierDiaphragmProperties;
   CCopyPierModelProperties      m_CopyPierModelProperties;

   CCopyTempSupportConnectionProperties m_CopyTempSupportConnectionProperties;

   // these are the standard copy girder callbacks
   CCopyGirderAllProperties m_CopyGirderAllProperties;
   CCopyGirderStirrups      m_CopyGirderStirrups;
   CCopyGirderPrestressing  m_CopyGirderPrestressing;
   CCopyGirderHandling      m_CopyGirderHandling;
   CCopyGirderMaterial      m_CopyGirderMaterials;
   CCopyGirderRebar         m_CopyGirderRebar;


   psgLibraryManager m_LibMgr;

   bool m_bAutoCalcEnabled;
   UINT m_BridgeModelEditorSettings;
   UINT m_AlignmentEditorSettings;
   UINT m_GirderModelEditorSettings;
   bool m_bShowProjectProperties;

   CComPtr<IDocUnitSystem> m_DocUnitSystem;
   
   virtual CPGSuperPluginMgrBase* CreatePluginManager() = 0;
   CPGSuperPluginMgrBase* m_pPluginMgr; // manages data importer and exporter plug-ins

   CSelection m_Selection;

   // callback IDs for any status callbacks we register
   StatusCallbackIDType m_scidInformationalError;
   StatusGroupIDType m_StatusGroupID;

   virtual void LoadToolbarState() override;
   virtual void SaveToolbarState() override;

   virtual void OnCreateInitialize() override;
   virtual void OnCreateFinalize() override;

   virtual LPCTSTR GetTemplateExtension() = 0;
   virtual CATID GetComponentInfoCategoryID() = 0;

   virtual BOOL OnOpenDocument(LPCTSTR lpszPathName) override;
   virtual BOOL OpenTheDocument(LPCTSTR lpszPathName) override;

   virtual void HandleOpenDocumentError( HRESULT hr, LPCTSTR lpszPathName ) override;
   virtual void HandleSaveDocumentError( HRESULT hr, LPCTSTR lpszPathName ) override;

   virtual HRESULT ConvertTheDocument(LPCTSTR lpszPathName, CString* realFileName) override;
   virtual void HandleConvertDocumentError( HRESULT hr, LPCTSTR lpszPathName ) override;

   virtual CString GetRootNodeName() override;
   virtual Float64 GetRootNodeVersion() override;

   virtual HRESULT LoadTheDocument(IStructuredLoad* pStrLoad) override;
   virtual HRESULT WriteTheDocument(IStructuredSave* pStrSave) override;

   virtual void OnErrorDeletingBadSave(LPCTSTR lpszPathName,LPCTSTR lpszBackup) override;
   virtual void OnErrorRenamingSaveBackup(LPCTSTR lpszPathName,LPCTSTR lpszBackup) override;

   virtual void LoadDocumentSettings() override;
   virtual void SaveDocumentSettings() override;

   virtual void LoadDocumentationMap() override;
   virtual CString GetDocumentationRootLocation() override;
   virtual eafTypes::HelpResult GetDocumentLocation(LPCTSTR lpszDocSetName,UINT nHID,CString& strURL) override;

   virtual void OnLogFileOpened() override; // called when the log file is first opened

   virtual void BrokerShutDown() override;
   virtual void OnStatusChanged() override;

   virtual BOOL CreateBroker() override;
   virtual HINSTANCE GetResourceInstance() override;

   BOOL UpdateTemplates(IProgress* pProgress,LPCTSTR lpszDir);
   virtual void ModifyTemplate(LPCTSTR strTemplate); 

   virtual CString GetToolbarSectionName() override;

   virtual void CreateReportView(CollectionIndexType rptIdx,BOOL bPrompt) override;
   virtual void CreateGraphView(CollectionIndexType graphIdx) override;

   virtual void DeleteContents() override;

   virtual CATID GetBeamFamilyCategoryID() = 0;

   virtual BOOL LoadAgents() override;

   void UIHint(const CString& strText, UINT hint);

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
	afx_msg void OnCopyGirderProps(UINT nID);
	afx_msg void OnCopyGirderPropsAll();
	afx_msg void OnCopyPierProps(UINT nID);
	afx_msg void OnCopyPierPropsAll();
	afx_msg void OnImportProjectLibrary();
	afx_msg void OnAddPointload();
	afx_msg void OnAddDistributedLoad();
	afx_msg void OnAddMomentLoad();
   afx_msg void OnConstructionLoads();
	afx_msg void OnProjectAlignment();
   afx_msg void OnProjectProfile();
   afx_msg void OnProjectBarriers();
	afx_msg void OnEditPier();
	afx_msg void OnEditSpan();
	afx_msg void OnDeleteSelection();
	afx_msg void OnUpdateDeleteSelection(CCmdUI* pCmdUI);
	afx_msg void OnInsert();
   afx_msg void OnOptionsLabels();
   afx_msg void OnUpdateNow();
	afx_msg void OnUpdateUpdateNow(CCmdUI* pCmdUI);
   afx_msg void OnLosses();
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
   afx_msg void OnEditTimeline();
   afx_msg void OnUpdateCopyGirderPropsTb(CCmdUI* pCmdUI); // Tb means tool bar
   afx_msg BOOL OnCopyGirderPropsTb(NMHDR* pnmtb,LRESULT* plr);
   afx_msg void OnUpdateCopyPierPropsTb(CCmdUI* pCmdUI);
   afx_msg BOOL OnCopyPierPropsTb(NMHDR* pnmtb,LRESULT* plr);

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
   afx_msg void OnEditBearing();
   afx_msg void OnUpdateEditBearing(CCmdUI* pCmdUI);

   bool LoadMasterLibrary();
   bool DoLoadMasterLibrary(const CString& rPath);

   bool DoEditBearing();
   bool DoEditHaunch();

   void InitProjectProperties();

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

