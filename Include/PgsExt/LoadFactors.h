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

#ifndef INCLUDED_PGSEXT_LOADFACTORS_H_
#define INCLUDED_PGSEXT_LOADFACTORS_H_

// SYSTEM INCLUDES
//
#include <WBFLCore.h>

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

class PGSEXTCLASS CLoadFactors
{
public:
   double DCmin[6];   // index is one of pgsTypes::LimitState constants (except for CLLIM)
   double DWmin[6];
   double LLIMmin[6];
   double DCmax[6];
   double DWmax[6];
   double LLIMmax[6];

   CLoadFactors();
   CLoadFactors(const CLoadFactors& rOther);
   CLoadFactors& operator=(const CLoadFactors& rOther);

   bool operator==(const CLoadFactors& rOther) const; 
   bool operator!=(const CLoadFactors& rOther) const;

protected:
   void MakeCopy(const CLoadFactors& rOther);
   void MakeAssignment(const CLoadFactors& rOther);
};

// INLINE METHODS
//

// EXTERNAL REFERENCES
//


#endif // LOADFACTORS

