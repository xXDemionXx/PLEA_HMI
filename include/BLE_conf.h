
#define BLE_SERVER_NAME      "PLEA HMI"     // name of server that client will see

#define BLE_CHUNK_SIZE              20      // how many char can be sent over BLE at once


#define NETWORK_SERVICE_UUID                "9fd1e9cf-97f7-4b0b-9c90-caac19dba4f8"
#define NETWORK_NAMES_CH_UUID               "0fd59f95-2c93-4bf8-b5f0-343e838fa302"
#define NETWORK_CONNECT_CH_UUID             "15eb77c1-6581-4144-b510-37d09f4294ed"
#define NETWORK_MESSAGE_CH_UUID             "773f99ff-4d87-4fe4-81ff-190ee1a6c916"
#define NETWORK_COMMANDS_CH_UUID            "68b82c8a-28ce-43c3-a6d2-509c71569c44"



#define SEARCH_NETWORKS_COMMAND     's'     // command for searching networks   's'
#define REQUEST_IP_COMMAND          'p'     // command for getting IP           'p'
#define DISCONNECT_NETWORK_COMMAND  'd'     // command for disconnecting        'd'



#define NETWORK_CON_MESSAGE         'C'     // network connected status         'C'
#define NETWORK_DISCON_MESSAGE      'D'     // network disconnected             'D'
#define NETWORK_ERROR_MESSAGE       'E'     // message for errors               'E'