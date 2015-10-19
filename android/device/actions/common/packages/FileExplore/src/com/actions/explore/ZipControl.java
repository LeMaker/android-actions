package com.actions.explore;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.Enumeration;
import java.util.zip.CRC32;
import java.util.zip.CheckedInputStream;
import java.util.zip.CheckedOutputStream;
import java.util.zip.Deflater;
import java.util.zip.ZipException;
import java.util.zip.ZipInputStream;
import org.apache.tools.zip.ZipEntry;
import org.apache.tools.zip.ZipFile;
import org.apache.tools.zip.ZipOutputStream;

public class ZipControl {
	
	/**
	 * 
	 * @author lizihao@actions-semi.com
	 */
	    private static boolean isCreateSrcDir = true;
	    private static String TAG="ZipControl";

	    public  void writeByApacheZipOutputStream(String[] src,
	        String archive, String comment) throws FileNotFoundException,
	        IOException
	    {

	        FileOutputStream f = new FileOutputStream(archive);
	        CheckedOutputStream csum = new CheckedOutputStream(f, new CRC32());
	        ZipOutputStream zos = new ZipOutputStream(csum);
	        zos.setEncoding("GBK");
	        BufferedOutputStream out = new BufferedOutputStream(zos);
	        zos.setComment(comment);
	        zos.setMethod(ZipOutputStream.DEFLATED);
	        zos.setLevel(Deflater.BEST_COMPRESSION);
	        for (int i = 0; i < src.length; i++)
	        {
	            //Log.e(TAG, "src["+i+"] is "+src[i]);
	            File srcFile = new File(src[i]);
	            if (!srcFile.exists())
	            {
	                //Log.e(TAG, "!srcFile.exists()");
	                throw new FileNotFoundException(
	                    "File must exist and ZIP file must have at least one entry.");
	            }

			/*	 empty folder should also support zip
				if(srcFile.isDirectory() && srcFile.list().length == 0){
				}
			*/
	            String strSrcString = src[i];
	            strSrcString = strSrcString.replaceAll("////", "/");
	            String prefixDir = null;
	            if (srcFile.isFile())
	            {
	                prefixDir = strSrcString.substring(0, strSrcString
	                    .lastIndexOf("/") + 1);
	            }
	            else
	            {
	                prefixDir = (strSrcString.replaceAll("/$", "") + "/");
	            }
	            if (prefixDir.indexOf("/") != (prefixDir.length() - 1)
	                && isCreateSrcDir)
	            {
	                prefixDir = prefixDir.replaceAll("[^/]+/$", "");
	            }
	            writeRecursive(zos, out, srcFile, prefixDir);
	        }

	        out.close();
	        //Log.e(TAG, "Checksum: " + csum.getChecksum().getValue());
	        @SuppressWarnings("unused")
	        BufferedInputStream bi;
	    }

	    @SuppressWarnings("unchecked")
	    public static void readByApacheZipFile(String archive, String decompressDir)
	        throws IOException, FileNotFoundException, ZipException
	    {
	        //Log.e(TAG, "readByApacheZipFile");
	        BufferedInputStream bi;
	        ZipFile zf = new ZipFile(archive, "GBK");
	        Enumeration e = zf.getEntries();
	        while (e.hasMoreElements())
	        {
	            ZipEntry ze2 = (ZipEntry) e.nextElement();
	            String entryName = ze2.getName();
	            String path = decompressDir + "/" + entryName;
	            if (ze2.isDirectory())
	            {
	                File decompressDirFile = new File(path);
	                if (!decompressDirFile.exists())
	                {
	                    decompressDirFile.mkdirs();
	                }
	            }
	            else
	            {
	                String fileDir = path.substring(0, path.lastIndexOf("/"));
	                File fileDirFile = new File(fileDir);
	                if (!fileDirFile.exists())
	                {
	                    fileDirFile.mkdirs();
	                }
	                BufferedOutputStream bos = new BufferedOutputStream(
	                    new FileOutputStream(decompressDir + "/" + entryName));
	                bi = new BufferedInputStream(zf.getInputStream(ze2));
	                byte[] readContent = new byte[1024];
	                int readCount = bi.read(readContent);
	                while (readCount != -1)
	                {
	                    bos.write(readContent, 0, readCount);
	                    readCount = bi.read(readContent);
	                }
	                bos.close();
	            }
	        }
	        zf.close();
	    }

	    public static void readByZipInputStream(String archive, String decompressDir)
	        throws FileNotFoundException, IOException
	    {
	        BufferedInputStream bi;
	        FileInputStream fi = new FileInputStream(archive);
	        CheckedInputStream csumi = new CheckedInputStream(fi, new CRC32());
	        ZipInputStream in2 = new ZipInputStream(csumi);
	        bi = new BufferedInputStream(in2);
	        java.util.zip.ZipEntry ze;
	        while ((ze = in2.getNextEntry()) != null)
	        {
	            String entryName = ze.getName();
	            if (ze.isDirectory())
	            {
	                File decompressDirFile = new File(decompressDir + "/"
	                    + entryName);
	                if (!decompressDirFile.exists())
	                {
	                    decompressDirFile.mkdirs();
	                }
	            }
	            else
	            {
	                BufferedOutputStream bos = new BufferedOutputStream(
	                    new FileOutputStream(decompressDir
	                        + "/"
	                        + entryName.substring(entryName.lastIndexOf("//"),
	                            entryName.length()
	                                - (entryName.lastIndexOf("//") - 2))));
	                byte[] buffer = new byte[1024];
	                int readCount = bi.read(buffer);
	                while (readCount != -1)
	                {
	                    bos.write(buffer, 0, readCount);
	                    readCount = bi.read(buffer);
	                }
	                bos.close();
	            }
	        }
	        bi.close();
	        //Log.e(TAG, "Checksum: " + csumi.getChecksum().getValue());
	    }

	    private static void writeRecursive(ZipOutputStream zos,
	        BufferedOutputStream bo, File srcFile, String prefixDir)
	        throws IOException, FileNotFoundException
	    {
	        //Log.e(TAG, "writeRecursive");
	        ZipEntry zipEntry;
	        String filePath = srcFile.getAbsolutePath().replaceAll("////", "/")
	            .replaceAll("//", "/");
	        if (srcFile.isDirectory())
	        {
	            filePath = filePath.replaceAll("/$", "") + "/";
	        }
	        String entryName = filePath.replace(prefixDir, "").replaceAll("/$", "");
	        if (srcFile.isDirectory())
	        {
	            if (!"".equals(entryName))
	            {
	                zipEntry = new ZipEntry(entryName + "/");
	                zos.putNextEntry(zipEntry);
	            }
	            
	            File srcFiles[] = srcFile.listFiles();
		        for (int i = 0; i < srcFiles.length; i++){
		                writeRecursive(zos, bo, srcFiles[i], prefixDir);
		        }
	        }
	        else
	        {
	            BufferedInputStream bi = new BufferedInputStream(
	                new FileInputStream(srcFile));
	            zipEntry = new ZipEntry(entryName);
	            zos.putNextEntry(zipEntry);
	            byte[] buffer = new byte[1024];
	            int readCount = bi.read(buffer);
	            while (readCount != -1)
	            {
	                bo.write(buffer, 0, readCount);
	                readCount = bi.read(buffer);
	            }
	            bo.flush();
	            bi.close();
	        }
	    }

}
