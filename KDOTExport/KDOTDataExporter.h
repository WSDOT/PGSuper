// PGSuperExporter.h : Declaration of the CPGSuperExporter

#pragma once

#include <Plugins/PGSuperIEPlugin.h>
#include <PsgLib\BridgeDescription2.h>
#include <IFace\AnalysisResults.h>
#include "KDOTExporter.hxx"
#include <EAF\ComponentObject.h>

class IShapes;
class LiveLoadLibraryEntry;

/////////////////////////////////////////////////////////////////////////////
// CKDOTDataExporter
class CKDOTDataExporter : public WBFL::EAF::ComponentObject,
   public PGS::IDataExporter,
   public PGS::IPluginDocumentation
{
public:
   CKDOTDataExporter();

   CBitmap m_bmpLogo;

// IDataExporter
public:
   HRESULT Init(UINT nCmdID) override;
   CString GetMenuText() const override;
   HBITMAP GetBitmapHandle() const override;
   CString GetCommandHintText() const override;
   HRESULT Export(std::shared_ptr<WBFL::EAF::Broker> pBroker) override;

// IPGSuperDocumentation
public:
   CString GetDocumentationSetName() const override;
   STDMETHOD(LoadDocumentationMap)() override;
   std::pair<WBFL::EAF::HelpResult,CString> GetDocumentLocation(UINT nHID) const override;

private:
   Float64 m_BearingHeight;
   HRESULT Export(std::shared_ptr<WBFL::EAF::Broker> pBroker,CString& strFileName, const std::vector<CGirderKey>& girderKeys);

   std::map<UINT,CString> m_HelpTopics;
   CString GetDocumentationURL() const;
};
