///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2021  Washington State Department of Transportation
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

#if !defined INCLUDED_PGSEXT_STIRRUPCHECKATZONESARTIFACT_H_
#define INCLUDED_PGSEXT_STIRRUPCHECKATZONESARTIFACT_H_

// SYSTEM INCLUDES
//

// PROJECT INCLUDES
//
#if !defined INCLUDED_PGSEXTEXP_H_
#include <PgsExt\PgsExtExp.h>
#endif

#include <array>

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//

// MISCELLANEOUS
//


/*****************************************************************************
CLASS 
   pgsStirrupCheckAtZonesArtifactKey

   A lookup key for shear zone-based artifacts


DESCRIPTION
   A lookup key for shear zone-based artifacts

LOG
   rdp : 1.7.1999 : Created file
*****************************************************************************/

class PGSEXTCLASS pgsStirrupCheckAtZonesArtifactKey
{
public:
   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   pgsStirrupCheckAtZonesArtifactKey();

   //------------------------------------------------------------------------
   pgsStirrupCheckAtZonesArtifactKey(ZoneIndexType zoneNum);

   //------------------------------------------------------------------------
   // Copy constructor
   pgsStirrupCheckAtZonesArtifactKey(const pgsStirrupCheckAtZonesArtifactKey& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~pgsStirrupCheckAtZonesArtifactKey();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   pgsStirrupCheckAtZonesArtifactKey& operator = (const pgsStirrupCheckAtZonesArtifactKey& rOther);

   bool operator<(const pgsStirrupCheckAtZonesArtifactKey& rOther) const;

   // GROUP: OPERATIONS

   // GROUP: ACCESS

   // GROUP: INQUIRY

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   //------------------------------------------------------------------------
   void MakeCopy(const pgsStirrupCheckAtZonesArtifactKey& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const pgsStirrupCheckAtZonesArtifactKey& rOther);

   // GROUP: ACCESS
   // GROUP: INQUIRY

private:
   // GROUP: DATA MEMBERS
   ZoneIndexType m_ZoneNum;

   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   // GROUP: ACCESS
   // GROUP: INQUIRY

public:
   // GROUP: DEBUG
   #if defined _DEBUG
   //------------------------------------------------------------------------
   // Returns true if the object is in a valid state, otherwise returns false.
   virtual bool AssertValid() const;

   //------------------------------------------------------------------------
   // Dumps the contents of the object to the given dump context.
   virtual void Dump(dbgDumpContext& os) const;
   #endif // _DEBUG

   #if defined _UNITTEST
   //------------------------------------------------------------------------
   // Runs a self-diagnostic test.  Returns true if the test passed,
   // otherwise false.
   static bool TestMe(dbgLog& rlog);
   #endif // _UNITTEST
};

// INLINE METHODS
//

// EXTERNAL REFERENCES
//


/*****************************************************************************
CLASS 
   pgsSplittingZoneArtifact

   Artifact that holds shear design/check results in the burssting zone.


DESCRIPTION
   Artifact that holds shear design/check results in the Splitting zone.

LOG
   rab : 12.29.1997 : Created file
*****************************************************************************/

class PGSEXTCLASS pgsSplittingZoneArtifact
{
public:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   pgsSplittingZoneArtifact();

   //------------------------------------------------------------------------
   // Copy constructor
   pgsSplittingZoneArtifact(const pgsSplittingZoneArtifact& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~pgsSplittingZoneArtifact();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   pgsSplittingZoneArtifact& operator = (const pgsSplittingZoneArtifact& rOther);

   // GROUP: OPERATIONS
   // All of girder
   bool GetIsApplicable() const;
   void SetIsApplicable(bool isApp);
   pgsTypes::SplittingDirection GetSplittingDirection() const;
   void SetSplittingDirection(pgsTypes::SplittingDirection sd);
   Float64 GetSplittingZoneLengthFactor() const;
   void SetSplittingZoneLengthFactor(Float64 bzlf);
   bool   Passed() const;

   Float64 GetH(pgsTypes::MemberEndType endType) const;
   void SetH(pgsTypes::MemberEndType endType,Float64 h);
   Float64 GetShearWidth(pgsTypes::MemberEndType endType) const;
   void SetShearWidth(pgsTypes::MemberEndType endType, Float64 bv);
   Float64 GetSplittingZoneLength(pgsTypes::MemberEndType endType) const;
   void SetSplittingZoneLength(pgsTypes::MemberEndType endType,Float64 bzl);
   Float64 GetFs(pgsTypes::MemberEndType endType) const;
   void SetFs(pgsTypes::MemberEndType endType,Float64 fs);
   Float64 GetAvs(pgsTypes::MemberEndType endType) const;
   void SetAvs(pgsTypes::MemberEndType endType,Float64 avs);
   Float64 GetAps(pgsTypes::MemberEndType endType,pgsTypes::StrandType strandType) const;
   void SetAps(pgsTypes::MemberEndType endType,pgsTypes::StrandType strandType, Float64 aps);
   Float64 GetFpj(pgsTypes::MemberEndType endType,pgsTypes::StrandType strandType) const;
   void SetFpj(pgsTypes::MemberEndType endType,pgsTypes::StrandType strandType, Float64 fpj);
   Float64 GetLossesAfterTransfer(pgsTypes::MemberEndType endType,pgsTypes::StrandType strandType) const;
   void SetLossesAfterTransfer(pgsTypes::MemberEndType endType,pgsTypes::StrandType strandType, Float64 dFpT);
   Float64 GetSplittingForce(pgsTypes::MemberEndType endType,pgsTypes::StrandType strandType) const;
   Float64 GetTotalSplittingForce(pgsTypes::MemberEndType endType) const;
   Float64 GetSplittingResistance(pgsTypes::MemberEndType endType) const;
   void SetSplittingResistance(pgsTypes::MemberEndType endType,Float64 p);
   Float64 GetUHPCStrengthAtFirstCrack() const;
   void SetUHPCStrengthAtFirstCrack(Float64 f1);
   bool Passed(pgsTypes::MemberEndType end) const;

   // GROUP: ACCESS
   // GROUP: INQUIRY

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   //------------------------------------------------------------------------
   void MakeCopy(const pgsSplittingZoneArtifact& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const pgsSplittingZoneArtifact& rOther);

   // GROUP: ACCESS
   // GROUP: INQUIRY

private:
   // GROUP: DATA MEMBERS
   bool m_IsApplicable;
   pgsTypes::SplittingDirection m_SplittingDirection;
   Float64 m_SplittingZoneLengthFactor;
   Float64 m_f1;

   // array index is pgsTypes::MemberEndType
   std::array<Float64, 2> m_SplittingZoneLength;
   std::array<Float64, 2> m_H;
   std::array<Float64, 2> m_bv;
   std::array<Float64, 2> m_Avs;
   std::array<std::array<Float64, 3>, 2> m_Aps;  //[endType][strandType]
   std::array<std::array<Float64, 3>, 2> m_Fpj; //[endType][strandType]
   std::array<std::array<Float64, 3>, 2> m_dFpT; //[endType][strandType]
   std::array<Float64, 2> m_Fs;
   std::array<Float64, 2> m_Pr; // resistance

   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   // GROUP: ACCESS
   // GROUP: INQUIRY
};

// INLINE METHODS
//

// EXTERNAL REFERENCES
//

/*****************************************************************************
CLASS 
   pgsConfinementArtifact

   Artifact that holds shear design/check results in the burssting zone.


DESCRIPTION
   Artifact that holds shear design/check results in the Splitting zone.

LOG
   rab : 12.29.1997 : Created file
*****************************************************************************/

class PGSEXTCLASS pgsConfinementArtifact
{
public:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   pgsConfinementArtifact();

   //------------------------------------------------------------------------
   // Copy constructor
   pgsConfinementArtifact(const pgsConfinementArtifact& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~pgsConfinementArtifact();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   pgsConfinementArtifact& operator = (const pgsConfinementArtifact& rOther);

   // GROUP: OPERATIONS
   // if not applicable, the rest of the values are undefined
   bool IsApplicable() const;
   void SetApplicability(bool isAp);

   const matRebar* GetMinBar() const;
   void SetMinBar(const matRebar* pBar);
   Float64 GetSMax() const;
   void SetSMax(Float64 smax);

   // Factor multiplied by d (1.5)
   Float64 GetZoneLengthFactor() const;
   void SetZoneLengthFactor(Float64 fac);

   Float64 GetStartProvidedZoneLength() const;
   void SetStartProvidedZoneLength(Float64 zl);
   Float64 GetStartRequiredZoneLength() const;
   void SetStartRequiredZoneLength(Float64 zl);
   const matRebar* GetStartBar() const;
   void SetStartBar(const matRebar* pRebar);
   Float64 GetStartS() const;
   void SetStartS(Float64 s);
   Float64 GetStartd() const; // beam depth
   void SetStartd(Float64 d);

   bool   StartPassed() const;

   Float64 GetEndProvidedZoneLength() const;
   void SetEndProvidedZoneLength(Float64 zl);
   Float64 GetEndRequiredZoneLength() const;
   void SetEndRequiredZoneLength(Float64 zl);
   const matRebar* GetEndBar() const;
   void SetEndBar(const matRebar* pRebar);
   Float64 GetEndS() const;
   void SetEndS(Float64 s);
   Float64 GetEndd() const;
   void SetEndd(Float64 d);
   bool   EndPassed() const;

   bool Passed() const;

   // GROUP: ACCESS
   // GROUP: INQUIRY
   // GROUP: DEBUG
   #if defined _DEBUG
   //------------------------------------------------------------------------
   // Returns <b>true</b> if the class is in a valid state, otherwise returns
   // <b>false</b>.
   virtual bool AssertValid() const;

   //------------------------------------------------------------------------
   // Dumps the contents of the class to the given stream.
   virtual void Dump(dbgDumpContext& os) const;
   #endif // _DEBUG

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   //------------------------------------------------------------------------
   void MakeCopy(const pgsConfinementArtifact& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const pgsConfinementArtifact& rOther);

   // GROUP: ACCESS
   // GROUP: INQUIRY

private:
   // GROUP: DATA MEMBERS
   bool m_IsApplicable;

   const matRebar* m_pMinRebar;
   Float64 m_SMax;

   Float64 m_ZoneLengthFactor;

   Float64 m_StartProvidedZoneLength;
   Float64 m_StartRequiredZoneLength;
   const matRebar* m_pStartRebar;
   Float64 m_StartS;
   Float64 m_Startd;

   Float64 m_EndProvidedZoneLength;
   Float64 m_EndRequiredZoneLength;
   const matRebar* m_pEndRebar;
   Float64 m_EndS;
   Float64 m_Endd;

   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   // GROUP: ACCESS
   // GROUP: INQUIRY
};

// INLINE METHODS
//

// EXTERNAL REFERENCES
//


/*****************************************************************************
CLASS 
   pgsStirrupCheckAtZonesArtifact

   Artifact that holds shear design/check results.


DESCRIPTION
   Artifact that holds shear design/check results.

LOG
   rab : 12.29.1997 : Created file
*****************************************************************************/

// Deleted this class on 5/4/11 - rdp
/*

class PGSEXTCLASS pgsStirrupCheckAtZonesArtifact
{
public:
   // GROUP: DATA MEMBERS

   // vertical shear check results

   bool   Passed() const;

   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   pgsStirrupCheckAtZonesArtifact();

   //------------------------------------------------------------------------
   // Copy constructor
   pgsStirrupCheckAtZonesArtifact(const pgsStirrupCheckAtZonesArtifact& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~pgsStirrupCheckAtZonesArtifact();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   pgsStirrupCheckAtZonesArtifact& operator = (const pgsStirrupCheckAtZonesArtifact& rOther);

   // GROUP: OPERATIONS
   // GROUP: ACCESS
   // GROUP: INQUIRY
   // GROUP: DEBUG
   #if defined _DEBUG
   //------------------------------------------------------------------------
   // Returns <b>true</b> if the class is in a valid state, otherwise returns
   // <b>false</b>.
   virtual bool AssertValid() const override;

   //------------------------------------------------------------------------
   // Dumps the contents of the class to the given stream.
   virtual void Dump(dbgDumpContext& os) const override;
   #endif // _DEBUG

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   //------------------------------------------------------------------------
   void MakeCopy(const pgsStirrupCheckAtZonesArtifact& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const pgsStirrupCheckAtZonesArtifact& rOther);

   // GROUP: ACCESS
   // GROUP: INQUIRY

private:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   // GROUP: ACCESS
   // GROUP: INQUIRY
};

// INLINE METHODS
//

// EXTERNAL REFERENCES
//
*/

#endif // 
