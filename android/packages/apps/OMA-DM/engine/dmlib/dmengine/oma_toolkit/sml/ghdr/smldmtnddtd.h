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

/*--------------------------------------------------------------------------------------------------

   Header Name: smldmtnddtd.h

   General Description: OMA DM TND DTD specific type definitions   

--------------------------------------------------------------------------------------------------*/

#ifndef _SML_DM_TND_DTD_H
#define _SML_DM_TND_DTD_H

/* process only if we really use DM TND DTD */
#ifdef __USE_DMTND__

/*************************************************************************/
/*  Definitions                                                          */
/*************************************************************************/


#include <smldef.h>
#include <smldtd.h>

typedef struct sml_dmtnd_format_s {
   SmlPcdataPtr_t	value;  /* optional */
} *SmlDmTndFormatPtr_t, SmlDmTndFormat_t;

typedef struct sml_dmtnd_type_s {
   SmlPcdataPtr_t	mime;  /* optional */
   SmlPcdataPtr_t	ddfname;  /* optional */
} *SmlDmTndTypePtr_t, SmlDmTndType_t;

typedef struct sml_dmtnd_rtprops_s {
   SmlPcdataPtr_t	acl;    /* optional */
   SmlDmTndFormatPtr_t	format; /* optional */
   SmlPcdataPtr_t	name;   /* optional */
   SmlPcdataPtr_t	size;   /* optional */
   SmlPcdataPtr_t	title;  /* optional */
   SmlPcdataPtr_t	tstamp; /* optional */
   SmlDmTndTypePtr_t	type; 
   SmlPcdataPtr_t	verno;  /* optional */
} *SmlDmTndRTPropsPtr_t, SmlDmTndRTProps_t;

typedef struct sml_dmtnd_dfelement_s {
   SmlPcdataPtr_t	value;  /* optional */
} *SmlDmTndDFElementPtr_t, SmlDmTndDFElement_t;

typedef struct sml_dmtnd_dfprops_s {
   SmlDmTndDFElementPtr_t accesstype;
   SmlPcdataPtr_t	  defaultvalue;  /* optional */
   SmlPcdataPtr_t	  description;   /* optional */
   SmlDmTndFormatPtr_t	  dfformat;   
   SmlDmTndDFElementPtr_t occurrence;    /* optional */
   SmlDmTndDFElementPtr_t scope;         /* optional */
   SmlPcdataPtr_t	  dftitle;       /* optional */
   SmlDmTndTypePtr_t	  dftype;
   SmlDmTndDFElementPtr_t casesense; 
} *SmlDmTndDFPropsPtr_t, SmlDmTndDFProps_t;

typedef struct sml_dmtnd_node_list_s *SmlDmTndNodeListPtr_t;

typedef struct sml_dmtnd_node_s {
   SmlPcdataPtr_t	 nodename; 
   SmlPcdataPtr_t	 path;          /* optional */
   SmlDmTndRTPropsPtr_t  rtprops;      /* optional */
   SmlDmTndDFPropsPtr_t  dfprops;       /* optional */
   SmlPcdataPtr_t	 value;         /* optional */
   SmlDmTndNodeListPtr_t nodelist;          /* optional */
} *SmlDmTndNodePtr_t, SmlDmTndNode_t;

typedef struct sml_dmtnd_node_list_s {
   SmlDmTndNodePtr_t             node;
   struct sml_dmtnd_node_list_s *next;
} SmlDmTndNodeList_t;

typedef struct sml_dmtnd_s {
   SmlPcdataPtr_t	 verdtd;
   SmlPcdataPtr_t	 man;      /* optional */
   SmlPcdataPtr_t	 mod;      /* optional */
   SmlDmTndNodeListPtr_t nodelist; /* optional */
} *SmlDmTndPtr_t, SmlDmTnd_t;
 
#endif // __USE_DMTND__
#endif     /* _SML_DM_TND_DTD_H */
