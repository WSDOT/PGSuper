///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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

#pragma once

#include <EAF\EAFViewController.h>
#include <PgsExt\PointOfInterest.h>

// {D6566A01-9DF9-4136-8008-18D246C0BE8C}
DEFINE_GUID(IID_IDstressHistoryGraphViewController,
   0xd6566a01, 0x9df9, 0x4136, 0x80, 0x8, 0x18, 0xd2, 0x46, 0xc0, 0xbe, 0x8c);
struct __declspec(uuid("{D6566A01-9DF9-4136-8008-18D246C0BE8C}")) IStressHistoryGraphViewController;

interface IStressHistoryGraphViewController : IEAFViewController
{
   enum XAxisType { TimeLinear = 0, TimeLog = 1, Interval = 4 }; // constants match those in the LocationGraphController.h file

   virtual void SelectLocation(const pgsPointOfInterest& poi) = 0;
   virtual const pgsPointOfInterest& GetLocation() const = 0;

   virtual void SetXAxisType(XAxisType type) = 0;
   virtual XAxisType GetXAxisType() const = 0;

   virtual void Stresses(pgsTypes::StressLocation stressLocation,bool bEnable) = 0;
   virtual bool Stresses(pgsTypes::StressLocation stressLocation) const = 0;

   virtual void ShowGrid(bool bShow) = 0;
   virtual bool ShowGrid() const = 0;
};
#pragma once
