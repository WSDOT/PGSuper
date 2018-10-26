///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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

#ifndef INCLUDED_GEVEDITLOAD_H_
#define INCLUDED_GEVEDITLOAD_H_


// {305877B5-8FE7-444d-80CD-88D11026DBB6}
DEFINE_GUID(IID_iGevEditLoad, 
0x305877b5, 0x8fe7, 0x444d, 0x80, 0xcd, 0x88, 0xd1, 0x10, 0x26, 0xdb, 0xb6);


interface iGevEditLoad : public IUnknown
{
  STDMETHOD_(HRESULT,EditLoad)() PURE;
  STDMETHOD_(HRESULT,DeleteLoad)() PURE;
};

#endif // INCLUDED_GEVEDITLOAD_H_