///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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

#ifndef INCLUDED_GIRDERDETAILINGCHECK_H_
#define INCLUDED_GIRDERDETAILINGCHECK_H_

#include <Reporting\ReportingExp.h>

class IEAFDisplayUnits;
class pgsGirderArtifact;

/*****************************************************************************
CLASS 
   CGirderDetailingCheck

   Encapsulates the construction of the girder detailing check report content.


DESCRIPTION
   Encapsulates the construction of the girder detailing check report content.

LOG
   rab : 11.27.1998 : Created file
*****************************************************************************/

class REPORTINGCLASS CGirderDetailingCheck
{
public:
   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   CGirderDetailingCheck();

   //------------------------------------------------------------------------
   // Constructor if a basic version (no girder dimensions) is desired
   CGirderDetailingCheck(bool basic);

   //------------------------------------------------------------------------
   // Copy constructor
   CGirderDetailingCheck(const CGirderDetailingCheck& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~CGirderDetailingCheck();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   CGirderDetailingCheck& operator = (const CGirderDetailingCheck& rOther);

   // GROUP: OPERATIONS

   //------------------------------------------------------------------------
   // Builds the strand eccentricity table.
   virtual void Build(rptChapter* pChapter,
                      std::shared_ptr<WBFL::EAF::Broker> pBroker,const pgsGirderArtifact* pGirderArtifact,
                      std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) const;
   // GROUP: ACCESS
   // GROUP: INQUIRY

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   //------------------------------------------------------------------------
   void MakeCopy(const CGirderDetailingCheck& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const CGirderDetailingCheck& rOther);

   // GROUP: ACCESS
   // GROUP: INQUIRY

private:
   // GROUP: DATA MEMBERS
   bool m_BasicVersion;

   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   void BuildDimensionCheck(rptChapter* pChapter,
                            std::shared_ptr<WBFL::EAF::Broker> pBroker,const pgsGirderArtifact* pGirderArtifact,
                            std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) const;

   void BuildStirrupLayoutCheck(rptChapter* pChapter,
                                std::shared_ptr<WBFL::EAF::Broker> pBroker,const pgsGirderArtifact* pGirderArtifact,
                                std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) const;
   // GROUP: ACCESS
   // GROUP: INQUIRY
};

// INLINE METHODS
//

// EXTERNAL REFERENCES
//

#endif // INCLUDED_GIRDERDETAILINGCHECK_H_
