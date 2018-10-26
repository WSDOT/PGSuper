///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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

#ifndef INCLUDED_SECTIONCUTDRAWSTRATEGY_H_
#define INCLUDED_SECTIONCUTDRAWSTRATEGY_H_

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

// {C56878AE-5504-4f1a-A060-F2C56991663D}
DEFINE_GUID(IID_iSectionCutDrawStrategy, 
0xc56878ae, 0x5504, 0x4f1a, 0xa0, 0x60, 0xf2, 0xc5, 0x69, 0x91, 0x66, 0x3d);

interface iSectionCutDrawStrategy : public IUnknown
{
   STDMETHOD_(void,SetColor)(COLORREF color) PURE;
	STDMETHOD_(void,Init)(iPointDisplayObject* pDO, IBroker* pBroker,SpanIndexType spanIdx,GirderIndexType gdrIdx, iCutLocation* pCutLoc) PURE;
};

#endif // INCLUDED_SECTIONCUTDRAWSTRATEGY_H_