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

#ifndef INCLUDED_STATUSCALLBACKS_H_
#define INCLUDED_STATUSCALLBACKS_H_

#include "PGSuperDoc.h"

class CPointLoadStatusCallback : public iStatusCallback
{
public:
   CPointLoadStatusCallback(CPGSuperDoc* pDoc, long statusLevel);
   virtual long GetSeverity();
   virtual void Execute(pgsStatusItem* pStatusItem);

private:
   CPGSuperDoc* m_pDoc;
   long         m_StatusLevel;
};

///////////////////////////
class CDistributedLoadStatusCallback : public iStatusCallback
{
public:
   CDistributedLoadStatusCallback(CPGSuperDoc* pDoc, long statusLevel);
   virtual long GetSeverity();
   virtual void Execute(pgsStatusItem* pStatusItem);

private:
   CPGSuperDoc* m_pDoc;
   long         m_StatusLevel;
};

///////////////////////////
class CMomentLoadStatusCallback : public iStatusCallback
{
public:
   CMomentLoadStatusCallback(CPGSuperDoc* pDoc, long statusLevel);
   virtual long GetSeverity();
   virtual void Execute(pgsStatusItem* pStatusItem);

private:
   CPGSuperDoc* m_pDoc;
   long         m_StatusLevel;
};

///////////////////////////
class CConcreteStrengthStatusCallback : public iStatusCallback
{
public:
   CConcreteStrengthStatusCallback(CPGSuperDoc* pDoc,long statusLevel);
   virtual long GetSeverity();
   virtual void Execute(pgsStatusItem* pStatusItem);

private:
   CPGSuperDoc* m_pDoc;
   long         m_StatusLevel;
};

///////////////////////////
class CVSRatioStatusCallback : public iStatusCallback
{
public:
   CVSRatioStatusCallback(CPGSuperDoc* pDoc);
   virtual long GetSeverity();
   virtual void Execute(pgsStatusItem* pStatusItem);

private:
   CPGSuperDoc* m_pDoc;
};

///////////////////////////
class CLiftingSupportLocationStatusCallback : public iStatusCallback
{
public:
   CLiftingSupportLocationStatusCallback(CPGSuperDoc* pDoc);
   virtual long GetSeverity();
   virtual void Execute(pgsStatusItem* pStatusItem);

private:
   CPGSuperDoc* m_pDoc;
};

///////////////////////////
class CTruckStiffnessStatusCallback : public iStatusCallback
{
public:
   CTruckStiffnessStatusCallback(CPGSuperDoc* pDoc);
   virtual long GetSeverity();
   virtual void Execute(pgsStatusItem* pStatusItem);

private:
   CPGSuperDoc* m_pDoc;
};

///////////////////////////
class CBridgeDescriptionStatusCallback : public iStatusCallback
{
public:
   CBridgeDescriptionStatusCallback(CPGSuperDoc* pDoc,long severity);
   virtual long GetSeverity();
   virtual void Execute(pgsStatusItem* pStatusItem);

private:
   CPGSuperDoc* m_pDoc;
   long m_Severity;
};

///////////////////////////
class CRefinedAnalysisStatusCallback : public iStatusCallback
{
public:
   CRefinedAnalysisStatusCallback(CPGSuperDoc* pDoc);
   virtual long GetSeverity();
   virtual void Execute(pgsStatusItem* pStatusItem);

private:
   CPGSuperDoc* m_pDoc;
};

///////////////////////////
class CInstallationErrorStatusCallback : public iStatusCallback
{
public:
   CInstallationErrorStatusCallback(CPGSuperDoc* pDoc);
   virtual long GetSeverity();
   virtual void Execute(pgsStatusItem* pStatusItem);

private:
   CPGSuperDoc* m_pDoc;
};

///////////////////////////
class CUnknownErrorStatusCallback : public iStatusCallback
{
public:
   CUnknownErrorStatusCallback(CPGSuperDoc* pDoc);
   virtual long GetSeverity();
   virtual void Execute(pgsStatusItem* pStatusItem);

private:
   CPGSuperDoc* m_pDoc;
};

///////////////////////////
class CInformationalStatusCallback : public iStatusCallback
{
public:
   CInformationalStatusCallback(CPGSuperDoc* pDoc,long severity,UINT helpID=0);
   virtual long GetSeverity();
   virtual void Execute(pgsStatusItem* pStatusItem);

private:
   CPGSuperDoc* m_pDoc;
   long m_Severity;
   UINT m_HelpID;
};

///////////////////////////
class CGirderDescriptionStatusCallback : public iStatusCallback
{
public:
   CGirderDescriptionStatusCallback(CPGSuperDoc* pDoc,long severity);
   virtual long GetSeverity();
   virtual void Execute(pgsStatusItem* pStatusItem);

private:
   CPGSuperDoc* m_pDoc;
   long m_Severity;
};

///////////////////////////
class CAlignmentDescriptionStatusCallback : public iStatusCallback
{
public:
   CAlignmentDescriptionStatusCallback(CPGSuperDoc* pDoc,long severity);
   virtual long GetSeverity();
   virtual void Execute(pgsStatusItem* pStatusItem);

private:
   CPGSuperDoc* m_pDoc;
   long m_Severity;
};

///////////////////////////
class CLiveLoadStatusCallback : public iStatusCallback
{
public:
   CLiveLoadStatusCallback(CPGSuperDoc* pDoc);
   virtual long GetSeverity();
   virtual void Execute(pgsStatusItem* pStatusItem);

private:
   CPGSuperDoc* m_pDoc;
};

///////////////////////////
class CStructuralAnalysisTypeStatusCallback : public iStatusCallback
{
public:
   CStructuralAnalysisTypeStatusCallback(CPGSuperDoc* pDoc);
   virtual long GetSeverity();
   virtual void Execute(pgsStatusItem* pStatusItem);

private:
   CPGSuperDoc* m_pDoc;
};

#endif // INCLUDED_STATUSCALLBACKS_H_