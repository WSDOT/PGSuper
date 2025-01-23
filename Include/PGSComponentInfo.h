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

// {EEA46351-7884-47e2-ADD8-5FD35BF51ABE}
DEFINE_GUID(IID_IPGSuperComponentInfo, 
0xeea46351, 0x7884, 0x47e2, 0xad, 0xd8, 0x5f, 0xd3, 0x5b, 0xf5, 0x1a, 0xbe);
struct __declspec(uuid("{EEA46351-7884-47e2-ADD8-5FD35BF51ABE}")) IPGSuperComponentInfo;// for __uuidof

class CPGSuperDoc;

/// @brief Interface that returns information about PGSuper components
interface IPGSuperComponentInfo : IUnknown
{
   /// @brief Called by the application framework to initialize this object.
   /// This is a good place to validate license keys for 3rd party components
   /// @return Returns TRUE if the component is successfully initialized
   virtual BOOL Init(CPGSuperDoc* pDoc) = 0;

   /// @brief Called by the application framework to do any clean up while terminating
   virtual void Terminate() = 0;

   /// @brief Returns the name of your component
   virtual CString GetName() const = 0;

   /// @brief Returns a description of the component
   virtual CString GetDescription() const = 0;

   /// @brief The icon returned goes in the About dialog
   virtual HICON GetIcon() const = 0;

   /// @brief Return true if there is additional information to be displayed about the component
   virtual bool HasMoreInfo() const = 0;

   /// @brief When this function is called, display more detailed information about your component
   virtual void OnMoreInfo() const = 0;
};

// {4D566A4F-289C-4e73-B47C-6A4CC7905BA9}
DEFINE_GUID(IID_IPGSpliceComponentInfo, 
0x4d566a4f, 0x289c, 0x4e73, 0xb4, 0x7c, 0x6a, 0x4c, 0xc7, 0x90, 0x5b, 0xa9);
struct __declspec(uuid("{4D566A4F-289C-4e73-B47C-6A4CC7905BA9}")) IPGSpliceComponentInfo;// for __uuidof

class CPGSpliceDoc;

/// @brief Interface that returns information about PGSplice components
interface IPGSpliceComponentInfo : IUnknown
{
   /// @brief Called by the application framework to initialize this object.
   /// This is a good place to validate license keys for 3rd party components
   /// @return Returns TRUE if the component is successfully initialized
   virtual BOOL Init(CPGSpliceDoc* pDoc) = 0;

   /// @brief Called by the application framework to do any clean up while terminating
   virtual void Terminate() = 0;

   /// @brief Returns the name of your component
   virtual CString GetName() const = 0;

   /// @brief Returns a description of the component
   virtual CString GetDescription() const = 0;

   /// @brief The icon returned goes in the About dialog
   virtual HICON GetIcon() const = 0;

   /// @brief Return true if there is additional information to be displayed about the component
   virtual bool HasMoreInfo() const = 0;

   ///@ brief When this function is called, display more detailed information about your component
   virtual void OnMoreInfo() const = 0;
};
