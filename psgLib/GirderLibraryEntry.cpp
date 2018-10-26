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
#include <psgLib\GirderLibraryEntry.h>
#include <System\IStructuredSave.h>
#include <System\IStructuredLoad.h>
#include <System\XStructuredLoad.h>
#include <Units\sysUnits.h>

#include <psgLib\BeamFamilyManager.h>

#include <GeomModel\PrecastBeam.h>

#include "resource.h"
#include "GirderMainSheet.h"

#include "ComCat.h"
#include "PGSuperCatCom.h"

#include <MathEx.h>

#include <BridgeModeling\GirderProfile.h>

#include <WBFLGeometry.h>
#include <WBFLSections.h>
#include <WBFLGenericBridge.h>
#include <WBFLGenericBridgeTools.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define DUMMY_AGENT_ID -1

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
#define CURRENT_VERSION 16.0

////////////////////////// PUBLIC     ///////////////////////////////////////

// NOTE: This collection must be emptied before the DLL is unloaded. In order to
// do this we have to purposely call gs_ClassFactores.clear(). Since an individual
// GirderLibraryEntry has no way of knowing it is the last one to be released,
// we turn to the library manager. If the library manager is going out of scope, then
// there shouldn't be any library entires left. ms_ClassFactories is cleared
// in psgLibraryManager's destructor.
GirderLibraryEntry::ClassFactoryCollection GirderLibraryEntry::ms_ClassFactories;


/****************************************************************************
CLASS
   GirderLibraryEntry
****************************************************************************/
static const char* SMaterialName={"AASHTO M31 (A615) - Grade 60"};


//======================== LIFECYCLE  =======================================
GirderLibraryEntry::GirderLibraryEntry() :
m_bUseDifferentHarpedGridAtEnds(true),
m_ShearSteelBarSize(0),
m_LastConfinementZone(0),
m_HarpingPointLocation(0.25),
m_HarpPointMeasure(mtFractionOfGirderLength),
m_bMinHarpingPointLocation(false),
m_MinHarpingPointLocation(::ConvertToSysUnits(5.0,unitMeasure::Feet)),
m_HarpPointReference(mlBearing),
m_ShearSteelMaterial(SMaterialName),
m_LongSteelMaterial(SMaterialName),
m_TopFlangeShearBarSize(0),
m_TopFlangeShearBarSpacing(0.0),
m_bStirrupsEngageDeck(true),
m_bOddNumberOfHarpedStrands(true),
// debonding criteria - use aashto defaults
m_MaxDebondStrands(0.25),
m_MaxDebondStrandsPerRow(0.40),
m_MaxNumDebondedStrandsPerSection(4),
m_MaxDebondedStrandsPerSection(0.40),
m_MinDebondLength(::ConvertToSysUnits(3.0,unitMeasure::Feet)), // not aashto, but reasonable
m_DefaultDebondLength(::ConvertToSysUnits(3.0,unitMeasure::Feet)),
m_MaxDebondLengthBySpanFraction(-1.0), // 
m_MaxDebondLengthByHardDistance(-1.0),
m_bCanPostTension(false)
{
	CWaitCursor cursor;

   // When the user creates a new library entry, it needs a beam type. The beam type
   // is defined by the beam factory this object holds. Therefore we need to create a
   // beam factory. Since we don't what kind of beam the user wants, use the first 
   // one registered as a default
   std::vector<CString> familyNames = CBeamFamilyManager::GetBeamFamilyNames();
   ATLASSERT(0 < familyNames.size());
   CComPtr<IBeamFamily> beamFamily;
   HRESULT hr = CBeamFamilyManager::GetBeamFamily(familyNames.front(),&beamFamily);
   if ( FAILED(hr) )
      return;

   CComPtr<IBeamFactory> beam_factory;
   std::vector<CString> factoryNames = beamFamily->GetFactoryNames();
   ATLASSERT(0 < factoryNames.size());
   beamFamily->CreateFactory(factoryNames.front(),&beam_factory);

   SetBeamFactory(beam_factory);
   // m_pBeamFactory is NULL if we were unable to create a beam factory
}

GirderLibraryEntry::GirderLibraryEntry(const GirderLibraryEntry& rOther) :
libLibraryEntry(rOther)
{
   MakeCopy(rOther);
}

GirderLibraryEntry::~GirderLibraryEntry()
{
}

//======================== OPERATORS  =======================================
GirderLibraryEntry& GirderLibraryEntry::operator= (const GirderLibraryEntry& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================

bool GirderLibraryEntry::SaveMe(sysIStructuredSave* pSave)
{
   USES_CONVERSION;
   pSave->BeginUnit("GirderLibraryEntry", CURRENT_VERSION);
   pSave->Property("Name",this->GetName().c_str());

   LPOLESTR pszCLSID;
   ::StringFromCLSID(m_pBeamFactory->GetCLSID(),&pszCLSID);
   pSave->Property("CLSID",OLE2A(pszCLSID));
   ::CoTaskMemFree((void*)pszCLSID);

   pSave->Property("Publisher",m_pBeamFactory->GetPublisher().c_str());
   LPOLESTR pszUserType;
   OleRegGetUserType(m_pBeamFactory->GetCLSID(),USERCLASSTYPE_SHORT,&pszUserType);
   pSave->Property("SectionName",CString(pszUserType));

   m_pBeamFactory->SaveSectionDimensions(pSave,m_Dimensions);

   // added version 16
   pSave->Property("CanPostTension", m_bCanPostTension );

   pSave->Property("UseDifferentHarpedGridAtEnds",  m_bUseDifferentHarpedGridAtEnds);

   pSave->Property("EndAllowVertAdjustment",   m_EndAdjustment.m_AllowVertAdjustment);
   pSave->Property("EndStrandIncrement",       m_EndAdjustment.m_StrandIncrement);
   pSave->Property("EndBottomFace",            (long)m_EndAdjustment.m_BottomFace);
   pSave->Property("EndBottomLimit",           m_EndAdjustment.m_BottomLimit);
   pSave->Property("EndTopFace",               (long)m_EndAdjustment.m_TopFace);
   pSave->Property("EndTopLimit",              m_EndAdjustment.m_TopLimit);

   pSave->Property("HPAllowVertAdjustment",   m_HPAdjustment.m_AllowVertAdjustment);
   pSave->Property("HPStrandIncrement",       m_HPAdjustment.m_StrandIncrement);
   pSave->Property("HPBottomFace",            (long)m_HPAdjustment.m_BottomFace);
   pSave->Property("HPBottomLimit",           m_HPAdjustment.m_BottomLimit);
   pSave->Property("HPTopFace",               (long)m_HPAdjustment.m_TopFace);
   pSave->Property("HPTopLimit",              m_HPAdjustment.m_TopLimit);

   pSave->Property("ShearSteelBarSize",        m_ShearSteelBarSize);
   pSave->Property("LastConfinementZone",      m_LastConfinementZone);
   pSave->Property("HarpingPointLocation",     m_HarpingPointLocation);
   pSave->Property("UseMinHarpingPointLocation",m_bMinHarpingPointLocation);

   if ( m_bMinHarpingPointLocation )
      pSave->Property("MinHarpingPointLocation",m_MinHarpingPointLocation);

   pSave->Property("HarpingPointReference",    (long)m_HarpPointReference);
   pSave->Property("HarpingPointMeasure",      (long)m_HarpPointMeasure); // added in version 4
   pSave->Property("ShearSteelMaterial",       m_ShearSteelMaterial.c_str());
   pSave->Property("LongSteelMaterial",        m_LongSteelMaterial.c_str());
   pSave->Property("DoStirrupsEngageDeck",     m_bStirrupsEngageDeck);
   pSave->Property("TopFlangeShearBarSize",    m_TopFlangeShearBarSize);
   pSave->Property("TopFlangeShearBarSpacing", m_TopFlangeShearBarSpacing);

   // debond criteria - added in version 13
   pSave->BeginUnit("DebondingCritia", 1.0);

   pSave->Property("MaxDebondStrands",               m_MaxDebondStrands);
   pSave->Property("MaxDebondStrandsPerRow",         m_MaxDebondStrandsPerRow);
   pSave->Property("MaxNumDebondedStrandsPerSection",m_MaxNumDebondedStrandsPerSection);
   pSave->Property("MaxDebondedStrandsPerSection",   m_MaxDebondedStrandsPerSection);
   pSave->Property("MinDebondLength",                m_MinDebondLength);
   pSave->Property("DefaultDebondLength",            m_DefaultDebondLength);
   pSave->Property("MaxDebondLengthBySpanFraction",  m_MaxDebondLengthBySpanFraction);
   pSave->Property("MaxDebondLengthByHardDistance",  m_MaxDebondLengthByHardDistance);

   pSave->EndUnit(); // DebondingCritia

   // Added start and end strand grids in version 15 of parent unit
   pSave->BeginUnit("StraightStrands",1.0);
   StraightStrandIterator strandIter;
   for (StraightStrandIterator strandIter = m_StraightStrands.begin(); strandIter != m_StraightStrands.end(); strandIter++)
   {
      pSave->BeginUnit("StrandLocation", 1.0);

      StraightStrandLocation& strandLocation = *strandIter;

      pSave->Property("Xstart", strandLocation.m_Xstart);
      pSave->Property("Ystart", strandLocation.m_Ystart);
      pSave->Property("Xend",   strandLocation.m_Xend);
      pSave->Property("Yend",   strandLocation.m_Yend);
      pSave->Property("CanDebond", strandLocation.m_bCanDebond);

      pSave->EndUnit(); // StrandLocation
   }
   pSave->EndUnit(); // StraightStrands

   pSave->BeginUnit("HarpedStrands",1.0);
   // harped strands - consolidated end and hp locations in version 4.0
   for (HarpedStrandIterator iter = m_HarpedStrands.begin(); iter != m_HarpedStrands.end(); iter++)
   {
      pSave->BeginUnit("StrandLocation", 1.0);

      HarpedStrandLocation& strandLocation = *iter;

      pSave->Property("Xstart", strandLocation.m_Xstart);
      pSave->Property("Ystart", strandLocation.m_Ystart);
      pSave->Property("Xhp",    strandLocation.m_Xhp);
      pSave->Property("Yhp",    strandLocation.m_Yhp);
      pSave->Property("Xend",   strandLocation.m_Xend);
      pSave->Property("Yend",   strandLocation.m_Yend);

      pSave->EndUnit();
   }
   pSave->EndUnit();

   pSave->Property("OddNumberOfHarpedStrands",m_bOddNumberOfHarpedStrands);

   // Temporary strands
   // Added start and end strand grids in version 15 of parent unit
   pSave->BeginUnit("TemporaryStrands",1.0);
   for (StraightStrandIterator strandIter = m_TemporaryStrands.begin(); strandIter != m_TemporaryStrands.end(); strandIter++)
   {
      pSave->BeginUnit("StrandLocation", 1.0);

      StraightStrandLocation& strandLocation = *strandIter;

      pSave->Property("Xstart", strandLocation.m_Xstart);
      pSave->Property("Ystart", strandLocation.m_Ystart);
      pSave->Property("Xend",   strandLocation.m_Xend);
      pSave->Property("Yend",   strandLocation.m_Yend);

      //pSave->Property("CanDebond", strandLocation.m_bCanDebond);
      // debond parameter is ignored for temporary strands

      pSave->EndUnit(); // StrandLocation
   }
   pSave->EndUnit(); // TemporaryStrands

   // strand ordering
   // added in version 5.0
   for (GlobalStrandOrderIterator gs_it = m_GlobalStrandOrder.begin(); gs_it!=m_GlobalStrandOrder.end(); gs_it++)
   {
      pSave->BeginUnit("GlobalStrandOrder", 1.0);

      psStrandType type = gs_it->m_StrandType;

      if (type == stStraight)
      {
        pSave->Property("StrandType", "Straight");
      }
      else if (type == stHarped)
      {
        pSave->Property("StrandType", "Harped");
      }
      else
      {
         ATLASSERT(0);
      }

      pSave->Property("LocalSortOrder", gs_it->m_LocalSortOrder);

      pSave->EndUnit();

   }

   // shear zones
   for (ShearZoneInfoVec::const_iterator its = m_ShearZoneInfo.begin(); its!=m_ShearZoneInfo.end(); its++)
   {
      pSave->BeginUnit("ShearZones", 2.0);

      pSave->Property("ZoneLength",  (*its).ZoneLength);
      pSave->Property("Spacing",     (*its).StirrupSpacing);
      pSave->Property("VertBarSize", (*its).VertBarSize);
      pSave->Property("VertBars",    (*its).nVertBars);
      pSave->Property("HorzBarSize", (*its).HorzBarSize);
      pSave->Property("HorzBars",    (*its).nHorzBars);

      pSave->EndUnit();
   }
   
   // Longitudinal Steel rows
   for (LongSteelInfoVec::const_iterator itl = m_LongSteelInfo.begin(); itl!=m_LongSteelInfo.end(); itl++)
   {
      pSave->BeginUnit("LongSteelInfo", 1.0);

      if (itl->Face==GirderLibraryEntry::GirderBottom)
         pSave->Property("Face", "Bottom");
      else
         pSave->Property("Face", "Top");

      pSave->Property("NumberOfBars", (*itl).NumberOfBars);
      pSave->Property("BarSize",  (*itl).BarSize);
      pSave->Property("BarCover", (*itl).Cover);
      pSave->Property("BarSpacing", (*itl).BarSpacing);

      pSave->EndUnit();
   }

   for ( DiaphragmLayoutRules::const_iterator itd = m_DiaphragmLayoutRules.begin(); itd != m_DiaphragmLayoutRules.end(); itd++ )
   {
      const DiaphragmLayoutRule& dlr = *itd;
      pSave->BeginUnit("DiaphragmLayoutRule",1.0);

      pSave->Property("Description",dlr.Description.c_str());
      pSave->Property("MinSpan",dlr.MinSpan);
      pSave->Property("MaxSpan",dlr.MaxSpan);
      pSave->Property("Height",dlr.Height);
      pSave->Property("Thickness",dlr.Thickness);
      pSave->Property("DiaphragmType",(long)dlr.Type);
      pSave->Property("ConstructionType",(long)dlr.Construction);
      pSave->Property("MeasurementType",(long)dlr.MeasureType);
      pSave->Property("MeasurmentLocation",(long)dlr.MeasureLocation);
      pSave->Property("Location",dlr.Location);

      pSave->EndUnit();
   }


   pSave->EndUnit();

   return false;
}

bool GirderLibraryEntry::LoadMe(sysIStructuredLoad* pLoad)
{
   USES_CONVERSION;

   if(pLoad->BeginUnit("GirderLibraryEntry"))
   {
      Float64 version = pLoad->GetVersion();
      if ( version > CURRENT_VERSION)
         THROW_LOAD(BadVersion,pLoad);

      std::string name;
      if(pLoad->Property("Name",&name))
         this->SetName(name.c_str());
      else
         THROW_LOAD(InvalidFileFormat,pLoad);

      m_Dimensions.clear();
      if ( version <= 1.1 )
      {
         // version 1.1 and ealier everything was an I-Beam
         LoadIBeamDimensions(pLoad);

#if defined _DEBUG
         std::vector<std::string> vDimNames = m_pBeamFactory->GetDimensionNames();
         ATLASSERT( vDimNames.size() == m_Dimensions.size() );
         // if this assert fires, new beam dimensions have been added and not accounted for
#endif
      }
      else
      {
         // Version 1.2 or greater... dictionary based defintion of girder dimensions
         std::string strCLSID;
         if (!pLoad->Property("CLSID",&strCLSID) )
            THROW_LOAD(InvalidFileFormat,pLoad);

         std::string strPublisher;
         if ( !pLoad->Property("Publisher",&strPublisher) )
            THROW_LOAD(InvalidFileFormat,pLoad);

         std::string strSectionName;
         if ( !pLoad->Property("SectionName",&strSectionName) )
            THROW_LOAD(InvalidFileFormat,pLoad);

         HRESULT hr = CreateBeamFactory(strCLSID);
         if ( FAILED(hr) )
         {
            CString strMsg;
            strMsg.Format("Unable to create the %s section, published by %s\nThe %s section is not registered on your computer\nPlease contact the publisher for assistance.",strSectionName.c_str(),strPublisher.c_str(),strSectionName.c_str());
            sysXStructuredLoad e(sysXStructuredLoad::UserDefined,__FILE__,__LINE__);
            e.SetExtendedMessage(strMsg);
            e.Throw();
         }

         ATLASSERT(m_pBeamFactory != NULL);

         m_Dimensions.clear();
         m_Dimensions = m_pBeamFactory->LoadSectionDimensions(pLoad);
      } // end of ( version < 1.1 )


      if (version < 5.0)
      {
         // max adjustment goes away and replaced with cover values
         double dummy;
         if(!pLoad->Property("DownwardStrandAdjustment", &dummy))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if(!pLoad->Property("DownwardStrandIncrement", &m_EndAdjustment.m_StrandIncrement))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if(!pLoad->Property("UpwardStrandAdjustment", &dummy))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if(!pLoad->Property("UpwardStrandIncrement", &m_HPAdjustment.m_StrandIncrement))
            THROW_LOAD(InvalidFileFormat,pLoad);
      }
      else
      {
         if ( 15 < version )
         {
            // added version 16
            if ( !pLoad->Property("CanPostTension", &m_bCanPostTension ) )
               THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if(!pLoad->Property("UseDifferentHarpedGridAtEnds", &m_bUseDifferentHarpedGridAtEnds))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if (version < 7.0)
         {
            if(!pLoad->Property("EndAllowVertAdjustment", &m_EndAdjustment.m_AllowVertAdjustment))
            THROW_LOAD(InvalidFileFormat,pLoad);

            if(!pLoad->Property("HPAllowVertAdjustment", &m_HPAdjustment.m_AllowVertAdjustment))
            THROW_LOAD(InvalidFileFormat,pLoad);

            double top_cover, bottom_cover;
            if(!pLoad->Property("TopCover", &top_cover))
            THROW_LOAD(InvalidFileFormat,pLoad);

            if(!pLoad->Property("BottomCover", &bottom_cover))
            THROW_LOAD(InvalidFileFormat,pLoad);

            // convert old cover to adjustment limit
            m_EndAdjustment.m_TopFace = GirderTop;
            m_EndAdjustment.m_TopLimit = top_cover;
            m_EndAdjustment.m_BottomFace = GirderBottom;
            m_EndAdjustment.m_BottomLimit = bottom_cover;

            m_HPAdjustment.m_TopFace = GirderTop;
            m_HPAdjustment.m_TopLimit = top_cover;
            m_HPAdjustment.m_BottomFace = GirderBottom;
            m_HPAdjustment.m_BottomLimit = bottom_cover;

            if(!pLoad->Property("EndStrandIncrement", &m_EndAdjustment.m_StrandIncrement))
            THROW_LOAD(InvalidFileFormat,pLoad);
         
            if(!pLoad->Property("HPStrandIncrement", &m_HPAdjustment.m_StrandIncrement))
            THROW_LOAD(InvalidFileFormat,pLoad);
         }
         else
         {

            if(!pLoad->Property("EndAllowVertAdjustment", &m_EndAdjustment.m_AllowVertAdjustment))
               THROW_LOAD(InvalidFileFormat,pLoad);

            if(!pLoad->Property("EndStrandIncrement", &m_EndAdjustment.m_StrandIncrement))
               THROW_LOAD(InvalidFileFormat,pLoad);

            long lface;
            if(!pLoad->Property("EndBottomFace", &lface))
               THROW_LOAD(InvalidFileFormat,pLoad); 

            m_EndAdjustment.m_BottomFace = lface==0 ? GirderTop : GirderBottom;

            if(!pLoad->Property("EndBottomLimit", &m_EndAdjustment.m_BottomLimit))
               THROW_LOAD(InvalidFileFormat,pLoad);

            if(!pLoad->Property("EndTopFace", &lface))
               THROW_LOAD(InvalidFileFormat,pLoad); 

            m_EndAdjustment.m_TopFace = lface==0 ? GirderTop : GirderBottom;

            if(!pLoad->Property("EndTopLimit", &m_EndAdjustment.m_TopLimit))
               THROW_LOAD(InvalidFileFormat,pLoad);

            if(!pLoad->Property("HPAllowVertAdjustment", &m_HPAdjustment.m_AllowVertAdjustment))
               THROW_LOAD(InvalidFileFormat,pLoad);

            if(!pLoad->Property("HPStrandIncrement", &m_HPAdjustment.m_StrandIncrement))
               THROW_LOAD(InvalidFileFormat,pLoad);

            if(!pLoad->Property("HPBottomFace", &lface))
               THROW_LOAD(InvalidFileFormat,pLoad); 

            m_HPAdjustment.m_BottomFace = lface==0 ? GirderTop : GirderBottom;

            if(!pLoad->Property("HPBottomLimit", &m_HPAdjustment.m_BottomLimit))
               THROW_LOAD(InvalidFileFormat,pLoad);

            if(!pLoad->Property("HPTopFace", &lface))
               THROW_LOAD(InvalidFileFormat,pLoad); 

            m_HPAdjustment.m_TopFace = lface==0 ? GirderTop : GirderBottom;

            if(!pLoad->Property("HPTopLimit", &m_HPAdjustment.m_TopLimit))
               THROW_LOAD(InvalidFileFormat,pLoad);

         }
      }


      Int32 maxStrandsInBottomBundle;
      Float64 bottomBundleLocation;
      Float64 topBundleLocation;
      if ( version <= 1.1 )
      {
         if(!pLoad->Property("MaxStrandsInBottomBundle", &maxStrandsInBottomBundle))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if(!pLoad->Property("BottomBundleLocation", &bottomBundleLocation))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if(!pLoad->Property("TopBundleLocation", &topBundleLocation))
            THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if(!pLoad->Property("ShearSteelBarSize", &m_ShearSteelBarSize))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if(!pLoad->Property("LastConfinementZone", &m_LastConfinementZone))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if(!pLoad->Property("HarpingPointLocation", &m_HarpingPointLocation))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if ( version <= 1.4 )
         m_HarpingPointLocation *= -1.0; // fractional distances < 0 starting with version 1.5

      if ( version <= 3.9 )
         m_HarpingPointLocation *= -1.0; // fractional distances are positive values again starting with version 4

      if ( 4.0 <= version )
      {
         if ( !pLoad->Property("UseMinHarpingPointLocation",&m_bMinHarpingPointLocation) )
            THROW_LOAD(InvalidFileFormat,pLoad);

         if ( m_bMinHarpingPointLocation && !pLoad->Property("MinHarpingPointLocation",&m_MinHarpingPointLocation) )
            THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( version <= 1.5 )
      {
         m_HarpPointReference = mlBearing;
      }
      else
      {
         long value;
         if(!pLoad->Property("HarpingPointReference", &value))
            THROW_LOAD(InvalidFileFormat,pLoad);

         m_HarpPointReference = (MeasurementLocation)value;
      }

      if ( 4 <= version )
      {
         long value;
         if ( !pLoad->Property("HarpingPointMeasure",&value))
            THROW_LOAD(InvalidFileFormat,pLoad);

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
         double H,W;
         if(!pLoad->Property("DiaphragmHeight", &H))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if(!pLoad->Property("DiaphragmWidth", &W))
            THROW_LOAD(InvalidFileFormat,pLoad);

         DiaphragmLayoutRule rule1;
         rule1.Description = "40-80ft";
         rule1.MinSpan = ::ConvertToSysUnits(40,unitMeasure::Feet);
         rule1.MaxSpan = ::ConvertToSysUnits(80,unitMeasure::Feet);
         rule1.Height = H;
         rule1.Thickness = W;
         rule1.MeasureLocation = mlBearing;
         rule1.MeasureType     = mtFractionOfSpanLength;
         rule1.Construction    = ctBridgeSite;
         rule1.Type = dtExternal;
         rule1.Location = 0.5;
         m_DiaphragmLayoutRules.push_back(rule1);

         DiaphragmLayoutRule rule2a;
         rule2a.Description = "80-120ft (Mid-Span)";
         rule2a.MinSpan = ::ConvertToSysUnits(80,unitMeasure::Feet);
         rule2a.MaxSpan = ::ConvertToSysUnits(120,unitMeasure::Feet);
         rule2a.Height = H;
         rule2a.Thickness = W;
         rule2a.MeasureLocation = mlBearing;
         rule2a.MeasureType     = mtFractionOfSpanLength;
         rule2a.Construction    = ctBridgeSite;
         rule2a.Type = dtExternal;
         rule2a.Location = 0.5;
         m_DiaphragmLayoutRules.push_back(rule2a);

         DiaphragmLayoutRule rule2b;
         rule2b.Description = "80-120ft (1/3 points)";
         rule2b.MinSpan = ::ConvertToSysUnits(80,unitMeasure::Feet);
         rule2b.MaxSpan = ::ConvertToSysUnits(120,unitMeasure::Feet);
         rule2b.Height = H;
         rule2b.Thickness = W;
         rule2b.MeasureLocation = mlBearing;
         rule2b.MeasureType     = mtFractionOfSpanLength;
         rule2b.Construction    = ctBridgeSite;
         rule2b.Type = dtExternal;
         rule2b.Location = 0.3333333333;
         m_DiaphragmLayoutRules.push_back(rule2b);

         DiaphragmLayoutRule rule3a;
         rule3a.Description = "Greater than 120ft (Mid-Span)";
         rule3a.MinSpan = ::ConvertToSysUnits(120,unitMeasure::Feet);
         rule3a.MaxSpan = ::ConvertToSysUnits(999,unitMeasure::Feet);
         rule3a.Height = H;
         rule3a.Thickness = W;
         rule3a.MeasureLocation = mlBearing;
         rule3a.MeasureType     = mtFractionOfSpanLength;
         rule3a.Construction    = ctBridgeSite;
         rule3a.Type = dtExternal;
         rule3a.Location = 0.5;
         m_DiaphragmLayoutRules.push_back(rule3a);

         DiaphragmLayoutRule rule3b;
         rule3b.Description = "Greater than 120ft (1/4 points)";
         rule3b.MinSpan = ::ConvertToSysUnits(120,unitMeasure::Feet);
         rule3b.MaxSpan = ::ConvertToSysUnits(999,unitMeasure::Feet);
         rule3b.Height = H;
         rule3b.Thickness = W;
         rule3b.MeasureLocation = mlBearing;
         rule3b.MeasureType     = mtFractionOfSpanLength;
         rule3b.Construction    = ctBridgeSite;
         rule3b.Type = dtExternal;
         rule3b.Location = 0.25;
         m_DiaphragmLayoutRules.push_back(rule3b);

         bOldIntermediateDiaphragms = true;
      }

      if(!pLoad->Property("ShearSteelMaterial", &m_ShearSteelMaterial))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if(!pLoad->Property("LongSteelMaterial",  &m_LongSteelMaterial))
         THROW_LOAD(InvalidFileFormat,pLoad);

      // Bug fix: There was an error saving m_LongSteelMaterial that caused it to be blank. This
      //          caused no harm until we needed the value. Reset it to correct value if blank
      if (m_LongSteelMaterial.empty())
         m_LongSteelMaterial = SMaterialName;

      // top flange shear steel
      if ( version >= 1.8 )
      {
         if ( !pLoad->Property("DoStirrupsEngageDeck",    &m_bStirrupsEngageDeck) )
            THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if (version>=1.1)
      {
         if(!pLoad->Property("TopFlangeShearBarSize",  &m_TopFlangeShearBarSize))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if(!pLoad->Property("TopFlangeShearBarSpacing",  &m_TopFlangeShearBarSpacing))
            THROW_LOAD(InvalidFileFormat,pLoad);
      }
      else
      {
          m_TopFlangeShearBarSize = 0;
          m_TopFlangeShearBarSpacing = 0.0;
      }

      // debond criteria - added in version 13
      if (12 < version)
      {
         if(!pLoad->BeginUnit("DebondingCritia"))
            THROW_LOAD(InvalidFileFormat,pLoad);

         Float64 local_vers = pLoad->GetVersion();
         
            if (local_vers!=1.0)
               THROW_LOAD(InvalidFileFormat,pLoad);

            if ( !pLoad->Property("MaxDebondStrands",               &m_MaxDebondStrands))
               THROW_LOAD(BadVersion,pLoad);

            if ( !pLoad->Property("MaxDebondStrandsPerRow",         &m_MaxDebondStrandsPerRow))
               THROW_LOAD(BadVersion,pLoad);

            if ( !pLoad->Property("MaxNumDebondedStrandsPerSection",&m_MaxNumDebondedStrandsPerSection))
               THROW_LOAD(BadVersion,pLoad);

            if ( !pLoad->Property("MaxDebondedStrandsPerSection",   &m_MaxDebondedStrandsPerSection))
               THROW_LOAD(BadVersion,pLoad);

            if ( !pLoad->Property("MinDebondLength",                &m_MinDebondLength))
               THROW_LOAD(BadVersion,pLoad);

            if ( !pLoad->Property("DefaultDebondLength",            &m_DefaultDebondLength))
               THROW_LOAD(BadVersion,pLoad);

            if ( !pLoad->Property("MaxDebondLengthBySpanFraction",  &m_MaxDebondLengthBySpanFraction))
               THROW_LOAD(BadVersion,pLoad);

            if ( !pLoad->Property("MaxDebondLengthByHardDistance",  &m_MaxDebondLengthByHardDistance))
               THROW_LOAD(BadVersion,pLoad);

         if ( !pLoad->EndUnit())
            THROW_LOAD(BadVersion,pLoad);
      }

      // old global debonding of strands. transfer this value to all straight strands in girder
      bool canDebondStraightStrands(false);
      if ( 1.6 < version && version < 12.0)
      {
         if ( !pLoad->Property("CanDebondStraightStrands",&canDebondStraightStrands) )
            THROW_LOAD(InvalidFileFormat,pLoad);
      }

      // straight strand locations
      m_StraightStrands.clear();
      if ( version < 15 )
      {
         while(pLoad->BeginUnit("StraightStrandLocations"))
         {
            Float64 local_vers = pLoad->GetVersion();
            
            if (local_vers!=1.0 && local_vers!=2.0)
               THROW_LOAD(BadVersion,pLoad);

            StraightStrandLocation locs;
            if(!pLoad->Property("X", &locs.m_Xstart))
               THROW_LOAD(InvalidFileFormat,pLoad);

            if(!pLoad->Property("Y", &locs.m_Ystart))
               THROW_LOAD(InvalidFileFormat,pLoad);

            locs.m_Xend = locs.m_Xstart;
            locs.m_Yend = locs.m_Ystart;

            if (local_vers>=2.0)
            {
               if(!pLoad->Property("CanDebond", &locs.m_bCanDebond))
                  THROW_LOAD(InvalidFileFormat,pLoad);
            }
            else
            {
               locs.m_bCanDebond = canDebondStraightStrands; // old version was all or none
            }

            if(!pLoad->EndUnit())
               THROW_LOAD(InvalidFileFormat,pLoad);

            m_StraightStrands.push_back(locs);
         }
      }
      else
      {
         if ( !pLoad->BeginUnit("StraightStrands") )
            THROW_LOAD(InvalidFileFormat,pLoad);

         while( pLoad->BeginUnit("StrandLocation") )
         {
            StraightStrandLocation strandLocation;
            if(!pLoad->Property("Xstart", &strandLocation.m_Xstart))
               THROW_LOAD(InvalidFileFormat,pLoad);

            if(!pLoad->Property("Ystart", &strandLocation.m_Ystart))
               THROW_LOAD(InvalidFileFormat,pLoad);

            if(!pLoad->Property("Xend", &strandLocation.m_Xend))
               THROW_LOAD(InvalidFileFormat,pLoad);

            if(!pLoad->Property("Yend", &strandLocation.m_Yend))
               THROW_LOAD(InvalidFileFormat,pLoad);

            if(!pLoad->Property("CanDebond", &strandLocation.m_bCanDebond))
               THROW_LOAD(InvalidFileFormat,pLoad);

            if(!pLoad->EndUnit()) // StrandLocation
               THROW_LOAD(InvalidFileFormat,pLoad);

            m_StraightStrands.push_back(strandLocation);
         }

         // StraightStrands
         if(!pLoad->EndUnit())
            THROW_LOAD(InvalidFileFormat,pLoad);
      }

      // Harped Strands
      m_HarpedStrands.clear();
      if ( version < 15 )
      {
         if ( 5.0 <= version )
         {
            while(pLoad->BeginUnit("HarpedStrandLocations"))
            {
               if(pLoad->GetVersion()!=2.0)
                  THROW_LOAD(BadVersion,pLoad);

               HarpedStrandLocation strandLocation;
               if(!pLoad->Property("HpX", &strandLocation.m_Xhp))
                  THROW_LOAD(InvalidFileFormat,pLoad);

               if(!pLoad->Property("HpY", &strandLocation.m_Yhp))
                  THROW_LOAD(InvalidFileFormat,pLoad);

               if(!pLoad->Property("EndX", &strandLocation.m_Xstart))
                  THROW_LOAD(InvalidFileFormat,pLoad);

               if(!pLoad->Property("EndY", &strandLocation.m_Ystart))
                  THROW_LOAD(InvalidFileFormat,pLoad);

               strandLocation.m_Xend = strandLocation.m_Xstart;
               strandLocation.m_Yend = strandLocation.m_Ystart;

               if(!pLoad->EndUnit())
                  THROW_LOAD(InvalidFileFormat,pLoad);

               m_HarpedStrands.push_back(strandLocation);
            }

            pLoad->Property("OddNumberOfHarpedStrands",&m_bOddNumberOfHarpedStrands);

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
            while(pLoad->BeginUnit("HarpedStrandLocations"))
            {
               if(pLoad->GetVersion()!=1.0)
                  THROW_LOAD(BadVersion,pLoad);

               double x,y;
               if(!pLoad->Property("X", &x))
                  THROW_LOAD(InvalidFileFormat,pLoad);

               num_harped += x>0.0 ? 2 : 1; // track total number for hp strand computation

               if(!pLoad->Property("Y", &y))
                  THROW_LOAD(InvalidFileFormat,pLoad);

               if(!pLoad->EndUnit())
                  THROW_LOAD(InvalidFileFormat,pLoad);

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
            if ( version >= 1.2 )
            {
               while(pLoad->BeginUnit("HPStrandLocations"))
               {
                  if(pLoad->GetVersion()!=1.0)
                     THROW_LOAD(BadVersion,pLoad);

                  double x,y;
                  if(!pLoad->Property("X", &x))
                     THROW_LOAD(InvalidFileFormat,pLoad);

                  if(!pLoad->Property("Y", &y))
                     THROW_LOAD(InvalidFileFormat,pLoad);

                  if(!pLoad->EndUnit())
                     THROW_LOAD(InvalidFileFormat,pLoad);

                  CComPtr<IPoint2d> point;
                  point.CoCreateInstance(CLSID_Point2d);
                  point->Move(x,y);
                  pHPStrandLocations->Add(point);
               }

               pLoad->Property("OddNumberOfHarpedStrands",&m_bOddNumberOfHarpedStrands);
            }
            else
            {
               m_bOddNumberOfHarpedStrands = true;

               // Generate strand locations based on bundle definitions
               for ( Int32 i = 0; i < num_harped; i++ )
               {
                  double x,y;
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
            CollectionIndexType end_cnt, hp_cnt;
            pEndStrandLocations->get_Count(&end_cnt);
            pHPStrandLocations->get_Count(&hp_cnt);

            CollectionIndexType end_idx=0;
            for (CollectionIndexType hp_idx=0; hp_idx<hp_cnt; hp_idx++)
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
                  ATLASSERT(0);
                  THROW_LOAD(InvalidFileFormat,pLoad);
               }

               // store our data
               HarpedStrandLocation hp(endx, endy, hpx, hpy, endx, endy);
               m_HarpedStrands.push_back(hp);

               // now deal with issues related to X>0 and double locations
               // indexes must be incremented if double x==0's
               if (hpx>0.0 && IsZero(endx))
               {
                  // next end point better have x==0
                  Float64 x;
                  CComPtr<IPoint2d> end_pnt;
                  pEndStrandLocations->get_Item(end_idx++, &end_pnt); // note increment
                  end_pnt->get_X(&x);
                  if (!IsZero(x))
                  {
                     ATLASSERT(0);
                     THROW_LOAD(InvalidFileFormat,pLoad);
                  }
               }
               if (endx>0.0 && IsZero(hpx))
               {
                  // next HP point better have x==0
                  Float64 x;
                  CComPtr<IPoint2d> hp_pnt;
                  pHPStrandLocations->get_Item(++hp_idx, &hp_pnt); // note increment
                  hp_pnt->get_X(&x);
                  if (!IsZero(x))
                  {
                     ATLASSERT(0);
                     THROW_LOAD(InvalidFileFormat,pLoad);
                  }
               }
            }
            
            m_bUseDifferentHarpedGridAtEnds = true;

         }
      }
      else
      {
         if ( !pLoad->BeginUnit("HarpedStrands") )
            THROW_LOAD(InvalidFileFormat,pLoad);

         while( pLoad->BeginUnit("StrandLocation") )
         {
            HarpedStrandLocation strandLocation;
            if(!pLoad->Property("Xstart", &strandLocation.m_Xstart))
               THROW_LOAD(InvalidFileFormat,pLoad);

            if(!pLoad->Property("Ystart", &strandLocation.m_Ystart))
               THROW_LOAD(InvalidFileFormat,pLoad);

            if(!pLoad->Property("Xhp", &strandLocation.m_Xhp))
               THROW_LOAD(InvalidFileFormat,pLoad);

            if(!pLoad->Property("Yhp", &strandLocation.m_Yhp))
               THROW_LOAD(InvalidFileFormat,pLoad);

            if(!pLoad->Property("Xend", &strandLocation.m_Xend))
               THROW_LOAD(InvalidFileFormat,pLoad);

            if(!pLoad->Property("Yend", &strandLocation.m_Yend))
               THROW_LOAD(InvalidFileFormat,pLoad);

            // debond is ignore for temporary strands
            //if(!pLoad->Property("CanDebond", &strandLocation.m_bCanDebond))
            //   THROW_LOAD(InvalidFileFormat,pLoad);

            if(!pLoad->EndUnit()) // StrandLocation
               THROW_LOAD(InvalidFileFormat,pLoad);

            m_HarpedStrands.push_back(strandLocation);
         }

         // HarpedStrands
         if(!pLoad->EndUnit())
            THROW_LOAD(InvalidFileFormat,pLoad);


         pLoad->Property("OddNumberOfHarpedStrands",&m_bOddNumberOfHarpedStrands);
      }

      // Temporary strands
      m_TemporaryStrands.clear();
      if ( version < 15 )
      {
         while(pLoad->BeginUnit("TemporaryStrandLocations"))
         {
            if(pLoad->GetVersion()!=1.0)
               THROW_LOAD(BadVersion,pLoad);

            double x,y;
            if(!pLoad->Property("X", &x))
               THROW_LOAD(InvalidFileFormat,pLoad);

            if(!pLoad->Property("Y", &y))
               THROW_LOAD(InvalidFileFormat,pLoad);

            if(!pLoad->EndUnit())
               THROW_LOAD(InvalidFileFormat,pLoad);

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
         if ( !pLoad->BeginUnit("TemporaryStrands") )
            THROW_LOAD(InvalidFileFormat,pLoad);

         while( pLoad->BeginUnit("StrandLocation") )
         {
            StraightStrandLocation strandLocation;
            if(!pLoad->Property("Xstart", &strandLocation.m_Xstart))
               THROW_LOAD(InvalidFileFormat,pLoad);

            if(!pLoad->Property("Ystart", &strandLocation.m_Ystart))
               THROW_LOAD(InvalidFileFormat,pLoad);

            if(!pLoad->Property("Xend", &strandLocation.m_Xend))
               THROW_LOAD(InvalidFileFormat,pLoad);

            if(!pLoad->Property("Yend", &strandLocation.m_Yend))
               THROW_LOAD(InvalidFileFormat,pLoad);

            // debond is ignore for temporary strands
            //if(!pLoad->Property("CanDebond", &strandLocation.m_bCanDebond))
            //   THROW_LOAD(InvalidFileFormat,pLoad);

            if(!pLoad->EndUnit()) // StrandLocation
               THROW_LOAD(InvalidFileFormat,pLoad);

            m_TemporaryStrands.push_back(strandLocation);
         }

         // TemporaryStrands
         if(!pLoad->EndUnit())
            THROW_LOAD(InvalidFileFormat,pLoad);
      }

      // global strand order
      m_GlobalStrandOrder.clear();

      if (version < 5.0)
      {
         // must convert old data to new - just pile them together
         // straight strand locations
         Uint16 count = m_StraightStrands.size();
         Uint16 idx;
         for ( idx=0; idx<count; idx++)
         {
            m_GlobalStrandOrder.push_back( GlobalStrand(stStraight, idx) );
         }

         // harped strands
         count = m_HarpedStrands.size();
         for ( idx=0; idx<count; idx++)
         {
            m_GlobalStrandOrder.push_back( GlobalStrand(stHarped, idx) );
         }

      }
      else
      {
         while(pLoad->BeginUnit("GlobalStrandOrder"))
         {
            if(pLoad->GetVersion()!=1.0)
               THROW_LOAD(BadVersion,pLoad);

            std::string name;
            if(!pLoad->Property("StrandType",&name))
               THROW_LOAD(InvalidFileFormat,pLoad);

            GlobalStrand gstrand;

            if (name == "Straight")
            {
               gstrand.m_StrandType = stStraight;
            }
            else if (name == "Harped")
            {
               gstrand.m_StrandType = stHarped;
            }
            else
            {
               ATLASSERT(0);
               THROW_LOAD(InvalidFileFormat,pLoad);
            }
         
            if(!pLoad->Property("LocalSortOrder", &gstrand.m_LocalSortOrder))
               THROW_LOAD(InvalidFileFormat,pLoad);


            if(!pLoad->EndUnit())
               THROW_LOAD(InvalidFileFormat,pLoad);

            m_GlobalStrandOrder.push_back(gstrand);
         }

         if (version < 7.0)
         {
            // added in version 5, removed in 7
         long smtype;
         if(!pLoad->Property("StressMitigationType", &smtype))
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

      }

      // shear zones
      ShearZoneInfo zi;
      m_ShearZoneInfo.clear();
      while(pLoad->BeginUnit("ShearZones"))
      {
         double shear_zone_version = pLoad->GetVersion();

         if(2 < shear_zone_version )
            THROW_LOAD(BadVersion,pLoad);

         if ( shear_zone_version < 2 )
         {
            if(!pLoad->Property("ZoneLength",&zi.ZoneLength))
               THROW_LOAD(InvalidFileFormat,pLoad);

            if(!pLoad->Property("BarSize", &zi.VertBarSize))
               THROW_LOAD(InvalidFileFormat,pLoad);


            if ( version < 11.0 )
            {
               if(!pLoad->Property("SS", &zi.StirrupSpacing))
                  THROW_LOAD(InvalidFileFormat,pLoad);
            }
            else
            {
               if(!pLoad->Property("Spacing", &zi.StirrupSpacing))
                  THROW_LOAD(InvalidFileFormat,pLoad);
            }

            if ( 1.1 <= shear_zone_version )
            {
               ATLASSERT( shear_zone_version < 2 );

               if ( !pLoad->Property("NBars", &zi.nVertBars) )
                  THROW_LOAD(InvalidFileFormat,pLoad);
            }
            else
            {
               zi.nVertBars = 2;
            }

            zi.HorzBarSize = 0;
            zi.nHorzBars   = 2;
         }
         else
         {
            if(!pLoad->Property("ZoneLength",&zi.ZoneLength))
               THROW_LOAD(InvalidFileFormat,pLoad);

            if(!pLoad->Property("Spacing", &zi.StirrupSpacing))
               THROW_LOAD(InvalidFileFormat,pLoad);

            if(!pLoad->Property("VertBarSize", &zi.VertBarSize))
               THROW_LOAD(InvalidFileFormat,pLoad);

            if(!pLoad->Property("VertBars", &zi.nVertBars))
               THROW_LOAD(InvalidFileFormat,pLoad);

            if(!pLoad->Property("HorzBarSize", &zi.HorzBarSize))
               THROW_LOAD(InvalidFileFormat,pLoad);

            if(!pLoad->Property("HorzBars", &zi.nHorzBars))
               THROW_LOAD(InvalidFileFormat,pLoad);
         }


         if(!pLoad->EndUnit())
            THROW_LOAD(InvalidFileFormat,pLoad);

         m_ShearZoneInfo.push_back(zi);
      }

      // Longitudinal Steel rows
      LongSteelInfo li;
      std::string tmp;
      m_LongSteelInfo.clear();
      while(pLoad->BeginUnit("LongSteelInfo"))
      {
         if(pLoad->GetVersion()!=1.0)
            THROW_LOAD(BadVersion,pLoad);

         if(!pLoad->Property("Face", &tmp))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if (tmp=="Bottom")
            li.Face = GirderLibraryEntry::GirderBottom;
         else if (tmp=="Top")
            li.Face = GirderLibraryEntry::GirderTop;
         else
            THROW_LOAD(InvalidFileFormat,pLoad);

         if(!pLoad->Property("NumberOfBars", &li.NumberOfBars))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if(!pLoad->Property("BarSize",  &li.BarSize))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if(!pLoad->Property("BarCover", &li.Cover))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if(!pLoad->Property("BarSpacing", &li.BarSpacing))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if(!pLoad->EndUnit())
            THROW_LOAD(InvalidFileFormat,pLoad);

         m_LongSteelInfo.push_back(li);
      }
   

      if ( !bOldIntermediateDiaphragms )
         m_DiaphragmLayoutRules.clear();

      while ( pLoad->BeginUnit("DiaphragmLayoutRule") )
      {
         DiaphragmLayoutRule dlr;

         if ( !pLoad->Property("Description",&dlr.Description) )
            THROW_LOAD(InvalidFileFormat,pLoad);

         if ( !pLoad->Property("MinSpan",&dlr.MinSpan) )
            THROW_LOAD(InvalidFileFormat,pLoad);

         if ( !pLoad->Property("MaxSpan",&dlr.MaxSpan) )
            THROW_LOAD(InvalidFileFormat,pLoad);

         if ( !pLoad->Property("Height",&dlr.Height) )
            THROW_LOAD(InvalidFileFormat,pLoad);

         if ( !pLoad->Property("Thickness",&dlr.Thickness) )
            THROW_LOAD(InvalidFileFormat,pLoad);

         long value;
         if ( !pLoad->Property("DiaphragmType",&value) )
            THROW_LOAD(InvalidFileFormat,pLoad);
         dlr.Type = (DiaphragmType)(value);

         if ( !pLoad->Property("ConstructionType",&value) )
            THROW_LOAD(InvalidFileFormat,pLoad);
         dlr.Construction = (ConstructionType)(value);

         if ( !pLoad->Property("MeasurementType",&value) )
            THROW_LOAD(InvalidFileFormat,pLoad);
         dlr.MeasureType = (MeasurementType)(value);

         if ( !pLoad->Property("MeasurmentLocation",&value) )
            THROW_LOAD(InvalidFileFormat,pLoad);
         dlr.MeasureLocation = (MeasurementLocation)(value);

         if ( !pLoad->Property("Location",&dlr.Location) )
            THROW_LOAD(InvalidFileFormat,pLoad);

         m_DiaphragmLayoutRules.push_back(dlr);

         if ( !pLoad->EndUnit() )
            THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if(!pLoad->EndUnit())
         THROW_LOAD(InvalidFileFormat,pLoad);
   }
   else
   {
      return false; // not a GirderLibraryEntry
   }

   return true;
}

HRESULT GirderLibraryEntry::CreateBeamFactory(const std::string& strCLSID)
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
      ::CLSIDFromString(A2OLE(strCLSID.c_str()),&clsid);

      CComPtr<IClassFactory> classFactory;
      HRESULT hr = ::CoGetClassObject(clsid,CLSCTX_ALL,NULL,IID_IClassFactory,(void**)&classFactory);
      if ( FAILED(hr) )
         return hr;

      CClassFactoryHolder cfh(classFactory);

      // Save it for next time
      std::pair<ClassFactoryCollection::iterator,bool> result;
      result = ms_ClassFactories.insert(std::make_pair(strCLSID,cfh));
      ClassFactoryCollection::iterator iter = result.first;
      pClassFactory = &(*iter).second;
   }

   // Using the class factory, create the beam factory
   HRESULT hr = pClassFactory->CreateInstance(NULL,IID_IBeamFactory,(void**)&m_pBeamFactory);
   return hr;
}

void GirderLibraryEntry::LoadIBeamDimensions(sysIStructuredLoad* pLoad)
{
   CLSID clsid;
   ::CLSIDFromString(L"{30962206-2412-4001-AA20-CF359BC60142}",&clsid);
   m_pBeamFactory.Release();
   HRESULT hr = ::CoCreateInstance(clsid,NULL,CLSCTX_ALL,IID_IBeamFactory,(void**)&m_pBeamFactory);

   double value;
   if(!pLoad->Property("D1", &value))
      THROW_LOAD(InvalidFileFormat,pLoad);

   m_Dimensions.push_back(Dimension("D1",value));

   if(!pLoad->Property("D2", &value))
      THROW_LOAD(InvalidFileFormat,pLoad);

   m_Dimensions.push_back(Dimension("D2",value));

   if(!pLoad->Property("D3", &value))
      THROW_LOAD(InvalidFileFormat,pLoad);

   m_Dimensions.push_back(Dimension("D3",value));

   if(!pLoad->Property("D4", &value))
      THROW_LOAD(InvalidFileFormat,pLoad);

   m_Dimensions.push_back(Dimension("D4",value));

   if(!pLoad->Property("D5", &value))
      THROW_LOAD(InvalidFileFormat,pLoad);

   m_Dimensions.push_back(Dimension("D5",value));

   if(!pLoad->Property("D6", &value))
      THROW_LOAD(InvalidFileFormat,pLoad);

   m_Dimensions.push_back(Dimension("D6",value));

   if(!pLoad->Property("D7", &value))
      THROW_LOAD(InvalidFileFormat,pLoad);

   m_Dimensions.push_back(Dimension("D7",value));

   if(!pLoad->Property("W1", &value))
      THROW_LOAD(InvalidFileFormat,pLoad);

   m_Dimensions.push_back(Dimension("W1",value));

   if(!pLoad->Property("W2", &value))
      THROW_LOAD(InvalidFileFormat,pLoad);

   m_Dimensions.push_back(Dimension("W2",value));

   if(!pLoad->Property("W3", &value))
      THROW_LOAD(InvalidFileFormat,pLoad);

   m_Dimensions.push_back(Dimension("W3",value));

   if(!pLoad->Property("W4", &value))
      THROW_LOAD(InvalidFileFormat,pLoad);

   m_Dimensions.push_back(Dimension("W4",value));

   if(!pLoad->Property("T1", &value))
      THROW_LOAD(InvalidFileFormat,pLoad);

   m_Dimensions.push_back(Dimension("T1",value));

   if(!pLoad->Property("T2", &value))
      THROW_LOAD(InvalidFileFormat,pLoad);

   m_Dimensions.push_back(Dimension("T2",value));

   // C1 was added at a later date
   m_Dimensions.push_back(Dimension("C1", 0.0));
   m_Dimensions.push_back(Dimension("EndBlockWidth",0.0));
   m_Dimensions.push_back(Dimension("EndBlockLength",0.0));
   m_Dimensions.push_back(Dimension("EndBlockTransition",0.0));
}

bool GirderLibraryEntry::IsEqual(const GirderLibraryEntry& rOther, bool considerName) const
{
   bool test = true;

   test &= (m_bUseDifferentHarpedGridAtEnds       == rOther.m_bUseDifferentHarpedGridAtEnds);
   test &= (m_bOddNumberOfHarpedStrands           == rOther.m_bOddNumberOfHarpedStrands);

   test &= (m_HPAdjustment             == rOther.m_HPAdjustment);
   test &= (m_EndAdjustment            == rOther.m_EndAdjustment);
   test &= (m_ShearSteelBarSize        == rOther.m_ShearSteelBarSize);
   test &= (m_LastConfinementZone      == rOther.m_LastConfinementZone);
   test &= (m_HarpingPointLocation     == rOther.m_HarpingPointLocation);
   test &= (m_bMinHarpingPointLocation == rOther.m_bMinHarpingPointLocation);

   if ( m_bMinHarpingPointLocation )
      test &= (m_MinHarpingPointLocation == rOther.m_MinHarpingPointLocation);

   test &= (m_HarpPointReference       == rOther.m_HarpPointReference);
   test &= (m_HarpPointMeasure         == rOther.m_HarpPointMeasure);

   test &= (m_StraightStrands  == rOther.m_StraightStrands);
   test &= (m_HarpedStrands    == rOther.m_HarpedStrands);
   test &= (m_TemporaryStrands == rOther.m_TemporaryStrands);

   test &= (m_GlobalStrandOrder == rOther.m_GlobalStrandOrder);

   test &= (m_ShearZoneInfo == rOther.m_ShearZoneInfo);
   test &= (m_LongSteelInfo == rOther.m_LongSteelInfo);

   test &= (m_TopFlangeShearBarSize    == rOther.m_TopFlangeShearBarSize);
   test &= (m_TopFlangeShearBarSpacing == rOther.m_TopFlangeShearBarSpacing);

   test &= (m_MaxDebondStrands == rOther.m_MaxDebondStrands);
   test &= (m_MaxDebondStrandsPerRow == rOther.m_MaxDebondStrandsPerRow);
   test &= (m_MaxNumDebondedStrandsPerSection == rOther.m_MaxNumDebondedStrandsPerSection);
   test &= (m_MaxDebondedStrandsPerSection == rOther.m_MaxDebondedStrandsPerSection);
   test &= (m_MinDebondLength == rOther.m_MinDebondLength);
   test &= (m_DefaultDebondLength == rOther.m_DefaultDebondLength);

   test &= (m_MaxDebondLengthBySpanFraction == rOther.m_MaxDebondLengthBySpanFraction);
   test &= (m_MaxDebondLengthByHardDistance == rOther.m_MaxDebondLengthByHardDistance);

   test &= (m_ShearSteelMaterial == rOther.m_ShearSteelMaterial);
   test &= (m_LongSteelMaterial == rOther.m_LongSteelMaterial);

   test &= (m_Dimensions == rOther.m_Dimensions);

   test &= (m_bStirrupsEngageDeck == rOther.m_bStirrupsEngageDeck);

   test &= (m_DiaphragmLayoutRules == rOther.m_DiaphragmLayoutRules);

   test &= (m_bCanPostTension == rOther.m_bCanPostTension);

   if (considerName)
      test &= this->GetName()==rOther.GetName();

   return test;
}

void GirderLibraryEntry::ValidateData(GirderLibraryEntry::GirderEntryDataErrorVec* pvec)
{
   pvec->clear();

   // check if strands are in girder and other possible issues
   CComPtr<IGirderSection> gdrSection;
   m_pBeamFactory->CreateGirderSection(NULL,DUMMY_AGENT_ID,INVALID_INDEX,INVALID_INDEX,m_Dimensions,&gdrSection);

   double end_increment = this->GetEndStrandIncrement();
   double hp_increment  = this->GetHPStrandIncrement();

   GirderFace endTopFace, endBottomFace;
   Float64 endTopLimit, endBottomLimit;
   this->GetEndAdjustmentLimits(&endTopFace, &endTopLimit, &endBottomFace, &endBottomLimit);

   IBeamFactory::BeamFace etf = endTopFace==GirderBottom ? IBeamFactory::BeamBottom : IBeamFactory::BeamTop;
   IBeamFactory::BeamFace ebf = endBottomFace==GirderBottom ? IBeamFactory::BeamBottom : IBeamFactory::BeamTop;

   GirderFace hpTopFace, hpBottomFace;
   Float64 hpTopLimit, hpBottomLimit;
   this->GetHPAdjustmentLimits(&hpTopFace, &hpTopLimit, &hpBottomFace, &hpBottomLimit);
 
   IBeamFactory::BeamFace htf = hpTopFace==GirderBottom ? IBeamFactory::BeamBottom : IBeamFactory::BeamTop;
   IBeamFactory::BeamFace hbf = hpBottomFace==GirderBottom ? IBeamFactory::BeamBottom : IBeamFactory::BeamTop;

   CComPtr<IStrandMover> strand_mover;
   m_pBeamFactory->CreateStrandMover(m_Dimensions, 
                                     etf, endTopLimit, ebf, endBottomLimit,
                                     htf, hpTopLimit,  hbf, hpBottomLimit,
                                     end_increment, hp_increment, &strand_mover);

   double height;
   gdrSection->get_GirderHeight(&height);

   if (IsZero(height))
   {
      pvec->push_back(GirderEntryDataError(GirderHeightIsZero,
         "The height of the girder must be greater than zero."));
      // this will cause so many problems that we might as well just return here
      return;
   }

   double botwidth;
   gdrSection->get_BottomWidth(&botwidth);
   if (IsZero(botwidth))
   {
      pvec->push_back(GirderEntryDataError(BottomFlangeWidthIsZero,
         "The width of the girder bottom flange must be greater than zero."));
      return;
   }

   double topwidth;
   gdrSection->get_TopWidth(&topwidth);
   if (IsZero(topwidth))
   {
      pvec->push_back(GirderEntryDataError(TopFlangeWidthIsZero,
         "The width of the girder must be greater than zero."));
      return;
   }

   CComQIPtr<IShape> shape(gdrSection);
   ATLASSERT(shape != NULL); // girder section must support the shape interface

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
   for (GlobalStrandOrderIterator g_iter=m_GlobalStrandOrder.begin(); g_iter!=m_GlobalStrandOrder.end(); g_iter++)
   {
      const GlobalStrand& gstrand = *g_iter;

      if (gstrand.m_StrandType==stStraight)
      {
         // straight strands
         const StraightStrandLocation& strandLocation = m_StraightStrands[gstrand.m_LocalSortOrder];

         point->Move(strandLocation.m_Xstart, strandLocation.m_Ystart);

         VARIANT_BOOL bPointInShape;
         shape->PointInShape( point,&bPointInShape );
         if ( bPointInShape == VARIANT_FALSE )
         {
            std::ostringstream os;
            os << "Straight strand #"<<total_num<<" is outside of the girder section";
            pvec->push_back(GirderEntryDataError(StraightStrandOutsideOfGirder, os.str(), total_num));
         }

         total_num += (0 < strandLocation.m_Xstart || 0 < strandLocation.m_Xend ? 2 : 1);
      }
      else if (gstrand.m_StrandType==stHarped)
      {
         // harped strands at HP
         const HarpedStrandLocation& strandLocation = m_HarpedStrands[gstrand.m_LocalSortOrder];

         point->Move(strandLocation.m_Xhp, strandLocation.m_Yhp);

         VARIANT_BOOL bPointInShape;
         shape->PointInShape( point,&bPointInShape );
         if ( bPointInShape == VARIANT_FALSE )
         {
            std::ostringstream os;
            os << "Harped strand #"<<total_num<<" at harping point is outside of the girder section";
            pvec->push_back(GirderEntryDataError(HarpedStrandOutsideOfGirder, os.str(), total_num));
         }


         VARIANT_BOOL is_within;
         strand_mover->TestHpStrandLocation(strandLocation.m_Xhp, strandLocation.m_Yhp, 0.0, &is_within);
         if (is_within!=VARIANT_TRUE)
         {
            std::ostringstream os;
            os << "Harped strand #"<<total_num<<" at harping point must be within offset bounds and lie within the thinnest portion of a web";
            pvec->push_back(GirderEntryDataError(HarpedStrandOutsideOfGirder, os.str(), total_num));
         }

         // next check zero value if odd number of strands is allowed
         if (this->OddNumberOfHarpedStrands())
         {
            // must have all X>0 for odd number of strands case
            if ( IsZero(strandLocation.m_Xhp) && IsZero(strandLocation.m_Xstart) && IsZero(strandLocation.m_Xend) )
            {
               std::ostringstream os;
               os << "Harped strand #"<<total_num<<" has zero X value at HP and End. This cannot be the case if odd number of harped strands is allowed.";
               pvec->push_back(GirderEntryDataError(HarpedStrandOutsideOfGirder, os.str(), total_num));
            }

            // check locations of zero X
            point->Move(0.0, strandLocation.m_Yhp);

            shape->PointInShape( point,&bPointInShape );
            if ( bPointInShape == VARIANT_FALSE )
            {
               std::ostringstream os;
               os << "Odd Harped strand #"<<total_num<<" at harping point is outside of the girder section. Disable odd strands";
               pvec->push_back(GirderEntryDataError(HarpedStrandOutsideOfGirder, os.str(), total_num));
            }

            strand_mover->TestHpStrandLocation(0.0, strandLocation.m_Yhp, 0.0, &is_within);
            if (is_within!=VARIANT_TRUE)
            {
               std::ostringstream os;
               os << "Odd Harped strand #"<<total_num<<" at harping point must be within offset bounds and lie within the thinnest portion of a web. Disable odd strands";
               pvec->push_back(GirderEntryDataError(HarpedStrandOutsideOfGirder, os.str(), total_num));
            }
         }

         // Harped strands at ends
         if (m_bUseDifferentHarpedGridAtEnds)
         {
            // start
            point->Move(strandLocation.m_Xstart, strandLocation.m_Ystart);

            VARIANT_BOOL bPointInShape;
            shape->PointInShape( point,&bPointInShape );
            if ( bPointInShape == VARIANT_FALSE )
            {
               std::ostringstream os;
               os << "Harped strand #"<<total_num<<" at girder end is outside of the girder section";
               pvec->push_back(GirderEntryDataError(HarpedStrandOutsideOfGirder, os.str(), total_num));
            }


            strand_mover->TestEndStrandLocation(strandLocation.m_Xstart, strandLocation.m_Ystart, 0.0, &is_within);
            if (is_within!=VARIANT_TRUE)
            {
               std::ostringstream os;
               os << "Harped strand #"<<total_num<<" at girder end must be within offset bounds and lie within the thinnest portion of a web";
               pvec->push_back(GirderEntryDataError(HarpedStrandOutsideOfGirder, os.str(), total_num));
            }

            // next check zero value if odd number of strands is allowed
            if (this->OddNumberOfHarpedStrands())
            {
               point->Move(0.0, strandLocation.m_Ystart);

               shape->PointInShape( point,&bPointInShape );
               if ( bPointInShape == VARIANT_FALSE )
               {
                  std::ostringstream os;
                  os << "Odd Harped strand #"<<total_num<<" at girder end is outside of the girder section. Disable odd strands";
                  pvec->push_back(GirderEntryDataError(HarpedStrandOutsideOfGirder, os.str(), total_num));
               }

               strand_mover->TestEndStrandLocation(0.0, strandLocation.m_Ystart, 0.0, &is_within);
               if (is_within!=VARIANT_TRUE)
               {
                  std::ostringstream os;
                  os << "Odd Harped strand #"<<total_num<<" at girder end must be within offset bounds and lie within the thinnest portion of a web. Disable odd strands";
                  pvec->push_back(GirderEntryDataError(HarpedStrandOutsideOfGirder, os.str(), total_num));
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
            //   std::ostringstream os;
            //   os << "Harped strand #"<<total_num<<" at girder end is outside of the girder section";
            //   pvec->push_back(GirderEntryDataError(HarpedStrandOutsideOfGirder, os.str(), total_num));
            //}


            //strand_mover->TestEndStrandLocation(strandLocation.m_Xend, strandLocation.m_Yend, 0.0, &is_within);
            //if (is_within!=VARIANT_TRUE)
            //{
            //   std::ostringstream os;
            //   os << "Harped strand #"<<total_num<<" at girder end must be within offset bounds and lie within the thinnest portion of a web";
            //   pvec->push_back(GirderEntryDataError(HarpedStrandOutsideOfGirder, os.str(), total_num));
            //}

            //// next check zero value if odd number of strands is allowed
            //if (this->OddNumberOfHarpedStrands())
            //{
            //   point->Move(0.0, strandLocation.m_Yend);

            //   shape->PointInShape( point,&bPointInShape );
            //   if ( bPointInShape == VARIANT_FALSE )
            //   {
            //      std::ostringstream os;
            //      os << "Odd Harped strand #"<<total_num<<" at girder end is outside of the girder section. Disable odd strands";
            //      pvec->push_back(GirderEntryDataError(HarpedStrandOutsideOfGirder, os.str(), total_num));
            //   }

            //   strand_mover->TestEndStrandLocation(0.0, strandLocation.m_Yend, 0.0, &is_within);
            //   if (is_within!=VARIANT_TRUE)
            //   {
            //      std::ostringstream os;
            //      os << "Odd Harped strand #"<<total_num<<" at girder end must be within offset bounds and lie within the thinnest portion of a web. Disable odd strands";
            //      pvec->push_back(GirderEntryDataError(HarpedStrandOutsideOfGirder, os.str(), total_num));
            //   }
            //}
         }

         total_num += ( (0<strandLocation.m_Xhp || 0<strandLocation.m_Xstart || 0<strandLocation.m_Xend) ? 2 : 1);
      }
      else
      {
         // temporary strands
         const StraightStrandLocation& strandLocation = m_TemporaryStrands[gstrand.m_LocalSortOrder];

         point->Move(strandLocation.m_Xstart, strandLocation.m_Ystart);

         VARIANT_BOOL bPointInShape;
         shape->PointInShape( point,&bPointInShape );
         if ( bPointInShape == VARIANT_FALSE )
         {
            std::ostringstream os;
            os << "Temporary strand #"<<total_num<<" is outside of the girder section";
            pvec->push_back(GirderEntryDataError(TemporaryStrandOutsideOfGirder, os.str(), total_num));
         }

         total_num += (0 < strandLocation.m_Xstart || 0 < strandLocation.m_Xend ? 2 : 1);
      }
   }


   // bundle adjustment
   if (end_increment>height/2.0)
   {
      pvec->push_back(GirderEntryDataError(BundleAdjustmentGtIncrement,
         "Harp bundle increment value at HP cannot be greater than half of the girder depth"));
   }

   // strand adjustment at end of girder
   if (hp_increment>height/2.0)
   {
      pvec->push_back(GirderEntryDataError(EndAdjustmentGtIncrement,
         "Upward adjustment increment value at end of girder cannot be greater than half of the girder depth"));
   }

   if (m_TopFlangeShearBarSize!=0 && m_TopFlangeShearBarSpacing <=0.0)
   {
      std::ostringstream os;
      os << "The top flange shear steel bar spacing must be greater than zero.";
      pvec->push_back(GirderEntryDataError(TopFlangeBarSpacingIsZero,os.str()));
   }

   // stirrups and shear zones
   Int32 num=1;
   int size = m_ShearZoneInfo.size();
   for(ShearZoneInfoVec::iterator its=m_ShearZoneInfo.begin(); its!=m_ShearZoneInfo.end(); its++)
   {
      // last zone has infinite length 
      if (num<size)
      {
         if(IsZero((*its).ZoneLength))
         {
            std::ostringstream os;
            os << "The length of shear zone #"<<num<<" must be greater than zero.";
            pvec->push_back(GirderEntryDataError(ZoneLengthIsZero,os.str(),num));
         }
      }

      if(IsZero((*its).StirrupSpacing) && ((*its).VertBarSize!=0 || (*its).HorzBarSize!=0))
      {
         std::ostringstream os;
         os << "The stirrup spacing in shear zone #"<<num<<" must be greater than zero because stirrups exist.";
         pvec->push_back(GirderEntryDataError(StirrupSpacingIsZero,os.str(),num));
      }

      if(IsZero((*its).StirrupSpacing) && num<=m_LastConfinementZone)
      {
         std::ostringstream os;
         os << "The stirrup spacing in shear zone #"<<num<<" must be greater than zero because it is a confinement zone.";
         pvec->push_back(GirderEntryDataError(StirrupSpacingIsZero,os.str(),num));
      }

      num++;
   }

   // long bars
   point.Release(); // I think it is already NULL, but do it again here just to be sure
   point.CoCreateInstance(CLSID_Point2d); // recycle point object
   num=1;
   for(LongSteelInfoVec::iterator itl=m_LongSteelInfo.begin(); itl!=m_LongSteelInfo.end(); itl++)
   {
      if ((*itl).Cover==0)
      {
         std::ostringstream os;
         os << "The cover for long. rebar row #"<<num<<" must be greater than zero.";
         pvec->push_back(GirderEntryDataError(BarCoverIsZero,os.str(),num));
      }

      if ((*itl).NumberOfBars==0)
      {
         std::ostringstream os;
         os << "The number of bars in long. rebar row #"<<num<<" must be greater than zero.";
         pvec->push_back(GirderEntryDataError(NumberOfBarsIsZero,os.str(),num));
      }

      if ((*itl).BarSpacing==0)
      {
         std::ostringstream os;
         os << "The bar spacing in long. rebar row #"<<num<<" must be greater than zero.";
         pvec->push_back(GirderEntryDataError(BarSpacingIsZero,os.str(),num));
      }

      // make sure bars are inside of girder - use shape symmetry
      gpPoint2d testpnt;
      testpnt.X() = (*itl).BarSpacing * ((*itl).NumberOfBars-1)/2.;
      if ((*itl).Face==GirderLibraryEntry::GirderBottom)
         testpnt.Y() = (*itl).Cover;
      else
         testpnt.Y() = height-(*itl).Cover;

      point->Move(testpnt.X(),testpnt.Y());
      VARIANT_BOOL bPointInShape;
      shape->PointInShape( point,&bPointInShape );
      if ( bPointInShape == VARIANT_FALSE )
      {
         std::ostringstream os;
         os << "Longitudinal rebar row #"<<num<<" is outside of the girder section";
         pvec->push_back(GirderEntryDataError(LongitudinalRebarOutsideOfGirder, os.str(), num));
      }

      num++;
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

//======================== ACCESS     =======================================
void GirderLibraryEntry::SetBeamFactory(IBeamFactory* pFactory)
{
   m_pBeamFactory = pFactory;

   // Clear out old dimensions
   m_Dimensions.clear();

   // Seed the dimension container
   std::vector<std::string> names = m_pBeamFactory->GetDimensionNames();
   std::vector<std::string>::iterator name_iter = names.begin();

   std::vector<double> dims = m_pBeamFactory->GetDefaultDimensions();
   std::vector<double>::iterator dim_iter = dims.begin();

   ATLASSERT( dims.size() == names.size() );

   for ( ; name_iter != names.end() && dim_iter != dims.end(); name_iter++, dim_iter++ )
   {
      std::string& rname = *name_iter;
      double value = *dim_iter;
      AddDimension(rname,value);
   }
}

void GirderLibraryEntry::GetBeamFactory(IBeamFactory** ppFactory) const
{
   (*ppFactory) = m_pBeamFactory;
   (*ppFactory)->AddRef();
}

std::string GirderLibraryEntry::GetGirderFamilyName() const
{
   return m_pBeamFactory->GetGirderFamilyName();
}

const GirderLibraryEntry::Dimensions& GirderLibraryEntry::GetDimensions() const
{
   return m_Dimensions;
}

double GirderLibraryEntry::GetDimension(const std::string& name) const
{
   Dimensions::const_iterator iter;
   for ( iter = m_Dimensions.begin(); iter != m_Dimensions.end(); iter++ )
   {
      const Dimension& dim = *iter;
      if ( name == dim.first )
         return dim.second;
   }

   ATLASSERT(false); // should never get here
   return -99999;
}

void GirderLibraryEntry::AddDimension(const std::string& name,double value)
{
   m_Dimensions.push_back(Dimension(name.c_str(),value));
}

void GirderLibraryEntry::SetDimension(const std::string& name,double value,bool bAdjustStrands)
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
      double deltaY[2];
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

StrandIndexType GirderLibraryEntry::GetNumStraightStrandCoordinates() const
{
   return m_StraightStrands.size();
}

void GirderLibraryEntry::GetStraightStrandCoordinates(StrandIndexType ssIndex, Float64* Xstart, Float64* Ystart, Float64* Xend, Float64* Yend, bool* canDebond) const
{
   ATLASSERT(ssIndex<(StrandIndexType)m_StraightStrands.size());

   const StraightStrandLocation& strandLocation = m_StraightStrands[ssIndex];

   *Xstart    = strandLocation.m_Xstart;
   *Ystart    = strandLocation.m_Ystart;
   *Xend      = strandLocation.m_Xend;
   *Yend      = strandLocation.m_Yend;
   *canDebond = strandLocation.m_bCanDebond;
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

void GirderLibraryEntry::GetHarpedStrandCoordinates(StrandIndexType hsIndex, Float64* Xstart,Float64* Ystart, Float64* Xhp, Float64* Yhp,Float64* Xend, Float64* Yend) const
{
   ATLASSERT(hsIndex>=0 && hsIndex<(StrandIndexType)m_HarpedStrands.size());

   const HarpedStrandLocation& strandLocation = m_HarpedStrands[hsIndex];
   *Xstart = strandLocation.m_Xstart;
   *Ystart = strandLocation.m_Ystart;
   *Xhp    = strandLocation.m_Xhp;
   *Yhp    = strandLocation.m_Yhp;
   *Xend   = strandLocation.m_Xend;
   *Yend   = strandLocation.m_Yend;
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

void GirderLibraryEntry::GetTemporaryStrandCoordinates(StrandIndexType tsIndex, Float64* Xstart, Float64* Ystart, Float64* Xend, Float64* Yend) const
{
   StrandIndexType cStrands = 0;

   for (ConstStraightStrandIterator iter = m_TemporaryStrands.begin(); iter != m_TemporaryStrands.end(); iter++)
   {
      const StraightStrandLocation& strandLocation = *iter;

      if ( cStrands == tsIndex || (0 < strandLocation.m_Xstart && cStrands+1 == tsIndex) )
      {
         *Xstart    = strandLocation.m_Xstart;
         *Ystart    = strandLocation.m_Ystart;
         *Xend      = strandLocation.m_Xend;
         *Yend      = strandLocation.m_Yend;
      //   *canDebond = strandLocation.m_bCanDebond; // debond is ignored for temporary strands
         return;
      }

      cStrands += ( 0 < strandLocation.m_Xstart ? 2 : 1 );
   }
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

StrandIndexType GirderLibraryEntry::GetMaxGlobalStrands() const
{
   return m_GlobalStrandOrder.size();
}

void GirderLibraryEntry::GetGlobalStrandAtFill(StrandIndexType index, psStrandType* type, StrandIndexType* localIndex) const
{
   ATLASSERT(0 <= index && index < (StrandIndexType)m_GlobalStrandOrder.size() );
   
   const GlobalStrand& strand = m_GlobalStrandOrder[index];
   *type       = strand.m_StrandType;
   *localIndex = strand.m_LocalSortOrder;
}

bool GirderLibraryEntry::ComputeGlobalStrands(StrandIndexType totalNumStrands, StrandIndexType* numStraight, StrandIndexType* numHarped) const
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
      for (ConstGlobalStrandOrderIterator gs_it = m_GlobalStrandOrder.begin(); gs_it!=m_GlobalStrandOrder.end(); gs_it++)
      {
         const GlobalStrand& strand = *gs_it;
         psStrandType type       = strand.m_StrandType;
         long localIndex = strand.m_LocalSortOrder;

         if (type == stStraight)
         {
            const StraightStrandLocation& strandLocation = m_StraightStrands[localIndex];

            ns += strandLocation.m_Xstart > 0.0 ? 2 : 1;
         }
         else if (type == stHarped)
         {
            const HarpedStrandLocation& strandLocation = m_HarpedStrands[localIndex];
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
            ATLASSERT(0);

         np = nh + ns;

         if (totalNumStrands <= np)
            break;
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
      for (ConstStraightStrandIterator iter=m_StraightStrands.begin(); iter!=m_StraightStrands.end(); iter++)
      {
         const StraightStrandLocation& strandLocation = *iter;

         nStrands++;

         if (0.0 < strandLocation.m_Xstart)
            nStrands++;

         if (ns <= nStrands)
            break;
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
         for (ConstHarpedStrandIterator iter=m_HarpedStrands.begin(); iter!=m_HarpedStrands.end(); iter++)
         {
            const HarpedStrandLocation& strandLocation = *iter;

            nHarped++;

            if (0.0 < strandLocation.m_Xhp || (m_bUseDifferentHarpedGridAtEnds && 0.0 < strandLocation.m_Xend))
            {
               nHarped++;
            }

            if (nh <= nHarped)
               break;
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
      for (ConstStraightStrandIterator iter = m_TemporaryStrands.begin(); iter != m_TemporaryStrands.end(); iter++)
      {
         const StraightStrandLocation& strandLocation = *iter;

         nStrands++;

         if (0.0 < strandLocation.m_Xstart)
            nStrands++;

         if (nt <= nStrands)
            break;
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


void GirderLibraryEntry::SetEndAdjustmentLimits(GirderFace  topFace, Float64  topLimit, GirderFace  bottomFace, Float64  bottomLimit)
{
   m_EndAdjustment.m_TopFace     = topFace;
   m_EndAdjustment.m_TopLimit    = topLimit;
   m_EndAdjustment.m_BottomFace  = bottomFace;
   m_EndAdjustment.m_BottomLimit = bottomLimit;
}

void GirderLibraryEntry::GetEndAdjustmentLimits(GirderFace* topFace, Float64* topLimit, GirderFace* bottomFace, Float64* bottomLimit) const
{
   *topFace = m_EndAdjustment.m_TopFace;
   *topLimit = m_EndAdjustment.m_TopLimit;
   *bottomFace = m_EndAdjustment.m_BottomFace;
   *bottomLimit = m_EndAdjustment.m_BottomLimit;
}

void GirderLibraryEntry::SetHPAdjustmentLimits(GirderFace  topFace, Float64  topLimit, GirderFace  bottomFace, Float64  bottomLimit)
{
   m_HPAdjustment.m_TopFace     = topFace;
   m_HPAdjustment.m_TopLimit    = topLimit;
   m_HPAdjustment.m_BottomFace  = bottomFace;
   m_HPAdjustment.m_BottomLimit = bottomLimit;
}

void GirderLibraryEntry::GetHPAdjustmentLimits(GirderFace* topFace, Float64* topLimit, GirderFace* bottomFace, Float64* bottomLimit) const
{
   *topFace = m_HPAdjustment.m_TopFace;
   *topLimit = m_HPAdjustment.m_TopLimit;
   *bottomFace = m_HPAdjustment.m_BottomFace;
   *bottomLimit = m_HPAdjustment.m_BottomLimit;
}

void GirderLibraryEntry::SetShearSteelMaterial(const std::string& name)
{
   m_ShearSteelMaterial = name;
}

std::string GirderLibraryEntry::GetShearSteelMaterial() const
{
   return m_ShearSteelMaterial;
}

void GirderLibraryEntry::SetShearZoneInfo(const GirderLibraryEntry::ShearZoneInfoVec& vec)
{
   m_ShearZoneInfo = vec;
}

GirderLibraryEntry::ShearZoneInfoVec GirderLibraryEntry::GetShearZoneInfo() const
{
   return m_ShearZoneInfo;
}

void GirderLibraryEntry::SetShearSteelBarSize(BarSizeType size)
{
   m_ShearSteelBarSize = size;
}

BarSizeType GirderLibraryEntry::GetShearSteelBarSize() const
{
   return m_ShearSteelBarSize;
}

void GirderLibraryEntry::SetLastConfinementZone(Uint16 zone)
{
   m_LastConfinementZone = zone;
}

Uint16 GirderLibraryEntry::GetNumConfinementZones() const
{
   return m_LastConfinementZone;
}
   
void GirderLibraryEntry::DoStirrupsEngageDeck(bool bEngage)
{
   m_bStirrupsEngageDeck = bEngage;
}

bool GirderLibraryEntry::DoStirrupsEngageDeck() const
{
   return m_bStirrupsEngageDeck;
}

void GirderLibraryEntry::SetTopFlangeShearBarSize(BarSizeType size)
{
   CHECK(size>=0);
   m_TopFlangeShearBarSize = size;
}

BarSizeType GirderLibraryEntry::GetTopFlangeShearBarSize() const
{
   return m_TopFlangeShearBarSize;
}

void GirderLibraryEntry::SetTopFlangeShearBarSpacing(Float64 Spacing)
{
   CHECK(Spacing>=0.0);
   m_TopFlangeShearBarSpacing = Spacing;
}

Float64 GirderLibraryEntry::GetTopFlangeShearBarSpacing() const
{
   return m_TopFlangeShearBarSpacing;
}

void GirderLibraryEntry::SetLongSteelInfo(const GirderLibraryEntry::LongSteelInfoVec& vec)
{
   m_LongSteelInfo = vec;
}

GirderLibraryEntry::LongSteelInfoVec GirderLibraryEntry::GetLongSteelInfo() const
{
   return m_LongSteelInfo;
}

void GirderLibraryEntry::SetLongSteelMaterial(const std::string& name)
{
   m_LongSteelMaterial = name;
}

std::string GirderLibraryEntry::GetLongSteelMaterial() const
{
   return m_LongSteelMaterial;
}

void GirderLibraryEntry::SetHarpingPointLocation(Float64 d)
{
   m_HarpingPointLocation = d;
}

Float64 GirderLibraryEntry::GetHarpingPointLocation() const
{
   return m_HarpingPointLocation;
}

void GirderLibraryEntry::SetMinHarpingPointLocation(bool bUseMin,double min)
{
   m_bMinHarpingPointLocation = bUseMin;
   m_MinHarpingPointLocation = min;
}

bool GirderLibraryEntry::IsMinHarpingPointLocationUsed() const
{
   return m_bMinHarpingPointLocation;
}

double GirderLibraryEntry::GetMinHarpingPointLocation() const
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
bool GirderLibraryEntry::Edit(bool allowEditing)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   if ( m_pBeamFactory == NULL )
   {
      // For some reason we were unable to create a beam factory.
      // This could be because IE was not installed correctly so the
      // component category manager is missing, or there weren't any
      // beam factories defined
      AfxMessageBox("Girder Sections not defined. This library entry cannot be edited.\nCheck your PGSuper installation",MB_OK | MB_ICONWARNING);
   }

   // exchange data with dialog
   // make a temporary copy of this and have the dialog work on it.
   GirderLibraryEntry tmp(*this);

   CGirderMainSheet dlg(tmp, IDS_GIRDER_SHEET, allowEditing);
   int i = dlg.DoModal();
   if (i==IDOK)
   {
      *this = tmp;
      return true;
   }

   return false;
}

void GirderLibraryEntry::MakeCopy(const GirderLibraryEntry& rOther)
{

   m_ShearSteelBarSize        = rOther.m_ShearSteelBarSize;
   m_LastConfinementZone      = rOther.m_LastConfinementZone;
   m_HarpingPointLocation     = rOther.m_HarpingPointLocation;
   m_bMinHarpingPointLocation = rOther.m_bMinHarpingPointLocation;
   m_MinHarpingPointLocation  = rOther.m_MinHarpingPointLocation;
   m_HarpPointReference       = rOther.m_HarpPointReference;
   m_HarpPointMeasure         = rOther.m_HarpPointMeasure;
   m_ShearSteelMaterial       = rOther.m_ShearSteelMaterial;
   m_LongSteelMaterial        = rOther.m_LongSteelMaterial;

   m_HPAdjustment             = rOther.m_HPAdjustment;
   m_EndAdjustment            = rOther.m_EndAdjustment;

   m_TopFlangeShearBarSize    = rOther.m_TopFlangeShearBarSize;
   m_TopFlangeShearBarSpacing = rOther.m_TopFlangeShearBarSpacing;

   m_MaxDebondStrands                       = rOther.m_MaxDebondStrands;
   m_MaxDebondStrandsPerRow                 = rOther.m_MaxDebondStrandsPerRow;
   m_MaxNumDebondedStrandsPerSection        = rOther.m_MaxNumDebondedStrandsPerSection;
   m_MaxDebondedStrandsPerSection           = rOther.m_MaxDebondedStrandsPerSection;
   m_MinDebondLength                        = rOther.m_MinDebondLength;
   m_DefaultDebondLength                    = rOther.m_DefaultDebondLength;
   m_MaxDebondLengthBySpanFraction          = rOther.m_MaxDebondLengthBySpanFraction;
   m_MaxDebondLengthByHardDistance          = rOther.m_MaxDebondLengthByHardDistance;

   // deep copy of strand locations
   // use a factory since we have lots of points to create
   CComPtr<IGeomUtil> geom_util;
   geom_util.CoCreateInstance(CLSID_GeomUtil);
   CComPtr<IPoint2dFactory> factory;
   geom_util->get_Point2dFactory(&factory);

   m_StraightStrands = rOther.m_StraightStrands;
   m_TemporaryStrands = rOther.m_TemporaryStrands;

   m_HarpedStrands               = rOther.m_HarpedStrands;
   m_GlobalStrandOrder           = rOther.m_GlobalStrandOrder;

   m_ShearZoneInfo = rOther.m_ShearZoneInfo;
   m_LongSteelInfo = rOther.m_LongSteelInfo;

   m_Dimensions = rOther.m_Dimensions;

   m_pBeamFactory.Release();
   m_pBeamFactory = rOther.m_pBeamFactory;

   m_bOddNumberOfHarpedStrands = rOther.m_bOddNumberOfHarpedStrands;
   m_bUseDifferentHarpedGridAtEnds = rOther.m_bUseDifferentHarpedGridAtEnds;

   m_bStirrupsEngageDeck = rOther.m_bStirrupsEngageDeck;

   m_DiaphragmLayoutRules = rOther.m_DiaphragmLayoutRules;

   m_bCanPostTension = rOther.m_bCanPostTension;
}

void GirderLibraryEntry::MakeAssignment(const GirderLibraryEntry& rOther)
{
   libLibraryEntry::MakeAssignment( rOther );
   MakeCopy( rOther );
}

HICON  GirderLibraryEntry::GetIcon() const
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   if (m_pBeamFactory)
      return m_pBeamFactory->GetIcon();
   else
      return ::LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_GIRDER_ENTRY) );
}

void GirderLibraryEntry::ConfigureStraightStrandGrid(IStrandGrid* pStartGrid,IStrandGrid* pEndGrid) const
{
   // need to break local data structures into IPoints
   CComPtr<IPoint2dCollection> startPoints, endPoints;
   startPoints.CoCreateInstance(CLSID_Point2dCollection);
   endPoints.CoCreateInstance(CLSID_Point2dCollection);

   for (ConstStraightStrandIterator iter = m_StraightStrands.begin(); iter != m_StraightStrands.end(); iter++)
   {
      const StraightStrandLocation& strandLocation = *iter;

      CComPtr<IPoint2d> startPoint, endPoint;
      startPoint.CoCreateInstance(CLSID_Point2d);
      endPoint.CoCreateInstance(CLSID_Point2d);

      startPoint->Move(strandLocation.m_Xstart,strandLocation.m_Ystart);
      startPoints->Add(startPoint);

      endPoint->Move(strandLocation.m_Xend,strandLocation.m_Yend);
      endPoints->Add(endPoint);
   }

   pStartGrid->ClearGridPoints();
   pStartGrid->AddGridPoints(startPoints);

   pEndGrid->ClearGridPoints();
   pEndGrid->AddGridPoints(endPoints);
}

void GirderLibraryEntry::ConfigureHarpedStrandGrids(IStrandGrid* pEndGridAtStart, IStrandGrid* pHPGridAtStart, IStrandGrid* pHPGridAtEnd, IStrandGrid* pEndGridAtEnd) const
{
   // need to break local data structures into IPoints
   CComPtr<IPoint2dCollection> start_pts, end_pts, hp_pts;
   end_pts.CoCreateInstance(CLSID_Point2dCollection);
   start_pts.CoCreateInstance(CLSID_Point2dCollection);
   hp_pts.CoCreateInstance(CLSID_Point2dCollection);

   for (ConstHarpedStrandIterator iter = m_HarpedStrands.begin(); iter != m_HarpedStrands.end(); iter++)
   {
      const HarpedStrandLocation& strandLocation = *iter;

      CComPtr<IPoint2d> start_point, hp_point, end_point;
      start_point.CoCreateInstance(CLSID_Point2d);
      hp_point.CoCreateInstance(CLSID_Point2d);
      end_point.CoCreateInstance(CLSID_Point2d);

      // assume harped points are used at the ends as well
      start_point->Move(strandLocation.m_Xhp,strandLocation.m_Yhp);
      hp_point->Move(strandLocation.m_Xhp,strandLocation.m_Yhp);
      end_point->Move(strandLocation.m_Xhp,strandLocation.m_Yhp);

      hp_pts->Add(hp_point);

      if (m_bUseDifferentHarpedGridAtEnds)
      {
         // different points are used at the end of the girder
         start_point->Move(strandLocation.m_Xstart,strandLocation.m_Ystart);
         end_point->Move(strandLocation.m_Xend,strandLocation.m_Yend);
      }

      start_pts->Add(start_point);
      end_pts->Add(end_point);
   }

   pEndGridAtStart->ClearGridPoints();
   pEndGridAtStart->AddGridPoints(start_pts);

   pHPGridAtStart->ClearGridPoints();
   pHPGridAtStart->AddGridPoints(hp_pts);

   pHPGridAtEnd->ClearGridPoints();
   pHPGridAtEnd->AddGridPoints(hp_pts);

   pEndGridAtEnd->ClearGridPoints();
   pEndGridAtEnd->AddGridPoints(end_pts);
}

void GirderLibraryEntry::ConfigureTemporaryStrandGrid(IStrandGrid* pStartGrid,IStrandGrid* pEndGrid) const
{
   // need to break local data structures into IPoints
   CComPtr<IPoint2dCollection> startPoints, endPoints;
   startPoints.CoCreateInstance(CLSID_Point2dCollection);
   endPoints.CoCreateInstance(CLSID_Point2dCollection);

   for (ConstStraightStrandIterator iter = m_TemporaryStrands.begin(); iter != m_TemporaryStrands.end(); iter++)
   {
      const StraightStrandLocation& strandLocation = *iter;

      CComPtr<IPoint2d> startPoint, endPoint;
      startPoint.CoCreateInstance(CLSID_Point2d);
      endPoint.CoCreateInstance(CLSID_Point2d);

      startPoint->Move(strandLocation.m_Xstart,strandLocation.m_Ystart);
      startPoints->Add(startPoint);

      endPoint->Move(strandLocation.m_Xend,strandLocation.m_Yend);
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
   double x1,y1;
   p1->get_X(&x1);
   p1->get_Y(&y1);

   double x2,y2;
   p2->get_X(&x2);
   p2->get_Y(&y2);

   return ::IsEqual(x1,x2) && ::IsEqual(y1,y2);
}

bool GirderLibraryEntry::CanDebondStraightStrands() const
{
   for (ConstStraightStrandIterator iter = m_StraightStrands.begin(); iter != m_StraightStrands.end(); iter++)
   {
      const StraightStrandLocation& strandLocation = *iter;

      if (strandLocation.m_bCanDebond)
         return true; // if there is one, we can debond
   }

   return false;
}

Float64 GirderLibraryEntry::GetMaxTotalFractionDebondedStrands() const
{
   return m_MaxDebondStrands;
}

void GirderLibraryEntry::SetMaxTotalFractionDebondedStrands(Float64 fraction) 
{
   ATLASSERT(fraction>=0.0 && fraction<=1.0);
   m_MaxDebondStrands = fraction;
}

Float64 GirderLibraryEntry::GetMaxFractionDebondedStrandsPerRow() const
{
   return m_MaxDebondStrandsPerRow;
}

void GirderLibraryEntry::SetMaxFractionDebondedStrandsPerRow(Float64 fraction)
{
   ATLASSERT(fraction>=0.0 && fraction<=1.0);
   m_MaxDebondStrandsPerRow = fraction;
}

void  GirderLibraryEntry::GetMaxDebondedStrandsPerSection(StrandIndexType* pNumber, Float64* pFraction) const
{
   *pNumber = m_MaxNumDebondedStrandsPerSection;
   *pFraction = m_MaxDebondedStrandsPerSection;
}

void GirderLibraryEntry::SetMaxDebondedStrandsPerSection(StrandIndexType number, Float64 fraction)
{
   ATLASSERT(fraction>=0.0 && fraction<=1.0);

   m_MaxNumDebondedStrandsPerSection = number;
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

Float64 GirderLibraryEntry::GetMinDebondSectionLength() const
{
   return m_MinDebondLength;
}

void GirderLibraryEntry::SetMinDebondSectionLength(Float64 fraction)
{
   ATLASSERT(fraction>=0.0);
   m_MinDebondLength=fraction;
}

void GirderLibraryEntry::SetDefaultDebondSectionLength(Float64 l)
{
   ATLASSERT(l>=0.0);
   m_DefaultDebondLength=l;
}

Float64 GirderLibraryEntry::GetDefaultDebondSectionLength() const
{
   return m_DefaultDebondLength;
}

bool GirderLibraryEntry::IsEqual(IPoint2dCollection* points1,IPoint2dCollection* points2) const
{
   CollectionIndexType c1,c2;
   points1->get_Count(&c1);
   points2->get_Count(&c2);

   if ( c1 != c2 )
      return false;

   CollectionIndexType idx;
   for ( idx = 0; idx < c1; idx++ )
   {
      CComPtr<IPoint2d> p1, p2;
      points1->get_Item(idx,&p1);
      points2->get_Item(idx,&p2);

      if ( !IsEqual(p1,p2) )
         return false;
   }

   return true;
}

std::string GirderLibraryEntry::GetSectionName() const
{
   std::string name;
   if (m_pBeamFactory)
   {
      LPOLESTR pszUserType;
      OleRegGetUserType(m_pBeamFactory->GetCLSID(),USERCLASSTYPE_SHORT,&pszUserType);
      return std::string( CString(pszUserType) );
   }
   else
   {
      ATLASSERT(0);
      return std::string("Unknown Girder Type");
   }
}

bool GirderLibraryEntry::CanPostTension() const
{
   return m_bCanPostTension;
}

//======================== ACCESS     =======================================
//======================== INQUERY    =======================================
