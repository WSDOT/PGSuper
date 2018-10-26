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

#ifndef INCLUDED_PGSEXT_GIRDERSPACING_H_
#define INCLUDED_PGSEXT_GIRDERSPACING_H_

#include <WBFLCore.h>

#if !defined INCLUDED_PGSEXTEXP_H_
#include <PgsExt\PgsExtExp.h>
#endif

#include <PgsExt\GirderTypes.h>

class CSpanData;

/*****************************************************************************
CLASS 
   CGirderSpacing

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
#if defined _DEBUG
#define IS_VALID AssertValid();
#else
#define IS_VALID
#endif

class PGSEXTCLASS CGirderSpacingData
{
public:
   CGirderSpacingData();
   CGirderSpacingData(const CGirderSpacingData& rOther);
   ~CGirderSpacingData();

   CGirderSpacingData& operator = (const CGirderSpacingData& rOther);
   bool operator == (const CGirderSpacingData& rOther) const;
   bool operator != (const CGirderSpacingData& rOther) const;

   HRESULT Load(IStructuredLoad* pStrLoad,IProgress* pProgress);
   HRESULT Save(IStructuredSave* pStrSave,IProgress* pProgress);

   virtual void SetMeasurementType(pgsTypes::MeasurementType mt);
   virtual pgsTypes::MeasurementType GetMeasurementType() const;

   virtual void SetMeasurementLocation(pgsTypes::MeasurementLocation ml);
   virtual pgsTypes::MeasurementLocation GetMeasurementLocation() const;

   virtual void SetRefGirder(GirderIndexType refGdrIdx);
   virtual GirderIndexType GetRefGirder() const;
   virtual void SetRefGirderOffset(double offset);
   virtual double GetRefGirderOffset() const;
   virtual void SetRefGirderOffsetType(pgsTypes::OffsetMeasurementType offsetDatum);
   virtual pgsTypes::OffsetMeasurementType GetRefGirderOffsetType() const;

   virtual void SetGirderSpacing(GroupIndexType grpIdx,double s);
   virtual GroupIndexType GetSpacingGroupCount() const;
   virtual void GetSpacingGroup(GroupIndexType groupIdx,GirderIndexType* pFirstGdrIdx,GirderIndexType* pLastGdrIdx,double* pSpacing) const;
   virtual SpacingIndexType GetSpacingCount() const;
   virtual double GetGirderSpacing(SpacingIndexType spacingIdx) const;

   void ExpandAll();
   void Expand(GroupIndexType grpIdx);
   void JoinAll(SpacingIndexType spacingKey);
   void Join(GirderIndexType firstGdrIdx,GirderIndexType lastGdrIdx,SpacingIndexType spacingKey);

   void SetGirderCount(GirderIndexType nGirders);
   void AddGirders(GirderIndexType nGirders);
   void RemoveGirders(GirderIndexType nGirders);

#if defined _DEBUG
   long Debug_GetGirderCount() const { return m_GirderSpacing.size()+1; }
   void AssertValid() const;
#endif

protected:
   void MakeCopy(const CGirderSpacingData& rOther);
   void MakeAssignment(const CGirderSpacingData& rOther);

   pgsTypes::MeasurementType m_MeasurementType;
   pgsTypes::MeasurementLocation m_MeasurementLocation;

   GirderIndexType m_RefGirderIdx; // use ALL_GIRDERS to indicate centers of the girder group
   pgsTypes::OffsetMeasurementType m_RefGirderOffsetType;
   double m_RefGirderOffset;

   std::vector<double> m_GirderSpacing;

   double m_DefaultSpacing; // default spacing... 5ft, or cache of last spacing removed

   typedef std::pair<GirderIndexType,GirderIndexType> SpacingGroup; // first and second can never be the same value
   std::vector<SpacingGroup> m_SpacingGroups; // defines how girder lines are grouped
};

class PGSEXTCLASS CGirderSpacing : public CGirderSpacingData
{
public:
   CGirderSpacing();
   CGirderSpacing(const CGirderSpacingData& rOther);
   CGirderSpacing(const CGirderSpacing& rOther);
   ~CGirderSpacing();

   CGirderSpacing& operator = (const CGirderSpacing& rOther);

   void SetSpan(const CSpanData* pSpan);

   virtual pgsTypes::MeasurementType GetMeasurementType() const;
   virtual pgsTypes::MeasurementLocation GetMeasurementLocation() const;
   virtual GirderIndexType GetRefGirder() const;
   virtual double GetRefGirderOffset() const;
   virtual pgsTypes::OffsetMeasurementType GetRefGirderOffsetType() const;
   virtual void SetGirderSpacing(GroupIndexType grpIdx,double s);
   virtual GroupIndexType GetSpacingGroupCount() const;
   virtual void GetSpacingGroup(GroupIndexType groupIdx,GirderIndexType* pFirstGdrIdx,GirderIndexType* pLastGdrIdx,double* pSpacing) const;
   virtual SpacingIndexType GetSpacingCount() const;
   virtual double GetGirderSpacing(SpacingIndexType spacingIdx) const;

protected:
   friend CSpanData;
   const CSpanData* m_pSpan;

   void MakeCopy(const CGirderSpacing& rOther);
   void MakeAssignment(const CGirderSpacing& rOther);
};

#endif // INCLUDED_PGSEXT_GIRDERSPACING_H_
