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

package com.android.server.telecom.tests.unit;

import android.os.UserHandle;
import com.android.internal.util.FastXmlSerializer;
import com.android.server.telecom.Log;
import com.android.server.telecom.PhoneAccountRegistrar;
import com.android.server.telecom.tests.R;

import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlSerializer;

import android.content.ComponentName;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.net.Uri;
import android.os.Parcel;
import android.telecom.PhoneAccount;
import android.telecom.PhoneAccountHandle;
import android.test.AndroidTestCase;
import android.util.Xml;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.util.Arrays;

public class PhoneAccountRegistrarTest extends AndroidTestCase {

    private static final int MAX_VERSION = Integer.MAX_VALUE;
    private static final String FILE_NAME = "phone-account-registrar-test.xml";
    private PhoneAccountRegistrar mRegistrar;

    @Override
    public void setUp() {
        mRegistrar = new PhoneAccountRegistrar(getContext(), FILE_NAME);
    }

    @Override
    public void tearDown() {
        mRegistrar = null;
        new File(getContext().getFilesDir(), FILE_NAME).delete();
    }

    public void testPhoneAccountHandle() throws Exception {
        PhoneAccountHandle input = new PhoneAccountHandle(new ComponentName("pkg0", "cls0"), "id0");
        PhoneAccountHandle result = roundTripXml(this, input,
                PhoneAccountRegistrar.sPhoneAccountHandleXml, mContext);
        assertPhoneAccountHandleEquals(input, result);

        PhoneAccountHandle inputN = new PhoneAccountHandle(new ComponentName("pkg0", "cls0"), null);
        PhoneAccountHandle resultN = roundTripXml(this, inputN,
                PhoneAccountRegistrar.sPhoneAccountHandleXml, mContext);
        Log.i(this, "inputN = %s, resultN = %s", inputN, resultN);
        assertPhoneAccountHandleEquals(inputN, resultN);
    }

    public void testPhoneAccount() throws Exception {
        PhoneAccount input = makeQuickAccountBuilder("id0", 0)
                .addSupportedUriScheme(PhoneAccount.SCHEME_TEL)
                .addSupportedUriScheme(PhoneAccount.SCHEME_VOICEMAIL)
                .build();
        PhoneAccount result = roundTripXml(this, input, PhoneAccountRegistrar.sPhoneAccountXml,
                mContext);
        assertPhoneAccountEquals(input, result);
    }

    public void testState() throws Exception {
        PhoneAccountRegistrar.State input = makeQuickState();
        PhoneAccountRegistrar.State result = roundTripXml(this, input,
                PhoneAccountRegistrar.sStateXml,
                mContext);
        assertStateEquals(input, result);
    }

    public void testAccounts() throws Exception {
        int i = 0;
        mRegistrar.registerPhoneAccount(makeQuickAccountBuilder("id" + i, i++)
                .setCapabilities(PhoneAccount.CAPABILITY_CONNECTION_MANAGER
                        | PhoneAccount.CAPABILITY_CALL_PROVIDER)
                .build());
        mRegistrar.registerPhoneAccount(makeQuickAccountBuilder("id" + i, i++)
                .setCapabilities(PhoneAccount.CAPABILITY_CONNECTION_MANAGER
                        | PhoneAccount.CAPABILITY_CALL_PROVIDER)
                .build());
        mRegistrar.registerPhoneAccount(makeQuickAccountBuilder("id" + i, i++)
                .setCapabilities(PhoneAccount.CAPABILITY_CONNECTION_MANAGER
                        | PhoneAccount.CAPABILITY_CALL_PROVIDER)
                .build());
        mRegistrar.registerPhoneAccount(makeQuickAccountBuilder("id" + i, i++)
                .setCapabilities(PhoneAccount.CAPABILITY_CONNECTION_MANAGER)
                .build());

        assertEquals(4, mRegistrar.getAllPhoneAccountHandles().size());
        assertEquals(3, mRegistrar.getCallCapablePhoneAccounts().size());
        assertEquals(null, mRegistrar.getSimCallManager());
        assertEquals(null, mRegistrar.getDefaultOutgoingPhoneAccount(PhoneAccount.SCHEME_TEL));
    }

    public void testSimCallManager() throws Exception {
        PhoneAccountHandle simManager = makeQuickAccountHandle("sim_mgr");
        PhoneAccount simManagerAccount = new PhoneAccount.Builder(simManager, "sim_mgr")
                .setCapabilities(PhoneAccount.CAPABILITY_CALL_PROVIDER
                        | PhoneAccount.CAPABILITY_CONNECTION_MANAGER)
                .build();
        mRegistrar.registerPhoneAccount(simManagerAccount);
        assertNull(mRegistrar.getSimCallManager());

        // Test the basic case
        mRegistrar.setSimCallManager(simManager);
        assertEquals(simManager, mRegistrar.getSimCallManager());

        // Make sure clearing it works, too
        mRegistrar.unregisterPhoneAccount(simManager);
        assertNull(mRegistrar.getSimCallManager());

        // Re-registering it makes the setting come back
        mRegistrar.registerPhoneAccount(simManagerAccount);
        assertEquals(simManager, mRegistrar.getSimCallManager());

        // Make sure that the manager has CAPABILITY_CONNECTION_MANAGER
        PhoneAccountHandle simManagerImposter = makeQuickAccountHandle("imposter");
        PhoneAccount simManagerImposterAccount =
                new PhoneAccount.Builder(simManagerImposter, "imposter")
                .setCapabilities(PhoneAccount.CAPABILITY_CALL_PROVIDER)
                .build();
        mRegistrar.registerPhoneAccount(simManagerImposterAccount);

        mRegistrar.setSimCallManager(null);
        assertNull(mRegistrar.getSimCallManager());
        mRegistrar.setSimCallManager(simManagerImposter);
        assertNull(mRegistrar.getSimCallManager());
    }

    public void testDefaultOutgoing() {
        // By default, there is no default outgoing account (nothing has been registered)
        assertNull(mRegistrar.getDefaultOutgoingPhoneAccount(PhoneAccount.SCHEME_TEL));

        // Register one tel: account
        PhoneAccountHandle telAccount = makeQuickAccountHandle("tel_acct");
        mRegistrar.registerPhoneAccount(new PhoneAccount.Builder(telAccount, "tel_acct")
                .setCapabilities(PhoneAccount.CAPABILITY_CALL_PROVIDER)
                .addSupportedUriScheme(PhoneAccount.SCHEME_TEL)
                .build());
        PhoneAccountHandle defaultAccount =
                mRegistrar.getDefaultOutgoingPhoneAccount(PhoneAccount.SCHEME_TEL);
        assertEquals(telAccount, defaultAccount);

        // Add a SIP account, make sure tel: doesn't change
        PhoneAccountHandle sipAccount = makeQuickAccountHandle("sip_acct");
        mRegistrar.registerPhoneAccount(new PhoneAccount.Builder(sipAccount, "sip_acct")
                .setCapabilities(PhoneAccount.CAPABILITY_CALL_PROVIDER)
                .addSupportedUriScheme(PhoneAccount.SCHEME_SIP)
                .build());
        defaultAccount = mRegistrar.getDefaultOutgoingPhoneAccount(PhoneAccount.SCHEME_SIP);
        assertEquals(sipAccount, defaultAccount);
        defaultAccount = mRegistrar.getDefaultOutgoingPhoneAccount(PhoneAccount.SCHEME_TEL);
        assertEquals(telAccount, defaultAccount);

        // Add a connection manager, make sure tel: doesn't change
        PhoneAccountHandle connectionManager = makeQuickAccountHandle("mgr_acct");
        mRegistrar.registerPhoneAccount(new PhoneAccount.Builder(connectionManager, "mgr_acct")
                .setCapabilities(PhoneAccount.CAPABILITY_CONNECTION_MANAGER)
                .addSupportedUriScheme(PhoneAccount.SCHEME_TEL)
                .build());
        defaultAccount = mRegistrar.getDefaultOutgoingPhoneAccount(PhoneAccount.SCHEME_TEL);
        assertEquals(telAccount, defaultAccount);

        // Unregister the tel: account, make sure there is no tel: default now.
        mRegistrar.unregisterPhoneAccount(telAccount);
        assertNull(mRegistrar.getDefaultOutgoingPhoneAccount(PhoneAccount.SCHEME_TEL));
    }

    public void testPhoneAccountParceling() throws Exception {
        PhoneAccountHandle handle = makeQuickAccountHandle("foo");
        roundTripPhoneAccount(new PhoneAccount.Builder(handle, null).build());
        roundTripPhoneAccount(new PhoneAccount.Builder(handle, "foo").build());
        roundTripPhoneAccount(
                new PhoneAccount.Builder(handle, "foo")
                        .setAddress(Uri.parse("tel:123456"))
                        .setCapabilities(23)
                        .setHighlightColor(0xf0f0f0)
                        .setIcon(
                                "com.android.server.telecom.tests",
                                R.drawable.stat_sys_phone_call,
                                0xfefefe)
                        .setShortDescription("short description")
                        .setSubscriptionAddress(Uri.parse("tel:2345678"))
                        .setSupportedUriSchemes(Arrays.asList("tel", "sip"))
                        .build());
        roundTripPhoneAccount(
                new PhoneAccount.Builder(handle, "foo")
                        .setAddress(Uri.parse("tel:123456"))
                        .setCapabilities(23)
                        .setHighlightColor(0xf0f0f0)
                        .setIcon(
                                BitmapFactory.decodeResource(
                                        getContext().getResources(),
                                        R.drawable.stat_sys_phone_call))
                        .setShortDescription("short description")
                        .setSubscriptionAddress(Uri.parse("tel:2345678"))
                        .setSupportedUriSchemes(Arrays.asList("tel", "sip"))
                        .build());
    }

    private static PhoneAccountHandle makeQuickAccountHandle(String id) {
        return new PhoneAccountHandle(
                new ComponentName(
                        "com.android.server.telecom.tests",
                        "com.android.server.telecom.tests.MockConnectionService"
                ),
                id,
                new UserHandle(5));
    }

    private PhoneAccount.Builder makeQuickAccountBuilder(String id, int idx) {
        return new PhoneAccount.Builder(
                makeQuickAccountHandle(id),
                "label" + idx);
    }

    private PhoneAccount makeQuickAccount(String id, int idx) {
        return makeQuickAccountBuilder(id, idx)
                .setAddress(Uri.parse("http://foo.com/" + idx))
                .setSubscriptionAddress(Uri.parse("tel:555-000" + idx))
                .setCapabilities(idx)
                .setIcon("com.android.server.telecom.tests", R.drawable.stat_sys_phone_call)
                .setShortDescription("desc" + idx)
                .build();
    }

    private static void roundTripPhoneAccount(PhoneAccount original) throws Exception {
        PhoneAccount copy = null;

        {
            Parcel parcel = Parcel.obtain();
            parcel.writeParcelable(original, 0);
            parcel.setDataPosition(0);
            copy = parcel.readParcelable(PhoneAccountRegistrarTest.class.getClassLoader());
            parcel.recycle();
        }

        assertPhoneAccountEquals(original, copy);
    }

    private static <T> T roundTripXml(
            Object self,
            T input,
            PhoneAccountRegistrar.XmlSerialization<T> xml,
            Context context)
            throws Exception {
        Log.d(self, "Input = %s", input);

        byte[] data;
        {
            XmlSerializer serializer = new FastXmlSerializer();
            ByteArrayOutputStream baos = new ByteArrayOutputStream();
            serializer.setOutput(new BufferedOutputStream(baos), "utf-8");
            xml.writeToXml(input, serializer, context);
            serializer.flush();
            data = baos.toByteArray();
        }

        Log.d(self, "====== XML data ======\n%s", new String(data));

        T result = null;
        {
            XmlPullParser parser = Xml.newPullParser();
            parser.setInput(new BufferedInputStream(new ByteArrayInputStream(data)), null);
            parser.nextTag();
            result = xml.readFromXml(parser, MAX_VERSION, context);
        }

        Log.d(self, "result = " + result);

        return result;
    }

    private static void assertPhoneAccountHandleEquals(PhoneAccountHandle a, PhoneAccountHandle b) {
        if (a != b) {
            assertEquals(
                    a.getComponentName().getPackageName(),
                    b.getComponentName().getPackageName());
            assertEquals(
                    a.getComponentName().getClassName(),
                    b.getComponentName().getClassName());
            assertEquals(a.getId(), b.getId());
        }
    }

    private static void assertPhoneAccountEquals(PhoneAccount a, PhoneAccount b) {
        if (a != b) {
            assertPhoneAccountHandleEquals(a.getAccountHandle(), b.getAccountHandle());
            assertEquals(a.getAddress(), b.getAddress());
            assertEquals(a.getSubscriptionAddress(), b.getSubscriptionAddress());
            assertEquals(a.getCapabilities(), b.getCapabilities());
            assertEquals(a.getIconResId(), b.getIconResId());
            assertEquals(a.getIconPackageName(), b.getIconPackageName());
            assertBitmapEquals(a.getIconBitmap(), b.getIconBitmap());
            assertEquals(a.getIconTint(), b.getIconTint());
            assertEquals(a.getHighlightColor(), b.getHighlightColor());
            assertEquals(a.getLabel(), b.getLabel());
            assertEquals(a.getShortDescription(), b.getShortDescription());
            assertEquals(a.getSupportedUriSchemes(), b.getSupportedUriSchemes());
        }
    }

    private static void assertBitmapEquals(Bitmap a, Bitmap b) {
        if (a == null || b == null) {
            assertEquals(null, a);
            assertEquals(null, b);
        } else {
            assertEquals(a.getWidth(), b.getWidth());
            assertEquals(a.getHeight(), b.getHeight());
            for (int x = 0; x < a.getWidth(); x++) {
                for (int y = 0; y < a.getHeight(); y++) {
                    assertEquals(a.getPixel(x, y), b.getPixel(x, y));
                }
            }
        }
    }

    private static void assertStateEquals(
            PhoneAccountRegistrar.State a, PhoneAccountRegistrar.State b) {
        assertPhoneAccountHandleEquals(a.defaultOutgoing, b.defaultOutgoing);
        assertPhoneAccountHandleEquals(a.simCallManager, b.simCallManager);
        assertEquals(a.accounts.size(), b.accounts.size());
        for (int i = 0; i < a.accounts.size(); i++) {
            assertPhoneAccountEquals(a.accounts.get(i), b.accounts.get(i));
        }
    }

    private PhoneAccountRegistrar.State makeQuickState() {
        PhoneAccountRegistrar.State s = new PhoneAccountRegistrar.State();
        s.accounts.add(makeQuickAccount("id0", 0));
        s.accounts.add(makeQuickAccount("id1", 1));
        s.accounts.add(makeQuickAccount("id2", 2));
        s.defaultOutgoing = new PhoneAccountHandle(new ComponentName("pkg0", "cls0"), "id0");
        s.simCallManager = new PhoneAccountHandle(new ComponentName("pkg0", "cls0"), "id1");
        return s;
    }
}