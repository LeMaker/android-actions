#ifndef __SET_SERIAL_NUMBER_H__
#define __SET_SERIAL_NUMBER_H__

#define MAX_MISCINFO_LEN	(1024*1024)

extern int set_serial_number(char *buf, int len);
extern int get_serial_number(char *buf, int len);
extern int get_serial_number_len(void);


extern int set_miscinfo(char *buf, int len);
extern int get_miscinfo(char *buf);

extern void set_upgrade_flags_and_restart(void);
extern int gadget_andorid_shutdown_machine(void);


struct chipID_packet {
	__le32	len;
	__u8	data[0];
};

extern int get_chipID_len(void);
extern int get_chipID(char *buf, int len);


#endif
