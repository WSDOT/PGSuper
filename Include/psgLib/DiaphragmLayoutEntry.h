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

#pragma once

#include "PsgLibLib.h"
#include <vector>
#include <MathEx.h>
#include <PsgLib\ISupportIcon.h>
#include <libraryFw\LibraryEntry.h>
#include <System\SubjectT.h>

class DiaphragmLayoutEntry;
class DiaphragmLayoutEntryObserver;
namespace PGS {namespace Library{class DifferenceItem;};};

#pragma warning(disable:4231)
PSGLIBTPL WBFL::System::SubjectT<DiaphragmLayoutEntryObserver, DiaphragmLayoutEntry>;

/*****************************************************************************
CLASS 
   DiaphragmLayoutEntryObserver

   A pure virtual entry class for observing Diaphragm material entries.


DESCRIPTION
   This class may be used to describe observe Diaphragm  materials in a library.

LOG
   rdp : 07.20.1998 : Created file
*****************************************************************************/
class PSGLIBCLASS DiaphragmLayoutEntryObserver
{
public:

   // GROUP: LIFECYCLE
   //------------------------------------------------------------------------
   // called by our subject to let us now he's changed, along with an optional
   // hint
   virtual void Update(DiaphragmLayoutEntry& subject, Int32 hint)=0;
};

/*****************************************************************************
CLASS 
   DiaphragmLayoutEntry

   A library entry class for diaphragm layouts


DESCRIPTION
   This class may be used to describe diaphragm layouts in a library.

LOG
   rdp : 07.20.1998 : Created file
*****************************************************************************/

class PSGLIBCLASS DiaphragmLayoutEntry : public WBFL::Library::LibraryEntry, public ISupportIcon,
       public WBFL::System::SubjectT<DiaphragmLayoutEntryObserver, DiaphragmLayoutEntry>
{
public:
   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // a struct that describes the layout
   struct DiaphragmLayout {
      Float64 EndOfRange;         // Longest span length that layout applies to
      Int32   NumberOfDiaphragms; // Number of evenly-spaced diaphragms in this range.
      bool operator==(const DiaphragmLayout& rOther) const
      {return ::IsEqual(EndOfRange, rOther.EndOfRange) &&
              NumberOfDiaphragms == rOther.NumberOfDiaphragms;} 
   };
   // Vector of layouts. Span lengths must be order from shortest to longest.
   typedef std::vector<DiaphragmLayout> DiaphragmLayoutVec;

   //------------------------------------------------------------------------
   // Default constructor
   DiaphragmLayoutEntry() = default;

   //------------------------------------------------------------------------
   // Copy constructor
   DiaphragmLayoutEntry(const DiaphragmLayoutEntry& rOther) = default;

   //------------------------------------------------------------------------
   // Destructor
   virtual ~DiaphragmLayoutEntry() = default;

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   DiaphragmLayoutEntry& operator=(const DiaphragmLayoutEntry& rOther) = default;

   // GROUP: OPERATIONS
   //------------------------------------------------------------------------
   // Get the icon for this entry
   virtual HICON GetIcon() const;

   //------------------------------------------------------------------------
   // Edit the entry
   virtual bool Edit(bool allowEditing,int nPage=0);

   //------------------------------------------------------------------------
   // Save to structured storage
   virtual bool SaveMe(WBFL::System::IStructuredSave* pSave);

   //------------------------------------------------------------------------
   // Load from structured storage
   virtual bool LoadMe(WBFL::System::IStructuredLoad* pLoad);

    // GROUP: ACCESS
   //------------------------------------------------------------------------
   // SetDiaphragmLayout - set diaphragm layout
   void SetDiaphragmLayout(const DiaphragmLayoutVec& vec);

   //------------------------------------------------------------------------
   // GetDiaphragmLayout - get diaphragm layout
   DiaphragmLayoutVec GetDiaphragmLayout() const;

   // Compares this library entry with rOther. Returns true if the entries are the same.
   // vDifferences contains a listing of the differences. The caller is responsible for deleting the difference items
   bool Compare(const DiaphragmLayoutEntry& rOther, std::vector<std::unique_ptr<PGS::Library::DifferenceItem>>& vDifferences, bool& bMustRename, bool bReturnOnFirstDifference=false,bool considerName=false) const;

   bool IsEqual(const DiaphragmLayoutEntry& rOther,bool bConsiderName=false) const;

private:
   DiaphragmLayoutVec m_DiaphragmLayoutVec;
};
