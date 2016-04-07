#include <jni.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <utils/Log.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>
#include <linux/spi/spidev.h>
#include <linux/i2c-dev.h>
#include <termios.h>
#define	ttyS0	0x00
#define	ttyS1	0x01
#define	ttyS2	0x02
#define	ttyS3	0x03
#define	ttyS4	0x04
#define	ttyS5	0x05
#define	ttyS6	0x06

/**
 * Description:
 * this is jni file for ActduinoTest
 *********************************************
 *ActionsCode(author:jiangjinzhang, new_code)
 * @version 1.0
 */

//uart test
static int uart_fileHandler = -1;

JNIEXPORT jint JNICALL Java_com_actions_jni_uart_openuart(JNIEnv *env,
		jobject obj, jint key) {
	switch (key) {
	case ttyS0:
		uart_fileHandler = open("/dev/ttyS0", O_RDWR | O_NOCTTY | O_NDELAY);
		ALOGE("open /dev/ttyS0 %d", uart_fileHandler);
		if (uart_fileHandler < 0) {
			ALOGE("open /dev/ttyS0 fail");
		}

		break;
	case ttyS3:
		uart_fileHandler = open("/dev/ttyS3", O_RDWR | O_NOCTTY | O_NDELAY);
		ALOGE("open /dev/ttyS3 %d", uart_fileHandler);
		if (uart_fileHandler < 0) {
			ALOGE("open /dev/ttyS3 fail");
		}

		break;
	default:
		break;
	}
	return uart_fileHandler;
}
JNIEXPORT jobject JNICALL Java_com_actions_jni_uart_openuart11(JNIEnv *env,
		jobject obj, jint key, jint nSpeed, jint nBits, jint nEvent, jint nStop) {
	int fd = 0;
	speed_t speed = 115200;
	jobject mFileDescriptor;

	switch (key) {
	case ttyS0:
		uart_fileHandler = open("/dev/ttyS0", O_RDWR | O_NOCTTY | O_NDELAY);
		ALOGE("open /dev/ttyS0 %d", uart_fileHandler);
		if (uart_fileHandler < 0) {
			ALOGE("open /dev/ttyS0 fail");
		}

		break;
	case ttyS1:
		uart_fileHandler = open("/dev/ttyS1", O_RDWR | O_NOCTTY | O_NDELAY);
		ALOGE("open /dev/ttyS1 %d", uart_fileHandler);
		if (uart_fileHandler < 0) {
			ALOGE("open /dev/ttyS1 fail");
		}

		break;
	case ttyS2:
		uart_fileHandler = open("/dev/ttyS2", O_RDWR | O_NOCTTY | O_NDELAY);
		ALOGE("open /dev/ttyS2 %d", uart_fileHandler);
		if (uart_fileHandler < 0) {
			ALOGE("open /dev/ttyS2 fail");
		}

		break;
	case ttyS3:
		uart_fileHandler = open("/dev/ttyS3", O_RDWR | O_NOCTTY | O_NDELAY);
		ALOGE("open /dev/ttyS3 %d", uart_fileHandler);
		if (uart_fileHandler < 0) {
			ALOGE("open /dev/ttyS3 fail");
		}

		break;
	case ttyS4:
		uart_fileHandler = open("/dev/ttyS4", O_RDWR | O_NOCTTY | O_NDELAY);
		ALOGE("open /dev/ttyS4 %d", uart_fileHandler);
		if (uart_fileHandler < 0) {
			ALOGE("open /dev/ttyS4 fail");
		}

		break;
	case ttyS5:
		uart_fileHandler = open("/dev/ttyS5", O_RDWR | O_NOCTTY | O_NDELAY);
		ALOGE("open /dev/ttyS5 %d", uart_fileHandler);
		if (uart_fileHandler < 0) {
			ALOGE("open /dev/ttyS5 fail");
		}

		break;
	case ttyS6:
			uart_fileHandler = open("/dev/ttyS6", O_RDWR | O_NOCTTY | O_NDELAY);
			ALOGE("open /dev/ttyS6 %d", uart_fileHandler);
			if (uart_fileHandler < 0) {
				ALOGE("open /dev/ttyS6 fail");
			}

			break;
	default:
		break;
	}
	if (uart_fileHandler == -1) {
		/* Throw an exception */
		ALOGE("Cannot open port");
		/* TODO: throw an exception */
		return NULL;
	}

	struct termios newtio, oldtio;
	if (tcgetattr(uart_fileHandler, &oldtio) != 0) {
		perror("SetupSerial 1");
		return -1;
	}
	bzero(&newtio, sizeof(newtio));
	newtio.c_cflag |= CLOCAL | CREAD;
	newtio.c_cflag &= ~CSIZE;

	switch (nBits) {
	case 7:
		newtio.c_cflag |= CS7;
		break;
	case 8:
		newtio.c_cflag |= CS8;
		break;
	}

	switch (nEvent) {
	case 'O':                     //奇校验
		newtio.c_cflag |= PARENB;
		newtio.c_cflag |= PARODD;
		newtio.c_iflag |= (INPCK | ISTRIP);
		break;
	case 'E':                     //偶校验
		newtio.c_iflag |= (INPCK | ISTRIP);
		newtio.c_cflag |= PARENB;
		newtio.c_cflag &= ~PARODD;
		break;
	case 'N':                    //无校验
		newtio.c_cflag &= ~PARENB;
		break;
	}

	switch (nSpeed) {
	case 2400:
		cfsetispeed(&newtio, B2400);
		cfsetospeed(&newtio, B2400);
		break;
	case 4800:
		cfsetispeed(&newtio, B4800);
		cfsetospeed(&newtio, B4800);
		break;
	case 9600:
		cfsetispeed(&newtio, B9600);
		cfsetospeed(&newtio, B9600);
		break;
	case 115200:
		cfsetispeed(&newtio, B115200);
		cfsetospeed(&newtio, B115200);
		break;
	default:
		cfsetispeed(&newtio, B9600);
		cfsetospeed(&newtio, B9600);
		break;
	}
	if (nStop == 1) {
		newtio.c_cflag &= ~CSTOPB;
	} else if (nStop == 2) {
		newtio.c_cflag |= CSTOPB;
	}
	newtio.c_cc[VTIME] = 0;
	newtio.c_cc[VMIN] = 0;
	tcflush(fd, TCIFLUSH);
	if ((tcsetattr(uart_fileHandler, TCSANOW, &newtio)) != 0) {
		perror("com set error");
		return -1;
	}
	printf("set done!\n");

	/* Create a corresponding file descriptor */
	{
		jclass cFileDescriptor = (*env)->FindClass(env,
				"java/io/FileDescriptor");
		jmethodID iFileDescriptor = (*env)->GetMethodID(env, cFileDescriptor,
				"<init>", "()V");
		jfieldID descriptorID = (*env)->GetFieldID(env, cFileDescriptor,
				"descriptor", "I");
		mFileDescriptor = (*env)->NewObject(env, cFileDescriptor,
				iFileDescriptor);
		(*env)->SetIntField(env, mFileDescriptor, descriptorID,
				(jint) uart_fileHandler);
	}

	return mFileDescriptor;
}
JNIEXPORT jint JNICALL Java_com_actions_jni_uart_close11(JNIEnv *env, jobject obj) {
	if (uart_fileHandler > 0) {
		close(uart_fileHandler);
	}
	return uart_fileHandler;
}
JNIEXPORT void JNICALL Java_com_actions_jni_uart_close
(JNIEnv *env, jobject obj)
{
	jclass SerialPortClass = (*env)->GetObjectClass(env, obj);
	jclass FileDescriptorClass = (*env)->FindClass(env, "java/io/FileDescriptor");

	jfieldID mFdID = (*env)->GetFieldID(env, SerialPortClass, "mFd", "Ljava/io/FileDescriptor;");
	jfieldID descriptorID = (*env)->GetFieldID(env, FileDescriptorClass, "descriptor", "I");

	jobject mFd = (*env)->GetObjectField(env, obj, mFdID);
	jint descriptor = (*env)->GetIntField(env, mFd, descriptorID);

	ALOGD("close(fd = %d)", descriptor);
	close(descriptor);
}
JNIEXPORT jint JNICALL Java_com_actions_jni_uart_setmodeuart(JNIEnv *env,
		jobject obj, jint nSpeed, jint nBits, jchar nEvent, jint nStop) {
	//jobject mFileDescriptor;
	struct termios newtio, oldtio;
	if (tcgetattr(uart_fileHandler, &oldtio) != 0) {
		perror("SetupSerial 1");
		return -1;
	}
	bzero(&newtio, sizeof(newtio));
	newtio.c_cflag |= CLOCAL | CREAD;
	newtio.c_cflag &= ~CSIZE;

	switch (nBits) {
	case 7:
		newtio.c_cflag |= CS7;
		break;
	case 8:
		newtio.c_cflag |= CS8;
		break;
	}

	switch (nEvent) {
	case 'O':                     //奇校验
		newtio.c_cflag |= PARENB;
		newtio.c_cflag |= PARODD;
		newtio.c_iflag |= (INPCK | ISTRIP);
		break;
	case 'E':                     //偶校验
		newtio.c_iflag |= (INPCK | ISTRIP);
		newtio.c_cflag |= PARENB;
		newtio.c_cflag &= ~PARODD;
		break;
	case 'N':                    //无校验
		newtio.c_cflag &= ~PARENB;
		break;
	}

	switch (nSpeed) {
	case 2400:
		cfsetispeed(&newtio, B2400);
		cfsetospeed(&newtio, B2400);
		break;
	case 4800:
		cfsetispeed(&newtio, B4800);
		cfsetospeed(&newtio, B4800);
		break;
	case 9600:
		cfsetispeed(&newtio, B9600);
		cfsetospeed(&newtio, B9600);
		break;
	case 115200:
		cfsetispeed(&newtio, B115200);
		cfsetospeed(&newtio, B115200);
		break;
	default:
		cfsetispeed(&newtio, B9600);
		cfsetospeed(&newtio, B9600);
		break;
	}
	if (nStop == 1) {
		newtio.c_cflag &= ~CSTOPB;
	} else if (nStop == 2) {
		newtio.c_cflag |= CSTOPB;
	}
	newtio.c_cc[VTIME] = 0;
	newtio.c_cc[VMIN] = 0;
	tcflush(uart_fileHandler, TCIFLUSH);
	if ((tcsetattr(uart_fileHandler, TCSANOW, &newtio)) != 0) {
		perror("com set error");
		return -1;
	}
	printf("set done!\n");
	/* Create a corresponding file descriptor */
//	        {
//	                jclass cFileDescriptor = (*env)->FindClass(env, "java/io/FileDescriptor");
//	                jmethodID iFileDescriptor = (*env)->GetMethodID(env, cFileDescriptor, "<init>", "()V");
//	                jfieldID descriptorID = (*env)->GetFieldID(env, cFileDescriptor, "descriptor", "I");
//	                mFileDescriptor = (*env)->NewObject(env, cFileDescriptor, iFileDescriptor);
//	                (*env)->SetIntField(env, mFileDescriptor, descriptorID, (jint)uart_fileHandler);
//	        }
//
//	        return mFileDescriptor;
	return 0;

}

//i2c ms test

static int i2c_fileHandler = -1;
JNIEXPORT jint JNICALL Java_com_actions_jni_i2c_openi2c(JNIEnv *env,
		jobject obj, jint key) {
	switch (key) {
	case 0:
		i2c_fileHandler = open("/dev/i2c-0", O_RDWR);
		ALOGE("open /dev/i2c-0 %d", i2c_fileHandler);
		break;
	case 1:
		i2c_fileHandler = open("/dev/i2c-1", O_RDWR);
		ALOGE("open /dev/i2c-1 %d", i2c_fileHandler);
		break;
	case 2:
		i2c_fileHandler = open("/dev/i2c-2", O_RDWR);
		ALOGE("open /dev/i2c-2 %d", i2c_fileHandler);
		break;
	case 3:
		i2c_fileHandler = open("/dev/i2c-3", O_RDWR);
		ALOGE("open /dev/i2c-3 %d", i2c_fileHandler);
		break;
	default:
		break;
	}

	if (i2c_fileHandler < 0) {
		ALOGE("open /dev/i2c-%d fail",key);
	}

	return i2c_fileHandler;
}
JNIEXPORT jint JNICALL Java_com_actions_jni_i2c_close(JNIEnv *env, jobject obj) {
	if (i2c_fileHandler > 0) {
		close(i2c_fileHandler);
	}
	return i2c_fileHandler;
}
JNIEXPORT jint JNICALL Java_com_actions_jni_i2c_setmodei2c(JNIEnv *env,
		jobject obj) {
	//ALOGE("sendcmd %d %d", cmd ,arg);
	int i2cdev_addr = 0x50;
	int err = ioctl(i2c_fileHandler, I2C_SLAVE_FORCE, i2cdev_addr);
	if (err == -1) {
		printf("can't set i2c mode\n");
	}
	return err;
}
// jstring to char*
char* JstringToPchar(JNIEnv* env, jstring jstr, const char * encoding) {
	char* rtn = NULL;
	jstring jencoding;
	jclass gStringClass = (*env)->FindClass(env, "java/lang/String");
	jmethodID gmidStringGetBytes = (*env)->GetMethodID(env, gStringClass,
			"getBytes", "(Ljava/lang/String;)[B");
	jencoding = (*env)->NewStringUTF(env, encoding);
	jbyteArray barr = (jbyteArray)(*env)->CallObjectMethod(env, jstr,
			gmidStringGetBytes, jencoding);
	jsize alen = (*env)->GetArrayLength(env, barr);
	jbyte* ba = (*env)->GetByteArrayElements(env, barr, JNI_FALSE);
	if (alen > 0) {
		//LOGI("alen = %d ",alen);
		rtn = (char*) malloc(alen + 1);
		memcpy(rtn, ba, alen);
		rtn[alen] = 0;
	}
	(*env)->ReleaseByteArrayElements(env, barr, ba, 0);

	return rtn;
}
//char* to jstring
jstring PcharToJstring(JNIEnv* env, const char* pchar, const char * encoding) {
	jstring jencoding;
	jclass gStringClass = (*env)->FindClass(env, "java/lang/String");
	jmethodID gmidStringInit = (*env)->GetMethodID(env, gStringClass, "<init>",
			"([BLjava/lang/String;)V");
	jbyteArray bytes = (*env)->NewByteArray(env, strlen(pchar));
	(*env)->SetByteArrayRegion(env, bytes, 0, strlen(pchar), (jbyte*) pchar);
	jencoding = (*env)->NewStringUTF(env, encoding);
	return (jstring)(*env)->NewObject(env, gStringClass, gmidStringInit, bytes,
			jencoding);
}
static int i2c_len = -1;
JNIEXPORT jstring JNICALL Java_com_actions_jni_i2c_readi2c(JNIEnv *env,
		jobject obj, jint size) {
	ALOGE("read size = %d", size);
	char re[size + 1];
	memset(re, 0x0, size + 1);
	int ret = read(i2c_fileHandler, re, size);
	if (ret < 0) {
		ALOGE("read i2c dev fail !!!");
	}
	ALOGE("readi2c string:%s", re);
	return PcharToJstring(env, re, "utf-8");

}

JNIEXPORT jint JNICALL Java_com_actions_jni_i2c_writei2c(JNIEnv *env,
		jobject obj, jstring s, jint size) {
	//const char *str = (*env)->GetStringUTFChars(env, s, 0);
	char *str = JstringToPchar(env, s, "utf-8");
//	 ALOGE("strlen(str) = %d",strlen(str));
//	 		int i;
//	 		for(i = 0 ; i < strlen(str);i++)
//	 			ALOGE("str[%d] = %c",i,str[i]);
	int ret = write(i2c_fileHandler, str, strlen(str));
	usleep(10000);
	ALOGE(" ALOGE writei2c string:%s   strlen(str):%d   realwrite:%d", str,
			strlen(str), ret);
	if (ret < 0) {
		ALOGE("write i2c dev fail !!!");
	}
	(*env)->ReleaseStringUTFChars(env, s, str);
	return ret;
}

//spi  test
#define	SPI_CPHA	0x01			/* clock phase */
#define	SPI_CPOL	0x02			/* clock polarity */

static int spi_fileHandler = -1;
JNIEXPORT jint JNICALL Java_com_actions_jni_spi_openspi(JNIEnv *env,
		jobject obj, jint key) {

	switch (key) {
	case 0:
		spi_fileHandler = open("/dev/spidev0.0", O_RDWR);
		ALOGE("open /dev/spidev0.0 %d", spi_fileHandler);
		break;
	case 1:
		spi_fileHandler = open("/dev/spidev1.0", O_RDWR);
		ALOGE("open /dev/spidev1.0 %d", spi_fileHandler);
		break;
	case 2:
		spi_fileHandler = open("/dev/spidev2.0", O_RDWR);
		ALOGE("open /dev/spidev2.0 %d", spi_fileHandler);
		break;
	case 3:
		spi_fileHandler = open("/dev/spidev3.0", O_RDWR);
		ALOGE("open /dev/spidev3.0 %d", spi_fileHandler);
		break;
	default:
		break;
	}

	if (spi_fileHandler < 0) {
		ALOGE("open /dev/spidev%d.0 fail",key);
	}
	return spi_fileHandler;
}

JNIEXPORT jint JNICALL Java_com_actions_jni_spi_close(JNIEnv *env, jobject obj) {
	if (spi_fileHandler > 0) {
		close(spi_fileHandler);
	}
	return spi_fileHandler;
}
JNIEXPORT jint JNICALL Java_com_actions_jni_spi_setmode(JNIEnv *env,
		jobject obj) {
	//ALOGE("sendcmd %d %d", cmd ,arg);
	int mode = SPI_CPHA | SPI_CPOL;
	int err = ioctl(spi_fileHandler, SPI_IOC_WR_MODE, &mode);
	if (err == -1) {
		printf("can't set spi mode\n");
	}
	return err;
}

JNIEXPORT jstring JNICALL Java_com_actions_jni_spi_readspi(JNIEnv *env,
		jobject obj, jint size) {
	ALOGE("read size = %d", size);
	char re[size + 1];
	memset(re, 0x0, size + 1);
	int ret = read(spi_fileHandler, re, size);
	if (ret < 0) {
		ALOGE("read spi dev fail !!!");
	}
	return PcharToJstring(env, re, "utf-8");

}

JNIEXPORT jint JNICALL Java_com_actions_jni_spi_writespi(JNIEnv *env,
		jobject obj, jstring s, jint size) {

	char *str = JstringToPchar(env, s, "utf-8");
	int ret = write(spi_fileHandler, str, strlen(str));
	usleep(10000);
	ALOGE("string:%s", str);
	if (ret < 0) {
		ALOGE("write spi dev fail !!!");
	}

	(*env)->ReleaseStringUTFChars(env, s, str);

	return ret;
}

//GPIO test
static const char *CONST_GPIO_TEST_DEV = "/dev/gpio_owl_test";
static int gpio_fileHandler = -1;
JNIEXPORT jint JNICALL Java_com_actions_jni_gpio_open(JNIEnv *env, jobject obj) {
	gpio_fileHandler = open(CONST_GPIO_TEST_DEV, O_RDWR);
	ALOGE("open /dev/gpio_owl_test %d", gpio_fileHandler);

	if (gpio_fileHandler < 0) {
		ALOGE("open /dev/gpio_owl_test fail");
	}

	return gpio_fileHandler;
}

JNIEXPORT jlong JNICALL Java_com_actions_jni_gpio_sendcmd(JNIEnv *env,
		jobject obj, jint cmd, jint arg) {
	ALOGE("sendcmd %d %d", cmd, arg);
	long ret = 0;

	if (gpio_fileHandler >= 0) {
		ret = ioctl(gpio_fileHandler, cmd, arg);
	}
	return ret;
}

JNIEXPORT jint JNICALL Java_com_actions_jni_gpio_close(JNIEnv *env, jobject obj) {
	if (gpio_fileHandler > 0) {
		close(gpio_fileHandler);
	}
	return gpio_fileHandler;
}

//PWMADC test
static const char *CONST_ADC_TEST_DEV = "/dev/adc_owl_test";
static int adc_fileHandler = -1;
JNIEXPORT jint JNICALL Java_com_actions_jni_adc_openadc(JNIEnv *env,
		jobject obj) {
	adc_fileHandler = open(CONST_ADC_TEST_DEV, O_RDWR);
	ALOGE("open /dev/adc_owl_test %d", adc_fileHandler);

	if (adc_fileHandler < 0) {
		ALOGE("open /dev/adc_owl_test fail");
	}

	return adc_fileHandler;
}
JNIEXPORT jint JNICALL Java_com_actions_jni_adc_close(JNIEnv *env, jobject obj) {
	if (adc_fileHandler > 0) {
		close(adc_fileHandler);
	}
	return adc_fileHandler;
}
JNIEXPORT jlong JNICALL Java_com_actions_jni_adc_sendcmdadc(JNIEnv *env,
		jobject obj, jint cmd, jint arg) {
	ALOGE("sendcmd %d %d", cmd, arg);
	long ret = 0;

	if (adc_fileHandler >= 0) {
		ret = ioctl(adc_fileHandler, cmd, arg);
	}
	return ret;
}
JNIEXPORT jint JNICALL Java_com_actions_jni_adc_readadccase(JNIEnv *env,
		jobject obj) {
	int ret = 0;
	char cases[1] = "0";
	memset(cases, 0, 1);

	if (adc_fileHandler >= 0) {
		read(adc_fileHandler, cases, 1);
	}
	ALOGE("readadccase buf %d  %d", cases[0], adc_fileHandler);
	ret = cases[0];
	return ret;
}
static const char *CONST_PWM_TEST_DEV = "/dev/pwm_owl_test";
static int pwm_fileHandler = -1;
JNIEXPORT jint JNICALL Java_com_actions_jni_pwm_openpwm(JNIEnv *env,
		jobject obj) {
	pwm_fileHandler = open(CONST_PWM_TEST_DEV, O_RDWR);
	ALOGE("open /dev/pwm_owl_test %d", pwm_fileHandler);

	if (pwm_fileHandler < 0) {
		ALOGE("open /dev/pwm_owl_test fail");
	}

	return pwm_fileHandler;
}
JNIEXPORT jint JNICALL Java_com_actions_jni_pwm_close(JNIEnv *env, jobject obj) {
	if (pwm_fileHandler > 0) {
		close(pwm_fileHandler);
	}
	return pwm_fileHandler;
}
JNIEXPORT jlong JNICALL Java_com_actions_jni_pwm_sendcmdpwm(JNIEnv *env,
		jobject obj, jint cmd, jint arg) {
	ALOGE("sendcmd %d %d", cmd, arg);
	long ret = 0;

	if (pwm_fileHandler >= 0) {
		ret = ioctl(pwm_fileHandler, cmd, arg);
	}
	return ret;
}

