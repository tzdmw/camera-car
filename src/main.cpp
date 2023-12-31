#include "esp_camera.h"
#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <iostream>
#include <sstream>

struct MOTOR_PINS
{
    int pinEn;
    int pinIN1;
    int pinIN2;
};

std::vector<MOTOR_PINS> motorPins =
        {
                {12, 13, 15},  //RIGHT_MOTOR Pins (EnA, IN1, IN2)
                {12, 14, 2},  //LEFT_MOTOR  Pins (EnB, IN3, IN4)
        };
#define LIGHT_PIN 4

#define RIGHT_UP 1
#define UP 2
#define LIFT_UP 3
#define LIFT 4
#define LIFT_DOWN 5
#define DOWN 6
#define RIGHT_DOWN 7
#define RIGHT 8
#define STOP 0

#define RIGHT_MOTOR 0
#define LEFT_MOTOR 1

#define FORWARD 1
#define BACKWARD -1

const int PWMFreq = 1000; /* 1 KHz */
const int PWMResolution = 8;
const int PWMSpeedChannel = 2;
const int PWMLightChannel = 3;

//Camera related constants
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

const char* ssid     = "droid";
const char* password = "droid@2023/9/20";

AsyncWebServer server(80);
AsyncWebSocket wsCamera("/Camera");
AsyncWebSocket wsCarInput("/CarInput");
uint32_t cameraClientId = 0;

const char* htmlHomePage PROGMEM = R"HTMLHOMEPAGE(
<!DOCTYPE html>
<html>
<head>
    <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no">
    <script src="https://unpkg.com/nipplejs"></script>
    <title>esp32小车</title>
    <style>
        * {
            margin: 0;
            padding: 0;
        }
        #joystick-container {
            width: 200px;
            height: 200px;
            background: rgba(0, 0, 0, 0.15);
            border-radius: 100%;
            position: relative;
            margin: 50px auto;
        }
        #joystick{
            position: absolute;
            top: 50%;
            left: 50%;
            width: 60px;
            height: 60px;
            background: rgb(255, 255, 255);
            border-radius: 100%;
            transform: translate(-50%, -50%);
        }

        .noselect {
            -webkit-touch-callout: none; /* iOS Safari */
            -webkit-user-select: none; /* Safari */
            -khtml-user-select: none; /* Konqueror HTML */
            -moz-user-select: none; /* Firefox */
            -ms-user-select: none; /* Internet Explorer/Edge */
            user-select: none; /* Non-prefixed version, currently
                                      supported by Chrome and Opera */
        }

        .slidecontainer {
            width: 100%;
        }

        .slider {
            -webkit-appearance: none;
            width: 100%;
            height: 15px;
            border-radius: 5px;
            background: #d3d3d3;
            outline: none;
            opacity: 0.7;
            -webkit-transition: .2s;
            transition: opacity .2s;
        }

        .slider:hover {
            opacity: 1;
        }

        .slider::-webkit-slider-thumb {
            -webkit-appearance: none;
            appearance: none;
            width: 25px;
            height: 25px;
            border-radius: 50%;
            background: red;
            cursor: pointer;
        }

        .slider::-moz-range-thumb {
            width: 25px;
            height: 25px;
            border-radius: 50%;
            background: red;
            cursor: pointer;
        }

    </style>

</head>
<body class="noselect" align="center" style="background-color:white">
<h2 style="color: teal;text-align:center;">Wi-Fi Camera &#128663; Control</h2>

<table id="mainTable" style="width:400px;margin:auto;table-layout:fixed" CELLSPACING=10>
    <tr>
        <img id="cameraImage" src="" style="width:400px;height:250px"></td>
    </tr>
    <div id="joystick-container">
        <div id="joystick"></div>
    </div>
    <tr>
        <td style="text-align:left"><b>Light:</b></td>
        <td colspan=2>
            <div class="slidecontainer">
                <input type="range" min="0" max="255" value="0" class="slider" id="Light" oninput='sendButtonInput("Light",value)'>
            </div>
        </td>
    </tr>
</table>

<script>
    var webSocketCameraUrl = "ws:\/\/" + window.location.hostname + "/Camera";
    var webSocketCarInputUrl = "ws:\/\/" + window.location.hostname + "/CarInput";
    var websocketCamera;
    var websocketCarInput;



    var options = {
        zone: document.getElementById('joystick'),
        mode: 'static',
        position: { left: '50%', top: '50%' },
        color: 'green'
    };

    var manager = nipplejs.create(options);

    manager.on('move', function (evt, data) {
        var angle = data.angle.degree;
        var distance = data.distance;
        var direction = '';
        var speed = 0;
        if (distance > 10) {
            if (angle >= 22.5 && angle < 67.5) {
                direction = 'RIGHT_UP';
                speed = (distance - 10) / 90;//右上
            }else if (angle >= 67.5 && angle < 112.5) {
                direction = 'UP';
                speed = (distance - 10) / 90;//前
            } else if (angle >= 112.5 && angle < 157.5) {
                direction = 'LEFT_UP';
                speed = (distance - 10) / 90;//左上
            } else if (angle >= 157.5 && angle < 202.5) {
                direction = 'LEFT';
                speed = (distance - 10) / 90;//左
            } else if (angle >= 202.5 && angle < 247.5) {
                direction = 'LEFT_DOWN';
                speed = (distance - 10) / 90;//左下
            } else if (angle >= 247.5 && angle < 292.5) {
                direction = 'DOWN';
                speed = (distance - 10) / 90;//下
            } else if (angle >= 292.5 && angle < 337.5) {
                direction = 'RIGHT_DOWN';
                speed = (distance - 10) / 90;//右下
            } else if (angle >= 337.5 || angle < 22.5) {
                direction = 'RIGHT';
                speed = (distance - 10) / 90;//右
            }
            console.log(direction);
            console.log(speed);
        }
        sendjoystickInput("MoveCar",direction,speed);
    });

       manager.on('end', function () {
        sendjoystickInput("MoveCar","RIGHT_UP","0")
    });
    function initCameraWebSocket()
    {
        websocketCamera = new WebSocket(webSocketCameraUrl);
        websocketCamera.binaryType = 'blob';
        websocketCamera.onopen    = function(event){};
        websocketCamera.onclose   = function(event){setTimeout(initCameraWebSocket, 2000);};
        websocketCamera.onmessage = function(event)
        {
            var imageId = document.getElementById("cameraImage");
            imageId.src = URL.createObjectURL(event.data);
        };
    }

    function initCarInputWebSocket()
    {
        websocketCarInput = new WebSocket(webSocketCarInputUrl);
        websocketCarInput.onopen    = function(event)
        {
            var lightButton = document.getElementById("Light");
            sendButtonInput("Light", lightButton.value);
        };
        websocketCarInput.onclose   = function(event){sestTimeout(initCarInputWebSocket, 2000);};
        websocketCarInput.onmessage = function(event){};
    }

    function initWebSocket()
    {
        initCameraWebSocket ();
        initCarInputWebSocket();
    }

    function sendButtonInput(key, value)
    {
        var data = key + "," + value;
        websocketCarInput.send(data);
    }
    function sendjoystickInput(key, direction,speed)
    {
        var data = key + "," + direction+ "," +speed ;
        websocketCarInput.send(data);
    }

    window.onload = initWebSocket;
    document.getElementById("mainTable").addEventListener("touchend", function(event){
        event.preventDefault()
    });
</script>
</body>
</html>
)HTMLHOMEPAGE";


void rotateMotor(int motorNumber, int motorDirection,double speedInt)//TODO:speedInt没有写
{
    if (motorDirection == FORWARD)
    {
        digitalWrite(motorPins[motorNumber].pinIN1, HIGH);
        digitalWrite(motorPins[motorNumber].pinIN2, LOW);
        ledcWrite(PWMSpeedChannel, speedInt*570);
    }
    else if (motorDirection == BACKWARD)
    {
        digitalWrite(motorPins[motorNumber].pinIN1, LOW);
        digitalWrite(motorPins[motorNumber].pinIN2, HIGH);
        ledcWrite(PWMSpeedChannel, speedInt*570);
    }
    else
    {
        digitalWrite(motorPins[motorNumber].pinIN1, LOW);
        digitalWrite(motorPins[motorNumber].pinIN2, LOW);
    }
}

void moveCar(int directionInt,double speedInt)
{
    Serial.printf("Got directionInt as %d speedInt as %f\n", directionInt,speedInt);
    switch(directionInt)
    {

        case RIGHT_UP:
            rotateMotor(RIGHT_MOTOR, 0,speedInt);
            rotateMotor(LEFT_MOTOR, FORWARD,speedInt);
            break;

        case UP:
            rotateMotor(RIGHT_MOTOR, FORWARD,speedInt);
            rotateMotor(LEFT_MOTOR, FORWARD,speedInt);
            break;

        case LIFT_UP:
            rotateMotor(RIGHT_MOTOR, FORWARD,speedInt);
            rotateMotor(LEFT_MOTOR, 0,speedInt);
            break;

        case LIFT:
            rotateMotor(RIGHT_MOTOR, FORWARD,speedInt);
            rotateMotor(LEFT_MOTOR, BACKWARD,speedInt);
            break;

        case LIFT_DOWN:
            rotateMotor(RIGHT_MOTOR, BACKWARD,speedInt);
            rotateMotor(LEFT_MOTOR, 0,speedInt);
            break;

        case DOWN:
            rotateMotor(RIGHT_MOTOR, BACKWARD,speedInt);
            rotateMotor(LEFT_MOTOR, BACKWARD,speedInt);
            break;


        case RIGHT_DOWN:
            rotateMotor(RIGHT_MOTOR, 0,speedInt);
            rotateMotor(LEFT_MOTOR, BACKWARD,speedInt);
            break;

        case RIGHT:
            rotateMotor(RIGHT_MOTOR, BACKWARD,speedInt);
            rotateMotor(LEFT_MOTOR, FORWARD,speedInt);
            break;

        case STOP:
            rotateMotor(RIGHT_MOTOR, STOP,0);
            rotateMotor(LEFT_MOTOR, STOP,0);
            break;

        default:
            rotateMotor(RIGHT_MOTOR, STOP,0);
            rotateMotor(LEFT_MOTOR, STOP,0);
            break;
    }
}
int countCommas(const std::string& str) {
    int commaCount = 0;
    for (char c : str) {
        if (c == ',') {
            commaCount++;
        }
    }
    return commaCount;
}

void handleRoot(AsyncWebServerRequest *request)
{
    request->send_P(200, "text/html", htmlHomePage);
}

void handleNotFound(AsyncWebServerRequest *request)
{
    request->send(404, "text/plain", "File Not Found");
}

void onCarInputWebSocketEvent(AsyncWebSocket *server,
                              AsyncWebSocketClient *client,
                              AwsEventType type,
                              void *arg,
                              uint8_t *data,
                              size_t len)
{
    switch (type)
    {
        case WS_EVT_CONNECT:
            Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
            break;
        case WS_EVT_DISCONNECT:
            Serial.printf("WebSocket client #%u disconnected\n", client->id());
            moveCar(STOP,0);
            ledcWrite(PWMLightChannel, 0);
            break;
        case WS_EVT_DATA:
            AwsFrameInfo *info;
            info = (AwsFrameInfo*)arg;
            if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
            {
                std::string myData = "";
                myData.assign((char *)data, len);
                std::istringstream ss(myData);
                std::string key, direction,speed,value;
                if(countCommas(myData)==2)
                {
                    std::getline(ss, key, ',');
                    std::getline(ss, direction, ',');
                    std::getline(ss, speed, ',');
                    Serial.printf("Key [%s] direction[%s] speed[%s]\n", key.c_str(), direction.c_str(),speed.c_str());
                }
                else
                {
                    std::getline(ss, key, ',');
                    std::getline(ss, value, ',');
                    Serial.printf("Key [%s] valueInt[%s] \n", key.c_str(), value.c_str());
                }
                double speedInt = atof(speed.c_str());
                int valueInt = atoi(value.c_str());
                int directionInt;
                if(direction=="RIGHT_UP")
                    directionInt=1;
                else if(direction=="UP")
                    directionInt=2;
                else if(direction=="LEFT_UP")
                    directionInt=3;
                else if(direction=="LEFT")
                    directionInt=4;
                else if(direction=="LEFT_DOWN")
                    directionInt=5;
                else if(direction=="DOWN")
                    directionInt=6;
                else if(direction=="RIGHT_DOWN")
                    directionInt=7;
                else if(direction=="RIGHT")
                    directionInt=8;
                else if(direction=="STOP")
                    directionInt=0;
                if (key == "MoveCar")
                {
                    moveCar(directionInt,speedInt);
                }
                else if (key == "Light")
                {
                    ledcWrite(PWMLightChannel, valueInt);
                }
            }
            break;
        case WS_EVT_PONG:
        case WS_EVT_ERROR:
            break;
        default:
            break;
    }
}

void onCameraWebSocketEvent(AsyncWebSocket *server,
                            AsyncWebSocketClient *client,
                            AwsEventType type,
                            void *arg,
                            uint8_t *data,
                            size_t len)
{
    switch (type)
    {
        case WS_EVT_CONNECT:
            Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
            cameraClientId = client->id();
            break;
        case WS_EVT_DISCONNECT:
            Serial.printf("WebSocket client #%u disconnected\n", client->id());
            cameraClientId = 0;
            break;
        case WS_EVT_DATA:
            break;
        case WS_EVT_PONG:
        case WS_EVT_ERROR:
            break;
        default:
            break;
    }
}

void setupCamera()
{
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sscb_sda = SIOD_GPIO_NUM;
    config.pin_sscb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;

    config.frame_size = FRAMESIZE_VGA;
    config.jpeg_quality = 10;
    config.fb_count = 1;

    // camera init
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK)
    {
        Serial.printf("Camera init failed with error 0x%x", err);
        return;
    }

    if (psramFound())
    {
        heap_caps_malloc_extmem_enable(20000);
        Serial.printf("PSRAM initialized. malloc to take memory from psram above this size");
    }
}

void sendCameraPicture()
{
    if (cameraClientId == 0)
    {
        return;
    }
    unsigned long  startTime1 = millis();
    //capture a frame
    camera_fb_t * fb = esp_camera_fb_get();
    if (!fb)
    {
        Serial.println("Frame buffer could not be acquired");
        return;
    }

    unsigned long  startTime2 = millis();
    wsCamera.binary(cameraClientId, fb->buf, fb->len);
    esp_camera_fb_return(fb);

    //Wait for message to be delivered
    while (true)
    {
        AsyncWebSocketClient * clientPointer = wsCamera.client(cameraClientId);
        if (!clientPointer || !(clientPointer->queueIsFull()))
        {
            break;
        }
        delay(1);
    }

    unsigned long  startTime3 = millis();
//    Serial.printf("Time taken Total: %d|%d|%d\n",startTime3 - startTime1, startTime2 - startTime1, startTime3-startTime2 );
}
void setUpPinModes()
{
    //Set up PWM
    ledcSetup(PWMSpeedChannel, PWMFreq, PWMResolution);
    ledcSetup(PWMLightChannel, PWMFreq, PWMResolution);

    for (int i = 0; i < motorPins.size(); i++)
    {
        pinMode(motorPins[i].pinEn, OUTPUT);
        pinMode(motorPins[i].pinIN1, OUTPUT);
        pinMode(motorPins[i].pinIN2, OUTPUT);

        /* Attach the PWM Channel to the motor enb Pin */
        ledcAttachPin(motorPins[i].pinEn, PWMSpeedChannel);
    }
    moveCar(STOP,0);

    pinMode(LIGHT_PIN, OUTPUT);
    ledcAttachPin(LIGHT_PIN, PWMLightChannel);
}


void setup(void)
{
    setUpPinModes();
    Serial.begin(115200);

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println(WiFi.localIP());

    server.on("/", HTTP_GET, handleRoot);
    server.onNotFound(handleNotFound);
    wsCamera.onEvent(onCameraWebSocketEvent);
    server.addHandler(&wsCamera);
    wsCarInput.onEvent(onCarInputWebSocketEvent);
    server.addHandler(&wsCarInput);

    server.begin();
    Serial.println("HTTP server started");
    setupCamera();
}


void loop()
{
    wsCamera.cleanupClients();
    wsCarInput.cleanupClients();
    sendCameraPicture();
//    Serial.printf("SPIRam Total heap %d, SPIRam Free Heap %d\n", ESP.getPsramSize(), ESP.getFreePsram());
}
