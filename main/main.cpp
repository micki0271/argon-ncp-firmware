/*
 * Copyright (c) 2018 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include <esp_system.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "util.h"
#include "at_transport_uart.h"
#include "at_command_manager.h"

/* :( */
extern "C" {
#include "esp_at.h"
}

extern "C" void app_main(void);

const char* FIRMWARE_VERSION = "0.0.2";

const auto UART_CONF_INSTANCE = UART_NUM_2;
const auto UART_CONF_TX_PIN = 17;
const auto UART_CONF_RX_PIN = 16;
const auto UART_CONF_RTS_PIN = 18;
const auto UART_CONF_CTS_PIN = 19;
const auto UART_CONF_BAUD_RATE = 921600;
const auto UART_CONF_DATA_BITS = UART_DATA_8_BITS;
const auto UART_CONF_PARITY = UART_PARITY_DISABLE;
const auto UART_CONF_STOP_BITS = UART_STOP_BITS_1;
const auto UART_CONF_FLOW_CONTROL = UART_HW_FLOWCTRL_DISABLE;
/* magick number */
const auto UART_CONF_RX_FLOW_CTRL_THRESH = 122;

using namespace particle;
using namespace particle::util;
using namespace particle::ncp;

int atInitialize() {
    /* UART transport configuration */
    AtUartTransport::Config conf = {};
    conf.uart = UART_CONF_INSTANCE;
    conf.txPin = UART_CONF_TX_PIN;
    conf.rxPin = UART_CONF_RX_PIN;
    conf.rtsPin = UART_CONF_RTS_PIN;
    conf.ctsPin = UART_CONF_CTS_PIN;

    conf.config.baud_rate = UART_CONF_BAUD_RATE;
    conf.config.data_bits = UART_CONF_DATA_BITS;
    conf.config.parity = UART_CONF_PARITY;
    conf.config.stop_bits = UART_CONF_STOP_BITS;
    conf.config.flow_ctrl = UART_CONF_FLOW_CONTROL;
    conf.config.rx_flow_ctrl_thresh = UART_CONF_RX_FLOW_CTRL_THRESH;

    /* Initialize transport first */
    static AtUartTransport transport(conf);
    CHECK(transport.init());

    CHECK(AtCommandManager::instance()->init());

    /* Initialize AT library */
    esp_at_module_init(CONFIG_LWIP_MAX_SOCKETS - 1, (const uint8_t*)FIRMWARE_VERSION);

    /* Initialize command sets */
    /* Base AT commands */
    CHECK_BOOL(esp_at_base_cmd_regist());
    /* WiFi AT commands */
    CHECK_BOOL(esp_at_wifi_cmd_regist());

    CHECK(transport.postInit());

    return 0;
}

int miscInitialize() {
    /* For some reason by default esp32-at configures wifi as STA + SoftAP.
     * We don't want that for now, so we check the current mode and set it to STA only
     * if needed.
     */
    wifi_mode_t mode;
    CHECK_ESP(esp_wifi_get_mode(&mode));
    if (mode != WIFI_MODE_STA) {
        CHECK_ESP(esp_wifi_set_mode(WIFI_MODE_STA));
    }

    return 0;
}

int main() {
    LOG(INFO, "Starting Argon NCP firmware version: %s", FIRMWARE_VERSION);

    CHECK(nvsInitialize());
    CHECK(atInitialize());

    LOG(INFO, "Initialized");

    return 0;
}

void app_main(void) {
    main();

    while(true) {
        vTaskDelay(1000);
    }
}
