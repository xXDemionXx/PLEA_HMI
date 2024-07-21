// HEADERS //

#include <lvgl.h>
#include <hardware.h>
#include <gfx_conf.h>
#include <unordered_map>
#include <vector>
#include <sstream>
#include <regex>
#include <UI_parameters.h>

////////////////////////////////////////////////////////////////////////////////////////////

// VARIABLES //

std::vector<std::string> networkNames;      // names of networks WiFi or Ethernet
std::vector<std::string> wifiNetworkNames;  // names of WiFi networks
std::vector<std::string> ethNetworkNames;   // names of ethernet networks

std::string test_network_names = "W:<<Joe_Biden>>E:<<Donald_Trump>>W:<<Barack OBAMA>>W:<<Hillary Clinton>>E:<<gogo>>W:<<Zelinski>>E:<<Sheisty>>E:<<KOKO>>W:<<AMERICA>>E:<<LOLO>>W:<<League>>E:<<Off>>W:<<Legends>>";



// LVGL VARIABLES //

static lv_disp_draw_buf_t draw_buf;
static lv_color_t disp_draw_buf1[screenWidth * screenHeight / 8];
//static lv_color_t disp_draw_buf2[screenWidth * screenHeight / 8];     // to much RAM usage
static lv_disp_drv_t disp_drv;

// screens
lv_obj_t *default_screen;    // create root parent screen

// widgets

lv_obj_t *main_tabview;
lv_obj_t *connection_status_label;
lv_obj_t *connection_table;

// styles


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
void networks_string_chop_and_assign(std::string networks_string);
void put_network_names_in_table(std::vector<std::string> networkNames);
//void BLE_assemble_networks_string();

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
    // Setup functions //
    lvgl_setup();
    init_tabs();
    networks_string_chop_and_assign(test_network_names);
    put_network_names_in_table(networkNames);
    //
}

void loop()
{
    // Timer handler //
    lv_timer_handler();     // don't touch this
    delay(5);               //
    //
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

    /*Fill the first column*/
    connection_table = lv_table_create(connection_table_backdrop);
    //lv_table_set_row_cnt(connection_table, 3);
    lv_table_set_col_cnt(connection_table, 1);
    lv_table_set_col_width(connection_table, 0, LV_PCT(100));
    //lv_table_set_cell_value(connection_table, 0, 0, "Name");
    //lv_table_set_cell_value(connection_table, 1, 0, "Apple");
    //lv_table_set_cell_value(connection_table, 2, 0, "Bananadsdvsdvsdvsd");

    lv_obj_set_flex_flow(connection_table, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_grow(connection_table, 1);
    lv_obj_set_size(connection_table, LV_PCT(100), LV_PCT(100));

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

void put_network_names_in_table(std::vector<std::string> networkNames){
    byte number_of_networks = networkNames.size();
    lv_table_set_col_cnt(connection_table, number_of_networks );    // number of columns = number of networks
    for(byte i=0; i<number_of_networks; i++){
        lv_table_set_cell_value(connection_table, i, 0, networkNames.at(i).c_str());
        //Serial.println(networkNames.at(i).c_str());       // troubleshooting line
    } 
}

// NETWORKS STRING FUNCTIONS //

void networks_string_chop_and_assign(std::string networks_string){
    /*
    *   Chops the networks_string into individual networks
    *   and assigns them to either the wifiNetworkNames or
    *   ethNetworkNames
    * 
    *   Function searches string chunks that start with W:
    *   or E: and the name of the network that is between << and >>
    * 
    *   Legenda poruke:
    *   W:  WiFi network
    *   E:  Ethernet network
    *   <<  Start of a network's name
    *   >>  End of a network's name
    * 
    *   Example (WiFi network with a name net123):
    *   W:<<net123>>
    */

    std::regex re(R"((W:|E:)<<([^>]+)>>)");         // re - the regular expresion that we will be searching for
    std::smatch match;                              // object that will hold matches for the requirment in the last line
    std::string::const_iterator searchStart(networks_string.cbegin());  // initialize iterator at the string's beginning
    while (std::regex_search(searchStart, networks_string.cend(), match, re)) {
        std::string type = match[1];
        std::string name = match[2];
        networkNames.push_back(type + name);
        searchStart = match.suffix().first;
    }

    /*
    Serial.println("Network names:");                   //
    for(int i = 0; i < networkNames.size(); i++){       //  troubleshooting block:
        Serial.println(networkNames.at(i).c_str());     //  writes the names of networks
    }                                                   //
    */
}