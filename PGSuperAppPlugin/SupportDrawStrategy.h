///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2020  Washington State Department of Transportation
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

#ifndef INCLUDED_SUPPORTDRAWSTRATEGY_H_
#define INCLUDED_SUPPORTDRAWSTRATEGY_H_

// {A1C49648-0A01-4126-93A9-AC059A3D9DF9}
DEFINE_GUID(IID_iSupportDrawStrategy, 
0xa1c49648, 0xa01, 0x4126, 0x93, 0xa9, 0xac, 0x5, 0x9a, 0x3d, 0x9d, 0xf9);

interface iSupportDrawStrategy : public IUnknown
{
//   STDMETHOD_(void,SetSupport)(ISupport* jnt, long supportID) PURE;
};

#endif // INCLUDED_SUPPORTDRAWSTRATEGY_H_