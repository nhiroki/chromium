// Copyright 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CCLayerTreeHostImpl_h
#define CCLayerTreeHostImpl_h

#include "base/basictypes.h"
#include "base/memory/scoped_ptr.h"
#include "base/time.h"
#include "CCAnimationEvents.h"
#include "CCInputHandler.h"
#include "CCLayerSorter.h"
#include "CCRenderer.h"
#include "CCRenderPass.h"
#include "CCRenderPassSink.h"
#include "third_party/skia/include/core/SkColor.h"
#include <public/WebCompositorOutputSurfaceClient.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/RefPtr.h>

namespace cc {

class CCCompletionEvent;
class CCDebugRectHistory;
class CCFrameRateCounter;
class CCHeadsUpDisplayLayerImpl;
class CCLayerImpl;
class CCLayerTreeHostImplTimeSourceAdapter;
class CCPageScaleAnimation;
class CCRenderPassDrawQuad;
class CCResourceProvider;
struct RendererCapabilities;
struct CCRenderingStats;

// CCLayerTreeHost->CCProxy callback interface.
class CCLayerTreeHostImplClient {
public:
    virtual void didLoseContextOnImplThread() = 0;
    virtual void onSwapBuffersCompleteOnImplThread() = 0;
    virtual void onVSyncParametersChanged(double monotonicTimebase, double intervalInSeconds) = 0;
    virtual void onCanDrawStateChanged(bool canDraw) = 0;
    virtual void setNeedsRedrawOnImplThread() = 0;
    virtual void setNeedsCommitOnImplThread() = 0;
    virtual void postAnimationEventsToMainThreadOnImplThread(scoped_ptr<CCAnimationEventsVector>, double wallClockTime) = 0;
    virtual void releaseContentsTexturesOnImplThread() = 0;
};

// CCPinchZoomViewport models the bounds and offset of the viewport that is used during a pinch-zoom operation.
// It tracks the layout-space dimensions of the viewport before any applied scale, and then tracks the layout-space
// coordinates of the viewport respecting the pinch settings.
class CCPinchZoomViewport {
public:
    CCPinchZoomViewport();

    float totalPageScaleFactor() const;

    void setPageScaleFactor(float factor) { m_pageScaleFactor = factor; }
    float pageScaleFactor() const { return m_pageScaleFactor; }

    void setPageScaleDelta(float delta);
    float pageScaleDelta() const  { return m_pageScaleDelta; }

    float minPageScaleFactor() const { return m_minPageScaleFactor; }
    float maxPageScaleFactor() const { return m_maxPageScaleFactor; }

    void setSentPageScaleDelta(float delta) { m_sentPageScaleDelta = delta; }
    float sentPageScaleDelta() const { return m_sentPageScaleDelta; }

    // Returns true if the passed parameters were different from those previously
    // cached.
    bool setPageScaleFactorAndLimits(float pageScaleFactor,
                                     float minPageScaleFactor,
                                     float maxPageScaleFactor);

    // Returns the bounds and offset of the scaled and translated viewport to use for pinch-zoom.
    FloatRect bounds() const;
    const FloatPoint& scrollDelta() const { return m_pinchViewportScrollDelta; }

    void setLayoutViewportSize(const FloatSize& size) { m_layoutViewportSize = size; }

    // Apply the scroll offset in layout space to the offset of the pinch-zoom viewport. The viewport cannot be
    // scrolled outside of the layout viewport bounds. Returns the component of the scroll that is un-applied due to
    // this constraint.
    FloatSize applyScroll(FloatSize&);

    WebKit::WebTransformationMatrix implTransform() const;

private:
    float m_pageScaleFactor;
    float m_pageScaleDelta;
    float m_sentPageScaleDelta;
    float m_maxPageScaleFactor;
    float m_minPageScaleFactor;

    FloatPoint m_pinchViewportScrollDelta;
    FloatSize m_layoutViewportSize;
};

// CCLayerTreeHostImpl owns the CCLayerImpl tree as well as associated rendering state
class CCLayerTreeHostImpl : public CCInputHandlerClient,
                            public CCRendererClient,
                            public WebKit::WebCompositorOutputSurfaceClient {
    typedef std::vector<CCLayerImpl*> CCLayerList;

public:
    static scoped_ptr<CCLayerTreeHostImpl> create(const CCLayerTreeSettings&, CCLayerTreeHostImplClient*);
    virtual ~CCLayerTreeHostImpl();

    // CCInputHandlerClient implementation
    virtual CCInputHandlerClient::ScrollStatus scrollBegin(const IntPoint&, CCInputHandlerClient::ScrollInputType) OVERRIDE;
    virtual void scrollBy(const IntPoint&, const IntSize&) OVERRIDE;
    virtual void scrollEnd() OVERRIDE;
    virtual void pinchGestureBegin() OVERRIDE;
    virtual void pinchGestureUpdate(float, const IntPoint&) OVERRIDE;
    virtual void pinchGestureEnd() OVERRIDE;
    virtual void startPageScaleAnimation(const IntSize& targetPosition, bool anchorPoint, float pageScale, double startTime, double duration) OVERRIDE;
    virtual void scheduleAnimation() OVERRIDE;

    struct FrameData : public CCRenderPassSink {
        FrameData();
        ~FrameData();

        Vector<IntRect> occludingScreenSpaceRects;
        CCRenderPassList renderPasses;
        CCRenderPassIdHashMap renderPassesById;
        CCLayerList* renderSurfaceLayerList;
        CCLayerList willDrawLayers;

        // CCRenderPassSink implementation.
        virtual void appendRenderPass(scoped_ptr<CCRenderPass>) OVERRIDE;
    };

    // Virtual for testing.
    virtual void beginCommit();
    virtual void commitComplete();
    virtual void animate(double monotonicTime, double wallClockTime);

    // Returns false if problems occured preparing the frame, and we should try
    // to avoid displaying the frame. If prepareToDraw is called,
    // didDrawAllLayers must also be called, regardless of whether drawLayers is
    // called between the two.
    virtual bool prepareToDraw(FrameData&);
    virtual void drawLayers(const FrameData&);
    // Must be called if and only if prepareToDraw was called.
    void didDrawAllLayers(const FrameData&);

    // CCRendererClient implementation
    virtual const IntSize& deviceViewportSize() const OVERRIDE;
    virtual const CCLayerTreeSettings& settings() const OVERRIDE;
    virtual void didLoseContext() OVERRIDE;
    virtual void onSwapBuffersComplete() OVERRIDE;
    virtual void setFullRootLayerDamage() OVERRIDE;
    virtual void releaseContentsTextures() OVERRIDE;
    virtual void setMemoryAllocationLimitBytes(size_t) OVERRIDE;

    // WebCompositorOutputSurfaceClient implementation.
    virtual void onVSyncParametersChanged(double monotonicTimebase, double intervalInSeconds) OVERRIDE;

    // Implementation
    bool canDraw();
    CCGraphicsContext* context() const;

    std::string layerTreeAsText() const;

    void finishAllRendering();
    int sourceAnimationFrameNumber() const;

    bool initializeRenderer(scoped_ptr<CCGraphicsContext>);
    bool isContextLost();
    CCRenderer* renderer() { return m_renderer.get(); }
    const RendererCapabilities& rendererCapabilities() const;

    bool swapBuffers();

    void readback(void* pixels, const IntRect&);

    void setRootLayer(scoped_ptr<CCLayerImpl>);
    CCLayerImpl* rootLayer() { return m_rootLayerImpl.get(); }

    void setHudLayer(CCHeadsUpDisplayLayerImpl* layerImpl) { m_hudLayerImpl = layerImpl; }
    CCHeadsUpDisplayLayerImpl* hudLayer() { return m_hudLayerImpl; }

    // Release ownership of the current layer tree and replace it with an empty
    // tree. Returns the root layer of the detached tree.
    scoped_ptr<CCLayerImpl> detachLayerTree();

    CCLayerImpl* rootScrollLayer() const { return m_rootScrollLayerImpl; }

    bool visible() const { return m_visible; }
    void setVisible(bool);

    int sourceFrameNumber() const { return m_sourceFrameNumber; }
    void setSourceFrameNumber(int frameNumber) { m_sourceFrameNumber = frameNumber; }

    bool contentsTexturesPurged() const { return m_contentsTexturesPurged; }
    void setContentsTexturesPurged();
    void resetContentsTexturesPurged();
    size_t memoryAllocationLimitBytes() const { return m_memoryAllocationLimitBytes; }

    void setViewportSize(const IntSize& layoutViewportSize, const IntSize& deviceViewportSize);
    const IntSize& layoutViewportSize() const { return m_layoutViewportSize; }

    float deviceScaleFactor() const { return m_deviceScaleFactor; }
    void setDeviceScaleFactor(float);

    float pageScaleFactor() const;
    void setPageScaleFactorAndLimits(float pageScaleFactor, float minPageScaleFactor, float maxPageScaleFactor);

    scoped_ptr<CCScrollAndScaleSet> processScrollDeltas();
    WebKit::WebTransformationMatrix implTransform() const;

    void startPageScaleAnimation(const IntSize& tragetPosition, bool useAnchor, float scale, double durationSec);

    SkColor backgroundColor() const { return m_backgroundColor; }
    void setBackgroundColor(SkColor color) { m_backgroundColor = color; }

    bool hasTransparentBackground() const { return m_hasTransparentBackground; }
    void setHasTransparentBackground(bool transparent) { m_hasTransparentBackground = transparent; }

    bool needsAnimateLayers() const { return m_needsAnimateLayers; }
    void setNeedsAnimateLayers() { m_needsAnimateLayers = true; }

    void setNeedsRedraw();

    void renderingStats(CCRenderingStats*) const;

    void updateRootScrollLayerImplTransform();

    CCFrameRateCounter* fpsCounter() const { return m_fpsCounter.get(); }
    CCDebugRectHistory* debugRectHistory() const { return m_debugRectHistory.get(); }
    CCResourceProvider* resourceProvider() const { return m_resourceProvider.get(); }

    class CullRenderPassesWithCachedTextures {
    public:
        bool shouldRemoveRenderPass(const CCRenderPassDrawQuad&, const FrameData&) const;

        // Iterates from the root first, in order to remove the surfaces closest
        // to the root with cached textures, and all surfaces that draw into
        // them.
        size_t renderPassListBegin(const CCRenderPassList& list) const { return list.size() - 1; }
        size_t renderPassListEnd(const CCRenderPassList&) const { return 0 - 1; }
        size_t renderPassListNext(size_t it) const { return it - 1; }

        CullRenderPassesWithCachedTextures(CCRenderer& renderer) : m_renderer(renderer) { }
    private:
        CCRenderer& m_renderer;
    };

    class CullRenderPassesWithNoQuads {
    public:
        bool shouldRemoveRenderPass(const CCRenderPassDrawQuad&, const FrameData&) const;

        // Iterates in draw order, so that when a surface is removed, and its
        // target becomes empty, then its target can be removed also.
        size_t renderPassListBegin(const CCRenderPassList&) const { return 0; }
        size_t renderPassListEnd(const CCRenderPassList& list) const { return list.size(); }
        size_t renderPassListNext(size_t it) const { return it + 1; }
    };

    template<typename RenderPassCuller>
    static void removeRenderPasses(RenderPassCuller, FrameData&);

protected:
    CCLayerTreeHostImpl(const CCLayerTreeSettings&, CCLayerTreeHostImplClient*);

    void animatePageScale(double monotonicTime);
    void animateScrollbars(double monotonicTime);

    // Exposed for testing.
    void calculateRenderSurfaceLayerList(CCLayerList&);

    // Virtual for testing.
    virtual void animateLayers(double monotonicTime, double wallClockTime);

    // Virtual for testing.
    virtual base::TimeDelta lowFrequencyAnimationInterval() const;

    CCLayerTreeHostImplClient* m_client;
    int m_sourceFrameNumber;

private:
    void computeDoubleTapZoomDeltas(CCScrollAndScaleSet* scrollInfo);
    void computePinchZoomDeltas(CCScrollAndScaleSet* scrollInfo);
    void makeScrollAndScaleSet(CCScrollAndScaleSet* scrollInfo, const IntSize& scrollOffset, float pageScale);

    void setPageScaleDelta(float);
    void updateMaxScrollPosition();
    void trackDamageForAllSurfaces(CCLayerImpl* rootDrawLayer, const CCLayerList& renderSurfaceLayerList);

    // Returns false if the frame should not be displayed. This function should
    // only be called from prepareToDraw, as didDrawAllLayers must be called
    // if this helper function is called.
    bool calculateRenderPasses(FrameData&);
    void animateLayersRecursive(CCLayerImpl*, double monotonicTime, double wallClockTime, CCAnimationEventsVector*, bool& didAnimate, bool& needsAnimateLayers);
    void setBackgroundTickingEnabled(bool);
    IntSize contentSize() const;

    void sendDidLoseContextRecursive(CCLayerImpl*);
    void clearRenderSurfaces();
    bool ensureRenderSurfaceLayerList();
    void clearCurrentlyScrollingLayer();

    void animateScrollbarsRecursive(CCLayerImpl*, double monotonicTime);

    void dumpRenderSurfaces(std::string*, int indent, const CCLayerImpl*) const;

    scoped_ptr<CCGraphicsContext> m_context;
    scoped_ptr<CCResourceProvider> m_resourceProvider;
    scoped_ptr<CCRenderer> m_renderer;
    scoped_ptr<CCLayerImpl> m_rootLayerImpl;
    CCLayerImpl* m_rootScrollLayerImpl;
    CCLayerImpl* m_currentlyScrollingLayerImpl;
    CCHeadsUpDisplayLayerImpl* m_hudLayerImpl;
    int m_scrollingLayerIdFromPreviousTree;
    bool m_scrollDeltaIsInScreenSpace;
    CCLayerTreeSettings m_settings;
    IntSize m_layoutViewportSize;
    IntSize m_deviceViewportSize;
    float m_deviceScaleFactor;
    bool m_visible;
    bool m_contentsTexturesPurged;
    size_t m_memoryAllocationLimitBytes;

    SkColor m_backgroundColor;
    bool m_hasTransparentBackground;

    // If this is true, it is necessary to traverse the layer tree ticking the animators.
    bool m_needsAnimateLayers;
    bool m_pinchGestureActive;
    IntPoint m_previousPinchAnchor;

    scoped_ptr<CCPageScaleAnimation> m_pageScaleAnimation;

    // This is used for ticking animations slowly when hidden.
    scoped_ptr<CCLayerTreeHostImplTimeSourceAdapter> m_timeSourceClientAdapter;

    CCLayerSorter m_layerSorter;

    // List of visible layers for the most recently prepared frame. Used for
    // rendering and input event hit testing.
    CCLayerList m_renderSurfaceLayerList;

    CCPinchZoomViewport m_pinchZoomViewport;

    scoped_ptr<CCFrameRateCounter> m_fpsCounter;
    scoped_ptr<CCDebugRectHistory> m_debugRectHistory;

    size_t m_numImplThreadScrolls;
    size_t m_numMainThreadScrolls;

    DISALLOW_COPY_AND_ASSIGN(CCLayerTreeHostImpl);
};

}  // namespace cc

#endif
