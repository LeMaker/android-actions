/*
 * hdmi_edid.c
 *
 * HDMI OWL IP driver Library
 *
 * Copyright (C) 2014 Actions Corporation
 * Author: Guo Long  <guolong@actions-semi.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */
 
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/poll.h>

#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/ctype.h>
#include <linux/init.h>
#include <asm/atomic.h>
#include <linux/kthread.h>
#include <linux/miscdevice.h>
#include <linux/switch.h>
#include <linux/sysfs.h>

#include <mach/storage_access.h>

#include "hdmi_ip.h"
#include "hdmi.h"

#define owl_hdmi_write(a, b)  hdmi.ip_data.ops->write_reg(&hdmi.ip_data, b, a)
#define owl_hdmi_read(a)  hdmi.ip_data.ops->read_reg(&hdmi.ip_data, a)

/*HDCP authen*/
#define HDCP_XMT_LINK_H0    0
#define HDCP_XMT_LINK_H1    1
#define HDCP_XMT_AUTH_A0    2
#define HDCP_XMT_AUTH_A1    3
#define HDCP_XMT_AUTH_A2    4
#define HDCP_XMT_AUTH_A3    5
#define HDCP_XMT_AUTH_A4    6
#define HDCP_XMT_AUTH_A5    7
#define HDCP_XMT_AUTH_A6    8
#define HDCP_XMT_AUTH_A7    9
#define HDCP_XMT_AUTH_A8    10
#define HDCP_XMT_AUTH_A9    11
#define HDCP_XMT_AUTH_A9_1  12

#define HDCP_FAIL_TIMES       50

unsigned char key_r0[40][7];

unsigned char aksv[6] = { 
    0x10,  
    0x1e, 
    0xc8, 
    0x42, 
    0xdd, 
    0xd9 
};

//#define READ_HDCP_KEY_FROM_NATIVE

#ifdef READ_HDCP_KEY_FROM_NATIVE
char test_keyr0[40][15] ={
    "aa6197eb701e4e",
    "780717d2d425d3",
    "7c4f7efe39f44d",
    "1e05f28f0253bd",
    "d614c6ccb090ee",
    "82f7f2803ffefc",
    "4ae2ebe12741d7",
    "e203695a82311c",
    "4537b3e7f9f557",
    "b02d715e2961ed",
    "cd6e0ab9834016",
    "d64fd9edfdd35b",
    "3a13f170726840",
    "4f52fb759d6f92",
    "2afc1d08b4221f",
    "b99db2a29aea40",
    "2020da8eb3282a",
    "0b4c6aee4aa771",
    "edeef811ec6ac0",
    "8f451ad92112f3",
    "705e2fddc47e6e",
    "6efc72aaa3348f",
    "7debb76130fb0e",
    "5fd1a853c40f52",
    "70168fbeaeba3c",
    "a0cb8f3162d198",
    "5ed0a9eef0c1ed",
    "fa18541a02d283",
    "0661135d19feec",
    "da6b02ca363cf3",
    "dd7142f81e832f",
    "cd3490a48be99a",
    "0e08bceaeec81a",
    "9b0d183dbbbbf3",
    "7788f601c07e57",
    "4e7271a716e4aa",
    "f510b293bec62a",
    "16158c6a1f3c84",
    "33656236296f06",
    "a5438e67d72d80",
};
#else
unsigned char test_keyr0[40][15];
#endif

#define M0_LENGTH       8
#define BSTATUS_LENGTH  2
#define KSV_LENGTH      5
#define KSV_RESERVED  3
#define KEY_COL_LENGTH 7
#define KEY_ARRAY_LENGTH 40
#define KEY_LENGTH      280
#define VERIFY_LENGTH 20
#define MAX_SHA_1_INPUT_LENGTH  704

#define MISC_INFO_TYPE_HDCP             STORAGE_DATA_TYPE_HDCP

#define swapl(x)	((((x)&0x000000ff)<<24)+(((x)&0x0000ff00)<<8)+(((x)&0x00ff0000)>>8)+(((x)&0xff000000)>>24))
#define SXOLN(n,x)	((x<<n)|(x>>(32-n)))    //x leftrotate n
#define SXORN(n,x)	((x>>n)|(x<<(32-n)))    //x rightrotate n
#define hLen    20   //sha_1 160bit

int hdcp_timer_interval = 50;       
int hdcp_timer_error_interval = 100;

//store aksv key
void hdcp_set_aksv(unsigned char* _aksv) {
    int i = 0;
    for (i = 0; i < 5; i++) {
        aksv[i] = _aksv[i];
    }
}

//key
void hdcp_set_key(unsigned char *key) {
    int i = 0, j = 0;
    for (j = 0; j < 40; j++) {
        for (i = 0; i < 7; i++) {
            key_r0[j][i] = key[j * 7 + i];
        }
    }
}


int hdcp_anlay_SRM(unsigned char *srm, unsigned char *srm_buf) {
    unsigned int tmp = 0;
    int j = 0, i = 0, k = 0;
    if (srm != NULL) {
        tmp = srm[0];
    }
    if ((srm != NULL) && (srm_buf != NULL)) {
        if ((tmp & 0x80) != 0) { //hdcp flag
            tmp = srm[8];
            HDCP_DEBUG("there are %d device Number\n", tmp);
            for (i = 1; i <= (int) tmp; i++) {
                for (j = 4; j >= 0; j--) {
                    srm_buf[j + (i - 1) * 5] = srm[9 + k];
                    k++;
                }
            }

            return tmp;
        } else {
            return -1;
        }
    } else {
        return -1;
    }

}


/*
  * check if Bksv is valid, need have 20 "1"  and 20 "0"
  */
int Check_Bksv_Invalid(void) {
    int i, j;
    unsigned char counter = 0;
    unsigned char invalid_bksv[4][5] = {
        {0x0b,0x37,0x21,0xb4,0x7d},
        {0xf4,0xc8,0xde,0x4b,0x82},
        {0x23,0xde,0x5c,0x43,0x93},
        {0x4e,0x4d,0xc7,0x12,0x7c},
    };

    for (i = 0; i < 5; i++) {
        for (j = 0; j < 8; j++) {
            if (((hdmi.ip_data.hdcp.Bksv[i] >> j) & 0x1) != 0) {
                counter++;
            }
        }
    }

    if (counter != 20) {
        DEBUG_ERR("[%s]fail  0x%x 0x%x 0x%x 0x%x 0x%x\n", __func__, hdmi.ip_data.hdcp.Bksv[0], hdmi.ip_data.hdcp.Bksv[1], \
		hdmi.ip_data.hdcp.Bksv[2], hdmi.ip_data.hdcp.Bksv[3], hdmi.ip_data.hdcp.Bksv[4]);
        return 0;
    }

    for (i = 0; i < 4; i++) {
        for(j = 0; j < 5; j++) {
            if (invalid_bksv[i][j] != hdmi.ip_data.hdcp.Bksv[j]) {
                break;
            }
			if(j==4)
				return 0;
        }
    }

    HDCP_DEBUG("[%s]successful!\n", __func__);
    return 1;
}


unsigned char sha_1(unsigned char *sha_output, unsigned char *M_input, int len) {
    unsigned int Kt[4] = {
        0x5a827999, 0x6ed9eba1, 0x8f1bbcdc, 0xca62c1d6 
    };
    
    unsigned int h[5] = {
        0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476,0xc3d2e1f0 
    };

    int t, n, n_block;
    unsigned int Ft, x, temp;
    unsigned int A, B, C, D, E;
    int i, j;
    unsigned char *sha_input;
    unsigned char *out_temp;
    unsigned int W[80];
	sha_input=kmalloc(700, GFP_KERNEL);
	if(NULL==sha_input)
	{
		return ENOMEM;
	}
    memset(sha_input, 0, 700);

    for (i = 0; i < len; i++)
        sha_input[i] = M_input[i];

    /* do message padding */
    n_block = (len * 8 + 1 + 64) / 512 + 1;
    sha_input[len] = 0x80;
    HDCP_DEBUG("n_block=%d\n", n_block);
    /* set len */

    sha_input[(n_block - 1) * 64 + 60] = (unsigned char)(
            ((len * 8) & 0xff000000) >> 24);
    sha_input[(n_block - 1) * 64 + 61] = (unsigned char)(
            ((len * 8) & 0x00ff0000) >> 16);
    sha_input[(n_block - 1) * 64 + 62] = (unsigned char)(((len * 8) & 0x0000ff00) >> 8);
    sha_input[(n_block - 1) * 64 + 63] = (unsigned char)((len * 8) & 0x000000ff);

    for (j = 0; j < n_block; j++) {
        HDCP_DEBUG("\nBlock %d\n", j);
        for (i = 0; i < 64; i++) {
            if ((i % 16 == 0) && (i != 0))
                HDCP_DEBUG("\n0x%2x,", sha_input[j * 64 + i]);
            else
                HDCP_DEBUG("0x%2x,", sha_input[j * 64 + i]);
        }
    }
    HDCP_DEBUG("SHA sha_input end\n");

    for (n = 0; n < n_block; n++) {
        for (t = 0; t < 16; t++) {
            x = *((unsigned int *) &sha_input[n * 64 + t * 4]);
            W[t] = swapl(x);
//          if (t%2 == 0)
//              HDCP_DEBUG("\n");
//          HDCP_DEBUG("x=0x%x,W[%d]=0x%x\t",x,t,W[t]);
        }
//      HDCP_DEBUG("\n");   
        for (t = 16; t < 80; t++) {
            x = W[t - 3] ^ W[t - 8] ^ W[t - 14] ^ W[t - 16];
            W[t] = SXOLN(1, x);
//          HDCP_DEBUG("W[%d]=0x%x\t",t,W[t]);
//          if (t % 3 == 0)
//              HDCP_DEBUG("\n");
        }

        A = h[0];
        B = h[1];
        C = h[2];
        D = h[3];
        E = h[4];

        for (t = 0; t < 80; t++) {
            if (t >= 0 && t <= 19)
                Ft = (B & C) | ((~B) & D);
            else if (t >= 20 && t <= 39)
                Ft = B ^ C ^ D;
            else if (t >= 40 && t <= 59)
                Ft = (B & C) | (B & D) | (C & D);
            else
                Ft = (B ^ C ^ D);

            temp = SXOLN(5, A) + Ft + E + W[t] + Kt[t / 20]; //temp = S^5(A) + f(t;B,C,D) + E + W(t) + K(t)

            E = D;
            D = C;
            C = SXOLN(30, B); //C = S^30(B)
            B = A;
            A = temp;
        }

        h[0] += A; // H0 = H0 + A
        h[1] += B; // H1 = H1 + B
        h[2] += C; //H2 = H2 + C
        h[3] += D; //H3 = H3 + D
        h[4] += E; //H4 = H4 + E
    }

    HDCP_DEBUG("\noutput original sha_input:\n");
    for (i = 0; i < hLen / 4; i++)
        HDCP_DEBUG("0x%x\t", h[i]);

    HDCP_DEBUG("\nconvert to little endien\n");
    for (i = 0; i < hLen / 4; i++) {
        h[i] = swapl(h[i]);
        HDCP_DEBUG("0x%x\t", h[i]);
    }
    HDCP_DEBUG("\n");

    /* copy to output pointer */
    out_temp = (unsigned char *) h;
    HDCP_DEBUG("out_temp:\n");
    for (i = 0; i < hLen; i++) {
        sha_output[i] = out_temp[i];
        HDCP_DEBUG("0x%x\t", out_temp[i]);
    }
    HDCP_DEBUG("\n");
	kfree(sha_input);
    return 0;
}

int check_one_number(unsigned char data)
{
    int num = 0, i;
    for(i=0;i<8;i++) {
	if ((data >> i) & 0x1) {
		num ++;
	}
    }
    return num;
}

int hdcp_SetKm(unsigned char *key, int pnt) {
    unsigned int tmp;
    unsigned char dKey[11];
    dKey[0] = key[0] ^ pnt;
    dKey[1] = ~key[1] ^ dKey[0];
    dKey[2] = key[2] ^ dKey[1];
    dKey[3] = key[3] ^ dKey[2];
    dKey[4] = key[4] ^ dKey[3];
    dKey[5] = ~key[5] ^ dKey[4];
    dKey[6] = ~key[6] ^ dKey[5];
    /* write to HW */
    /*P_HDCP_DPKLR*/
    tmp = pnt | (dKey[0] << 8) | (dKey[1] << 16) | (dKey[2] << 24);
	//HDCP_DEBUG("gl+++ km %x\n", tmp);
    owl_hdmi_write(tmp, HDCP_DPKLR);
    /*P_HDCP_DPKMR*/
    tmp = dKey[3] | (dKey[4] << 8) | (dKey[5] << 16) | (dKey[6] << 24);
	//HDCP_DEBUG("gl+++ km %x\n", tmp);
    owl_hdmi_write(tmp, HDCP_DPKMR);

    /* trigger accumulation */

    while (!(owl_hdmi_read(HDCP_SR) & (1 << 3)))
        ;
    return 0;
}

int hdcp_ReadKsvList(unsigned char *Bstatus, unsigned char *ksvlist) {
    int cnt;

    /* get device count in Bstatus [6:0] */
    if (i2c_hdcp_read(Bstatus, 0x41, 2) < 0) {
        if (i2c_hdcp_read(Bstatus, 0x41, 2) < 0) {
            return 0;
        }
    }

    if (Bstatus[0] & 0x80) //if Max_devs_exceeded then quit
        return 0;

    if (Bstatus[1] & 0x8) //if Max_cascade_exceeded then quit
        return 0;

    cnt = Bstatus[0] & 0x7f;

    if (!cnt) {
        return 1;
    }

    if (i2c_hdcp_read(ksvlist, 0x43, 5 * cnt) < 0) {
        if (i2c_hdcp_read(ksvlist, 0x43, 5 * cnt) < 0) {
            return 0;
        }
    }

    return 1;
}

/* convert INT8 number to little endien number */
int c2ln14(unsigned char *num, char *a) {
    int i;
    int n = 14;
    for (i = 0; i < 11; i++) {
        num[i] = 0;
    }

    for (i = 0; i < n; i++) {
        if (i % 2) {
            if (a[n - i - 1] >= '0' && a[n - i - 1] <= '9') {
                num[i / 2] |= (a[n - i - 1] - '0') << 4;
            } else if (a[n - i - 1] >= 'a' && a[n - i - 1] <= 'f') {
                num[i / 2] |= (a[n - i - 1] - 'a' + 10) << 4;
            } else if (a[n - i - 1] >= 'A' && a[n - i - 1] <= 'F') {
                num[i / 2] |= (a[n - i - 1] - 'A' + 10) << 4;
            }
        } else {
            if (a[n - i - 1] >= '0' && a[n - i - 1] <= '9') {
                num[i / 2] |= (a[n - i - 1] - '0');
            } else if (a[n - i - 1] >= 'a' && a[n - i - 1] <= 'f') {
                num[i / 2] |= (a[n - i - 1] - 'a' + 10);
            } else if (a[n - i - 1] >= 'A' && a[n - i - 1] <= 'F') {
                num[i / 2] |= (a[n - i - 1] - 'A' + 10);
            }
        }
    }
    return 0;
}


extern int read_mi_item(char *name, void *buf, unsigned int count);

int hdcp_read_key(void)
{
    int array, col, index;
    int num = 0;
    int i = 0;
	int ret;
    unsigned char key[308];
    unsigned char sha1_verify[20];
    unsigned char sha1_result[20];
	
#ifdef READ_HDCP_KEY_FROM_NATIVE
	return 0;
#endif		

    ret =  read_mi_item("HDCP", key, sizeof(key));
    if (ret < 0) {
        DEBUG_ERR("failed to read hdcp key from secure storage\n");
		return ret;
    }
	printk("hdcp key cnt %d\n", ret);

    for (i =1; i < sizeof(aksv); i++) {
        aksv[i] = key[i -1];
    }

    for (i = 0; i < KSV_LENGTH; i++) {
        num += check_one_number(key[i]);
    }
    
    //key
    if (num == 20) {
    	HDCP_DEBUG("aksv is valid\n");
		HDCP_DEBUG("hdcp key is as follows:\n");
		for (array = 0; array < KEY_ARRAY_LENGTH; array++) {
			 for (col = 0; col < KEY_COL_LENGTH; col++) {
				index = (KSV_LENGTH + KSV_RESERVED + (array + 1) * KEY_COL_LENGTH) - col - 1;
				sprintf(&test_keyr0[array][2 * col], "%x", ((key[index] & 0xf0) >> 4) & 0x0f);
				sprintf(&test_keyr0[array][2 * col +1], "%x", key[index] & 0x0f);

				HDCP_DEBUG("key_tmp[%d][%d]:%c\n", array, 2 * col, test_keyr0[array][2 * col]);
				HDCP_DEBUG("key_tmp[%d][%d]:%c\n", array, 2 * col + 1, test_keyr0[array][2 * col + 1]);
			}
			HDCP_DEBUG("%s\n", test_keyr0[array]);
		}
        HDCP_DEBUG("\n\nhdcp key parse finished\n\n");
        HDCP_DEBUG("verify code is as follows:\n");
        for (i = 0; i < sizeof(sha1_verify); i++) {
	        index = i + KSV_LENGTH + KSV_RESERVED + KEY_LENGTH;
            sha1_verify[i] = key[index];
	        HDCP_DEBUG("sha1_verify[%d]:%x\n", i, sha1_verify[i]);
        }
        HDCP_DEBUG("verify code parse finished\n\n");
        if(sha_1(sha1_result, key, KSV_LENGTH + KSV_RESERVED + KEY_LENGTH))
        {
			HDCP_DEBUG("aksv kmalloc mem is failed\n");
			return 1;
	    }

    } else {
    	HDCP_DEBUG("aksv is invalid\n");
        return 1;
    }
    
    //verify
    for(i = 0; i < VERIFY_LENGTH; i++) {
		if (sha1_verify[i] != sha1_result[i]) {
			DEBUG_ERR("sha1 verify error!\n");
			return 1;
		}
    }
    HDCP_DEBUG("[%s]sha1 verify success !\n", __func__);
    return 0; 
}

int do_Vmatch(struct hdmi_ip_data *ip_data, unsigned char *v, unsigned char *ksvlist, unsigned char *bstatus,
        unsigned char * m0) {
    unsigned int tmp;
    int data_counter;
    unsigned char sha_1_input_data[MAX_SHA_1_INPUT_LENGTH];
    int nblock, llen;
    int cnt2 = ip_data->hdcp.Bstatus[0] & 0x7f;
    int i, j;
    volatile int hdcp_shacr = 0;

    llen = 8 * M0_LENGTH + 8 * BSTATUS_LENGTH + cnt2 * 8 * KSV_LENGTH;

    for (i = 0; i < MAX_SHA_1_INPUT_LENGTH; i++)
        sha_1_input_data[i] = 0;

    for (data_counter = 0;
            data_counter < cnt2 * KSV_LENGTH + BSTATUS_LENGTH + M0_LENGTH;
            data_counter++) {
        if (data_counter < cnt2 * KSV_LENGTH) {
            sha_1_input_data[data_counter] = ksvlist[data_counter];

        } else if ((data_counter >= cnt2 * KSV_LENGTH)
                && (data_counter < cnt2 * KSV_LENGTH + BSTATUS_LENGTH)) {
            sha_1_input_data[data_counter] = bstatus[data_counter
                    - (cnt2 * KSV_LENGTH)];
        } else {
            sha_1_input_data[data_counter] = m0[data_counter
                    - (cnt2 * KSV_LENGTH + BSTATUS_LENGTH)];
        }
    }
    sha_1_input_data[data_counter] = 0x80; //block ending signal       

    nblock = (int) (data_counter / 64);
    sha_1_input_data[nblock * 64 + 62] = (unsigned char) (((data_counter * 8)
            >> 8) & 0xff); //total SHA counter high
    sha_1_input_data[nblock * 64 + 63] = (unsigned char) ((data_counter * 8)
            & 0xff); //total SHA counter low

    //  P_HDCP_SHACR |= 0x1;    //reset SHA write pointer
    tmp = owl_hdmi_read(HDCP_SHACR);
    owl_hdmi_write(tmp | 0x1, HDCP_SHACR);

    while (owl_hdmi_read(HDCP_SHACR) & 0x1); //wait completing reset operation
    //  P_HDCP_SHACR |= 0x2;            //set new SHA-1 operation
    tmp = owl_hdmi_read(HDCP_SHACR);
    owl_hdmi_write(tmp | 0x2, HDCP_SHACR);

    for (i = 0; i < nblock; i++) {
        for (j = 0; j < 16; j++) {
            //P_HDCP_SHADR 
            tmp = (sha_1_input_data[i * 64 + (j * 4 + 0)] << 24)
                    | (sha_1_input_data[i * 64 + (j * 4 + 1)] << 16)
                    | (sha_1_input_data[i * 64 + (j * 4 + 2)] << 8)
                    | (sha_1_input_data[i * 64 + (j * 4 + 3)]);
            owl_hdmi_write(tmp, HDCP_SHADR);
            owl_hdmi_read(HDCP_SHADR);
        }
        //      P_HDCP_SHACR |= 0x4;         //Start 512bit SHA operation           
        tmp = owl_hdmi_read(HDCP_SHACR);
        owl_hdmi_write(tmp | 0x4, HDCP_SHACR);
        while (!(owl_hdmi_read(HDCP_SHACR) & 0x8))
            ; //after 512bit SHA operation, this bit will be set to 1

        //P_HDCP_SHACR &= 0xfd;                       //clear SHAfirst bit
        tmp = owl_hdmi_read(HDCP_SHACR);
        owl_hdmi_write(tmp & 0xfd, HDCP_SHACR);
        owl_hdmi_read(HDCP_SHACR);
    }
    for (j = 0; j < 16; j++) {
        // P_HDCP_SHADR 
        tmp = (sha_1_input_data[nblock * 64 + (j * 4 + 0)] << 24)
                | (sha_1_input_data[nblock * 64 + (j * 4 + 1)] << 16)
                | (sha_1_input_data[nblock * 64 + (j * 4 + 2)] << 8)
                | (sha_1_input_data[nblock * 64 + (j * 4 + 3)]);
        owl_hdmi_write(tmp, HDCP_SHADR);
        owl_hdmi_read(HDCP_SHADR);
    }
    //P_HDCP_SHACR |= 0x4;                        //Start 512bit SHA operation        
    tmp = owl_hdmi_read(HDCP_SHACR);
    owl_hdmi_write(tmp | 0x4, HDCP_SHACR);

    while (!(owl_hdmi_read(HDCP_SHACR) & 0x8))
        ; //after 512bit SHA operation, this bit will be set to 1
    //write V
    //P_HDCP_SHADR 
    tmp = (v[3] << 24) | (v[2] << 16) | (v[1] << 8) | (v[0] << 0);
    owl_hdmi_write(tmp, HDCP_SHADR);
    owl_hdmi_read(HDCP_SHADR);

    //P_HDCP_SHADR 
    tmp = (v[7] << 24) | (v[6] << 16) | (v[5] << 8) | (v[4] << 0);
    owl_hdmi_write(tmp, HDCP_SHADR);
    owl_hdmi_read(HDCP_SHADR);

    //P_HDCP_SHADR 
    tmp = (v[11] << 24) | (v[10] << 16) | (v[9] << 8) | (v[8] << 0);
    owl_hdmi_write(tmp, HDCP_SHADR);
    owl_hdmi_read(HDCP_SHADR);

    //P_HDCP_SHADR 
    tmp = (v[15] << 24) | (v[14] << 16) | (v[13] << 8) | (v[12] << 0);
    owl_hdmi_write(tmp, HDCP_SHADR);
    owl_hdmi_read(HDCP_SHADR);

    //P_HDCP_SHADR 
    tmp = (v[19] << 24) | (v[18] << 16) | (v[17] << 8) | (v[16] << 0);
    owl_hdmi_write(tmp, HDCP_SHADR);
    owl_hdmi_read(HDCP_SHADR);

    //wait Vmatch

    for (i = 0; i < 3; i++) {
        j = 0;
        while ((j++) < 100)
            ;
        hdcp_shacr = owl_hdmi_read(HDCP_SHACR);
        if (hdcp_shacr & 0x10) {
            return 1; //Vmatch
        }
    }
    return 0; //V unmatch

}


/*读取M0的前4字节*/
void hdcp_read_hdcp_MILR(unsigned char *M0) {
    unsigned int tmp = 0;
    tmp = owl_hdmi_read(HDCP_MILR);
    M0[0] = (unsigned char) (tmp & 0xff);
    M0[1] = (unsigned char) ((tmp >> 8) & 0xff);
    M0[2] = (unsigned char) ((tmp >> 16) & 0xff);
    M0[3] = (unsigned char) ((tmp >> 24) & 0xff);
}
/*读取M0的后4字节*/
void hdcp_read_hdcp_MIMR(unsigned char *M0) {
    unsigned int tmp = 0;
    tmp = owl_hdmi_read(HDCP_MIMR);
    M0[0] = (unsigned char) (tmp & 0xff);
    M0[1] = (unsigned char) ((tmp >> 8) & 0xff);
    M0[2] = (unsigned char) ((tmp >> 16) & 0xff);
    M0[3] = (unsigned char) ((tmp >> 24) & 0xff);
}

void hdcp_set_out_opportunity_window(void)
{
	/*42,end 651,star 505 */
	owl_hdmi_write(HDCP_KOWR_HDCPREKEYKEEPOUTWIN(0x2a) |
		   HDCP_KOWR_HDCPVERKEEPOUTWINEND(0x28b) |
		   HDCP_KOWR_HDCPVERTKEEPOUTWINSTART(0x1f9), HDCP_KOWR);
	/*HDCP1.1 Mode: start 510,end 526 */
	owl_hdmi_write(HDCP_OWR_HDCPOPPWINEND(0x20e) |
		   HDCP_OWR_HDCPOPPWINSTART(0x1fe), HDCP_OWR);

}

static void hdcp_launch_authen_seq(void)
{
	int i = 6;
	
	hdcp_set_out_opportunity_window();
	while ((owl_hdmi_read(HDMI_LPCR) & HDMI_LPCR_CURLINECNTR) == 0 && i < 65) {
		udelay(1);
		i++;
	}
	if( i > 64) {
		DEBUG_ERR( "hdcp:HDMI_LPCR_CURLINECNTR timeout!\n");
	}
	i=6;
	while ((owl_hdmi_read(HDMI_LPCR) & HDMI_LPCR_CURLINECNTR) != 0 && i < 65) {
		udelay(1);
		i++;
	}
	/*set Ri/Pj udpdate:128,16 */
	owl_hdmi_write(HDCP_ICR_RIRATE(0x7f) | HDCP_ICR_PJRATE(0x0f), HDCP_ICR);
		
}

void set_hdcp_ri_pj(void) {
	int tmp;
	HDCP_DEBUG("set_hdcp_ri_pj   ~~\n");
	owl_hdmi_write(0x7f0f, HDCP_ICR);
	
	tmp = owl_hdmi_read(HDCP_CR);
    tmp &=  (~HDCP_CR_ENRIUPDINT);
	tmp &=  (~HDCP_CR_ENPJUPDINT);
	tmp &=  (~HDCP_CR_HDCP_ENCRYPTENABLE);
	tmp |=  HDCP_CR_FORCETOUNAUTHENTICATED;
    owl_hdmi_write(tmp, HDCP_CR);
	
	tmp = owl_hdmi_read(HDCP_CR);
    tmp |= HDCP_CR_EN1DOT1_FEATURE;
    owl_hdmi_write(tmp, HDCP_CR);
	

	HDCP_DEBUG("end set_hdcp_ri_pj  temp %x ~~\n", tmp);
}

int hdcp_get_ri(struct hdmi_ip_data *ip_data)
{
	ip_data->hdcp.Ri = (owl_hdmi_read(HDCP_LIR) >> 16) & 0xffff;
	HDCP_DEBUG("[%s Ri(master):0x%x]\n", __func__, ip_data->hdcp.Ri);

	return ip_data->hdcp.Ri;
}

int hdcp_check_ri(void) {
    int Ri, Ri_Read;
    unsigned char Ri_temp[8] = {0};
	HDCP_DEBUG("start hdcp_check_ri\n");
    Ri = (owl_hdmi_read(HDCP_LIR) >> 16) & 0xffff;
    if (i2c_hdcp_read(Ri_temp, 0x08, 2) == 0) {
        memset(Ri_temp, 0, sizeof(Ri_temp)); 
    }
    Ri_Read = (Ri_temp[1] << 8) | Ri_temp[0];
	HDCP_DEBUG("start hdcp_check_ri ri%x  riread%x\n", Ri, Ri_Read);
    if (Ri != Ri_Read) {
		set_hdcp_ri_pj();
		hdmi.ip_data.hdcp.hdcpOper_state = HDCP_XMT_LINK_H0;
		queue_delayed_work(hdmi.ip_data.hdcp.wq, &hdmi.ip_data.hdcp.hdcp_work,
                msecs_to_jiffies(3000));
        return -1;
    }
    return 0;
}


int hdcp_GenKm(struct hdmi_ip_data *ip_data) 
{
    unsigned char key[11];
    int i, j;
	//test_hdcp_key();
	for(i=0;i<30;i++)
	HDCP_DEBUG("test_keyr0  %d %d\n", i, test_keyr0[0][i]);

 for (i = 0; i < 5; i++) {
        for (j = 0; j < 8; j++) {
            if (ip_data->hdcp.Bksv[i] & (1 << j)) {
                c2ln14(key, test_keyr0[i * 8 + j]);
                hdcp_SetKm(&key[0], 0x55);
            }
        }
    }
    return 0;
}



int hdcp_ReadVprime(unsigned char *Vp) {
    /* read Vp */
    if (i2c_hdcp_read(Vp, 0x20, 20) != 0) {
        if (i2c_hdcp_read(Vp, 0x20, 20) != 0) {
            return 0;
        }
    }
    return 1;
}



int hdcp_AuthenticationSequence(struct hdmi_ip_data *ip_data) {
    /* force Encryption disable */
    //  P_HDCP_CR &= ~HDCP_CR_ENC_ENABLE; //6 bit
    /* reset Km accumulation */
    //  P_HDCP_CR |= HDCP_CR_RESET_KM_ACC; //4 bit
	int i;
    unsigned int tmp;
    tmp = owl_hdmi_read(HDCP_CR);
    tmp = (tmp & (~HDCP_CR_HDCP_ENCRYPTENABLE)) | HDCP_CR_RESETKMACC;
    owl_hdmi_write(tmp, HDCP_CR);

    /* set Bksv to accumulate Km */
    hdcp_GenKm(ip_data);

    /* disable Ri update interrupt */
    tmp = owl_hdmi_read(HDCP_CR);
    tmp &= (~HDCP_CR_ENRIUPDINT);
    owl_hdmi_write(tmp, HDCP_CR);

    /* clear Ri updated pending bit */
    tmp = owl_hdmi_read(HDCP_SR);
    tmp |= HDCP_SR_RIUPDATED;
    owl_hdmi_write(tmp, HDCP_SR);
	
	tmp = owl_hdmi_read(HDCP_CR);
    tmp |= HDCP_CR_AUTHCOMPUTE;
    owl_hdmi_write(tmp, HDCP_CR);

    /* trigger hdcpBlockCipher at authentication */
    //  P_HDCP_CR |= HDCP_CR_AUTH_COMP; //1 bit
    /* wait 48+56 pixel clock to get R0 */
    //  while(!(P_HDCP_SR & HDCP_SR_RI_UPDATE));
	HDCP_DEBUG("  while (!(owl_hdmi_read(HDCP_SR) & HDCP_SR_RIUPDATED)\n");
	i = 100;
    while ((!(owl_hdmi_read(HDCP_SR) & HDCP_SR_RIUPDATED))&&(i--)){
		mdelay(1);
    }
		HDCP_DEBUG("  while (!(owl_hdmi_read(HDCP_SR) & HDCP_SR_RIUPDATED)  end  %d\n", i);
    //HDCP_DEBUG("wait Ri\n");
    /* get Ri */
    ip_data->hdcp.Ri = (owl_hdmi_read(HDCP_LIR) >> 16) & 0xffff;
    HDCP_DEBUG("Ri:0x%x\n", ip_data->hdcp.Ri);

    return 0;
}

//
int hdcp_FreeRunGetAn(unsigned char *an) {
    /* Get An */
    /* get An influence from CRC64 */
    unsigned int tmp;
    int i;
    tmp = owl_hdmi_read(HDCP_CR);
    tmp |= HDCP_CR_ANINFREQ ;
    owl_hdmi_write(tmp, HDCP_CR);
	
    tmp = owl_hdmi_read(HDCP_CR);
    tmp |= HDCP_CR_ANINFLUENCEMODE;
    owl_hdmi_write(tmp, HDCP_CR);	
	
    tmp = owl_hdmi_read(HDCP_CR);
    tmp |= HDCP_CR_AUTHREQUEST;
    owl_hdmi_write(tmp, HDCP_CR);

    //P_HDCP_CR |= HDCP_CR_AN_INF_REQ; //25 bit

    /* set An Influence Mode, influence will be load from AnIR0, AnIR1 */
    //P_HDCP_CR |= HDCP_CR_LOAD_AN;  //7 bit
    /* trigger to get An */
    //P_HDCP_CR |= HDCP_CR_AUTH_REQ;  //0 bit  --写1，生成an
    HDCP_DEBUG("[hdcp_FreeRunGetAn]:wait An\n");
	i=100;
    while (!(owl_hdmi_read(HDCP_SR) & (HDCP_SR_ANREADY))&&(i--)){
		udelay(1);
	} //等待An ready

    HDCP_DEBUG("[hdcp_FreeRunGetAn]:wait An ok\n");
    /* leave An influence mode */
    tmp = owl_hdmi_read(HDCP_CR);

    tmp &= (~HDCP_CR_ANINFLUENCEMODE);
    owl_hdmi_write(tmp, HDCP_CR);

    /* 
     * Convert HDCP An from bit endien to little endien 
     * HDCP An should stored in little endien, 
     * but HDCP HW store in bit endien. 
     */
    an[0] = 0x18;
    tmp = owl_hdmi_read(HDCP_ANLR);
    an[1] = tmp & 0xff;
    an[2] = (tmp >> 8) & 0xff;
    an[3] = (tmp >> 16) & 0xff;
    an[4] = (tmp >> 24) & 0xff;

    tmp = owl_hdmi_read(HDCP_ANMR);

    an[5] = tmp & 0xff;
    an[6] = (tmp >> 8) & 0xff;
    an[7] = (tmp >> 16) & 0xff;
    an[8] = (tmp >> 24) & 0xff;

    for (i = 0; i < 9; i++)
        HDCP_DEBUG("an[%d]:0x%x\n", i, an[i]);
    
    return 0;
}


void hdcp_ForceUnauthentication(void)
{
    /* disable link integry check */
    /* force Encryption disable */
    unsigned int tmp;
    tmp = owl_hdmi_read(HDCP_CR);
    tmp = (tmp & (~HDCP_CR_ENRIUPDINT) & (~HDCP_CR_ENPJUPDINT) & (~HDCP_CR_HDCP_ENCRYPTENABLE))
            | HDCP_CR_FORCETOUNAUTHENTICATED;
    owl_hdmi_write(tmp, HDCP_CR);
    owl_hdmi_read(HDCP_CR);
    /* force HDCP module to unauthenticated state */
    //P_HDCP_CR |= HDCP_CR_FORCE_UNAUTH;
}

void enable_hdcp_repeater(void)
{
    unsigned int tmp;
    tmp = owl_hdmi_read(HDCP_CR);
    tmp |= HDCP_CR_DOWNSTRISREPEATER;
    owl_hdmi_write(tmp, HDCP_CR);
    tmp = owl_hdmi_read(HDCP_CR);
}

int check_ri_irq(void)
{
    unsigned int tmp;
    tmp = owl_hdmi_read(HDCP_SR);
	HDCP_DEBUG("HDCP_SR 0x%x\n", tmp);
	if(tmp&HDCP_SR_RIUPDATED)
	{
		tmp |= HDCP_SR_RIUPDATED;
		tmp |= HDCP_SR_PJUPDATED;
		owl_hdmi_write(tmp, HDCP_SR);	
		return 1;
	}
	return 0;
}

void disable_hdcp_repeater(void)
{
    unsigned int tmp;
    tmp = owl_hdmi_read(HDCP_CR);
    tmp &= (~HDCP_CR_DOWNSTRISREPEATER);
    owl_hdmi_write(tmp, HDCP_CR);
    tmp = owl_hdmi_read(HDCP_CR);
}

void enable_ri_update_check(void)
{
    unsigned int tmp;
	
    tmp = owl_hdmi_read(HDCP_SR);
    tmp |= HDCP_SR_RIUPDATED;
    owl_hdmi_write(tmp, HDCP_SR);

    tmp = owl_hdmi_read(HDCP_CR);
    tmp |= HDCP_CR_ENRIUPDINT;
    owl_hdmi_write(tmp, HDCP_CR);
    owl_hdmi_read(HDCP_CR);
}

void set_hdcp_to_Authenticated_state(void)
{
    // Authenticated 
    // set HDCP module to authenticated state 
    unsigned int tmp;
    tmp = owl_hdmi_read(HDCP_CR);
    tmp |= HDCP_CR_DEVICEAUTHENTICATED;
    owl_hdmi_write(tmp, HDCP_CR);
    // start encryption    
    tmp = owl_hdmi_read(HDCP_CR);
    tmp |= HDCP_CR_HDCP_ENCRYPTENABLE;
    owl_hdmi_write(tmp, HDCP_CR);

    tmp = owl_hdmi_read(HDCP_CR);
}

void hdcp_check_handle(struct work_struct *work) {
    unsigned char An[9] = {0};
    unsigned char Bcaps = 0, Ri_temp[8] = {0};

	//hdcp_launch_authen_seq();
	
	if((owl_hdmi_read(HDMI_CR)&0x01)&&(owl_hdmi_read(HDMI_CR)&(0x01<<29))){
	
	}else{
		HDCP_CASE("hdmi plug out in hdcp\n");	
		set_hdcp_ri_pj();
		hdmi.ip_data.hdcp.hdcpOper_state = HDCP_XMT_LINK_H0;
		return ;		
	}
	
	if(owl_hdmi_read(HDMI_GCPCR)&0x01){
		HDCP_CASE("owl_hdmi_read(HDCP_GCPCR)&0x01 is true 0x%x\n", owl_hdmi_read(HDMI_GCPCR));	
		set_hdcp_ri_pj();
		hdmi.ip_data.hdcp.hdcpOper_state = HDCP_XMT_LINK_H0;
		return ;
	}
	
	if(aksv[0] == 0x00 && aksv[1] == 0x00 && aksv[2] ==	0x00 &&	aksv[3]	== 0x00	&& aksv[4] == 0x00)
	{
		HDCP_CASE("aksv == 0\n");	
		set_hdcp_ri_pj();
		hdmi.ip_data.hdcp.hdcpOper_state = HDCP_XMT_LINK_H0;
		return ;
	}
	
    HDCP_CASE("\n**********hdcp start guolong************ \n");
    if(hdmi.ip_data.hdcp.hdcp_fail_times > HDCP_FAIL_TIMES) {//stop play
        hdmi.ip_data.hdcp.hdcp_authentication_success = false;
        HDCP_CASE("\n**********hdcp fail many times************ \n");
        goto end;
    }   

    if((hdmi.ip_data.hdcp.need_to_delay -= 50) > 0) {
        goto restart;
    }

    //state machine
    switch (hdmi.ip_data.hdcp.hdcpOper_state) {
    case HDCP_XMT_LINK_H0:
        HDCP_CASE("************HDCP_XMT_LINK_H0*************\n");
        hdmi.ip_data.hdcp.hdcpOper_state = HDCP_XMT_LINK_H1;

    case HDCP_XMT_LINK_H1:
        HDCP_CASE("************HDCP_XMT_LINK_H1*************\n");
        hdmi.ip_data.hdcp.hdcpOper_state = HDCP_XMT_AUTH_A0;
	
    /*
     * Authentication phase 1: A0,A1,A2,A3
     * A0.generate An, get Aksv;
     * A1.write An and Aksv, read Bksv; 
     * A2.get R0;
     * A3.computes Km, Ks, M0 and R0;
     *
     * Authentication phase 2:  A6  or  A6,A8,A9,A91
     * A6.check if sink support repeater, if not support repeater then authentication finish; 
     * A8. check if  KSV is ready;
     * A9:get KSV list;
     * A91:compare V value;
     *
     * Authentication phase 3: A4,A5
     * A4.set hdcp to Authenticated state;
     * A5.authentication successful
     */
    case HDCP_XMT_AUTH_A0: //Authentication phase 1
        HDCP_CASE("************HDCP_XMT_AUTH_A0*************\n");
        /* genrate An, get Aksv */
        hdmi.ip_data.hdcp.hdcpOper_retry = 0;
        //get An value
        hdcp_FreeRunGetAn(An);
		msleep(10);
       //send An
        HDCP_CASE("************send An*************\n");

        if (i2c_hdcp_write(&An[0], 0x18, 9) < 0) {
            HDCP_DEBUG("\n Write An error \n"); 
        }
        //send aksv
        HDCP_CASE("************send aksv*************\n");  

        if (i2c_hdcp_write(aksv, 0x10, 6) < 0) {
            HDCP_DEBUG("\n Write aksv error \n"); 
        }

        hdmi.ip_data.hdcp.hdcpOper_state = HDCP_XMT_AUTH_A1;
		//msleep(100);
        hdmi.ip_data.hdcp.need_to_delay = 100;
        break;

    case HDCP_XMT_AUTH_A1://Authentication phase 1
        HDCP_CASE("************HDCP_XMT_AUTH_A1*************\n");
        /* 
         * write An and Aksv, read Bksv 
         * if get Bksv successful mean receiver/repeater support HDCP 
         * check valid Bksv 20 ones and 20 zero 
         */
        HDCP_CASE("************read Bksv*************\n");
        while (i2c_hdcp_read(hdmi.ip_data.hdcp.Bksv, 0x00, 5) < 0) {//if read successful, means support HDCP
            hdmi.ip_data.hdcp.i2c_error++;
            if (hdmi.ip_data.hdcp.i2c_error > 3) {
                hdmi.ip_data.hdcp.i2c_error = 0;
                HDCP_DEBUG("[631]Do not support HDCP \n");
                break;
            }
        }
		msleep(110);
        HDCP_CASE("************Check_Bksv*************\n");
        if (Check_Bksv_Invalid() == 0) {
            hdmi.ip_data.hdcp.hdcp_fail_times++;
            hdcp_ForceUnauthentication();
            hdmi.ip_data.hdcp.hdcpOper_state = HDCP_XMT_LINK_H0;
            HDCP_DEBUG("\n Check_Bksv_Invalid \n");
            hdmi.ip_data.hdcp.need_to_delay = 100;
            break;
        }

        hdmi.ip_data.hdcp.hdcpOper_state = HDCP_XMT_AUTH_A2;
        hdmi.ip_data.hdcp.need_to_delay = 100;
		//msleep(100);
        break;
   
    case HDCP_XMT_AUTH_A2://Authentication phase 1
        HDCP_CASE("************HDCP_XMT_AUTH_A2*************\n");
        // Computations
        // computes Km, Ks, M0 and R0   
        HDCP_CASE("************read Bcaps*************\n");
        if (i2c_hdcp_read(&Bcaps, 0x40, 1) < 0) {
            HDCP_DEBUG("Read Bcaps error \n"); //error_handle();
        }

        if ((Bcaps & (1 << 6)) != 0) {
            // set to support repeater 
            HDCP_CASE("************support repeater*************\n");
            hdmi.ip_data.hdcp.repeater = 1;
            enable_hdcp_repeater();
        } else {
            // set to support NO repeater
            HDCP_CASE("************dont  support repeater*************\n");
            hdmi.ip_data.hdcp.repeater = 0;
            disable_hdcp_repeater();
        }
        HDCP_CASE("************generate Ri*************\n");
        hdcp_AuthenticationSequence(&hdmi.ip_data);
        hdmi.ip_data.hdcp.hdcpOper_retry = 3; //add by keith

        /* if computed results are available */
        hdmi.ip_data.hdcp.hdcpOper_state = HDCP_XMT_AUTH_A3;
        //wait for 100 msec to read R0p.        
        mdelay(130); // 建议不改为可调度

        break; //add by keith

    case HDCP_XMT_AUTH_A3://Authentication phase 1
        HDCP_CASE("************HDCP_XMT_AUTH_A3*************\n");
        // Validate Receiver
        // computes Km, Ks, M0 and R0
        // if computed results are available
         HDCP_DEBUG("************read R0*************\n");
        if (i2c_hdcp_read(Ri_temp, 0x08, 2) < 0) {
            memset(Ri_temp, 0, sizeof(Ri_temp)); 
        }
        hdmi.ip_data.hdcp.Ri_Read = (int) ((unsigned int) Ri_temp[1] << 8)
                | Ri_temp[0];
        HDCP_CASE("****Ri_Read:0x%x\n****", hdmi.ip_data.hdcp.Ri_Read);
        if (hdmi.ip_data.hdcp.Ri != hdmi.ip_data.hdcp.Ri_Read) {
            HDCP_DEBUG("\n R0 != Ri_Read \n");
            if (hdmi.ip_data.hdcp.hdcpOper_retry != 0) {
                hdmi.ip_data.hdcp.hdcpOper_retry--;
                hdmi.ip_data.hdcp.need_to_delay = 100;
            } else {
                /* authentication part I failed */
                hdmi.ip_data.hdcp.hdcp_fail_times++;
                hdcp_ForceUnauthentication();
                // restart
                hdmi.ip_data.hdcp.hdcpOper_state = HDCP_XMT_AUTH_A0;
                hdmi.ip_data.hdcp.need_to_delay = 200;
            }
            break;
        }
        //hdmi.ip_data.hdcp.hdcp_fail_times = 0;
        hdmi.ip_data.hdcp.hdcpOper_retry = 0;
        hdcp_read_hdcp_MILR(&hdmi.ip_data.hdcp.hdcpOper_M0[0]);
        hdcp_read_hdcp_MIMR(&hdmi.ip_data.hdcp.hdcpOper_M0[4]);
        // authentication part I successful 
        hdmi.ip_data.hdcp.hdcpOper_state = HDCP_XMT_AUTH_A6;

    case HDCP_XMT_AUTH_A6://Authentication phase 2
        HDCP_CASE("************HDCP_XMT_AUTH_A6*************\n");
        // Test for Repeater
        // get REPEATER
        if (hdmi.ip_data.hdcp.repeater != 0) {
            // change to Authentication part II 
            hdmi.ip_data.hdcp.hdcpOper_state = HDCP_XMT_AUTH_A8;
            /* wait 100msec */
            hdmi.ip_data.hdcp.need_to_delay = 100;
            hdmi.ip_data.hdcp.retry_times_for_set_up_5_second = 0;
            break;
        }

        // NO repeater 
        // change to Authentication part III
        hdmi.ip_data.hdcp.hdcpOper_state = HDCP_XMT_AUTH_A4;

    case HDCP_XMT_AUTH_A4: ////Authentication phase 3
        HDCP_CASE("************HDCP_XMT_AUTH_A4*************\n");
        // Authenticated 
        // set HDCP module to authenticated state      
        // start encryption
        set_hdcp_to_Authenticated_state();

        hdmi.ip_data.hdcp.hdcpOper_state = HDCP_XMT_AUTH_A5;
        hdmi.ip_data.hdcp.hdcpOper_retry = 0;
        // enable Ri update check

        break;

    case HDCP_XMT_AUTH_A5://Authentication phase 3
        HDCP_CASE("************HDCP_XMT_AUTH_A5*************\n");
        /* Link Integrity Check */
        /* Interrupt and BH will do this job */
        //hdmi.ip_data.hdcp.hdcp_fail_times = 0;
        HDCP_CASE("********hdcp Authentication suceesful******** \n");
        enable_ri_update_check();
        hdmi.ip_data.hdcp.hdcp_authentication_success = true;
        return;

    case HDCP_XMT_AUTH_A8: ////Authentication phase 2
        HDCP_CASE("************HDCP_XMT_AUTH_A8*************\n");
        /* 2nd part authentication */
        /* Wait for Ready */
        /* set up 5 second timer poll for KSV list ready */
        if ((hdmi.ip_data.hdcp.retry_times_for_set_up_5_second % 5) == 0) {
            if (i2c_hdcp_read(&Bcaps, 0x40, 1) < 0) {
                HDCP_DEBUG("\n Read Bcaps err \n"); 
            }
        }
        if (!((Bcaps >> 5) & 0x1)) { //if KSVlist not ready!   
            if (hdmi.ip_data.hdcp.retry_times_for_set_up_5_second <= 50) { //100 msec * 50 = 5 sec
                hdmi.ip_data.hdcp.retry_times_for_set_up_5_second++;
                hdmi.ip_data.hdcp.hdcpOper_state = HDCP_XMT_AUTH_A8;
                /* wait 100msec */
                hdmi.ip_data.hdcp.need_to_delay = 100;
                break;
            } else {
                /* restart */
                hdmi.ip_data.hdcp.hdcp_fail_times++;
                hdmi.ip_data.hdcp.retry_times_for_set_up_5_second = 0;
                hdmi.ip_data.hdcp.hdcpOper_state = HDCP_XMT_AUTH_A0;
                hdcp_ForceUnauthentication();
                /* wait 100msec */
                hdmi.ip_data.hdcp.need_to_delay = 100;
                HDCP_DEBUG("\n retry_times_for_set_up_5_second > 50 \n");
                break;
            }

        }
        //hdmi.ip_data.hdcp.hdcp_fail_times = 0;
        hdmi.ip_data.hdcp.retry_times_for_set_up_5_second = 0;
        hdmi.ip_data.hdcp.hdcpOper_state = HDCP_XMT_AUTH_A9;

    case HDCP_XMT_AUTH_A9://Authentication phase 2
         HDCP_CASE("************HDCP_XMT_AUTH_A9*************\n");
        /* Read KSV List and Bstatus */
        //hdcp_ReadKsvList(&hdcpOper.Bstatus, hdcpOper.ksvList);//mark by keith
        if (!hdcp_ReadKsvList(hdmi.ip_data.hdcp.Bstatus,
                hdmi.ip_data.hdcp.ksvList)) { //add by keith
            hdmi.ip_data.hdcp.hdcpOper_state = HDCP_XMT_AUTH_A0;
            hdcp_ForceUnauthentication();
            hdmi.ip_data.hdcp.hdcp_fail_times++;
            HDCP_DEBUG("\n hdcp_ReadKsvList \n");
            hdmi.ip_data.hdcp.need_to_delay = 100;

            break;
        }

        hdmi.ip_data.hdcp.need_to_delay = 100;
        hdmi.ip_data.hdcp.hdcpOper_state = HDCP_XMT_AUTH_A9_1;
        break;

    case HDCP_XMT_AUTH_A9_1://Authentication phase 2
        HDCP_CASE("************HDCP_XMT_AUTH_A9_1*************\n");
        hdcp_ReadVprime(hdmi.ip_data.hdcp.Vp); //add by ciwu     
        if (!do_Vmatch(&hdmi.ip_data, hdmi.ip_data.hdcp.Vp, hdmi.ip_data.hdcp.ksvList,
                hdmi.ip_data.hdcp.Bstatus, hdmi.ip_data.hdcp.hdcpOper_M0)) {
            /* compare with V' */
            /* authentication part II failed */
            hdmi.ip_data.hdcp.hdcp_fail_times++;
            hdcp_ForceUnauthentication();
            hdmi.ip_data.hdcp.hdcpOper_state = HDCP_XMT_AUTH_A0;

            hdmi.ip_data.hdcp.need_to_delay = 100;
            HDCP_DEBUG("\n do_Vmatch \n");
            break;
        }

        /* KSV list correct , transit to Authentication Part III */
        hdmi.ip_data.hdcp.hdcpOper_state = HDCP_XMT_AUTH_A4;
        hdmi.ip_data.hdcp.need_to_delay = 100;
        break;

    default:
        break;
    }

restart: 
    queue_delayed_work(hdmi.ip_data.hdcp.wq,
            &hdmi.ip_data.hdcp.hdcp_work, msecs_to_jiffies(hdcp_timer_interval));
    return;
    
end: 
    hdcp_ForceUnauthentication();
    return;
}

void hdcp_init(void)
{
	static bool first_read_hdcpkey = true;
	hdmi.ip_data.hdcp.hdcpOper_state = HDCP_XMT_LINK_H0;
	hdmi.ip_data.hdcp.hdcp_fail_times = 0;
		
	/*if(first_read_hdcpkey){
		first_read_hdcpkey = false;
		if(hdcp_read_key()){
			DEBUG_ERR("hdcp_read_key  read  fail!!!!\n");
		}
	}*/
	
	set_hdcp_ri_pj();
	hdcp_launch_authen_seq();
	hdcp_ForceUnauthentication();
}

