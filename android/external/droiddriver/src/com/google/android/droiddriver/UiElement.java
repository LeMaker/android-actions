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

package com.google.android.droiddriver;

import android.graphics.Rect;

import com.google.android.droiddriver.actions.Action;
import com.google.android.droiddriver.actions.InputInjector;
import com.google.android.droiddriver.finders.Attribute;
import com.google.android.droiddriver.finders.Predicate;
import com.google.android.droiddriver.instrumentation.InstrumentationDriver;
import com.google.android.droiddriver.scroll.Direction.PhysicalDirection;
import com.google.android.droiddriver.uiautomation.UiAutomationDriver;

import java.util.List;

/**
 * Represents an UI element within an Android App.
 * <p>
 * UI elements are generally views. Users can get attributes and perform
 * actions. Note that actions often update UiElement, so users are advised not
 * to store instances for later use -- the instances could become stale.
 */
public interface UiElement {
  /**
   * Gets the text of this element.
   */
  String getText();

  /**
   * Gets the content description of this element.
   */
  String getContentDescription();

  /**
   * Gets the class name of the underlying view. The actual name could be
   * overridden.
   *
   * @see com.google.android.droiddriver.instrumentation.ViewElement#overrideClassName
   */
  String getClassName();

  /**
   * Gets the resource id of this element.
   */
  String getResourceId();

  /**
   * Gets the package name of this element.
   */
  String getPackageName();

  /**
   * @return whether or not this element is visible on the device's display.
   */
  boolean isVisible();

  /**
   * @return whether this element is checkable.
   */
  boolean isCheckable();

  /**
   * @return whether this element is checked.
   */
  boolean isChecked();

  /**
   * @return whether this element is clickable.
   */
  boolean isClickable();

  /**
   * @return whether this element is enabled.
   */
  boolean isEnabled();

  /**
   * @return whether this element is focusable.
   */
  boolean isFocusable();

  /**
   * @return whether this element is focused.
   */
  boolean isFocused();

  /**
   * @return whether this element is scrollable.
   */
  boolean isScrollable();

  /**
   * @return whether this element is long-clickable.
   */
  boolean isLongClickable();

  /**
   * @return whether this element is password.
   */
  boolean isPassword();

  /**
   * @return whether this element is selected.
   */
  boolean isSelected();

  /**
   * Gets the UiElement bounds in screen coordinates. The coordinates may not be
   * visible on screen.
   */
  Rect getBounds();

  /**
   * Gets the UiElement bounds in screen coordinates. The coordinates will be
   * visible on screen.
   */
  Rect getVisibleBounds();

  /**
   * @return value of the given attribute.
   */
  <T> T get(Attribute attribute);

  /**
   * Executes the given action.
   *
   * @param action The action to execute
   * @return true if the action is successful
   */
  boolean perform(Action action);

  /**
   * Sets the text of this element.
   *
   * @param text The text to enter.
   */
  void setText(String text);

  /**
   * Clicks this element. The click will be at the center of the visible
   * element.
   */
  void click();

  /**
   * Long-clicks this element. The click will be at the center of the visible
   * element.
   */
  void longClick();

  /**
   * Double-clicks this element. The click will be at the center of the visible
   * element.
   */
  void doubleClick();

  /**
   * Scrolls in the given direction.
   *
   * @param direction specifies where the view port will move, instead of the
   *        finger.
   */
  void scroll(PhysicalDirection direction);

  /**
   * Gets an immutable {@link List} of immediate children that satisfy
   * {@code predicate}. It always filters children that are null. This gives a
   * low level access to the underlying data. Do not use it unless you are sure
   * about the subtle details. Note the count may not be what you expect. For
   * instance, a dynamic list may show more items when scrolling beyond the end,
   * varying the count. The count also depends on the driver implementation:
   * <ul>
   * <li>{@link InstrumentationDriver} includes all.</li>
   * <li>the Accessibility API (which {@link UiAutomationDriver} depends on)
   * does not include off-screen children, but may include invisible on-screen
   * children.</li>
   * </ul>
   * <p>
   * Another discrepancy between {@link InstrumentationDriver}
   * {@link UiAutomationDriver} is the order of children. The Accessibility API
   * returns children in the order of layout (see
   * {@link android.view.ViewGroup#addChildrenForAccessibility}, which is added
   * in API16).
   * </p>
   */
  List<? extends UiElement> getChildren(Predicate<? super UiElement> predicate);

  /**
   * Filters out invisible children.
   */
  Predicate<UiElement> VISIBLE = new Predicate<UiElement>() {
    @Override
    public boolean apply(UiElement element) {
      return element.isVisible();
    }

    @Override
    public String toString() {
      return "VISIBLE";
    }
  };

  /**
   * Gets the parent.
   */
  UiElement getParent();

  /**
   * Gets the {@link InputInjector} for injecting InputEvent.
   */
  InputInjector getInjector();
}
