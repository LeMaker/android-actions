package com.android.actions.cts.autosetting;

import android.content.ContentResolver;
import android.content.ContentValues;
import android.database.SQLException;
import android.net.Uri;
import android.provider.BaseColumns;
import android.util.Log;

public final class GoogleSettingsContract
{
  public static class NameValueTable
    implements BaseColumns
  {
    protected static boolean putString(ContentResolver paramContentResolver, Uri paramUri, String paramString1, String paramString2)
    {
      //int i = 0;
      try
      {
        ContentValues localContentValues = new ContentValues();
        localContentValues.put("name", paramString1);
        localContentValues.put("value", paramString2);
        paramContentResolver.insert(paramUri, localContentValues);
        //i = 1;
        //
      }
      catch (SQLException localSQLException)
      {
        Log.e("GoogleSettings", "Can't set key " + paramString1 + " in " + paramUri, localSQLException);
      }
      catch (IllegalArgumentException localIllegalArgumentException)
      {
        Log.e("GoogleSettings", "Can't set key " + paramString1 + " in " + paramUri, localIllegalArgumentException);
      }
      return true;
    }
  }

  public static final class Partner extends GoogleSettingsContract.NameValueTable
  {
    public static final Uri CONTENT_URI = Uri.parse("content://com.google.settings/partner");

    public static boolean putInt(ContentResolver paramContentResolver, String paramString, int paramInt)
    {
      return putString(paramContentResolver, paramString, String.valueOf(paramInt));
    }

    public static boolean putString(ContentResolver paramContentResolver, String paramString1, String paramString2)
    {
      return putString(paramContentResolver, CONTENT_URI, paramString1, paramString2);
    }
  }
}