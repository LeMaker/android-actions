#include "case.h"

inline int _write_sysfs_int(char *filename, char *basedir, int val, int verify)
{
    int ret = 0;
	FILE *sysfsfp;
	int test;
	char *temp = malloc(strlen(basedir) + strlen(filename) + 2);
	if (temp == NULL)
		return -ENOMEM;
        
	sprintf(temp, "%s/%s", basedir, filename);

	sysfsfp = fopen(temp, "w");
	if (sysfsfp == NULL) {
		printf("failed to open %s\n", temp);
		ret = -errno;
		goto error_free;
	}
	fprintf(sysfsfp, "%d", val);
	fclose(sysfsfp);

	if (verify) {
		sysfsfp = fopen(temp, "r");
		if (sysfsfp == NULL) {
			printf("failed to open %s\n", temp);
			ret = -errno;
			goto error_free;
		}
		fscanf(sysfsfp, "%d", &test);
        fclose(sysfsfp);

		if (test != val) {
            printf("Possible failure in int write %d to %s\n",
                   val, temp);
			ret = -1;
		}
	}

error_free:
	free(temp);
	return ret;
}

int write_sysfs_int(char *filename, char *basedir, int val)
{
	return _write_sysfs_int(filename, basedir, val, 0);
}

int write_sysfs_int_and_verify(char *filename, char *basedir, int val)
{
	return _write_sysfs_int(filename, basedir, val, 1);
}


void get_sensor_data(char *d, iio_info *sensor)
{
    sensor->lock = 1;
	sensor->x = *(short *)(d + 2);
	sensor->y = *(short *)(d + 4);
	sensor->z = *(short *)(d + 6);
	sensor->lock = 0;
}

static int read_data(char *buffer_access, iio_info* iio_info)
{
#define ACCEL_HDR                0x4000
#define GYRO_HDR                 0x2000
#define COMPASS_HDR              0x1000

    static int left_over_size = 0;
    char data[1048], *dptr, tmp[24];
    int q[3];
    int ret, i, ind, fp;
    int buf_size, read_size;
    unsigned short hdr;
    bool done_flag;

    fp = open(buffer_access, O_RDONLY | O_NONBLOCK);
    if(fp == -1)
	{ /* if it isn't there make the node */
        printf("Failed to open %s\n", buffer_access);
        return -errno;
    }
    ind = 0;

    while(1)
    {
        struct pollfd pfd = {
            .fd = fp,
            .events = POLLIN,
        };
        poll(&pfd, 1, -1);

        if(left_over_size > 0)
            memcpy(data, tmp, left_over_size);
        dptr = data + left_over_size;

        read_size = read(fp,  dptr, 1024);
        if(read_size <= 0)
		{
            printf("Wrong size=%d\n", read_size);
            return -EINVAL;
        }

        ind = read_size + left_over_size;
        dptr = data;
        buf_size = ind - (dptr - data);
        done_flag = false;
        while ((buf_size > 0) && (!done_flag)) {
            hdr = *((short *)(dptr));
            if (hdr & 1)
                printf("STEP\n");

            switch (hdr & (~1)) {
            case ACCEL_HDR:
                if (buf_size >= 16) {
                    get_sensor_data(dptr, iio_info);
                    dptr += 8;
                    // printf("ACCEL, %d, %d, %d\n", iio_info->x, iio_info->y, iio_info->z);
                } else
                    done_flag = true;
                break;
            case GYRO_HDR:
                if (buf_size >= 16) {
                    get_sensor_data(dptr, iio_info + 1);
                    dptr += 8;
                    // printf("GYRO, %d, %d, %d\n", (iio_info + 1)->x, (iio_info + 1)->y, (iio_info + 1)->z);
                } else
                    done_flag = true;
                break;
            case COMPASS_HDR:
                if (buf_size >= 16) {
                    get_sensor_data(dptr, iio_info + 2);
                    dptr += 8;
                    // printf("COMPASS, %d, %d, %d\n", (iio_info + 2)->x, (iio_info + 2)->y, (iio_info + 2)->z);
                } else
                    done_flag = true;
                break;
            default:
                printf("unknown, \n");
                for (i = 0; i < 8; i++)
                    printf("%02x, ", dptr[i]);
                printf("\n");
                break;
            }
            if (!done_flag)
                dptr += 8;
            buf_size = ind - (dptr - data);
        }
        if (ind - (dptr - data) > 0)
            memcpy(tmp, dptr, ind - (dptr - data));
        left_over_size = ind - (dptr - data);
    }
    
    close(fp);
}

bool is_iio_enable()
{
	struct stat st;
	char dev_dir_name[100];
	strcpy(dev_dir_name, "/sys/bus/iio/devices/iio:device0");
	
	if(-1 == stat(dev_dir_name, &st))
		return false;
	return true;
}

int iio_read(iio_info* iio_info, bool accel_enable, bool gyro_enable, bool comp_enable)
{
	char dev_dir_name[100];
	char buf_dir_name[100];
	char buffer_access[50];
    unsigned long num_loops = -1;
    int ret, c, i;

	if(!is_iio_enable())
		return -1;
	
    // all output to stdout must be delivered immediately, no buffering
    setvbuf(stdout, NULL, _IONBF, 0);

	strcpy(dev_dir_name, "/sys/bus/iio/devices/iio:device0");
    printf("INFO: dev_dir_name=%s\n", dev_dir_name);

    sprintf(buf_dir_name, "%s/buffer", dev_dir_name);
    printf("INFO: buf_dir_name=%s\n", buf_dir_name);
    
    /* attempt to open non blocking the access dev */
    strcpy(buffer_access, "/dev/iio:device0");
    printf("INFO: buffer_access=%s\n", buffer_access);
	
    ret = write_sysfs_int_and_verify("master_enable", dev_dir_name, 0);
	if (ret < 0)
	{
		printf("write master_enable failed\n");
        return ret;
	}

    ret = write_sysfs_int_and_verify("enable", buf_dir_name, 0);
	if (ret < 0)
	{
		printf("write buffer/enable failed\n");
        return ret;
	}


    ret = write_sysfs_int_and_verify("accel_enable", dev_dir_name, 0);
	if (ret < 0)
	{
		printf("write accel_enable failed\n");
        return ret;
	}
    ret = write_sysfs_int_and_verify("accel_fifo_enable", dev_dir_name, 0);
	if (ret < 0)
	{
		printf("write accel_fifo_enable failed\n");
        return ret;
	}
    
    ret = write_sysfs_int_and_verify("gyro_enable", dev_dir_name, 0);
	if (ret < 0)
	{
		printf("write gyro_enable failed\n");
        return ret;
	}
    ret = write_sysfs_int_and_verify("gyro_fifo_enable", dev_dir_name, 0);
	if (ret < 0)
	{
		printf("write gyro_fifo_enable failed\n");
        return ret;
	}

    ret = write_sysfs_int_and_verify("compass_enable", dev_dir_name, 0);
	if (ret < 0)
	{
		printf("write compass_enable failed\n");
        return ret;
	}

    if(accel_enable)
    {
        ret = write_sysfs_int_and_verify("accel_enable", dev_dir_name, 1);
	    if (ret < 0)
	    {
		    printf("write accel_enable failed\n");
            return ret;
	    }
        ret = write_sysfs_int_and_verify("accel_fifo_enable", dev_dir_name, 1);
	    if (ret < 0)
	    {
		    printf("write accel_fifo_enable failed\n");
            return ret;
	    }
        
    }

    if(gyro_enable)
    {
        ret = write_sysfs_int_and_verify("gyro_enable", dev_dir_name, 1);
	    if (ret < 0)
	    {
		    printf("write gyro_enable failed\n");
            return ret;
	    }
        ret = write_sysfs_int_and_verify("gyro_fifo_enable", dev_dir_name, 1);
	    if (ret < 0)
	    {
		    printf("write gyro_fifo_enable failed\n");
            return ret;
	    }
    }

    if(comp_enable)
    {
        ret = write_sysfs_int_and_verify("compass_enable", dev_dir_name, 1);
	    if (ret < 0)
	    {
		    printf("write compass_enable failed\n");
            return ret;
	    }  
    }

 
    ret = write_sysfs_int_and_verify("length", buf_dir_name, 480);
	if (ret < 0)
	{
		printf("write buffer/length failed\n");
        return ret;
	} 

    ret = write_sysfs_int_and_verify("enable", buf_dir_name, 1);
	if (ret < 0)
	{
		printf("write buffer/enable failed\n");
        return ret;
	} 


    ret = write_sysfs_int_and_verify("master_enable", dev_dir_name, 1);
	if (ret < 0)
	{
		printf("write master_enable failed\n");
        return ret;
	}
    
    read_data(buffer_access, iio_info);

}