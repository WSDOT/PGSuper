///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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

#include "psgLibLib.h"

#include <psgLib\ISupportIcon.h>
#include <libraryFw\LibraryEntry.h>

#include <System\SubjectT.h>


class DuctLibraryEntry;
class DuctLibraryEntryObserver;
#pragma warning(disable:4231)
PSGLIBTPL sysSubjectT<DuctLibraryEntryObserver, DuctLibraryEntry>;

/*****************************************************************************
CLASS 
   DuctLibraryEntryObserver

   A pure virtual entry class for observing ducts entries.
*****************************************************************************/
class PSGLIBCLASS DuctLibraryEntryObserver
{
public:
   virtual void Update(DuctLibraryEntry* pSubject, Int32 hint)=0;
};

/*****************************************************************************
CLASS 
   DuctLibraryEntry

   A library entry class for duct definitions.
*****************************************************************************/

class PSGLIBCLASS DuctLibraryEntry : public libLibraryEntry, public ISupportIcon,
       public sysSubjectT<DuctLibraryEntryObserver, DuctLibraryEntry>
{
public:
   DuctLibraryEntry();
   DuctLibraryEntry(const DuctLibraryEntry& rOther);
   virtual ~DuctLibraryEntry();

   DuctLibraryEntry& operator = (const DuctLibraryEntry& rOther);

   //------------------------------------------------------------------------
   // Edit the entry
   virtual bool Edit(bool allowEditing,int nPage=0);

   //------------------------------------------------------------------------
   // Get the icon for this entry
   virtual HICON GetIcon() const;

   //------------------------------------------------------------------------
   // Save to structured storage
   virtual bool SaveMe(sysIStructuredSave* pSave);

   //------------------------------------------------------------------------
   // Load from structured storage
   virtual bool LoadMe(sysIStructuredLoad* pLoad);

   //------------------------------------------------------------------------
   // Set/Get Outside Diameter
   void SetOD(Float64 od);
   Float64 GetOD() const;

   //------------------------------------------------------------------------
   // Set/Get Inside Diameter
   void SetID(Float64 id);
   Float64 GetID() const;

   //------------------------------------------------------------------------
   // Set/Get distance from center of duct to center of strands at top/bottom
   void SetZ(Float64 z);
   Float64 GetZ() const;

   // Returns the inside area of the duct
   Float64 GetInsideArea() const;

   //------------------------------------------------------------------------
   // Equality - test if two entries are equal. Ignore names by default
   bool IsEqual(const DuctLibraryEntry& rOther, bool considerName=false) const;

protected:
   void MakeCopy(const DuctLibraryEntry& rOther);
   virtual void MakeAssignment(const DuctLibraryEntry& rOther);

private:
   // GROUP: DATA MEMBERS
   Float64 m_OD;
   Float64 m_ID;
   Float64 m_Z;
};
