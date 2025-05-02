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

#pragma once

#include <EAF\EAFViewController.h>
#include <PsgLib\PointOfInterest.h>

// {4E1200E1-41A1-47D5-9A58-9837CC9D20AD}
DEFINE_GUID(IID_IDeflectionHistoryGraphViewController,
   0x4e1200e1, 0x41a1, 0x47d5, 0x9a, 0x58, 0x98, 0x37, 0xcc, 0x9d, 0x20, 0xad);
struct __declspec(uuid("{4E1200E1-41A1-47D5-9A58-9837CC9D20AD}")) IDeflectionHistoryGraphViewController;

interface IDeflectionHistoryGraphViewController : IEAFViewController
{
   enum XAxisType { TimeLinear = 0, TimeLog = 1, Interval = 4 }; // constants match those in the LocationGraphController.h file

   virtual void SelectLocation(const pgsPointOfInterest& poi) = 0;
   virtual const pgsPointOfInterest& GetLocation() const = 0;

   virtual void SetXAxisType(XAxisType type) = 0;
   virtual XAxisType GetXAxisType() const = 0;

   virtual void IncludeElevationAdjustment(bool bAdjust) = 0;
   virtual bool IncludeElevationAdjustment() const = 0;

   virtual void IncludeUnrecoverableDefl(bool bInclude) = 0;
   virtual bool IncludeUnrecoverableDefl() const = 0;

   virtual void ShowGrid(bool bShow) = 0;
   virtual bool ShowGrid() const = 0;
};
