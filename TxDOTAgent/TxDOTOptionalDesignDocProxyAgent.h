///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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

#pragma once

#include <EAF\EAFInterfaceCache.h>

#include <IFace\UpdateTemplates.h>
#include <IFace\Selection.h>
#include <IFace\VersionInfo.h>
#include "TxDOTOptionalDesignData.h"

#include <pgsExt\GirderArtifact.h>


class CTxDOTOptionalDesignDoc;
struct IBroker;

// {4C58E3C0-6BEC-44bc-AF29-35596951BCBB}
DEFINE_GUID(CLSID_TxDOTOptionalDesignDocProxyAgent, 
0x4c58e3c0, 0x6bec, 0x44bc, 0xaf, 0x29, 0x35, 0x59, 0x69, 0x51, 0xbc, 0xbb);


/*****************************************************************************
CLASS 
   CTxDOTOptionalDesignDocProxyAgent

   Proxy agent for CTxDOTOptionalDesign document.


DESCRIPTION
   Proxy agent for CTxDOTOptionalDesign document.

   Instances of this object allow the CDocument class to be plugged into the
   Agent-Broker architecture.


COPYRIGHT
   Copyright © 1997-1998
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rab : 11.01.1998 : Created file
   rdp : 02.21.2010 : Turned into txdot optional design agent
*****************************************************************************/

class CTxDOTOptionalDesignDocProxyAgent :
   public CComObjectRootEx<CComSingleThreadModel>,
   public CComCoClass<CTxDOTOptionalDesignDocProxyAgent,&CLSID_TxDOTOptionalDesignDocProxyAgent>,
   public IAgentEx,
   public IUpdateTemplates,
   public ISelection,
   public IVersionInfo,
   public IGetTogaData,
   public IGetTogaResults,
   public ITxDataObserver
{
public:
   CTxDOTOptionalDesignDocProxyAgent();
   ~CTxDOTOptionalDesignDocProxyAgent();

BEGIN_COM_MAP(CTxDOTOptionalDesignDocProxyAgent)
   COM_INTERFACE_ENTRY(IAgent)
   COM_INTERFACE_ENTRY(IAgentEx)
   COM_INTERFACE_ENTRY(IUpdateTemplates)
   COM_INTERFACE_ENTRY(ISelection)
   COM_INTERFACE_ENTRY(IVersionInfo)
   COM_INTERFACE_ENTRY(IGetTogaData)
   COM_INTERFACE_ENTRY(IGetTogaResults)
END_COM_MAP()

   void SetDocument(CTxDOTOptionalDesignDoc* pDoc);

// IAgentEx
public:
   STDMETHOD(SetBroker)(/*[in]*/ IBroker* pBroker);
	STDMETHOD(RegInterfaces)();
	STDMETHOD(Init)();
	STDMETHOD(Init2)();
	STDMETHOD(Reset)();
	STDMETHOD(ShutDown)();
   STDMETHOD(GetClassID)(CLSID* pCLSID);

// IUpdateTemplates
public:
   virtual bool UpdatingTemplates();

   // ISelection
public:
   virtual CSelection GetSelection();
   virtual void ClearSelection();
   virtual PierIndexType GetSelectedPier();
   virtual SpanIndexType GetSelectedSpan();
   virtual CGirderKey GetSelectedGirder();
   virtual CSegmentKey GetSelectedSegment();
   virtual CClosureKey GetSelectedClosureJoint();
   virtual SupportIDType GetSelectedTemporarySupport();
   virtual bool IsDeckSelected();
   virtual bool IsAlignmentSelected();
   virtual void SelectPier(PierIndexType pierIdx);
   virtual void SelectSpan(SpanIndexType spanIdx);
   virtual void SelectGirder(const CGirderKey& girderKey);
   virtual void SelectSegment(const CSegmentKey& segmentKey);
   virtual void SelectClosureJoint(const CClosureKey& closureKey);
   virtual void SelectTemporarySupport(SupportIDType tsID);
   virtual void SelectDeck();
   virtual void SelectAlignment();
   virtual Float64 GetSectionCutStation();

// IVersionInfo
public:
   virtual CString GetVersionString(bool bIncludeBuildNumber = false);
   virtual CString GetVersion(bool bIncludeBuildNumber = false);

// IGetTogaData
   virtual const CTxDOTOptionalDesignData* GetTogaData();

// IGetTogaResults
   virtual void GetControllingTensileStress(Float64* pStress, Float64* pStressFactor, Float64* pDistFromStart);
   virtual void GetControllingCompressiveStress(Float64* pStress, Float64* pStressFactor, Float64* pDistFromStart);

   virtual Float64 GetUltimateMomentCapacity();
   virtual Float64 GetMaximumCamber();

   virtual Float64 GetRequiredUltimateMoment();
   virtual Float64 GetRequiredFc();
   virtual Float64 GetRequiredFci();

   virtual const pgsGirderArtifact* GetFabricatorDesignArtifact();
   virtual Float64 GetFabricatorMaximumCamber();

   virtual bool ShearPassed();

// ITxDataObserver
   virtual void OnTxDotDataChanged(int change);


private:
   DECLARE_AGENT_DATA;

   CTxDOTOptionalDesignDoc* m_pTxDOTOptionalDesignDoc;

   // Cached response data
   bool m_NeedValidate;

   pgsGirderArtifact m_GirderArtifact;

   Float64 m_CtrlTensileStress;
   Float64 m_CtrlTensileStressLocation;
   Float64 m_CtrlTensileStressFactor;

   Float64 m_CtrlCompressiveStress;
   Float64 m_CtrlCompressiveStressLocation;
   Float64 m_CtrlCompressiveStressFactor;

   Float64 m_RequiredUltimateMoment;
   Float64 m_UltimateMomentCapacity;

   Float64 m_MaximumCamber;
   Float64 m_FabricatorMaximumCamber;

   Float64 m_RequiredFc;
   Float64 m_RequiredFci;

   bool m_ShearPassed;

   void Validate();
   void CheckShear(IPointOfInterest* pPoi);
};

