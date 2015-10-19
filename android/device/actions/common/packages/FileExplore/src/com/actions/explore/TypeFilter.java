/*
   @author lizihao@actions-semi.com
*/

package com.actions.explore;

public final class TypeFilter{
	
	static private TypeFilter sInstance;
	public synchronized static TypeFilter getInstance() {
        if (sInstance == null) {
        	sInstance = new TypeFilter();
        }
        return sInstance;
    }
	
	public boolean isPadFile(String ext)
	{
		if(ext.equalsIgnoreCase("pad"))
		{
			return true;
		}
		
		return false;
	}
	
	public boolean isTxtFile(String ext)
	{
		if(ext.equalsIgnoreCase("txt"))
		{
			return true;
		}
		
		return false;
	}
	
	public boolean isPdfFile(String ext)
	{
		if(ext.equalsIgnoreCase("pdf"))
		{
			return true;
		}
		
		return false;
	}
	
	public boolean isMusicFile(String ext)
	{
		if(ext.equalsIgnoreCase("mp3") ||
				ext.equalsIgnoreCase("ogg") ||
				ext.equalsIgnoreCase("wav") ||
				ext.equalsIgnoreCase("wma") ||
				ext.equalsIgnoreCase("m4a") ||
                                ext.equalsIgnoreCase("ape") ||
                                ext.equalsIgnoreCase("dts") ||
                                ext.equalsIgnoreCase("flac") ||
                                ext.equalsIgnoreCase("mp1") ||
                                ext.equalsIgnoreCase("mp2") ||
                                ext.equalsIgnoreCase("aac") ||
                                ext.equalsIgnoreCase("midi") ||
                                ext.equalsIgnoreCase("mid") ||
                                ext.equalsIgnoreCase("mp5") ||
                                ext.equalsIgnoreCase("mpga") ||
                                ext.equalsIgnoreCase("mpa") ||
				ext.equalsIgnoreCase("m4p") ||
				ext.equalsIgnoreCase("aac") ||
				ext.equalsIgnoreCase("amr"))
		{
			return true;
		}
		
		return false;
	}
	
	public boolean isPictureFile(String ext)
	{
		if(ext.equalsIgnoreCase("png") ||
				ext.equalsIgnoreCase("jpeg") ||
				ext.equalsIgnoreCase("jpg") ||
				ext.equalsIgnoreCase("gif") ||
				ext.equalsIgnoreCase("bmp") ||
                                ext.equalsIgnoreCase("jfif") ||
				ext.equalsIgnoreCase("tiff"))
		{
			return true;
		}
		
		return false;
	}
	
	public boolean isZipFile(String ext)
	{
		if(ext.equalsIgnoreCase("zip"))
		{
			return true;
		}
		
		return false;
	}
	
	public boolean isRarFile(String ext)
	{
		if(ext.equalsIgnoreCase("rar"))
		{
			return true;
		}
		
		return false;
	}
	
	public boolean isGZipFile(String ext)
	{
		if(ext.equalsIgnoreCase("gzip") ||
				ext.equalsIgnoreCase("gz"))
		{
			return true;
		}
		
		return false;
	}
	
	public boolean isMovieFile(String ext)
	{
		if(ext.equalsIgnoreCase("avi") ||
				ext.equalsIgnoreCase("wmv") ||
				ext.equalsIgnoreCase("rmvb") ||
				ext.equalsIgnoreCase("mkv") ||
				ext.equalsIgnoreCase("m4v") ||
                ext.equalsIgnoreCase("mov") ||
                ext.equalsIgnoreCase("mpg") ||
                ext.equalsIgnoreCase("rm") ||
                ext.equalsIgnoreCase("flv") ||
                ext.equalsIgnoreCase("pmp") ||
                ext.equalsIgnoreCase("vob") ||
                ext.equalsIgnoreCase("asf") ||
                ext.equalsIgnoreCase("psr") ||
                ext.equalsIgnoreCase("3gp") ||
                ext.equalsIgnoreCase("mpeg") ||
                ext.equalsIgnoreCase("ram") ||
                ext.equalsIgnoreCase("divx") ||
                ext.equalsIgnoreCase("m4p") ||
                ext.equalsIgnoreCase("m4b") ||
				ext.equalsIgnoreCase("mp4") ||
				ext.equalsIgnoreCase("f4v") ||
				ext.equalsIgnoreCase("3gpp") ||
				ext.equalsIgnoreCase("3g2") ||
				ext.equalsIgnoreCase("3gpp2") ||
				ext.equalsIgnoreCase("webm") ||
				ext.equalsIgnoreCase("ts") ||
				ext.equalsIgnoreCase("tp") ||
				ext.equalsIgnoreCase("m2ts") ||
				ext.equalsIgnoreCase("3dv") ||
				ext.equalsIgnoreCase("3dm"))	
		{
			return true;
		}
		
		return false;
	}
	
	public boolean isWordFile(String ext)
	{
		if(ext.equalsIgnoreCase("doc") ||
				ext.equalsIgnoreCase("docx"))
		{
			return true;
		}
		
		return false;
	}
	
	public boolean isExcelFile(String ext)
	{
		if(ext.equalsIgnoreCase("xls") ||
				ext.equalsIgnoreCase("xlsx"))
		{
			return true;
		}
		
		return false;
	}
	
	public boolean isPptFile(String ext)
	{
		if(ext.equalsIgnoreCase("ppt") ||
				ext.equalsIgnoreCase("pptx"))
		{
			return true;
		}
		
		return false;
	}
	
	public boolean isHtml32File(String ext)
	{
		if(ext.equalsIgnoreCase("html"))
		{
			return true;
		}
		
		return false;
	}
	
	public boolean isXml32File(String ext)
	{
		if(ext.equalsIgnoreCase("xml"))
		{
			return true;
		}
		
		return false;
	}
	
	public boolean isConfig32File(String ext)
	{
		if(ext.equalsIgnoreCase("conf"))
		{
			return true;
		}
		
		return false;
	}
	
	public boolean isApkFile(String ext)
	{
		if(ext.equalsIgnoreCase("apk"))
		{
			return true;
		}
		
		return false;
	}

	public boolean isJarFile(String ext)
	{
		if(ext.equalsIgnoreCase("jar"))
		{
			return true;
		}
		
		return false;
	}
//mac address add start
	public boolean isShellFile(String ext)
    {
        if(ext.equalsIgnoreCase("wmsh"))
        {
            return true;
        }
        
        return false;
    }
//mac address add end	
	public boolean catalogMusicFile(String ext)
	{
		return isMusicFile(ext);
	}
	
	public boolean catalogMovieFile(String ext)
	{
		return isMovieFile(ext);
	}
	
	public boolean catalogEbookFile(String ext)
	{
		return isTxtFile(ext) || isPdfFile(ext) || isWordFile(ext)
				|| isExcelFile(ext) || isPptFile(ext);
	}
	
	public boolean catalogPictureFile(String ext)
	{
		return isPictureFile(ext);
	}
	
	public boolean catalogApkFile(String ext)
	{
		return isApkFile(ext);
	}
}
