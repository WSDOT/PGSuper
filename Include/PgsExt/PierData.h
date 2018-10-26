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

#ifndef INCLUDED_PGSEXT_PIERDATA_H_
#define INCLUDED_PGSEXT_PIERDATA_H_

// SYSTEM INCLUDES
//
#include <WBFLCore.h>

#if !defined INCLUDED_MATHEX_H_
#include <MathEx.h>
#endif

// PROJECT INCLUDES
//
#if !defined INCLUDED_PGSEXTEXP_H_
#include <PgsExt\PgsExtExp.h>
#endif

#include <StrData.h>

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//
class ConnectionLibraryEntry;
class CSpanData;
class CBridgeDescription;

// MISCELLANEOUS
//

/*****************************************************************************
CLASS 
   CPierData

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

class PGSEXTCLASS CPierData
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

	HRESULT Load(IStructuredLoad* pStrLoad,IProgress* pProgress);
	HRESULT Save(IStructuredSave* pStrSave,IProgress* pProgress);

   static LPCTSTR AsString(pgsTypes::PierConnectionType type);

   void SetPierIndex(PierIndexType pierIdx);
   PierIndexType GetPierIndex() const;
   
   void SetBridgeDescription(const CBridgeDescription* pBridge);
   const CBridgeDescription* GetBridgeDescription() const;

   void SetSpans(CSpanData* pPrevSpan,CSpanData* pNextSpan);
   CSpanData* GetPrevSpan();
   CSpanData* GetNextSpan();
   CSpanData* GetSpan(pgsTypes::PierFaceType face);

   const CSpanData* GetPrevSpan() const;
   const CSpanData* GetNextSpan() const;
   const CSpanData* GetSpan(pgsTypes::PierFaceType face) const;

   double GetStation() const;
   void SetStation(double station);

   LPCTSTR GetOrientation() const;
   void SetOrientation(LPCTSTR strOrientation);

   pgsTypes::PierConnectionType GetConnectionType() const;
   void SetConnectionType(pgsTypes::PierConnectionType type);

   LPCTSTR GetConnection(pgsTypes::PierFaceType pierFace) const;
   void SetConnection(pgsTypes::PierFaceType pierFace,LPCTSTR strConnection);

   const ConnectionLibraryEntry* GetConnectionLibraryEntry(pgsTypes::PierFaceType pierFace) const;
   void SetConnectionLibraryEntry(pgsTypes::PierFaceType pierFace,const ConnectionLibraryEntry* pLibEntry);

   double GetLLDFNegMoment(GirderIndexType gdrIdx, pgsTypes::LimitState ls) const;
   void SetLLDFNegMoment(GirderIndexType gdrIdx, pgsTypes::LimitState ls, double gM);
   void SetLLDFNegMoment(pgsTypes::GirderLocation gdrloc, pgsTypes::LimitState ls, double gM);

   double GetLLDFReaction(GirderIndexType gdrIdx, pgsTypes::LimitState ls) const;
   void SetLLDFReaction(GirderIndexType gdrIdx, pgsTypes::LimitState ls,double gR);
   void SetLLDFReaction(pgsTypes::GirderLocation gdrloc, pgsTypes::LimitState ls,double gR);

   bool IsContinuous() const;
   void IsIntegral(bool* pbLeft,bool* pbRight) const;

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
   double m_Station;
   std::_tstring m_strOrientation;
   pgsTypes::PierConnectionType m_ConnectionType;
   std::_tstring m_Connection[2];
   const ConnectionLibraryEntry* m_pConnectionEntry[2];

   CSpanData* m_pPrevSpan;
   CSpanData* m_pNextSpan;
   const CBridgeDescription* m_pBridgeDesc;

   // LLDF
   // 0 for strength/service limit state, 1 for fatigue limit state
   struct LLDF
   {
      double gM[2];
      double gR[2];

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
};

// INLINE METHODS
//

// EXTERNAL REFERENCES
//

//////////////////////////////////////////////////////////////////////////
// STL Predicate Objects
class PGSEXTCLASS PierLess
{
public:
   bool operator()(const CPierData& a, const CPierData& b)
   {
      return a.GetStation() < b.GetStation();
   }
};

class PGSEXTCLASS FindPier
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

#endif // INCLUDED_PGSEXT_PIERDATA_H_
