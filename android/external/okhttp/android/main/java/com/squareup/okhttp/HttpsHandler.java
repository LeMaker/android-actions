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

import java.net.Proxy;
import java.util.Arrays;
import java.util.List;

import javax.net.ssl.DefaultHostnameVerifier;
import javax.net.ssl.HostnameVerifier;
import javax.net.ssl.HttpsURLConnection;

public final class HttpsHandler extends HttpHandler {
    private static final List<Protocol> ENABLED_PROTOCOLS = Arrays.asList(Protocol.HTTP_11);
    private final ConfigAwareConnectionPool configAwareConnectionPool =
            ConfigAwareConnectionPool.getInstance();

    @Override protected int getDefaultPort() {
        return 443;
    }

    @Override
    protected OkHttpClient newOkHttpClient(Proxy proxy) {
        OkHttpClient okHttpClient = createHttpsOkHttpClient(proxy);
        okHttpClient.setConnectionPool(configAwareConnectionPool.get());
        return okHttpClient;
    }

    /**
     * Creates an OkHttpClient suitable for creating {@link HttpsURLConnection} instances on
     * Android.
     */
    public static OkHttpClient createHttpsOkHttpClient(Proxy proxy) {
        // The HTTPS OkHttpClient is an HTTP OkHttpClient with extra configuration.
        OkHttpClient client = HttpHandler.createHttpOkHttpClient(proxy);

        client.setProtocols(ENABLED_PROTOCOLS);

        HostnameVerifier verifier = HttpsURLConnection.getDefaultHostnameVerifier();
        // Assume that the internal verifier is better than the
        // default verifier.
        if (!(verifier instanceof DefaultHostnameVerifier)) {
            client.setHostnameVerifier(verifier);
        }
        // OkHttp does not automatically honor the system-wide SSLSocketFactory set with
        // HttpsURLConnection.setDefaultSSLSocketFactory().
        // See https://github.com/square/okhttp/issues/184 for details.
        client.setSslSocketFactory(HttpsURLConnection.getDefaultSSLSocketFactory());

        return client;
    }
}
