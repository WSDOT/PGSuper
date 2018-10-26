///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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

#ifndef INCLUDED_PGSUPEREXCEPTION_H_
#define INCLUDED_PGSUPEREXCEPTION_H_

#include <MfcTools\Exceptions.h>

// Failed to load a library
#define XREASON_LIBFAILURE                   0x0001

// An agent could not validate its data
#define XREASON_AGENTVALIDATIONFAILURE       0x0002

// An assumption in the LRFD specification was violated
#define XREASON_ASSUMPTIONVIOLATED           0x0004

// The bridge cannot be analyzed using the approximate methods
// described in the LRFD specification
#define XREASON_REFINEDANALYSISREQUIRED      0x0008

// The bridge must be entirely within the limits of the horizontal curve
#define XREASON_NOTINHCURVE                  0x0010

// The bridge must be entirely within the limits of the vertical curve
#define XREASON_NOTINVCURVE                  0x0020

// Cant have negative length girders
#define XREASON_NEGATIVE_GIRDER_LENGTH       0x0040

// Error creating COM component
#define XREASON_COMCREATE_ERROR              0x0080

// The calculation, analysis method, or other requires a different version of the LRFD than is currently selected
#define XREASON_LRFD_VERSION                 0x0100

#endif // INCLUDED_PGSUPEREXCEPTION_H_