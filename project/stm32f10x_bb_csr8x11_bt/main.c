#include <stdio.h>
#include "stm32f10x_conf.h"
#include "board_wrapper.h"
#include "bt_wrapper.h"
#include "cJSON.h"

uint32_t sys_time = 0;
uint32_t last_sys_time = 0;

#define CONF_BSP_TICKS_PER_SEC 100

struct bd_addr_t connect_addr;


#define OLED_SHOW_SIZE 32
uint8_t func_show[OLED_SHOW_SIZE];
uint8_t operate_show[OLED_SHOW_SIZE];
uint8_t status_show[OLED_SHOW_SIZE];
uint8_t key1_show[OLED_SHOW_SIZE];
uint8_t key2_show[OLED_SHOW_SIZE];
uint8_t key3_show[OLED_SHOW_SIZE];

void operate_stauts_oled_show(uint8_t *func,uint8_t *operate,uint8_t *status,uint8_t *key1,uint8_t *value1,uint8_t *key2,uint8_t *value2,uint8_t * key3,uint8_t *value3)
{
    hw_memset(func_show,0,OLED_SHOW_SIZE);
    hw_memset(operate_show,0,OLED_SHOW_SIZE);
    hw_memset(status_show,0,OLED_SHOW_SIZE);
    hw_memset(key1_show,0,OLED_SHOW_SIZE);
    hw_memset(key2_show,0,OLED_SHOW_SIZE);
    hw_memset(key3_show,0,OLED_SHOW_SIZE);

    hw_sprintf((char*)func_show,"FUC:%s",func);
    hw_sprintf((char*)operate_show,"OPERATE:%s",operate);
    hw_sprintf((char*)status_show,"STATUS:%s",status);

    hw_oled_clear();
    hw_oled_show_string(0,0,func_show,8);
    hw_oled_show_string(0,1,operate_show,8);
    hw_oled_show_string(0,2,status_show,8);

    if(key1 && value1)
    {
        hw_sprintf((char*)key1_show,"%s:%s",key1,value1);
        hw_oled_show_string(0,3,key1_show,8);
    }

    if(key2 && value2)
    {
        hw_sprintf((char*)key2_show,"%s:%s",key2,value2);
        hw_oled_show_string(0,4,key2_show,8);
    }

    if(key3 && value3)
    {
        hw_sprintf((char*)key3_show,"%s:%s",key3,value3);
        hw_oled_show_string(0,4,key3_show,8);
    }
}


uint8_t uart_send_json(uint8_t *func,uint8_t *operate,uint8_t *status,uint8_t *para1,uint8_t *para2,uint8_t *para3,uint8_t *para4,uint8_t *para5)
{
    uint8_t *bt_status_string;
    cJSON *bt_json_status = cJSON_CreateObject();

    cJSON_AddStringToObject(bt_json_status, "FUNC", (const char*)func);
    cJSON_AddStringToObject(bt_json_status, "OPERATE", (const char*)operate);
    cJSON_AddStringToObject(bt_json_status, "STATUS", (const char*)status);

    if(para1)
        cJSON_AddStringToObject(bt_json_status, "PARAM1", (const char*)para1);
    if(para2)
        cJSON_AddStringToObject(bt_json_status, "PARAM2", (const char*)para2);
    if(para3)
        cJSON_AddStringToObject(bt_json_status, "PARAM3", (const char*)para3);
    if(para4)
        cJSON_AddStringToObject(bt_json_status, "PARAM4", (const char*)para4);
    if(para5)
        cJSON_AddStringToObject(bt_json_status, "PARAM5", (const char*)para5);
    bt_status_string = (uint8_t *)cJSON_Print(bt_json_status);

    printf("%s\n",bt_status_string);
    cJSON_Delete(bt_json_status);
    free(bt_status_string);

    return 0;
}


void bt_app_init_result(uint8_t status,uint16_t profile_mask)
{
    uint8_t profile_mask_buf[8] = {0};
    printf("bt_app_init_result(%d) profile_mask(0x%x)\n",status,profile_mask);
    sprintf((char *)profile_mask_buf,"%x",profile_mask);
    uart_send_json("BT","BT_START",status==0?(uint8_t*)"SUCCESS":(uint8_t*)"FAIL",profile_mask_buf,0,0,0,0);
    operate_stauts_oled_show("BT","BT_START","INIT_SUCCESS",0,0,0,0,0,0);
}

void bt_app_inquiry_status(uint8_t status)
{
    printf("bt_inquiry_status %d\n",status);
    uart_send_json("BT","BT_INQUIRY_STATUS",(uint8_t*)"SUCCESS",status==0?(uint8_t*)"START":(uint8_t*)"STOP",0,0,0,0);
}

void bt_app_inquiry_result(struct bd_addr_t *address,uint8_t dev_type,uint8_t *name)
{
    uint8_t address_buf[16] = {0};
    uint8_t device_type_buf[16] = {0};

    sprintf((char *)address_buf,"%02x:%02x:%02x:%02x:%02x:%02x",address->addr[0],address->addr[1],address->addr[2],\
            address->addr[3],address->addr[4],address->addr[5]);
    if(dev_type == BT_COD_TYPE_HEADSET)
        sprintf((char *)device_type_buf,"%s","HEADSET");
    else
        sprintf((char *)device_type_buf,"%s","UNKNOW");
    uart_send_json("BT","BT_INQUIRY_RESULT",(uint8_t*)"SUCCESS",address_buf,device_type_buf,name,0,0);
}

static bt_app_common_cb_t bt_app_common_cb =
{
    bt_app_init_result,
    bt_app_inquiry_status,
    bt_app_inquiry_result,
};

void bt_app_hfp_connect(struct bd_addr_t *remote_addr,uint8_t status)
{
    printf("bt_app_hfp_connect status %d address:\n",status);
    bt_hex_dump(remote_addr->addr,6);
    connect_addr.addr[5] = remote_addr->addr[5];
    connect_addr.addr[4] = remote_addr->addr[4];
    connect_addr.addr[3] = remote_addr->addr[3];
    connect_addr.addr[2] = remote_addr->addr[2];
    connect_addr.addr[1] = remote_addr->addr[1];
    connect_addr.addr[0] = remote_addr->addr[0];
}


static bt_app_hfp_cb_t bt_app_hfp_cb =
{
    bt_app_hfp_connect,
};


void bt_app_spp_connect(struct bd_addr_t *remote_addr,uint8_t status)
{
	uint8_t addr_buf[32] = {0};
    printf("bt_app_spp_connect status %d address:\n",status);
    bt_hex_dump(remote_addr->addr,6);
    connect_addr.addr[5] = remote_addr->addr[5];
    connect_addr.addr[4] = remote_addr->addr[4];
    connect_addr.addr[3] = remote_addr->addr[3];
    connect_addr.addr[2] = remote_addr->addr[2];
    connect_addr.addr[1] = remote_addr->addr[1];
    connect_addr.addr[0] = remote_addr->addr[0];
	hw_sprintf((char*)addr_buf,"%02x:%02x:%02x:%02x:%02x:%02x",remote_addr->addr[5],remote_addr->addr[4],remote_addr->addr[3],\
		remote_addr->addr[2],remote_addr->addr[1],remote_addr->addr[0]);
	uart_send_json("BT","BT_CON_RESULT",(uint8_t*)"SUCCESS","SPP",addr_buf,0,0,0);
}

void bt_app_spp_disconnect(struct bd_addr_t *remote_addr,uint8_t status)
{
    printf("bt_app_spp_disconnect status %d address:\n",status);
    bt_hex_dump(remote_addr->addr,6);
    memset(&connect_addr,0,sizeof(connect_addr));

	uart_send_json("BT","BT_DISCON_RESULT",(uint8_t*)"SUCCESS","SPP",0,0,0,0);
}

uint8_t spp_recv_data_json[1024] = {0};
void bt_app_spp_recv_data(struct bd_addr_t *remote_addr,uint8_t *data,uint16_t data_len)
{
	uint8_t data_len_buf[8] = {0};
    printf("bt_app_spp_recv_data len %d address:\n",data_len);
    bt_hex_dump(remote_addr->addr,6);
    printf("data is :");
    bt_hex_dump(data,data_len);

	hw_sprintf((char*)data_len_buf,"%d",data_len);
	hw_memset(spp_recv_data_json,0,sizeof(spp_recv_data_json));
	hw_memcpy(spp_recv_data_json,data,data_len);
	uart_send_json("BT","BT_SPP_RECV",(uint8_t*)"SUCCESS",spp_recv_data_json,data_len_buf,0,0,0);
}

static bt_app_spp_cb_t bt_app_spp_cb =
{
    bt_app_spp_connect,
    bt_app_spp_disconnect,
    bt_app_spp_recv_data,
};


static bt_app_cb_t bt_app_cb =
{
    &bt_app_common_cb,
    &bt_app_spp_cb,
    &bt_app_hfp_cb,
};


#define BT_START_CMD "BT_START"
#define BT_START_DES "Start bluetooth stack"
#define BT_STOP_CMD "BT_STOP"
#define BT_STOP_DES "Stop blueooth stack"
#define BT_INQUIRY_CMD "BT_INQUIRY"
#define BT_INQUIRY_DES "Inquiry device"
#define BT_CANCEL_INQUIRY_CMD "BT_CANCEL_INQUIRY"
#define BT_CANCEL_INQUIRY_DES "Cancel inquiry device"
#define BT_PERIOID_INQUIRY_CMD "BT_PERIOID_INQUIRY"
#define BT_PERIOID_INQUIRY_DES "Perioid inquiry device"
#define BT_CANCEL_PERIOID_INQUIRY_CMD "BT_CANCEL_PERIOID_INQUIRY"
#define BT_CANCEL_PERIOID_INQUIRY_DES "Cancel perioid inquiry device"
#define BT_LE_INQUIRY_CMD "BT_LE_INQUIRY"
#define BT_LE_INQUIRY_DES "BLE Inquiry device"
#define BT_LE_INQUIRY_CANCEL_CMD "BT_LE_INQUIRY_STOP"
#define BT_LE_INQUIRY_CANCEL_DES "BLE cancel Inquiry device"
#define BT_SPP_CON_CMD "SPP_CON"
#define BT_SPP_CON_DES "Connect spp profile"
#define BT_SPP_SEND_CMD "SPP_SEND"
#define BT_SPP_SEND_DES "Spp sned data"
#define BT_SPP_DISCON_CMD "SPP_DISCON"
#define BT_SPP_DISCON_DES "Disconnect spp profile"
#define BT_HFP_CON_CMD "HFP_CON"
#define BT_HFP_CON_DES "Connect hfp profile"
#define BT_HFP_DISCON_CMD "HFP_DISCON"
#define BT_HFP_DISCON_DES "Disconnect hfp profile"
#define BT_HFP_SCO_CON_CMD "HFP_SCO_CON"
#define BT_HFP_SCO_CON_DES "Connect hfp sco"
#define BT_HFP_SCO_DISCON_CMD "HFP_SCO_DISCON"
#define BT_HFP_SCO_DISCON_DES "Disconnect hfp sco"
#define BT_HFP_ANSWER_CMD "HFP_ANSWER"
#define BT_HFP_ANSWER_DES "Answer the incoming call"
#define BT_HFP_CALLOUT_PN_CMD "HFP_CALLOUT_PN"
#define BT_HFP_CALLOUT_PN_DES "Call out phone number(10086)"
#define BT_HFP_CALLOUT_MEM_CMD "HFP_CALLOUT_MEM"
#define BT_HFP_CALLOUT_MEM_DES "Call out phone number with memory 1"
#define BT_HFP_CALLOUT_LN_CMD "HFP_CALLOUT_LC"
#define BT_HFP_CALLOUT_LN_DES "Call out last number"




typedef struct
{
    uint8_t *cmd;
    uint8_t *description;
} cmd_desctiption_t;

cmd_desctiption_t cmd_usage[] =
{
    {(uint8_t *)BT_START_CMD,(uint8_t *)BT_START_DES},
    {(uint8_t *)BT_STOP_CMD,(uint8_t *)BT_STOP_DES},
    {(uint8_t *)BT_INQUIRY_CMD,(uint8_t *)BT_INQUIRY_CMD},
    {(uint8_t *)BT_CANCEL_INQUIRY_CMD,(uint8_t *)BT_CANCEL_INQUIRY_DES},
    {(uint8_t *)BT_PERIOID_INQUIRY_CMD,(uint8_t *)BT_PERIOID_INQUIRY_DES},
    {(uint8_t *)BT_CANCEL_PERIOID_INQUIRY_CMD,(uint8_t *)BT_CANCEL_PERIOID_INQUIRY_DES},
    {(uint8_t *)BT_LE_INQUIRY_CMD,(uint8_t *)BT_LE_INQUIRY_DES},
    {(uint8_t *)BT_LE_INQUIRY_CANCEL_CMD,(uint8_t *)BT_LE_INQUIRY_CANCEL_DES},
    {(uint8_t *)BT_SPP_CON_CMD,(uint8_t *)BT_SPP_CON_DES},
    {(uint8_t *)BT_SPP_SEND_CMD,(uint8_t *)BT_SPP_SEND_DES},
    {(uint8_t *)BT_SPP_DISCON_CMD,(uint8_t *)BT_SPP_DISCON_DES},
    {(uint8_t *)BT_HFP_CON_CMD,(uint8_t *)BT_HFP_CON_DES},
    {(uint8_t *)BT_HFP_DISCON_CMD,(uint8_t *)BT_HFP_DISCON_DES},
    {(uint8_t *)BT_HFP_SCO_CON_CMD,(uint8_t *)BT_HFP_SCO_CON_DES},
    {(uint8_t *)BT_HFP_SCO_DISCON_CMD,(uint8_t *)BT_HFP_SCO_DISCON_DES},
    {(uint8_t *)BT_HFP_ANSWER_CMD,(uint8_t *)BT_HFP_ANSWER_DES},
    {(uint8_t *)BT_HFP_CALLOUT_PN_CMD,(uint8_t *)BT_HFP_CALLOUT_PN_DES},
    {(uint8_t *)BT_HFP_CALLOUT_MEM_CMD,(uint8_t *)BT_HFP_CALLOUT_MEM_DES},
    {(uint8_t *)BT_HFP_CALLOUT_LN_CMD,(uint8_t *)BT_HFP_CALLOUT_LN_DES},
};

void show_usage()
{
    uint32_t index = 0;
    for(index = 0; index < sizeof(cmd_usage)/sizeof(cmd_desctiption_t); index++)
    {
        printf("CMD(%s) -> DESCRIPTION(%s)\n",cmd_usage[index].cmd,cmd_usage[index].description);
    }
}

uint8_t shell_json_parse(uint8_t *operate_value,
                         uint8_t *para1,uint8_t *para2,uint8_t *para3,
                         uint8_t *para4,uint8_t *para5,uint8_t *para6)
{

    HW_DEBUG("OPERATE:%s\n",operate_value);
    if(hw_strcmp(BT_START_CMD,(const char*)operate_value) == 0)
    {
        HW_DEBUG("SHELL:operate BT_START\n");
        bt_start(&bt_app_cb);
        operate_stauts_oled_show("BT",operate_value,"SUCCESS",0,0,0,0,0,0);
        return HW_ERR_OK;
    }

    if(hw_strcmp(BT_STOP_CMD,(const char*)operate_value) == 0)
    {
        HW_DEBUG("SHELL:operate bt stop\n");
        bt_stop();
        return 0;
    }

    if(hw_strcmp(BT_INQUIRY_CMD,(const char*)operate_value) == 0)
    {
        HW_DEBUG("SHELL:operate BT_INQUIRY\n");
        bt_start_inquiry(0x30,HCI_INQUIRY_MAX_DEV);
        return HW_ERR_OK;
    }

    if(hw_strcmp(BT_CANCEL_INQUIRY_CMD,(const char*)operate_value) == 0)
    {
        HW_DEBUG("SHELL:operate bt cancel inquiry\n");
        bt_stop_inquiry();
        return 0;
    }

#if PROFILE_SPP_ENABLE > 0
	if(hw_strcmp(BT_SPP_SEND_CMD,(const char*)operate_value) == 0)
    {
    	uint16_t send_len = atoi((const char*)para2);
        HW_DEBUG("SHELL:operate bt spp send\n");
        spp_send_data(&connect_addr,para1,send_len);
        return 0;
    }
#endif

    HW_DEBUG("NO OPERATE:%s\n",operate_value);
    return HW_ERR_SHELL_NO_CMD;
}


uint8_t shell_at_cmd_parse(uint8_t *shell_string)
{

    if(hw_strcmp(BT_START_CMD,(const char*)shell_string) == 0)
    {
        HW_DEBUG("SHELL:operate bt start\n");
        bt_start(&bt_app_cb);
        return HW_ERR_OK;
    }

    if(hw_strcmp(BT_STOP_CMD,(const char*)shell_string) == 0)
    {
        HW_DEBUG("SHELL:operate bt stop\n");
        bt_stop();
        return HW_ERR_OK;
    }

    if(hw_strcmp(BT_INQUIRY_CMD,(const char*)shell_string) == 0)
    {
        HW_DEBUG("SHELL:operate bt inquiry\n");
        bt_start_inquiry(0x30,HCI_INQUIRY_MAX_DEV);
        return HW_ERR_OK;
    }

    if(hw_strcmp(BT_CANCEL_INQUIRY_CMD,(const char*)shell_string) == 0)
    {
        HW_DEBUG("SHELL:operate bt cancel inquiry\n");
        bt_stop_inquiry();
        return 0;
    }

#if PROFILE_SPP_ENABLE > 0
    if(hw_strcmp("SPP_SEND",(const char*)shell_string) == 0)
    {
        HW_DEBUG("SHELL:operate bt stop\n");
        spp_send_data(&connect_addr,"111111",hw_strlen("111111"));
        return HW_ERR_OK;
    }


    if(hw_strcmp("SPP_DISCON",(const char*)shell_string) == 0)
    {
        HW_DEBUG("SHELL:operate spp DISCON\n");

        spp_disconnect(&connect_addr);
        return HW_ERR_OK;
    }
#endif

#if PROFILE_PBAP_ENABLE > 0
    if(hw_strcmp("PBAP_CON",(const char*)shell_string) == 0)
    {
        HW_DEBUG("SHELL:operate PBAP CON\n");

        pbap_client_connect(&connect_addr);
        return HW_ERR_OK;
    }

    if(hw_strcmp("PBAP_DISCON",(const char*)shell_string) == 0)
    {
        HW_DEBUG("SHELL:operate PBAP DISCON\n");

        pbap_client_disconnect(&connect_addr);
        return HW_ERR_OK;
    }

    if(hw_strcmp("PBAP_LP",(const char*)shell_string) == 0)
    {
        HW_DEBUG("SHELL:operate PBAP PBAP_LPB\n");

        pbap_client_download_phonebook(&connect_addr,PB_LOCAL_REPOSITORY,PB_PHONEBOOK_TYPE);
        return HW_ERR_OK;
    }

    if(hw_strcmp("PBAP_LI",(const char*)shell_string) == 0)
    {
        HW_DEBUG("SHELL:operate PBAP PBAP_LI\n");

        pbap_client_download_phonebook(&connect_addr,PB_LOCAL_REPOSITORY,PB_INCOMING_BOOK_TYPE);
        return HW_ERR_OK;
    }

    if(hw_strcmp("PBAP_LO",(const char*)shell_string) == 0)
    {
        HW_DEBUG("SHELL:operate PBAP PBAP_LI\n");

        pbap_client_download_phonebook(&connect_addr,PB_LOCAL_REPOSITORY,PB_OUTGOING_BOOK_TYPE);
        return HW_ERR_OK;
    }

    if(hw_strcmp("PBAP_LM",(const char*)shell_string) == 0)
    {
        HW_DEBUG("SHELL:operate PBAP PBAP_LI\n");

        pbap_client_download_phonebook(&connect_addr,PB_LOCAL_REPOSITORY,PB_MISSING_BOOK_TYPE);
        return HW_ERR_OK;
    }

    if(hw_strcmp("PBAP_LC",(const char*)shell_string) == 0)
    {
        HW_DEBUG("SHELL:operate PBAP PBAP_LI\n");

        pbap_client_download_phonebook(&connect_addr,PB_LOCAL_REPOSITORY,PB_COMBINE_BOOK_TYPE);
        return HW_ERR_OK;
    }

    if(hw_strcmp("PBAP_LPC",(const char*)shell_string) == 0)
    {
        HW_DEBUG("SHELL:operate PBAP PBAP_LPB\n");

        pbap_client_query_phonebook_size(&connect_addr,PB_LOCAL_REPOSITORY,PB_PHONEBOOK_TYPE);
        return HW_ERR_OK;
    }

    if(hw_strcmp("PBAP_LIC",(const char*)shell_string) == 0)
    {
        HW_DEBUG("SHELL:operate PBAP PBAP_LI\n");

        pbap_client_query_phonebook_size(&connect_addr,PB_LOCAL_REPOSITORY,PB_INCOMING_BOOK_TYPE);
        return HW_ERR_OK;
    }

    if(hw_strcmp("PBAP_LOC",(const char*)shell_string) == 0)
    {
        HW_DEBUG("SHELL:operate PBAP PBAP_LI\n");

        pbap_client_query_phonebook_size(&connect_addr,PB_LOCAL_REPOSITORY,PB_OUTGOING_BOOK_TYPE);
        return HW_ERR_OK;
    }

    if(hw_strcmp("PBAP_LMC",(const char*)shell_string) == 0)
    {
        HW_DEBUG("SHELL:operate PBAP PBAP_LI\n");

        pbap_client_query_phonebook_size(&connect_addr,PB_LOCAL_REPOSITORY,PB_MISSING_BOOK_TYPE);
        return HW_ERR_OK;
    }

    if(hw_strcmp("PBAP_LCC",(const char*)shell_string) == 0)
    {
        HW_DEBUG("SHELL:operate PBAP PBAP_LI\n");

        pbap_client_query_phonebook_size(&connect_addr,PB_LOCAL_REPOSITORY,PB_COMBINE_BOOK_TYPE);
        return HW_ERR_OK;
    }

    if(hw_strcmp("PBAP_SP",(const char*)shell_string) == 0)
    {
        HW_DEBUG("SHELL:operate PBAP PBAP_LPB\n");

        pbap_client_set_path(&connect_addr,PB_LOCAL_REPOSITORY,PB_PHONEBOOK_TYPE);
        return HW_ERR_OK;
    }

    if(hw_strcmp("PBAP_SPL",(const char*)shell_string) == 0)
    {
        HW_DEBUG("SHELL:operate PBAP PBAP_LPB\n");

        pbap_client_download_vcard_list(&connect_addr,PB_LOCAL_REPOSITORY,PB_PHONEBOOK_TYPE);
        return HW_ERR_OK;
    }

    if(hw_strcmp("PBAP_DVE",(const char*)shell_string) == 0)
    {
        HW_DEBUG("SHELL:operate PBAP PBAP_LPB\n");

        pbap_client_download_vcard_entry(&connect_addr,PB_LOCAL_REPOSITORY,PB_PHONEBOOK_TYPE,1);
        return HW_ERR_OK;
    }
#endif


#if PROFILE_HFP_ENABLE > 0
    if(hw_strcmp("HFP_CON",(const char*)shell_string) == 0)
    {
        HW_DEBUG("SHELL:operate HFP CON\n");

        hfp_hf_connect(&connect_addr);
        return HW_ERR_OK;
    }

    if(hw_strcmp("HFP_DISCON",(const char*)shell_string) == 0)
    {
        HW_DEBUG("SHELL:operate HFP DISCON\n");

        hfp_hf_disconnect(&connect_addr);
        return HW_ERR_OK;
    }

    if(hw_strcmp("HFP_SCO_CON",(const char*)shell_string) == 0)
    {
        HW_DEBUG("SHELL:operate HFP SCO CON\n");

        hfp_hf_audio_connect(&connect_addr);
        return HW_ERR_OK;
    }

    if(hw_strcmp("HFP_SCO_DISCON",(const char*)shell_string) == 0)
    {
        HW_DEBUG("SHELL:operate HFP SCO DISCON\n");

        hfp_hf_audio_disconnect(&connect_addr);
        return HW_ERR_OK;
    }

    if(hw_strcmp("HFP_ANSWER",(const char*)shell_string) == 0)
    {
        HW_DEBUG("SHELL:operate HFP ANSWER INCOMING CALL\n");

        hfp_hf_answer_incoming_call(&connect_addr);
        return HW_ERR_OK;
    }

    if(hw_strcmp("HFP_CALLOUT_PN",(const char*)shell_string) == 0)
    {
        HW_DEBUG("SHELL:operate call out number\n");
        hfp_hf_callout_with_phone_number(&connect_addr,"10086");
        return HW_ERR_OK;
    }

    if(hw_strcmp("HFP_CALLOUT_MEM",(const char*)shell_string) == 0)
    {
        HW_DEBUG("SHELL:operate bt stop\n");
        hfp_hf_callout_with_memory(&connect_addr,1);
        return HW_ERR_OK;
    }

    if(hw_strcmp("HFP_CALLOUT_LC",(const char*)shell_string) == 0)
    {
        HW_DEBUG("SHELL:operate bt stop\n");
        hfp_hf_callout_with_last_number(&connect_addr);
        return HW_ERR_OK;
    }

    if(hw_strcmp("HFP_WN",(const char*)shell_string) == 0)
    {
        HW_DEBUG("SHELL:operate call wait enable\n");
        hfp_hf_set_call_waiting_notification(&connect_addr,1);
        return HW_ERR_OK;
    }

    if(hw_strcmp("HFP_WD",(const char*)shell_string) == 0)
    {
        HW_DEBUG("SHELL:operate call wait disenable\n");
        hfp_hf_set_call_waiting_notification(&connect_addr,0);
        return HW_ERR_OK;
    }

    if(hw_strcmp("HFP_CLIE",(const char*)shell_string) == 0)
    {
        HW_DEBUG("SHELL:operate CLI enable\n");
        hfp_hf_set_call_line_identification_notification(&connect_addr,1);
        return HW_ERR_OK;
    }

    if(hw_strcmp("HFP_CLID",(const char*)shell_string) == 0)
    {
        HW_DEBUG("SHELL:operate CLI disable\n");
        hfp_hf_set_call_line_identification_notification(&connect_addr,0);
        return HW_ERR_OK;
    }


    if(hw_strcmp("HFP_NRECD",(const char*)shell_string) == 0)
    {
        HW_DEBUG("SHELL:operate CLI disable\n");
        hfp_hf_disable_ag_nrec(&connect_addr);
        return HW_ERR_OK;
    }

    if(hw_strcmp("HFP_VGE",(const char*)shell_string) == 0)
    {
        HW_DEBUG("SHELL:operate CLI enable\n");
        hfp_hf_set_voice_recognition(&connect_addr,1);
        return HW_ERR_OK;
    }

    if(hw_strcmp("HFP_VGD",(const char*)shell_string) == 0)
    {
        HW_DEBUG("SHELL:operate CLI disable\n");
        hfp_hf_set_voice_recognition(&connect_addr,0);
        return HW_ERR_OK;
    }

    if(hw_strcmp("HFP_GPN",(const char*)shell_string) == 0)
    {
        HW_DEBUG("SHELL:operate get phone number via voice tag\n");
        hfp_hf_get_phone_number_via_voice_tag(&connect_addr);
        return HW_ERR_OK;
    }

    if(hw_strcmp("HFP_DTMF",(const char*)shell_string) == 0)
    {
        HW_DEBUG("SHELL:operate get phone number via voice tag\n");
        hfp_hf_transmit_dtmf(&connect_addr,1);
        return HW_ERR_OK;
    }

    if(hw_strcmp("HFP_VGM",(const char*)shell_string) == 0)
    {
        HW_DEBUG("SHELL:operate VGM\n");
        hfp_hf_set_mic_volume(&connect_addr,1);
        return HW_ERR_OK;
    }

    if(hw_strcmp("HFP_VGS",(const char*)shell_string) == 0)
    {
        HW_DEBUG("SHELL:operate VGM\n");
        hfp_hf_set_spk_volume(&connect_addr,1);
        return HW_ERR_OK;
    }

    if(hw_strcmp("HFP_LPN",(const char*)shell_string) == 0)
    {
        HW_DEBUG("SHELL:operate local number\n");
        hfp_hf_get_local_phone_number(&connect_addr);
        return HW_ERR_OK;
    }


    if(hw_strcmp("HFP_CLCC",(const char*)shell_string) == 0)
    {
        HW_DEBUG("SHELL:operate bt stop\n");
        hfp_hf_query_call_list(&connect_addr);
        return HW_ERR_OK;
    }

    if(hw_strcmp("HFP_CALLEND",(const char*)shell_string) == 0)
    {
        HW_DEBUG("SHELL:operate bt stop\n");
        hfp_hf_hangup(&connect_addr);
        return HW_ERR_OK;
    }

    if(hw_strcmp("HFP_NET_F",(const char*)shell_string) == 0)
    {
        HW_DEBUG("SHELL:operate bt stop\n");
        hfp_hf_set_format_network(&connect_addr);
        return HW_ERR_OK;
    }

    if(hw_strcmp("HFP_NET_N",(const char*)shell_string) == 0)
    {
        HW_DEBUG("SHELL:operate bt stop\n");
        hfp_hf_get_network(&connect_addr);
        return HW_ERR_OK;
    }

    if(hw_strcmp("HFP_I",(const char*)shell_string) == 0)
    {
        HW_DEBUG("SHELL:operate HFP_I\n");
        hfp_hf_set_indicator_enable_value(&connect_addr,"call",0);
        return HW_ERR_OK;
    }

    if(hw_strcmp("HFP_CGMI",(const char*)shell_string) == 0)
    {
        HW_DEBUG("SHELL:operate HFP_I\n");
        hfp_hf_get_manufacturer_id(&connect_addr);
        return HW_ERR_OK;
    }

    if(hw_strcmp("HFP_CGMM",(const char*)shell_string) == 0)
    {
        HW_DEBUG("SHELL:operate HFP_I\n");
        hfp_hf_get_model_id(&connect_addr);
        return HW_ERR_OK;
    }

    if(hw_strcmp("HFP_CGMR",(const char*)shell_string) == 0)
    {
        HW_DEBUG("SHELL:operate HFP_I\n");
        hfp_hf_get_revision_id(&connect_addr);
        return HW_ERR_OK;
    }

    if(hw_strcmp("HFP_PID",(const char*)shell_string) == 0)
    {
        HW_DEBUG("SHELL:operate HFP_I\n");
        hfp_hf_get_pid(&connect_addr);
        return HW_ERR_OK;
    }
#endif


#if PROFILE_AVRCP_ENABLE > 0
    if(hw_strcmp("AVRCP_GET_CAP1",(const char*)shell_string) == 0)
    {
        HW_DEBUG("SHELL:AVRCP_GET_CAP1\n");
        avrcp_controller_get_supported_company_ids();
        return HW_ERR_OK;
    }

    if(hw_strcmp("AVRCP_GET_CAP2",(const char*)shell_string) == 0)
    {
        HW_DEBUG("SHELL:AVRCP_GET_CAP2\n");
        avrcp_controller_get_supported_events();
        return HW_ERR_OK;
    }

    if(hw_strcmp("AVRCP_REG_TRACK_C",(const char*)shell_string) == 0)
    {
        HW_DEBUG("SHELL:AVRCP_REG_TRACK_C\n");
        avrcp_controller_enable_notification(AVRCP_NOTIFICATION_EVENT_TRACK_CHANGED);
        return HW_ERR_OK;
    }

    if(hw_strcmp("AVRCP_GET_INFO",(const char*)shell_string) == 0)
    {
        HW_DEBUG("SHELL:AVRCP_REG_TRACK_C\n");
        uint8_t media_info[7] = {1,2,3,4,5,6,7};
        avrcp_controller_get_element_attributes(media_info,sizeof(media_info));
        return HW_ERR_OK;
    }

    if(hw_strcmp("AVRCP_PLAY",(const char*)shell_string) == 0)
    {
        HW_DEBUG("SHELL:AVRCP PLAY\n");
        avrcp_controller_play();
        return HW_ERR_OK;
    }

    if(hw_strcmp("AVRCP_PAUSE",(const char*)shell_string) == 0)
    {
        HW_DEBUG("SHELL:AVRCP pause\n");
        avrcp_controller_pause();
        return HW_ERR_OK;
    }

    if(hw_strcmp("AVRCP_FORWARD",(const char*)shell_string) == 0)
    {
        HW_DEBUG("SHELL:AVRCP forward\n");
        avrcp_controller_forward();
        return HW_ERR_OK;
    }

    if(hw_strcmp("AVRCP_BACKWARD",(const char*)shell_string) == 0)
    {
        HW_DEBUG("SHELL:AVRCP backward\n");
        avrcp_controller_backward();
        return HW_ERR_OK;
    }
#endif


    show_usage();
    return HW_ERR_SHELL_NO_CMD;
}

uint8_t shell_parse(uint8_t *shell_string)
{
    uint8_t result = HW_ERR_OK;


    cJSON* parse_json = cJSON_Parse((const char *)shell_string);
    uint8_t* func_value = (uint8_t*)((cJSON *)cJSON_GetObjectItem(parse_json,"FUNC"))->valuestring;
    uint8_t* operate_value = (uint8_t*)((cJSON *)cJSON_GetObjectItem(parse_json,"OPERATE"))->valuestring;
    uint8_t* para1 = (uint8_t*)((cJSON *)cJSON_GetObjectItem(parse_json,"PARAM1"))->valuestring;
    uint8_t* para2 = (uint8_t*)((cJSON *)cJSON_GetObjectItem(parse_json,"PARAM2"))->valuestring;
    uint8_t* para3 = (uint8_t*)((cJSON *)cJSON_GetObjectItem(parse_json,"PARAM3"))->valuestring;
    uint8_t* para4 = (uint8_t*)((cJSON *)cJSON_GetObjectItem(parse_json,"PARAM4"))->valuestring;
    uint8_t* para5 = (uint8_t*)((cJSON *)cJSON_GetObjectItem(parse_json,"PARAM5"))->valuestring;
    uint8_t* para6 = (uint8_t*)((cJSON *)cJSON_GetObjectItem(parse_json,"PARAM6"))->valuestring;

    if(hw_strcmp((const char *)func_value,"BT") == 0)
    {
        result = shell_json_parse(operate_value,para1,para2,para3,para4,para5,para6);
    }
    else
    {
        result = shell_at_cmd_parse(shell_string);
    }

    cJSON_Delete(parse_json);
    return result;

}

void bt_reset_chip(void)
{
    GPIO_InitTypeDef  GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_WriteBit(GPIOA, GPIO_Pin_8, Bit_RESET);
    hw_delay_ms(200);
    GPIO_WriteBit(GPIOA, GPIO_Pin_8, Bit_SET);
}
void board_init()
{
    last_sys_time = sys_time;
    utimer_init();
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    hw_uart_debug_init(115200);
    hw_systick_init(SystemCoreClock/CONF_BSP_TICKS_PER_SEC);

    hw_button_init();
    hw_led_init();
    hw_oled_init();
    hw_sht2x_init();
    hw_spi_flash_init();
    hw_usb_init();
    file_system_init();
    bt_reset_chip();
}



void SysTick_Handler(void)
{
    sys_time += 1000/CONF_BSP_TICKS_PER_SEC;
    utimer_polling();
}



extern struct phybusif_cb uart_if;
int main()
{
    static uint8_t led_on = 0;
    board_init();

    while(1)
    {

        //NVIC_DisableIRQ(USART2_IRQn);
        phybusif_input(&uart_if);
        //NVIC_EnableIRQ(USART2_IRQn);

        if(sys_time - last_sys_time > 1000)
        {
            //printf("bt stack running\n");
            last_sys_time = sys_time;
            l2cap_tmr();
            rfcomm_tmr();

            if(!led_on)
            {
                LED1_ON;
                LED2_ON;
                led_on = 1;
            }
            else
            {
                LED1_OFF;
                LED2_OFF;
                led_on = 0;
            }
        }
    }
}
