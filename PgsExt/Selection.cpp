///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2024  Washington State Department of Transportation
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

#include <PgsExt\PgsExtLib.h>
#include <PgsExt\Selection.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CSelection::CSelection() :
Type(None),
SpanIdx(INVALID_INDEX),
PierIdx(INVALID_INDEX),
GroupIdx(INVALID_INDEX),
GirderIdx(INVALID_INDEX),
SegmentIdx(INVALID_INDEX),
tsID(INVALID_ID)
{
}

CSelection::CSelection(const CSelection& other) :
Type(other.Type),
SpanIdx(other.SpanIdx),
PierIdx(other.PierIdx),
GroupIdx(other.GroupIdx),
GirderIdx(other.GirderIdx),
SegmentIdx(other.SegmentIdx),
tsID(other.tsID)
{
}

CSelection::~CSelection()
{
}

CSelection& CSelection::operator=(const CSelection& other)
{
   Type       = other.Type;
   SpanIdx    = other.SpanIdx;
   PierIdx    = other.PierIdx;
   GroupIdx   = other.GroupIdx;
   GirderIdx  = other.GirderIdx;
   SegmentIdx = other.SegmentIdx;
   tsID       = other.tsID;
   return *this;
}

bool CSelection::operator==(const CSelection& other) const
{
   if ( Type != other.Type )
   {
      return false;
   }

   if ( SpanIdx != other.SpanIdx )
   {
      return false;
   }

   if ( PierIdx != other.PierIdx )
   {
      return false;
   }

   if ( GroupIdx != other.GroupIdx )
   {
      return false;
   }

   if ( GirderIdx != other.GirderIdx )
   {
      return false;
   }

   if (SegmentIdx != other.SegmentIdx )
   {
      return false;
   }

   if ( tsID != other.tsID )
   {
      return false;
   }

   return true;
}

bool CSelection::operator!=(const CSelection& other) const
{
   return !operator==(other);
}
