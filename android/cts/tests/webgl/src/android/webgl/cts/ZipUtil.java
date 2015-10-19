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

package android.webgl.cts;

import android.util.Log;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.lang.String;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;


/**
 * Some boilerplate code to unzip files.
 */
public class ZipUtil {
    private final static String TAG = "ZipUtil";

    /**
     * Stream to a file.
     */
    public static void streamToPath(InputStream is,
                                  File directory,
                                  String name) throws Exception {
        File file = new File(directory, name);
        streamToPath(is, file);
    }

    public static void streamToPath(InputStream is,
                                  File file) throws Exception {
        Log.i(TAG, "Streaming to path " + file.getPath());
        OutputStream os = null;
        os = new FileOutputStream(file);
        int count = -1;
        byte[] buffer = new byte[10 * 1024];
        while ((count = is.read(buffer)) != -1) {
            os.write(buffer, 0, count);
        }
        os.close();
    }

    /**
     * Unzip to a directory.
     */
    public static void unzipToPath(InputStream is,
                                   File filePath) throws Exception {
        ZipInputStream zis = new ZipInputStream(is);
        unzipToPath(zis, filePath.getPath());
    }

    public static void unzipToPath(ZipInputStream zis,
                                   String path) throws Exception {
        Log.i(TAG, "Unzipping to path " + path);
        byte[] buffer = new byte[10 * 1024];
        ZipEntry entry;
        while ((entry = zis.getNextEntry()) != null) {
            File entryFile = new File(path, entry.getName());
            if (entry.isDirectory()) {
                if (!entryFile.exists()) {
                   entryFile.mkdirs();
                }
                continue;
            }
            if (entryFile.getParentFile() != null &&
                    !entryFile.getParentFile().exists()) {
                entryFile.getParentFile().mkdirs();
            }
            if (!entryFile.exists()) {
                entryFile.createNewFile();
                entryFile.setReadable(true);
                entryFile.setExecutable(true);
            }
            streamToPath(zis, entryFile);
        }
        zis.close();
    }

    /**
     * Cleanup a directory.
     */
    static public boolean deleteDirectory(String directoryPath) {
        File path = new File(directoryPath);
        return deleteDirectory(path);
    }

    static public boolean deleteDirectory(File path) {
        if (path.exists()) {
            File[] files = path.listFiles();
            for(int i = 0; i < files.length; i++) {
                if(files[i].isDirectory()) {
                    deleteDirectory(files[i]);
                } else {
                    files[i].delete();
                }
            }
            return path.delete();
        }
        return false;
    }
}
