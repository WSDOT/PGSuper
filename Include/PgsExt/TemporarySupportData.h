///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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
#include <PgsExt\GirderSpacing2.h>
#include <PsgLib\ConnectionLibraryEntry.h>
#include <array>

class CSpanData2;
class CClosureJointData;
class CBridgeDescription2;

/*****************************************************************************
CLASS 
   CTemporarySupportData

   Utility class for temporary support data

DESCRIPTION
   Utility class for temporary support data. This class encapsulates all
   the input data for a temporary support.

LOG
   rab : 04.25.208 : Created file
*****************************************************************************/

class PGSEXTCLASS CTemporarySupportData
{
public:
   CTemporarySupportData();
   CTemporarySupportData(const CTemporarySupportData& rOther);
   ~CTemporarySupportData();

   // Set/Get the type of temporary support (Erection Tower or Strongback)
   void SetSupportType(pgsTypes::TemporarySupportType type);
   pgsTypes::TemporarySupportType GetSupportType() const;

   // Set/Get the connection type when the temporary support type is Erection Tower.
   // When setting the connection type to pgsTypes::tsctClosureJoint the casting event
   // for the closures that are created at this temporary support are set to castClosureJointEvent
   // otherwise this parameter is not used.
   // When setting the connection type to pgsTypes::tsctContinuousSegment the casting event
   // for the closure joints that are removed are automatically removed from the timeline manager
   void SetConnectionType(pgsTypes::TempSupportSegmentConnectionType type,EventIndexType castClosureJointEvent);
   pgsTypes::TempSupportSegmentConnectionType GetConnectionType() const;

   void SetID(SupportIDType id);
   SupportIDType GetID() const;

   // Index of this temporary support from the start of the bridge
   // This value changes as supports are added, deleted, and moved
   void SetIndex(SupportIndexType idx);
   SupportIndexType GetIndex() const;

   // returns the station where the temporary support is located
   void SetStation(Float64 station);
   Float64 GetStation() const;

   LPCTSTR GetOrientation() const;
   void SetOrientation(LPCTSTR strOrientation);

   void SetSpan(CSpanData2* pSpan);
   CSpanData2* GetSpan();
   const CSpanData2* GetSpan() const;

   CClosureJointData* GetClosureJoint(GirderIndexType gdrIdx);
   const CClosureJointData* GetClosureJoint(GirderIndexType gdrIdx) const;

   CBridgeDescription2* GetBridgeDescription();
   const CBridgeDescription2* GetBridgeDescription() const;

   void SetGirderEndDistance(Float64 endDist,ConnectionLibraryEntry::EndDistanceMeasurementType measure);
   void GetGirderEndDistance(Float64* pEndDist,ConnectionLibraryEntry::EndDistanceMeasurementType* pMeasure) const;

   void SetBearingOffset(Float64 offset,ConnectionLibraryEntry::BearingOffsetMeasurementType measure);
   void GetBearingOffset(Float64* pOffset,ConnectionLibraryEntry::BearingOffsetMeasurementType* pMeasure) const;

   // Slab offset input was removed from PGSplice in 2022. This function is only needed for compatibility with older file format
   void GetSlabOffsetPrivate(Float64* pBackSlabOffset, Float64* pAheadSlabOffset) const;

   bool HasElevationAdjustment() const;
   void SetElevationAdjustment(Float64 elevAdj);
   Float64 GetElevationAdjustment() const;

   // Defines the segment spacing measured at the centerline of this temporary support.
   // Spacing is invalid (not used) if the connection type is sctContinuousSegment.
   // Spacing can only be defined at the ends of precast segments.
   bool HasSpacing() const;
   void SetSegmentSpacing(const CGirderSpacing2& spacing);
   CGirderSpacing2* GetSegmentSpacing();
   const CGirderSpacing2* GetSegmentSpacing() const;

   CTemporarySupportData& operator = (const CTemporarySupportData& rOther);
   void CopyTemporarySupportData(const CTemporarySupportData* pTS);
   bool operator==(const CTemporarySupportData& rOther) const;
   bool operator!=(const CTemporarySupportData& rOther) const;
   bool operator<(const CTemporarySupportData& rOther) const;

   HRESULT Load(IStructuredLoad* pStrLoad,IProgress* pProgress,CBridgeDescription2* pBridgeDesc);
   HRESULT Save(IStructuredSave* pStrSave,IProgress* pProgress);
   
   static LPCTSTR AsString(pgsTypes::TemporarySupportType type);
   static LPCTSTR AsString(pgsTypes::TempSupportSegmentConnectionType type);

#if defined _DEBUG
   void AssertValid();
#endif

protected:
   void MakeCopy(const CTemporarySupportData& rOther,bool bCopyDataOnly);
   void MakeAssignment(const CTemporarySupportData& rOther);

private:
   SupportIDType m_ID;
   SupportIndexType m_Index;
   pgsTypes::TemporarySupportType m_SupportType;
   pgsTypes::TempSupportSegmentConnectionType m_ConnectionType;
   Float64 m_Station;
   std::_tstring m_strOrientation;
   Float64 m_GirderEndDistance;
   ConnectionLibraryEntry::EndDistanceMeasurementType m_EndDistanceMeasurementType;
   Float64 m_GirderBearingOffset;
   ConnectionLibraryEntry::BearingOffsetMeasurementType m_BearingOffsetMeasurementType;

   std::array<Float64, 2> m_PrivateSlabOffset; // This data only exists for historical compatibility. "A" dimension input was removed from PGSplice in 2022

   Float64 m_ElevationAdjustment;

   // Spacing at the centerline of this temporary support
   // This member is invalid if the connection type is sctContinuousSegment
   CGirderSpacing2 m_Spacing;

   // Reference to the span that this temporary support is part of
   CSpanData2* m_pSpan;

   void RemoveFromTimeline();
};
