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
#include "resource.h"
#include <psgLib\GirderLibraryEntry.h>
#include <System\IStructuredSave.h>
#include <System\IStructuredLoad.h>
#include <System\XStructuredLoad.h>

#include <IFace\BeamFactory.h>

#include <psgLib\BeamFamilyManager.h>
#include <PgsExt\GirderLabel.h>

#include <GeomModel\PrecastBeam.h>

#include "GirderMainSheet.h"

#include "ComCat.h"
#include "PGSuperCatCom.h"
#include "PGSpliceCatCom.h"
#include <Plugins\BeamFactoryCATID.h>
#include <Plugins\BeamFamilyCLSID.h>
#include <Plugins\Beams.h>

#include <MathEx.h>

#include <WBFLSTL.h>

#include <WBFLGeometry.h>
#include <WBFLGenericBridge.h>
#include <WBFLGenericBridgeTools.h>

#include <LRFD\RebarPool.h>

#include <EAF\EAFApp.h>
#include <psgLib\LibraryEntryDifferenceItem.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define DUMMY_AGENT_ID INVALID_ID

// from  2.0 to 3.0, added chamfer dimensions to several sections
// from  3.0 to 4.0, harping point locations
// from  4.0 to 5.0, added global sort order for prestressing strands
//                   replaced strand adjustments with cover values
// from  5.0 to 6.0  added diaphragm layout rules
// from  6.0 to 7.0  got rid of cover and added direct harped strand adjustments
// from  7.0 to 8.0  added Jmax to voided slab
// from  8.0 to 9.0  added Jmax to box beams
// from  9.0 to 10.0 re-arranged box dimensions 
// from 10.0 to 11.0 added horizontal transverse reinforcement
// from 11.0 to 12.0 chose whether individual straight strands can be debonded (was global)
// from 12.0 to 13.0 added debond criteria. This was removed from the spec criteria
// from 13.0 to 14.0 added SectionDimensions data block
// from 14.0 to 15.0 added strand grids for each end of the girder
// from 15.0 to 16.0 added input for post-tensioning
// from 16.0 to 17.0 removed the post-tensioning input
// from 17.0 to 18.0 changed class for new rebar object
// from 18.0 to 19.0 move all shear reinforcment into CShearData. Created LegacyShearData for conversion
// from 19.0 to 20.0 added shear design parameters
// from 20.0 to 21.0 Web strands can be harped or straight e.g.,  ForceHarpedStrandsStraight
// from 21.0 to 22.0 Added layout options for longitudinal rebar
// from 22   to 23   Changed ForceHarpedStrandsStraight to AdjustableStrandType for more flexibility
// from 23   to 24   Added Prestressed design strategies (2.9), Added option for variable depth girders (3.0)
// from 24   to 25   Added Publisher Contact Information
// from 25   to 26   Added haunch checking and camber multiplication factors
// from 26   to 27   Added drag coefficient
// from 27 to 28, added precamber limit
// from 28 to 29, added option to print bearing elevations at girder edges
#define CURRENT_VERSION 29.0


// Initialize static class members
bool GirderLibraryEntry::m_bsInitCLSIDMap = true;
std::map<std::_tstring, std::_tstring> GirderLibraryEntry::m_CLSIDMap;
std::vector<CComPtr<IBeamFactoryCLSIDTranslator>> GirderLibraryEntry::ms_ExternalCLSIDTranslators;

// predicate function for comparing doubles
inline bool EqualDoublePred(Float64 i, Float64 j) 
{
   return ::IsEqual(i,j);
}

inline bool CompareDimensions(const GirderLibraryEntry::Dimension& d1,const GirderLibraryEntry::Dimension& d2)
{
   return (d1.first == d2.first && ::IsEqual(d1.second,d2.second));
}


////////////////////////// PUBLIC     ///////////////////////////////////////

// NOTE: This collection must be emptied before the DLL is unloaded. In order to
// do this we have to purposely call gs_ClassFactores.clear(). Since an individual
// GirderLibraryEntry has no way of knowing it is the last one to be released,
// we turn to the library manager. If the library manager is going out of scope, then
// there shouldn't be any library entires left. ms_ClassFactories is cleared
// in psgLibraryManager's destructor.
GirderLibraryEntry::ClassFactoryCollection GirderLibraryEntry::ms_ClassFactories;

CString GirderLibraryEntry::GetAdjustableStrandType(pgsTypes::AdjustableStrandType strandType)
{
   LPCTSTR lpszStrandType;
   switch(strandType)
   {
   case pgsTypes::asHarped:
      lpszStrandType = _T("Adjustable Strands are Harped");
      break;

   case pgsTypes::asStraight:
      lpszStrandType = _T("Adjustable Strands are Straight");
      break;

   case pgsTypes::asStraightOrHarped:
      lpszStrandType = _T("Adjustable can be Harped or Straight");
      break;

   default:
      ATLASSERT(false); // should never get here
   }
   return lpszStrandType;
}

/****************************************************************************
CLASS
   GirderLibraryEntry
****************************************************************************/
//======================== LIFECYCLE  =======================================
GirderLibraryEntry::GirderLibraryEntry(CreateType createType) :
m_bUseDifferentHarpedGridAtEnds(true),
m_HarpingPointLocation(0.25),
m_HarpPointMeasure(mtFractionOfGirderLength),
m_bMinHarpingPointLocation(false),
m_MinHarpingPointLocation(WBFL::Units::ConvertToSysUnits(5.0,WBFL::Units::Measure::Feet)),
m_HarpPointReference(mlBearing),
m_LongitudinalBarType(WBFL::Materials::Rebar::Type::A615),
m_LongitudinalBarGrade(WBFL::Materials::Rebar::Grade::Grade60),
m_bOddNumberOfHarpedStrands(true),
m_AdjustableStrandType(pgsTypes::asHarped), // Adjustable strand type - harp was only option before 21
// debonding criteria
m_bCheckMaxDebondStrands(false),
m_MaxDebondStrands(0.25),
m_MaxDebondStrandsPerRow(0.45), // changed from 40% to 45% in LRFD 9th Edition
m_MaxNumDebondedStrandsPerSection10orLess(4),
m_MaxNumDebondedStrandsPerSection(6),
m_bCheckMaxNumDebondedStrandsPerSection(false),
m_MaxDebondedStrandsPerSection(0.40),
m_MinDebondLengthDB(60),
m_bCheckMinDebondLength(false),
m_MinDebondLength(WBFL::Units::ConvertToSysUnits(3.0,WBFL::Units::Measure::Feet)), // not aashto, but reasonable
m_bCheckDebondingSymmetry(true),
m_bCheckAdjacentDebonding(true),
m_bCheckDebondingInWebWidthProjections(true),
m_MaxDebondLengthBySpanFraction(0.20), //  LRFD 9th Edition, default to 20%
m_MaxDebondLengthByHardDistance(-1.0),
m_MinFilletValue(WBFL::Units::ConvertToSysUnits(0.75,WBFL::Units::Measure::Inch)),
m_DoCheckMinHaunchAtBearingLines(false),
m_MinHaunchAtBearingLines(0.0),
m_ExcessiveSlabOffsetWarningTolerance(WBFL::Units::ConvertToSysUnits(0.25,WBFL::Units::Measure::Inch)),
m_DragCoefficient(2.2),
m_PrecamberLimit(80),
m_DoReportBearingElevationsAtGirderEdges(false),
m_pCompatibilityData(nullptr)
{
	CWaitCursor cursor;

   m_bSupportsVariableDepthSection = false;
   m_bIsVariableDepthSectionEnabled = false;


   // When the user creates a new library entry, it needs a beam type. The beam type
   // is defined by the beam factory this object holds. Therefore we need to create a
   // beam factory. Since we don't what kind of beam the user wants, use the first 
   // one registered as a default
   CComPtr<IBeamFactory> beam_factory;
   if ( createType == DEFAULT || createType == PRECAST )
   {
      beam_factory.CoCreateInstance(CLSID_WFBeamFactory);
   }
   else if ( createType == SPLICED )
   {
      beam_factory.CoCreateInstance(CLSID_SplicedIBeamFactory);
   }
   else
   {
      ATLASSERT(false); // is there a new create type?
   }

   if ( beam_factory == nullptr )
   {
	   std::vector<CString> familyNames;
	   if ( createType == DEFAULT )
	   {
	      familyNames = CBeamFamilyManager::GetBeamFamilyNames();
	   }
	   else if ( createType == PRECAST )
	   {
	      familyNames = CBeamFamilyManager::GetBeamFamilyNames(CATID_PGSuperBeamFamily);
	   }
	   else if ( createType == SPLICED )
	   {
	      familyNames = CBeamFamilyManager::GetBeamFamilyNames(CATID_PGSpliceBeamFamily);
	   }
	   else
	   {
	      ATLASSERT(false); // is there a new create type?
	   }
	   ATLASSERT(0 < familyNames.size());
	   CComPtr<IBeamFamily> beamFamily;
	   HRESULT hr = CBeamFamilyManager::GetBeamFamily(familyNames.front(),&beamFamily);
	   if ( FAILED(hr) )
	   {
	      return;
	   }
	
	   std::vector<CString> factoryNames = beamFamily->GetFactoryNames();
	   ATLASSERT(0 < factoryNames.size());
	   beamFamily->CreateFactory(factoryNames.front(),&beam_factory);
   }

   SetBeamFactory(beam_factory);
   // m_pBeamFactory is nullptr if we were unable to create a beam factory

   // Shear design

   // Set some defaults for shear design container values
   // Bar combo 2-#4's, 2-#5's and 2-#6's
   StirrupSizeBarCombo cbo;
   cbo.Size = WBFL::Materials::Rebar::Size::bs4;
   cbo.NLegs = 2.0;
   m_StirrupSizeBarComboColl.push_back(cbo);
   cbo.Size = WBFL::Materials::Rebar::Size::bs5;
   m_StirrupSizeBarComboColl.push_back(cbo);
   cbo.Size = WBFL::Materials::Rebar::Size::bs6;
   m_StirrupSizeBarComboColl.push_back(cbo);

   // Spacings
   m_AvailableBarSpacings.push_back(WBFL::Units::ConvertToSysUnits(3.0, WBFL::Units::Measure::Inch));
   m_AvailableBarSpacings.push_back(WBFL::Units::ConvertToSysUnits(4.0, WBFL::Units::Measure::Inch));
   m_AvailableBarSpacings.push_back(WBFL::Units::ConvertToSysUnits(6.0, WBFL::Units::Measure::Inch));
   m_AvailableBarSpacings.push_back(WBFL::Units::ConvertToSysUnits(9.0, WBFL::Units::Measure::Inch));
   m_AvailableBarSpacings.push_back(WBFL::Units::ConvertToSysUnits(12.0, WBFL::Units::Measure::Inch));
   m_AvailableBarSpacings.push_back(WBFL::Units::ConvertToSysUnits(18.0, WBFL::Units::Measure::Inch));

   m_MaxSpacingChangeInZone          = WBFL::Units::ConvertToSysUnits(6.0,WBFL::Units::Measure::Inch);
   m_MaxShearCapacityChangeInZone    = 0.50;
   m_MinZoneLengthSpacings           = 3;
   m_MinZoneLengthLength             = WBFL::Units::ConvertToSysUnits(12.0,WBFL::Units::Measure::Inch);
   m_DoExtendBarsIntoDeck            = true;
   m_DoBarsActAsConfinement          = true;
   m_LongShearCapacityIncreaseMethod   = isAddingRebar;
   m_InterfaceShearWidthReduction = 0.0;
   
   InitCLSIDMap();

   SetDefaultPrestressDesignStrategy();
}

GirderLibraryEntry::GirderLibraryEntry(const GirderLibraryEntry& rOther) :
WBFL::Library::LibraryEntry(rOther)
{
   m_pCompatibilityData = nullptr;
   if (rOther.m_pCompatibilityData != nullptr)
   {
      m_pCompatibilityData = new pgsCompatibilityData(rOther.m_pCompatibilityData);
   }
   InitCLSIDMap();
   CopyValuesAndAttributes(rOther);
}

GirderLibraryEntry::~GirderLibraryEntry()
{
   if (m_pCompatibilityData)
   {
      delete m_pCompatibilityData;
      m_pCompatibilityData = nullptr;
   }
}

void GirderLibraryEntry::InitCLSIDMap()
{
   // Maps the old PGSuper CLSID for the beam factories to the new CLSIDs
   // This allows us to open old files
   if (m_bsInitCLSIDMap)
   {
                                                      // Old CLSID (Before Version 3.0)                New CLSID (Version 3.0 and later)
      m_CLSIDMap.emplace(                      _T("{AC828108-B982-4C95-867B-B4BF4E37B7EB}"), _T("{2583C7C1-FF57-4113-B45B-702CFA6AD013}"));
      m_CLSIDMap.emplace_hint(m_CLSIDMap.end(),_T("{A9FC0D19-F6A4-4A9D-835B-FEE2E42F574C}"), _T("{171FC948-C4CB-4920-B9FC-72D729F3E91A}"));
      m_CLSIDMap.emplace_hint(m_CLSIDMap.end(),_T("{C28FA58A-3D4D-4DF1-8021-2A0BBA90F304}"), _T("{DD999E90-E181-4EC8-BB82-FDFB364B7620}"));
      m_CLSIDMap.emplace_hint(m_CLSIDMap.end(),_T("{F008218C-B19B-49A0-9084-253A09D4EA5A}"), _T("{F72BA192-6D82-4D66-92CA-EB13466E6BA9}"));
      m_CLSIDMap.emplace_hint(m_CLSIDMap.end(),_T("{F1504E79-8810-4B5C-9797-BCE6C1022C4C}"), _T("{DA3C413D-6413-4485-BD29-E8A419E981AF}"));
      m_CLSIDMap.emplace_hint(m_CLSIDMap.end(),_T("{FD766540-C635-43E1-B809-06B837EFA752}"), _T("{A367FEEC-9E24-4ED6-9DD3-57F62AD752C9}"));
      m_CLSIDMap.emplace_hint(m_CLSIDMap.end(),_T("{0CE7D624-7CC5-4300-8257-0C41A585852F}"), _T("{51CB9247-8200-43E5-BFB0-E386C1E6A0D0}"));
      m_CLSIDMap.emplace_hint(m_CLSIDMap.end(),_T("{09D3E8EA-B66F-4D99-8DDE-3F55BBC14D54}"), _T("{B1F474E4-15F7-4AC8-AD2D-7FBD12E3B0EB}"));
      m_CLSIDMap.emplace_hint(m_CLSIDMap.end(),_T("{30962206-2412-4001-AA20-CF359BC60142}"), _T("{EF144A97-4C75-4234-AF3C-71DC89B1C8F8}"));
      m_CLSIDMap.emplace_hint(m_CLSIDMap.end(),_T("{59BAD0A2-91F0-4E8A-8A90-4241787E9B50}"), _T("{5D6AFD91-84F4-4755-9AF7-B760114A4551}"));
      m_CLSIDMap.emplace_hint(m_CLSIDMap.end(),_T("{63C3AE0C-ADD3-44F7-AC22-16303D00E303}"), _T("{E6F28403-A396-453B-91BE-15A68D194255}"));
      m_CLSIDMap.emplace_hint(m_CLSIDMap.end(),_T("{64E8DD89-EC9E-48DD-B4E9-A457F6BFB9B1}"), _T("{9EDBDD8D-ABBB-413A-9B2D-9EB2712BE914}"));
      m_CLSIDMap.emplace_hint(m_CLSIDMap.end(),_T("{71AF6935-2501-4FB7-8FA3-7DDB505D5C63}"), _T("{D8824656-6CB6-45C0-B5F7-13CFFA890F0B}"));
      m_CLSIDMap.emplace_hint(m_CLSIDMap.end(),_T("{73D5A32E-F747-4FFF-8B48-D561A93494AC}"), _T("{B195CB90-CEB0-4495-B91D-7FC6DFBF31CF}"));
      m_CLSIDMap.emplace_hint(m_CLSIDMap.end(),_T("{9AA6D6FF-2D05-46DB-B3F0-479B182B3193}"), _T("{16CC6372-256E-4828-9BDA-3185C27DC65E}"));
      m_CLSIDMap.emplace_hint(m_CLSIDMap.end(),_T("{9C219793-A1F1-402A-B865-0AA6BD22B0A6}"), _T("{DEFA27AD-3D22-481B-9006-627C65D2648F}"));

      // Create external beam factory publisher CLSID translators
      CComPtr<ICatRegister> pICatReg = 0;
      HRESULT hr;
      hr = ::CoCreateInstance(CLSID_StdComponentCategoriesMgr,
         nullptr,
         CLSCTX_INPROC_SERVER,
         IID_ICatRegister,
         (void**)&pICatReg);
      if (FAILED(hr))
      {
         CString msg;
         msg.Format(_T("Failed to create Component Category Manager. hr = %d\nIs the correct version of Internet Explorer installed"), hr);
         AfxMessageBox(msg, MB_OK | MB_ICONWARNING);
         return;
      }

      CComPtr<ICatInformation> pICatInfo;
      pICatReg->QueryInterface(IID_ICatInformation, (void**)&pICatInfo);
      CComPtr<IEnumCLSID> pIEnumCLSID;

      const CATID catid[1]{ (CATID)CATID_BeamFactoryCLSIDTranslator };
      hr = pICatInfo->EnumClassesOfCategories(1, catid, 0, nullptr, &pIEnumCLSID);

      CLSID clsid;
      ULONG nFetched;
      while (pIEnumCLSID->Next(1, &clsid, &nFetched) != S_FALSE)
      {
         CComPtr<IBeamFactoryCLSIDTranslator> translator;
         hr = ::CoCreateInstance(clsid, nullptr, CLSCTX_ALL, IID_IBeamFactoryCLSIDTranslator, (void**)&translator);
         if (SUCCEEDED(hr))
         {
            ms_ExternalCLSIDTranslators.push_back(translator);
         }
      }

      m_bsInitCLSIDMap = false; // false = it is initialized
   }
}

//======================== OPERATORS  =======================================
GirderLibraryEntry& GirderLibraryEntry::operator= (const GirderLibraryEntry& rOther)
{
   if( this != &rOther )
   {
      CopyValuesAndAttributes(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================

bool GirderLibraryEntry::SaveMe(WBFL::System::IStructuredSave* pSave)
{
   USES_CONVERSION;
   pSave->BeginUnit(_T("GirderLibraryEntry"), CURRENT_VERSION);
   pSave->Property(_T("Name"),this->GetName().c_str());

   LPOLESTR pszCLSID;
   ::StringFromCLSID(m_pBeamFactory->GetCLSID(),&pszCLSID);
   pSave->Property(_T("CLSID"),OLE2T(pszCLSID));
   ::CoTaskMemFree((void*)pszCLSID);

   pSave->Property(_T("Publisher"),m_pBeamFactory->GetPublisher().c_str());
   pSave->Property(_T("ContactInfo"),m_pBeamFactory->GetPublisherContactInformation().c_str()); // added in version 25

   LPOLESTR pszUserType;
   OleRegGetUserType(m_pBeamFactory->GetCLSID(),USERCLASSTYPE_SHORT,&pszUserType);
   pSave->Property(_T("SectionName"),CString(pszUserType));

   // added in version 24
   pSave->BeginUnit(_T("SectionDimensions"),1.0);
      if ( m_bSupportsVariableDepthSection )
      {
         pSave->Property(_T("VariableDepthSection"),m_bIsVariableDepthSectionEnabled);
      }
      m_pBeamFactory->SaveSectionDimensions(pSave,m_Dimensions);
   pSave->EndUnit(); // SectionDimensions

   // added version 16, removed version 17
   //pSave->Property(_T("CanPostTension"), m_bCanPostTension );

   pSave->Property(_T("UseDifferentHarpedGridAtEnds"),  m_bUseDifferentHarpedGridAtEnds);

   pSave->Property(_T("EndAllowVertAdjustment"),   m_EndAdjustment.m_AllowVertAdjustment);
   pSave->Property(_T("EndStrandIncrement"),       m_EndAdjustment.m_StrandIncrement);
   pSave->Property(_T("EndBottomFace"),            (long)m_EndAdjustment.m_BottomFace);
   pSave->Property(_T("EndBottomLimit"),           m_EndAdjustment.m_BottomLimit);
   pSave->Property(_T("EndTopFace"),               (long)m_EndAdjustment.m_TopFace);
   pSave->Property(_T("EndTopLimit"),              m_EndAdjustment.m_TopLimit);

   pSave->Property(_T("HPAllowVertAdjustment"),   m_HPAdjustment.m_AllowVertAdjustment);
   pSave->Property(_T("HPStrandIncrement"),       m_HPAdjustment.m_StrandIncrement);
   pSave->Property(_T("HPBottomFace"),            (long)m_HPAdjustment.m_BottomFace);
   pSave->Property(_T("HPBottomLimit"),           m_HPAdjustment.m_BottomLimit);
   pSave->Property(_T("HPTopFace"),               (long)m_HPAdjustment.m_TopFace);
   pSave->Property(_T("HPTopLimit"),              m_HPAdjustment.m_TopLimit);

   // added in version 23
   pSave->Property(_T("StraightAllowVertAdjustment"),   m_StraightAdjustment.m_AllowVertAdjustment);
   pSave->Property(_T("StraightStrandIncrement"),       m_StraightAdjustment.m_StrandIncrement);
   pSave->Property(_T("StraightBottomFace"),            (long)m_StraightAdjustment.m_BottomFace);
   pSave->Property(_T("StraightBottomLimit"),           m_StraightAdjustment.m_BottomLimit);
   pSave->Property(_T("StraightTopFace"),               (long)m_StraightAdjustment.m_TopFace);
   pSave->Property(_T("StraightTopLimit"),              m_StraightAdjustment.m_TopLimit);

   //  Removed below in version 19
   // pSave->Property(_T("ConfinementBarSize"),        (long)m_ConfinementBarSize); // data type and property name changed in version 18
   // pSave->Property(_T("LastConfinementZone"),      m_LastConfinementZone);
   pSave->Property(_T("HarpingPointLocation"),     m_HarpingPointLocation);
   pSave->Property(_T("UseMinHarpingPointLocation"),m_bMinHarpingPointLocation);

   if ( m_bMinHarpingPointLocation )
   {
      pSave->Property(_T("MinHarpingPointLocation"),m_MinHarpingPointLocation);
   }

   pSave->Property(_T("HarpingPointReference"),    (long)m_HarpPointReference);
   pSave->Property(_T("HarpingPointMeasure"),      (long)m_HarpPointMeasure); // added in version 4
   //pSave->Property(_T("ShearSteelMaterial"),       m_ShearSteelMaterial.c_str()); // removed in version 18
   //pSave->Property(_T("LongSteelMaterial"),        m_LongSteelMaterial.c_str()); // removed in version 18
   // removed below in version 19
   //pSave->Property(_T("DoStirrupsEngageDeck"),     m_bStirrupsEngageDeck);
   //pSave->Property(_T("IsRoughenedSurface"),       m_bIsRoughenedSurface); // added in version 17
   //pSave->Property(_T("TopFlangeShearBarSize"),    (long)m_TopFlangeShearBarSize); // data type changed in version 18
   //pSave->Property(_T("TopFlangeShearBarSpacing"), m_TopFlangeShearBarSpacing);

   // debond criteria - added in version 13
   pSave->BeginUnit(_T("DebondingCritia"), 2.0);
   pSave->Property(_T("CheckMaxDebondedStrands"), m_bCheckMaxDebondStrands); // added in version 2 of this data block
   pSave->Property(_T("MaxDebondStrands"),               m_MaxDebondStrands);
   pSave->Property(_T("MaxDebondStrandsPerRow"),         m_MaxDebondStrandsPerRow);
   pSave->Property(_T("MaxNumDebondedStrandsPerSection10orLess"), m_MaxNumDebondedStrandsPerSection10orLess); // added in version 2
   pSave->Property(_T("MaxNumDebondedStrandsPerSection"),m_MaxNumDebondedStrandsPerSection);
   pSave->Property(_T("CheckMaxDebondedStrandsPerSection"), m_bCheckMaxNumDebondedStrandsPerSection); // added in version 2
   pSave->Property(_T("MaxDebondedStrandsPerSection"),   m_MaxDebondedStrandsPerSection);
   pSave->Property(_T("MinDebondLengthDB"), m_MinDebondLengthDB); // added in version 2
   pSave->Property(_T("CheckMinDebondLength"), m_bCheckMinDebondLength); // added in version 2
   pSave->Property(_T("MinDebondLength"),                m_MinDebondLength);
   //pSave->Property(_T("DefaultDebondLength"),            m_DefaultDebondLength); // removed in version 2 of this data block
   pSave->Property(_T("CheckDebondingSymmetry"), m_bCheckDebondingSymmetry); // added in version 2
   pSave->Property(_T("CheckAdjacentDebonding"), m_bCheckAdjacentDebonding); // added in version 2
   pSave->Property(_T("CheckDebondingInWebWidthProjections"), m_bCheckDebondingInWebWidthProjections); // added in version 2
   pSave->Property(_T("MaxDebondLengthBySpanFraction"),  m_MaxDebondLengthBySpanFraction);
   pSave->Property(_T("MaxDebondLengthByHardDistance"),  m_MaxDebondLengthByHardDistance);

   pSave->EndUnit(); // DebondingCritia

   // Added start and end strand grids in version 15 of parent unit
   pSave->BeginUnit(_T("StraightStrands"),1.0);
   StraightStrandIterator strandIter;
   for (StraightStrandIterator strandIter = m_StraightStrands.begin(); strandIter != m_StraightStrands.end(); strandIter++)
   {
      pSave->BeginUnit(_T("StrandLocation"), 1.0);

      StraightStrandLocation& strandLocation = *strandIter;

      pSave->Property(_T("Xstart"), strandLocation.m_Xstart);
      pSave->Property(_T("Ystart"), strandLocation.m_Ystart);
      pSave->Property(_T("Xend"),   strandLocation.m_Xend);
      pSave->Property(_T("Yend"),   strandLocation.m_Yend);
      pSave->Property(_T("CanDebond"), strandLocation.m_bCanDebond);

      pSave->EndUnit(); // StrandLocation
   }
   pSave->EndUnit(); // StraightStrands

   pSave->BeginUnit(_T("HarpedStrands"),1.0);
   // harped strands - consolidated end and hp locations in version 4.0
   for (HarpedStrandIterator iter = m_HarpedStrands.begin(); iter != m_HarpedStrands.end(); iter++)
   {
      pSave->BeginUnit(_T("StrandLocation"), 1.0);

      HarpedStrandLocation& strandLocation = *iter;

      pSave->Property(_T("Xstart"), strandLocation.m_Xstart);
      pSave->Property(_T("Ystart"), strandLocation.m_Ystart);
      pSave->Property(_T("Xhp"),    strandLocation.m_Xhp);
      pSave->Property(_T("Yhp"),    strandLocation.m_Yhp);
      pSave->Property(_T("Xend"),   strandLocation.m_Xend);
      pSave->Property(_T("Yend"),   strandLocation.m_Yend);

      pSave->EndUnit();
   }
   pSave->EndUnit();

   pSave->Property(_T("OddNumberOfHarpedStrands"),m_bOddNumberOfHarpedStrands);

   //pSave->Property(_T("ForceHarpedStrandsStraight"),m_bForceHarpedStrandsStraight); // removed in version 23
   pSave->Property(_T("AdjustableStrandType"),(Int32)m_AdjustableStrandType); // added in version 23

   // Temporary strands
   // Added start and end strand grids in version 15 of parent unit
   pSave->BeginUnit(_T("TemporaryStrands"),1.0);
   for (StraightStrandIterator strandIter = m_TemporaryStrands.begin(); strandIter != m_TemporaryStrands.end(); strandIter++)
   {
      pSave->BeginUnit(_T("StrandLocation"), 1.0);

      StraightStrandLocation& strandLocation = *strandIter;

      pSave->Property(_T("Xstart"), strandLocation.m_Xstart);
      pSave->Property(_T("Ystart"), strandLocation.m_Ystart);
      pSave->Property(_T("Xend"),   strandLocation.m_Xend);
      pSave->Property(_T("Yend"),   strandLocation.m_Yend);

      //pSave->Property(_T("CanDebond"), strandLocation.m_bCanDebond);
      // debond parameter is ignored for temporary strands

      pSave->EndUnit(); // StrandLocation
   }
   pSave->EndUnit(); // TemporaryStrands

   // strand ordering
   // added in version 5.0
   // data types were re-named to better define their purpose. The data block
   // changes were not changed to keep data compatibility
   PermanentStrandCollection::iterator permStrandIter(m_PermanentStrands.begin());
   PermanentStrandCollection::iterator permStrandIterEnd(m_PermanentStrands.end());
   for ( ; permStrandIter != permStrandIterEnd; permStrandIter++ )
   {
      pSave->BeginUnit(_T("GlobalStrandOrder"), 1.0);

      psStrandType type = permStrandIter->m_StrandType;

      if (type == stStraight)
      {
        pSave->Property(_T("StrandType"), _T("Straight"));
      }
      else if (type == stAdjustable)
      {
        pSave->Property(_T("StrandType"), _T("Harped"));
      }
      else
      {
         ATLASSERT(false);
      }

      pSave->Property(_T("LocalSortOrder"), permStrandIter->m_GridIdx);

      pSave->EndUnit();

   }

   // Shear data
   m_ShearData.Save(pSave); // added in version 19

   // All below removed in version 19
   //pSave->Property(_T("ShearSteelBarType"),        (long)m_StirrupBarType); // added in version 18
   //pSave->Property(_T("ShearSteelBarGrade"),       (long)m_StirrupBarGrade); // added in version 18
   //for (ShearZoneInfoVec::const_iterator its = m_ShearZoneInfo.begin(); its!=m_ShearZoneInfo.end(); its++)
   //{
   //   pSave->BeginUnit(_T("ShearZones"), 3.0); // changed to version 3.0 because VertBarSize and HorzBarSize have changed

   //   pSave->Property(_T("ZoneLength"),  (*its).ZoneLength);
   //   pSave->Property(_T("Spacing"),     (*its).StirrupSpacing);
   //   pSave->Property(_T("VertBarSize"), (Int32)(*its).VertBarSize);
   //   pSave->Property(_T("VertBars"),    (*its).nVertBars);
   //   pSave->Property(_T("HorzBarSize"), (Int32)(*its).HorzBarSize);
   //   pSave->Property(_T("HorzBars"),    (*its).nHorzBars);

   //   pSave->EndUnit();
   //}

   // Longitudinal Steel rows
   pSave->Property(_T("LongitudinalBarType"),    (long)m_LongitudinalBarType); // added in version 18
   pSave->Property(_T("LongitudinalBarGrade"),   (long)m_LongitudinalBarGrade); // added in version 18
   for (LongSteelInfoVec::const_iterator itl = m_LongSteelInfo.begin(); itl!=m_LongSteelInfo.end(); itl++)
   {
      pSave->BeginUnit(_T("LongSteelInfo"), 3.0); // From 2-3.0 in version 22.0.

      pSave->Property(_T("BarLayout"), Int32((*itl).BarLayout)); // These 3 added in version 22.0
      pSave->Property(_T("DistFromEnd"), (*itl).DistFromEnd);
      pSave->Property(_T("BarLength"), (*itl).BarLength);

      if (itl->Face==pgsTypes::BottomFace)
      {
         pSave->Property(_T("Face"), _T("Bottom"));
      }
      else
      {
         pSave->Property(_T("Face"), _T("Top"));
      }

      pSave->Property(_T("NumberOfBars"), (*itl).NumberOfBars);
      pSave->Property(_T("BarSize"),  (Int32)(*itl).BarSize);
      pSave->Property(_T("BarCover"), (*itl).Cover);
      pSave->Property(_T("BarSpacing"), (*itl).BarSpacing);

      pSave->EndUnit();
   }

   for ( const auto& dlr : m_DiaphragmLayoutRules)
   {
      pSave->BeginUnit(_T("DiaphragmLayoutRule"),2.0);

      pSave->Property(_T("Description"),dlr.Description.c_str());
      pSave->Property(_T("MinSpan"),dlr.MinSpan);
      pSave->Property(_T("MaxSpan"),dlr.MaxSpan);
      pSave->Property(_T("Method"),(long)dlr.Method);
      if ( dlr.Method == dwmCompute )
      {
         pSave->Property(_T("Height"),dlr.Height);
         pSave->Property(_T("Thickness"),dlr.Thickness);
      }
      else
      {
         pSave->Property(_T("Weight"),dlr.Weight);
      }

      pSave->Property(_T("DiaphragmType"),(long)dlr.Type);
      pSave->Property(_T("ConstructionType"),(long)dlr.Construction);
      pSave->Property(_T("MeasurementType"),(long)dlr.MeasureType);
      pSave->Property(_T("MeasurmentLocation"),(long)dlr.MeasureLocation);
      pSave->Property(_T("Location"),dlr.Location);

      pSave->EndUnit();
   }


   // Added in version 20 - shear design data
   pSave->BeginUnit(_T("ShearDesign"),2.0);
   {
      pSave->Property(_T("StirrupSizeBarComboCollSize"),(long)m_StirrupSizeBarComboColl.size());
      pSave->BeginUnit(_T("StirrupSizeBarComboColl"),1.0);
         for(const auto& stirrup_size : m_StirrupSizeBarComboColl)
         {
            pSave->Property(_T("BarSize"),(long)stirrup_size.Size);
            pSave->Property(_T("NLegs"),(long)stirrup_size.NLegs);

         }
      pSave->EndUnit(); // StirrupSizeBarComboColl

      pSave->Property(_T("NumAvailableBarSpacings"),(long)m_AvailableBarSpacings.size());
      pSave->BeginUnit(_T("AvailableBarSpacings"),1.0);
         for(const auto& spacing : m_AvailableBarSpacings)
         {
               pSave->Property(_T("Spacing"), spacing);
         }
      pSave->EndUnit(); // AvailableBarSpacings

      pSave->Property(_T("MaxSpacingChangeInZone"), m_MaxSpacingChangeInZone);
      pSave->Property(_T("MaxShearCapacityChangeInZone"), m_MaxShearCapacityChangeInZone);
      pSave->Property(_T("MinZoneLengthSpacings"), m_MinZoneLengthSpacings);
      pSave->Property(_T("MinZoneLengthLength"), m_MinZoneLengthLength);
      pSave->Property(_T("IsTopFlangeRoughened"), true); // NOTE: This value was removed after beta testing was started. Keep value for reverse compatibility
      pSave->Property(_T("DoExtendBarsIntoDeck"), m_DoExtendBarsIntoDeck);
      pSave->Property(_T("DoBarsProvideSplittingCapacity"), true); // Same note as IsTopFlangeRoughened above
      pSave->Property(_T("DoBarsActAsConfinement"), m_DoBarsActAsConfinement);
      pSave->Property(_T("LongShearCapacityIncreaseMethod"), (long)m_LongShearCapacityIncreaseMethod);
      pSave->Property(_T("InterfaceShearWidthReduction"), m_InterfaceShearWidthReduction); // added in version 2 of ShearDesign data block
   }
   pSave->EndUnit(); // ShearDesign


   // PrestressDesignStrategies added in version 24
   pSave->BeginUnit(_T("PrestressDesignStrategies"),1.0);
   for (PrestressDesignStrategyIterator iter = m_PrestressDesignStrategies.begin(); iter != m_PrestressDesignStrategies.end(); iter++)
   {
      pSave->BeginUnit(_T("PrestressDesignStrategy"), 1.0);

      PrestressDesignStrategy& strategy = *iter;

      pSave->Property(_T("FlexuralDesignType"), strategy.m_FlexuralDesignType);
      pSave->Property(_T("MaxFc"), strategy.m_MaxFc);
      pSave->Property(_T("MaxFci"), strategy.m_MaxFci);

      pSave->EndUnit();
   }
   pSave->EndUnit(); // PrestressDesignStrategies

   // Haunch and camber stuff added in version 26
   pSave->Property(_T("MinFilletValue"), m_MinFilletValue);
   pSave->Property(_T("DoCheckMinHaunchAtBearingLines"), m_DoCheckMinHaunchAtBearingLines);
   pSave->Property(_T("MinHaunchAtBearingLines"), m_MinHaunchAtBearingLines);
   pSave->Property(_T("ExcessiveSlabOffsetWarningTolerance"), m_ExcessiveSlabOffsetWarningTolerance);

   pSave->BeginUnit(_T("CamberMultipliers"),1.0);
   {
      pSave->Property(_T("ErectionFactor"), m_CamberMultipliers.ErectionFactor);
      pSave->Property(_T("CreepFactor"), m_CamberMultipliers.CreepFactor);
      pSave->Property(_T("DiaphragmFactor"), m_CamberMultipliers.DiaphragmFactor);
      pSave->Property(_T("DeckPanelFactor"), m_CamberMultipliers.DeckPanelFactor);
      pSave->Property(_T("SlabUser1Factor"), m_CamberMultipliers.SlabUser1Factor);
      pSave->Property(_T("HaunchLoadFactor"), m_CamberMultipliers.SlabPadLoadFactor);
      pSave->Property(_T("BarrierSwOverlayUser2Factor"), m_CamberMultipliers.BarrierSwOverlayUser2Factor);
   }
   pSave->EndUnit(); // CamberMultipliers

   pSave->Property(_T("DragCoefficient"),m_DragCoefficient); // added version 27
   pSave->Property(_T("PrecamberLimit"), m_PrecamberLimit); // added version 28
   pSave->Property(_T("DoReportBearingElevationsAtGirderEdges"), m_DoReportBearingElevationsAtGirderEdges); // added version 29

   pSave->EndUnit();

   return false;
}

std::_tstring GirderLibraryEntry::TranslateCLSID(const std::_tstring& strCLSID)
{
   // check our map first
   std::map<std::_tstring,std::_tstring>::iterator found(m_CLSIDMap.find(strCLSID));
   if ( found != m_CLSIDMap.end() )
   {
      return found->second;
   }

   // check third party translaters
   for (const auto& translator : ms_ExternalCLSIDTranslators)
   {
      LPCTSTR newCLSID;
      if ( translator->TranslateCLSID(strCLSID.c_str(),&newCLSID) )
      {
         return std::_tstring(newCLSID);
      }
   }

   // no translation availble... just return the original CLSID
   return strCLSID;
}

bool GirderLibraryEntry::LoadMe(WBFL::System::IStructuredLoad* pLoad)
{
   USES_CONVERSION;

   if(pLoad->BeginUnit(_T("GirderLibraryEntry")))
   {
      Float64 version = pLoad->GetVersion();
      if ( version > CURRENT_VERSION)
      {
         THROW_LOAD(BadVersion,pLoad);
      }

      std::_tstring name;
      if( pLoad->Property(_T("Name"),&name) )
      {
         this->SetName(name.c_str());
      }
      else
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      m_Dimensions.clear();
      if ( version <= 1.1 )
      {
         // version 1.1 and ealier everything was an I-Beam
         LoadIBeamDimensions(pLoad);

#if defined _DEBUG
         const std::vector<std::_tstring>& vDimNames = m_pBeamFactory->GetDimensionNames();
         ATLASSERT( vDimNames.size() == m_Dimensions.size() );
         // if this assert fires, new beam dimensions have been added and not accounted for
#endif
      }
      else
      {
         // Version 1.2 or greater... dictionary based defintion of girder dimensions
         std::_tstring strCLSID;
         if (!pLoad->Property(_T("CLSID"),&strCLSID) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         make_upper(strCLSID.begin(),strCLSID.end());

         std::_tstring strPublisher;
         if ( !pLoad->Property(_T("Publisher"),&strPublisher) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         std::_tstring strPublisherContactInfo;
         if ( 24 < version )
         {
            if ( !pLoad->Property(_T("ContactInfo"),&strPublisherContactInfo) )
            {
              THROW_LOAD(InvalidFileFormat,pLoad);
            }
         }
         else
         {
            strPublisherContactInfo = _T("");
         }

         std::_tstring strSectionName;
         if ( !pLoad->Property(_T("SectionName"),&strSectionName) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         std::_tstring strNewCLSID = TranslateCLSID(strCLSID);

         HRESULT hr = CreateBeamFactory(strNewCLSID);
         if ( FAILED(hr))
         {
            LPTSTR errorMsgBuffer = nullptr;
            size_t size = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                          NULL, hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&errorMsgBuffer, 0, NULL);

            CString strMsg;
            strMsg.Format(_T("Unable to create the \"%s\" section, published by %s.\n\nError code %#x\n%s\nContact the publisher for assistance.\n%s"), strSectionName.c_str(), strPublisher.c_str(), hr, errorMsgBuffer, strPublisherContactInfo.c_str());

            WBFL::System::XStructuredLoad ex(WBFL::System::XStructuredLoad::UserDefined,_T(__FILE__),__LINE__);
            ex.SetExtendedMessage(strMsg);

            LocalFree(errorMsgBuffer);

            ex.Throw();
         }

         ATLASSERT(m_pBeamFactory != nullptr);


         CComQIPtr<ISplicedBeamFactory,&IID_ISplicedBeamFactory> splicedBeamFactory(m_pBeamFactory);
         if ( splicedBeamFactory )
         {
            m_bSupportsVariableDepthSection = splicedBeamFactory->SupportsVariableDepthSection();
         }
         else
         {
            m_bSupportsVariableDepthSection = false;
         }

         // this data block is "officially" added in version 24 however, this was originally part
         // of the version 23 data block during pgsplice development. PGSuper version 2.9
         // was updated and used version 23. This creates a problem. When loading a PGSuper
         // file from version 2.9 we are going to encounter version 23 or 24 of this datablock however
         // it wont have the expected data. To work around this problem, let it fail
         // and move on.
         if ( 22 < version )
         {
            bool bOkToFail = false;
            if ( version == 23 || version == 24  || version == 25 )
            {
               bOkToFail = true; // it is OK if version 23, 24, or 25 datablock fails
            }

            bool bDidBeginUnit = true;
            if ( !pLoad->BeginUnit(_T("SectionDimensions")) )
            {
               bDidBeginUnit = false;
            }

            if ( bDidBeginUnit )
            {
               // BeginUnit succeeded so read the rest of the data block
               if ( m_bSupportsVariableDepthSection ) 
               {
                  if ( !pLoad->Property(_T("VariableDepthSection"),&m_bIsVariableDepthSectionEnabled) )
                  {
                     THROW_LOAD(InvalidFileFormat,pLoad);
                  }
               }

               m_Dimensions.clear();
               m_Dimensions = m_pBeamFactory->LoadSectionDimensions(pLoad);

               if ( !pLoad->EndUnit() )
               {
                  THROW_LOAD(InvalidFileFormat,pLoad);
               }
            }
            else
            {
               // BeginUnit failed...
               if ( !bOkToFail )
               {
                  // not ok to fail so throw an error exceptoin
                  THROW_LOAD(InvalidFileFormat,pLoad);
               }
               else
               {
                  // ok to fail... this means we have a PGSuper file with version 23 datablock
                  // the SectionDimensions datablock isn't valid so we want to just load
                  // the section dimensions (as is done in the else block below)
                  m_Dimensions.clear();
                  m_Dimensions = m_pBeamFactory->LoadSectionDimensions(pLoad);
               }
            }
          }
         else
         {
            m_Dimensions.clear();
            m_Dimensions = m_pBeamFactory->LoadSectionDimensions(pLoad);
         }
      } // end of ( version < 1.1 )


      if (version < 5.0)
      {
         // max adjustment goes away and replaced with cover values
         Float64 dummy;
         if(!pLoad->Property(_T("DownwardStrandAdjustment"), &dummy))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if(!pLoad->Property(_T("DownwardStrandIncrement"), &m_EndAdjustment.m_StrandIncrement))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if(!pLoad->Property(_T("UpwardStrandAdjustment"), &dummy))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if(!pLoad->Property(_T("UpwardStrandIncrement"), &m_HPAdjustment.m_StrandIncrement))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }
      }
      else
      {
         if ( 15 < version && version < 17 )
         {
            // added version 16
            // This input was stubbed in, but never used...
            // The data member has been removed, but the keyword may still exist in the
            // data stream.
            bool bDummy;
            pLoad->Property(_T("CanPostTension"), &bDummy);
         }

         if(!pLoad->Property(_T("UseDifferentHarpedGridAtEnds"), &m_bUseDifferentHarpedGridAtEnds))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if (version < 7.0)
         {
            if(!pLoad->Property(_T("EndAllowVertAdjustment"), &m_EndAdjustment.m_AllowVertAdjustment))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            if(!pLoad->Property(_T("HPAllowVertAdjustment"), &m_HPAdjustment.m_AllowVertAdjustment))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            Float64 top_cover, bottom_cover;
            if(!pLoad->Property(_T("TopCover"), &top_cover))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }
   
            if(!pLoad->Property(_T("BottomCover"), &bottom_cover))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            // convert old cover to adjustment limit
            m_EndAdjustment.m_TopFace = pgsTypes::TopFace;
            m_EndAdjustment.m_TopLimit = top_cover;
            m_EndAdjustment.m_BottomFace = pgsTypes::BottomFace;
            m_EndAdjustment.m_BottomLimit = bottom_cover;

            m_HPAdjustment.m_TopFace = pgsTypes::TopFace;
            m_HPAdjustment.m_TopLimit = top_cover;
            m_HPAdjustment.m_BottomFace = pgsTypes::BottomFace;
            m_HPAdjustment.m_BottomLimit = bottom_cover;

            if(!pLoad->Property(_T("EndStrandIncrement"), &m_EndAdjustment.m_StrandIncrement))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }
         
            if(!pLoad->Property(_T("HPStrandIncrement"), &m_HPAdjustment.m_StrandIncrement))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }
         }
         else
         {

            if(!pLoad->Property(_T("EndAllowVertAdjustment"), &m_EndAdjustment.m_AllowVertAdjustment))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            if(!pLoad->Property(_T("EndStrandIncrement"), &m_EndAdjustment.m_StrandIncrement))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            int lface;
            if(!pLoad->Property(_T("EndBottomFace"), &lface))
            {
               THROW_LOAD(InvalidFileFormat,pLoad); 
            }

            m_EndAdjustment.m_BottomFace = lface==0 ? pgsTypes::TopFace : pgsTypes::BottomFace;

            if(!pLoad->Property(_T("EndBottomLimit"), &m_EndAdjustment.m_BottomLimit))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            if(!pLoad->Property(_T("EndTopFace"), &lface))
            {
               THROW_LOAD(InvalidFileFormat,pLoad); 
            }

            m_EndAdjustment.m_TopFace = lface==0 ? pgsTypes::TopFace : pgsTypes::BottomFace;

            if(!pLoad->Property(_T("EndTopLimit"), &m_EndAdjustment.m_TopLimit))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            if(!pLoad->Property(_T("HPAllowVertAdjustment"), &m_HPAdjustment.m_AllowVertAdjustment))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            if(!pLoad->Property(_T("HPStrandIncrement"), &m_HPAdjustment.m_StrandIncrement))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            if(!pLoad->Property(_T("HPBottomFace"), &lface))
            {
               THROW_LOAD(InvalidFileFormat,pLoad); 
            }

            m_HPAdjustment.m_BottomFace = lface==0 ? pgsTypes::TopFace : pgsTypes::BottomFace;

            if(!pLoad->Property(_T("HPBottomLimit"), &m_HPAdjustment.m_BottomLimit))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            if(!pLoad->Property(_T("HPTopFace"), &lface))
            {
               THROW_LOAD(InvalidFileFormat,pLoad); 
            }

            m_HPAdjustment.m_TopFace = lface==0 ? pgsTypes::TopFace : pgsTypes::BottomFace;

            if(!pLoad->Property(_T("HPTopLimit"), &m_HPAdjustment.m_TopLimit))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            // Added separate adjustment for adj straight strands in version 23.0
            if (23.0 <= version)
            {
               // PGSuper V2.9 and PGSuper/PGSplice V3.0 both used version 23 for different data
               // If the version number is exactly 23, it is ok for the load to fail. Failing
               // indicates the expected data is missing. This happens when a PGSplice version 23
               // data block is being loaded.
               bool bOkToFail = false;
               if ( version == 23 )
               {
                  bOkToFail = true;
               }

               if( pLoad->Property(_T("StraightAllowVertAdjustment"), &m_StraightAdjustment.m_AllowVertAdjustment) )
               {
                  // load succeeded so carry on normally
                  if(!pLoad->Property(_T("StraightStrandIncrement"), &m_StraightAdjustment.m_StrandIncrement))
                  {
                     THROW_LOAD(InvalidFileFormat,pLoad);
                  }

                  if(!pLoad->Property(_T("StraightBottomFace"), &lface))
                  {
                     THROW_LOAD(InvalidFileFormat,pLoad); 
                  }

                  m_StraightAdjustment.m_BottomFace = lface==0 ? pgsTypes::TopFace : pgsTypes::BottomFace;

                  if(!pLoad->Property(_T("StraightBottomLimit"), &m_StraightAdjustment.m_BottomLimit))
                  {
                     THROW_LOAD(InvalidFileFormat,pLoad);
                  }

                  if(!pLoad->Property(_T("StraightTopFace"), &lface))
                  {
                     THROW_LOAD(InvalidFileFormat,pLoad); 
                  }

                  m_StraightAdjustment.m_TopFace = lface==0 ? pgsTypes::TopFace : pgsTypes::BottomFace;

                  if(!pLoad->Property(_T("StraightTopLimit"), &m_StraightAdjustment.m_TopLimit))
                  {
                     THROW_LOAD(InvalidFileFormat,pLoad);
                  }
               }
               else
               {
                  // load StraightAllowVertAdjustment failed
                  if ( bOkToFail )
                  {
                     // it is ok to fail which means we have a file with a different version 23
                     // datablock. set the strand adjustment as would have been done if version was < 23
                     m_StraightAdjustment = m_HPAdjustment; // see else block below
                  }
                  else
                  {
                     // it is not ok to fail so throw the error
                     THROW_LOAD(InvalidFileFormat,pLoad);
                  }
               }
            }
            else
            {
               // Versions before 23 used harp locations for all straight adjustable
               m_StraightAdjustment = m_HPAdjustment;
            }
         }
      }


      Int32 maxStrandsInBottomBundle;
      Float64 bottomBundleLocation;
      Float64 topBundleLocation;
      if ( version <= 1.1 )
      {
         if(!pLoad->Property(_T("MaxStrandsInBottomBundle"), &maxStrandsInBottomBundle))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if(!pLoad->Property(_T("BottomBundleLocation"), &bottomBundleLocation))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if(!pLoad->Property(_T("TopBundleLocation"), &topBundleLocation))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }
      }

      // Create a legacy bar saver if we need it
      LegacyShearData legacy;

      if ( version < 18 )
      {
         Int32 size;
         if(!pLoad->Property(_T("ShearSteelBarSize"), &size))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         WBFL::LRFD::RebarPool::MapOldRebarKey(size,legacy.m_StirrupBarGrade,legacy.m_StirrupBarType,legacy.m_ConfinementBarSize);
      }
      else if (version == 18)
      {
         // rename in version 18
         Int32 value;
         if ( !pLoad->Property(_T("ConfinementBarSize"), &value) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         legacy.m_ConfinementBarSize = WBFL::Materials::Rebar::Size(value);
      }

      if ( version < 19 )
      {
         if(!pLoad->Property(_T("LastConfinementZone"), &legacy.m_LastConfinementZone))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }
      }

      if(!pLoad->Property(_T("HarpingPointLocation"), &m_HarpingPointLocation))
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( version <= 1.4 )
      {
         m_HarpingPointLocation *= -1.0; // fractional distances < 0 starting with version 1.5
      }

      if ( version <= 3.9 )
      {
         m_HarpingPointLocation *= -1.0; // fractional distances are positive values again starting with version 4
      }

      if ( 4.0 <= version )
      {
         if ( !pLoad->Property(_T("UseMinHarpingPointLocation"),&m_bMinHarpingPointLocation) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( m_bMinHarpingPointLocation && !pLoad->Property(_T("MinHarpingPointLocation"),&m_MinHarpingPointLocation) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }
      }

      if ( version <= 1.5 )
      {
         m_HarpPointReference = mlBearing;
      }
      else
      {
         int value;
         if(!pLoad->Property(_T("HarpingPointReference"), &value))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         m_HarpPointReference = (MeasurementLocation)value;
      }

      if ( 4 <= version )
      {
         int value;
         if ( !pLoad->Property(_T("HarpingPointMeasure"),&value))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         m_HarpPointMeasure = (MeasurementType)value;
      }

      bool bOldIntermediateDiaphragms = false;
      if ( version < 6 )
      {
         // Before version 6 there was data for the height and width(thickness) of the intermediate
         // diaphragms. These diaphragms were added to the girders based on a set of diaphragm layout
         // rules in a different library.
         //
         // It is going to be very difficult to lay out this diaphragm with the rules from another 
         // library so it wont be done. The following rules will be created as reasonable defaults:
         // 40' -  80' = Mid Span
         // 80  - 120  = 1/3 points
         // 120+       = 1/4 points
         Float64 H,W;
         if(!pLoad->Property(_T("DiaphragmHeight"), &H))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if(!pLoad->Property(_T("DiaphragmWidth"), &W))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         DiaphragmLayoutRule rule1;
         rule1.Description = _T("40-80ft");
         rule1.MinSpan = WBFL::Units::ConvertToSysUnits(40,WBFL::Units::Measure::Feet);
         rule1.MaxSpan = WBFL::Units::ConvertToSysUnits(80,WBFL::Units::Measure::Feet);
         rule1.Height = H;
         rule1.Thickness = W;
         rule1.MeasureLocation = mlBearing;
         rule1.MeasureType     = mtFractionOfSpanLength;
         rule1.Construction    = ctBridgeSite;
         rule1.Type = dtExternal;
         rule1.Location = 0.5;
         rule1.Method = dwmCompute;
         m_DiaphragmLayoutRules.push_back(rule1);

         DiaphragmLayoutRule rule2a;
         rule2a.Description = _T("80-120ft (Mid-Span)");
         rule2a.MinSpan = WBFL::Units::ConvertToSysUnits(80,WBFL::Units::Measure::Feet);
         rule2a.MaxSpan = WBFL::Units::ConvertToSysUnits(120,WBFL::Units::Measure::Feet);
         rule2a.Height = H;
         rule2a.Thickness = W;
         rule2a.MeasureLocation = mlBearing;
         rule2a.MeasureType     = mtFractionOfSpanLength;
         rule2a.Construction    = ctBridgeSite;
         rule2a.Type = dtExternal;
         rule2a.Location = 0.5;
         rule2a.Method = dwmCompute;
         m_DiaphragmLayoutRules.push_back(rule2a);

         DiaphragmLayoutRule rule2b;
         rule2b.Description = _T("80-120ft (1/3 points)");
         rule2b.MinSpan = WBFL::Units::ConvertToSysUnits(80,WBFL::Units::Measure::Feet);
         rule2b.MaxSpan = WBFL::Units::ConvertToSysUnits(120,WBFL::Units::Measure::Feet);
         rule2b.Height = H;
         rule2b.Thickness = W;
         rule2b.MeasureLocation = mlBearing;
         rule2b.MeasureType     = mtFractionOfSpanLength;
         rule2b.Construction    = ctBridgeSite;
         rule2b.Type = dtExternal;
         rule2b.Location = 0.3333333333;
         rule2b.Method = dwmCompute;
         m_DiaphragmLayoutRules.push_back(rule2b);

         DiaphragmLayoutRule rule3a;
         rule3a.Description = _T("Greater than 120ft (Mid-Span)");
         rule3a.MinSpan = WBFL::Units::ConvertToSysUnits(120,WBFL::Units::Measure::Feet);
         rule3a.MaxSpan = WBFL::Units::ConvertToSysUnits(999,WBFL::Units::Measure::Feet);
         rule3a.Height = H;
         rule3a.Thickness = W;
         rule3a.MeasureLocation = mlBearing;
         rule3a.MeasureType     = mtFractionOfSpanLength;
         rule3a.Construction    = ctBridgeSite;
         rule3a.Type = dtExternal;
         rule3a.Location = 0.5;
         rule3a.Method = dwmCompute;
         m_DiaphragmLayoutRules.push_back(rule3a);

         DiaphragmLayoutRule rule3b;
         rule3b.Description = _T("Greater than 120ft (1/4 points)");
         rule3b.MinSpan = WBFL::Units::ConvertToSysUnits(120,WBFL::Units::Measure::Feet);
         rule3b.MaxSpan = WBFL::Units::ConvertToSysUnits(999,WBFL::Units::Measure::Feet);
         rule3b.Height = H;
         rule3b.Thickness = W;
         rule3b.MeasureLocation = mlBearing;
         rule3b.MeasureType     = mtFractionOfSpanLength;
         rule3b.Construction    = ctBridgeSite;
         rule3b.Type = dtExternal;
         rule3b.Location = 0.25;
         rule3b.Method = dwmCompute;
         m_DiaphragmLayoutRules.push_back(rule3b);

         bOldIntermediateDiaphragms = true;
      }

      // Removed in version 18
      if ( version < 18 )
      {
         // just load the string, but it doesn't need to be translated into any current data
         // the current data is stirrup and longitudinal bar type and grade. THe defaults are A615, Grade 60
         std::_tstring strMat;

         if(!pLoad->Property(_T("ShearSteelMaterial"), &strMat))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if(!pLoad->Property(_T("LongSteelMaterial"),  &strMat))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }
      }

      // No longer needed in vesrion 18
      //// Bug fix: There was an error saving m_LongSteelMaterial that caused it to be blank. This
      ////          caused no harm until we needed the value. Reset it to correct value if blank
      //if (m_LongSteelMaterial.empty())
      //   m_LongSteelMaterial = SMaterialName;

      // top flange shear steel
      if (version < 19 )
      {
         if ( 1.8 <= version )
         {
            if ( !pLoad->Property(_T("DoStirrupsEngageDeck"),    &legacy.m_bStirrupsEngageDeck) )
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }
         }

         if ( 17 <= version )
         {
            if ( !pLoad->Property(_T("IsRoughenedSurface"),       &legacy.m_bIsRoughenedSurface) )
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }
         }

         if (1.1 <= version )
         {
            Int32 size;
            if(!pLoad->Property(_T("TopFlangeShearBarSize"),  &size))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            if ( version < 18 )
            {
               WBFL::Materials::Rebar::Grade grade;
               WBFL::Materials::Rebar::Type type;
               WBFL::LRFD::RebarPool::MapOldRebarKey(size,grade,type,legacy.m_TopFlangeShearBarSize);
            }
            else
            {
               legacy.m_TopFlangeShearBarSize = WBFL::Materials::Rebar::Size(size);
            }

            if(!pLoad->Property(_T("TopFlangeShearBarSpacing"),  &legacy.m_TopFlangeShearBarSpacing))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }
         }
         else
         {
            legacy.m_TopFlangeShearBarSize = WBFL::Materials::Rebar::Size::bsNone;
            legacy.m_TopFlangeShearBarSpacing = 0.0;
         }
      }

      // debond criteria - added in version 13
      if (12 < version)
      {
         if(!pLoad->BeginUnit(_T("DebondingCritia")))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         Float64 local_vers = pLoad->GetVersion();
         
         if (1 < local_vers)
         {
            // added in vesion 2
            if (!pLoad->Property(_T("CheckMaxDebondedStrands"), &m_bCheckMaxDebondStrands))
            {
               THROW_LOAD(InvalidFileFormat, pLoad);
            }
         }
         else
         {
            m_bCheckMaxDebondStrands = true; // this check was not optional in LRFD 8th edition and earlier... set it to true so it gets checked
         }

         if ( !pLoad->Property(_T("MaxDebondStrands"),               &m_MaxDebondStrands))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("MaxDebondStrandsPerRow"),         &m_MaxDebondStrandsPerRow))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if (1 < local_vers)
         {
            // added in version 2
            if (!pLoad->Property(_T("MaxNumDebondedStrandsPerSection10orLess"), &m_MaxNumDebondedStrandsPerSection10orLess))
            {
               THROW_LOAD(InvalidFileFormat, pLoad);
            }
         }

         if ( !pLoad->Property(_T("MaxNumDebondedStrandsPerSection"),&m_MaxNumDebondedStrandsPerSection))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if (1 < local_vers)
         {
            // added in version 2
            if (!pLoad->Property(_T("CheckMaxDebondedStrandsPerSection"), &m_bCheckMaxNumDebondedStrandsPerSection))
            {
               THROW_LOAD(InvalidFileFormat, pLoad);
            }
         }
         else
         {
            m_MaxNumDebondedStrandsPerSection10orLess = m_MaxNumDebondedStrandsPerSection; // make same for older files where there was only one parameter
            m_bCheckMaxNumDebondedStrandsPerSection = true; // this check was not optional in LRFD 8th edition and earlier... set it to true so it gets checked
         }

         if ( !pLoad->Property(_T("MaxDebondedStrandsPerSection"),   &m_MaxDebondedStrandsPerSection))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if (1 < local_vers)
         {
            if (!pLoad->Property(_T("MinDebondLengthDB"), &m_MinDebondLengthDB))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            if (!pLoad->Property(_T("CheckMinDebondLength"), &m_bCheckMinDebondLength))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }
         }
         else
         {
            m_bCheckMinDebondLength = true; // this was not an optional check earlier so when opening old files, enable it
         }

         if ( !pLoad->Property(_T("MinDebondLength"),                &m_MinDebondLength))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if (local_vers < 2)
         {
            // removed in version 2 so just load and discard value
            Float64 dummy;
            pLoad->Property(_T("DefaultDebondLength"), &dummy);
         }

         if (1 < local_vers)
         {
            // added in version 2
            if (!pLoad->Property(_T("CheckDebondingSymmetry"), &m_bCheckDebondingSymmetry))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            if (!pLoad->Property(_T("CheckAdjacentDebonding"), &m_bCheckAdjacentDebonding))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            if (!pLoad->Property(_T("CheckDebondingInWebWidthProjections"), &m_bCheckDebondingInWebWidthProjections))
            {
               THROW_LOAD(InvalidFileFormat, pLoad);
            }
         }
         else
         {
            m_bCheckDebondingSymmetry = false; // was not a requirement before LRFD 9th edition... set to false so this isn't checked for files created before version 2 of this data block
            m_bCheckAdjacentDebonding = false; // was not a requirement before LRFD 9th edition... set to false so this isn't checked for files created before version 2 of this data block
            m_bCheckDebondingInWebWidthProjections = false; // was not a requirement before LRFD 9th edition... set to false so this isn't check for files created before version 2 of this data block
         }

         if ( !pLoad->Property(_T("MaxDebondLengthBySpanFraction"),  &m_MaxDebondLengthBySpanFraction))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("MaxDebondLengthByHardDistance"),  &m_MaxDebondLengthByHardDistance))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->EndUnit())
         {
            THROW_LOAD(BadVersion,pLoad);
         }
      }

      // old global debonding of strands. transfer this value to all straight strands in girder
      bool canDebondStraightStrands(false);
      if ( 1.6 < version && version < 12.0)
      {
         if ( !pLoad->Property(_T("CanDebondStraightStrands"),&canDebondStraightStrands) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }
      }

      // straight strand locations
      m_StraightStrands.clear();
      if ( version < 15 )
      {
         while(pLoad->BeginUnit(_T("StraightStrandLocations")))
         {
            Float64 local_vers = pLoad->GetVersion();
            
            if (local_vers!=1.0 && local_vers!=2.0)
            {
               THROW_LOAD(BadVersion,pLoad);
            }

            StraightStrandLocation locs;
            if(!pLoad->Property(_T("X"), &locs.m_Xstart))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            if(!pLoad->Property(_T("Y"), &locs.m_Ystart))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            locs.m_Xend = locs.m_Xstart;
            locs.m_Yend = locs.m_Ystart;

            if (2.0 <= local_vers)
            {
               if(!pLoad->Property(_T("CanDebond"), &locs.m_bCanDebond))
               {
                  THROW_LOAD(InvalidFileFormat,pLoad);
               }
            }
            else
            {
               locs.m_bCanDebond = canDebondStraightStrands; // old version was all or none
            }

            if(!pLoad->EndUnit())
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            m_StraightStrands.push_back(locs);
         }
      }
      else
      {
         if ( !pLoad->BeginUnit(_T("StraightStrands")) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         while( pLoad->BeginUnit(_T("StrandLocation")) )
         {
            StraightStrandLocation strandLocation;
            if(!pLoad->Property(_T("Xstart"), &strandLocation.m_Xstart))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            if(!pLoad->Property(_T("Ystart"), &strandLocation.m_Ystart))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            if(!pLoad->Property(_T("Xend"), &strandLocation.m_Xend))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            if(!pLoad->Property(_T("Yend"), &strandLocation.m_Yend))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            if(!pLoad->Property(_T("CanDebond"), &strandLocation.m_bCanDebond))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            if(!pLoad->EndUnit()) // StrandLocation
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            m_StraightStrands.push_back(strandLocation);
         }

         // StraightStrands
         if(!pLoad->EndUnit())
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }
      }

      // Harped Strands
      m_HarpedStrands.clear();
      if ( version < 15 )
      {
         if ( 5.0 <= version )
         {
            while(pLoad->BeginUnit(_T("HarpedStrandLocations")))
            {
               if(pLoad->GetVersion()!=2.0)
               {
                  THROW_LOAD(BadVersion,pLoad);
               }

               HarpedStrandLocation strandLocation;
               if(!pLoad->Property(_T("HpX"), &strandLocation.m_Xhp))
               {
                  THROW_LOAD(InvalidFileFormat,pLoad);
               }

               if(!pLoad->Property(_T("HpY"), &strandLocation.m_Yhp))
               {
                  THROW_LOAD(InvalidFileFormat,pLoad);
               }

               if(!pLoad->Property(_T("EndX"), &strandLocation.m_Xstart))
               {
                  THROW_LOAD(InvalidFileFormat,pLoad);
               }

               if(!pLoad->Property(_T("EndY"), &strandLocation.m_Ystart))
               {
                  THROW_LOAD(InvalidFileFormat,pLoad);
               }

               strandLocation.m_Xend = strandLocation.m_Xstart;
               strandLocation.m_Yend = strandLocation.m_Ystart;

               if(!pLoad->EndUnit())
               {
                  THROW_LOAD(InvalidFileFormat,pLoad);
               }

               m_HarpedStrands.push_back(strandLocation);
            }

            pLoad->Property(_T("OddNumberOfHarpedStrands"),&m_bOddNumberOfHarpedStrands);

         }
         else
         {
            // at version 4, we consolidated the harped strand
            // locations at the ends and HP's. Major conversion here:

            // harped strands at end of girder
            CComPtr<IPoint2dCollection> pEndStrandLocations;
            pEndStrandLocations.CoCreateInstance(CLSID_Point2dCollection);

            long num_harped=0;
            std::vector<CComPtr<IPoint2d> > points;
            while(pLoad->BeginUnit(_T("HarpedStrandLocations")))
            {
               if(pLoad->GetVersion()!=1.0)
               {
                  THROW_LOAD(BadVersion,pLoad);
               }

               Float64 x,y;
               if(!pLoad->Property(_T("X"), &x))
               {
                  THROW_LOAD(InvalidFileFormat,pLoad);
               }

               num_harped += x>0.0 ? 2 : 1; // track total number for hp strand computation

               if(!pLoad->Property(_T("Y"), &y))
               {
                  THROW_LOAD(InvalidFileFormat,pLoad);
               }

               if(!pLoad->EndUnit())
               {
                  THROW_LOAD(InvalidFileFormat,pLoad);
               }

               CComPtr<IPoint2d> point;
               point.CoCreateInstance(CLSID_Point2d);
               point->Move(x,y);

               if ( version < 2.0 )
               {
                  // if before version 2 of this Unit, cache the data for later processing
                  points.push_back(point);
               }
               else
               {
                  pEndStrandLocations->Add(point);
               }
            }

            // the order of the strand pattern definition at the end of the girder changed
            // at version 2 for this data Unit. If the data is before version 2, fill
            // up the strand Grid in reverse order.
            if( version < 2.0 )
            {
               std::vector<CComPtr<IPoint2d> >::reverse_iterator iter;
               for ( iter = points.rbegin(); iter != points.rend(); iter++ )
               {
                  pEndStrandLocations->Add(*iter);
               }
            }

            // harped strands at harping point
            CComPtr<IPoint2dCollection> pHPStrandLocations;
            pHPStrandLocations.CoCreateInstance(CLSID_Point2dCollection);
            if ( 1.2 <= version )
            {
               while(pLoad->BeginUnit(_T("HPStrandLocations")))
               {
                  if(pLoad->GetVersion()!=1.0)
                  {
                     THROW_LOAD(BadVersion,pLoad);
                  }

                  Float64 x,y;
                  if(!pLoad->Property(_T("X"), &x))
                  {
                     THROW_LOAD(InvalidFileFormat,pLoad);
                  }

                  if(!pLoad->Property(_T("Y"), &y))
                  {
                     THROW_LOAD(InvalidFileFormat,pLoad);
                  }

                  if(!pLoad->EndUnit())
                  {
                     THROW_LOAD(InvalidFileFormat,pLoad);
                  }

                  CComPtr<IPoint2d> point;
                  point.CoCreateInstance(CLSID_Point2d);
                  point->Move(x,y);
                  pHPStrandLocations->Add(point);
               }

               pLoad->Property(_T("OddNumberOfHarpedStrands"),&m_bOddNumberOfHarpedStrands);
            }
            else
            {
               m_bOddNumberOfHarpedStrands = true;

               // Generate strand locations based on bundle definitions
               for ( Int32 i = 0; i < num_harped; i++ )
               {
                  Float64 x,y;
                  if ( i < maxStrandsInBottomBundle )
                  {
                     x = 0;
                     y = bottomBundleLocation;
                  }
                  else
                  {
                     x = 0;
                     y = bottomBundleLocation + topBundleLocation;
                  }

                  CComPtr<IPoint2d> point;
                  point.CoCreateInstance(CLSID_Point2d);
                  point->Move(x,y);
                  pHPStrandLocations->Add(point);
               }
            }

            // now that we have our strands in pre-version 5.0 temp data structures, let's convert
            IndexType end_cnt, hp_cnt;
            pEndStrandLocations->get_Count(&end_cnt);
            pHPStrandLocations->get_Count(&hp_cnt);

            IndexType end_idx=0;
            for (IndexType hp_idx=0; hp_idx<hp_cnt; hp_idx++)
            {
               CComPtr<IPoint2d> hp_pnt;
               pHPStrandLocations->get_Item(hp_idx, &hp_pnt);
               Float64 hpx, hpy;
               hp_pnt->get_X(&hpx);
               hp_pnt->get_Y(&hpy);

               Float64 endx, endy;
               if (end_idx<end_cnt)
               {
                  CComPtr<IPoint2d> end_pnt;
                  pEndStrandLocations->get_Item(end_idx++, &end_pnt);
                  end_pnt->get_X(&endx);
                  end_pnt->get_Y(&endy);
               }
               else
               {
                  ATLASSERT(false);
                  THROW_LOAD(InvalidFileFormat,pLoad);
               }

               // store our data
               HarpedStrandLocation hp(endx, endy, hpx, hpy, endx, endy);
               m_HarpedStrands.push_back(hp);

               // now deal with issues related to X>0 and Float64 locations
               // indexes must be incremented if Float64 x==0's
               if (0.0 < hpx && IsZero(endx))
               {
                  // next end point better have x==0
                  Float64 x;
                  CComPtr<IPoint2d> end_pnt;
                  pEndStrandLocations->get_Item(end_idx++, &end_pnt); // note increment
                  end_pnt->get_X(&x);
                  if (!IsZero(x))
                  {
                     ATLASSERT(false);
                     THROW_LOAD(InvalidFileFormat,pLoad);
                  }
               }
               if (0.0 < endx && IsZero(hpx))
               {
                  // next HP point better have x==0
                  Float64 x;
                  CComPtr<IPoint2d> hp_pnt;
                  pHPStrandLocations->get_Item(++hp_idx, &hp_pnt); // note increment
                  hp_pnt->get_X(&x);
                  if (!IsZero(x))
                  {
                     ATLASSERT(false);
                     THROW_LOAD(InvalidFileFormat,pLoad);
                  }
               }
            }
            
            m_bUseDifferentHarpedGridAtEnds = true;

         }
      }
      else
      {
         if ( !pLoad->BeginUnit(_T("HarpedStrands")) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         while( pLoad->BeginUnit(_T("StrandLocation")) )
         {
            HarpedStrandLocation strandLocation;
            if(!pLoad->Property(_T("Xstart"), &strandLocation.m_Xstart))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            if(!pLoad->Property(_T("Ystart"), &strandLocation.m_Ystart))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            if(!pLoad->Property(_T("Xhp"), &strandLocation.m_Xhp))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            if(!pLoad->Property(_T("Yhp"), &strandLocation.m_Yhp))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            if(!pLoad->Property(_T("Xend"), &strandLocation.m_Xend))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            if(!pLoad->Property(_T("Yend"), &strandLocation.m_Yend))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            // debond is ignore for temporary strands
            //if(!pLoad->Property(_T("CanDebond"), &strandLocation.m_bCanDebond))
            //   THROW_LOAD(InvalidFileFormat,pLoad);

            if(!pLoad->EndUnit()) // StrandLocation
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            m_HarpedStrands.push_back(strandLocation);
         }

         // HarpedStrands
         if(!pLoad->EndUnit())
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }


         pLoad->Property(_T("OddNumberOfHarpedStrands"),&m_bOddNumberOfHarpedStrands);

         if ( 21.0 == version || 22.0 == version )
         {
            bool forceStr;
            if ( !pLoad->Property(_T("ForceHarpedStrandsStraight"),&forceStr) )
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            if (forceStr)
            {
               m_AdjustableStrandType = pgsTypes::asStraight;
            }
         }
         else if ( 23.0 <= version)
         {
            Int32 val;
            bool bOkToFail = (version == 23 ? true : false);
            if ( pLoad->Property(_T("AdjustableStrandType"),&val) )
            {
               m_AdjustableStrandType = (pgsTypes::AdjustableStrandType)val;
            }
            else
            {
               if ( bOkToFail )
               {
                  bool forceStr;
                  if ( !pLoad->Property(_T("ForceHarpedStrandsStraight"),&forceStr) )
                  {
                     THROW_LOAD(InvalidFileFormat,pLoad);
                  }

                  if (forceStr)
                  {
                     m_AdjustableStrandType = pgsTypes::asStraight;
                  }
               }
               else
               {
                  THROW_LOAD(InvalidFileFormat,pLoad);
               }
            }
         }
      }

      // Temporary strands
      m_TemporaryStrands.clear();
      if ( version < 15 )
      {
         while(pLoad->BeginUnit(_T("TemporaryStrandLocations")))
         {
            if(pLoad->GetVersion()!=1.0)
            {
               THROW_LOAD(BadVersion,pLoad);
            }

            Float64 x,y;
            if(!pLoad->Property(_T("X"), &x))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            if(!pLoad->Property(_T("Y"), &y))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            if(!pLoad->EndUnit())
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            StraightStrandLocation strandLocation;
            strandLocation.m_bCanDebond = false;
            strandLocation.m_Xstart = x;
            strandLocation.m_Ystart = y;
            strandLocation.m_Xend   = x;
            strandLocation.m_Yend   = y;

            m_TemporaryStrands.push_back(strandLocation);
         }
      }
      else
      {
         if ( !pLoad->BeginUnit(_T("TemporaryStrands")) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         while( pLoad->BeginUnit(_T("StrandLocation")) )
         {
            StraightStrandLocation strandLocation;
            if(!pLoad->Property(_T("Xstart"), &strandLocation.m_Xstart))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            if(!pLoad->Property(_T("Ystart"), &strandLocation.m_Ystart))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            if(!pLoad->Property(_T("Xend"), &strandLocation.m_Xend))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            if(!pLoad->Property(_T("Yend"), &strandLocation.m_Yend))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            // debond is ignore for temporary strands
            //if(!pLoad->Property(_T("CanDebond"), &strandLocation.m_bCanDebond))
            //   THROW_LOAD(InvalidFileFormat,pLoad);

            if(!pLoad->EndUnit()) // StrandLocation
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            m_TemporaryStrands.push_back(strandLocation);
         }

         // TemporaryStrands
         if(!pLoad->EndUnit())
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }
      }

      // global strand order
      m_PermanentStrands.clear();

      if (version < 5.0)
      {
         // must convert old data to new - just pile them together
         // straight strand locations
         GridIndexType nGridEntries = m_StraightStrands.size();
         GridIndexType gridIdx;
         for ( gridIdx = 0; gridIdx < nGridEntries; gridIdx++ )
         {
            m_PermanentStrands.push_back( PermanentStrand(stStraight, gridIdx) );
         }

         // harped strands
         nGridEntries = m_HarpedStrands.size();
         for ( gridIdx = 0; gridIdx < nGridEntries; gridIdx++ )
         {
            m_PermanentStrands.push_back( PermanentStrand(stAdjustable, gridIdx) );
         }

      }
      else
      {
         while(pLoad->BeginUnit(_T("GlobalStrandOrder")))
         {
            if(pLoad->GetVersion()!=1.0)
            {
               THROW_LOAD(BadVersion,pLoad);
            }

            std::_tstring name;
            if(!pLoad->Property(_T("StrandType"),&name))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            PermanentStrand permStrand;

            if (name == _T("Straight"))
            {
               permStrand.m_StrandType = stStraight;
            }
            else if (name == _T("Harped"))
            {
               permStrand.m_StrandType = stAdjustable;
            }
            else
            {
               ATLASSERT(false);
               THROW_LOAD(InvalidFileFormat,pLoad);
            }
         
            if(!pLoad->Property(_T("LocalSortOrder"), &permStrand.m_GridIdx))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }


            if(!pLoad->EndUnit())
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            m_PermanentStrands.push_back(permStrand);
         }

         if (version < 7.0)
         {
            // added in version 5, removed in 7
            int smtype;
            if(!pLoad->Property(_T("StressMitigationType"), &smtype))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }
         }
      }

      // shear zones
      if (19 <= version)
      {
         // After 19, we just user common data class
         m_ShearData.Load(pLoad);
      }
      else
      {
         // Before 19, data was stored in local classes. put into legacy class and 
         // convert once it's loaded
         if ( 17 < version )
         {
            Int32 value;
            if ( !pLoad->Property(_T("ShearSteelBarType"), &value) )
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            legacy.m_StirrupBarType = WBFL::Materials::Rebar::Type(value);

            if ( !pLoad->Property(_T("ShearSteelBarGrade"), &value) )
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            legacy.m_StirrupBarGrade = WBFL::Materials::Rebar::Grade(value);
         }

         LegacyShearData::ShearZoneInfo zi;
         legacy.m_ShearZoneInfo.clear();
         while(pLoad->BeginUnit(_T("ShearZones")))
         {
            Float64 shear_zone_version = pLoad->GetVersion();

            if(3 < shear_zone_version )
            {
               THROW_LOAD(BadVersion,pLoad);
            }

            if ( shear_zone_version < 2 )
            {
               if(!pLoad->Property(_T("ZoneLength"),&zi.ZoneLength))
               {
                  THROW_LOAD(InvalidFileFormat,pLoad);
               }

               Int32 size;
               if(!pLoad->Property(_T("BarSize"), &size))
               {
                  THROW_LOAD(InvalidFileFormat,pLoad);
               }

               WBFL::Materials::Rebar::Grade grade;
               WBFL::Materials::Rebar::Type type;
               WBFL::LRFD::RebarPool::MapOldRebarKey(size,grade,type,zi.VertBarSize);


               if ( version < 11.0 )
               {
                  if(!pLoad->Property(_T("SS"), &zi.StirrupSpacing))
                  {
                     THROW_LOAD(InvalidFileFormat,pLoad);
                  }
               }
               else
               {
                  if(!pLoad->Property(_T("Spacing"), &zi.StirrupSpacing))
                  {
                     THROW_LOAD(InvalidFileFormat,pLoad);
                  }
               }

               if ( 1.1 <= shear_zone_version )
               {
                  ATLASSERT( shear_zone_version < 2 );

                  if ( !pLoad->Property(_T("NBars"), &zi.nVertBars) )
                  {
                     THROW_LOAD(InvalidFileFormat,pLoad);
                  }
               }
               else
               {
                  zi.nVertBars = 2;
               }

               zi.HorzBarSize = WBFL::Materials::Rebar::Size::bsNone;
               zi.nHorzBars   = 2;
            }
            else
            {
               if(!pLoad->Property(_T("ZoneLength"),&zi.ZoneLength))
               {
                  THROW_LOAD(InvalidFileFormat,pLoad);
               }

               if(!pLoad->Property(_T("Spacing"), &zi.StirrupSpacing))
               {
                  THROW_LOAD(InvalidFileFormat,pLoad);
               }

               if ( shear_zone_version < 3 )
               {
                  Int32 size;
                  if(!pLoad->Property(_T("VertBarSize"), &size))
                  {
                     THROW_LOAD(InvalidFileFormat,pLoad);
                  }

                  WBFL::Materials::Rebar::Grade grade;
                  WBFL::Materials::Rebar::Type type;
                  WBFL::LRFD::RebarPool::MapOldRebarKey(size,grade,type,zi.VertBarSize);
               }
               else
               {
                  Int32 value;
                  if(!pLoad->Property(_T("VertBarSize"), &value))
                  {
                     THROW_LOAD(InvalidFileFormat,pLoad);
                  }

                  zi.VertBarSize = WBFL::Materials::Rebar::Size(value);
               }

               if(!pLoad->Property(_T("VertBars"), &zi.nVertBars))
               {
                  THROW_LOAD(InvalidFileFormat,pLoad);
               }

               if ( shear_zone_version < 3 )
               {
                  Int32 size;
                  if(!pLoad->Property(_T("HorzBarSize"), &size))
                  {
                     THROW_LOAD(InvalidFileFormat,pLoad);
                  }

                  WBFL::Materials::Rebar::Grade grade;
                  WBFL::Materials::Rebar::Type type;
                  WBFL::LRFD::RebarPool::MapOldRebarKey(size,grade,type,zi.HorzBarSize);
               }
               else
               {
                  Int32 value;
                  if(!pLoad->Property(_T("HorzBarSize"), &value))
                  {
                     THROW_LOAD(InvalidFileFormat,pLoad);
                  }

                  zi.HorzBarSize = WBFL::Materials::Rebar::Size(value);
               }

               if(!pLoad->Property(_T("HorzBars"), &zi.nHorzBars))
               {
                  THROW_LOAD(InvalidFileFormat,pLoad);
               }
            }


            if(!pLoad->EndUnit())
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            legacy.m_ShearZoneInfo.push_back(zi);
         }

         // At this point we have all legacy shear data loaded - convert it to CShearData
         m_ShearData = legacy.ConvertToShearData();
      }

      // Longitudinal Steel rows
      if ( 17 < version )
      {
         int value;

         if(!pLoad->Property(_T("LongitudinalBarType"), &value))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         m_LongitudinalBarType = WBFL::Materials::Rebar::Type(value);

         if(!pLoad->Property(_T("LongitudinalBarGrade"), &value))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         m_LongitudinalBarGrade = WBFL::Materials::Rebar::Grade(value);
      }

      LongSteelInfo li;
      std::_tstring tmp;
      m_LongSteelInfo.clear();
      while(pLoad->BeginUnit(_T("LongSteelInfo")))
      {
         // Added layout options in local version 3.0, outer version 22.0
         if ( pLoad->GetVersion() < 3.0 )
         {
            li.BarLayout = pgsTypes::blFullLength;
            li.DistFromEnd = 0.0;
            li.BarLength = 0.0;
         }
         else
         {
            Int32 layout;
            if(!pLoad->Property(_T("BarLayout"),  &layout))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            li.BarLayout = (pgsTypes::RebarLayoutType)layout;

            if(!pLoad->Property(_T("DistFromEnd"), &li.DistFromEnd))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            if(!pLoad->Property(_T("BarLength"), &li.BarLength))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }
         }

         if(!pLoad->Property(_T("Face"), &tmp))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if (tmp==_T("Bottom"))
         {
            li.Face = pgsTypes::BottomFace;
         }
         else if (tmp==_T("Top"))
         {
            li.Face = pgsTypes::TopFace;
         }
         else
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if(!pLoad->Property(_T("NumberOfBars"), &li.NumberOfBars))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( pLoad->GetVersion() < 2.0 )
         {
            Int32 barSize;
            if(!pLoad->Property(_T("BarSize"),  &barSize))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            WBFL::Materials::Rebar::Grade grade;
            WBFL::Materials::Rebar::Type type;
            WBFL::LRFD::RebarPool::MapOldRebarKey(barSize,grade,type,li.BarSize);
         }
         else
         {
            Int32 value;
            if(!pLoad->Property(_T("BarSize"),  &value))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            li.BarSize = WBFL::Materials::Rebar::Size(value);
         }

         if(!pLoad->Property(_T("BarCover"), &li.Cover))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if(!pLoad->Property(_T("BarSpacing"), &li.BarSpacing))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if(!pLoad->EndUnit())
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         m_LongSteelInfo.push_back(li);
      }
   

      if ( !bOldIntermediateDiaphragms )
      {
         m_DiaphragmLayoutRules.clear();
      }

      while ( pLoad->BeginUnit(_T("DiaphragmLayoutRule")) )
      {
         DiaphragmLayoutRule dlr;
         Float64 diaVersion = pLoad->GetVersion();

         if ( !pLoad->Property(_T("Description"),&dlr.Description) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("MinSpan"),&dlr.MinSpan) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("MaxSpan"),&dlr.MaxSpan) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( diaVersion < 2 )
         {
            if ( !pLoad->Property(_T("Height"),&dlr.Height) )
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            if ( !pLoad->Property(_T("Thickness"),&dlr.Thickness) )
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            dlr.Method = dwmCompute;
         }
         else
         {
            int value;
            if ( !pLoad->Property(_T("Method"),&value) )
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            dlr.Method = (DiaphragmWeightMethod)value;

            if ( dlr.Method == dwmCompute )
            {
               if ( !pLoad->Property(_T("Height"),&dlr.Height) )
               {
                  THROW_LOAD(InvalidFileFormat,pLoad);
               }

               if ( !pLoad->Property(_T("Thickness"),&dlr.Thickness) )
               {
                  THROW_LOAD(InvalidFileFormat,pLoad);
               }
            }
            else
            {
               if ( !pLoad->Property(_T("Weight"),&dlr.Weight) )
               {
                  THROW_LOAD(InvalidFileFormat,pLoad);
               }
            }
         }

         int value;
         if ( !pLoad->Property(_T("DiaphragmType"),&value) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }
         dlr.Type = (DiaphragmType)(value);

         if ( !pLoad->Property(_T("ConstructionType"),&value) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }
         dlr.Construction = (ConstructionType)(value);

         if ( !pLoad->Property(_T("MeasurementType"),&value) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }
/*
         if ( value == 0 )
         {
            // value = 0 is fraction of span length... this doesn't make sense for the generalized approach for spliced girders
            // set value to 1, fraction of precast segment length
            value = 1;
         }
*/
         dlr.MeasureType = (MeasurementType)(value);

         if ( !pLoad->Property(_T("MeasurmentLocation"),&value) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }
         dlr.MeasureLocation = (MeasurementLocation)(value);

         if ( !pLoad->Property(_T("Location"),&dlr.Location) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         m_DiaphragmLayoutRules.push_back(dlr);

         if ( !pLoad->EndUnit() )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }
      }

      if (20 <= version)
      {
         if ( !pLoad->BeginUnit(_T("ShearDesign")) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         Float64 shear_design_version = pLoad->GetVersion();

         IndexType size;
         if ( !pLoad->Property(_T("StirrupSizeBarComboCollSize"),&size) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->BeginUnit(_T("StirrupSizeBarComboColl")) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         m_StirrupSizeBarComboColl.clear();
         for (IndexType is=0; is<size; is++)
         {
            StirrupSizeBarCombo cbo;

            Int32 value;
            if ( !pLoad->Property(_T("BarSize"), &value) )
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            cbo.Size = WBFL::Materials::Rebar::Size(value);

             if ( !pLoad->Property(_T("NLegs"),&(cbo.NLegs)) )
             {
                THROW_LOAD(InvalidFileFormat,pLoad);
             }

             m_StirrupSizeBarComboColl.push_back(cbo);
         }

         if ( !pLoad->EndUnit() ) // StirrupSizeBarComboColl
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("NumAvailableBarSpacings"),&size) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->BeginUnit(_T("AvailableBarSpacings")) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         m_AvailableBarSpacings.clear();
         for (IndexType is=0; is<size; is++)
         {
            Float64 spacing;
            if ( !pLoad->Property(_T("Spacing"),&spacing) )
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            m_AvailableBarSpacings.push_back(spacing);
         }

         if ( !pLoad->EndUnit() ) // AvailableBarSpacings
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }


         if ( !pLoad->Property(_T("MaxSpacingChangeInZone"),&m_MaxSpacingChangeInZone) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("MaxShearCapacityChangeInZone"),&m_MaxShearCapacityChangeInZone) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("MinZoneLengthSpacings"),&m_MinZoneLengthSpacings) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("MinZoneLengthLength"),&m_MinZoneLengthLength) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }


         bool bogus_bool; // Load these values even though they are no longer used.
         if ( !pLoad->Property(_T("IsTopFlangeRoughened"),&bogus_bool) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("DoExtendBarsIntoDeck"),&m_DoExtendBarsIntoDeck) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("DoBarsProvideSplittingCapacity"),&bogus_bool) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("DoBarsActAsConfinement"),&m_DoBarsActAsConfinement) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         Int32 lval;
         if ( !pLoad->Property(_T("LongShearCapacityIncreaseMethod"),&lval) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         m_LongShearCapacityIncreaseMethod = (LongShearCapacityIncreaseMethod)lval;

         if (1 < shear_design_version)
         {
            // added in version 2 of ShearDesign data block
            if (!pLoad->Property(_T("InterfaceShearWidthReduction"), &m_InterfaceShearWidthReduction))
            {
               THROW_LOAD(InvalidFileFormat, pLoad);
            }
         }

         if ( !pLoad->EndUnit() ) // ShearDesign
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }
      }

      // Strand data is all loaded now - need to clean up adjustable strand type so design strategies
      // get set up correctly next
      if ( version <= 23.0 )
      {
         m_AdjustableStrandType = m_HarpedStrands.empty() ? pgsTypes::asStraight : pgsTypes::asHarped;
      }
      // Prestress design strategies
      m_PrestressDesignStrategies.clear();
      if (24.0 <= version)
      {
         bool bOkToFail = false;
         if ( version == 24 )
         {
            bOkToFail = true; // it is OK if version 24 datablock fails
         }

         bool bDidBeginUnit = true;
         if (!pLoad->BeginUnit(_T("PrestressDesignStrategies")))
         {
            bDidBeginUnit = false;
         }

         if ( bDidBeginUnit )
         {
            while( pLoad->BeginUnit(_T("PrestressDesignStrategy")) )
            {
               PrestressDesignStrategy strategy;
               Int32 lval;
               if(!pLoad->Property(_T("FlexuralDesignType"),&lval))
               {
                  THROW_LOAD(InvalidFileFormat,pLoad);
               }

               strategy.m_FlexuralDesignType = (arFlexuralDesignType)lval;

               if(!pLoad->Property(_T("MaxFc"), &strategy.m_MaxFc))
               {
                  THROW_LOAD(InvalidFileFormat,pLoad);
               }

               if(!pLoad->Property(_T("MaxFci"), &strategy.m_MaxFci))
               {
                  THROW_LOAD(InvalidFileFormat,pLoad);
               }

               if(!pLoad->EndUnit()) // PrestressDesignStrategy
               {
                  THROW_LOAD(InvalidFileFormat,pLoad);
               }

               m_PrestressDesignStrategies.push_back(strategy);
            }

            if ( !pLoad->EndUnit() ) // PrestressDesignStrategies
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }
         }
         else
         {
            // BeginUnit failed...
            if ( !bOkToFail )
            {
               // not ok to fail so throw an error exceptoin
               THROW_LOAD(InvalidFileFormat,pLoad);
            }
            else
            {
               // ok to fail... this means we have a PGSuper file with version 24 datablock
               // the PrestressDesignStrategies datablock isn't valid so we just want to
               // use the defaults

               // Note that this must happen after all prestress data is loaded - careful if you want to move this block
               SetDefaultPrestressDesignStrategy();
            }
         }
      }
      else
      {
         // Note that this must happen after all prestress data is loaded - careful if you want to move this block
         SetDefaultPrestressDesignStrategy();
      }

      if (26.0 <= version)
      {
         if ( !pLoad->Property(_T("MinFilletValue"),&m_MinFilletValue) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("DoCheckMinHaunchAtBearingLines"),&m_DoCheckMinHaunchAtBearingLines) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("MinHaunchAtBearingLines"),&m_MinHaunchAtBearingLines) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("ExcessiveSlabOffsetWarningTolerance"),&m_ExcessiveSlabOffsetWarningTolerance) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if (!pLoad->BeginUnit(_T("CamberMultipliers")))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("ErectionFactor"), &m_CamberMultipliers.ErectionFactor))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("CreepFactor"), &m_CamberMultipliers.CreepFactor))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("DiaphragmFactor"), &m_CamberMultipliers.DiaphragmFactor))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("DeckPanelFactor"), &m_CamberMultipliers.DeckPanelFactor))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("SlabUser1Factor"), &m_CamberMultipliers.SlabUser1Factor))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("HaunchLoadFactor"), &m_CamberMultipliers.SlabPadLoadFactor))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("BarrierSwOverlayUser2Factor"), &m_CamberMultipliers.BarrierSwOverlayUser2Factor))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if(!pLoad->EndUnit())// CamberMultipliers
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }
      }

      if ( 26 < version )
      {
         // added version 27
         if ( !pLoad->Property(_T("DragCoefficient"),&m_DragCoefficient) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }
      }
      else
      {
         // default for U-Beams is 1.5, otherwise 2.2
         if ( m_pBeamFactory->GetFamilyCLSID() == CLSID_UBeamFamily || m_pBeamFactory->GetFamilyCLSID() == CLSID_SplicedUBeamFamily )
         {
            m_DragCoefficient = 1.5;
         }
         else
         {
            m_DragCoefficient = 2.2;
         }
      }

      if (27 < version)
      {
         if (!pLoad->Property(_T("PrecamberLimit"), &m_PrecamberLimit))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
      }

      if (28 < version)
      {
         if (!pLoad->Property(_T("DoReportBearingElevationsAtGirderEdges"), &m_DoReportBearingElevationsAtGirderEdges))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
      }


      if(!pLoad->EndUnit())
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }
   }
   else
   {
      return false; // not a GirderLibraryEntry
   }

   ATLASSERT(m_pBeamFactory != nullptr);
   CComQIPtr<IBeamFactoryCompatibility> compatibility(m_pBeamFactory);
   if (compatibility)
   {
      // Beam Factories are singletons, so we must capture the compatibility data here, otherwise the current
      // value will be changed when the a girder library entry using this type of beam factory is loaded
      ATLASSERT(m_pCompatibilityData == nullptr); // did we get this data already???
      m_pCompatibilityData = compatibility->GetCompatibilityData();
   }

   return true;
}

HRESULT GirderLibraryEntry::CreateBeamFactory(const std::_tstring& strCLSID)
{
   USES_CONVERSION;

   m_pBeamFactory.Release();
   
   // Get the class factory for this type
   CClassFactoryHolder* pClassFactory;
   ClassFactoryCollection::iterator found = ms_ClassFactories.find(strCLSID);
   if ( found != ms_ClassFactories.end() )
   {
      pClassFactory = &(*found).second;
   }
   else
   {
      // Class factory was not found... create it
      CLSID clsid;
      ::CLSIDFromString(CT2OLE(strCLSID.c_str()),&clsid);

      CComPtr<IClassFactory> classFactory;
      HRESULT hr = ::CoGetClassObject(clsid,CLSCTX_ALL,nullptr,IID_IClassFactory,(void**)&classFactory);
      if ( FAILED(hr) )
      {
         return hr;
      }

      CClassFactoryHolder cfh(classFactory);

      // Save it for next time
      std::pair<ClassFactoryCollection::iterator,bool> result;
      result = ms_ClassFactories.insert(std::make_pair(strCLSID,cfh));
      ClassFactoryCollection::iterator iter = result.first;
      pClassFactory = &(*iter).second;
   }

   // Using the class factory, create the beam factory
   HRESULT hr = pClassFactory->CreateInstance(nullptr,IID_IBeamFactory,(void**)&m_pBeamFactory);
   return hr;
}

void GirderLibraryEntry::LoadIBeamDimensions(WBFL::System::IStructuredLoad* pLoad)
{
   CLSID clsid;
   ::CLSIDFromString(_T("{EF144A97-4C75-4234-AF3C-71DC89B1C8F8}"),&clsid);
   m_pBeamFactory.Release();
   HRESULT hr = ::CoCreateInstance(clsid,nullptr,CLSCTX_ALL,IID_IBeamFactory,(void**)&m_pBeamFactory);

   m_Dimensions.clear();

   Float64 value;
   if(!pLoad->Property(_T("D1"), &value))
   {
      THROW_LOAD(InvalidFileFormat,pLoad);
   }

   m_Dimensions.push_back(Dimension(_T("D1"),value));

   if(!pLoad->Property(_T("D2"), &value))
   {
      THROW_LOAD(InvalidFileFormat,pLoad);
   }

   m_Dimensions.push_back(Dimension(_T("D2"),value));

   if(!pLoad->Property(_T("D3"), &value))
   {
      THROW_LOAD(InvalidFileFormat,pLoad);
   }

   m_Dimensions.push_back(Dimension(_T("D3"),value));

   if(!pLoad->Property(_T("D4"), &value))
   {
      THROW_LOAD(InvalidFileFormat,pLoad);
   }

   m_Dimensions.push_back(Dimension(_T("D4"),value));

   if(!pLoad->Property(_T("D5"), &value))
   {
      THROW_LOAD(InvalidFileFormat,pLoad);
   }

   m_Dimensions.push_back(Dimension(_T("D5"),value));

   if(!pLoad->Property(_T("D6"), &value))
   {
      THROW_LOAD(InvalidFileFormat,pLoad);
   }

   m_Dimensions.push_back(Dimension(_T("D6"),value));

   if(!pLoad->Property(_T("D7"), &value))
   {
      THROW_LOAD(InvalidFileFormat,pLoad);
   }

   m_Dimensions.push_back(Dimension(_T("D7"),value));

   if(!pLoad->Property(_T("W1"), &value))
   {
      THROW_LOAD(InvalidFileFormat,pLoad);
   }

   m_Dimensions.push_back(Dimension(_T("W1"),value));

   if(!pLoad->Property(_T("W2"), &value))
   {
      THROW_LOAD(InvalidFileFormat,pLoad);
   }

   m_Dimensions.push_back(Dimension(_T("W2"),value));

   if(!pLoad->Property(_T("W3"), &value))
   {
      THROW_LOAD(InvalidFileFormat,pLoad);
   }

   m_Dimensions.push_back(Dimension(_T("W3"),value));

   if(!pLoad->Property(_T("W4"), &value))
   {
      THROW_LOAD(InvalidFileFormat,pLoad);
   }

   m_Dimensions.push_back(Dimension(_T("W4"),value));

   if(!pLoad->Property(_T("T1"), &value))
   {
      THROW_LOAD(InvalidFileFormat,pLoad);
   }

   m_Dimensions.push_back(Dimension(_T("T1"),value));

   if(!pLoad->Property(_T("T2"), &value))
   {
      THROW_LOAD(InvalidFileFormat,pLoad);
   }

   m_Dimensions.push_back(Dimension(_T("T2"),value));

   // These dimensions were added later
   m_Dimensions.push_back(Dimension(_T("C1"), 0.0));
   m_Dimensions.push_back(Dimension(_T("EndBlockWidth"),0.0));
   m_Dimensions.push_back(Dimension(_T("EndBlockLength"),0.0));
   m_Dimensions.push_back(Dimension(_T("EndBlockTransition"),0.0));

   // This is the expected order of the dimensions for the conversion method
   // The dimensions are alphabetical, except for the last 3 (EndBlock...)
   // Sort the container, except for the last 3 elements, using the dimension name as the sort key
   //m_DimNames.emplace_back(_T("C1"));
   //m_DimNames.emplace_back(_T("D1"));
   //m_DimNames.emplace_back(_T("D2"));
   //m_DimNames.emplace_back(_T("D3"));
   //m_DimNames.emplace_back(_T("D4"));
   //m_DimNames.emplace_back(_T("D5"));
   //m_DimNames.emplace_back(_T("D6"));
   //m_DimNames.emplace_back(_T("D7"));
   //m_DimNames.emplace_back(_T("T1"));
   //m_DimNames.emplace_back(_T("T2"));
   //m_DimNames.emplace_back(_T("W1"));
   //m_DimNames.emplace_back(_T("W2"));
   //m_DimNames.emplace_back(_T("W3"));
   //m_DimNames.emplace_back(_T("W4"));
   //m_DimNames.emplace_back(_T("EndBlockWidth"));
   //m_DimNames.emplace_back(_T("EndBlockLength"));
   //m_DimNames.emplace_back(_T("EndBlockTransition"));

   std::sort(std::begin(m_Dimensions), std::end(m_Dimensions) - 3, [](const auto& a, const auto& b) {return a.first < b.first; });

   m_Dimensions = ConvertIBeamDimensions<GirderLibraryEntry::Dimensions>(m_Dimensions);
}

bool GirderLibraryEntry::IsEqual(const GirderLibraryEntry& rOther,bool bConsiderName) const
{
   std::vector<pgsLibraryEntryDifferenceItem*> vDifferences;
   bool bMustRename;
   return Compare(rOther,vDifferences,bMustRename,true,bConsiderName);
}

bool GirderLibraryEntry::Compare(const GirderLibraryEntry& rOther, std::vector<pgsLibraryEntryDifferenceItem*>& vDifferences, bool& bMustRename, bool bReturnOnFirstDifference, bool considerName,bool bCompareSeedValues) const
{
   CEAFApp* pApp = EAFGetApp();
   const WBFL::Units::IndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

   CComQIPtr<ISplicedBeamFactory,&IID_ISplicedBeamFactory> splicedBeamFactory(m_pBeamFactory);
   bool bSplicedGirder = (splicedBeamFactory == nullptr ? false : true);

   bMustRename = false;

   if (!const_cast<CComPtr<IBeamFactory>*>(&m_pBeamFactory)->IsEqualObject(rOther.m_pBeamFactory))
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Girder are different type."), _T(""), _T("")));
      bMustRename = true;
   }


   //
   // General Tab
   //
   if ( m_Dimensions.size() != rOther.m_Dimensions.size() ||
       !std::equal(m_Dimensions.begin(),m_Dimensions.end(),rOther.m_Dimensions.begin(),CompareDimensions) ||
       (bSplicedGirder ? m_bIsVariableDepthSectionEnabled != rOther.m_bIsVariableDepthSectionEnabled : false)
       )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Girder dimensions are different"),_T(""),_T("")));
   }

   //
   // Permanent Strands Tab
   //
   if ( !bSplicedGirder )
   {
      if ( m_AdjustableStrandType != rOther.m_AdjustableStrandType )
      {
         RETURN_ON_DIFFERENCE;
         vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Adjustable Strand Type"),GetAdjustableStrandType(m_AdjustableStrandType),GetAdjustableStrandType(rOther.m_AdjustableStrandType)));
      }

      if ( m_bOddNumberOfHarpedStrands != rOther.m_bOddNumberOfHarpedStrands )
      {
         RETURN_ON_DIFFERENCE;
         vDifferences.push_back(new pgsLibraryEntryDifferenceBooleanItem(_T("Coerce Odd Number of Harped Strands"),m_bOddNumberOfHarpedStrands,rOther.m_bOddNumberOfHarpedStrands,_T("Checked"),_T("Unchecked")));
      }

      if ( m_bUseDifferentHarpedGridAtEnds != rOther.m_bUseDifferentHarpedGridAtEnds )
      {
         RETURN_ON_DIFFERENCE;
         vDifferences.push_back(new pgsLibraryEntryDifferenceBooleanItem(_T("Use Different Harped Strand Locations at Girder Ends"),m_bUseDifferentHarpedGridAtEnds,rOther.m_bUseDifferentHarpedGridAtEnds,_T("Checked"),_T("Unchecked")));
      }

      if ( m_StraightStrands != rOther.m_StraightStrands )
      {
         RETURN_ON_DIFFERENCE;
         vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Straight Strands Positions are different"),_T(""),_T("")));
      }
      if ( m_HarpedStrands != rOther.m_HarpedStrands  )
      {
         RETURN_ON_DIFFERENCE;
         vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Harped Strands Positions are different"),_T(""),_T("")));
      }
      if ( m_PermanentStrands != rOther.m_PermanentStrands )
      {
         RETURN_ON_DIFFERENCE;
         vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Permanent Strands Positions are different"),_T(""),_T("")));
      }

      if ( m_HPAdjustment != rOther.m_HPAdjustment )
      {
         RETURN_ON_DIFFERENCE;
         vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Harped strand adjustment at harping points are different"),_T(""),_T("")));
      }

      if ( m_EndAdjustment != rOther.m_EndAdjustment )
      {
         RETURN_ON_DIFFERENCE;
         vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Harped strand adjustment at girder ends are different"),_T(""),_T("")));
      }

      if ( m_StraightAdjustment != rOther.m_StraightAdjustment )
      {
         RETURN_ON_DIFFERENCE;
         vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Adjustable straight strand adjustment are different"),_T(""),_T("")));
      }
   }

   //
   // Temporary Strands Tab
   //
   if ( !bSplicedGirder )
   {
      if ( m_TemporaryStrands != rOther.m_TemporaryStrands )
      {
         RETURN_ON_DIFFERENCE;
         vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Temporary Strands Positions are different"),_T(""),_T("")));
      }
   }

   //
   // Flexure Design Tab (Debonding Tab for spliced girders)
   //
   if  (m_bCheckMaxDebondStrands != rOther.m_bCheckMaxDebondStrands ||
        (m_bCheckMaxDebondStrands && !::IsEqual(m_MaxDebondStrands,      rOther.m_MaxDebondStrands))                ||
        !::IsEqual(m_MaxDebondStrandsPerRow,rOther.m_MaxDebondStrandsPerRow)          ||
      m_MaxNumDebondedStrandsPerSection10orLess != rOther.m_MaxNumDebondedStrandsPerSection10orLess ||
        m_MaxNumDebondedStrandsPerSection != rOther.m_MaxNumDebondedStrandsPerSection ||
      m_bCheckMaxNumDebondedStrandsPerSection != rOther.m_bCheckMaxNumDebondedStrandsPerSection ||
        (m_bCheckMaxNumDebondedStrandsPerSection && !::IsEqual(m_MaxDebondedStrandsPerSection,rOther.m_MaxDebondedStrandsPerSection))  ||
      !::IsEqual(m_MinDebondLengthDB, rOther.m_MinDebondLengthDB) ||
      m_bCheckMinDebondLength != rOther.m_bCheckMinDebondLength ||
      (m_bCheckMinDebondLength && !::IsEqual(m_MinDebondLength, rOther.m_MinDebondLength)) ||
      !::IsEqual(m_MaxDebondLengthBySpanFraction, rOther.m_MaxDebondLengthBySpanFraction) ||
      !::IsEqual(m_MaxDebondLengthByHardDistance, rOther.m_MaxDebondLengthByHardDistance) ||
      m_bCheckDebondingSymmetry != rOther.m_bCheckDebondingSymmetry ||
      m_bCheckAdjacentDebonding != rOther.m_bCheckAdjacentDebonding ||
      m_bCheckDebondingInWebWidthProjections != rOther.m_bCheckDebondingInWebWidthProjections
      )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Debonding Strand Limits are different"),_T(""),_T("")));
   }

   if ( !bSplicedGirder )
   {
      if ( m_PrestressDesignStrategies != rOther.m_PrestressDesignStrategies )
      {
         RETURN_ON_DIFFERENCE;
         vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Prestress Design Stratigies are different"),_T(""),_T("")));
      }
   }

   //
   // Shear Design Tab
   //
   if ( !bSplicedGirder )
   {
      if ( m_StirrupSizeBarComboColl != rOther.m_StirrupSizeBarComboColl ||
           m_AvailableBarSpacings.size() != rOther.m_AvailableBarSpacings.size() ||
           !std::equal(m_AvailableBarSpacings.begin(),m_AvailableBarSpacings.end(),rOther.m_AvailableBarSpacings.begin(),EqualDoublePred) ||
           !::IsEqual(m_MaxSpacingChangeInZone, rOther.m_MaxSpacingChangeInZone) ||
           !::IsEqual(m_MaxShearCapacityChangeInZone, rOther.m_MaxShearCapacityChangeInZone) ||
           m_MinZoneLengthSpacings != rOther.m_MinZoneLengthSpacings ||
           !::IsEqual(m_MinZoneLengthLength, rOther.m_MinZoneLengthLength) ||
           m_DoExtendBarsIntoDeck != rOther.m_DoExtendBarsIntoDeck ||
           m_DoBarsActAsConfinement != rOther.m_DoBarsActAsConfinement ||
           m_LongShearCapacityIncreaseMethod != rOther.m_LongShearCapacityIncreaseMethod ||
         !::IsEqual(m_InterfaceShearWidthReduction,rOther.m_InterfaceShearWidthReduction)
         )
      {
         RETURN_ON_DIFFERENCE;
         vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Shear Design Parameters are different"),_T(""),_T("")));
      }
   }

   //
   // Harping Points Tab
   //
   if ( !bSplicedGirder )
   {
      if ( !::IsEqual(m_HarpingPointLocation,rOther.m_HarpingPointLocation) ||
         (m_bMinHarpingPointLocation != rOther.m_bMinHarpingPointLocation || (m_bMinHarpingPointLocation == true && !::IsEqual(m_MinHarpingPointLocation,rOther.m_MinHarpingPointLocation))) ||
           m_HarpPointReference != rOther.m_HarpPointReference ||
           m_HarpPointMeasure != rOther.m_HarpPointMeasure )
      {
         RETURN_ON_DIFFERENCE;
         vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Harping Point locations are different"),_T(""),_T("")));
      }
   }

   //
   // Long. Reinforcement Tab
   //
   if ( !bSplicedGirder && bCompareSeedValues )
   {
      if ( m_LongitudinalBarType != rOther.m_LongitudinalBarType ||
           m_LongitudinalBarGrade != rOther.m_LongitudinalBarGrade ||
           m_LongSteelInfo != rOther.m_LongSteelInfo )
      {
         RETURN_ON_DIFFERENCE;
         vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Long. Reinforcement seed values are different"),_T(""),_T("")));
      }
   }

   //
   // Trans. Reinforcement Tab
   //
   if ( !bSplicedGirder && bCompareSeedValues)
   {
      if (m_ShearData != rOther.m_ShearData)
      {
         RETURN_ON_DIFFERENCE;
         vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Transv. Reinforcement seed values are different"),_T(""),_T("")));
      }
   }

   //
   // Haunch and Camber Tab
   //
   if ( !::IsEqual(m_MinFilletValue, rOther.m_MinFilletValue) ||
        (m_DoCheckMinHaunchAtBearingLines != rOther.m_DoCheckMinHaunchAtBearingLines || (m_DoCheckMinHaunchAtBearingLines == true && !::IsEqual(m_MinHaunchAtBearingLines, rOther.m_MinHaunchAtBearingLines))) ||
        !::IsEqual(m_ExcessiveSlabOffsetWarningTolerance,rOther.m_ExcessiveSlabOffsetWarningTolerance)
        )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Haunch Design Parameters are different"),_T(""),_T("")));
   }

   if ( !bSplicedGirder )
   {
      if ( m_CamberMultipliers != rOther.m_CamberMultipliers )
      {
         RETURN_ON_DIFFERENCE;
         vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Camber Multipliers are different"),_T(""),_T("")));
      }
   }

   if ( !::IsEqual(m_DragCoefficient,rOther.m_DragCoefficient) )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Drag Coefficients are different"),_T(""),_T("")));
   }

   if (!::IsEqual(m_PrecamberLimit, rOther.m_PrecamberLimit))
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Precamber limits are different"), _T(""), _T("")));
   }

   if (m_DoReportBearingElevationsAtGirderEdges != rOther.m_DoReportBearingElevationsAtGirderEdges)
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Options to Report Bearing Elevations at Girder Edges are different"), _T(""), _T("")));
   }

   //
   // Diaphragms Tab
   //
   if ( m_DiaphragmLayoutRules != rOther.m_DiaphragmLayoutRules )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Diaphragm Layout Rules are different"),_T(""),_T("")));
   }

   if (considerName &&  GetName() != rOther.GetName() )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Name"),GetName().c_str(),rOther.GetName().c_str()));
   }

   return vDifferences.size() == 0 ? true : false;
}

void GirderLibraryEntry::ValidateData(GirderLibraryEntry::GirderEntryDataErrorVec* pvec)
{
   pvec->clear();

  CComQIPtr<ISplicedBeamFactory,&IID_ISplicedBeamFactory> splicedBeamFactory(m_pBeamFactory);

  if ( splicedBeamFactory )
  {
     //  nothing extra to validate for spliced girders
     return;
  }

   // check if strands are in girder and other possible issues
   CComPtr<IGirderSection> gdrSection;
   m_pBeamFactory->CreateGirderSection(nullptr,DUMMY_AGENT_ID,m_Dimensions,-1.0,-1.0,&gdrSection);

   Float64 end_increment = this->IsVerticalAdjustmentAllowedEnd() ? this->GetEndStrandIncrement() : -1.0;
   Float64 hp_increment  = this->IsVerticalAdjustmentAllowedHP()  ? this->GetHPStrandIncrement() : -1.0;

   pgsTypes::FaceType endTopFace, endBottomFace;
   Float64 endTopLimit, endBottomLimit;
   this->GetEndAdjustmentLimits(&endTopFace, &endTopLimit, &endBottomFace, &endBottomLimit);

   IBeamFactory::BeamFace etf = endTopFace==pgsTypes::BottomFace ? IBeamFactory::BeamBottom : IBeamFactory::BeamTop;
   IBeamFactory::BeamFace ebf = endBottomFace==pgsTypes::BottomFace ? IBeamFactory::BeamBottom : IBeamFactory::BeamTop;

   pgsTypes::FaceType hpTopFace, hpBottomFace;
   Float64 hpTopLimit, hpBottomLimit;
   this->GetHPAdjustmentLimits(&hpTopFace, &hpTopLimit, &hpBottomFace, &hpBottomLimit);
 
   IBeamFactory::BeamFace htf = hpTopFace==pgsTypes::BottomFace ? IBeamFactory::BeamBottom : IBeamFactory::BeamTop;
   IBeamFactory::BeamFace hbf = hpBottomFace==pgsTypes::BottomFace ? IBeamFactory::BeamBottom : IBeamFactory::BeamTop;

   CComPtr<IStrandMover> strand_mover;
   m_pBeamFactory->CreateStrandMover(m_Dimensions, -1,
                                     etf, endTopLimit, ebf, endBottomLimit,
                                     htf, hpTopLimit,  hbf, hpBottomLimit,
                                     end_increment, hp_increment, &strand_mover);


   // Need separate strand mover for all-straight adjustable strand case
   CComPtr<IStrandMover> straight_strand_mover;
   bool do_all_straight = (IsVerticalAdjustmentAllowedStraight() && m_AdjustableStrandType != pgsTypes::asHarped ? true : false);
   if (do_all_straight)
   {
      Float64 straight_increment = this->GetStraightStrandIncrement();

      pgsTypes::FaceType straightTopFace, straightBottomFace;
      Float64 straightTopLimit, straightBottomLimit;
      this->GetStraightAdjustmentLimits(&straightTopFace, &straightTopLimit, &straightBottomFace, &straightBottomLimit);

      IBeamFactory::BeamFace strtf = straightTopFace==pgsTypes::BottomFace ? IBeamFactory::BeamBottom : IBeamFactory::BeamTop;
      IBeamFactory::BeamFace strbf = straightBottomFace==pgsTypes::BottomFace ? IBeamFactory::BeamBottom : IBeamFactory::BeamTop;

      m_pBeamFactory->CreateStrandMover(m_Dimensions, -1,
                                        strtf, straightTopLimit, strbf, straightBottomLimit,
                                        strtf, straightTopLimit, strbf, straightBottomLimit,
                                        straight_increment, straight_increment, &straight_strand_mover);
   }


   Float64 height;
   gdrSection->get_OverallHeight(&height);

   if (IsZero(height))
   {
      pvec->push_back(GirderEntryDataError(GirderHeightIsZero,
         _T("The height of the girder must be greater than zero.")));
      // this will cause so many problems that we might as well just return here
      return;
   }

   Float64 lftwidth, rgtwidth;
   gdrSection->get_BottomWidth(&lftwidth,&rgtwidth);
   if (IsZero(lftwidth + rgtwidth))
   {
      pvec->push_back(GirderEntryDataError(BottomFlangeWidthIsZero,
         _T("The width of the girder bottom flange must be greater than zero.")));
      return;
   }

   gdrSection->get_TopWidth(&lftwidth,&rgtwidth);
   if (IsZero(lftwidth + rgtwidth))
   {
      pvec->push_back(GirderEntryDataError(TopFlangeWidthIsZero,
         _T("The width of the girder must be greater than zero.")));
      return;
   }

   CComQIPtr<IShape> shape(gdrSection);
   ATLASSERT(shape != nullptr); // girder section must support the shape interface

   // establish top and bottom bounds
   Float64 max_y_loc = height;
   Float64 min_y_loc = 0.0;

   // a utility point
   CComPtr<IPoint2d> point;
   point.CoCreateInstance(CLSID_Point2d);

   // loop around global strand sequence
   // straight strands
   Int32 total_num=1;
   Int32 end_idx=0;
   PermanentStrandCollection::iterator g_iter(m_PermanentStrands.begin());
   PermanentStrandCollection::iterator g_iter_end(m_PermanentStrands.end());
   for ( ; g_iter != g_iter_end; g_iter++ )
   {
      const PermanentStrand& permStrand = *g_iter;

      if (permStrand.m_StrandType==stStraight)
      {
         // straight strands
         const StraightStrandLocation& strandLocation = m_StraightStrands[permStrand.m_GridIdx];

         point->Move(strandLocation.m_Xstart, strandLocation.m_Ystart - height);

         VARIANT_BOOL bPointInShape;
         shape->PointInShape( point,&bPointInShape );
         if ( bPointInShape == VARIANT_FALSE )
         {
            std::_tostringstream os;
            os << _T("Straight strand #")<<total_num<<_T(" is outside of the girder section");
            pvec->push_back(GirderEntryDataError(StraightStrandOutsideOfGirder, os.str(), total_num));
         }

         total_num += (0 < strandLocation.m_Xstart || 0 < strandLocation.m_Xend ? 2 : 1);
      }
      else if (permStrand.m_StrandType==stAdjustable)
      {
         // harped strands at HP
         const HarpedStrandLocation& strandLocation = m_HarpedStrands[permStrand.m_GridIdx];

         bool cantBeHarped = this->m_AdjustableStrandType!=pgsTypes::asHarped;

         point->Move(strandLocation.m_Xhp, strandLocation.m_Yhp - height);

         VARIANT_BOOL bPointInShape;
         shape->PointInShape( point,&bPointInShape );
         if ( bPointInShape == VARIANT_FALSE )
         {
            std::_tostringstream os;
            if (cantBeHarped)
            {
               os << LABEL_HARP_TYPE(cantBeHarped)<<_T(" strand #")<<total_num<<_T(" at is outside of the girder section");
            }
            else
            {
               os << LABEL_HARP_TYPE(cantBeHarped)<<_T(" strand #")<<total_num<<_T(" at harping point is outside of the girder section");
            }

            pvec->push_back(GirderEntryDataError(HarpedStrandOutsideOfGirder, os.str(), total_num));
         }


         VARIANT_BOOL is_within;
         strand_mover->TestHpStrandLocation(etStart,height,strandLocation.m_Xhp, strandLocation.m_Yhp-height, 0.0, &is_within);
         if (is_within!=VARIANT_TRUE)
         {
            std::_tostringstream os;
            if (cantBeHarped)
            {
               os << LABEL_HARP_TYPE(cantBeHarped)<<_T(" strand #")<<total_num<<_T(" must be located within vertical adjustment bounds and lie within the thinnest portion of a web");
            }
            else
            {
               os << LABEL_HARP_TYPE(cantBeHarped)<<_T(" strand #")<<total_num<<_T(" at harping point must be located within vertical adjustment bounds and lie within the thinnest portion of a web");
            }

            pvec->push_back(GirderEntryDataError(HarpedStrandOutsideOfGirder, os.str(), total_num));
         }

         // All straight case, if pertinant
         if (do_all_straight)
         {
            straight_strand_mover->TestHpStrandLocation(etStart,height,strandLocation.m_Xhp, strandLocation.m_Yhp-height, 0.0, &is_within);
            if (is_within!=VARIANT_TRUE)
            {
               std::_tostringstream os;
               os << LABEL_HARP_TYPE(true)<<_T(" strand #")<<total_num<<_T(" must be located within vertical straight strand adjustment bounds and lie within the thinnest portion of a web");

               pvec->push_back(GirderEntryDataError(HarpedStrandOutsideOfGirder, os.str(), total_num));
            }
         }

         // next check zero value if odd number of strands is allowed
         if (this->OddNumberOfHarpedStrands())
         {
            // must have all X>0 for odd number of strands case
            if ( IsZero(strandLocation.m_Xhp) && IsZero(strandLocation.m_Xstart) && IsZero(strandLocation.m_Xend) )
            {
               std::_tostringstream os;
               if (cantBeHarped)
               {
                  os << LABEL_HARP_TYPE(cantBeHarped)<<_T(" strand #")<<total_num<<_T(" has a zero X value. This is cannot be if odd number of harped strands is allowed.");
               }
               else
               {
                  os << LABEL_HARP_TYPE(cantBeHarped)<<_T(" strand #")<<total_num<<_T(" has zero X value at HP and End. This cannot be if odd number of harped strands is allowed.");
               }
               pvec->push_back(GirderEntryDataError(HarpedStrandOutsideOfGirder, os.str(), total_num));
            }

            // check locations of zero X
            point->Move(0.0, strandLocation.m_Yhp-height);

            shape->PointInShape( point,&bPointInShape );
            if ( bPointInShape == VARIANT_FALSE )
            {
               std::_tostringstream os;
               if (cantBeHarped)
               {
                  os << _T("Odd ")<<LABEL_HARP_TYPE(cantBeHarped)<<_T(" strand #")<<total_num<<_T(" is outside of the girder section. Disable odd strands");
               }
               else
               {
                  os << _T("Odd ")<<LABEL_HARP_TYPE(cantBeHarped)<<_T(" strand #")<<total_num<<_T(" at harping point is outside of the girder section. Disable odd strands");
               }

               pvec->push_back(GirderEntryDataError(HarpedStrandOutsideOfGirder, os.str(), total_num));
            }

            strand_mover->TestHpStrandLocation(etStart,height, 0.0, strandLocation.m_Yhp-height, 0.0, &is_within);
            if (is_within!=VARIANT_TRUE)
            {
               std::_tostringstream os;
               if (cantBeHarped)
               {
                  os << _T("Odd ")<<LABEL_HARP_TYPE(cantBeHarped)<<_T(" strand #")<<total_num<<_T(" must be within offset bounds and lie within the thinnest portion of a web. Disable odd strands");
               }
               else
               {
                  os << _T("Odd ")<<LABEL_HARP_TYPE(cantBeHarped)<<_T(" strand #")<<total_num<<_T(" at harping point must be within offset bounds and lie within the thinnest portion of a web. Disable odd strands");
               }

               pvec->push_back(GirderEntryDataError(HarpedStrandOutsideOfGirder, os.str(), total_num));
            }
         }

         // Harped strands at ends
         if (m_bUseDifferentHarpedGridAtEnds && !cantBeHarped)
         {
            // start
            point->Move(strandLocation.m_Xstart, strandLocation.m_Ystart-height);

            VARIANT_BOOL bPointInShape;
            shape->PointInShape( point,&bPointInShape );
            if ( bPointInShape == VARIANT_FALSE )
            {
               std::_tostringstream os;
               os << LABEL_HARP_TYPE(cantBeHarped)<<_T(" strand #")<<total_num<<_T(" at girder end is outside of the girder section");
               pvec->push_back(GirderEntryDataError(HarpedStrandOutsideOfGirder, os.str(), total_num));
            }


            strand_mover->TestEndStrandLocation(etStart,height,strandLocation.m_Xstart, strandLocation.m_Ystart-height, 0.0, &is_within);
            if (is_within!=VARIANT_TRUE)
            {
               std::_tostringstream os;
               os << LABEL_HARP_TYPE(cantBeHarped)<<_T(" strand #")<<total_num<<_T(" at girder end must be within offset bounds and lie within the thinnest portion of a web");
               pvec->push_back(GirderEntryDataError(HarpedStrandOutsideOfGirder, os.str(), total_num));
            }

            // next check zero value if odd number of strands is allowed
            if (this->OddNumberOfHarpedStrands())
            {
               point->Move(0.0, strandLocation.m_Ystart-height);

               bool didMsg = false;
               shape->PointInShape( point,&bPointInShape );
               if ( bPointInShape == VARIANT_FALSE )
               {
                  std::_tostringstream os;
                  os << _T("Odd")<<LABEL_HARP_TYPE(cantBeHarped)<<_T(" strand #")<<total_num<<_T(" at girder end is outside of the girder section. Disable odd strands");
                  pvec->push_back(GirderEntryDataError(HarpedStrandOutsideOfGirder, os.str(), total_num));
                  didMsg = true;
               }

               strand_mover->TestEndStrandLocation(etStart,height,0.0, strandLocation.m_Ystart-height, 0.0, &is_within);
               if (is_within!=VARIANT_TRUE)
               {
                  std::_tostringstream os;
                  os << _T("Odd ")<<LABEL_HARP_TYPE(cantBeHarped)<<_T(" strand #")<<total_num<<_T(" at girder end must be within offset bounds and lie within the thinnest portion of a web. Disable odd strands");
                  pvec->push_back(GirderEntryDataError(HarpedStrandOutsideOfGirder, os.str(), total_num));
                  didMsg = true;
               }

               if (didMsg)
               {
                  // Coerce number of harped strands should probably not be on for multi-web sections
                  WebIndexType wc;
                  gdrSection->get_WebCount(&wc);
                  if (wc > 1)
                  {
                     pvec->push_back(GirderEntryDataError(HarpedStrandOutsideOfGirder, _T("\"Coerce Odd Number of Harped Strands\" is enabled and this girder has more than one web - Could this be the problem?"), total_num));
                  }
               }
            }

            // NOTE: The girder shape is for the start of the girder so we can't really
            // check the strands at the end... let's just assume they are ok for now
            
            // TODO: Validate strands at end of girder

            //// end
            //point->Move(strandLocation.m_Xend, strandLocation.m_Yend);

            //shape->PointInShape( point,&bPointInShape );
            //if ( bPointInShape == VARIANT_FALSE )
            //{
            //   std::_tostringstream os;
            //   os << _T("Harped strand #")<<total_num<<_T(" at girder end is outside of the girder section");
            //   pvec->push_back(GirderEntryDataError(HarpedStrandOutsideOfGirder, os.str(), total_num));
            //}


            //strand_mover->TestEndStrandLocation(strandLocation.m_Xend, strandLocation.m_Yend, 0.0, &is_within);
            //if (is_within!=VARIANT_TRUE)
            //{
            //   std::_tostringstream os;
            //   os << _T("Harped strand #")<<total_num<<_T(" at girder end must be within offset bounds and lie within the thinnest portion of a web");
            //   pvec->push_back(GirderEntryDataError(HarpedStrandOutsideOfGirder, os.str(), total_num));
            //}

            //// next check zero value if odd number of strands is allowed
            //if (this->OddNumberOfHarpedStrands())
            //{
            //   point->Move(0.0, strandLocation.m_Yend);

            //   shape->PointInShape( point,&bPointInShape );
            //   if ( bPointInShape == VARIANT_FALSE )
            //   {
            //      std::_tostringstream os;
            //      os << _T("Odd Harped strand #")<<total_num<<_T(" at girder end is outside of the girder section. Disable odd strands");
            //      pvec->push_back(GirderEntryDataError(HarpedStrandOutsideOfGirder, os.str(), total_num));
            //   }

            //   strand_mover->TestEndStrandLocation(0.0, strandLocation.m_Yend, 0.0, &is_within);
            //   if (is_within!=VARIANT_TRUE)
            //   {
            //      std::_tostringstream os;
            //      os << _T("Odd Harped strand #")<<total_num<<_T(" at girder end must be within offset bounds and lie within the thinnest portion of a web. Disable odd strands");
            //      pvec->push_back(GirderEntryDataError(HarpedStrandOutsideOfGirder, os.str(), total_num));
            //   }
            //}
         }

         total_num += ( (0<strandLocation.m_Xhp || 0<strandLocation.m_Xstart || 0<strandLocation.m_Xend) ? 2 : 1);
      }
      else
      {
         // temporary strands
         const StraightStrandLocation& strandLocation = m_TemporaryStrands[permStrand.m_GridIdx];

         point->Move(strandLocation.m_Xstart, strandLocation.m_Ystart-height);

         VARIANT_BOOL bPointInShape;
         shape->PointInShape( point,&bPointInShape );
         if ( bPointInShape == VARIANT_FALSE )
         {
            std::_tostringstream os;
            os << _T("Temporary strand #")<<total_num<<_T(" is outside of the girder section");
            pvec->push_back(GirderEntryDataError(TemporaryStrandOutsideOfGirder, os.str(), total_num));
         }

         total_num += (0 < strandLocation.m_Xstart || 0 < strandLocation.m_Xend ? 2 : 1);
      }
   }

   // bundle adjustments
   if (this->m_AdjustableStrandType!=pgsTypes::asStraight)
   {
      if(!this->IsVerticalAdjustmentAllowedEnd() && 
         !this->IsVerticalAdjustmentAllowedHP() &&
         !m_bUseDifferentHarpedGridAtEnds)
      {
         pvec->push_back(GirderEntryDataError(BundleAdjustmentGtIncrement,
            _T("Harp bundle vertical adjustment is disabled both at girder ends and harping points. This means that the strands cannot be harped. You must enable an adjustment.")));
      }
   }

   if (end_increment>height/2.0)
   {
      pvec->push_back(GirderEntryDataError(BundleAdjustmentGtIncrement,
         _T("Harp bundle increment value at HP cannot be greater than half of the girder depth")));
   }

   // strand adjustment at end of girder
   if (hp_increment>height/2.0)
   {
      pvec->push_back(GirderEntryDataError(EndAdjustmentGtIncrement,
         _T("Upward adjustment increment value at end of girder cannot be greater than half of the girder depth")));
   }

   if (this->m_AdjustableStrandType!=pgsTypes::asHarped)
   {
      Float64 straight_increment = this->GetStraightStrandIncrement();
      if (straight_increment > height/2.0)
      {
         pvec->push_back(GirderEntryDataError(EndAdjustmentGtIncrement,
            _T("Adjustment increment value at for straight adjustable strands cannot be greater than half of the girder depth")));
      }
   }


   // stirrups and shear zones
   ZoneIndexType num=1;
   ZoneIndexType size = m_ShearData.ShearZones.size();
   for(CShearData2::ShearZoneIterator its=m_ShearData.ShearZones.begin(); its!=m_ShearData.ShearZones.end(); its++)
   {
      // last zone has infinite length 
      if (num<size)
      {
         if(IsZero((*its).ZoneLength))
         {
            std::_tostringstream os;
            os << _T("The length of shear zone #")<<num<<_T(" must be greater than zero.");
            pvec->push_back(GirderEntryDataError(ZoneLengthIsZero,os.str(),num));
         }
      }

      if(IsZero((*its).BarSpacing) && ((*its).VertBarSize != WBFL::Materials::Rebar::Size::bsNone || (*its).ConfinementBarSize != WBFL::Materials::Rebar::Size::bsNone))
      {
         std::_tostringstream os;
         os << _T("The stirrup spacing in shear zone #")<<num<<_T(" must be greater than zero because stirrups exist.");
         pvec->push_back(GirderEntryDataError(StirrupSpacingIsZero,os.str(),num));
      }

      num++;
   }

   // long bars
   point.Release(); // I think it is already nullptr, but do it again here just to be sure
   point.CoCreateInstance(CLSID_Point2d); // recycle point object
   num=1;
   for(LongSteelInfoVec::iterator itl=m_LongSteelInfo.begin(); itl!=m_LongSteelInfo.end(); itl++)
   {
      if ((*itl).Cover==0)
      {
         std::_tostringstream os;
         os << _T("The cover for long. rebar row #")<<num<<_T(" must be greater than zero.");
         pvec->push_back(GirderEntryDataError(BarCoverIsZero,os.str(),num));
      }

      if ((*itl).NumberOfBars==0)
      {
         std::_tostringstream os;
         os << _T("The number of bars in long. rebar row #")<<num<<_T(" must be greater than zero.");
         pvec->push_back(GirderEntryDataError(NumberOfBarsIsZero,os.str(),num));
      }

      if ( 1 < (*itl).NumberOfBars && (*itl).BarSpacing==0)
      {
         std::_tostringstream os;
         os << _T("The bar spacing in long. rebar row #")<<num<<_T(" must be greater than zero.");
         pvec->push_back(GirderEntryDataError(BarSpacingIsZero,os.str(),num));
      }

      // make sure bars are inside of girder - use shape symmetry
      WBFL::Geometry::Point2d testpnt;
      testpnt.X() = (*itl).BarSpacing * ((*itl).NumberOfBars-1)/2.;
      if ((*itl).Face==pgsTypes::BottomFace)
      {
         testpnt.Y() = (*itl).Cover;
      }
      else
      {
         testpnt.Y() = height-(*itl).Cover;
      }

      point->Move(testpnt.X(),testpnt.Y()-height);
      VARIANT_BOOL bPointInShape;
      shape->PointInShape( point,&bPointInShape );
      if ( bPointInShape == VARIANT_FALSE )
      {
         std::_tostringstream os;
         os << _T("Longitudinal rebar row #")<<num<<_T(" is outside of the girder section");
         pvec->push_back(GirderEntryDataError(LongitudinalRebarOutsideOfGirder, os.str(), num));
      }

      num++;
   }

   // Design Algorithm strategies
   bool cant_straight = pgsTypes::asHarped   ==  GetAdjustableStrandType() && IsDifferentHarpedGridAtEndsUsed();
   bool cant_harp     = pgsTypes::asStraight ==  GetAdjustableStrandType();
   bool cant_debond   = !CanDebondStraightStrands();

   // NOTE: It is ok to have a harped adjustable strategy without harped strands. This would be the case of straight only with no adjustable straight strands
   // or a straight/harped girder that doesn't have any harped strands defined.

   // Find any incompatible strategies and remove them
   PrestressDesignStrategyConstIterator it = m_PrestressDesignStrategies.begin();
   while(it != m_PrestressDesignStrategies.end())
   {
      if ( cant_straight && (dtDesignFullyBonded==it->m_FlexuralDesignType || dtDesignFullyBondedRaised==it->m_FlexuralDesignType))
      {
         pvec->push_back(GirderEntryDataError(DesignAlgorithmStrategyMismatch,
                         _T("An all straight bonded strand flexural design algorithm strategy is selected and adjustable strands can only be harped - The strategy has been removed.") ));

         it = m_PrestressDesignStrategies.erase(it);
         continue;
      }
      else if ( cant_harp && dtDesignForHarping==it->m_FlexuralDesignType )
      {
         pvec->push_back(GirderEntryDataError(DesignAlgorithmStrategyMismatch,
                         _T("A harped strand flexural design algorithm strategy is selected and harped strands are not allowed - The strategy has been removed.") ));

         it = m_PrestressDesignStrategies.erase(it);
         continue;
      }
      else if ( cant_debond && (dtDesignForDebonding==it->m_FlexuralDesignType ||
                           dtDesignForDebondingRaised==it->m_FlexuralDesignType))
      {
         pvec->push_back(GirderEntryDataError(DesignAlgorithmStrategyMismatch,
                         _T("A debonded strand flexural design algorithm strategy is selected and no debondable strands exist - The strategy has been removed.") ));

         it = m_PrestressDesignStrategies.erase(it);
         continue;
      }

      it++;
   }

   if (m_PrestressDesignStrategies.size()==0)
   {
      pvec->push_back(GirderEntryDataError(DesignAlgorithmStrategyMismatch,
                      _T("At least one flexural design algorithm strategy must be selected - Please go to the Flexural Design tab and select a strategy") ));
   }
}

bool GirderLibraryEntry::OddNumberOfHarpedStrands() const
{
   return m_bOddNumberOfHarpedStrands;
}

void GirderLibraryEntry::EnableOddNumberOfHarpedStrands(bool bEnable)
{
   m_bOddNumberOfHarpedStrands = bEnable;
}

pgsTypes::AdjustableStrandType GirderLibraryEntry::GetAdjustableStrandType() const
{
   return m_AdjustableStrandType;
}

void GirderLibraryEntry::SetAdjustableStrandType(pgsTypes::AdjustableStrandType type)
{
   m_AdjustableStrandType = type;
}

//======================== ACCESS     =======================================
void GirderLibraryEntry::SetBeamFactory(IBeamFactory* pFactory)
{
   if ( pFactory == nullptr )
   {
      return;
   }

   m_pBeamFactory = pFactory;

   // Clear out old dimensions
   m_Dimensions.clear();

   // Seed the dimension container
   const auto& names = m_pBeamFactory->GetDimensionNames();
   auto name_iter = names.begin();
   auto name_end = names.end();

   const auto& dims = m_pBeamFactory->GetDefaultDimensions();
   auto dim_iter = dims.begin();
   auto dim_end = dims.end();

   ATLASSERT( dims.size() == names.size() );

   for ( ; name_iter != name_end && dim_iter != dim_end; name_iter++, dim_iter++ )
   {
      const auto& name = *name_iter;
      Float64 value = *dim_iter;
      AddDimension(name,value);
   }

   CComQIPtr<ISplicedBeamFactory,&IID_ISplicedBeamFactory> splicedBeamFactory(m_pBeamFactory);
   if ( splicedBeamFactory && splicedBeamFactory->SupportsVariableDepthSection() )
   {
      m_bSupportsVariableDepthSection = true;
   }
   else
   {
      m_bSupportsVariableDepthSection = false;
   }
}

void GirderLibraryEntry::GetBeamFactory(IBeamFactory** ppFactory) const
{
   (*ppFactory) = m_pBeamFactory;
   (*ppFactory)->AddRef();
}

std::_tstring GirderLibraryEntry::GetGirderName() const
{
   return m_pBeamFactory->GetName(); // name of the general girder type (e.g. WSDOT U-Beam), not to be confused with a library entry name ("U55G6")
}

std::_tstring GirderLibraryEntry::GetGirderFamilyName() const
{
   return m_pBeamFactory->GetGirderFamilyName();
}

const GirderLibraryEntry::Dimensions& GirderLibraryEntry::GetDimensions() const
{
   return m_Dimensions;
}

Float64 GirderLibraryEntry::GetDimension(const std::_tstring& name) const
{
   Dimensions::const_iterator iter;
   for ( iter = m_Dimensions.begin(); iter != m_Dimensions.end(); iter++ )
   {
      const Dimension& dim = *iter;
      if ( name == dim.first )
      {
         return dim.second;
      }
   }

   ATLASSERT(false); // should never get here
   return -99999;
}

void GirderLibraryEntry::AddDimension(const std::_tstring& name,Float64 value)
{
   m_Dimensions.push_back(Dimension(name.c_str(),value));
}

void GirderLibraryEntry::SetDimension(const std::_tstring& name,Float64 value,bool bAdjustStrands)
{
   Float64 oldHeight[2];
   oldHeight[pgsTypes::metStart] = GetBeamHeight(pgsTypes::metStart);
   oldHeight[pgsTypes::metEnd]   = GetBeamHeight(pgsTypes::metEnd);

   Dimensions::iterator iter;
   for ( iter = m_Dimensions.begin(); iter != m_Dimensions.end(); iter++ )
   {
      Dimension& dim = *iter;
      if ( name == dim.first )
      {
         dim.second = value;
         break;
      }
   }

   ATLASSERT( iter != m_Dimensions.end() ); // should never get to the end

   Float64 newHeight[2];
   newHeight[pgsTypes::metStart] = GetBeamHeight(pgsTypes::metStart);
   newHeight[pgsTypes::metEnd]   = GetBeamHeight(pgsTypes::metEnd);

   if ( (!::IsEqual(oldHeight[pgsTypes::metStart],newHeight[pgsTypes::metStart]) ||
         !::IsEqual(oldHeight[pgsTypes::metEnd],  newHeight[pgsTypes::metEnd]))  &&
         bAdjustStrands )
   {
      Float64 deltaY[2];
      deltaY[pgsTypes::metStart] = newHeight[pgsTypes::metStart] - oldHeight[pgsTypes::metStart];
      deltaY[pgsTypes::metEnd]   = newHeight[pgsTypes::metEnd]   - oldHeight[pgsTypes::metEnd];

      HarpedStrandIterator hpIter;
      for ( hpIter = m_HarpedStrands.begin(); hpIter != m_HarpedStrands.end(); hpIter++ )
      {
         HarpedStrandLocation& strandLocation = *hpIter;
         strandLocation.m_Ystart += deltaY[pgsTypes::metStart];
         strandLocation.m_Yend   += deltaY[pgsTypes::metEnd];
      }

      StraightStrandIterator tempIter;
      for ( tempIter = m_TemporaryStrands.begin(); tempIter != m_TemporaryStrands.end(); tempIter++ )
      {
         StraightStrandLocation& strandLocation = *tempIter;
         strandLocation.m_Ystart += deltaY[pgsTypes::metStart];
         strandLocation.m_Yend   += deltaY[pgsTypes::metEnd];
      }
   }
}

void GirderLibraryEntry::EnableVariableDepthSection(bool bEnable)
{
   m_bIsVariableDepthSectionEnabled = bEnable;
}

bool GirderLibraryEntry::IsVariableDepthSectionEnabled() const
{
   if ( m_bSupportsVariableDepthSection )
   {
      return m_bIsVariableDepthSectionEnabled;
   }
   else
   {
      return false;
   }
}

void GirderLibraryEntry::ClearAllStrands()
{
   m_StraightStrands.clear();
   m_HarpedStrands.clear();
   m_TemporaryStrands.clear();
   m_PermanentStrands.clear();
}

std::vector<GirderLibraryEntry::StrandDefinitionType> GirderLibraryEntry::GetPermanentStrands() const
{
   // first need to determine if odd harped strands are possible
   bool allow_odd = false;
   if (OddNumberOfHarpedStrands())
   {
      if (0 < GetNumHarpedStrandCoordinates())
      {
         Float64 startx, starty, hpx, hpy, endx, endy;
         GetHarpedStrandCoordinates(0,&startx, &starty, &hpx, &hpy, &endx, &endy);
         if (0.0 < endx)
         {
            allow_odd = true;
         }
      }
   }

   GridIndexType nGridPositions = GetPermanentStrandGridSize();

   std::vector<StrandDefinitionType> permStrands;
   permStrands.reserve(2*nGridPositions);
   permStrands.push_back(ptStraight); // zero is always an option


   for (GridIndexType permStrandGridIdx = 0; permStrandGridIdx < nGridPositions; permStrandGridIdx++)
   {
      GirderLibraryEntry::psStrandType type;
      GridIndexType localGridIdx;
      GetGridPositionFromPermStrandGrid(permStrandGridIdx, &type, &localGridIdx);
      if (type == GirderLibraryEntry::stAdjustable)
      {
         Float64 startx, starty, hpx, hpy, endx, endy;
         GetHarpedStrandCoordinates(localGridIdx, &startx, &starty, &hpx, &hpy, &endx, &endy);
         if (0.0 < endx || 0.0 < hpx)
         {
            // two strand locations. if odd is allowed we can use both
            if (allow_odd)
            {
               permStrands.push_back(ptHarped);
               permStrands.push_back(ptHarped);
            }
            else
            {
               permStrands.push_back(ptNone);
               permStrands.push_back(ptHarped);
            }
         }
         else
         {
            permStrands.push_back(ptHarped);
         }
      }
      else if (type == GirderLibraryEntry::stStraight)
      {
         Float64 start_x, start_y, end_x, end_y;
         bool can_debond;
         GetStraightStrandCoordinates(localGridIdx, &start_x, &start_y, &end_x, &end_y, &can_debond);

         if (0.0 < start_x)
         {
            permStrands.push_back(ptNone);
            permStrands.push_back(ptStraight);
         }
         else
         {
            permStrands.push_back(ptStraight);
         }
      }
   }

   return permStrands;
}

StrandIndexType GirderLibraryEntry::GetNumStraightStrandCoordinates() const
{
   return m_StraightStrands.size();
}

void GirderLibraryEntry::GetStraightStrandCoordinates(GridIndexType ssGridIdx, Float64* Xstart, Float64* Ystart, Float64* Xend, Float64* Yend, bool* canDebond) const
{
   ATLASSERT(ssGridIdx < (GridIndexType)m_StraightStrands.size());

   const StraightStrandLocation& strandLocation = m_StraightStrands[ssGridIdx];

   *Xstart    = strandLocation.m_Xstart;
   *Ystart    = strandLocation.m_Ystart;
   *Xend      = strandLocation.m_Xend;
   *Yend      = strandLocation.m_Yend;
   *canDebond = strandLocation.m_bCanDebond;
}

StrandIndexType GirderLibraryEntry::AddStraightStrandCoordinates(Float64 Xstart, Float64 Ystart, Float64 Xend, Float64 Yend, bool canDebond)
{
   StraightStrandLocation strandLocation;
   strandLocation.m_Xstart     = Xstart;
   strandLocation.m_Ystart     = Ystart;
   strandLocation.m_Xend       = Xend;
   strandLocation.m_Yend       = Yend;
   strandLocation.m_bCanDebond = canDebond;

   m_StraightStrands.push_back(strandLocation);

   return m_StraightStrands.size();
}

StrandIndexType GirderLibraryEntry::GetMaxStraightStrands() const
{
   StrandIndexType cStrands=0;

   for (ConstStraightStrandIterator iter = m_StraightStrands.begin(); iter != m_StraightStrands.end(); iter++)
   {
      const StraightStrandLocation& strandLocation = *iter;

      cStrands += ( 0 < strandLocation.m_Xstart ? 2 : 1 );
   }

   return cStrands;
}

StrandIndexType GirderLibraryEntry::GetNumHarpedStrandCoordinates() const
{
   return m_HarpedStrands.size();
}

void GirderLibraryEntry::GetHarpedStrandCoordinates(GridIndexType hsGridIdx, Float64* Xstart,Float64* Ystart, Float64* Xhp, Float64* Yhp,Float64* Xend, Float64* Yend) const
{
   ATLASSERT( hsGridIdx < (GridIndexType)m_HarpedStrands.size());

   const HarpedStrandLocation& strandLocation = m_HarpedStrands[hsGridIdx];

   if (m_bUseDifferentHarpedGridAtEnds)
   {
      *Xstart = strandLocation.m_Xstart;
      *Ystart = strandLocation.m_Ystart;
      *Xhp    = strandLocation.m_Xhp;
      *Yhp    = strandLocation.m_Yhp;
      *Xend   = strandLocation.m_Xend;
      *Yend   = strandLocation.m_Yend;
   }
   else
   {
      // Data stored in end locations is not always reliably stored.
      // Force all coordinates to hp location
      Float64 x = strandLocation.m_Xhp;
      Float64 y = strandLocation.m_Yhp;

      *Xstart = x;
      *Ystart = y;
      *Xhp    = x;
      *Yhp    = y;
      *Xend   = x;
      *Yend   = y;
   }
}

StrandIndexType GirderLibraryEntry::AddHarpedStrandCoordinates(Float64 Xstart,Float64 Ystart, Float64 Xhp, Float64 Yhp,Float64 Xend, Float64 Yend)
{
   HarpedStrandLocation strandLocation;
   strandLocation.m_Xstart = Xstart;
   strandLocation.m_Ystart = Ystart;
   strandLocation.m_Xhp    = Xhp;
   strandLocation.m_Yhp    = Yhp;
   strandLocation.m_Xend   = Xend;
   strandLocation.m_Yend   = Yend;

   m_HarpedStrands.push_back(strandLocation);

   return m_HarpedStrands.size();
}

StrandIndexType GirderLibraryEntry::GetMaxHarpedStrands() const
{
   StrandIndexType cStrands = 0;
   for (ConstHarpedStrandIterator iter = m_HarpedStrands.begin(); iter != m_HarpedStrands.end(); iter++)
   {
      const HarpedStrandLocation& strandLocation = *iter;

      if (0.0 < strandLocation.m_Xhp)
      {
         cStrands += 2;
      }
      else if (m_bUseDifferentHarpedGridAtEnds && 0.0 < strandLocation.m_Xend)
      {
         cStrands += 2;
      }
      else
      {
         cStrands += 1;
      }
   }

   return cStrands;
}

StrandIndexType GirderLibraryEntry::GetNumTemporaryStrandCoordinates() const
{
   return m_TemporaryStrands.size();
}

void GirderLibraryEntry::GetTemporaryStrandCoordinates(GridIndexType tsGridIdx, Float64* Xstart, Float64* Ystart, Float64* Xend, Float64* Yend) const
{
   ATLASSERT(tsGridIdx < (GridIndexType)m_TemporaryStrands.size());

   const StraightStrandLocation& strandLocation = m_TemporaryStrands[tsGridIdx];

   *Xstart    = strandLocation.m_Xstart;
   *Ystart    = strandLocation.m_Ystart;
   *Xend      = strandLocation.m_Xend;
   *Yend      = strandLocation.m_Yend;
}

StrandIndexType GirderLibraryEntry::AddTemporaryStrandCoordinates(Float64 Xstart, Float64 Ystart, Float64 Xend, Float64 Yend)
{
   StraightStrandLocation strandLocation;
   strandLocation.m_Xstart = Xstart;
   strandLocation.m_Ystart = Ystart;
   strandLocation.m_Xend   = Xend;
   strandLocation.m_Yend   = Yend;

   m_TemporaryStrands.push_back(strandLocation);
   return m_TemporaryStrands.size();
}

StrandIndexType GirderLibraryEntry::GetMaxTemporaryStrands() const
{
   StrandIndexType cStrands=0;

   for (ConstStraightStrandIterator iter = m_TemporaryStrands.begin(); iter != m_TemporaryStrands.end(); iter++)
   {
      const StraightStrandLocation& strandLocation = *iter;

      cStrands += ( 0 < strandLocation.m_Xstart ? 2 : 1 );
   }

   return cStrands;
}

GridIndexType GirderLibraryEntry::GetPermanentStrandGridSize() const
{
   return m_PermanentStrands.size();
}

void GirderLibraryEntry::GetGridPositionFromPermStrandGrid(GridIndexType permStrandGridIdx, psStrandType* type, GridIndexType* gridIdx) const
{
   ATLASSERT(permStrandGridIdx < (GridIndexType)m_PermanentStrands.size() );
   
   const PermanentStrand& strand = m_PermanentStrands[permStrandGridIdx];
   *type    = strand.m_StrandType;
   *gridIdx = strand.m_GridIdx;
}

GridIndexType GirderLibraryEntry::AddStrandToPermStrandGrid(psStrandType type,  GridIndexType gridIdx)
{
   PermanentStrand strand;
   strand.m_StrandType = type;
   strand.m_GridIdx = gridIdx;

   m_PermanentStrands.push_back(strand);
   return m_PermanentStrands.size();
}

bool GirderLibraryEntry::GetPermStrandDistribution(StrandIndexType totalNumStrands, StrandIndexType* numStraight, StrandIndexType* numHarped) const
{
   if (totalNumStrands == 0)
   {
      *numStraight = 0;
      *numHarped   = 0;
      return true;
   }
   else
   {
      StrandIndexType np = 0;
      StrandIndexType ns = 0;
      StrandIndexType nh = 0;
      PermanentStrandCollection::const_iterator gs_it( m_PermanentStrands.begin() );
      PermanentStrandCollection::const_iterator gs_it_end( m_PermanentStrands.end() );
      for ( ; gs_it != gs_it_end; gs_it++ )
      {
         const PermanentStrand& strand = *gs_it;
         psStrandType type       = strand.m_StrandType;
         StrandIndexType gridIdx = strand.m_GridIdx;

         if (type == stStraight)
         {
            const StraightStrandLocation& strandLocation = m_StraightStrands[gridIdx];

            ns += (0.0 < strandLocation.m_Xstart ? 2 : 1);
         }
         else if (type == stAdjustable)
         {
            const HarpedStrandLocation& strandLocation = m_HarpedStrands[gridIdx];
            if (0.0 < strandLocation.m_Xhp)
            {
               nh += 2;
            }
            else if (m_bUseDifferentHarpedGridAtEnds && 0.0 < strandLocation.m_Xstart)
            {
               nh += 2;
            }
            else
            {
               nh += 1;
            }
         }
         else
         {
            ATLASSERT(false);
         }

         np = nh + ns;

         if (totalNumStrands <= np)
         {
            break;
         }
      }

      if (np == totalNumStrands)
      {
         *numStraight = ns;
         *numHarped   = nh;
         return true;
      }
      else
      {
         ATLASSERT(np-1 == totalNumStrands);

         // might be able to use odd harped strand
         if (0 < nh && m_bOddNumberOfHarpedStrands && nh%2==0 )
         {
            if (0.0 < m_HarpedStrands[0].m_Xhp || (m_bUseDifferentHarpedGridAtEnds && 0.0 < m_HarpedStrands[0].m_Xstart))
            {
               // can use odd harped strand
               *numStraight = ns;
               *numHarped   = nh-1;
               return true;
            }
            else
            {
               return false;
            }
         }
         else
         {
            *numStraight = INVALID_INDEX;
            *numHarped = INVALID_INDEX;
            return false;
         }
      }
   }
}

bool GirderLibraryEntry::IsValidNumberOfStraightStrands(StrandIndexType ns) const
{
   if (ns == 0)
   {
      return true;
   }
   else
   {
      StrandIndexType nStrands = 0;
      ConstStraightStrandIterator iter(m_StraightStrands.begin());
      ConstStraightStrandIterator end(m_StraightStrands.end());
      for ( ; iter != end; iter++ )
      {
         const StraightStrandLocation& strandLocation = *iter;

         nStrands++;

         if (0.0 < strandLocation.m_Xstart)
         {
            nStrands++;
         }

         if (ns <= nStrands)
         {
            break;
         }
      }

      return (nStrands == ns) ? true : false;
   }
}

bool GirderLibraryEntry::IsValidNumberOfHarpedStrands(StrandIndexType nh) const
{
   if (nh == 0)
   {
      return true;
   }
   else
   {

      if (m_bOddNumberOfHarpedStrands)
      {
         // any number less than max is valid
         StrandIndexType maxHarped = GetMaxHarpedStrands();
         return nh <= maxHarped;
      }
      else
      {
         StrandIndexType nHarped = 0;
         ConstHarpedStrandIterator iter(m_HarpedStrands.begin());
         ConstHarpedStrandIterator end(m_HarpedStrands.end());
         for ( ; iter != end; iter++ )
         {
            const HarpedStrandLocation& strandLocation = *iter;

            nHarped++;

            if (0.0 < strandLocation.m_Xhp || (m_bUseDifferentHarpedGridAtEnds && 0.0 < strandLocation.m_Xend))
            {
               nHarped++;
            }

            if (nh <= nHarped)
            {
               break;
            }
         }

         return (nh == nHarped ? true : false);
      }
   }
}

bool GirderLibraryEntry::IsValidNumberOfTemporaryStrands(StrandIndexType nt) const
{
   if (nt == 0)
   {
      return true;
   }
   else
   {
      StrandIndexType nStrands = 0;
      ConstStraightStrandIterator iter(m_TemporaryStrands.begin());
      ConstStraightStrandIterator end(m_TemporaryStrands.end());
      for ( ; iter != end; iter++ )
      {
         const StraightStrandLocation& strandLocation = *iter;

         nStrands++;

         if (0.0 < strandLocation.m_Xstart)
         {
            nStrands++;
         }

         if (nt <= nStrands)
         {
            break;
         }
      }

      return (nStrands == nt) ? true : false;
   }
}

void GirderLibraryEntry::UseDifferentHarpedGridAtEnds(bool d)
{
   m_bUseDifferentHarpedGridAtEnds = d;
}

bool GirderLibraryEntry::IsDifferentHarpedGridAtEndsUsed() const
{
   return m_bUseDifferentHarpedGridAtEnds;
}

void GirderLibraryEntry::AllowVerticalAdjustmentEnd(bool d)
{
   m_EndAdjustment.m_AllowVertAdjustment = d;
}

bool GirderLibraryEntry::IsVerticalAdjustmentAllowedEnd() const
{
   return m_EndAdjustment.m_AllowVertAdjustment;
}

void GirderLibraryEntry::AllowVerticalAdjustmentHP(bool d)
{
   m_HPAdjustment.m_AllowVertAdjustment = d;
}

bool GirderLibraryEntry::IsVerticalAdjustmentAllowedHP() const
{
   return m_HPAdjustment.m_AllowVertAdjustment;
}

void GirderLibraryEntry::AllowVerticalAdjustmentStraight(bool d)
{
   m_StraightAdjustment.m_AllowVertAdjustment = d;
}

bool GirderLibraryEntry::IsVerticalAdjustmentAllowedStraight() const
{
   return m_StraightAdjustment.m_AllowVertAdjustment;
}

void GirderLibraryEntry::SetEndStrandIncrement(Float64 d)
{
   m_EndAdjustment.m_StrandIncrement = d;
}

Float64 GirderLibraryEntry::GetEndStrandIncrement() const
{
   return m_EndAdjustment.m_AllowVertAdjustment ? m_EndAdjustment.m_StrandIncrement : -1.0;
}

void GirderLibraryEntry::SetHPStrandIncrement(Float64 d)
{
   m_HPAdjustment.m_StrandIncrement = d;
}

Float64 GirderLibraryEntry::GetHPStrandIncrement() const
{
   return m_HPAdjustment.m_AllowVertAdjustment ? m_HPAdjustment.m_StrandIncrement : -1.0;
}

void GirderLibraryEntry::SetStraightStrandIncrement(Float64 d)
{
   m_StraightAdjustment.m_StrandIncrement = d;
}

Float64 GirderLibraryEntry::GetStraightStrandIncrement() const
{
   return m_StraightAdjustment.m_AllowVertAdjustment ? m_StraightAdjustment.m_StrandIncrement : -1.0;
}

void GirderLibraryEntry::SetEndAdjustmentLimits(pgsTypes::FaceType  topFace, Float64  topLimit, pgsTypes::FaceType  bottomFace, Float64  bottomLimit)
{
   m_EndAdjustment.m_TopFace     = topFace;
   m_EndAdjustment.m_TopLimit    = topLimit;
   m_EndAdjustment.m_BottomFace  = bottomFace;
   m_EndAdjustment.m_BottomLimit = bottomLimit;
}

void GirderLibraryEntry::GetEndAdjustmentLimits(pgsTypes::FaceType* topFace, Float64* topLimit, pgsTypes::FaceType* bottomFace, Float64* bottomLimit) const
{
   *topFace = m_EndAdjustment.m_TopFace;
   *topLimit = m_EndAdjustment.m_TopLimit;
   *bottomFace = m_EndAdjustment.m_BottomFace;
   *bottomLimit = m_EndAdjustment.m_BottomLimit;
}

void GirderLibraryEntry::SetHPAdjustmentLimits(pgsTypes::FaceType  topFace, Float64  topLimit, pgsTypes::FaceType  bottomFace, Float64  bottomLimit)
{
   m_HPAdjustment.m_TopFace     = topFace;
   m_HPAdjustment.m_TopLimit    = topLimit;
   m_HPAdjustment.m_BottomFace  = bottomFace;
   m_HPAdjustment.m_BottomLimit = bottomLimit;
}

void GirderLibraryEntry::GetHPAdjustmentLimits(pgsTypes::FaceType* topFace, Float64* topLimit, pgsTypes::FaceType* bottomFace, Float64* bottomLimit) const
{
   *topFace     = m_HPAdjustment.m_TopFace;
   *topLimit    = m_HPAdjustment.m_TopLimit;
   *bottomFace  = m_HPAdjustment.m_BottomFace;
   *bottomLimit = m_HPAdjustment.m_BottomLimit;
}

void GirderLibraryEntry::SetStraightAdjustmentLimits(pgsTypes::FaceType  topFace, Float64  topLimit, pgsTypes::FaceType  bottomFace, Float64  bottomLimit)
{
   m_StraightAdjustment.m_TopFace     = topFace;
   m_StraightAdjustment.m_TopLimit    = topLimit;
   m_StraightAdjustment.m_BottomFace  = bottomFace;
   m_StraightAdjustment.m_BottomLimit = bottomLimit;
}

void GirderLibraryEntry::GetStraightAdjustmentLimits(pgsTypes::FaceType* topFace, Float64* topLimit, pgsTypes::FaceType* bottomFace, Float64* bottomLimit) const
{
   *topFace     = m_StraightAdjustment.m_TopFace;
   *topLimit    = m_StraightAdjustment.m_TopLimit;
   *bottomFace  = m_StraightAdjustment.m_BottomFace;
   *bottomLimit = m_StraightAdjustment.m_BottomLimit;
}

void GirderLibraryEntry::SetShearData(const CShearData2& cdata)
{
   m_ShearData = cdata;
}

const CShearData2& GirderLibraryEntry::GetShearData() const
{
   return m_ShearData;
}

void GirderLibraryEntry::SetLongSteelInfo(const GirderLibraryEntry::LongSteelInfoVec& vec)
{
   m_LongSteelInfo = vec;
}

GirderLibraryEntry::LongSteelInfoVec GirderLibraryEntry::GetLongSteelInfo() const
{
   return m_LongSteelInfo;
}

void GirderLibraryEntry::SetLongSteelMaterial(WBFL::Materials::Rebar::Type type,WBFL::Materials::Rebar::Grade grade)
{
   m_LongitudinalBarType = type;
   m_LongitudinalBarGrade = grade;
}

void GirderLibraryEntry::GetLongSteelMaterial(WBFL::Materials::Rebar::Type& type,WBFL::Materials::Rebar::Grade& grade) const
{
   type = m_LongitudinalBarType;
   grade = m_LongitudinalBarGrade;
}

void GirderLibraryEntry::SetHarpingPointLocation(Float64 d)
{
   m_HarpingPointLocation = d;
}

Float64 GirderLibraryEntry::GetHarpingPointLocation() const
{
   return m_HarpingPointLocation;
}

void GirderLibraryEntry::SetMinHarpingPointLocation(bool bUseMin,Float64 min)
{
   m_bMinHarpingPointLocation = bUseMin;
   m_MinHarpingPointLocation = min;
}

bool GirderLibraryEntry::IsMinHarpingPointLocationUsed() const
{
   return m_bMinHarpingPointLocation;
}

Float64 GirderLibraryEntry::GetMinHarpingPointLocation() const
{
   return m_MinHarpingPointLocation;
}

void GirderLibraryEntry::SetHarpingPointReference(GirderLibraryEntry::MeasurementLocation reference)
{
   m_HarpPointReference = reference;
}

GirderLibraryEntry::MeasurementLocation GirderLibraryEntry::GetHarpingPointReference() const
{
   return m_HarpPointReference;
}

void GirderLibraryEntry::SetHarpingPointMeasure(GirderLibraryEntry::MeasurementType measure)
{
   m_HarpPointMeasure = measure;
}

GirderLibraryEntry::MeasurementType GirderLibraryEntry::GetHarpingPointMeasure() const
{
   return m_HarpPointMeasure;
}

Float64 GirderLibraryEntry::GetBeamHeight(pgsTypes::MemberEndType endType) const
{
   return m_pBeamFactory->GetBeamHeight(m_Dimensions,endType);
}

Float64 GirderLibraryEntry::GetBeamWidth(pgsTypes::MemberEndType endType) const
{
   return m_pBeamFactory->GetBeamWidth(m_Dimensions,endType);
}

void GirderLibraryEntry::GetBeamTopWidth(pgsTypes::MemberEndType endType, Float64* pLeftWidth, Float64* pRightWidth) const
{
   m_pBeamFactory->GetBeamTopWidth(m_Dimensions, endType, pLeftWidth, pRightWidth);
}

void GirderLibraryEntry::SetDiaphragmLayoutRules(const DiaphragmLayoutRules& rules)
{
   m_DiaphragmLayoutRules = rules;
}

const GirderLibraryEntry::DiaphragmLayoutRules& GirderLibraryEntry::GetDiaphragmLayoutRules() const
{
   return m_DiaphragmLayoutRules;
}

//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
bool GirderLibraryEntry::Edit(bool allowEditing,int nPage)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   if ( m_pBeamFactory == nullptr )
   {
      // For some reason we were unable to create a beam factory.
      // This could be because IE was not installed correctly so the
      // component category manager is missing, or there weren't any
      // beam factories defined
      AfxMessageBox(_T("Girder Sections not defined. This library entry cannot be edited.\nCheck your installation."),MB_OK | MB_ICONWARNING);
   }

   // exchange data with dialog
   // make a temporary copy of this and have the dialog work on it.
   GirderLibraryEntry tmp(*this);

   CGirderMainSheet dlg(tmp, allowEditing, GetRefCount());
   dlg.SetActivePage(nPage);
   INT_PTR i = dlg.DoModal();
   if (i==IDOK)
   {
      *this = tmp;
      this->SetShearData(dlg.m_ShearSteelPage.m_ShearData);
      return true;
   }

   return false;
}

void GirderLibraryEntry::CopyValuesAndAttributes(const GirderLibraryEntry& rOther)
{
   __super::CopyValuesAndAttributes(rOther);

   m_LongitudinalBarType      = rOther.m_LongitudinalBarType;
   m_LongitudinalBarGrade     = rOther.m_LongitudinalBarGrade;
   m_HarpingPointLocation     = rOther.m_HarpingPointLocation;
   m_bMinHarpingPointLocation = rOther.m_bMinHarpingPointLocation;
   m_MinHarpingPointLocation  = rOther.m_MinHarpingPointLocation;
   m_HarpPointReference       = rOther.m_HarpPointReference;
   m_HarpPointMeasure         = rOther.m_HarpPointMeasure;

   m_HPAdjustment             = rOther.m_HPAdjustment;
   m_EndAdjustment            = rOther.m_EndAdjustment;
   m_StraightAdjustment       = rOther.m_StraightAdjustment;

   m_bCheckMaxDebondStrands = rOther.m_bCheckMaxDebondStrands;
   m_MaxDebondStrands                       = rOther.m_MaxDebondStrands;
   m_MaxDebondStrandsPerRow                 = rOther.m_MaxDebondStrandsPerRow;
   m_MaxNumDebondedStrandsPerSection10orLess = rOther.m_MaxNumDebondedStrandsPerSection10orLess;
   m_MaxNumDebondedStrandsPerSection        = rOther.m_MaxNumDebondedStrandsPerSection;
   m_bCheckMaxNumDebondedStrandsPerSection = rOther.m_bCheckMaxNumDebondedStrandsPerSection;
   m_MaxDebondedStrandsPerSection           = rOther.m_MaxDebondedStrandsPerSection;
   m_MinDebondLengthDB = rOther.m_MinDebondLengthDB;
   m_bCheckMinDebondLength = rOther.m_bCheckMinDebondLength;
   m_MinDebondLength                        = rOther.m_MinDebondLength;
   m_bCheckDebondingSymmetry = rOther.m_bCheckDebondingSymmetry;
   m_bCheckAdjacentDebonding = rOther.m_bCheckAdjacentDebonding;
   m_bCheckDebondingInWebWidthProjections = rOther.m_bCheckDebondingInWebWidthProjections;
   m_MaxDebondLengthBySpanFraction          = rOther.m_MaxDebondLengthBySpanFraction;
   m_MaxDebondLengthByHardDistance          = rOther.m_MaxDebondLengthByHardDistance;

   m_StraightStrands = rOther.m_StraightStrands;
   m_TemporaryStrands = rOther.m_TemporaryStrands;

   m_HarpedStrands               = rOther.m_HarpedStrands;
   m_PermanentStrands           = rOther.m_PermanentStrands;

   m_ShearData = rOther.m_ShearData;

   m_LongSteelInfo = rOther.m_LongSteelInfo;

   m_Dimensions = rOther.m_Dimensions;
   m_bSupportsVariableDepthSection = rOther.m_bSupportsVariableDepthSection;
   m_bIsVariableDepthSectionEnabled = rOther.m_bIsVariableDepthSectionEnabled;

   m_pBeamFactory.Release();
   m_pBeamFactory = rOther.m_pBeamFactory;

   m_bOddNumberOfHarpedStrands     = rOther.m_bOddNumberOfHarpedStrands;
   m_AdjustableStrandType          = rOther.m_AdjustableStrandType;
   m_bUseDifferentHarpedGridAtEnds = rOther.m_bUseDifferentHarpedGridAtEnds;

   m_DiaphragmLayoutRules = rOther.m_DiaphragmLayoutRules;

   m_StirrupSizeBarComboColl         = rOther.m_StirrupSizeBarComboColl;
   m_AvailableBarSpacings            = rOther.m_AvailableBarSpacings;
   m_MaxSpacingChangeInZone          = rOther.m_MaxSpacingChangeInZone;
   m_MaxShearCapacityChangeInZone    = rOther.m_MaxShearCapacityChangeInZone;
   m_MinZoneLengthSpacings           = rOther.m_MinZoneLengthSpacings;
   m_MinZoneLengthLength             = rOther.m_MinZoneLengthLength;
   m_DoExtendBarsIntoDeck            = rOther.m_DoExtendBarsIntoDeck;
   m_DoBarsActAsConfinement          = rOther.m_DoBarsActAsConfinement;
   m_LongShearCapacityIncreaseMethod = rOther.m_LongShearCapacityIncreaseMethod;
   m_InterfaceShearWidthReduction = rOther.m_InterfaceShearWidthReduction;

   m_PrestressDesignStrategies     = rOther.m_PrestressDesignStrategies;

   m_MinFilletValue                       = rOther.m_MinFilletValue;
   m_DoCheckMinHaunchAtBearingLines       = rOther.m_DoCheckMinHaunchAtBearingLines;
   m_MinHaunchAtBearingLines              = rOther.m_MinHaunchAtBearingLines;
   m_ExcessiveSlabOffsetWarningTolerance  = rOther.m_ExcessiveSlabOffsetWarningTolerance;

   m_CamberMultipliers                    = rOther.m_CamberMultipliers;

   m_DragCoefficient = rOther.m_DragCoefficient;
   m_PrecamberLimit = rOther.m_PrecamberLimit;
   m_DoReportBearingElevationsAtGirderEdges = rOther.m_DoReportBearingElevationsAtGirderEdges;
}

HICON  GirderLibraryEntry::GetIcon() const
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   if (m_pBeamFactory)
   {
      return m_pBeamFactory->GetIcon();
   }
   else
   {
      return ::LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_GIRDER_ENTRY) );
   }
}

void GirderLibraryEntry::ConfigureStraightStrandGrid(Float64 HgStart,Float64 HgEnd,IStrandGrid* pStartGrid,IStrandGrid* pEndGrid) const
{
   // need to break local data structures into IPoints
   CComPtr<IPoint2dCollection> startPoints, endPoints;
   startPoints.CoCreateInstance(CLSID_Point2dCollection);
   endPoints.CoCreateInstance(CLSID_Point2dCollection);

   ConstStraightStrandIterator iter(m_StraightStrands.begin());
   ConstStraightStrandIterator end(m_StraightStrands.end());
   for ( ; iter != end; iter++ )
   {
      const StraightStrandLocation& strandLocation = *iter; // located up from the bottom of the girder

      CComPtr<IPoint2d> startPoint, endPoint;
      startPoint.CoCreateInstance(CLSID_Point2d);
      endPoint.CoCreateInstance(CLSID_Point2d);

      // Adjust Y elevation so that the strand point is in Girder Section Coordinates (0,0 at top center of girder)
      startPoint->Move(strandLocation.m_Xstart,strandLocation.m_Ystart-HgStart);
      startPoints->Add(startPoint);

      endPoint->Move(strandLocation.m_Xend,strandLocation.m_Yend-HgEnd);
      endPoints->Add(endPoint);
   }

   pStartGrid->ClearGridPoints();
   pStartGrid->AddGridPoints(startPoints);

   pEndGrid->ClearGridPoints();
   pEndGrid->AddGridPoints(endPoints);
}

void GirderLibraryEntry::ConfigureHarpedStrandGrids(Float64 HgStart,Float64 HgHP1,Float64 HgHP2,Float64 HgEnd,IStrandGrid* pEndGridAtStart, IStrandGrid* pHPGridAtStart, IStrandGrid* pHPGridAtEnd, IStrandGrid* pEndGridAtEnd) const
{
   // need to break local data structures into IPoints
   CComPtr<IPoint2dCollection> start_pts, hp1_pts, hp2_pts, end_pts;
   end_pts.CoCreateInstance(CLSID_Point2dCollection);
   start_pts.CoCreateInstance(CLSID_Point2dCollection);
   hp1_pts.CoCreateInstance(CLSID_Point2dCollection);
   hp2_pts.CoCreateInstance(CLSID_Point2dCollection);

   ConstHarpedStrandIterator iter(m_HarpedStrands.begin());
   ConstHarpedStrandIterator end(m_HarpedStrands.end());
   for ( ; iter != end; iter++ )
   {
      const HarpedStrandLocation& strandLocation = *iter;

      CComPtr<IPoint2d> start_point, hp1_point, hp2_point, end_point;
      start_point.CoCreateInstance(CLSID_Point2d);
      hp1_point.CoCreateInstance(CLSID_Point2d);
      hp2_point.CoCreateInstance(CLSID_Point2d);
      end_point.CoCreateInstance(CLSID_Point2d);

      // Adjust Y elevation so that the strand point is in Girder Section Coordinates (0,0 at top center of girder)

      // assume harped points are used at the ends as well
      start_point->Move(strandLocation.m_Xhp,strandLocation.m_Yhp-HgStart);
      hp1_point->Move(strandLocation.m_Xhp,strandLocation.m_Yhp-HgHP1);
      hp2_point->Move(strandLocation.m_Xhp,strandLocation.m_Yhp-HgHP2);
      end_point->Move(strandLocation.m_Xhp,strandLocation.m_Yhp-HgEnd);

      hp1_pts->Add(hp1_point);
      hp2_pts->Add(hp2_point);

      if (m_AdjustableStrandType==pgsTypes::asHarped && m_bUseDifferentHarpedGridAtEnds)
      {
         // different points are used at the end of the girder
         start_point->Move(strandLocation.m_Xstart,strandLocation.m_Ystart-HgStart);
         end_point->Move(strandLocation.m_Xend,strandLocation.m_Yend-HgEnd);
      }

      start_pts->Add(start_point);
      end_pts->Add(end_point);
   }

   pEndGridAtStart->ClearGridPoints();
   pEndGridAtStart->AddGridPoints(start_pts);

   pHPGridAtStart->ClearGridPoints();
   pHPGridAtStart->AddGridPoints(hp1_pts);

   pHPGridAtEnd->ClearGridPoints();
   pHPGridAtEnd->AddGridPoints(hp2_pts);

   pEndGridAtEnd->ClearGridPoints();
   pEndGridAtEnd->AddGridPoints(end_pts);
}

void GirderLibraryEntry::ConfigureTemporaryStrandGrid(Float64 HgStart,Float64 HgEnd,IStrandGrid* pStartGrid,IStrandGrid* pEndGrid) const
{
   // need to break local data structures into IPoints
   CComPtr<IPoint2dCollection> startPoints, endPoints;
   startPoints.CoCreateInstance(CLSID_Point2dCollection);
   endPoints.CoCreateInstance(CLSID_Point2dCollection);

   ConstStraightStrandIterator iter(m_TemporaryStrands.begin());
   ConstStraightStrandIterator end(m_TemporaryStrands.end());
   for ( ; iter != end; iter++ )
   {
      const StraightStrandLocation& strandLocation = *iter;

      CComPtr<IPoint2d> startPoint, endPoint;
      startPoint.CoCreateInstance(CLSID_Point2d);
      endPoint.CoCreateInstance(CLSID_Point2d);

      // Adjust Y elevation so that the strand point is in Girder Section Coordinates (0,0 at top center of girder)
      startPoint->Move(strandLocation.m_Xstart,strandLocation.m_Ystart-HgStart);
      startPoints->Add(startPoint);

      endPoint->Move(strandLocation.m_Xend,strandLocation.m_Yend-HgEnd);
      endPoints->Add(endPoint);
   }

   pStartGrid->ClearGridPoints();
   pStartGrid->AddGridPoints(startPoints);

   pEndGrid->ClearGridPoints();
   pEndGrid->AddGridPoints(endPoints);
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PRIVATE    ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
bool GirderLibraryEntry::IsEqual(IPoint2d* p1,IPoint2d* p2) const
{
   Float64 x1,y1;
   p1->get_X(&x1);
   p1->get_Y(&y1);

   Float64 x2,y2;
   p2->get_X(&x2);
   p2->get_Y(&y2);

   return ::IsEqual(x1,x2) && ::IsEqual(y1,y2);
}

bool GirderLibraryEntry::CanDebondStraightStrands() const
{
   ConstStraightStrandIterator iter(m_StraightStrands.begin());
   ConstStraightStrandIterator end(m_StraightStrands.end());
   for ( ; iter != end; iter++ )
   {
      const StraightStrandLocation& strandLocation = *iter;

      if (strandLocation.m_bCanDebond)
      {
         return true; // if there is one, we can debond
      }
   }

   return false;
}

bool GirderLibraryEntry::CheckMaxTotalFractionDebondedStrands() const
{
   return m_bCheckMaxDebondStrands;
}

void GirderLibraryEntry::CheckMaxTotalFractionDebondedStrands(bool bCheck)
{
   m_bCheckMaxDebondStrands = bCheck;
}

Float64 GirderLibraryEntry::GetMaxTotalFractionDebondedStrands() const
{
   return m_MaxDebondStrands;
}

void GirderLibraryEntry::SetMaxTotalFractionDebondedStrands(Float64 fraction) 
{
   ATLASSERT(0.0 <= fraction && fraction <= 1.0);
   m_MaxDebondStrands = fraction;
}

Float64 GirderLibraryEntry::GetMaxFractionDebondedStrandsPerRow() const
{
   return m_MaxDebondStrandsPerRow;
}

void GirderLibraryEntry::SetMaxFractionDebondedStrandsPerRow(Float64 fraction)
{
   ATLASSERT(0.0 <= fraction && fraction <= 1.0);
   m_MaxDebondStrandsPerRow = fraction;
}

void  GirderLibraryEntry::GetMaxDebondedStrandsPerSection(StrandIndexType* p10orLess, StrandIndexType* pNumber, bool* pbCheck, Float64* pFraction) const
{
   *p10orLess = m_MaxNumDebondedStrandsPerSection10orLess;
   *pNumber = m_MaxNumDebondedStrandsPerSection;
   *pbCheck = m_bCheckMaxNumDebondedStrandsPerSection;
   *pFraction = m_MaxDebondedStrandsPerSection;
}

void GirderLibraryEntry::SetMaxDebondedStrandsPerSection(StrandIndexType n10orLess, StrandIndexType number, bool bCheck,Float64 fraction)
{
   ATLASSERT(0.0 <= fraction && fraction <= 1.0);

   m_MaxNumDebondedStrandsPerSection10orLess = n10orLess;
   m_MaxNumDebondedStrandsPerSection = number;
   m_bCheckMaxNumDebondedStrandsPerSection = bCheck;
   m_MaxDebondedStrandsPerSection    = fraction;
}

void GirderLibraryEntry::GetMaxDebondedLength(bool* pUseSpanFraction, Float64* pSpanFraction, bool* pUseHardDistance, Float64* pHardDistance) const
{
   if (*pUseSpanFraction = m_MaxDebondLengthBySpanFraction>=0.0)
   {
      *pSpanFraction = m_MaxDebondLengthBySpanFraction;
   }

   if (*pUseHardDistance = m_MaxDebondLengthByHardDistance>=0.0)
   {
      *pHardDistance = m_MaxDebondLengthByHardDistance;
   }
}

void GirderLibraryEntry::SetMaxDebondedLength(bool useSpanFraction, Float64 spanFraction, bool useHardDistance, Float64 hardDistance)
{
   if (useSpanFraction)
   {
      ATLASSERT(spanFraction>=0.0);
      m_MaxDebondLengthBySpanFraction = spanFraction;
   }
   else
   {
      m_MaxDebondLengthBySpanFraction = -1.0;
   }

   if (useHardDistance)
   {
      ATLASSERT(hardDistance>=0.0);
      m_MaxDebondLengthByHardDistance = hardDistance;
   }
   else
   {
      m_MaxDebondLengthByHardDistance = -1.0;
   }
}

void GirderLibraryEntry::SetMinDistanceBetweenDebondSections(Float64 ndb, bool bCheck, Float64 minDistance)
{
   m_MinDebondLengthDB = ndb;
   m_bCheckMinDebondLength = bCheck;
   m_MinDebondLength = minDistance;
}

void GirderLibraryEntry::GetMinDistanceBetweenDebondSections(Float64* pndb, bool* pbCheck, Float64* pMinDistance) const
{
   *pndb = m_MinDebondLengthDB;
   *pbCheck = m_bCheckMinDebondLength;
   *pMinDistance = m_MinDebondLength;
}

void GirderLibraryEntry::CheckDebondingSymmetry(bool bCheck)
{
   m_bCheckDebondingSymmetry = bCheck;
}

bool GirderLibraryEntry::CheckDebondingSymmetry() const
{
   return m_bCheckDebondingSymmetry;
}

void GirderLibraryEntry::CheckAdjacentDebonding(bool bCheck)
{
   m_bCheckAdjacentDebonding = bCheck;
}

bool GirderLibraryEntry::CheckAdjacentDebonding() const
{
   return m_bCheckAdjacentDebonding;
}

void GirderLibraryEntry::CheckDebondingInWebWidthProjections(bool bCheck)
{
   m_bCheckDebondingInWebWidthProjections = bCheck;
}

bool GirderLibraryEntry::CheckDebondingInWebWidthProjections() const
{
   return m_bCheckDebondingInWebWidthProjections;
}

bool GirderLibraryEntry::IsEqual(IPoint2dCollection* points1,IPoint2dCollection* points2) const
{
   IndexType c1,c2;
   points1->get_Count(&c1);
   points2->get_Count(&c2);

   if ( c1 != c2 )
   {
      return false;
   }

   IndexType idx;
   for ( idx = 0; idx < c1; idx++ )
   {
      CComPtr<IPoint2d> p1, p2;
      points1->get_Item(idx,&p1);
      points2->get_Item(idx,&p2);

      if ( !IsEqual(p1,p2) )
      {
         return false;
      }
   }

   return true;
}

std::_tstring GirderLibraryEntry::GetSectionName() const
{
   std::_tstring name;
   if (m_pBeamFactory)
   {
      LPOLESTR pszUserType;
      OleRegGetUserType(m_pBeamFactory->GetCLSID(),USERCLASSTYPE_SHORT,&pszUserType);
      return std::_tstring( CString(pszUserType) );
   }
   else
   {
      ATLASSERT(false);
      return std::_tstring(_T("Unknown Girder Type"));
   }
}

CShearData2 GirderLibraryEntry::LegacyShearData::ConvertToShearData() const
{
   // Convert old legacy data to common CShearData class
   CShearData2 sdata;

   sdata.ShearBarType = m_StirrupBarType;
   sdata.ShearBarGrade = m_StirrupBarGrade;
   sdata.bIsRoughenedSurface = m_bIsRoughenedSurface;

   if (!m_ShearZoneInfo.empty())
   {
      sdata.ShearZones.clear();
      Uint32 zn = 0;
      for ( ShearZoneInfoVec::const_iterator it = m_ShearZoneInfo.begin(); it!=m_ShearZoneInfo.end(); it++)
      {
         const ShearZoneInfo& rinfo = *it;
         CShearZoneData2 zdata;
         zdata.BarSpacing = rinfo.StirrupSpacing;
         zdata.VertBarSize = rinfo.VertBarSize;
         zdata.ZoneLength = rinfo.ZoneLength;
         zdata.nVertBars = rinfo.nVertBars;

         if (zn < m_LastConfinementZone)
         {
            zdata.ConfinementBarSize = rinfo.VertBarSize;
         }

         if (m_bStirrupsEngageDeck)
         {
            zdata.nHorzInterfaceBars = rinfo.nVertBars;
         }

         zdata.ZoneNum = zn++;

         sdata.ShearZones.push_back(zdata);
      }
   }

   // Additional top flange horiz interface shear
   if (m_TopFlangeShearBarSize!=WBFL::Materials::Rebar::Size::bsNone && m_TopFlangeShearBarSpacing>0.0)
   {
      sdata.HorizontalInterfaceZones.clear();
      CHorizontalInterfaceZoneData zdata;
      zdata.ZoneNum = 0;
      zdata.BarSize = m_TopFlangeShearBarSize;
      zdata.BarSpacing = m_TopFlangeShearBarSpacing;
      zdata.nBars = 2;
      zdata.ZoneLength = Float64_Max;

      sdata.HorizontalInterfaceZones.push_back(zdata);
   }


   return sdata;
}

IndexType GirderLibraryEntry::GetNumStirrupSizeBarCombos() const
{
   return m_StirrupSizeBarComboColl.size();
}

void GirderLibraryEntry::ClearStirrupSizeBarCombos()
{
   m_StirrupSizeBarComboColl.clear();
}

void GirderLibraryEntry::GetStirrupSizeBarCombo(IndexType index, WBFL::Materials::Rebar::Size* pSize, Float64* pNLegs) const
{
   ATLASSERT(index>=0 && index<m_StirrupSizeBarComboColl.size());
   const StirrupSizeBarCombo& cbo = m_StirrupSizeBarComboColl[index];
   *pSize = cbo.Size;
   *pNLegs = cbo.NLegs;
}

void GirderLibraryEntry::AddStirrupSizeBarCombo(WBFL::Materials::Rebar::Size Size, Float64 NLegs)
{
   StirrupSizeBarCombo cbo;
   cbo.Size = Size;
   cbo.NLegs = NLegs;

   m_StirrupSizeBarComboColl.push_back(cbo);
}


// Available bar spacings for design
IndexType GirderLibraryEntry::GetNumAvailableBarSpacings() const
{
   return m_AvailableBarSpacings.size();
}

void GirderLibraryEntry::ClearAvailableBarSpacings()
{
   m_AvailableBarSpacings.clear();
}

Float64 GirderLibraryEntry::GetAvailableBarSpacing(IndexType index) const
{
   ATLASSERT(index>=0 && index<m_AvailableBarSpacings.size());
   return m_AvailableBarSpacings[index];
}

void GirderLibraryEntry::AddAvailableBarSpacing(Float64 Spacing)
{
   m_AvailableBarSpacings.push_back(Spacing);
}


// Max change in spacing between zones
Float64 GirderLibraryEntry::GetMaxSpacingChangeInZone() const
{
   return m_MaxSpacingChangeInZone;
}

void GirderLibraryEntry::SetMaxSpacingChangeInZone(Float64 Change)
{
   m_MaxSpacingChangeInZone = Change;
}


// Max change in shear capacity between zones (% fraction)
Float64 GirderLibraryEntry::GetMaxShearCapacityChangeInZone() const
{
   return m_MaxShearCapacityChangeInZone;
}

void GirderLibraryEntry::SetMaxShearCapacityChangeInZone(Float64 Change)
{
   m_MaxShearCapacityChangeInZone = Change;
}


void GirderLibraryEntry::GetMinZoneLength(Uint32* pSpacings, Float64* pLength) const
{
   *pSpacings = m_MinZoneLengthSpacings;
   *pLength = m_MinZoneLengthLength;
}

void GirderLibraryEntry::SetMinZoneLength(Uint32 Spacings, Float64 Length)
{
   m_MinZoneLengthSpacings = Spacings;
   m_MinZoneLengthLength = Length;
}

bool GirderLibraryEntry::GetExtendBarsIntoDeck() const
{
   return m_DoExtendBarsIntoDeck;
}

void GirderLibraryEntry::SetExtendBarsIntoDeck(bool isTrue) 
{
   m_DoExtendBarsIntoDeck = isTrue;
}

bool GirderLibraryEntry::GetBarsActAsConfinement() const
{
   return m_DoBarsActAsConfinement;
}

void GirderLibraryEntry::SetBarsActAsConfinement(bool isTrue)
{
   m_DoBarsActAsConfinement = isTrue;
}

GirderLibraryEntry::LongShearCapacityIncreaseMethod GirderLibraryEntry::GetLongShearCapacityIncreaseMethod() const
{
   return m_LongShearCapacityIncreaseMethod;
}

void GirderLibraryEntry::SetLongShearCapacityIncreaseMethod(LongShearCapacityIncreaseMethod method) 
{
   m_LongShearCapacityIncreaseMethod = method;
}

Float64 GirderLibraryEntry::GetInterfaceShearWidthReduction() const
{
   return m_InterfaceShearWidthReduction;
}

void GirderLibraryEntry::SetInterfaceShearWidthReduction(Float64 bvir)
{
   m_InterfaceShearWidthReduction = bvir;
}

IndexType GirderLibraryEntry::GetNumPrestressDesignStrategies() const
{
   return m_PrestressDesignStrategies.size();
}

void GirderLibraryEntry::GetPrestressDesignStrategy(IndexType index,  arFlexuralDesignType* pFlexuralDesignType, Float64* pMaxFci, Float64* pMaxFc) const
{
   ATLASSERT(index>=0 && index<m_PrestressDesignStrategies.size());
   const PrestressDesignStrategy& rds = m_PrestressDesignStrategies[index];

   *pFlexuralDesignType = rds.m_FlexuralDesignType;
   *pMaxFci = rds.m_MaxFci; 
   *pMaxFc = rds.m_MaxFc;
}

void GirderLibraryEntry::SetDefaultPrestressDesignStrategy()
{
   // Set prestress design options to pre 2015 defaults. 
   // i.e., a single strategy based on prestressing in girder
   m_PrestressDesignStrategies.clear();

   PrestressDesignStrategy ds;
   StrandIndexType  nh = GetNumHarpedStrandCoordinates();

   if (m_AdjustableStrandType==pgsTypes::asHarped || (0 < nh && m_AdjustableStrandType==pgsTypes::asStraightOrHarped) )
   {
      ds.m_FlexuralDesignType = dtDesignForHarping;
   }
   else if ( CanDebondStraightStrands() )
   {
      ds.m_FlexuralDesignType  = dtDesignForDebonding;
   }
   else
   {
      ds.m_FlexuralDesignType  = dtDesignFullyBonded;
   }

   m_PrestressDesignStrategies.push_back(ds);
   ATLASSERT(0 < m_PrestressDesignStrategies.size());
}

Float64 GirderLibraryEntry::GetMinFilletValue() const
{
   return m_MinFilletValue;
}

void GirderLibraryEntry::SetMinFilletValue(Float64 minVal)
{
   m_MinFilletValue = minVal;
}

bool GirderLibraryEntry::GetMinHaunchAtBearingLines(Float64* minVal) const
{
   if (m_DoCheckMinHaunchAtBearingLines)
   {
      *minVal = m_MinHaunchAtBearingLines;
   }

   return m_DoCheckMinHaunchAtBearingLines;
}

void GirderLibraryEntry::SetMinHaunchAtBearingLines(bool doCheck, Float64 minVal)
{
   m_DoCheckMinHaunchAtBearingLines = doCheck;
   m_MinHaunchAtBearingLines = minVal;
}

Float64 GirderLibraryEntry::GetExcessiveSlabOffsetWarningTolerance() const
{
   return m_ExcessiveSlabOffsetWarningTolerance;
}

void GirderLibraryEntry::SetExcessiveSlabOffsetWarningTolerance(Float64 tol)
{
   m_ExcessiveSlabOffsetWarningTolerance = tol;
}

void GirderLibraryEntry::SetCamberMultipliers(CamberMultipliers& factors)
{
   m_CamberMultipliers = factors;
}

CamberMultipliers GirderLibraryEntry::GetCamberMultipliers() const
{
   return m_CamberMultipliers;
}

void GirderLibraryEntry::SetDragCoefficient(Float64 Cd)
{
   m_DragCoefficient = Cd;
}

Float64 GirderLibraryEntry::GetDragCoefficient() const
{
   return m_DragCoefficient;
}

void GirderLibraryEntry::SetPrecamberLimit(Float64 limit)
{
   m_PrecamberLimit = limit;
}

Float64 GirderLibraryEntry::GetPrecamberLimit() const
{
   return m_PrecamberLimit;
}

bool GirderLibraryEntry::CanPrecamber() const
{
   return m_pBeamFactory->CanPrecamber();
}

void GirderLibraryEntry::SetDoReportBearingElevationsAtGirderEdges(bool doit)
{
   m_DoReportBearingElevationsAtGirderEdges = doit;
}

bool GirderLibraryEntry::GetDoReportBearingElevationsAtGirderEdges() const
{
   return m_DoReportBearingElevationsAtGirderEdges;
}

pgsCompatibilityData* GirderLibraryEntry::GetCompatibilityData() const
{
   return m_pCompatibilityData;
}

//======================== ACCESS     =======================================
//======================== INQUERY    =======================================

static const Float64 TwoInches = WBFL::Units::ConvertToSysUnits(2.0,WBFL::Units::Measure::Inch);

GirderLibraryEntry::HarpedStrandAdjustment::HarpedStrandAdjustment() : 
m_AllowVertAdjustment(false), 
m_StrandIncrement(TwoInches),
m_TopFace(pgsTypes::TopFace), 
m_TopLimit(TwoInches),
m_BottomFace(pgsTypes::BottomFace), 
m_BottomLimit(TwoInches)
{
}
