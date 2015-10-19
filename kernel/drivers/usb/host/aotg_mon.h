#ifndef __AOTG_UHOST_MON_H__
#define __AOTG_UHOST_MON_H__

void aotg_dev_plugout_msg(int id);
extern int port_host_plug_detect[2];

void aotg_uhost_mon_init(void);
void aotg_uhost_mon_exit(void);

#endif /* __AOTG_UHOST_MON_H__ */

