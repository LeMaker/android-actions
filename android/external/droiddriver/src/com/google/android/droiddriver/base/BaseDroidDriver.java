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

import com.google.android.droiddriver.DroidDriver;
import com.google.android.droiddriver.Poller;
import com.google.android.droiddriver.UiElement;
import com.google.android.droiddriver.actions.InputInjector;
import com.google.android.droiddriver.exceptions.ElementNotFoundException;
import com.google.android.droiddriver.exceptions.TimeoutException;
import com.google.android.droiddriver.finders.ByXPath;
import com.google.android.droiddriver.finders.Finder;
import com.google.android.droiddriver.util.Logs;

/**
 * Base DroidDriver that implements the common operations.
 */
public abstract class BaseDroidDriver<R, E extends BaseUiElement<R, E>> implements DroidDriver {

  private Poller poller = new DefaultPoller();
  private E rootElement;

  @Override
  public UiElement find(Finder finder) {
    Logs.call(this, "find", finder);
    return finder.find(getRootElement());
  }

  @Override
  public boolean has(Finder finder) {
    try {
      refreshUiElementTree();
      find(finder);
      return true;
    } catch (ElementNotFoundException enfe) {
      return false;
    }
  }

  @Override
  public boolean has(Finder finder, long timeoutMillis) {
    try {
      getPoller().pollFor(this, finder, Poller.EXISTS, timeoutMillis);
      return true;
    } catch (TimeoutException e) {
      return false;
    }
  }

  @Override
  public UiElement on(Finder finder) {
    Logs.call(this, "on", finder);
    return getPoller().pollFor(this, finder, Poller.EXISTS);
  }

  @Override
  public void checkExists(Finder finder) {
    Logs.call(this, "checkExists", finder);
    getPoller().pollFor(this, finder, Poller.EXISTS);
  }

  @Override
  public void checkGone(Finder finder) {
    Logs.call(this, "checkGone", finder);
    getPoller().pollFor(this, finder, Poller.GONE);
  }

  @Override
  public Poller getPoller() {
    return poller;
  }

  @Override
  public void setPoller(Poller poller) {
    this.poller = poller;
  }

  public abstract InputInjector getInjector();

  protected abstract E newRootElement();

  /**
   * Returns a new UiElement of type {@code E}.
   */
  protected abstract E newUiElement(R rawElement, E parent);

  public E getRootElement() {
    if (rootElement == null) {
      refreshUiElementTree();
    }
    return rootElement;
  }

  @Override
  public void refreshUiElementTree() {
    rootElement = newRootElement();
  }

  @Override
  public boolean dumpUiElementTree(String path) {
    Logs.call(this, "dumpUiElementTree", path);
    return ByXPath.dumpDom(path, getRootElement());
  }
}
