///////////////////////////////////////////////////////////////////////
// PGSuperIE - PGSuper Import/Export Plug-in
// Copyright © 1999-2026  Washington State Department of Transportation
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

/// @file PGSuperIEPlugin.h
/// @brief Interfaces implemented by PGSuper/PGSplice Project Importer, Data Importer, and Data
/// Exporter plug-ins.
///
/// These interfaces are deliberately licensed under the ARLOSL so third-party developers can build
/// proprietary plug-ins against them: the *interface* declarations in this file are covered by the
/// Alternate Route Library Open Source License, but a plug-in's *implementation* behind an
/// interface - the concrete class in a developer's own DLL - can be licensed however that developer
/// chooses. See \ref extensibility "Extensibility" for an overview of this and the other
/// PGSuper/PGSplice extension mechanisms, and \ref creating_a_project_importer "Creating a Project
/// Importer" / \ref creating_a_data_plugin "Creating a Data Plugin" for worked examples
/// (`IEPluginExample`).

#pragma once

#include <EAF\Broker.h>

/// @brief Component category for PGSuper Project Importer plug-ins.
// {289D1CFF-D1A4-4b65-B673-867D7F41C7DB}
DEFINE_GUID(CATID_PGSuperProjectImporter,
   0x289d1cff, 0xd1a4, 0x4b65, 0xb6, 0x73, 0x86, 0x7d, 0x7f, 0x41, 0xc7, 0xdb);
/// @brief Component category for PGSuper Data Importer plug-ins.
// {BD3B6F1E-7826-478b-99C0-A946C12C89CF}
DEFINE_GUID(CATID_PGSuperDataImporter,
   0xbd3b6f1e, 0x7826, 0x478b, 0x99, 0xc0, 0xa9, 0x46, 0xc1, 0x2c, 0x89, 0xcf);
/// @brief Component category for PGSuper Data Exporter plug-ins.
// {369A62A2-8995-4404-9C16-15AE5A0681E2}
DEFINE_GUID(CATID_PGSuperDataExporter,
   0x369a62a2, 0x8995, 0x4404, 0x9c, 0x16, 0x15, 0xae, 0x5a, 0x6, 0x81, 0xe2);
/// @brief Component category for PGSplice Project Importer plug-ins.
// {DB115813-3828-4564-A2FA-D8DDB368B1DB}
DEFINE_GUID(CATID_PGSpliceProjectImporter,
   0xdb115813, 0x3828, 0x4564, 0xa2, 0xfa, 0xd8, 0xdd, 0xb3, 0x68, 0xb1, 0xdb);
/// @brief Component category for PGSplice Data Importer plug-ins.
// {88E6E707-A7EA-431a-B787-41377D75E0F3}
DEFINE_GUID(CATID_PGSpliceDataImporter,
   0x88e6e707, 0xa7ea, 0x431a, 0xb7, 0x87, 0x41, 0x37, 0x7d, 0x75, 0xe0, 0xf3);
/// @brief Component category for PGSplice Data Exporter plug-ins.
// {D889AF1D-0CA1-4f01-AA2D-84F8F9F3A2DD}
DEFINE_GUID(CATID_PGSpliceDataExporter,
   0xd889af1d, 0xca1, 0x4f01, 0xaa, 0x2d, 0x84, 0xf8, 0xf9, 0xf3, 0xa2, 0xdd);

namespace PGS
{
   /// @brief Implemented by a Data Importer plug-in - merges data from an external source into the
   /// currently open PGSuper/PGSplice project. See \ref extensibility "Extensibility" for how this
   /// differs from a Project Importer or Data Exporter.
   class IDataImporter
   {
   public:
      /// @brief Called once when the plug-in is loaded, before any UI is built. nCmdID is the menu
      /// command ID the host has reserved for this plug-in's Import menu item.
      virtual HRESULT Init(UINT nCmdID) = 0;

      /// @brief Text displayed for this plug-in's item on the File > Import menu.
      virtual CString GetMenuText() const = 0;

      /// @brief Bitmap displayed next to this plug-in's menu item, if any.
      virtual HBITMAP GetBitmapHandle() const = 0;

      /// @brief Status bar / tooltip text shown when the user hovers over this plug-in's command.
      virtual CString GetCommandHintText() const = 0;

      /// @brief Imports data from an external source into the currently open project. pBroker
      /// gives access to the host application's agents (e.g. the interfaces in Include/IFace) so
      /// the imported data can be applied to the current model.
      virtual HRESULT Import(std::shared_ptr<WBFL::EAF::Broker> pBroker) = 0;
   };

   /// @brief Implemented by a Data Exporter plug-in - exports data from the currently open
   /// PGSuper/PGSplice project to an external destination.
   class IDataExporter
   {
   public:
      /// @brief Called once when the plug-in is loaded, before any UI is built. nCmdID is the menu
      /// command ID the host has reserved for this plug-in's Export menu item.
      virtual HRESULT Init(UINT nCmdID) = 0;

      /// @brief Text displayed for this plug-in's item on the File > Export menu.
      virtual CString GetMenuText() const = 0;

      /// @brief Bitmap displayed next to this plug-in's menu item, if any.
      virtual HBITMAP GetBitmapHandle() const = 0;

      /// @brief Status bar / tooltip text shown when the user hovers over this plug-in's command.
      virtual CString GetCommandHintText() const = 0;

      /// @brief Exports data from the currently open project to an external destination. pBroker
      /// gives access to the host application's agents so the current project's data can be read.
      virtual HRESULT Export(std::shared_ptr<WBFL::EAF::Broker> pBroker) = 0;
   };

   /// @brief Implemented by a Project Importer plug-in - creates a brand new PGSuper/PGSplice
   /// project by importing data from an external source. Unlike IDataImporter, this runs at
   /// "New Project" time and produces an entire project rather than modifying an already-open one.
   class IProjectImporter
   {
   public:
      /// @brief Text displayed for this importer's entry in the New Project template gallery.
      virtual CString GetItemText() const = 0;

      /// @brief Creates a new project by importing data from an external source. pBroker gives
      /// access to the newly created project's agents so the imported data can be applied.
      virtual HRESULT Import(std::shared_ptr<WBFL::EAF::Broker> pBroker) = 0;

      /// @brief Icon displayed for this importer's entry in the New Project template gallery.
      virtual HICON GetIcon() const = 0;

      /// @brief CLSID of this importer, used by the host to locate and instantiate it.
      virtual CLSID GetCLSID() const = 0;

      /// @brief Path to a template file used to pre-populate the project before Import() runs, or
      /// an empty string if this importer builds the entire project itself in Import().
      virtual CString GetTemplateFilePath() const = 0;
   };

   /// @brief Optional interface a Project Importer, Data Importer, or Data Exporter plug-in can
   /// implement to integrate its own help content with the host application's documentation system.
   class IPluginDocumentation
   {
   public:
      /// @brief Name of this plug-in's documentation set, as registered with the help system.
      virtual CString GetDocumentationSetName() const = 0;

      /// @brief Loads the map from context-sensitive-help IDs to document locations.
      virtual HRESULT LoadDocumentationMap() = 0;

      /// @brief Returns the location of the help document for the given context-sensitive-help ID.
      virtual std::pair<WBFL::EAF::HelpResult,CString> GetDocumentLocation(UINT nHID) const = 0;
   };
};

#include <EAF\EAFTemplateGroup.h>
/// @brief Adapts an IProjectImporter into the New Project template gallery's CEAFTemplateItem model.
class CPGSProjectImporterTemplateItem : public CEAFTemplateItem
{
public:
   CPGSProjectImporterTemplateItem(CEAFDocTemplate* pDocTemplate, LPCTSTR name, LPCTSTR path, HICON hIcon, std::shared_ptr<PGS::IProjectImporter> pImporter) :
      CEAFTemplateItem(pDocTemplate, name, path, hIcon)
   {
      m_Importer = pImporter;
   }

   virtual CEAFTemplateItem* Clone() const
   {
      CPGSProjectImporterTemplateItem* pClone = new CPGSProjectImporterTemplateItem(m_pDocTemplate, m_Name, m_Path, m_hIcon, m_Importer);
      return pClone;
   }

   std::shared_ptr<PGS::IProjectImporter> m_Importer;
   DECLARE_DYNAMIC(CPGSProjectImporterTemplateItem)
};
