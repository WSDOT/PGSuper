///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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


   double GetLLDFNegMoment(pgsTypes::LimitState ls,pgsTypes::GirderLocation loc) const;
   void SetLLDFNegMoment(pgsTypes::LimitState ls,pgsTypes::GirderLocation loc,double gM);

   double GetLLDFReaction(pgsTypes::LimitState ls,pgsTypes::GirderLocation loc) const;
   void SetLLDFReaction(pgsTypes::LimitState ls,pgsTypes::GirderLocation loc,double gR);

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

   // LLDF (first index is pgsTypes::GirderLocation, second index is 0 for strength/service, 1 for fatigue limit states)
   double m_gM[2][2]; // negative moment over pier
   double m_gR[2][2]; // reaction at pier
   
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
