#ifndef PROXYCLIENT_H_INCLUDED
#define PROXYCLIENT_H_INCLUDED

std::string tryGetModel(std::string &swVersion);

int ctrl_mac_access(char* mac, int internetAccess);
void handle_get_blacklist(duk_context *ctx, duk_idx_t indx0) ;
void handle_get_net_lan_info(duk_context *ctx, duk_idx_t indx0);
void BuildPortStatusObj(duk_context *ctx, duk_idx_t indx);
void BuildWiFiStatusObj(duk_context *ctx, duk_idx_t indx);
void BuildBasicStatusObj(duk_context *ctx, duk_idx_t indx);
void BuildSystemInfoObj(duk_context *ctx, duk_idx_t indx);

#endif