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
#include "pmsxx.h"

#define DBG_TAG "sensor.plantower.pms"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>


#define PMS_THREAD_STACK_SIZE          1024
#define PMS_THREAD_PRIORITY            (RT_THREAD_PRIORITY_MAX/2)

#define ntohs(x) ((((x)&0x00ffUL) << 8) | (((x)&0xff00UL) >> 8))


static struct pms_cmd preset_commands[] = {
    {0x42, 0x4d, 0xe2, 0x00, 0x00, 0x01, 0x71}, /* Read in passive mode */
    {0x42, 0x4d, 0xe1, 0x00, 0x00, 0x01, 0x70}, /* Change to passive mode */
    {0x42, 0x4d, 0xe1, 0x00, 0x01, 0x01, 0x71}, /* Change to active  mode */
    {0x42, 0x4d, 0xe4, 0x00, 0x00, 0x01, 0x73}, /* Change to standby mode */
    {0x42, 0x4d, 0xe4, 0x00, 0x01, 0x01, 0x74}  /* Change to normal  mode */
};

void pms_show_command(pms_cmd_t cmd)
{
    RT_ASSERT(cmd);

    rt_kprintf("\n+-----------------------------------------------------+\n");
    rt_kprintf("| HEAD1 | HEAD2 |  CMD  | DATAH | DATAL | LRCH | LRCL |\n");
    rt_kprintf(" ----------------------------------------------------- \n");
    rt_kprintf("|   %02x  |   %02x  |   %02x  |   %02x  |   %02x  |  %02x  |  %02x  |\n", 
               cmd->START1, cmd->START2, cmd->CMD, cmd->DATAH, cmd->DATAL, cmd->LRCH, cmd->LRCL);
    rt_kprintf("+-----------------------------------------------------+\n");
}

void pms_show_response(pms_response_t resp)
{
    RT_ASSERT(resp);

#ifdef PKG_USING_PMSXX_BASIC
    rt_kprintf("\nResponse => len: %d bytes, version: %02X, Error: %02X\n", resp->length+4, resp->version, resp->errorCode);
    rt_kprintf("+-----------------------------------------------------+\n");
    rt_kprintf("|  CF=1  | PM1.0 = %-4d | PM2.5 = %-4d | PM10  = %-4d |\n", resp->PM1_0_CF1, resp->PM2_5_CF1, resp->PM10_0_CF1);
    rt_kprintf("|  atm.  | PM1.0 = %-4d | PM2.5 = %-4d | PM10  = %-4d |\n", resp->PM1_0_atm, resp->PM2_5_atm, resp->PM10_0_atm);
    rt_kprintf("|        | 0.3um = %-4d | 0.5um = %-4d | 1.0um = %-4d |\n", resp->air_0_3um, resp->air_0_5um, resp->air_1_0um);
    rt_kprintf("|        | 2.5um = %-4d | 5.0um = %-4d | 10um  = %-4d |\n", resp->air_2_5um, resp->air_5_0um, resp->air_10_0um);
    rt_kprintf("+-----------------------------------------------------+\n");
#endif
#ifdef PKG_USING_PMSXX_ENHANCED
    rt_kprintf("\nResponse => len: %d bytes, version: %02X, Error: %02X\n", resp->length+4, resp->version, resp->errorCode);
    rt_kprintf("+-----------------------------------------------------+\n");
    rt_kprintf("|  CF=1  | PM1.0 = %-4d | PM2.5 = %-4d | PM10  = %-4d |\n", resp->PM1_0_CF1, resp->PM2_5_CF1, resp->PM10_0_CF1);
    rt_kprintf("|  atm.  | PM1.0 = %-4d | PM2.5 = %-4d | PM10  = %-4d |\n", resp->PM1_0_atm, resp->PM2_5_atm, resp->PM10_0_atm);
    rt_kprintf("|        | 0.3um = %-4d | 0.5um = %-4d | 1.0um = %-4d |\n", resp->air_0_3um, resp->air_0_5um, resp->air_1_0um);
    rt_kprintf("|        | 2.5um = %-4d | 5.0um = %-4d | 10um  = %-4d |\n", resp->air_2_5um, resp->air_5_0um, resp->air_10_0um);
    rt_kprintf("| extra  | hcho  = %-4d | temp  = %-4d | humi  = %-4d |\n", resp->hcho, resp->temp, resp->humi);
    rt_kprintf("+-----------------------------------------------------+\n");
#endif
}

void pms_dump(const char *buf, rt_uint16_t size)
{
#ifdef PKG_USING_PMSXX_DEBUG_SHOW_RULER
#ifdef PKG_USING_PMSXX_BASIC
    rt_kprintf("\n_______________________________________________________________________________________________\n");
    rt_kprintf("01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32\n");
    rt_kprintf("-----------------------------------------------------------------------------------------------\n");
#endif
#ifdef PKG_USING_PMSXX_ENHANCED
    rt_kprintf("\n_______________________________________________________________________________________________________________________\n");
    rt_kprintf("01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39 40\n");
    rt_kprintf("-----------------------------------------------------------------------------------------------------------------------\n");
#endif
#endif
    for (rt_uint16_t i = 0; i < size; i++)
    {
        rt_kprintf("%02x ", buf[i]);
    }
    rt_kprintf("\n");
}

/**
 * Receive callback function
 */
static rt_err_t pms_uart_input(rt_device_t dev, rt_size_t size)
{
    RT_ASSERT(dev);
    pms_device_t pms = (pms_device_t)dev->user_data;

#ifdef PKG_USING_PMSXX_UART_DMA
    if (pms) rt_mb_send(pms->rx_mb, size);
#else
    if (pms) rt_sem_release(pms->rx_sem);
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

static rt_err_t pms_check_frame(pms_device_t dev, const char *buf, rt_uint16_t size)
{
    RT_ASSERT(dev);

    if (size < FRAME_LEN_MIN || size > FRAME_LEN_MAX)
        return -RT_ERROR;

    rt_uint16_t sum = 0, i;
    pms_response_t resp = &dev->resp;

#ifdef PKG_USING_PMSXX_DEBUG_DUMP_RESP
    pms_dump(buf, size);
#endif

    rt_mutex_take(dev->lock, RT_WAITING_FOREVER);
    rt_memcpy(resp, buf, size);

    resp->length     = ntohs(resp->length);
    resp->PM1_0_CF1  = ntohs(resp->PM1_0_CF1);
    resp->PM2_5_CF1  = ntohs(resp->PM2_5_CF1);
    resp->PM10_0_CF1 = ntohs(resp->PM10_0_CF1);
    resp->PM1_0_atm  = ntohs(resp->PM1_0_atm);
    resp->PM2_5_atm  = ntohs(resp->PM2_5_atm);
    resp->PM10_0_atm = ntohs(resp->PM10_0_atm);
    resp->air_0_3um  = ntohs(resp->air_0_3um);
    resp->air_0_5um  = ntohs(resp->air_0_5um);
    resp->air_1_0um  = ntohs(resp->air_1_0um);
    resp->air_2_5um  = ntohs(resp->air_2_5um);
    resp->air_5_0um  = ntohs(resp->air_5_0um);
    resp->air_10_0um = ntohs(resp->air_10_0um);
#ifdef PKG_USING_PMSXX_ENHANCED
    resp->hcho       = ntohs(resp->hcho);
    resp->temp       = ntohs(resp->temp);
    resp->humi       = ntohs(resp->humi);
#endif
    resp->checksum   = ntohs(resp->checksum);
    rt_mutex_release(dev->lock);

    for (i = 0; i < (size - 2); i++)
        sum += buf[i];

    if (sum != resp->checksum)
    {
        LOG_E("Checksum incorrect (%04x != %04x)", sum, resp->checksum);
        return -RT_ERROR;
    }

#ifdef PKG_USING_PMSXX_DEBUG_SHOW_RESP
    pms_show_response(resp);
#endif

    return RT_EOK;
}

#ifdef PKG_USING_PMSXX_UART_DMA
static void pms_recv_thread_entry(void *parameter)
{
    pms_device_t dev = (pms_device_t)parameter;
    
    char ch;
    pms_frame_t state = PMS_FRAME_END;
    rt_uint16_t idx = 0;
    rt_uint16_t frame_len = 0;
    rt_uint16_t frame_idx = 0;
    rt_ubase_t  size;
    rt_uint32_t len;
    rt_uint8_t  buf[RT_SERIAL_RB_BUFSZ];
    
    while (1)
    {
        if (RT_EOK != rt_mb_recv(dev->rx_mb, &size, RT_WAITING_FOREVER))
        {
            continue;
        }

        len = rt_device_read(dev->serial, 0, buf+idx, size);
        LOG_D("[recv thread] Receive %d bytes, read %d bytes", size, len);

        for (rt_uint32_t i=0; i<len; i++)
        {
            ch = buf[idx];
            switch (state)
            {
            case PMS_FRAME_END:
                if (ch == FRAME_START1)
                {
                    idx = 1;
                    state = PMS_FRAME_HEAD;
                }
                break;
            case PMS_FRAME_HEAD:
                if (ch == FRAME_START2)
                {
                    idx++;
                    state = PMS_FRAME_HEAD_ACK;
                }
                break;
            case PMS_FRAME_HEAD_ACK:
            {
                idx++;
                state = PMS_FRAME_LENGTH;
            }
            break;
            case PMS_FRAME_LENGTH:
            {
                idx++;
                frame_len = buf[idx - 2] << 8 | buf[idx - 1];
                frame_idx = 0;

                if (frame_len <= FRAME_LEN_MAX - 4)
                    state = PMS_FRAME_PAYLOAD;
                else
                    state = PMS_FRAME_END;
            }
            break;
            case PMS_FRAME_PAYLOAD:
            {
                idx++;
                frame_idx++;

                if (frame_idx >= frame_len)
                {
                    state = PMS_FRAME_END;
                    idx = 0;
                    if (RT_EOK == pms_check_frame(dev, buf, frame_len + 4))
                    {
                        rt_sem_release(dev->ack);
                    }
                }
            }
            break;
            default:
            {
                idx = 0;
                state = PMS_FRAME_END;
            }
            break;
            }
        }
    }
}
#else
static void pms_recv_thread_entry(void *parameter)
{
    pms_device_t dev = (pms_device_t)parameter;

    char ch;
    pms_frame_t state = PMS_FRAME_END;
    rt_uint16_t idx = 0;
    rt_uint16_t frame_len = 0;
    rt_uint16_t frame_idx = 0;
    rt_uint8_t  buf[FRAME_LEN_MAX] = {0};

    while (1)
    {
        while (rt_device_read(dev->serial, -1, &ch, 1) != 1)
        {
            rt_sem_take(dev->rx_sem, RT_WAITING_FOREVER);
        }

        switch (state)
        {
        case PMS_FRAME_END:
            if (ch == FRAME_START1)
            {
                idx = 0;
                buf[idx++] = ch;
                state = PMS_FRAME_HEAD;
            }
            break;
        case PMS_FRAME_HEAD:
            if (ch == FRAME_START2)
            {
                buf[idx++] = ch;
                state = PMS_FRAME_HEAD_ACK;
            }
            break;
        case PMS_FRAME_HEAD_ACK:
        {
            buf[idx++] = ch;
            state = PMS_FRAME_LENGTH;
        }
        break;
        case PMS_FRAME_LENGTH:
        {
            buf[idx++] = ch;
            frame_len = buf[idx - 2] << 8 | buf[idx - 1];
            frame_idx = 0;

            if (frame_len <= FRAME_LEN_MAX - 4)
                state = PMS_FRAME_PAYLOAD;
            else
                state = PMS_FRAME_END;
        }
        break;
        case PMS_FRAME_PAYLOAD:
        {
            buf[idx++] = ch;
            frame_idx++;

            if (frame_idx >= frame_len)
            {
                state = PMS_FRAME_END;
                idx = 0;
                if (RT_EOK == pms_check_frame(dev, buf, frame_len + 4))
                {
                    rt_sem_release(dev->ack);
                }
            }
        }
        break;
        default:
            idx = 0;
            break;
        }
    }
}
#endif

rt_err_t pms_set_mode(pms_device_t dev, pms_mode_t mode)
{
    RT_ASSERT(dev);

    rt_device_write(dev->serial, 0, &preset_commands[mode], sizeof(struct pms_cmd));
#ifdef PKG_USING_PMSXX_DEBUG_SHOW_CMD
    pms_show_command(&preset_commands[mode]);
#endif
    rt_thread_mdelay(PKG_USING_PMSXX_SEND_WAIT_TIME);

    return RT_EOK;
}

rt_uint16_t pms_read(pms_device_t dev, void *buf, rt_uint16_t size, rt_int32_t time)
{
    RT_ASSERT(dev);

    rt_sem_control(dev->ack, RT_IPC_CMD_RESET, RT_NULL);

    rt_device_write(dev->serial, 0, &preset_commands[PMS_MODE_READ], sizeof(struct pms_cmd));

    if (RT_EOK != rt_sem_take(dev->ack, time))
        return 0;

    rt_mutex_take(dev->lock, RT_WAITING_FOREVER);
    rt_memcpy(buf, &dev->resp, size);
    rt_mutex_release(dev->lock);

    return size;
}

rt_uint16_t pms_wait(pms_device_t dev, void *buf, rt_uint16_t size)
{
    RT_ASSERT(dev);

    rt_sem_take(dev->ack, RT_WAITING_FOREVER);

    rt_mutex_take(dev->lock, RT_WAITING_FOREVER);
    rt_memcpy(buf, &dev->resp, size);
    rt_mutex_release(dev->lock);

    return size;
}

rt_bool_t pms_is_ready(pms_device_t dev)
{
    return dev->version == 0x00 ? RT_FALSE : RT_TRUE;
}

static void sensor_init_entry(void *parameter)
{
    pms_device_t dev = (pms_device_t)parameter;

    rt_uint16_t ret;
    struct pms_response resp;

    pms_set_mode(dev, PMS_MODE_NORMAL);
    pms_set_mode(dev, PMS_MODE_PASSIVE);

    ret = pms_read(dev, &resp, sizeof(resp), rt_tick_from_millisecond(PMS_READ_WAIT_TIME));
    if (ret != sizeof(resp))
    {
        LOG_E("Can't receive response from pmsxx device");
        pms_set_mode(dev, PMS_MODE_STANDBY);
        dev->version = 0x00;
    }
    else
    {
        dev->version = resp.version;
    }
}

/**
 * This function initializes pmsxx registered device driver
 *
 * @param dev the name of pmsxx device
 *
 * @return the pmsxx device.
 */
pms_device_t pms_create(const char *uart_name)
{
    RT_ASSERT(uart_name);
    rt_err_t ret;

    pms_device_t dev = rt_calloc(1, sizeof(struct pms_device));
    if (dev == RT_NULL)
    {
        LOG_E("Can't allocate memory for pmsxx device on '%s'", uart_name);
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

#ifdef PKG_USING_PMSXX_UART_DMA
    /* init messagequeue */
    dev->rx_mb = rt_mb_create("pms_rx", 16, RT_IPC_FLAG_FIFO);
    if (dev->rx_mb == RT_NULL)
    {
        LOG_E("Can't create mailbox for pmsxx device");
        goto __exit;
    }
#else
    /* init semaphore */
    dev->rx_sem = rt_sem_create("pms_rx", 0, RT_IPC_FLAG_FIFO);
    if (dev->rx_sem == RT_NULL)
    {
        LOG_E("Can't create semaphore for pmsxx device");
        goto __exit;
    }
#endif

    dev->ack  = rt_sem_create("pms_ack", 0, RT_IPC_FLAG_FIFO);
    if (dev->ack == RT_NULL)
    {
        LOG_E("Can't create semaphore for pmsxx device");
        goto __exit;
    }

    /* init mutex */
    dev->lock = rt_mutex_create("pms_lock", RT_IPC_FLAG_FIFO);
    if (dev->lock == RT_NULL)
    {
        LOG_E("Can't create mutex for pmsxx device");
        goto __exit;
    }

    /* create thread */
    dev->rx_tid = rt_thread_create("pms_rx", pms_recv_thread_entry, (void *)dev, 
                                   PMS_THREAD_STACK_SIZE, PMS_THREAD_PRIORITY, 10);
    if (dev->rx_tid == RT_NULL)
    {
        LOG_E("Can't create thread for pmsxx device");
        goto __exit;
    }

    rt_thread_startup(dev->rx_tid);

    /* open UART device and enable UART RX */
#ifdef PKG_USING_PMSXX_UART_DMA
    ret = rt_device_open(dev->serial, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_DMA_RX);
#else
    ret = rt_device_open(dev->serial, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX);
#endif
    if (ret != RT_EOK)
    {
        LOG_E("Can't open '%s' serial device", uart_name);
        goto __exit;
    }

    rt_device_set_rx_indicate(dev->serial, pms_uart_input);

    /* run init thread or call init function */
#ifdef PKG_USING_PMSXX_INIT_ASYN
    rt_thread_t tid;

    tid = rt_thread_create("pms_init", sensor_init_entry, (void *)dev,
                            PMS_THREAD_STACK_SIZE, PMS_THREAD_PRIORITY, 10);
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

    if (!pms_is_ready(dev))
    {
        rt_device_close(dev->serial);
        goto __exit;
    }
#endif /* PKG_USING_PMSXX_INIT_ASYN */

    return dev;

__exit:
    if (dev->rx_tid)   rt_thread_delete(dev->rx_tid);
    if (dev->lock)     rt_mutex_delete(dev->lock);
#ifdef PKG_USING_PMSXX_UART_DMA
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
void pms_delete(pms_device_t dev)
{
    if (dev)
    {
        pms_set_mode(dev, PMS_MODE_STANDBY);
        dev->serial->user_data = RT_NULL;
        
        rt_sem_delete(dev->ack);
#ifdef PKG_USING_PMSXX_UART_DMA
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
