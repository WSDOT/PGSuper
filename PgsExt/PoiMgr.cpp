///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright (C) 1999  Washington State Department of Transportation
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
#include <PgsExt\PoiMgr.h>
#include <algorithm>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   pgsPoiMgr
****************************************************************************/



// Predicate STL objects.

class SamePlace
{
public:
   SamePlace(const pgsPointOfInterest& poi,Float64 tol) : m_Poi(poi), m_Tol(tol) {}

   bool operator()(const pgsPointOfInterest& other) const
   {
      if ( m_Poi.GetSpan() == other.GetSpan() &&
           m_Poi.GetGirder() == other.GetGirder() &&
           IsZero( m_Poi.GetDistFromStart() - other.GetDistFromStart(), m_Tol ) )
      {
         return true;
      }

      return false;
   }

   const pgsPointOfInterest& GetPoi() const { return m_Poi; }

private:
   const pgsPointOfInterest& m_Poi;
   Float64 m_Tol;
};

class ExactlySame
{
public:
   ExactlySame(const pgsPointOfInterest& poi,Float64 tol) : m_SamePlace(poi,tol) {}
   bool operator()(const pgsPointOfInterest& other) const
   {
      if ( m_SamePlace.operator()(other) &&  
           m_SamePlace.GetPoi().GetAttributes() == other.GetAttributes() )
      {
         return true;
      }

      return false;
   }

private:
   SamePlace m_SamePlace;
};

class FindByID
{
public:
   FindByID(PoiIDType id) : m_ID(id) {}
   bool operator()(const pgsPointOfInterest& other) const
   {
      if ( m_ID == other.GetID() )
      {
         return true;
      }

      return false;
   }

private:
   PoiIDType m_ID;
};


PoiIDType pgsPoiMgr::ms_NextID = 0;


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
pgsPoiMgr::pgsPoiMgr()
{
   m_Tolerance = ::ConvertToSysUnits( 1.0, unitMeasure::Millimeter );
   pgsPointOfInterest::SetTolerance( m_Tolerance );
}

pgsPoiMgr::pgsPoiMgr(const pgsPoiMgr& rOther)
{
   MakeCopy(rOther);
}

pgsPoiMgr::~pgsPoiMgr()
{
}

//======================== OPERATORS  =======================================
pgsPoiMgr& pgsPoiMgr::operator= (const pgsPoiMgr& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
PoiIDType pgsPoiMgr::AddPointOfInterest(const pgsPointOfInterest& poi)
{
   ATLASSERT( poi.GetStageCount() != 0 ); // poi must belong to at least one stage

   pgsPointOfInterest newpoi = poi;
   if ( m_Poi.size() != 0 )
   {
      std::vector<pgsPointOfInterest>::iterator i;
      for ( i = m_Poi.begin(); i != m_Poi.end(); i++ )
      {
         const pgsPointOfInterest& curpoi = *i;
         if ( AtSamePlace( curpoi, poi ) )
         {
            newpoi = Merge(curpoi,poi);
            m_Poi.erase(i); // remove current poi from vector.
            break;
         }
      }
   }

   if ( newpoi.m_ID < 0 )
   {
      // assert if we are about to roll over the id
      ATLASSERT(ms_NextID != Int16_Max-1);
      newpoi.m_ID = ms_NextID++;
   }

   m_Poi.push_back(newpoi);
   std::sort(m_Poi.begin(),m_Poi.end());

   return newpoi.m_ID;
}

void pgsPoiMgr::RemovePointOfInterest(const pgsPointOfInterest& poi)
{
   std::vector<pgsPointOfInterest>::iterator found;
   found = std::find_if(m_Poi.begin(), m_Poi.end(), ExactlySame(poi,m_Tolerance) );
   if ( found != m_Poi.end() )
   {
      m_Poi.erase(found);
      std::sort(m_Poi.begin(),m_Poi.end());
   }
}

void pgsPoiMgr::RemovePointOfInterest(SpanIndexType span,GirderIndexType gdr,Float64 distFromStart)
{
   ATLASSERT( span != ALL_SPANS );
   ATLASSERT( gdr  != ALL_GIRDERS );

   std::vector<pgsPointOfInterest>::iterator found;
   found = std::find_if(m_Poi.begin(), m_Poi.end(), SamePlace(pgsPointOfInterest(span,gdr,distFromStart),m_Tolerance) );
   if ( found != m_Poi.end() )
   {
      m_Poi.erase(found);
      std::sort(m_Poi.begin(),m_Poi.end());
   }
}

void pgsPoiMgr::RemoveAll()
{
   m_Poi.clear();
}

pgsPointOfInterest pgsPoiMgr::GetPointOfInterest(SpanIndexType span,GirderIndexType gdr,Float64 distFromStart) 
{
   ATLASSERT( span != ALL_SPANS );
   ATLASSERT( gdr  != ALL_GIRDERS );

   pgsPointOfInterest poi(span,gdr,distFromStart);
   std::vector<pgsPointOfInterest>::const_iterator found;
   found = std::find_if(m_Poi.begin(), m_Poi.end(), SamePlace(poi,m_Tolerance) );
   if ( found != m_Poi.end() )
      return (*found);

   return pgsPointOfInterest();
}

pgsPointOfInterest pgsPoiMgr::GetPointOfInterest(pgsTypes::Stage stage,SpanIndexType span,GirderIndexType gdr,Float64 distFromStart) 
{
   ATLASSERT( span != ALL_SPANS );
   ATLASSERT( gdr  != ALL_GIRDERS );

   pgsPointOfInterest poi(span,gdr,distFromStart);
   std::vector<pgsPointOfInterest>::const_iterator found;
   found = std::find_if(m_Poi.begin(), m_Poi.end(), SamePlace(poi,m_Tolerance) );
   if ( found != m_Poi.end() && found->HasStage(stage) )
      return (*found);

   return pgsPointOfInterest();
}

pgsPointOfInterest pgsPoiMgr::GetNearestPointOfInterest(SpanIndexType span,GirderIndexType gdr,Float64 distFromStart) 
{
   ATLASSERT( span != ALL_SPANS );
   ATLASSERT( gdr  != ALL_GIRDERS );

   // get the poi just for this span/girder
   std::vector<pgsPointOfInterest> vPOI;
   std::vector<pgsPointOfInterest>::const_iterator iter;
   for ( iter = m_Poi.begin(); iter != m_Poi.end(); iter++ )
   {
      const pgsPointOfInterest& poi = *iter;
      if ( poi.GetSpan() == span && poi.GetGirder() == gdr )
         vPOI.push_back(poi);
   }

   if ( vPOI.size() == 0 )
      return pgsPointOfInterest();

   std::vector<pgsPointOfInterest>::const_iterator iter1, iter2;
   iter1 = vPOI.begin();
   iter2 = iter1;
   iter2++;
   for ( ; iter2 != m_Poi.end(); iter1++, iter2++ )
   {
      const pgsPointOfInterest& prevPOI = *iter1;
      const pgsPointOfInterest& nextPOI = *iter2;

      ATLASSERT( prevPOI.GetSpan()   == span );
      ATLASSERT( prevPOI.GetGirder() == gdr  );
      ATLASSERT( nextPOI.GetSpan()   == span );
      ATLASSERT( nextPOI.GetGirder() == gdr  );

      if ( InRange(prevPOI.GetDistFromStart(),distFromStart,nextPOI.GetDistFromStart()) )
      {
         Float64 d1 = distFromStart - prevPOI.GetDistFromStart();
         Float64 d2 = nextPOI.GetDistFromStart() - distFromStart;

         if ( d1 < d2 )
            return prevPOI;
         else
            return nextPOI;
      }
   }

   return pgsPointOfInterest();
}

pgsPointOfInterest pgsPoiMgr::GetNearestPointOfInterest(pgsTypes::Stage stage,SpanIndexType span,GirderIndexType gdr,Float64 distFromStart) 
{
   ATLASSERT( span != ALL_SPANS );
   ATLASSERT( gdr  != ALL_GIRDERS );

   // get the poi just for this span/girder and stage
   std::vector<pgsPointOfInterest> vPOI;
   std::vector<pgsPointOfInterest>::const_iterator iter;
   for ( iter = m_Poi.begin(); iter != m_Poi.end(); iter++ )
   {
      const pgsPointOfInterest& poi = *iter;
      if ( poi.HasStage(stage) && poi.GetSpan() == span && poi.GetGirder() == gdr )
         vPOI.push_back(poi);
   }

   if ( vPOI.size() == 0 )
      return pgsPointOfInterest();

   std::vector<pgsPointOfInterest>::const_iterator iter1, iter2;
   iter1 = vPOI.begin();
   iter2 = iter1;
   iter2++;
   for ( ; iter2 != vPOI.end(); iter1++, iter2++ )
   {
      const pgsPointOfInterest& prevPOI = *iter1;
      const pgsPointOfInterest& nextPOI = *iter2;

      ATLASSERT( prevPOI.GetSpan()   == span );
      ATLASSERT( prevPOI.GetGirder() == gdr  );
      ATLASSERT( nextPOI.GetSpan()   == span );
      ATLASSERT( nextPOI.GetGirder() == gdr  );

      if ( InRange(prevPOI.GetDistFromStart(),distFromStart,nextPOI.GetDistFromStart()) )
      {
         Float64 d1 = distFromStart - prevPOI.GetDistFromStart();
         Float64 d2 = nextPOI.GetDistFromStart() - distFromStart;

         if ( d1 < d2 )
            return prevPOI;
         else
            return nextPOI;
      }
   }

   return pgsPointOfInterest();
}

pgsPointOfInterest pgsPoiMgr::GetPointOfInterest(PoiIDType id) const
{
   std::vector<pgsPointOfInterest>::const_iterator found;
   found = std::find_if(m_Poi.begin(), m_Poi.end(), FindByID(id) );
   if ( found != m_Poi.end() )
      return (*found);

   return pgsPointOfInterest();
}

void pgsPoiMgr::GetPointsOfInterest(SpanIndexType span,GirderIndexType gdr,PoiAttributeType attrib,Uint32 mode,std::vector<pgsPointOfInterest>* pPois) const
{
   if ( mode == POIMGR_AND )
      AndFind(span,gdr,attrib,pPois);
   else
      OrFind(span,gdr,attrib,pPois);
}

void pgsPoiMgr::GetPointsOfInterest(pgsTypes::Stage stage,SpanIndexType span,GirderIndexType gdr,PoiAttributeType attrib,Uint32 mode,std::vector<pgsPointOfInterest>* pPois) const
{
   if ( mode == POIMGR_AND )
      AndFind(stage,span,gdr,attrib,pPois);
   else
      OrFind(stage,span,gdr,attrib,pPois);
}

void pgsPoiMgr::GetPointsOfInterest(std::set<pgsTypes::Stage> stages,SpanIndexType span,GirderIndexType gdr,PoiAttributeType attrib,Uint32 mode,std::vector<pgsPointOfInterest>* pPois) const
{
   if ( mode == POIMGR_AND )
      AndFind(stages,span,gdr,attrib,pPois);
   else
      OrFind(stages,span,gdr,attrib,pPois);
}

//======================== ACCESS     =======================================
void pgsPoiMgr::SetTolerance(Float64 tol)
{
   m_Tolerance = tol;
   pgsPointOfInterest::SetTolerance( m_Tolerance );
}

Float64 pgsPoiMgr::GetTolerance() const
{
   return m_Tolerance;
}

//======================== INQUIRY    =======================================
Uint32 pgsPoiMgr::GetPointOfInterestCount() const
{
   return m_Poi.size();
}

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void pgsPoiMgr::MakeCopy(const pgsPoiMgr& rOther)
{
   // Add copy code here...
   m_Poi       = rOther.m_Poi;
   m_Tolerance = rOther.m_Tolerance;
}

void pgsPoiMgr::MakeAssignment(const pgsPoiMgr& rOther)
{
   MakeCopy( rOther );
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PRIVATE    ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
bool pgsPoiMgr::AtSamePlace(const pgsPointOfInterest& a,const pgsPointOfInterest& b)
{
   if ( a.GetSpan()   == b.GetSpan()   &&
        a.GetGirder() == b.GetGirder() &&
        IsZero( a.GetDistFromStart() - b.GetDistFromStart(), m_Tolerance ) )
   {
      return true;
   }

   return false;
}

pgsPointOfInterest pgsPoiMgr::Merge(const pgsPointOfInterest& a,const pgsPointOfInterest& b)
{
   ATLASSERT( AtSamePlace(a,b) );
   pgsPointOfInterest poi(a);
   poi.SetAttributes( a.GetAttributes() | b.GetAttributes() );
   poi.m_Stages.insert(b.m_Stages.begin(),b.m_Stages.end());

   return poi;
}

void pgsPoiMgr::GetTenthPointPOIs(pgsTypes::Stage stage,SpanIndexType span,GirderIndexType gdr,std::vector<pgsPointOfInterest>* pPois) const
{
   ATLASSERT( gdr  != ALL_GIRDERS );
   PoiAttributeType basisType = (stage == pgsTypes::CastingYard ? POI_GIRDER_POINT : POI_SPAN_POINT);
   pPois->clear();
   std::vector<pgsPointOfInterest>::const_iterator i;
   for ( i = m_Poi.begin(); i != m_Poi.end(); i++ )
   {
      const pgsPointOfInterest& poi = *i;

      if ( poi.HasStage(stage) && (poi.GetSpan() == span || span == ALL_SPANS) && (poi.GetGirder() == gdr) && poi.IsATenthPoint(basisType) )
      {
         pPois->push_back( poi );
      }
   }
}

void pgsPoiMgr::AndFind(pgsTypes::Stage stage,SpanIndexType span,GirderIndexType gdr,PoiAttributeType attrib,std::vector<pgsPointOfInterest>* pPois) const
{
   ATLASSERT( gdr  != ALL_GIRDERS );

   pPois->clear();
   std::vector<pgsPointOfInterest>::const_iterator i;
   for ( i = m_Poi.begin(); i != m_Poi.end(); i++ )
   {
      const pgsPointOfInterest& poi = *i;
      if ( poi.HasStage(stage) && AndFind(poi,span,gdr,attrib) )
         pPois->push_back(poi);
   }
}

void pgsPoiMgr::AndFind(std::set<pgsTypes::Stage> stages,SpanIndexType span,GirderIndexType gdr,PoiAttributeType attrib,std::vector<pgsPointOfInterest>* pPois) const
{
   ATLASSERT( gdr  != ALL_GIRDERS );

   pPois->clear();
   std::vector<pgsPointOfInterest>::const_iterator i;
   for ( i = m_Poi.begin(); i != m_Poi.end(); i++ )
   {
      const pgsPointOfInterest& poi = *i;
      bool bHasStage = true;
      std::set<pgsTypes::Stage>::iterator iter;
      for ( iter = stages.begin(); iter != stages.end(); iter++)
      {
         if ( !poi.HasStage(*iter) )
         {
            bHasStage = false;
            break;
         }
      }

      if ( bHasStage && AndFind(poi,span,gdr,attrib) )
         pPois->push_back(poi);
   }
}

void pgsPoiMgr::AndFind(SpanIndexType span,GirderIndexType gdr,PoiAttributeType attrib,std::vector<pgsPointOfInterest>* pPois) const
{
   ATLASSERT( gdr  != ALL_GIRDERS );

   pPois->clear();
   std::vector<pgsPointOfInterest>::const_iterator i;
   for ( i = m_Poi.begin(); i != m_Poi.end(); i++ )
   {
      const pgsPointOfInterest& poi = *i;
      if ( AndFind(poi,span,gdr,attrib) )
         pPois->push_back(poi);
   }
}

bool pgsPoiMgr::AndFind(const pgsPointOfInterest& poi,SpanIndexType span,GirderIndexType gdr,PoiAttributeType attrib) const
{
   // TRICKY CODE
   // This if expression first check to make sure we have the same span and girder, then
   // it check if each flag in attrib is set, if it is, the corrosponding flag in poi must
   // be set, otherwise, the test is irrelavent (and always passes as indicated by the true)
   if ( (poi.GetSpan() == span || span == ALL_SPANS) && poi.GetGirder() == gdr &&
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_FLEXURECAPACITY) ? poi.IsFlexureCapacity()         : true) &&
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_FLEXURESTRESS)   ? poi.IsFlexureStress()           : true) &&
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_SHEAR)           ? poi.IsShear()                   : true) &&
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_DISPLACEMENT)    ? poi.IsDisplacement()            : true) &&
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_CRITSECTSHEAR1)  ? poi.HasAttribute(POI_CRITSECTSHEAR1) : true) &&
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_CRITSECTSHEAR2)  ? poi.HasAttribute(POI_CRITSECTSHEAR2) : true) &&
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_HARPINGPOINT)    ? poi.IsHarpingPoint()            : true) &&
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_PICKPOINT)       ? poi.HasAttribute(POI_PICKPOINT) : true) &&
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_BUNKPOINT)       ? poi.HasAttribute(POI_BUNKPOINT) : true) &&
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_CONCLOAD)        ? poi.IsConcentratedLoad()        : true) &&
	    (sysFlags<PoiAttributeType>::IsSet(attrib,POI_MIDSPAN)         ? poi.IsMidSpan()                 : true) &&
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_TABULAR)         ? poi.IsTabular()                 : true) &&
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_GRAPHICAL)       ? poi.IsGraphical()               : true) &&
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_H)               ? poi.IsAtH()                     : true) &&
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_15H)             ? poi.IsAt15H()                   : true) &&
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_PSXFER)          ? poi.HasAttribute(POI_PSXFER) : true) &&
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_SECTCHANGE)      ? poi.HasAttribute(POI_SECTCHANGE) : true) &&
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_FACEOFSUPPORT)   ? poi.HasAttribute(POI_FACEOFSUPPORT) : true)
      )
   {
      // This poi matches the selection criteria. Add it to the vector
      return true;
   }
   return false;
}

void pgsPoiMgr::OrFind(pgsTypes::Stage stage,SpanIndexType span,GirderIndexType gdr,PoiAttributeType attrib,std::vector<pgsPointOfInterest>* pPois) const
{
   ATLASSERT( gdr  != ALL_GIRDERS );

   pPois->clear();
   std::vector<pgsPointOfInterest>::const_iterator i;
   for ( i = m_Poi.begin(); i != m_Poi.end(); i++ )
   {
      const pgsPointOfInterest& poi = *i;
      if ( poi.HasStage(stage) && OrFind(poi,span,gdr,attrib) )
      {
         pPois->push_back( poi );
      }
   }
}

void pgsPoiMgr::OrFind(std::set<pgsTypes::Stage> stages,SpanIndexType span,GirderIndexType gdr,PoiAttributeType attrib,std::vector<pgsPointOfInterest>* pPois) const
{
   ATLASSERT( gdr  != ALL_GIRDERS );

   pPois->clear();
   std::vector<pgsPointOfInterest>::const_iterator i;
   for ( i = m_Poi.begin(); i != m_Poi.end(); i++ )
   {
      const pgsPointOfInterest& poi = *i;
      bool bHasStage = true;
      std::set<pgsTypes::Stage>::iterator iter;
      for ( iter = stages.begin(); iter != stages.end(); iter++)
      {
         if ( !poi.HasStage(*iter) )
         {
            bHasStage = false;
            break;
         }
      }

      if ( bHasStage && OrFind(poi,span,gdr,attrib) )
         pPois->push_back(poi);
   }
}

void pgsPoiMgr::OrFind(SpanIndexType span,GirderIndexType gdr,PoiAttributeType attrib,std::vector<pgsPointOfInterest>* pPois) const
{
   ATLASSERT( gdr  != ALL_GIRDERS );

   pPois->clear();
   std::vector<pgsPointOfInterest>::const_iterator i;
   for ( i = m_Poi.begin(); i != m_Poi.end(); i++ )
   {
      const pgsPointOfInterest& poi = *i;
      if ( OrFind(poi,span,gdr,attrib) )
      {
         pPois->push_back( poi );
      }
   }
}

bool pgsPoiMgr::OrFind(const pgsPointOfInterest& poi,SpanIndexType span,GirderIndexType gdr,PoiAttributeType attrib) const
{
   ATLASSERT( gdr  != ALL_GIRDERS );

   // TRICKY CODE
   // This if expression first check to make sure we have the same span and girder, then
   // it check if each flag in attrib is set, if it is, the corrosponding flag in poi must
   // be set, otherwise, the test is irrelavent (and always passes as indicated by the true)
   if ( (poi.GetSpan() == span || span == ALL_SPANS) && poi.GetGirder() == gdr )
   {
      if (
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_FLEXURECAPACITY) ? poi.IsFlexureCapacity()         : false) ||
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_FLEXURESTRESS)   ? poi.IsFlexureStress()           : false) ||
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_SHEAR)           ? poi.IsShear()                   : false) ||
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_DISPLACEMENT)    ? poi.IsDisplacement()            : false) ||
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_CRITSECTSHEAR1)  ? poi.HasAttribute(POI_CRITSECTSHEAR1) : false) ||
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_CRITSECTSHEAR2)  ? poi.HasAttribute(POI_CRITSECTSHEAR2) : false) ||
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_HARPINGPOINT)    ? poi.IsHarpingPoint()            : false) ||
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_PICKPOINT)       ? poi.HasAttribute(POI_PICKPOINT) : false) ||
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_BUNKPOINT)       ? poi.HasAttribute(POI_BUNKPOINT) : false) ||
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_CONCLOAD)        ? poi.IsConcentratedLoad()        : false) ||
	    (sysFlags<PoiAttributeType>::IsSet(attrib,POI_MIDSPAN)         ? poi.IsMidSpan()                 : false) ||
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_TABULAR)         ? poi.IsTabular()                 : false) ||
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_GRAPHICAL)       ? poi.IsGraphical()               : false) ||
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_H)               ? poi.IsAtH()                     : false) ||
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_15H)             ? poi.IsAt15H()                   : false) ||
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_DEBOND)          ? poi.HasAttribute(POI_DEBOND) : false) ||
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_PSXFER)          ? poi.HasAttribute(POI_PSXFER) : false) ||
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_SECTCHANGE)      ? poi.HasAttribute(POI_SECTCHANGE) : false) ||
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_FACEOFSUPPORT)   ? poi.HasAttribute(POI_FACEOFSUPPORT) : false)
      )

      {
         // This poi matches the selection criteria. Add it to the vector
         return true;
      }
   }

   return false;
}

//======================== ACCESS     =======================================
//======================== INQUERY    =======================================

//======================== DEBUG      =======================================
#if defined _DEBUG
bool pgsPoiMgr::AssertValid() const
{
   if ( m_Tolerance < 0 )
      return false;

   return true;
}

void pgsPoiMgr::Dump(dbgDumpContext& os) const
{
   os << "Dump for pgsPoiMgr" << endl;
   os << "m_Tolerance = " << m_Tolerance << endl;

   std::vector<pgsPointOfInterest>::const_iterator i;
   for ( i = m_Poi.begin(); i != m_Poi.end(); i++ )
   {
      const pgsPointOfInterest& poi = *i;
      poi.Dump(os);
   }
}
#endif // _DEBUG

#if defined _UNITTEST
bool pgsPoiMgr::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("pgsPoiMgr");

   // Create a poi mgr and fill it with some std::vector<pgsPointOfInterest>'s
   pgsPoiMgr mgr;

   mgr.AddPointOfInterest( pgsPointOfInterest(pgsTypes::CastingYard,0,0,0.00) );
   mgr.AddPointOfInterest( pgsPointOfInterest(pgsTypes::CastingYard,0,0,1.00) );
   mgr.AddPointOfInterest( pgsPointOfInterest(pgsTypes::CastingYard,0,0,2.00) );
   mgr.AddPointOfInterest( pgsPointOfInterest(pgsTypes::CastingYard,0,0,3.00) );
   mgr.AddPointOfInterest( pgsPointOfInterest(pgsTypes::CastingYard,0,0,4.00) );
   mgr.AddPointOfInterest( pgsPointOfInterest(pgsTypes::CastingYard,0,0,5.00) );

   TRY_TESTME( mgr.GetPointOfInterestCount() == 6 );

   // Add a std::vector<pgsPointOfInterest> very near poi #6.  The tolerancing should eliminate it.
   // Use all the non-standard std::vector<pgsPointOfInterest> attributes and verify the attributes merged correctly
   mgr.AddPointOfInterest( pgsPointOfInterest(0,0,4.99999, POI_CRITSECTSHEAR1 | POI_HARPINGPOINT | POI_CONCLOAD) );

   TRY_TESTME( mgr.GetPointOfInterestCount() == 6 );
   pgsPointOfInterest poi = mgr.GetPointOfInterest(0,0,5.0001);
   TRY_TESTME( poi.GetID() != -1 );
   TRY_TESTME( poi.IsConcentratedLoad() );
   TRY_TESTME( poi.IsDisplacement() );
   TRY_TESTME( poi.IsFlexureCapacity() );
   TRY_TESTME( poi.IsFlexureStress() );
   TRY_TESTME( poi.IsGraphical() );
   TRY_TESTME( poi.IsHarpingPoint() );
   TRY_TESTME( poi.IsShear() );
   TRY_TESTME( poi.IsTabular() );

   // Try to get a bogus std::vector<pgsPointOfInterest>
   TRY_TESTME( mgr.GetPointOfInterest(0,0,10000000.0).GetID() == -1 );
   
   // Get a vector of std::vector<pgsPointOfInterest> that meet a certain criteria
   std::vector<pgsPointOfInterest> pois;
   mgr.GetPointsOfInterest(pgsTypes::CastingYard,0,0,POI_HARPINGPOINT,POIMGR_AND,&pois);
   TRY_TESTME( pois.size() == 1 );
   TRY_TESTME( (*pois.begin()).IsHarpingPoint() );

   TESTME_EPILOG("PoiMgr");
}
#endif // _UNITTEST
