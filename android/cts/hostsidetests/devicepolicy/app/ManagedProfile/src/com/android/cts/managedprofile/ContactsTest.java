/*
 * Copyright (C) 2015 The Android Open Source Project
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

package com.android.cts.managedprofile;

import android.annotation.TargetApi;
import android.app.admin.DevicePolicyManager;
import android.content.ContentProviderOperation;
import android.content.ContentResolver;
import android.content.Context;
import android.content.OperationApplicationException;
import android.content.res.Resources.NotFoundException;
import android.database.Cursor;
import android.net.Uri;
import android.os.Build;
import android.os.RemoteException;
import android.provider.ContactsContract;
import android.provider.ContactsContract.CommonDataKinds.Phone;
import android.provider.ContactsContract.CommonDataKinds.Photo;
import android.provider.ContactsContract.PhoneLookup;
import android.provider.ContactsContract.RawContacts;
import android.test.AndroidTestCase;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;

@TargetApi(Build.VERSION_CODES.LOLLIPOP)
public class ContactsTest extends AndroidTestCase {

    private static final String TEST_ACCOUNT_NAME = "CTS";
    private static final String TEST_ACCOUNT_TYPE = "com.android.cts.test";
    private static final String PRIMARY_CONTACT_DISPLAY_NAME = "Primary";
    private static final String PRIMARY_CONTACT_PHONE = "00000001";
    private static final String MANAGED_CONTACT_DISPLAY_NAME = "Managed";
    private static final String MANAGED_CONTACT_PHONE = "6891999";

    private DevicePolicyManager mDevicePolicyManager;
    private ContentResolver mResolver;

    private static class ContactInfo {
        String contactId;
        String displayName;
        String photoUri;
        String photoThumbnailUri;
        String photoId;

        public ContactInfo(String contactId, String displayName, String photoUri,
                String photoThumbnailUri, String photoId) {
            this.contactId = contactId;
            this.displayName = displayName;
            this.photoUri = photoUri;
            this.photoThumbnailUri = photoThumbnailUri;
            this.photoId = photoId;
        }

        private boolean hasPhotoUri() {
            return photoUri != null && photoThumbnailUri != null;
        }

        private boolean hasPhotoId() {
            return photoId != null;
        }
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mResolver = getContext().getContentResolver();
        mDevicePolicyManager = (DevicePolicyManager) mContext
                .getSystemService(Context.DEVICE_POLICY_SERVICE);
    }

    public void testPrimaryProfilePhoneLookup_insertedAndfound() throws RemoteException,
            OperationApplicationException, NotFoundException, IOException {
        assertFalse(isManagedProfile());
        // Do not insert to primary contact
        insertContact(PRIMARY_CONTACT_DISPLAY_NAME, PRIMARY_CONTACT_PHONE, 0);

        ContactInfo contactInfo = getContactInfo(PRIMARY_CONTACT_PHONE);
        assertNotNull(contactInfo);
        assertEquals(PRIMARY_CONTACT_DISPLAY_NAME, contactInfo.displayName);
        assertFalse(contactInfo.hasPhotoUri());
        assertFalse(contactInfo.hasPhotoId());
        assertFalse(isEnterpriseContactId(contactInfo.contactId));
    }

    public void testManagedProfilePhoneLookup_insertedAndfound() throws RemoteException,
            OperationApplicationException, NotFoundException, IOException {
        assertTrue(isManagedProfile());
        // Insert ic_contact_picture as photo in managed contact
        insertContact(MANAGED_CONTACT_DISPLAY_NAME, MANAGED_CONTACT_PHONE,
                com.android.cts.managedprofile.R.raw.ic_contact_picture);

        ContactInfo contactInfo = getContactInfo(MANAGED_CONTACT_PHONE);
        assertNotNull(contactInfo);
        assertEquals(MANAGED_CONTACT_DISPLAY_NAME, contactInfo.displayName);
        assertTrue(contactInfo.hasPhotoUri());
        assertTrue(contactInfo.hasPhotoId());
        assertFalse(isEnterpriseContactId(contactInfo.contactId));
    }

    public void testPrimaryProfileEnterprisePhoneLookup_canAccessEnterpriseContact() {
        assertFalse(isManagedProfile());
        ContactInfo contactInfo = getEnterpriseContactInfo(MANAGED_CONTACT_PHONE);
        assertEquals(MANAGED_CONTACT_DISPLAY_NAME, contactInfo.displayName);
        assertTrue(contactInfo.hasPhotoUri());
        // Cannot get photo id in ENTERPRISE_CONTENT_FILTER_URI
        assertFalse(contactInfo.hasPhotoId());
        assertTrue(isEnterpriseContactId(contactInfo.contactId));
    }

    public void testPrimaryProfilePhoneLookup_canAccessPrimaryContact() {
        assertFalse(isManagedProfile());
        ContactInfo contactInfo = getEnterpriseContactInfo(PRIMARY_CONTACT_PHONE);
        assertEquals(PRIMARY_CONTACT_DISPLAY_NAME, contactInfo.displayName);
        assertFalse(contactInfo.hasPhotoUri());
        assertFalse(contactInfo.hasPhotoId());
        assertFalse(isEnterpriseContactId(contactInfo.contactId));
    }

    public void testManagedProfilePhoneLookup_canAccessEnterpriseContact() {
        assertTrue(isManagedProfile());
        ContactInfo contactInfo = getEnterpriseContactInfo(MANAGED_CONTACT_PHONE);
        assertEquals(MANAGED_CONTACT_DISPLAY_NAME, contactInfo.displayName);
        assertTrue(contactInfo.hasPhotoUri());
        assertTrue(contactInfo.hasPhotoId());
        assertFalse(isEnterpriseContactId(contactInfo.contactId));
    }

    public void testPrimaryProfilePhoneLookup_canNotAccessEnterpriseContact() {
        assertFalse(isManagedProfile());
        ContactInfo contactInfo = getEnterpriseContactInfo(MANAGED_CONTACT_PHONE);
        assertNull(contactInfo);
    }

    public void testManagedProfilePhoneLookup_canNotAccessPrimaryContact() {
        assertTrue(isManagedProfile());
        ContactInfo contactInfo = getEnterpriseContactInfo(PRIMARY_CONTACT_PHONE);
        assertNull(contactInfo);
    }

    public void testSetCrossProfileCallerIdDisabled_true() {
        assertTrue(isManagedProfile());
        mDevicePolicyManager.setCrossProfileCallerIdDisabled(
                BaseManagedProfileTest.ADMIN_RECEIVER_COMPONENT, true);
    }

    public void testSetCrossProfileCallerIdDisabled_false() {
        assertTrue(isManagedProfile());
        mDevicePolicyManager.setCrossProfileCallerIdDisabled(
                BaseManagedProfileTest.ADMIN_RECEIVER_COMPONENT, false);
    }

    public void testCurrentProfileContacts_removeContacts() {
        removeAllTestContactsInProfile();
    }

    private boolean isManagedProfile() {
        String adminPackage = BaseManagedProfileTest.ADMIN_RECEIVER_COMPONENT.getPackageName();
        return mDevicePolicyManager.isProfileOwnerApp(adminPackage);
    }

    private void insertContact(String displayName, String phoneNumber, int photoResId)
            throws RemoteException,
            OperationApplicationException, NotFoundException, IOException {
        ArrayList<ContentProviderOperation> ops = new ArrayList<ContentProviderOperation>();
        ops.add(ContentProviderOperation
                .newInsert(ContactsContract.RawContacts.CONTENT_URI)
                .withValue(ContactsContract.RawContacts.ACCOUNT_TYPE, TEST_ACCOUNT_TYPE)
                .withValue(ContactsContract.RawContacts.ACCOUNT_NAME, TEST_ACCOUNT_NAME)
                .build());
        ops.add(ContentProviderOperation
                .newInsert(ContactsContract.Data.CONTENT_URI)
                .withValueBackReference(ContactsContract.Data.RAW_CONTACT_ID, 0)
                .withValue(
                        ContactsContract.Data.MIMETYPE,
                        ContactsContract.CommonDataKinds.StructuredName.CONTENT_ITEM_TYPE)
                .withValue(
                        ContactsContract.CommonDataKinds.StructuredName.DISPLAY_NAME,
                        displayName)
                .build());
        ops.add(ContentProviderOperation
                .newInsert(ContactsContract.Data.CONTENT_URI)
                .withValueBackReference(ContactsContract.Data.RAW_CONTACT_ID, 0)
                .withValue(
                        ContactsContract.Data.MIMETYPE,
                        ContactsContract.CommonDataKinds.Phone.CONTENT_ITEM_TYPE)
                .withValue(ContactsContract.CommonDataKinds.Phone.NUMBER,
                        phoneNumber)
                .withValue(ContactsContract.CommonDataKinds.Phone.TYPE,
                        Phone.TYPE_MOBILE)
                .build());
        if (photoResId != 0) {
            InputStream phoneInputStream = mContext.getResources().openRawResource(photoResId);
            byte[] rawPhoto = getByteFromStream(phoneInputStream);
            ops.add(ContentProviderOperation
                    .newInsert(ContactsContract.Data.CONTENT_URI)
                    .withValueBackReference(ContactsContract.Data.RAW_CONTACT_ID, 0)
                    .withValue(
                            ContactsContract.Data.MIMETYPE,
                            ContactsContract.CommonDataKinds.Photo.CONTENT_ITEM_TYPE)
                    .withValue(Photo.PHOTO, rawPhoto)
                    .build());
        }

        mResolver.applyBatch(ContactsContract.AUTHORITY, ops);
    }

    private ContactInfo getContactInfoFromUri(Uri phoneLookupUri, String phoneNumber) {
        Uri uri = Uri.withAppendedPath(phoneLookupUri,
                Uri.encode(phoneNumber));
        Cursor cursor = mResolver.query(uri,
                new String[] {
                        PhoneLookup._ID, PhoneLookup.DISPLAY_NAME, PhoneLookup.PHOTO_URI,
                        PhoneLookup.PHOTO_THUMBNAIL_URI, PhoneLookup.PHOTO_ID
                }, null, null, null);
        if (cursor == null) {
            return null;
        }
        ContactInfo result = null;
        if (cursor.moveToFirst()) {
            result = new ContactInfo(
                    cursor.getString(cursor.getColumnIndexOrThrow(PhoneLookup._ID)),
                    cursor.getString(cursor.getColumnIndexOrThrow(PhoneLookup.DISPLAY_NAME)),
                    cursor.getString(cursor.getColumnIndexOrThrow(PhoneLookup.PHOTO_URI)),
                    cursor.getString(cursor.getColumnIndexOrThrow(PhoneLookup.PHOTO_THUMBNAIL_URI)),
                    cursor.getString(cursor.getColumnIndexOrThrow(PhoneLookup.PHOTO_ID)));
        }
        cursor.close();
        return result;
    }

    private ContactInfo getContactInfo(String phoneNumber) {
        return getContactInfoFromUri(PhoneLookup.CONTENT_FILTER_URI,
                phoneNumber);
    }

    private ContactInfo getEnterpriseContactInfo(String phoneNumber) {
        return getContactInfoFromUri(
                PhoneLookup.ENTERPRISE_CONTENT_FILTER_URI,
                phoneNumber);
    }

    private void removeAllTestContactsInProfile() {
        ArrayList<ContentProviderOperation> ops = new ArrayList<ContentProviderOperation>();
        ops.add(ContentProviderOperation.newDelete(RawContacts.CONTENT_URI)
                .withSelection(RawContacts.ACCOUNT_TYPE + "=?", new String[] {TEST_ACCOUNT_TYPE})
                .build());
        try {
            mResolver.applyBatch(ContactsContract.AUTHORITY, ops);
        } catch (Exception e) {
            // Catch all exceptions to let tearDown() run smoothly
            e.printStackTrace();
        }
    }

    private static byte[] getByteFromStream(InputStream is) throws IOException {
        ByteArrayOutputStream outputStream = new ByteArrayOutputStream();
        byte[] buf = new byte[1024 * 10];
        int i = 0;
        while ((i = is.read(buf, 0, buf.length)) > 0) {
            outputStream.write(buf, 0, i);
        }
        return outputStream.toByteArray();
    }

    private boolean isEnterpriseContactId(String contactId) {
        return ContactsContract.Contacts.isEnterpriseContactId(Long.valueOf(contactId));
    }
}
