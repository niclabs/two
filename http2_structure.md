#Documentation for the HTTP/2 implementation.

TODO: How to set client and server variables.

========= **Global Variables** ============

remote_setting: array used to store the remote conf. settings. First slot is not used.
local_settings: array used to store the local conf. settings. First slot is not used.
local_cache: array used to store possible changes on local settings. First slot is not used.
client: boolean that indicates if the current process is client.
server: boolean that indicates if the curren process is server.
waiting_sett_ack: bool that indicates if the current process is waiting for settings ack.

========== **Methods** ====================

---- **Settings methods** ------
send_local_settings: sends a settings frame with the local configuration
update_settings_table: updates one of the settings tables
send_settings_ack: sends a settings frame with the ACK flag.
read_setting_from: read a setting parameter from one of the settings tables.


---- **Connection methods** -----
init_connection: initialize a HTTP/2 connection with prior knowledge.

---- **Frames methods** ----
receive_frame: TODO
receive_settings: TODO
callback: TODO

----- **HTTP methods** -----
send_headers: TODO
receive_headers: TODO



