// HEADERS //

#include <lvgl.h>
#include <gfx_conf.h>
#include <unordered_map>
#include <vector>
#include <sstream>
#include <regex>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// custom
#include <hardware.h>
#include <UI_parameters.h>
#include <BLE_conf.h>

////////////////////////////////////////////////////////////////////////////////////////////

// TASK HANDLES //
TaskHandle_t LVGL_handler_task;

// VARIABLES //

// Network
std::vector<std::string> networkNames;      // names of networks WiFi or Ethernet
std::string network_names_string;

//Network flags
bool networks_string_received = false;
bool network_string_completed = false;       // must be 0 at the start

// Struct that holds network information
struct Network {
    std::string name;       //
    std::string type;       // "WiFi" or "Ethernet"
    std::string password;   //
    byte table_entry_nmb;   // 
};
std::vector<Network> networks;

std::string test_network_names = "W:<<Joe_Biden>>E:<<Donald_Trump>>W:<<Barack OBAMA>>W:<<Hillary Clinton>>E:<<gogo>>W:<<Zelinski>>E:<<Sheisty>>E:<<KOKO>>W:<<AMERICA>>E:<<LOLO>>W:<<League>>E:<<Off>>W:<<Legends>>#";


// LVGL VARIABLES //

static lv_disp_draw_buf_t draw_buf;
static lv_color_t disp_draw_buf1[screenWidth * screenHeight / 8];
//static lv_color_t disp_draw_buf2[screenWidth * screenHeight / 8];     // to much RAM usage
static lv_disp_drv_t disp_drv;

// screens
lv_obj_t *default_screen;    // create root parent screen

// widgets

lv_obj_t* main_tabview;
lv_obj_t* connection_status_label;
lv_obj_t* connection_table;

// styles


////////////////////////////////////////////////////////////////////////////////////////////

// BLE //

// variables

BLEServer* BLE_HMI_server = NULL;
BLEService* BLE_network_service = NULL;
BLECharacteristic* BLE_network_names_ch = NULL;
BLEAdvertising* BLE_advertising = NULL;
bool BLEdeviceConnected = false;
bool BLEolddeviceConnected = false;


////////////////////////////////////////////////////////////////////////////////////////////

// FUNCTION DECLARATIONS //

// gfx functions
void touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data);
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p);

// UI init functions
void init_tabs();
void init_connection_tab();
void init_macros_tab();
void init_settings_tab();
void init_PLEA_settings_tab();

// LVGL task
void LVGL_handler_function(void * pvParameters);

// network name operations
void put_network_names_in_table(const std::vector<Network>& networkss);
//void BLE_assemble_networks_string();

// BLE functions
void init_BLE();
void init_BLE_network_service();
void BLE_string_from_chunks(std::string chunk, std::string* storage_string, bool* completed_message_indicator);
void BLE_network_names_from_string(std::string networks_string, std::vector<Network>& networks);


////////////////////////////////////////////////////////////////////////////////////////////

// connect/disconnect callback class
class ServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* BLE_HMI_server) {
        // what to do on connection
        BLEdeviceConnected = true;
        Serial.println("Device connected");
    };

    void onDisconnect(BLEServer* BLE_HMI_server) {
        // what to do on disconnection
        BLEdeviceConnected = false;
        Serial.println("Device disconnected");
        BLEDevice::startAdvertising();  // wait for another connection
    }
};

// class that deals with receiving from client
class recieveNetworkNamesCallback: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *receiver_characteristic) {
        std::string BLE_received_string = receiver_characteristic->getValue();
        /*
        if (BLE_received_string.length() > 0) {                     //
            Serial.println("*********");                            //
            Serial.print("Recieved string: ");                      //
        for (int i = 0; i < BLE_received_string.length(); i++)      //  troubleshooting block
            Serial.print(BLE_received_string[i]);                   //
        Serial.println();                                           //
        Serial.println("*********");                                //
        }
        */
        BLE_string_from_chunks(BLE_received_string, &network_names_string, &network_string_completed);
        if(network_string_completed){
            //BLE_network_names_from_string(network_names_string, networks);
            //networks_string_chop_and_assign(network_names_string);
            networks_string_received = true;
            network_string_completed = false;
        }
    }
};

////////////////////////////////////////////////////////////////////////////////////////////

// LVGL SETUP FUNCTIONS AND DRIVERS //
void lvgl_setup()
{
    Serial.begin(115200);

    // gfx setup
    pinMode(BL_PIN, OUTPUT);        // backlight initialization
    digitalWrite(BL_PIN, 1);        // 
    gfx.begin();                    // start the LovyanGFX
    gfx.fillScreen(TFT_BLACK);      
    delay(200);
    //
    lv_init();                      // LVGL initialize
    delay(100);

    // display setup
    lv_disp_draw_buf_init(&draw_buf, disp_draw_buf1, NULL, screenWidth * screenHeight / 8);
    lv_disp_drv_init(&disp_drv);    // display driver
    disp_drv.hor_res = screenWidth;
    disp_drv.ver_res = screenHeight;
    disp_drv.flush_cb = my_disp_flush;  // flush function
    disp_drv.full_refresh = 1;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    // touchscreen setup
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touchpad_read; // touchpad reed function
    lv_indev_drv_register(&indev_drv);

    // default screen

    default_screen = lv_scr_act();

    // SANITY TEST //
    /*
    screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x003a57), LV_PART_MAIN);
    lv_obj_t *label = lv_label_create(screen);
    lv_label_set_text(label, "Hello world");
    lv_obj_set_style_text_color(label, lv_color_hex(0xd61a1a), LV_PART_MAIN);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    lv_scr_load(screen);
    delay(3000);
    */
    //
}

void touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{
    // This function gives LVGL the touch cordinates
    uint16_t touchX, touchY;
    bool touched = gfx.getTouch(&touchX, &touchY);
    if (!touched)
    {
        data->state = LV_INDEV_STATE_REL;
    }
    else
    {
        // set the coordinates
        data->state = LV_INDEV_STATE_PR;
        data->point.x = touchX;
        data->point.y = touchY;
    }
}

void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    // This function is used by LVGL to push graphics to the screen
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    gfx.pushImageDMA(area->x1, area->y1, w, h, (lgfx::rgb565_t *)&color_p->full);
    lv_disp_flush_ready(disp);
}

////////////////////////////////////////////////////////////////////////////////////////////

void setup()
{
    // Create a task to handle the LVGL updates
    xTaskCreatePinnedToCore(
                    LVGL_handler_function,      /* Task function. */
                    "LVGL_handler_task",        /* name of task. */
                    10000,                      /* Stack size of task */
                    NULL,                       /* parameter of the task */
                    1,                          /* priority of the task */
                    &LVGL_handler_task,         /* Task handle to keep track of created task */
                    0);                         /* pin task to core 0 */                  
    delay(500); 
    // Setup functions //
    lvgl_setup();
    init_tabs();
    init_BLE();
    //
}

void loop()
{
    if(BLEdeviceConnected == true){     // If we are connected
        // Check if we received a network_names_string
        if(networks_string_received == true){
            networks.clear();
            BLE_network_names_from_string(network_names_string, networks);
            put_network_names_in_table(networks);
            network_names_string = "";
            network_string_completed = false;
            networks_string_received = false;
        }
    }
}

void LVGL_handler_function(void * pvParameters){
    /*
    *   This task handles the LVGL timer as to
    *   free up the main loop
    */
    for(;;) {
        // Timer handler //
        lv_timer_handler();     // don't touch this
        delay(5);               //
        //
    }
}

////////////////////////////////////////////////////////////////////////////////////////////

// UI INIT FUNCTIONS //

void init_tabs(){
    main_tabview = lv_tabview_create(default_screen, LV_DIR_TOP, MAIN_TABVIEW_HEIGHT);      //create main tabview
    init_connection_tab();
    init_macros_tab();
    init_PLEA_settings_tab();
    init_settings_tab();
}

void init_connection_tab(){
    /*
    *   This tab deals with connecting wifi/ethernet to PLEA
    */
    lv_obj_t *connection_tab = lv_tabview_add_tab(main_tabview, "Connection");              //create main tabview
    lv_obj_set_flex_flow(connection_tab, LV_FLEX_FLOW_ROW);

    lv_obj_t *connection_table_backdrop = lv_obj_create(connection_tab);
    lv_obj_t *connection_buttons_backdrop = lv_obj_create(connection_tab);
    lv_obj_set_height(connection_table_backdrop, CONNECTION_TABLE_BACKDROP_HEIGHT);
    lv_obj_set_height(connection_buttons_backdrop, CONNECTION_BUTTONS_BACKDROP_HEIGHT);
    lv_obj_set_flex_flow(connection_table_backdrop, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_flow(connection_buttons_backdrop, LV_FLEX_FLOW_COLUMN);

    lv_obj_set_flex_grow(connection_table_backdrop, 1);             // set the ratio of table
    lv_obj_set_flex_grow(connection_buttons_backdrop, 1);           // to buttons

    // Reduce padding and remove radius
    static lv_style_t no_padding_style;
    lv_style_init(&no_padding_style);
    lv_style_set_radius(&no_padding_style, 0);
    lv_style_set_pad_all(&no_padding_style, 0);
    lv_style_set_pad_gap(&no_padding_style, 0);

    lv_obj_add_style(connection_tab, &no_padding_style, 0);
    lv_obj_add_style(connection_table_backdrop, &no_padding_style, 0);

    connection_table = lv_table_create(connection_table_backdrop);
    lv_obj_set_size(connection_table, LV_PCT(100), LV_PCT(100));
    lv_table_set_col_cnt(connection_table, 3);
    
    lv_table_set_col_width(connection_table, 0, 80);
    lv_table_set_col_width(connection_table, 2, 80);
    lv_coord_t table_width = lv_obj_get_width(connection_table);
    lv_table_set_col_width(connection_table, 1, 220);

    /*
    lv_coord_t table_width = lv_obj_get_width(connection_table);
    lv_table_set_col_width(connection_table, 0, table_width * 0.15); // 30% width
    lv_table_set_col_width(connection_table, 1, table_width * 0.7); // 40% width
    lv_table_set_col_width(connection_table, 2, table_width * 0.15); // 30% width
    */

    lv_obj_set_flex_flow(connection_table, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_grow(connection_table, 1);

    lv_obj_set_flex_flow(connection_buttons_backdrop, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(connection_buttons_backdrop, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);  // how to align backdrop
    //lv_style_t *connection_buttons_backdrop_style;
    //lv_style_set_pad_gap


    // Create a style for buttons
    static lv_style_t connect_btn_style;
    lv_style_init(&connect_btn_style);
    lv_style_set_width(&connect_btn_style, LV_PCT(100));
    lv_style_set_flex_grow(&connect_btn_style, 1);
    lv_style_set_radius(&connect_btn_style, 0);
    lv_style_set_border_color(&connect_btn_style, lv_color_hex(0x2f3436));
    lv_style_set_text_align(&connect_btn_style, LV_TEXT_ALIGN_CENTER);

    // Add connection indicator object
    lv_obj_t *con_indicator_backdrop = lv_obj_create(connection_buttons_backdrop);
    lv_obj_add_style(con_indicator_backdrop, &connect_btn_style, LV_PART_MAIN);
    lv_obj_set_flex_grow(con_indicator_backdrop, 2);
    connection_status_label = lv_label_create(con_indicator_backdrop);
    lv_label_set_text(connection_status_label, "Disconnected");
    lv_obj_center(connection_status_label);

    // Add connect/disconnect button - it can connect or disconnect
    lv_obj_t *connect_btn = lv_btn_create(connection_buttons_backdrop);
    lv_obj_t *connect_btn_label = lv_label_create(connect_btn);
    lv_label_set_text(connect_btn_label, "Connect");
    lv_obj_add_style(connect_btn, &connect_btn_style, 0);
    lv_obj_add_style(connect_btn_label, &connect_btn_style, 0);

    // Add search connections button
    lv_obj_t *srch_connections_btn = lv_btn_create(connection_buttons_backdrop);
    lv_obj_t *srch_connections_btn_label = lv_label_create(srch_connections_btn);
    lv_label_set_text(srch_connections_btn_label, "Search connections");
    lv_obj_add_style(srch_connections_btn, &connect_btn_style, 0);
    lv_obj_add_style(srch_connections_btn_label, &connect_btn_style, 0);

    // Add password button
    lv_obj_t *passwor_btn = lv_btn_create(connection_buttons_backdrop);
    lv_obj_t *passwor_btn_label = lv_label_create(passwor_btn);
    lv_label_set_text(passwor_btn_label, "Password");
    lv_obj_add_style(passwor_btn, &connect_btn_style, 0);
    lv_obj_add_style(passwor_btn_label, &connect_btn_style, 0);

    // Add empty memory button
    lv_obj_t *empty_btn = lv_btn_create(connection_buttons_backdrop);
    lv_obj_t *empty_btn_label = lv_label_create(empty_btn);
    lv_label_set_text(empty_btn_label, "Empty");
    lv_obj_add_style(empty_btn, &connect_btn_style, 0);
    lv_obj_add_style(empty_btn_label, &connect_btn_style, 0);

    // Add IP button
    lv_obj_t *IP_btn = lv_btn_create(connection_buttons_backdrop);
    lv_obj_t *IP_btn_label = lv_label_create(IP_btn);
    lv_label_set_text(IP_btn_label, "IP");
    lv_obj_add_style(IP_btn, &connect_btn_style, 0);
    lv_obj_add_style(IP_btn_label, &connect_btn_style, 0);

    /*
    lv_obj_set_flex_grow(btn1, 1);
    lv_obj_set_flex_grow(btn2, 1);
    */
}

void init_macros_tab(){
    /*
    *   This tab deals with quick macro buttons
    */
    lv_obj_t *macros_tab = lv_tabview_add_tab(main_tabview, "Macros");
    lv_obj_t *btn = lv_btn_create(macros_tab);
   
}

void init_settings_tab(){
    /*
    *   This tab deals with settings of the HMI
    */
    lv_obj_t *settings_tab = lv_tabview_add_tab(main_tabview, "Settings");

}

void init_PLEA_settings_tab(){
    /*
    *   This tab deals with PLEA settings
    */
    lv_obj_t *PLEA_settings_tab = lv_tabview_add_tab(main_tabview, "PLEA settings");
}

// UI FUNCTIONS //

void put_network_names_in_table(const std::vector<Network>& networks){
    byte number_of_networks = networks.size();
    lv_table_set_row_cnt(connection_table, number_of_networks);  // number of rows = number of networks
    for (byte i = 0; i < number_of_networks; i++) {
        lv_table_set_cell_value(connection_table, i, 0, networks[i].type.c_str());
        lv_table_set_cell_value(connection_table, i, 1, networks[i].name.c_str());
        lv_table_set_cell_value(connection_table, i, 2, std::to_string(networks[i].table_entry_nmb).c_str());
        Serial.print(networks[i].name.c_str());  // troubleshooting line
        Serial.print(" entry: ");
        Serial.println(networks[i].table_entry_nmb);
    }
}

// NETWORKS STRING FUNCTIONS //

void BLE_network_names_from_string(std::string networks_string, std::vector<Network>& networks){
    /*
    *   Chops the networks_string into individual networks
    * 
    *   Function searches string chunks that start with W:
    *   or E: and the name of the network that is between << and >>
    *   '#' marks the end of the string
    * 
    *   Legend:
    *   W:  WiFi network
    *   E:  Ethernet network
    *   <<  Start of a network's name
    *   >>  End of a network's name
    * 
    *   Example (WiFi network with a name net123):
    *   W:<<net123>>
    */

    // Regular expression to extract network names and types
    std::regex re(R"((W:|E:)<<([^>]+)>>)");
    std::smatch match;
    std::string::const_iterator searchStart(networks_string.cbegin());

    // Extract network names and their types
    byte i=0;
    while (std::regex_search(searchStart, networks_string.cend(), match, re)) {
        std::string type = match[1] == "W:" ? "WiFi" : "Eth";
        std::string name = match[2];
        networks.push_back({name, type, "", i});  // Initially, the password is empty
        searchStart = match.suffix().first;
        i++;
    }

    /*
    for (const auto& network : networks) {          //
        Serial.print("Network Name: ");             //
        Serial.print(network.name.c_str());         //
        Serial.print(", Type: ");                   //  Troubleshooting block
        Serial.print(network.type.c_str());         //
        Serial.print(", Password: ");               //
        Serial.println(network.password.c_str());   //
    }
    */
}

// BLE FUNCTIONS //

void init_BLE(){

    // Create the BLE Device
    BLEDevice::init("PLEA HMI");

    // Create the BLE Server
    BLE_HMI_server = BLEDevice::createServer();
    BLE_HMI_server->setCallbacks(new ServerCallbacks());            // Set callback on connect and disconnect

    BLE_advertising = BLEDevice::getAdvertising();  // Create advertising

    // Initialize services
    init_BLE_network_service();

    // Start advertising
    BLE_advertising->setScanResponse(false);
    BLE_advertising->setMinPreferred(0x0);  // Set value to 0x00 to not advertise this parameter
    BLEDevice::startAdvertising();
    Serial.println("Waiting for a client connection...");
    //
}

void init_BLE_network_service(){
    /*
    *   This function creates the BLE_network_service
    *   and it starts it.
    */
    BLE_network_service = BLE_HMI_server->createService(NETWORK_SERVICE_UUID);    // Create BLE network service

    // Create BLE_network_names_ch characteristic
    // It is intended for receiveing network names from Raspberry PI
    BLE_network_names_ch = BLE_network_service->createCharacteristic(
                      NETWORK_NAMES_CH_UUID,
                      BLECharacteristic::PROPERTY_WRITE
                    );
    BLE_network_names_ch->setCallbacks(new recieveNetworkNamesCallback());  // Add callback on recieve from client

    BLE_advertising->addServiceUUID(NETWORK_SERVICE_UUID);      // Add BLE_network_service to advertising

    BLE_network_service -> start();    // Start the service
}

void BLE_string_from_chunks(std::string chunk, std::string* storage_string, bool* completed_message_indicator){
    /*
    *   Takes the incoming chunks of the string coming in
    *   and appends them to a string. Detects the end of a
    *   string with the '#' and changes an indicator booll 
    *   to true.
    */
    *storage_string += chunk;       // Append the chunk that was sent over BLE to
                                    // a string that will containt the whole message
    if(chunk.back() == '#' || chunk == ""){ // If the chunk ends with '#' or it is empty
        *completed_message_indicator = true;
        /*
        Serial.println("*********");                    //
        Serial.println(storage_string->c_str());        // troubleshooting block
        Serial.println();                               //
        Serial.println("Message indicator:");           //
        Serial.println(*completed_message_indicator);   //
        Serial.println("*********");                    //
        */
    }
}

