///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright (C) 2008  Washington State Department of Transportation
//                     Bridge and Structures Office
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
#include <PgsExt\GirderSpacing.h>
#include <PgsExt\BridgeDescription.h>
#include <PgsExt\SpanData.h>
#include <WbflAtlExt.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CGirderSpacingData
****************************************************************************/

CGirderSpacingData::CGirderSpacingData()
{
   m_MeasurementType     = pgsTypes::NormalToItem;
   m_MeasurementLocation = pgsTypes::AtCenterlinePier;
   
   m_RefGirderIdx = ALL_GIRDERS; // reference girder is the center of the girder group
   m_RefGirderOffsetType = pgsTypes::omtBridge;
   m_RefGirderOffset = 0;

   m_DefaultSpacing = ::ConvertToSysUnits(5.0,unitMeasure::Feet);
}

CGirderSpacingData::CGirderSpacingData(const CGirderSpacingData& rOther)
{
   MakeCopy(rOther);
}

CGirderSpacingData::~CGirderSpacingData()
{
}

CGirderSpacingData& CGirderSpacingData::operator= (const CGirderSpacingData& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

bool CGirderSpacingData::operator == (const CGirderSpacingData& rOther) const
{
   if ( m_MeasurementType != rOther.m_MeasurementType )
      return false;

   if ( m_MeasurementLocation != rOther.m_MeasurementLocation )
      return false;

   if ( m_RefGirderIdx != rOther.m_RefGirderIdx )
      return false;

   if ( m_RefGirderOffsetType != rOther.m_RefGirderOffsetType )
      return false;

   if ( !IsEqual(m_RefGirderOffset,rOther.m_RefGirderOffset) )
      return false;

   if ( m_GirderSpacing != rOther.m_GirderSpacing )
      return false;

   if ( m_SpacingGroups != rOther.m_SpacingGroups )
      return false;

   return true;
}

bool CGirderSpacingData::operator != (const CGirderSpacingData& rOther) const
{
   return !operator==(rOther);
}

HRESULT CGirderSpacingData::Load(IStructuredLoad* pStrLoad,IProgress* pProgress)
{
   USES_CONVERSION;

   CHRException hr;

   try
   {
      m_SpacingGroups.clear();
      m_GirderSpacing.clear();

      hr = pStrLoad->BeginUnit("GirderSpacing");

      double version;
      hr = pStrLoad->get_Version(&version);


      CComVariant var;

      var.vt = VT_I4;
      hr = pStrLoad->get_Property("MeasurementLocation",&var);
      m_MeasurementLocation = (pgsTypes::MeasurementLocation)(var.lVal);

      var.vt = VT_I4;
      hr = pStrLoad->get_Property("MeasurementType",&var);
      m_MeasurementType = (pgsTypes::MeasurementType)(var.lVal);

      if ( 2 <= version )
      {
         // added in version 2
         var.vt = VT_I2;
         hr = pStrLoad->get_Property("RefGirder",&var);
         m_RefGirderIdx = (GirderIndexType)var.iVal;

         hr = pStrLoad->get_Property("RefGirderOffsetType",&var);
         m_RefGirderOffsetType = (pgsTypes::OffsetMeasurementType)(var.iVal);

         var.vt = VT_R8;
         hr = pStrLoad->get_Property("RefGirderOffset",&var);
         m_RefGirderOffset = var.dblVal;
      }

      var.vt = VT_I4;
      hr = pStrLoad->get_Property("SpacingGroupCount", &var );

      long nGroups = var.lVal;
      for ( long grpIdx = 0; grpIdx < nGroups; grpIdx++ )
      {
         hr = pStrLoad->BeginUnit("SpacingGroup");

         double grp_version;
         hr = pStrLoad->get_Version(&grp_version);
         ATLASSERT(grp_version == 1);

         var.vt = VT_R8;
         hr = pStrLoad->get_Property("Spacing",&var);
         double spacing = var.dblVal;

         var.vt = VT_I2;
         hr = pStrLoad->get_Property("FirstGirderIndex",&var);
         GirderIndexType firstGdrIdx = var.iVal;

         var.vt = VT_I2;
         hr = pStrLoad->get_Property("LastGirderIndex",&var);
         GirderIndexType lastGdrIdx = var.iVal;

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
   catch(...)
   {
      ATLASSERT(0);
   }

   return hr;
}

HRESULT CGirderSpacingData::Save(IStructuredSave* pStrSave,IProgress* pProgress)
{
   HRESULT hr = S_OK;
   pStrSave->BeginUnit("GirderSpacing",2.0);

   pStrSave->put_Property("MeasurementLocation",CComVariant((long)m_MeasurementLocation));
   pStrSave->put_Property("MeasurementType",CComVariant((long)m_MeasurementType));

   // added in version 2
   pStrSave->put_Property("RefGirder",CComVariant(m_RefGirderIdx));
   pStrSave->put_Property("RefGirderOffsetType",CComVariant(m_RefGirderOffsetType));
   pStrSave->put_Property("RefGirderOffset",CComVariant(m_RefGirderOffset));

   pStrSave->put_Property("SpacingGroupCount",CComVariant((long)m_SpacingGroups.size()));

   std::vector<SpacingGroup>::iterator iter;
   for ( iter = m_SpacingGroups.begin(); iter != m_SpacingGroups.end(); iter++ )
   {
      SpacingGroup group = *iter;

      pStrSave->BeginUnit("SpacingGroup",1.0);

      pStrSave->put_Property("Spacing",CComVariant(m_GirderSpacing[group.first]));
      pStrSave->put_Property("FirstGirderIndex",CComVariant(group.first));
      pStrSave->put_Property("LastGirderIndex",CComVariant(group.second));

      pStrSave->EndUnit();
   }


   pStrSave->EndUnit();
   return hr;
}

void CGirderSpacingData::MakeCopy(const CGirderSpacingData& rOther)
{
   m_MeasurementType     = rOther.m_MeasurementType;
   m_MeasurementLocation = rOther.m_MeasurementLocation;

   m_RefGirderIdx        = rOther.m_RefGirderIdx;
   m_RefGirderOffset     = rOther.m_RefGirderOffset;
   m_RefGirderOffsetType = rOther.m_RefGirderOffsetType;

   m_GirderSpacing = rOther.m_GirderSpacing;
   m_SpacingGroups = rOther.m_SpacingGroups;

   m_DefaultSpacing = rOther.m_DefaultSpacing;

   _ASSERT( m_MeasurementType == pgsTypes::AlongItem || m_MeasurementType == pgsTypes::NormalToItem );
   _ASSERT( m_MeasurementLocation == pgsTypes::AtCenterlinePier || m_MeasurementLocation == pgsTypes::AtCenterlineBearing );
}

void CGirderSpacingData::MakeAssignment(const CGirderSpacingData& rOther)
{
   MakeCopy( rOther );
}

void CGirderSpacingData::SetGirderSpacing(GroupIndexType grpIdx,double s)
{
   GirderIndexType firstGdrIdx, lastGdrIdx;
   double spacing;
   GetSpacingGroup(grpIdx,&firstGdrIdx,&lastGdrIdx,&spacing);

   SpacingIndexType firstSpaceIdx = firstGdrIdx;
   SpacingIndexType lastSpaceIdx  = lastGdrIdx-1;

   _ASSERT( 0 <= firstSpaceIdx && firstSpaceIdx < (SpacingIndexType)m_GirderSpacing.size() );
   _ASSERT( 0 <= lastSpaceIdx  && lastSpaceIdx  < (SpacingIndexType)m_GirderSpacing.size() );
   for ( SpacingIndexType i = firstSpaceIdx; i <= lastSpaceIdx; i++ )
   {
      m_GirderSpacing[i] = s;
   }
}

void CGirderSpacingData::SetMeasurementType(pgsTypes::MeasurementType mt)
{
   _ASSERT( mt == pgsTypes::AlongItem || mt == pgsTypes::NormalToItem );
   m_MeasurementType = mt;
}

pgsTypes::MeasurementType CGirderSpacingData::GetMeasurementType() const
{
   return m_MeasurementType;
}

void CGirderSpacingData::SetMeasurementLocation(pgsTypes::MeasurementLocation ml)
{
   _ASSERT( ml == pgsTypes::AtCenterlinePier || ml == pgsTypes::AtCenterlineBearing );
   m_MeasurementLocation = ml;
}

pgsTypes::MeasurementLocation CGirderSpacingData::GetMeasurementLocation() const
{
   return m_MeasurementLocation;
}

void CGirderSpacingData::SetRefGirder(GirderIndexType refGdrIdx)
{
   m_RefGirderIdx = refGdrIdx;
}

GirderIndexType CGirderSpacingData::GetRefGirder() const
{
   return m_RefGirderIdx;
}

void CGirderSpacingData::SetRefGirderOffset(double offset)
{
   m_RefGirderOffset = offset;
}

double CGirderSpacingData::GetRefGirderOffset() const
{
   return m_RefGirderOffset;
}

void CGirderSpacingData::SetRefGirderOffsetType(pgsTypes::OffsetMeasurementType offsetDatum)
{
   m_RefGirderOffsetType = offsetDatum;
}

pgsTypes::OffsetMeasurementType CGirderSpacingData::GetRefGirderOffsetType() const
{
   return m_RefGirderOffsetType;
}

GroupIndexType CGirderSpacingData::GetSpacingGroupCount() const
{
   return m_SpacingGroups.size();
}

void CGirderSpacingData::GetSpacingGroup(GroupIndexType groupIdx,GirderIndexType* pFirstGdrIdx,GirderIndexType* pLastGdrIdx,double* pSpacing) const
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

void CGirderSpacingData::ExpandAll()
{
   m_SpacingGroups.clear();
   GirderIndexType nGirders = m_GirderSpacing.size();
   for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
   {
      m_SpacingGroups.push_back(SpacingGroup(gdrIdx,gdrIdx+1));
   }
}

void CGirderSpacingData::Expand(GroupIndexType groupIdx)
{
   GirderIndexType firstGdrIdx, lastGdrIdx;
   double spacing;
   GetSpacingGroup(groupIdx,&firstGdrIdx,&lastGdrIdx,&spacing);

   if ( (lastGdrIdx - firstGdrIdx) <= 1 )
      return; // there is only one girder in the group...nothing to expand

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

void CGirderSpacingData::JoinAll(SpacingIndexType spacingKey)
{
   if ( m_GirderSpacing.size() == 0 )
      return;

   double spacing = m_GirderSpacing[spacingKey];
   SpacingGroup firstGroup = m_SpacingGroups.front();
   SpacingGroup lastGroup  = m_SpacingGroups.back();

   SpacingGroup joinedGroup;
   joinedGroup.first  = firstGroup.first;
   joinedGroup.second = lastGroup.second;

   m_SpacingGroups.clear();
   m_SpacingGroups.push_back(joinedGroup);

   std::vector<double>::iterator iter;
   for ( iter = m_GirderSpacing.begin(); iter != m_GirderSpacing.end(); iter++ )
   {
      (*iter) = spacing;
   }
}

void CGirderSpacingData::Join(GirderIndexType firstGdrIdx,GirderIndexType lastGdrIdx,SpacingIndexType spacingKey)
{
   _ASSERT(firstGdrIdx != lastGdrIdx);

   // girder index must be in the range
   _ASSERT( firstGdrIdx <= spacingKey && spacingKey <= lastGdrIdx );

   // get the girder spacing for the group
   double spacing = m_GirderSpacing[spacingKey];

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
         break;
      else
         spacingGroups.push_back(spacingGroup);
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
      spacingGroups.insert(spacingGroups.end(),iter,m_SpacingGroups.end());

   // finally replace the data member with the local girder groups
   m_SpacingGroups = spacingGroups;
}

SpacingIndexType CGirderSpacingData::GetSpacingCount() const
{
   return m_GirderSpacing.size();
}

double CGirderSpacingData::GetGirderSpacing(SpacingIndexType spacingIdx) const
{
   _ASSERT( 0 <= spacingIdx && spacingIdx < (SpacingIndexType)m_GirderSpacing.size() );
   return m_GirderSpacing[spacingIdx];
}

void CGirderSpacingData::AddGirders(GirderIndexType nGirders)
{
   if ( m_GirderSpacing.size() == 0 )
   {
      SpacingGroup group;
      group.first = 0;
      group.second = nGirders-1;

      // this takes care of the case when we have 1 girder and are adding 1 girder
      if( group.first == group.second && group.first == 0 )
      {
         group.second++;
      }

      m_SpacingGroups.push_back(group);

      for ( SpacingIndexType i = group.first; i < group.second; i++ )
      {
         m_GirderSpacing.push_back(m_DefaultSpacing);
      }
   }
   else
   {
      double spacing = m_GirderSpacing.back();
      m_GirderSpacing.insert(m_GirderSpacing.end(),nGirders, spacing );

      SpacingGroup& group = m_SpacingGroups.back();
      group.second += nGirders;
   }
}

void CGirderSpacingData::RemoveGirders(GirderIndexType nGirdersToRemove)
{
   _ASSERT( nGirdersToRemove <= (GirderIndexType)(m_GirderSpacing.size()+1) ); // trying to remove too many girders

   SpacingIndexType nSpacesToRemove = nGirdersToRemove;
   for ( SpacingIndexType i = 0; i < nSpacesToRemove; i++ )
      m_GirderSpacing.pop_back();

   GirderIndexType nGirders = m_GirderSpacing.size() + 1;

   // fix up the girder groups
   if ( nGirders == 1 )
   {
      // if there is only one girder left, then there is no spacing
      m_DefaultSpacing = m_GirderSpacing.front();
      m_GirderSpacing.clear();
      m_SpacingGroups.clear();
   }
   else
   {
      std::vector<SpacingGroup>::iterator iter;
      GirderIndexType gdrCount = 0;
      for ( iter = m_SpacingGroups.begin(); iter != m_SpacingGroups.end(); iter++ )
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

void CGirderSpacingData::SetGirderCount(GirderIndexType nGirders)
{
   if ( m_GirderSpacing.size() == 0 && nGirders == 1 )
      return;

   long nDeltaGirders;
   if ( m_GirderSpacing.size() == 0 )
      nDeltaGirders = nGirders;
   else
      nDeltaGirders = nGirders - (m_GirderSpacing.size()+1);

   if ( nDeltaGirders < 0 )
      RemoveGirders((GirderIndexType)(-nDeltaGirders));
   else
      AddGirders((GirderIndexType)nDeltaGirders);
}


/****************************************************************************
CLASS
   CGirderSpacing
****************************************************************************/
CGirderSpacing::CGirderSpacing()
{
   m_pSpan = NULL;
}

CGirderSpacing::CGirderSpacing(const CGirderSpacing& rOther)
{
   m_pSpan = NULL;
   MakeCopy(rOther);
}

CGirderSpacing::CGirderSpacing(const CGirderSpacingData& rOther)
{
   m_pSpan = NULL;
   CGirderSpacingData::MakeCopy(rOther);
}

CGirderSpacing::~CGirderSpacing()
{
}


CGirderSpacing& CGirderSpacing::operator= (const CGirderSpacing& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

void CGirderSpacing::SetSpan(const CSpanData* pSpan)
{
   m_pSpan = pSpan;
}

pgsTypes::MeasurementType CGirderSpacing::GetMeasurementType() const
{
   if ( m_pSpan && IsBridgeSpacing(m_pSpan->GetBridgeDescription()->GetGirderSpacingType()) )
   {
      return m_pSpan->GetBridgeDescription()->GetMeasurementType();
   }
   else
   {
      return CGirderSpacingData::GetMeasurementType();
   }
}

pgsTypes::MeasurementLocation CGirderSpacing::GetMeasurementLocation() const
{
   if ( m_pSpan && IsBridgeSpacing(m_pSpan->GetBridgeDescription()->GetGirderSpacingType()) )
   {
      return m_pSpan->GetBridgeDescription()->GetMeasurementLocation();
   }
   else
   {
      return CGirderSpacingData::GetMeasurementLocation();
   }
}

GirderIndexType CGirderSpacing::GetRefGirder() const
{
   if ( m_pSpan && IsBridgeSpacing(m_pSpan->GetBridgeDescription()->GetGirderSpacingType()) )
   {
      return m_pSpan->GetBridgeDescription()->GetRefGirder();
   }
   else
   {
      return CGirderSpacingData::GetRefGirder();
   }
}

double CGirderSpacing::GetRefGirderOffset() const
{
   if ( m_pSpan && IsBridgeSpacing(m_pSpan->GetBridgeDescription()->GetGirderSpacingType()) )
   {
      return m_pSpan->GetBridgeDescription()->GetRefGirderOffset();
   }
   else
   {
      return CGirderSpacingData::GetRefGirderOffset();
   }
}

pgsTypes::OffsetMeasurementType CGirderSpacing::GetRefGirderOffsetType() const
{
   if ( m_pSpan && IsBridgeSpacing(m_pSpan->GetBridgeDescription()->GetGirderSpacingType()) )
   {
      return m_pSpan->GetBridgeDescription()->GetRefGirderOffsetType();
   }
   else
   {
      return CGirderSpacingData::GetRefGirderOffsetType();
   }
}

void CGirderSpacing::SetGirderSpacing(GroupIndexType grpIdx,double s)
{
   if ( m_pSpan && m_pSpan->GetGirderCount() < 2 )
      return;

   CGirderSpacingData::SetGirderSpacing(grpIdx,s);
}

GroupIndexType CGirderSpacing::GetSpacingGroupCount() const
{
   if ( m_pSpan && IsBridgeSpacing(m_pSpan->GetBridgeDescription()->GetGirderSpacingType()) )
   {
      return 1;
   }
   else
   {
      return CGirderSpacingData::GetSpacingGroupCount();
   }
}

void CGirderSpacing::GetSpacingGroup(GroupIndexType groupIdx,GirderIndexType* pFirstGdrIdx,GirderIndexType* pLastGdrIdx,double* pSpacing) const
{
   if ( m_pSpan && IsBridgeSpacing(m_pSpan->GetBridgeDescription()->GetGirderSpacingType()) )
   {
      *pFirstGdrIdx = 0;
      *pLastGdrIdx = m_pSpan->GetGirderCount()-1;
      *pSpacing = m_pSpan->GetBridgeDescription()->GetGirderSpacing();
      return;
   }

   if ( m_pSpan && m_pSpan->GetGirderCount() < 2 )
   {
      // if there aren't any spaces, provide some defaults and get the heck outta here
      *pFirstGdrIdx = 0;
      *pLastGdrIdx = 0;
      *pSpacing = -999; // < 0 makes it clear that the spacing isn't real
      return;
   }

   CGirderSpacingData::GetSpacingGroup(groupIdx,pFirstGdrIdx,pLastGdrIdx,pSpacing);
}

SpacingIndexType CGirderSpacing::GetSpacingCount() const
{
   if ( m_pSpan && IsBridgeSpacing(m_pSpan->GetBridgeDescription()->GetGirderSpacingType()) )
   {
      return m_pSpan->GetGirderCount()-1;
   }
   else
   {
      return CGirderSpacingData::GetSpacingCount();
   }
}

double CGirderSpacing::GetGirderSpacing(SpacingIndexType spacingIdx) const
{
   if ( m_pSpan && IsBridgeSpacing(m_pSpan->GetBridgeDescription()->GetGirderSpacingType()) )
   {
      return m_pSpan->GetBridgeDescription()->GetGirderSpacing();
   }
   else
   {
      return CGirderSpacingData::GetGirderSpacing(spacingIdx);
   }
}

void CGirderSpacing::MakeCopy(const CGirderSpacing& rOther)
{
   m_pSpan = rOther.m_pSpan;
   CGirderSpacingData::MakeCopy(rOther);
}

void CGirderSpacing::MakeAssignment(const CGirderSpacing& rOther)
{
   MakeCopy(rOther);
}
