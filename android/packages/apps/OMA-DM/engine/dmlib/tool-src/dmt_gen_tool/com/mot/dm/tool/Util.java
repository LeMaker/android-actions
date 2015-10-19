package com.mot.dm.tool;

import java.io.*;
import com.mot.dm.io.Node;

public class Util {

  public static boolean VERBOSE = false;

  // The following 4 functions are using in all classes to read/write files with UTF-8 encoding
  
	public static Reader openUtf8FileReader( String filename ) throws java.io.FileNotFoundException
	{
		try {
			return new InputStreamReader( new FileInputStream( filename ), "UTF-8" );
		} catch( java.io.UnsupportedEncodingException e ) {
			throw new Error( "UTF-8 encoding is unsupported !!?", e );
		}
	}

	public static Writer openUtf8FileWriter( String filename ) throws java.io.FileNotFoundException
	{
		try {
			return new OutputStreamWriter( new FileOutputStream( filename ), "UTF-8" );
		} catch( java.io.UnsupportedEncodingException e ) {
			throw new Error( "UTF-8 encoding is unsupported !!?", e );
		}
	}

	public static Reader openUtf8FileReader( File filename ) throws java.io.FileNotFoundException
	{
		try {
			return new InputStreamReader( new FileInputStream( filename ), "UTF-8" );
		} catch( java.io.UnsupportedEncodingException e ) {
			throw new Error( "UTF-8 encoding is unsupported !!?", e );
		}
	}

	public static Writer openUtf8FileWriter( File filename ) throws java.io.FileNotFoundException
	{
		try {
			return new OutputStreamWriter( new FileOutputStream( filename ), "UTF-8" );
		} catch( java.io.UnsupportedEncodingException e ) {
			throw new Error( "UTF-8 encoding is unsupported !!?", e );
		}
	}
  
  public static String replaceStr(String str, String oldPat, String newPat) {
    String result = str;
    int from = 0;
    while ( (from = str.indexOf(oldPat, from)) >= 0) {
      result = str.substring(0, from);
      result += newPat;
      result += str.substring(from + oldPat.length(), str.length());
      str = result;
    }
    return result;
  }

  public static void writeFile(String path, String body) throws Exception {
    PrintWriter fileWriter = new PrintWriter(new FileWriter(path));
    fileWriter.print(body);
    fileWriter.flush();
    fileWriter.close();
  }

  public static boolean deleteDir(File dir) {
    if (dir.isDirectory()) {
      String[] children = dir.list();
      for (int i = 0; i < children.length; i++) {
        boolean success = deleteDir(new File(dir, children[i]));
        if (!success) {
          return false;
        }
      }
    }
    return dir.delete();
  }
  
	public static void quickSort(Node children[]) {
		q_sort(children, 0, children.length - 1);
	}

	private static void q_sort(Node a[], int low, int high) {
		int lo = low;
		int hi = high;
		Node mid;

		if (high > low) {
			mid = a[(low + high) / 2];

			while (lo <= hi) {

				while ((lo < high)
						&& (compareStringMultiNodeFirst(a[lo].getName(), mid
								.getName()) < 0))
					++lo;

				while ((hi > low)
						&& (compareStringMultiNodeFirst(a[hi].getName(), mid
								.getName()) > 0))
					--hi;

				if (lo <= hi) {
					Node n = a[hi];
					a[hi] = a[lo];
					a[lo] = n;

					++lo;
					--hi;
				}
			}

			if (low < hi)
				q_sort(a, low, hi);

			if (lo < high)
				q_sort(a, lo, high);

		}
	}
	
	private static int compareStringMultiNodeFirst(String str1, String str2) {
		if (str1 == null) {
			if (str2 == null) {
				return 0;
			} else {
				return -1;
			}

		} else if (str2 == null) {
			return 1;
		}
		// Check first for multinode definition "[]". The file "[]" should be always first.
		if (str1.equals("[]") && str2.equals("[]")) {
			return 0;
		} else if (str1.equals("[]")) {
			return -1;
		} else if (str2.equals("[]")) {
			return 1;
		}

		return str1.compareToIgnoreCase(str2);
	}

  public static void verbose(String msg) {
    if (VERBOSE) {
      System.out.println(msg);
    }
  }

}
