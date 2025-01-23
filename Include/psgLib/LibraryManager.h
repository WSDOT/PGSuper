///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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

#ifndef INCLUDED_PSGLIB_LIBRARYMANAGER_H_
#define INCLUDED_PSGLIB_LIBRARYMANAGER_H_

// SYSTEM INCLUDES
//

// PROJECT INCLUDES
//
#include <psgLib\psgLibLib.h>
#include <LibraryFw\Library.h>
#include <LibraryFw\LibraryManager.h>
#include <psgLib\ConcreteLibraryEntry.h>
#include <psgLib\ConnectionLibraryEntry.h>
#include <psgLib\GirderLibraryEntry.h>
#include <psgLib\DiaphragmLayoutEntry.h>
#include <psgLib\TrafficBarrierEntry.h>
#include <psgLib\SpecLibraryEntry.h>
#include <psgLib\RatingLibraryEntry.h>
#include <psgLib\LiveLoadLibraryEntry.h>
#include <psgLib\DuctLibraryEntry.h>
#include <psgLib\HaulTruckLibraryEntry.h>

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//

// MISCELLANEOUS
//
// the available library types

#define DECLARE_LIBRARY(name,entry_type,min_count) \
   PSGLIBTPL WBFL::Library::Library<entry_type,min_count>; \
   using name = WBFL::Library::Library<entry_type,min_count>;

DECLARE_LIBRARY( ConcreteLibrary,        ConcreteLibraryEntry,   0 )
DECLARE_LIBRARY( ConnectionLibrary,      ConnectionLibraryEntry, 1 )
DECLARE_LIBRARY( GirderLibraryBase,      GirderLibraryEntry,     1 )
DECLARE_LIBRARY( DiaphragmLayoutLibrary, DiaphragmLayoutEntry,   0 )
DECLARE_LIBRARY( TrafficBarrierLibrary,  TrafficBarrierEntry,    1 )
DECLARE_LIBRARY( SpecLibrary,            SpecLibraryEntry,       1 )
DECLARE_LIBRARY( RatingLibrary,          RatingLibraryEntry,     1 )
DECLARE_LIBRARY( LiveLoadLibrary,        LiveLoadLibraryEntry,   0 )
DECLARE_LIBRARY( DuctLibrary,            DuctLibraryEntry,       1 )
DECLARE_LIBRARY( HaulTruckLibrary,       HaulTruckLibraryEntry,  1 )

class GirderLibrary : public GirderLibraryBase
{
public:
   GirderLibrary(LPCTSTR idName, LPCTSTR displayName, bool bIsDepreciated = false);
   virtual bool NewEntry(LPCTSTR key);
};


/*****************************************************************************
CLASS 
   psgLibraryManager

   PSGuper specific library manager


DESCRIPTION
   Loads PGSuper libraries

LOG
   rdp : 08.18.1998 : Created file
*****************************************************************************/

class PSGLIBCLASS psgLibraryManager : public WBFL::Library::LibraryManager
{
public:
   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   psgLibraryManager();

   //------------------------------------------------------------------------
   // Destructor
   virtual ~psgLibraryManager();

   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   // GROUP: ACCESS

   //------------------------------------------------------------------------
   // access to the different types of libraries in the manager
   ConcreteLibrary&        GetConcreteLibrary();
   const ConcreteLibrary&        GetConcreteLibrary() const;
   ConnectionLibrary&      GetConnectionLibrary();
   const ConnectionLibrary&      GetConnectionLibrary() const;
   GirderLibrary&          GetGirderLibrary();
   const GirderLibrary&          GetGirderLibrary() const;
   DiaphragmLayoutLibrary& GetDiaphragmLayoutLibrary();
   const DiaphragmLayoutLibrary& GetDiaphragmLayoutLibrary() const;
   TrafficBarrierLibrary& GetTrafficBarrierLibrary();
   const TrafficBarrierLibrary& GetTrafficBarrierLibrary() const;
   SpecLibrary* GetSpecLibrary();
   const SpecLibrary* GetSpecLibrary() const;
   RatingLibrary* GetRatingLibrary();
   const RatingLibrary* GetRatingLibrary() const;
   LiveLoadLibrary* GetLiveLoadLibrary();
   const LiveLoadLibrary* GetLiveLoadLibrary() const;
   DuctLibrary* GetDuctLibrary();
   const DuctLibrary* GetDuctLibrary() const;
   HaulTruckLibrary* GetHaulTruckLibrary();
   const HaulTruckLibrary* GetHaulTruckLibrary() const;

   virtual bool LoadMe(WBFL::System::IStructuredLoad* pLoad);

   void SetMasterLibraryInfo(LPCTSTR strPublisher,LPCTSTR strConfiguration,LPCTSTR strLibFile);
   void GetMasterLibraryInfo(std::_tstring& strServer,std::_tstring& strConfiguration,std::_tstring& strLibFile) const;

   // GROUP: INQUIRY

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   // GROUP: ACCESS
   // GROUP: INQUIRY
   void UpdateLibraryNames();

private:
   // GROUP: DATA MEMBERS
   IndexType m_ConcLibIdx;
   IndexType m_ConnLibIdx;
   IndexType m_GirdLibIdx;
   IndexType m_DiapLibIdx;
   IndexType m_BarrLibIdx;
   IndexType m_SpecLibIdx;
   IndexType m_RatingLibIdx;
   IndexType m_LiveLibIdx;
   IndexType m_DuctLibIdx;
   IndexType m_HaulTruckLibIdx;

   std::_tstring m_strServer;
   std::_tstring m_strConfiguration;
   std::_tstring m_strLibFile;

   // GROUP: LIFECYCLE

   // Prevent accidental copying and assignment
   psgLibraryManager(const psgLibraryManager&) = delete;
   psgLibraryManager& operator=(const psgLibraryManager&) = delete;

   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   // GROUP: ACCESS
   // GROUP: INQUIRY
};

// INLINE METHODS
//

// EXTERNAL REFERENCES
//

#endif // INCLUDED_PSGLIB_LIBRARYMANAGER_H_
