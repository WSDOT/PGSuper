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

#include <WBFLCore.h>
#include <MathEx.h>
#include <PgsExt\PgsExtExp.h>
#include <StrData.h>
#include <PsgLib\ConnectionLibraryEntry.h>

#include <PgsExt\GirderSpacing2.h>

class CSpanData2;
class CBridgeDescription2;
class CPierData;
class CClosurePourData;

/*****************************************************************************
CLASS 
   CPierData2

   Utility class for pier description data.

DESCRIPTION
   Utility class for pier description data. This class encapsulates all
   the input data for a pier and implements the IStructuredLoad and 
   IStructuredSave persistence interfaces.


COPYRIGHT
   Copyright © 1997-1998
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rab : 08.25.1998 : Created file
*****************************************************************************/

class PGSEXTCLASS CPierData2
{
public:
   CPierData2();
   CPierData2(const CPierData2& rOther); // copies only data, not ID or Index
   ~CPierData2();

   // Assigns pier data from rOther, including the Pier ID and Index
   CPierData2& operator = (const CPierData2& rOther);

   // Copies only the pier definition data from pPier. 
   // Does not alter the ID or Index of this pier
   void CopyPierData(const CPierData2* pPier);

   bool operator==(const CPierData2& rOther) const;
   bool operator!=(const CPierData2& rOther) const;

	HRESULT Load(IStructuredLoad* pStrLoad,IProgress* pProgress);
	HRESULT Save(IStructuredSave* pStrSave,IProgress* pProgress);

   // =================================================================================
   // Methods called by the framework. Don't call these methods directly
   // =================================================================================
   void SetIndex(PierIndexType pierIdx);
   void SetID(PierIDType pierID);
   void SetBridgeDescription(CBridgeDescription2* pBridge);
   void SetSpans(CSpanData2* pPrevSpan,CSpanData2* pNextSpan);

   // configures this pier with data from the old CPierData object
   void SetPierData(CPierData* pPier);

   // =================================================================================
   // Miscellaneous
   // =================================================================================

   // Returns a consistent string name for a pier connection type.
   static LPCTSTR AsString(pgsTypes::PierConnectionType type);

   bool IsContinuous() const;
   void IsIntegral(bool* pbLeft,bool* pbRight) const;

   bool IsAbutment() const; // returns true if pier is an end abutment
   bool IsPier() const; // returns true if pier is an intermediate pier


   // =================================================================================
   // Configuration information
   // =================================================================================

   // returns the pier index
   PierIndexType GetIndex() const;
   PierIDType GetID() const;
   
   // returns the bridge this pier is a part of
   CBridgeDescription2* GetBridgeDescription();
   const CBridgeDescription2* GetBridgeDescription() const;

   // returns the spans that connect to this pier
   CSpanData2* GetPrevSpan();
   CSpanData2* GetNextSpan();
   CSpanData2* GetSpan(pgsTypes::PierFaceType face);

   const CSpanData2* GetPrevSpan() const;
   const CSpanData2* GetNextSpan() const;
   const CSpanData2* GetSpan(pgsTypes::PierFaceType face) const;

   // returns the girder groups that connect to this pier
   CGirderGroupData* GetPrevGirderGroup();
   CGirderGroupData* GetNextGirderGroup();
   CGirderGroupData* GetGirderGroup(pgsTypes::PierFaceType face);

   const CGirderGroupData* GetPrevGirderGroup() const;
   const CGirderGroupData* GetNextGirderGroup() const;
   const CGirderGroupData* GetGirderGroup(pgsTypes::PierFaceType face) const;

   // Returns the location of this pier
   Float64 GetStation() const;

   // Sets the location for this pier (does not change the station of subsequent piers)
   void SetStation(Float64 station);

   // Sets/Get the orientation of the pier as an input string (bearing "N 35 23 15 E" or skew angle "15 01 14 L")
   LPCTSTR GetOrientation() const;
   void SetOrientation(LPCTSTR strOrientation);

   // Set/Get the connection type at the pier (boundary condition)
   pgsTypes::PierConnectionType GetConnectionType() const;
   void SetConnectionType(pgsTypes::PierConnectionType type);

   // Set/Get the distance from the CL bearing to the end of the girders at this pier
   // These parameters are meaningless if the connection type is pgsTypes::ContinuousSegment
   void SetGirderEndDistance(pgsTypes::PierFaceType face,Float64 endDist,ConnectionLibraryEntry::EndDistanceMeasurementType measure);
   void GetGirderEndDistance(pgsTypes::PierFaceType face,Float64* pEndDist,ConnectionLibraryEntry::EndDistanceMeasurementType* pMeasure) const;

   // Set/Get the distance from the CL pier (defined by its station) to the CL bearing
   // These parameters are meaningless if the connection type is pgsTypes::ContinuousSegment
   void SetBearingOffset(pgsTypes::PierFaceType face,Float64 offset,ConnectionLibraryEntry::BearingOffsetMeasurementType measure);
   void GetBearingOffset(pgsTypes::PierFaceType face,Float64* pOffset,ConnectionLibraryEntry::BearingOffsetMeasurementType* pMeasure) const;

   // Set/Get the support width at this pier
   void SetSupportWidth(pgsTypes::PierFaceType face,Float64 w);
   Float64 GetSupportWidth(pgsTypes::PierFaceType face) const;

   // Set/Get the girder spacing at this pier
   void SetGirderSpacing(pgsTypes::PierFaceType pierFace,const CGirderSpacing2& spacing);
   CGirderSpacing2* GetGirderSpacing(pgsTypes::PierFaceType pierFace);
   const CGirderSpacing2* GetGirderSpacing(pgsTypes::PierFaceType pierFace) const;

   // Get a closure pour associated with this pier
   const CClosurePourData* GetClosurePour(GirderIndexType gdrIdx) const;

   // =================================================================================
   // Diaphragm
   // =================================================================================

   void SetDiaphragmHeight(pgsTypes::PierFaceType pierFace,Float64 d);
   Float64 GetDiaphragmHeight(pgsTypes::PierFaceType pierFace) const;
   void SetDiaphragmWidth(pgsTypes::PierFaceType pierFace,Float64 w);
   Float64 GetDiaphragmWidth(pgsTypes::PierFaceType pierFace)const;
   ConnectionLibraryEntry::DiaphragmLoadType GetDiaphragmLoadType(pgsTypes::PierFaceType pierFace) const;
   void SetDiaphragmLoadType(pgsTypes::PierFaceType pierFace,ConnectionLibraryEntry::DiaphragmLoadType type);
   Float64 GetDiaphragmLoadLocation(pgsTypes::PierFaceType pierFace) const;
   void SetDiaphragmLoadLocation(pgsTypes::PierFaceType pierFace,Float64 loc);


   // =================================================================================
   // Live load distribution factors (neg moment and reactions at this pier)
   // =================================================================================
   Float64 GetLLDFNegMoment(GirderIndexType gdrIdx, pgsTypes::LimitState ls) const;
   void SetLLDFNegMoment(GirderIndexType gdrIdx, pgsTypes::LimitState ls, Float64 gM);
   void SetLLDFNegMoment(pgsTypes::GirderLocation gdrloc, pgsTypes::LimitState ls, Float64 gM);

   Float64 GetLLDFReaction(GirderIndexType gdrIdx, pgsTypes::LimitState ls) const;
   void SetLLDFReaction(GirderIndexType gdrIdx, pgsTypes::LimitState ls,Float64 gR);
   void SetLLDFReaction(pgsTypes::GirderLocation gdrloc, pgsTypes::LimitState ls,Float64 gR);

#if defined _DEBUG
   void AssertValid() const;
#endif

protected:
   //------------------------------------------------------------------------
   void MakeCopy(const CPierData2& rOther,bool bCopyDataOnly);

   //------------------------------------------------------------------------
   virtual void MakeAssignment(const CPierData2& rOther);

private:
   typedef enum PierOrientation { Normal, Skew, Bearing } PierOrientation;
   PierOrientation Orientation;
   Float64 Angle;

   PierIndexType m_PierIdx;
   PierIDType m_PierID;

   Float64 m_Station;
   std::_tstring m_strOrientation;
   pgsTypes::PierConnectionType m_ConnectionType;

   Float64 m_GirderEndDistance[2];
   ConnectionLibraryEntry::EndDistanceMeasurementType m_EndDistanceMeasurementType[2];
   Float64 m_GirderBearingOffset[2];
   ConnectionLibraryEntry::BearingOffsetMeasurementType m_BearingOffsetMeasurementType[2];
   Float64 m_SupportWidth[2];

   CGirderSpacing2 m_GirderSpacing[2]; // index is pgsTypes::PierFaceType

   Float64 m_DiaphragmHeight[2];
   Float64 m_DiaphragmWidth[2];
   ConnectionLibraryEntry::DiaphragmLoadType m_DiaphragmLoadType[2];
   Float64 m_DiaphragmLoadLocation[2];

   CSpanData2* m_pPrevSpan;
   CSpanData2* m_pNextSpan;
   CBridgeDescription2* m_pBridgeDesc;


   // LLDF
   // 0 for strength/service limit state, 1 for fatigue limit state
   struct LLDF
   {
      Float64 gM[2];
      Float64 gR[2];

      LLDF()
      {
         gM[0]=1.0; gM[1]=1.0; gR[0]=1.0; gR[1]=1.0;
      }

      bool operator==(const LLDF& rOther) const
      {
         return IsEqual(gM[0], rOther.gM[0]) && IsEqual(gM[1], rOther.gM[1]) &&
                IsEqual( gR[0], rOther.gR[0])  && IsEqual( gR[1], rOther.gR[1]);
      }
   };

   mutable std::vector<LLDF> m_LLDFs; // this is mutable because we may have to change the container size on Get functions

   mutable bool m_bDistributionFactorsFromOlderVersion; // If this is true, we need to do some processing into the new format

   // safe internal function for getting lldfs in lieue of girder count changes
   LLDF& GetLLDF(GirderIndexType gdrIdx) const;

   GirderIndexType GetLldfGirderCount() const;

   HRESULT LoadOldPierData(Float64 version,IStructuredLoad* pStrLoad,IProgress* pProgress,const std::_tstring& strUnitName);

   void ChangeConnectionType(pgsTypes::PierConnectionType connectionType);

   friend CBridgeDescription2;
};

//////////////////////////////////////////////////////////////////////////
// STL Predicate Objects
class PGSEXTCLASS PierLess2
{
public:
   bool operator()(const CPierData2& a, const CPierData2& b)
   {
      return a.GetStation() < b.GetStation();
   }
};

class PGSEXTCLASS FindPier2
{
public:
   FindPier2(const CPierData2& pd) : PierData( pd ) {}
   bool operator()(const CPierData2& b)
   {
      return IsEqual( PierData.GetStation(), b.GetStation() );
   }
private:
   CPierData2 PierData;
};
