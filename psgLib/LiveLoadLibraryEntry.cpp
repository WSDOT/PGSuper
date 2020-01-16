///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2020  Washington State Department of Transportation
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
#include <psgLib\LiveLoadLibraryEntry.h>

#include <System\IStructuredSave.h>
#include <System\IStructuredLoad.h>
#include <System\XStructuredLoad.h>

#include "resource.h"
#include <Units\sysUnits.h>
#include "LiveLoadDlg.h"

#include <MathEx.h>
#include <EAF\EAFApp.h>
#include <psgLib\LibraryEntryDifferenceItem.h>
#include <PgsExt\GirderLabel.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   LiveLoadLibraryEntry
****************************************************************************/



////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
LiveLoadLibraryEntry::LiveLoadLibraryEntry() :
m_IsNotional(false),
m_LiveLoadConfigurationType(lcTruckPlusLane),
m_LiveLoadApplicabilityType(pgsTypes::llaEntireStructure),
m_MaxVariableAxleSpacing(0.0),
m_VariableAxleIndex(INVALID_INDEX)
{
   m_LaneLoadSpanLength = 0; // always use lane load if it is defined

   // default to hs20-44
   m_LaneLoad   = ::ConvertToSysUnits( 0.64, unitMeasure::KipPerFoot );
   Axle axle;
   axle.Weight =  ::ConvertToSysUnits(  8.0, unitMeasure::Kip );
   AddAxle(axle);

   axle.Weight =  ::ConvertToSysUnits( 32.0, unitMeasure::Kip );
   axle.Spacing = ::ConvertToSysUnits( 14.0, unitMeasure::Feet );
   AddAxle(axle);

   AddAxle(axle);
}

LiveLoadLibraryEntry::LiveLoadLibraryEntry(const LiveLoadLibraryEntry& rOther) :
libLibraryEntry(rOther)
{
   MakeCopy(rOther);
}

LiveLoadLibraryEntry::~LiveLoadLibraryEntry()
{
}

//======================== OPERATORS  =======================================
LiveLoadLibraryEntry& LiveLoadLibraryEntry::operator= (const LiveLoadLibraryEntry& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
bool LiveLoadLibraryEntry::SaveMe(sysIStructuredSave* pSave)
{
   pSave->BeginUnit(_T("LiveLoadLibraryEntry"), 2.0);

   pSave->Property(_T("Name"),this->GetName().c_str());
   pSave->Property(_T("IsNotional"), m_IsNotional);
   pSave->Property(_T("LiveLoadApplicabilityType"), (Int16)m_LiveLoadApplicabilityType); // added version 2.0
   pSave->Property(_T("LiveLoadConfigurationType"), (Int16)m_LiveLoadConfigurationType);
   pSave->Property(_T("LaneLoad"), m_LaneLoad);
   pSave->Property(_T("LaneLoadSpanLength"),m_LaneLoadSpanLength); // added version 2.0
   pSave->Property(_T("MaxVariableAxleSpacing"), m_MaxVariableAxleSpacing);
   pSave->Property(_T("VariableAxleIndex"), m_VariableAxleIndex);

   pSave->BeginUnit(_T("Axles"), 1.0);
   pSave->Property(_T("AxleCount"), (long)m_Axles.size());

   for (AxleIterator it=m_Axles.begin(); it!=m_Axles.end(); it++)
   {
      const Axle& axle = *it;
      pSave->BeginUnit(_T("Axle"), 1.0);

      pSave->Property(_T("Weight"), axle.Weight);
      pSave->Property(_T("Spacing"), axle.Spacing);

      pSave->EndUnit();
   }

   pSave->EndUnit(); // Axles

   pSave->EndUnit();

   return false;
}

bool LiveLoadLibraryEntry::LoadMe(sysIStructuredLoad* pLoad)
{
   if(pLoad->BeginUnit(_T("LiveLoadLibraryEntry")))
   {
      Float64 version = pLoad->GetVersion();
      if (2.0 < version)
         THROW_LOAD(BadVersion,pLoad);

      std::_tstring name;
      if(pLoad->Property(_T("Name"),&name))
         this->SetName(name.c_str());
      else
         THROW_LOAD(InvalidFileFormat,pLoad);

      if(!pLoad->Property(_T("IsNotional"), &m_IsNotional))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if ( 1.0 < version ) // added version 2.0
      {
         if(!pLoad->Property(_T("LiveLoadApplicabilityType"), (Int16*)&m_LiveLoadApplicabilityType))
            THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if(!pLoad->Property(_T("LiveLoadConfigurationType"), (Int16*)&m_LiveLoadConfigurationType))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if(!pLoad->Property(_T("LaneLoad"), &m_LaneLoad))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if ( 1.0 < version ) // added version 2.0
      {
         if(!pLoad->Property(_T("LaneLoadSpanLength"), &m_LaneLoadSpanLength))
            THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if(!pLoad->Property(_T("MaxVariableAxleSpacing"), &m_MaxVariableAxleSpacing))
         THROW_LOAD(InvalidFileFormat,pLoad);



      if(!pLoad->Property(_T("VariableAxleIndex"), &m_VariableAxleIndex))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if(!pLoad->BeginUnit(_T("Axles")))
         THROW_LOAD(InvalidFileFormat,pLoad);

      AxleIndexType size;
      if(!pLoad->Property(_T("AxleCount"), &size))
         THROW_LOAD(InvalidFileFormat,pLoad);

      m_Axles.clear();

      for (AxleIndexType iaxl=0; iaxl<size; iaxl++)
      {
         if(!pLoad->BeginUnit(_T("Axle")))
            THROW_LOAD(InvalidFileFormat,pLoad);

         Axle axle;

         if(!pLoad->Property(_T("Weight"), &(axle.Weight)))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if(!pLoad->Property(_T("Spacing"), &(axle.Spacing)))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if(!pLoad->EndUnit())
            THROW_LOAD(InvalidFileFormat,pLoad);

         m_Axles.push_back(axle);
      }

      if(!pLoad->EndUnit())
         THROW_LOAD(InvalidFileFormat,pLoad);

      if(!pLoad->EndUnit())
         THROW_LOAD(InvalidFileFormat,pLoad);
   }
   else
      return false; // not a tb entry
   
   return true;
}

CString LiveLoadLibraryEntry::GetConfigurationType(LiveLoadConfigurationType configuration)
{
   LPCTSTR lpszConfiguration;
   switch(configuration )
   {
   case LiveLoadLibraryEntry::lcTruckOnly:
      lpszConfiguration = _T("Truck Only");
      break;
      
   case LiveLoadLibraryEntry::lcLaneOnly:
      lpszConfiguration = _T("Lane Load Only");
      break;
      
   case LiveLoadLibraryEntry::lcTruckPlusLane:
      lpszConfiguration = _T("Sum of Lane Load and Truck");
      break;

   case LiveLoadLibraryEntry::lcTruckLaneEnvelope:
      lpszConfiguration = _T("Envelope of Lane Load and Truck");
      break;

   default:
      ATLASSERT(false); // should never get here
   }

   return lpszConfiguration;
}

CString LiveLoadLibraryEntry::GetApplicabilityType(pgsTypes::LiveLoadApplicabilityType applicability)
{
   LPCTSTR lpszApplicability;
   switch(applicability)
   {
   case pgsTypes::llaEntireStructure:
      lpszApplicability = _T("Use for all actions at all locations");
      break;

   case pgsTypes::llaContraflexure:
      lpszApplicability = _T("Use only for negative moments between points of contraflexure and interior pier reactions");
      break;

   case pgsTypes::llaNegMomentAndInteriorPierReaction:
      lpszApplicability = _T("Use only for negative moments and interior pier reactions");
      break;

   default:
      ATLASSERT(false); // should never get here
   }
   return lpszApplicability;
}

bool LiveLoadLibraryEntry::IsEqual(const LiveLoadLibraryEntry& rOther,bool bConsiderName) const
{
   std::vector<pgsLibraryEntryDifferenceItem*> vDifferences;
   bool bMustRename;
   return Compare(rOther,vDifferences,bMustRename,true,bConsiderName);
}

bool LiveLoadLibraryEntry::Compare(const LiveLoadLibraryEntry& rOther, std::vector<pgsLibraryEntryDifferenceItem*>& vDifferences, bool& bMustRename, bool bReturnOnFirstDifference, bool considerName) const
{
   CEAFApp* pApp = EAFGetApp();
   const unitmgtIndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

   bMustRename = false;

   if ( m_IsNotional != rOther.m_IsNotional )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceBooleanItem(_T("Neglect axles that do not contribute to the maximum load effect under consideration"),m_IsNotional,rOther.m_IsNotional,_T("Checked"),_T("Unchecked")));
   }

   if ( m_LiveLoadConfigurationType != rOther.m_LiveLoadConfigurationType )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Load Type"),GetConfigurationType(m_LiveLoadConfigurationType),GetConfigurationType(rOther.m_LiveLoadConfigurationType)));
   }

   if ( m_LiveLoadApplicabilityType != rOther.m_LiveLoadApplicabilityType )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Usage"),GetApplicabilityType(m_LiveLoadApplicabilityType),GetApplicabilityType(rOther.m_LiveLoadApplicabilityType)));
   }

   if ( !::IsEqual(m_LaneLoad,rOther.m_LaneLoad) )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceForcePerLengthItem(_T("Lane Load"),m_LaneLoad,rOther.m_LaneLoad,pDisplayUnits->ForcePerLength));
   }

   if ( !::IsEqual(m_LaneLoadSpanLength,rOther.m_LaneLoadSpanLength) )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceLengthItem(_T("Lane Load Minimum Span Length"),m_LaneLoadSpanLength,rOther.m_LaneLoadSpanLength,pDisplayUnits->SpanLength));
   }

   if ( m_LiveLoadConfigurationType != LiveLoadLibraryEntry::lcLaneOnly )
   {
      AxleIndexType nAxles = m_Axles.size();
      if ( nAxles != rOther.m_Axles.size() )
      {
         RETURN_ON_DIFFERENCE;
         vDifferences.push_back(new pgsLibraryEntryDifferenceIndexItem(_T("Number of Axles"),nAxles,rOther.m_Axles.size()));
      }

      if ( m_VariableAxleIndex != rOther.m_VariableAxleIndex )
      {
         RETURN_ON_DIFFERENCE;
         vDifferences.push_back(new pgsLibraryEntryDifferenceIndexItem(_T("Variable Axle Index"),m_VariableAxleIndex,rOther.m_VariableAxleIndex));
      }

      if ( !::IsEqual(m_MaxVariableAxleSpacing,rOther.m_MaxVariableAxleSpacing) )
      {
         RETURN_ON_DIFFERENCE;
         vDifferences.push_back(new pgsLibraryEntryDifferenceLengthItem(_T("Maximum Variable Axle Spacing"),m_MaxVariableAxleSpacing,rOther.m_MaxVariableAxleSpacing,pDisplayUnits->SpanLength));
      }

      for ( AxleIndexType axleIdx = 0; axleIdx < nAxles; axleIdx++ )
      {
         const Axle& axle = m_Axles[axleIdx];
         const Axle& otherAxle = rOther.m_Axles[axleIdx];

         if ( !::IsEqual(axle.Weight,otherAxle.Weight) )
         {
            RETURN_ON_DIFFERENCE;
            CString strAxle;
            strAxle.Format(_T("Axle %d - Weight"),LABEL_INDEX(axleIdx));
            vDifferences.push_back(new pgsLibraryEntryDifferenceForceItem(strAxle,axle.Weight,otherAxle.Weight,pDisplayUnits->GeneralForce));
         }

         if ( !::IsEqual(axle.Spacing,otherAxle.Spacing) )
         {
            RETURN_ON_DIFFERENCE;
            CString strAxle;
            strAxle.Format(_T("Axle %d - Spacing"),LABEL_INDEX(axleIdx));
            vDifferences.push_back(new pgsLibraryEntryDifferenceLengthItem(strAxle,axle.Spacing,otherAxle.Spacing,pDisplayUnits->SpanLength));
         }
      }

   }

   if (considerName &&  GetName() != rOther.GetName() )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Name"),GetName().c_str(),rOther.GetName().c_str()));
   }

   return vDifferences.size() == 0 ? true : false;
}


//======================== ACCESS     =======================================
void LiveLoadLibraryEntry::SetIsNotional(bool isno)
{
   m_IsNotional=isno;
}

bool LiveLoadLibraryEntry::GetIsNotional() const
{
   return m_IsNotional;
}

void LiveLoadLibraryEntry::SetLiveLoadConfigurationType(LiveLoadConfigurationType config)
{
   ASSERT(config>=0 && config<=lcTruckLaneEnvelope);
   m_LiveLoadConfigurationType=config;
}

LiveLoadLibraryEntry::LiveLoadConfigurationType LiveLoadLibraryEntry::GetLiveLoadConfigurationType() const
{
   return m_LiveLoadConfigurationType;
}

void LiveLoadLibraryEntry::SetLiveLoadApplicabilityType(pgsTypes::LiveLoadApplicabilityType applicability)
{
   m_LiveLoadApplicabilityType = applicability;
}

pgsTypes::LiveLoadApplicabilityType LiveLoadLibraryEntry::GetLiveLoadApplicabilityType() const
{
   return m_LiveLoadApplicabilityType;
}

void LiveLoadLibraryEntry::SetLaneLoad(Float64 load)
{
   ASSERT(load>=0);
   m_LaneLoad=load;
}

Float64 LiveLoadLibraryEntry::GetLaneLoad() const
{
   return m_LaneLoad;
}

void LiveLoadLibraryEntry::SetLaneLoadSpanLength(Float64 length)
{
   ASSERT(0 <= length);
   m_LaneLoadSpanLength = length;
}

Float64 LiveLoadLibraryEntry::GetLaneLoadSpanLength() const
{
   return m_LaneLoadSpanLength;
}

AxleIndexType LiveLoadLibraryEntry::GetNumAxles() const
{
   return m_Axles.size();
}

void LiveLoadLibraryEntry::AddAxle(Axle axle)
{
   ASSERT(axle.Weight>0.0);

   m_Axles.push_back(axle);
}

void LiveLoadLibraryEntry::SetAxle(AxleIndexType idx, Axle axle)
{
   ASSERT(0 <= idx && idx < m_Axles.size());
   ASSERT(0.0 < axle.Weight);
   ASSERT(0.0 < axle.Spacing);

   Axle& raxle = m_Axles[idx];
   raxle.Spacing = axle.Spacing;
   raxle.Weight = axle.Weight;
}

LiveLoadLibraryEntry::Axle LiveLoadLibraryEntry::GetAxle(AxleIndexType idx) const
{
   ASSERT(0 <= idx && idx < m_Axles.size());
   return m_Axles[idx];
}

void LiveLoadLibraryEntry::ClearAxles()
{
   m_Axles.clear();
}

void LiveLoadLibraryEntry::SetVariableAxleIndex(AxleIndexType idx)
{
   m_VariableAxleIndex = idx;
}

AxleIndexType LiveLoadLibraryEntry::GetVariableAxleIndex() const
{
   return m_VariableAxleIndex;
}

void LiveLoadLibraryEntry::SetMaxVariableAxleSpacing(Float64 val)
{
   ASSERT(val>=0.0);
   m_MaxVariableAxleSpacing = val;
}

Float64 LiveLoadLibraryEntry::GetMaxVariableAxleSpacing() const
{
   return m_MaxVariableAxleSpacing;
}


//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================

bool LiveLoadLibraryEntry::Edit(bool allowEditing,int nPage)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // exchange data with dialog
   CLiveLoadDlg dlg(allowEditing);
   dlg.m_EntryName  = this->GetName().c_str();
   dlg.m_ConfigType = this->GetLiveLoadConfigurationType();
   dlg.m_UsageType  = this->GetLiveLoadApplicabilityType();
   dlg.m_LaneLoad   = this->GetLaneLoad();
   dlg.m_LaneLoadSpanLength = this->GetLaneLoadSpanLength();
   dlg.m_IsNotional = this->GetIsNotional() ? TRUE : FALSE;

   dlg.m_MaxVariableAxleSpacing = this->GetMaxVariableAxleSpacing();
   dlg.m_VariableAxleIndex      = this->GetVariableAxleIndex();

   AxleIndexType naxles = this->GetNumAxles();
   for (AxleIndexType iax=0; iax<naxles; iax++)
   {
      LiveLoadLibraryEntry::Axle axle = this->GetAxle(iax);
      dlg.m_Axles.push_back(axle);
   }

   INT_PTR i = dlg.DoModal();
   if (i==IDOK)
   {
      this->SetLiveLoadConfigurationType(dlg.m_ConfigType);
      this->SetLiveLoadApplicabilityType(dlg.m_UsageType);
      this->SetLaneLoad(dlg.m_LaneLoad);
      this->SetLaneLoadSpanLength(dlg.m_LaneLoadSpanLength);
      this->SetIsNotional(dlg.m_IsNotional==TRUE);

      this->SetMaxVariableAxleSpacing(dlg.m_MaxVariableAxleSpacing);
      this->SetVariableAxleIndex(dlg.m_VariableAxleIndex);

      m_Axles = dlg.m_Axles;

      return true;
   }

   return false;
}


void LiveLoadLibraryEntry::MakeCopy(const LiveLoadLibraryEntry& rOther)
{
   m_IsNotional = rOther.m_IsNotional;
   m_LiveLoadConfigurationType = rOther.m_LiveLoadConfigurationType;
   m_LiveLoadApplicabilityType = rOther.m_LiveLoadApplicabilityType;
   m_LaneLoad = rOther.m_LaneLoad;
   m_LaneLoadSpanLength = rOther.m_LaneLoadSpanLength;
   m_MaxVariableAxleSpacing = rOther.m_MaxVariableAxleSpacing;
   m_VariableAxleIndex = rOther.m_VariableAxleIndex;
   m_Axles = rOther.m_Axles;
}

void LiveLoadLibraryEntry::MakeAssignment(const LiveLoadLibraryEntry& rOther)
{
   libLibraryEntry::MakeAssignment( rOther );
   MakeCopy( rOther );
}

HICON  LiveLoadLibraryEntry::GetIcon() const
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   return ::LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_LIVE_LOAD_ENTRY) );
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
bool LiveLoadLibraryEntry::AssertValid() const
{
   return libLibraryEntry::AssertValid();
}

void LiveLoadLibraryEntry::Dump(dbgDumpContext& os) const
{
   os << _T("Dump for LiveLoadLibraryEntry ")<< GetName() <<endl;

   os << _T("   m_IsNotional                = ")<< m_IsNotional <<endl;
   os << _T("   m_LiveLoadConfigurationType = ")<< m_LiveLoadConfigurationType<<endl;
   os << _T("   m_LaneLoad                  = ")<< m_LaneLoad<<endl;
   os << _T("   m_MaxVariableAxleSpacing    = ")<< m_MaxVariableAxleSpacing<<endl;
   os << _T("   m_VariableAxleIndex         = ")<< m_VariableAxleIndex<<endl;

   AxleIndexType size = m_Axles.size();
   os << _T("   Number of Axles = ")<<size<<endl;
   for (AxleIndexType iaxl=0; iaxl<size; iaxl++)
   {
      os<<_T("    Axle ")<<iaxl<<_T(" Weight  = ")<<m_Axles[iaxl].Weight<<endl;
      os<<_T("    Axle ")<<iaxl<<_T(" Spacing = ")<<m_Axles[iaxl].Spacing<<endl;
   }

   libLibraryEntry::Dump( os );
}
#endif // _DEBUG

#if defined _UNITTEST
bool LiveLoadLibraryEntry::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("LiveLoadLibraryEntry");

   // tests are performed on entire library.

   TESTME_EPILOG("LiveLoadLibraryEntry");
}
#endif // _UNITTEST
