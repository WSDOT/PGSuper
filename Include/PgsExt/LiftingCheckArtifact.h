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

#ifndef INCLUDED_PGSEXT_LIFTINGCHECKARTIFACT_H_
#define INCLUDED_PGSEXT_LIFTINGCHECKARTIFACT_H_

// SYSTEM INCLUDES
//

// PROJECT INCLUDES
//
#if !defined INCLUDED_PGSEXTEXP_H_
#include <PgsExt\PgsExtExp.h>
#endif

#include <PgsExt\LiftingAnalysisArtifact.h>

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//
class pgsLiftingCheckArtifact;

// MISCELLANEOUS
//


/*****************************************************************************
CLASS 
   pgsLiftingStressCheckArtifact

   Artifact that holds lifting stress check results at a location.


DESCRIPTION
   Artifact that holds lifting stress check results at a location.


COPYRIGHT
   Copyright (c) 1999
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rdp : 03.26.1999 : Created file
*****************************************************************************/

class PGSEXTCLASS pgsLiftingStressCheckArtifact : public pgsLiftingStressAnalysisArtifact
{
public:
   // GROUP: DATA MEMBERS

   bool   Passed() const;
   bool   CompressionPassed() const;
   bool   TensionPassed() const;
   bool   AlternativeTensionPassed() const;
   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // constructor
   pgsLiftingStressCheckArtifact(const pgsLiftingCheckArtifact& rParent);

   //------------------------------------------------------------------------
   // Copy constructor
   pgsLiftingStressCheckArtifact(const pgsLiftingStressCheckArtifact& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~pgsLiftingStressCheckArtifact();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   pgsLiftingStressCheckArtifact& operator = (const pgsLiftingStressCheckArtifact& rOther);

   // GROUP: OPERATIONS
   // GROUP: ACCESS
   //------------------------------------------------------------------------
   // we always have a parent
   void SetParent(const pgsLiftingCheckArtifact* pParent);

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

   const pgsLiftingCheckArtifact* m_pParent;

   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   //------------------------------------------------------------------------
   void MakeCopy(const pgsLiftingStressCheckArtifact& rOther);

   //------------------------------------------------------------------------
   virtual void MakeAssignment(const pgsLiftingStressCheckArtifact& rOther);

   // GROUP: ACCESS
   // GROUP: INQUIRY

private:
   // GROUP: DATA MEMBERS

   // GROUP: LIFECYCLE
   // Default constructor
   pgsLiftingStressCheckArtifact();
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
   pgsLiftingCrackingCheckArtifact

   Artifact that holds lifting cracking check results at a location.


DESCRIPTION
   Artifact that holds lifting cracking check results at a location.


COPYRIGHT
   Copyright (c) 1999
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rdp : 03.26.1999 : Created file
*****************************************************************************/

class PGSEXTCLASS pgsLiftingCrackingCheckArtifact : public pgsLiftingCrackingAnalysisArtifact
{
public:

   // GROUP: DATA MEMBERS

   bool   Passed() const;

   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   pgsLiftingCrackingCheckArtifact(const pgsLiftingCheckArtifact& rParent);

   //------------------------------------------------------------------------
   // Copy constructor
   pgsLiftingCrackingCheckArtifact(const pgsLiftingCrackingCheckArtifact& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~pgsLiftingCrackingCheckArtifact();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   pgsLiftingCrackingCheckArtifact& operator = (const pgsLiftingCrackingCheckArtifact& rOther);

   // GROUP: OPERATIONS
   // GROUP: ACCESS
   //------------------------------------------------------------------------
   // we always have a parent
   void SetParent(const pgsLiftingCheckArtifact* pParent);

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
   void MakeCopy(const pgsLiftingCrackingCheckArtifact& rOther);

   //------------------------------------------------------------------------
   virtual void MakeAssignment(const pgsLiftingCrackingCheckArtifact& rOther);

   // GROUP: ACCESS
   // GROUP: INQUIRY

private:
   // GROUP: DATA MEMBERS
   const pgsLiftingCheckArtifact* m_pParent;

   // GROUP: LIFECYCLE
   // Default constructor
   pgsLiftingCrackingCheckArtifact();

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
   pgsLiftingCheckArtifact

   Artifact which holds the detailed results of a girder lifting check


DESCRIPTION
   Artifact which holds the detailed results of a girder lifting check


COPYRIGHT
   Copyright © 1997-1998
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rdp : 03.25.1999 : Created file
*****************************************************************************/

class PGSEXTCLASS pgsLiftingCheckArtifact : public pgsLiftingAnalysisArtifact
{
public:
   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   pgsLiftingCheckArtifact();

   //------------------------------------------------------------------------
   // Copy constructor
   pgsLiftingCheckArtifact(const pgsLiftingCheckArtifact& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~pgsLiftingCheckArtifact();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   pgsLiftingCheckArtifact& operator = (const pgsLiftingCheckArtifact& rOther);

   // GROUP: OPERATIONS
   bool Passed() const;
   bool PassedFailureCheck() const;

   // GROUP: ACCESS

   // allowable FS's
   Float64 GetAllowableFsForCracking() const;
   void SetAllowableFsForCracking(Float64 val);

   Float64 GetAllowableFsForFailure() const;
   void SetAllowableFsForFailure(Float64 val);

   Float64 GetAllowableTensileStress() const;
   void SetAllowableTensileStress(Float64 val);

   Float64 GetAllowableCompressionStress() const;
   void SetAllowableCompressionStress(Float64 val);

   pgsLiftingStressCheckArtifact GetLiftingStressCheckArtifact(Float64 distFromStart) const;
   pgsLiftingCrackingCheckArtifact GetLiftingCrackingCheckArtifact(Float64 distFromStart) const;

   void SetAlternativeTensionAllowableStress(Float64 val);
   Float64 GetAlternativeTensionAllowableStress() const;

   // GROUP: INQUIRY

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   //------------------------------------------------------------------------
   void MakeCopy(const pgsLiftingCheckArtifact& rOther);

   //------------------------------------------------------------------------
   virtual void MakeAssignment(const pgsLiftingCheckArtifact& rOther);

   // GROUP: ACCESS
   // GROUP: INQUIRY

private:
   // GROUP: DATA MEMBERS
   Float64 m_AllowableTensileStress;
   Float64 m_AllowableCompressionStress;
   Float64 m_AllowableFsForCracking;
   Float64 m_AllowableFsForFailure;
   Float64 m_AllowableAlternativeTensileStress;

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

#endif // INCLUDED_PGSEXT_LIFTINGCHECKARTIFACT_H_
