///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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

#ifndef INCLUDED_DOCPROXYAGENT_H_
#define INCLUDED_DOCPROXYAGENT_H_

// SYSTEM INCLUDES
//
#include <ObjBase.h>

// PROJECT INCLUDES
//
#include <IFace\File.h>
#include <IFace\StatusCenter.h>
#include <IFace\UpdateTemplates.h>
#include <IFace\Selection.h>
#include <IFace\EditByUI.h>

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//
class CPGSuperDoc;
struct IBroker;

// MISCELLANEOUS
//

/*****************************************************************************
CLASS 
   pgsDocProxyAgent

   Proxy agent for CPGSuper document.


DESCRIPTION
   Proxy agent for CPGSuper document.

   Instances of this object allow the CDocument class to be plugged into the
   Agent-Broker architecture.


COPYRIGHT
   Copyright © 1999-2022
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rab : 11.01.1998 : Created file
*****************************************************************************/

class pgsDocProxyAgent : public IAgentEx,
                         public IBridgeDescriptionEventSink,
                         public IEnvironmentEventSink,
                         public IProjectPropertiesEventSink,
                         public IProjectSettingsEventSink,
                         public ISpecificationEventSink,
                         public ILoadModifiersEventSink,
                         public ILibraryConflictEventSink,
                         public IFile,
                         public IUIEvents,
                         public IStatusCenter,
                         public IUpdateTemplates,
                         public ISelection,
                         public IEditByUI
{
public:
   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   pgsDocProxyAgent();

   //------------------------------------------------------------------------
   // Destructor
   ~pgsDocProxyAgent();

   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   // GROUP: ACCESS

   //------------------------------------------------------------------------
   void SetDocument(CPGSuperDoc* pDoc);

   // GROUP: INQUIRY

   // IUnknown
   STDMETHOD(QueryInterface)(const IID& iid, void** ppv);
   STDMETHOD_(ULONG,AddRef)();
   STDMETHOD_(ULONG,Release)();

   // IAgent
   STDMETHOD(SetBroker)(/*[in]*/ IBroker* pBroker);
	STDMETHOD(RegInterfaces)();
	STDMETHOD(Init)();
	STDMETHOD(Reset)();
	STDMETHOD(ShutDown)();
   STDMETHOD(Init2)();
   STDMETHOD(GetClassID)(CLSID* pCLSID);

   // IBridgeDescriptionEventSink
   virtual HRESULT OnBridgeChanged();
   virtual HRESULT OnGirderFamilyChanged();
   virtual HRESULT OnGirderChanged(SpanIndexType span,GirderIndexType gdr,Uint32 lHint);
   virtual HRESULT OnLiveLoadChanged();
   virtual HRESULT OnLiveLoadNameChanged(const char* strOldName,const char* strNewName);

   // IEnvironmentEventSink
   virtual HRESULT OnExposureConditionChanged();
   virtual HRESULT OnRelHumidityChanged();

   // IProjectSettingsEventSink
   virtual HRESULT OnUnitsChanged(Int32 units);

   // IProjectPropertiesEventSink
   virtual HRESULT OnProjectPropertiesChanged();

   // ISpecificationEventSink
   virtual HRESULT OnSpecificationChanged();
   virtual HRESULT OnAnalysisTypeChanged();

   // ILoadModifersEventSink
   virtual HRESULT OnLoadModifiersChanged();

   // ILibraryConflictSink
   virtual HRESULT OnLibraryConflictResolved();

   // IFile
   virtual std::string GetFileName();
   virtual std::string GetFileTitle();
   virtual std::string GetFilePath();
   virtual std::string GetFileRoot();

   // IStatusCenter
   virtual StatusCallbackIDType RegisterCallback(iStatusCallback* pCallback);
   virtual AgentIDType GetAgentID();
   virtual StatusItemIDType Add(pgsStatusItem* pItem);
   virtual bool RemoveByID(StatusItemIDType id);
   virtual bool RemoveByIndex(CollectionIndexType index);
   virtual bool RemoveByAgentID(AgentIDType agentID);
   virtual pgsStatusItem* GetByID(StatusItemIDType id);
   virtual pgsStatusItem* GetByIndex(CollectionIndexType index);
   virtual pgsTypes::StatusSeverityType GetSeverity(const pgsStatusItem* pItem);
   virtual CollectionIndexType Count();

   // IUpdateTemplates
   virtual bool UpdatingTemplates();

   // IUIEvents
   virtual void HoldEvents(bool bHold=true);
   virtual void FirePendingEvents();

   // ISelection
   virtual PierIndexType GetPierIdx();
   virtual SpanIndexType GetSpanIdx();
   virtual GirderIndexType GetGirderIdx();
   virtual void SelectPier(PierIndexType pierIdx);
   virtual void SelectSpan(SpanIndexType spanIdx);
   virtual void SelectGirder(SpanIndexType spanIdx,GirderIndexType gdrIdx);
   virtual Float64 GetSectionCutStation();

   // IEditByUI
   virtual void EditBridgeDescription(int nPage);
   virtual void EditAlignmentDescription(int nPage);
   virtual bool EditGirderDescription(SpanIndexType span,GirderIndexType girder, int nPage);
   virtual bool EditSpanDescription(SpanIndexType spanIdx, int nPage);
   virtual bool EditPierDescription(PierIndexType pierIdx, int nPage);
   virtual void EditLiveLoads();
   virtual void EditLiveLoadDistributionFactors(pgsTypes::DistributionFactorMethod method,LldfRangeOfApplicabilityAction roaAction);
   virtual bool EditPointLoad(CollectionIndexType loadIdx);
   virtual bool EditDistributedLoad(CollectionIndexType loadIdx);
   virtual bool EditMomentLoad(CollectionIndexType loadIdx);

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   // GROUP: ACCESS
   // GROUP: INQUIRY

private:
   // GROUP: DATA MEMBERS
   CPGSuperDoc* m_pDoc;
   LONG m_cRef; // ref count

   DECLARE_AGENT_DATA;

   DWORD m_dwBridgeDescCookie;
   DWORD m_dwEnvironmentCookie;
   DWORD m_dwProjectPropertiesCookie;
   DWORD m_dwProjectSettingsCookie;
   DWORD m_dwSpecificationCookie;
   DWORD m_dwLoadModiferCookie;
   DWORD m_dwLibraryConflictGuiCookie;

   // GROUP: LIFECYCLE

   // Prevent accidental copying and assignment
   pgsDocProxyAgent(const pgsDocProxyAgent&);
   pgsDocProxyAgent& operator=(const pgsDocProxyAgent&);

   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   // GROUP: ACCESS
   IUnknown* GetUnknown();

   bool m_bHoldingEvents;
   struct UIEvent
   {
      CView* pSender;
      LPARAM lHint;
      CObject* pHint;
   };
   std::vector<UIEvent> m_UIEvents;
   void FireEvent(CView* pSender,LPARAM lHint,CObject* pHint);


   // GROUP: INQUIRY

public:
   // GROUP: DEBUG
   #if defined _DEBUG
   //------------------------------------------------------------------------
   // Returns true if the object is in a valid state, otherwise returns false.
   virtual bool AssertValid() const;

   //------------------------------------------------------------------------
   // Dumps the contents of the object to the given dump context.
   virtual void Dump(dbgDumpContext& os) const;
   #endif // _DEBUG

   #if defined _UNITTEST
   //------------------------------------------------------------------------
   // Runs a self-diagnostic test.  Returns true if the test passed,
   // otherwise false.
   static bool TestMe(dbgLog& rlog);
   #endif // _UNITTEST
};

// INLINE METHODS
//

// EXTERNAL REFERENCES
//

#endif // INCLUDED_DOCPROXYAGENT_H_
