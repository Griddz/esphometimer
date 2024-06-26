/****
Copyright (c) 2023 RebbePod

This library is free software; you can redistribute it and/or modify it 
under the terms of the GNU Lesser GeneralPublic License as published by the Free Software Foundation; 
either version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,but WITHOUT ANY WARRANTY; 
without even the impliedwarranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
See the GNU Lesser General Public License for more details. 
You should have received a copy of the GNU Lesser General Public License along with this library; 
if not, write tothe Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA, 
or connect to: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
****/

#pragma once
#include <array>
//#include <iostream>
//#include <chrono>
//#include <thread>

static template_::TemplateSwitch* * switches[] = {&enabled, &sunday, &monday, &tuesday, &wednesday, &thursday, &friday, &saturday, &repeat, &negative_offset};

static template_::TemplateNumber* * numbers[] = {&time_hour, &time_minute, &output, &action, &offset_hour, &offset_minute, &mode};

// the number should match the amount of relays below
const uint8_t num_of_relays = 1;

// match the output number to the position of the relay here
static gpio::GPIOSwitch* * relays[num_of_relays] = {&relay_0_heater};

const float protect_temp = 80;

//抗冻开始温度阀值
const float antifreez_temp = 3;

//抗冻结束温度阀值
const float antifreezstop_temp = 5;

//抗冻时水箱温度阀值
const float antifreeztank_temp =7;

//太阳能板与水箱的(换热开始阀值)温差高阀值
const float high_deltasolartanktop_temp = 8;

//太阳能板与水箱的温差(换热停止阀值))低阀值
const float low_deltasolartanktop_temp = 4;



/***************************************************
*      ***** Timer Format Cheat Guide *****
* **************************************************
*              ***** Switches *****
*   *** Value set to '1' enabled '0' disabled ***
* 0 = Enabled
* 1-7 = Days (sun-sat)
* 8 = Repeat
* 9 = Negative Offset
*               ***** Numbers *****
* 10 = Time hour
* 11 = Time minute
* 12 = Output  {value '0' for the first position of switch in the 'relays' variable}
* 13 = Action  {'0' turn off, '1' turn on, '2' toggle}
* 14 = Offset hour
* 15 = Offset minute
* 16 = Mode    {'0' use time, '1' use sunrise, '2' use sunset} 
****************************************************/

void doRelayAction(uint8_t i, time_t timestamp, bool set_relays) {
    ESP_LOGD("doRelayAction", "------ doRelayAction ran ------");
    if(set_relays == true){
        if(id(global_timer)[i][13] == 2) {
            ESP_LOGD("doRelayAction", "------ toggle ------");
            // toggle relays [position 12]
            (*relays[id(global_timer)[i][12]])->toggle();
        } else {
            ESP_LOGD("doRelayAction", "------ timer number %i ------", i);
            // check if action needed to be done
            if((*relays[id(global_timer)[i][12]])->state != id(global_timer)[i][13]) {
                ESP_LOGD("doRelayAction", "------ set state %i ------", id(global_timer)[i][13]);
                // set relay state [position 12] from timer action [position 13]
             //   (*relays[id(global_timer)[i][12]])->publish_state(id(global_timer)[i][13]);
                if(id(global_timer)[i][13] == true){
                    ESP_LOGD("doRelayAction", "------ set heater on");
                    (*relays[id(global_timer)[i][12]])->turn_on();
                    
                } else {
                    ESP_LOGD("doRelayAction", "------ set heater off");
                    (*relays[id(global_timer)[i][12]])->turn_off();

                }
            }
        }
    }
    // set last run time for this timer
    id(global_last_run)[i] = timestamp;
    // if repeat was disabled [position 8] deactivate timer [position 0] after running
    if(id(global_timer)[i][8] == 0) {
        id(global_timer)[i][0] = 0;
    }
}

void setTimestamps(uint8_t set_timer = 100) {
    uint8_t i = 0;
    uint8_t loops = 20;
    if(set_timer != 100) {
        i = set_timer;
        loops = set_timer + 1;
        ESP_LOGD("setTimestamps", "------ updating timer %i ------", set_timer + 1);
    } else {
        ESP_LOGD("setTimestamps", "------ updating all timers ------");
    }
    // set variables
    esphome::ESPTime date = id(sntp_time).now();
    struct tm tm;
    time_t timestamp = 0;
    // loop timers
    for(; i < loops; i++) {
        if(id(global_timer)[i][0] == 1) {
            // either repeat is disabled [position 8] or todays day of week is enabled [position 1-7]
            if(id(global_timer)[i][8] == 0 || id(global_timer)[i][0 + date.day_of_week] == 1) {
                ESP_LOGD("setTimestamps", "------ Active Timer %i is set to run today ------", i + 1);
                ESP_LOGD("setTimestamps", "------ timestamp today %lu ------", date.timestamp);
                // check timer mode [position 16] (time, sunrise etc.)
                if(id(global_timer)[i][16] == 0) {
                    ESP_LOGD("setTimestamps", "------ mode is set to time ------");
                    date.hour = date.minute = date.second = 0;                          // uptime time to 00:00:00
                    date.hour = id(global_timer)[i][10];
                    date.minute = id(global_timer)[i][11];
                    tm = date.to_c_tm();
                    timestamp = mktime(&tm);
                    ESP_LOGD("setTimestamps", "------ timestamp timer (time) %lu ------", timestamp);
                } else if(id(global_timer)[i][16] == 1) {
                    ESP_LOGD("setTimestamps", "------ mode is set to sunrise ------");
                    date.hour = date.minute = date.second = 0;                          // uptime time to 00:00:00
                    date.recalc_timestamp_utc();
                    timestamp = id(mysun).sunrise(date, -0.833)->timestamp;
                    ESP_LOGD("setTimestamps", "------ timestamp timer (sunrise) %lu ------", timestamp);
                } else if(id(global_timer)[i][16] == 2) {
                    ESP_LOGD("setTimestamps", "------ mode is set to sunset ------");
                    date.hour = date.minute = date.second = 0;                          // uptime time to 00:00:00
                    date.recalc_timestamp_utc();
                    timestamp = id(mysun).sunset(date, -0.833)->timestamp;
                    ESP_LOGD("setTimestamps", "------ timestamp timer (sunset)) %lu ------", timestamp);
                }
                // check if negative offset [position 9]
                if(id(global_timer)[i][9] == 1) {
                    // deduct offset hour and minutes [position 14-15] from timestamp
                    timestamp -= id(global_timer)[i][14] * 60 * 60;
                    timestamp -= id(global_timer)[i][15] * 60;
                } else {
                    // add offset hour and minutes [position 14-15] to timestamp
                    timestamp += id(global_timer)[i][14] * 60 * 60;
                    timestamp += id(global_timer)[i][15] * 60;
                }
                // set seconds to 0
                timestamp -= timestamp % 60;
                ESP_LOGD("setTimestamps", "------ timestamp after offset %lu ------", timestamp);
                // set to proper timestamp
                id(global_next_run)[i] = timestamp;
            } else {
                // timer not avalible for today set to 0
                id(global_next_run)[i] = 0;
                // ESP_LOGD("setTimestamps", "------ Active Timer %i is not set to run today ------", i + 1);
            }
        } else {       
            // timer disabled set to 0
            id(global_next_run)[i] = 0;
            // ESP_LOGD("setTimestamps", "------ Inactive Timer %i ------", i + 1);
        }
    }
} // setTimestamps

void onInterval() {
    ESP_LOGD("interval", "------ Ran interval ------");
    if(id(override_timer).state == true) {
        return;
    }
    // set variables
    esphome::ESPTime date = id(sntp_time).now();
    date.timestamp -= date.timestamp % 60;   // set seconds to 0
    bool is_missed_timer = false;
    time_t global_missed_timers[20];
    // loop all 20 timer timestamps
    for (uint8_t i = 0; i < 20; i++) {
        if(id(global_next_run)[i] > 0) {   // check if timer is active
            // ESP_LOGD("interval", "------ Timer %i Active ------", i + 1);
            // ESP_LOGD("interval", "------ Current Time %lu Next Run Time %lu Last Run Time %lu ------", date.timestamp, id(global_next_run)[i], id(global_last_run)[i]);
            if(id(global_next_run)[i] == date.timestamp) { // check if time matche
                ESP_LOGD("interval", "------ Timer %i matches current time ------", i + 1);
                doRelayAction(i, date.timestamp, true);
            }
        } else if(id(global_next_run)[i] > 0 && id(global_next_run)[i] < date.timestamp && id(global_last_run)[i] < (date.timestamp - 86340)) {
            // timer time is before current time and the latest time ran is more the 23h59m ago
            ESP_LOGD("interval", "------ Timer %i not matched - Missed timer ------", i + 1);
            // add timestamp to list of missed timers
            global_missed_timers[i] = id(global_next_run)[i];
            is_missed_timer = true;
        } else {
            // ESP_LOGD("interval", "------ Timer %i not active ------", i + 1);
            // time not active
            global_missed_timers[i] = 0;

        }
    }
    if(is_missed_timer == true) {
        ESP_LOGD("interval", "------ is_missed_timer is true ------");
        // set variables
        bool temp_relays[num_of_relays];
        uint8_t relay_index;
        uint8_t index[20] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19};
        // sort array to earliest to latest got from https://stackoverflow.com/questions/46382252/sort-array-by-first-item-in-subarray-c
        std::sort(index, index + 20, [&](uint8_t n1, uint8_t n2){ return global_missed_timers[n1] < global_missed_timers[n2]; });
        // set the current values of the relays
        for (uint8_t i = 0; i < num_of_relays; i++) {
            temp_relays[i] = (*relays[i])->state;
            // ESP_LOGD("interval", "------ Set default state for relay %i ------", i);
        }
        // set the tem relays with the result of the timer sequence
        for (uint8_t i = 0; i < 20; i++) {
            // ESP_LOGD("interval", "------ loop index %i ------", index[i]);
            // use the sorted index location so we do the actions in the order the timer was set for them
            if(global_missed_timers[index[i]] > 0) {
                // do action without really setting relays
                doRelayAction(index[i], date.timestamp, false);
                ESP_LOGD("interval", "------ index %i is set do action ------", index[i]);
                // set relay index based on output from current timer
                relay_index = id(global_timer)[index[i]][12];
                if(id(global_timer)[index[i]][13] == 2) {
                    // toggle
                    if(temp_relays[relay_index] == 0) {
                        temp_relays[relay_index] = 1;
                    } else {
                        temp_relays[relay_index] = 0;
                    }
                } else {
                    // set state
                    temp_relays[relay_index] = id(global_timer)[index[i]][13];
                }
            }
        }
        // loop relays and set state
        for (uint8_t i = 0; i < num_of_relays; i++) {
            ESP_LOGD("interval", "------ Set relay %i to state %i ------", i, temp_relays[i]);
            if((*relays[i])->state != temp_relays[i]) {
                //(*relays[i])->publish_state(temp_relays[i]);
                if(temp_relays[i] == 1){
                    ESP_LOGD("interval", "------ Set missed timer--heater on");
                    (*relays[i])->turn_on();
                } else {
                    ESP_LOGD("interval", "------ Set missed timer--heater off");
                    (*relays[i])->turn_off();
                }
            }
        }
        
    }
} // onInterval

void onSelect(std::string x) {
    ESP_LOGD("onSelect", "------ Ran onSelect ------");
    // ESP_LOGD("onSelect", "%s", x.c_str());
    uint8_t num_timer = 0;
    if(x == "-- Select --") {
        num_timer = 0;
    } else if(x == "Timer 1 HeaterOn") {
        num_timer = 1;
    } else if(x == "Timer 2 HeaterOff") {
        num_timer = 2;
    } else if(x == "Timer 3 HeaterOn") {
        num_timer = 3;
    } else if(x == "Timer 4 HeaterOff") {
        num_timer = 4;
    } else if(x == "Timer 5 HeaterOn") {
        num_timer = 5;
    } else if(x == "Timer 6 HeaterOff") {
        num_timer = 6;
    } else if(x == "Timer 7") {
        num_timer = 7;
    } else if(x == "Timer 8") {
        num_timer = 8;
    } else if(x == "Timer 9") {
        num_timer = 9;
    } else if(x == "Timer 10") {
        num_timer = 10;
    } else if(x == "Timer 11") {
        num_timer = 11;
    } else if(x == "Timer 12") {
        num_timer = 12;
    } else if(x == "Timer 13") {
        num_timer = 13;
    } else if(x == "Timer 14") {
        num_timer = 14;
    } else if(x == "Timer 15") {
        num_timer = 15;
    } else if(x == "Timer 16") {
        num_timer = 16;
    } else if(x == "Timer 17") {
        num_timer = 17;
    } else if(x == "Timer 18") {
        num_timer = 18;
    } else if(x == "Timer 19") {
        num_timer = 19;
    } else if(x == "Timer 20") {
        num_timer = 20;
    }
    ESP_LOGD("onSelect", "------ Timer %i Selected ------", num_timer);
    // ESP_LOGD("onSelect", "------ Timer %i last run at %lu next run at %lu ------", num_timer, id(global_last_run)[num_timer - 1], id(global_next_run)[num_timer - 1]);
    if(num_timer > 0) {
        // set num_timer to be base on position starting from 0
        num_timer -= 1;
        // set all the switches states
        for (uint8_t i = 0; i < 10; i++) {
            (*switches[i])->publish_state(id(global_timer)[num_timer][i]);
            // ESP_LOGD("onSelect", "------ Loop # %i value %i (switch) ------", i, id(global_timer)[num_timer][i]);
        }
        // set all the Number states
        for (uint8_t i = 0; i < 7; i++) {
            (*numbers[i])->publish_state(id(global_timer)[num_timer][10 + i]);
            // ESP_LOGD("onSelect", "------ Loop # %i value %i (number) ------", 10 + i, id(global_timer)[num_timer][10 + i]);
        }
    } else {
        // no timer set all switches off
        for (uint8_t i = 0; i < 10; i++) {
            (*switches[i])->publish_state(false);
        }
        // no timer set all numbers to 0
        for (uint8_t i = 0; i < 7; i++) {
            (*numbers[i])->publish_state(0);
        }
    }
} // onSelect

void onPressSave() {
    ESP_LOGD("onPressSave", "------ Save Button Pressed ------");
    uint8_t num_timer = 0;
    if(id(select_timer).state == "Timer 1 HeaterOn") {
        num_timer = 1;
    } else if(id(select_timer).state == "Timer 2 HeaterOff") {
        num_timer = 2;
    } else if(id(select_timer).state == "Timer 3 HeaterOn") {
        num_timer = 3;
    } else if(id(select_timer).state == "Timer 4 HeaterOff") {
        num_timer = 4;
    } else if(id(select_timer).state == "Timer 5 HeaterOn") {
        num_timer = 5;
    } else if(id(select_timer).state == "Timer 6 HeaterOff") {
        num_timer = 6;
    } else if(id(select_timer).state == "Timer 7") {
        num_timer = 7;
    } else if(id(select_timer).state == "Timer 8") {
        num_timer = 8;
    } else if(id(select_timer).state == "Timer 9") {
        num_timer = 9;
    } else if(id(select_timer).state == "Timer 10") {
        num_timer = 10;
    } else if(id(select_timer).state == "Timer 11") {
        num_timer = 11;
    } else if(id(select_timer).state == "Timer 12") {
        num_timer = 12;
    } else if(id(select_timer).state == "Timer 13") {
        num_timer = 13;
    } else if(id(select_timer).state == "Timer 14") {
        num_timer = 14;
    } else if(id(select_timer).state == "Timer 15") {
        num_timer = 15;
    } else if(id(select_timer).state == "Timer 16") {
        num_timer = 16;
    } else if(id(select_timer).state == "Timer 17") {
        num_timer = 17;
    } else if(id(select_timer).state == "Timer 18") {
        num_timer = 18;
    } else if(id(select_timer).state == "Timer 19") {
        num_timer = 19;
    } else if(id(select_timer).state == "Timer 20") {
        num_timer = 20;
    }
    if(num_timer != 0) {
        // set num_timer to be base on position starting from 0
        num_timer -= 1;
        // set the 10 switches states
        for (uint8_t i = 0; i < 10; i++) {
            id(global_timer)[num_timer][i] = (*switches[i])->state;
               //ESP_LOGD("onPressSave", "------ save Loop Number %i value %i (switch) ------", i, id(global_timer)[num_timer][i]);
        }
        // set the 7 number states (in the global array we start at 10)
        for (uint8_t i = 0; i < 7; i++) {
            id(global_timer)[num_timer][10 + i] = (*numbers[i])->state;
              //ESP_LOGD("onPressSave", "------ save Loop Number %i value %d (number) ------", 10 + i, (*numbers[i])->state);
        }
    }
    // timer settings changed reset timestamps
    setTimestamps(num_timer);
} // onPressSave

void onPressgetglobalnextrun(){
    uint8_t i = 0;
    uint8_t loops = 20;

        ESP_LOGD("onPressgetglobalnextrun", "get all timers ------");
        ESP_LOGD("onPressgetglobalnextrun", "manual next run Timer 1 --- %lu" , id(global_next_run)[0]);
        ESP_LOGD("onPressgetglobalnextrun", "manual next run Timer 2 --- %lu" , id(global_next_run)[1]);

    for(; i < loops; i++) {

        ESP_LOGD("onPressgetglobalnextrun", "global_timer %i ---%i:hour %i:minute", i + 1 , id(global_timer)[i][10], id(global_timer)[i][11]);
        ESP_LOGD("onPressgetglobalnextrun", "next run Timer  --- %lu", id(global_next_run)[i]);
    }
}//onPressgetglobalnetxrun
 // add by zhou start 
void antifreezeSolar(){
    id(flag_count_antifreezSolar) = false;
    ESP_LOGD("antifreezeSolar","进入防冻太阳板程序");
    if(id(t4_tank_bottom).state < antifreeztank_temp){
        if(id(relay_0_heater).state == false){
            ESP_LOGD("antifreezeSolar","电加热器开关被太阳能板防冻函数打开");
            id(relay_0_heater).turn_on();
        }else if(id(relay_0_heater).state == true){
            ESP_LOGD("antifreezeSolar","电加热器开关已经被打开");  
          //  id(relay_0_heater).turn_on(); 
        }
        else{
            ESP_LOGD("antifreezeSolar","警告：电加热器开关不可用，太阳能板抗冻可能失败");
        }     
        }
    ESP_LOGD("antifreezeSolar","打开太阳能循环泵抗冻");   
    id(pump1).turn_on();
    if(id(t1_solar).state > antifreezstop_temp){
        ESP_LOGD("antifreezeSolar","antifreezeSolar","关闭太阳能循环泵");
        id(pump1).turn_off();
     //水箱温度这么低，索性将水箱加热至目标标温度，虽然浪费能源但确保防冻！
     //为安全起见，如果水箱达到目标温度，在这儿关闭一下电加热。
        if(id(t2_tank_top).state > id(target_temp).state){
            ESP_LOGD("antifreezeSolar","关闭电加热开关");
            id(relay_0_heater).turn_off();
        }

        id(flag_count_antifreezSolar) = false;
        ESP_LOGD("antifreezeSolar","太阳能板抗冻程序结束");
    }else{
        ESP_LOGD("antifreezeSolar","需再入太阳能板抗冻程序");
        id(flag_count_antifreezSolar) = true;
    }
}
        
void antifreezePipe(){
    id(flag_count_antifreezPipe) = false;
    ESP_LOGD("antifreezePipe","进入防冻管道程序");
    if(id(t4_tank_bottom).state < antifreeztank_temp){
        if(id(relay_0_heater).state == false){
            ESP_LOGD("antifreezePipe","电加热器开关被管道防冻函数打开");
            id(relay_0_heater).turn_on();
        }else if(id(relay_0_heater).state == true){
            ESP_LOGD("antifreezePipe","电加热器开关已经被打开"); 
           // id(relay_0_heater).turn_on(); 
        }
        else{
            ESP_LOGD("antifreezePipe","警告：电加热器开关不可用，管道抗冻可能失败");
        }     
        }
    ESP_LOGD("antifreezePipe","打开管道循环泵抗冻");   
    id(pump2).turn_on();
    /** 回水管道和太阳能板管道处的水泵位置失温最大，回水管道T3温度传感器紧临水泵，
    但太阳能板循环管道水泵处没有温度传感器，推测如果回水管道需要防冻，则太阳能板管道此时也需要防冻 ***/
    ESP_LOGD("antifreezePipe","也打开太阳能板循环泵抗冻");
    id(pump1).turn_on();
    if(id(t3_pipe).state > antifreezstop_temp){
        ESP_LOGD("antifreezePipe","关闭管道循环泵");
        id(pump2).turn_off();
        ESP_LOGD("antifreezePipe","关闭太阳能板管道循环泵");
        id(pump1).turn_off();
     //水箱温度这么低，索性将水箱加热至目标标温度，虽然浪费能源但确保防冻！
     //为安全起见，如果水箱达到目标温度，在这儿关闭一下电加热。
        if(id(t2_tank_top).state > id(target_temp).state){
            ESP_LOGD("antifreezeSolar","关闭电加热开关");
            id(relay_0_heater).turn_off();
        }

        id(flag_count_antifreezPipe) = false;
        ESP_LOGD("antifreezePipe","管道抗冻程序结束");
    }else{
        ESP_LOGD("antifreezePipe","需再入管道抗冻程序");
        id(flag_count_antifreezPipe) = true;
    }
}
        
void protectTank(){
    ESP_LOGD("protectTank","水箱高温保护，关闭电加热器");
    id(relay_0_heater).turn_off();
    ESP_LOGD("protectTank","水箱高温保护，关闭太阳能板循环泵");
    id(pump1).turn_off();
    
}



void checkSwitchOnDurationheater(time_t maxdurationtime) {
    struct tm tm ;
    time_t timestamp1 = 0;
    time_t timestamp2 = 0;
    time_t timestamp3 = 0;


    if(id(relay_0_heater).state){
        esphome::ESPTime date1 =id(sntp_time).now(); 
        tm = date1.to_c_tm();
        timestamp1 = mktime(&tm);

        timestamp2 = id(global_heateron);
        if(((timestamp1-timestamp2) >= maxdurationtime)&&(timestamp2 != timestamp3)){
            ESP_LOGD("checkSwitchOnDuration","电加热开关打开已持续:%i分钟",maxdurationtime/60);
            ESP_LOGD("checkSwitchOnDuration","为了保证安全,电加热开关已自动关闭！");
            id(relay_0_heater).turn_off();

        }
    }
}

void checkSwitchOnDurationpump1(time_t maxdurationtime) {
    struct tm tm ;
    time_t timestamp1 = 0;
    time_t timestamp2 = 0;
    time_t timestamp3 = 0;
    if(id(pump1).state){
        esphome::ESPTime date1 =id(sntp_time).now(); 
        tm = date1.to_c_tm();
        timestamp1 = mktime(&tm);

        timestamp2 = id(global_pump1on);
        if(((timestamp1-timestamp2) >= maxdurationtime)&&(timestamp2 != timestamp3)){
            ESP_LOGD("checkSwitchOnDurationpump1","太阳能循环泵工作已打持续:%i分钟",maxdurationtime/60);
            ESP_LOGD("checkSwitchOnDurationpump1","为了保证安全,太阳能循环泵开关已自动关闭！");
            id(pump1).turn_off();

        }
    }

}

void checkSwitchOnDurationpump2(time_t maxdurationtime) {
    struct tm tm ;
    time_t timestamp1 = 0;
    time_t timestamp2 = 0;
    time_t timestamp3 = 0;
    if(id(pump2).state){
        esphome::ESPTime date1 =id(sntp_time).now(); 
        tm = date1.to_c_tm();
        timestamp1 = mktime(&tm);

        timestamp2 = id(global_pump2on);
        if(((timestamp1-timestamp2) >= maxdurationtime)&&(timestamp2 != timestamp3)){
            ESP_LOGD("checkSwitchOnDurationpump2","管道循环泵工作已持续:%i分钟",maxdurationtime/60);
            ESP_LOGD("checkSwitchOnDurationpump2","为了保证安全,管道循环泵已自动关闭！");
            id(pump2).turn_off();

        }
    }

}



void mainonInterval(){
    //time_t t = 60;
    ESP_LOGD("mainonInterval", "------ Ran mainonInerval ------");
    if((id(t1_solar).state <= antifreez_temp)||(id(flag_count_antifreezSolar))){
        
        antifreezeSolar(); //调用太阳能板防冻函数
    }
    if((id(t3_pipe).state <= antifreez_temp)||(id(flag_count_antifreezPipe))){

        antifreezePipe(); //调用循环管道防冻函数
    }
    checkSwitchOnDurationheater(18000);//检查电加器加热是否超过5小时，超过即关掉。
    checkSwitchOnDurationpump1(1200); //检查太阳能循环泵工作是否超过20分钟，超过即关掉。
    checkSwitchOnDurationpump2(1200); //检查热水管道循环泵工作是否超过20分钟，超过即关掉。
    if(id(t2_tank_top).state >= protect_temp){

        protectTank();  //调用水箱高温保护函数
        return;
    }else{
        //如果t4_tank_bottom失效，但t1_solar,t2_tank_top可用，也能打开太阳板循环
        if((((id(t1_solar).state - id(t4_tank_bottom).state)>= high_deltasolartanktop_temp)&&
            (id(id(t2_tank_top).state <= protect_temp)))
            ||(((id(t1_solar).state - id(t2_tank_top).state)>= high_deltasolartanktop_temp)&&
            (id(id(t2_tank_top).state <= protect_temp)))){
            ESP_LOGD("mainonInterval","打开太阳能板循环泵换热");   
            id(pump1).turn_on();
         //   id(pump1).publish_state(true);    
        }

 
        // 回水管道防冻过程没有完成时不要关闭太阳能板循环泵
        if(((id(t1_solar).state - id(t4_tank_bottom).state)< low_deltasolartanktop_temp)
            &&(id(t1_solar).state > antifreezstop_temp)&&(!id(flag_count_antifreezPipe))){
            ESP_LOGD("mainonInterval","太阳能板循环泵关闭,完成换热");   
            id(pump1).turn_off();
        //    id(pump1).publish_state(false);       
        }
        if(id(holiday_mode).state){
            if(id(t2_tank_top).state > id(target_temp).state){
                ESP_LOGD("mainonInterval","度假中,水箱已到目标温度,关掉电加热器"); 
                id(relay_0_heater).turn_off();
                } 
            ESP_LOGD("mainonInterval","度假中,跳出mainInterval一次");    
            return;

        }else{
            if(id(t2_tank_top).state <= id(target_temp).state){
                ESP_LOGD("mainonInterval","检查并执行定时加热"); 
                onInterval(); //
            }else{
                 //防止t2_tank_top温度值无效时关闭电加热
                if(id(t2_tank_top).state > id(target_temp).state){
                    ESP_LOGD("mainonInterval","水箱已到目标温度,关掉电加热器,跳出mainInterval一次"); 
                    id(relay_0_heater).turn_off();
                    return;
                } 

            }
        
        }
    }

}



