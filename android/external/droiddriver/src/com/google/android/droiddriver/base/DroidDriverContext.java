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

package com.google.android.droiddriver.base;

import android.app.Instrumentation;
import android.os.Looper;
import android.util.Log;

import com.google.android.droiddriver.exceptions.DroidDriverException;
import com.google.android.droiddriver.exceptions.TimeoutException;
import com.google.android.droiddriver.finders.ByXPath;
import com.google.android.droiddriver.util.Logs;

import java.util.Map;
import java.util.WeakHashMap;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.FutureTask;
import java.util.concurrent.TimeUnit;

/**
 * Internal helper for DroidDriver implementation.
 */
public class DroidDriverContext<R, E extends BaseUiElement<R, E>> {
  private final Instrumentation instrumentation;
  private final BaseDroidDriver<R, E> driver;
  private final Map<R, E> map;

  public DroidDriverContext(Instrumentation instrumentation, BaseDroidDriver<R, E> driver) {
    this.instrumentation = instrumentation;
    this.driver = driver;
    map = new WeakHashMap<R, E>();
  }

  public Instrumentation getInstrumentation() {
    return instrumentation;
  }

  public BaseDroidDriver<R, E> getDriver() {
    return driver;
  }

  public E getElement(R rawElement, E parent) {
    E element = map.get(rawElement);
    if (element == null) {
      element = driver.newUiElement(rawElement, parent);
      map.put(rawElement, element);
    }
    return element;
  }

  public E newRootElement(R rawRoot) {
    clearData();
    return getElement(rawRoot, null /* parent */);
  }

  private void clearData() {
    map.clear();
    ByXPath.clearData();
  }

  /**
   * Tries to wait for an idle state on the main thread on best-effort basis up
   * to {@code timeoutMillis}. The main thread may not enter the idle state when
   * animation is playing, for example, the ProgressBar.
   */
  public boolean tryWaitForIdleSync(long timeoutMillis) {
    validateNotAppThread();
    FutureTask<?> futureTask = new FutureTask<Void>(new Runnable() {
      @Override
      public void run() {}
    }, null);
    instrumentation.waitForIdle(futureTask);

    try {
      futureTask.get(timeoutMillis, TimeUnit.MILLISECONDS);
    } catch (InterruptedException e) {
      throw new DroidDriverException(e);
    } catch (ExecutionException e) {
      throw new DroidDriverException(e);
    } catch (java.util.concurrent.TimeoutException e) {
      Logs.log(Log.DEBUG, String.format(
          "Timed out after %d milliseconds waiting for idle on main looper", timeoutMillis));
      return false;
    }
    return true;
  }

  /**
   * Tries to run {@code runnable} on the main thread on best-effort basis up to
   * {@code timeoutMillis}. The {@code runnable} may never run, for example, in
   * case that the main Looper has exited due to uncaught exception.
   */
  public boolean tryRunOnMainSync(Runnable runnable, long timeoutMillis) {
    validateNotAppThread();
    final FutureTask<?> futureTask = new FutureTask<Void>(runnable, null);
    new Thread(new Runnable() {
      @Override
      public void run() {
        instrumentation.runOnMainSync(futureTask);
      }
    }).start();

    try {
      futureTask.get(timeoutMillis, TimeUnit.MILLISECONDS);
    } catch (InterruptedException e) {
      throw new DroidDriverException(e);
    } catch (ExecutionException e) {
      throw new DroidDriverException(e);
    } catch (java.util.concurrent.TimeoutException e) {
      Logs.log(Log.WARN, getRunOnMainSyncTimeoutMessage(timeoutMillis));
      return false;
    }
    return true;
  }

  public void runOnMainSync(Runnable runnable) {
    long timeoutMillis = getDriver().getPoller().getTimeoutMillis();
    if (!tryRunOnMainSync(runnable, timeoutMillis)) {
      throw new TimeoutException(getRunOnMainSyncTimeoutMessage(timeoutMillis));
    }
  }

  private String getRunOnMainSyncTimeoutMessage(long timeoutMillis) {
    return String.format(
        "Timed out after %d milliseconds waiting for Instrumentation.runOnMainSync", timeoutMillis);
  }

  private void validateNotAppThread() {
    if (Looper.myLooper() == Looper.getMainLooper()) {
      throw new DroidDriverException(
          "This method can not be called from the main application thread");
    }
  }
}
