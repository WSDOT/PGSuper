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

#ifndef INCLUDED_PGSEXT_GIRDERSPACING_H_
#define INCLUDED_PGSEXT_GIRDERSPACING_H_



#include "GirderTypes.h"

class CSpanData;
class CGirderSpacing2;

///////////////////////////////////////////////////////
// NOTE: 
// This class only exists to load old PGSuper files.
///////////////////////////////////////////////////////

/*****************************************************************************
CLASS 
   CGirderSpacing

   Utility class for girder spacing information.

DESCRIPTION
   Utility class for girder spacing information.

LOG
   rab : 04.25.208 : Created file
*****************************************************************************/
class CGirderSpacingData
{
public:
   CGirderSpacingData();
   CGirderSpacingData(const CGirderSpacingData& rOther);
   ~CGirderSpacingData();

   CGirderSpacingData& operator = (const CGirderSpacingData& rOther);
   bool operator == (const CGirderSpacingData& rOther) const;
   bool operator != (const CGirderSpacingData& rOther) const;

   HRESULT Load(IStructuredLoad* pStrLoad,std::shared_ptr<IEAFProgress> pProgress);
   HRESULT Save(IStructuredSave* pStrSave,std::shared_ptr<IEAFProgress> pProgress);

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

   // Returns the out-to-out width of the girder spacing (summation of the individual spaces)
   Float64 GetSpacingWidth() const; 

   // Returns the width of the girder spacing from the left most girder to
   // the specified girder index (summation of the individual spaces)
   Float64 GetSpacingWidthToGirder(GirderIndexType gdrIdx) const; 

   void SetSpacingData(CGirderSpacing2* pGirderSpacing) const;

#if defined _DEBUG
   GirderIndexType Debug_GetGirderCount() const { return m_GirderSpacing.size()+1; }
   void AssertValid() const;
#endif

protected:
   void MakeCopy(const CGirderSpacingData& rOther);
   void MakeAssignment(const CGirderSpacingData& rOther);

   pgsTypes::MeasurementType m_MeasurementType;
   pgsTypes::MeasurementLocation m_MeasurementLocation;

   GirderIndexType m_RefGirderIdx; // use ALL_GIRDERS to indicate centers of the girder group
   pgsTypes::OffsetMeasurementType m_RefGirderOffsetType;
   Float64 m_RefGirderOffset;

   std::vector<Float64> m_GirderSpacing;

   Float64 m_DefaultSpacing; // default spacing... 5ft, or cache of last spacing removed

   typedef std::pair<GirderIndexType,GirderIndexType> SpacingGroup; // first and second can never be the same value
   std::vector<SpacingGroup> m_SpacingGroups; // defines how girder lines are grouped
};

class PSGLIBCLASS CGirderSpacing : public CGirderSpacingData
{
public:
   CGirderSpacing();
   CGirderSpacing(const CGirderSpacingData& rOther);
   CGirderSpacing(const CGirderSpacing& rOther);
   ~CGirderSpacing();

   CGirderSpacing& operator = (const CGirderSpacing& rOther);

   void SetSpan(const CSpanData* pSpan);

   virtual pgsTypes::MeasurementType GetMeasurementType() const override;
   virtual pgsTypes::MeasurementLocation GetMeasurementLocation() const override;
   virtual GirderIndexType GetRefGirder() const override;
   virtual Float64 GetRefGirderOffset() const override;
   virtual pgsTypes::OffsetMeasurementType GetRefGirderOffsetType() const override;
   virtual void SetGirderSpacing(GroupIndexType grpIdx,Float64 s) override;
   virtual GroupIndexType GetSpacingGroupCount() const override;
   virtual void GetSpacingGroup(GroupIndexType groupIdx,GirderIndexType* pFirstGdrIdx,GirderIndexType* pLastGdrIdx,Float64* pSpacing) const override;
   virtual SpacingIndexType GetSpacingCount() const override;
   virtual Float64 GetGirderSpacing(SpacingIndexType spacingIdx) const override;

protected:
   friend CSpanData;
   const CSpanData* m_pSpan;

   void MakeCopy(const CGirderSpacing& rOther);
   void MakeAssignment(const CGirderSpacing& rOther);
};

#endif // INCLUDED_PGSEXT_GIRDERSPACING_H_
