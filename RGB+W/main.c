#include <stdio.h>
#include <espressif/esp_wifi.h>
#include <espressif/esp_sta.h>
#include <esp/uart.h>
#include <esp8266.h>
#include <FreeRTOS.h>
#include <task.h>
#include <math.h> 

#include <homekit/homekit.h>
#include <homekit/characteristics.h>
#include <wifi_config.h>

#include "multipwm.h"

#define LFS 4

#define LED_RGB_SCALE 255    


#ifndef RED_PIN
#error WHITE_PIN is not specified
#endif


#ifndef GREEN_PIN
#error WHITE_PIN is not specified
#endif


#ifndef BLUE_PIN
#error WHITE_PIN is not specified
#endif


#ifndef WHITE_PIN
#error WHITE_PIN is not specified
#endif


typedef union {
    struct {
        uint16_t red;
        uint16_t green;
        uint16_t blue;
        uint16_t white;
    };
} rgb_color_t;

rgb_color_t current_color = {{  0, 0, 0, 0 }} ;
rgb_color_t target_color = {{  0, 0, 0, 0 }} ;

// Global variables
float led_hue = 0;
float led_saturation = 59;
float led_brightness = 100;
bool led_on = false;

//http://blog.saikoled.com/post/44677718712/how-to-convert-from-hsi-to-rgb-white
static void hsi2rgb(float h, float s, float i, rgb_color_t* rgb) {
    int r, g, b, w;

    while (h < 0) { h += 360.0F; };
    while (h >= 360) { h -= 360.0F; };
    h = 3.14159F*h / 180.0F;
    s /= 100.0F;
    i /= 100.0F;
    s = s > 0 ? (s < 1 ? s : 1) : 0;
    i = i > 0 ? (i < 1 ? i : 1) : 0;

    if (h < 2.09439) {
        r = LED_RGB_SCALE * i / 3 * (1 + s * cos(h) / cos(1.047196667 - h));
        g = LED_RGB_SCALE * i / 3 * (1 + s * (1 - cos(h) / cos(1.047196667 - h)));
        b = LED_RGB_SCALE * i / 3 * (1 - s);
    }
    else if (h < 4.188787) {
        h = h - 2.09439;
        g = LED_RGB_SCALE * i / 3 * (1 + s * cos(h) / cos(1.047196667 - h));
        b = LED_RGB_SCALE * i / 3 * (1 + s * (1 - cos(h) / cos(1.047196667 - h)));
        r = LED_RGB_SCALE * i / 3 * (1 - s);
    }
    else {
        h = h - 4.188787;
        b = LED_RGB_SCALE * i / 3 * (1 + s * cos(h) / cos(1.047196667 - h));
        r = LED_RGB_SCALE * i / 3 * (1 + s * (1 - cos(h) / cos(1.047196667 - h)));
        g = LED_RGB_SCALE * i / 3 * (1 - s);
    }

    if( r == g && g == b && r == b ){
        rgb->red = 0;
        rgb->green = 0;
        rgb->blue = 0;
        rgb->white = (uint8_t) (255 * i);
    }else {
        rgb->red = (uint8_t) r;
        rgb->green = (uint8_t) g;
        rgb->blue = (uint8_t) b;
        rgb->white  = 0;
    }
}

void led_identify_task(void *_args) {
    printf("LED identify\n");
    
    rgb_color_t color = target_color;
    rgb_color_t black_color = {0, 0, 0, 0};
    rgb_color_t red_color = {255, 0, 0, 0};
    
    for (int i=0; i<3; i++) {
        for (int j=0; j<2; j++) {
            target_color = red_color;
            vTaskDelay(100 / portTICK_PERIOD_MS);
            
            target_color = black_color;
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }

        vTaskDelay(250 / portTICK_PERIOD_MS);
    }

    target_color = color;

    vTaskDelete(NULL);
}

void led_identify(homekit_value_t _value) {
    xTaskCreate(led_identify_task, "LED identify", 128, NULL, 2, NULL);
}

homekit_value_t led_on_get() {
    return HOMEKIT_BOOL(led_on);
}

void led_on_set(homekit_value_t value) {
    if (value.format != homekit_format_bool) {
        // printf("Invalid on-value format: %d\n", value.format);
        return;
    }

    led_on = value.bool_value;
}

homekit_value_t led_brightness_get() {
    return HOMEKIT_INT(led_brightness);
}

void led_brightness_set(homekit_value_t value) {
    if (value.format != homekit_format_int) {
        // printf("Invalid brightness-value format: %d\n", value.format);
        return;
    }
    led_brightness = value.int_value;
}

homekit_value_t led_hue_get() {=
    return HOMEKIT_FLOAT(led_hue);
}

void led_hue_set(homekit_value_t value) {
    if (value.format != homekit_format_float) {
        // printf("Invalid hue-value format: %d\n", value.format);
        return;
    }
    led_hue = value.float_value;
}

homekit_value_t led_saturation_get() {
    return HOMEKIT_FLOAT(led_saturation);
}

void led_saturation_set(homekit_value_t value) {
    if (value.format != homekit_format_float) {
        // printf("Invalid sat-value format: %d\n", value.format);
        return;
    }
    led_saturation = value.float_value;
}

homekit_characteristic_t name = HOMEKIT_CHARACTERISTIC_(NAME, "LED Strip");

homekit_accessory_t *accessories[] = {
    HOMEKIT_ACCESSORY(.id = 1, .category = homekit_accessory_category_lightbulb, .services = (homekit_service_t*[]) {
        HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]){
            HOMEKIT_CHARACTERISTIC(NAME, "Light strip"),
            HOMEKIT_CHARACTERISTIC(MANUFACTURER, "Michael's Software"),
            HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "0x10"),
            HOMEKIT_CHARACTERISTIC(MODEL, "Make It Light"),
            HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "1.0"),
            HOMEKIT_CHARACTERISTIC(IDENTIFY, led_identify),
            NULL
        }),
        HOMEKIT_SERVICE(LIGHTBULB, .primary = true, .characteristics = (homekit_characteristic_t*[]) {
            HOMEKIT_CHARACTERISTIC(NAME, "LED Strip"),
            HOMEKIT_CHARACTERISTIC(
                ON, true,
                .getter = led_on_get,
                .setter = led_on_set
            ),
            HOMEKIT_CHARACTERISTIC(
                BRIGHTNESS, 100,
                .getter = led_brightness_get,
                .setter = led_brightness_set
            ),
            HOMEKIT_CHARACTERISTIC(
                HUE, 0,
                .getter = led_hue_get,
                .setter = led_hue_set
            ),
            HOMEKIT_CHARACTERISTIC(
                SATURATION, 0,
                .getter = led_saturation_get,
                .setter = led_saturation_set
            ),
            NULL
        }),
        NULL
    }),
    NULL
};

homekit_server_config_t config = {
    .accessories = accessories,
    .password = "248-12-524",
    .setupId="1A20"
};


IRAM void multipwm_task(void *pvParameters) {
    uint8_t pins[] = {RED_PIN, GREEN_PIN, BLUE_PIN, WHITE_PIN};

    pwm_info_t pwm_info;
    pwm_info.channels = 4;

    multipwm_init(&pwm_info);
    multipwm_set_freq(&pwm_info, 1000);
    for (uint8_t i=0; i<pwm_info.channels; i++) {
        multipwm_set_pin(&pwm_info, i, pins[i]);
    }

    while(1) { // ta petla bedzie sie wykonywac w kolko
       if (led_on) {
            hsi2rgb(led_hue, led_saturation, led_brightness, &target_color);
        } else {
            target_color.red = 0;
            target_color.green = 0;
            target_color.blue = 0;
            target_color.white = 0;
        }

        current_color.red += ((target_color.red * 256) - current_color.red) >> LFS ;
        current_color.green += ((target_color.green * 256) - current_color.green) >> LFS ;
        current_color.blue += ((target_color.blue * 256) - current_color.blue) >> LFS ;
        current_color.white += ((target_color.white * 256) - current_color.white) >> LFS ;

        multipwm_stop(&pwm_info);
        multipwm_set_duty(&pwm_info, 0, current_color.red);
        multipwm_set_duty(&pwm_info, 1, current_color.green);
        multipwm_set_duty(&pwm_info, 2, current_color.blue);
        multipwm_set_duty(&pwm_info, 3, current_color.white);
        multipwm_start(&pwm_info);
                
        vTaskDelay(1);
    }
}

void on_wifi_event(wifi_config_event_t event) {
    if (event == WIFI_CONFIG_CONNECTED) {
        printf("Connected to WiFi\n");
    } else if (event == WIFI_CONFIG_DISCONNECTED) {
        printf("Disconnected from WiFi\n");
    }
}

void user_init(void) {

    uart_set_baud(0, 115200);
    wifi_config_init2("Apple Home 0x10", NULL, on_wifi_event);

    xTaskCreate(multipwm_task, "multipwm", 256, NULL, 2, NULL);


    homekit_server_init(&config);
}
