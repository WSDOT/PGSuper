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

#pragma once

/////////////////////////////////////////////////////////////////////////////////////////
// User interface extension callback interfaces are defined in this header file. 
//
// These are prototype interfaces. That means they are subject to change. It is unclear
// as to exactly which events extenions need to receive and how they are to be broadcast.
//
// Be warned that these interfaces may change in the future and your extension agents
// are likely to break.
/////////////////////////////////////////////////////////////////////////////////////////

class txnTransaction;

interface IEditBridgeCallback;
interface IEditSplicedGirderCallback;
interface IEditLoadRatingOptionsCallback;

class CPierData2;
class rptParagraph;

struct EditBridgeExtension
{
   IDType callbackID;
   IEditBridgeCallback* pCallback;
   CPropertyPage* pPage;

   bool operator<(const EditBridgeExtension& other) const { return callbackID < other.callbackID; }
   bool operator==(const EditBridgeExtension& other) const { return callbackID == other.callbackID; }
};

struct EditSplicedGirderExtension
{
   IDType callbackID;
   IEditSplicedGirderCallback* pCallback;
   CPropertyPage* pPage;

   bool operator<(const EditSplicedGirderExtension& other) const { return callbackID < other.callbackID; }
   bool operator==(const EditSplicedGirderExtension& other) const { return callbackID == other.callbackID; }
};

struct EditLoadRatingOptionsExtension
{
   IDType callbackID;
   IEditLoadRatingOptionsCallback* pCallback;
   CPropertyPage* pPage;
   bool operator<(const EditLoadRatingOptionsExtension& other) const { return callbackID < other.callbackID; }
};

// Extend the Edit Pier Dialog
interface IEditPierData
{
   virtual CPierData2* GetPierData() = 0;
   virtual PierIndexType GetPierCount() = 0;
   virtual PierIndexType GetPier() = 0;
   virtual pgsTypes::BoundaryConditionType GetConnectionType() = 0;
   virtual GirderIndexType GetGirderCount(pgsTypes::PierFaceType face) = 0;
};

interface IEditPierCallback
{
   // Called by the framework to create the property page for the Pier dialog is opened for stand alone editing
   virtual CPropertyPage* CreatePropertyPage(IEditPierData* pEditPierData) = 0;

   // Called by the framework when stand alone editing is complete. Return a transaction object if you
   // want the editing the occured on this extension page to be in the transaction queue for undo/redo,
   // otherwise return nullptr
   virtual txnTransaction* OnOK(CPropertyPage* pPropertyPage,IEditPierData* pEditPierData) = 0;

   // Return the ID of EditBridgeCallback or INVALID_ID if extensions to the Bridge dialog are not related to
   // the Pier dialog
   virtual IDType GetEditBridgeCallbackID() = 0;

   // Called by the framework to create a propery page for the Pier dialog when the Pier dialog is created from the Bridge dialog
   virtual CPropertyPage* CreatePropertyPage(IEditPierData* pEditPierData,CPropertyPage* pBridgePropertyPage) = 0;
};

// Callback interface for copying Pier properties. If you are extending the Pier dialog with
// additional Pier-based data you may want to also have that data copied when the Copy Pier Properties
// command is executed. Implement this interface, and register it with the IExtendUI interface to
// have your Pier properties listed in the Copy Pier Properties dialog and for your code
// to be notified when it is time to copy the data
interface ICopyPierPropertiesCallback
{
   // Text string to be displayed in the Copy Pier Properties dialog
   virtual LPCTSTR GetName() = 0;

   // Return TRUE if your Pier properties check box should be included in the copy properties list.
   // This method is called whenever the selection of the source or target Piers changes.
   virtual BOOL CanCopy(PierIndexType fromPierIdx,const std::vector<PierIndexType>& toPiers) = 0;

   // called by the framework when you need to create a transaction object that
   // will cause your Pier data to be copied. Allocate the transaction object
   // on the heap. The framework will delete it when it is no longer needed.
   virtual txnTransaction* CreateCopyTransaction(PierIndexType fromPierIdx,const std::vector<PierIndexType>& toPiers) = 0;

   // UI has an edit button for this callback. Returns the tab index that will be opened
   virtual UINT GetPierEditorTabIndex() = 0;

   // returns new paragraph to be inserted into Pier comparison report
   virtual rptParagraph* BuildComparisonReportParagraph(PierIndexType fromPierIdx,const std::vector<PierIndexType>& toPiers) = 0;
};

// Callback interface for copying TempSupport properties. If you are extending the TempSupport dialog with
// additional TempSupport-based data you may want to also have that data copied when the Copy TempSupport Properties
// command is executed. Implement this interface, and register it with the IExtendUI interface to
// have your TempSupport properties listed in the Copy TempSupport Properties dialog and for your code
// to be notified when it is time to copy the data
interface ICopyTemporarySupportPropertiesCallback
{
   // Text string to be displayed in the Copy TempSupport Properties dialog
   virtual LPCTSTR GetName() = 0;

   // Return TRUE if your TempSupport properties check box should be included in the copy properties list.
   // This method is called whenever the selection of the source or target TempSupports changes.
   virtual BOOL CanCopy(PierIndexType fromTempSupportIdx,const std::vector<PierIndexType>& toTempSupports) = 0;

   // called by the framework when you need to create a transaction object that
   // will cause your TempSupport data to be copied. Allocate the transaction object
   // on the heap. The framework will delete it when it is no longer needed.
   virtual txnTransaction* CreateCopyTransaction(PierIndexType fromTempSupportIdx,const std::vector<PierIndexType>& toTempSupports) = 0;

   // UI has an edit button for this callback. Returns the tab index that will be opened
   virtual UINT GetTempSupportEditorTabIndex() = 0;

   // returns new paragraph to be inserted into TempSupport comparison report
   virtual rptParagraph* BuildComparisonReportParagraph(PierIndexType fromTempSupportIdx,const std::vector<PierIndexType>& toTempSupports) = 0;
};


// Extend the Edit Temporary Support Dialog
interface IEditTemporarySupportData
{
   virtual SupportIndexType GetTemporarySupport() = 0;
};

interface IEditTemporarySupportCallback
{
   // Called by the framework to create the property page for the Pier dialog is opened for stand alone editing
   virtual CPropertyPage* CreatePropertyPage(IEditTemporarySupportData* pEditTemporarySupportData) = 0;

   // Called by the framework when stand alone editing is complete. Return a transaction object if you
   // want the editing the occured on this extension page to be in the transaction queue for undo/redo,
   // otherwise return nullptr
   virtual txnTransaction* OnOK(CPropertyPage* pPropertyPage,IEditTemporarySupportData* pEditTemporarySupportData) = 0;

   // Return the ID of EditBridgeCallback or INVALID_ID if extensions to the Bridge dialog are not related to
   // the Pier dialog
   virtual IDType GetEditBridgeCallbackID() = 0;

   // Called by the framework to create a propery page for the Pier dialog when the Pier dialog is created from the Bridge dialog
   virtual CPropertyPage* CreatePropertyPage(IEditTemporarySupportData* pEditTemporarySupportData,CPropertyPage* pBridgePropertyPage) = 0;
};

// Extend the Edit Span Dialog
interface IEditSpanData
{
   virtual SpanIndexType GetSpanCount() = 0;
   virtual SpanIndexType GetSpan() = 0;
   virtual pgsTypes::BoundaryConditionType GetConnectionType(pgsTypes::MemberEndType end) = 0;
   virtual GirderIndexType GetGirderCount() = 0;
};

interface IEditSpanCallback
{
   virtual CPropertyPage* CreatePropertyPage(IEditSpanData* pSpanData) = 0;
   virtual txnTransaction* OnOK(CPropertyPage* pPage,IEditSpanData* pSpanData) = 0;

   // Return the ID of EditBridgeCallback or INVALID_ID if extensions to the Bridge dialog are not related to
   // the Span dialog
   virtual IDType GetEditBridgeCallbackID() = 0;

   // Called by the framework to create a propery page for the Span dialog when the Pier dialog is created from the Bridge dialog
   virtual CPropertyPage* CreatePropertyPage(IEditSpanData* pEditSpanData,CPropertyPage* pBridgePropertyPage) = 0;
};

// Extend the Edit Segment Dialog
interface IEditSegmentData
{
   virtual const CSegmentKey& GetSegmentKey() = 0;
};

interface IEditSegmentCallback
{
   virtual CPropertyPage* CreatePropertyPage(IEditSegmentData* pSegmentData) = 0;
   virtual txnTransaction* OnOK(CPropertyPage* pPage,IEditSegmentData* pSegmentData) = 0;

   // Return the ID of EditSplicedGirderCallback or INVALID_ID if extensions to the Spliced Girder dialog are not related to
   // the Segment dialog
   virtual IDType GetEditSplicedGirderCallbackID() = 0;

   // Called by the framework to create a propery page for the Segment dialog when the Segment dialog is created from the Spliced Girder dialog
   virtual CPropertyPage* CreatePropertyPage(IEditSegmentData* pEditSegmentData,CPropertyPage* pSplicedGirderPropertyPage) = 0;
};

// Extend the Edit Closure Joint Dialog
interface IEditClosureJointData
{
   virtual const CClosureKey& GetClosureKey() = 0;
};

interface IEditClosureJointCallback
{
   virtual CPropertyPage* CreatePropertyPage(IEditClosureJointData* pClosureJointData) = 0;
   virtual txnTransaction* OnOK(CPropertyPage* pPage,IEditClosureJointData* pClosureJointData) = 0;

   // Return the ID of EditSplicedGirderCallback or INVALID_ID if extensions to the Spliced Girder dialog are not related to
   // the Segment dialog
   virtual IDType GetEditSplicedGirderCallbackID() = 0;

   // Called by the framework to create a propery page for the Closure Joint dialog when the Closure Joint dialog is created from the Spliced Girder dialog
   virtual CPropertyPage* CreatePropertyPage(IEditClosureJointData* pEditClosureJointData,CPropertyPage* pSplicedGirderPropertyPage) = 0;
};

// Extend the Edit Girder Dialog
interface IEditSplicedGirderData
{
   virtual const CGirderKey& GetGirderKey() = 0;
};

interface IEditSplicedGirderCallback
{
   virtual CPropertyPage* CreatePropertyPage(IEditSplicedGirderData* pGirderData) = 0;
   virtual txnTransaction* OnOK(CPropertyPage* pPage,IEditSplicedGirderData* pGirderData) = 0;

   // Called by the framework after editing segment data from the Spliced Girder general page completes successfully
   // so that data from the Segment and Spliced Girder editing dialogs can be made consistent with each other
   virtual void EditSegment_OnOK(CPropertyPage* pSplicedGirderPropertyPage,CPropertyPage* pSegmentPropertyPage) = 0;

   // Called by the framework after editing closure joint data from the Spliced Girder general page completes successfully
   // so that data from the Closure Joint and Spliced Girder editing dialogs can be made consistent with each other
   virtual void EditClosureJoint_OnOK(CPropertyPage* pSplicedGirderPropertyPage,CPropertyPage* pClosureJointPropertyPage) = 0;
};

// Extend the Edit Girder Dialog
interface IEditGirderData
{
   virtual const CSegmentKey& GetSegmentKey() = 0;
};

interface IEditGirderCallback
{
   virtual CPropertyPage* CreatePropertyPage(IEditGirderData* pGirderData) = 0;
   virtual txnTransaction* OnOK(CPropertyPage* pPage,IEditGirderData* pGirderData) = 0;
};

// Extend the Edit Bridge Dialog
interface IEditBridgeData
{
   virtual void EBDummy() = 0;
};

interface IEditBridgeCallback
{
   virtual CPropertyPage* CreatePropertyPage(IEditBridgeData* pBridgeData) = 0;
   virtual txnTransaction* OnOK(CPropertyPage* pPage,IEditBridgeData* pBridgeData) = 0;

   // Called by the framework after editing pier data from the Framing page completes successfully
   // so that data from the Pier and Bridge editing dialogs can be made consistent with each other
   virtual void EditPier_OnOK(CPropertyPage* pBridgePropertyPage,CPropertyPage* pPierPropertyPage) = 0;

   // Called by the framework after editing temporary support data from the Framing page completes successfully
   // so that data from the Temp Support and Bridge editing dialogs can be made consistent with each other
   virtual void EditTemporarySupport_OnOK(CPropertyPage* pBridgePropertyPage,CPropertyPage* pTempSupportPropertyPage) = 0;

   // Called by the framework after editing span data from the Framing page completes successfully
   // so that data from the Span and Bridge editing dialogs can be made consistent with each other
   virtual void EditSpan_OnOK(CPropertyPage* pBridgePropertyPage,CPropertyPage* pSpanPropertyPage) = 0;
};

// Callback interface for copying girder properties. If you are extending the girder dialog with
// additional girder-based data you may want to also have that data copied when the Copy Girder Properties
// command is executed. Implement this interface, and register it with the IExtendUI interface to
// have your girder properties listed in the Copy Girder Properties dialog and for your code
// to be notified when it is time to copy the data
interface ICopyGirderPropertiesCallback
{
   // Text string to be displayed in the Copy Girder Properties dialog
   virtual LPCTSTR GetName() = 0;

   // Return TRUE if your girder properties check box should be included in the copy properties list.
   // Example: Prestressing can only be copied  among like girders
   // This method is called whenever the selection of the source or target girders changes.
   virtual BOOL CanCopy(const CGirderKey& fromGirderKey,const std::vector<CGirderKey>& toGirderKeys) = 0;


   // called by the framework when you need to create a transaction object that
   // will cause your girder data to be copied. Allocate the transaction object
   // on the heap. The framework will delete it when it is no longer needed.
   virtual txnTransaction* CreateCopyTransaction(const CGirderKey& fromGirderKey,const std::vector<CGirderKey>& toGirderKeys) = 0;

   // UI has an edit button for this callback. Returns the tab index that will be opened
   virtual UINT GetGirderEditorTabIndex() = 0;

   // returns new paragraph to be inserted into girder comparison report
   virtual rptParagraph* BuildComparisonReportParagraph(const CGirderKey& fromGirderKey) = 0;
};

// Callback interface for editing load rating options
interface IEditLoadRatingOptions
{
   virtual void LRDummy() = 0;
};

interface IEditLoadRatingOptionsCallback
{
   virtual CPropertyPage* CreatePropertyPage(IEditLoadRatingOptions* pLoadRatingOptions) = 0;
   virtual txnTransaction* OnOK(CPropertyPage* pPage,IEditLoadRatingOptions* pLoadRatingOptions) = 0;
};

/////////////////////////////////////////////////////////
// IExtendUI
// Use this interface to register callbacks for user interface extensions

// {F477FBFC-2C57-42bf-8FB5-A32296087B64}
DEFINE_GUID(IID_IExtendUI, 
0xf477fbfc, 0x2c57, 0x42bf, 0x8f, 0xb5, 0xa3, 0x22, 0x96, 0x8, 0x7b, 0x64);
struct __declspec(uuid("{F477FBFC-2C57-42bf-8FB5-A32296087B64}")) IExtendUI;
interface IExtendUI : IUnknown
{
   virtual IDType RegisterEditPierCallback(IEditPierCallback* pCallback,ICopyPierPropertiesCallback* pCopyCallback) = 0;
   virtual IDType RegisterEditSpanCallback(IEditSpanCallback* pCallback) = 0;
   virtual IDType RegisterEditBridgeCallback(IEditBridgeCallback* pCallback) = 0;
   virtual IDType RegisterEditLoadRatingOptionsCallback(IEditLoadRatingOptionsCallback* pCallback) = 0;

   virtual bool UnregisterEditPierCallback(IDType ID) = 0;
   virtual bool UnregisterEditSpanCallback(IDType ID) = 0;
   virtual bool UnregisterEditBridgeCallback(IDType ID) = 0;
   virtual bool UnregisterEditLoadRatingOptionsCallback(IDType ID) = 0;
};

/////////////////////////////////////////////////////////
// IExtendPGSuperUI
// Use this interface to register callbacks for user interface extensions

// {D3CF52A4-A37E-4e9b-A71C-F9B37A045B8A}
DEFINE_GUID(IID_IExtendPGSuperUI, 
0xd3cf52a4, 0xa37e, 0x4e9b, 0xa7, 0x1c, 0xf9, 0xb3, 0x7a, 0x4, 0x5b, 0x8a);
struct __declspec(uuid("{D3CF52A4-A37E-4e9b-A71C-F9B37A045B8A}")) IExtendPGSuperUI;
interface IExtendPGSuperUI : IExtendUI
{
   virtual IDType RegisterEditGirderCallback(IEditGirderCallback* pCallback,ICopyGirderPropertiesCallback* pCopyCallback=nullptr) = 0;
   virtual bool UnregisterEditGirderCallback(IDType ID) = 0;
};

/////////////////////////////////////////////////////////
// IExtendPGSpliceUI
// Use this interface to register callbacks for user interface extensions

// {0303E609-6BBD-45b0-AFA2-E642CE7DA219}
DEFINE_GUID(IID_IExtendPGSpliceUI, 
0x303e609, 0x6bbd, 0x45b0, 0xaf, 0xa2, 0xe6, 0x42, 0xce, 0x7d, 0xa2, 0x19);
struct __declspec(uuid("{0303E609-6BBD-45b0-AFA2-E642CE7DA219}")) IExtendPGSpliceUI;
interface IExtendPGSpliceUI : IExtendUI
{
   virtual IDType RegisterEditTemporarySupportCallback(IEditTemporarySupportCallback* pCallback, ICopyTemporarySupportPropertiesCallback* pCopyCallBack) = 0;
   virtual IDType RegisterEditSplicedGirderCallback(IEditSplicedGirderCallback* pCallback,ICopyGirderPropertiesCallback* pCopyCallback=nullptr) = 0;
   virtual IDType RegisterEditSegmentCallback(IEditSegmentCallback* pCallback) = 0;
   virtual IDType RegisterEditClosureJointCallback(IEditClosureJointCallback* pCallback) = 0;

   virtual bool UnregisterEditTemporarySupportCallback(IDType ID) = 0;
   virtual bool UnregisterEditSplicedGirderCallback(IDType ID) = 0;
   virtual bool UnregisterEditSegmentCallback(IDType ID) = 0;
   virtual bool UnregisterEditClosureJointCallback(IDType ID) = 0;
};

// {7FB4E6EF-0639-47dc-AE76-0948F9184291}
DEFINE_GUID(IID_IExtendUIEventSink, 
0x7fb4e6ef, 0x639, 0x47dc, 0xae, 0x76, 0x9, 0x48, 0xf9, 0x18, 0x42, 0x91);
interface IExtendUIEventSink : IUnknown
{
   virtual HRESULT OnHintsReset() = 0;
};
