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
#pragma once

#include <WBFLCore.h>

#include <MathEx.h>

#include <StrData.h>
#include <PsgLib\ConnectionLibraryEntry.h>
#include <PgsExt\PierData2.h>

class CSpanData;
class CBridgeDescription;

///////////////////////////////////////////////////////
// NOTE: 
// This class only exists to load old PGSuper files.
///////////////////////////////////////////////////////

/*****************************************************************************
CLASS 
   CPierData

   Utility class for pier description data.

DESCRIPTION
   Utility class for pier description data. This class encapsulates all
   the input data for a pier and implements the IStructuredLoad and 
   IStructuredSave persistence interfaces.

LOG
   rab : 08.25.1998 : Created file
*****************************************************************************/

class CPierData
{
public:
   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   CPierData();

   //------------------------------------------------------------------------
   // Copy constructor
   CPierData(const CPierData& rOther);

   //------------------------------------------------------------------------
   // Destructor
   ~CPierData();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   CPierData& operator = (const CPierData& rOther);
   bool operator==(const CPierData& rOther) const;
   bool operator!=(const CPierData& rOther) const;

   // GROUP: OPERATIONS

   HRESULT Load(Float64 version,IStructuredLoad* pStrLoad,IProgress* pProgress,const std::_tstring& strUnitName);
	HRESULT Load(IStructuredLoad* pStrLoad,IProgress* pProgress);
	HRESULT Save(IStructuredSave* pStrSave,IProgress* pProgress);

   static LPCTSTR AsString(pgsTypes::BoundaryConditionType type);

   void SetPierIndex(PierIndexType pierIdx);
   PierIndexType GetPierIndex() const;
   
   void SetBridgeDescription(CBridgeDescription* pBridge);
   CBridgeDescription* GetBridgeDescription();
   const CBridgeDescription* GetBridgeDescription() const;

   void SetSpans(CSpanData* pPrevSpan,CSpanData* pNextSpan);
   CSpanData* GetPrevSpan();
   CSpanData* GetNextSpan();
   CSpanData* GetSpan(pgsTypes::PierFaceType face);

   const CSpanData* GetPrevSpan() const;
   const CSpanData* GetNextSpan() const;
   const CSpanData* GetSpan(pgsTypes::PierFaceType face) const;

   Float64 GetStation() const;
   void SetStation(Float64 station);

   LPCTSTR GetOrientation() const;
   void SetOrientation(LPCTSTR strOrientation);

   pgsTypes::BoundaryConditionType GetConnectionType() const;
   void SetConnectionType(pgsTypes::BoundaryConditionType type);

   void SetGirderEndDistance(pgsTypes::PierFaceType face,Float64 endDist,ConnectionLibraryEntry::EndDistanceMeasurementType measure);
   void GetGirderEndDistance(pgsTypes::PierFaceType face,Float64* pEndDist,ConnectionLibraryEntry::EndDistanceMeasurementType* pMeasure) const;

   void SetBearingOffset(pgsTypes::PierFaceType face,Float64 offset,ConnectionLibraryEntry::BearingOffsetMeasurementType measure);
   void GetBearingOffset(pgsTypes::PierFaceType face,Float64* pOffset,ConnectionLibraryEntry::BearingOffsetMeasurementType* pMeasure) const;

   void SetSupportWidth(pgsTypes::PierFaceType face,Float64 w);
   Float64 GetSupportWidth(pgsTypes::PierFaceType face) const;

   void SetDiaphragmHeight(pgsTypes::PierFaceType face,Float64 d);
   Float64 GetDiaphragmHeight(pgsTypes::PierFaceType face) const;
   void SetDiaphragmWidth(pgsTypes::PierFaceType face,Float64 s);
   Float64 GetDiaphragmWidth(pgsTypes::PierFaceType face)const;
   ConnectionLibraryEntry::DiaphragmLoadType GetDiaphragmLoadType(pgsTypes::PierFaceType face) const;
   void SetDiaphragmLoadType(pgsTypes::PierFaceType face,ConnectionLibraryEntry::DiaphragmLoadType type);
   Float64 GetDiaphragmLoadLocation(pgsTypes::PierFaceType face) const;
   void SetDiaphragmLoadLocation(pgsTypes::PierFaceType face,Float64 loc);

   Float64 GetLLDFNegMoment(GirderIndexType gdrIdx, pgsTypes::LimitState ls) const;
   void SetLLDFNegMoment(GirderIndexType gdrIdx, pgsTypes::LimitState ls, Float64 gM);
   void SetLLDFNegMoment(pgsTypes::GirderLocation gdrloc, pgsTypes::LimitState ls, Float64 gM);

   Float64 GetLLDFReaction(GirderIndexType gdrIdx, pgsTypes::LimitState ls) const;
   void SetLLDFReaction(GirderIndexType gdrIdx, pgsTypes::LimitState ls,Float64 gR);
   void SetLLDFReaction(pgsTypes::GirderLocation gdrloc, pgsTypes::LimitState ls,Float64 gR);


   bool IsContinuous() const;
   void IsIntegral(bool* pbLeft,bool* pbRight) const;

   bool IsAbutment() const; // returns true if pier is an end abutment
   bool IsPier() const; // returns true if pier is an intermediate pier

   // GROUP: ACCESS
   // GROUP: INQUIRY

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   //------------------------------------------------------------------------
   void MakeCopy(const CPierData& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const CPierData& rOther);

   // GROUP: ACCESS
   // GROUP: INQUIRY

private:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   // GROUP: ACCESS
   // GROUP: INQUIRY
   typedef enum PierOrientation { Normal, Skew, Bearing } PierOrientation;
   PierOrientation Orientation;
   Float64 Angle;

   PierIndexType m_PierIdx;
   Float64 m_Station;
   std::_tstring m_strOrientation;
   pgsTypes::BoundaryConditionType m_ConnectionType;

   Float64 m_GirderEndDistance[2];
   ConnectionLibraryEntry::EndDistanceMeasurementType m_EndDistanceMeasurementType[2];
   Float64 m_GirderBearingOffset[2];
   ConnectionLibraryEntry::BearingOffsetMeasurementType m_BearingOffsetMeasurementType[2];
   Float64 m_SupportWidth[2];

   Float64 m_DiaphragmHeight[2];
   Float64 m_DiaphragmWidth[2];
   ConnectionLibraryEntry::DiaphragmLoadType m_DiaphragmLoadType[2];
   Float64 m_DiaphragmLoadLocation[2];

   CSpanData* m_pPrevSpan;
   CSpanData* m_pNextSpan;
   CBridgeDescription* m_pBridgeDesc;

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

   mutable bool m_DistributionFactorsFromOlderVersion; // If this is true, we need to do some processing into the new format

   // safe internal function for getting lldfs in lieue of girder count changes
   LLDF& GetLLDF(GirderIndexType igs) const;

   GirderIndexType GetLldfGirderCount() const;
   
   DECLARE_STRSTORAGEMAP(CPierData)

   friend CPierData2; 
};

// INLINE METHODS
//

// EXTERNAL REFERENCES
//

//////////////////////////////////////////////////////////////////////////
// STL Predicate Objects
class PierLess
{
public:
   bool operator()(const CPierData& a, const CPierData& b)
   {
      return a.GetStation() < b.GetStation();
   }
};

class FindPier
{
public:
   FindPier(const CPierData& pd) : PierData( pd ) {}
   bool operator()(const CPierData& b)
   {
      return IsEqual( PierData.GetStation(), b.GetStation() );
   }
private:
   CPierData PierData;
};
