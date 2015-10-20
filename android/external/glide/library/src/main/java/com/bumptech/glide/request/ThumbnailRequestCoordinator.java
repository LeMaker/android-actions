package com.bumptech.glide.request;

/**
 * A coordinator that coordinates two individual {@link Request}s that load a small thumbnail version of an image and
 * the full size version of the image at the same time.
 */
public class ThumbnailRequestCoordinator implements RequestCoordinator, Request {
    private Request full;
    private Request thumb;
    private RequestCoordinator coordinator;

    public ThumbnailRequestCoordinator() {
        this(null);
    }

    public ThumbnailRequestCoordinator(RequestCoordinator coordinator) {
        this.coordinator = coordinator;
    }

    public void setRequests(Request full, Request thumb) {
        this.full = full;
        this.thumb = thumb;
    }

    @Override
    public boolean canSetImage(Request request) {
        return parentCanSetImage() && (request == full || !full.isComplete());
    }

    private boolean parentCanSetImage() {
        return coordinator == null || coordinator.canSetImage(this);
    }

    @Override
    public boolean canSetPlaceholder(Request request) {
        return parentCanSetPlaceholder() && (request == full && !isAnyRequestComplete());
    }

    private boolean parentCanSetPlaceholder() {
        return coordinator == null || coordinator.canSetPlaceholder(this);
    }

    @Override
    public boolean isAnyRequestComplete() {
        return parentIsAnyRequestComplete() || full.isComplete() || thumb.isComplete();
    }

    private boolean parentIsAnyRequestComplete() {
        return coordinator != null && coordinator.isAnyRequestComplete();
    }

    @Override
    public void run() {
        if (!thumb.isRunning()) {
            thumb.run();
        }
        if (!full.isRunning()) {
            full.run();
        }
    }

    @Override
    public void clear() {
        full.clear();
        thumb.clear();
    }

    @Override
    public boolean isRunning() {
        return full.isRunning();
    }

    @Override
    public boolean isComplete() {
        // TODO: this is a little strange, but we often want to avoid restarting the request or
        // setting placeholders even if only the thumb is complete.
        return full.isComplete() || thumb.isComplete();
    }

    @Override
    public boolean isFailed() {
        return full.isFailed();
    }

    @Override
    public void recycle() {
        full.recycle();
        thumb.recycle();
    }
}
