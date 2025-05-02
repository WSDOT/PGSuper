///////////////////////////////////////////////////////////////////////
// PGSuperIE - PGSuper Import/Export Plug-in
// Copyright © 1999-2022  Washington State Department of Transportation
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

#include <EAF\Broker.h>

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

namespace PGS
{
   class IDataImporter
   {
   public:
      virtual HRESULT Init(UINT nCmdID) = 0;
      virtual CString GetMenuText() const = 0;
      virtual HBITMAP GetBitmapHandle() const = 0;
      virtual CString GetCommandHintText() const = 0;
      virtual HRESULT Import(std::shared_ptr<WBFL::EAF::Broker> pBroker) = 0;
   };

   class IDataExporter
   {
   public:
      virtual HRESULT Init(UINT nCmdID) = 0;
      virtual CString GetMenuText() const = 0;
      virtual HBITMAP GetBitmapHandle() const = 0;
      virtual CString GetCommandHintText() const = 0;
      virtual HRESULT Export(std::shared_ptr<WBFL::EAF::Broker> pBroker) = 0;
   };

   class IProjectImporter
   {
   public:
      virtual CString GetItemText() const = 0;
      virtual HRESULT Import(std::shared_ptr<WBFL::EAF::Broker> pBroker) = 0;
      virtual HICON GetIcon() const = 0;
      virtual CLSID GetCLSID() const = 0;
   };

   class IPluginDocumentation
   {
   public:
      virtual CString GetDocumentationSetName() const = 0;
      virtual HRESULT LoadDocumentationMap() = 0;
      virtual std::pair<WBFL::EAF::HelpResult,CString> GetDocumentLocation(UINT nHID) const = 0;
   };
};