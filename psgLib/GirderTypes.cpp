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

#include "StdAfx.h"

#include "GirderTypes.h"
#include "GirderSpacing.h"
#include "BridgeDescription.h"
#include <WbflAtlExt.h>
#include <PGSuperException.h>

#include <PsgLib\GirderData.h>

void CGirderTypes::SetSpan(const CSpanData* pSpan)
{
   m_pSpan = pSpan;
}

const CSpanData* CGirderTypes::GetSpan() const
{
   return m_pSpan;
}

bool CGirderTypes::operator == (const CGirderTypes& rOther) const
{
   if ( m_GirderGroups != rOther.m_GirderGroups )
   {
      return false;
   }

   if ( m_GirderData != rOther.m_GirderData )
   {
      return false;
   }

   if ( m_pSpan->GetBridgeDescription()->GetSlabOffsetType() == pgsTypes::sotSegment )
   {
      if ( m_SlabOffset[pgsTypes::metStart] != rOther.m_SlabOffset[pgsTypes::metStart] )
      {
         return false;
      }

      if ( m_SlabOffset[pgsTypes::metEnd] != rOther.m_SlabOffset[pgsTypes::metEnd] )
      {
         return false;
      }
   }

   return true;
}

bool CGirderTypes::operator != (const CGirderTypes& rOther) const
{
   return !operator==( rOther );
}

HRESULT CGirderTypes::Load(IStructuredLoad* pStrLoad,std::shared_ptr<IEAFProgress> pProgress)
{
   USES_CONVERSION;

   CHRException hr;

   m_GirderGroups.clear();
   m_GirderData.clear();
   m_SlabOffset[pgsTypes::metStart].clear();
   m_SlabOffset[pgsTypes::metEnd].clear();

   std::vector<std::_tstring> girderNames; // holds girder names that were stored with this object
                                           // names ahve been moved to CGirderData object
   try
   {
      hr = pStrLoad->BeginUnit(_T("GirderTypes"));

      Float64 version;
      hr = pStrLoad->get_Version(&version);

      bool bSameGirder = m_pSpan->GetBridgeDescription()->UseSameGirderForEntireBridge();
      CComVariant var;
      if ( !bSameGirder )
      {
         var.vt = VT_INDEX;
         hr = pStrLoad->get_Property(_T("GirderGroupCount"), &var );

         if ( 3 <= version )
         {
            pStrLoad->BeginUnit(_T("GirderGroups")); // added in GirderTypes version 3
         }

         GroupIndexType nGroups = VARIANT2INDEX(var);
         for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
         {
            hr = pStrLoad->BeginUnit(_T("GirderGroup"));

            Float64 grp_version;
            hr = pStrLoad->get_Version(&grp_version);

            if ( grp_version < 2.0 )
            {
               var.vt = VT_BSTR;
               hr = pStrLoad->get_Property(_T("GirderName"),&var);

               girderNames.push_back(OLE2T(var.bstrVal));
            }

            var.vt = VT_INDEX;
            hr = pStrLoad->get_Property(_T("FirstGirderIndex"),&var);
            GirderIndexType firstGdrIdx = VARIANT2INDEX(var);

            hr = pStrLoad->get_Property(_T("LastGirderIndex"),&var);
            GirderIndexType lastGdrIdx = VARIANT2INDEX(var);
         
            GirderGroup group(firstGdrIdx,lastGdrIdx);
            m_GirderGroups.push_back(group);

            hr = pStrLoad->EndUnit();
         }

         if ( 3 <= version )
         {
            pStrLoad->EndUnit(); // added in GirderTypes version 3
         }
      }

      std::_tstring strGirder = m_pSpan->GetBridgeDescription()->GetGirderName();
      GirderIndexType nGirders = m_pSpan->GetGirderCount();

      // Sometimes bad data get saved... fix it up here
      //if ( nGirders < (GirderIndexType)m_GirderNames.size() )
      //{
      //   ATLASSERT(false); // fixing bad data that got stored in the file
      //   m_GirderNames.resize(nGirders);
      //}

      //if ( nGirders < (GirderIndexType)m_GirderLibraryEntries.size() )
      //{
      //   ATLASSERT(false); // fixing bad data that got stored in the file
      //   m_GirderLibraryEntries.resize(nGirders);
      //}

      if ( nGirders < (GirderIndexType)m_GirderGroups.size() )
      {
         ATLASSERT(false); // fixing bad data that got stored in the file
         m_GirderGroups.resize(nGirders);
         //if ( m_GirderGroups[nGirders-1].second == INVALID_INDEX || m_GirderGroups[nGirders-1].second != nGirders-1 )
         //   m_GirderGroups[nGirders-1].second = nGirders-1;

         std::vector<GirderGroup>::iterator iter;
         GirderIndexType lastGirderIdx = 0;
         for ( iter = m_GirderGroups.begin(); iter != m_GirderGroups.end(); iter++ )
         {
            GirderGroup& grp = *iter;

            // index must be less than nGirders
            if ( nGirders <= grp.first )
            {
               grp.first = nGirders-1;
            }

            // the first girder in a group must be one more than the last girder
            // in the previous group
            if ( grp.first < lastGirderIdx )
            {
               grp.first = lastGirderIdx + 1; 
            }

            // second cannot be less than first
            if ( grp.second < grp.first )
            {
               grp.second = grp.first;
            }

            lastGirderIdx = grp.second;
         }

         // the last index in the last group must be the last girder index possible
         m_GirderGroups.back().second = nGirders-1;
      }

      // girder groups weren't created before because the same girder is used for all girder lines
      // create a default group here
      if ( bSameGirder )
      {
         GirderGroup group(0,nGirders-1);
         m_GirderGroups.push_back(group);
      }

      if ( 3 <= version )
      {
         pStrLoad->BeginUnit(_T("Girders")); // added in GirderTypes version 3
      }

      for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
      {
         CGirderData girderData;
         girderData.Load(pStrLoad,pProgress,0,0,0,0);

         // girderNames has entries if GirderTypes data block was less than version 2
         // put the name that used to be stored in this object into the girder data object
         if ( 0 < girderNames.size() )
         {
            GroupIndexType grpIdx = FindGroup(gdrIdx);
            girderData.m_GirderName = girderNames[grpIdx];
         }

         if ( bSameGirder )
         {
            girderData.m_GirderName = strGirder;
         }

         m_GirderData.push_back(girderData);

         //if ( bSameGirder )
         //{
         //   // these weren't created before because the same girder isn't used
         //   m_GirderNames.push_back(strGirder);
         //   m_GirderLibraryEntries.push_back(nullptr); // create a place holder for the girder library entry
         //}
      }
   
      if ( 3 <= version )
      {
         pStrLoad->EndUnit(); // added in GirderTypes version 3
      }

      // added in version 2
      if ( 2 <= version && m_pSpan->GetBridgeDescription()->GetSlabOffsetType() == pgsTypes::sotSegment )
      {
         pStrLoad->BeginUnit(_T("SlabOffset"));
         for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
         {
            pStrLoad->BeginUnit(_T("Girder"));

            var.vt = VT_R8;

            pStrLoad->get_Property(_T("Start"),&var);
            m_SlabOffset[pgsTypes::metStart].push_back(var.dblVal);

            pStrLoad->get_Property(_T("End"),&var);
            m_SlabOffset[pgsTypes::metEnd].push_back(var.dblVal);

            pStrLoad->EndUnit();
         }
         pStrLoad->EndUnit();
      }
      else
      {
         // before version 2... need to allocate space for slab offset
         Float64 slabOffset = m_pSpan->GetBridgeDescription()->GetSlabOffset();
         for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
         {
            m_SlabOffset[pgsTypes::metStart].push_back(slabOffset);
            m_SlabOffset[pgsTypes::metEnd].push_back(slabOffset);
         }
      }

      hr = pStrLoad->EndUnit();
   }
   catch (HRESULT)
   {
      ATLASSERT(false);
      THROW_LOAD(InvalidFileFormat,pStrLoad);
   }

   PGS_ASSERT_VALID;

   return hr;
}

HRESULT CGirderTypes::Save(IStructuredSave* pStrSave,std::shared_ptr<IEAFProgress> pProgress)
{
   PGS_ASSERT_VALID;

   HRESULT hr = S_OK;
   pStrSave->BeginUnit(_T("GirderTypes"),3.0);

   if ( !m_pSpan->GetBridgeDescription()->UseSameGirderForEntireBridge() )
   {
      pStrSave->put_Property(_T("GirderGroupCount"),CComVariant((long)m_GirderGroups.size()));

      // added in version 3 of GirderTypes
      pStrSave->BeginUnit(_T("GirderGroups"),1.0);
      std::vector<GirderGroup>::iterator iter;
      for ( iter = m_GirderGroups.begin(); iter != m_GirderGroups.end(); iter++ )
      {
         GirderGroup group = *iter;

         pStrSave->BeginUnit(_T("GirderGroup"),2.0);

         // moved into girder data object... Data block versions: Girder Group = 2.0
         //pStrSave->put_Property(_T("GirderName"),CComVariant(CComBSTR(m_GirderNames[group.first].c_str())));
         pStrSave->put_Property(_T("FirstGirderIndex"),CComVariant(group.first));
         pStrSave->put_Property(_T("LastGirderIndex"),CComVariant(group.second));

         pStrSave->EndUnit();
      }
      // added in version 3 of GirderTypes
      pStrSave->EndUnit();
   }


   GirderIndexType nGirders = m_pSpan->GetGirderCount();

   // added in version 3 of GirderTypes
   pStrSave->BeginUnit(_T("Girders"),1.0);
   for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
   {
      CGirderData& girderData = m_GirderData[gdrIdx];
      girderData.Save(pStrSave,pProgress);
   }
   pStrSave->EndUnit(); // added in version 3 of GirderTypes

   // added in version 2
   if ( m_pSpan->GetBridgeDescription()->GetSlabOffsetType() == pgsTypes::sotSegment )
   {
      pStrSave->BeginUnit(_T("SlabOffset"),1.0);
      for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
      {
         pStrSave->BeginUnit(_T("Girder"),1.0);
         pStrSave->put_Property(_T("Start"),CComVariant(m_SlabOffset[pgsTypes::metStart][gdrIdx]));
         pStrSave->put_Property(_T("End"),  CComVariant(m_SlabOffset[pgsTypes::metEnd][gdrIdx])  );
         pStrSave->EndUnit();
      }
      pStrSave->EndUnit();
   }

   pStrSave->EndUnit();

   return hr;
}

GroupIndexType CGirderTypes::FindGroup(GirderIndexType gdrIdx) const
{
   if ( m_pSpan && m_pSpan->GetBridgeDescription()->UseSameGirderForEntireBridge() )
   {
      return 0; // can't create a new group... return group 0
   }

   GroupIndexType nGroups = GetGirderGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      GirderIndexType firstGdrIdx, lastGdrIdx;

      GirderGroup group = m_GirderGroups[grpIdx];

      firstGdrIdx = group.first;
      lastGdrIdx  = group.second;

      if ( firstGdrIdx <= gdrIdx && gdrIdx <= lastGdrIdx )
      {
         return grpIdx;
      }
   }

   ATLASSERT(false); // should never get here
   return INVALID_INDEX;
}

GroupIndexType CGirderTypes::CreateGroup(GirderIndexType firstGdrIdx,GirderIndexType lastGdrIdx)
{
   if ( m_pSpan && m_pSpan->GetBridgeDescription()->UseSameGirderForEntireBridge() )
   {
      return 0; // can't create a new group... return group 0
   }

   GroupIndexType newGroupIdx = 9999;

   std::vector<GirderGroup> gdrGroups;

   std::vector<GirderGroup>::const_iterator iter;
   for ( iter = m_GirderGroups.begin(); iter != m_GirderGroups.end(); iter++ )
   {
      GirderGroup gdrGroup = *iter;
      if ( gdrGroup.first == firstGdrIdx && gdrGroup.second == lastGdrIdx )
      {
         // no need to create a new group (we already have it)
         newGroupIdx = iter - m_GirderGroups.begin();
         return newGroupIdx;
      }

      if ( lastGdrIdx < gdrGroup.first || gdrGroup.second < firstGdrIdx )
      {
         // the new group ends before the start of this group -OR-
         // the new group starts after the end of this group
         // just save the group
         gdrGroups.push_back(gdrGroup);
      }
      else if ( gdrGroup.first < firstGdrIdx && firstGdrIdx <= gdrGroup.second )
      {
         // the new group starts in the middle of this group
         gdrGroup.second = firstGdrIdx-1; // set the end of this group one girder before the 
         gdrGroups.push_back(gdrGroup);
         break;
      }
      else if ( lastGdrIdx <= gdrGroup.second )
      {
         // new group ends in the middle of this group

         if ( gdrGroup.first == firstGdrIdx )
         {
            // new group starts at start of this group
            // (n,m) ->  (n,k) (k+1,m)
            // k=2 (0,4) ->  (0,2) (3,4)
            GirderGroup newGrp;
            newGrp.first  = firstGdrIdx;
            newGrp.second = lastGdrIdx;
            gdrGroups.push_back(newGrp);
            newGroupIdx = gdrGroups.size()-1;

            newGrp.first = lastGdrIdx+1;
            newGrp.second = gdrGroup.second;
            gdrGroups.push_back(newGrp);
         }
         else if ( gdrGroup.second == lastGdrIdx )
         {
            // new group ends at end of this group
            // (n,m) -> (n,k-1)(k,m)
            // k=2 (0,4) -> (0,1)(2,4)
            GirderGroup newGrp;
            newGrp.first  = gdrGroup.first;
            newGrp.second = firstGdrIdx-1;
            gdrGroups.push_back(newGrp);

            newGrp.first = firstGdrIdx;
            newGrp.second = lastGdrIdx;
            gdrGroups.push_back(newGrp);
            newGroupIdx = gdrGroups.size()-1;
         }
         else
         {
            // new group is completely within this group
            // (n,m) -> (n,k-1)(k,l)(l+1,m)
            // k=2,l=2  (0,4) -> (0,1)(2,2)(3,4)
            GirderGroup newGrp;
            newGrp.first  = gdrGroup.first;
            newGrp.second = firstGdrIdx-1;
            gdrGroups.push_back(newGrp);

            newGrp.first = firstGdrIdx;
            newGrp.second = lastGdrIdx;
            gdrGroups.push_back(newGrp);
            newGroupIdx = gdrGroups.size()-1;

            newGrp.first = lastGdrIdx+1;
            newGrp.second = gdrGroup.second;
            gdrGroups.push_back(newGrp);
         }
      }
   }

   for ( iter; iter != m_GirderGroups.end(); iter++ )
   {
      GirderGroup gdrGroup = *iter;
      if ( gdrGroup.first < lastGdrIdx && lastGdrIdx <= gdrGroup.second )
      {
         // the new group ends in the middle of this group

         // add the new group
         gdrGroup.first = firstGdrIdx;
         gdrGroup.second = lastGdrIdx;
         gdrGroups.push_back(gdrGroup);
         newGroupIdx = gdrGroups.size()-1;

         // get the current group back
         gdrGroup = *iter;
         gdrGroup.first = lastGdrIdx + 1; // group begins one girder after the last group
         if ( gdrGroup.first <= gdrGroup.second ) 
         {
            gdrGroups.push_back(gdrGroup); // if last is < first, then this isn't a group... the new group ends at the last girder
         }
      }
      else
      {
         gdrGroups.push_back(gdrGroup); // save the rest of the groups
      }
   }

   m_GirderGroups = gdrGroups;

   PGS_ASSERT_VALID;

   return newGroupIdx;
}

void CGirderTypes::GetGirderGroup(GroupIndexType groupIdx,GirderIndexType* pFirstGdrIdx,GirderIndexType* pLastGdrIdx,std::_tstring& strName) const
{
   if ( m_pSpan && 
        m_pSpan->GetBridgeDescription()->UseSameNumberOfGirdersInAllSpans() &&
        m_pSpan->GetBridgeDescription()->UseSameGirderForEntireBridge() )
   {
      *pFirstGdrIdx = 0;
      *pLastGdrIdx = min(m_GirderGroups[groupIdx].second,m_pSpan->GetBridgeDescription()->GetGirderCount()-1);
      strName = m_pSpan->GetBridgeDescription()->GetGirderName();
   }
   else
   {
      _ASSERT( groupIdx < (SpacingIndexType)m_GirderGroups.size() );
      GirderGroup group = m_GirderGroups[groupIdx];

      *pFirstGdrIdx = group.first;
      *pLastGdrIdx  = group.second;

      strName = m_GirderData[group.first].GetGirderName();

#if defined _DEBUG
      // make sure every girder in the group has the same name
      for ( GirderIndexType i = group.first + 1; i <= group.second; i++ )
      {
         _ASSERT(strName == m_GirderData[i].GetGirderName());
      }
#endif
   }
}

void CGirderTypes::AddGirders(GirderIndexType nGirders)
{
   const CGirderData& gdrData = (m_GirderData.size() != 0 ? m_GirderData.back() : CGirderData());
   Float64 slabOffsetStart = (m_SlabOffset[pgsTypes::metStart].size() != 0 ? m_SlabOffset[pgsTypes::metStart].back() : m_pSpan->GetBridgeDescription()->GetSlabOffset());
   Float64 slabOffsetEnd   = (m_SlabOffset[pgsTypes::metEnd].size()   != 0 ? m_SlabOffset[pgsTypes::metEnd].back()   : m_pSpan->GetBridgeDescription()->GetSlabOffset());

   for ( GirderIndexType i = 0; i < nGirders; i++ )
   {
      m_GirderData.push_back(gdrData);
      m_SlabOffset[pgsTypes::metStart].push_back(slabOffsetStart);
      m_SlabOffset[pgsTypes::metEnd].push_back(slabOffsetEnd);
   }

   if ( m_GirderGroups.size() == 0 )
   {
      GirderGroup group;
      group.first  = 0;
      group.second = INVALID_INDEX;
      m_GirderGroups.push_back(group);
   }

   GirderGroup& group = m_GirderGroups.back();
   if ( group.second == INVALID_INDEX )
   {
      group.second = nGirders-1;
   }
   else
   {
      group.second += nGirders;
   }
   
   PGS_ASSERT_VALID;
}

void CGirderTypes::RemoveGirders(GirderIndexType nGirdersToRemove)
{
   _ASSERT( nGirdersToRemove < (GirderIndexType)m_GirderData.size() ); // trying to remove too many girders

   for ( GirderIndexType i = 0; i < nGirdersToRemove; i++ )
   {
      m_GirderData.pop_back();
      m_SlabOffset[pgsTypes::metStart].pop_back();
      m_SlabOffset[pgsTypes::metEnd].pop_back();
   }

   GirderIndexType nGirders = m_GirderData.size();

   std::vector<GirderGroup>::iterator iter;
   GirderIndexType gdrCount = 0;
   for ( iter = m_GirderGroups.begin(); iter != m_GirderGroups.end(); iter++ )
   {
      GirderGroup& group = *iter;

      if ( nGirders-1 <= group.second )
      {
         // the current group goes beyond the number of girders..
         group.second = nGirders-1;
         iter++;
         break;
      }
   }

   m_GirderGroups.erase(iter,m_GirderGroups.end());
   PGS_ASSERT_VALID;
}

void CGirderTypes::ExpandAll()
{
   m_GirderGroups.clear();
   GirderIndexType nGirders = m_GirderData.size();
   for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
   {
      m_GirderGroups.emplace_back(gdrIdx,gdrIdx);
   }
   PGS_ASSERT_VALID;
}

void CGirderTypes::Expand(GroupIndexType groupIdx)
{
   GirderIndexType firstGdrIdx, lastGdrIdx;
   std::_tstring strName;
   GetGirderGroup(groupIdx,&firstGdrIdx,&lastGdrIdx,strName);

   if ( firstGdrIdx == lastGdrIdx )
   {
      return; // nothing to expand
   }

   std::vector<GirderGroup>::iterator iter = m_GirderGroups.begin() + groupIdx;
   std::vector<GirderGroup>::iterator pos = m_GirderGroups.erase(iter); // returns the iter the element after the one removed

   // inserted element goes before the iterator... insert retuns position of newly inserted item
   // go in reverse order
   for ( GirderIndexType gdrIdx = lastGdrIdx; firstGdrIdx <= gdrIdx; gdrIdx-- )
   {
      GirderGroup group(gdrIdx,gdrIdx);
      pos = m_GirderGroups.insert(pos,group);
   }
   PGS_ASSERT_VALID;
}


void CGirderTypes::JoinAll(GirderIndexType gdrIdx)
{
   if ( m_GirderData.size() == 0 )
   {
      return;
   }

   std::_tstring strName = m_GirderData[gdrIdx].GetGirderName();
   const GirderLibraryEntry* pGdrEntry = m_GirderData[gdrIdx].GetGirderLibraryEntry();

   GirderGroup firstGroup = m_GirderGroups.front();
   GirderGroup lastGroup  = m_GirderGroups.back();

   GirderGroup joinedGroup;
   joinedGroup.first  = firstGroup.first;
   joinedGroup.second = lastGroup.second;

   m_GirderGroups.clear();
   m_GirderGroups.push_back(joinedGroup);

   // make girder name and lib entry the same for all members of the group
   std::vector<CGirderData>::iterator iter;
   for ( iter = m_GirderData.begin(); iter != m_GirderData.end(); iter++ )
   {
      CGirderData& girderData = *iter;
      girderData.SetGirderName(strName.c_str());
      girderData.SetGirderLibraryEntry(pGdrEntry);
   }
   PGS_ASSERT_VALID;
}

void CGirderTypes::Join(GirderIndexType firstGdrIdx,GirderIndexType lastGdrIdx,GirderIndexType gdrIdx)
{
   // girder index must be in the range
   _ASSERT( firstGdrIdx <= gdrIdx && gdrIdx <= lastGdrIdx );

   // get the girder name for the group
   std::_tstring strName = m_GirderData[gdrIdx].GetGirderName();
   const GirderLibraryEntry* pGdrEntry = m_GirderData[gdrIdx].GetGirderLibraryEntry();

   // assign the name for the group
   for ( GirderIndexType i = firstGdrIdx; i <= lastGdrIdx; i++ )
   {
      m_GirderData[i].SetGirderName(strName.c_str());
      m_GirderData[i].SetGirderLibraryEntry(pGdrEntry);
   }

   // firstGdrIdx must match the "first" parameter of a GirderGroup
   // lastGdrIdx  must match the "last"  parameter of a GirderGroup
   // this is the way the UI grid works so we don't need to allow
   // for any more complicated joining than this.

   // create a local girder groups container. It is easier to fill it up as we go
   // rather than manipulating the class data member... update the class data member
   // at the end of this function
   std::vector<GirderGroup> gdrGroups;

   // loop until the first index in a group matches firstGdrIdx
   // save any group that comes before the first group
   std::vector<GirderGroup>::iterator iter;
   for ( iter = m_GirderGroups.begin(); iter != m_GirderGroups.end(); iter++ )
   {
      GirderGroup gdrGroup = *iter;
      if ( gdrGroup.first == firstGdrIdx )
      {
         break;
      }
      else
      {
         gdrGroups.push_back(gdrGroup);
      }
   }

   _ASSERT(iter != m_GirderGroups.end()); // shouldn't have gone through the full vector

   // loop until the last index in a group matches lastGdrIdx
   // don't save any groups in the middle... these are the groups
   // being joined
   for ( ; iter != m_GirderGroups.end(); iter++ )
   {
      GirderGroup gdrGroup = *iter;
      if ( gdrGroup.second == lastGdrIdx )
      {
         iter++; // move iter to next position because breaking out of the loop skips incrementing
         break;
      }
   }

   // save the new group
   GirderGroup newGroup(firstGdrIdx,lastGdrIdx);
   gdrGroups.push_back(newGroup);

   // copy the remaining groups
   if ( iter != m_GirderGroups.end() )
   {
      gdrGroups.insert(gdrGroups.end(),iter,m_GirderGroups.end());
   }

   // finally replace the data member with the local girder groups
   m_GirderGroups = gdrGroups;
   PGS_ASSERT_VALID;
}

GroupIndexType CGirderTypes::GetGirderGroupCount() const
{
   if ( m_pSpan && m_pSpan->GetBridgeDescription()->UseSameGirderForEntireBridge() )
   {
      return 1;
   }
   else
   {
      return m_GirderGroups.size();
   }
}

void CGirderTypes::SetGirderCount(GirderIndexType nGirders)
{
   if ( nGirders < m_GirderData.size() )
   {
      RemoveGirders(m_GirderData.size() - nGirders);
   }
   else
   {
      AddGirders(nGirders - m_GirderData.size());
   }

   ATLASSERT( m_GirderGroups.size() != 0 );
#if defined _DEBUG
   GirderGroup grp = m_GirderGroups.front();
   GirderIndexType firstIdx = grp.first;
   grp = m_GirderGroups.back();
   GirderIndexType lastIdx = grp.second;
   ATLASSERT(nGirders == (lastIdx-firstIdx+1));
#endif
   PGS_ASSERT_VALID;
}

GirderIndexType CGirderTypes::GetGirderCount() const
{
   PGS_ASSERT_VALID;
   return m_GirderData.size();
}

void CGirderTypes::RenameGirder(GroupIndexType grpIdx,LPCTSTR strName)
{
   std::_tstring strGirder;
   GirderIndexType firstGdrIdx, lastGdrIdx;
   GetGirderGroup(grpIdx,&firstGdrIdx,&lastGdrIdx,strGirder);

   for ( GirderIndexType gdrIdx = firstGdrIdx; gdrIdx <= lastGdrIdx; gdrIdx++ )
   {
      m_GirderData[gdrIdx].m_GirderName = strName;
   }
}

void CGirderTypes::SetGirderName(GroupIndexType grpIdx,LPCTSTR strName)
{
   std::_tstring strGirder;
   GirderIndexType firstGdrIdx, lastGdrIdx;
   GetGirderGroup(grpIdx,&firstGdrIdx,&lastGdrIdx,strGirder);

   for ( GirderIndexType gdrIdx = firstGdrIdx; gdrIdx <= lastGdrIdx; gdrIdx++ )
   {
      m_GirderData[gdrIdx].SetGirderName(strName);
   }
}

LPCTSTR CGirderTypes::GetGirderName(GirderIndexType gdrIdx) const
{
   if ( m_pSpan && m_pSpan->GetBridgeDescription()->UseSameGirderForEntireBridge() )
   {
      return m_pSpan->GetBridgeDescription()->GetGirderName();
   }
   else
   {
      return m_GirderData[gdrIdx].GetGirderName();
   }
}

void CGirderTypes::SetGirderLibraryEntry(GroupIndexType grpIdx,const GirderLibraryEntry* pEntry)
{
   std::_tstring strGirder;
   GirderIndexType firstGdrIdx, lastGdrIdx;
   GetGirderGroup(grpIdx,&firstGdrIdx,&lastGdrIdx,strGirder);

   for ( GirderIndexType gdrIdx = firstGdrIdx; gdrIdx <= lastGdrIdx; gdrIdx++ )
   {
      m_GirderData[gdrIdx].SetGirderLibraryEntry(pEntry);
   }
}

const GirderLibraryEntry* CGirderTypes::GetGirderLibraryEntry(GirderIndexType gdrIdx) const
{
   const GirderLibraryEntry* pEntry = nullptr;

   if ( m_pSpan && m_pSpan->GetBridgeDescription()->UseSameGirderForEntireBridge() )
   {
      pEntry = m_pSpan->GetBridgeDescription()->GetGirderLibraryEntry();
   }
   else
   {
      pEntry = m_GirderData[gdrIdx].GetGirderLibraryEntry();
   }

   return pEntry;
}

void CGirderTypes::SetGirderData(GirderIndexType gdrIdx,const CGirderData& gdrData)
{
   // The original implementation of this method did not change the girder name or library
   // entry (they were stored in this object, not the girder data object). Now that the
   // girder name and library entry are part of the girder data object, the semantics of
   // this method need to be preserved. That is why the name and entry aren't changed.
   std::_tstring strName = m_GirderData[gdrIdx].GetGirderName();
   const GirderLibraryEntry* pEntry = m_GirderData[gdrIdx].GetGirderLibraryEntry();

   m_GirderData[gdrIdx] = gdrData;

   m_GirderData[gdrIdx].SetGirderName(strName.c_str());
   m_GirderData[gdrIdx].SetGirderLibraryEntry(pEntry);
}

const CGirderData& CGirderTypes::GetGirderData(GirderIndexType gdrIdx) const
{
   // only call this method for girder bridges
   return m_GirderData[gdrIdx];
}

CGirderData& CGirderTypes::GetGirderData(GirderIndexType gdrIdx)
{
   // only call this method for girder bridges
   return m_GirderData[gdrIdx];
}

void CGirderTypes::SetSlabOffset(GirderIndexType gdrIdx,pgsTypes::MemberEndType end,Float64 offset)
{
   if ( gdrIdx == ALL_GIRDERS )
   {
      std::vector<Float64>::iterator iter;
      for ( iter = m_SlabOffset[end].begin(); iter != m_SlabOffset[end].end(); iter++ )
      {
         Float64& slabOffset = *iter;
         slabOffset = offset;
      }
   }
   else
   {
      m_SlabOffset[end][gdrIdx] = offset;
   }
}

Float64 CGirderTypes::GetSlabOffset(GirderIndexType gdrIdx,pgsTypes::MemberEndType end)
{
   if ( m_pSpan && m_pSpan->GetBridgeDescription()->GetSlabOffsetType() != pgsTypes::sotSegment )
   {
      return m_pSpan->GetSlabOffset(end);
   }
   else
   {
      ATLASSERT(gdrIdx != INVALID_INDEX);
      return m_SlabOffset[end][gdrIdx];
   }
}

Float64 CGirderTypes::GetSlabOffset(GirderIndexType gdrIdx,pgsTypes::MemberEndType end) const
{
   if ( m_pSpan && m_pSpan->GetBridgeDescription()->GetSlabOffsetType() != pgsTypes::sotSegment )
   {
      return m_pSpan->GetSlabOffset(end);
   }
   else
   {
      ATLASSERT(gdrIdx != INVALID_INDEX);
      return m_SlabOffset[end][gdrIdx];
   }
}

#if defined _DEBUG
void CGirderTypes::AssertValid() const
{
   ATLASSERT(m_SlabOffset[0].size() == m_SlabOffset[1].size());
   ATLASSERT(m_GirderData.size() != 0 ? m_GirderData.size() == m_GirderGroups.back().second+1 : true);
   ATLASSERT(m_GirderData.size() != 0 ? m_GirderGroups.size() <= m_GirderData.size() : true);

   // last index must less than or equal to the current index
   GirderIndexType lastIdx = 0;
   std::vector<GirderGroup>::const_iterator iter(m_GirderGroups.begin());
   std::vector<GirderGroup>::const_iterator end(m_GirderGroups.end());
   for ( ; iter != end; iter++ )
   {
      GirderGroup grp = *iter;
      GirderIndexType idx = grp.first;
      ATLASSERT( lastIdx <= idx );

      lastIdx = idx;
      idx = grp.second;
      ATLASSERT( lastIdx <= idx );

      lastIdx = idx;
   }

}
#endif
