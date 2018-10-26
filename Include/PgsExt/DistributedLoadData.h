///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2017  Washington State Department of Transportation
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

// DistributedLoadData.h: interface for the CDistributedLoadData class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DISTRIBUTEDLOADDATA_H__83982300_F548_44FC_B84A_A7C9731FE381__INCLUDED_)
#define AFX_DISTRIBUTEDLOADDATA_H__83982300_F548_44FC_B84A_A7C9731FE381__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <PgsExt\PgsExtExp.h>

#include <PgsExt\PointLoadData.h>  // for enums

struct IStructuredSave;
struct IStructuredLoad;

class PGSEXTCLASS CDistributedLoadData  
{
public:
	CDistributedLoadData();
   CDistributedLoadData(const CDistributedLoadData& other);
	virtual ~CDistributedLoadData();

   CDistributedLoadData& operator=(const CDistributedLoadData& other);

   HRESULT Save(IStructuredSave* pSave);
   HRESULT Load(IStructuredLoad* pSave);

   bool operator == (const CDistributedLoadData& rOther) const;
   bool operator != (const CDistributedLoadData& rOther) const;

   // properties
   IDType                         m_ID; // this is the load's ID
   UserLoads::LoadCase            m_LoadCase;
   UserLoads::DistributedLoadType m_Type;

   CSpanKey m_SpanKey;
   Float64  m_StartLocation; 
   Float64  m_EndLocation;  
   Float64  m_WStart;    // if load type is uniform, then locations are ignored and this is only 
   Float64  m_WEnd;
   bool     m_Fractional;
   std::_tstring m_Description;

   EventIndexType m_StageIndex; // this is the event index corrosponding to the old stage model (BrideSite1,2,3)

protected:
   void MakeCopy(const CDistributedLoadData& rOther);
   void MakeAssignment(const CDistributedLoadData& rOther);
};

#endif // !defined(AFX_DISTRIBUTEDLOADDATA_H__83982300_F548_44FC_B84A_A7C9731FE381__INCLUDED_)
