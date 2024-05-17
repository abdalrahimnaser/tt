// Copyright (C) 2024 Toitware ApS.
// Use of this source code is governed by a Zero-Clause BSD license that can
// be found in the LICENSE file.

#include <toit/toit.h>
#include <qrcode.h>
#include <esp_log.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

toit_msg_request_handle_t current_handle;

static void reply(void* data, int data_size) {
  toit_err_t err = toit_msg_request_reply(current_handle, data, data_size, true);
  if (err != TOIT_OK) {
    ESP_LOGE("toit-qrcode", "Failed to send QR code: %d", err);
  }
}

static void fail(const char* message) {
  toit_err_t err = toit_msg_request_fail(current_handle, message);
  if (err != TOIT_OK) {
    ESP_LOGE("toit-qrcode", "Failed to send exception '%s': %d", message, err);
  }
}

static void on_qrcode_generated(esp_qrcode_handle_t qrcode) {
  int size = esp_qrcode_get_size(qrcode);
  // 1 byte to store the size of the QR code.
  // Then ceil(size * size / 8) bytes to store the data.
  int data_size = 1 + (size * size + 7) / 8;
  uint8_t* data = toit_calloc(1, data_size);
  if (data == NULL) {
    fail("Failed to allocate memory for QR code");
    return;
  }
  data[0] = size;
  int bit_index = 8;
  for (int x = 0; x < size; x++) {
    for (int y = 0; y < size; y++) {
      if (esp_qrcode_get_module(qrcode, x, y)) {
        data[bit_index >> 3] |= 1 << (bit_index & 0x7);
      }
      bit_index++;
    }
  }
  reply(data, data_size);
}

static toit_err_t on_rpc_request(void* user_data, int sender, int function, toit_msg_request_handle_t handle, uint8_t* data, int length) {
  current_handle = handle;
  if (data[length] != '\0') {
    fail("Invalid request");
    return TOIT_OK;
  }
  esp_qrcode_config_t config = {
    .display_func = on_qrcode_generated,
    .max_qrcode_version = 10,
    .qrcode_ecc_level = ESP_QRCODE_ECC_LOW,
  };
  esp_err_t err = esp_qrcode_generate(&config, (const char*)data);
  if (err == ESP_ERR_NO_MEM) {
    toit_gc();
    err = esp_qrcode_generate(&config, (const char*)data);
  }
  if (err != ESP_OK) fail("Failed to generate QR code");
  return TOIT_OK;
}

static void __attribute__((constructor)) init() {
  toit_msg_cbs_t cbs = TOIT_MSG_EMPTY_CBS();
  cbs.on_rpc_request = on_rpc_request;
  toit_msg_add_handler("toitlang.org/tutorial-qrcode", NULL, cbs);
}
