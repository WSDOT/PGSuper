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

#include <PgsExt\PgsExtLib.h>
#include <PgsExt\PoiMgr.h>
#include <algorithm>
#include <iterator>


/****************************************************************************
CLASS
   pgsPoiMgr
****************************************************************************/



// Predicate STL objects.
class TestEqual
{
public:
   TestEqual(const pgsPointOfInterest& poi) : m_Poi(poi) {}

   bool operator()(const std::unique_ptr<pgsPointOfInterest>& pOther) const
   {
      return m_Poi == *(pOther.get());
   }

private:
   const pgsPointOfInterest& m_Poi;
};


class RemoveTest
{
public:
   RemoveTest(PoiAttributeType targetAttribute,PoiAttributeType exceptAttribute) : m_TargetAttribute(targetAttribute), m_ExceptAttribute(exceptAttribute) {}

   bool operator()(std::unique_ptr<pgsPointOfInterest>& poi)
   {
      // return true if the poi has the target attribute, except if it has the exception attribute
      if ( poi->HasAttribute(m_TargetAttribute) && !poi->HasAttribute(m_ExceptAttribute) )
      {
         // the poi can possibly be removed... more testing is needed
         // at minimum we can remove the target attribute, but the actual poi may need to
         // stay in the container (example, removing lifting 10th points pois and the point
         // of prestress transfer is at a tenth poi.. we want to remove the lifting attributes
         // but retain the poi)
         if ( pgsPointOfInterest::IsReferenceAttribute(m_TargetAttribute) )
         {
            poi->RemoveAttributes(m_TargetAttribute | POI_TENTH_POINTS);
         }
         else
         {
            poi->RemoveAttributes(m_TargetAttribute);
         }

         if ( poi->GetNonReferencedAttributes() != 0 )
         {
            return false; // has attributes, don't remove the poi from the container
         }

         PoiAttributeType refAttribute[] = 
         {
            POI_RELEASED_SEGMENT, 
            POI_LIFT_SEGMENT, 
            POI_STORAGE_SEGMENT,
            POI_HAUL_SEGMENT, 
            POI_ERECTED_SEGMENT, 
            POI_SPAN 
         };
         for ( int i = 0; i < 6; i++ )
         {
            if (poi->GetReferencedAttributes(refAttribute[i]) != 0 )
            {
               return false; // has attributes, don't remove the poi from the container
            }
         }

         return true; // no attributes, remove poi from the container
      }

      return false;
   }

private:
   PoiAttributeType m_TargetAttribute;
   PoiAttributeType m_ExceptAttribute;
};

PoiIDType pgsPoiMgr::ms_NextID = 0;


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
pgsPoiMgr::pgsPoiMgr()
{
}

pgsPoiMgr::~pgsPoiMgr()
{
   RemoveAll();
}

PoiIDType pgsPoiMgr::AddPointOfInterest(const pgsPointOfInterest& poi)
{
   ATLASSERT(poi.GetID() == INVALID_ID); // why are you adding a POI that has already been assigned an ID? ID's get assigned when the POI is added

   // get the POI container for the segment key
   const CSegmentKey& segmentKey = poi.GetSegmentKey();
   auto& poiContainer = GetPoiContainer(segmentKey);

#if defined _DEBUG
   // verify that the POI is not currently in the container
   auto found(std::find_if(std::begin(poiContainer), std::end(poiContainer), [&poi](auto& p) {return p->GetID() == poi.GetID(); }));
   ATLASSERT(found == std::end(poiContainer));
#endif

   // first see if we have an existing poi at this location. Just merge attributes into existing if we do
   auto iter(std::begin(poiContainer));
   auto end(std::end(poiContainer));
   for ( ; iter != end; iter++)
   {
      auto& pCurrentPoi(*iter);
      ATLASSERT(pCurrentPoi->GetSegmentKey().IsEqual(segmentKey));

      if (pCurrentPoi->AtSamePlace(poi) )
      {
         // the new poi is at the same place as a previously stored poi
         // try to merge them together
         if (pCurrentPoi->MergeAttributes(poi))
         {
            // merge was successful, we are done
            return pCurrentPoi->m_ID; 
         }
         else
         {
            // could not be merged

            // sometimes there are multiple POI at the same location... make sure
            // the next poi is not at the same location... if it is, continue because
            // the new poi might be able to be merged with it, otherwise bail out because
            // there aren't any other POIs at this same place
            auto iter_next = iter + 1;
            if (iter_next != end && !poi.AtSamePlace(*(*(iter_next))))
            {
               break; // poi's can't be merged... just break out of here and continue
            }
         }
      }

      if (poi.GetDistFromStart() < pCurrentPoi->GetDistFromStart())
      {
         // the containers are sorted... once the current POI goes past the new poi there wont be a match so break the loop
         break;
      }
   } // next poi

   // if we get this far, there is a new poi to go into the poi container

   PoiIDType id = poi.m_ID;
   if ( id == INVALID_ID )
   {
      ATLASSERT(ms_NextID != MAX_ID); // assert if we are about to roll over the id
      id = ms_NextID++;
   }
   else
   {
      // if in new POI has already been assigned an ID
      // make sure the next POI ID won't conflict with it
      ms_NextID = Max(ms_NextID,id+1);
   }

#if defined _DEBUG
   // verify there isn't a POI in the container with the new POI's ID
   found = std::find_if(std::begin(poiContainer), std::end(poiContainer), [id](auto& p) {return p->GetID() == id; });
   ATLASSERT(found == std::end(poiContainer));
#endif
   
   // create the new POI and assign it's ID
   poiContainer.emplace_back(std::make_unique<pgsPointOfInterest>(poi));
   poiContainer.back()->m_ID = id;

   // sort POIs, not their pointers
   std::sort(std::begin(poiContainer), std::end(poiContainer), [](auto& a, auto& b) {return *a < *b;});

   return id;
}

bool pgsPoiMgr::RemovePointOfInterest(const pgsPointOfInterest& poi)
{
   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   auto& poiContainer = GetPoiContainer(segmentKey);

   PoiContainer::iterator found = std::find_if(std::begin(poiContainer), std::end(poiContainer), TestEqual(poi));

   if ( found != std::end(poiContainer) )
   {
      poiContainer.erase(found);
      return true;
   }

   return false;
}

bool pgsPoiMgr::RemovePointOfInterest(PoiIDType poiID)
{
   ATLASSERT(poiID != INVALID_ID);
   if ( poiID == INVALID_ID )
   {
      return false;
   }

   for ( auto& poiData : m_PoiData)
   {
      PoiContainer& poiContainer(poiData.second);

      auto found( std::find_if(std::begin(poiContainer),std::end(poiContainer), [poiID](auto& p) {return p->GetID() == poiID; }) );
      if ( found != std::end(poiContainer) )
      {
         poiContainer.erase(found);
         return true; // found it... stop searching
      }
   }

   return false; // never found it
}

void pgsPoiMgr::RemovePointsOfInterest(PoiAttributeType targetAttribute,PoiAttributeType exceptionAttribute)
{
   for (auto& poiData : m_PoiData)
   {
      PoiContainer& poiContainer(poiData.second);
      auto newEnd( std::remove_if(std::begin(poiContainer),std::end(poiContainer),RemoveTest(targetAttribute,exceptionAttribute)) );
      poiContainer.erase(newEnd,std::end(poiContainer));
   }
}

void pgsPoiMgr::RemoveAll()
{
   m_PoiData.clear();
   m_DuctBoundaries.clear();
}

void pgsPoiMgr::AddDuctBoundary(const CGirderKey& girderKey, DuctIndexType ductIdx, PoiIDType startPoiID, PoiIDType endPoiID)
{
   auto result = m_DuctBoundaries.emplace(girderKey, ductIdx, startPoiID, endPoiID);
   ATLASSERT(result.second == true);

#if defined _DEBUG
   const pgsPointOfInterest& startPoi = GetPointOfInterest(startPoiID);
   ATLASSERT(startPoi.HasAttribute(POI_DUCT_START));

   const pgsPointOfInterest& endPoi = GetPointOfInterest(endPoiID);
   ATLASSERT(endPoi.HasAttribute(POI_DUCT_END));
#endif
}

void pgsPoiMgr::GetDuctBoundary(const CGirderKey& girderKey, DuctIndexType ductIdx, PoiIDType* pStartPoiID, PoiIDType* pEndPoiID) const
{
   auto found = m_DuctBoundaries.find(DuctBoundaryRecord(girderKey, ductIdx, INVALID_ID, INVALID_ID));
   ATLASSERT(found != m_DuctBoundaries.end());
   *pStartPoiID = found->startID;
   *pEndPoiID = found->endID;

#if defined _DEBUG
   const pgsPointOfInterest& startPoi = GetPointOfInterest(*pStartPoiID);
   ATLASSERT(startPoi.HasAttribute(POI_DUCT_START));

   const pgsPointOfInterest& endPoi = GetPointOfInterest(*pEndPoiID);
   ATLASSERT(endPoi.HasAttribute(POI_DUCT_END));
#endif
}

pgsPointOfInterest pgsPoiMgr::GetPointOfInterest(const CSegmentKey& segmentKey,Float64 Xpoi) const
{
   // there may not be an actual poi at (segmentKey,Xpoi)... this is why we need to return the
   // poi by value
   pgsPointOfInterest poi(segmentKey,Xpoi);

   const auto& poiContainer = GetPoiContainer(segmentKey);

   auto begin(std::begin(poiContainer));
   auto end(std::end(poiContainer));

   // find all poi that are at the same place
   auto found = begin;
   std::vector<pgsPointOfInterest> vPoi;
   while ( found != end )
   {
      found = std::find_if(begin, end, [&poi](const auto& pPoi) {return pPoi->AtSamePlace(poi); });
      if ( found != end )
      {
         vPoi.push_back(*(*found));
         begin = found+1;
      }
   }

   // no poi at the same place, just return the poi created on the fly above
   if ( vPoi.size() == 0 )
   {
      return poi;
   }

   // figure out which poi is closest to Xpoi
   auto poi_iter(std::begin(vPoi));
   auto poi_iter_end(std::end(vPoi));
   poi = (*poi_iter);
   Float64 minDist = fabs(Xpoi - poi.GetDistFromStart());
   poi_iter++;
   for ( ; poi_iter != poi_iter_end; poi_iter++ )
   {
      Float64 dist = fabs(Xpoi - poi_iter->GetDistFromStart());
      if ( dist < minDist )
      {
         minDist = dist;
         poi = (*poi_iter);
      }
   }
   return poi;
}

const pgsPointOfInterest& pgsPoiMgr::GetNearestPointOfInterest(const CSegmentKey& segmentKey,Float64 Xpoi) const
{
   auto& poiContainer = GetPoiContainer(segmentKey);

   if ( poiContainer.empty() )
   {
      return m_DefaultPoi;
   }

   // is the Xpoi before the start?
   // this happens if a poi is referenced to this segment, but
   // located in the previous closure joint
   if ( ::IsLE(Xpoi,poiContainer.front()->GetDistFromStart()) )
   {
      return *poiContainer.front();
   }

   auto iter1(std::begin(poiContainer));
   auto iter2(iter1+1);
   auto end2(std::end(poiContainer));
   for ( ; iter2 != end2; iter1++, iter2++ )
   {
      auto& ptrPrevPoi(*iter1);
      auto& ptrNextPoi(*iter2);
      const pgsPointOfInterest& prevPOI(*ptrPrevPoi);
      const pgsPointOfInterest& nextPOI(*ptrNextPoi);

      ATLASSERT( prevPOI.GetSegmentKey() == segmentKey);
      ATLASSERT( nextPOI.GetSegmentKey() == segmentKey);

      if ( InRange(prevPOI.GetDistFromStart(),Xpoi,nextPOI.GetDistFromStart()) )
      {
         Float64 d1 = Xpoi - prevPOI.GetDistFromStart();
         Float64 d2 = nextPOI.GetDistFromStart() - Xpoi;

         if ( d1 < d2 )
         {
            return prevPOI;
         }
         else
         {
            return nextPOI;
         }
      }
   }

   ATLASSERT(false); // nearest poi not found
   return m_DefaultPoi;
}

const pgsPointOfInterest& pgsPoiMgr::GetPrevPointOfInterest(PoiIDType poiID,PoiAttributeType attrib,Uint32 mode) const
{
   if ( poiID == INVALID_ID )
   {
      return m_DefaultPoi;
   }

   const pgsPointOfInterest& currentPoi = GetPointOfInterest(poiID);
   CSegmentKey segmentKey(currentPoi.GetSegmentKey()); // want a copy, not a reference... we change the segment key below

   const auto& poiContainer = GetPoiContainer(segmentKey);
   const auto* pPoiContainer = &poiContainer;

   auto begin(std::begin(*pPoiContainer));
   auto end(std::end(*pPoiContainer));
   auto found( std::find_if(begin, end, [poiID](auto& p) {return p->GetID() == poiID; }) );
   ATLASSERT(found != end);
   if ( found == begin )
   {
      if ( segmentKey.segmentIndex == 0 )
      {
         // at the beginning so we can't back up one poi
         return m_DefaultPoi;
      }
      else
      {
         // the previous poi is in the container for the previous segment
         CSegmentKey prevSegmentKey(segmentKey);
         prevSegmentKey.segmentIndex--;
         const auto& poiContainer = GetPoiContainer(prevSegmentKey);
         pPoiContainer = &poiContainer;
         found = std::end(*pPoiContainer)-1; // make found point to the first candidate POI
      }
   }
   else
   {
      // back up one so found points to the first candidate POI
      found--;
   }

   if ( attrib == 0 )
   {
      // unconditionally want the previous poi
      return *(*found);
   }

   // work backwards through the container from "found" to the first poi which is at the end of the reverse collection
   while ( true )
   {
      PoiContainer::const_reverse_iterator iter(found);
      PoiContainer::const_reverse_iterator rend(pPoiContainer->rend());
      for ( ; iter != rend; iter++ )
      {
         const auto& thisPoi(*iter);
         ATLASSERT(thisPoi->GetSegmentKey().girderIndex == segmentKey.girderIndex);
         if ( mode == POIMGR_OR ? OrAttributeEvaluation(*thisPoi,attrib) : AndAttributeEvaluation(*thisPoi,attrib) )
         {
            return *thisPoi;
         }
      }

      if ( segmentKey.segmentIndex == 0 )
      {
         // we are at the start and the previous POI wasn't found... there isn't a previous one
         return m_DefaultPoi;
      }
      else
      {
         // the container was exhausted... the poi must be in a previous container
         segmentKey.segmentIndex--;
         const auto& poiContainer = GetPoiContainer(segmentKey);
         pPoiContainer = &poiContainer;
         found = std::end(*pPoiContainer)-1; // make found point to the next candidate POI
      }
   }

   ATLASSERT(false); // should never get here
   return m_DefaultPoi;
}

const pgsPointOfInterest& pgsPoiMgr::GetNextPointOfInterest(PoiIDType poiID,PoiAttributeType attrib,Uint32 mode) const
{
   if ( poiID == INVALID_ID )
   {
      return m_DefaultPoi;
   }

   const pgsPointOfInterest& currentPoi = GetPointOfInterest(poiID);
   CSegmentKey segmentKey(currentPoi.GetSegmentKey()); // want a copy, not a ref... segmentKey gets changed below

   const auto& poiContainer = GetPoiContainer(segmentKey);
   const auto* pPoiContainer = &poiContainer;

   auto begin(std::begin(*pPoiContainer));
   auto end(std::end(*pPoiContainer));
   auto found( std::find_if(begin, end, [poiID](auto& p) {return p->GetID() == poiID; }) );
   ATLASSERT(found != end);
   if ( found == end-1 )
   {
      // we are at the last poi in the container and can't go forward
      // look in the container for the next segment
      CSegmentKey nextSegmentKey(segmentKey);
      nextSegmentKey.segmentIndex++;
      const auto& poiContainer = GetPoiContainer(nextSegmentKey);
      pPoiContainer = &poiContainer;
      if (pPoiContainer->size() == 0 )
      {
         // there isn't a container for the next segment so we can't go any further
         return m_DefaultPoi;
      }
      else
      {
         found = std::end(*pPoiContainer)-1; // make found point to the first candidate POI
      }
   }
   else
   {
      // go forward one so found points to the first candidate POI
      found++;
   }

   if ( attrib == 0 )
   {
      // unconditionally want the previous poi
      return *(*found);
   }

   // work forward through the container from "found" to the end
   while ( true )
   {
      auto iter(found);
      auto end(std::end(*pPoiContainer));
      for ( ; iter != end; iter++ )
      {
         const auto& thisPoi(*iter);
         ATLASSERT(thisPoi->GetSegmentKey().girderIndex == segmentKey.girderIndex);
         if ( mode == POIMGR_OR ? OrAttributeEvaluation(*thisPoi,attrib) : AndAttributeEvaluation(*thisPoi,attrib) )
         {
            return *thisPoi;
         }
      }

      // the container was exhausted... try the container for the next segment
      segmentKey.segmentIndex++;
      const auto& poiContainer = GetPoiContainer(segmentKey);
      pPoiContainer = &poiContainer;
      if (pPoiContainer->size() == 0 )
      {
         // there isn't a next poi
         return m_DefaultPoi;
      }
      else
      {
         found = std::begin(*pPoiContainer); // make found point to the next candidate POI
      }
   }

   ATLASSERT(false); // should never get here
   return m_DefaultPoi;
}

const pgsPointOfInterest& pgsPoiMgr::GetPointOfInterest(PoiIDType poiID) const
{
   if ( poiID == INVALID_ID )
   {
      return m_DefaultPoi;
   }

   for( const auto& poiData : m_PoiData )
   {
      const PoiContainer& poiContainer(poiData.second);

      auto begin(std::begin(poiContainer));
      auto end(std::end(poiContainer));
      auto found( std::find_if(begin, end, [poiID](auto& p) {return p->GetID() == poiID; } ) );
      if ( found != end )
      {
         return *(*found);
      }
   }

   return m_DefaultPoi;
}

void pgsPoiMgr::GetPointsOfInterest(const CSegmentKey& segmentKey,PoiAttributeType attrib,Uint32 mode,PoiList* pPois) const
{
   if ( mode == POIMGR_AND )
   {
      AndFind(segmentKey,attrib,pPois);
   }
   else
   {
      OrFind(segmentKey,attrib,pPois);
   }
}

void pgsPoiMgr::GetPointsOfInterest(const CSegmentKey& segmentKey,PoiList* pPoiList) const
{
   for (const auto& poiData : m_PoiData)
   {
      if (segmentKey == poiData.first) // NOTE: segmentKey is the one that could have ALL_GIRDER, etc so it has to be the LHS operand
      {
         const PoiContainer& poiContainer(poiData.second);
         for (const auto& pPoi : poiContainer)
         {
            pPoiList->emplace_back(*pPoi.get());
         }
      }
   }

//   std::sort(poiList.begin(), poiList.end(), [](const auto& a, const auto& b) {return *a.get() < *b.get();});
   ATLASSERT(std::is_sorted(std::begin(*pPoiList), std::end(*pPoiList), [](const pgsPointOfInterest& a, const pgsPointOfInterest& b) {return a < b; }));
}

void pgsPoiMgr::MergePoiLists(const PoiList& list1, const PoiList& list2,PoiList* pPoiList) const
{
   *pPoiList = list1;
   pPoiList->insert(std::end(*pPoiList), std::begin(list2),std::end(list2));
   SortPoiList(pPoiList);
}

void pgsPoiMgr::SortPoiList(PoiList* pPoiList) const
{
   std::sort(std::begin(*pPoiList), std::end(*pPoiList), [](const pgsPointOfInterest& a, const pgsPointOfInterest& b) {return a < b;});
   pPoiList->erase(std::unique(std::begin(*pPoiList), std::end(*pPoiList), [](const pgsPointOfInterest& a, const pgsPointOfInterest& b) {return a == b; }), std::end(*pPoiList));

   ATLASSERT(std::is_sorted(std::begin(*pPoiList), std::end(*pPoiList), [](const pgsPointOfInterest& a, const pgsPointOfInterest& b) {return a < b; })); // must be sorted
   ATLASSERT(std::adjacent_find(std::begin(*pPoiList), std::end(*pPoiList), [](const pgsPointOfInterest& a, const pgsPointOfInterest& b) {return a == b; }) == std::end(*pPoiList)); // can't be duplicates
}

bool pgsPoiMgr::ReplacePointOfInterest(PoiIDType ID,const pgsPointOfInterest& poi)
{
   for ( auto& poiData : m_PoiData)
   {
      PoiContainer& poiContainer(poiData.second);

      auto begin(std::begin(poiContainer));
      auto end(std::end(poiContainer));
      auto found(std::find_if(begin, end, [ID](auto& p) {return p->GetID() == ID; }) );
      if ( found != end )
      {
         ATLASSERT((*found)->GetSegmentKey() == poi.GetSegmentKey()); // the replacement poi must be on the same segment
         *(*found) = poi;
         (*found)->m_ID = ID;
         return true;
      }
   }

   return false;
}

//======================== ACCESS     =======================================
IndexType pgsPoiMgr::GetPointOfInterestCount() const
{
   IndexType nPoi = 0;
   for( const auto& poiData : m_PoiData)
   {
      const PoiContainer& poiContainer(poiData.second);
      nPoi += poiContainer.size();
   }

   return nPoi;
}

void pgsPoiMgr::GetTenthPointPOIs(PoiAttributeType reference,const CSegmentKey& segmentKey,PoiList* pPois) const
{
   pPois->reserve(pPois->size() + 11);

   if (segmentKey.groupIndex == ALL_GROUPS || segmentKey.girderIndex == ALL_GIRDERS || segmentKey.segmentIndex == ALL_SEGMENTS)
   {
      // caller is asking for something like 10th points in all segments for all girders in group 1....
      for (const auto& poiData : m_PoiData)
      {
         const auto& poiContainer(poiData.second);
         for (const auto& poi : poiContainer)
         {
            if (poi->GetSegmentKey() == segmentKey && poi->IsTenthPoint(reference))
            {
               pPois->push_back(*(poi.get()));
            }
         }
      }
   }
   else
   {
      // we can limit our search to a specific container because the caller used a specific segmentKey
      const auto& poiContainer = GetPoiContainer(segmentKey);
      for (const auto& poi : poiContainer)
      {
         if (poi->IsTenthPoint(reference))
         {
            pPois->push_back(*(poi.get()));
         }
      }
   }
}

void pgsPoiMgr::GetPointsOfInterestInRange(const CSegmentKey& segmentKey, Float64 xMin, Float64 xMax, PoiList* pPois) const
{
   const auto& poiContainer = GetPoiContainer(segmentKey);
   for ( const auto& poi : poiContainer)
   {
      if ( ::InRange(xMin,poi->GetDistFromStart(),xMax) )
      {
          pPois->push_back(*(poi.get()));
      }
   }
}


void pgsPoiMgr::AndFind(const CSegmentKey& segmentKey, PoiAttributeType attrib, PoiList* pPois) const
{
   if (segmentKey.groupIndex == ALL_GROUPS || segmentKey.girderIndex == ALL_GIRDERS || segmentKey.segmentIndex == ALL_SEGMENTS)
   {
      for (const auto& poiData : m_PoiData)
      {
         if (segmentKey == poiData.first) // NOTE: segmentKey is the one that could have ALL_GIRDER, etc so it has to be the LHS operand
         {
            const auto& poiContainer(poiData.second);
            for (const auto& pPoi : poiContainer)
            {
               if (AndFind(*pPoi, segmentKey, attrib))
               {
                  // poi has desired attributes - keep it
                  pPois->push_back(*(pPoi.get()));
               }
            }
         }
      }
   }
   else
   {
      const auto& poiContainer = GetPoiContainer(segmentKey);
      for (const auto& pPoi : poiContainer)
      {
         if (AndFind(*pPoi, segmentKey, attrib))
         {
            // poi has desired attributes - keep it
            pPois->push_back(*(pPoi.get()));
         }
      }
   }
}

bool pgsPoiMgr::AndFind(const pgsPointOfInterest& poi,const CSegmentKey& segmentKey,PoiAttributeType attrib) const
{
   // TRICKY CODE
   // This if expression first check to make sure we have the same segment, then
   // it checks if each flag in attrib is set. If it is, the corresponding flag in poi must
   // also be set, otherwise, the test is irrelevant (and always passes as indicated by the true)
   //
   // segmentKey can match exactly or match with ALL_XXX constants (kind of like a wild card)

   // If the search attribute has referenced attributes, get the reference type
   PoiAttributeType targetReference = pgsPointOfInterest::GetReference(attrib);

   // NOTE: you can only search for POIs with referenced or non-referenced attributes. You can't search
   // for both types at the same time

   if ( segmentKey == poi.GetSegmentKey() )
   {
      // segmentKeys match....
      return AndAttributeEvaluation(poi,attrib);
   }

   return false;
}

bool pgsPoiMgr::AndAttributeEvaluation(const pgsPointOfInterest& poi,PoiAttributeType attrib) const
{
   // TRICKY CODE
   // This if expression checks if each flag in attrib is set. If it is, the corresponding flag in poi must
   // also be set, otherwise, the test is irrelevant (and always passes as indicated by the true)

   // If the search attribute has referenced attributes, get the reference type
   PoiAttributeType targetReference = pgsPointOfInterest::GetReference(attrib);

   // NOTE: you can only search for POIs with referenced or non-referenced attributes. You can't search
   // for both types at the same time

   if ( targetReference == 0 )
   {
      // searching for a non-referenced attribute
      if ((WBFL::System::Flags<PoiAttributeType>::IsSet(attrib,POI_CRITSECTSHEAR1)           ? poi.HasAttribute(POI_CRITSECTSHEAR1)           : true) &&
          (WBFL::System::Flags<PoiAttributeType>::IsSet(attrib,POI_CRITSECTSHEAR2)           ? poi.HasAttribute(POI_CRITSECTSHEAR2)           : true) &&
          (WBFL::System::Flags<PoiAttributeType>::IsSet(attrib,POI_HARPINGPOINT)             ? poi.IsHarpingPoint()                           : true) &&
          (WBFL::System::Flags<PoiAttributeType>::IsSet(attrib,POI_CONCLOAD)                 ? poi.IsConcentratedLoad()                       : true) &&
          (WBFL::System::Flags<PoiAttributeType>::IsSet(attrib,POI_DIAPHRAGM)                ? poi.HasAttribute(POI_DIAPHRAGM)                : true) &&
          (WBFL::System::Flags<PoiAttributeType>::IsSet(attrib,POI_H)                        ? poi.IsAtH()                                    : true) &&
          (WBFL::System::Flags<PoiAttributeType>::IsSet(attrib,POI_15H)                      ? poi.IsAt15H()                                  : true) &&
          (WBFL::System::Flags<PoiAttributeType>::IsSet(attrib,POI_PSXFER)                   ? poi.HasAttribute(POI_PSXFER)                   : true) &&
          (WBFL::System::Flags<PoiAttributeType>::IsSet(attrib,POI_DECKBARCUTOFF)            ? poi.HasAttribute(POI_DECKBARCUTOFF)            : true) &&
          (WBFL::System::Flags<PoiAttributeType>::IsSet(attrib,POI_BARCUTOFF)                ? poi.HasAttribute(POI_BARCUTOFF)                : true) &&
          (WBFL::System::Flags<PoiAttributeType>::IsSet(attrib,POI_BARDEVELOP)               ? poi.HasAttribute(POI_BARDEVELOP)               : true) &&
          (WBFL::System::Flags<PoiAttributeType>::IsSet(attrib,POI_SECTCHANGE_RIGHTFACE)     ? poi.HasAttribute(POI_SECTCHANGE_RIGHTFACE)     : true) &&
          (WBFL::System::Flags<PoiAttributeType>::IsSet(attrib,POI_SECTCHANGE_LEFTFACE)      ? poi.HasAttribute(POI_SECTCHANGE_LEFTFACE)      : true) &&
          (WBFL::System::Flags<PoiAttributeType>::IsSet(attrib,POI_SECTCHANGE_TRANSITION)    ? poi.HasAttribute(POI_SECTCHANGE_TRANSITION)    : true) &&
          (WBFL::System::Flags<PoiAttributeType>::IsSet(attrib,POI_FACEOFSUPPORT)            ? poi.HasAttribute(POI_FACEOFSUPPORT)            : true) &&
          (WBFL::System::Flags<PoiAttributeType>::IsSet(attrib,POI_INTERMEDIATE_PIER)        ? poi.HasAttribute(POI_INTERMEDIATE_PIER)        : true) &&
          (WBFL::System::Flags<PoiAttributeType>::IsSet(attrib,POI_BOUNDARY_PIER)            ? poi.HasAttribute(POI_BOUNDARY_PIER)            : true) &&
          (WBFL::System::Flags<PoiAttributeType>::IsSet(attrib,POI_ABUTMENT)                 ? poi.HasAttribute(POI_ABUTMENT)                 : true) &&
          (WBFL::System::Flags<PoiAttributeType>::IsSet(attrib,POI_STIRRUP_ZONE)             ? poi.HasAttribute(POI_STIRRUP_ZONE)             : true) &&
          (WBFL::System::Flags<PoiAttributeType>::IsSet(attrib,POI_INTERMEDIATE_TEMPSUPPORT) ? poi.HasAttribute(POI_INTERMEDIATE_TEMPSUPPORT) : true) &&
          (WBFL::System::Flags<PoiAttributeType>::IsSet(attrib,POI_CLOSURE)                  ? poi.HasAttribute(POI_CLOSURE)                  : true) &&
          (WBFL::System::Flags<PoiAttributeType>::IsSet(attrib,POI_START_FACE)               ? poi.HasAttribute(POI_START_FACE)               : true) &&
          (WBFL::System::Flags<PoiAttributeType>::IsSet(attrib,POI_END_FACE)                 ? poi.HasAttribute(POI_END_FACE)                 : true) &&
          (WBFL::System::Flags<PoiAttributeType>::IsSet(attrib, POI_DUCT_START)              ? poi.HasAttribute(POI_DUCT_START)               : true) &&
          (WBFL::System::Flags<PoiAttributeType>::IsSet(attrib, POI_DUCT_END)                ? poi.HasAttribute(POI_DUCT_END)                 : true) &&
          (WBFL::System::Flags<PoiAttributeType>::IsSet(attrib,POI_CASTING_BOUNDARY_START)   ? poi.HasAttribute(POI_CASTING_BOUNDARY_START)   : true) &&
          (WBFL::System::Flags<PoiAttributeType>::IsSet(attrib,POI_CASTING_BOUNDARY_END)     ? poi.HasAttribute(POI_CASTING_BOUNDARY_END)     : true)
         )
      {
         // This poi matches the selection criteria.
         return true;
      }
   }
   else
   {
      // searching for a referenced attribute
      if ( attrib == targetReference )
      {
         // didn't specify which referenced attribute is being searched for... can't match them all
         return false;
      }

      if (
          (WBFL::System::Flags<PoiAttributeType>::IsSet(attrib,POI_PICKPOINT) ? poi.HasAttribute(targetReference | POI_PICKPOINT)   : true) &&
          (WBFL::System::Flags<PoiAttributeType>::IsSet(attrib,POI_BUNKPOINT) ? poi.HasAttribute(targetReference | POI_BUNKPOINT)   : true) &&
          (WBFL::System::Flags<PoiAttributeType>::IsSet(attrib,POI_0L)  ? poi.IsTenthPoint(targetReference) == 1  : true) &&
          (WBFL::System::Flags<PoiAttributeType>::IsSet(attrib,POI_1L)  ? poi.IsTenthPoint(targetReference) == 2  : true) &&
          (WBFL::System::Flags<PoiAttributeType>::IsSet(attrib,POI_2L)  ? poi.IsTenthPoint(targetReference) == 3  : true) &&
          (WBFL::System::Flags<PoiAttributeType>::IsSet(attrib,POI_3L)  ? poi.IsTenthPoint(targetReference) == 4  : true) &&
          (WBFL::System::Flags<PoiAttributeType>::IsSet(attrib,POI_4L)  ? poi.IsTenthPoint(targetReference) == 5  : true) &&
          (WBFL::System::Flags<PoiAttributeType>::IsSet(attrib,POI_5L)  ? poi.IsTenthPoint(targetReference) == 6  : true) &&
          (WBFL::System::Flags<PoiAttributeType>::IsSet(attrib,POI_6L)  ? poi.IsTenthPoint(targetReference) == 7  : true) &&
          (WBFL::System::Flags<PoiAttributeType>::IsSet(attrib,POI_7L)  ? poi.IsTenthPoint(targetReference) == 8  : true) &&
          (WBFL::System::Flags<PoiAttributeType>::IsSet(attrib,POI_8L)  ? poi.IsTenthPoint(targetReference) == 9  : true) &&
          (WBFL::System::Flags<PoiAttributeType>::IsSet(attrib,POI_9L)  ? poi.IsTenthPoint(targetReference) == 10 : true) &&
          (WBFL::System::Flags<PoiAttributeType>::IsSet(attrib,POI_10L) ? poi.IsTenthPoint(targetReference) == 11 : true)
         )
      {
         // This poi matches the selection criteria.
         return true;
      }
   }

   return false;
}

void pgsPoiMgr::OrFind(const CSegmentKey& segmentKey,PoiAttributeType attrib,PoiList* pPois) const
{
   if (segmentKey.groupIndex == ALL_GROUPS || segmentKey.girderIndex == ALL_GIRDERS || segmentKey.segmentIndex == ALL_SEGMENTS)
   {
      for (const auto& poiData : m_PoiData)
      {
         if (segmentKey == poiData.first) // NOTE: segmentKey is the one that could have ALL_GIRDER, etc so it has to be the LHS operand
         {
            const auto& poiContainer(poiData.second);
            for (const auto& pPoi : poiContainer)
            {
               if (OrFind(*pPoi, segmentKey, attrib))
               {
                  pPois->push_back(*(pPoi.get()));
               }
            }
         }
      }
   }
   else
   {
      const auto& poiContainer = GetPoiContainer(segmentKey);
      for (const auto& pPoi : poiContainer)
      {
         if (OrFind(*pPoi, segmentKey, attrib))
         {
            // poi has desired attributes - keep it
            pPois->push_back(*(pPoi.get()));
         }
      }
   }
}

bool pgsPoiMgr::OrFind(const pgsPointOfInterest& poi,const CSegmentKey& segmentKey,PoiAttributeType attrib) const
{
   // TRICKY CODE
   // This if expression first check to make sure we have the same segment, then
   // it checks if each flag in attrib is set. If it is, the corresponding flag in the poi must
   // be set, otherwise, the test is irrelevant (and always passes as indicated by the true)

   // segmentKey can match exactly or match with ALL_XXX constants (kind of like a wild card)

   if ( segmentKey != poi.GetSegmentKey() )
   {
      return false;
   }

   return OrAttributeEvaluation(poi,attrib);
}

bool pgsPoiMgr::OrAttributeEvaluation(const pgsPointOfInterest& poi,PoiAttributeType attrib) const
{
   // TRICKY CODE
   // This if expression checks if each flag in attrib is set. If it is, the corresponding flag in the poi must
   // be set, otherwise, the test is irrelevant (and always passes as indicated by the true)

   if ( attrib == 0 )
   {
      return true; // always match if we don't care what the attributes are
   }

   // If the search attribute has referenced attributes, get the reference type
   PoiAttributeType targetReference = pgsPointOfInterest::GetReference(attrib);

   // NOTE: you can only search for POIs with referenced or non-referenced attributes. You can't search
   // for both types at the same time

   if ( targetReference == 0 )
   {
      // searching for a non-referenced attribute
      if (
       (WBFL::System::Flags<PoiAttributeType>::IsSet(attrib,POI_CRITSECTSHEAR1)           ? poi.HasAttribute(POI_CRITSECTSHEAR1)           : false) ||
       (WBFL::System::Flags<PoiAttributeType>::IsSet(attrib,POI_CRITSECTSHEAR2)           ? poi.HasAttribute(POI_CRITSECTSHEAR2)           : false) ||
       (WBFL::System::Flags<PoiAttributeType>::IsSet(attrib,POI_HARPINGPOINT)             ? poi.IsHarpingPoint()                           : false) ||
       (WBFL::System::Flags<PoiAttributeType>::IsSet(attrib,POI_CONCLOAD)                 ? poi.IsConcentratedLoad()                       : false) ||
       (WBFL::System::Flags<PoiAttributeType>::IsSet(attrib,POI_DIAPHRAGM)                ? poi.HasAttribute(POI_DIAPHRAGM)                : false) ||
       (WBFL::System::Flags<PoiAttributeType>::IsSet(attrib,POI_H)                        ? poi.IsAtH()                                    : false) ||
       (WBFL::System::Flags<PoiAttributeType>::IsSet(attrib,POI_15H)                      ? poi.IsAt15H()                                  : false) ||
       (WBFL::System::Flags<PoiAttributeType>::IsSet(attrib,POI_DEBOND)                   ? poi.HasAttribute(POI_DEBOND)                   : false) ||
       (WBFL::System::Flags<PoiAttributeType>::IsSet(attrib,POI_PSXFER)                   ? poi.HasAttribute(POI_PSXFER)                   : false) ||
       (WBFL::System::Flags<PoiAttributeType>::IsSet(attrib,POI_DECKBARCUTOFF)            ? poi.HasAttribute(POI_DECKBARCUTOFF)            : false) ||
       (WBFL::System::Flags<PoiAttributeType>::IsSet(attrib,POI_BARCUTOFF)                ? poi.HasAttribute(POI_BARCUTOFF)                : false) ||
       (WBFL::System::Flags<PoiAttributeType>::IsSet(attrib,POI_BARDEVELOP)               ? poi.HasAttribute(POI_BARDEVELOP)               : false) ||
       (WBFL::System::Flags<PoiAttributeType>::IsSet(attrib,POI_SECTCHANGE_RIGHTFACE)     ? poi.HasAttribute(POI_SECTCHANGE_RIGHTFACE)     : false) ||
       (WBFL::System::Flags<PoiAttributeType>::IsSet(attrib,POI_SECTCHANGE_LEFTFACE)      ? poi.HasAttribute(POI_SECTCHANGE_LEFTFACE)      : false) ||
       (WBFL::System::Flags<PoiAttributeType>::IsSet(attrib,POI_SECTCHANGE_TRANSITION)    ? poi.HasAttribute(POI_SECTCHANGE_TRANSITION)    : false) ||
       (WBFL::System::Flags<PoiAttributeType>::IsSet(attrib,POI_FACEOFSUPPORT)            ? poi.HasAttribute(POI_FACEOFSUPPORT)            : false) ||
       (WBFL::System::Flags<PoiAttributeType>::IsSet(attrib,POI_INTERMEDIATE_PIER)        ? poi.HasAttribute(POI_INTERMEDIATE_PIER)        : false) ||
       (WBFL::System::Flags<PoiAttributeType>::IsSet(attrib,POI_BOUNDARY_PIER)            ? poi.HasAttribute(POI_BOUNDARY_PIER)            : false) ||
       (WBFL::System::Flags<PoiAttributeType>::IsSet(attrib,POI_ABUTMENT)                 ? poi.HasAttribute(POI_ABUTMENT)                 : false) ||
       (WBFL::System::Flags<PoiAttributeType>::IsSet(attrib,POI_STIRRUP_ZONE)             ? poi.HasAttribute(POI_STIRRUP_ZONE)             : false) ||
       (WBFL::System::Flags<PoiAttributeType>::IsSet(attrib,POI_INTERMEDIATE_TEMPSUPPORT) ? poi.HasAttribute(POI_INTERMEDIATE_TEMPSUPPORT) : false) ||
       (WBFL::System::Flags<PoiAttributeType>::IsSet(attrib,POI_CLOSURE)                  ? poi.HasAttribute(POI_CLOSURE)                  : false) ||
       (WBFL::System::Flags<PoiAttributeType>::IsSet(attrib, POI_START_FACE)              ? poi.HasAttribute(POI_START_FACE)               : false) ||
       (WBFL::System::Flags<PoiAttributeType>::IsSet(attrib, POI_END_FACE)                ? poi.HasAttribute(POI_END_FACE)                 : false) ||
       (WBFL::System::Flags<PoiAttributeType>::IsSet(attrib, POI_DUCT_START)              ? poi.HasAttribute(POI_DUCT_START)               : false) ||
       (WBFL::System::Flags<PoiAttributeType>::IsSet(attrib, POI_DUCT_END)                ? poi.HasAttribute(POI_DUCT_END)                 : false) ||
       (WBFL::System::Flags<PoiAttributeType>::IsSet(attrib, POI_CASTING_BOUNDARY_START)  ? poi.HasAttribute(POI_CASTING_BOUNDARY_START)   : false) ||
       (WBFL::System::Flags<PoiAttributeType>::IsSet(attrib, POI_CASTING_BOUNDARY_END)    ? poi.HasAttribute(POI_CASTING_BOUNDARY_END)     : false)
      )
      {
         // This poi matches the selection criteria.
         return true;
      }
   }
   else
   {
      // searching for a referenced attribute
      if ( attrib == targetReference && poi.GetReferencedAttributes(targetReference) != 0 )
      {
         // didn't specify which referenced attribute is being searched for...
         // the subject poi as at least one referenced attribute of the target type
         // so it is a match
         return true;
      }

      if (
       (WBFL::System::Flags<PoiAttributeType>::IsSet(attrib,POI_PICKPOINT) ? poi.HasAttribute(targetReference | POI_PICKPOINT) : false) ||
       (WBFL::System::Flags<PoiAttributeType>::IsSet(attrib,POI_BUNKPOINT) ? poi.HasAttribute(targetReference | POI_BUNKPOINT) : false) ||
       (WBFL::System::Flags<PoiAttributeType>::IsSet(attrib,POI_0L)  ? poi.IsTenthPoint(targetReference) == 1  : false) ||
       (WBFL::System::Flags<PoiAttributeType>::IsSet(attrib,POI_1L)  ? poi.IsTenthPoint(targetReference) == 2  : false) ||
       (WBFL::System::Flags<PoiAttributeType>::IsSet(attrib,POI_2L)  ? poi.IsTenthPoint(targetReference) == 3  : false) ||
       (WBFL::System::Flags<PoiAttributeType>::IsSet(attrib,POI_3L)  ? poi.IsTenthPoint(targetReference) == 4  : false) ||
       (WBFL::System::Flags<PoiAttributeType>::IsSet(attrib,POI_4L)  ? poi.IsTenthPoint(targetReference) == 5  : false) ||
       (WBFL::System::Flags<PoiAttributeType>::IsSet(attrib,POI_5L)  ? poi.IsTenthPoint(targetReference) == 6  : false) ||
       (WBFL::System::Flags<PoiAttributeType>::IsSet(attrib,POI_6L)  ? poi.IsTenthPoint(targetReference) == 7  : false) ||
       (WBFL::System::Flags<PoiAttributeType>::IsSet(attrib,POI_7L)  ? poi.IsTenthPoint(targetReference) == 8  : false) ||
       (WBFL::System::Flags<PoiAttributeType>::IsSet(attrib,POI_8L)  ? poi.IsTenthPoint(targetReference) == 9  : false) ||
       (WBFL::System::Flags<PoiAttributeType>::IsSet(attrib,POI_9L)  ? poi.IsTenthPoint(targetReference) == 10 : false) ||
       (WBFL::System::Flags<PoiAttributeType>::IsSet(attrib,POI_10L) ? poi.IsTenthPoint(targetReference) == 11 : false)
       )
      {
         // This poi matches the selection criteria.
         return true;
      }
   }

   return false;
}

pgsPoiMgr::PoiContainer& pgsPoiMgr::GetPoiContainer(const CSegmentKey& segmentKey)
{
   ASSERT_SEGMENT_KEY(segmentKey);
   auto found = m_PoiData.find(segmentKey);
   if (found == std::end(m_PoiData))
   {
      auto result = m_PoiData.insert(std::make_pair(segmentKey, PoiContainer()));
      ATLASSERT(result.second);
      found = m_PoiData.find(segmentKey);
      ATLASSERT(found != std::end(m_PoiData));
   }

   return found->second;
}

const pgsPoiMgr::PoiContainer& pgsPoiMgr::GetPoiContainer(const CSegmentKey& segmentKey) const
{
   ASSERT_SEGMENT_KEY(segmentKey);
   auto found = m_PoiData.find(segmentKey);
   if (found == std::end(m_PoiData))
   {
      auto result = m_PoiData.insert(std::make_pair(segmentKey, PoiContainer()));
      ATLASSERT(result.second);
      found = m_PoiData.find(segmentKey);
      ATLASSERT(found != std::end(m_PoiData));
   }

   return found->second;
}
