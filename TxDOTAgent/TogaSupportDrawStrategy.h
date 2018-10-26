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

#ifndef INCLUDED_TogaSupportDrawStrategy_H_
#define INCLUDED_TogaSupportDrawStrategy_H_

// {DBA89E78-838D-4a9b-95DF-0885FD85580C}
DEFINE_GUID(IID_iTogaSupportDrawStrategy, 
0xdba89e78, 0x838d, 0x4a9b, 0x95, 0xdf, 0x8, 0x85, 0xfd, 0x85, 0x58, 0xc);

interface iTogaSupportDrawStrategy : public IUnknown
{
//   STDMETHOD_(void,SetSupport)(ISupport* jnt, long supportID) PURE;
};

#endif // INCLUDED_TogaSupportDrawStrategy_H_