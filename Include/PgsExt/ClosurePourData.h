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
#pragma once

#include <PgsExt\PgsExtExp.h>
#include <WBFLCore.h>
#include <StrData.h>
#include <PgsExt\SplicedGirderData.h>
#include <PgsExt\PrecastSegmentData.h>
#include <PgsExt\ConcreteMaterial.h>
#include <PsgLib\ShearData.h>
#include <PgsExt\TemporarySupportData.h>
#include <PgsExt\PierData2.h>
#include <PgsExt\LongitudinalRebarData.h>
#include <boost\shared_ptr.hpp>

/*****************************************************************************
CLASS 
   CClosurePourData

   Utility class for cast-in-place concrete closure pour input parameters.

DESCRIPTION
   Utility class for cast-in-place concrete closure pour input parameters.
*****************************************************************************/

class PGSEXTCLASS CClosurePourData
{
public:
   // This is more like a "Joint" in a spliced girder. Joints can have a closure
   // pour or are match cast.

   // The cross section of the closure pour is taken from the precast elements on both sides.
   // Length is computed from the connection geometry at the associated temporary support or pier.

   // Closure objects are associated with either a pier or a temporary support.
   // Use GetPier and GetTemporarySupport to return the associated object.

   CClosurePourData();
   CClosurePourData(CSplicedGirderData* pGirder,const CTemporarySupportData* pTempSupport);
   CClosurePourData(CSplicedGirderData* pGirder,const CPierData2* pPier);
   CClosurePourData(const CClosurePourData& rOther); // copies only data, not ID or Index
   ~CClosurePourData();

   CClosurePourData& operator = (const CClosurePourData& rOther);
   void CopyClosurePourData(const CClosurePourData* pClosure);
   bool operator==(const CClosurePourData& rOther) const;
   bool operator!=(const CClosurePourData& rOther) const;
   bool operator<(const CClosurePourData& rOther) const;

   CollectionIndexType GetIndex() const;
   IDType GetID() const;

   CClosureKey GetClosureKey() const;

   void SetGirder(CSplicedGirderData* pGirder);
   CSplicedGirderData* GetGirder();
   const CSplicedGirderData* GetGirder() const;

   void SetTemporarySupport(const CTemporarySupportData* pTS);
   const CTemporarySupportData* GetTemporarySupport() const;
   SupportIndexType GetTemporarySupportIndex() const;
   SupportIDType GetTemporarySupportID() const;

   void SetPier(const CPierData2* pPier);
   const CPierData2* GetPier() const;
   PierIndexType GetPierIndex() const;
   PierIDType GetPierID() const;

   void SetLeftSegment(CPrecastSegmentData* pLeftSegment);
   const CPrecastSegmentData* GetLeftSegment() const;
   CPrecastSegmentData* GetLeftSegment();

   void SetRightSegment(CPrecastSegmentData* pRightSegment);
   const CPrecastSegmentData* GetRightSegment() const;
   CPrecastSegmentData* GetRightSegment();

   // Set/Get the slab offset at this closure pour
   // Valid for slab offset type of pgsTypes::sotSegment
   void SetSlabOffset(Float64 offset);
   Float64 GetSlabOffset() const;

   void SetConcrete(const CConcreteMaterial& concrete);
   const CConcreteMaterial& GetConcrete() const;
   CConcreteMaterial& GetConcrete();

   void SetStirrups(const CShearData2& stirrups);
   const CShearData2& GetStirrups() const;
   CShearData2& GetStirrups();

   void SetRebar(const CLongitudinalRebarData& rebar);
   const CLongitudinalRebarData& GetRebar() const;
   CLongitudinalRebarData& GetRebar();

   void CopyMaterialFrom(const CClosurePourData& rOther);
   void CopyLongitudinalReinforcementFrom(const CClosurePourData& rOther);
   void CopyTransverseReinforcementFrom(const CClosurePourData& rOther);

	HRESULT Load(IStructuredLoad* pStrLoad,IProgress* pProgress);
	HRESULT Save(IStructuredSave* pStrSave,IProgress* pProgress);

#if defined _DEBUG
   void AssertValid();
#endif

protected:
   void MakeCopy(const CClosurePourData& rOther,bool bCopyDataOnly);
   virtual void MakeAssignment(const CClosurePourData& rOther);
   void ResolveReferences();

   void SetIndex(CollectionIndexType idx) { m_Index = idx; }
   CollectionIndexType m_Index; // index of this closure pour

   Float64 m_SlabOffset;

   CSplicedGirderData* m_pGirder; // weak reference to girder this closure is a part of
   const CTemporarySupportData* m_pTempSupport;
   const CPierData2* m_pPier;
   CPrecastSegmentData* m_pLeftSegment;   // weak reference... owned by CSplicedGirderData
   CPrecastSegmentData* m_pRightSegment;  // weak reference... owned by CSplicedGirderData

   CConcreteMaterial m_Concrete;
   CShearData2 m_Stirrups;
   CLongitudinalRebarData m_Rebar;

   friend CSplicedGirderData;

   // These ID values are INVALID_ID when m_pTempSupport or m_pPier are defined,
   // otherwise they contain the ID of a support element that needs to be resolved
   SupportIDType m_TempSupportID;
   PierIDType m_PierID;
};
