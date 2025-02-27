///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2025  Washington State Department of Transportation
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
#include <Units\Convert.h>

#include <MathEx.h>

#include <EAF\EAFApp.h>
#include <psgLib\LibraryEntryDifferenceItem.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/****************************************************************************
CLASS
   ConnectionLibraryEntry
****************************************************************************/
CString ConnectionLibraryEntry::GetBearingOffsetMeasurementType(ConnectionLibraryEntry::BearingOffsetMeasurementType measurementType)
{
   LPCTSTR lpszType;
   switch(measurementType)
   {
   case AlongGirder:
      lpszType = _T("Measured Along Centerline Girder");
      break;

   case NormalToPier:
      lpszType = _T("Measured Normal to Abutment/Pier Line");
      break;

   default:
      ATLASSERT(false);
   }

   return lpszType;
}

CString ConnectionLibraryEntry::GetEndDistanceMeasurementType(ConnectionLibraryEntry::EndDistanceMeasurementType measurementType)
{
   LPCTSTR lpszType;
   switch(measurementType)
   {
   case FromBearingAlongGirder:
      lpszType = _T("Measured from CL Bearing, Along Girder");
      break;

   case FromBearingNormalToPier:
      lpszType = _T("Measured from and Normal to CL Bearing");
      break;

   case FromPierAlongGirder:
      lpszType = _T("Measured from Abutment/Pier Line, Along Girder");
      break;

   case FromPierNormalToPier:
      lpszType = _T("Measured from and Normal to the Abutment/Pier Line");
      break;

   default:
      ATLASSERT(false);
   }

   return lpszType;
}


std::_tstring ConnectionLibraryEntry::StringForEndDistanceMeasurementType(ConnectionLibraryEntry::EndDistanceMeasurementType type)
{
   switch (type)
   {
   case ConnectionLibraryEntry::FromBearingAlongGirder:
      return std::_tstring(_T("FromBearingAlongGirder"));
   case ConnectionLibraryEntry::FromBearingNormalToPier:
      return std::_tstring(_T("FromBearingNormalToPier"));
   case ConnectionLibraryEntry::FromPierAlongGirder:
      return std::_tstring(_T("FromPierAlongGirder"));
   case ConnectionLibraryEntry::FromPierNormalToPier:
      return std::_tstring(_T("FromPierNormalToPier"));
   default:
      ATLASSERT(false);
      return std::_tstring(_T("FromPierNormalToPier"));
   };
}

ConnectionLibraryEntry::EndDistanceMeasurementType ConnectionLibraryEntry::EndDistanceMeasurementTypeFromString(LPCTSTR strType)
{
   std::_tstring type(strType);
   if ( type == std::_tstring(_T("FromBearingAlongGirder")) )
   {
      return FromBearingAlongGirder;
   }
   else if ( type == std::_tstring(_T("FromBearingNormalToPier")) )
   {
      return FromBearingNormalToPier;
   }
   else if ( type == std::_tstring(_T("FromPierAlongGirder")) )
   {
      return FromPierAlongGirder;
   }
   else if ( type == std::_tstring(_T("FromPierNormalToPier")) )
   {
      return FromPierNormalToPier;
   }
   else
   {
      ATLASSERT(false);
      return FromPierNormalToPier;
   }
}

std::_tstring ConnectionLibraryEntry::StringForBearingOffsetMeasurementType(ConnectionLibraryEntry::BearingOffsetMeasurementType type)
{
   switch (type)
   {
   case ConnectionLibraryEntry::AlongGirder:
      return std::_tstring(_T("AlongGirder"));
   case ConnectionLibraryEntry::NormalToPier:
      return std::_tstring(_T("NormalToPier"));
   default:
      ATLASSERT(false);
      return std::_tstring(_T("NormalToPier"));
   };
}

ConnectionLibraryEntry::BearingOffsetMeasurementType ConnectionLibraryEntry::BearingOffsetMeasurementTypeFromString(LPCTSTR strType)
{
   std::_tstring type(strType);
   if ( type == std::_tstring(_T("AlongGirder")) )
   {
      return AlongGirder;
   }
   else if ( type == std::_tstring(_T("NormalToPier")) )
   {
      return NormalToPier;
   }
   else
   {
      ATLASSERT(false);
      return NormalToPier;
   }
}

////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
ConnectionLibraryEntry::ConnectionLibraryEntry() :
m_GirderEndDistance(0),
m_GirderBearingOffset(0),
m_EndDistanceMeasure(FromBearingNormalToPier),
m_BearingOffsetMeasure(NormalToPier),
m_SupportWidth(0),
m_DiaphragmHeight(-1), // compute
m_DiaphragmWidth(-1), // compute
m_DiaphragmLoadType(ApplyAtBearingCenterline),
m_DiaphragmLoadLocation(0.0)
{
}

ConnectionLibraryEntry::~ConnectionLibraryEntry()
{
}

//======================== OPERATIONS =======================================
bool ConnectionLibraryEntry::SaveMe(WBFL::System::IStructuredSave* pSave)
{
   pSave->BeginUnit(_T("ConnectionLibraryEntry"), 6.0);

   pSave->Property(_T("Name"),this->GetName().c_str());
   pSave->Property(_T("DiaphragmHeight"), m_DiaphragmHeight);
   pSave->Property(_T("GirderEndDistance"), m_GirderEndDistance);
   pSave->Property(_T("GirderBearingOffset"), m_GirderBearingOffset);
//   pSave->Property(_T("SupportWidth"),m_SupportWidth);// added in version 4. Removed in version 6

   // changed/added in version 5
   pSave->Property(_T("EndDistanceMeasurementType"), StringForEndDistanceMeasurementType(m_EndDistanceMeasure).c_str() );
   pSave->Property(_T("BearingOffsetMeasurementType"), StringForBearingOffsetMeasurementType(m_BearingOffsetMeasure).c_str() );

   pSave->Property(_T("DiaphragmWidth"),m_DiaphragmWidth);

   // diaphragm load type - added for version 2.0
   if (m_DiaphragmLoadType==ApplyAtBearingCenterline)
   {
      pSave->Property(_T("DiaphragmLoadType"),_T("ApplyAtBearingCenterline"));
   }
   else if (m_DiaphragmLoadType==ApplyAtSpecifiedLocation)
   {
      pSave->Property(_T("DiaphragmLoadType"),_T("ApplyAtSpecifiedLocation"));
      pSave->Property(_T("DiaphragmLoadLocation"),m_DiaphragmLoadLocation);
   }
   else if (m_DiaphragmLoadType==DontApply)
   {
      pSave->Property(_T("DiaphragmLoadType"),_T("DontApply"));
   }
   else
   {
      ATLASSERT(0);
   }

   pSave->EndUnit();

   return true;
}

bool ConnectionLibraryEntry::LoadMe(WBFL::System::IStructuredLoad* pLoad)
{
   if(pLoad->BeginUnit(_T("ConnectionLibraryEntry")))
   {
      Float64 version = pLoad->GetVersion();
      if (6.0 < version)
      {
         THROW_LOAD(BadVersion,pLoad);
      }

      std::_tstring name;
      if(pLoad->Property(_T("Name"),&name))
      {
         this->SetName(name.c_str());
      }
      else
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if(!pLoad->Property(_T("DiaphragmHeight"), &m_DiaphragmHeight))
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if(!pLoad->Property(_T("GirderEndDistance"), &m_GirderEndDistance))
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if(!pLoad->Property(_T("GirderBearingOffset"), &m_GirderBearingOffset))
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( 4.0 <= version && 6.0 > version )
      {
         if(!pLoad->Property(_T("SupportWidth"), &m_SupportWidth))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }
      }

      if ( 3.0 <= version && version < 5.0 )
      {
         std::_tstring tmp;

         if(!pLoad->Property(_T("MeasurementType"),&tmp))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if (tmp==_T("AlongGirder"))
         {
            m_BearingOffsetMeasure = AlongGirder;
            m_EndDistanceMeasure = FromBearingAlongGirder;
         }
         else if (tmp==_T("NormalToPier"))
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
         std::_tstring tmp;

         if(!pLoad->Property(_T("EndDistanceMeasurementType"),&tmp))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if (tmp==_T("FromBearingAlongGirder") || tmp==_T("AlongGirder"))
         {
            m_EndDistanceMeasure = FromBearingAlongGirder;
         }
         else if (tmp==_T("FromBearingNormalToPier") || tmp==_T("NormalToPier"))
         {
            m_EndDistanceMeasure = FromBearingNormalToPier;
         }
         else if (tmp==_T("FromPierAlongGirder"))
         {
            m_EndDistanceMeasure = FromPierAlongGirder;
         }
         else if (tmp==_T("FromPierNormalToPier"))
         {
            m_EndDistanceMeasure = FromPierNormalToPier;
         }
         else
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }


         if(!pLoad->Property(_T("BearingOffsetMeasurementType"),&tmp))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if (tmp==_T("AlongAlignment") || tmp==_T("AlongGirder")) // AlongAlignment is an artifact after testing but before release of 2.1
         {
            m_BearingOffsetMeasure = AlongGirder;
         }
         else if (tmp==_T("NormalToPier"))
         {
            m_BearingOffsetMeasure = NormalToPier;
         }
         else
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }
      }

      if(!pLoad->Property(_T("DiaphragmWidth"), &m_DiaphragmWidth))
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if (2.0 <= version)
      {
         std::_tstring tmp;
         if(!pLoad->Property(_T("DiaphragmLoadType"),&tmp))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if (tmp==_T("ApplyAtBearingCenterline"))
         {
            m_DiaphragmLoadType = ApplyAtBearingCenterline;
         }
         else if (tmp==_T("ApplyAtSpecifiedLocation"))
         {
            m_DiaphragmLoadType = ApplyAtSpecifiedLocation;
            if(!pLoad->Property(_T("DiaphragmLoadLocation"),&m_DiaphragmLoadLocation))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }
         }
         else if (tmp==_T("DontApply"))
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
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }
   }
   else
   {
      return false; // not a concrete entry
   }
   
   return true;
}

bool ConnectionLibraryEntry::IsEqual(const ConnectionLibraryEntry& rOther,bool bConsiderName) const
{
   std::vector<std::unique_ptr<pgsLibraryEntryDifferenceItem>> vDifferences;
   bool bMustRename;
   return Compare(rOther,vDifferences,bMustRename,true,bConsiderName);
}

bool ConnectionLibraryEntry::Compare(const ConnectionLibraryEntry& rOther, std::vector<std::unique_ptr<pgsLibraryEntryDifferenceItem>>& vDifferences, bool& bMustRename, bool bReturnOnFirstDifference, bool considerName) const
{
   CEAFApp* pApp = EAFGetApp();
   const WBFL::Units::IndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

   bMustRename = false;

   if ( !::IsEqual(m_GirderEndDistance,rOther.m_GirderEndDistance) )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.emplace_back(std::make_unique<pgsLibraryEntryDifferenceLengthItem>(_T("End Distance"),m_GirderEndDistance,rOther.m_GirderEndDistance,pDisplayUnits->ComponentDim));
   }

   if ( m_EndDistanceMeasure != rOther.m_EndDistanceMeasure )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.emplace_back(std::make_unique<pgsLibraryEntryDifferenceStringItem>(_T("End Distance Measure"),GetEndDistanceMeasurementType(m_EndDistanceMeasure),GetEndDistanceMeasurementType(rOther.m_EndDistanceMeasure)));
   }

   if ( !::IsEqual(m_GirderBearingOffset,rOther.m_GirderBearingOffset) )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.emplace_back(std::make_unique<pgsLibraryEntryDifferenceLengthItem>(_T("Bearing Offset"),m_GirderBearingOffset,rOther.m_GirderBearingOffset,pDisplayUnits->ComponentDim));
   }

   if ( m_BearingOffsetMeasure != rOther.m_BearingOffsetMeasure )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.emplace_back(std::make_unique<pgsLibraryEntryDifferenceStringItem>(_T("Bearing Offset Measure"),GetBearingOffsetMeasurementType(m_BearingOffsetMeasure),GetBearingOffsetMeasurementType(rOther.m_BearingOffsetMeasure)));
   }

   if ( !::IsEqual(m_DiaphragmHeight,rOther.m_DiaphragmHeight) )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.emplace_back(std::make_unique<pgsLibraryEntryDifferenceLengthKeywordItem>(_T("Diaphragm Height"),m_DiaphragmHeight,rOther.m_DiaphragmHeight,pDisplayUnits->ComponentDim,_T("Compute")));
   }

   if ( !::IsEqual(m_DiaphragmWidth,rOther.m_DiaphragmWidth) )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.emplace_back(std::make_unique<pgsLibraryEntryDifferenceLengthKeywordItem>(_T("Diaphragm Width"),m_DiaphragmWidth,rOther.m_DiaphragmWidth,pDisplayUnits->ComponentDim,_T("Compute")));
   }

   if ( m_DiaphragmLoadType != rOther.m_DiaphragmLoadType )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.emplace_back(std::make_unique<pgsLibraryEntryDifferenceStringItem>(_T("Application of Diaphragm Load is different"),_T(""),_T("")));
   }
   else
   {
      if ( !::IsEqual(m_DiaphragmLoadLocation, rOther.m_DiaphragmLoadLocation) )
      {
         RETURN_ON_DIFFERENCE;
         vDifferences.emplace_back(std::make_unique<pgsLibraryEntryDifferenceLengthItem>(_T("Load Distance from CL Pier to CG of Diaphragm"),m_DiaphragmLoadLocation,rOther.m_DiaphragmLoadLocation,pDisplayUnits->ComponentDim));
      }
   }

   if (considerName &&  GetName() != rOther.GetName() )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.emplace_back(std::make_unique<pgsLibraryEntryDifferenceStringItem>(_T("Name"),GetName().c_str(),rOther.GetName().c_str()));
   }

   return vDifferences.size() == 0 ? true : false;
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
   return m_DiaphragmLoadLocation;
}

void ConnectionLibraryEntry::SetDiaphragmLoadLocation(Float64 loc)
{
   m_DiaphragmLoadLocation = loc;
}

//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
bool ConnectionLibraryEntry::Edit(bool allowEditing,int nPage)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // exchange data with dialog
   CConnectionEntryDlg dlg(allowEditing);
   dlg.m_GirderEndDistance  = this->GetGirderEndDistance();
   dlg.m_GirderBearingOffset  = this->GetGirderBearingOffset();
   dlg.m_EndDistanceMeasurementType = this->GetEndDistanceMeasurementType();
   dlg.m_BearingOffsetMeasurementType = this->GetBearingOffsetMeasurementType();
   dlg.m_DiaphragmHeight  = this->GetDiaphragmHeight();
   dlg.m_DiaphragmWidth = this->GetDiaphragmWidth();
   dlg.m_Name    = this->GetName().c_str();
   dlg.m_DiaphragmLoadType = this->GetDiaphragmLoadType();
   dlg.m_DiaphragmLoadLocation = this->m_DiaphragmLoadLocation;

   INT_PTR i = dlg.DoModal();
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
      if (dlg.m_DiaphragmLoadType==ApplyAtSpecifiedLocation)
      {
         this->SetDiaphragmLoadLocation(dlg.m_DiaphragmLoadLocation);
      }

      return true;
   }
   return false;
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
