/*
 * NDI Client to JACK (Jack Audio Connection Kit) Output
 * 
 * This program can be used and distrubuted without resrictions
 */

#include <cassert>
#include <cstdio>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <math.h>
#include <signal.h>
#include <ctime>
#include <atomic>
#include <unistd.h>
#include <fstream> //for reading and writing preset file
#include <mongoose.h>
#include "mjson.h"
#include <thread>

#include <getopt.h> 
#include <condition_variable>

#include <Processing.NDI.Lib.h>

NDIlib_find_create_t NDI_find_create_desc; /* Default settings for NDI find */
NDIlib_find_instance_t pNDI_find;
const NDIlib_source_t* p_sources = NULL;
struct mg_mgr mgr;   

//Function Definitions
std::string convertToString(char* a);

static char            *store_path;


struct recorder {
 recorder(const char* source, const char* path); //constructor
 ~recorder(void); //destructor 
 private:	
  pid_t pid;
	NDIlib_recv_instance_t m_pNDI_recv; // Create the receiver
  NDIlib_framesync_instance_t m_pNDI_framesync; //NDI framesync
	std::atomic<bool> m_exit;	// Are we ready to exit
  void receive(void); // This is called to receive frames
};




//Constructor
recorder::recorder(const char* source, const char* path): m_pNDI_recv(NULL), m_pNDI_framesync(NULL), m_exit(false){
  printf("Starting Recorder for %s\n", source);
  std::string store_path_string = path;
  store_path_string += source;
  const char *final_path = store_path_string.c_str();
  printf("Storing at %s\n", path);
  pid = fork();
  if(pid == 0){
   printf("child process, pid = %u\n",getpid()); 
   execl("/opt/ndi_recorder/bin/ndi-record", "/opt/ndi_recorder/bin/ndi-record", "-i", source, "-o", final_path, NULL); //start the recorder process
  }
}

// Destructor
recorder::~recorder(void){	// Wait for the thread to exit
	m_exit = true;
  kill(pid, SIGINT); //send INT signal to the ndi-record process
	// Destroy the receiver
}


static const int no_receivers = 50; //max number of recorders
recorder* p_receivers[no_receivers] = { 0 };
std::string ndi_running_name[no_receivers] = { "" };

static void fn(struct mg_connection *c, int ev, void *ev_data, void *fn_data){
  if(ev == MG_EV_WS_OPEN){
    c->label[0] = 'W';  // Mark this connection as an established WS client
  }
  if (ev == MG_EV_HTTP_MSG){
  struct mg_http_message *hm = (struct mg_http_message *) ev_data;
  struct mg_connection *c2 = mgr.conns;
   if(mg_http_match_uri(hm, "/ws")){ //upgrade to WebSocket
      mg_ws_upgrade(c, hm, NULL);
   }else if(mg_http_match_uri(hm, "/rest")) { //handle REST events
      mg_http_reply(c, 200, "", "{\"result\": %d}\n", 123);
   }else{ // Serve static files
      struct mg_http_serve_opts opts = {.root_dir = "/opt/ndi_recorder/assets/"};
      mg_http_serve_dir(c, (mg_http_message*)ev_data, &opts);
   }
  }else if (ev == MG_EV_WS_MSG){
    // Got websocket frame. Received data is wm->data. Echo it back!
    struct mg_ws_message *wm = (struct mg_ws_message *) ev_data;
    //std::cout << "WebSocket: " << wm->data.ptr << std::endl;
    char prefix_buf[100];
    char action_buf[100];
    mjson_get_string(wm->data.ptr, wm->data.len, "$.prefix", prefix_buf, sizeof(prefix_buf)); //get prefix
    mjson_get_string(wm->data.ptr, wm->data.len, "$.action", action_buf, sizeof(action_buf)); //get action
    std::string prefix_string = convertToString(prefix_buf);
    std::string action_string = convertToString(action_buf);
    if(prefix_string == "refresh"){
     if(action_string == "refresh"){

      uint32_t no_sources = 0; 
      p_sources = NDIlib_find_get_current_sources(pNDI_find, &no_sources);
      std::string discover_json;
      std::string source_json = "";
      discover_json = "{\"prefix\":\"discover_source\",\"action\":\"display\",\"source_list\":{";
      for(uint32_t i = 0; i < no_sources; i++){
       std::string ndi_string = p_sources[i].p_ndi_name;
       std::string url_string = p_sources[i].p_url_address;
       std::string source_id = std::to_string(i); 
       int conflict = 0;
       for(uint32_t i = 0; i < no_receivers; i++){ //check for conflicts
        if((ndi_running_name[i] == ndi_string)&&(conflict == 0)){
         conflict = 1; //found conflict with a name that is already stored - already running this receiver
        }  
       }
       if(conflict == 0){ //since this is not running on a receiver - display
        //std::cout << "Source IP: " << p_sources[i].p_url_address << std::endl;
        if(source_json == ""){
         source_json += "\""+source_id + "\":{\"name\":\""+ndi_string+"\",\"url\":\""+url_string+"\"}";  
        }else{
         source_json += ",\""+source_id + "\":{\"name\":\""+ndi_string+"\",\"url\":\""+url_string+"\"}";  
        }
       }
      }
      discover_json += source_json;
      discover_json += "}";
      discover_json += "}";
      const char* pub_json1 = discover_json.c_str();
      for (struct mg_connection *c2 = mgr.conns; c2 != NULL; c2 = c2->next) { //traverse over all client connections
       if (c2->label[0] == 'W'){ //make sure it is a websocket connection
        mg_ws_send(c2, pub_json1, strlen(pub_json1), WEBSOCKET_OP_TEXT);
       }
      }

      std::string connected_json;
      source_json = "";
      connected_json = "{\"prefix\":\"playing_source\",\"action\":\"display\",\"source_list\":{";
      for(uint32_t i = 0; i < no_receivers; i++){
       if(ndi_running_name[i] != ""){ //make sure receiver is not empty
        std::string source_id = std::to_string(i); 
        if(source_json == ""){
         source_json += "\""+source_id + "\":{\"name\":\""+ndi_running_name[i]+"\"}";  
        }else{
         source_json += ",\""+source_id + "\":{\"name\":\""+ndi_running_name[i]+"\"}";  
        }
       }
      }
      connected_json += source_json;
      connected_json += "}";
      connected_json += "}";
      const char* pub_json2 = connected_json.c_str();
      for (struct mg_connection *c2 = mgr.conns; c2 != NULL; c2 = c2->next) { //traverse over all client connections
       if (c2->label[0] == 'W'){ //make sure it is a websocket connection
        mg_ws_send(c2, pub_json2, strlen(pub_json2), WEBSOCKET_OP_TEXT);
       }
      }
     } 
    }

    if(prefix_string == "connect_source"){
     int source_id = std::stoi(action_string);
     int stored = 0;
     int receiver_id = 0;
     int conflict = 0;
     std::string ndi_string = p_sources[source_id].p_ndi_name;
     for(uint32_t i = 0; i < no_receivers; i++){ //check for conflicts
      if((ndi_running_name[i] == ndi_string)&&(conflict == 0)){
       conflict = 1; //found conflict with a name that is already stored - already running this receiver
      }  
     }
     if(conflict == 0){
      for(uint32_t i = 0; i < no_receivers; i++){
       if(stored == 0){
        if(ndi_running_name[i] == ""){ //empty string array
         ndi_running_name[i] = ndi_string;
         std::cout << "ID: " << i << std::endl;
         stored = 1;
         receiver_id = i;
        } 
       }
      }
      p_receivers[receiver_id] = new recorder(p_sources[source_id].p_ndi_name, store_path); 
     }else{
      //std::cout << "Receiver already running for:  " << p_sources[source_id].p_ndi_name << std::endl; 
     }
    }

    if(prefix_string == "disconnect_source"){
     int source_id = std::stoi(action_string);
     delete p_receivers[source_id]; //delete receiver
     ndi_running_name[source_id] = ""; //update the running receiver
    }

    if(prefix_string == "save_streams"){
     std::ofstream preset_file("/opt/ndi2jack/assets/presets.txt");
     for(uint32_t i = 0; i < no_receivers; i++){
      if(ndi_running_name[i] != ""){ //make sure a receiver is stored before trying to save in file
      preset_file << ndi_running_name[i];
      preset_file << std::endl;
      }
     }
     preset_file.close();
    }
    
  }
}

static void usage(FILE *fp, int argc, char **argv){
        fprintf(fp,
                 "Usage: NDI Recorder [options]\n\n"
                 "Version 1.0\n"
                 "Options:\n"
                 "-h | --help          Print this message\n"
                 "-p | --path          Specify storage path (e.g. /media/store/)\n"
                 "",
                 argv[0]);
}

static const char short_options[] = "p:";

static const struct option
long_options[] = {
        { "help",   no_argument,       NULL, 'h' },
        { "path", required_argument,  NULL, 'p' },
        { 0, 0, 0, 0 }
};

int main (int argc, char *argv[]){
  for (;;) {
   int idx;
   int c;
   c = getopt_long(argc, argv,short_options, long_options, &idx);
   if (-1 == c){
    break;
   }
   switch(c){
    case 'h':
     usage(stdout, argc, argv);
     exit(EXIT_SUCCESS);
    case 'p':
     store_path = optarg;
     break;           
    default:
     usage(stderr, argc, argv);
     exit(EXIT_FAILURE);
   }
  }

  if(!NDIlib_initialize()){	
	 printf("Cannot run NDI."); // Cannot run NDI. Most likely because the CPU is not sufficient.
	 return 0;
	}

	// Create a NDI finder	
  NDI_find_create_desc.show_local_sources = (bool)false; //don't include local sources when searching for NDI
	pNDI_find = NDIlib_find_create_v2(&NDI_find_create_desc);
	if (!pNDI_find) return 0; //error out if the NDI finder can't be created

  std::string output_text; //preset file is temporary stored in this variable
  std::ifstream preset_file("/opt/ndi2jack/assets/presets.txt"); //open the presets file
  while(getline(preset_file, output_text)){
   int stored = 0;
   int receiver_id = 0;
   const char* ndi_name = output_text.c_str();;
   std::string ndi_string = ndi_name;
   for(uint32_t i = 0; i < no_receivers; i++){
    if(stored == 0){
     if(ndi_running_name[i] == ""){ //empty string array - make sure it is empty before trying to start receiver
      ndi_running_name[i] = ndi_string;
      stored = 1;
      receiver_id = i;
     } 
    }
   }
   p_receivers[receiver_id] = new recorder(ndi_name,store_path);
  }
                               
  mg_mgr_init(&mgr);
  mg_http_listen(&mgr, "ws://0.0.0.0:80", fn, NULL);   // Create WebSocket and HTTP connection
  for (;;) mg_mgr_poll(&mgr, 1000);  // Block forever
  /* keep running until the Ctrl+C */
  while(1){
   sleep(1);
  }
  
  exit (0);
}

std::string convertToString(char* a){
  std::string s = a;
  return s;
}
