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

#if !defined INCLUDED_ISUPPORTLIBRARYMANAGER_H_
#define INCLUDED_ISUPPORTLIBRARYMANAGER_H_

class libLibraryManager;

interface libISupportLibraryManager
{
   // get the number of library managers available
   virtual CollectionIndexType GetNumberOfLibraryManagers() const=0;

   // return a pointer to a in the list library manager
   virtual libLibraryManager* GetLibraryManager(CollectionIndexType num)=0;

   // return a pointer to the ONLY library manager that 
   // is a target for copied items
   virtual libLibraryManager* GetTargetLibraryManager()=0;
};

#endif