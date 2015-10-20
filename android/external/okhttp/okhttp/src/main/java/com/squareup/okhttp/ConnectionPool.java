/*
 *  Licensed to the Apache Software Foundation (ASF) under one or more
 *  contributor license agreements.  See the NOTICE file distributed with
 *  this work for additional information regarding copyright ownership.
 *  The ASF licenses this file to You under the Apache License, Version 2.0
 *  (the "License"); you may not use this file except in compliance with
 *  the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
package com.squareup.okhttp;

import com.squareup.okhttp.internal.Platform;
import com.squareup.okhttp.internal.Util;
import java.net.SocketException;
import java.util.ArrayList;
import java.util.LinkedList;
import java.util.List;
import java.util.ListIterator;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.ThreadPoolExecutor;
import java.util.concurrent.TimeUnit;

/**
 * Manages reuse of HTTP and SPDY connections for reduced network latency. HTTP
 * requests that share the same {@link com.squareup.okhttp.Address} may share a
 * {@link com.squareup.okhttp.Connection}. This class implements the policy of
 * which connections to keep open for future use.
 *
 * <p>The {@link #getDefault() system-wide default} uses system properties for
 * tuning parameters:
 * <ul>
 *     <li>{@code http.keepAlive} true if HTTP and SPDY connections should be
 *         pooled at all. Default is true.
 *     <li>{@code http.maxConnections} maximum number of idle connections to
 *         each to keep in the pool. Default is 5.
 *     <li>{@code http.keepAliveDuration} Time in milliseconds to keep the
 *         connection alive in the pool before closing it. Default is 5 minutes.
 *         This property isn't used by {@code HttpURLConnection}.
 * </ul>
 *
 * <p>The default instance <i>doesn't</i> adjust its configuration as system
 * properties are changed. This assumes that the applications that set these
 * parameters do so before making HTTP connections, and that this class is
 * initialized lazily.
 */
public class ConnectionPool {
  private static final int MAX_CONNECTIONS_TO_CLEANUP = 2;
  private static final long DEFAULT_KEEP_ALIVE_DURATION_MS = 5 * 60 * 1000; // 5 min

  private static final ConnectionPool systemDefault;

  static {
    String keepAlive = System.getProperty("http.keepAlive");
    String keepAliveDuration = System.getProperty("http.keepAliveDuration");
    String maxIdleConnections = System.getProperty("http.maxConnections");
    long keepAliveDurationMs = keepAliveDuration != null ? Long.parseLong(keepAliveDuration)
        : DEFAULT_KEEP_ALIVE_DURATION_MS;
    if (keepAlive != null && !Boolean.parseBoolean(keepAlive)) {
      systemDefault = new ConnectionPool(0, keepAliveDurationMs);
    } else if (maxIdleConnections != null) {
      systemDefault = new ConnectionPool(Integer.parseInt(maxIdleConnections), keepAliveDurationMs);
    } else {
      systemDefault = new ConnectionPool(5, keepAliveDurationMs);
    }
  }

  /** The maximum number of idle connections for each address. */
  private final int maxIdleConnections;
  private final long keepAliveDurationNs;

  private final LinkedList<Connection> connections = new LinkedList<Connection>();

  /** We use a single background thread to cleanup expired connections. */
  private final ExecutorService executorService = new ThreadPoolExecutor(0, 1,
      60L, TimeUnit.SECONDS, new LinkedBlockingQueue<Runnable>(),
      Util.threadFactory("OkHttp ConnectionPool", true));

  private enum CleanMode {
    /**
     * Connection clean up is driven by usage of the pool. Each usage of the pool can schedule a
     * clean up. A pool left in this state and unused may contain idle connections indefinitely.
     */
    NORMAL,
    /**
     * Entered when a pool has been orphaned and is not expected to receive more usage, except for
     * references held by existing connections. See {@link #enterDrainMode()}.
     * A thread runs periodically to close idle connections in the pool until the pool is empty and
     * then the state moves to {@link #DRAINED}.
     */
    DRAINING,
    /**
     * The pool is empty and no clean-up is taking place. Connections may still be added to the
     * pool due to latent references to the pool, in which case the pool re-enters
     * {@link #DRAINING}. If the pool is DRAINED and no longer referenced it is safe to be garbage
     * collected.
     */
    DRAINED
  }
  /** The current mode for cleaning connections in the pool */
  private CleanMode cleanMode = CleanMode.NORMAL;

  // A scheduled drainModeRunnable keeps a reference to the enclosing ConnectionPool,
  // preventing the ConnectionPool from being garbage collected before all held connections have
  // been explicitly closed. If this was not the case any open connections in the pool would trigger
  // StrictMode violations in Android when they were garbage collected. http://b/18369687
  private final Runnable drainModeRunnable = new Runnable() {
    @Override public void run() {
      // Close any connections we can.
      connectionsCleanupRunnable.run();

      synchronized (ConnectionPool.this) {
        // See whether we should continue checking the connection pool.
        if (connections.size() > 0) {
          // Pause to avoid checking too regularly, which would drain the battery on mobile
          // devices. The wait() surrenders the pool monitor and will not block other calls.
          try {
            // Use the keep alive duration as a rough indicator of a good check interval.
            long keepAliveDurationMillis = keepAliveDurationNs / (1000 * 1000);
            ConnectionPool.this.wait(keepAliveDurationMillis);
          } catch (InterruptedException e) {
            // Ignored.
          }

          // Reschedule "this" to perform another clean-up.
          executorService.execute(this);
        } else {
          cleanMode = CleanMode.DRAINED;
        }
      }
    }
  };

  private final Runnable connectionsCleanupRunnable = new Runnable() {
    @Override public void run() {
      List<Connection> expiredConnections = new ArrayList<Connection>(MAX_CONNECTIONS_TO_CLEANUP);
      int idleConnectionCount = 0;
      synchronized (ConnectionPool.this) {
        for (ListIterator<Connection> i = connections.listIterator(connections.size());
            i.hasPrevious(); ) {
          Connection connection = i.previous();
          if (!connection.isAlive() || connection.isExpired(keepAliveDurationNs)) {
            i.remove();
            expiredConnections.add(connection);
            if (expiredConnections.size() == MAX_CONNECTIONS_TO_CLEANUP) break;
          } else if (connection.isIdle()) {
            idleConnectionCount++;
          }
        }

        for (ListIterator<Connection> i = connections.listIterator(connections.size());
            i.hasPrevious() && idleConnectionCount > maxIdleConnections; ) {
          Connection connection = i.previous();
          if (connection.isIdle()) {
            expiredConnections.add(connection);
            i.remove();
            --idleConnectionCount;
          }
        }
      }
      for (Connection expiredConnection : expiredConnections) {
        Util.closeQuietly(expiredConnection);
      }
    }
  };

  public ConnectionPool(int maxIdleConnections, long keepAliveDurationMs) {
    this.maxIdleConnections = maxIdleConnections;
    this.keepAliveDurationNs = keepAliveDurationMs * 1000 * 1000;
  }

  /**
   * Returns a snapshot of the connections in this pool, ordered from newest to
   * oldest. Waits for the cleanup callable to run if it is currently scheduled.
   * Only use in tests.
   */
  List<Connection> getConnections() {
    waitForCleanupCallableToRun();
    synchronized (this) {
      return new ArrayList<Connection>(connections);
    }
  }

  /**
   * Blocks until the executor service has processed all currently enqueued
   * jobs.
   */
  private void waitForCleanupCallableToRun() {
    try {
      executorService.submit(new Runnable() {
        @Override public void run() {
        }
      }).get();
    } catch (Exception e) {
      throw new AssertionError();
    }
  }

  public static ConnectionPool getDefault() {
    return systemDefault;
  }

  /** Returns total number of connections in the pool. */
  public synchronized int getConnectionCount() {
    return connections.size();
  }

  /** Returns total number of spdy connections in the pool. */
  public synchronized int getSpdyConnectionCount() {
    int total = 0;
    for (Connection connection : connections) {
      if (connection.isSpdy()) total++;
    }
    return total;
  }

  /** Returns total number of http connections in the pool. */
  public synchronized int getHttpConnectionCount() {
    int total = 0;
    for (Connection connection : connections) {
      if (!connection.isSpdy()) total++;
    }
    return total;
  }

  /** Returns a recycled connection to {@code address}, or null if no such connection exists. */
  public synchronized Connection get(Address address) {
    Connection foundConnection = null;
    for (ListIterator<Connection> i = connections.listIterator(connections.size());
        i.hasPrevious(); ) {
      Connection connection = i.previous();
      if (!connection.getRoute().getAddress().equals(address)
          || !connection.isAlive()
          || System.nanoTime() - connection.getIdleStartTimeNs() >= keepAliveDurationNs) {
        continue;
      }
      i.remove();
      if (!connection.isSpdy()) {
        try {
          Platform.get().tagSocket(connection.getSocket());
        } catch (SocketException e) {
          Util.closeQuietly(connection);
          // When unable to tag, skip recycling and close
          Platform.get().logW("Unable to tagSocket(): " + e);
          continue;
        }
      }
      foundConnection = connection;
      break;
    }

    if (foundConnection != null && foundConnection.isSpdy()) {
      connections.addFirst(foundConnection); // Add it back after iteration.
    }

    scheduleCleanupAsRequired();
    return foundConnection;
  }

  /**
   * Gives {@code connection} to the pool. The pool may store the connection,
   * or close it, as its policy describes.
   *
   * <p>It is an error to use {@code connection} after calling this method.
   */
  public void recycle(Connection connection) {
    if (connection.isSpdy()) {
      return;
    }

    if (!connection.clearOwner()) {
      return; // This connection isn't eligible for reuse.
    }

    if (!connection.isAlive()) {
      Util.closeQuietly(connection);
      return;
    }

    try {
      Platform.get().untagSocket(connection.getSocket());
    } catch (SocketException e) {
      // When unable to remove tagging, skip recycling and close.
      Platform.get().logW("Unable to untagSocket(): " + e);
      Util.closeQuietly(connection);
      return;
    }

    synchronized (this) {
      connections.addFirst(connection);
      connection.incrementRecycleCount();
      connection.resetIdleStartTime();
      scheduleCleanupAsRequired();
    }

  }

  /**
   * Shares the SPDY connection with the pool. Callers to this method may
   * continue to use {@code connection}.
   */
  public void share(Connection connection) {
    if (!connection.isSpdy()) throw new IllegalArgumentException();
    if (connection.isAlive()) {
      synchronized (this) {
        connections.addFirst(connection);
        scheduleCleanupAsRequired();
      }
    }
  }

  /** Close and remove all connections in the pool. */
  public void evictAll() {
    List<Connection> connections;
    synchronized (this) {
      connections = new ArrayList<Connection>(this.connections);
      this.connections.clear();
    }

    for (int i = 0, size = connections.size(); i < size; i++) {
      Util.closeQuietly(connections.get(i));
    }
  }

  /**
   * A less abrupt way of draining the pool than {@link #evictAll()}. For use when the pool
   * may still be referenced by active shared connections which cannot safely be closed.
   */
  public void enterDrainMode() {
    synchronized(this) {
      cleanMode = CleanMode.DRAINING;
      executorService.execute(drainModeRunnable);
    }
  }

  public boolean isDrained() {
    synchronized(this) {
      return cleanMode == CleanMode.DRAINED;
    }
  }

  // Callers must synchronize on "this".
  private void scheduleCleanupAsRequired() {
    switch (cleanMode) {
      case NORMAL:
        executorService.execute(connectionsCleanupRunnable);
        break;
      case DRAINING:
        // Do nothing -drainModeRunnable is already scheduled, and will reschedules itself as
        // needed.
        break;
      case DRAINED:
        // A new connection has potentially been offered up to a drained pool. Restart the drain.
        cleanMode = CleanMode.DRAINING;
        executorService.execute(drainModeRunnable);
        break;
    }
  }
}
