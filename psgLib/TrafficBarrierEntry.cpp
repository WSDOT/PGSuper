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

#include "StdAfx.h"

/****************************************************************************
CLASS
   TrafficBarrierEntry
****************************************************************************/

#include <psgLib\TrafficBarrierEntry.h>

#include <System\IStructuredSave.h>
#include <System\IStructuredLoad.h>
#include <System\XStructuredLoad.h>

#include "resource.h"
#include "TrafficBarrierDlg.h"
#include <Units\sysUnits.h>

#include <MathEx.h>
#include <WBFLGenericBridge.h>

#include <LRFD\ConcreteUtil.h>
#include <LRFD\VersionMgr.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// obsolete enum... need to use it to load old files
typedef enum Configuration
{
   ExteriorBarrier,
   ExteriorBarrier_Sidewalk,
   ExteriorBarrier_Sidewalk_InteriorBarrier
} Configuration;

////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
TrafficBarrierEntry::TrafficBarrierEntry() :
m_WeightMethod(Compute),
m_Weight(::ConvertToSysUnits(0.100,unitMeasure::KipPerFoot)),
m_CurbOffset(0)
{
   m_BarrierPoints.CoCreateInstance(CLSID_Point2dCollection);
   m_bStructurallyContinuous = false;

   Float64 fc = ::ConvertToSysUnits(4.0,unitMeasure::KSI);
   Float64 density = ::ConvertToSysUnits(155.0,unitMeasure::LbmPerFeet3);

   lrfdVersionMgr::Units old_units = lrfdVersionMgr::GetUnits();
   lrfdVersionMgr::SetUnits(lrfdVersionMgr::US);
   m_Ec = lrfdConcreteUtil::ModE(fc,density,false);
   lrfdVersionMgr::SetUnits(old_units);
}

TrafficBarrierEntry::TrafficBarrierEntry(const TrafficBarrierEntry& rOther) :
libLibraryEntry(rOther)
{
   m_BarrierPoints.CoCreateInstance(CLSID_Point2dCollection);

   MakeCopy(rOther);
}

TrafficBarrierEntry::~TrafficBarrierEntry()
{
}

//======================== OPERATORS  =======================================
TrafficBarrierEntry& TrafficBarrierEntry::operator= (const TrafficBarrierEntry& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
bool TrafficBarrierEntry::SaveMe(sysIStructuredSave* pSave)
{
   pSave->BeginUnit(_T("TrafficBarrierEntry"), 7.0);

   pSave->Property(_T("Name"),this->GetName().c_str());

   pSave->BeginUnit(_T("BarrierPoints"),1.0);
   
   CollectionIndexType count;
   m_BarrierPoints->get_Count(&count);
   pSave->Property(_T("Count"),count);

   CComPtr<IEnumPoint2d> enum_points;
   m_BarrierPoints->get__Enum(&enum_points);
   CComPtr<IPoint2d> point;
   while ( enum_points->Next(1,&point,NULL) != S_FALSE )
   {
      Float64 x,y;
      point->get_X(&x);
      point->get_Y(&y);

      pSave->BeginUnit(_T("Point"),1.0);
      pSave->Property(_T("X"),x);
      pSave->Property(_T("Y"),y);
      pSave->EndUnit();

      point.Release();
   }
   pSave->EndUnit();

   pSave->BeginUnit(_T("BarrierWeight"),1.0);
   pSave->Property(_T("WeightMethod"),(long)m_WeightMethod);
   pSave->Property(_T("Weight"),m_Weight);
   pSave->Property(_T("Ec"),m_Ec); // added in version 7
   pSave->EndUnit();

   pSave->Property(_T("CurbOffset"),m_CurbOffset); // added version 6

   pSave->Property(_T("IsBarrierStructurallyContinuous"),m_bStructurallyContinuous);

   pSave->EndUnit();

   return false;
}

bool TrafficBarrierEntry::LoadMe(sysIStructuredLoad* pLoad)
{
   m_BarrierPoints->Clear();

   if(pLoad->BeginUnit(_T("TrafficBarrierEntry")))
   {
      Float64 version = pLoad->GetVersion();
      if (7.0 < version)
         THROW_LOAD(BadVersion,pLoad);

      std::_tstring name;
      if(pLoad->Property(_T("Name"),&name))
         this->SetName(name.c_str());
      else
         THROW_LOAD(InvalidFileFormat,pLoad);

      if ( version < 3 ) // 2 or earlier
      {
         Float64 x1,x2,x3,x4,x5,y1,y2,y3;

         if(!pLoad->Property(_T("X1"), &x1))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if(!pLoad->Property(_T("X2"), &x2))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if(!pLoad->Property(_T("X3"), &x3))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if(!pLoad->Property(_T("X4"), &x4))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if(!pLoad->Property(_T("X5"), &x5))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if(!pLoad->Property(_T("Y1"), &y1))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if(!pLoad->Property(_T("Y2"), &y2))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if(!pLoad->Property(_T("Y3"), &y3))
            THROW_LOAD(InvalidFileFormat,pLoad);

         ConvertDimensionsToPoints(x1,x2,x3,x4,x5,y1,y2,y3);

         if ( 2 <= version ) // added in version 2 (skip for version 1)
         {
            long value;
            if ( !pLoad->Property(_T("WeightMethod"),&value) )
               THROW_LOAD(InvalidFileFormat,pLoad);

            m_WeightMethod = (WeightMethod)value;

            if ( !pLoad->Property(_T("Weight"),&m_Weight) )
               THROW_LOAD(InvalidFileFormat,pLoad);
         }
      }
      else
      {
         // version 3 and later
         CollectionIndexType count;
         Float64 x,y;

         Configuration configuration;

         if ( version < 5 )
         {
            ATLASSERT( version == 3 || version == 4 );

            // this is obsolete data... just load it and ignore it
            long config;
            if ( !pLoad->Property(_T("Configuration"),&config) )
               THROW_LOAD(InvalidFileFormat,pLoad);

            configuration = (Configuration)config;

            // this unit got renamed in version 5... load old name here
            if ( !pLoad->BeginUnit(_T("ExteriorBarrierPoints")) )
               THROW_LOAD(InvalidFileFormat,pLoad);
         }
         else
         {
            if ( !pLoad->BeginUnit(_T("BarrierPoints")) )
               THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("Count"),&count) )
            THROW_LOAD(InvalidFileFormat,pLoad);

         for ( CollectionIndexType i = 0; i < count; i++ )
         {
            if ( !pLoad->BeginUnit(_T("Point")) )
               THROW_LOAD(InvalidFileFormat,pLoad);

            if ( !pLoad->Property(_T("X"),&x) )
               THROW_LOAD(InvalidFileFormat,pLoad);

            if ( !pLoad->Property(_T("Y"),&y) )
               THROW_LOAD(InvalidFileFormat,pLoad);

            if ( !pLoad->EndUnit() )
               THROW_LOAD(InvalidFileFormat,pLoad);


            CComPtr<IPoint2d> p;
            p.CoCreateInstance(CLSID_Point2d);
            p->Move(x,y);
            m_BarrierPoints->Add(p);
         }

         if ( !pLoad->EndUnit() ) // end of BarrierPoints/ExteriorBarrierPoints
            THROW_LOAD(InvalidFileFormat,pLoad);

         if ( version < 5 )
         {
            ATLASSERT( version == 3 || version == 4 );

            // more obsolete data... load it and ignore it
            Float64 dummy_val;

            if ( configuration == ExteriorBarrier_Sidewalk ||
                 configuration == ExteriorBarrier_Sidewalk_InteriorBarrier )
            {
               if ( !pLoad->BeginUnit(_T("Sidewalk")) )
                  THROW_LOAD(InvalidFileFormat,pLoad);

               if ( !pLoad->Property(_T("H1"),&dummy_val) )
                  THROW_LOAD(InvalidFileFormat,pLoad);

               if ( !pLoad->Property(_T("H2"),&dummy_val) )
                  THROW_LOAD(InvalidFileFormat,pLoad);

               if ( configuration == ExteriorBarrier_Sidewalk )
               {
                  if ( !pLoad->Property(_T("W"),&dummy_val) )
                     THROW_LOAD(InvalidFileFormat,pLoad);
               }

               if ( !pLoad->EndUnit() ) // end of Sidewalk
                  THROW_LOAD(InvalidFileFormat,pLoad);
            }



            if ( configuration == ExteriorBarrier_Sidewalk_InteriorBarrier )
            {
               if ( !pLoad->BeginUnit(_T("InteriorBarrierPoints")) )
                  THROW_LOAD(InvalidFileFormat,pLoad);

               if ( !pLoad->Property(_T("Count"),&count) )
                  THROW_LOAD(InvalidFileFormat,pLoad);

               for ( CollectionIndexType i = 0; i < count; i++ )
               {
                  if ( !pLoad->BeginUnit(_T("Point")) )
                     THROW_LOAD(InvalidFileFormat,pLoad);

                  if ( !pLoad->Property(_T("X"),&x) )
                     THROW_LOAD(InvalidFileFormat,pLoad);

                  if ( !pLoad->Property(_T("Y"),&y) )
                     THROW_LOAD(InvalidFileFormat,pLoad);

                  if ( !pLoad->EndUnit() )
                     THROW_LOAD(InvalidFileFormat,pLoad);
               }

               if ( !pLoad->EndUnit() ) // end of InteriorBarrierPoints
                  THROW_LOAD(InvalidFileFormat,pLoad);
            }
         } // end of if (version < 5)



         // Barrier Weight unit
         if ( !pLoad->BeginUnit(_T("BarrierWeight")) )
            THROW_LOAD(InvalidFileFormat,pLoad);

         long value;
         if ( !pLoad->Property(_T("WeightMethod"),&value) )
            THROW_LOAD(InvalidFileFormat,pLoad);

         m_WeightMethod = (WeightMethod)value;

         if ( !pLoad->Property(_T("Weight"),&m_Weight) )
            THROW_LOAD(InvalidFileFormat,pLoad);

         if ( 7 <= version )
         {
            if ( !pLoad->Property(_T("Ec"),&m_Ec) )
               THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if(!pLoad->EndUnit())
            THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( 5 < version )
      {
         // added version 6
         if ( !pLoad->Property(_T("CurbOffset"),&m_CurbOffset) )
            THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( 4 <= version )
      {
         // added version 4.0
         if ( version < 5 )
         {
            if ( !pLoad->Property(_T("ExteriorGirderStructurallyContinuous"),&m_bStructurallyContinuous) )
               THROW_LOAD(InvalidFileFormat,pLoad);
         }
         else
         {
            // renamed in version 5
            if ( !pLoad->Property(_T("IsBarrierStructurallyContinuous"),&m_bStructurallyContinuous) )
               THROW_LOAD(InvalidFileFormat,pLoad);
         }
      }

      if(!pLoad->EndUnit())
         THROW_LOAD(InvalidFileFormat,pLoad);
   }
   else
      return false; // not a tb entry
   
   return true;
}

bool TrafficBarrierEntry::IsEqual(const TrafficBarrierEntry& rOther, bool considerName) const
{
   bool test = false;

   if ( m_WeightMethod != rOther.m_WeightMethod )
      return false;

   if ( !::IsEqual(m_Weight,rOther.m_Weight) )
      return false;

   if ( !::IsEqual(m_Ec,rOther.m_Ec) )
      return false;

   if ( m_bStructurallyContinuous != rOther.m_bStructurallyContinuous )
      return false;

   if ( !::IsEqual(m_CurbOffset,rOther.m_CurbOffset) )
      return false;

   if ( !ComparePoints(m_BarrierPoints,rOther.m_BarrierPoints) )
      return false;
   
   if (considerName && GetName() != rOther.GetName() )
      return false;

   return true;
}

//======================== ACCESS     =======================================
void TrafficBarrierEntry::SetBarrierPoints(IPoint2dCollection* points)
{
   CopyPoints(m_BarrierPoints,points);
}

void TrafficBarrierEntry::GetBarrierPoints(IPoint2dCollection** points) const
{
   CComPtr<IPoint2dCollection> p;
   p.CoCreateInstance(CLSID_Point2dCollection);

   (*points) = p;
   (*points)->AddRef();

   CopyPoints(*points,m_BarrierPoints);
}

void TrafficBarrierEntry::SetWeightMethod(TrafficBarrierEntry::WeightMethod method)
{
   m_WeightMethod = method;
}

TrafficBarrierEntry::WeightMethod TrafficBarrierEntry::GetWeightMethod() const
{
   return m_WeightMethod;
}

void TrafficBarrierEntry::SetWeight(Float64 w)
{
   m_Weight = w;
}

Float64 TrafficBarrierEntry::GetWeight() const
{
   ATLASSERT( m_WeightMethod == Input ); // if weight method is compute, this value is undefined
   return m_Weight;
}

void TrafficBarrierEntry::SetEc(Float64 ec)
{
   m_Ec = ec;
}

Float64 TrafficBarrierEntry::GetEc() const
{
   ATLASSERT( m_WeightMethod == Input ); // if weight method is compute, this value is undefined
   return m_Ec;
}

bool TrafficBarrierEntry::IsBarrierStructurallyContinuous() const
{
   return m_bStructurallyContinuous;
}

void TrafficBarrierEntry::IsBarrierStructurallyContinuous(bool bContinuous)
{
   m_bStructurallyContinuous = bContinuous;
}

void TrafficBarrierEntry::SetCurbOffset(Float64 curbOffset)
{
   m_CurbOffset = curbOffset;
}

Float64 TrafficBarrierEntry::GetCurbOffset() const
{
   return m_CurbOffset;
}

//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
bool TrafficBarrierEntry::Edit(bool allowEditing,int nPage)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // exchange data with dialog
   CTrafficBarrierDlg dlg(allowEditing);

   dlg.m_WeightMethod = m_WeightMethod;
   dlg.m_Weight       = m_Weight;
   dlg.m_Ec           = m_Ec;
   dlg.m_CurbOffset   = m_CurbOffset;
   dlg.m_bStructurallyContinuous = m_bStructurallyContinuous;

   CopyPoints(dlg.m_BarrierPoints,m_BarrierPoints);
   
   dlg.m_Name    = this->GetName().c_str();

   INT_PTR i = dlg.DoModal();
   if (i==IDOK)
   {
      SetEc(dlg.m_Ec);
      SetWeight(dlg.m_Weight);
      SetWeightMethod(dlg.m_WeightMethod);

      CopyPoints(m_BarrierPoints,dlg.m_BarrierPoints);

      IsBarrierStructurallyContinuous(dlg.m_bStructurallyContinuous);

      SetName(dlg.m_Name);

      SetCurbOffset(dlg.m_CurbOffset);

      return true;
   }

   return false;
}

void TrafficBarrierEntry::MakeCopy(const TrafficBarrierEntry& rOther)
{
   CopyPoints(m_BarrierPoints,rOther.m_BarrierPoints);

   m_Weight = rOther.m_Weight;
   m_Ec     = rOther.m_Ec;
   m_WeightMethod = rOther.m_WeightMethod;
   m_bStructurallyContinuous = rOther.m_bStructurallyContinuous;

   m_CurbOffset = rOther.m_CurbOffset;
}

void TrafficBarrierEntry::MakeAssignment(const TrafficBarrierEntry& rOther)
{
   libLibraryEntry::MakeAssignment( rOther );
   MakeCopy( rOther );
}

HICON  TrafficBarrierEntry::GetIcon() const
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   return ::LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_TRAFFIC_BARRIER_ENTRY) );
}

bool TrafficBarrierEntry::ComparePoints(IPoint2dCollection* points1,IPoint2dCollection* points2) const
{
   CollectionIndexType count1, count2;
   points1->get_Count(&count1);
   points2->get_Count(&count2);

   if ( count1 != count2 )
      return false;

   for (CollectionIndexType i = 0; i < count1; i++ )
   {
      CComPtr<IPoint2d> p1,p2;
      points1->get_Item(i,&p1);
      points2->get_Item(i,&p2);

      Float64 x1,y1;
      p1->get_X(&x1);
      p1->get_Y(&y1);

      Float64 x2,y2;
      p2->get_X(&x2);
      p2->get_Y(&y2);

      if ( !::IsEqual(x1,x2) || !::IsEqual(y1,y2) )
         return false;
   }

   return true;
}

void TrafficBarrierEntry::CopyPoints(IPoint2dCollection* points1,IPoint2dCollection* points2) 
{
   points1->Clear();
   CComPtr<IEnumPoint2d> enum_points;
   points2->get__Enum(&enum_points);
   CComPtr<IPoint2d> point;
   while ( enum_points->Next(1,&point,NULL) != S_FALSE )
   {
      Float64 x,y;
      point->get_X(&x);
      point->get_Y(&y);

      CComPtr<IPoint2d> p;
      p.CoCreateInstance(CLSID_Point2d);
      p->Move(x,y);

      points1->Add(p);

      point.Release();
   }
}

void TrafficBarrierEntry::CopyPoints(IPoint2dCollection* points1,IPoint2dCollection* points2) const
{
   points1->Clear();
   CComPtr<IEnumPoint2d> enum_points;
   points2->get__Enum(&enum_points);
   CComPtr<IPoint2d> point;
   while ( enum_points->Next(1,&point,NULL) != S_FALSE )
   {
      Float64 x,y;
      point->get_X(&x);
      point->get_Y(&y);

      CComPtr<IPoint2d> p;
      p.CoCreateInstance(CLSID_Point2d);
      p->Move(x,y);

      points1->Add(p);

      point.Release();
   }
}

void TrafficBarrierEntry::ConvertDimensionsToPoints(Float64 x1,Float64 x2,Float64 x3,Float64 x4,Float64 x5,Float64 y1,Float64 y2,Float64 y3)
{
   Float64 y4 = ::ConvertToSysUnits(7.0,unitMeasure::Inch);
   CComPtr<ITrafficBarrier> barrier;
   barrier.CoCreateInstance(CLSID_TrafficBarrier);
   barrier->put_X1(x1);
   barrier->put_X2(x2);
   barrier->put_X3(x3);
   barrier->put_X4(x4);
   barrier->put_X5(x5);
   barrier->put_Y1(y1);
   barrier->put_Y2(y2);
   barrier->put_Y3(y3);
   barrier->put_Y4(y4);

   CComPtr<IPoint2d> hookPoint;
   barrier->get_HookPoint(&hookPoint);
   hookPoint->Move(0,0);

   CComQIPtr<IShape> shape(barrier);
   CComPtr<IPoint2dCollection> points;
   shape->get_PolyPoints(&points);

   CopyPoints(m_BarrierPoints,points);
}

void TrafficBarrierEntry::CreatePolyShape(pgsTypes::TrafficBarrierOrientation orientation,IPolyShape** polyShape) const
{
   CreatePolyShape(orientation,m_BarrierPoints,polyShape);
}

void TrafficBarrierEntry::CreatePolyShape(pgsTypes::TrafficBarrierOrientation orientation,IPoint2dCollection* points,IPolyShape** polyShape) const
{
   CComPtr<IPolyShape> polyshape;
   polyshape.CoCreateInstance(CLSID_PolyShape);

   Float64 sign = (orientation == pgsTypes::tboLeft ? 1 : -1);

   CComPtr<IEnumPoint2d> enum_points;
   points->get__Enum(&enum_points);
   CComPtr<IPoint2d> point;
   while ( enum_points->Next(1,&point,NULL) != S_FALSE )
   {
      Float64 x,y;
      point->get_X(&x);
      point->get_Y(&y);
      polyshape->AddPoint(sign*x,y);
      point.Release();
   }

   CollectionIndexType nPoints;
   polyshape->get_NumPoints(&nPoints);
   if ( nPoints == 0 )
   {
      polyshape->AddPoint(0,0);
   }

   (*polyShape) = polyshape;
   (*polyShape)->AddRef();
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PRIVATE    ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
//======================== INQUERY    =======================================

//======================== DEBUG      =======================================
#if defined _DEBUG
bool TrafficBarrierEntry::AssertValid() const
{
   return libLibraryEntry::AssertValid();
}

void TrafficBarrierEntry::Dump(dbgDumpContext& os) const
{
   os << _T("Dump for TrafficBarrierEntry ")<< GetName() <<endl;

   libLibraryEntry::Dump( os );
}
#endif // _DEBUG

#if defined _UNITTEST
bool TrafficBarrierEntry::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("TrafficBarrierEntry");

   // tests are performed on entire library.

   TESTME_EPILOG("TrafficBarrierEntry");
}
#endif // _UNITTEST
