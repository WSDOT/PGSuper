///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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
#include <psgLib\ConnectionLibraryEntry.h>

#include <System\IStructuredSave.h>
#include <System\IStructuredLoad.h>
#include <System\XStructuredLoad.h>

#include "resource.h"
#include "ConnectionEntryDlg.h"
#include <Units\sysUnits.h>

#include <MathEx.h>

#ifdef _DEBUG
#define new DEBUG_NEW
//#undef THIS_FILE
//static char THIS_FILE[] = __FILE__;
#endif


/****************************************************************************
CLASS
   ConnectionLibraryEntry
****************************************************************************/


inline std::string StringForEndDistanceMeasurementType(ConnectionLibraryEntry::EndDistanceMeasurementType type)
{
   switch (type)
   {
   case ConnectionLibraryEntry::FromBearingAlongGirder:
      return std::string("FromBearingAlongGirder");
   case ConnectionLibraryEntry::FromBearingNormalToPier:
      return std::string("FromBearingNormalToPier");
   case ConnectionLibraryEntry::FromPierAlongGirder:
      return std::string("FromPierAlongGirder");
   case ConnectionLibraryEntry::FromPierNormalToPier:
      return std::string("FromPierNormalToPier");
   default:
      ATLASSERT(0);
      return std::string("FromPierNormalToPier");
   };
}


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
ConnectionLibraryEntry::ConnectionLibraryEntry() :
m_GirderEndDistance(0),
m_GirderBearingOffset(0),
m_EndDistanceMeasure(FromBearingNormalToPier),
m_BearingOffsetMeasure(NormalToPier),
m_SupportWidth(0),
m_DiaphragmHeight(::ConvertToSysUnits(24.0,unitMeasure::Inch)),
m_DiaphragmWidth(::ConvertToSysUnits(12.0,unitMeasure::Inch)),
m_DiaphragmLoadType(ApplyAtBearingCenterline),
m_DiaphragmLoadLocation(0.0)
{
}

ConnectionLibraryEntry::ConnectionLibraryEntry(const ConnectionLibraryEntry& rOther) :
libLibraryEntry(rOther)
{
   MakeCopy(rOther);
}

ConnectionLibraryEntry::~ConnectionLibraryEntry()
{
}

//======================== OPERATORS  =======================================
ConnectionLibraryEntry& ConnectionLibraryEntry::operator= (const ConnectionLibraryEntry& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
bool ConnectionLibraryEntry::SaveMe(sysIStructuredSave* pSave)
{
   pSave->BeginUnit("ConnectionLibraryEntry", 5.0);

   pSave->Property("Name",this->GetName().c_str());
   pSave->Property("DiaphragmHeight", m_DiaphragmHeight);
   pSave->Property("GirderEndDistance", m_GirderEndDistance);
   pSave->Property("GirderBearingOffset", m_GirderBearingOffset);
   pSave->Property("SupportWidth",m_SupportWidth);// added in version 4

   // changed/added in version 5
   pSave->Property("EndDistanceMeasurementType", StringForEndDistanceMeasurementType(m_EndDistanceMeasure).c_str() );
   pSave->Property("BearingOffsetMeasurementType",m_BearingOffsetMeasure == AlongGirder ? "AlongGirder" : "NormalToPier");

   pSave->Property("DiaphragmWidth",m_DiaphragmWidth);

   // diaphragm load type - added for version 2.0
   if (m_DiaphragmLoadType==ApplyAtBearingCenterline)
      pSave->Property("DiaphragmLoadType","ApplyAtBearingCenterline");
   else if (m_DiaphragmLoadType==ApplyAtSpecifiedLocation)
   {
      pSave->Property("DiaphragmLoadType","ApplyAtSpecifiedLocation");
      pSave->Property("DiaphragmLoadLocation",m_DiaphragmLoadLocation);
   }
   else if (m_DiaphragmLoadType==DontApply)
      pSave->Property("DiaphragmLoadType","DontApply");
   else
      PRECONDITION(0);

   pSave->EndUnit();

   return true;
}

bool ConnectionLibraryEntry::LoadMe(sysIStructuredLoad* pLoad)
{
   if(pLoad->BeginUnit("ConnectionLibraryEntry"))
   {
      Float64 version = pLoad->GetVersion();
      if (5.0 < version)
         THROW_LOAD(BadVersion,pLoad);

      std::string name;
      if(pLoad->Property("Name",&name))
         this->SetName(name.c_str());
      else
         THROW_LOAD(InvalidFileFormat,pLoad);

      if(!pLoad->Property("DiaphragmHeight", &m_DiaphragmHeight))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if(!pLoad->Property("GirderEndDistance", &m_GirderEndDistance))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if(!pLoad->Property("GirderBearingOffset", &m_GirderBearingOffset))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if ( 4.0 <= version )
      {
         if(!pLoad->Property("SupportWidth", &m_SupportWidth))
            THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( 3.0 <= version && version < 5.0 )
      {
         std::string tmp;

         if(!pLoad->Property("MeasurementType",&tmp))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if (tmp=="AlongGirder")
         {
            m_BearingOffsetMeasure = AlongGirder;
            m_EndDistanceMeasure = FromBearingAlongGirder;
         }
         else if (tmp=="NormalToPier")
         {
            m_BearingOffsetMeasure = NormalToPier;
            m_EndDistanceMeasure = FromBearingNormalToPier;
         }
         else
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }
      }
      else if ( 5.0 <= version )
      {
         std::string tmp;

         if(!pLoad->Property("EndDistanceMeasurementType",&tmp))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if (tmp=="FromBearingAlongGirder" || tmp=="AlongGirder")
         {
            m_EndDistanceMeasure = FromBearingAlongGirder;
         }
         else if (tmp=="FromBearingNormalToPier" || tmp=="NormalToPier")
         {
            m_EndDistanceMeasure = FromBearingNormalToPier;
         }
         else if (tmp=="FromPierAlongGirder")
         {
            m_EndDistanceMeasure = FromPierAlongGirder;
         }
         else if (tmp=="FromPierNormalToPier")
         {
            m_EndDistanceMeasure = FromPierNormalToPier;
         }
         else
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }


         if(!pLoad->Property("BearingOffsetMeasurementType",&tmp))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if (tmp=="AlongAlignment" || tmp=="AlongGirder") // AlongAlignment is an artifact after testing but before release of 2.1
         {
            m_BearingOffsetMeasure = AlongGirder;
         }
         else if (tmp=="NormalToPier")
         {
            m_BearingOffsetMeasure = NormalToPier;
         }
         else
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }
      }

      if(!pLoad->Property("DiaphragmWidth", &m_DiaphragmWidth))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if (2.0 <= version)
      {
         std::string tmp;
         if(!pLoad->Property("DiaphragmLoadType",&tmp))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if (tmp=="ApplyAtBearingCenterline")
         {
            m_DiaphragmLoadType = ApplyAtBearingCenterline;
         }
         else if (tmp=="ApplyAtSpecifiedLocation")
         {
            m_DiaphragmLoadType = ApplyAtSpecifiedLocation;
            if(!pLoad->Property("DiaphragmLoadLocation",&m_DiaphragmLoadLocation))
               THROW_LOAD(InvalidFileFormat,pLoad);
         }
         else if (tmp=="DontApply")
         {
            m_DiaphragmLoadType=DontApply;
         }
         else
            THROW_LOAD(InvalidFileFormat,pLoad);
      }
      else
      {
         // default
         SetDiaphragmLoadType(ApplyAtBearingCenterline);
      }

      if(!pLoad->EndUnit())
         THROW_LOAD(InvalidFileFormat,pLoad);
   }
   else
      return false; // not a concrete entry
   
   return true;
}

bool ConnectionLibraryEntry::IsEqual(const ConnectionLibraryEntry& rOther, bool considerName) const
{
   bool test = ( ::IsEqual(m_GirderEndDistance,   rOther.m_GirderEndDistance)      &&
                 ::IsEqual(m_GirderBearingOffset, rOther.m_GirderBearingOffset)    &&
                 ::IsEqual(m_SupportWidth, rOther.m_SupportWidth)                  &&
                 m_BearingOffsetMeasure == rOther.m_BearingOffsetMeasure &&
                 m_EndDistanceMeasure == rOther.m_EndDistanceMeasure &&
                 ::IsEqual(m_DiaphragmHeight,     rOther.m_DiaphragmHeight)        &&
                 ::IsEqual(m_DiaphragmWidth,      rOther.m_DiaphragmWidth )        &&
                 m_DiaphragmLoadType == rOther.m_DiaphragmLoadType);

   if (test && m_DiaphragmLoadType==ApplyAtSpecifiedLocation)
      test = ::IsEqual(m_DiaphragmLoadLocation,  rOther.m_DiaphragmLoadLocation );

   if (considerName)
      test &= this->GetName()==rOther.GetName();

   return test;
}


//======================== ACCESS     =======================================

void ConnectionLibraryEntry::SetGirderEndDistance(Float64 fc)
{
   m_GirderEndDistance=fc;
}

Float64 ConnectionLibraryEntry::GetGirderEndDistance() const
{
   return m_GirderEndDistance;
}

void ConnectionLibraryEntry::SetGirderBearingOffset(Float64 d)
{
   m_GirderBearingOffset = d;
}

Float64 ConnectionLibraryEntry::GetGirderBearingOffset() const
{
   return m_GirderBearingOffset;
}

void ConnectionLibraryEntry::SetEndDistanceMeasurementType(ConnectionLibraryEntry::EndDistanceMeasurementType mt)
{
   m_EndDistanceMeasure = mt;
}

ConnectionLibraryEntry::EndDistanceMeasurementType ConnectionLibraryEntry::GetEndDistanceMeasurementType() const
{
   return m_EndDistanceMeasure;
}

void ConnectionLibraryEntry::SetBearingOffsetMeasurementType(ConnectionLibraryEntry::BearingOffsetMeasurementType mt)
{
   m_BearingOffsetMeasure = mt;
}

ConnectionLibraryEntry::BearingOffsetMeasurementType ConnectionLibraryEntry::GetBearingOffsetMeasurementType() const
{
   return m_BearingOffsetMeasure;
}

void ConnectionLibraryEntry::SetSupportWidth(Float64 w)
{
   m_SupportWidth = w;
}

Float64 ConnectionLibraryEntry::GetSupportWidth() const
{
   return m_SupportWidth;
}

void ConnectionLibraryEntry::SetDiaphragmHeight(Float64 d)
{
   m_DiaphragmHeight = d;
}

Float64 ConnectionLibraryEntry::GetDiaphragmHeight() const
{
   return m_DiaphragmHeight;
}

void ConnectionLibraryEntry::SetDiaphragmWidth(Float64 w)
{
   m_DiaphragmWidth = w;
}

Float64 ConnectionLibraryEntry::GetDiaphragmWidth()const
{
   return m_DiaphragmWidth;
}

ConnectionLibraryEntry::DiaphragmLoadType ConnectionLibraryEntry::GetDiaphragmLoadType() const
{
   return m_DiaphragmLoadType;
}

void ConnectionLibraryEntry::SetDiaphragmLoadType(DiaphragmLoadType type)
{
   m_DiaphragmLoadType = type;
   m_DiaphragmLoadLocation=0.0;
}

Float64 ConnectionLibraryEntry::GetDiaphragmLoadLocation() const
{
   CHECK(m_DiaphragmLoadType==ApplyAtSpecifiedLocation);
   return m_DiaphragmLoadLocation;
}

void ConnectionLibraryEntry::SetDiaphragmLoadLocation(Float64 loc)
{
   CHECK(m_DiaphragmLoadType==ApplyAtSpecifiedLocation);
   m_DiaphragmLoadLocation = loc;
}

//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
bool ConnectionLibraryEntry::Edit(libUnitsMode::Mode mode, bool allowEditing)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // exchange data with dialog
   CConnectionEntryDlg dlg(mode, allowEditing);
   dlg.m_GirderEndDistance  = this->GetGirderEndDistance();
   dlg.m_GirderBearingOffset  = this->GetGirderBearingOffset();
   dlg.m_EndDistanceMeasurementType = this->GetEndDistanceMeasurementType();
   dlg.m_BearingOffsetMeasurementType = this->GetBearingOffsetMeasurementType();
   dlg.m_DiaphragmHeight  = this->GetDiaphragmHeight();
   dlg.m_DiaphragmWidth = this->GetDiaphragmWidth();
   dlg.m_Name    = this->GetName().c_str();
   dlg.m_DiaphragmLoadType = this->GetDiaphragmLoadType();
   dlg.m_DiaphragmLoadLocation = this->m_DiaphragmLoadLocation;
   dlg.m_SupportWidth = GetSupportWidth();

   int i = dlg.DoModal();
   if (i==IDOK)
   {
      this->SetGirderEndDistance(dlg.m_GirderEndDistance);
      this->SetGirderBearingOffset(dlg.m_GirderBearingOffset);
      this->SetDiaphragmHeight(dlg.m_DiaphragmHeight );
      this->SetDiaphragmWidth(dlg.m_DiaphragmWidth);
      this->SetName(dlg.m_Name);
      this->SetDiaphragmLoadType(dlg.m_DiaphragmLoadType);
      this->SetEndDistanceMeasurementType(dlg.m_EndDistanceMeasurementType);
      this->SetBearingOffsetMeasurementType(dlg.m_BearingOffsetMeasurementType);
      this->SetSupportWidth(dlg.m_SupportWidth);
      if (dlg.m_DiaphragmLoadType==ApplyAtSpecifiedLocation)
         this->SetDiaphragmLoadLocation(dlg.m_DiaphragmLoadLocation);

      return true;
   }
   return false;
}


void ConnectionLibraryEntry::MakeCopy(const ConnectionLibraryEntry& rOther)
{
   m_GirderEndDistance     = rOther.m_GirderEndDistance;
   m_GirderBearingOffset   = rOther.m_GirderBearingOffset;     
   m_SupportWidth          = rOther.m_SupportWidth;
   m_DiaphragmHeight       = rOther.m_DiaphragmHeight;       
   m_DiaphragmWidth        = rOther.m_DiaphragmWidth;
   m_DiaphragmLoadType     = rOther.m_DiaphragmLoadType;
   m_DiaphragmLoadLocation = rOther.m_DiaphragmLoadLocation;
   m_EndDistanceMeasure    = rOther.m_EndDistanceMeasure;
   m_BearingOffsetMeasure  = rOther.m_BearingOffsetMeasure;
}

void ConnectionLibraryEntry::MakeAssignment(const ConnectionLibraryEntry& rOther)
{
   libLibraryEntry::MakeAssignment( rOther );
   MakeCopy( rOther );
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================
HICON  ConnectionLibraryEntry::GetIcon() const
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   return ::LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_CONNECTION_ENTRY) );
}

////////////////////////// PRIVATE    ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
//======================== INQUERY    =======================================

//======================== DEBUG      =======================================
#if defined _DEBUG
bool ConnectionLibraryEntry::AssertValid() const
{
   return libLibraryEntry::AssertValid();
}

void ConnectionLibraryEntry::Dump(dbgDumpContext& os) const
{
   os << "Dump for ConnectionLibraryEntry"<<endl;
   os << "   m_GirderEndDistance     ="<< m_GirderEndDistance <<endl;
   os << "   m_GirderBearingOffset   ="<< m_GirderBearingOffset << endl;
   os << "   m_DiaphragmHeight       ="<< m_DiaphragmHeight <<endl;
   os << "   m_DiaphragmWidth        ="<< m_DiaphragmWidth <<endl;
   os << "   m_DiaphragmLoadType     ="<<m_DiaphragmLoadType     <<endl;
   os << "   m_DiaphragmLoadLocation ="<<m_DiaphragmLoadLocation <<endl;

   libLibraryEntry::Dump( os );
}
#endif // _DEBUG

#if defined _UNITTEST
bool ConnectionLibraryEntry::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("ConnectionLibraryEntry");

   // tests are performed on entire library.

   TESTME_EPILOG("ConnectionLibraryEntry");
}
#endif // _UNITTEST
