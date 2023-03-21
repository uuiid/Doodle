#pragma once

#include "CoreMinimal.h"
#include "Input/CursorReply.h"
#include "Input/Reply.h"
#include "Widgets/SWidget.h"
#include "ITimeSlider.h"

class FSlateWindowElementList;
struct FContextMenuSuppressor;
struct FScrubRangeToScreen;
struct FSlateBrush;

// struct FPaintSectionAreaViewArgs {
//   FPaintSectionAreaViewArgs()
//       : bDisplayTickLines(false), bDisplayScrubPosition(false) {}
//
//   /** Whether to display tick lines */
//   bool bDisplayTickLines;
//   /** Whether to display the scrub position */
//   bool bDisplayScrubPosition;
//   /** Optional Paint args for the playback range*/
//   TOptional<FPaintPlaybackRangeArgs> PlaybackRangeArgs;
// };

/**
 * A time slider controller for the anim timeline
 */
class FCreateCharacterSliderController : public ITimeSliderController {
 public:
  FCreateCharacterSliderController(
      const FTimeSliderArgs& InArgs
  );

  /** 开始 ITimeSliderController 接口 */
  virtual int32 OnPaintTimeSlider(
      bool bMirrorLabels, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
      FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled
  ) const override;
  virtual int32 OnPaintViewArea(
      const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
      int32 LayerId, bool bEnabled, const FPaintViewAreaArgs& Args
  ) const override;
  virtual FReply OnMouseButtonDown(SWidget& WidgetOwner, const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
      override;
  virtual FReply OnMouseButtonUp(SWidget& WidgetOwner, const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
      override;
  virtual FReply OnMouseMove(SWidget& WidgetOwner, const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
      override;
  virtual FReply OnMouseWheel(SWidget& WidgetOwner, const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
      override;
  virtual FCursorReply OnCursorQuery(
      TSharedRef<const SWidget> WidgetOwner, const FGeometry& MyGeometry, const FPointerEvent& CursorEvent
  ) const override;
  virtual void SetViewRange(double NewRangeMin, double NewRangeMax, EViewRangeInterpolation Interpolation) override;
  virtual void SetClampRange(double NewRangeMin, double NewRangeMax) override;
  virtual void SetPlayRange(FFrameNumber RangeStart, int32 RangeDuration) override;
  virtual FFrameRate GetDisplayRate() const override { return TimeSliderArgs.DisplayRate.Get(); }
  virtual FFrameRate GetTickResolution() const override { return TimeSliderArgs.TickResolution.Get(); }
  virtual FAnimatedRange GetViewRange() const override { return TimeSliderArgs.ViewRange.Get(); }
  virtual FAnimatedRange GetClampRange() const override { return TimeSliderArgs.ClampRange.Get(); }
  virtual TRange<FFrameNumber> GetPlayRange() const override {
    return TimeSliderArgs.PlaybackRange.Get(TRange<FFrameNumber>());
  }
  virtual FFrameTime GetScrubPosition() const override { return TimeSliderArgs.ScrubPosition.Get(); }
  virtual void SetScrubPosition(FFrameTime InTime, bool bEvaluate) override {
    TimeSliderArgs.OnScrubPositionChanged.ExecuteIfBound(InTime, false, bEvaluate);
  }
  /** 结束 ITimeSliderController 接口 */

 private:
  struct FDrawTickArgs;
  struct FScrubRangeToScreen;
  FTimeSliderArgs TimeSliderArgs;

  /** 用于在洗地机上绘制填充区域的刷子 */
  const FSlateBrush* ScrubFillBrush;

  /** 用于画一个朝上的擦洗手柄的刷子 */
  const FSlateBrush* ScrubHandleUpBrush;

  /** 用于画一个朝下的刷子的手柄 */
  const FSlateBrush* ScrubHandleDownBrush;

  /** 用于绘制可编辑的时间的画笔 */
  const FSlateBrush* EditableTimeBrush;

  /** 拖动过程中的鼠标总delta值 **/
  float DistanceDragged;

  /** If we are dragging a scrubber or dragging to set the time range */
  enum DragType {
    DRAG_SCRUBBING_TIME,
    DRAG_SETTING_RANGE,
    DRAG_PLAYBACK_START,
    DRAG_PLAYBACK_END,
    DRAG_SELECTION_START,
    DRAG_SELECTION_END,
    DRAG_TIME,
    DRAG_NONE
  };

  DragType MouseDragType;

  /** 如果我们目前正在平移面板 */
  bool bPanning;

  /** 当前拖动时间的索引 */
  int32 DraggedTimeIndex;

  /** 鼠标向下的位置范围 */
  FVector2D MouseDownPosition[2];

  /** 鼠标落下时的几何图形 */
  FGeometry MouseDownGeometry;

  /** 范围叠加 */
  TArray<TRange<double>> ViewRangeStack;

 private:
  /**
   * Clamp the given range to the clamp range
   *
   * @param NewRangeMin		The new lower bound of the range
   * @param NewRangeMax		The new upper bound of the range
   */
  void ClampViewRange(double& NewRangeMin, double& NewRangeMax);

  /**
   * Zoom the range by a given delta.
   *
   * @param InDelta		The total amount to zoom by (+ve = zoom out, -ve = zoom in)
   * @param ZoomBias		Bias to apply to lower/upper extents of the range. (0 = lower, 0.5 = equal, 1 = upper)
   */
  bool ZoomByDelta(float InDelta, float ZoomBias = 0.5f);

  /**
   * Pan the range by a given delta
   *
   * @param InDelta		The total amount to pan by (+ve = pan forwards in time, -ve = pan backwards in time)
   */
  void PanByDelta(float InDelta);

  /** Determine frame time from a mouse position */
  FFrameTime GetFrameTimeFromMouse(const FGeometry& Geometry, FVector2D ScreenSpacePosition) const;

  /**
   * Call this method when the user's interaction has changed the scrub position
   *
   * @param NewValue				Value resulting from the user's interaction
   * @param bIsScrubbing			True if done via scrubbing, false if just releasing scrubbing
   */
  void CommitScrubPosition(FFrameTime NewValue, bool bIsScrubbing);

  /**
   * Draw time tick marks
   *
   * @param OutDrawElements	List to add draw elements to
   * @param ViewRange			The currently visible time range in seconds
   * @param RangeToScreen		Time range to screen space converter
   * @param InArgs			Parameters for drawing the tick lines
   */
  void DrawTicks(
      FSlateWindowElementList& OutDrawElements, const TRange<double>& ViewRange,
      const FScrubRangeToScreen& RangeToScreen, FDrawTickArgs& InArgs
  ) const;

  /**
   * Draw the selection range.
   *
   * @return The new layer ID.
   */
  int32 DrawSelectionRange(
      const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
      int32 LayerId, const FScrubRangeToScreen& RangeToScreen, const FPaintPlaybackRangeArgs& Args
  ) const;

  /**
   * Draw the playback range.
   *
   * @return the new layer ID
   */
  int32 DrawPlaybackRange(
      const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
      int32 LayerId, const FScrubRangeToScreen& RangeToScreen, const FPaintPlaybackRangeArgs& Args
  ) const;

 private:
  /**
   * Hit test the lower bound of a range
   */
  bool HitTestRangeStart(const FScrubRangeToScreen& RangeToScreen, const TRange<double>& Range, float HitPixel) const;

  /**
   * Hit test the upper bound of a range
   */
  bool HitTestRangeEnd(const FScrubRangeToScreen& RangeToScreen, const TRange<double>& Range, float HitPixel) const;

  /**
   * Hit test a time
   */
  bool HitTestTime(const FScrubRangeToScreen& RangeToScreen, double Time, float HitPixel) const;

  /**
   * Hit test draggable times
   * @return the index of the hit time, or INDEX_NONE if not hit
   */
  int32 HitTestTimes(const FScrubRangeToScreen& RangeToScreen, float HitPixel) const;

  void SetPlaybackRangeStart(FFrameNumber NewStart);
  void SetPlaybackRangeEnd(FFrameNumber NewEnd);

  void SetSelectionRangeStart(FFrameNumber NewStart);
  void SetSelectionRangeEnd(FFrameNumber NewEnd);

  TSharedRef<SWidget> OpenSetPlaybackRangeMenu(FFrameNumber FrameNumber);
  FFrameTime ComputeFrameTimeFromMouse(
      const FGeometry& Geometry, FVector2D ScreenSpacePosition, FScrubRangeToScreen RangeToScreen,
      bool CheckSnapping = true
  ) const;

 private:
  struct FScrubPixelRange {
    TRange<float> Range;
    TRange<float> HandleRange;
    bool bClamped;
  };

  FScrubPixelRange GetHitTestScrubberPixelRange(FFrameTime ScrubTime, const FScrubRangeToScreen& RangeToScreen) const;

  FScrubPixelRange GetScrubberPixelRange(FFrameTime ScrubTime, const FScrubRangeToScreen& RangeToScreen) const;
  FScrubPixelRange GetScrubberPixelRange(
      FFrameTime ScrubTime, FFrameRate Resolution, FFrameRate PlayRate, const FScrubRangeToScreen& RangeToScreen,
      float DilationPixels = 0.f
  ) const;

  void SetEditableTime(int32 TimeIndex, float Time, bool bIsDragging);

  int32 DrawEditableTimes(
      const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
      int32 LayerId, const FScrubRangeToScreen& RangeToScreen
  ) const;
};