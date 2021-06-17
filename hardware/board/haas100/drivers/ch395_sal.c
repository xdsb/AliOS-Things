/*
 * Copyright (C) 2021-2023 Alibaba Group Holding Limited
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include "aos/kernel.h"
#include "sys/socket.h"
#include "hal_sal.h"
#include <uservice/uservice.h>
#include <uservice/eventid.h>
#include "ulog/ulog.h"

#include "ch395_spi.h"
#include "ch395_cmd.h"

#define TAG "ch395_sal_module"

typedef enum {
    SOCK_STATE_OPEN = 0,
    SOCK_STATE_CONNECT,
    SOCK_STATE_DISCONN,
    SOCK_STATE_CLOSE,
} en_sock_state;

typedef struct {
    int32_t fd;
    aos_sem_t st_send_sem;
    aos_sem_t st_conn_sem;
    aos_sem_t st_disconn_sem;
    uint32_t tcp_conn_result;
    uint32_t send_result;
    uint8_t  destip[4];
    uint8_t  proto_type;
    uint8_t  sock_state;
    uint8_t  send_blk_num;
    uint8_t  recv_blk_num;
    uint16_t dest_port;
    uint16_t local_port;
} st_ch395_sock_t;

#define CH395_TASK_PRIO       32
#define CH395_TASK_SIZE       (6 * 1024)
#define CH395_BLK_SIZE        512
#define SOCK_DEFAULT_TIMEOUT  3000

static int8_t g_i8_dev_add_flag = 0;
static int8_t g_i8_dev_init_flag = 0;
static st_ch395_info_t  g_st_ch395info = {0};
static st_ch395_sock_t g_st_ch395_sock[MAX_SURPPORT_SOCK_NUM] = {0};
static aos_mutex_t g_st_sock_mutex = {0};
static aos_task_t g_st_ch395_int_task = {0};
static netconn_data_input_cb_t g_netconn_data_input_cb;


static int32_t fd_to_linkid(int fd)
{
    int link_id;

    if (aos_mutex_lock(&g_st_sock_mutex, AOS_WAIT_FOREVER) != 0) {
        LOGE(TAG, "Failed to lock mutex (%s).", __func__);
        return -1;
    }
    for (link_id = 0; link_id < MAX_SURPPORT_SOCK_NUM; link_id++) {
        if (g_st_ch395_sock[link_id].fd == fd) {
            break;
        }
    }

    aos_mutex_unlock(&g_st_sock_mutex);

    return link_id;
}


static void ch395_sock_interrupt_proc(uint8_t sockindex)
{
    uint8_t  sock_int_socket = 0;
    uint16_t recv_len = 0;
    uint8_t  *precv_data = NULL;
    uint8_t  remote_ip[16] = {0};
    int32_t  ret = 0;

    /* get sock interrupt status */
    ret = ch395_get_sock_int_status(sockindex, &sock_int_socket);
    // LOGI(TAG, "proc sock %d interrup 0x%x", sockindex, sock_int_socket);
    if (sock_int_socket & SINT_STAT_SENBUF_FREE) {
        // LOGI(TAG, "sock %d send data done ", sockindex);
        /*it means send ok */
        if (aos_sem_is_valid(&g_st_ch395_sock[sockindex].st_send_sem)) {
            g_st_ch395_sock[sockindex].send_result = 0;
            aos_sem_signal(&g_st_ch395_sock[sockindex].st_send_sem);
        }
    }

    if (sock_int_socket & SINT_STAT_SEND_OK) {
        /*only one buf is ok, so do nothing for now*/
    }

    if (sock_int_socket & SINT_STAT_CONNECT) {
        /*it means connect ok */
        // LOGI(TAG, "sock %d connect ok ", sockindex);
        if (g_st_ch395_sock[sockindex].proto_type == PROTO_TYPE_TCP
            && aos_sem_is_valid(&g_st_ch395_sock[sockindex].st_conn_sem)) {
            g_st_ch395_sock[sockindex].tcp_conn_result = 0;
            aos_sem_signal(&g_st_ch395_sock[sockindex].st_conn_sem);
        }
    }

    if (sock_int_socket & SINT_STAT_DISCONNECT) {
        // LOGI(TAG, "sock %d disconnect ", sockindex);
        /*it means connect ok */
        if (g_st_ch395_sock[sockindex].proto_type == PROTO_TYPE_TCP
            && aos_sem_is_valid(&g_st_ch395_sock[sockindex].st_disconn_sem)) {
            aos_sem_signal(&g_st_ch395_sock[sockindex].st_disconn_sem);
        }
    }

    if (sock_int_socket & SINT_STAT_TIM_OUT) {
        // LOGI(TAG, "sock %d connect timeout ", sockindex);
        /*it means connect ok */
        if (g_st_ch395_sock[sockindex].proto_type == PROTO_TYPE_TCP
            && aos_sem_is_valid(&g_st_ch395_sock[sockindex].st_conn_sem)) {
            g_st_ch395_sock[sockindex].tcp_conn_result = -1;
            aos_sem_signal(&g_st_ch395_sock[sockindex].st_conn_sem);
        }

        if (g_st_ch395_sock[sockindex].proto_type == PROTO_TYPE_UDP
            && aos_sem_is_valid(&g_st_ch395_sock[sockindex].st_send_sem)) {
            g_st_ch395_sock[sockindex].send_result = -1;
            aos_sem_signal(&g_st_ch395_sock[sockindex].st_send_sem);
        }
    }

    if (sock_int_socket & SINT_STAT_RECV) {
        // LOGI(TAG, "sock %d recv data ", sockindex);
        /*get recv data length*/
        ret = ch395_socket_recv_data_len(sockindex, &recv_len);
        if (ret) {
            LOGE(TAG, "Fail to get sock %d recv length", sockindex);
            return ;
        }
        if (recv_len == 0) {
            LOGE(TAG, "sock %d no data need to recv ", sockindex);
            return ;
        }
        /* then we need to recv the data */
        precv_data = aos_malloc(recv_len);
        if (NULL == precv_data) {
            LOGE(TAG, "Fail to malloc %d ", recv_len);
            return ;
        }
        memset(precv_data, 0, recv_len);

        ret = ch395_socket_recv_data(sockindex, recv_len, precv_data);
        if (ret) {
            LOGE(TAG, "sock %d recv data fail len %d", sockindex, recv_len);
            aos_free(precv_data);
            return ;
        }

        sprintf(remote_ip, "%d.%d.%d.%d", g_st_ch395_sock[sockindex].destip[0],
                g_st_ch395_sock[sockindex].destip[1], g_st_ch395_sock[sockindex].destip[2],
                g_st_ch395_sock[sockindex].destip[3]);
        if (g_netconn_data_input_cb && (g_st_ch395_sock[sockindex].fd >= 0)) {
            if (g_netconn_data_input_cb(g_st_ch395_sock[sockindex].fd, precv_data, recv_len, remote_ip, g_st_ch395_sock[sockindex].dest_port)) {
                LOGE(TAG, " %s socket %d get data len %d fail to post to sal, drop it\n",
                    __func__, g_st_ch395_sock[sockindex].fd, recv_len);
            }
        }
        aos_free(precv_data);
   }
}

static void ch395_inter_proc(void)
{
    uint16_t ch395_int_status;
    uint8_t  dhcp_status = 0;
    uint8_t  phy_status = 0;
    uint32_t retry = 0;
    int32_t  ret = 0;

    while (1) {
        /*every 100 ms proc the chip interrupt*/
        aos_msleep(100);

        /*if dev haven't init , then nothing to do continue*/
        if (g_i8_dev_add_flag == 0 || g_i8_dev_init_flag == 0) {
            continue;
        }

        ch395_int_status = 0;
        ret = ch395_get_global_all_int_status(&ch395_int_status);
        // LOGI(TAG, "proc global interrup 0x%x ", ch395_int_status);
        if (ch395_int_status & GINT_STAT_UNREACH) {
            /* nothing to do for now*/
        }

        if (ch395_int_status & GINT_STAT_IP_CONFLI) {
            /* nothing to do for now*/
        }

        if (ch395_int_status & GINT_STAT_PHY_CHANGE) {
            /*get phy status*/
            ret = ch395_get_phy_status(&phy_status);
            if (ret != 0) {
                LOGE(TAG, "Fail to get phy status");
                continue;
            }

            g_st_ch395info.phystate = phy_status;

            if (phy_status == PHY_DISCONN) {
                LOGI(TAG, "eth link down");
            } else {
                /*start up to dhcp*/
                LOGI(TAG, "eth link up");
                ret = ch395_dhcp_enable(1);
                if (ret) {
                    LOGE(TAG, "Fail to enable dhcp");
                }
            }
        }

        /* dhcp/pppoe interrup proc */
        if (ch395_int_status & GINT_STAT_DHCP) {
            ret = ch395_dhcp_get_status(&dhcp_status);
            if (ret) {
                LOGE(TAG, "Fail to dhcp result");
                continue;
            }

            if (dhcp_status == 0) {
                /*try to get ip interface */
                do {
                    memset(&g_st_ch395info.ip_info, 0, sizeof(g_st_ch395info.ip_info));
                    ret = ch395_get_ip_interface(&g_st_ch395info.ip_info);
                    if (ret) {
                        LOGE(TAG, "Fail to get eth interface ip info");
                        continue;
                    }
                    if (g_st_ch395info.ip_info.ipaddr[0] != 0 && g_st_ch395info.ip_info.ipaddr[1] != 0) {
                        /* Post got ip event */
                        LOGI(TAG, "get ip info %d.%d.%d.%d gateway %d.%d.%d.%d mask %d.%d.%d.%d dns1 %d.%d.%d.%d dns2 %d.%d.%d.%d\r\n",
                            g_st_ch395info.ip_info.ipaddr[0], g_st_ch395info.ip_info.ipaddr[1], g_st_ch395info.ip_info.ipaddr[2], g_st_ch395info.ip_info.ipaddr[3],
                            g_st_ch395info.ip_info.gateway[0], g_st_ch395info.ip_info.gateway[1], g_st_ch395info.ip_info.gateway[2], g_st_ch395info.ip_info.gateway[3],
                            g_st_ch395info.ip_info.ip_mask[0], g_st_ch395info.ip_info.ip_mask[1], g_st_ch395info.ip_info.ip_mask[2], g_st_ch395info.ip_info.ip_mask[3],
                            g_st_ch395info.ip_info.ip_dns1[0], g_st_ch395info.ip_info.ip_dns1[1], g_st_ch395info.ip_info.ip_dns1[2], g_st_ch395info.ip_info.ip_dns1[3],
                            g_st_ch395info.ip_info.ip_dns2[0], g_st_ch395info.ip_info.ip_dns2[1], g_st_ch395info.ip_info.ip_dns2[2], g_st_ch395info.ip_info.ip_dns2[3]);

                        event_publish(EVENT_NETMGR_DHCP_SUCCESS, NULL);
                        break;
                    }
                    aos_msleep(1000);
                    retry++;
                } while (retry < 10);

            } else {
                // LOGD(TAG, "dhcp time out, cannot get ip addr, it will go on dhcp after 16 second");
            }
        }
        if (ch395_int_status & GINT_STAT_SOCK0) {
            ch395_sock_interrupt_proc(0);                                     /* sock 0 interrupt proc */
        }
        if (ch395_int_status & GINT_STAT_SOCK1) {
            ch395_sock_interrupt_proc(1);                                     /* sock 1 interrupt proc*/
        }
        if (ch395_int_status & GINT_STAT_SOCK2) {
            ch395_sock_interrupt_proc(2);                                     /* sock 2 interrupt proc*/
        }
        if (ch395_int_status & GINT_STAT_SOCK3) {
            ch395_sock_interrupt_proc(3);                                     /* sock 3 interrupt proc*/
        }
        if (ch395_int_status & GINT_STAT_SOCK4) {
            // ch395_sock_interrupt_proc(4);                                     /* sock 4 interrupt proc*/
        }
        if (ch395_int_status & GINT_STAT_SOCK5) {
            // ch395_sock_interrupt_proc(5);                                     /* sock 5 interrupt proc*/
        }
        if (ch395_int_status & GINT_STAT_SOCK6) {
            // ch395_sock_interrupt_proc(6);                                     /* sock 6 interrupt proc*/
        }
        if (ch395_int_status & GINT_STAT_SOCK7) {
            // ch395_sock_interrupt_proc(7);                                     /* sock 7 interrupt proc*/
        }
    }
}

static void ch395_interface_info_init(void)
{
    memset(&g_st_ch395info, 0, sizeof(g_st_ch395info));
    g_st_ch395info.phystate = PHY_DISCONN;

    return ;
}

static void ch395_sock_info_init(void)
{
    int8_t i = 0;

    for (i = 0; i < MAX_SURPPORT_SOCK_NUM; i++) {
        memset(&g_st_ch395_sock[i], -1, sizeof(st_ch395_sock_t));

        if (i < 4) {
            g_st_ch395_sock[i].send_blk_num = 4;
            g_st_ch395_sock[i].recv_blk_num = 8;
        }
        aos_sem_new(&g_st_ch395_sock[i].st_send_sem, 0);
        aos_sem_new(&g_st_ch395_sock[i].st_conn_sem, 0);
        aos_sem_new(&g_st_ch395_sock[i].st_disconn_sem, 0);
        g_st_ch395_sock[i].sock_state = SINT_STAT_DISCONNECT;
    }
}

static void ch395_sock_info_deinit()
{
    uint8_t i = 0;
    for (i = 0; i < MAX_SURPPORT_SOCK_NUM; i++) {
        aos_sem_free(&g_st_ch395_sock[i].st_send_sem);
        aos_sem_free(&g_st_ch395_sock[i].st_conn_sem);
        aos_sem_free(&g_st_ch395_sock[i].st_disconn_sem);
        memset(&g_st_ch395_sock[i], -1, sizeof(st_ch395_sock_t));
    }
}

static int32_t HAL_SAL_Init(void)
{
    int32_t ret = 0;

    if (g_i8_dev_add_flag == 0) {
        LOGE(TAG, "%s device haven't add yet", TAG);
        return -1;
    }

    if (g_i8_dev_init_flag) {
        LOGE(TAG, "ch395 have already inited %d ", g_i8_dev_init_flag);
        return 0;
    }

    /*need to check wether need to set mac ip addr, gateway, mask etc*/
    ret = aos_mutex_new(&g_st_sock_mutex);
    if (ret) {
        LOGE(TAG, "Fail to new mutex 0x%x", ret);
        return -1;
    }

    ret = aos_task_new_ext(&g_st_ch395_int_task, TAG, ch395_inter_proc, NULL, CH395_TASK_SIZE, CH395_TASK_PRIO);
    if (ret) {
        LOGE(TAG, "Fail to start chip interrupt proc task 0x%x", ret);
        aos_mutex_free(&g_st_sock_mutex);
        return -1;
    }

    ch395_interface_info_init();

    ch395_sock_info_init();

    g_i8_dev_init_flag = 1;

    return 0;
}

static int32_t HAL_SAL_Deinit(void)
{
    if (g_i8_dev_init_flag == 0) {
        LOGE(TAG, "ch395 have not init yet ");
        return 0;
    }
    g_i8_dev_init_flag = 0;
    aos_mutex_free(&g_st_sock_mutex);
    aos_task_delete(TAG);
    ch395_sock_info_deinit();

    return 0;
}

static int32_t ch395_sock_client_open(uint8_t sock, uint8_t destip[4], uint16_t rport, uint16_t lport, uint8_t proto_type)
{
    int32_t ret = 0;

    ret = ch395_set_sock_proto_type(sock, proto_type);
    if (ret) {
        LOGE(TAG, "Fail to set sock %d proto type 0x%x", sock, proto_type);
        return -1;
    }

    aos_mutex_lock(&g_st_sock_mutex, AOS_WAIT_FOREVER);
    g_st_ch395_sock[sock].proto_type = proto_type;
    aos_mutex_unlock(&g_st_sock_mutex);

    ret = ch395_set_sock_dest_ip(sock, destip);
    if (ret) {
        LOGE(TAG, "Fail to set sock %d dest ip 0x%x", sock, destip);
        return -1;
    }

    aos_mutex_lock(&g_st_sock_mutex, AOS_WAIT_FOREVER);
    memcpy(g_st_ch395_sock[sock].destip, destip, sizoef(g_st_ch395_sock[sock].destip));
    aos_mutex_unlock(&g_st_sock_mutex);

    ret = ch395_set_sock_dest_port(sock, rport);
    if (ret) {
        LOGE(TAG, "Fail to set sock %d remote port %d", sock, rport);
        return -1;
    }

    aos_mutex_lock(&g_st_sock_mutex, AOS_WAIT_FOREVER);
    g_st_ch395_sock[sock].dest_port = rport;
    aos_mutex_unlock(&g_st_sock_mutex);

    if (-1 != lport) {
        ret = ch395_set_sock_src_port(sock, lport);
        if (ret) {
            LOGE(TAG, "Fail to set sock %d local port %d", sock, lport);
            return -1;
        }
        aos_mutex_lock(&g_st_sock_mutex, AOS_WAIT_FOREVER);
        g_st_ch395_sock[sock].local_port = lport;
        aos_mutex_unlock(&g_st_sock_mutex);
    }

    ret = ch395_socket_open(sock);
    if (ret) {
        LOGE(TAG, "Fail to open sock %d, proto 0x%x ip 0x%x dport %d lport %d",
            sock, proto_type, destip, rport, lport);
        return -1;
    }

    return 0;
}

static int32_t HAL_SAL_Start(sal_conn_t *conn)
{
    int32_t ret = 0;
    uint8_t linkid = 0;
    uint8_t remote_ip[4]  = {0};
    static int32_t test_count  = 0;

    if (!g_i8_dev_init_flag) {
        LOGE(TAG, "ch395 module haven't init yet ");
        return -1;
    }

    if (!conn || !conn->addr) {
        LOGE(TAG, "invalid input");
        return -1;
    }

    if (ch395_str_to_ip4addr(conn->addr, remote_ip) != 0) {
        /*input addr is domain , need to translated into binary */
        /*for now return -1*/
        LOGE(TAG, "%s %d Invalid ip str %s", __func__, __LINE__, conn->addr);
        return -1;
    }

    aos_mutex_lock(&g_st_sock_mutex, AOS_WAIT_FOREVER);
    for (linkid = 0; linkid < MAX_SURPPORT_SOCK_NUM; linkid++) {
        if (g_st_ch395_sock[linkid].fd >= 0) {
            continue;
        }
        g_st_ch395_sock[linkid].fd = conn->fd;
        break;
    }
    aos_mutex_unlock(&g_st_sock_mutex);

    if (linkid >= MAX_SURPPORT_SOCK_NUM) {
        LOGE(TAG, "No sock available for now sock %d.", linkid);
        return -1;
    }

    switch (conn->type) {
        case TCP_SERVER:
            LOGE(TAG, "tcp server not supported for now");
            goto err;
            break;
        case TCP_CLIENT:
            ret = ch395_sock_client_open(linkid, remote_ip, conn->r_port, conn->l_port, PROTO_TYPE_TCP);
            if (ret) {
                LOGE(TAG, "Fail to open sock %d", linkid);
                goto err;
            }

            ret = ch395_socket_tcp_connect(linkid);
            if (ret) {
                ret = ch395_socket_close(linkid);
                if (ret) {
                    LOGE(TAG, "Fatal error ! socket close fail !");
                }
                goto err;
            }

            /* start to wait for the connect sem4 */
            ret = aos_sem_wait(&g_st_ch395_sock[linkid].st_conn_sem, SOCK_DEFAULT_TIMEOUT);
            if (ret) {
                LOGE(TAG, "Wait sock %d tcp connect sem fail", linkid);
                return -1;
            }

            if (g_st_ch395_sock[linkid].tcp_conn_result != 0) {
                LOGE(TAG, "sock %d tcp connect fail ");
                return -1;
            }

            break;
        case UDP_UNICAST:
            ret = ch395_sock_client_open(linkid, remote_ip, conn->r_port, conn->l_port, PROTO_TYPE_UDP);
            if (ret) {
                LOGE(TAG, "Fail to open sock %d", linkid);
                goto err;
            }
            break;
        case SSL_CLIENT:
        case UDP_BROADCAST:
        default:
            LOGE(TAG, "ch395 module connect type %d not support ", conn->type);
            /*release the linkinfo*/
            goto err;
    }

    return 0;

err:
    aos_mutex_lock(&g_st_sock_mutex, AOS_WAIT_FOREVER);
    g_st_ch395_sock[linkid].fd = -1;
    g_st_ch395_sock[linkid].dest_port = 0;
    g_st_ch395_sock[linkid].local_port = 0;
    memset(g_st_ch395_sock[linkid].destip, 0, 4);
    g_st_ch395_sock[linkid].proto_type = 0;
    g_st_ch395_sock[linkid].sock_state = 0;
    aos_mutex_unlock(&g_st_sock_mutex);
    return -1;
}

static int32_t HAL_SAL_Close(int fd, int32_t remote_port)
{
    int32_t ret = 0;
    int32_t linkid = 0;

    if (!g_i8_dev_init_flag) {
        LOGE(TAG, "ch395 module haven't init yet ");
        return -1;
    }

    linkid = fd_to_linkid(fd);

    if (linkid >= MAX_SURPPORT_SOCK_NUM) {
        LOGE(TAG, "No connection found for fd (%d)", fd);
        return -1;
    }

    if (g_st_ch395_sock[linkid].proto_type == PROTO_TYPE_TCP) {
        ret = ch395_socket_tcp_disconnect(linkid);
        if (ret) {
            LOGE(TAG, "Fail to disconnect %d tcp link and nothing to do", linkid);
        } else {
            /* wait for the disconnect sem4 */
            ret = aos_sem_wait(&g_st_ch395_sock[linkid].st_disconn_sem, SOCK_DEFAULT_TIMEOUT);
            if (ret) {
                LOGE(TAG, "Wait sock %d tcp disconnect sem fail, nothing to do", linkid);
                return -1;
            }
        }
    }

    ret = ch395_clear_sock_recv_buff(linkid);
    if (ret) {
        LOGE(TAG, "sock %d fail to clear recv buff", linkid);
    }

    ret = ch395_socket_close(linkid);
    if (ret) {
        LOGE(TAG, "Fail to close sock %d ret %d", linkid, ret);
    }

    aos_mutex_lock(&g_st_sock_mutex, AOS_WAIT_FOREVER);
    g_st_ch395_sock[linkid].fd = -1;
    g_st_ch395_sock[linkid].dest_port = 0;
    g_st_ch395_sock[linkid].local_port = 0;
    memset(g_st_ch395_sock[linkid].destip, 0, 4);
    g_st_ch395_sock[linkid].proto_type = 0;
    g_st_ch395_sock[linkid].sock_state = 0;
    aos_mutex_unlock(&g_st_sock_mutex);

    return 0;

}

static int32_t HAL_SAL_Send(int32_t fd, uint8_t *data, uint32_t len,
                 char remote_ip[16], int32_t remote_port, int32_t timeout)
{
    int32_t ret = 0;
    int32_t linkid = 0;
    uint8_t destip[4] = {0};

    if (NULL == data || 0 == len) {
        LOGE(TAG, "Invalid input");
        return -1;
    }

    if (!g_i8_dev_init_flag) {
        LOGE(TAG, "ch395 module haven't init yet ");
        return -1;
    }

    linkid = fd_to_linkid(fd);

    if (linkid >= MAX_SURPPORT_SOCK_NUM) {
        LOGE(TAG, "No connection found for fd (%d)", fd);
        return -1;
    }

    if (len >= (g_st_ch395_sock[linkid].send_blk_num * CH395_BLK_SIZE)) {
        LOGE(TAG, "send data len %d over limite %d ", len, g_st_ch395_sock[linkid].send_blk_num * CH395_BLK_SIZE);
        return -1;
    }

    ret = ch395_socket_data_send(linkid, len, data);
    if (ret) {
        LOGE(TAG, "sock %d send data fail, len %d", linkid, len);
        return -1;
    }

    /* wait for the disconnect sem4 */
    ret = aos_sem_wait(&g_st_ch395_sock[linkid].st_send_sem, SOCK_DEFAULT_TIMEOUT);
    if (ret) {
        LOGE(TAG, "Wait sock %d data send sem fail", linkid);
        return -1;
    }

    if (g_st_ch395_sock[linkid].send_result != 0) {
        LOGE(TAG, "%s %d sock %d send data fail ", __FILE__, __LINE__, linkid);
        return -1;
    }

    return 0;
}

int32_t ch395_sal_add_dev(void *data)
{
    int32_t ret = 0;
    uint8_t chip_ver = 0;
    uint8_t soft_ver = 0;

    if (NULL == data) {
        LOGE(TAG, "Invalid input");
        return -1;
    }

    ret = ch395_module_init();
    if (ret) {
        LOGE(TAG, "ch395 module init fail %d", ret);
        return -1;
    }

    ret = ch395_get_version(&chip_ver, &soft_ver);
    if (ret || chip_ver != 0x4) {
        LOGE(TAG, "Fail to get chip ver: 0x%x soft ver 0x%x ret : 0x%x", chip_ver, soft_ver, ret);
        return -1;
    }

    ret = ch395_set_func_param(SOCK_CTRL_FLAG_SOCKET_CLOSE);
    if (ret) {
        LOGE(TAG, "sal add dev : ch395 set func param fail %d", ret);
        return -1;
    }

    ret = ch395_dev_init();
    if (ret) {
        LOGE(TAG, "ch395 dev init fail %d", ret);
        return -1;
    }

    ret = ch395_ping_enable(1);
    if (ret) {
        LOGE(TAG, "ch395 ping enable fail %d", ret);
    }

    g_i8_dev_add_flag = 1;
    return 0;

}

#define ALIYUN_IOT_ADDR    "139.196.135.135"
static int32_t HAL_SAL_DomainToIp(char *domain, char ip[16])
{
    int i;
    if (NULL == domain) {
        LOGE(TAG, "Invalid input %s %d", __FILE__, __LINE__);
    }

    LOGD(TAG, "For now ch395 driver only support iot platform");
    // use fixed ip:
    if (strstr(domain, "iot-as-mqtt.cn-shanghai.aliyuncs.com") != NULL) {
        LOGD(TAG, "using fixed ip\n");
        memset(ip, 0, 16);
        strncpy(ip, ALIYUN_IOT_ADDR, strlen(ALIYUN_IOT_ADDR));
        return 0;
    }

    return -1;
}

static int32_t HAL_SAL_RegisterNetconnDataInputCb(netconn_data_input_cb_t cb)
{
    if (cb) {
        g_netconn_data_input_cb = cb;
    }
    return 0;
}

static sal_op_t sal_op = {
    .next = NULL,
    .version = "1.0.0",
    .name = "ch395",
    .add_dev = ch395_sal_add_dev,
    .init = HAL_SAL_Init,
    .start = HAL_SAL_Start,
    .send_data = HAL_SAL_Send,
    .domain_to_ip = HAL_SAL_DomainToIp,
    .finish = HAL_SAL_Close,
    .deinit = HAL_SAL_Deinit,
    .register_netconn_data_input_cb = HAL_SAL_RegisterNetconnDataInputCb,
};

int ch395_sal_device_init(void)
{
    return sal_module_register(&sal_op);
}
