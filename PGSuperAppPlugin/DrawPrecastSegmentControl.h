#pragma once

#include <GraphicsLib\PointMapper.h>
#include <PgsExt\PrecastSegmentData.h>
#include <PgsExt\SegmentKey.h>

// CDrawPrecastSegmentControl

interface IPrecastSegmentDataSource
{
public:
   virtual const CSplicedGirderData* GetGirder() const = 0;
   virtual const CSegmentKey& GetSegmentKey() const = 0;
   virtual SegmentIDType GetSegmentID() const = 0;
};

class CDrawPrecastSegmentControl : public CWnd
{
	DECLARE_DYNAMIC(CDrawPrecastSegmentControl)

public:
	CDrawPrecastSegmentControl();
	virtual ~CDrawPrecastSegmentControl();

   void CustomInit(IPrecastSegmentDataSource* pSource);

   afx_msg void OnPaint();

protected:
	DECLARE_MESSAGE_MAP()
   IPrecastSegmentDataSource* m_pSource;

   void CreateSegmentShape(const CSegmentKey& segmentKey,IShape** ppShape,IPoint2dCollection** ppPoints);
   void DrawShape(CDC* pDC,grlibPointMapper& mapper,IShape* pShape);
   void DrawBottomFlange(CDC* pDC,grlibPointMapper& mapper,IPoint2dCollection* pPoints);
public:
   afx_msg BOOL OnEraseBkgnd(CDC* pDC);
};


