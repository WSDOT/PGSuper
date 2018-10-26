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

#if !defined INCLUDED_PGSEXT_STIRRUPCHECKATZONESARTIFACT_H_
#define INCLUDED_PGSEXT_STIRRUPCHECKATZONESARTIFACT_H_

// SYSTEM INCLUDES
//

// PROJECT INCLUDES
//
#if !defined INCLUDED_PGSEXTEXP_H_
#include <PgsExt\PgsExtExp.h>
#endif

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


COPYRIGHT
   Copyright © 1997-1998
   Washington State Department Of Transportation
   All Rights Reserved

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
   pgsStirrupCheckAtZonesArtifactKey(Uint32 zoneNum);

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
   Uint32 m_ZoneNum;

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


COPYRIGHT
   Copyright (c) 1997
   Washington State Department Of Transportation
   All Rights Reserved

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
   bool GetIsApplicable() const;
   void SetIsApplicable(bool isApp);
   Float64 GetH() const;
   void SetH(Float64 h);
   pgsTypes::SplittingDirection GetSplittingDirection() const;
   void SetSplittingDirection(pgsTypes::SplittingDirection sd);
   Float64 GetSplittingZoneLength() const;
   void SetSplittingZoneLength(Float64 bzl);
   Float64 GetSplittingZoneLengthFactor() const;
   void SetSplittingZoneLengthFactor(Float64 bzlf);
   Float64 GetFs() const;
   void SetFs(Float64 fs);
   Float64 GetAvs() const;
   void SetAvs(Float64 avs);
   Float64 GetAps() const;
   void SetAps(Float64 aps);
   Float64 GetFpj() const;
   void SetFpj(Float64 fpj);
   Float64 GetLossesAfterTransfer() const;
   void SetLossesAfterTransfer(double dFpT);
   Float64 GetSplittingForce() const;
   Float64 GetSplittingResistance() const;
   void SetSplittingResistance(Float64 p);
   bool   Passed() const;

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
   Float64 m_SplittingZoneLength;
   Float64 m_SplittingZoneLengthFactor;
   Float64 m_H;
   Float64 m_Avs;
   Float64 m_Aps;
   Float64 m_Fpj;
   Float64 m_dFpT;
   Float64 m_Fs;
   Float64 m_Pr; // resistance
   pgsTypes::SplittingDirection m_SplittingDirection;

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


COPYRIGHT
   Copyright (c) 1997
   Washington State Department Of Transportation
   All Rights Reserved

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
   Float64 GetApplicableZoneLength() const;
   void SetApplicableZoneLength(Float64 zl);
   Float64 GetZoneEnd() const;
   void SetZoneEnd(Float64 zl);
   const matRebar* GetBar() const;
   void SetBar(const matRebar* pRebar);
   Float64 GetS() const;
   void SetS(Float64 s);
   const matRebar* GetMinBar() const;
   void SetMinBar(const matRebar* pBar);
   Float64 GetSMax() const;
   void SetSMax(Float64 smax);

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
   Float64 m_ApplicableZoneLength;
   Float64 m_ZoneEnd;
   const matRebar* m_pRebar;
   Float64 m_S;
   const matRebar* m_pMinRebar;
   Float64 m_SMax;
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


COPYRIGHT
   Copyright (c) 1997
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rab : 12.29.1997 : Created file
*****************************************************************************/

class PGSEXTCLASS pgsStirrupCheckAtZonesArtifact
{
public:
   // GROUP: DATA MEMBERS

   // vertical shear check results
   void SetConfinementArtifact(const pgsConfinementArtifact& artifact);
   const pgsConfinementArtifact* GetConfinementArtifact() const;

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
   void MakeCopy(const pgsStirrupCheckAtZonesArtifact& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const pgsStirrupCheckAtZonesArtifact& rOther);

   // GROUP: ACCESS
   // GROUP: INQUIRY

private:
   // GROUP: DATA MEMBERS
   pgsConfinementArtifact  m_ConfinementArtifact;
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


#endif // 
