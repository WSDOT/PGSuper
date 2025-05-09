///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2025  Washington State Department of Transportation
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

#include "CLSID.h"

#include <EAF\EAFInterfaceCache.h>

#include <IFace\UpdateTemplates.h>
#include <IFace\Selection.h>
#include <IFace\VersionInfo.h>
#include <IFace\DocumentType.h>
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
   public IDocumentType,
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
   COM_INTERFACE_ENTRY(IDocumentType)
   COM_INTERFACE_ENTRY(IVersionInfo)
   COM_INTERFACE_ENTRY(IGetTogaData)
   COM_INTERFACE_ENTRY(IGetTogaResults)
END_COM_MAP()

   void SetDocument(CTxDOTOptionalDesignDoc* pDoc);

// IAgentEx
public:
   STDMETHOD(SetBroker)(/*[in]*/ IBroker* pBroker) override;
	STDMETHOD(RegInterfaces)() override;
	STDMETHOD(Init)() override;
	STDMETHOD(Init2)() override;
	STDMETHOD(Reset)() override;
	STDMETHOD(ShutDown)() override;
   STDMETHOD(GetClassID)(CLSID* pCLSID) override;

// IUpdateTemplates
public:
   virtual bool UpdatingTemplates() override;

   // ISelection
public:
   virtual CSelection GetSelection() override;
   virtual void ClearSelection() override;
   virtual PierIndexType GetSelectedPier() override;
   virtual SpanIndexType GetSelectedSpan() override;
   virtual CGirderKey GetSelectedGirder() override;
   virtual CSegmentKey GetSelectedSegment() override;
   virtual CClosureKey GetSelectedClosureJoint() override;
   virtual SupportIDType GetSelectedTemporarySupport() override;
   virtual bool IsDeckSelected() override;
   virtual bool IsAlignmentSelected() override;
   virtual bool IsRailingSystemSelected(pgsTypes::TrafficBarrierOrientation orientation) override;
   virtual void SelectPier(PierIndexType pierIdx) override;
   virtual void SelectSpan(SpanIndexType spanIdx) override;
   virtual void SelectGirder(const CGirderKey& girderKey) override;
   virtual void SelectSegment(const CSegmentKey& segmentKey) override;
   virtual void SelectClosureJoint(const CClosureKey& closureKey) override;
   virtual void SelectTemporarySupport(SupportIDType tsID) override;
   virtual void SelectDeck() override;
   virtual void SelectAlignment() override;
   virtual void SelectRailingSystem(pgsTypes::TrafficBarrierOrientation orientation) override;
   virtual Float64 GetSectionCutStation() override;

// IDocumentType
public:
   virtual bool IsPGSuperDocument() override { return true; }
   virtual bool IsPGSpliceDocument() override { return false; }

// IVersionInfo
public:
   virtual CString GetVersionString(bool bIncludeBuildNumber = false) override;
   virtual CString GetVersion(bool bIncludeBuildNumber = false) override;

// IGetTogaData
   virtual const CTxDOTOptionalDesignData* GetTogaData() override;

// IGetTogaResults
   virtual void GetControllingTensileStress(Float64* pStress, Float64* pStressFactor, Float64* pDistFromStart) override;
   virtual void GetControllingCompressiveStress(Float64* pStress, Float64* pStressFactor, Float64* pDistFromStart) override;

   virtual Float64 GetUltimateMomentCapacity() override;
   virtual Float64 GetMaximumCamber() override;

   virtual Float64 GetRequiredUltimateMoment() override;
   virtual Float64 GetRequiredFc() override;
   virtual Float64 GetRequiredFci() override;

   virtual const pgsGirderArtifact* GetFabricatorDesignArtifact() override;
   virtual Float64 GetFabricatorMaximumCamber() override;

   virtual bool ShearPassed() override;

// ITxDataObserver
   virtual void OnTxDotDataChanged(int change) override;


private:
   DECLARE_EAF_AGENT_DATA;

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

