///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2012  Washington State Department of Transportation
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
#include "Beams\Interfaces.h"
#include "PsLossEngineer.h"
#include <Plugins\CLSID.h>

/////////////////////////////////////////////////////////////////////////////
// CPsBeamLossEngineer
class ATL_NO_VTABLE CPsBeamLossEngineer : 
   public CComObjectRootEx<CComSingleThreadModel>,
   public CComCoClass<CPsBeamLossEngineer, &CLSID_PsBeamLossEngineer>,
   public IPsBeamLossEngineer,
   public IInitialize
{
public:
	CPsBeamLossEngineer()
	{
	}

   HRESULT FinalConstruct();

DECLARE_REGISTRY_RESOURCEID(IDR_PSBEAMLOSSENGINEER)

BEGIN_COM_MAP(CPsBeamLossEngineer)
   COM_INTERFACE_ENTRY(IPsBeamLossEngineer)
   COM_INTERFACE_ENTRY(IPsLossEngineer)
   COM_INTERFACE_ENTRY(IInitialize)
END_COM_MAP()

// IInitialize
public:
   virtual void SetBroker(IBroker* pBroker,StatusGroupIDType statusGroupID);

// IPsBeamLossEngineer
public:
   virtual void Init(BeamTypes beamType) { m_BeamType = beamType; }

// IPsLossEngineer
public:
   virtual LOSSDETAILS ComputeLosses(const pgsPointOfInterest& poi);
   virtual LOSSDETAILS ComputeLossesForDesign(const pgsPointOfInterest& poi,const GDRCONFIG& config);
   virtual void BuildReport(SpanIndexType span,GirderIndexType gdr,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits);
   virtual void ReportFinalLosses(SpanIndexType span,GirderIndexType gdr,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits);

private:
   IBroker* m_pBroker;
   StatusGroupIDType m_StatusGroupID;
   BeamTypes m_BeamType;
   CPsLossEngineer m_Engineer;
};

#endif //__PSBEAMLOSSENGINEER_H_
