// This #include statement was automatically added by the Particle IDE.
STARTUP(WiFi.selectAntenna(ANT_EXTERNAL));
SYSTEM_THREAD(ENABLED);

void dimThread(void *param);
Thread thread("backgroundThread" ,dimThread);
void rgbWiFiThread(void *param);
Thread thread2("background2Thread" ,rgbWiFiThread);
system_tick_t lastThreadTime = 0;


int spkrOn = 0;
double voltage;
int light;
int dim = 255;
int touchDetect; 
int counter;
int button;


void setup() {
    Particle.variable("voltage", voltage); 
    Particle.variable("light", light);
    Particle.variable("button", button);
    Particle.function("relayHandler", relayHandler);
    pinMode(A1, INPUT); //photoresistor on A1
    pinMode(D5,INPUT); //D5 is button
    RGB.control(true);
}

void loop() {
    Particle.process();
    button = digitalRead(D5);
    if(button == 1){
        if (spkrOn == 1){
            digitalWrite(D3,LOW); //relay sense on D3
            spkrOn = 0;
            delay(250);
        }
        else{
            digitalWrite(D3,HIGH);
            spkrOn = 1;
            delay(250);
        }
    }
    voltage = (analogRead(A2)/4095.0)*3.3*4.0; 
    //analogRead will map 0-3.3v as 0 to 4095; multiply by 4 resistors to get vbatt
    //voltage divider on A2
    
    /*
    broken IR sensor code
    touchDetect = digitalRead(D4);
    if(touchDetect == 1) //something is breaking the line
    {
        counter = 0; 
    }
    else if(touchDetect == 0){
        counter++;
        if(counter>50){
            if(spkrOn == 1){
                digitalWrite(D3,LOW);
                spkrOn = 0;
                counter = 0;
            }
            else{
                digitalWrite(D3,HIGH);
                spkrOn = 1;
                counter = 0;
            }
            
        }
    }
    */
    delay(50);    
}

int relayHandler(String args){
    if (args == "on" || args == "connect"){
        digitalWrite(D3,HIGH);
        spkrOn = 1;
    }
    else if (args =="off" || args == "disconnect"){
        digitalWrite(D3,LOW);
        spkrOn = 0;
    }
    else if (spkrOn == 1){
        digitalWrite(D3,LOW);
        spkrOn = 0;
    }
    else{
        digitalWrite(D3,HIGH);
        spkrOn = 1;
    }
    return spkrOn;
}

void dimThread(void *param){
    while(true) {
        if(!spkrOn){
    		light = analogRead(A1);
            if(light<500 && dim > 13){ //after the photoresistor returns ~<1 V, dim the RGB led to 5% 
                dim--;
                RGB.brightness(dim);
            }
            else if(light<1500 && light>500){ //inbetween values, we need to check if dim is above or below
                if(dim<100){
                    dim++;
                    RGB.brightness(dim);
                }
                else if (dim>100){
                    dim--;
                    RGB.brightness(dim);
                }
            }
            else if(light>1500 && dim < 255){ //greater than ~1V will increment and increase RGB led to 100%
                dim++;
                RGB.brightness(dim);
            }
        }
		os_thread_delay_until(&lastThreadTime, 1500); 
	}
}

void rgbWiFiThread(void *param){ //sets the color of led based on Wifi status and battery
    while(true){
        if(WiFi.ready())
            RGB.color(0,255,255);
        else if (WiFi.connecting())
            RGB.color(252, 192, 202);
        else
            RGB.color(255, 234, 150);
    os_thread_delay_until(&lastThreadTime, 1000);// Delay so we're called every 1500 milliseconds 
    }
}


