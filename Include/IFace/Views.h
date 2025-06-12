///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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

namespace WBFL
{
   namespace EAF
   {
      class Menu;
   };
};

interface IEAFViewController;
interface IBridgeModelViewController;
interface IGirderModelViewController;
interface ILoadsViewController;


// {40E55658-360F-47d8-86DB-0AF21974BA30}
DEFINE_GUID(IID_IViews, 
0x40e55658, 0x360f, 0x47d8, 0x86, 0xdb, 0xa, 0xf2, 0x19, 0x74, 0xba, 0x30);
/// @brief Interface to programmatically create views
class __declspec(uuid("{40E55658-360F-47d8-86DB-0AF21974BA30}")) IViews
{
public:
   /// @brief Creates a Bridge Model View
   /// @param ppViewController If not nullptr, returns the BridgeModelViewController
   virtual void CreateBridgeModelView(IBridgeModelViewController** ppViewController=nullptr) = 0;

   /// @brief Creates the Girder Model View
   /// @param girderKey Girder to be displayed
   /// @param ppViewController If not nullptr, returns the GirderModelViewController
   virtual void CreateGirderView(const CGirderKey& girderKey, IGirderModelViewController** ppViewController = nullptr) = 0;

   /// @brief Creates the Loads View
   /// @param ppViewController If not nullptr, returns the LoadsViewController
   virtual void CreateLoadsView(ILoadsViewController** ppViewController=nullptr) = 0;

   /// @brief Creates the Library Editor View
   virtual void CreateLibraryEditorView() = 0;

   /// @brief Creates the Report View
   /// @param rptIdx Index of the report to create
   /// @param bPromptForSpec If TRUE, the user is prompted to customize the report
   virtual void CreateReportView(IndexType rptIdx,BOOL bPromptForSpec=TRUE) = 0;

   /// @brief Creates a Graph View
   /// @param graphIdx Index of the graph to create
   /// @param ppViewController If not nullptr, returns the EAFViewController.
   virtual void CreateGraphView(IndexType graphIdx, IEAFViewController** ppViewController = nullptr) = 0;

   /// @brief Creates a Graph View
   /// @param lpszGraph Name of the graph to create
   /// @param ppViewController If not nullptr, returns the EAFViewController.
   virtual void CreateGraphView(LPCTSTR lpszGraph, IEAFViewController** ppViewController = nullptr) = 0;

   /// @brief Fills pMenu with commands to create report views
   /// @param pMenu The menu to be filled
   /// @param bQuickReport If true, quick report (no prompt) commands are created otherwise commands that cause the report prompt to be displayed are created
   virtual void BuildReportMenu(std::shared_ptr<WBFL::EAF::Menu> menu, bool bQuickReport) = 0;

   /// @brief Fills pMenu with commands to create graph views
   /// @param pMenu The menu to be filled
   virtual void BuildGraphMenu(std::shared_ptr<WBFL::EAF::Menu> menu) = 0;

   /// @brief Returns the Bridge Model Editor View key
   virtual long GetBridgeModelEditorViewKey() = 0;

   /// @brief Returns the Girder Model Editor View key
   virtual long GetGirderModelEditorViewKey() = 0;

   /// @brief Returns the Library Editor View key
   virtual long GetLibraryEditorViewKey() = 0;

   /// @brief Returns the Report View key
   virtual long GetReportViewKey() = 0;

   /// @brief Returns the Graphing View key
   virtual long GetGraphingViewKey() = 0;

   /// @brief Returns the Loads View key
   virtual long GetLoadsViewKey() = 0;
};
