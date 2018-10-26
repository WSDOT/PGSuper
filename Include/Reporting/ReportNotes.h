///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright (C) 1999  Washington State Department of Transportation
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

// ReportNotes.h : Define common notes in reports

#pragma once

#define RPT_GDR_END_LOCATION      "Location from" << rptNewLine << "End of Girder"
#define RPT_LFT_SUPPORT_LOCATION  "Location from" << rptNewLine << "Left Support"

#define LIVELOAD_PER_LANE             "* Live Load values are per lane and include impact."
#define LIVELOAD_PER_GIRDER           "* Live Load values are per girder and include impact."
#define LIVELOAD_PER_GIRDER_NO_IMPACT "* Live Load values are per girder and do not include impact."

#define STRUT_AND_TIE_REQUIRED "* [LRFD 5.8.3.2] The shear stress at the critical section exceeds 0.18f'c and the beam-type element is not built integrally with the support. The end region shall be designed using the strut-and-tie model specified in LRFD 5.6.3"
#define SUPPORT_COMPRESSION    "$ [LRFD 5.8.3.2] The reaction introduces compression into the end of the girder. Sectional design is not used at this location." << rptNewLine << "[LRFD C5.8.3.2] Loads close to the support are transferred directly to the support by compressive arching action without causing additional stresses in the stirrups. Av/S at this section has been compared to Av/S at the critical section."
