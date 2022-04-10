//Inspired and Retrieved from: https://randomnerdtutorials.com/esp32-cam-pan-and-tilt-2-axis/

//The web server for Haircut Camera Computation
//The server is used for creating the interface for camera streaming

//Camera libraries
#include <esp_camera.h>
#include <Arduino.h>
#include "img_converters.h"
#include "esp_timer.h"
#include <WiFi.h>
#include "fb_gfx.h"
#include "soc/soc.h"             // disable brownout problems
#include "soc/rtc_cntl_reg.h"    // disable brownout problems
#include "esp_http_server.h"
#include <ESP32Servo.h>

//Neopixel libraries
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
 #include <avr/power.h>
#endif

/*  Wrong lib 
////www.github.com/me-no-dev/AsyncTCP
//#include <AsyncTCP.h>
////www.github.com/me-no-dev/ESPAsyncWebServer
//#include <ESPAsyncWebServer.h>
//#include <iostream>
//#include <sstream>  */


//AI THINKER ESP32-CAM GPIO pins set up
//camera_pins.h
#define PART_BOUNDARY "123456789000000000000987654321"
#define CAMERA_MODEL_AI_THINKER

#if defined(CAMERA_MODEL_AI_THINKER)
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

#else
  #error "Camera model not selected"
#endif


//Define LED Pins and Object
#define Light  14
#define Led_Numbers 50
Adafruit_NeoPixel strip(Led_Numbers, Light, NEO_GRB + NEO_KHZ800);

//Wi-Fi information
const char* ssid = "freeman";
const char* password = "cogeco388";

int buttonInput = 2;
int buttonState = 14;


//mjpeg streaming
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

//data initialized without value
httpd_handle_t camera_httpd = NULL;
httpd_handle_t stream_httpd = NULL;


//HTML Code for web server interface
static const char PROGMEM INDEX_HTML[] = R"homePage(
<html>
<head>
    <title>Personal Haircut Camera</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">

    <style>
        body {
            width: 100%;
            font-family: Roboto;
            background: linear-gradient(45deg, #343C60, #313646);
            color: #ffffff;
            text-align: center;
            margin: 0;
            align-content: center;
        }

        .Header {
            /*            width: 100%;*/
            align-items: center;
            margin: 2rem auto 2rem;
            height: 4rem;
            padding: 0px 0;
            border-radius: 25px;

            padding: -20px 0 20px;
            /*            background: linear-gradient(45deg, #343C60, #313646);*/
            /*
            box-shadow: 8px 8px 30px #202539,
            -8px -8px 30px #495488;
*/

        }

        h2 {
            text-align: center;
            font-size: 1.5em;
            font-weight: 900;
        }

        p {
            text-align: center;
            font-size: 1em;
        }

        img {
            width: auto;
            max-width: 100%;
            height: auto;
            /* border: 10px solid #ffffff;  */
            border-radius: 25px;
        }

        .center {
            max-width: 1200px;
            text-align: center;
            align-items: center;
            vertical-align: baseline;
            background-color: #ffffff;
            margin: auto;
            border-radius: 25px;
            padding: 0px 0 30px;
            background: linear-gradient(45deg, #343C60, #313646);
            box-shadow: 8px 8px 30px #202539,
                -8px -8px 30px #495488;
        }


        @media only screen and (min-width: 300px) {
            .Header {
                max-width: 350px;
            }

            .center {
                max-width: 350px;
            }
        }

        @media only screen and (min-width: 768px) {
            .Header {
                max-width: 700px;
            }

            .center {
                max-width: 700px;
            }
        }

        @media only screen and (min-width: 1200px) {
            .Header {
                max-width: 1000px;
            }

            .center {
                max-width: 1000px;
            }
        }

    </style>
</head>

<body>
    <div class="Header">
        <h2>
            Haircut Camera
        </h2>
    </div>

    <div class="center">
        <img src="" id="photo">
        <br>
        <p>
            <ion-icon name="information-circle-outline" style="font-size: 16px; padding-top:20px"></ion-icon>
            The camera should face the back head.
        </p>
    </div>


    <!-- Abandoned interface elements for controlling the light -->
    <!--  <div class="hueContainer">
          Hue<br>
      <input type="range" align="center" min="1" max="360" value="0" id="hue" onchange="updateH(this)">
      <p id="outputH"></p>
    </div>
    <br>
    <div class="satContainer">
          Saturation<br>
      <input type="range" align="center" min="1" max="100" value="100" id="sat" onchange="updateS(this)">
      <p id="outputS"></p>
    </div>
    <br>
    <div class="ligContainer">
          Lightness<br>
      <input type="range" align="center" min="1" max="100" value="50" id="lig" onchange="updateL(this)">
      <p id="outputL"></p>
    </div>   -->

    <script>
        //  var sliderH = document.getElementById("hue").value;
        //  var sliderS = document.getElementById("sat").value;
        //  var sliderL = document.getElementById("lig").value;
        //  var outputH = sliderH.innerHTML;
        //  var outputS = sliderS.innerHTML;
        //  var outputL = SLIDERb.innerHTML;
        //  
        //  updateH =>{var xhr = new XMLHttpRequest(); xhr.open("GET", "/action?slider="+outputH, true); xhr.send()}
        //  updateS =>{var xhr = new XMLHttpRequest(); xhr.open("GET", "/action?slider="+outputS, true); xhr.send()}
        //  updateL =>{var xhr = new XMLHttpRequest(); xhr.open("GET", "/action?slider="+outputL, true); xhr.send()}


        //streaming via the http link
        window.onload = document.getElementById("photo").src = window.location.href.slice(0, -1) + ":81/stream";

    </script>
</body>

</html>
)homePage";


/*No static const esp_err_t index_handler(httpd_req_t *req){ */
//https://www.geeksforgeeks.org/difference-between-static-and-const-in-javascript/#:~:text=The%20static%20methods%20are%20basically,or%20a%20method%20as%20static.
static esp_err_t index_handler(httpd_req_t *req){
  httpd_resp_set_type(req, "text/html");
  return httpd_resp_send(req, (const char *)INDEX_HTML, strlen(INDEX_HTML));
}

//streaming control
static esp_err_t stream_handler(httpd_req_t *req){
  camera_fb_t * fb = NULL;
  esp_err_t res = ESP_OK;
  size_t _jpg_buf_len = 0;
  uint8_t * _jpg_buf = NULL;
  char * part_buf[64];

  //reset link
  res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
  if(res != ESP_OK){
    return res;
  }

  //apply lens and buff effects
  while(true){
    fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      res = ESP_FAIL;
    } else {
      if(fb->width > 400){
        if(fb->format != PIXFORMAT_JPEG){
          bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
          esp_camera_fb_return(fb);
          fb = NULL;
          if(!jpeg_converted){
            Serial.println("JPEG compression failed");
            res = ESP_FAIL;
          }
        } else {
          _jpg_buf_len = fb->len;
          _jpg_buf = fb->buf;
        }
      }
    }

    //if the ESP Cam works fine, send link to the <img> through /stream
    if(res == ESP_OK){
      size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, _jpg_buf_len);
      res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
    }
    if(res == ESP_OK){
      res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
    }
    if(res == ESP_OK){
      res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
    }
    if(fb){
      esp_camera_fb_return(fb);
      fb = NULL;
      _jpg_buf = NULL;
    } else if(_jpg_buf){
      free(_jpg_buf);
      _jpg_buf = NULL;
    }
    if(res != ESP_OK){
      break;
    }
    //Serial.printf("MJPG: %uB\n",(uint32_t)(_jpg_buf_len));
  }
  return res;
}

//control that send and receive the /action link. This part should be ignored
static esp_err_t cmd_handler(httpd_req_t *req){
  char*  buf;
  size_t buf_len;
  char variable[32] = {0,};
  
  buf_len = httpd_req_get_url_query_len(req) + 1;
  if (buf_len > 1) {
    buf = (char*)malloc(buf_len);
    if(!buf){
      httpd_resp_send_500(req);
      return ESP_FAIL;
    }
    if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
      if (httpd_query_key_value(buf, "go", variable, sizeof(variable)) == ESP_OK) {
      } else {
        free(buf);
        httpd_resp_send_404(req);
        return ESP_FAIL;
      }
    } else {
      free(buf);
      httpd_resp_send_404(req);
      return ESP_FAIL;
    }
    free(buf);
  } else {
    httpd_resp_send_404(req);
    return ESP_FAIL;
  }

  sensor_t * s = esp_camera_sensor_get();
  //flip the camera vertically
  //s->set_vflip(s, 1);          // 0 = disable , 1 = enable
  // mirror effect
  //s->set_hmirror(s, 1);          // 0 = disable , 1 = enable

  int res = 0;

  if(res){
    return httpd_resp_send_500(req);
  }

  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  return httpd_resp_send(req, NULL, 0);
}

//Links for connecting the server and the front end
void startCameraServer(){
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 80;

  //creates an html page
  httpd_uri_t index_uri = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = index_handler,
    .user_ctx  = NULL
  };

  //interaction (should be ignored)
  httpd_uri_t cmd_uri = {
    .uri       = "/action",
    .method    = HTTP_GET,
    .handler   = cmd_handler,
    .user_ctx  = NULL
  };

  //stream in MJPEG format
  httpd_uri_t stream_uri = {
    .uri       = "/stream",
    .method    = HTTP_GET,
    .handler   = stream_handler,
    .user_ctx  = NULL
  };
  if (httpd_start(&camera_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(camera_httpd, &index_uri);
    httpd_register_uri_handler(camera_httpd, &cmd_uri);
  }
  config.server_port += 1;
  config.ctrl_port += 1;
  if (httpd_start(&stream_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(stream_httpd, &stream_uri);
  }
}

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector
  
  Serial.begin(115200);
  Serial.setDebugOutput(false);

  //default ESP settings for streaming
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
  
  if(psramFound()){
    config.frame_size = FRAMESIZE_VGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 15;
    config.fb_count = 1;
  }
  
  // Camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
  // Wi-Fi connection
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  
  Serial.print("Camera Stream Ready! Go to: http://");
  Serial.println(WiFi.localIP());
  
  // Start streaming web server
  startCameraServer();

  //Start the neopixel project
  strip.begin();           
  strip.show();            
  strip.setBrightness(30);

  pinMode(buttonInput, INPUT);
  pinMode(buttonState, OUTPUT);
}

void loop() {
//  buttonState = digitalRead(buttonInput);
//  if(buttonState == HIGH){
//  Serial.println("Button ON!!");

  //ColorWipe iterating effect in the strip
  colorWipe(strip.Color(255, 255, 255), 60); // White with some special effects
//  }else if(buttonState == LOW){
//  Serial.println("Button OFF!!");
//  };
}

void colorWipe(uint32_t color, int wait) {
  for(int i=0; i<strip.numPixels(); i++) { // For each pixel in strip...
    strip.setPixelColor(i, color);         //  Set pixel's color (in RAM)
    strip.show();                          //  Update strip to match
    delay(wait);                           //  Pause for a moment
  }
}
