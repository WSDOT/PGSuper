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

#pragma once

#include <PgsExt\PgsExtExp.h>
#include <WBFLCore.h>

class CPierData2;
class CTemporarySupportData;
class CBridgeDescription2;
class CBridgeDescription;
class CGirderGroupData;
class CSplicedGirderData;

/*****************************************************************************
CLASS 
   CGirderSpacing2

   Utility class for girder spacing information.

DESCRIPTION
   Utility class for girder spacing information.


COPYRIGHT
   Copyright © 1997-2008
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rab : 04.25.208 : Created file
*****************************************************************************/

class PGSEXTCLASS CGirderSpacingData2
{
public:
   CGirderSpacingData2();
   CGirderSpacingData2(const CGirderSpacingData2& rOther);
   ~CGirderSpacingData2();

   CGirderSpacingData2& operator = (const CGirderSpacingData2& rOther);
   bool operator == (const CGirderSpacingData2& rOther) const;
   bool operator != (const CGirderSpacingData2& rOther) const;

   HRESULT Load(IStructuredLoad* pStrLoad,IProgress* pProgress);
   HRESULT Save(IStructuredSave* pStrSave,IProgress* pProgress);

   virtual void SetMeasurementType(pgsTypes::MeasurementType mt);
   virtual pgsTypes::MeasurementType GetMeasurementType() const;

   virtual void SetMeasurementLocation(pgsTypes::MeasurementLocation ml);
   virtual pgsTypes::MeasurementLocation GetMeasurementLocation() const;

   virtual void SetRefGirder(GirderIndexType refGdrIdx);
   virtual GirderIndexType GetRefGirder() const;
   virtual void SetRefGirderOffset(Float64 offset);
   virtual Float64 GetRefGirderOffset() const;
   virtual void SetRefGirderOffsetType(pgsTypes::OffsetMeasurementType offsetDatum);
   virtual pgsTypes::OffsetMeasurementType GetRefGirderOffsetType() const;

   virtual void SetGirderSpacing(GroupIndexType grpIdx,Float64 s);
   virtual GroupIndexType GetSpacingGroupCount() const;
   virtual void GetSpacingGroup(GroupIndexType groupIdx,GirderIndexType* pFirstGdrIdx,GirderIndexType* pLastGdrIdx,Float64* pSpacing) const;
   virtual SpacingIndexType GetSpacingCount() const;
   virtual Float64 GetGirderSpacing(SpacingIndexType spacingIdx) const;

   void ExpandAll();
   void Expand(GroupIndexType grpIdx);
   void JoinAll(SpacingIndexType spacingKey);
   void Join(GirderIndexType firstGdrIdx,GirderIndexType lastGdrIdx,SpacingIndexType spacingKey);

   void SetGirderCount(GirderIndexType nGirders);
   void AddGirders(GirderIndexType nGirders);
   void RemoveGirders(GirderIndexType nGirders);

#if defined _DEBUG
   GirderIndexType Debug_GetGirderCount() const { return m_GirderSpacing.size()+1; }
   virtual void AssertValid() const;
#endif

protected:
   void MakeCopy(const CGirderSpacingData2& rOther);
   virtual void MakeAssignment(const CGirderSpacingData2& rOther);

   pgsTypes::MeasurementType m_MeasurementType;
   pgsTypes::MeasurementLocation m_MeasurementLocation;

   GirderIndexType m_RefGirderIdx; // use ALL_GIRDERS to indicate centers of the girder group
   pgsTypes::OffsetMeasurementType m_RefGirderOffsetType; // defines the location the ref girder offset is measured from (Alignment of BridgeLine)
   Float64 m_RefGirderOffset; // offset to the reference girder, measured from the reference line 
                              // at the location defined by m_MeasurementLocation and
                              // in the direction defined by measurement type


   std::vector<Float64> m_GirderSpacing;

   Float64 m_DefaultSpacing; // default spacing... 5ft, or cache of last spacing removed

   typedef std::pair<GirderIndexType,GirderIndexType> SpacingGroup; // first and second can never be the same value
   std::vector<SpacingGroup> m_SpacingGroups; // defines how girder lines are grouped

   // secret backdoor that certain types can use to initialize
   // this girder spacing so the internal m_GirderSpacing vector has the right
   // number of entires.
   void InitGirderCount(GirderIndexType nGirders);
   friend CGirderGroupData;
   friend CBridgeDescription;
   friend CBridgeDescription2;
};

class PGSEXTCLASS CGirderSpacing2 : public CGirderSpacingData2
{
public:
   CGirderSpacing2();
   CGirderSpacing2(const CGirderSpacingData2& rOther);
   CGirderSpacing2(const CGirderSpacing2& rOther);
   ~CGirderSpacing2();

   CGirderSpacing2& operator = (const CGirderSpacing2& rOther);

   // This spacing can be defined at a pier or a temporary support, but not both
   void SetPier(const CPierData2* pPier);
   const CPierData2* GetPier() const;

   void SetTemporarySupport(const CTemporarySupportData* pTS);
   const CTemporarySupportData* GetTemporarySupport() const;

   const CBridgeDescription2* GetBridgeDescription() const;

   virtual pgsTypes::MeasurementType GetMeasurementType() const;
   virtual pgsTypes::MeasurementLocation GetMeasurementLocation() const;
   virtual GirderIndexType GetRefGirder() const;
   virtual Float64 GetRefGirderOffset() const;
   virtual pgsTypes::OffsetMeasurementType GetRefGirderOffsetType() const;
   virtual void SetGirderSpacing(GroupIndexType spacingGroupIdx,Float64 s);
   virtual GroupIndexType GetSpacingGroupCount() const;
   virtual void GetSpacingGroup(GroupIndexType spacingGroupIdx,GirderIndexType* pFirstGdrIdx,GirderIndexType* pLastGdrIdx,Float64* pSpacing) const;
   virtual SpacingIndexType GetSpacingCount() const;
   virtual Float64 GetGirderSpacing(SpacingIndexType spacingIdx) const;

   // Returns the out-to-out width of the girder spacing (summation of the individual spaces)
   Float64 GetSpacingWidth() const; 

   // Returns the width of the girder spacing from the left most girder to
   // the specified girder index (summation of the individual spaces)
   Float64 GetSpacingWidthToGirder(GirderIndexType gdrIdx) const; 

protected:
   const CPierData2* m_pPier;
   const CTemporarySupportData* m_pTempSupport;

   void MakeCopy(const CGirderSpacing2& rOther);
   virtual void MakeAssignment(const CGirderSpacing2& rOther);

   GirderIndexType GetGirderCount() const;
   const CGirderGroupData* GetGirderGroup() const;
   Float64 GetGirderWidth(const CSplicedGirderData* pGirder) const;

#if defined _DEBUG
   virtual void AssertValid() const;
#endif
};
