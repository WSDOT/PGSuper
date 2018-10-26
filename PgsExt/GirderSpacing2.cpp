///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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
#include <PgsExt\GirderSpacing2.h>
#include <PgsExt\PierData2.h>
#include <PgsExt\TemporarySupportData.h>
#include <PgsExt\BridgeDescription2.h>
#include <PsgLib\GirderLibraryEntry.h>
#include <WbflAtlExt.h>
#include <numeric>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CGirderSpacingData2
****************************************************************************/

CGirderSpacingData2::CGirderSpacingData2()
{
   m_MeasurementType     = pgsTypes::NormalToItem;
   m_MeasurementLocation = pgsTypes::AtPierLine;
   
   m_RefGirderIdx = ALL_GIRDERS; // reference girder is the center of the girder group
   m_RefGirderOffsetType = pgsTypes::omtBridge;
   m_RefGirderOffset = 0;

   m_DefaultSpacing = ::ConvertToSysUnits(5.0,unitMeasure::Feet);

   PGS_ASSERT_VALID;
}

CGirderSpacingData2::CGirderSpacingData2(const CGirderSpacingData2& rOther)
{
   MakeCopy(rOther);
}

CGirderSpacingData2::~CGirderSpacingData2()
{
}

CGirderSpacingData2& CGirderSpacingData2::operator= (const CGirderSpacingData2& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

bool CGirderSpacingData2::operator == (const CGirderSpacingData2& rOther) const
{
   if ( m_MeasurementType != rOther.m_MeasurementType )
   {
      return false;
   }

   if ( m_MeasurementLocation != rOther.m_MeasurementLocation )
   {
      return false;
   }

   if ( m_RefGirderIdx != rOther.m_RefGirderIdx )
   {
      return false;
   }

   if ( m_RefGirderOffsetType != rOther.m_RefGirderOffsetType )
   {
      return false;
   }

   if ( !IsEqual(m_RefGirderOffset,rOther.m_RefGirderOffset) )
   {
      return false;
   }

   if ( m_GirderSpacing != rOther.m_GirderSpacing )
   {
      return false;
   }

   if ( m_SpacingGroups != rOther.m_SpacingGroups )
   {
      return false;
   }

   return true;
}

bool CGirderSpacingData2::operator != (const CGirderSpacingData2& rOther) const
{
   return !operator==(rOther);
}

HRESULT CGirderSpacingData2::Load(IStructuredLoad* pStrLoad,IProgress* pProgress)
{
   USES_CONVERSION;

   CHRException hr;

   try
   {
      m_SpacingGroups.clear();
      m_GirderSpacing.clear();

      hr = pStrLoad->BeginUnit(_T("GirderSpacing"));

      Float64 version;
      hr = pStrLoad->get_Version(&version);


      CComVariant var;

      var.vt = VT_I4;
      hr = pStrLoad->get_Property(_T("MeasurementLocation"),&var);
      m_MeasurementLocation = (pgsTypes::MeasurementLocation)(var.lVal);

      if ( 1 < m_MeasurementLocation )
      {
         ATLASSERT(false); // bad data read from file... setting it to a default value
         m_MeasurementLocation = pgsTypes::AtPierLine;
      }

      var.vt = VT_I4;
      hr = pStrLoad->get_Property(_T("MeasurementType"),&var);
      m_MeasurementType = (pgsTypes::MeasurementType)(var.lVal);

      if ( 1 < m_MeasurementType )
      {
         ATLASSERT(false); // bad data read from file... setting it to a default value
         m_MeasurementType = pgsTypes::NormalToItem;
      }

      var.vt = VT_INDEX;
      hr = pStrLoad->get_Property(_T("RefGirder"),&var);
      m_RefGirderIdx = VARIANT2INDEX(var);

      var.vt = VT_I4;
      hr = pStrLoad->get_Property(_T("RefGirderOffsetType"),&var);
      m_RefGirderOffsetType = (pgsTypes::OffsetMeasurementType)(var.iVal);

      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("RefGirderOffset"),&var);
      m_RefGirderOffset = var.dblVal;

      var.vt = VT_INDEX;
      hr = pStrLoad->get_Property(_T("SpacingGroupCount"), &var );

      GroupIndexType nGroups = VARIANT2INDEX(var);
      for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
      {
         hr = pStrLoad->BeginUnit(_T("SpacingGroup"));

         Float64 grp_version;
         hr = pStrLoad->get_Version(&grp_version);
         ATLASSERT(grp_version == 1);

         var.vt = VT_R8;
         hr = pStrLoad->get_Property(_T("Spacing"),&var);
         Float64 spacing = var.dblVal;

         var.vt = VT_INDEX;
         hr = pStrLoad->get_Property(_T("FirstGirderIndex"),&var);
         GirderIndexType firstGdrIdx = VARIANT2INDEX(var);

         hr = pStrLoad->get_Property(_T("LastGirderIndex"),&var);
         GirderIndexType lastGdrIdx = VARIANT2INDEX(var);

         for ( GirderIndexType gdrIdx = firstGdrIdx; gdrIdx < lastGdrIdx; gdrIdx++ )
         {
            m_GirderSpacing.push_back(spacing);
         }
         
         SpacingGroup group(firstGdrIdx,lastGdrIdx);
         m_SpacingGroups.push_back(group);


         hr = pStrLoad->EndUnit();
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

HRESULT CGirderSpacingData2::Save(IStructuredSave* pStrSave,IProgress* pProgress)
{
   PGS_ASSERT_VALID;

   HRESULT hr = S_OK;
   pStrSave->BeginUnit(_T("GirderSpacing"),1.0);

   pStrSave->put_Property(_T("MeasurementLocation"),CComVariant((long)m_MeasurementLocation));
   pStrSave->put_Property(_T("MeasurementType"),CComVariant((long)m_MeasurementType));

   // added in version 2
   pStrSave->put_Property(_T("RefGirder"),CComVariant(m_RefGirderIdx));
   pStrSave->put_Property(_T("RefGirderOffsetType"),CComVariant(m_RefGirderOffsetType));
   pStrSave->put_Property(_T("RefGirderOffset"),CComVariant(m_RefGirderOffset));

   pStrSave->put_Property(_T("SpacingGroupCount"),CComVariant((long)m_SpacingGroups.size()));

   std::vector<SpacingGroup>::iterator iter;
   for ( iter = m_SpacingGroups.begin(); iter != m_SpacingGroups.end(); iter++ )
   {
      SpacingGroup group = *iter;

      pStrSave->BeginUnit(_T("SpacingGroup"),1.0);

      pStrSave->put_Property(_T("Spacing"),CComVariant(m_GirderSpacing[group.first]));
      pStrSave->put_Property(_T("FirstGirderIndex"),CComVariant(group.first));
      pStrSave->put_Property(_T("LastGirderIndex"),CComVariant(group.second));

      pStrSave->EndUnit();
   }


   pStrSave->EndUnit();
   return hr;
}

void CGirderSpacingData2::MakeCopy(const CGirderSpacingData2& rOther)
{
   m_MeasurementType     = rOther.m_MeasurementType;
   m_MeasurementLocation = rOther.m_MeasurementLocation;

   m_RefGirderIdx        = rOther.m_RefGirderIdx;
   m_RefGirderOffset     = rOther.m_RefGirderOffset;
   m_RefGirderOffsetType = rOther.m_RefGirderOffsetType;

   m_GirderSpacing = rOther.m_GirderSpacing;
   m_SpacingGroups = rOther.m_SpacingGroups;

   m_DefaultSpacing = rOther.m_DefaultSpacing;

   PGS_ASSERT_VALID;
}

void CGirderSpacingData2::MakeAssignment(const CGirderSpacingData2& rOther)
{
   MakeCopy( rOther );
}

void CGirderSpacingData2::SetGirderSpacing(GroupIndexType grpIdx,Float64 s)
{
   GirderIndexType firstGdrIdx, lastGdrIdx;
   Float64 spacing;
   GetSpacingGroup(grpIdx,&firstGdrIdx,&lastGdrIdx,&spacing);

   SpacingIndexType firstSpaceIdx = firstGdrIdx;
   SpacingIndexType lastSpaceIdx  = lastGdrIdx-1;

   if ( m_GirderSpacing.size() <= lastSpaceIdx )
   {
      // the vector is too small... resize it so that everything fits
      m_GirderSpacing.resize(lastSpaceIdx+1);
   }

   _ASSERT( 0 <= firstSpaceIdx && firstSpaceIdx < (SpacingIndexType)m_GirderSpacing.size() );
   _ASSERT( 0 <= lastSpaceIdx  && lastSpaceIdx  < (SpacingIndexType)m_GirderSpacing.size() );
   for ( SpacingIndexType i = firstSpaceIdx; i <= lastSpaceIdx; i++ )
   {
      m_GirderSpacing[i] = s;
   }
   PGS_ASSERT_VALID;
}

void CGirderSpacingData2::SetMeasurementType(pgsTypes::MeasurementType mt)
{
   _ASSERT( mt == pgsTypes::AlongItem || mt == pgsTypes::NormalToItem );
   m_MeasurementType = mt;
   PGS_ASSERT_VALID;
}

pgsTypes::MeasurementType CGirderSpacingData2::GetMeasurementType() const
{
   return m_MeasurementType;
}

void CGirderSpacingData2::SetMeasurementLocation(pgsTypes::MeasurementLocation ml)
{
   _ASSERT( ml == pgsTypes::AtPierLine || ml == pgsTypes::AtCenterlineBearing );
   m_MeasurementLocation = ml;
   PGS_ASSERT_VALID;
}

pgsTypes::MeasurementLocation CGirderSpacingData2::GetMeasurementLocation() const
{
   return m_MeasurementLocation;
}

void CGirderSpacingData2::SetRefGirder(GirderIndexType refGdrIdx)
{
   m_RefGirderIdx = refGdrIdx;
}

GirderIndexType CGirderSpacingData2::GetRefGirder() const
{
   return m_RefGirderIdx;
}

void CGirderSpacingData2::SetRefGirderOffset(Float64 offset)
{
   m_RefGirderOffset = offset;
}

Float64 CGirderSpacingData2::GetRefGirderOffset() const
{
   return m_RefGirderOffset;
}

void CGirderSpacingData2::SetRefGirderOffsetType(pgsTypes::OffsetMeasurementType offsetDatum)
{
   m_RefGirderOffsetType = offsetDatum;
}

pgsTypes::OffsetMeasurementType CGirderSpacingData2::GetRefGirderOffsetType() const
{
   return m_RefGirderOffsetType;
}

GroupIndexType CGirderSpacingData2::GetSpacingGroupCount() const
{
   return m_SpacingGroups.size();
}

void CGirderSpacingData2::GetSpacingGroup(GroupIndexType groupIdx,GirderIndexType* pFirstGdrIdx,GirderIndexType* pLastGdrIdx,Float64* pSpacing) const
{
   _ASSERT( groupIdx < (SpacingIndexType)m_SpacingGroups.size() );
   SpacingGroup group = m_SpacingGroups[groupIdx];

   *pSpacing = m_GirderSpacing[group.first];
   *pFirstGdrIdx = group.first;
   *pLastGdrIdx  = group.second;

#if defined _DEBUG
   // make sure every spacing in the group has the value
   GirderIndexType firstGirderIdx = group.first;
   GirderIndexType lastGirderIdx  = group.second-1;
   for ( GirderIndexType i = firstGirderIdx + 1; i < lastGirderIdx; i++ )
   {
//      _ASSERT(IsEqual(*pSpacing,m_GirderSpacing[group.first]));
      _ASSERT(IsEqual(*pSpacing,m_GirderSpacing[i]));
   }
#endif
}

void CGirderSpacingData2::ExpandAll()
{
   m_SpacingGroups.clear();
   GirderIndexType nGirders = m_GirderSpacing.size();
   for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
   {
      m_SpacingGroups.push_back(SpacingGroup(gdrIdx,gdrIdx+1));
   }
}

void CGirderSpacingData2::Expand(GroupIndexType groupIdx)
{
   GirderIndexType firstGdrIdx, lastGdrIdx;
   Float64 spacing;
   GetSpacingGroup(groupIdx,&firstGdrIdx,&lastGdrIdx,&spacing);

   if ( (lastGdrIdx - firstGdrIdx) <= 1 )
   {
      return; // there is only one girder in the group...nothing to expand
   }

   std::vector<SpacingGroup>::iterator iter = m_SpacingGroups.begin() + groupIdx;
   std::vector<SpacingGroup>::iterator pos  = m_SpacingGroups.erase(iter); // returns the iter the element after the one removed

   // inserted element goes before the iterator... insert retuns position of newly inserted item
   // go in reverse order
   for ( GirderIndexType gdrIdx = lastGdrIdx; firstGdrIdx < gdrIdx; gdrIdx-- )
   {
      SpacingGroup group(gdrIdx-1,gdrIdx);
      pos = m_SpacingGroups.insert(pos,group);
   }
}

void CGirderSpacingData2::JoinAll(SpacingIndexType spacingKey)
{
   if ( m_GirderSpacing.size() == 0 )
   {
      return;
   }

   Float64 spacing = m_GirderSpacing[spacingKey];
   SpacingGroup firstGroup = m_SpacingGroups.front();
   SpacingGroup lastGroup  = m_SpacingGroups.back();

   SpacingGroup joinedGroup;
   joinedGroup.first  = firstGroup.first;
   joinedGroup.second = lastGroup.second;

   m_SpacingGroups.clear();
   m_SpacingGroups.push_back(joinedGroup);

   std::vector<Float64>::iterator iter;
   for ( iter = m_GirderSpacing.begin(); iter != m_GirderSpacing.end(); iter++ )
   {
      (*iter) = spacing;
   }
}

void CGirderSpacingData2::Join(GirderIndexType firstGdrIdx,GirderIndexType lastGdrIdx,SpacingIndexType spacingKey)
{
   _ASSERT(firstGdrIdx != lastGdrIdx);

   // girder index must be in the range
   _ASSERT( firstGdrIdx <= spacingKey && spacingKey <= lastGdrIdx );

   // get the girder spacing for the group
   Float64 spacing = m_GirderSpacing[spacingKey];

   // assign the spacing to the group
   SpacingIndexType firstSpaceIdx = SpacingIndexType(firstGdrIdx);
   SpacingIndexType nSpaces       = SpacingIndexType(lastGdrIdx - firstGdrIdx);
   for ( SpacingIndexType spaceIdx = firstSpaceIdx; spaceIdx < nSpaces; spaceIdx++ )
   {
      m_GirderSpacing[spaceIdx] = spacing;
   }

   // firstGdrIdx must match the "first" parameter of a SpacingGroup
   // lastGdrIdx  must match the "last"  parameter of a SpacingGroup
   // this is they way the UI grid works so we don't need to allow
   // for any more complicated joining than this.

   // create a local spacing groups container. It is easier to fill it up as we go
   // rather than manipulating the class data member... update the class data member
   // at the end of this function
   std::vector<SpacingGroup> spacingGroups;

   // loop until the first index in a group matches firstGdrIdx
   // save any group that comes before the first group
   std::vector<SpacingGroup>::iterator iter;
   for ( iter = m_SpacingGroups.begin(); iter != m_SpacingGroups.end(); iter++ )
   {
      SpacingGroup spacingGroup = *iter;
      if ( spacingGroup.first == firstGdrIdx )
      {
         break;
      }
      else
      {
         spacingGroups.push_back(spacingGroup);
      }
   }

   _ASSERT(iter != m_SpacingGroups.end()); // shouldn't have gone through the full vector

   // loop until the last index in a group matches lastGdrIdx
   // don't save any groups in the middle... these are the groups
   // being joined
   for ( ; iter != m_SpacingGroups.end(); iter++ )
   {
      SpacingGroup spacingGroup = *iter;
      if ( spacingGroup.second == lastGdrIdx )
      {
         iter++; // move iter to next position... breaking out of the loop skips the incrementer
         break;
      }
   }

   // same the new group
   SpacingGroup newGroup(firstGdrIdx,lastGdrIdx);
   spacingGroups.push_back(newGroup);

   // copy the remaining groups
   if ( iter != m_SpacingGroups.end() )
   {
      spacingGroups.insert(spacingGroups.end(),iter,m_SpacingGroups.end());
   }

   // finally replace the data member with the local girder groups
   m_SpacingGroups = spacingGroups;
}

SpacingIndexType CGirderSpacingData2::GetSpacingCount() const
{
   return m_GirderSpacing.size();
}

Float64 CGirderSpacingData2::GetGirderSpacing(SpacingIndexType spacingIdx) const
{
   if ( m_GirderSpacing.size() == 0 )
   {
      return 0;
   }
   else
   {
      _ASSERT( 0 <= spacingIdx && spacingIdx < (SpacingIndexType)m_GirderSpacing.size() );
      return m_GirderSpacing[spacingIdx];
   }
}

void CGirderSpacingData2::InitGirderCount(GirderIndexType nGirders)
{
   m_SpacingGroups.clear();
   m_GirderSpacing.clear();

   SpacingGroup group;
   group.first = 0;
   group.second = nGirders-1;

   m_SpacingGroups.push_back(group);

   m_GirderSpacing.insert(m_GirderSpacing.end(),nGirders-1,m_DefaultSpacing);
}

void CGirderSpacingData2::AddGirders(GirderIndexType nGirders)
{
   if ( m_GirderSpacing.size() == 0 )
   {
      SpacingGroup group;
      group.first = 0;
      group.second = nGirders-1;

      m_SpacingGroups.push_back(group);

      SpacingIndexType nSpaces = nGirders-1; // there is one fewer spaces then girders
      m_GirderSpacing.insert(m_GirderSpacing.end(),nSpaces,m_DefaultSpacing);
   }
   else
   {
      SpacingIndexType nSpaces = nGirders; // adding the same number of spaces as girders
      Float64 spacing = m_GirderSpacing.back();
      m_GirderSpacing.insert(m_GirderSpacing.end(),nSpaces, spacing );

      SpacingGroup& group = m_SpacingGroups.back();
      group.second += nGirders;
   }
}

void CGirderSpacingData2::RemoveGirders(GirderIndexType nGirdersToRemove)
{
   _ASSERT( nGirdersToRemove <= (GirderIndexType)(m_GirderSpacing.size()+1) ); // trying to remove too many girders

   Float64 default_spacing = m_GirderSpacing.front(); // hang onto this spacing before we change the size of the vector

   SpacingIndexType nSpacesToRemove = nGirdersToRemove;
   for ( SpacingIndexType i = 0; i < nSpacesToRemove; i++ )
      m_GirderSpacing.pop_back();

   GirderIndexType nGirders = m_GirderSpacing.size() + 1;

   // fix up the girder groups
   if ( nGirders == 1 )
   {
      // if there is only one girder left, then there is no spacing
      m_DefaultSpacing = default_spacing; // the default spacing, if girders are added back, is the last spacing used before nGirders when to 1
      m_GirderSpacing.clear();
      m_SpacingGroups.clear();
   }
   else
   {
      std::vector<SpacingGroup>::iterator iter(m_SpacingGroups.begin());
      std::vector<SpacingGroup>::iterator end(m_SpacingGroups.end());
      GirderIndexType gdrCount = 0;
      for ( ; iter != end; iter++ )
      {
         SpacingGroup& group = *iter;

         if ( nGirders-1 <= group.second )
         {
            // the current group goes beyond the number of girders..
            group.second = nGirders-1;
            iter++;
            break;
         }
      }

      m_SpacingGroups.erase(iter,m_SpacingGroups.end());
   }
}

void CGirderSpacingData2::SetGirderCount(GirderIndexType nGirders)
{
   if ( m_GirderSpacing.size() == 0 && nGirders == 1 )
   {
      return;
   }

   if ( m_GirderSpacing.size() == 0 )
   {
      AddGirders(nGirders);
   }
   else
   {
      if ( nGirders < m_GirderSpacing.size()+1 )
      {
         RemoveGirders(m_GirderSpacing.size() + 1 - nGirders);
      }
      else
      {
         AddGirders(nGirders - (m_GirderSpacing.size()+1));
      }
   }
}

#if defined _DEBUG
void CGirderSpacingData2::AssertValid() const
{
   ATLASSERT( m_MeasurementType     == pgsTypes::AlongItem  || m_MeasurementType     == pgsTypes::NormalToItem );
   ATLASSERT( m_MeasurementLocation == pgsTypes::AtPierLine || m_MeasurementLocation == pgsTypes::AtCenterlineBearing );
}
#endif


/****************************************************************************
CLASS
   CGirderSpacing2
****************************************************************************/
CGirderSpacing2::CGirderSpacing2()
{
   m_pPier = NULL;
   m_pTempSupport = NULL;
}

CGirderSpacing2::CGirderSpacing2(const CGirderSpacing2& rOther)
{
   m_pPier = NULL;
   m_pTempSupport = NULL;
   MakeCopy(rOther);
}

CGirderSpacing2::CGirderSpacing2(const CGirderSpacingData2& rOther)
{
   m_pPier = NULL;
   m_pTempSupport = NULL;
   CGirderSpacingData2::MakeCopy(rOther);
}

CGirderSpacing2::~CGirderSpacing2()
{
}


CGirderSpacing2& CGirderSpacing2::operator= (const CGirderSpacing2& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

void CGirderSpacing2::SetPier(const CPierData2* pPier)
{
   m_pPier = pPier;
   m_pTempSupport = NULL;
}

const CPierData2* CGirderSpacing2::GetPier() const
{
   return m_pPier;
}

void CGirderSpacing2::SetTemporarySupport(const CTemporarySupportData* pTS)
{
   m_pTempSupport = pTS;
   m_pPier = NULL;
}

const CTemporarySupportData* CGirderSpacing2::GetTemporarySupport() const
{
   return m_pTempSupport;
}

pgsTypes::MeasurementType CGirderSpacing2::GetMeasurementType() const
{
   const CBridgeDescription2* pBridgeDesc = GetBridgeDescription();
   if ( m_pTempSupport == NULL && pBridgeDesc && IsBridgeSpacing(pBridgeDesc->GetGirderSpacingType()) )
   {
      return pBridgeDesc->GetMeasurementType();
   }
   else
   {
      return CGirderSpacingData2::GetMeasurementType();
   }
}

pgsTypes::MeasurementLocation CGirderSpacing2::GetMeasurementLocation() const
{
   const CBridgeDescription2* pBridgeDesc = GetBridgeDescription();
   if ( m_pTempSupport == NULL && pBridgeDesc && IsBridgeSpacing(pBridgeDesc->GetGirderSpacingType()) )
   {
      return pBridgeDesc->GetMeasurementLocation();
   }
   else
   {
      return CGirderSpacingData2::GetMeasurementLocation();
   }
}

GirderIndexType CGirderSpacing2::GetRefGirder() const
{
   const CBridgeDescription2* pBridgeDesc = GetBridgeDescription();
   if ( pBridgeDesc && IsBridgeSpacing(pBridgeDesc->GetGirderSpacingType()) )
   {
      return pBridgeDesc->GetRefGirder();
   }
   else
   {
      return CGirderSpacingData2::GetRefGirder();
   }
}

Float64 CGirderSpacing2::GetRefGirderOffset() const
{
   const CBridgeDescription2* pBridgeDesc = GetBridgeDescription();
   if ( pBridgeDesc && IsBridgeSpacing(pBridgeDesc->GetGirderSpacingType()) )
   {
      return pBridgeDesc->GetRefGirderOffset();
   }
   else
   {
      return CGirderSpacingData2::GetRefGirderOffset();
   }
}

pgsTypes::OffsetMeasurementType CGirderSpacing2::GetRefGirderOffsetType() const
{
   const CBridgeDescription2* pBridgeDesc = GetBridgeDescription();
   if ( pBridgeDesc && IsBridgeSpacing(pBridgeDesc->GetGirderSpacingType()) )
   {
      return pBridgeDesc->GetRefGirderOffsetType();
   }
   else
   {
      return CGirderSpacingData2::GetRefGirderOffsetType();
   }
}

void CGirderSpacing2::SetGirderSpacing(GroupIndexType spacingGroupIdx,Float64 s)
{
   if ( 1 < GetGirderCount() )
   {
      CGirderSpacingData2::SetGirderSpacing(spacingGroupIdx,s);
   }
}

GroupIndexType CGirderSpacing2::GetSpacingGroupCount() const
{
   const CBridgeDescription2* pBridgeDesc = GetBridgeDescription();
   if ( pBridgeDesc && IsBridgeSpacing(pBridgeDesc->GetGirderSpacingType()) )
   {
      if ( pBridgeDesc->UseSameNumberOfGirdersInAllGroups() )
      {
         return 1;
      }
      else
      {
         return CGirderSpacingData2::GetSpacingGroupCount();
      }
   }
   else
   {
      return CGirderSpacingData2::GetSpacingGroupCount();
   }
}

void CGirderSpacing2::GetSpacingGroup(GroupIndexType spacingGroupIdx,GirderIndexType* pFirstGdrIdx,GirderIndexType* pLastGdrIdx,Float64* pSpacing) const
{
   const CBridgeDescription2* pBridgeDesc = GetBridgeDescription();
   if ( pBridgeDesc && IsBridgeSpacing(pBridgeDesc->GetGirderSpacingType()) )
   {
      // this spacing object is attached to a bridge model
      // the spacing is defined at the bridge model level (one spacing used everywhere)
      *pSpacing = pBridgeDesc->GetGirderSpacing();
      *pFirstGdrIdx = 0;

      if ( pBridgeDesc->UseSameNumberOfGirdersInAllGroups() )
      {
         // the same number of girders is used in all groups so the last girder
         // in this spacing group is the nGirders-1 from the bridge level
         *pLastGdrIdx = pBridgeDesc->GetGirderCount()-1;
      }
      else
      {
         // a different number of girders is used in each group.
         // the last girder in this group comes from the spacing group
         const SpacingGroup& group = m_SpacingGroups[spacingGroupIdx];
         *pLastGdrIdx  = group.second;
      }
   }
   else
   {
      _ASSERT( spacingGroupIdx < (SpacingIndexType)m_SpacingGroups.size() );
      const SpacingGroup& group = m_SpacingGroups[spacingGroupIdx];

      *pSpacing = m_GirderSpacing[group.first];
      *pFirstGdrIdx = group.first;
      *pLastGdrIdx  = group.second;

#if defined _DEBUG
      // make sure every spacing in the group has the value
      GirderIndexType firstGirderIdx = group.first;
      GirderIndexType lastGirderIdx  = group.second-1;
      for ( GirderIndexType i = firstGirderIdx + 1; i < lastGirderIdx; i++ )
      {
   //      _ASSERT(IsEqual(*pSpacing,m_GirderSpacing[group.first]));
         _ASSERT(IsEqual(*pSpacing,m_GirderSpacing[i]));
      }
#endif
   }
}

GirderIndexType CGirderSpacing2::GetGirderCount() const
{
   return GetSpacingCount()+1;
}

SpacingIndexType CGirderSpacing2::GetSpacingCount() const
{
   const CBridgeDescription2* pBridgeDesc = GetBridgeDescription();
   if ( pBridgeDesc && IsBridgeSpacing(pBridgeDesc->GetGirderSpacingType()) )
   {
      // this spacing object is connected to a bridge and the spacing type is for the whole bridge
      if ( pBridgeDesc->UseSameNumberOfGirdersInAllGroups() )
      {
         // using the same number of girders in all spans
         return pBridgeDesc->GetGirderCount()-1;
      }
      else
      {
         // using a different number of girders in each group
         const CGirderGroupData* pGroup;
         if ( m_pPier )
         {
            // this spacing is measured at a pier
            if ( m_pPier->GetGirderSpacing(pgsTypes::Back) == this )
            {
               // this spacing is for the back side of this pier
               // get the girder group for the span on the back side of this pier
               pGroup = pBridgeDesc->GetGirderGroup(m_pPier->GetSpan(pgsTypes::Back));
            }
            else
            {
               // this spacing is for the ahead side of this pier
               // get the girder group for the span on the ahead side of this pier
               pGroup = pBridgeDesc->GetGirderGroup(m_pPier->GetSpan(pgsTypes::Ahead));
            }
         }
         else if ( m_pTempSupport )
         {
            // this spacing is measured at a temporary support
            pGroup = pBridgeDesc->GetGirderGroup(m_pTempSupport->GetSpan());
         }

         if ( pGroup == NULL )
         {
            return CGirderSpacingData2::GetSpacingCount();
         }

         // get the number of girders in the group and compute the number of spaces
         GirderIndexType nGirders = pGroup->GetGirderCount();
         SpacingIndexType nSpaces = nGirders-1;
         return nSpaces;
      }
   }
   else
   {
      if ( pBridgeDesc && pBridgeDesc->UseSameNumberOfGirdersInAllGroups() )
      {
         // using the same number of girders in all spans
         return pBridgeDesc->GetGirderCount()-1;
      }

      return CGirderSpacingData2::GetSpacingCount();
   }
}

Float64 CGirderSpacing2::GetGirderSpacing(SpacingIndexType spacingIdx) const
{
   const CBridgeDescription2* pBridgeDesc = GetBridgeDescription();
   if ( pBridgeDesc && IsBridgeSpacing(pBridgeDesc->GetGirderSpacingType()) )
   {
      return pBridgeDesc->GetGirderSpacing();
   }
   else
   {
      return CGirderSpacingData2::GetGirderSpacing(spacingIdx);
   }
}

void CGirderSpacing2::MakeCopy(const CGirderSpacing2& rOther)
{
   CGirderSpacingData2::MakeCopy(rOther);
   m_pPier = rOther.m_pPier;
   m_pTempSupport = rOther.m_pTempSupport;
}

void CGirderSpacing2::MakeAssignment(const CGirderSpacing2& rOther)
{
   MakeCopy(rOther);
}

const CBridgeDescription2* CGirderSpacing2::GetBridgeDescription() const
{
   if ( m_pPier )
   {
      return m_pPier->GetBridgeDescription();
   }

   if ( m_pTempSupport && m_pTempSupport->GetSpan() )
   {
      return m_pTempSupport->GetSpan()->GetBridgeDescription();
   }

   return NULL;
}

Float64 CGirderSpacing2::GetSpacingWidth() const
{
   const CBridgeDescription2* pBridgeDesc = GetBridgeDescription();
   if ( ::IsJointSpacing(pBridgeDesc->GetGirderSpacingType()) )
   {
      Float64 total_width = 0;
      const CGirderGroupData* pGroup = GetGirderGroup();

      GirderIndexType nGirders = GetGirderCount();
      for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
      {
         const CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
         Float64 width = GetGirderWidth(pGirder);
         if ( gdrIdx == 0 || gdrIdx == nGirders-1 )
         {
            width /= 2;
         }

         Float64 joint_width = 0;
         if ( gdrIdx != nGirders-1 )
         {
            joint_width = m_GirderSpacing[gdrIdx];
         }

         total_width += width + joint_width;
      }

      return total_width;
   }
   else
   {
      return std::accumulate(m_GirderSpacing.begin(),m_GirderSpacing.end(),0.0);
   }
}

Float64 CGirderSpacing2::GetSpacingWidthToGirder(GirderIndexType nGirders) const
{
   const CBridgeDescription2* pBridgeDesc = GetBridgeDescription();
   if ( ::IsJointSpacing(pBridgeDesc->GetGirderSpacingType()) )
   {
      Float64 total_width = 0;
      const CGirderGroupData* pGroup = GetGirderGroup();

      for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
      {
         const CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
         Float64 width = GetGirderWidth(pGirder);
         Float64 joint_width = 0;
         if ( gdrIdx != nGirders-1 )
         {
            joint_width = m_GirderSpacing[gdrIdx];
         }

         total_width += width + joint_width;
      }

      return total_width;
   }
   else
   {
      return std::accumulate(m_GirderSpacing.begin(),m_GirderSpacing.begin()+nGirders,0.0);
   }
}

const CGirderGroupData* CGirderSpacing2::GetGirderGroup() const
{
   const CSpanData2* pSpan;
   if ( m_pPier )
   {
      if ( m_pPier->GetGirderSpacing(pgsTypes::Back) == this )
      {
         pSpan = m_pPier->GetPrevSpan();
      }
      else
      {
         pSpan = m_pPier->GetNextSpan();
      }
   }
   else if ( m_pTempSupport )
   {
      pSpan = m_pTempSupport->GetSpan();
   }
   else
   {
      // spacing data is not hooked up to a bridge model
      return NULL;
   }

   const CGirderGroupData* pGroup = GetBridgeDescription()->GetGirderGroup(pSpan);
   return pGroup;
}

Float64 CGirderSpacing2::GetGirderWidth(const CSplicedGirderData* pGirder) const
{
   const GirderLibraryEntry* pLibEntry = pGirder->GetGirderLibraryEntry();
   return pLibEntry->GetBeamWidth(pgsTypes::metStart);
}

#if defined _DEBUG
void CGirderSpacing2::AssertValid() const
{
   CGirderSpacingData2::AssertValid();

   const CGirderGroupData* pGroup = GetGirderGroup();
   if ( pGroup )
   {
      GirderIndexType nGirders = pGroup->m_Girders.size();
      bool bIsSpacingHere = false;
      if ( m_pPier )
      {
         bIsSpacingHere = m_pPier->HasSpacing();
      }
      else
      {
         bIsSpacingHere = m_pTempSupport->HasSpacing();
      }

      if ( bIsSpacingHere )
      {
         ATLASSERT(nGirders == m_GirderSpacing.size()+1);
         ATLASSERT(m_SpacingGroups.front().first == 0);
         ATLASSERT(m_SpacingGroups.back().second == nGirders-1);
      }
   }
}
#endif
