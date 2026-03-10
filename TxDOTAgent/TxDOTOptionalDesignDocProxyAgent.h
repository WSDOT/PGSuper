///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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

#include <EAF\Agent.h>


#include <IFace\UpdateTemplates.h>
#include <IFace\Selection.h>
#include <IFace\VersionInfo.h>
#include <IFace\DocumentType.h>
#include <IFace/PointOfInterest.h>

#include "TxDOTOptionalDesignData.h"

#include <pgsExt\GirderArtifact.h>


class CTxDOTOptionalDesignDoc;

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

class CTxDOTOptionalDesignDocProxyAgent : public WBFL::EAF::Agent,
   public IUpdateTemplates,
   public ISelection,
   public IDocumentType,
   public IVersionInfo,
   public IGetTogaData,
   public IGetTogaResults,
   public ITxDataObserver
{
public:
   CTxDOTOptionalDesignDocProxyAgent(CTxDOTOptionalDesignDoc* pDoc);
   ~CTxDOTOptionalDesignDocProxyAgent();

// Agent
public:
   std::_tstring GetName() const override { return _T("TOGO DocProxy Agent"); }
   bool RegisterInterfaces() override;
   bool Init() override;
   bool Reset() override;
   bool ShutDown() override;
   CLSID GetCLSID() const override;

// IUpdateTemplates
public:
   bool UpdatingTemplates() override;

   // ISelection
public:
   CSelection GetSelection() override;
   void ClearSelection() override;
   PierIndexType GetSelectedPier() override;
   SpanIndexType GetSelectedSpan() override;
   CGirderKey GetSelectedGirder() override;
   CSegmentKey GetSelectedSegment() override;
   CClosureKey GetSelectedClosureJoint() override;
   SupportIDType GetSelectedTemporarySupport() override;
   bool IsDeckSelected() override;
   bool IsAlignmentSelected() override;
   bool IsRailingSystemSelected(pgsTypes::TrafficBarrierOrientation orientation) override;
   void SelectPier(PierIndexType pierIdx) override;
   void SelectSpan(SpanIndexType spanIdx) override;
   void SelectGirder(const CGirderKey& girderKey) override;
   void SelectSegment(const CSegmentKey& segmentKey) override;
   void SelectClosureJoint(const CClosureKey& closureKey) override;
   void SelectTemporarySupport(SupportIDType tsID) override;
   void SelectDeck() override;
   void SelectAlignment() override;
   void SelectRailingSystem(pgsTypes::TrafficBarrierOrientation orientation) override;
   Float64 GetSectionCutStation() override;

// IDocumentType
public:
   bool IsPGSuperDocument() const override { return true; }
   bool IsPGSpliceDocument() const override { return false; }

// IVersionInfo
public:
   CString GetVersionString(bool bIncludeBuildNumber = false) override;
   CString GetVersion(bool bIncludeBuildNumber = false) override;

// IGetTogaData
   const CTxDOTOptionalDesignData* GetTogaData() override;

// IGetTogaResults
   void GetControllingTensileStress(Float64* pStress, Float64* pStressFactor, Float64* pDistFromStart) override;
   void GetControllingCompressiveStress(Float64* pStress, Float64* pStressFactor, Float64* pDistFromStart) override;

   Float64 GetUltimateMomentCapacity() override;
   Float64 GetMaximumCamber() override;

   Float64 GetRequiredUltimateMoment() override;
   Float64 GetRequiredFc() override;
   Float64 GetRequiredFci() override;

   const pgsGirderArtifact* GetFabricatorDesignArtifact() override;
   Float64 GetFabricatorMaximumCamber() override;

   bool ShearPassed() override;

// ITxDataObserver
   void OnTxDotDataChanged(int change) override;


private:
   EAF_DECLARE_AGENT_DATA;

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
   void CheckShear(std::shared_ptr<IPointOfInterest> pPoi);
};

