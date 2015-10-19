#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
  
#define MAX_STRING_LENGTH 255


bool IsEnd(int handler)
{
    int nPos;

    nPos = lseek(handler,0,SEEK_CUR);

    if ( nPos < 0 )
        return true; // means some error

    struct stat st;
    if(fstat(handler, &st) < 0)
       return true;

    if ( st.st_size == nPos )
        return true;
    else
        return false;
}


char* ReadLine(int handler, char* s, int size )
{

    char c = 0;
    int nPos = 0;
    
    while ( read(handler, &c, 1 ) != -1 && !IsEnd(handler) ) 
    {
        s[nPos] = c;
        nPos++;

        if ( c == '\n' )
            break;
    }
    s[nPos] = 0;
    return s;
}


void ConvertFile(const char * pPath, const char * pFileName, const char * pExtension, bool bTextFile)
{


    char full_path_in[MAX_STRING_LENGTH];
    char full_path_out_c[MAX_STRING_LENGTH];
    char full_path_out_h[MAX_STRING_LENGTH];
    char signature[MAX_STRING_LENGTH]; 
    char str[MAX_STRING_LENGTH];
    char str_size[20];
    int size;
    
    int len = pExtension == NULL ? 0 : strlen(pExtension)+1;

    strcpy(full_path_in,pPath);
    strcat(full_path_in,"/");
    strcat(full_path_in,pFileName);
 
    memset(signature,0,MAX_STRING_LENGTH);
    memset(str,0,MAX_STRING_LENGTH);
    
    strncpy(signature,pFileName,strlen(pFileName)-len);
    
    int file_handler_in;
    file_handler_in = open(full_path_in, O_RDONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP ); 
    if ( file_handler_in == -1 )
    {
        printf("Unable open file %s\n",full_path_in);  
        return;
    }

    struct stat st;
    if(fstat(file_handler_in, &st) < 0)
    {
       printf("Unable get size of file %s\n",full_path_in);  
       return;
    }   
    
    size = st.st_size;
    
    if ( pExtension )
    {
       strcat(signature,"_");
       strcat(signature,pExtension);
    }   

    strcpy(full_path_out_c,signature);
    strcpy(full_path_out_h,signature);
        
    strcat(full_path_out_c,".c");
    strcat(full_path_out_h,".h");
    
    strcpy(str,"#include \"");
    strcat(str,signature);
    strcat(str,".h\"\n\n#ifdef __cplusplus\nextern \"C\" {\n#endif\n\n");   
    strcat(str,"static UINT8 ");
    strcat(str,signature);
    strcat(str,"[] = {\n");
    
    int file_handler_c;
    int file_handler_h;

    file_handler_c = open(full_path_out_c, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP ); 
    if ( file_handler_c == -1 )
    {
        printf("Unable open file %s\n",full_path_out_c);  
        return;
    }

  
    file_handler_h = open(full_path_out_h, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP ); 
    if ( file_handler_h == -1 )
    {
        printf("Unable open file %s\n",full_path_out_h);  
        return;
    }

    write(file_handler_c,str,strlen(str));
    
    int current_byte_in_line = 0;
    int current_byte = 0;
    char hex_byte[20];
    int i;
    
    if ( pExtension && strcmp(pExtension,"bmdf") == 0  )
    {
        int value = 0;
        unsigned char value1[4]; 

        read(file_handler_in,(unsigned char*)value1,4);
        if ( value1[0] == '[' && (value1[1] == '/' || value1[2] == '.') )
        {
            printf("BMDF: file %s not recognized as a bmdf file\n",full_path_in);
            return;
        }
         
        for (i=0; i<4; i++) 
        {
           value |= value1[i] << (i*8);
        }
        if ( value != size )
        {
            printf("corrupted mdf file %s ,size %d\n",full_path_in,value); 
            return;
        } 
        printf("mdf file %s ,size %d\n",full_path_in,value);
        lseek(file_handler_in,0,SEEK_SET);
    }


    if ( bTextFile == true )
    {
        char line[300];
        int new_size = 0;
        int line_len = 0;
        while ( !IsEnd(file_handler_in) )
        {
           ReadLine(file_handler_in,line,300);
           if ( line[0] == '#' )
             continue;

           line_len = strlen(line); 
           while ( line_len > 0 &&  (line[line_len-1]=='\r' || line[line_len-1]=='\n' || 
                                     line[line_len-1]==' ' || line[line_len-1]=='\t' ) )
              line[--line_len]='\0';

           if ( line_len == 0 )
                continue;

           char * new_line = line;
      
           while ( ( *new_line == ' ' || *new_line == '\t') && *new_line != '\0' )
             new_line ++;

           line_len = strlen(new_line);

           if ( line_len == 0 )
                continue;

           line[line_len] = '\0';
           for (i = 0; i<=line_len; i++)
           {
               if ( line[i] == '\t' )
                 line[i] = ' ';
               current_byte_in_line++;
               new_size++;
               if ( i < line_len )
               {
                      if ( current_byte_in_line != 30 )
                        sprintf(hex_byte,"%#4x,",line[i]);
                      else
                      {
                        sprintf(hex_byte,"%#4x,\n",line[i]);
                        current_byte_in_line = 0;
                      }
               }
               else
               {
                    if ( IsEnd(file_handler_in) )
                        sprintf(hex_byte,"%#4x\n",line[i]);
                    else
                    {
                      if ( current_byte_in_line != 30 )
                        sprintf(hex_byte,"%#4x,",line[i]);
                      else
                      {
                        sprintf(hex_byte,"%#4x,\n",line[i]);
                        current_byte_in_line = 0;
                      }
                    }  
               }
               write(file_handler_c,hex_byte,strlen(hex_byte));
           }   
        }
        size = new_size;
    }
    else
    {
      
      for (int i=0; i<size; i++)
      {
           read(file_handler_in,&current_byte,1);
           current_byte_in_line++;
           if ( i != size - 1 ) 
           {
              if ( current_byte_in_line != 30 )
                sprintf(hex_byte,"%#4x,",current_byte);
              else
              {
                sprintf(hex_byte,"%#4x,\n",current_byte);
                current_byte_in_line = 0;
              }
           } 
           else
              sprintf(hex_byte,"%#4x\n",current_byte);
           write(file_handler_c,hex_byte,strlen(hex_byte));
        }
    }  
        
    strcpy(str,"};\n\n");

    write(file_handler_c,str,strlen(str));

    sprintf(str_size,"%d",size);

    strcpy(str,"#ifndef ");
    strcat(str,signature);
    strcat(str,"_H\n#define ");
    strcat(str,signature);
    strcat(str,"_H\n\n#include \"xpl_Types.h\"\n\n#ifdef __cplusplus\nextern \"C\" {\n#endif\n\n");

    write(file_handler_h,str,strlen(str));


    strcpy(str,"const UINT8 * dmGet_");
    strcat(str,signature);  
    strcat(str,"(UINT32 * size)");

    write(file_handler_h,str,strlen(str));
    write(file_handler_c,str,strlen(str));

    strcpy(str,";\n\n#ifdef __cplusplus\n}\n#endif\n\n#endif\n\n");

    write(file_handler_h,str,strlen(str));

    strcpy(str,"\n{\n    if ( size )\n    {\n       *size = ");
    strcat(str,str_size);
    strcat(str,";\n       return ");
    strcat(str,signature);
    strcat(str,";\n    }\n    else\n    {\n       return NULL;\n    }\n}\n\n#ifdef __cplusplus\n}\n#endif\n\n");

    write(file_handler_c,str,strlen(str));

    close(file_handler_c);
    close(file_handler_h);
    close(file_handler_in);
    
}    


void ConvertFiles(const char * pPath, const char * pExtension, bool bTextFile)
{
  
  DIR *dir = NULL;
  struct dirent *de = NULL;

    
  dir = opendir(pPath);
  if ( dir == NULL )
  {
    printf("cannot open dir : %s\n",pPath); 
    return;
  }  

  int nExtensionLen = pExtension == NULL ? 0 : strlen(pExtension);
  while ( de = readdir(dir) )
  {
     int nNameLen = strlen(de->d_name);
     if (nNameLen > nExtensionLen && !strncmp((de->d_name + nNameLen - nExtensionLen), pExtension, nExtensionLen))
     {
       ConvertFile(pPath,de->d_name,pExtension,bTextFile);
     }  
  }      
  closedir(dir);

}  

int main(int argc, char** argv)
{

  
  char * settings_path = getenv("dm_setting_root");
  char * plugin_path = getenv("dm_setting_plugin");

  if ( settings_path == NULL && plugin_path == NULL )
  {
    printf("env variables are not set :dm_setting_root, dm_setting_plugin\n");
    return 0;
  }      

  if ( settings_path )
  {
      printf("dm settings : %s\n",settings_path);   

      char * s = (char*)strchr( (const char *)settings_path, ':' );
      if ( s ) 
        s = '\0';

      ConvertFiles(settings_path,"bmdf",false);
      ConvertFile(settings_path,"fstab",NULL,true);
  }
  if ( plugin_path )
  {
      printf("plugin settings : %s\n",plugin_path ); 
      ConvertFiles(plugin_path,"ini",true);
  }    
  return 0;
}
