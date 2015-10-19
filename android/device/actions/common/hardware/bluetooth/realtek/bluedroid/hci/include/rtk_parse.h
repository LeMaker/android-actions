#ifndef RTK_PARSE_H
#define RTK_PARSE_H

#include <stdlib.h>

/******************************************************************************
**  Constants & Macros
******************************************************************************/
#define HOST_PROFILE_INFO

/******************************************************************************
**  Type definitions
******************************************************************************/
typedef unsigned char   UINT8;
#define BD_ADDR_LEN     6                   /* Device address length */
typedef UINT8 BD_ADDR[BD_ADDR_LEN];         /* Device address */

/******************************************************************************
**  Extern variables and functions
******************************************************************************/
extern uint8_t coex_log_enable;

/******************************************************************************
**  Functions
******************************************************************************/
void rtk_parse_internal_event_intercept(uint8_t *p);

void rtk_parse_l2cap_data(uint8_t *p, uint8_t direction);

void rtk_parse_init();

void rtk_parse_cleanup();

void rtk_parse_command(uint8_t *p);

void rtk_add_le_profile(BD_ADDR bdaddr, uint16_t handle, uint8_t profile_map);

void rtk_delete_le_profile(BD_ADDR bdaddr, uint16_t handle, uint8_t profile_map);

void rtk_add_le_data_count(uint8_t data_type);
#endif /* HCI_H */

