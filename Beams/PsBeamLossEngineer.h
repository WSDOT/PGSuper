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

// PsBeamLossEngineer.h : Declaration of the CPsBeamLossEngineer

#ifndef __PSBEAMLOSSENGINEER_H_
#define __PSBEAMLOSSENGINEER_H_

#include "resource.h"       // main symbols
#include "IFace\PsLossEngineer.h"
#include "Beams\Interfaces.h"
#include "PsLossEngineer.h"
#include <Plugins\CLSID.h>

#include <PgsExt\PoiKey.h>

// Class for storing Design Losses
// Design losses are losses computed during design iterations and need not
// be recomputed if the design is accepted
class CDesignLosses
{
   struct Losses
   {
      GDRCONFIG m_Config;
      LOSSDETAILS m_Details;
   };

   std::map<pgsPointOfInterest,Losses,ComparePoi> m_Losses;
   
public:
   CDesignLosses();
   void Invalidate();
   const LOSSDETAILS* GetFromCache(const pgsPointOfInterest& poi, const GDRCONFIG& config);
   void SaveToCache(const pgsPointOfInterest& poi, const GDRCONFIG& config, const LOSSDETAILS& losses);
};

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
   virtual void SetBroker(IBroker* pBroker,StatusGroupIDType statusGroupID) override;

// IPsBeamLossEngineer
public:
   virtual void Init(BeamTypes beamType)  override { m_BeamType = beamType; }

// IPsLossEngineer
public:
   virtual const LOSSDETAILS* GetLosses(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx) override;
   virtual const LOSSDETAILS* GetLosses(const pgsPointOfInterest& poi,const GDRCONFIG& config,IntervalIndexType intervalIdx) override;
   virtual void ClearDesignLosses() override;
   virtual void BuildReport(const CGirderKey& girderKey,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits) override;
   virtual void ReportFinalLosses(const CGirderKey& girderKey,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits) override;
   virtual const ANCHORSETDETAILS* GetAnchorSetDetails(const CGirderKey& girderKey,DuctIndexType ductIdx) override;
   virtual Float64 GetElongation(const CGirderKey& girderKey,DuctIndexType ductIdx,pgsTypes::MemberEndType endType) override;
   virtual void GetAverageFrictionAndAnchorSetLoss(const CGirderKey& girderKey,DuctIndexType ductIdx,Float64* pfpF,Float64* pfpA) override;

private:
   IBroker* m_pBroker;
   StatusGroupIDType m_StatusGroupID;
   BeamTypes m_BeamType;
   CPsLossEngineer m_Engineer;


   // Losses are cached for two different cases:
   // 1) This data structure caches losses for the current project data
   std::map<PoiIDType,LOSSDETAILS> m_PsLosses;

   // 2) This data structure is for design cases. It caches the most recently
   //    computed losses
   CDesignLosses m_DesignLosses;
};

#endif //__PSBEAMLOSSENGINEER_H_
