///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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
// MomentLoadData.h: interface for the CMomentLoadData class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_MOMENTLOADDATA_H__83982300_F548_44FC_B84A_A7C9731FE381__INCLUDED_)
#define AFX_MOMENTLOADDATA_H__83982300_F548_44FC_B84A_A7C9731FE381__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <PgsExt\PgsExtExp.h>

#include <PgsExt\PointLoadData.h>  // for enums

struct IStructuredSave;
struct IStructuredLoad;

class PGSEXTCLASS CMomentLoadData  
{
public:
	CMomentLoadData();
	virtual ~CMomentLoadData();

   HRESULT Save(IStructuredSave* pSave);
   HRESULT Load(IStructuredLoad* pSave);

   bool operator == (const CMomentLoadData& rOther) const;
   bool operator != (const CMomentLoadData& rOther) const;

   // properties
   IDType                         m_ID;
   EventIndexType                 m_EventIndex;
   UserLoads::LoadCase            m_LoadCase;

   CSpanGirderKey m_SpanGirderKey;
   Float64  m_Location;   // cannot be negative
   bool     m_Fractional;
   Float64  m_Magnitude;
   std::_tstring m_Description;
};

#endif // !defined(AFX_MOMENTLOADDATA_H__83982300_F548_44FC_B84A_A7C9731FE381__INCLUDED_)
