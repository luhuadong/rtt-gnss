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

#define DBG_TAG "sensor.gps"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

#define PKG_USING_GPS_CLI
#ifdef PKG_USING_GPS_CLI

static struct gps_device gps;

static rt_err_t (*odev_rx_ind)(rt_device_t dev, rt_size_t size) = RT_NULL;


static void gps_cli_init(void)
{
    rt_base_t int_lvl;
    rt_device_t console;

    rt_sem_init(&console_rx_notice, "gps_cli", 0, RT_IPC_FLAG_FIFO);

    /* create RX FIFO */
    console_rx_fifo = rt_ringbuffer_create(AT_CLI_FIFO_SIZE);
    /* created must success */
    RT_ASSERT(console_rx_fifo);

    int_lvl = rt_hw_interrupt_disable();
    console = rt_console_get_device();
    if (console)
    {
        /* backup RX indicate */
        odev_rx_ind = console->rx_indicate;
        rt_device_set_rx_indicate(console, console_getchar_rx_ind);
    }

    rt_hw_interrupt_enable(int_lvl);
}

static void gps_cli_deinit(void)
{
    rt_base_t int_lvl;
    rt_device_t console;

    int_lvl = rt_hw_interrupt_disable();
    console = rt_console_get_device();
    if (console && odev_rx_ind)
    {
        /* restore RX indicate */
        rt_device_set_rx_indicate(console, odev_rx_ind);
    }
    rt_hw_interrupt_enable(int_lvl);

    rt_sem_detach(&console_rx_notice);
    rt_ringbuffer_destroy(console_rx_fifo);
}

static char client_getchar(void)
{
    char ch;

    rt_sem_take(&client_rx_notice, RT_WAITING_FOREVER);
    rt_ringbuffer_getchar(client_rx_fifo, (rt_uint8_t *)&ch);

    return ch;
}

static void at_client_entry(void *param)
{
    char ch;

    while(1)
    {
        ch = client_getchar();
        rt_kprintf("%c", ch);
    }
}

static rt_err_t client_getchar_rx_ind(rt_device_t dev, rt_size_t size)
{
    uint8_t ch;
    rt_size_t i;

    for (i = 0; i < size; i++)
    {
        /* read a char */
        if (rt_device_read(dev, 0, &ch, 1))
        {
            rt_ringbuffer_put_force(client_rx_fifo, &ch, 1);
            rt_sem_release(&client_rx_notice);
        }
    }

    return RT_EOK;
}
static void client_cli_parser(at_client_t  client)
{
#define ESC_KEY                 0x1B
#define BACKSPACE_KEY           0x08
#define DELECT_KEY              0x7F

    char ch;
    char cur_line[FINSH_CMD_SIZE] = { 0 };
    rt_size_t cur_line_len = 0;
    static rt_err_t (*client_odev_rx_ind)(rt_device_t dev, rt_size_t size) = RT_NULL;
    rt_base_t int_lvl;
    rt_thread_t at_client;
    at_status_t client_odev_status;

    if (client)
    {
        /* backup client status */
        {
            client_odev_status = client->status;
            client->status = AT_STATUS_CLI;
        }

        /* backup client device RX indicate */
        {
            int_lvl = rt_hw_interrupt_disable();
            client_odev_rx_ind = client->device->rx_indicate;
            rt_device_set_rx_indicate(client->device, client_getchar_rx_ind);
            rt_hw_interrupt_enable(int_lvl);
        }

        rt_sem_init(&client_rx_notice, "cli_r", 0, RT_IPC_FLAG_FIFO);
        client_rx_fifo = rt_ringbuffer_create(AT_CLI_FIFO_SIZE);

        at_client = rt_thread_create("at_cli", at_client_entry, RT_NULL, 512, 8, 8);
        if (client_rx_fifo && at_client)
        {
            rt_kprintf("======== Welcome to using RT-Thread AT command client cli ========\n");
            rt_kprintf("Cli will forward your command to server port(%s). Press 'ESC' to exit.\n", client->device->parent.name);
            rt_thread_startup(at_client);
            /* process user input */
            while (ESC_KEY != (ch = console_getchar()))
            {
                if (ch == BACKSPACE_KEY || ch == DELECT_KEY)
                {
                    if (cur_line_len)
                    {
                        cur_line[--cur_line_len] = 0;
                        rt_kprintf("\b \b");
                    }
                    continue;
                }
                else if (ch == '\r' || ch == '\n')
                {
                    /* execute a AT request */
                    if (cur_line_len)
                    {
                        rt_kprintf("\n");
                        at_obj_exec_cmd(client, RT_NULL, "%.*s", cur_line_len, cur_line);
                    }
                    cur_line_len = 0;
                }
                else
                {
                    rt_kprintf("%c", ch);
                    cur_line[cur_line_len++] = ch;
                }
            }

            /* restore client status */
            client->status = client_odev_status;

            /* restore client device RX indicate */
            {
                int_lvl = rt_hw_interrupt_disable();
                rt_device_set_rx_indicate(client->device, client_odev_rx_ind);
                rt_hw_interrupt_enable(int_lvl);
            }

            rt_thread_delete(at_client);
            rt_sem_detach(&client_rx_notice);
            rt_ringbuffer_destroy(client_rx_fifo);
        }
        else
        {
            rt_kprintf("No mem for AT cli client\n");
        }
    }
    else
    {
        rt_kprintf("AT client not initialized\n");
    }
}

static void gpsdump(int argc, char **argv)
{
    if (argc > 2)
    {
        rt_kprintf("Please input 'gpsdump [dev_name]' \n");
        return;
    }

    if (argc == 2)
    {
        gps.serial = rt_device_find(argv[1]);
    }
    else
    {
        gps.serial = rt_device_find(PKG_USING_GPS_SAMPLE_UART);
    }

    if (gps.serial == RT_NULL)
    {
        rt_kprintf("Can not find '%s' uart device.\n");
        return;
    }

    gps_cli_init();
    
    client_cli_parser();

    gps_cli_deinit();
}
MSH_CMD_EXPORT(gps, RT-Thread GPS package cli: gpsdump [dev_name]);

#endif /* PKG_USING_GPS_CLI */
