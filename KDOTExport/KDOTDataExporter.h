// PGSuperExporter.h : Declaration of the CPGSuperExporter

#pragma once

#include <Plugins/PGSuperIEPlugin.h>
#include <PgsExt\BridgeDescription2.h>
#include <IFace\AnalysisResults.h>
#include "KDOTExporter.hxx"
#include <EAF\ComponentObject.h>

interface IShapes;
class LiveLoadLibraryEntry;

/////////////////////////////////////////////////////////////////////////////
// CKDOTDataExporter
class CKDOTDataExporter : public WBFL::EAF::ComponentObject,
   public PGSuper::IDataExporter,
   public PGSuper::IPluginDocumentation
{
public:
   CKDOTDataExporter();

   CBitmap m_bmpLogo;

// IDataExporter
public:
   STDMETHOD(Init)(UINT nCmdID) override;
   CString GetMenuText() const override;
   HBITMAP GetBitmapHandle() const override;
   CString GetCommandHintText() const override;
   STDMETHOD(Export)(/*[in]*/IBroker* pBroker) override;

// IPGSuperDocumentation
public:
   CString GetDocumentationSetName() const override;
   STDMETHOD(LoadDocumentationMap)() override;
   CString GetDocumentLocation(UINT nHID) const override;

private:
   Float64 m_BearingHeight;
   HRESULT Export(IBroker* pBroker,CString& strFileName, const std::vector<CGirderKey>& girderKeys);

   std::map<UINT,CString> m_HelpTopics;
   CString GetDocumentationURL() const;
};
