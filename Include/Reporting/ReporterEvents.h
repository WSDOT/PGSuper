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

/*****************************************************************************
INTERFACE
   IReporterEventSink

   Callback interface for report agent events.
*****************************************************************************/
// {E2E4D451-EFA3-408D-8674-6BD3EE70CAE5}
DEFINE_GUID(IID_IReporterEventSink, 
0xe2e4d451, 0xefa3, 0x408d, 0x86, 0x74, 0x6b, 0xd3, 0xee, 0x70, 0xca, 0xe5);
class __declspec(uuid("{E2E4D451-EFA3-408D-8674-6BD3EE70CAE5}")) IReporterEventSink
{
public:
   virtual HRESULT OnReportsChanged() = 0;
};
