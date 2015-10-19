#include "dmt.hpp"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>


#include "plugin/dmtPlugin.hpp"

#define Debug printf
//#define Debug //

//---------------------------Declaration----------------------

extern "C" 
SYNCML_DM_RET_STATUS_T DMT_PluginLib_Execute2(
	const char * path, 
	DMMap<DMString, DMString> & mapParameters,
	const char * args, 
   CPCHAR szCorrelator, 
	PDmtTree tree,
	DMString & results
)
{
   if (args==NULL || strlen(args)==0 )
   {
      Debug("No arguments\n");
      results += ("No argument specified");
      return SYNCML_DM_SUCCESS;
   }

   Debug("execute path=%s args=%s\n", path, args);

   DMString cmd;
   cmd=args;
   cmd += " > ";   

   //cmd += "/tmp/dmt_execute_result.txt";
   char tmpn[100]="/tmp/dmt_execute_";
   //strcat(tmpn,tmpnam(NULL)); 
   tmpnam(tmpn); 
   //Debug("tmpn=%s\n", tmpn);

   cmd += tmpn;

   //int res=system(args);
   int res=system(cmd.c_str());
   //Debug("res=%d\n", res);

   //char  resstr[20];
   //sprintf(resstr, "res=%d\n", res);
   //results.append(resstr);

   FILE * fp=fopen(tmpn, "r+b");
   if (fp!=NULL)
   {
      //Debug("fp=0x%x\n", fp);
      char buf[1024];
      int n=fread(buf, 1, sizeof(buf)-1, fp);
      buf[n]=0;
      //Debug("bytes read=%d\n", n);
      if (n>=0)
      {
         results += (buf);
         //std::string s(buf);
         //results=s;
      }
      fclose(fp);

      //
      unlink(tmpn);
   } else 
   {
      results += ("No result");
   }
   return SYNCML_DM_SUCCESS;

}


extern "C" 
int DMT_PluginLib_GetAPIVersion(void)
{
   return DMT_PLUGIN_VERSION_1_1;
}

