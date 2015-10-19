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
package android.jobscheduler.cts;

import android.annotation.TargetApi;
import android.app.job.JobInfo;

/**
 * Schedules jobs with various timing constraints and ensures that they are executed when
 * appropriate.
 */
@TargetApi(21)
public class TimingConstraintsTest extends ConstraintTest {
    private static final int TIMING_JOB_ID = TimingConstraintsTest.class.hashCode() + 0;
    private static final int CANCEL_JOB_ID = TimingConstraintsTest.class.hashCode() + 1;

    public void testScheduleOnce() throws Exception {
        JobInfo oneTimeJob = new JobInfo.Builder(TIMING_JOB_ID, kJobServiceComponent)
                        .setOverrideDeadline(5000)  // 5 secs
                        .build();

        kTestEnvironment.setExpectedExecutions(1);
        mJobScheduler.schedule(oneTimeJob);
        final boolean executed = kTestEnvironment.awaitExecution();
        assertTrue("Timed out waiting for override deadline.", executed);
    }

    public void testSchedulePeriodic() throws Exception {
        JobInfo periodicJob =
                new JobInfo.Builder(TIMING_JOB_ID, kJobServiceComponent)
                        .setPeriodic(5000L)  // 5 second period.
                        .build();

        kTestEnvironment.setExpectedExecutions(3);
        mJobScheduler.schedule(periodicJob);
        final boolean countedDown = kTestEnvironment.awaitExecution();
        assertTrue("Timed out waiting for periodic jobs to execute", countedDown);
    }

    public void testCancel() throws Exception {
        JobInfo cancelJob = new JobInfo.Builder(CANCEL_JOB_ID, kJobServiceComponent)
                .setMinimumLatency(5000L) // make sure it doesn't actually run immediately
                .setOverrideDeadline(7000L)
                .build();

        kTestEnvironment.setExpectedExecutions(0);
        mJobScheduler.schedule(cancelJob);
        // Now cancel it.
        mJobScheduler.cancel(CANCEL_JOB_ID);
        assertTrue("Cancel failed: job executed when it shouldn't have.",
                kTestEnvironment.awaitTimeout());
    }
}