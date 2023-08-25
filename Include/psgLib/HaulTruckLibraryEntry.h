///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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
class HaulTruckLibraryEntry;
class HaulTruckLibraryEntryObserver;
#pragma warning(disable:4231)
PSGLIBTPL WBFL::System::SubjectT<HaulTruckLibraryEntryObserver, HaulTruckLibraryEntry>;

/*****************************************************************************
CLASS 
   HaulTruckLibraryEntryObserver

   A pure virtual entry class for observing ducts entries.
*****************************************************************************/
class PSGLIBCLASS HaulTruckLibraryEntryObserver
{
public:
   virtual void Update(HaulTruckLibraryEntry& subject, Int32 hint)=0;
};

/*****************************************************************************
CLASS 
   HaulTruckLibraryEntry

   A library entry class for duct definitions.
*****************************************************************************/

class PSGLIBCLASS HaulTruckLibraryEntry : public WBFL::Library::LibraryEntry, public ISupportIcon,
       public WBFL::System::SubjectT<HaulTruckLibraryEntryObserver, HaulTruckLibraryEntry>
{
public:
   HaulTruckLibraryEntry();
   HaulTruckLibraryEntry(const HaulTruckLibraryEntry& rOther) = default;
   virtual ~HaulTruckLibraryEntry() = default;

   HaulTruckLibraryEntry& operator=(const HaulTruckLibraryEntry& rOther) = default;

   //------------------------------------------------------------------------
   // Edit the entry
   virtual bool Edit(bool allowEditing,int nPage=0);

   //------------------------------------------------------------------------
   // Get the icon for this entry
   virtual HICON GetIcon() const;

   // Set/Get height of bottom of girder above roadway during transport
   Float64 GetBottomOfGirderHeight() const;
   void SetBottomOfGirderHeight(Float64 hbg);

   // Set/Get height of truck roll center above roadway.
   Float64 GetRollCenterHeight() const;
   void SetRollCenterHeight(Float64 hrc);

   // Set/Get center-to-center distance between truck tires
   Float64 GetAxleWidth() const;
   void SetAxleWidth(Float64 wcc);

   // Set/Get truck roll stiffness
   Float64 GetRollStiffness() const;
   void SetRollStiffness(Float64 ktheta);

   // Set/Get the maximum distance between bunk points
   Float64 GetMaxDistanceBetweenBunkPoints() const;
   void SetMaxDistanceBetweenBunkPoints(Float64 lmax);

   // Set/Get the maximum leading overhang distance during hauling
   Float64 GetMaximumLeadingOverhang() const;
   void SetMaximumLeadingOverhang(Float64 maxoh);

   // Set/Get maximum girder weight for transportation
   Float64 GetMaxGirderWeight() const;
   void SetMaxGirderWeight(Float64 maxwgt);

   //------------------------------------------------------------------------
   // Save to structured storage
   virtual bool SaveMe(WBFL::System::IStructuredSave* pSave);

   //------------------------------------------------------------------------
   // Load from structured storage
   virtual bool LoadMe(WBFL::System::IStructuredLoad* pLoad);

   //------------------------------------------------------------------------
   // Compares this library entry with rOther. Returns true if the entries are the same.
   // vDifferences contains a listing of the differences. The caller is responsible for deleting the difference items
   bool Compare(const HaulTruckLibraryEntry& rOther, std::vector<std::unique_ptr<pgsLibraryEntryDifferenceItem>>& vDifferences, bool& bMustRename, bool bReturnOnFirstDifference=false,bool considerName=false) const;
 
   bool IsEqual(const HaulTruckLibraryEntry& rOther,bool bConsiderName=false) const;

protected:

private:
   Float64 m_Hbg; // height from roadway to bottom of girder
   Float64 m_Hrc; // height of roll center above roadway
   Float64 m_Wcc; // center-to-center width of wheels
   Float64 m_Ktheta; // Roll stiffness
   Float64 m_Lmax; // maximum span between bunk points
   Float64 m_MaxOH; // maximum leading overhang
   Float64 m_MaxWeight; // maximum weight of girder that can be hauled for this truck
};
