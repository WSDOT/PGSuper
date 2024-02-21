///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2024  Washington State Department of Transportation
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

#ifndef INCLUDED_POINTLOADDRAWSTRATEGY_H_
#define INCLUDED_POINTLOADDRAWSTRATEGY_H_

#include "IFace\Bridge.h"
#include "pgsExt\PointLoadData.h"
#include <DManip/PointDisplayObject.h>

interface IBroker;

class iPointLoadDrawStrategy
{
public:
  virtual void Init(std::shared_ptr<WBFL::DManip::iPointDisplayObject> pDO, IBroker* pBroker, const CPointLoadData& load, IndexType loadIndex, Float64 spanLength, Float64 maxMagnitude, COLORREF color) = 0;
};

#endif // INCLUDED_POINTLOADDRAWSTRATEGY_H_