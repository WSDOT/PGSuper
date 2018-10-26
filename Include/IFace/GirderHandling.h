#ifndef INCLUDED_IFACE_GIRDERHANDLING_H_
#define INCLUDED_IFACE_GIRDERHANDLING_H_

/*****************************************************************************
COPYRIGHT
   Copyright © 1997-1998
   Washington State Department Of Transportation
   All Rights Reserved
*****************************************************************************/

// SYSTEM INCLUDES
//
#if !defined INCLUDED_WBFLTYPES_H_
#include <WbflTypes.h>
#endif

#if !defined INCLUDED_PGSUPERTYPES_H_
#include <PGSuperTypes.h>
#endif

#if !defined INCLUDED_DETAILS_H_
#include <Details.h>
#endif

// PROJECT INCLUDES
//

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//

// MISCELLANEOUS
//

/*****************************************************************************
INTERFACE
   IGirderLifting

   Interface to product-level girder handling information

DESCRIPTION
   Interface to girder handling information
*****************************************************************************/
// {E53A3DB2-DD61-11d2-AD34-00105A9AF985}
DEFINE_GUID(IID_IGirderLifting, 
0xe53a3db2, 0xdd61, 0x11d2, 0xad, 0x34, 0x0, 0x10, 0x5a, 0x9a, 0xf9, 0x85);
interface IGirderLifting : IUnknown
{
   // location of lifting loop measured from end of girder
   virtual Float64 GetLeftLiftingLoopLocation(SpanIndexType span,GirderIndexType girder)=0;
   virtual Float64 GetRightLiftingLoopLocation(SpanIndexType span,GirderIndexType girder)=0;
   virtual bool SetLiftingLoopLocations(SpanIndexType span,GirderIndexType girder, Float64 left,Float64 right)=0;
};


/*****************************************************************************
INTERFACE
   IGirderHauling

   Interface to product-level girder handling information

DESCRIPTION
   Interface to girder handling information
*****************************************************************************/
// {1D543E66-DD7E-11d2-AD34-00105A9AF985}
DEFINE_GUID(IID_IGirderHauling, 
0x1d543e66, 0xdd7e, 0x11d2, 0xad, 0x34, 0x0, 0x10, 0x5a, 0x9a, 0xf9, 0x85);
interface IGirderHauling : IUnknown
{
   // location of truck support location measured from end of girder
   virtual Float64 GetLeadingOverhang(SpanIndexType span, GirderIndexType girder)=0;
   virtual Float64 GetTrailingOverhang(SpanIndexType span, GirderIndexType girder)=0;
   virtual bool SetTruckSupportLocations(SpanIndexType span, GirderIndexType girder, Float64 leading,Float64 trailing)=0;
};

#endif // INCLUDED_IFACE_GIRDERHANDLING_H_

