package android.uirendering.cts.testclasses.view;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.util.AttributeSet;
import android.widget.FrameLayout;

public class UnclippedBlueView extends FrameLayout {
    public UnclippedBlueView(Context context) {
        this(context, null);
    }

    public UnclippedBlueView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public UnclippedBlueView(Context context, AttributeSet attrs, int defStyleAttr) {
        this(context, attrs, defStyleAttr, 0);
    }

    public UnclippedBlueView(Context context, AttributeSet attrs,
            int defStyleAttr, int defStyleRes) {
        super(context, attrs, defStyleAttr, defStyleRes);
        setWillNotDraw(false);
    }

    @Override
    protected void onDraw(Canvas canvas) {
        canvas.drawColor(Color.BLUE);
    }
}
