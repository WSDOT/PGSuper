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

#ifndef INCLUDED_DISTRIBUTEDLOADDRAWSTRATEGY_H_
#define INCLUDED_DISTRIBUTEDLOADDRAWSTRATEGY_H_

#include <IFace\Bridge.h>
#include <PgsExt\DistributedLoadData.h>
#include <DManip\DManip.h>

interface IBroker;

// {1B3F3992-4CE5-4216-BF6F-50418291E670}
DEFINE_GUID(IID_iDistributedLoadDrawStrategy, 
0x1b3f3992, 0x4ce5, 0x4216, 0xbf, 0x6f, 0x50, 0x41, 0x82, 0x91, 0xe6, 0x70);

interface iDistributedLoadDrawStrategy : public IUnknown
{
  STDMETHOD_(void,Init)(iPointDisplayObject* pDO, IBroker* pBroker, CDistributedLoadData load, CollectionIndexType loadIndex, 
                        Float64 loadLength, Float64 spanLength, Float64 maxMagnitude, COLORREF color) PURE;
};

#endif // INCLUDED_DISTRIBUTEDLOADDRAWSTRATEGY_H_