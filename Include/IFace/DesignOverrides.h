///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2018  Washington State Department of Transportation
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

// {35EEC676-EA6C-4AE1-9751-707F6660D49B}
DEFINE_GUID(IID_IDesignOverrides, 
0x35eec676, 0xea6c, 0x4ae1, 0x97, 0x51, 0x70, 0x7f, 0x66, 0x60, 0xd4, 0x9b);
interface IDesignOverrides : IUnknown
{
   // This interface is to be used during design time to override bridge and 
   // project criteria settings so we can design if transformed sections or other
   // settings exist that are incompatible with the design algo.
   // This interface is meant to be a back door into the BridgeAgent to be used
   // by the Designer. It should not be used for any other purpose.
   // 
   // Override transformed section analysis (switch to gross)
   virtual void ApplyTransFormedSectionOverride() = 0;
   virtual void RemoveTransFormedSectionOverride() = 0;
   virtual bool IsTransFormedSectionOverriden() = 0;

   // Override Parabolic Composite section properties to make
   // constant haunch along girders
   virtual void ApplyParabolicCompositeSectionOverride() = 0;
   virtual void RemoveParabolicCompositeSectionOverride() = 0;
   virtual bool IsParabolicCompositeSectionOverriden() = 0;

};


