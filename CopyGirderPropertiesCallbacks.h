///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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

#include <IFace\ExtendUI.h>
#include <System\Transaction.h>
#include <PsgLib\ShearData.h>
#include <PgsExt\LongitudinalRebarData.h>
#include <PgsExt\PrestressData.h>
#include <PgsExt\HandlingData.h>
#include <PgsExt\GirderMaterial.h>

class txnCopyGirderType :  public txnTransaction
{
public:
   txnCopyGirderType(SpanGirderHashType fromSpanGirderHashValue,const std::vector<SpanGirderHashType>& toSpanGirderHashValues);
   virtual ~txnCopyGirderType();
   virtual bool Execute();
   virtual void Undo();
   virtual txnTransaction* CreateClone() const;
   virtual std::_tstring Name() const;
   virtual bool IsUndoable() { return true; }
   virtual bool IsRepeatable() { return false; }

private:
   SpanGirderHashType m_FromSpanGirderHashValue;
   std::vector<SpanGirderHashType> m_ToSpanGirderHashValues;
   std::vector<std::_tstring> m_strOldNames;
};

class txnCopyGirderStirrups :  public txnTransaction
{
public:
   txnCopyGirderStirrups(SpanGirderHashType fromSpanGirderHashValue,const std::vector<SpanGirderHashType>& toSpanGirderHashValues);
   virtual ~txnCopyGirderStirrups();
   virtual bool Execute();
   virtual void Undo();
   virtual txnTransaction* CreateClone() const;
   virtual std::_tstring Name() const;
   virtual bool IsUndoable() { return true; }
   virtual bool IsRepeatable() { return false; }

private:
   SpanGirderHashType m_FromSpanGirderHashValue;
   std::vector<SpanGirderHashType> m_ToSpanGirderHashValues;
   std::vector<CShearData> m_OldShearData;
};

class txnCopyGirderPrestressing :  public txnTransaction
{
public:
   txnCopyGirderPrestressing(SpanGirderHashType fromSpanGirderHashValue,const std::vector<SpanGirderHashType>& toSpanGirderHashValues);
   virtual ~txnCopyGirderPrestressing();
   virtual bool Execute();
   virtual void Undo();
   virtual txnTransaction* CreateClone() const;
   virtual std::_tstring Name() const;
   virtual bool IsUndoable() { return true; }
   virtual bool IsRepeatable() { return false; }

private:
   SpanGirderHashType m_FromSpanGirderHashValue;
   std::vector<SpanGirderHashType> m_ToSpanGirderHashValues;
   std::vector<CPrestressData> m_OldPrestressData;
};

class txnCopyGirderHandling :  public txnTransaction
{
public:
   txnCopyGirderHandling(SpanGirderHashType fromSpanGirderHashValue,const std::vector<SpanGirderHashType>& toSpanGirderHashValues);
   virtual ~txnCopyGirderHandling();
   virtual bool Execute();
   virtual void Undo();
   virtual txnTransaction* CreateClone() const;
   virtual std::_tstring Name() const;
   virtual bool IsUndoable() { return true; }
   virtual bool IsRepeatable() { return false; }

private:
   SpanGirderHashType m_FromSpanGirderHashValue;
   std::vector<SpanGirderHashType> m_ToSpanGirderHashValues;
   std::vector<CHandlingData> m_OldHandlingData;
};

class txnCopyGirderMaterial :  public txnTransaction
{
public:
   txnCopyGirderMaterial(SpanGirderHashType fromSpanGirderHashValue,const std::vector<SpanGirderHashType>& toSpanGirderHashValues);
   virtual ~txnCopyGirderMaterial();
   virtual bool Execute();
   virtual void Undo();
   virtual txnTransaction* CreateClone() const;
   virtual std::_tstring Name() const;
   virtual bool IsUndoable() { return true; }
   virtual bool IsRepeatable() { return false; }

private:
   SpanGirderHashType m_FromSpanGirderHashValue;
   std::vector<SpanGirderHashType> m_ToSpanGirderHashValues;
   std::vector<CGirderMaterial> m_OldMaterialData;
};

class txnCopyGirderRebar :  public txnTransaction
{
public:
   txnCopyGirderRebar(SpanGirderHashType fromSpanGirderHashValue,const std::vector<SpanGirderHashType>& toSpanGirderHashValues);
   virtual ~txnCopyGirderRebar();
   virtual bool Execute();
   virtual void Undo();
   virtual txnTransaction* CreateClone() const;
   virtual std::_tstring Name() const;
   virtual bool IsUndoable() { return true; }
   virtual bool IsRepeatable() { return false; }

private:
   SpanGirderHashType m_FromSpanGirderHashValue;
   std::vector<SpanGirderHashType> m_ToSpanGirderHashValues;
   std::vector<CLongitudinalRebarData> m_OldRebarData;
};

class txnCopyGirderSlabOffset :  public txnTransaction
{
public:
   txnCopyGirderSlabOffset(SpanGirderHashType fromSpanGirderHashValue,const std::vector<SpanGirderHashType>& toSpanGirderHashValues);
   virtual ~txnCopyGirderSlabOffset();
   virtual bool Execute();
   virtual void Undo();
   virtual txnTransaction* CreateClone() const;
   virtual std::_tstring Name() const;
   virtual bool IsUndoable() { return true; }
   virtual bool IsRepeatable() { return false; }

private:
   SpanGirderHashType m_FromSpanGirderHashValue;
   std::vector<SpanGirderHashType> m_ToSpanGirderHashValues;
   std::vector<Float64> m_OldSlabOffsetData[2];
};

////////////////////////////////////////////////////////////////////////////

class CCopyGirderType : public ICopyGirderPropertiesCallback
{
public:
   CCopyGirderType();
   virtual LPCTSTR GetName();
   virtual BOOL CanCopy(SpanGirderHashType fromSpanGirderHashValue,const std::vector<SpanGirderHashType>& toSpanGirderHashValues);
   virtual txnTransaction* CreateCopyTransaction(SpanGirderHashType fromSpanGirderHashValue,const std::vector<SpanGirderHashType>& toSpanGirderHashValues);
};

class CCopyGirderStirrups : public ICopyGirderPropertiesCallback
{
public:
   CCopyGirderStirrups();
   virtual LPCTSTR GetName();
   virtual BOOL CanCopy(SpanGirderHashType fromSpanGirderHashValue,const std::vector<SpanGirderHashType>& toSpanGirderHashValues);
   virtual txnTransaction* CreateCopyTransaction(SpanGirderHashType fromSpanGirderHashValue,const std::vector<SpanGirderHashType>& toSpanGirderHashValues);
};

class CCopyGirderPrestressing : public ICopyGirderPropertiesCallback
{
public:
   CCopyGirderPrestressing();
   virtual LPCTSTR GetName();
   virtual BOOL CanCopy(SpanGirderHashType fromSpanGirderHashValue,const std::vector<SpanGirderHashType>& toSpanGirderHashValues);
   virtual txnTransaction* CreateCopyTransaction(SpanGirderHashType fromSpanGirderHashValue,const std::vector<SpanGirderHashType>& toSpanGirderHashValues);
};

class CCopyGirderHandling : public ICopyGirderPropertiesCallback
{
public:
   CCopyGirderHandling();
   virtual LPCTSTR GetName();
   virtual BOOL CanCopy(SpanGirderHashType fromSpanGirderHashValue,const std::vector<SpanGirderHashType>& toSpanGirderHashValues);
   virtual txnTransaction* CreateCopyTransaction(SpanGirderHashType fromSpanGirderHashValue,const std::vector<SpanGirderHashType>& toSpanGirderHashValues);
};

class CCopyGirderMaterial : public ICopyGirderPropertiesCallback
{
public:
   CCopyGirderMaterial();
   virtual LPCTSTR GetName();
   virtual BOOL CanCopy(SpanGirderHashType fromSpanGirderHashValue,const std::vector<SpanGirderHashType>& toSpanGirderHashValues);
   virtual txnTransaction* CreateCopyTransaction(SpanGirderHashType fromSpanGirderHashValue,const std::vector<SpanGirderHashType>& toSpanGirderHashValues);
};

class CCopyGirderRebar : public ICopyGirderPropertiesCallback
{
public:
   CCopyGirderRebar();
   virtual LPCTSTR GetName();
   virtual BOOL CanCopy(SpanGirderHashType fromSpanGirderHashValue,const std::vector<SpanGirderHashType>& toSpanGirderHashValues);
   virtual txnTransaction* CreateCopyTransaction(SpanGirderHashType fromSpanGirderHashValue,const std::vector<SpanGirderHashType>& toSpanGirderHashValues);
};

class CCopyGirderSlabOffset : public ICopyGirderPropertiesCallback
{
public:
   CCopyGirderSlabOffset();
   virtual LPCTSTR GetName();
   virtual BOOL CanCopy(SpanGirderHashType fromSpanGirderHashValue,const std::vector<SpanGirderHashType>& toSpanGirderHashValues);
   virtual txnTransaction* CreateCopyTransaction(SpanGirderHashType fromSpanGirderHashValue,const std::vector<SpanGirderHashType>& toSpanGirderHashValues);
};
