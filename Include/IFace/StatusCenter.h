///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright (C) 2004  Washington State Department of Transportation
//                     Bridge and Structures Office
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

#ifndef INCLUDED_IFACE_STATUSCENTER_H_
#define INCLUDED_IFACE_STATUSCENTER_H_

/*****************************************************************************
COPYRIGHT
   Copyright © 1997-2004
   Washington State Department Of Transportation
   All Rights Reserved
*****************************************************************************/

#include <WbflTypes.h>
#include <PgsExt\StatusItem.h>

/*****************************************************************************
INTERFACE
   IStatusCenter

   Interface for the status center.

DESCRIPTION
   Interface for the status center. The status center maintains current application
   status information.
*****************************************************************************/
// {77977E9B-B074-401f-8994-73A418FC4FFF}
DEFINE_GUID(IID_IStatusCenter, 
0x77977e9b, 0xb074, 0x401f, 0x89, 0x94, 0x73, 0xa4, 0x18, 0xfc, 0x4f, 0xff);
interface IStatusCenter : IUnknown
{
   virtual long GetAgentID() = 0;
   virtual long Add(pgsStatusItem* pItem) = 0;
   virtual bool RemoveByID(long id) = 0;
   virtual bool RemoveByIndex(long index) = 0;
   virtual bool RemoveByAgentID(long agentID) = 0;
   virtual pgsStatusItem* GetByID(long id) = 0;
   virtual pgsStatusItem* GetByIndex(long index) = 0;
   virtual long GetSeverity(const pgsStatusItem* pItem) = 0;
   virtual long Count() = 0;
};

#endif // INCLUDED_IFACE_STATUSCENTER_H_
