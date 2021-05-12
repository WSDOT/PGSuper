///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2021  Washington State Department of Transportation
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

#include "StdAfx.h"
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\Intervals.h>

#include <EAF\EAFDisplayUnits.h>

#include "RaisedStraightStrandDesignTool.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
////// local functions
//////////////////////

/****************************************************************************
CLASS
   pgsRaisedStraightStrandDesignTool
   - Generic class for managing resequencing of strand fill
****************************************************************************/
pgsStrandResequencer::pgsStrandResequencer(const GirderLibraryEntry* pGdrEntry)
{
   ATLASSERT(pGdrEntry);
   m_pGdrEntry = pGdrEntry;

   ResetToLibraryDefault();
}

void pgsStrandResequencer::ResetToLibraryDefault()
{
   // Fill empty list in library order
   m_FillItemList.clear();

   GridIndexType numStrands = m_pGdrEntry->GetPermanentStrandGridSize();
   m_FillItemList.reserve(numStrands);

   m_MaxPermStrands = 0;
   for (GridIndexType is=0; is<numStrands; is++)
   {
      GirderLibraryEntry::psStrandType type;
      GridIndexType locGridIdx;
      m_pGdrEntry->GetGridPositionFromPermStrandGrid(is, &type, &locGridIdx);
      GridIndexType avslots;
      if(GirderLibraryEntry::stAdjustable == type)
      {
         Float64 xs, xe, xh, ys, ye, yh;
         m_pGdrEntry->GetHarpedStrandCoordinates(locGridIdx, &xs, &ys, &xh, &yh, &xe, &ye);

         avslots = ( IsZero(xs) && IsZero(xh) && IsZero(xe) ) ? 1 : 2;
      }
      else
      {
         Float64 xs, ys, xe, ye;
         bool bIsDebondable;
         m_pGdrEntry->GetStraightStrandCoordinates(locGridIdx,  &xs, &ys, &xe, &ye, &bIsDebondable);

         avslots = ( IsZero(xs) && IsZero(xe) ) ? 1 : 2;
      }

      m_MaxPermStrands += avslots;

      fillItem item(is, type, locGridIdx, avslots);
      m_FillItemList.push_back(item);
   }
}

void pgsStrandResequencer::MovePermLibraryIndexToNewLocation(GridIndexType libraryGridIdx, GridIndexType newGridIdx)
{
   // Not super efficient. Fortunately this shouldn't be called often
   fillItem shill(libraryGridIdx, GirderLibraryEntry::stStraight, 0, 0); // only first index matters, others are bogus
   FillItemListIterator iter = std::find(m_FillItemList.begin(), m_FillItemList.end(), shill);
   if (iter != m_FillItemList.end())
   {
      // Erase item from vector
      fillItem copy = *iter;
      m_FillItemList.erase(iter);

      // and insert copy where it goes
      FillItemListIterator insiter = m_FillItemList.begin();
      insiter += newGridIdx;

      if (insiter != m_FillItemList.end())
      {
         m_FillItemList.insert(insiter, copy);
      }
      else
      {
         m_FillItemList.push_back(copy);
      }
   }
   else
   {
      ATLASSERT(0); // should always be able to find a library value
   }
}

GridIndexType pgsStrandResequencer::GetPermanentStrandGridSize() const
{
   return m_FillItemList.size();
}

void pgsStrandResequencer::GetGridPositionFromPermStrandGrid(GridIndexType gridIdx, GridIndexType* permStrandGridIdx, GirderLibraryEntry::psStrandType* type, GridIndexType* libraryTypeGridIdx) const
{
   ATLASSERT(gridIdx<GetPermanentStrandGridSize());

   const fillItem ritem = m_FillItemList.at(gridIdx);

   *permStrandGridIdx  = ritem.m_LibraryFillIdx;
   *type               = ritem.m_strandType;
   *libraryTypeGridIdx = ritem.m_LocalLibFillIdx;
}

   // Return basic querying about strand ordering
StrandIndexType pgsStrandResequencer::GetNextNumPermanentStrands(StrandIndexType currNum) const
{
   StrandIndexType running_cnt=0;
   FillItemListConstIterator iter(m_FillItemList.begin());
   FillItemListConstIterator enditer(m_FillItemList.end());
   while (iter!=enditer)
   {
      StrandIndexType nslots = iter->m_AvailableSlots;

      running_cnt += nslots;
     
      if (currNum < running_cnt)
      {
         return running_cnt;
      }

      iter++;
   }

   return INVALID_INDEX; // not found
}

StrandIndexType pgsStrandResequencer::GetPreviousNumPermanentStrands(StrandIndexType currNum) const
{
   if (currNum<=0)
   {
      return INVALID_INDEX;
   }
   else
   {
      StrandIndexType prev_cnt=0;
      StrandIndexType running_cnt=0;

      FillItemListConstIterator iter(m_FillItemList.begin());
      FillItemListConstIterator enditer(m_FillItemList.end());
      while (iter!=enditer)
      {
         StrandIndexType nslots = iter->m_AvailableSlots;

         running_cnt += nslots;
        
         if (running_cnt >= currNum )
         {
            return prev_cnt;
         }

         iter++;
         prev_cnt = running_cnt;
      }

      // Tough call here - could return error. but just return highest possible count
      return running_cnt; 
   }
}

bool pgsStrandResequencer::IsValidNumPermanentStrands(StrandIndexType num) const
{
   if (num==0)
   {
      return true;
   }
   else if (num==INVALID_INDEX)
   {
      return false;
   }
   else
   {
      return GetNextNumPermanentStrands(num-1) != INVALID_INDEX;
   }
}

StrandIndexType pgsStrandResequencer::GetMaxPermanentStrands() const
{
   return m_MaxPermStrands;
}

void pgsStrandResequencer::ComputeNumStrands(StrandIndexType numPermStrands, StrandIndexType* pNStraightStrands, StrandIndexType* pNHarpedStrands) const
{
   if (numPermStrands==0)
   {
      *pNStraightStrands = 0;
      *pNHarpedStrands = 0;
   }
   else if ( !IsValidNumPermanentStrands(numPermStrands) )
   {
      *pNStraightStrands = INVALID_INDEX;
      *pNHarpedStrands   = INVALID_INDEX;
   }
   else
   {
      StrandIndexType ns(0), nh(0);
      StrandIndexType cnt(0);
      FillItemListConstIterator iter(m_FillItemList.begin());
      FillItemListConstIterator enditer(m_FillItemList.end());
      while (iter!=enditer)
      {
         const fillItem& rFill = *iter;
         StrandIndexType nslots = iter->m_AvailableSlots;

         if (GirderLibraryEntry::stStraight == rFill.m_strandType)
         {
            ns += nslots;
         }
         else
         {
            nh += nslots;
         }

         cnt += nslots;
         if (numPermStrands <= cnt)
         {
            ATLASSERT(numPermStrands == cnt); // should always be dead on from IsValidNumPermanentStrands
            break;
         }

         iter++;
      }

      ATLASSERT(ns + nh == numPermStrands);

      *pNStraightStrands = ns;
      *pNHarpedStrands   = nh;
   }
}

// return a config fill vector for given strand type
ConfigStrandFillVector pgsStrandResequencer::CreateStrandFill(GirderLibraryEntry::psStrandType stype, StrandIndexType numStrands) const
{
   ConfigStrandFillVector fillvec;
   if (GirderLibraryEntry::stStraight == stype)
   {
      // straight strands
      StrandIndexType ns = m_pGdrEntry->GetMaxStraightStrands();
      fillvec.assign(ns, 0); // size and initialize vector

      if (0 < numStrands)
      {
         StrandIndexType cnt=0;
         FillItemListConstIterator iter(m_FillItemList.begin());
         FillItemListConstIterator enditer(m_FillItemList.end());
         bool found=false;
         while (iter!=enditer)
         {
            const fillItem& rFill = *iter;

            if (GirderLibraryEntry::stStraight == rFill.m_strandType)
            {
               StrandIndexType nslots = rFill.m_AvailableSlots;

               fillvec.at(rFill.m_LocalLibFillIdx) = nslots;
               cnt += nslots;

               if (numStrands <= cnt)
               {
                  ATLASSERT(cnt==numStrands); // clients should be smart enough to get right number of strands
                  found = true;
                  break;
               }
            }

            iter++;
         }

         ATLASSERT(found);
      }
   }
   else
   {
      // harped (adjustable) strands
      StrandIndexType ns = m_pGdrEntry->GetMaxHarpedStrands();
      fillvec.assign(ns, 0); // size and initialize vector

      if (0 < numStrands)
      {
         StrandIndexType cnt=0;
         FillItemListConstIterator iter(m_FillItemList.begin());
         FillItemListConstIterator enditer(m_FillItemList.end());
         bool found=false;
         while (iter!=enditer)
         {
            const fillItem& rFill = *iter;

            if (GirderLibraryEntry::stAdjustable == rFill.m_strandType)
            {
               StrandIndexType nslots = rFill.m_AvailableSlots;

               fillvec.at(rFill.m_LocalLibFillIdx) = nslots;
               cnt += nslots;

               if (numStrands <= cnt)
               {
                  ATLASSERT(cnt==numStrands); // clients should be smart enough to get right number of strands
                  found = true;
                  break;
               }
            }

            iter++;
         }

         ATLASSERT(found);
      }
   }

   return fillvec;
}

StrandIndexType pgsStrandResequencer::GetPermanentStrandCountFromPermGridIndex(GridIndexType gridIndex) const
{
   if (gridIndex >= m_FillItemList.size())
   {
      ATLASSERT(0);
      return INVALID_INDEX;
   }

   StrandIndexType num=0;
   for (GridIndexType ig=0; ig<=gridIndex; ig++)
   {
      num += m_FillItemList[ig].m_AvailableSlots;
   }

   return num;
}



/****************************************************************************
CLASS
   pgsRaisedStraightStrandDesignTool
****************************************************************************/


pgsRaisedStraightStrandDesignTool::pgsRaisedStraightStrandDesignTool(SHARED_LOGFILE lf, const GirderLibraryEntry* pGdrEntry):
LOGFILE(lf),
m_pBroker(nullptr),
m_StatusGroupID(INVALID_ID),
m_pGdrEntry(pGdrEntry),
m_StrandResequencer(pGdrEntry)
{
}

void pgsRaisedStraightStrandDesignTool::Initialize(IBroker* pBroker, StatusGroupIDType statusGroupID, pgsSegmentDesignArtifact* pArtif)
{
   ATLASSERT(pBroker);

   // cache some needed data
   m_pBroker = pBroker;
   m_StatusGroupID = statusGroupID;

   m_pArtifact = pArtif;
   m_SegmentKey = m_pArtifact->GetSegmentKey();

   // Get kern elevations
   GET_IFACE(IPointOfInterest, pPoi);
   PoiList vPoi;
   pPoi->GetPointsOfInterest(m_SegmentKey, POI_5L | POI_RELEASED_SEGMENT, &vPoi);
   ATLASSERT(vPoi.size() == 1);
   const pgsPointOfInterest& poi = vPoi.front();

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(m_SegmentKey);

   GET_IFACE(ISectionProperties,pSectProp);
   // Strands must be above NA in order to be considered for raising
   m_Yb        = pSectProp->GetY(releaseIntervalIdx, poi, pgsTypes::BottomGirder); //pSectProp->GetKt(poi) + pSectProp->GetHg();
   m_LowerKern = pSectProp->GetKb(releaseIntervalIdx,poi);

   // Compute candidate adjustable strand locations we can use for raised strands
   // Also: Figure number of straight strand rows below lower kern - this will be limit for # raised rows
   //       Get highest index of straight strands - will use this to compute max strand count
   GridIndexType highestStraightIdx = 0;
   std::set<Float64> straightRows;
   RaisedStrandCollection raisedLocs;
   StrandIndexType numperm = m_pGdrEntry->GetPermanentStrandGridSize();
   for(GridIndexType ip = 0; ip < numperm; ip++)
   {
      GirderLibraryEntry::psStrandType type;
      GridIndexType typidx;
      m_pGdrEntry->GetGridPositionFromPermStrandGrid(ip, &type, &typidx);
      if(GirderLibraryEntry::stAdjustable == type)
      {
         Float64 xs, xe, xh, ys, ye, yh;
         m_pGdrEntry->GetHarpedStrandCoordinates(typidx, &xs, &ys, &xh, &yh, &xe, &ye);
         if (m_Yb < yh)
         {
            StrandIndexType fillcnt = IsZero(xs) && IsZero(xh) ? 1 : 2;
            RaisedStrandLocation rasloc(xh, yh, ip, typidx, fillcnt);
            raisedLocs.insert(rasloc);
         }
      }
      else
      {
         highestStraightIdx = ip;

         Float64 xs, ys, xe, ye;
         bool bIsDebondable;
         m_pGdrEntry->GetStraightStrandCoordinates(typidx, &xs, &ys, &xe, &ye, &bIsDebondable);
         if (ys < m_LowerKern)
         {
            straightRows.insert(ys);
         }
      }
   }

   // Compute max allowable number of strands - this is lowest indexed adjustable strand in the upper kern
   // with an index more than highest indexed straight strand
   GridIndexType maxPermStrandGridIdx = MAX_INDEX;
   for (RaisedStrandIterator riter=raisedLocs.begin(); riter!=raisedLocs.end(); riter++)
   {
       if(highestStraightIdx < riter->m_PermLibraryFillIdx)
       {
          maxPermStrandGridIdx = Min(maxPermStrandGridIdx, riter->m_PermLibraryFillIdx);
       }
   }

   if( maxPermStrandGridIdx == MAX_INDEX)
   {
      maxPermStrandGridIdx = highestStraightIdx;
   }

   m_MaxPermStrandCount = this->m_StrandResequencer.GetPermanentStrandCountFromPermGridIndex(maxPermStrandGridIdx);

   GridIndexType nrows = straightRows.size();
   GridIndexType nrais = raisedLocs.size();
   if (nrows < nrais)
   {
      // More raised locations available than straight rows. Trim down lower values
      for (GridIndexType is=0; is<(nrais-nrows); is++)
      {
         RaisedStrandIterator riter = raisedLocs.begin();
         raisedLocs.erase(riter);
      }
   }

   // Store our potential raised locations
   m_RaisedStrandPotentialLocations = raisedLocs;
   m_UsedRaisedStrandLocations = 0;
}


// minimum required number of strands as well
bool pgsRaisedStraightStrandDesignTool::AddRaisedStraightStrands()
{
   LOG(_T("Entering  pgsRaisedStraightStrandDesignTool::AddRaisedStraightStrands"));
   LOG(_T("  m_UsedRaisedStrandLocations = ") << m_UsedRaisedStrandLocations << _T("m_RaisedStrandPotentialLocations.size() = ") << m_RaisedStrandPotentialLocations.size());

   if (m_UsedRaisedStrandLocations < m_RaisedStrandPotentialLocations.size())
   {
      // move next potential raised location toward top of fill order
      RaisedStrandReverseIterator riter = m_RaisedStrandPotentialLocations.rbegin(); // highest strands are at end of list
      for (GridIndexType is=0; is<m_UsedRaisedStrandLocations; is++)
      {
         riter ++;
      }

      m_StrandResequencer.MovePermLibraryIndexToNewLocation(riter->m_PermLibraryFillIdx, m_UsedRaisedStrandLocations);

      m_UsedRaisedStrandLocations++;
      LOG(_T("  Succeeded adding raised straight strand at permanent index = ") << riter->m_PermLibraryFillIdx );
      return true;
   }
   else
   {
      LOG(_T("  Failed to add raised strands in pgsRaisedStraightStrandDesignTool::AddRaisedStraightStrands"));
      return false;
   }
}

StrandIndexType pgsRaisedStraightStrandDesignTool::GetMaxPermanentStrands()
{
   return m_MaxPermStrandCount;
}

StrandIndexType pgsRaisedStraightStrandDesignTool::GetNextNumPermanentStrands(StrandIndexType prevNum)
{
   return m_StrandResequencer.GetNextNumPermanentStrands(prevNum);
}

StrandIndexType pgsRaisedStraightStrandDesignTool::GetPreviousNumPermanentStrands(StrandIndexType nextNum)
{
   return m_StrandResequencer.GetPreviousNumPermanentStrands(nextNum);
}

bool pgsRaisedStraightStrandDesignTool::IsValidNumPermanentStrands(StrandIndexType num)
{
   return m_StrandResequencer.IsValidNumPermanentStrands(num);
}

StrandIndexType pgsRaisedStraightStrandDesignTool::GetMinimumPermanentStrands() const
{
   // make sure design count always includes our raised locations and a couple more
   return 2 + m_UsedRaisedStrandLocations*2;
}

void pgsRaisedStraightStrandDesignTool::ComputeNumStrands(StrandIndexType numPermStrands, StrandIndexType* pNStraightStrands, StrandIndexType* pNHarpedStrands) const
{
   return m_StrandResequencer.ComputeNumStrands(numPermStrands, pNStraightStrands, pNHarpedStrands);
}

ConfigStrandFillVector pgsRaisedStraightStrandDesignTool::CreateStrandFill(GirderLibraryEntry::psStrandType type, StrandIndexType numStrands) const
{
   return m_StrandResequencer.CreateStrandFill(type, numStrands);
}

