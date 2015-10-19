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
package com.android.cts.deviceowner;

import static com.android.cts.deviceowner.FakeKeys.FAKE_RSA_1;

import android.app.admin.DevicePolicyManager;

import java.io.ByteArrayInputStream;
import java.security.cert.CertificateException;
import java.security.cert.CertificateFactory;
import java.security.cert.Certificate;
import java.security.KeyFactory;
import java.security.NoSuchAlgorithmException;
import java.security.PrivateKey;
import java.security.spec.InvalidKeySpecException;
import java.security.spec.PKCS8EncodedKeySpec;

public class KeyManagementTest extends BaseDeviceOwnerTest {

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        assertTrue(mDevicePolicyManager.resetPassword("test", 0));
    }

    @Override
    protected void tearDown() throws Exception {
        mDevicePolicyManager.setPasswordQuality(getWho(),
                DevicePolicyManager.PASSWORD_QUALITY_UNSPECIFIED);
        mDevicePolicyManager.setPasswordMinimumLength(getWho(), 0);
        assertTrue(mDevicePolicyManager.resetPassword("", 0));
        super.tearDown();
    }

    public void testCanInstallValidRsaKeypair()
            throws CertificateException, NoSuchAlgorithmException, InvalidKeySpecException {
        final String alias = "com.android.test.valid-rsa-key-1";
        final PrivateKey privKey = getPrivateKey(FAKE_RSA_1.privateKey , "RSA");
        final Certificate cert = getCertificate(FAKE_RSA_1.caCertificate);
        assertTrue(mDevicePolicyManager.installKeyPair(getWho(), privKey, cert, alias));
    }

    public void testNullKeyParamsFailGracefully()
            throws CertificateException, NoSuchAlgorithmException, InvalidKeySpecException {
        final String alias = "com.android.test.null-key-1";
        final PrivateKey privKey = getPrivateKey(FAKE_RSA_1.privateKey, "RSA");
        final Certificate cert = getCertificate(FAKE_RSA_1.caCertificate);
        try {
            assertFalse(mDevicePolicyManager.installKeyPair(getWho(), null, cert, alias));
        } catch (NullPointerException accept) {
            // Accept either false return value or NPE
        }
        try {
            assertFalse(mDevicePolicyManager.installKeyPair(getWho(), privKey, null, alias));
        } catch (NullPointerException accept) {
            // Accept either false return value or NPE
        }
    }

    public void testNullAdminComponentIsDenied()
            throws CertificateException, NoSuchAlgorithmException, InvalidKeySpecException {
        final String alias = "com.android.test.null-admin-1";
        final PrivateKey privKey = getPrivateKey(FAKE_RSA_1.privateKey, "RSA");
        final Certificate cert = getCertificate(FAKE_RSA_1.caCertificate);
        try {
            assertFalse(mDevicePolicyManager.installKeyPair(null, privKey, cert, alias));
            fail("Exception should have been thrown for null ComponentName");
        } catch (SecurityException | NullPointerException expected) {
        }
    }

    PrivateKey getPrivateKey(final byte[] key, String type)
            throws NoSuchAlgorithmException, InvalidKeySpecException {
        return KeyFactory.getInstance(type).generatePrivate(
                new PKCS8EncodedKeySpec(key));
    }

    Certificate getCertificate(byte[] cert) throws CertificateException {
        return CertificateFactory.getInstance("X.509").generateCertificate(
                new ByteArrayInputStream(cert));
    }

}
