///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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
   virtual void MakeAssignment(const pgsStirrupCheckAtZonesArtifactKey& rOther);

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
   // All of girder
   bool GetIsApplicable() const;
   void SetIsApplicable(bool isApp);
   pgsTypes::SplittingDirection GetSplittingDirection() const;
   void SetSplittingDirection(pgsTypes::SplittingDirection sd);
   Float64 GetSplittingZoneLengthFactor() const;
   void SetSplittingZoneLengthFactor(Float64 bzlf);
   bool   Passed() const;

   // Start of girder
   Float64 GetStartH() const;
   void SetStartH(Float64 h);
   Float64 GetStartSplittingZoneLength() const;
   void SetStartSplittingZoneLength(Float64 bzl);
   Float64 GetStartFs() const;
   void SetStartFs(Float64 fs);
   Float64 GetStartAvs() const;
   void SetStartAvs(Float64 avs);
   Float64 GetStartAps() const;
   void SetStartAps(Float64 aps);
   Float64 GetStartFpj() const;
   void SetStartFpj(Float64 fpj);
   Float64 GetStartLossesAfterTransfer() const;
   void SetStartLossesAfterTransfer(Float64 dFpT);
   Float64 GetStartSplittingForce() const;
   Float64 GetStartSplittingResistance() const;
   void SetStartSplittingResistance(Float64 p);
   bool   StartPassed() const;

   // End of girder
   Float64 GetEndH() const;
   void SetEndH(Float64 h);
   Float64 GetEndSplittingZoneLength() const;
   void SetEndSplittingZoneLength(Float64 bzl);
   Float64 GetEndFs() const;
   void SetEndFs(Float64 fs);
   Float64 GetEndAvs() const;
   void SetEndAvs(Float64 avs);
   Float64 GetEndAps() const;
   void SetEndAps(Float64 aps);
   Float64 GetEndFpj() const;
   void SetEndFpj(Float64 fpj);
   Float64 GetEndLossesAfterTransfer() const;
   void SetEndLossesAfterTransfer(Float64 dFpT);
   Float64 GetEndSplittingForce() const;
   Float64 GetEndSplittingResistance() const;
   void SetEndSplittingResistance(Float64 p);
   bool   EndPassed() const;

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
   virtual void MakeAssignment(const pgsSplittingZoneArtifact& rOther);

   // GROUP: ACCESS
   // GROUP: INQUIRY

private:
   // GROUP: DATA MEMBERS
   bool m_IsApplicable;
   pgsTypes::SplittingDirection m_SplittingDirection;
   Float64 m_SplittingZoneLengthFactor;

   Float64 m_StartSplittingZoneLength;
   Float64 m_StartH;
   Float64 m_StartAvs;
   Float64 m_StartAps;
   Float64 m_StartFpj;
   Float64 m_StartdFpT;
   Float64 m_StartFs;
   Float64 m_StartPr; // resistance

   Float64 m_EndSplittingZoneLength;
   Float64 m_EndH;
   Float64 m_EndAvs;
   Float64 m_EndAps;
   Float64 m_EndFpj;
   Float64 m_EnddFpT;
   Float64 m_EndFs;
   Float64 m_EndPr; // resistance

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
   virtual void MakeAssignment(const pgsConfinementArtifact& rOther);

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


COPYRIGHT
   Copyright (c) 1997
   Washington State Department Of Transportation
   All Rights Reserved

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
   virtual void MakeAssignment(const pgsStirrupCheckAtZonesArtifact& rOther);

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
