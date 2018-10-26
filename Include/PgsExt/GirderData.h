///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright (C) 1999  Washington State Department of Transportation
//                     Bridge and Structures Office
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

#ifndef INCLUDED_PGSEXT_GIRDERDATA_H_
#define INCLUDED_PGSEXT_GIRDERDATA_H_

// SYSTEM INCLUDES
//
#include <WBFLCore.h>

// PROJECT INCLUDES
//
#if !defined INCLUDED_PGSEXTEXP_H_
#include <PgsExt\PgsExtExp.h>
#endif

#if !defined INCLUDED_STRDATA_H_
#include <StrData.h>
#endif

#include <PgsExt\GirderMaterial.h>
#include <PgsExt\ShearData.h>
#include <PgsExt\LongitudinalRebarData.h>
#include <PgsExt\HandlingData.h>

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//
class matPsStrand;
class ConcreteLibraryEntry;

// MISCELLANEOUS
//

// Class to store debonding input
class PGSEXTCLASS CDebondInfo
{
public:
   StrandIndexType idxStrand1;
   StrandIndexType idxStrand2; // -1 if not used
   Float64 Length1; // debond length at left end of girder
   Float64 Length2; // debond length at right end of girder

	HRESULT Load(IStructuredLoad* pStrLoad,IProgress* pProgress);
	HRESULT Save(IStructuredSave* pStrSave,IProgress* pProgress);

   bool operator==(const CDebondInfo& rOther) const; 
   bool operator!=(const CDebondInfo& rOther) const;
};


/*****************************************************************************
CLASS 
   CGirderData

   Utility class for prestressing description data.

DESCRIPTION
   Utility class for prestressing description data. This class encapsulates all
   the input data the Prestress page of the Bridge Description Dialog, and 
   implements the IStructuredLoad and IStructuredSave persistence interfaces.


COPYRIGHT
   Copyright © 1997-1998
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rab : 09.30.1998 : Created file
*****************************************************************************/
// Method for describing number of permanent strands
#define NPS_TOTAL_NUMBER    0    // use total number and order as defined by library entry
#define NPS_STRAIGHT_HARPED 1    // use number of straight and number of harped.

class PGSEXTCLASS CGirderData
{
public:
   int     NumPermStrandsType; // one of NPS_ above
   // Note that the arrays with size 3 and4 below are indexed using pgsTypes::StrandType.
   // The pgsTypes::Permanent position is used when NumPermStrandsType==NPS_TOTAL_NUMBER.
   // When this is the case, values must be divided proportionally to straight and harped strands into 
   // the pgsTypes::Harped and pgsTypes::Straight strand locations because these are the values
   // used internally by the analysis and engineering agents
   StrandIndexType  Nstrands[4];
   Float64 Pjack[4];
   HarpedStrandOffsetType HsoEndMeasurement; // one of HarpedStrandOffsetType enums
   Float64 HpOffsetAtEnd;
   HarpedStrandOffsetType HsoHpMeasurement;  // one of HarpedStrandOffsetType enums
   Float64 HpOffsetAtHp;

   std::vector<CDebondInfo> Debond[3];
   bool bSymmetricDebond; // if true, left and right debond are the same (Only use Length1 of CDebondInfo struct)

   bool bPjackCalculated[4]; // true if Pjack was calculated
   Float64 LastUserPjack[4]; // Last Pjack entered by user

   pgsTypes::TTSUsage TempStrandUsage; // One of the tts constants above.

   CGirderMaterial Material; // concrete and strand data
   CShearData ShearData;
   CLongitudinalRebarData LongitudinalRebarData;
   CHandlingData HandlingData;


   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   CGirderData();

   //------------------------------------------------------------------------
   // Copy constructor
   CGirderData(const CGirderData& rOther);

   //------------------------------------------------------------------------
   // Destructor
   ~CGirderData();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   CGirderData& operator = (const CGirderData& rOther);

   //------------------------------------------------------------------------
   // Resets all the prestressing input to default values.
   // Useful when changing girder types
   void ResetPrestressData();

   //------------------------------------------------------------------------
   // An == operator is not enough. We must know the type of change that was
   // made in order to fire the right events. That's what the following enum 
   // and function do.
   enum ChangeType {ctNone         = 0x0000,
                    ctPrestress    = 0x0001,  // # or configuration of prestress changed
                    ctConcrete     = 0x0002,  // concrete material properties changed
                    ctStrand       = 0x0004,
                    ctLifting      = 0x0008,
                    ctShipping     = 0x0010
   };

   // return or'ed enums above 
   int GetChangeType(const CGirderData& rOther) const;

   //------------------------------------------------------------------------
   // specialized function to copy only material data or only prestressing data
   // from another
   void CopyMaterialFrom(const CGirderData& rOther);
   void CopyPrestressingFrom(const CGirderData& rOther);

   long GetDebondCount(pgsTypes::StrandType strandType) const;
   void ClearDebondData();

   //------------------------------------------------------------------------
   bool operator==(const CGirderData& rOther) const;

   //------------------------------------------------------------------------
   bool operator!=(const CGirderData& rOther) const;

   // GROUP: OPERATIONS

   // the last 4 paramters in Load is are concrete properties. Concrete data for the girder used
   // to be stored elsewhere. Passing the default data makes it possible to handle older data structures
	HRESULT Load(IStructuredLoad* pStrLoad,IProgress* pProgress, Float64 fc=0,Float64 weightDensity=0,Float64 strengthDensity=0,Float64 maxAggSize=0);
	HRESULT Save(IStructuredSave* pStrSave,IProgress* pProgress);

   // GROUP: ACCESS
   // GROUP: INQUIRY

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   //------------------------------------------------------------------------
   void MakeCopy(const CGirderData& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const CGirderData& rOther);

   // GROUP: ACCESS
   // GROUP: INQUIRY

private:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   // GROUP: ACCESS
   // GROUP: INQUIRY

   DECLARE_STRSTORAGEMAP(CGirderData)
};

// INLINE METHODS
//

// EXTERNAL REFERENCES
//


#endif // INCLUDED_PGSEXT_GIRDERDATA_H_

