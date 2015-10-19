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

package android.hardware.cts.helpers;

import junit.framework.TestCase;

import java.util.Arrays;
import java.util.Collection;
import java.util.HashSet;
import java.util.List;
import java.util.concurrent.TimeUnit;

/**
 * Unit tests for the {@link SensorCtsHelper} class.
 */
public class SensorCtsHelperTest extends TestCase {

    /**
     * Test {@link SensorCtsHelper#get95PercentileValue(Collection)}.
     */
    public void testGet95PercentileValue() {
        Collection<Integer> values = new HashSet<Integer>();
        for (int i = 0; i < 100; i++) {
            values.add(i);
        }
        assertEquals(95, (int) SensorCtsHelper.get95PercentileValue(values));

        values = new HashSet<Integer>();
        for (int i = 0; i < 1000; i++) {
            values.add(i);
        }
        assertEquals(950, (int) SensorCtsHelper.get95PercentileValue(values));

        values = new HashSet<Integer>();
        for (int i = 0; i < 100; i++) {
            values.add(i * i);
        }
        assertEquals(95 * 95, (int) SensorCtsHelper.get95PercentileValue(values));
    }

    /**
     * Test {@link SensorCtsHelper#getMean(Collection)}.
     */
    public void testGetMean() {
        List<Integer> values = Arrays.asList(0, 1, 2, 3, 4);
        double mean = SensorCtsHelper.getMean(values);
        assertEquals(2.0, mean, 0.00001);

        values = Arrays.asList(1, 2, 3, 4, 5);
        mean = SensorCtsHelper.getMean(values);
        assertEquals(3.0, mean, 0.00001);

        values = Arrays.asList(0, 1, 4, 9, 16);
        mean = SensorCtsHelper.getMean(values);
        assertEquals(6.0, mean, 0.00001);
    }

    /**
     * Test {@link SensorCtsHelper#getVariance(Collection)}.
     */
    public void testGetVariance() {
        List<Integer> values = Arrays.asList(0, 1, 2, 3, 4);
        double variance = SensorCtsHelper.getVariance(values);
        assertEquals(2.5, variance, 0.00001);

        values = Arrays.asList(1, 2, 3, 4, 5);
        variance = SensorCtsHelper.getVariance(values);
        assertEquals(2.5, variance, 0.00001);

        values = Arrays.asList(0, 2, 4, 6, 8);
        variance = SensorCtsHelper.getVariance(values);
        assertEquals(10.0, variance, 0.00001);
    }

    /**
     * Test {@link SensorCtsHelper#getStandardDeviation(Collection)}.
     */
    public void testGetStandardDeviation() {
        List<Integer> values = Arrays.asList(0, 1, 2, 3, 4);
        double stddev = SensorCtsHelper.getStandardDeviation(values);
        assertEquals(Math.sqrt(2.5), stddev, 0.00001);

        values = Arrays.asList(1, 2, 3, 4, 5);
        stddev = SensorCtsHelper.getStandardDeviation(values);
        assertEquals(Math.sqrt(2.5), stddev, 0.00001);

        values = Arrays.asList(0, 2, 4, 6, 8);
        stddev = SensorCtsHelper.getStandardDeviation(values);
        assertEquals(Math.sqrt(10.0), stddev, 0.00001);
    }

    /**
     * Test {@link SensorCtsHelper#getFrequency(Number, TimeUnit)}.
     */
    public void testGetFrequency() {
        assertEquals(1.0, SensorCtsHelper.getFrequency(1, TimeUnit.SECONDS), 0.001);
        assertEquals(10.0, SensorCtsHelper.getFrequency(0.1, TimeUnit.SECONDS), 0.001);
        assertEquals(10.0, SensorCtsHelper.getFrequency(100, TimeUnit.MILLISECONDS), 0.001);
        assertEquals(1000.0, SensorCtsHelper.getFrequency(1, TimeUnit.MILLISECONDS), 0.001);
        assertEquals(10000.0, SensorCtsHelper.getFrequency(0.1, TimeUnit.MILLISECONDS), 0.001);
        assertEquals(10000.0, SensorCtsHelper.getFrequency(100, TimeUnit.MICROSECONDS), 0.001);
        assertEquals(1000000.0, SensorCtsHelper.getFrequency(1, TimeUnit.MICROSECONDS), 0.001);
        assertEquals(10000000.0, SensorCtsHelper.getFrequency(0.1, TimeUnit.MICROSECONDS), 0.001);
        assertEquals(10000000.0, SensorCtsHelper.getFrequency(100, TimeUnit.NANOSECONDS), 0.001);
        assertEquals(1000000000.0, SensorCtsHelper.getFrequency(1, TimeUnit.NANOSECONDS), 0.001);
    }

    /**
     * Test {@link SensorCtsHelper#getPeriod(Number, TimeUnit)}.
     */
    public void testGetPeriod() {
        assertEquals(1.0, SensorCtsHelper.getPeriod(1, TimeUnit.SECONDS), 0.001);
        assertEquals(0.1, SensorCtsHelper.getPeriod(10, TimeUnit.SECONDS), 0.001);
        assertEquals(100, SensorCtsHelper.getPeriod(10, TimeUnit.MILLISECONDS), 0.001);
        assertEquals(1, SensorCtsHelper.getPeriod(1000, TimeUnit.MILLISECONDS), 0.001);
        assertEquals(0.1, SensorCtsHelper.getPeriod(10000, TimeUnit.MILLISECONDS), 0.001);
        assertEquals(100, SensorCtsHelper.getPeriod(10000, TimeUnit.MICROSECONDS), 0.001);
        assertEquals(1, SensorCtsHelper.getPeriod(1000000, TimeUnit.MICROSECONDS), 0.001);
        assertEquals(0.1, SensorCtsHelper.getPeriod(10000000, TimeUnit.MICROSECONDS), 0.001);
        assertEquals(100, SensorCtsHelper.getPeriod(10000000, TimeUnit.NANOSECONDS), 0.001);
        assertEquals(1, SensorCtsHelper.getPeriod(1000000000, TimeUnit.NANOSECONDS), 0.001);
    }
}
