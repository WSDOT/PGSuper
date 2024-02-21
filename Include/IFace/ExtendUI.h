///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2024  Washington State Department of Transportation
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
// as to exactly which events extensions need to receive and how they are to be broadcast.
//
// Be warned that these interfaces may change in the future and your extension agents
// are likely to break.
/////////////////////////////////////////////////////////////////////////////////////////

class CEAFTransaction;

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

/// @brief Interface that provides pier data to the IEditPierCallback::CreatePropertyPage method
interface IEditPierData
{
   /// @brief Returns the pier data being edited
   virtual CPierData2* GetPierData() = 0;
   
   /// @brief Returns the total number of piers in the bridge
   virtual PierIndexType GetPierCount() = 0;

   /// @brief Returns the index of the pier being edited
   virtual PierIndexType GetPier() = 0;

   /// @brief Returns the connection information for the pier being edited
   virtual pgsTypes::BoundaryConditionType GetConnectionType() = 0;

   /// @brief Returns the number of girders framing into the pier on the specified face
   virtual GirderIndexType GetGirderCount(pgsTypes::PierFaceType face) = 0;
};

/// @brief Callback interface for objects extending the Edit Pier dialog
interface IEditPierCallback
{
   /// @brief Called by the framework to create the property page when the Pier dialog is opened for stand alone editing
   virtual CPropertyPage* CreatePropertyPage(IEditPierData* pEditPierData) = 0;

   /// @brief Called by the framework to create a property page for the Pier dialog when the Pier dialog is created from the Bridge dialog
   virtual CPropertyPage* CreatePropertyPage(IEditPierData* pEditPierData, CPropertyPage* pBridgePropertyPage) = 0;

   /// @brief Called by the framework when stand alone editing is complete. 
   /// @return Return a transaction object if you
   /// want the editing the occurred on this extension page to be in the transaction queue for undo/redo,
   /// otherwise return nullptr
   virtual std::unique_ptr<CEAFTransaction> OnOK(CPropertyPage* pPropertyPage,IEditPierData* pEditPierData) = 0;

   /// @brief Return the ID of EditBridgeCallback or INVALID_ID if extensions to the Bridge dialog are not related to the Pier dialog
   virtual IDType GetEditBridgeCallbackID() = 0;
};

/// @brief Callback interface for copying Pier properties. If you are extending the Pier dialog with
/// additional Pier-based data you may want to also have that data copied when the Copy Pier Properties
/// command is executed. Implement this interface, and register it with the IExtendUI interface to
/// have your Pier properties listed in the Copy Pier Properties dialog and for your code
/// to be notified when it is time to copy the data
interface ICopyPierPropertiesCallback
{
   /// @brief Returns a text string to be displayed in the Copy Pier Properties dialog
   virtual LPCTSTR GetName() = 0;

   /// @brief Returns TRUE if your Pier properties check box should be included in the copy properties list.
   /// This method is called whenever the selection of the source or target Piers changes.
   virtual BOOL CanCopy(PierIndexType fromPierIdx,const std::vector<PierIndexType>& toPiers) = 0;

   /// @brief Called by the framework when you need to create a transaction object that will cause your Pier data to be copied.
   virtual std::unique_ptr<CEAFTransaction> CreateCopyTransaction(PierIndexType fromPierIdx,const std::vector<PierIndexType>& toPiers) = 0;

   /// @brief UI has an edit button for this callback. 
   /// @return Returns the tab index that will be opened
   virtual UINT GetPierEditorTabIndex() = 0;

   /// @brief Returns new paragraph to be inserted into Pier comparison report
   virtual rptParagraph* BuildComparisonReportParagraph(PierIndexType fromPierIdx,const std::vector<PierIndexType>& toPiers) = 0;
};

/// @brief Callback interface for copying TempSupport properties. If you are extending the TempSupport dialog with
/// additional TempSupport-based data you may want to also have that data copied when the Copy TempSupport Properties
/// command is executed. Implement this interface, and register it with the IExtendUI interface to
/// have your TempSupport properties listed in the Copy TempSupport Properties dialog and for your code
/// to be notified when it is time to copy the data
interface ICopyTemporarySupportPropertiesCallback
{
   /// @brief Returns a text string to be displayed in the Copy TempSupport Properties dialog
   virtual LPCTSTR GetName() = 0;

   /// @brief Returns TRUE if your TempSupport properties check box should be included in the copy properties list.
   /// This method is called whenever the selection of the source or target TempSupports changes.
   virtual BOOL CanCopy(PierIndexType fromTempSupportIdx,const std::vector<PierIndexType>& toTempSupports) = 0;

   /// @brief Called by the framework when you need to create a transaction object that will cause your TempSupport data to be copied.
   virtual std::unique_ptr<CEAFTransaction> CreateCopyTransaction(PierIndexType fromTempSupportIdx,const std::vector<PierIndexType>& toTempSupports) = 0;

   /// @brief UI has an edit button for this callback. 
   /// @return Returns the tab index that will be opened
   virtual UINT GetTempSupportEditorTabIndex() = 0;

   /// @brief Returns new paragraph to be inserted into TempSupport comparison report
   virtual rptParagraph* BuildComparisonReportParagraph(PierIndexType fromTempSupportIdx,const std::vector<PierIndexType>& toTempSupports) = 0;
};


/// @brief Interface that provides information to an IEditTemporarySupportCallback::CreatePropertyPage method
interface IEditTemporarySupportData
{
   /// @brief Returns the index of the temporary support being edited
   virtual SupportIndexType GetTemporarySupport() = 0;
};

/// @brief Callback interface for objects extending the Edit Temporary Support dialog
interface IEditTemporarySupportCallback
{
   /// @brief Called by the framework to create the property page for the Pier dialog is opened for stand alone editing
   virtual CPropertyPage* CreatePropertyPage(IEditTemporarySupportData* pEditTemporarySupportData) = 0;

   /// @brief Called by the framework to create a property page for the Pier dialog when the Pier dialog is created from the Bridge dialog
   virtual CPropertyPage* CreatePropertyPage(IEditTemporarySupportData* pEditTemporarySupportData, CPropertyPage* pBridgePropertyPage) = 0;

   /// @brief Called by the framework when stand alone editing is complete. 
   /// @return Return a transaction object if you want the editing the occurred on this extension page to be in the transaction queue for undo/redo,
   /// otherwise return nullptr
   virtual std::unique_ptr<CEAFTransaction> OnOK(CPropertyPage* pPropertyPage,IEditTemporarySupportData* pEditTemporarySupportData) = 0;

   //@ brief Returns the ID of EditBridgeCallback or INVALID_ID if extensions to the Bridge dialog are not related to the Temporary Support dialog
   virtual IDType GetEditBridgeCallbackID() = 0;
};

/// @brief Interface that provides information to the IEditSpanCallback::CreatePropertyPage method
interface IEditSpanData
{
   /// @brief Returns the number of spans in the bridge
   virtual SpanIndexType GetSpanCount() = 0;

   /// @brief Returns the index of the span being edited
   virtual SpanIndexType GetSpan() = 0;

   /// @brief Returns the connection information at the specified end of span
   virtual pgsTypes::BoundaryConditionType GetConnectionType(pgsTypes::MemberEndType end) = 0;

   /// @brief Returns the number of girders in the span
   virtual GirderIndexType GetGirderCount() = 0;
};

/// @brief Callback interface for objects extending the Edit Span dialog
interface IEditSpanCallback
{
   /// @brief Called by the framework to create the property page for the Span dialog is opened for stand alone editing
   virtual CPropertyPage* CreatePropertyPage(IEditSpanData* pSpanData) = 0;

   /// @brief Called by the framework to create a property page for the Span dialog when the Pier dialog is created from the Bridge dialog
   virtual CPropertyPage* CreatePropertyPage(IEditSpanData* pEditSpanData, CPropertyPage* pBridgePropertyPage) = 0;

   /// @brief Called by the framework when stand alone editing is complete. 
   /// @return Return a transaction object if you want the editing the occurred on this extension page to be in the transaction queue for undo/redo,
   /// otherwise return nullptr
   virtual std::unique_ptr<CEAFTransaction> OnOK(CPropertyPage* pPage,IEditSpanData* pSpanData) = 0;

   /// @brief Returns the ID of EditBridgeCallback or INVALID_ID if extensions to the Bridge dialog are not related to the Span dialog
   virtual IDType GetEditBridgeCallbackID() = 0;
};

/// @brief Interface that provides information to the IEditSegmentCallback::CreatePropertyPage method
interface IEditSegmentData
{
   /// @brief Returns the key of the segment being edited
   virtual const CSegmentKey& GetSegmentKey() = 0;
};

/// @brief Callback interface for objects extending the Edit Segment dialog
interface IEditSegmentCallback
{
   /// @brief Called by the framework to create the property page for the Segment dialog is opened for stand alone editing
   virtual CPropertyPage* CreatePropertyPage(IEditSegmentData* pSegmentData) = 0;

   /// @brief Called by the framework to create a property page for the Segment dialog when the Segment dialog is created from the Spliced Girder dialog
   virtual CPropertyPage* CreatePropertyPage(IEditSegmentData* pEditSegmentData, CPropertyPage* pSplicedGirderPropertyPage) = 0;

   /// @brief Called by the framework when stand alone editing is complete. 
   /// @return Return a transaction object if you want the editing the occurred on this extension page to be in the transaction queue for undo/redo,
   /// otherwise return nullptr
   virtual std::unique_ptr<CEAFTransaction> OnOK(CPropertyPage* pPage,IEditSegmentData* pSegmentData) = 0;

   /// @brief Returns the ID of EditSplicedGirderCallback or INVALID_ID if extensions to the Spliced Girder dialog are not related to the Segment dialog
   virtual IDType GetEditSplicedGirderCallbackID() = 0;
};

/// @brief Interface that provides information to the IEditClosureJointCallback::CreatePropertyPage method
interface IEditClosureJointData
{
   /// @brief Key for the closure joint being edited
   virtual const CClosureKey& GetClosureKey() = 0;
};

/// @brief Callback interface for objects extending the Edit Closure Joint dialog
interface IEditClosureJointCallback
{
   /// @brief Called by the framework to create the property page for the Closure Joint dialog is opened for stand alone editing
   virtual CPropertyPage* CreatePropertyPage(IEditClosureJointData* pClosureJointData) = 0;

   /// @brief Called by the framework to create a property page for the Closure Joint dialog when the Closure Joint dialog is created from the Spliced Girder dialog
   virtual CPropertyPage* CreatePropertyPage(IEditClosureJointData* pEditClosureJointData, CPropertyPage* pSplicedGirderPropertyPage) = 0;

   /// @brief Called by the framework when stand alone editing is complete. 
   /// @return Return a transaction object if you want the editing the occurred on this extension page to be in the transaction queue for undo/redo,
   /// otherwise return nullptr
   virtual std::unique_ptr<CEAFTransaction> OnOK(CPropertyPage* pPage,IEditClosureJointData* pClosureJointData) = 0;

   /// @brief Returns the ID of EditSplicedGirderCallback or INVALID_ID if extensions to the Spliced Girder dialog are not related to the Closure Joint dialog
   virtual IDType GetEditSplicedGirderCallbackID() = 0;
};

/// @brief Interface that provides information to the IEditSplicedGirderCallback::CreatePropertyPage method
interface IEditSplicedGirderData
{
   /// @brief Returns the key of the girder being edited
   virtual const CGirderKey& GetGirderKey() = 0;
};

/// @brief Callback interface for objects extending the Edit Spliced Girder dialog
interface IEditSplicedGirderCallback
{
   /// @brief Called by the framework to create the property page for the Girder dialog
   virtual CPropertyPage* CreatePropertyPage(IEditSplicedGirderData* pGirderData) = 0;

   /// @brief Called by the framework when stand alone editing is complete. 
   /// @return Return a transaction object if you want the editing the occurred on this extension page to be in the transaction queue for undo/redo,
   /// otherwise return nullptr
   virtual std::unique_ptr<CEAFTransaction> OnOK(CPropertyPage* pPage,IEditSplicedGirderData* pGirderData) = 0;

   /// @brief Called by the framework after editing segment data from the Spliced Girder general page completes successfully
   /// so that data from the Segment and Spliced Girder editing dialogs can be made consistent with each other
   virtual void EditSegment_OnOK(CPropertyPage* pSplicedGirderPropertyPage,CPropertyPage* pSegmentPropertyPage) = 0;

   /// @brief Called by the framework after editing closure joint data from the Spliced Girder general page completes successfully
   /// so that data from the Closure Joint and Spliced Girder editing dialogs can be made consistent with each other
   virtual void EditClosureJoint_OnOK(CPropertyPage* pSplicedGirderPropertyPage,CPropertyPage* pClosureJointPropertyPage) = 0;
};

/// @brief Interface that provides information to the IEditGirderCallback::CreatePropertyPage method
interface IEditGirderData
{
   /// @brief Returns the key of the segment being edited. In this case, the Girder dialog is for PGSuper
   /// so Girder and Segment are the same
   virtual const CSegmentKey& GetSegmentKey() = 0;
};

/// @brief Callback interface for objects extending the Edit Girder dialog
interface IEditGirderCallback
{
   /// @brief Called by the framework to create the property page for the Girder dialog
   virtual CPropertyPage* CreatePropertyPage(IEditGirderData* pGirderData) = 0;

   /// @brief Called by the framework when stand alone editing is complete. 
   /// @return Return a transaction object if you want the editing the occurred on this extension page to be in the transaction queue for undo/redo,
   /// otherwise return nullptr
   virtual std::unique_ptr<CEAFTransaction> OnOK(CPropertyPage* pPage,IEditGirderData* pGirderData) = 0;
};

/// @brief Interface that provides information to the IEditBridgeCallback::CreatePropertyPage method
interface IEditBridgeData
{
   /// @brief This is a dummy method - it doesn't do anything or provide any information.
   /// This interface and the methods may change in the future
   virtual void EBDummy() = 0;
};

/// @brief Callback interface for objects extending the Edit Bridge dialog
interface IEditBridgeCallback
{
   /// @brief Called by the framework to create the property page for the Edit Bridge dialog
   virtual CPropertyPage* CreatePropertyPage(IEditBridgeData* pBridgeData) = 0;

   /// @brief Called by the framework when stand alone editing is complete. 
   /// @return Return a transaction object if you want the editing the occurred on this extension page to be in the transaction queue for undo/redo,
   /// otherwise return nullptr
   virtual std::unique_ptr<CEAFTransaction> OnOK(CPropertyPage* pPage,IEditBridgeData* pBridgeData) = 0;

   /// @brief Called by the framework after editing pier data from the Framing page completes successfully
   /// so that data from the Pier and Bridge editing dialogs can be made consistent with each other
   virtual void EditPier_OnOK(CPropertyPage* pBridgePropertyPage,CPropertyPage* pPierPropertyPage) = 0;

   /// @brief Called by the framework after editing temporary support data from the Framing page completes successfully
   /// so that data from the Temp Support and Bridge editing dialogs can be made consistent with each other
   virtual void EditTemporarySupport_OnOK(CPropertyPage* pBridgePropertyPage,CPropertyPage* pTempSupportPropertyPage) = 0;

   /// @brief Called by the framework after editing span data from the Framing page completes successfully
   /// so that data from the Span and Bridge editing dialogs can be made consistent with each other
   virtual void EditSpan_OnOK(CPropertyPage* pBridgePropertyPage,CPropertyPage* pSpanPropertyPage) = 0;
};

/// @brief Callback interface for copying girder properties. If you are extending the girder dialog with
/// additional girder-based data you may want to also have that data copied when the Copy Girder Properties
/// command is executed. Implement this interface, and register it with the IExtendUI interface to
/// have your girder properties listed in the Copy Girder Properties dialog and for your code
/// to be notified when it is time to copy the data
interface ICopyGirderPropertiesCallback
{
   /// @brief Returns a text string to be displayed in the Copy Girder Properties dialog
   virtual LPCTSTR GetName() = 0;

   /// @brief Returns TRUE if your girder properties check box should be included in the copy properties list.
   /// Example: Prestressing can only be copied among like girders.
   /// This method is called whenever the selection of the source or target girders changes.
   virtual BOOL CanCopy(const CGirderKey& fromGirderKey,const std::vector<CGirderKey>& toGirderKeys) = 0;


   /// @brief Called by the framework when you need to create a transaction object that
   /// will cause your girder data to be copied.
   virtual std::unique_ptr<CEAFTransaction> CreateCopyTransaction(const CGirderKey& fromGirderKey,const std::vector<CGirderKey>& toGirderKeys) = 0;

   /// @brief UI has an edit button for this callback. 
   /// @return Returns the tab index that will be opened
   virtual UINT GetGirderEditorTabIndex() = 0;

   /// @brief Returns new paragraph to be inserted into girder comparison report
   virtual rptParagraph* BuildComparisonReportParagraph(const CGirderKey& fromGirderKey) = 0;
};

/// @brief Interface the provides information to the IEditLoadRatingOptionsCallback::CreatePropertyPage method
interface IEditLoadRatingOptions
{
   /// @brief This is a dummy method - it doesn't do anything or provide any information.
   /// This interface and the methods may change in the future
   virtual void LRDummy() = 0;
};

/// @brief Callback interface for objects extending the Edit Load Rating Ptions dialog
interface IEditLoadRatingOptionsCallback
{
   /// @brief Called by the framework to create the property page for the Edit Bridge dialog
   virtual CPropertyPage* CreatePropertyPage(IEditLoadRatingOptions* pLoadRatingOptions) = 0;

   /// @brief Called by the framework when stand alone editing is complete. 
   /// @return Return a transaction object if you want the editing the occurred on this extension page to be in the transaction queue for undo/redo,
   /// otherwise return nullptr
   virtual std::unique_ptr<CEAFTransaction> OnOK(CPropertyPage* pPage,IEditLoadRatingOptions* pLoadRatingOptions) = 0;
};

// {F477FBFC-2C57-42bf-8FB5-A32296087B64}
DEFINE_GUID(IID_IExtendUI, 
0xf477fbfc, 0x2c57, 0x42bf, 0x8f, 0xb5, 0xa3, 0x22, 0x96, 0x8, 0x7b, 0x64);
struct __declspec(uuid("{F477FBFC-2C57-42bf-8FB5-A32296087B64}")) IExtendUI;
/// @brief Interface used to manage the registration of callback objects that extend elements of the user interface common to both PGSuper and PGSplice.
interface IExtendUI : IUnknown
{
   /// @brief Registers a callback to extend the Edit Pier dialog
   /// @param pCallback Callback for extending the dialog
   /// @param pCopyCallback Callback for extending the pier data copying dialog
   /// @return ID of the callback
   virtual IDType RegisterEditPierCallback(IEditPierCallback* pCallback,ICopyPierPropertiesCallback* pCopyCallback) = 0;

   /// @brief Registers a callback to extend the Edit Span dialog
   /// @param pCallback Callback for extending the dialog
   /// @return ID of the callback
   virtual IDType RegisterEditSpanCallback(IEditSpanCallback* pCallback) = 0;

   /// @brief Registers a callback to extend the Edit Bridge dialog
   /// @param pCallback Callback for extending the dialog
   /// @return ID of the callback
   virtual IDType RegisterEditBridgeCallback(IEditBridgeCallback* pCallback) = 0;

   /// @brief Regesters a callback to extend the Edit Load Rating Options dialog
   /// @param pCallback Callback for extending the dialog
   /// @return ID of the callback
   virtual IDType RegisterEditLoadRatingOptionsCallback(IEditLoadRatingOptionsCallback* pCallback) = 0;

   /// @brief Unregistered a callback that extends the Edit Pier dialog
   /// @param ID ID of the callback
   /// @return true if successful
   virtual bool UnregisterEditPierCallback(IDType ID) = 0;

   /// @brief Unregistered a callback that extends the Edit Span dialog
   /// @param ID ID of the callback
   /// @return true if successful
   virtual bool UnregisterEditSpanCallback(IDType ID) = 0;

   /// @brief Unregistered a callback that extends the Edit Bridge dialog
   /// @param ID ID of the callback
   /// @return true if successful
   virtual bool UnregisterEditBridgeCallback(IDType ID) = 0;


   /// @brief Unregistered a callback that extends the Edit Load Rating Options dialog
   /// @param ID ID of the callback
   /// @return true if successful
   virtual bool UnregisterEditLoadRatingOptionsCallback(IDType ID) = 0;
};

// {D3CF52A4-A37E-4e9b-A71C-F9B37A045B8A}
DEFINE_GUID(IID_IExtendPGSuperUI, 
0xd3cf52a4, 0xa37e, 0x4e9b, 0xa7, 0x1c, 0xf9, 0xb3, 0x7a, 0x4, 0x5b, 0x8a);
struct __declspec(uuid("{D3CF52A4-A37E-4e9b-A71C-F9B37A045B8A}")) IExtendPGSuperUI;
/// @brief Interface used to manage the registration of callback objects that extend elements of the user interface of PGSuper only
interface IExtendPGSuperUI : IExtendUI
{
   /// @brief Registers a callback that extends the Edit Girder dialog
   /// @param pCallback Callback that extends the dialog
   /// @param pCopyCallback Callback that extends the Copy Girder Properties dialog
   /// @return ID of the callback
   virtual IDType RegisterEditGirderCallback(IEditGirderCallback* pCallback,ICopyGirderPropertiesCallback* pCopyCallback=nullptr) = 0;

   /// @brief Unregisters a callback that extends the Edit Girder dialog
   /// @param ID ID of the callback
   /// @return true if successful
   virtual bool UnregisterEditGirderCallback(IDType ID) = 0;
};


// {0303E609-6BBD-45b0-AFA2-E642CE7DA219}
DEFINE_GUID(IID_IExtendPGSpliceUI, 
0x303e609, 0x6bbd, 0x45b0, 0xaf, 0xa2, 0xe6, 0x42, 0xce, 0x7d, 0xa2, 0x19);
struct __declspec(uuid("{0303E609-6BBD-45b0-AFA2-E642CE7DA219}")) IExtendPGSpliceUI;
/// @brief Interface used to manage the registration of callback objects that extend elements of the user interface of PGSplice only
interface IExtendPGSpliceUI : IExtendUI
{
   /// @brief Registers a callback that extends the Edit Temporary Support dialog
   /// @param pCallback Callback that extends the dialog
   /// @param pCopyCallBack Callback that extends the Copy Temporary Support Properties dialog
   /// @return ID of the callback
   virtual IDType RegisterEditTemporarySupportCallback(IEditTemporarySupportCallback* pCallback, ICopyTemporarySupportPropertiesCallback* pCopyCallBack) = 0;

   /// @brief Registers a callback that extends the Edit Spliced Girder dialog
   /// @param pCallback Callback that extends the dialog
   /// @param pCopyCallBack Callback that extends the Copy Spliced GirderProperties dialog
   /// @return ID of the callback
   virtual IDType RegisterEditSplicedGirderCallback(IEditSplicedGirderCallback* pCallback,ICopyGirderPropertiesCallback* pCopyCallback=nullptr) = 0;

   /// @brief Registers a callback that extends the Edit Segment dialog
   /// @param pCallback Callback that extends the dialog
   /// @return ID of the callback
   virtual IDType RegisterEditSegmentCallback(IEditSegmentCallback* pCallback) = 0;

   /// @brief Registers a callback that extends the Edit Closure Joint dialog
   /// @param pCallback Callback that extends the dialog
   /// @return ID of the callback
   virtual IDType RegisterEditClosureJointCallback(IEditClosureJointCallback* pCallback) = 0;

   /// @brief Unregisters callback extending the Edit Temporary Support Dialog
   /// @param ID ID of the callback
   /// @return true if successful
   virtual bool UnregisterEditTemporarySupportCallback(IDType ID) = 0;

   /// @brief Unregisters callback extending the Edit Spliced Girder Dialog
   /// @param ID ID of the callback
   /// @return true if successful
   virtual bool UnregisterEditSplicedGirderCallback(IDType ID) = 0;

   /// @brief Unregisters callback extending the Edit Segment Dialog
   /// @param ID ID of the callback
   /// @return true if successful
   virtual bool UnregisterEditSegmentCallback(IDType ID) = 0;

   /// @brief Unregisters callback extending the Edit Closure Joint Dialog
   /// @param ID ID of the callback
   /// @return true if successful
   virtual bool UnregisterEditClosureJointCallback(IDType ID) = 0;
};

// {7FB4E6EF-0639-47dc-AE76-0948F9184291}
DEFINE_GUID(IID_IExtendUIEventSink, 
0x7fb4e6ef, 0x639, 0x47dc, 0xae, 0x76, 0x9, 0x48, 0xf9, 0x18, 0x42, 0x91);
/// @brief Callback interface objects extending the UI Event Sink.
/// Implement this interfaced to be notified when user interface hints are reset.
interface IExtendUIEventSink : IUnknown
{
   /// @brief Called by the framework when user interface hints are reset
   /// @return Returns S_OK if successful
   virtual HRESULT OnHintsReset() = 0;
};
