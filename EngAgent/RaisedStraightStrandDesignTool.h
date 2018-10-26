///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2017  Washington State Department of Transportation
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

#ifndef INCLUDED_RaisedStraightStrandDesignTool_H_
#define INCLUDED_RaisedStraightStrandDesignTool_H_

// SYSTEM INCLUDES
//

// PROJECT INCLUDES
//

#include <IFace\Artifact.h>
#include <PgsExt\PoiMgr.h>

#include <PgsExt\GirderDesignArtifact.h>
#include <PgsExt\GirderData.h>
#include <PsgLib\GirderLibraryEntry.h>

#include <algorithm>
#include<list>
#include<vector>

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//
interface IBroker;

// MISCELLANEOUS

/*****************************************************************************
CLASS 
   pgsRaisedStraightStrandDesignTool

   Encapsulates the strand design strategy for raised straight strands


DESCRIPTION

LOG
   rdp : 11.24.2014 : Created file 
*****************************************************************************/

// The class below can be used to create a new strand sequence (fill order) that varies from the
// girder library sequence. The new sequence can then reference into the library entries straight and 
// harped strand grids, and be used to compute ConfigStrandFillVector's
class pgsStrandResequencer
{
public:
   pgsStrandResequencer(const GirderLibraryEntry* pGdrEntry);

   // Functions for resequencing and computing library locations from new sequence
   ////////////////////////////////////////////////////////////////////////////////
   // Set fill back to library defaults
   void ResetToLibraryDefault();

   // Move a strand location using the original library index to a specific location in the 
   // reordered grid. This will remove the strand from its current location and insert it at the new
   void MovePermLibraryIndexToNewLocation(GridIndexType libraryGridIdx, GridIndexType newGridIdx);

   // Return reordered strand grid locations referenced to library permanent grid, and individual grids
   GridIndexType GetPermanentStrandGridSize() const;
   void GetGridPositionFromPermStrandGrid(GridIndexType gridIdx, GridIndexType* libraryPermStrandGridIdx, GirderLibraryEntry::psStrandType* type, GridIndexType* libraryTypeGridIdx) const;

   // Return basic querying about strand ordering
   // INVALID_INDEX is returned if can't find value
   StrandIndexType GetNextNumPermanentStrands(StrandIndexType prevNum) const;
   StrandIndexType GetPreviousNumPermanentStrands(StrandIndexType nextNum) const; 
   bool IsValidNumPermanentStrands(StrandIndexType num) const;
   StrandIndexType GetMaxPermanentStrands() const;

   // Compute the number of straight and harped strands based total permanent strands
   // Will return INVALID_INDEX for both if bad num perm
   void ComputeNumStrands(StrandIndexType numPermStrands, StrandIndexType* pNStraightStrands, StrandIndexType* pNHarpedStrands) const;

   // return a config fill vector for given strand type
   ConfigStrandFillVector CreateStrandFill(GirderLibraryEntry::psStrandType type, StrandIndexType numStrands) const; 

   StrandIndexType GetPermanentStrandCountFromPermGridIndex(GridIndexType gridIndex) const;

private:
   pgsStrandResequencer();

   const GirderLibraryEntry* m_pGdrEntry;

   // Data structure to hold resequenced strand data
   struct fillItem
   {
      GridIndexType                    m_LibraryFillIdx;  // Permanent fill index in library
      GirderLibraryEntry::psStrandType m_strandType;
      GridIndexType                    m_LocalLibFillIdx; // Fill index in library for strand type
      StrandIndexType                  m_AvailableSlots;  // 1 or 2 depending on x location

      fillItem(GridIndexType libraryFillIdx, GirderLibraryEntry::psStrandType strandType, 
               GridIndexType localLibFillIdx, StrandIndexType availableSlots):
      m_LibraryFillIdx(libraryFillIdx),m_strandType(strandType), m_LocalLibFillIdx(localLibFillIdx),m_AvailableSlots(availableSlots)
      {
      }

      bool operator == (const fillItem& rOther) const
      {
         return m_LibraryFillIdx == rOther.m_LibraryFillIdx; // weak equals
      }

   private:
      fillItem();

   };

   typedef std::vector<fillItem> FillItemList;
   typedef FillItemList::iterator FillItemListIterator;
   typedef FillItemList::const_iterator FillItemListConstIterator;

   FillItemList m_FillItemList;
   StrandIndexType m_MaxPermStrands;
};

class pgsRaisedStraightStrandDesignTool
{
public:
   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // constructor
   pgsRaisedStraightStrandDesignTool(SHARED_LOGFILE lf, const GirderLibraryEntry* pGdrEntry);

   ~pgsRaisedStraightStrandDesignTool()
   {
      ;
   }
   
   void Initialize(IBroker* pBroker, StatusGroupIDType statusGroupID, pgsSegmentDesignArtifact* pArtif);

   // GROUP: OPERATIONS

   // Add raised straight strands at the top of the adjustable group. This changes the 
   // minimum required number of strands as well
   // It also invalidates the current strand grid, so design loops are expected to restart after calling this
   bool AddRaisedStraightStrands();

   StrandIndexType GetMaxPermanentStrands();
   StrandIndexType GetNextNumPermanentStrands(StrandIndexType prevNum);
   StrandIndexType GetPreviousNumPermanentStrands(StrandIndexType nextNum); 
   bool IsValidNumPermanentStrands(StrandIndexType num);

   StrandIndexType GetNumUsedRaisedStrandLocations() const
   {
      return m_UsedRaisedStrandLocations;
   }

   StrandIndexType GetMinimumPermanentStrands() const;

   void ComputeNumStrands(StrandIndexType numPermStrands, StrandIndexType* pNStraightStrands, StrandIndexType* pNHarpedStrands) const;

   // return a config fill vector for given permanent strand type
   ConfigStrandFillVector CreateStrandFill(GirderLibraryEntry::psStrandType type, StrandIndexType numStrands) const; 

   // ACCESS
   //////////

   // GROUP: INQUIRY

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   // GROUP: ACCESS
   // GROUP: INQUIRY

   // GROUP: DATA MEMBERS

   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS

   // GROUP: ACCESS
   // GROUP: INQUIRY


private:
   IBroker* m_pBroker;
   StatusGroupIDType m_StatusGroupID;
   const GirderLibraryEntry* m_pGdrEntry;

   pgsSegmentDesignArtifact* m_pArtifact;
   CSegmentKey m_SegmentKey;

   Float64 m_Yb;
   Float64 m_LowerKern;

   StrandIndexType m_MaxPermStrandCount;

   pgsStrandResequencer m_StrandResequencer;

   // possible raised strand locations
   struct RaisedStrandLocation
   {
      Float64                          m_Y;
      Float64                          m_X; // need X for sorting
      GridIndexType                    m_PermLibraryFillIdx;  // Permanent fill index in library
      GridIndexType                    m_LocalLibFillIdx; // Fill index in library for strand type
      StrandIndexType                  m_FillCount; 

      RaisedStrandLocation(Float64 X, Float64 Y, GridIndexType libraryFillIdx, GridIndexType localLibFillIdx, StrandIndexType fillCnt):
      m_X(X),m_Y(Y),m_PermLibraryFillIdx(libraryFillIdx), m_LocalLibFillIdx(localLibFillIdx), m_FillCount(fillCnt)
      {
      }

      bool operator==(const RaisedStrandLocation& rOther) const
      { 
         return ::IsEqual(m_Y,rOther.m_Y) && ::IsEqual(m_X,rOther.m_X);
      }

      bool operator<(const RaisedStrandLocation& rOther) const 
      { 
         if (m_Y < rOther.m_Y) 
            return true;
         else if (m_Y > rOther.m_Y) 
            return false;
         else
            return m_X < rOther.m_X; 
      }

   private:
      RaisedStrandLocation();
   };

   typedef std::set<RaisedStrandLocation> RaisedStrandCollection;
   typedef RaisedStrandCollection::iterator RaisedStrandIterator;
   typedef RaisedStrandCollection::reverse_iterator RaisedStrandReverseIterator;

   RaisedStrandCollection m_RaisedStrandPotentialLocations; // possible locations
   GridIndexType          m_UsedRaisedStrandLocations;      // number we have used

private:
	DECLARE_SHARED_LOGFILE;

};

// INLINE METHODS
//

// EXTERNAL REFERENCES
//

#endif // INCLUDED_RaisedStraightStrandDesignTool_H_
