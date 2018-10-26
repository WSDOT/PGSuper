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
#include <psgLib\HaulTruckLibraryEntry.h>
#include <psgLib\LibraryEntryDifferenceItem.h>

#include <System\IStructuredSave.h>
#include <System\IStructuredLoad.h>
#include <System\XStructuredLoad.h>

#include "resource.h"
#include "HaulTruckDlg.h"
#include <Units\sysUnits.h>

#include <EAF\EAFApp.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

HaulTruckLibraryEntry::HaulTruckLibraryEntry()
{
   m_Hbg       = ::ConvertToSysUnits(72,unitMeasure::Inch);
   m_Hrc       = ::ConvertToSysUnits(24,unitMeasure::Inch);
   m_Wcc       = ::ConvertToSysUnits(72,unitMeasure::Inch);
   m_Ktheta    = ::ConvertToSysUnits(40000,unitMeasure::KipInchPerRadian);
   m_Lmax      = ::ConvertToSysUnits(230,unitMeasure::Feet);
   m_MaxOH     = ::ConvertToSysUnits(15,unitMeasure::Feet);
   m_MaxWeight = ::ConvertToSysUnits(200,unitMeasure::Kip);
}

HaulTruckLibraryEntry::HaulTruckLibraryEntry(const HaulTruckLibraryEntry& rOther) :
libLibraryEntry(rOther)
{
   MakeCopy(rOther);
}

HaulTruckLibraryEntry::~HaulTruckLibraryEntry()
{
}

//======================== OPERATORS  =======================================
HaulTruckLibraryEntry& HaulTruckLibraryEntry::operator= (const HaulTruckLibraryEntry& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
bool HaulTruckLibraryEntry::SaveMe(sysIStructuredSave* pSave)
{
   pSave->BeginUnit(_T("HaulTruckEntry"), 1.0);
   pSave->Property(_T("Name"),GetName().c_str());
   pSave->Property(_T("Hbg"),m_Hbg);
   pSave->Property(_T("Hrc"),m_Hrc);
   pSave->Property(_T("Wcc"),m_Wcc);
   pSave->Property(_T("Ktheta"),m_Ktheta);
   pSave->Property(_T("Lmax"),m_Lmax);
   pSave->Property(_T("MaxOH"),m_MaxOH);
   pSave->Property(_T("MaxWeight"),m_MaxWeight);
   pSave->EndUnit();

   return true;
}

bool HaulTruckLibraryEntry::LoadMe(sysIStructuredLoad* pLoad)
{
   if(pLoad->BeginUnit(_T("HaulTruckEntry")))
   {
      std::_tstring name;
      if(pLoad->Property(_T("Name"),&name))
      {
         SetName(name.c_str());
      }
      else
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->Property(_T("Hbg"),&m_Hbg) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->Property(_T("Hrc"),&m_Hrc) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->Property(_T("Wcc"),&m_Wcc) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->Property(_T("Ktheta"),&m_Ktheta) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->Property(_T("Lmax"),&m_Lmax) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->Property(_T("MaxOH"),&m_MaxOH) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->Property(_T("MaxWeight"),&m_MaxWeight) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->EndUnit() )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }
   }
   else
   {
      return false; // not a HaulTruck entry
   }
   
   return true;
}

bool HaulTruckLibraryEntry::IsEqual(const HaulTruckLibraryEntry& rOther,bool bConsiderName) const
{
   std::vector<pgsLibraryEntryDifferenceItem*> vDifferences;
   return Compare(rOther,vDifferences,true,bConsiderName);
}

bool HaulTruckLibraryEntry::Compare(const HaulTruckLibraryEntry& rOther, std::vector<pgsLibraryEntryDifferenceItem*>& vDifferences, bool bReturnOnFirstDifference, bool considerName) const
{
   if ( !::IsEqual(m_Hbg,rOther.m_Hbg) )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Hauling trucks are different"),_T(""),_T("")));
   }

   if ( !::IsEqual(m_Hrc,rOther.m_Hrc) )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Hauling trucks are different"),_T(""),_T("")));
   }

   if ( !::IsEqual(m_Wcc,rOther.m_Wcc) )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Hauling trucks are different"),_T(""),_T("")));
   }

   if ( !::IsEqual(m_Ktheta,rOther.m_Ktheta) )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Hauling trucks are different"),_T(""),_T("")));
   }

   if ( !::IsEqual(m_Lmax,rOther.m_Lmax) )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Hauling trucks are different"),_T(""),_T("")));
   }

   if ( !::IsEqual(m_MaxOH,rOther.m_MaxOH) )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Hauling trucks are different"),_T(""),_T("")));
   }

   if ( !::IsEqual(m_MaxWeight,rOther.m_MaxWeight) )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Hauling trucks are different"),_T(""),_T("")));
   }

   return vDifferences.size() == 0 ? true : false;
}

Float64 HaulTruckLibraryEntry::GetBottomOfGirderHeight() const
{
   return m_Hbg;
}

void HaulTruckLibraryEntry::SetBottomOfGirderHeight(Float64 hbg)
{
   m_Hbg = hbg;
}

Float64 HaulTruckLibraryEntry::GetRollCenterHeight() const
{
   return m_Hrc;
}

void HaulTruckLibraryEntry::SetRollCenterHeight(Float64 hrc)
{
   m_Hrc = hrc;
}

Float64 HaulTruckLibraryEntry::GetAxleWidth() const
{
   return m_Wcc;
}

void HaulTruckLibraryEntry::SetAxleWidth(Float64 wcc)
{
   m_Wcc = wcc;
}

Float64 HaulTruckLibraryEntry::GetRollStiffness() const
{
   return m_Ktheta;
}

void HaulTruckLibraryEntry::SetRollStiffness(Float64 ktheta)
{
   m_Ktheta = ktheta;
}

Float64 HaulTruckLibraryEntry::GetMaxDistanceBetweenBunkPoints() const
{
   return m_Lmax;
}

void HaulTruckLibraryEntry::SetMaxDistanceBetweenBunkPoints(Float64 lmax)
{
   m_Lmax = lmax;
}

Float64 HaulTruckLibraryEntry::GetMaximumLeadingOverhang() const
{
   return m_MaxOH;
}

void HaulTruckLibraryEntry::SetMaximumLeadingOverhang(Float64 maxoh)
{
   m_MaxOH = maxoh;
}

Float64 HaulTruckLibraryEntry::GetMaxGirderWeight() const
{
   return m_MaxWeight;
}

void HaulTruckLibraryEntry::SetMaxGirderWeight(Float64 maxwgt)
{
   m_MaxWeight = maxwgt;
}

HICON HaulTruckLibraryEntry::GetIcon() const
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   return ::LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_HAUL_TRUCK_ENTRY) );
}

bool HaulTruckLibraryEntry::Edit(bool allowEditing,int nPage)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CHaulTruckDlg dlg(allowEditing);
   dlg.m_Name = CString(GetName().c_str());
   dlg.m_Hbg       = m_Hbg;
   dlg.m_Hrc       = m_Hrc;
   dlg.m_Wcc       = m_Wcc;
   dlg.m_Ktheta    = m_Ktheta;
   dlg.m_Lmax      = m_Lmax;
   dlg.m_MaxOH     = m_MaxOH;
   dlg.m_MaxWeight = m_MaxWeight;

   if ( dlg.DoModal() == IDOK )
   {
      m_Hbg       = dlg.m_Hbg;
      m_Hrc       = dlg.m_Hrc;
      m_Wcc       = dlg.m_Wcc;
      m_Ktheta    = dlg.m_Ktheta;
      m_Lmax      = dlg.m_Lmax;
      m_MaxOH     = dlg.m_MaxOH;
      m_MaxWeight = dlg.m_MaxWeight;
      return true;
   }

   return false;
}

void HaulTruckLibraryEntry::MakeCopy(const HaulTruckLibraryEntry& rOther)
{
   m_Hbg       = rOther.m_Hbg;
   m_Hrc       = rOther.m_Hrc;
   m_Wcc       = rOther.m_Wcc;
   m_Ktheta    = rOther.m_Ktheta;
   m_Lmax      = rOther.m_Lmax;
   m_MaxOH     = rOther.m_MaxOH;
   m_MaxWeight = rOther.m_MaxWeight;
}

void HaulTruckLibraryEntry::MakeAssignment(const HaulTruckLibraryEntry& rOther)
{
   libLibraryEntry::MakeAssignment( rOther );
   MakeCopy( rOther );
}
