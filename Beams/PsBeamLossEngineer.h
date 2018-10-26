///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2011  Washington State Department of Transportation
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

// PsBeamLossEngineer.h : Declaration of the CPsBeamLossEngineer

#ifndef __PSBEAMLOSSENGINEER_H_
#define __PSBEAMLOSSENGINEER_H_

#include "resource.h"       // main symbols
#include "IFace\PsLossEngineer.h"
#include "PsLossEngineer.h"

// {959A33E0-E1FA-4932-AD3F-5FE259DB3E9A}
DEFINE_GUID(CLSID_PsBeamLossEngineer, 
0x959a33e0, 0xe1fa, 0x4932, 0xad, 0x3f, 0x5f, 0xe2, 0x59, 0xdb, 0x3e, 0x9a);

/////////////////////////////////////////////////////////////////////////////
// CPsBeamLossEngineer
class ATL_NO_VTABLE CPsBeamLossEngineer : 
   public CComObjectRootEx<CComSingleThreadModel>,
   public CComCoClass<CPsBeamLossEngineer, &CLSID_PsBeamLossEngineer>,
   public IPsLossEngineer
{
public:
	CPsBeamLossEngineer()
	{
	}

   HRESULT FinalConstruct();
   void Init(CPsLossEngineer::BeamType beamType) { m_BeamType = beamType; }

BEGIN_COM_MAP(CPsBeamLossEngineer)
   COM_INTERFACE_ENTRY(IPsLossEngineer)
END_COM_MAP()

public:
   virtual void SetBroker(IBroker* pBroker,long statusGroupID);
   virtual LOSSDETAILS ComputeLosses(const pgsPointOfInterest& poi);
   virtual LOSSDETAILS ComputeLossesForDesign(const pgsPointOfInterest& poi,const GDRCONFIG& config);
   virtual void BuildReport(SpanIndexType span,GirderIndexType gdr,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits);
   virtual void ReportFinalLosses(SpanIndexType span,GirderIndexType gdr,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits);

private:
   IBroker* m_pBroker;
   long m_StatusGroupID;
   CPsLossEngineer::BeamType m_BeamType;
   CPsLossEngineer m_Engineer;
};

#endif //__PSBEAMLOSSENGINEER_H_
