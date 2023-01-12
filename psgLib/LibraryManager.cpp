///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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
#include <psgLib\LibraryManager.h>

#include <EAF\EAFDocument.h>
#include <EAF\EAFUtilities.h>
#include <psgLib\LibraryEditorDoc.h>

#include <IFace\DocumentType.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   psgLibraryManager
****************************************************************************/

GirderLibrary::GirderLibrary(LPCTSTR idName, LPCTSTR displayName, bool bIsDepreciated) :
GirderLibraryBase(idName,displayName,bIsDepreciated)
{
}

bool GirderLibrary::NewEntry(LPCTSTR key)
{
   // Replacing the implementation of the base class... we want to prompt the user
   // for if they want to create a regular girder or a spliced girder. Only do this
   // if the parent document is a CLibraryEditorDoc. If the library is open in an application
   // the girder families are restricted to only those families supported by the application, 
   // therefore the default option is what we want to use

   if ( IsReservedName(key) )
      return false;

   // check if key exists already
   const GirderLibraryEntry* pentry = LookupEntryPrv(key);

   if (!pentry)
   {
      // doesn't exist - add a new one.
      GirderLibraryEntry::CreateType createType = GirderLibraryEntry::DEFAULT;
      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);
      CEAFDocument* pEAFDoc = EAFGetDocument();
      if ( pBroker )
      {
         CComPtr<IDocumentType> pDocType;
         pBroker->GetInterface(IID_IDocumentType,(IUnknown**)&pDocType);
         ATLASSERT(pDocType);
         createType = (pDocType->IsPGSuperDocument() ? GirderLibraryEntry::PRECAST : GirderLibraryEntry::SPLICED);
      }
      else if ( pEAFDoc->IsKindOf(RUNTIME_CLASS(CLibraryEditorDoc)) )
      {
         CString strResponses(_T("Precast Girder (PGSuper)\nPrecast Spliced Girder Segment (PGSplice)"));
         int result = AfxRBChoose(_T("Select Girder Type"),_T("What type of girder library entry would you like to create?"),strResponses,0,TRUE);
         if ( result == -1 )
         {
            return false; // user pressed cancel
         }

         createType = (result == 0 ? GirderLibraryEntry::PRECAST : GirderLibraryEntry::SPLICED);
      }
      LibItem apentry( new GirderLibraryEntry(createType) );
      apentry->SetName(key);
      apentry->SetLibrary(this);
      m_EntryList.insert(EntryList::value_type(key, apentry));
      return true;
   }
   else
   {
      return false;
   }
}


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
psgLibraryManager::psgLibraryManager() :
libLibraryManager()
{
   std::unique_ptr<ConcreteLibrary>        conc_lib(std::make_unique<ConcreteLibrary>(_T("CONCRETE_LIBRARY"), _T("Concrete Types")));
   std::unique_ptr<ConnectionLibrary>      conn_lib(std::make_unique<ConnectionLibrary>(_T("CONNECTION_LIBRARY"), _T("Connections")));
   std::unique_ptr<GirderLibrary>          gird_lib(std::make_unique<GirderLibrary>(_T("GIRDER_LIBRARY"), _T("Girders")));
   std::unique_ptr<DiaphragmLayoutLibrary> diap_lib(std::make_unique<DiaphragmLayoutLibrary>(_T("DIAPHRAGM_LAYOUT_LIBRARY"), _T("Diaphragm Layouts"),true));
   std::unique_ptr<TrafficBarrierLibrary>  barr_lib(std::make_unique<TrafficBarrierLibrary>(_T("TRAFFIC_BARRIER_LIBRARY"), _T("Traffic Barriers")));
   std::unique_ptr<SpecLibrary>            spec_lib(std::make_unique<SpecLibrary>(_T("SPECIFICATION_LIBRARY"), _T("Project Criteria")));
   std::unique_ptr<RatingLibrary>          rate_lib(std::make_unique<RatingLibrary>(_T("RATING_LIBRARY"), _T("Load Rating Criteria")));
   std::unique_ptr<LiveLoadLibrary>        live_lib(std::make_unique<LiveLoadLibrary>(_T("USER_LIVE_LOAD_LIBRARY"), _T("User-Defined Live Loads")));
   std::unique_ptr<DuctLibrary>            duct_lib(std::make_unique<DuctLibrary>(_T("DUCT_LIBRARY"), _T("Ducts")));
   std::unique_ptr<HaulTruckLibrary>       haul_lib(std::make_unique<HaulTruckLibrary>(_T("HAUL_TRUCK_LIBRARY"), _T("Haul Trucks")));

   live_lib->AddReservedName(_T("HL-93"));
   live_lib->AddReservedName(_T("Fatigue"));
   live_lib->AddReservedName(_T("Pedestrian on Sidewalk"));
   live_lib->AddReservedName(_T("AASHTO Legal Loads"));
   live_lib->AddReservedName(_T("Notional Rating Load (NRL)"));
   live_lib->AddReservedName(_T("Single-Unit SHVs"));
   live_lib->AddReservedName(_T("Emergency Vehicles"));
   live_lib->AddReservedName(NO_LIVE_LOAD_DEFINED); // this is for the dummy live load, when one isn't defined for analysis

   // don't change the order that the libraries are added to the library manager
   // add new libraries at the end of this list
   m_ConcLibIdx      = AddLibrary(conc_lib.release()); 
   m_ConnLibIdx      = AddLibrary(conn_lib.release());
   m_GirdLibIdx      = AddLibrary(gird_lib.release());
   m_DiapLibIdx      = AddLibrary(diap_lib.release());
   m_BarrLibIdx      = AddLibrary(barr_lib.release());
   m_SpecLibIdx      = AddLibrary(spec_lib.release());
   m_LiveLibIdx      = AddLibrary(live_lib.release());
   m_RatingLibIdx    = AddLibrary(rate_lib.release());
   m_DuctLibIdx      = AddLibrary(duct_lib.release());
   m_HaulTruckLibIdx = AddLibrary(haul_lib.release());

   m_strServer = _T("Unknown");
   m_strLibFile   = _T("Unknown");

   UpdateLibraryNames();
}

psgLibraryManager::~psgLibraryManager()
{
   // Release all the class factories before the DLL unloads
   GirderLibraryEntry::ms_ClassFactories.clear();
   GirderLibraryEntry::ms_ExternalCLSIDTranslators.clear();
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================

ConcreteLibrary&        psgLibraryManager::GetConcreteLibrary()
{
   libILibrary* pl = GetLibrary(m_ConcLibIdx);
   ConcreteLibrary* pc = dynamic_cast<ConcreteLibrary*>(pl);
   ASSERT(pc);
   return *pc;
}

const ConcreteLibrary&        psgLibraryManager::GetConcreteLibrary() const
{
   const libILibrary* pl = GetLibrary(m_ConcLibIdx);
   const ConcreteLibrary* pc = dynamic_cast<const ConcreteLibrary*>(pl);
   ASSERT(pc);
   return *pc;
}

ConnectionLibrary&      psgLibraryManager::GetConnectionLibrary()
{
   libILibrary* pl = GetLibrary(m_ConnLibIdx);
   ConnectionLibrary* pc = dynamic_cast<ConnectionLibrary*>(pl);
   ASSERT(pc);
   return *pc;
}

const ConnectionLibrary&      psgLibraryManager::GetConnectionLibrary() const
{
   const libILibrary* pl = GetLibrary(m_ConnLibIdx);
   const ConnectionLibrary* pc = dynamic_cast<const ConnectionLibrary*>(pl);
   ASSERT(pc);
   return *pc;
}

GirderLibrary&          psgLibraryManager::GetGirderLibrary()
{
   libILibrary* pl = GetLibrary(m_GirdLibIdx);
   GirderLibrary* pc = dynamic_cast<GirderLibrary*>(pl);
   ASSERT(pc);
   return *pc;
}

const GirderLibrary&          psgLibraryManager::GetGirderLibrary() const
{
   const libILibrary* pl = GetLibrary(m_GirdLibIdx);
   const GirderLibrary* pc = dynamic_cast<const GirderLibrary*>(pl);
   ASSERT(pc);
   return *pc;
}

DiaphragmLayoutLibrary& psgLibraryManager::GetDiaphragmLayoutLibrary()
{
   libILibrary* pl = GetLibrary(m_DiapLibIdx);
   DiaphragmLayoutLibrary* pc = dynamic_cast<DiaphragmLayoutLibrary*>(pl);
   ASSERT(pc);
   return *pc;
}

const DiaphragmLayoutLibrary& psgLibraryManager::GetDiaphragmLayoutLibrary() const
{
   const libILibrary* pl = GetLibrary(m_DiapLibIdx);
   const DiaphragmLayoutLibrary* pc = dynamic_cast<const DiaphragmLayoutLibrary*>(pl);
   ASSERT(pc);
   return *pc;
}

TrafficBarrierLibrary& psgLibraryManager::GetTrafficBarrierLibrary()
{
   libILibrary* pl = GetLibrary(m_BarrLibIdx);
   TrafficBarrierLibrary* pc = dynamic_cast<TrafficBarrierLibrary*>(pl);
   ASSERT(pc);
   return *pc;
}

const TrafficBarrierLibrary& psgLibraryManager::GetTrafficBarrierLibrary() const
{
   const libILibrary* pl = GetLibrary(m_BarrLibIdx);
   const TrafficBarrierLibrary* pc = dynamic_cast<const TrafficBarrierLibrary*>(pl);
   ASSERT(pc);
   return *pc;
}

SpecLibrary* psgLibraryManager::GetSpecLibrary()
{
   libILibrary* pl = GetLibrary(m_SpecLibIdx);
   SpecLibrary* pc = dynamic_cast<SpecLibrary*>(pl);
   ASSERT(pc);
   return pc;
}

const SpecLibrary* psgLibraryManager::GetSpecLibrary() const
{
   const libILibrary* pl = GetLibrary(m_SpecLibIdx);
   const SpecLibrary* pc = dynamic_cast<const SpecLibrary*>(pl);
   ASSERT(pc);
   return pc;
}

RatingLibrary* psgLibraryManager::GetRatingLibrary()
{
   libILibrary* pl = GetLibrary(m_RatingLibIdx);
   RatingLibrary* pc = dynamic_cast<RatingLibrary*>(pl);
   ASSERT(pc);
   return pc;
}

const RatingLibrary* psgLibraryManager::GetRatingLibrary() const
{
   const libILibrary* pl = GetLibrary(m_RatingLibIdx);
   const RatingLibrary* pc = dynamic_cast<const RatingLibrary*>(pl);
   ASSERT(pc);
   return pc;
}

LiveLoadLibrary* psgLibraryManager::GetLiveLoadLibrary()
{
   libILibrary* pl = GetLibrary(m_LiveLibIdx);
   LiveLoadLibrary* pc = dynamic_cast<LiveLoadLibrary*>(pl);
   ASSERT(pc);
   return pc;
}

const LiveLoadLibrary* psgLibraryManager::GetLiveLoadLibrary() const
{
   const libILibrary* pl = GetLibrary(m_LiveLibIdx);
   const LiveLoadLibrary* pc = dynamic_cast<const LiveLoadLibrary*>(pl);
   ASSERT(pc);
   return pc; 
}

DuctLibrary* psgLibraryManager::GetDuctLibrary()
{
   libILibrary* pl = GetLibrary(m_DuctLibIdx);
   DuctLibrary* pc = dynamic_cast<DuctLibrary*>(pl);
   ASSERT(pc);
   return pc;
}

const DuctLibrary* psgLibraryManager::GetDuctLibrary() const
{
   const libILibrary* pl = GetLibrary(m_DuctLibIdx);
   const DuctLibrary* pc = dynamic_cast<const DuctLibrary*>(pl);
   ASSERT(pc);
   return pc; 
}

HaulTruckLibrary* psgLibraryManager::GetHaulTruckLibrary()
{
   libILibrary* pl = GetLibrary(m_HaulTruckLibIdx);
   HaulTruckLibrary* pc = dynamic_cast<HaulTruckLibrary*>(pl);
   ASSERT(pc);
   return pc;
}

const HaulTruckLibrary* psgLibraryManager::GetHaulTruckLibrary() const
{
   const libILibrary* pl = GetLibrary(m_HaulTruckLibIdx);
   const HaulTruckLibrary* pc = dynamic_cast<const HaulTruckLibrary*>(pl);
   ASSERT(pc);
   return pc; 
}

bool psgLibraryManager::LoadMe(sysIStructuredLoad* pLoad)
{
   bool bResult = libLibraryManager::LoadMe(pLoad);
   return bResult;
}

void psgLibraryManager::UpdateLibraryNames()
{
   // Some libraries have been renamed. So as not to mess up the file format
   // simply change the display name
   SpecLibrary* pLib = GetSpecLibrary();
   pLib->SetDisplayName(_T("Project Criteria"));

   ConcreteLibrary& ConcreteLib = GetConcreteLibrary();
   ConcreteLib.SetDisplayName(_T("Concrete"));

   LiveLoadLibrary* LiveLoadLib = GetLiveLoadLibrary();
   LiveLoadLib->SetDisplayName(_T("Vehicular Live Load"));
}

void psgLibraryManager::SetMasterLibraryInfo(LPCTSTR strServer,LPCTSTR strConfiguration,LPCTSTR strLibFile)
{
   m_strServer = strServer;
   m_strConfiguration = strConfiguration;
   m_strLibFile   = strLibFile;
}

void psgLibraryManager::GetMasterLibraryInfo(std::_tstring& strServer, std::_tstring& strConfiguration, std::_tstring& strLibFile) const
{
   strServer = m_strServer;
   strConfiguration = m_strConfiguration;
   strLibFile = m_strLibFile;
}

//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
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
bool psgLibraryManager::AssertValid() const
{
   return libLibraryManager::AssertValid();
}

void psgLibraryManager::Dump(dbgDumpContext& os) const
{
   os << _T("Dump for psgLibraryManager")<<endl;

   os << _T(" m_ConcLibIdx = ")<< m_ConcLibIdx << endl;
   os << _T(" m_ConnLibIdx = ")<< m_ConnLibIdx << endl;
   os << _T(" m_GirdLibIdx = ")<< m_GirdLibIdx << endl;
   os << _T(" m_DiapLibIdx = ")<< m_DiapLibIdx << endl;
   os << _T(" m_BarrLibIdx = ")<< m_BarrLibIdx << endl;
   os << _T(" m_SpecLibIdx = ")<< m_SpecLibIdx << endl;
   os << _T(" m_RatingLibIdx = ")<< m_RatingLibIdx << endl;

   libLibraryManager::Dump( os );
}
#endif // _DEBUG

#if defined UNIT_TEST
bool psgLibraryManager::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("psgLibraryManager");

   // Create a library manager to be used through the testing procedure
   psgLibraryManager libmgr;

   // Test the library specific accessor methods
   ConcreteLibrary& conc_lib = libmgr.GetConcreteLibrary();
   const type_info& conc_lib_ti = typeid( conc_lib );
   TRY_TESTME( std::_tstring( conc_lib_ti.name() ) == std::_tstring(_T("libLibrary<ConcreteLibraryEntry>")) );

   ConnectionLibrary& conn_lib = libmgr.GetConnectionLibrary();
   const type_info& conn_lib_ti = typeid( conn_lib );
   TRY_TESTME( std::_tstring( conc_lib_ti.name() ) == std::_tstring(_T("libLibrary<ConnectionLibraryEntry>")) );

   GirderLibrary& gdr_lib = libmgr.GetGirderLibrary();
   const type_info& gdr_lib_ti = typeid( gdr_lib );
   TRY_TESTME( std::_tstring( gdr_lib_ti.name() ) == std::_tstring(_T("libLibrary<GirderLibraryEntry>")) );

   DiaphragmLayoutLibrary& diaph_lib = libmgr.GetDiaphragmLayoutLibrary();
   const type_info& diaph_lib_ti = typeid( diaph_lib );
   TRY_TESTME( std::_tstring( diaph_lib_ti.name() ) == std::_tstring(_T("libLibrary<DiaphragmLayoutEntry>")) );

   SpecLibrary* spec_lib = libmgr.GetSpecLibrary();
   const type_info& spec_lib_ti = typeid( *spec_lib );
   TRY_TESTME( std::_tstring( spec_lib_ti.name() ) == std::_tstring(_T("libLibrary<SpecLibraryEntry>")) );

   RatingLibrary* rating_lib = libmgr.GetRatingLibrary();
   const type_info& rating_lib_ti = typeid( *rating_lib );
   TRY_TESTME( std::_tstring( rating_lib_ti.name() ) == std::_tstring(_T("libLibrary<RatingLibraryEntry>")) );

   TESTME_EPILOG("LibraryManager");
}
#endif // UNIT_TEST
