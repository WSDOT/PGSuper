///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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

class txnTransaction;

interface IEditBridgeCallback;

struct EditBridgeExtension
{
   IDType callbackID;
   IEditBridgeCallback* pCallback;
   CPropertyPage* pPage;

   bool operator<(const EditBridgeExtension& other) const { return callbackID < other.callbackID; }
};

/////////////////////////////////////////////////////////////////////////////////////////
// User interface extension callback interfaces are defined in this header file. 
//
// These are prototype interfaces. That means they are subject to change. It is unclear
// as to exactly which events extenions need to receive and how they are to be broadcast.
//
// Be warned that these interfaces may change in the future and your extension agents
// are likely to break.
//

// Extend the Edit Pier Dialog
interface IEditPierData
{
   virtual PierIndexType GetPierCount() = 0;
   virtual PierIndexType GetPier() = 0;
   virtual pgsTypes::PierConnectionType GetConnectionType() = 0;
   virtual GirderIndexType GetGirderCount(pgsTypes::PierFaceType face) = 0;
};

interface IEditPierCallback
{
   // Called by the framework to create the property page for the Pier dialog is opened for stand alone editing
   virtual CPropertyPage* CreatePropertyPage(IEditPierData* pEditPierData) = 0;

   // Called by the framework when stand alone editing is complete. Return a transaction object if you
   // want the editing the occured on this extension page to be in the transaction queue for undo/redo,
   // otherwise return NULL
   virtual txnTransaction* OnOK(CPropertyPage* pPropertyPage,IEditPierData* pEditPierData) = 0;

   // Return the ID of EditBridgeCallback or INVALID_ID if extensions to the Bridge dialog are not related to
   // the Pier dialog
   virtual IDType GetEditBridgeCallbackID() = 0;

   // Called by the framework to create a propery page for the Pier dialog when the Pier dialog is created from the Bridge dialog
   virtual CPropertyPage* CreatePropertyPage(IEditPierData* pEditPierData,CPropertyPage* pBridgePropertyPage) = 0;
};

// Extend the Edit Span Dialog
interface IEditSpanData
{
   virtual SpanIndexType GetSpanCount() = 0;
   virtual SpanIndexType GetSpan() = 0;
   virtual pgsTypes::PierConnectionType GetConnectionType(pgsTypes::MemberEndType end) = 0;
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

// Extend the Edit Girder Dialog
interface IEditGirderData
{
   virtual SpanIndexType GetSpan() = 0;
   virtual GirderIndexType GetGirder() = 0;
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

   // Called by the framework after editing span data from the Framing page completes successfully
   // so that data from the Span and Bridge editing dialogs can be made consistent with each other
   virtual void EditSpan_OnOK(CPropertyPage* pBridgePropertyPage,CPropertyPage* pSpanPropertyPage) = 0;
};

// Callback interface for copying girder properties. If you are extending the girder dialog with
// additional girder-based data you may want to also have that data copied with the Copy Girder Properties
// command is executed. Implement this interface, and register it with the IExtendUI interface to
// have your girder properties listed in the Copy Girder Properties dialog and for your code
// to be notified when it is time to copy the data
interface ICopyGirderPropertiesCallback
{
   // Text string to be displayed in the Copy Girder Properties dialog
   virtual LPCTSTR GetName() = 0;

   // Return TRUE if your girder properties check box should be included in the copy properties list.
   // Example: The Slab Offset parameter should not be copied if it is defined as a single value
   // for the entire bridge. 
   // This method is called whenever the selection of the source or target girders changes.
   virtual BOOL CanCopy(SpanGirderHashType fromSpanGirderHashValue,const std::vector<SpanGirderHashType>& toSpanGirderHashValues) = 0;

   // called by the framework when you need to create a transaction object that
   // will cause your girder data to be copied. Allocate the transaction object
   // on the heap. The framework will delete it when it is no longer needed.
   virtual txnTransaction* CreateCopyTransaction(SpanGirderHashType fromSpanGirderHashValue,const std::vector<SpanGirderHashType>& toSpanGirderHashValues) = 0;
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
   virtual IDType RegisterEditPierCallback(IEditPierCallback* pCallback) = 0;
   virtual IDType RegisterEditSpanCallback(IEditSpanCallback* pCallback) = 0;
   virtual IDType RegisterEditGirderCallback(IEditGirderCallback* pCallback,ICopyGirderPropertiesCallback* pCopyCallback = NULL) = 0;
   virtual IDType RegisterEditBridgeCallback(IEditBridgeCallback* pCallback) = 0;

   virtual bool UnregisterEditPierCallback(IDType ID) = 0;
   virtual bool UnregisterEditSpanCallback(IDType ID) = 0;
   virtual bool UnregisterEditGirderCallback(IDType ID) = 0;
   virtual bool UnregisterEditBridgeCallback(IDType ID) = 0;
};

// {7FB4E6EF-0639-47dc-AE76-0948F9184291}
DEFINE_GUID(IID_IExtendUIEventSink, 
0x7fb4e6ef, 0x639, 0x47dc, 0xae, 0x76, 0x9, 0x48, 0xf9, 0x18, 0x42, 0x91);
interface IExtendUIEventSink : IUnknown
{
   virtual HRESULT OnHintsReset() = 0;
};
