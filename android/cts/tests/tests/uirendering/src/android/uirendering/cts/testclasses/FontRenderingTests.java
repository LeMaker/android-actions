/*
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.uirendering.cts.testclasses;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Typeface;
import android.test.suitebuilder.annotation.SmallTest;
import android.uirendering.cts.bitmapcomparers.BitmapComparer;
import android.uirendering.cts.bitmapcomparers.MSSIMComparer;
import android.uirendering.cts.bitmapverifiers.GoldenImageVerifier;
import android.uirendering.cts.testinfrastructure.ActivityTestBase;
import android.uirendering.cts.testinfrastructure.CanvasClient;

import com.android.cts.uirendering.R;

public class FontRenderingTests extends ActivityTestBase {
    // Threshold is barely loose enough for differences between sw and hw renderers
    static double MSSIM_THRESHOLD = 0.91;

    private final BitmapComparer mFuzzyComparer = new MSSIMComparer(MSSIM_THRESHOLD);

    // Representative characters including some from Unicode 7
    private final String mTestString1 = "Hamburg \u20bd";
    private final String mTestString2 = "\u20b9\u0186\u0254\u1e24\u1e43";

    private void fontTestBody(final Typeface typeface, int id) {
        Bitmap goldenBitmap = BitmapFactory.decodeResource(getActivity().getResources(), id);
        createTest()
                .addCanvasClient(new CanvasClient() {
                    @Override
                    public void draw(Canvas canvas, int width, int height) {
                        Paint p = new Paint();
                        p.setAntiAlias(true);
                        p.setColor(Color.BLACK);
                        p.setTextSize(30);
                        p.setTypeface(typeface);
                        canvas.drawText(mTestString1, 10, 60, p);
                        canvas.drawText(mTestString2, 10, 100, p);
                    }
                })
                .runWithVerifier(new GoldenImageVerifier(goldenBitmap, mFuzzyComparer));
    }

    @SmallTest
    public void testDefaultFont() {
        Typeface tf = Typeface.create("sans-serif", Typeface.NORMAL);
        fontTestBody(tf, R.drawable.hello1);
    }

    @SmallTest
    public void testBoldFont() {
        Typeface tf = Typeface.create("sans-serif", Typeface.BOLD);
        fontTestBody(tf, R.drawable.bold1);
    }

    @SmallTest
    public void testItalicFont() {
        Typeface tf = Typeface.create("sans-serif", Typeface.ITALIC);
        fontTestBody(tf, R.drawable.italic1);
    }

    @SmallTest
    public void testBoldItalicFont() {
        Typeface tf = Typeface.create("sans-serif", Typeface.BOLD | Typeface.ITALIC);
        fontTestBody(tf, R.drawable.bolditalic1);
    }

    @SmallTest
    public void testMediumFont() {
        Typeface tf = Typeface.create("sans-serif-medium", Typeface.NORMAL);
        fontTestBody(tf, R.drawable.medium1);
    }

    @SmallTest
    public void testMediumBoldFont() {
        // bold attribute on medium base font = black
        Typeface tf = Typeface.create("sans-serif-medium", Typeface.BOLD);
        fontTestBody(tf, R.drawable.black1);
    }

    @SmallTest
    public void testMediumItalicFont() {
        Typeface tf = Typeface.create("sans-serif-medium", Typeface.ITALIC);
        fontTestBody(tf, R.drawable.mediumitalic1);
    }

    @SmallTest
    public void testMediumBoldItalicFont() {
        Typeface tf = Typeface.create("sans-serif-medium", Typeface.BOLD | Typeface.ITALIC);
        fontTestBody(tf, R.drawable.blackitalic1);
    }

    @SmallTest
    public void testLightFont() {
        Typeface tf = Typeface.create("sans-serif-light", Typeface.NORMAL);
        fontTestBody(tf, R.drawable.light1);
    }

    @SmallTest
    public void testLightBoldFont() {
        // bold attribute on light base font = medium
        Typeface tf = Typeface.create("sans-serif-light", Typeface.BOLD);
        fontTestBody(tf, R.drawable.medium1);
    }

    @SmallTest
    public void testLightItalicFont() {
        Typeface tf = Typeface.create("sans-serif-light", Typeface.ITALIC);
        fontTestBody(tf, R.drawable.lightitalic1);
    }

    @SmallTest
    public void testLightBoldItalicFont() {
        Typeface tf = Typeface.create("sans-serif-light", Typeface.BOLD | Typeface.ITALIC);
        fontTestBody(tf, R.drawable.mediumitalic1);
    }

    @SmallTest
    public void testThinFont() {
        Typeface tf = Typeface.create("sans-serif-thin", Typeface.NORMAL);
        fontTestBody(tf, R.drawable.thin1);
    }

    @SmallTest
    public void testThinBoldFont() {
        // bold attribute on thin base font = normal
        Typeface tf = Typeface.create("sans-serif-thin", Typeface.BOLD);
        fontTestBody(tf, R.drawable.hello1);
    }

    @SmallTest
    public void testThinItalicFont() {
        Typeface tf = Typeface.create("sans-serif-thin", Typeface.ITALIC);
        fontTestBody(tf, R.drawable.thinitalic1);
    }

    @SmallTest
    public void testThinBoldItalicFont() {
        Typeface tf = Typeface.create("sans-serif-thin", Typeface.BOLD | Typeface.ITALIC);
        fontTestBody(tf, R.drawable.italic1);
    }

    @SmallTest
    public void testBlackFont() {
        Typeface tf = Typeface.create("sans-serif-black", Typeface.NORMAL);
        fontTestBody(tf, R.drawable.black1);
    }

    @SmallTest
    public void testBlackBoldFont() {
        // bold attribute on black base font = black
        Typeface tf = Typeface.create("sans-serif-black", Typeface.BOLD);
        fontTestBody(tf, R.drawable.black1);
    }

    @SmallTest
    public void testBlackItalicFont() {
        Typeface tf = Typeface.create("sans-serif-black", Typeface.ITALIC);
        fontTestBody(tf, R.drawable.blackitalic1);
    }

    @SmallTest
    public void testBlackBoldItalicFont() {
        Typeface tf = Typeface.create("sans-serif-black", Typeface.BOLD | Typeface.ITALIC);
        fontTestBody(tf, R.drawable.blackitalic1);
    }

    /* condensed fonts */

    @SmallTest
    public void testCondensedFont() {
        Typeface tf = Typeface.create("sans-serif-condensed", Typeface.NORMAL);
        fontTestBody(tf, R.drawable.condensed1);
    }

    @SmallTest
    public void testCondensedBoldFont() {
        Typeface tf = Typeface.create("sans-serif-condensed", Typeface.BOLD);
        fontTestBody(tf, R.drawable.condensedbold1);
    }

    @SmallTest
    public void testCondensedItalicFont() {
        Typeface tf = Typeface.create("sans-serif-condensed", Typeface.ITALIC);
        fontTestBody(tf, R.drawable.condenseditalic1);
    }

    @SmallTest
    public void testCondensedBoldItalicFont() {
        Typeface tf = Typeface.create("sans-serif-condensed", Typeface.BOLD | Typeface.ITALIC);
        fontTestBody(tf, R.drawable.condensedbolditalic1);
    }

    @SmallTest
    public void testCondensedLightFont() {
        Typeface tf = Typeface.create("sans-serif-condensed-light", Typeface.NORMAL);
        fontTestBody(tf, R.drawable.condensedlight1);
    }

    @SmallTest
    public void testCondensedLightItalicFont() {
        Typeface tf = Typeface.create("sans-serif-condensed-light", Typeface.ITALIC);
        fontTestBody(tf, R.drawable.condensedlightitalic1);
    }
}
