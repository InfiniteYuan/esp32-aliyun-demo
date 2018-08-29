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



#ifndef __TFS_ID2_H
#define __TFS_ID2_H

#include <stdint.h>

/* ID2 length */
#define TFS_ID2_LEN           24
/* for 3DES sign len is 16 + 8, for rsa sign len is 128 */
#define TFS_ID2_SIGN_SIZE     128

/* AES padding type*/
#define TFS_AES_PKCS7_PADDING 0X01
#define TFS_AES_ZERO_PADDING  0X02
#define TFS_AES_NO_PADDING    0X03

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief get ID2 informaiton
 *
 * @param[out] id2: the mem must >= TFS_ID2_LEN.
 * @param[inout] len: TFS_ID2_LEN.
 * @return: 0~OK, other~ERROR.
 * @note None.
 */
int tfs_get_ID2(uint8_t *id2, uint32_t *len);

/**
 * @brief decrypt cipher text with ID2 key
 *
 * @param[in] in: input data.
 * @param[in] in_len: length of intput data, which must <= 4096 bytes.
 * @param[out] out: decrypted data.
 * @param[inout] out_len: the length of decrypted data.
 * @return: 0~OK, other~ERROR.
 * @note None.
 */
int tfs_id2_decrypt(const uint8_t *in, uint32_t in_len,
                    uint8_t *out, uint32_t *out_len);

/**
 * @brief get auth code, challenge mode
 *
 * @param[in] challenge: sRandom from ID2 server.
 * @param[in] extra: extra data, optional, no more than 512 bytes.
 * @param[in] extra_len: length of extra data, which must <= 512 bytes.
 * @param[out] auth_code: auth code.
 * @param[out] len: length of auth code.
 * @return: 0~OK, other~ERROR.
 * @note None.
 */
int tfs_id2_get_challenge_auth_code(const uint8_t *challenge,
                                    const uint8_t *extra, uint32_t extra_len,
                                    uint8_t *auth_code, uint32_t *len);

/**
 * @brief get auth code, timestemp mode
 *
 * @param[in] timestamp: timestamp from caller, the number of milliseconds since the Epoch, 1970-01-01 00:00:00 +0000 (UTC).
 * @param[in] extra: extra data, optional, no more than 512 bytes.
 * @param[in] extra_len: length of extra data, which must <= 512 bytes.
 * @param[out] auth_code: auth code.
 * @param[out] len: length of auth code.
 * @return: 0~OK, other~ERROR
 * @note None.
 */
int tfs_id2_get_timestamp_auth_code(const uint8_t *timestamp,
                                    const uint8_t *extra, uint32_t extra_len,
                                    uint8_t *auth_code, uint32_t *len);

/**
 * @brief aes128 cbc encryption
 *
 * @param[in] key: key for aes128, key length should be 16 bytes.
 * @param[in/out] iv: initial vector for cbc, iv length should be 16 bytes. iv will be update.
 * @param[in] input_len:input data length, when padding type is nopadding, it should be multiple of 16, which must <= 2MB.
 * @param[in] input: input data for encryption.
 * @param[out] out_len:output data length.
 * @param[out] output: output data for encryption.
 * @param[in] padding: padding type, support pkcs7 padding, zero padding and no padding now.
 * @return: 0~OK, other~ERROR.
 * @note None.
 */
int32_t tfs_aes128_cbc_enc(const uint8_t *key,
                           uint8_t *iv,
                           int32_t input_len,
                           const uint8_t *input,
                           int32_t *output_len,
                           uint8_t *output,
                           uint8_t padding);

/**
 * @brief aes128 cbc decryption
 *
 * @param[in] key: key for aes128, key length should be 16 bytes.
 * @param[in/out] iv: initial vector for cbc, iv length should be 16 bytes. iv will be update.
 * @param[in] input_len:input data length, it should be multiple of 16, which must <= 2MB.
 * @param[in] input: input data for decryption.
 * @param[out] out_len:output data length.
 * @param[out] output: output data for decryption .
 * @param[in] padding: padding type, support pkcs7 padding, zero padding and no padding now.
 * @return: 0~OK, other~ERROR.
 * @note None.
 */
int32_t tfs_aes128_cbc_dec(const uint8_t *key,
                           uint8_t *iv,
                           int32_t input_len,
                           const uint8_t *input,
                           int32_t *output_len,
                           uint8_t *output,
                           uint8_t padding);

#ifdef __cplusplus
}
#endif

#endif
