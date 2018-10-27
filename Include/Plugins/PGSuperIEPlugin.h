///////////////////////////////////////////////////////////////////////
// PGSuperIE - PGSuper Import/Export Plug-in
// Copyright © 1999-2018  Washington State Department of Transportation
//                        Bridge and Structures Office
//
// This library is a part of the Washington Bridge Foundation Libraries
// and was developed as part of the Alternate Route Project
//
// This library is free software; you can redistribute it and/or modify it under
// the terms of the Alternate Route Library Open Source License as published by 
// the Washington State Department of Transportation, Bridge and Structures Office.
//
// This program is distributed in the hope that it will be useful, but is distributed 
// AS IS, WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
// or FITNESS FOR A PARTICULAR PURPOSE. See the Alternate Route Library Open Source 
// License for more details.
//
// You should have received a copy of the Alternate Route Library Open Source License 
// along with this program; if not, write to the Washington State Department of 
// Transportation, Bridge and Structures Office, P.O. Box  47340, 
// Olympia, WA 98503, USA or e-mail Bridge_Support@wsdot.wa.gov
///////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
// The intent of licensing this interface with the ARLOSL is to provide
// third party developers a method of developing proprietary plug-ins
// to the software.
// 
// Any changes made to the interfaces defined in this file are
// are subject to the terms of the Alternate Route Library Open Source License.
//
// Components that implement the interfaces defined in this file are
// governed by the terms and conditions deemed appropriate by legal 
// copyright holder of said software.
///////////////////////////////////////////////////////////////////////

#pragma once

// {289D1CFF-D1A4-4b65-B673-867D7F41C7DB}
DEFINE_GUID(CATID_PGSuperProjectImporter,
   0x289d1cff, 0xd1a4, 0x4b65, 0xb6, 0x73, 0x86, 0x7d, 0x7f, 0x41, 0xc7, 0xdb);
// {BD3B6F1E-7826-478b-99C0-A946C12C89CF}
DEFINE_GUID(CATID_PGSuperDataImporter,
   0xbd3b6f1e, 0x7826, 0x478b, 0x99, 0xc0, 0xa9, 0x46, 0xc1, 0x2c, 0x89, 0xcf);
// {369A62A2-8995-4404-9C16-15AE5A0681E2}
DEFINE_GUID(CATID_PGSuperDataExporter,
   0x369a62a2, 0x8995, 0x4404, 0x9c, 0x16, 0x15, 0xae, 0x5a, 0x6, 0x81, 0xe2);
// {DB115813-3828-4564-A2FA-D8DDB368B1DB}
DEFINE_GUID(CATID_PGSpliceProjectImporter,
   0xdb115813, 0x3828, 0x4564, 0xa2, 0xfa, 0xd8, 0xdd, 0xb3, 0x68, 0xb1, 0xdb);
// {88E6E707-A7EA-431a-B787-41377D75E0F3}
DEFINE_GUID(CATID_PGSpliceDataImporter,
   0x88e6e707, 0xa7ea, 0x431a, 0xb7, 0x87, 0x41, 0x37, 0x7d, 0x75, 0xe0, 0xf3);
// {D889AF1D-0CA1-4f01-AA2D-84F8F9F3A2DD}
DEFINE_GUID(CATID_PGSpliceDataExporter,
   0xd889af1d, 0xca1, 0x4f01, 0xaa, 0x2d, 0x84, 0xf8, 0xf9, 0xf3, 0xa2, 0xdd);

// {98B3DF17-7E0E-4d4a-B8A2-5443914FC608}
DEFINE_GUID(IID_IPGSDataImporter, 
   0x98B3DF17, 0x7E0E, 0x4d4a, 0xB8, 0xA2, 0x54, 0x43, 0x91, 0x4F, 0xC6, 0x08);
struct __declspec(uuid("{98B3DF17-7E0E-4d4a-B8A2-5443914FC608}")) IPGSDataImporter;
interface IPGSDataImporter : IUnknown
{
   virtual HRESULT Init(UINT nCmdID) = 0;
   virtual HRESULT GetMenuText(BSTR*  bstrText) const = 0;
   virtual HRESULT GetBitmapHandle(HBITMAP* phBmp) const = 0;
   virtual HRESULT GetCommandHintText(BSTR*  bstrText) const = 0;
   virtual HRESULT Import(IBroker* pBroker) = 0;
};

// {5DB8B1D3-C91D-4e62-81E6-A7B64B0D38FD}
DEFINE_GUID(IID_IPGSProjectImporter, 
   0x5DB8B1D3, 0xC91D, 0x4e62, 0x81, 0xE6, 0xA7, 0xB6, 0x4B, 0x0D, 0x38, 0xFD);
struct __declspec(uuid("{5DB8B1D3-C91D-4e62-81E6-A7B64B0D38FD}")) IPGSProjectImporter;
interface IPGSProjectImporter : IUnknown
{
   virtual HRESULT GetItemText(BSTR* bstrText) const = 0;
   virtual HRESULT Import(IBroker* pBroker) = 0;
   virtual HRESULT GetIcon(HICON* phIcon) const = 0;
};

// {BF6EC18A-43D2-4ea1-BC7F-54365DD645DA}
DEFINE_GUID(IID_IPGSDataExporter, 
   0xBF6EC18A, 0x43D2, 0x4ea1, 0xBC, 0x7F, 0x54, 0x36, 0x5D, 0xD6, 0x45, 0xDA);
struct __declspec(uuid("{BF6EC18A-43D2-4ea1-BC7F-54365DD645DA}")) IPGSDataExporter;
interface IPGSDataExporter : IUnknown
{
   virtual HRESULT Init(UINT nCmdID) = 0;
   virtual HRESULT GetMenuText(BSTR*  bstrText) const = 0;
   virtual HRESULT GetBitmapHandle(HBITMAP* phBmp) const = 0;
   virtual HRESULT GetCommandHintText(BSTR*  bstrText) const = 0;
   virtual HRESULT Export(IBroker* pBroker) = 0;
};

// {45C667CB-67C4-4b2e-89CD-51D07D665507}
DEFINE_GUID(IID_IPGSDocumentation, 
   0x45C667CB, 0x67C4, 0x4b2e, 0x89, 0xCD, 0x51, 0xD0, 0x7D, 0x66, 0x55, 0x07);
struct __declspec(uuid("{45C667CB-67C4-4b2e-89CD-51D07D665507}")) IPGSDocumentation;
interface IPGSDocumentation : IUnknown
{
   virtual HRESULT GetDocumentationSetName(BSTR* pbstrName) const = 0;
   virtual HRESULT LoadDocumentationMap() = 0;
   virtual HRESULT GetDocumentLocation(UINT nHID,BSTR* pbstrURL) const = 0;
};
