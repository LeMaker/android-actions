package com.android.tv.settings.widget;

import android.content.Context;
import android.content.res.TypedArray;
import android.text.Layout;
import android.util.AttributeSet;
import android.util.TypedValue;
import android.widget.TextView;

import java.text.BreakIterator;

import com.android.tv.settings.R;

/**
 * <p>A {@link android.widget.TextView} that changes the font size depending on the text width.</p>
 *
 * <p>We check for two signals that indicates that the text view is too small for the given text:
 * 1. whether there is an ellipsis, and 2. whether a word spans two lines.  If either of these
 * conditions are met, we move down to a smaller font size.  If the text is still too big after
 * resizing, we give up.</p>
 *
 * <p>This method is not completely fool-proof because the string tokenizing method used by
 * StaticLayout is not the same as we use here.  So some things (e.g. email addresses) might be
 * wrapped without shrinking.</p>
 */
public class ResizingTextView extends TextView {

    public ResizingTextView(Context ctx, AttributeSet attrs, int defStyleAttr, int defStyleRes) {
        super(ctx, attrs, defStyleAttr, defStyleRes);
    }

    public ResizingTextView(Context ctx, AttributeSet attrs, int defStyleAttr) {
        super(ctx, attrs, defStyleAttr);
    }

    public ResizingTextView(Context ctx, AttributeSet attrs) {
        super(ctx, attrs);
    }

    public ResizingTextView(Context ctx) {
        super(ctx);
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);
        boolean switchToSmallFont = false;

        final Layout layout = getLayout();
        if (layout != null) {
            final int lineCount = layout.getLineCount();
            if (lineCount > 0) {
                final int ellipsisCount = layout.getEllipsisCount(lineCount - 1);
                if (ellipsisCount > 0) {
                    // Always just go to small font if we have to ellipsize.
                    switchToSmallFont = true;
                } else {
                    // No ellipsis, but we could have words that span lines.
                    BreakIterator iter = BreakIterator.getWordInstance();
                    iter.setText(getText().toString());
                    int breakIndex = iter.next();
                    for (int line = 0; line < layout.getLineCount() - 1; line++) {
                        iter.following(layout.getLineStart(line));
                        if (breakIndex > layout.getLineVisibleEnd(line)) {
                            // Line breaks before a word boundary.
                            switchToSmallFont = true;
                            break;
                        }
                    }
                }
            }
        }

        if (switchToSmallFont) {
            final int normalSizePx = mContext.getResources().getDimensionPixelSize(
                    R.dimen.browse_item_title_font_size);
            final int smallSizePx = mContext.getResources().getDimensionPixelSize(
                    R.dimen.browse_item_title_font_size_small);

            // In addition to setting the text size, we fudge the line spacing and padding so it
            // doesn't look so strange next to non-resized text.
            setTextSize(TypedValue.COMPLEX_UNIT_PX, smallSizePx);
            setLineSpacing(normalSizePx - smallSizePx, 1.0f);
            setPaddingRelative(0, 0, 0, (normalSizePx - smallSizePx) / 2);
            super.onMeasure(widthMeasureSpec, heightMeasureSpec);
        }
    }
}
