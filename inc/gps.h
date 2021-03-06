/*
 * Copyright (c) 2020, RudyLo <luhuadong@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-08-20     luhuadong    the first version
 */

#ifndef __GPS_H__
#define __GPS_H__

#include <rtthread.h>
#include <rtdevice.h>
#include <sensor.h>

#define GPSLIB_VERSION       "0.0.1"

#define GPS_READ_WAIT_TIME   10000

struct gnrmc
{
	double     lon;      /* GPS longitude and latitude */
	double     lat;
    char       lon_area;
    char       lat_area;
    rt_uint8_t time_H;   /* Time */
    rt_uint8_t time_M;
    rt_uint8_t time_S;
    rt_uint8_t status;   /* 1: Successful positioning, 0：Positioning failed */
};
typedef struct gnrmc GNRMC_t;

struct coord
{
    double lon;
    double lat;
};
typedef struct coord coord_t;

struct dms
{
    int degrees;
    int minutes;
    int seconds;
};

struct gps_response
{

};
typedef struct gps_response *gps_response_t;

struct gps_device
{
    rt_device_t  serial;
    struct rt_ringbuffer *rx_fifo;

#ifdef PKG_USING_GPS_UART_DMA
    rt_mailbox_t rx_mb;
#else
    rt_sem_t     rx_sem;
#endif
    rt_sem_t     tx_done;
    rt_sem_t     ack;
    rt_thread_t  rx_tid;

    struct gps_response resp;

    rt_mutex_t   lock;
    rt_uint8_t   version;
};
typedef struct gps_device *gps_device_t;

struct gps_client
{
    rt_device_t  device;

    gps_status_t status;
    char end_sign;

    /* the current received one line data buffer */
    char *recv_line_buf;
    /* The length of the currently received one line data */
    rt_size_t recv_line_len;
    /* The maximum supported receive data length */
    rt_size_t recv_bufsz;
    rt_sem_t rx_notice;
    rt_mutex_t lock;

    rt_sem_t resp_notice;
    rt_size_t urc_table_size;

    rt_thread_t parser;
};
typedef struct gps_client *gps_client_t;


gps_device_t gps_create(const char *uart_name);
void         gps_delete(gps_device_t dev);

rt_err_t     gps_send_command(gps_device_t dev, const char *data);
rt_uint16_t  gps_read(gps_device_t dev, void *buf, rt_uint16_t size, rt_int32_t time);
rt_uint16_t  gps_wait(gps_device_t dev, void *buf, rt_uint16_t size);
GNRMC_t      gps_gat_gnrmc(gps_device_t dev);

rt_bool_t    gps_is_ready(gps_device_t dev);

void         gps_show_response(gps_response_t resp);
void         gps_dump(const char *buf, rt_uint16_t size);

double       dms2decimal(struct dms);

rt_err_t rt_hw_gps_init(const char *name, struct rt_sensor_config *cfg);

#endif /* __GPS_H__ */
