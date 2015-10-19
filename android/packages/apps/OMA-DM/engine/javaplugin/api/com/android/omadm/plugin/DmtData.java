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

package com.android.omadm.plugin;

import android.os.Parcel;
import android.os.Parcelable;
import android.text.TextUtils;
import android.util.Log;

import java.util.Arrays;
import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.Map;

/**
 * DmtData encapsulates various DMT node data formats.
 *
 * <pre>
 * Data formats includes:
 *     NULL
 *     String
 *     Integer
 *     Boolean
 *     Binary
 *     Date
 *     Time
 *     Float
 *     NODE
 * </pre>
 */
public class DmtData implements Parcelable {
    private static final String TAG = "DmtData";

    /**
     * NULL Data type.
     */
    public static final int NULL = 0;

    /**
     * Undefined.
     */
    public static final int UNDEFINED = NULL;

    /**
     * String Data
     */
    public static final int STRING = 1;

    /**
     * Integer Data
     */
    public static final int INT = 2;

    /**
     * Boolean Data
     */
    public static final int BOOL = 3;

    /**
     * Binary Data
     */
    public static final int BIN = 4;

    /**
     * Interior Node
     */
    public static final int NODE = 5;

    /**
     * Base 64 Node
     */
    public static final int B64 = 6;

    /**
     * XML Node
     */
    public static final int XML = 7;

    /**
     * Date data
     */
    public static final int DATE = 8;

    /**
     * Time data
     */
    public static final int TIME = 10;

    /**
     * Float data
     */
    public static final int FLOAT = 11;

    /*
     * Used to create a DmtData((String)null)
     */
    public static final String NULLVALUE = null;

    private int type;

    private String stringValue;

    private int intValue;

    private boolean boolValue;

    private byte[] binValue;

    private String dateValue;

    private String timeValue;

    private float floatValue;

    private final LinkedHashMap<String, DmtData> childNodes = new LinkedHashMap<String, DmtData>();

    /**
     * Data represent a default value, it is only used for setting default value
     * to a node. if the node does not have a default value, a null value is
     * used instead.
     */
    public DmtData() {
    }

    public DmtData(String str, int dataType) {
        init(str, dataType);
    }

    private void init(String str, int dataType) {
        type = dataType;
        if (str == null) {
            return;
        }
        switch (dataType) {
            case NULL:
                break;
            case STRING:
                stringValue = str;
                break;
            case INT:
                try {
                    intValue = Integer.parseInt(str);
                } catch (NumberFormatException e) {
                    Log.e(TAG, "can't parse init value as integer", e);
                }
                break;
            case BOOL:
                boolValue = Boolean.parseBoolean(str);
                break;
            case BIN:
                binValue = str.getBytes();
                break;
            case DATE:
                dateValue = str;
                break;
            case TIME:
                timeValue = str;
                break;
            case FLOAT:
                try {
                    floatValue = Float.parseFloat(str);
                } catch (NumberFormatException e) {
                    Log.e(TAG, "can't parse init value as float", e);
                }
                break;
            case NODE:
                childNodes.clear();
                String[] strArray = str.split("\\|");
                int cnt = strArray.length;
                if (cnt > 0 && !strArray[0].isEmpty()) {
                    for (String name : strArray) {
                        childNodes.put(name, new DmtData());
                    }
                }
                break;
            default:
                type = UNDEFINED;
                break;
        }
    }

    /**
     * Data represent an String type.
     *
     * @param str String type data. The size may be restricted by MDF.
     */
    public DmtData(String str) {
        stringValue = str;
        type = STRING;
    }

    /**
     * Data represent an integer type Data range may be restricted by MDF
     */
    public DmtData(int integer) {
        intValue = integer;
        type = INT;
    }

    /**
     * Data represent a boolean type
     */
    public DmtData(boolean bool) {
        boolValue = bool;
        type = BOOL;
    }

    /**
     * Data represent a binary type Data size may be restricted by MDF. Data may
     * be null.
     */
    public DmtData(byte[] bin) {
        binValue = bin;
        type = BIN;
    }

    public DmtData(float value) {
        floatValue = value;
        type = FLOAT;
    }

    @Override
    public String toString() {
        return getString();
    }

    /**
     * Get string representation of the value. It will automatically convert to
     * string value from other types. for NULL value, return empty String. for
     * Default value, return null.
     */
    public String getString() {
        switch (type) {
            case NULL:
                return "";
            case STRING:
                return stringValue;
            case INT:
                return String.valueOf(intValue);
            case BOOL:
                return String.valueOf(boolValue);
            case BIN:
                return (binValue == null) ? null : (new String(binValue));
            case DATE:
                return dateValue;
            case TIME:
                return timeValue;
            case FLOAT:
                return String.valueOf(floatValue);
            case NODE:
                StringBuilder tmpValue = new StringBuilder();
                try {
                    for (String node : childNodes.keySet()) {
                        tmpValue.append(node).append('|');
                    }
                    if (tmpValue.length() != 0) {
                        tmpValue.deleteCharAt(tmpValue.length() - 1);
                    }
                } catch (Exception e) {
                    Log.e(TAG, "getString() failed for node object", e);
                }
                return tmpValue.toString();
            default:
                return null;
        }
    }

    /**
     * Get Boolean value.
     *
     * @return boolean value.
     */
    public boolean getBoolean() throws DmtException {
        if (type != BOOL) {
            throw new DmtException(ErrorCodes.SYNCML_DM_INVALID_PARAMETER,
                    "The value requested is not boolean");
        }
        return boolValue;
    }

    /**
     * Get Integer value.
     *
     * @return integer value
     */
    public int getInt() throws DmtException {
        if (type != INT) {
            throw new DmtException(ErrorCodes.SYNCML_DM_INVALID_PARAMETER,
                    "The value requested is not integer");
        }
        return intValue;
    }

    /**
     * Get binary value.
     *
     * @return binary value in byte[].
     */
    public byte[] getBinary() throws DmtException {
        if (type != BIN) {
            throw new DmtException(ErrorCodes.SYNCML_DM_INVALID_PARAMETER,
                    "The value requested is not binary");
        }
        return binValue;
    }

    /**
     * Adds the specified child node to this interior node.
     * @param name the name of the node to add
     * @param child the node data to add
     * @throws DmtException if this is not an interior node
     */
    public void addChildNode(String name, DmtData child) throws DmtException {
        if (type != NODE) {
            throw new DmtException(ErrorCodes.SYNCML_DM_INVALID_PARAMETER,
                    "can't add children to leaf node");
        }
        childNodes.put(name, child);
    }

    public void removeChildNode(String name) throws DmtException {
        if (type != NODE) {
            throw new DmtException(ErrorCodes.SYNCML_DM_INVALID_PARAMETER,
                    "can't add children to leaf node");
        }
        childNodes.remove(name);
    }

    /**
     * Returns the specified child of this interior node.
     * @return the specified node data, or null if not found
     * @throws DmtException if this is not an interior node
     */
    public DmtData getChild(String name) throws DmtException {
        if (type != NODE) {
            throw new DmtException(ErrorCodes.SYNCML_DM_INVALID_PARAMETER,
                    "can't get children of leaf node");
        }
        return childNodes.get(name);
    }

    /**
     * Returns all children of this interior node.
     * @return a map from node names to node data
     * @throws DmtException if this is not an interior node
     */
    public Map<String, DmtData> getChildNodeMap() throws DmtException {
        if (type != NODE) {
            throw new DmtException(ErrorCodes.SYNCML_DM_INVALID_PARAMETER,
                    "can't get children of leaf node");
        }
        return Collections.unmodifiableMap(childNodes);
    }

    @Override
    public boolean equals(Object obj) {
        if (!(obj instanceof DmtData)) {
            return false;
        }
        DmtData data = (DmtData) obj;

        if (getType() != data.getType()) {
            return false;
        }

        try {
            switch (getType()) {
                case NULL:
                    return true;

                case DATE:
                case TIME:
                case FLOAT:
                case STRING:
                case NODE:
                    String str1 = getString();
                    String str2 = data.getString();
                    return TextUtils.equals(str1, str2);

                case INT:
                    return getInt() == data.getInt();

                case BOOL:
                    return getBoolean() == data.getBoolean();

                case BIN:
                    byte[] bytes1 = getBinary();
                    byte[] bytes2 = data.getBinary();
                    return Arrays.equals(bytes1, bytes2);

                default:
                    return false;
            }
        } catch (DmtException e) {
            return false;
        }
    }

    /**
     * Return the Type associated with the data This information is to be used
     * by the persistence layer. Persistence does NOT need to use meta info to
     * get the data
     */
    public int getType() {
        return type;
    }

    @Override
    public void writeToParcel(Parcel out, int flags) {
        out.writeInt(type);
        switch (type) {
            case NULL:
                break;

            case STRING:
                out.writeString(stringValue);
                break;

            case INT:
                out.writeInt(intValue);
                break;

            case BOOL:
                out.writeInt(boolValue ? 1 : 0);
                break;

            case BIN:
                if (binValue == null || binValue.length == 0) {
                    out.writeInt(0);
                } else {
                    out.writeInt(binValue.length);
                    out.writeByteArray(binValue);
                }
                break;

            case DATE:
                out.writeString(dateValue);
                break;

            case TIME:
                out.writeString(timeValue);
                break;

            case FLOAT:
                out.writeFloat(floatValue);
                break;

            case NODE:
                out.writeInt(childNodes.size());
                for (Map.Entry<String, DmtData> entry : childNodes.entrySet()) {
                    out.writeString(entry.getKey());
                    entry.getValue().writeToParcel(out, flags);
                }
                break;

            default:
                break;
        }
    }

    @Override
    public int describeContents() {
        return 0;
    }

    public static final Creator<DmtData> CREATOR = new Creator<DmtData>() {
        @Override
        public DmtData createFromParcel(Parcel in) {
            return new DmtData(in);
        }

        @Override
        public DmtData[] newArray(int size) {
            return new DmtData[size];
        }
    };

    DmtData(Parcel in) {
        type = in.readInt();
        switch (type) {
            case NULL:
                break;

            case STRING:
                stringValue = in.readString();
                break;

            case INT:
                intValue = in.readInt();
                break;

            case BOOL:
                boolValue = (in.readInt() != 0);
                break;

            case BIN:
                int length = in.readInt();
                if (length == 0) {
                    binValue = null;
                } else {
                    binValue = new byte[length];
                    in.readByteArray(binValue);
                }
                break;

            case DATE:
                dateValue = in.readString();
                break;

            case TIME:
                timeValue = in.readString();
                break;

            case FLOAT:
                floatValue = in.readFloat();
                break;

            case NODE:
                childNodes.clear();
                int childNodeCount = in.readInt();
                for (int i = 0; i < childNodeCount; i++) {
                    String name = in.readString();
                    DmtData value = new DmtData(in);
                    childNodes.put(name, value);
                }
                break;

            default:
                break;
        }
    }
}
