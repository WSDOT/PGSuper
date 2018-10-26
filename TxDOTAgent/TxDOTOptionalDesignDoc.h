///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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
class CBridgeDescription;

/*--------------------------------------------------------------------*/
class CTxDOTOptionalDesignDoc : public CEAFBrokerDocument, public ITxDataObserver, public ITxDOTBrokerRetriever
{
protected: // create from serialization only
	CTxDOTOptionalDesignDoc();
	DECLARE_DYNCREATE(CTxDOTOptionalDesignDoc)

// CEAFBrokerDocument overrides
public:
   virtual CATID GetAgentCategoryID();
   virtual CATID GetExtensionAgentCategoryID();
   virtual BOOL Init();
   virtual BOOL LoadSpecialAgents(IBrokerInitEx2* pBrokerInit); 
   virtual CString GetToolbarSectionName();

   virtual void DoIntegrateWithUI(BOOL bIntegrate);

   virtual void LoadDocumentSettings();
   virtual void SaveDocumentSettings();

	virtual BOOL OnNewDocumentFromTemplate(LPCTSTR lpszPathName);
   virtual BOOL OpenTheDocument(LPCTSTR lpszPathName);
   virtual BOOL SaveTheDocument(LPCTSTR lpszPathName);
   virtual HRESULT WriteTheDocument(IStructuredSave* pStrSave);
   virtual HRESULT LoadTheDocument(IStructuredLoad* pStrLoad);

   // ITxDOTBrokerRetriever
   virtual IBroker* GetUpdatedBroker();
   virtual IBroker* GetClassicBroker();
   virtual GirderLibrary* GetGirderLibrary();

// Operations
public:
   // listen to data events
   virtual void OnTxDotDataChanged(int change);


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTxDOTOptionalDesignDoc)
	public:
//	virtual BOOL OnNewDocumentFromTemplate(LPCTSTR lpszPathName);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CTxDOTOptionalDesignDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
   virtual void OnCreateInitialize();
   virtual void OnCreateFinalize();

// Generated message map functions
protected:
	//{{AFX_MSG(CTxDOTOptionalDesignDoc)
   //}}AFX_MSG
	DECLARE_MESSAGE_MAP()

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

private:
   // Implementation
   void InitializeLibraryManager();
   void MarryPGSuperTemplateWithBroker(LPCTSTR lpszPathName);
   void UpdatePgsuperModelWithData();
   void VerifyPgsuperTemplateData(CBridgeDescription& bridgeDesc);
   void SetGirderData(CTxDOTOptionalDesignGirderData* pOdGirderData, GirderIndexType gdr,
                      const char* gdrName, const GirderLibraryEntry* pGdrEntry, Float64 EcBeam,
                      CGirderTypes* pGirderTypes);

   void RecreateBroker();

   BOOL ParseTemplateFile();
   BOOL ParseTemplateFile(LPCTSTR lpszPathName);

   HRESULT LoadThePGSuperDocument(IStructuredLoad* pStrLoad);
   void HandleOpenDocumentError( HRESULT hr, LPCTSTR lpszPathName );

   BOOL UpdateCurrentViewInputData();

public:
   afx_msg void OnFileSave();
   afx_msg void OnFileSaveas();
   afx_msg void OnFileExportPgsuperModel();
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.
