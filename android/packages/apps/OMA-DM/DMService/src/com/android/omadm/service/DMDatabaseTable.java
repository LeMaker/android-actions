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

package com.android.omadm.service;

import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteStatement;
import android.util.Log;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * Represents a single table in an SQLite database.
 */
class DMDatabaseTable {
    private static final String TAG = "DMDatabaseTable";

    private static final int TABLE_INFO_PRAGMA_COLUMN_NAME  = 1;
    private static final int TABLE_INFO_PRAGMA_DATA_TYPE    = 2;
    public static final int TABLE_INFO_PRAGMA_NULL_ALLOWED  = 3;
    public static final int TABLE_INFO_PRAGMA_DEFAULT       = 4;

    private final String mName;

    // map the name a a column to its type
    private HashMap<String, String> mColumns = new HashMap<String, String>();
    public DMDatabaseTable(SQLiteDatabase db, String name) {
        mName = name;
        mColumns = init(db);
    }

    String getName() {
        return mName;
    }

    boolean containsColumn(String name) {
        return mColumns != null && mColumns.containsKey(name);
    }

    /**
     * Return whether the column map has been initialized.
     * @return true if column map is valid; false if column map is null.
     */
    boolean isValid() {
        return mColumns != null;
    }

    /**
     * populate the mColumns table based on content of the database
     *
     * @param db
     * @return
     */
    HashMap<String, String> init(SQLiteDatabase db) {
        HashMap<String, String> ret = null;
        Cursor cur = null;
        try {
            cur = db.rawQuery("PRAGMA table_info(" + mName + ')', null);
            int cnt = cur.getCount();
            logd(String.format("Cursor count for table %s is %d", mName, cnt));
            if (cnt > 0) {
                ret = new HashMap<String, String>(cur.getCount());
                while (cur.moveToNext()) {
                    String columnName = cur.getString(TABLE_INFO_PRAGMA_COLUMN_NAME);
                    String dataType = cur.getString(TABLE_INFO_PRAGMA_DATA_TYPE);
                    ret.put(columnName, dataType);
                    logd(String.format("%s %s", columnName, dataType));
                }
            }
        } finally {
            if (cur != null) cur.close();
        }

        return ret;
    }

    boolean rowValid(ArrayList<String> cols) {
        for (String col : cols) {
            if (!containsColumn(col)) {
                loge("Attempt to flex invalid column " + col);
                return false;
            }
        }
        return true;
    }

    /**
     * insert a single row
     * TODO:  call sequence should to be optimized so that we can reuse
     *        compiled statement rather than recreating on each row.
     * @param db
     * @param row a HashMap that was created from the UmlUtils.readXml() call
     */
    public void insertRow(SQLiteDatabase db, HashMap row) {

         ArrayList<String> cols = new ArrayList<String>();
         ArrayList<String> vals = new ArrayList<String>();
         for (Object entryObject : row.entrySet()) {
             Map.Entry<String, Object> entry = (Map.Entry<String, Object>) entryObject;
             String key = entry.getKey();
             if (!containsColumn(key)) {
                 loge("Attempt to flex invalid column " + key);
             }
             String val = entry.getValue().toString();
             cols.add(key);
             vals.add(val);
         }

         if (rowValid(cols)) {
             try {
                 insertRow(db, cols, vals);
             } catch (IllegalArgumentException iae) {
                 loge("Column count does not match values cannot create insert");
             } catch (Exception e) {
                 loge(e.getMessage());
             }
         }
         logd("insertRow - complete");
    }

    /**
     * insert a single row
     *
     * TODO: use this for mkitso files also
     *
     * @param db
     * @param cols list of columns
     * @param vals list of values
     */
    void insertRow(SQLiteDatabase db, ArrayList<String> cols, ArrayList<String> vals)
        throws IllegalArgumentException {
         int numberColumns = cols.size();
         int numberValues = vals.size();

         if (numberColumns != numberValues) {
             throw new IllegalArgumentException("vals.size() != cols.size()");
         }

         String insert = String.format("INSERT OR REPLACE INTO %s (%s) VALUES(%s);",
                 getName(), join(cols, ", "), joinNTimes("?", vals.size()));
         logd(insert);
         SQLiteStatement stmt = null;
         try {
             stmt = db.compileStatement(insert);
             for (int i = 0; i < numberColumns; i++) {
                 stmt.bindString(i + 1, vals.get(i));
             }
             stmt.execute();
         } catch (Exception e) {
             loge(String.format("Unexpected exception for %s: %s", insert, e.getMessage()));
         } finally {
           if (stmt != null) stmt.close();
         }
         logd("insertRow - complete");
    }

    /**
     * utility function to join a string separated by another string
     *
     * @param args
     * @param sep
     * @return
     */
    private static String join(List<String> args, String sep) {
        StringBuilder sb = new StringBuilder();
        if (args != null) {
            int cnt = args.size();
            if (cnt > 0) {
                for (int i = 0; i < cnt - 1; i++) {
                    sb.append(args.get(i));
                    sb.append(sep);
                }
                sb.append(args.get(cnt - 1));
            }
        }
        return sb.toString();
    }

    /**
     * utility function to repeat a string in times separated by a comma
     */
    private static String joinNTimes(String str, int cnt) {
        StringBuilder sb = new StringBuilder();
        if (cnt > 0) {
            for (int i = 0; i < cnt - 1; i++) {
                sb.append(str);
                sb.append(", ");
            }
            sb.append(str);
        }
        return sb.toString();
    }

    private static void logd(String msg) {
        Log.d(TAG, msg);
    }

    private static void loge(String msg) {
        Log.e(TAG, msg);
    }
}
