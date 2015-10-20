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

package com.squareup.okhttp.internal;

import com.android.org.conscrypt.OpenSSLSocketImpl;
import com.squareup.okhttp.Protocol;

import org.junit.Test;

import java.io.IOException;
import java.util.Arrays;
import java.util.List;
import javax.net.ssl.HandshakeCompletedListener;
import javax.net.ssl.SSLSession;
import javax.net.ssl.SSLSocket;

import okio.ByteString;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

/**
 * Tests for {@link Platform}.
 */
public class PlatformTest {

  @Test
  public void enableTlsExtensionOptionalMethods() throws Exception {
    Platform platform = new Platform();

    // Expect no error
    TestSSLSocketImpl arbitrarySocketImpl = new TestSSLSocketImpl();
    platform.enableTlsExtensions(arbitrarySocketImpl, "host");

    FullOpenSSLSocketImpl openSslSocket = new FullOpenSSLSocketImpl();
    platform.enableTlsExtensions(openSslSocket, "host");
    assertTrue(openSslSocket.useSessionTickets);
    assertEquals("host", openSslSocket.hostname);
  }

  @Test
  public void getNpnSelectedProtocol() throws Exception {
    Platform platform = new Platform();
    byte[] npnBytes = "npn".getBytes();
    byte[] alpnBytes = "alpn".getBytes();

    TestSSLSocketImpl arbitrarySocketImpl = new TestSSLSocketImpl();
    assertNull(platform.getNpnSelectedProtocol(arbitrarySocketImpl));

    NpnOnlySSLSocketImpl npnOnlySSLSocketImpl = new NpnOnlySSLSocketImpl();
    npnOnlySSLSocketImpl.npnProtocols = npnBytes;
    assertEquals(ByteString.of(npnBytes), platform.getNpnSelectedProtocol(npnOnlySSLSocketImpl));

    FullOpenSSLSocketImpl openSslSocket = new FullOpenSSLSocketImpl();
    openSslSocket.npnProtocols = npnBytes;
    openSslSocket.alpnProtocols = alpnBytes;
    assertEquals(ByteString.of(alpnBytes), platform.getNpnSelectedProtocol(openSslSocket));
  }

  @Test
  public void setNpnProtocols() throws Exception {
    Platform platform = new Platform();
    List<Protocol> protocols = Arrays.asList(Protocol.SPDY_3);

    // No error
    TestSSLSocketImpl arbitrarySocketImpl = new TestSSLSocketImpl();
    platform.setNpnProtocols(arbitrarySocketImpl, protocols);

    NpnOnlySSLSocketImpl npnOnlySSLSocketImpl = new NpnOnlySSLSocketImpl();
    platform.setNpnProtocols(npnOnlySSLSocketImpl, protocols);
    assertNotNull(npnOnlySSLSocketImpl.npnProtocols);

    FullOpenSSLSocketImpl openSslSocket = new FullOpenSSLSocketImpl();
    platform.setNpnProtocols(openSslSocket, protocols);
    assertNotNull(openSslSocket.npnProtocols);
    assertNotNull(openSslSocket.alpnProtocols);
  }

  private static class FullOpenSSLSocketImpl extends OpenSSLSocketImpl {
    private boolean useSessionTickets;
    private String hostname;
    private byte[] npnProtocols;
    private byte[] alpnProtocols;

    public FullOpenSSLSocketImpl() throws IOException {
      super(null);
    }

    @Override
    public void setUseSessionTickets(boolean useSessionTickets) {
      this.useSessionTickets = useSessionTickets;
    }

    @Override
    public void setHostname(String hostname) {
      this.hostname = hostname;
    }

    @Override
    public void setNpnProtocols(byte[] npnProtocols) {
      this.npnProtocols = npnProtocols;
    }

    @Override
    public byte[] getNpnSelectedProtocol() {
      return npnProtocols;
    }

    @Override
    public void setAlpnProtocols(byte[] alpnProtocols) {
      this.alpnProtocols = alpnProtocols;
    }

    @Override
    public byte[] getAlpnSelectedProtocol() {
      return alpnProtocols;
    }
  }

  // Legacy case
  private static class NpnOnlySSLSocketImpl extends TestSSLSocketImpl {

    private byte[] npnProtocols;

    public void setNpnProtocols(byte[] npnProtocols) {
      this.npnProtocols = npnProtocols;
    }

    public byte[] getNpnSelectedProtocol() {
      return npnProtocols;
    }
  }

  private static class TestSSLSocketImpl extends SSLSocket {

    @Override
    public String[] getSupportedCipherSuites() {
      return new String[0];
    }

    @Override
    public String[] getEnabledCipherSuites() {
      return new String[0];
    }

    @Override
    public void setEnabledCipherSuites(String[] suites) {
    }

    @Override
    public String[] getSupportedProtocols() {
      return new String[0];
    }

    @Override
    public String[] getEnabledProtocols() {
      return new String[0];
    }

    @Override
    public void setEnabledProtocols(String[] protocols) {
    }

    @Override
    public SSLSession getSession() {
      return null;
    }

    @Override
    public void addHandshakeCompletedListener(HandshakeCompletedListener listener) {
    }

    @Override
    public void removeHandshakeCompletedListener(HandshakeCompletedListener listener) {
    }

    @Override
    public void startHandshake() throws IOException {
    }

    @Override
    public void setUseClientMode(boolean mode) {
    }

    @Override
    public boolean getUseClientMode() {
      return false;
    }

    @Override
    public void setNeedClientAuth(boolean need) {
    }

    @Override
    public void setWantClientAuth(boolean want) {
    }

    @Override
    public boolean getNeedClientAuth() {
      return false;
    }

    @Override
    public boolean getWantClientAuth() {
      return false;
    }

    @Override
    public void setEnableSessionCreation(boolean flag) {
    }

    @Override
    public boolean getEnableSessionCreation() {
      return false;
    }
  }
}
