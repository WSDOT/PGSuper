///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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

#include <EAF\EAFViewController.h>

#include <Graphs\GirderPropertiesGraphBuilder.h>

// {2BE97C83-A5FA-475A-B493-CAA22D176338}
DEFINE_GUID(IID_IGirderPropertiesGraphViewController ,
   0x2be97c83, 0xa5fa, 0x475a, 0xb4, 0x93, 0xca, 0xa2, 0x2d, 0x17, 0x63, 0x38);
struct __declspec(uuid("{2BE97C83-A5FA-475A-B493-CAA22D176338}")) IGirderPropertiesGraphViewController;

interface IGirderPropertiesGraphViewController : IEAFViewController
{
   virtual bool SetPropertyType(CGirderPropertiesGraphBuilder::PropertyType propertyType) = 0;
   virtual CGirderPropertiesGraphBuilder::PropertyType GetPropertyType() const = 0;
   virtual bool IsInvariantProperty(CGirderPropertiesGraphBuilder::PropertyType propertyType) const = 0;

   virtual bool SetSectionPropertyType(pgsTypes::SectionPropertyType type) = 0;
   virtual pgsTypes::SectionPropertyType GetSectionPropertyType() const = 0;

   virtual void SelectGirder(const CGirderKey& girderKey) = 0;
   virtual const CGirderKey& GetGirder() const = 0;

   virtual void SetInterval(IntervalIndexType intervalIdx) = 0;
   virtual IntervalIndexType GetInterval() const = 0;
   virtual IntervalIndexType GetFirstInterval() const = 0;
   virtual IntervalIndexType GetLastInterval() const = 0;

   virtual void ShowGrid(bool bShow) = 0;
   virtual bool ShowGrid() const = 0;

   virtual void ShowGirder(bool bShow) = 0;
   virtual bool ShowGirder() const = 0;
};
