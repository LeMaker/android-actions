/*
 * Copyright (C) 2013 DroidDriver committers
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

package com.google.android.droiddriver.actions;

import android.os.Build;
import android.os.SystemClock;
import android.view.KeyEvent;

import com.google.android.droiddriver.UiElement;
import com.google.android.droiddriver.util.Events;
import com.google.android.droiddriver.util.Strings;

/**
 * An action to press a single key. While it is convenient for navigating the
 * UI, do not overuse it -- the application may interpret key codes in a custom
 * way and, more importantly, application users may not have access to it
 * because the device (physical or virtual keyboard) may not support all key
 * codes.
 */
public class SingleKeyAction extends KeyAction {
  /**
   * Common instances for convenience and memory preservation.
   */
  public static final SingleKeyAction MENU = new SingleKeyAction(KeyEvent.KEYCODE_MENU);
  public static final SingleKeyAction SEARCH = new SingleKeyAction(KeyEvent.KEYCODE_SEARCH);
  public static final SingleKeyAction BACK = new SingleKeyAction(KeyEvent.KEYCODE_BACK);
  public static final SingleKeyAction DELETE = new SingleKeyAction(KeyEvent.KEYCODE_DEL);

  private final int keyCode;

  /**
   * Defaults timeoutMillis to 100.
   */
  public SingleKeyAction(int keyCode) {
    this(keyCode, 100L, false);
  }

  public SingleKeyAction(int keyCode, long timeoutMillis, boolean checkFocused) {
    super(timeoutMillis, checkFocused);
    this.keyCode = keyCode;
  }

  @Override
  public boolean perform(InputInjector injector, UiElement element) {
    maybeCheckFocused(element);

    final long downTime = SystemClock.uptimeMillis();
    KeyEvent downEvent = Events.newKeyEvent(downTime, KeyEvent.ACTION_DOWN, keyCode);
    KeyEvent upEvent = Events.newKeyEvent(downTime, KeyEvent.ACTION_UP, keyCode);

    return injector.injectInputEvent(downEvent) && injector.injectInputEvent(upEvent);
  }

  @Override
  public String toString() {
    String keyCodeString =
        Build.VERSION.SDK_INT < Build.VERSION_CODES.HONEYCOMB_MR1 ? String.valueOf(keyCode)
            : KeyEvent.keyCodeToString(keyCode);
    return Strings.toStringHelper(this).addValue(keyCodeString).toString();
  }
}
