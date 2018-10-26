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

#ifndef INCLUDED_PGSEXT_DECKPOINT_H_
#define INCLUDED_PGSEXT_DECKPOINT_H_

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

#include <PGSuperTypes.h>
#include <PgsExt\DeckRebarData.h>

//#include <StrData.h>

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//

// MISCELLANEOUS
//

/*****************************************************************************
CLASS 
   CDeckPoint

   Utility class for describing points along the edge of deck.

DESCRIPTION
   Utility class for describing points along the edge of deck.

COPYRIGHT
   Copyright © 1997-2008
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rab : 06.18.2008 : Created file
*****************************************************************************/
class PGSEXTCLASS CDeckPoint
{
public:
   CDeckPoint();
   CDeckPoint(const CDeckPoint& rOther);
   ~CDeckPoint();

   CDeckPoint& operator = (const CDeckPoint& rOther);
   bool operator == (const CDeckPoint& rOther) const;
   bool operator != (const CDeckPoint& rOther) const;

   bool operator< (const CDeckPoint& rOther) const; // for sorting purposes


   HRESULT Load(IStructuredLoad* pStrLoad,IProgress* pProgress);
   HRESULT Save(IStructuredSave* pStrSave,IProgress* pProgress);

   double Station;   // station where this edge point is measured
   double LeftEdge;  // + = right of measurement datum
   double RightEdge; // + = right of measurement datum
   pgsTypes::OffsetMeasurementType MeasurementType; // datum of measurement
   pgsTypes::DeckPointTransitionType LeftTransitionType; // how the deck edge transitions to the next point
   pgsTypes::DeckPointTransitionType RightTransitionType; // how the deck edge transitions to the next point


protected:
   void MakeCopy(const CDeckPoint& rOther);
   void MakeAssignment(const CDeckPoint& rOther);
};


#endif // INCLUDED_PGSEXT_DECKPOINT_H_
