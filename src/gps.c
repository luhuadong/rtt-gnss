/*
 * Copyright (c) 2020, RudyLo <luhuadong@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-08-20     luhuadong    the first version
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <string.h>
#include "gps.h"

#define DBG_TAG "sensor.gps"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>


#define GPS_THREAD_STACK_SIZE          1024
#define GPS_THREAD_PRIORITY            (RT_THREAD_PRIORITY_MAX/2)

#define ntohs(x) ((((x)&0x00ffUL) << 8) | (((x)&0xff00UL) >> 8))

static int checksum(const char *s)
{
    int c = 0;

    while (*s)
        c ^= *s++;

    return c;
}

/**
 * Receive callback function
 */
static rt_err_t gps_uart_input(rt_device_t dev, rt_size_t size)
{
    RT_ASSERT(dev);
    gps_device_t gps = (gps_device_t)dev->user_data;

#ifdef PKG_USING_GPS_UART_DMA
    if (gps) rt_mb_send(gps->rx_mb, size);
#else
    if (gps) rt_sem_release(gps->rx_sem);
#endif

    return RT_EOK;
}

/** 
 * Cortex-M3 is Little endian usually
 */
static rt_bool_t is_little_endian(void)
{
    rt_uint16_t cell = 1;
    char *ptr = (char *)&cell;

    if (*ptr)
        return RT_TRUE;
    else
        return RT_FALSE;
}

static rt_err_t gps_check_frame(gps_device_t dev, const char *buf, rt_uint16_t size)
{
    RT_ASSERT(dev);

    return RT_EOK;
}

#ifdef PKG_USING_GPS_UART_DMA
static void gps_recv_thread_entry(void *parameter)
{
    gps_device_t dev = (gps_device_t)parameter;
}
#else
static void gps_recv_thread_entry(void *parameter)
{
    gps_device_t dev = (gps_device_t)parameter;
}
#endif

rt_err_t gps_send_command(gps_device_t dev, const char *data)
{
    RT_ASSERT(dev);

    return RT_EOK;
}

rt_uint16_t gps_read(gps_device_t dev, void *buf, rt_uint16_t size, rt_int32_t time)
{
    RT_ASSERT(dev);

    return size;
}

rt_uint16_t gps_wait(gps_device_t dev, void *buf, rt_uint16_t size)
{
    RT_ASSERT(dev);

    return size;
}

rt_bool_t gps_is_ready(gps_device_t dev)
{
    return RT_TRUE;
}

static void sensor_init_entry(void *parameter)
{
    gps_device_t dev = (gps_device_t)parameter;

    rt_uint16_t ret;
    struct gps_response resp;

    //gps_send_command(dev, "");

    ret = gps_read(dev, &resp, sizeof(resp), rt_tick_from_millisecond(GPS_READ_WAIT_TIME));
    if (ret != sizeof(resp))
    {
        LOG_E("Can't receive response from gps device");
        //gps_send_command(dev, "");
    }
}

/**
 * This function initializes gps registered device driver
 *
 * @param uart_name the name of serial device
 *
 * @return the gps device.
 */
gps_device_t gps_create(const char *uart_name)
{
    RT_ASSERT(uart_name);
    rt_err_t ret;

    gps_device_t dev = rt_calloc(1, sizeof(struct gps_device));
    if (dev == RT_NULL)
    {
        LOG_E("Can't allocate memory for gps device on '%s'", uart_name);
        return RT_NULL;
    }

    /* init uart */
    dev->serial = rt_device_find(uart_name);
    if (dev->serial == RT_NULL)
    {
        LOG_E("Can't find '%s' serial device", uart_name);
        goto __exit;
    }

    struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;

    config.baud_rate = BAUD_RATE_9600;
    config.data_bits = DATA_BITS_8;
    config.stop_bits = STOP_BITS_1;
    config.bufsz = RT_SERIAL_RB_BUFSZ;
    config.parity = PARITY_NONE;

    rt_device_control(dev->serial, RT_DEVICE_CTRL_CONFIG, &config);

    /* Dangerous? */
    dev->serial->user_data = (void *)dev;

    dev->ack  = rt_sem_create("gps_ack", 0, RT_IPC_FLAG_FIFO);
    if (dev->ack == RT_NULL)
    {
        LOG_E("Can't create semaphore for gps device");
        goto __exit;
    }

    /* init mutex */
    dev->lock = rt_mutex_create("gps_lock", RT_IPC_FLAG_FIFO);
    if (dev->lock == RT_NULL)
    {
        LOG_E("Can't create mutex for gps device");
        goto __exit;
    }

    /* create thread */
    dev->rx_tid = rt_thread_create("gps_rx", gps_recv_thread_entry, (void *)dev, 
                                   GPS_THREAD_STACK_SIZE, GPS_THREAD_PRIORITY, 10);
    if (dev->rx_tid == RT_NULL)
    {
        LOG_E("Can't create thread for gps device");
        goto __exit;
    }

    rt_thread_startup(dev->rx_tid);

    /* open UART device and enable UART RX */
#ifdef PKG_USING_GPS_UART_DMA
    ret = rt_device_open(dev->serial, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_DMA_RX);
#else
    ret = rt_device_open(dev->serial, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX);
#endif
    if (ret != RT_EOK)
    {
        LOG_E("Can't open '%s' serial device", uart_name);
        goto __exit;
    }

    rt_device_set_rx_indicate(dev->serial, gps_uart_input);

    /* run init thread or call init function */
#ifdef PKG_USING_GPS_INIT_ASYN
    rt_thread_t tid;

    tid = rt_thread_create("gps_init", sensor_init_entry, (void *)dev,
                            GPS_THREAD_STACK_SIZE, GPS_THREAD_PRIORITY, 10);
    if (tid)
    {
        rt_thread_startup(tid);
    }
    else
    {
        LOG_E("create sensor init thread failed.");
        goto __exit;
    }
#else
    sensor_init_entry(dev);

    if (!gps_is_ready(dev))
    {
        rt_device_close(dev->serial);
        goto __exit;
    }
#endif /* PKG_USING_GPS_INIT_ASYN */

    return dev;

__exit:
    if (dev->rx_tid)   rt_thread_delete(dev->rx_tid);
    if (dev->lock)     rt_mutex_delete(dev->lock);
#ifdef PKG_USING_GPS_UART_DMA
    if (dev->rx_mb)    rt_mb_delete(dev->rx_mb);
#else
    if (dev->rx_sem)   rt_sem_delete(dev->rx_sem);
#endif
    if (dev->ack)      rt_sem_delete(dev->ack);
    
    rt_free(dev);
    return RT_NULL;
}

/**
 * This function releases resources
 *
 * @param dev the pointer of device driver structure
 */
void gps_delete(gps_device_t dev)
{
    if (dev)
    {
        //gps_send_command(dev, "");
        dev->serial->user_data = RT_NULL;
        
        rt_sem_delete(dev->ack);
#ifdef PKG_USING_GPS_UART_DMA
        rt_mb_delete(dev->rx_mb);
#else
        rt_sem_delete(dev->rx_sem);
#endif
        rt_thread_delete(dev->rx_tid);
        rt_mutex_delete(dev->lock);
        rt_device_close(dev->serial);

        rt_free(dev);
    }
}
