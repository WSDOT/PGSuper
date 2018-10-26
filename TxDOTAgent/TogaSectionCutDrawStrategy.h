///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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

#ifndef INCLUDED_TOGASECTIONCUTDRAWSTRATEGY_H_
#define INCLUDED_TOGASECTIONCUTDRAWSTRATEGY_H_

#include <PgsExt\Keys.h>

interface iPointDisplayObject;

// pure virtual class for determining cut location along girder
// no need for com here
class iCutLocation
{
public:

   virtual Float64 GetCurrentCutLocation() = 0;
   virtual void CutAt(Float64 cut)=0;
   virtual void ShowCutDlg()=0;
};

// {8F1327F9-3055-4e44-9E23-71FCDB3B9AA5}
DEFINE_GUID(IID_iTogaSectionCutDrawStrategy, 
0x8f1327f9, 0x3055, 0x4e44, 0x9e, 0x23, 0x71, 0xfc, 0xdb, 0x3b, 0x9a, 0xa5);

interface iTogaSectionCutDrawStrategy : public IUnknown
{
   STDMETHOD_(void,SetColor)(COLORREF color) PURE;
	STDMETHOD_(void,Init)(iPointDisplayObject* pDO, IBroker* pBroker,const CSegmentKey& segmentKey, iCutLocation* pCutLoc) PURE;
};

#endif // INCLUDED_TOGASECTIONCUTDRAWSTRATEGY_H_