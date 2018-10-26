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

#ifndef INCLUDED_PGSEXT_HAULINGCHECKARTIFACT_H_
#define INCLUDED_PGSEXT_HAULINGCHECKARTIFACT_H_

// SYSTEM INCLUDES
//

// PROJECT INCLUDES
//
#if !defined INCLUDED_PGSEXTEXP_H_
#include <PgsExt\PgsExtExp.h>
#endif

#include <PgsExt\HaulingAnalysisArtifact.h>

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//
class pgsHaulingCheckArtifact;

// MISCELLANEOUS
//


/*****************************************************************************
CLASS 
   pgsHaulingStressCheckArtifact

   Artifact that holds Hauling stress check results at a location.


DESCRIPTION
   Artifact that holds Hauling stress check results at a location.


COPYRIGHT
   Copyright (c) 1999
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rdp : 03.26.1999 : Created file
*****************************************************************************/

class PGSEXTCLASS pgsHaulingStressCheckArtifact : public pgsHaulingStressAnalysisArtifact
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
   pgsHaulingStressCheckArtifact(const pgsHaulingCheckArtifact& rParent);

   //------------------------------------------------------------------------
   // Copy constructor
   pgsHaulingStressCheckArtifact(const pgsHaulingStressCheckArtifact& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~pgsHaulingStressCheckArtifact();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   pgsHaulingStressCheckArtifact& operator = (const pgsHaulingStressCheckArtifact& rOther);

   // GROUP: OPERATIONS
   // GROUP: ACCESS
   //------------------------------------------------------------------------
   // we always have a parent
   void SetParent(const pgsHaulingCheckArtifact* pParent);

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
   void MakeCopy(const pgsHaulingStressCheckArtifact& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const pgsHaulingStressCheckArtifact& rOther);

   // GROUP: ACCESS
   // GROUP: INQUIRY

private:
   // GROUP: DATA MEMBERS
   const pgsHaulingCheckArtifact* m_pParent;

   // GROUP: LIFECYCLE
   // Default constructor
   pgsHaulingStressCheckArtifact();
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
   pgsHaulingCrackingCheckArtifact

   Artifact that holds Hauling cracking check results at a location.


DESCRIPTION
   Artifact that holds Hauling cracking check results at a location.


COPYRIGHT
   Copyright (c) 1999
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rdp : 03.26.1999 : Created file
*****************************************************************************/

class PGSEXTCLASS pgsHaulingCrackingCheckArtifact : public pgsHaulingCrackingAnalysisArtifact
{
public:
   // GROUP: DATA MEMBERS

   bool   Passed() const;

   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   pgsHaulingCrackingCheckArtifact(const pgsHaulingCheckArtifact& rParent);

   //------------------------------------------------------------------------
   // Copy constructor
   pgsHaulingCrackingCheckArtifact(const pgsHaulingCrackingCheckArtifact& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~pgsHaulingCrackingCheckArtifact();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   pgsHaulingCrackingCheckArtifact& operator = (const pgsHaulingCrackingCheckArtifact& rOther);

   // GROUP: OPERATIONS
   // GROUP: ACCESS
   //------------------------------------------------------------------------
   // we always have a parent
   void SetParent(const pgsHaulingCheckArtifact* pParent);

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
   void MakeCopy(const pgsHaulingCrackingCheckArtifact& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const pgsHaulingCrackingCheckArtifact& rOther);

   // GROUP: ACCESS
   // GROUP: INQUIRY

private:
   // GROUP: DATA MEMBERS
   const pgsHaulingCheckArtifact* m_pParent;

   // GROUP: LIFECYCLE
   // Default constructor
   pgsHaulingCrackingCheckArtifact();

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
   pgsHaulingCheckArtifact

   Artifact which holds the detailed results of a girder Hauling check


DESCRIPTION
   Artifact which holds the detailed results of a girder Hauling check


COPYRIGHT
   Copyright © 1997-1998
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rdp : 03.25.1999 : Created file
*****************************************************************************/

class PGSEXTCLASS pgsHaulingCheckArtifact : public pgsHaulingAnalysisArtifact
{
public:
   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   pgsHaulingCheckArtifact();

   //------------------------------------------------------------------------
   // Copy constructor
   pgsHaulingCheckArtifact(const pgsHaulingCheckArtifact& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~pgsHaulingCheckArtifact();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   pgsHaulingCheckArtifact& operator = (const pgsHaulingCheckArtifact& rOther);

   // GROUP: OPERATIONS
   bool Passed() const;
   // GROUP: ACCESS

   // allowable FS's
   Float64 GetAllowableFsForCracking() const;
   void SetAllowableFsForCracking(Float64 val);

   Float64 GetAllowableFsForRollover() const;
   void SetAllowableFsForRollover(Float64 val);

   Float64 GetAllowableSpanBetweenSupportLocations() const;
   void SetAllowableSpanBetweenSupportLocations(Float64 val);

   Float64 GetAllowableLeadingOverhang() const;
   void SetAllowableLeadingOverhang(Float64 val);

   Float64 GetAllowableTensileStress() const;
   void SetAllowableTensileStress(Float64 val);

   Float64 GetAllowableCompressionStress() const;
   void SetAllowableCompressionStress(Float64 val);
   pgsHaulingStressCheckArtifact GetHaulingStressCheckArtifact(Float64 distFromStart) const;
   pgsHaulingCrackingCheckArtifact GetHaulingCrackingCheckArtifact(Float64 distFromStart) const;

   Float64 GetMaxGirderWgt() const;
   void SetMaxGirderWgt(Float64 maxWgt);

   Float64 GetAlternativeTensionAllowableStress() const;
   void SetAlternativeTensionAllowableStress(Float64 val);

   // GROUP: INQUIRY

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   //------------------------------------------------------------------------
   void MakeCopy(const pgsHaulingCheckArtifact& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const pgsHaulingCheckArtifact& rOther);

   // GROUP: ACCESS
   // GROUP: INQUIRY

private:
   // GROUP: DATA MEMBERS

   Float64 m_AllowableSpanBetweenSupportLocations;
   Float64 m_AllowableLeadingOverhang;
   Float64 m_AllowableTensileStress;
   Float64 m_AllowableCompressionStress;
   Float64 m_AllowableFsForCracking;
   Float64 m_AllowableFsForRollover;
   Float64 m_MaxGirderWgt;
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

#endif // INCLUDED_PGSEXT_HAULINGCHECKARTIFACT_H_
