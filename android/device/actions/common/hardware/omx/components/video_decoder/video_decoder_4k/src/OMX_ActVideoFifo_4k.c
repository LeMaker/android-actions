#include "OMX_ActVideoFifo_4k.h"
#include "omx_comp_debug_levels.h"
frame_buf_handle *raw_try_get_wbuf(port_t *port,unsigned int buf_size)
{
    raw_fifo_t *fifo_io = (raw_fifo_t *)port;
    buf_element_t *tmp_node,*cur_node;
    int node_flag = 0;
    unsigned int tmp_time = 0xffffffff;
    frame_buf_handle * empty_frame_buffer_handle = NULL;
    
    cur_node = fifo_io->node_temp;
    tmp_node = cur_node->next;
    dec_buf_t *vo_frame;

    
    fifo_io->node_for_decode = NULL;
 
    while(1) { //遍历，直到找到时间戳最小的空白帧
    	  vo_frame=tmp_node->buf_handle->vo_frame_info;
  
        if((vo_frame->display_flag == 0) 
                && (vo_frame->use_flag== 0 )) {
         
              fifo_io->node_for_decode = tmp_node; 
            	break;

        }
        if(tmp_node == cur_node){
            if(fifo_io->node_for_decode == NULL) {
                node_flag = 1;
            }
            break;
        }
        tmp_node = tmp_node->next;
    }

    if(fifo_io->node_for_decode != NULL) {
        fifo_io->av_buf_full = fifo_io->node_for_decode->buf_handle;

     }
   if(node_flag==1){
   		empty_frame_buffer_handle = NULL;
   }else{
   		fifo_io->node_temp = fifo_io->node_temp->next;
   		empty_frame_buffer_handle = fifo_io->av_buf_full;
   }
   return empty_frame_buffer_handle;

}
int try_get_wbuf_timeout(av_buf_t **raw_buf_dec, port_t *port, int *suspend_flag,int to_ms)
{
	int rt;
	raw_fifo_t *fifo_io = (raw_fifo_t *)port;
	struct timeval now;
	struct timespec timeout;
	
	*suspend_flag = 1;

	pthread_mutex_lock(&fifo_io->mutex);	
	*raw_buf_dec = port->try_get_wbuf(port, 0);
	if (*raw_buf_dec != NULL) {
		pthread_mutex_unlock(&fifo_io->mutex);
		*suspend_flag = 0;
		return 0;
	}


	gettimeofday(&now, NULL);
	timeout.tv_sec = now.tv_sec;
	timeout.tv_nsec = now.tv_usec * 1000 + to_ms*1000000;
		
	rt = pthread_cond_timedwait(&fifo_io->wbuf_cond, &fifo_io->mutex, &timeout);

//	if (rt == ETIMEDOUT) {
//		DEBUG(DEB_LEV_FULL_SEQ,"wbuf timedout\n");
//	}
	*raw_buf_dec = port->try_get_wbuf(port, 0);
	pthread_mutex_unlock(&fifo_io->mutex);
	*suspend_flag = 0;
	return rt;
}
frame_buf_handle *raw_get_wbuf(port_t *port,unsigned int buf_size)
{
   raw_fifo_t *fifo_io = (raw_fifo_t *)port;
   int ret=0;
   frame_buf_handle *empty_buffer=NULL;
   ret = pthread_mutex_lock(&fifo_io->mutex);
   if(ret != 0)
   {
        DEBUG(DEB_LEV_FULL_SEQ,"%s pthread_mutex_lock err\n",__FILE__);
   }
   empty_buffer =	fifo_io->av_buf_full;
   ret = pthread_mutex_unlock(&fifo_io->mutex);
   if(ret != 0)
   {
        DEBUG(DEB_LEV_FULL_SEQ,"%s pthread_mutex_lock err\n",__FILE__);
   }
   return empty_buffer;
   //return fifo_io->av_buf_full;
}
int get_rbuf_num(port_t *port)
{
	  raw_fifo_t *fifo_io = (raw_fifo_t *)port;
    buf_element_t *cur_node ,*tmp_node;
    int node_flag = 0;
    unsigned int tmp_time = 0xffffffff;
    dec_buf_t *vo_frame;
    int ret=0;
    int num=0;
    
   ret = pthread_mutex_lock(&fifo_io->mutex);
   if(ret != 0)
   {
        DEBUG(DEB_LEV_FULL_SEQ,"%s pthread_mutex_lock err\n",__FILE__);
   }	
    cur_node = fifo_io->node_empty;
    tmp_node = cur_node->next;	
   while(1)
   {

        vo_frame=tmp_node->buf_handle->vo_frame_info;
        if(vo_frame->display_flag == 1)
        {   
        	    num++;
       	}

        if(tmp_node == cur_node)
        {
   
            break;
        }
    
        tmp_node = tmp_node->next;
        
   }
   ret = pthread_mutex_unlock(&fifo_io->mutex);
   if(ret != 0)
   {
        DEBUG(DEB_LEV_FULL_SEQ,"%s pthread_mutex_lock err\n",__FILE__);
   }	
   return num;
}
frame_buf_handle *raw_get_rbuf(port_t *port)
{
    raw_fifo_t *fifo_io = (raw_fifo_t *)port;
    buf_element_t *cur_node ,*tmp_node;
    int node_flag = 0;
    unsigned int tmp_time = 0xffffffff;
    dec_buf_t *vo_frame;
    int ret=0;
    frame_buf_handle *frame_buf_handle_display=NULL;
    
   ret = pthread_mutex_lock(&fifo_io->mutex);
   if(ret != 0)
   {
        DEBUG(DEB_LEV_FULL_SEQ,"%s pthread_mutex_lock err\n",__FILE__);
   }	
    fifo_io->node_for_display = NULL;
    cur_node = fifo_io->node_empty;
    tmp_node = cur_node->next;

//    raw_fifo_dump_info(port);

    while(1) {

        vo_frame=tmp_node->buf_handle->vo_frame_info;
    //  DEBUG(DEB_LEV_FULL_SEQ,"====phy_addr is %x====\n",tmp_node->buf_handle->phy_addr);

//       if(vo_frame==NULL)
//       	{
//       		DEBUG(DEB_LEV_FULL_SEQ,"%s %d\n",__FUNCTION__,__LINE__);
//       	}
        if( vo_frame->display_flag == 1)
        {   

            if(vo_frame->time_stamp<tmp_time)
            {
//            	     DEBUG(DEB_LEV_FULL_SEQ,"%s %d\n",__FUNCTION__,__LINE__);
	                tmp_time = vo_frame->time_stamp;
	                fifo_io->node_for_display = tmp_node;	                
            }
            //seek后，rv34会给吐出上一次的一帧，
            //中间件raw_fifo_reset后会把ts置为-1，而此时display_flag却为1
            //所以碰到此情况硬性置0
            else
            {
//                DEBUG(DEB_LEV_FULL_SEQ,"%s %d\n",__FUNCTION__,__LINE__);
                if(vo_frame->time_stamp == 0xffffffff)
                {
 //                   DEBUG(DEB_LEV_FULL_SEQ,"%s %d\n",__FUNCTION__,__LINE__);
                    vo_frame->display_flag = 0;
                    
                }
            }
        }
 //       DEBUG(DEB_LEV_FULL_SEQ,"%s %d\n",__FUNCTION__,__LINE__);
        if(tmp_node == cur_node)
        {
    //        DEBUG(DEB_LEV_FULL_SEQ,"%s %d\n",__FUNCTION__,__LINE__);
            if(fifo_io->node_for_display == NULL)
            {
      //          DEBUG(DEB_LEV_FULL_SEQ,"%s %d\n",__FUNCTION__,__LINE__);
                node_flag = 1;
            }
      //      DEBUG(DEB_LEV_FULL_SEQ,"%s %d\n",__FUNCTION__,__LINE__);
            break;
        }
     //   DEBUG(DEB_LEV_FULL_SEQ,"%s %d\n",__FUNCTION__,__LINE__);
        tmp_node = tmp_node->next;
        
    }
    
    if(fifo_io->node_for_display != NULL)
    {
     //  DEBUG(DEB_LEV_FULL_SEQ,"%s %d\n",__FUNCTION__,__LINE__);
        frame_buf_handle_display=fifo_io->av_buf_empty= fifo_io->node_for_display->buf_handle;
    }
   if(node_flag == 1)
   	{
   		 frame_buf_handle_display=NULL;
   	}
   ret = pthread_mutex_unlock(&fifo_io->mutex);
   if(ret != 0)
   {
        DEBUG(DEB_LEV_FULL_SEQ,"%s pthread_mutex_lock err\n",__FILE__);
   }
   return frame_buf_handle_display;
//    if(node_flag == 1)
//    {
//     //   DEBUG(DEB_LEV_FULL_SEQ,"%s %d\n",__FUNCTION__,__LINE__);
//        return NULL;
//    }
//    else
//    { //  DEBUG(DEB_LEV_FULL_SEQ,"%s %d\n",__FUNCTION__,__LINE__);
//        return fifo_io->av_buf_empty;
//    }
}
void raw_fifo_dump_info(port_t *port)
{
	 raw_fifo_t *fifo_io = (raw_fifo_t *)port;
	 buf_element_t *cur_node ,*tmp_node;
	 dec_buf_t *vo_frame;
	 
   int ret=0;
    
   ret = pthread_mutex_lock(&fifo_io->mutex);
   if(ret != 0)
   {
        DEBUG(DEB_LEV_ERR,"%s pthread_mutex_lock err\n",__FILE__);
   }	
   cur_node = fifo_io->node_full;
   tmp_node = cur_node->next;
   while(1)
   {
   	 DEBUG(DEB_LEV_ERR,"buf_addr:%x,vo_frame_addr:%x,u:%d,d:%d \n",tmp_node->buf_handle->phy_addr,tmp_node->buf_handle->vo_frame_info,tmp_node->buf_handle->vo_frame_info->use_flag,tmp_node->buf_handle->vo_frame_info->display_flag);   	 
   	 if(tmp_node == cur_node)
     {
     	break;
     }
     tmp_node = tmp_node->next;
   }
    ret = pthread_mutex_unlock(&fifo_io->mutex);
   if(ret != 0)
   {
        DEBUG(DEB_LEV_FULL_SEQ,"%s pthread_mutex_lock err\n",__FILE__);
   }	
   
}

void raw_put_rbuf(port_t *port)
{    
    raw_fifo_t *fifo_io = (raw_fifo_t *)port;
    dec_buf_t *vo_frame;
    int ret=0;
    
   ret = pthread_mutex_lock(&fifo_io->mutex);
   if(ret != 0)
   {
        DEBUG(DEB_LEV_FULL_SEQ,"%s pthread_mutex_lock err\n",__FILE__);
   }	
   if(fifo_io->node_for_display != NULL)
   {
        vo_frame=fifo_io->node_for_display->buf_handle->vo_frame_info;
        vo_frame->display_flag = 0;
        vo_frame->reserved2=0;//
   }else
   {
    	vo_frame=fifo_io->node_for_decode->buf_handle->vo_frame_info;
    	vo_frame->reserved2=0;//
    	DEBUG(DEB_LEV_FULL_SEQ,"=====only h264 is valid,,in %s =====\n",__FILE__);
   }
	 ret = pthread_mutex_unlock(&fifo_io->mutex);
   if(ret != 0)
   {
        DEBUG(DEB_LEV_FULL_SEQ,"%s pthread_mutex_lock err\n",__FILE__);
   }	

    return ;    
}

void raw_fifo_reset(port_t *port)
{
    raw_fifo_t *fifo_io = (raw_fifo_t *)port;
    buf_element_t *cur_node ,*tmp_node;
    dec_buf_t *vo_frame;
    int ret=0;
    
   ret = pthread_mutex_lock(&fifo_io->mutex);
   if(ret != 0)
   {
        DEBUG(DEB_LEV_FULL_SEQ,"%s pthread_mutex_lock err\n",__FILE__);
   }	
    cur_node = fifo_io->node_full;
    tmp_node = cur_node->next;
    vo_frame = cur_node->buf_handle->vo_frame_info;
    
  
    vo_frame->display_flag=0;
    vo_frame->use_flag=0;
    vo_frame->time_stamp=0xffffffff;
  


    
    while(tmp_node != cur_node)
    {

        vo_frame=tmp_node->buf_handle->vo_frame_info; 
        vo_frame->display_flag=0;
    		vo_frame->use_flag=0;
    		vo_frame->time_stamp=0xffffffff;   			
        tmp_node = tmp_node->next;
    }
   fifo_io->node_empty = fifo_io->node_full;
   ret = pthread_mutex_unlock(&fifo_io->mutex);
   if(ret != 0)
   {
        DEBUG(DEB_LEV_FULL_SEQ,"%s pthread_mutex_lock err\n",__FILE__);
   }	
    return ;
}
void raw_fifo_timeout_wakeup(port_t *port,int *suspend_flag)
{
  raw_fifo_t *fifo_io = (raw_fifo_t *)port;
	if(*suspend_flag == 1)
	{
		pthread_mutex_lock(&fifo_io->mutex);		
		pthread_cond_signal(&fifo_io->wbuf_cond);
		pthread_mutex_unlock(&fifo_io->mutex);		
		*suspend_flag = 0;
	}	
}
#define PRINTF_RAW_STATUS
int get_fb_fifo_status(port_t *port)
{
    raw_fifo_t *fifo_io = (raw_fifo_t *)port;
    buf_element_t *cur_node ,*tmp_node;
    int num_display = 0;
    unsigned int tmp_time = 0xffffffff;
    dec_buf_t *vo_frame;
    
    cur_node = fifo_io->node_full;
    tmp_node = cur_node->next;    
    
    if(cur_node->buf_handle != NULL)
    {
    	vo_frame = cur_node->buf_handle->vo_frame_info;
#ifdef PRINTF_RAW_STATUS
    	DEBUG(DEB_LEV_FULL_SEQ,"addr:%x d:%x u:%x ts:%d \n",
    				cur_node->buf_handle->phy_addr,
    				vo_frame->display_flag,
    				vo_frame->use_flag,
    				vo_frame->time_stamp
    				);    
#endif            
        if(vo_frame->display_flag == 1)
        {
            if(vo_frame->time_stamp < tmp_time)
            num_display++;
            else
            {
               
                    vo_frame->display_flag = 0;

            }
        }
    }
    while(tmp_node != cur_node)
    {
        if(tmp_node->buf_handle != NULL)
        {
        	vo_frame = tmp_node->buf_handle->vo_frame_info;
#ifdef PRINTF_RAW_STATUS            
        	DEBUG(DEB_LEV_FULL_SEQ,"addr:%x d:%x u:%x ts:%d \n",
        		tmp_node->buf_handle->phy_addr,
    				vo_frame->display_flag,
    				vo_frame->use_flag,
    				vo_frame->time_stamp);     
#endif                
            if(vo_frame->display_flag == 1)
            {
                if(vo_frame->time_stamp<tmp_time)
                {
                    num_display++;
                }
                else
                {
                  
                        vo_frame->display_flag = 0;
                }
            }
        }
        
        tmp_node = tmp_node->next;
    }    

    return num_display;

}

int raw_fifo_init(port_t *port,int total_buf_num,unsigned int size){
	  raw_fifo_t *fifo_io = (raw_fifo_t *)port;
    int ret=0;
    int frame_buffer_number=0;
    int i = 0,j = 0;
    long phy_addr;
    
   
    ret = pthread_mutex_lock(&fifo_io->mutex);
    if(ret != 0)
    {
        DEBUG(DEB_LEV_FULL_SEQ,"%s pthread_mutex_lock err\n",__FILE__);
    }	
    if(fifo_io->cur_num_buffers == 0) {
    	fifo_io->node_empty = fifo_io->node_full = NULL;
    }

    i = fifo_io->cur_num_buffers;
    if(fifo_io->buf_element[i]->buf_handle != NULL) {  
    	
    	fifo_io->buf_element[i]->buf_handle->size = size;
      fifo_io->buf_element[i]->buf_handle->vir_addr=(unsigned char*)actal_malloc_cached_manual(size,&phy_addr);
      if(fifo_io->buf_element[i]->buf_handle->vir_addr==NULL){
      	pthread_mutex_unlock(&fifo_io->mutex);
      	DEBUG(DEB_LEV_ERR,"memory is not enough\n");
      	return -1;
      }
    	fifo_io->buf_element[i]->buf_handle->phy_addr =phy_addr;
    }

   

		if(i == 0) {
			fifo_io->node_empty = fifo_io->buf_element[i]; //保存首node
		}
		fifo_io->buf_element[i]->next = fifo_io->node_full;
		fifo_io->node_full = fifo_io->buf_element[i];

		fifo_io->cur_num_buffers++;

		if(fifo_io->cur_num_buffers == total_buf_num){
		//链表结尾和开头连接
		fifo_io->node_empty->next = fifo_io->node_full;

		//空满fifo变量同步
		fifo_io->node_empty = fifo_io->node_full;
		fifo_io->node_temp = fifo_io->node_full;
	}
	 frame_buffer_number=fifo_io->cur_num_buffers;
   ret = pthread_mutex_unlock(&fifo_io->mutex);
   if(ret != 0)
   {
        DEBUG(DEB_LEV_FULL_SEQ,"%s pthread_mutex_lock err\n",__FILE__);
   }
    return frame_buffer_number;
}


int raw_fifo_dispose(port_t *port)
{
    raw_fifo_t *fifo_io = (raw_fifo_t *)port;    
    unsigned int i;
    int rt=0;
    int ret=0;
    ret = pthread_mutex_lock(&fifo_io->mutex);
   if(ret != 0)
   {
        DEBUG(DEB_LEV_FULL_SEQ,"%s pthread_mutex_lock err\n",__FILE__);
   }	
   DEBUG(DEB_LEV_FULL_SEQ,"===raw_fifo_dispose start===\n");
    for(i = 0;i<FB_FIFO_MAX_BUF_NUM;i++) {
        if(fifo_io->buf_element[i] != NULL) {
        	if(fifo_io->buf_element[i]->buf_handle != NULL) {
		        		actal_free(fifo_io->buf_element[i]->buf_handle->vo_frame_info);
		        		fifo_io->buf_element[i]->buf_handle->vo_frame_info = NULL;
		        		if(fifo_io->buf_element[i]->buf_handle->vir_addr!=NULL){            	
                	actal_free_cached_manual(fifo_io->buf_element[i]->buf_handle->vir_addr);
                }
                fifo_io->buf_element[i]->buf_handle->vir_addr=NULL;
                actal_free(fifo_io->buf_element[i]->buf_handle);
                fifo_io->buf_element[i]->buf_handle = NULL;		
            }
            actal_free(fifo_io->buf_element[i]);
            fifo_io->buf_element[i] = NULL;
        }
    }
   ret = pthread_mutex_unlock(&fifo_io->mutex);
   if(ret != 0)
   {
        DEBUG(DEB_LEV_FULL_SEQ,"%s pthread_mutex_lock err\n",__FILE__);
   }	
    rt = pthread_mutex_destroy(&fifo_io->mutex);
    if(rt != 0)
    {
        DEBUG(DEB_LEV_FULL_SEQ,"%s pthread_mutex_destroy err\n",__FILE__);
    }
    rt =pthread_cond_destroy(&fifo_io->wbuf_cond);
    if(rt != 0)
    {
        DEBUG(DEB_LEV_FULL_SEQ,"%s pthread_cond_destroy err\n",__FILE__);
    }
   DEBUG(DEB_LEV_FULL_SEQ,"===raw_fifo_dispose end===\n");
    actal_free(fifo_io);
    fifo_io = NULL;
    
    return 0;
}

port_t *raw_fifo_open(void)
{
    unsigned int i;
    int rt;
    raw_fifo_t *fifo_io;    

    fifo_io = (raw_fifo_t *)actal_malloc(sizeof(raw_fifo_t));
    if(fifo_io == NULL)
    {
    	DEBUG(DEB_LEV_FULL_SEQ,"malloc raw fifo_io failed! \n");
        return NULL;
    }
    actal_memset(fifo_io,0,sizeof(raw_fifo_t));
    
    for (i = 0; i < FB_FIFO_MAX_BUF_NUM; i++)
    {
        fifo_io->buf_element[i] = (buf_element_t *) actal_malloc(sizeof(buf_element_t));
        if(fifo_io->buf_element[i] == NULL)
        {
        	DEBUG(DEB_LEV_FULL_SEQ,"malloc raw buf_element failed! \n");
            goto raw_fifo_open_err;        
        }

        actal_memset(fifo_io->buf_element[i], 0, sizeof(buf_element_t));

        fifo_io->buf_element[i]->size = 0;
        fifo_io->buf_element[i]->buf_handle =(frame_buf_handle*) actal_malloc(sizeof(frame_buf_handle));  
        actal_memset(fifo_io->buf_element[i]->buf_handle,0,sizeof(frame_buf_handle));
        fifo_io->buf_element[i]->buf_handle->vo_frame_info=(dec_buf_t*)actal_malloc(sizeof(dec_buf_t));
        actal_memset(fifo_io->buf_element[i]->buf_handle->vo_frame_info,0,sizeof(dec_buf_t));
       

    }    
    
    rt = pthread_mutex_init(&fifo_io->mutex,NULL);
    if(rt != 0)
    {
        DEBUG(DEB_LEV_FULL_SEQ,"%s pthread_mutex_init err\n",__FILE__);
        goto raw_fifo_open_err;
    }
    rt = pthread_cond_init(&fifo_io->wbuf_cond,NULL);
    if(rt != 0)
    {
        DEBUG(DEB_LEV_FULL_SEQ,"%s pthread_cond_init err\n",__FILE__);
        goto raw_fifo_open_err;
    }
    fifo_io->port.get_wbuf = raw_get_wbuf;
    fifo_io->port.try_get_wbuf = raw_try_get_wbuf;
    fifo_io->port.put_wbuf = NULL;
    fifo_io->port.get_rbuf = raw_get_rbuf;
    fifo_io->port.put_rbuf = raw_put_rbuf;
   	fifo_io->port.dump_info= raw_fifo_dump_info;
   	fifo_io->port.get_rbuf_num = get_rbuf_num;
    
    fifo_io->port.put_more_wbuf = NULL;
    fifo_io->port.fifo_init = NULL;
    fifo_io->port.fifo_reset = raw_fifo_reset;
    fifo_io->port.fifo_wakeup = NULL;  
    
    return &fifo_io->port;
    
    raw_fifo_open_err:

    for(i = 0;i<FB_FIFO_MAX_BUF_NUM;i++)
    {
        if(fifo_io->buf_element[i] != NULL)
        {
            actal_free(fifo_io->buf_element[i]->buf_handle->vo_frame_info);
            actal_free(fifo_io->buf_element[i]->buf_handle);
            actal_free(fifo_io->buf_element[i]);
            
            fifo_io->buf_element[i] = NULL;
        }
    }

    actal_free(fifo_io);
    fifo_io = NULL;

    return NULL;
}

void *fb_get_wbuf(struct fb_port_s *fb_port,unsigned int buf_size)
{
    fb_fifo_io_t *fb_fifo_io = (fb_fifo_io_t *)fb_port;    
    frame_buf_handle *av_buf;
    av_buf = fb_fifo_io->port->get_wbuf(fb_fifo_io->port,buf_size);    
    return (void *)av_buf;
}


int fb_fifo_dispose(fb_port_t *fb_port)
{
    int ret;
    fb_fifo_io_t *fb_fifo_io = (fb_fifo_io_t *)fb_port;
    
    ret = raw_fifo_dispose(fb_fifo_io->port);
    if(fb_fifo_io != NULL)
    {
        actal_free(fb_fifo_io);
        fb_fifo_io = NULL;
    }
    return ret;
}

fb_port_t *fb_fifo_open(void **fb_vo)
{
    fb_fifo_io_t *fb_fifo_io;    
    
    fb_fifo_io = (fb_fifo_io_t *)actal_malloc(sizeof(fb_fifo_io_t));
    if(fb_fifo_io == NULL)
    {
    	DEBUG(DEB_LEV_FULL_SEQ,"malloc fb_fifo_io->port failed! \n");
        return NULL;
    }
    actal_memset(fb_fifo_io,0,sizeof(fb_fifo_io_t));

    fb_fifo_io->port = raw_fifo_open();
    if(fb_fifo_io->port == NULL)
    {
    	DEBUG(DEB_LEV_FULL_SEQ,"malloc fb_fifo_io->port failed! \n");
        actal_free(fb_fifo_io);
        fb_fifo_io = NULL;
        return NULL;
    }
    *fb_vo = fb_fifo_io->port;
    
    fb_fifo_io->fb_port.get_wbuf = fb_get_wbuf;
    
    return &fb_fifo_io->fb_port;
}
