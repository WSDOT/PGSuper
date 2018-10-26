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

// PGSpliceDoc.h : interface of the CPGSpliceDoc class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "AutoCalcDoc.h"

#include <PsgLib\ISupportLibraryManager.h>
#include <PsgLib\LibraryManager.h>
#include <PgsExt\InterfaceCache.h>
#include <IReportManager.h>

#include "Hints.h"
#include "StatusCenterImp.h"
#include "StatusCenterDlg.h"
#include "PgsuperCommandLineInfo.h"

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


/*--------------------------------------------------------------------*/
class CPGSpliceDoc : public CAutoCalcDoc, public libISupportLibraryManager, public iStatusCenterEventSink
{
protected: // create from serialization only
	CPGSpliceDoc();
	DECLARE_DYNCREATE(CPGSpliceDoc)

// Attributes
public:

// Operations
public:
   virtual bool IsAutoCalcEnabled() const;
   virtual void EnableAutoCalc(bool bEnable);

   // flags for design dialog
   bool IsDesignFlexureEnabled() const;
   void EnableDesignFlexure( bool bEnable );
   bool IsDesignShearEnabled() const;
   void EnableDesignShear( bool bEnable );

   // index of selected object
   PierIndexType GetPierIdx();
   SpanIndexType GetSpanIdx();
   GirderIndexType GetGirderIdx();
   void SelectPier(PierIndexType pierIdx);
   void SelectSpan(SpanIndexType spanIdx);
   void SelectGirder(SpanIndexType spanIdx,GirderIndexType gdrIdx);

   virtual void OnUpdateError(const CString& errorMsg);

   void OnLibMgrChanged(psgLibraryManager* pNewLibMgr);

   bool DoTxDotCadReport(const CString& outputFileName, const CString& errorFileName, const CPGSuperCommandLineInfo& txInfo);

   // called by the framework to put the names of all available reports
   // on the Reports sub-menu
   void PopulateReportMenu();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPGSpliceDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	virtual BOOL OnSaveDocument(LPCTSTR lpszPathName);
	virtual void OnCloseDocument();
   virtual void DeleteContents();
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	//}}AFX_VIRTUAL
   virtual BOOL OnImportDocument(UINT nID);
   virtual BOOL OnExportDocument(UINT nID);
	virtual void SetModifiedFlag(BOOL bModified = TRUE);
   void UpdateAnalysisTypeStatusIndicator();
   void OnLoadsLldf();
   void OnLoadsLldf(pgsTypes::DistributionFactorMethod method,LldfRangeOfApplicabilityAction roaAction);
   void OnLiveLoads();

// Implementation
public:
	virtual ~CPGSpliceDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

   // ISupportLibraryManager
   virtual int GetNumberOfLibraryManagers() const;
   virtual libLibraryManager* GetLibraryManager(int num);
   virtual void SetUnitsMode(libUnitsMode::Mode mode);
   virtual libUnitsMode::Mode GetUnitsMode() const;
   virtual libLibraryManager* GetTargetLibraryManager();

   HRESULT GetBroker(IBroker** ppBroker);
   pgsStatusCenter& GetStatusCenter();

   void EditBridgeDescription(int nPage);
   void EditAlignmentDescription(int nPage);
   bool EditGirderDescription(SpanIndexType span,GirderIndexType girder, int nPage);
   bool EditSpanDescription(SpanIndexType spanIdx, int nPage);
   bool EditPierDescription(PierIndexType pierIdx, int nPage);
   bool EditPointLoad(CollectionIndexType loadIdx);
   bool EditDistributedLoad(CollectionIndexType loadIdx);
   bool EditMomentLoad(CollectionIndexType loadIdx);


   void EditGirderViewSettings(int nPage);
   void EditBridgeViewSettings(int nPage);

   void DesignGirderDirect(bool bDesignSlabOffset);
   void DoDesignGirder(SpanIndexType span,GirderIndexType gdr,const arDesignOptions& designOptions);

   void SaveFlexureDesign(SpanIndexType span,GirderIndexType gdr,const arDesignOptions& designOptions, const pgsDesignArtifact* pArtifact);

   void GetDocUnitSystem(IDocUnitSystem** ppDocUnitSystem);

   void DeletePier(SpanIndexType pierIdx);
   void DeletePier(SpanIndexType pierIdx,pgsTypes::PierFaceType face);
   void DeleteSpan(SpanIndexType spanIdx);
   void DeleteSpan(SpanIndexType spanIdx,pgsTypes::RemovePierType pierRemoveType);
   void InsertSpan(PierIndexType refPierIdx,pgsTypes::PierFaceType pierFace);

protected:
   DECLARE_AGENT_DATA;
   CComPtr<IBroker> m_TheRealBroker; // strong reference to broker (all other are weak)
   psgLibraryManager m_LibMgr;

   bool m_bIsReportMenuPopulated; 

   CComPtr<IDocUnitSystem> m_DocUnitSystem;

   PierIndexType   m_CurrentPierIdx;
   SpanIndexType   m_CurrentSpanIdx;
   GirderIndexType m_CurrentGirderIdx;

   bool m_DesignSlabOffset;

   pgsStatusCenter m_StatusCenter;
   CStatusCenterDlg m_StatusCenterDlg;

   friend CCopyGirderDlg;
   void OnApplyCopyGirder(SpanGirderHashType fromHash,std::vector<SpanGirderHashType> toHash,BOOL bGirder,BOOL bTransverse,BOOL bLongitudinalRebar,BOOL bPrestress,BOOL bHandling, BOOL bMaterial, BOOL bSlabOffset);

   BOOL Init();
   BOOL LoadAgents();
   BOOL LoadAgents(IBrokerInitEx2* pBrokerInit, CLSID* pClsid, long nClsid);
   BOOL OpenTheDocument(LPCTSTR lpszPathName);
   BOOL SaveTheDocument(LPCTSTR lpszPathName);
   // convert - returns same as vb convert function
   long ConvertTheDocument(LPCTSTR lpszPathName, CString* realFileName);
   void HandleOpenDocumentError( HRESULT hr, LPCTSTR lpszPathName );
   void HandleSaveDocumentError( HRESULT hr, LPCTSTR lpszPathName );
   void HandleConvertDocumentError( HRESULT hr, LPCTSTR lpszPathName );

   BOOL UpdateTemplates();

// Generated message map functions
protected:
	//{{AFX_MSG(CPGSpliceDoc)
	afx_msg void OnFileProjectProperties();
	afx_msg void OnProjectUnits();
   afx_msg void OnSiUnits();
   afx_msg void OnUpdateSiUnits(CCmdUI* pCmdUI);
   afx_msg void OnUsUnits();
   afx_msg void OnUpdateUsUnits(CCmdUI* pCmdUI);
	afx_msg void OnProjectEnvironment();
	afx_msg void OnProjectBridgeDesc();
	afx_msg void OnProjectSpec();
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
	afx_msg void OnEditUserloads();
	afx_msg void OnAddDistributedLoad();
	afx_msg void OnAddMomentLoad();
	afx_msg void OnProjectAlignment();
   afx_msg void OnReport(UINT nID);
   afx_msg void OnQuickReport(UINT nID);
   afx_msg void OnViewAnalysisResults();
	afx_msg void OnProjectAnalysis();
	afx_msg void OnEditPier();
	afx_msg void OnEditSpan();
	afx_msg void OnDeleteSelection();
	afx_msg void OnUpdateDeleteSelection(CCmdUI* pCmdUI);
	afx_msg void OnUndo();
	afx_msg void OnUpdateUndo(CCmdUI* pCmdUI);
	afx_msg void OnRedo();
	afx_msg void OnUpdateRedo(CCmdUI* pCmdUI);
	afx_msg void OnInsert();
	afx_msg void OnOptionsHints();
   afx_msg void OnOptionsLabels();
	//}}AFX_MSG
   afx_msg void OnUpdateStatusCenter(CCmdUI* pCmdUI);
	afx_msg void OnViewStatusCenter(UINT nID);
	DECLARE_MESSAGE_MAP()

	// Generated OLE dispatch map functions
	//{{AFX_DISPATCH(CPGSpliceDoc)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()
	DECLARE_INTERFACE_MAP()

   void SetUnitMode(pgsTypes::UnitMode unitsMode);
   pgsTypes::UnitMode GetUnitMode() const;
   bool LoadMasterLibrary();
   bool DoLoadMasterLibrary(const CString& rPath);

   void InitProjectProperties();
   int FindMenuItem(CMenu* pParentMenu,const char* strTargetMenu);

public:
   void OnStatusItemAdded(pgsStatusItem* pItem);
   void OnStatusItemRemoved(long id);

   UINT_PTR GetReportCommand(CollectionIndexType rptIdx,bool bQuickReport);
   CollectionIndexType GetReportIndex(UINT nID,bool bQuickReport);
};

