/*
 * atc260x_rtc.c  --  RTC driver for ATC260X
 *
 * Copyright 2011 Actions Semi Inc.
 * Author: Actions Semi, Inc.
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */

/* UTF-8 encoded.
 * 因为N颗IC的RTC寄存器位定义都是一致的, 仅仅是寄存器的地址有差别, regdef仅仅覆盖到寄存器地址即可. */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/of_device.h>
#include <linux/rtc.h>
#include <linux/delay.h>
#include <linux/mfd/atc260x/atc260x.h>

#define RTC_DEFAULT_START_YEAR      (2011)

/* RTC Control Register */
#define RTC_CTL_ALIP                (1 << 0)
#define RTC_CTL_ALIE                (1 << 1)
#define RTC_CTL_EXT_LOSC_STATUS     (1 << 3)
#define RTC_CTL_RTCE                (1 << 4)
#define RTC_CTL_CKSS0               (1 << 5)
#define RTC_CTL_EOSC                (1 << 6)
#define RTC_CTL_LEAP                (1 << 9)
#define RTC_CTL_VER1                (1 << 10)
#define RTC_CTL_RST                 (1 << 11)
#define RTC_CTL_LGS                 (1 << 11)


#define RTC_YMD_Y_SHIFT             (9)
#define RTC_YMD_Y_MASK              (0x7f << RTC_YMD_Y_SHIFT)
#define RTC_YMD_M_SHIFT             (5)
#define RTC_YMD_M_MASK              (0xf << RTC_YMD_M_SHIFT)
#define RTC_YMD_D_SHIFT             (0)
#define RTC_YMD_D_MASK              (0x1f << RTC_YMD_D_SHIFT)

#define RTC_H_H_SHIFT               (0)
#define RTC_H_H_MASK                (0x1f << RTC_H_H_SHIFT)

#define RTC_MS_M_SHIFT              (6)
#define RTC_MS_M_MASK               (0x3f << RTC_MS_M_SHIFT)
#define RTC_MS_S_SHIFT              (0)
#define RTC_MS_S_MASK               (0x3f << RTC_MS_S_SHIFT)

#define RTC_DC_D_SHIFT              (7)
#define RTC_DC_D_MASK               (0x7 << RTC_DC_D_SHIFT)
#define RTC_DC_C_SHIFT              (0)
#define RTC_DC_C_MASK               (0x7f << RTC_DC_C_SHIFT)

#define RTC_YMD_Y(ymd)              (((ymd) & RTC_YMD_Y_MASK) >> RTC_YMD_Y_SHIFT)
#define RTC_YMD_M(ymd)              (((ymd) & RTC_YMD_M_MASK) >> RTC_YMD_M_SHIFT)
#define RTC_YMD_D(ymd)              (((ymd) & RTC_YMD_D_MASK) >> RTC_YMD_D_SHIFT)
#define RTC_YMD_VAL(y, m, d)        (((y) << RTC_YMD_Y_SHIFT) | ((m) << RTC_YMD_M_SHIFT) | ((d) << RTC_YMD_D_SHIFT))

#define RTC_H_H(h)                  (((h) & RTC_H_H_MASK) >> RTC_H_H_SHIFT)
#define RTC_H_VAL(h)                (((h) << RTC_H_H_SHIFT))

#define RTC_MS_M(ms)                (((ms) & RTC_MS_M_MASK) >> RTC_MS_M_SHIFT)
#define RTC_MS_S(ms)                (((ms) & RTC_MS_S_MASK) >> RTC_MS_S_SHIFT)
#define RTC_MS_VAL(m, s)            (((m) << RTC_MS_M_SHIFT) | ((s) << RTC_MS_S_SHIFT))

#define RTC_DC_D(dc)                (((dc) & RTC_DC_D_MASK) >> RTC_DC_D_SHIFT)
#define RTC_DC_C(dc)                (((dc) & RTC_DC_C_MASK) >> RTC_DC_C_SHIFT)
#define RTC_DC_VAL(d, c)            (((d) << RTC_DC_D_SHIFT) | ((c) << RTC_DC_C_SHIFT))


#define REG_RTC_CTL                 0
#define REG_RTC_MSALM               1
#define REG_RTC_HALM                2
#define REG_RTC_YMDALM              3
#define REG_RTC_MS                  4
#define REG_RTC_H                   5
#define REG_RTC_DC                  6
#define REG_RTC_YMD                 7

struct atc260x_rtc_dev {
	struct atc260x_dev *atc260x;
	struct rtc_device *rtc;
	uint regbase;
	uint alarm_enabled;
	int  alm_irq;
};

/*
 * Read current time and date in RTC
 */
static int atc260x_rtc_readtime(struct device *dev, struct rtc_time *tm)
{
	struct atc260x_rtc_dev *atc260x_rtc = dev_get_drvdata(dev);
	struct atc260x_dev *atc260x = atc260x_rtc->atc260x;
	int ymd, h, ms, dc, century;

	ymd = atc260x_reg_read(atc260x, atc260x_rtc->regbase+REG_RTC_YMD);
	if (ymd < 0) {
		dev_err(dev, "%s() %u: read reg err, ret=%d\n", __func__, __LINE__, ymd);
		return ymd;
	}

	h = atc260x_reg_read(atc260x, atc260x_rtc->regbase+REG_RTC_H);
	if (h < 0) {
		dev_err(dev, "%s() %u: read reg err, ret=%d\n", __func__, __LINE__, h);
		return h;
	}

	ms = atc260x_reg_read(atc260x, atc260x_rtc->regbase+REG_RTC_MS);
	if (ms < 0) {
		dev_err(dev, "%s() %u: read reg err, ret=%d\n", __func__, __LINE__, ms);
		return ms;
	}

	dc = atc260x_reg_read(atc260x, atc260x_rtc->regbase+REG_RTC_DC);
	if (dc < 0) {
		dev_err(dev, "%s() %u: read reg err, ret=%d\n", __func__, __LINE__, dc);
		return dc;
	}

	century = RTC_DC_C(dc);
	if (century < 19) {
		/* default: 2011 */
		tm->tm_year = RTC_YMD_Y(ymd) +  RTC_DEFAULT_START_YEAR - 1900;
	} else {
		tm->tm_year = RTC_YMD_Y(ymd) +  century * 100 - 1900;
	}

	tm->tm_mon = RTC_YMD_M(ymd) - 1;
	tm->tm_mday = RTC_YMD_D(ymd);
	tm->tm_hour = RTC_H_H(h);
	tm->tm_min = RTC_MS_M(ms);
	tm->tm_sec = RTC_MS_S(ms);
	tm->tm_wday = RTC_DC_D(dc);

	dev_dbg(dev, "%s(): %4d-%02d-%02d %02d:%02d:%02d\n", __FUNCTION__,
		1900 + tm->tm_year, tm->tm_mon + 1, tm->tm_mday,
		tm->tm_hour, tm->tm_min, tm->tm_sec);

	return rtc_valid_tm(tm);
}


static int atc260x_rtc_settime(struct device *dev, struct rtc_time *tm)
{
	struct atc260x_rtc_dev *atc260x_rtc = dev_get_drvdata(dev);
	struct atc260x_dev *atc260x = atc260x_rtc->atc260x;
	int ret, century, year;

	dev_info(dev, "%s(): %4d-%02d-%02d %02d:%02d:%02d\n", __FUNCTION__,
		1900 + tm->tm_year, tm->tm_mon + 1, tm->tm_mday,
		tm->tm_hour, tm->tm_min, tm->tm_sec);

	century = (tm->tm_year + 1900) / 100;
	year = tm->tm_year % 100;

	/* Stop RTC while updating the TC registers */
	ret = atc260x_reg_wp_setbits(atc260x, atc260x_rtc->regbase+REG_RTC_CTL, RTC_CTL_ALIP, RTC_CTL_RTCE, 0);
	if (ret < 0) {
		dev_err(dev, "%s() %u: write reg err, ret=%d\n", __func__, __LINE__, ret);
		return ret;
	}

	/* update all the time registers in one shot */

	ret = atc260x_reg_write(atc260x, atc260x_rtc->regbase+REG_RTC_YMD,
				   RTC_YMD_VAL(year, tm->tm_mon + 1, tm->tm_mday));
	if (ret < 0) {
		dev_err(dev, "%s() %u: write reg err, ret=%d\n", __func__, __LINE__, ret);
		return ret;
	}

	ret = atc260x_reg_write(atc260x, atc260x_rtc->regbase+REG_RTC_H,
		RTC_H_VAL(tm->tm_hour));
	if (ret < 0) {
		dev_err(dev, "%s() %u: write reg err, ret=%d\n", __func__, __LINE__, ret);
		return ret;
	}

	ret = atc260x_reg_write(atc260x, atc260x_rtc->regbase+REG_RTC_MS,
		RTC_MS_VAL(tm->tm_min, tm->tm_sec));
	if (ret < 0) {
		dev_err(dev, "%s() %u: write reg err, ret=%d\n", __func__, __LINE__, ret);
		return ret;
	}

	ret = atc260x_reg_write(atc260x, atc260x_rtc->regbase+REG_RTC_DC,
		RTC_DC_VAL(tm->tm_wday, century));
	if (ret < 0) {
		dev_err(dev, "%s() %u: write reg err, ret=%d\n", __func__, __LINE__, ret);
		return ret;
	}

	/* Start RTC */
	ret = atc260x_reg_wp_clrpnd(atc260x, atc260x_rtc->regbase+REG_RTC_CTL, RTC_CTL_ALIP, RTC_CTL_ALIP);
	if (ret < 0) {
		dev_err(dev, "%s() %u: write reg err, ret=%d\n", __func__, __LINE__, ret);
		return ret;
	}
	ret = atc260x_reg_wp_setbits(atc260x, atc260x_rtc->regbase+REG_RTC_CTL, RTC_CTL_ALIP, RTC_CTL_RTCE, RTC_CTL_RTCE);
	if (ret < 0) {
		dev_err(dev, "%s() %u: write reg err, ret=%d\n", __func__, __LINE__, ret);
		return ret;
	}

	return ret;
}

/*
 * Read alarm time and date in RTC
 */
static int atc260x_rtc_readalarm(struct device *dev, struct rtc_wkalrm *alrm)
{
	struct atc260x_rtc_dev *atc260x_rtc = dev_get_drvdata(dev);
	struct atc260x_dev *atc260x = atc260x_rtc->atc260x;
	int ret, ymd, h, ms, dc, century;

	ymd = atc260x_reg_read(atc260x, atc260x_rtc->regbase+REG_RTC_YMDALM);
	if (ymd < 0) {
		dev_err(dev, "%s() %u: read reg err, ret=%d\n", __func__, __LINE__, ymd);
		return ymd;
	}

	h = atc260x_reg_read(atc260x, atc260x_rtc->regbase+REG_RTC_HALM);
	if (h < 0) {
		dev_err(dev, "%s() %u: read reg err, ret=%d\n", __func__, __LINE__, h);
		return h;
	}

	ms = atc260x_reg_read(atc260x, atc260x_rtc->regbase+REG_RTC_MSALM);
	if (ms < 0) {
		dev_err(dev, "%s() %u: read reg err, ret=%d\n", __func__, __LINE__, ms);
		return ms;
	}

	dc = atc260x_reg_read(atc260x, atc260x_rtc->regbase+REG_RTC_DC);
	if (dc < 0) {
		dev_err(dev, "%s() %u: read reg err, ret=%d\n", __func__, __LINE__, dc);
		return dc;
	}

	century = RTC_DC_C(dc);
	if (century < 19) {
		/* default: 2011 */
		alrm->time.tm_year = RTC_YMD_Y(ymd) +  RTC_DEFAULT_START_YEAR - 1900;
	} else {
		alrm->time.tm_year = RTC_YMD_Y(ymd) +  century * 100 - 1900;
	}

	alrm->time.tm_mon = RTC_YMD_M(ymd) - 1;
	alrm->time.tm_mday = RTC_YMD_D(ymd);
	alrm->time.tm_hour = RTC_H_H(h);
	alrm->time.tm_min = RTC_MS_M(ms);
	alrm->time.tm_sec = RTC_MS_S(ms);

	ret = atc260x_reg_read(atc260x_rtc->atc260x, atc260x_rtc->regbase+REG_RTC_CTL);
	if (ret < 0) {
		dev_err(dev, "%s() %u: read reg err, ret=%d\n", __func__, __LINE__, ret);
		return ret;
	}

	if (ret & RTC_CTL_ALIE)
		alrm->enabled = 1;
	else
		alrm->enabled = 0;

	return 0;
}

static int atc260x_rtc_stop_alarm(struct atc260x_rtc_dev *atc260x_rtc)
{
	atc260x_rtc->alarm_enabled = 0;

	return atc260x_reg_wp_setbits(atc260x_rtc->atc260x, atc260x_rtc->regbase+REG_RTC_CTL,
				   RTC_CTL_ALIP, RTC_CTL_ALIE, 0);
}

static int atc260x_rtc_start_alarm(struct atc260x_rtc_dev *atc260x_rtc)
{
	int ret;

	atc260x_rtc->alarm_enabled = 1;

	ret = atc260x_reg_wp_clrpnd(atc260x_rtc->atc260x, atc260x_rtc->regbase+REG_RTC_CTL,
				   RTC_CTL_ALIP, RTC_CTL_ALIP);
	if (ret)
		return ret;
	return atc260x_reg_wp_setbits(atc260x_rtc->atc260x, atc260x_rtc->regbase+REG_RTC_CTL,
				   RTC_CTL_ALIP, RTC_CTL_ALIE, RTC_CTL_ALIE);
}

static int atc260x_rtc_setalarm(struct device *dev, struct rtc_wkalrm *alrm)
{
	struct atc260x_rtc_dev *atc260x_rtc = dev_get_drvdata(dev);
	struct atc260x_dev *atc260x = atc260x_rtc->atc260x;
	int ret, year;

	struct rtc_time *tm = &alrm->time;
	if (tm != NULL) {
		dev_info(dev, "%s(): %4d-%02d-%02d %02d:%02d:%02d\n", __FUNCTION__,
			1900 + tm->tm_year, tm->tm_mon + 1, tm->tm_mday,
			tm->tm_hour, tm->tm_min, tm->tm_sec);
	}

	ret = atc260x_rtc_stop_alarm(atc260x_rtc);
	if (ret < 0) {
		dev_err(dev, "Failed to stop alarm: %d\n", ret);
		return ret;
	}

	year = alrm->time.tm_year % 100;

	ret = atc260x_reg_write(atc260x, atc260x_rtc->regbase+REG_RTC_YMDALM,
		RTC_YMD_VAL(year, alrm->time.tm_mon + 1, alrm->time.tm_mday));
	if (ret < 0) {
		dev_err(dev, "%s() %u: write reg err, ret=%d\n", __func__, __LINE__, ret);
		return ret;
	}

	ret = atc260x_reg_write(atc260x, atc260x_rtc->regbase+REG_RTC_HALM,
		RTC_H_VAL(alrm->time.tm_hour));
	if (ret < 0) {
		dev_err(dev, "%s() %u: write reg err, ret=%d\n", __func__, __LINE__, ret);
		return ret;
	}

	ret = atc260x_reg_write(atc260x, atc260x_rtc->regbase+REG_RTC_MSALM,
		RTC_MS_VAL(alrm->time.tm_min, alrm->time.tm_sec));
	if (ret < 0) {
		dev_err(dev, "%s() %u: write reg err, ret=%d\n", __func__, __LINE__, ret);
		return ret;
	}

	if (alrm->enabled) {
		ret = atc260x_rtc_start_alarm(atc260x_rtc);
		if (ret < 0) {
			dev_err(dev, "Failed to start alarm: %d\n", ret);
			return ret;
		}
	}

	return 0;
}

static int atc260x_rtc_alarm_irq_enable(struct device *dev,
					   unsigned int enabled)
{
	struct atc260x_rtc_dev *atc260x_rtc = dev_get_drvdata(dev);

	if (enabled)
		return atc260x_rtc_start_alarm(atc260x_rtc);
	else
		return atc260x_rtc_stop_alarm(atc260x_rtc);
}


static irqreturn_t atc260x_alm_irq(int irq, void *data)
{
	struct atc260x_rtc_dev *atc260x_rtc = data;
	struct atc260x_dev *atc260x = atc260x_rtc->atc260x;
	struct device *dev = atc260x_rtc->rtc->dev.parent;
	int ret;

	dev_info(dev, "alarm irq isr\n");

	/* clear alarm IRQ pending */
	ret = atc260x_reg_wp_clrpnd(atc260x, atc260x_rtc->regbase+REG_RTC_CTL, RTC_CTL_ALIP, RTC_CTL_ALIP);
	if (ret != 0) {
		dev_err(dev, "%s() %u: write reg err, ret=%d\n", __func__, __LINE__, ret);
	}

	rtc_update_irq(atc260x_rtc->rtc, 1, RTC_IRQF | RTC_AF);

	return IRQ_HANDLED;
}

static const struct rtc_class_ops atc260x_rtc_ops = {
	.read_time          = atc260x_rtc_readtime,
	.set_time           = atc260x_rtc_settime,
	.read_alarm         = atc260x_rtc_readalarm,
	.set_alarm          = atc260x_rtc_setalarm,
	.alarm_irq_enable   = atc260x_rtc_alarm_irq_enable,
};

#ifdef CONFIG_PM
/* Turn off the alarm if it should not be a wake source. */
static int atc260x_rtc_suspend(struct device *dev)
{
	struct platform_device *pdev;
	struct atc260x_rtc_dev *atc260x_rtc;
	int ret, enable;

	dev_info(dev, "%s() enter\n", __func__);

	pdev = to_platform_device(dev);
	atc260x_rtc = dev_get_drvdata(&pdev->dev);

	if (atc260x_rtc->alarm_enabled && device_may_wakeup(&pdev->dev))
		enable = RTC_CTL_ALIE;
	else
		enable = 0;

	ret = atc260x_reg_wp_setbits(atc260x_rtc->atc260x, atc260x_rtc->regbase+REG_RTC_CTL,
				  RTC_CTL_ALIP, RTC_CTL_ALIE, enable);
	if (ret != 0)
		dev_err(&pdev->dev, "Failed to update RTC alarm: %d\n", ret);

	return 0;
}

/* Enable the alarm if it should be enabled (in case it was disabled to
 * prevent use as a wake source).
 */
static int atc260x_rtc_resume(struct device *dev)
{
	struct platform_device *pdev;
	struct atc260x_rtc_dev *atc260x_rtc;
	int ret;

	dev_info(dev, "%s() enter\n", __func__);

	pdev = to_platform_device(dev);
	atc260x_rtc = dev_get_drvdata(&pdev->dev);

	if (atc260x_rtc->alarm_enabled) {
		ret = atc260x_rtc_start_alarm(atc260x_rtc);
		if (ret != 0)
			dev_err(&pdev->dev, "Failed to restart RTC alarm: %d\n", ret);
	}

	return 0;
}

/* Unconditionally disable the alarm */
static int atc260x_rtc_freeze(struct device *dev)
{
	struct platform_device *pdev;
	struct atc260x_rtc_dev *atc260x_rtc;
	int ret;

	dev_info(dev, "%s() enter\n", __func__);

	pdev = to_platform_device(dev);
	atc260x_rtc = dev_get_drvdata(&pdev->dev);

	ret = atc260x_reg_wp_setbits(atc260x_rtc->atc260x, atc260x_rtc->regbase+REG_RTC_CTL,
				  RTC_CTL_ALIP, RTC_CTL_ALIE, 0);
	if (ret != 0)
		dev_err(&pdev->dev, "Failed to stop RTC alarm: %d\n", ret);

	return 0;
}
#else
#define atc260x_rtc_suspend NULL
#define atc260x_rtc_resume NULL
#define atc260x_rtc_freeze NULL
#endif

#if 0
static int check_default_year(struct atc260x_rtc_dev *atc260x_rtc, int default_year)
{
	struct device *dev = atc260x_rtc->rtc->dev.parent;
	struct rtc_time tm;
	int ret;

	if (default_year < 1900)
		return -EINVAL;

	ret = atc260x_rtc_readtime(dev, &tm);
	if (ret)
		return ret;

	/* atc260x_rtc_readtime() doesn't return real year if the year < 1900,
	 * so the following code is dead code.
	 */
	if (tm.tm_year < (default_year - 1900)) {
		dev_err(dev, "invalid year(%d), reset to year %d\n",
			tm.tm_year + 1900, default_year);

		tm.tm_year = default_year - 1900;
		ret = atc260x_rtc_settime(dev, &tm);
		if (ret)
			return ret;
	}

	return 0;
}
#endif

/* take effect after 0.5s if reset time */
static int check_default_year(struct atc260x_rtc_dev *atc260x_rtc, int default_year)
{
	struct device *dev = atc260x_rtc->rtc->dev.parent;
	struct atc260x_dev *atc260x = atc260x_rtc->atc260x;
	int ymd, dc, century, year_in_century;

	if (default_year < 1900)
		return -EINVAL;

	ymd = atc260x_reg_read(atc260x, atc260x_rtc->regbase+REG_RTC_YMD);
	dc = atc260x_reg_read(atc260x, atc260x_rtc->regbase+REG_RTC_DC);
	century = RTC_DC_C(dc);
	if (century >= 19)
		return 0;

	dev_err(dev, "invalid year(%d), reset to year %d\n",
		century * 100 + RTC_YMD_Y(ymd), default_year);

	century = default_year / 100;
	year_in_century = default_year % 100;

	/* Stop RTC while updating the TC registers */
	atc260x_reg_wp_setbits(atc260x, atc260x_rtc->regbase+REG_RTC_CTL, RTC_CTL_ALIP, RTC_CTL_RTCE, 0);

	atc260x_reg_write(atc260x, atc260x_rtc->regbase+REG_RTC_DC,
		RTC_DC_VAL(RTC_DC_D(dc), century));

	atc260x_reg_write(atc260x, atc260x_rtc->regbase+REG_RTC_YMD,
		RTC_YMD_VAL(year_in_century, RTC_YMD_M(ymd), RTC_YMD_D(ymd)));

	/* Start RTC */
	atc260x_reg_wp_clrpnd(atc260x, atc260x_rtc->regbase+REG_RTC_CTL, RTC_CTL_ALIP, RTC_CTL_ALIP);
	atc260x_reg_wp_setbits(atc260x, atc260x_rtc->regbase+REG_RTC_CTL, RTC_CTL_ALIP, RTC_CTL_RTCE, RTC_CTL_RTCE);

	return 0;
}

/* init RTC hardware */
static int atc260x_rtc_hw_init(struct atc260x_rtc_dev *atc260x_rtc)
{
	struct atc260x_dev *atc260x = atc260x_rtc->atc260x;
	int ctl;

	/* check if external osc is available  */
	ctl = atc260x_reg_read(atc260x, atc260x_rtc->regbase+REG_RTC_CTL);
	if (!(ctl & RTC_CTL_EXT_LOSC_STATUS)) {
		atc260x_reg_wp_setbits(atc260x, atc260x_rtc->regbase+REG_RTC_CTL,
			RTC_CTL_ALIP, RTC_CTL_RTCE | RTC_CTL_EOSC, RTC_CTL_RTCE | RTC_CTL_EOSC);

		/* wait external osc ready */
		udelay(100);
		ctl = atc260x_reg_read(atc260x, atc260x_rtc->regbase+REG_RTC_CTL);
	}

	if (ctl & RTC_CTL_EXT_LOSC_STATUS) {
		/* choose external osc */
		atc260x_reg_wp_setbits(atc260x, atc260x_rtc->regbase+REG_RTC_CTL,
			RTC_CTL_ALIP, RTC_CTL_RTCE | RTC_CTL_CKSS0, RTC_CTL_RTCE | RTC_CTL_CKSS0);
		udelay(100);
	}

	return 0;
}

static ssize_t atc260x_rtc_sysfs_show_ext_osc(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct rtc_device *rtc = to_rtc_device(dev);
	struct atc260x_rtc_dev *atc260x_rtc = dev_get_drvdata(rtc->dev.parent);
	int ret;

	ret = atc260x_reg_read(atc260x_rtc->atc260x, atc260x_rtc->regbase+REG_RTC_CTL);
	if (ret < 0) {
		dev_err(dev, "Failed to read RTC control: ret=%d\n", ret);
		return ret;
	}

	if (ret & RTC_CTL_CKSS0)
		buf[0] = '1';
	else
		buf[0] = '0';
	buf[1] = '\n';
	buf[2] = 0;
	return 2;
}
static DEVICE_ATTR(ext_osc, S_IRUGO, atc260x_rtc_sysfs_show_ext_osc, NULL);

static int atc260x_rtc_probe(struct platform_device *pdev)
{
	struct atc260x_dev *atc260x;
	struct atc260x_rtc_dev *atc260x_rtc;
	uint ic_type;
	int alm_irq, ret = 0;

	dev_info(&pdev->dev, "Probing...\n");

	atc260x = atc260x_get_parent_dev(&pdev->dev);

	atc260x_rtc = devm_kzalloc(&pdev->dev, sizeof(*atc260x_rtc), GFP_KERNEL);
	if (atc260x_rtc == NULL)
		return -ENOMEM;

	platform_set_drvdata(pdev, atc260x_rtc);
	atc260x_rtc->atc260x = atc260x;

	/* select IC type. */
	ic_type = atc260x_get_ic_type(atc260x);
	switch (ic_type) {
	case ATC260X_ICTYPE_2603A:
		atc260x_rtc->regbase = ATC2603A_RTC_CTL;
		break;
	case ATC260X_ICTYPE_2603C:
		atc260x_rtc->regbase = ATC2603C_RTC_CTL;
		break;
	case ATC260X_ICTYPE_2609A:
		atc260x_rtc->regbase = ATC2609A_RTC_CTL;
		break;
	default :
		dev_dbg(&pdev->dev, "%s() unsupport IC_TYPE: 0x%x\n",
			__func__, ic_type);
		goto err;
	}

	ret = atc260x_reg_read(atc260x, atc260x_rtc->regbase+REG_RTC_CTL);
	if (ret < 0) {
		dev_err(&pdev->dev, "Failed to read RTC control: %d\n", ret);
		goto err;
	}
	if (ret & RTC_CTL_ALIE)
		atc260x_rtc->alarm_enabled = 1;

	atc260x_rtc_hw_init(atc260x_rtc);

	device_init_wakeup(&pdev->dev, 1);

	atc260x_rtc->rtc = rtc_device_register("atc260x-RTC", &pdev->dev,
						  &atc260x_rtc_ops, THIS_MODULE);
	if (IS_ERR(atc260x_rtc->rtc)) {
		ret = PTR_ERR(atc260x_rtc->rtc);
		goto err;
	}

	/* check default start year */
	check_default_year(atc260x_rtc, RTC_DEFAULT_START_YEAR);

	alm_irq = platform_get_irq(pdev, 0);
	dev_info(&pdev->dev, "RTC alarm IRQ num : %u\n", alm_irq);
	ret = devm_request_threaded_irq(&pdev->dev, alm_irq, NULL, atc260x_alm_irq,
				   IRQF_TRIGGER_HIGH, "RTC alarm",
				   atc260x_rtc);
	if (ret != 0) {
		dev_err(&pdev->dev, "Failed to request alarm IRQ %d: %d\n",
			alm_irq, ret);
	}
	atc260x_rtc->alm_irq = alm_irq;

	ret = device_create_file(&atc260x_rtc->rtc->dev, &dev_attr_ext_osc);
	if (ret) {
		dev_err(&pdev->dev, "failed to create ext_osc attribute, ret=%d\n", ret);
	}

	return 0;

err:
	platform_set_drvdata(pdev, NULL);
	return ret;
}

static int atc260x_rtc_remove(struct platform_device *pdev)
{
	struct atc260x_rtc_dev *atc260x_rtc = platform_get_drvdata(pdev);

	device_remove_file(&atc260x_rtc->rtc->dev, &dev_attr_ext_osc);
	devm_free_irq(&pdev->dev, atc260x_rtc->alm_irq, atc260x_rtc);
	rtc_device_unregister(atc260x_rtc->rtc);
	platform_set_drvdata(pdev, NULL);

	return 0;
}

static const struct dev_pm_ops atc260x_rtc_pm_ops = {
	.suspend = atc260x_rtc_suspend,
	.resume = atc260x_rtc_resume,

	.freeze = atc260x_rtc_freeze,
	.thaw = atc260x_rtc_resume,
	.restore = atc260x_rtc_resume,

	.poweroff = atc260x_rtc_suspend,
};


static const struct of_device_id atc260x_rtc_match[] = {
	{ .compatible = "actions,atc2603a-rtc", },
	{ .compatible = "actions,atc2603c-rtc", },
	{ .compatible = "actions,atc2609a-rtc", },
	{},
};
MODULE_DEVICE_TABLE(of, atc260x_rtc_match);

static struct platform_driver atc260x_rtc_driver = {
	.driver = {
		.name = "atc260x-rtc",
		.pm = &atc260x_rtc_pm_ops,
		.of_match_table = of_match_ptr(atc260x_rtc_match),
	},
	.probe = atc260x_rtc_probe,
	.remove = atc260x_rtc_remove,
};

module_platform_driver(atc260x_rtc_driver)

/*static int __init atc260x_rtc_init(void) */
/*{ */
/*  printk("%s() %d\n", __FUNCTION__, __LINE__); */
/*    return platform_driver_register(&atc260x_rtc_driver); */
/*} */
/*subsys_initcall(atc260x_rtc_init); */
/*static void __exit atc260x_rtc_exit(void) */
/*{ */
/*    platform_driver_unregister(&atc260x_rtc_driver); */
/*} */
/*module_exit(atc260x_rtc_exit); */

/* Module information */
MODULE_AUTHOR("Actions Semi, Inc");
MODULE_DESCRIPTION("RTC driver for ATC260X");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:atc260x-rtc");
