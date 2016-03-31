#ifndef __AOTG_UHOST_MON_H__
#define __AOTG_UHOST_MON_H__

void aotg_dev_plugout_msg(int id);
extern int port_host_plug_detect[2];
extern struct aotg_uhost_mon_t *aotg_uhost_mon0;
extern struct aotg_uhost_mon_t *aotg_uhost_mon1;

void aotg_uhost_mon_init(struct work_struct *w);
void aotg_uhost_mon_exit(void);
int usb2_set_dp_500k_15k(struct aotg_uhost_mon_t * umon, int enable_500k_up, int enable_15k_down);

#endif /* __AOTG_UHOST_MON_H__ */

