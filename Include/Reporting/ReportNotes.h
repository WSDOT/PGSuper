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

// ReportNotes.h : Define common notes in reports

#pragma once


#define RPT_GDR_END_LOCATION      _T("Location from") << rptNewLine << _T("End of Girder")
#define RPT_LFT_SUPPORT_LOCATION  _T("Location from") << rptNewLine << _T("Left Support")

#define LIVELOAD_PER_LANE             _T("* Live Load values are per lane and include impact.")
#define LIVELOAD_PER_LANE_NO_IMPACT   _T("* Live Load values are per lane and do not include impact.")
#define LIVELOAD_PER_GIRDER           _T("* Live Load values are per girder and include impact.")
#define LIVELOAD_PER_GIRDER_NO_IMPACT _T("* Live Load values are per girder and do not include impact.")

#define STRUT_AND_TIE_REQUIRED _T("* [LRFD ") << LrfdCw8th(_T("5.7.3.2"),_T("5.8.3.2")) << _T("] The shear stress ")<< RPT_vu << _T(" at the critical section exceeds 0.18") << RPT_FC << _T(" and the beam-type element is not built integrally with the support. The end region shall be designed using the strut-and-tie model specified in LRFD ") << LrfdCw8th(_T("5.6.3"),_T("5.8.2"))
#define SUPPORT_COMPRESSION    _T("[LRFD ") << LrfdCw8th(_T("5.7.3.2"),_T("5.8.3.2")) << _T("] The reaction introduces compression into the end of the girder. Load between the CSS and the support is transferred directly to the support by compressive arching action without causing additional stresses in the stirrups. Hence, ") << Sub2(_T("A"),_T("v")) << _T("/S in this region must be equal or greater than ") << Sub2(_T("A"),_T("v")) << _T("/S at the critical section.")

#define TIME_DEPENDENT_NOTE _T("CR, SH, and RE are secondary forces caused by the restraining action of the bridge frame.")
#define TIME_DEPENDENT_STRESS_NOTE _T("Time-dependent creep, shrinkage, and relaxation stresses are computed from internal forces on the concrete component using net section properties. Refer to the Time-Step Details Report for computation details.")

#define EPOXY_RELAXATION_NOTE _T("Strands are epoxy coated. Epoxy-coated strand manufacturers recommend doubling the relaxation loss of uncoated strands") << rptNewLine << _T("See \"Guidelines for the Use of Epoxy-Coated Strands\", PCI Journal, July-August 1993, Section 5.2.")

