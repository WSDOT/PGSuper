///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2025  Washington State Department of Transportation
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


class pgsLibraryEntryDifferenceItem;
class DuctLibraryEntry;
class DuctLibraryEntryObserver;
#pragma warning(disable:4231)
PSGLIBTPL WBFL::System::SubjectT<DuctLibraryEntryObserver, DuctLibraryEntry>;

/*****************************************************************************
CLASS 
   DuctLibraryEntryObserver

   A pure virtual entry class for observing ducts entries.
*****************************************************************************/
class PSGLIBCLASS DuctLibraryEntryObserver
{
public:
   virtual void Update(DuctLibraryEntry& subject, Int32 hint)=0;
};

/*****************************************************************************
CLASS 
   DuctLibraryEntry

   A library entry class for duct definitions.
*****************************************************************************/

class PSGLIBCLASS DuctLibraryEntry : public WBFL::Library::LibraryEntry, public ISupportIcon,
       public WBFL::System::SubjectT<DuctLibraryEntryObserver, DuctLibraryEntry>
{
public:
   DuctLibraryEntry() = default;
   DuctLibraryEntry(const DuctLibraryEntry& rOther) = default;
   virtual ~DuctLibraryEntry() = default;

   DuctLibraryEntry& operator=(const DuctLibraryEntry& rOther) = default;

   //------------------------------------------------------------------------
   // Edit the entry
   virtual bool Edit(bool allowEditing,int nPage=0);

   //------------------------------------------------------------------------
   // Get the icon for this entry
   virtual HICON GetIcon() const;

   virtual void Notify(int hint) override {};

   //------------------------------------------------------------------------
   // Save to structured storage
   virtual bool SaveMe(WBFL::System::IStructuredSave* pSave);

   //------------------------------------------------------------------------
   // Load from structured storage
   virtual bool LoadMe(WBFL::System::IStructuredLoad* pLoad);

   //------------------------------------------------------------------------
   // Set/Get Outside Diameter
   void SetOD(Float64 od);
   Float64 GetOD() const;

   //------------------------------------------------------------------------
   // Set/Get Inside Diameter
   void SetID(Float64 id);
   Float64 GetID() const;

   //------------------------------------------------------------------------
   // Set/Get Nominal Diameter
   void SetNominalDiameter(Float64 id);
   Float64 GetNominalDiameter() const;

   //------------------------------------------------------------------------
   // Set/Get distance from center of duct to center of strands at top/bottom
   void SetZ(Float64 z);
   Float64 GetZ() const;

   // Returns the inside area of the duct
   Float64 GetInsideArea() const;

   //------------------------------------------------------------------------
   // Compares this library entry with rOther. Returns true if the entries are the same.
   // vDifferences contains a listing of the differences. The caller is responsible for deleting the difference items
   bool Compare(const DuctLibraryEntry& rOther, std::vector<std::unique_ptr<pgsLibraryEntryDifferenceItem>>& vDifferences, bool& bMustRename, bool bReturnOnFirstDifference=false,bool considerName=false) const;
 
   bool IsEqual(const DuctLibraryEntry& rOther,bool bConsiderName=false) const;

protected:

private:
   // GROUP: DATA MEMBERS
   Float64 m_OD = 0.0;
   Float64 m_ID = 0.0;
   Float64 m_ND = 0.0;
   Float64 m_Z = 0.0;
};
