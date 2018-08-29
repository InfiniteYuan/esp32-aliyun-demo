/*
 * Copyright (c) 2014-2016 Alibaba Group. All rights reserved.
 * License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */


#include "iot_import.h"

#include "utils_list.h"
#include "utils_debug.h"
#include "lite-utils.h"
#include "shadow.h"
#include "shadow_common.h"
#include "shadow_update.h"
#include "shadow_delta.h"


/* check return code */
#define CHECK_RETURN_CODE(ret_code) \
    do{ \
        if (ret_code < 0) { \
            return ret_code; \
        } \
    }while(0);


/* check return code of snprintf */
#define CHECK_SNPRINTF_RET(ret_code, buf_len) \
    do{ \
        if ((ret_code) < 0) { \
            return FAIL_RETURN; \
        } else if ((ret_code) >= (buf_len)) { \
            return ERROR_NO_ENOUGH_MEM; \
        } \
    }while(0);


static void iotx_ds_handle_expire(iotx_shadow_pt pshadow)
{
    iotx_ds_update_wait_ack_list_handle_expire(pshadow);
}


/* This function will be called back when message published to topic(/shadow/get/) arrives. */
static void iotx_shadow_callback_get(iotx_shadow_pt pshadow, void *pclient, iotx_mqtt_event_msg_pt msg)
{
    const char *pname;

    iotx_mqtt_topic_info_pt topic_info = (iotx_mqtt_topic_info_pt)msg->msg;

    log_debug("topic=%.*s", topic_info->topic_len, topic_info->ptopic);
    log_debug("data of topic=%.*s", topic_info->payload_len, (char *)topic_info->payload);

    /* update time if there is 'timestamp' key in JSON string */
    pname = LITE_json_value_of((char *)"timestamp", (char *)topic_info->payload);
    if (NULL != pname) {
        iotx_ds_common_update_time(pshadow, atoi(pname));
    }
    LITE_free(pname);

    /* update 'version' if there is 'version' key in JSON string */
    pname = LITE_json_value_of((char *)"version", (char *)topic_info->payload);
    if (NULL != pname) {
        iotx_ds_common_update_version(pshadow, atoi(pname));
        LITE_free(pname);
    }

    /* get 'method' */
    pname = LITE_json_value_of((char *)"method", (char *)topic_info->payload);
    /* log_debug("pname(%d) = %s", (int)strlen(pname), pname); */
    if (NULL == pname) {
        log_err("Invalid JSON document: not 'method' key");
    } else if ((strlen("control") == strlen(pname)) && !strcmp(pname, "control")) {
        /* call delta handle function */
        log_debug("receive 'control' method");

        iotx_shadow_delta_entry(
                    pshadow,
                    topic_info->payload,
                    topic_info->payload_len);
        LITE_free(pname);
    } else if ((strlen("reply") == strlen(pname)) && !strcmp(pname, "reply")) {
        /* call update ACK handle function. */
        log_debug("receive 'reply' method");
        iotx_ds_update_wait_ack_list_handle_response(
                    pshadow,
                    topic_info->payload,
                    topic_info->payload_len);
        LITE_free(pname);
    } else {
        log_err("Invalid 'method' key");
        LITE_free(pname);
    }

    log_debug("End of method handle");
}

static int iotx_shadow_subcribe_get(iotx_shadow_pt pshadow)
{
    if (NULL == pshadow->inner_data.ptopic_get) {
        pshadow->inner_data.ptopic_get = iotx_ds_common_generate_topic_name(pshadow, "get");
        if (NULL == pshadow->inner_data.ptopic_get) {
            return FAIL_RETURN;
        }
    }

    return IOT_MQTT_Subscribe(pshadow->mqtt,
                              pshadow->inner_data.ptopic_get,
                              IOTX_MQTT_QOS1,
                              (iotx_mqtt_event_handle_func_fpt)iotx_shadow_callback_get,
                              pshadow);
}


iotx_err_t IOT_Shadow_PushFormat_Init(void *pshadow,
                                      format_data_pt pformat,
                                      char *buf,
                                      uint16_t size)
{
    return iotx_ds_common_format_init((iotx_shadow_pt)pshadow, pformat, buf, size, "update", "\"state\":{\"reported\":{");
}


iotx_err_t IOT_Shadow_PushFormat_Add(void *pshadow,
                                     format_data_pt pformat,
                                     iotx_shadow_attr_pt pattr)
{
    return iotx_ds_common_format_add((iotx_shadow_pt)pshadow, pformat, pattr->pattr_name, pattr->pattr_data,
                                     pattr->attr_type);
}


iotx_err_t IOT_Shadow_PushFormat_Finalize(void *pshadow, format_data_pt pformat)
{
    return iotx_ds_common_format_finalize((iotx_shadow_pt)pshadow, pformat, "}}");
}


int IOT_Shadow_Push_Async(
            void *handle,
            char *data,
            size_t data_len,
            uint16_t timeout_s,
            iotx_push_cb_fpt cb_fpt,
            void *pcontext)
{
    int rc = SUCCESS_RETURN;
    iotx_update_ack_wait_list_pt pelement;
    const char *ptoken;
    iotx_shadow_pt pshadow = (iotx_shadow_pt)handle;

    if ((NULL == handle) || (NULL == data)) {
        return NULL_VALUE_ERROR;
    }

    if (!IOT_MQTT_CheckStateNormal(pshadow->mqtt)) {
        log_err("The MQTT connection must be established before UPDATE data.");
        return ERROR_SHADOW_INVALID_STATE;
    }

    /*Add to callback list */

    log_debug("data(%d) = %s", (int)data_len, data);
    ptoken = LITE_json_value_of((char *)"clientToken", (char *)data);

    IOTX_ASSERT(NULL != ptoken, "Token should always exist.");

    pelement = iotx_shadow_update_wait_ack_list_add(pshadow, ptoken, strlen(ptoken), cb_fpt, pcontext, timeout_s);
    if (NULL == pelement) {
        LITE_free(ptoken);
        return ERROR_SHADOW_WAIT_LIST_OVERFLOW;
    }
    LITE_free(ptoken);

    if ((rc = iotx_ds_common_publish2update(pshadow, data, data_len)) < 0) {
        iotx_shadow_update_wait_ack_list_remove(pshadow, pelement);
        return rc;
    }

    return SUCCESS_RETURN;
}



static void iotx_update_ack_cb(
            void *pcontext,
            int ack_code,
            const char *ack_msg, /* NOTE: NOT a string. */
            uint32_t ack_msg_len)
{
    log_debug("ack_code=%d", ack_code);

    if (NULL != ack_msg) {
        log_debug("ack_msg=%.*s", ack_msg_len, ack_msg);
    } else {
        log_debug("ack_msg is NULL");
    }

    *((int *)pcontext) = ack_code;
}


iotx_err_t IOT_Shadow_Push(
            void *handle,
            char *data,
            uint32_t data_len,
            uint16_t timeout_s)
{
    iotx_shadow_ack_code_t ack_update = IOTX_SHADOW_ACK_NONE;
    iotx_shadow_pt pshadow = (iotx_shadow_pt)handle;

    if ((NULL == pshadow) || (NULL == data)) {
        return NULL_VALUE_ERROR;
    }

    if (!IOT_MQTT_CheckStateNormal(pshadow->mqtt)) {
        log_err("The MQTT connection must be established before UPDATE data.");
        return ERROR_SHADOW_INVALID_STATE;
    }

    /* update asynchronously */
    IOT_Shadow_Push_Async(pshadow, data, data_len, timeout_s, iotx_update_ack_cb, &ack_update);

    /* wait ACK */
    while (IOTX_SHADOW_ACK_NONE == ack_update) {
        IOT_Shadow_Yield(pshadow, 200);
    }

    if ((IOTX_SHADOW_ACK_SUCCESS == ack_update)
        || (IOTX_SHADOW_ACK_ERR_SHADOW_DOCUMENT_IS_NULL == ack_update)) {
        /* It is not the error that device shadow document is null */
        log_info("update success.");
        return SUCCESS_RETURN;
    } else if (IOTX_SHADOW_ACK_TIMEOUT == ack_update) {
        log_info("update timeout.");
        return ERROR_SHADOW_UPDATE_TIMEOUT;
    } else {
        log_info("update negative ack.");
        return ERROR_SHADOW_UPDATE_NACK;
    }
}


iotx_err_t IOT_Shadow_Pull(void *handle)
{
#define SHADOW_SYNC_MSG_SIZE      (256)

    iotx_err_t ret;
    void *buf;
    format_data_t format;
    iotx_shadow_pt pshadow = (iotx_shadow_pt)handle;

    log_info("Device Shadow sync start.");

    buf = LITE_malloc(SHADOW_SYNC_MSG_SIZE);
    if (NULL == buf) {
        log_err("Device Shadow sync failed");
        return ERROR_NO_MEM;
    }

    iotx_ds_common_format_init(pshadow, &format, buf, SHADOW_SYNC_MSG_SIZE, "get", NULL);
    iotx_ds_common_format_finalize(pshadow, &format, NULL);

    ret = IOT_Shadow_Push(pshadow, format.buf, format.offset, 10);
    if (SUCCESS_RETURN == ret) {
        log_info("Device Shadow sync success.");
    } else {
        log_info("Device Shadow sync failed.");
    }

    LITE_free(buf);
    HAL_SleepMs(1000);

    return ret;

#undef SHADOW_SYNC_MSG_SIZE
}


void iotx_ds_event_handle(void *pcontext, void *pclient, iotx_mqtt_event_msg_pt msg)
{
    iotx_shadow_pt pshadow = (iotx_shadow_pt)pcontext;
    uintptr_t packet_id = (uintptr_t)msg->msg;
    iotx_mqtt_topic_info_pt topic_info = (iotx_mqtt_topic_info_pt)msg->msg;

    switch (msg->event_type) {
        case IOTX_MQTT_EVENT_SUBCRIBE_SUCCESS:
            log_info("subscribe success, packet-id=%u", (unsigned int)packet_id);
            if (pshadow->inner_data.sync_status == packet_id) {
                pshadow->inner_data.sync_status = 0;
            }
            break;

        case IOTX_MQTT_EVENT_SUBCRIBE_TIMEOUT:
            log_info("subscribe wait ack timeout, packet-id=%u", (unsigned int)packet_id);
            if (pshadow->inner_data.sync_status == packet_id) {
                pshadow->inner_data.sync_status = -1;
            }
            break;

        case IOTX_MQTT_EVENT_SUBCRIBE_NACK:
            log_info("subscribe nack, packet-id=%u", (unsigned int)packet_id);
            if (pshadow->inner_data.sync_status == packet_id) {
                pshadow->inner_data.sync_status = -1;
            }
            break;

        case IOTX_MQTT_EVENT_PUBLISH_RECVEIVED:
            log_info("topic message arrived but without any related handle: topic=%.*s, topic_msg=%.*s",
                     topic_info->topic_len,
                     topic_info->ptopic,
                     topic_info->payload_len,
                     topic_info->payload);
            break;

        default:
            /* log_info("Should NOT arrive here."); */
            break;
    }
}

void *IOT_Shadow_Construct(iotx_shadow_para_pt pparams)
{
    int rc = 0;
    iotx_shadow_pt pshadow = NULL;

    /* initialize shadow */
    if (NULL == (pshadow = LITE_malloc(sizeof(iotx_shadow_t)))) {
        log_err("Not enough memory");
        return NULL;
    }
    memset(pshadow, 0x0, sizeof(iotx_shadow_t));

    if (NULL == (pshadow->mutex = HAL_MutexCreate())) {
        log_err("create mutex failed");
        goto do_exit;
    }

    pparams->mqtt.handle_event.h_fp = iotx_ds_event_handle;
    pparams->mqtt.handle_event.pcontext = pshadow;

    /* construct MQTT client */
    if (NULL == (pshadow->mqtt = IOT_MQTT_Construct(&pparams->mqtt))) {
        log_err("construct MQTT failed");
        goto do_exit;
    }

    rc = iotx_shadow_subcribe_get(pshadow);
    if (rc < 0) {
        log_err("subscribe 'get' topic fialed, rc=%d", rc);
        goto do_exit;
    }

    pshadow->inner_data.sync_status = rc;

    while (rc == pshadow->inner_data.sync_status) {
        IOT_Shadow_Yield(pshadow, 100);
    }

    if (0 == pshadow->inner_data.sync_status) {
        log_info("Sync device data successfully");
    } else {
        log_info("Sync device data failed");
    }


    pshadow->inner_data.attr_list = list_new();
    if (NULL == pshadow->inner_data.attr_list) {
        log_err("new list failed");
        goto do_exit;
    }

    return pshadow;

do_exit:
    IOT_Shadow_Destroy(pshadow);

    return NULL;
}


void IOT_Shadow_Yield(void *handle, uint32_t timeout)
{
    iotx_shadow_pt pshadow = (iotx_shadow_pt)handle;
    IOT_MQTT_Yield(pshadow->mqtt, timeout);
    iotx_ds_handle_expire(pshadow);
}


iotx_err_t IOT_Shadow_Destroy(void *handle)
{
    iotx_shadow_pt pshadow = (iotx_shadow_pt) handle;

    if (NULL != pshadow->mqtt) {
        if (NULL != pshadow->inner_data.ptopic_get) {
            IOT_MQTT_Unsubscribe(pshadow->mqtt, pshadow->inner_data.ptopic_get);
        }

        HAL_SleepMs(2000);
        IOT_MQTT_Destroy(&pshadow->mqtt);
    }

    if (NULL != pshadow->inner_data.ptopic_get) {
        LITE_free(pshadow->inner_data.ptopic_get);
    }

    if (NULL != pshadow->inner_data.ptopic_update) {
        LITE_free(pshadow->inner_data.ptopic_update);
    }

    if (NULL != pshadow->inner_data.attr_list) {
        list_destroy(pshadow->inner_data.attr_list);
    }

    if (NULL != pshadow->mutex) {
        HAL_MutexDestroy(pshadow->mutex);
    }

    LITE_free(handle);

    return SUCCESS_RETURN;
}


iotx_err_t IOT_Shadow_RegisterAttribute(void *handle, iotx_shadow_attr_pt pattr)
{
    /* check if already registered */
    if (iotx_ds_common_check_attr_existence((iotx_shadow_pt)handle, pattr)) {
        return ERROR_SHADOW_ATTR_EXIST;
    }

    if (SUCCESS_RETURN != iotx_ds_common_register_attr((iotx_shadow_pt)handle, pattr)) {
        return FAIL_RETURN;
    }

    return SUCCESS_RETURN;
}


/* Remove attribute from Device Shadow in cloud by delete method. */
iotx_err_t IOT_Shadow_DeleteAttribute(void *handle, iotx_shadow_attr_pt pattr)
{
#define SHADOW_DELETE_MSG_SIZE      (256)

    iotx_err_t ret;
    void *buf;
    format_data_t format;
    iotx_shadow_pt pshadow = (iotx_shadow_pt) handle;

    if (!iotx_ds_common_check_attr_existence(pshadow, pattr)) {
        return ERROR_SHADOW_ATTR_NO_EXIST;
    }

    buf = LITE_malloc(SHADOW_DELETE_MSG_SIZE);
    if (NULL == buf) {
        return ERROR_NO_MEM;
    }

    iotx_ds_common_format_init(pshadow, &format, buf, SHADOW_DELETE_MSG_SIZE, "delete", ",\"state\":{\"reported\":{");
    iotx_ds_common_format_add(pshadow, &format, pattr->pattr_name, NULL, IOTX_SHADOW_NULL);
    iotx_ds_common_format_finalize(pshadow, &format, "}}");

    ret = IOT_Shadow_Push(pshadow, format.buf, format.offset, 10);
    if (SUCCESS_RETURN != ret) {
        LITE_free(buf);
        return ret;
    }

    LITE_free(buf);

    return iotx_ds_common_remove_attr(pshadow, pattr);

#undef SHADOW_DELETE_MSG_SIZE
}
