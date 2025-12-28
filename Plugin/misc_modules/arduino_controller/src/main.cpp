/* 
  Arduino SDR++ Controller - Linux plugin.
  Bartłomiej Marcinkowski
*/

#include <imgui.h>
#include <module.h>
#include <gui/gui.h>
#include <gui/style.h>
#include <core.h>
#include <signal_path/signal_path.h>
#include <gui/widgets/frequency_select.h>
#include <radio_interface.h>
#include <config.h>
//#include <options.h> - not available in v1.06 
#define CONCAT(a, b) ((std::string(a) + b).c_str())

#include <stdio.h>
#include <string.h>

#include <serialib.h>

#if defined (_WIN32) || defined(_WIN64)
    #define DEFAULT_SERIAL_PORT "\\\\.\\COM4"
#endif      
#if defined (__linux__) || defined(__APPLE__)
    #define DEFAULT_SERIAL_PORT "/dev/ttyACM0"
#endif



SDRPP_MOD_INFO{
    /* Name:            */ "arduino_controller",
    /* Description:     */ "Arduino SDR++ controller",
    /* Author:          */ "Bartłomiej Marcinkowski",
    /* Version:         */ 0, 2, 0,
    /* Max instances    */ 1
};

ConfigManager config;

class ArduinoController : public ModuleManager::Instance {
public:
    ArduinoController(std::string name) {
        this->name = name;
        gui::menu.registerEntry(name, menuHandler, this, NULL);

        config.acquire();
        if (!config.conf.contains(name)) {
            config.conf[name]["ttyport"] = DEFAULT_SERIAL_PORT;
        }
        config.release(true);
        std::string port = config.conf[name]["ttyport"];
        strcpy(ttyport, port.c_str());
    }

    ~ArduinoController() {
        gui::menu.removeEntry(name);
    }

    void postInit() {
        //connectArduino();
    }

    void enable() {
        enabled = true;
    }

    void disable() {
        enabled = false;
    }

    bool isEnabled() {
        return enabled;
    }

private:
    static void menuHandler(void* ctx) {
//        float menuWidth = ImGui::GetContentRegionAvailWidth(); - not available in v1.06
        float menuWidth = ImGui::GetContentRegionAvail().x;
        int freq = int(gui::waterfall.getCenterFrequency() + sigpath::vfoManager.getOffset(gui::waterfall.selectedVFO));
        int mode;
        core::modComManager.callInterface(gui::waterfall.selectedVFO, RADIO_IFACE_CMD_GET_MODE, NULL, &mode);
        float gSNR = gui::waterfall.selectedVFOSNR;

        ArduinoController * _this = (ArduinoController *)ctx;

        ImGui::Text("Arduino SDR++ controller");

        ImGui::LeftLabel("Serial port:");
        ImGui::SetNextItemWidth(menuWidth - ImGui::GetCursorPosX());
        ImGui::SameLine();
        if (ImGui::InputText(CONCAT("##_arduino_controller_ttyport_", _this->name), _this->ttyport, 1023)) {
            config.acquire();
            config.conf[_this->name]["ttyport"] = std::string(_this->ttyport);
            config.release(true);
        }

        if (_this->serial_port && ImGui::Button(CONCAT("Stop##_arduino_controller_stop_", _this->name), ImVec2(menuWidth, 0))) {
            _this->disconnectArduino();
        }
        else if (!_this->serial_port && ImGui::Button(CONCAT("Start##_arduino_controller_stop_", _this->name), ImVec2(menuWidth, 0))) {
            _this->connectArduino();
        }
        ImGui::TextUnformatted("Status:");
        ImGui::SameLine();
        if (_this->serial_port) {
            ImGui::TextColored(ImVec4(0.0, 1.0, 0.0, 1.0), "Connected");
        } else {
            ImGui::TextColored(ImVec4(0.0, 1.0, 1.0, 1.0), "Disconnected");
        }

        ImGui::Text("Serial debug > %s", _this->commandReady);

        if (_this->serial_port) {
            _this->readArduino(); 
            _this->writeArduino(freq,mode,(int)gSNR); 
        }
    }

#if defined (_WIN32) || defined(_WIN64) // this is so ugly...
    #define CLOCK_MONOTONIC_RAW  4
    struct timespec { 
        long tv_sec;
        long tv_nsec; 
    }; 

    int clock_gettime(int, struct timespec *spec) {  
    # define CLOCK_MONOTONIC_RAW  4
        __int64 wintime; GetSystemTimeAsFileTime((FILETIME*)&wintime);
        wintime      -=116444736000000000i64;  //1jan1601 to 1jan1970
        spec->tv_sec  =wintime / 10000000i64;           //seconds
        spec->tv_nsec =wintime % 10000000i64 *100;      //nano-seconds
        return 0;
    }
#endif      

    std::string name;
    char ttyport[1024];
    bool enabled = true;
    struct timespec lastCall = {0,0};
    int serial_port = 0;
    serialib serial;

    int lastFreq = 0;
    int lastDemod = 0;
    int lastSnr = 0;
    int isInit = 0;
    float zoom = 1.0;

    char commandBuf[256];
    char commandReady[256];


    /* Serial commands (read):

    C1 , C4 - Right Knob - fine tune based on snap interval
    C2 , C3 - Left Knob - tune , step = snap interval * 10 
    C7 , C8 - Tune based on same rate - page up / page down
    C6 , C9 - Waterfall zoom in / zoom out
    C5 - Demodulator cycle 

    */
    void checkCommand(char *command) {

        ImGui::WaterfallVFO* vfo = gui::waterfall.vfos[gui::waterfall.selectedVFO];
        int snapInt = vfo->snapInterval;
        double freq = int(gui::waterfall.getCenterFrequency() + sigpath::vfoManager.getOffset(gui::waterfall.selectedVFO));
        double srate = sigpath::iqFrontEnd.getSampleRate();


        // right knob encoder (fine)tuner (snap interval)
        if(!strncmp(command,"C1",2)) {
            freq += snapInt;
        }
        if(!strncmp(command,"C4",2)) {
            freq -= snapInt;
        }

        // left knob encoder tuner (snap interval * 10)
        if(!strncmp(command,"C2",2)) {
            snapInt *= 10;
            freq += snapInt;
        }
        if(!strncmp(command,"C3",2)) {
            snapInt *= 10;
            freq -= snapInt;
        }

        // up/down tuner (sample rate based) aka pageup/pagedown
        if(!strncmp(command,"C7",2)) {
            freq += srate;
        }
        if(!strncmp(command,"C8",2)) {
            freq -= srate;
        }

        // zoom in / zoom out
        if(!strncmp(command,"C6",2)) {
            zoom -= 0.1;
            if(zoom < 0) {
                zoom = 0;
            }
        }
        if(!strncmp(command,"C9",2)) {
            zoom += 0.1;
            if(zoom > 1) {
                zoom = 1;
            }
        }

        // zoom in / zoom out main windows
        if (!strncmp(command,"C6",2) || !strncmp(command,"C9",2)) {
            double factor = (double)zoom * (double)zoom; // This code is duplicated from main_windows.c
            double wfBw = gui::waterfall.getBandwidth(); // I don't know how to update "Zoom" slider.
            double delta = wfBw - 1000.0;
            double finalBw = std::min<double>(1000.0 + (factor * delta), wfBw);
            gui::waterfall.setViewBandwidth(finalBw);

            if (vfo != NULL) {
                gui::waterfall.setViewOffset(vfo->centerOffset);
            }
        }

        // set up new frequency in normal mode 
        if (!strncmp(command,"C1",2) || !strncmp(command,"C2",2) || !strncmp(command,"C3",2) || !strncmp(command,"C4",2)) {
            freq = roundl(freq / snapInt) * snapInt;
            tuner::tune(tuner::TUNER_MODE_NORMAL, gui::waterfall.selectedVFO, freq);
        }

        // set up new frequency in center mode - for page up / page down / center knob click
        if (!strncmp(command,"C7",2) || !strncmp(command,"C8",2) || !strncmp(command,"CA",2)) {
            tuner::tune(tuner::TUNER_MODE_CENTER, gui::waterfall.selectedVFO, freq);
        }

        // cycle modes with SELECT button
        if(!strncmp(command,"C5",2)) {
            int mode;
            core::modComManager.callInterface(gui::waterfall.selectedVFO, RADIO_IFACE_CMD_GET_MODE, NULL, &mode);
            if(mode < 7) {
                mode++;
            } else {
                mode = 0;
            }
            core::modComManager.callInterface(gui::waterfall.selectedVFO, RADIO_IFACE_CMD_SET_MODE, &mode, NULL);
        }
    }

    void readArduino() {
        char read_buf[32];
        memset(&read_buf, '\0', sizeof(read_buf));

// this is not working exactly as I expected. I do not like windows
        int num_bytes = serial.readBytes(read_buf,sizeof(read_buf),1,0);

        if (num_bytes < 0) {
            flog::info("Error reading: {0}", strerror(errno));
        } else if (num_bytes > 0) {
            strncat(commandBuf,read_buf,num_bytes);
            if (commandBuf[strlen(commandBuf)-1] == '\n') {
                memcpy(commandReady,commandBuf,sizeof(commandReady));
                memset(&commandBuf,'\0',sizeof(commandBuf));  
                flog::info(">> {0}", commandReady);
                if (isInit == 0) { 
                    isInit = 1;
                    flog::info(">> Received first line");
                }
                checkCommand(commandReady);
            }
            //flog::info(">> {0}", commandBuf);
            //memset(&commandBuf,'\0',sizeof(commandBuf));
        }

    }
/* Serial commands (write):

F<frequency>\n - Update frequency info
S<snr>\n       - Update SNR level
M<mode>\n      - Update demodulator info
R\n            - Reset controller

*/

// this should be rewritten to support some kind of ACK and queue
// maybe some day ...

    void writeArduino(int frequency, int demod, int snr){
        struct timespec currentCall = {0,0};
        int delta_us = 0;
        char bsend;

        clock_gettime(CLOCK_MONOTONIC_RAW, &currentCall);
        delta_us = (currentCall.tv_sec - lastCall.tv_sec) * 1000000 + (currentCall.tv_nsec - lastCall.tv_nsec) / 1000;

        // do not flood this poor Arduino
        if (delta_us > 200000 && isInit > 0) {
            if(frequency != lastFreq) {
//                flog::info(">> Delta [{0}] {1} {2} {3} {4}", delta_us,lastCall.tv_nsec,lastCall.tv_sec,currentCall.tv_nsec,currentCall.tv_sec);
                char msg[14];
                memset(&msg,'\0',sizeof(msg));
                snprintf(msg,14,"F%d\n",frequency);
                bsend = serial.writeString(msg);
//                flog::info(">> Send freq {0}", msg);
                lastFreq = frequency;
                lastCall = currentCall;
                return;
            } 

            if(demod != lastDemod) {
                char dmsg[14];
                memset(&dmsg,'\0',sizeof(dmsg));
                snprintf(dmsg,14,"M%d\n",demod);
                bsend = serial.writeString(dmsg);
//                flog::info(">> Send demod {0}", dmsg);
                lastDemod = demod;
                lastCall = currentCall;
                return;
            }

            if(snr != lastSnr && delta_us > 500000) {
                char smsg[5];
                memset(&smsg,'\0',sizeof(smsg));
                if (lastSnr - snr > 6 && snr > 0 ) { // just to make readings more ... stable
                    snr = (lastSnr + snr ) / 2;
                }
                snprintf(smsg,5,"S%d\n",snr);
                bsend = serial.writeString(smsg);
//                flog::info(">> Send SNR {0}", smsg);
                lastSnr = snr;
                lastCall = currentCall;
                return;
            }
        }
    }

    void disconnectArduino(){
        serial.closeDevice();
        serial_port = 0;
        isInit = 0;
        lastDemod = 0;
        lastFreq = 0;
        lastSnr = 0;
        flog::info("Disconnect Arduino");
    }

    void connectArduino(){
        // Connection to serial port
        char errorOpening = serial.openDevice(ttyport, 9600);
        if (errorOpening!=1) {
            flog::info("Error connecting to Arduino");
            serial_port = 0;
            return;
        }
        serial_port = 1;
        clock_gettime(CLOCK_MONOTONIC_RAW, &lastCall);

        //send reset command
        char reset_char = serial.writeString("R\n");

    }
    
};

MOD_EXPORT void _INIT_() {
//    config.setPath(options::opts.root + "/arduino_controller_config.json"); - not available in v1.06
    config.setPath(core::args["root"].s() + "/arduino_controller_config.json");
    config.load(json::object());
    config.enableAutoSave();
}

MOD_EXPORT ModuleManager::Instance* _CREATE_INSTANCE_(std::string name) {
    return new ArduinoController(name);
}

MOD_EXPORT void _DELETE_INSTANCE_(void* instance) {
    delete (ArduinoController *)instance;
}

MOD_EXPORT void _END_() {
    config.disableAutoSave();
    config.save();
}
