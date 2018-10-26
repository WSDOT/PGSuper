///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright (C) 1999  Washington State Department of Transportation
//                     Bridge and Structures Office
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

#ifndef INCLUDED_MOMENTLOADDRAWSTRATEGY_H_
#define INCLUDED_MOMENTLOADDRAWSTRATEGY_H_

#include "IFace\Bridge.h"
#include "pgsExt\MomentLoadData.h"
#include "DManip\DManip.h"

interface IBroker;

// {4F172957-2BE4-42dd-9FEC-9B9ACA31326D}
DEFINE_GUID(IID_iMomentLoadDrawStrategy, 
0x4f172957, 0x2be4, 0x42dd, 0x9f, 0xec, 0x9b, 0x9a, 0xca, 0x31, 0x32, 0x6d);

interface iMomentLoadDrawStrategy : public IUnknown
{
  STDMETHOD_(void,Init)(iPointDisplayObject* pDO, IBroker* pBroker, CMomentLoadData load, Uint32 loadIndex, 
                        Float64 girderDepth, Float64 spanLength, Float64 maxMagnitude, COLORREF color) PURE;
};

#endif // INCLUDED_POINTLOADDRAWSTRATEGY_H_