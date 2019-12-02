/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <gio/gio.h>

#include "capi.h"

#include "libubox/ustream.h"
#include "libubox/uloop.h"
#include "libubox/usock.h"
#include "xb_socket.h" 
#include "wanclient.h"

int running = 1;

char *uds_file = "/uds/nets/nets.uds";

int xb_main(int argc, char ** argv);

static void uds_cb(struct ustream *s, int bytes)
{
	struct ustream_buf *buf = s->r.head;
	char *str;
	int len;

	do {
		str = ustream_get_read_buf(s, NULL);
		if (!str)
			break;
		len = strlen(buf->data);
		if (!len) {
			bytes -= 1;
			ustream_consume(s, 1);
			continue;
		}

		ustream_consume(s, len);
		bytes -= len;
	} while (bytes > 0);
}

struct ustream_fd uds = {
	.stream.string_data = true,
	.stream.notify_read =uds_cb,
};

/* Introspection data for the service we are exporting */

static const unsigned char intro_xml[] =
	"<node>"
	"  <interface name='com.xbsafe.nets1.intf2'>"
	"    <method name='Config'>"
	"      <arg type='s' name='name' direction='in'/>"
	"	 <arg type='i' name='response' direction='out'/>"
	"    </method>"
	"    <method name='Func1'>"
	"      <arg type='s' name='name' direction='in'/>"
	"	 <arg type='s' name='response' direction='out'/>"
	"    </method>"
	"    <method name='Func2' >"
	"	   <arg type='s' name='response' direction='out'/>"
	"    </method>"
	"    <method name='ReadOption' >"
	"	   <arg type='s' name='section_name' direction='in'/>"
	"	   <arg type='s' name='option' direction='in'/>"
	"	   <arg type='s' name='value' direction='out'/>"
	"    </method>"
	"    <method name='WriteOption' >"
	"	   <arg type='s' name='section_name' direction='in'/>"
	"	   <arg type='s' name='option' direction='in'/>"
	"	   <arg type='s' name='value' direction='in'/>"
	"    </method>"
	"    <method name='ReadSectionByName' >"
	"	   <arg type='s' name='section_name' direction='in'/>"
	"    </method>"
	"    <method name='ReadSectionByType' >"
	"	   <arg type='s' name='section_type' direction='in'/>"
	"    </method>"
	"	 <property type='s' name='Title' access='readwrite'/>"
	"	 <property type='s' name='ReadingAlwaysThrowsError' access='read'/>"
	"	 <property type='s' name='WritingAlwaysThrowsError' access='readwrite'/>"
	"  </interface>"
	"</node>";


static int
handle_method_call(const char           *object_path,
				   const char           *interface_name,
				   const char           *method_name,
				   GVariant            *inargs,
				   GVariant            **outargs,
				   void                *user_data)
{
	gchar *section, *option, *value;
	uint32_t size = 0, i = 0;
	CtSgwUCIOpt_t *opts = NULL;

	//g_print("%s,%d: call %s\n", __func__, __LINE__, method_name);
	if (g_strcmp0(method_name, "ReadOption") == 0) {

		CtSgwUCIOptVal_t value = {0};
		g_variant_get(inargs, "(&s&s)", &section, &option);

		CtSgwUCIReadOption("demoapp", section, option, value);

		*outargs = g_variant_new("(s)", value);
	} else if (g_strcmp0(method_name, "WriteOption") == 0) {

		g_variant_get(inargs, "(&s&s&s)", &section, &option, &value);

		CtSgwUCIWriteOption("demoapp", section, option, value);

	} else if (g_strcmp0(method_name, "ReadSectionByName") == 0) {

		g_variant_get(inargs, "(&s)", &section);

		CtSgwUCIReadSection("demoapp", NULL, section, &opts, &size);

		for (i = 0; i < size && opts; i++, opts++) {
			g_print("[%d] %s.%s.%s = %s\n", i, "demoapp", section, opts->name, opts->value);
		}
	} else if (g_strcmp0(method_name, "ReadSectionByType") == 0) {

		g_variant_get(inargs, "(&s)", &section);

		CtSgwUCIReadSection("demoapp", section, NULL, &opts, &size);

		for (i = 0; i < size && opts; i++, opts++) {
			g_print("[%d] %s.@%s[].%s = %s\n", i, "demoapp", section, opts->name, opts->value);
		}
	} else if (g_strcmp0(method_name, "Func1") == 0) {
		const gchar *name = NULL;
		char str[64] = {0};

		g_variant_get (inargs, "(&s)", &name);		

		sprintf(str, "Hello %s", name);
		*outargs = g_variant_new("(s)", str);
	} else if (g_strcmp0(method_name, "Func2") == 0) {
		gchar *response = "Byebye2";
#if 1		
		//printf("%d, touch /tmp/nets\n", __LINE__);
		//system("touch /tmp/nets");
		//system("touch .uci/nets");
		//printf("%d, cat /etc/config/network\n", __LINE__);
		//system("cat /etc/config/network");
		//system("touch etc/config/nets");
		//printf("%d, uci add\n", __LINE__);
		system("uci add nets sec1");
		//printf("%d, uci rename\n", __LINE__);
		system("uci rename nets.@sec1[0]=name1");
		//printf("%d, uci set\n", __LINE__);
		system("uci set nets.@sec1[0].key1=val1");
		//printf("%d, uci commit\n", __LINE__);
		system("uci commit nets");
#endif
		*outargs = g_variant_new("(s)", response);
	}

	return 0;
}

static const unsigned char config_global_xml[] =
	"<node>"
	"  <interface name='com.xbsafe.demoapp1.Config.global'>"
	"	 <property type='s' name='hi' access='readwrite'/>"
	"	 <property type='s' name='hello' access='read'/>"
	"	 <property type='s' name='shit' access='readwrite'/>"
	"  </interface>"
	"</node>";

static int _config_handle_method_call(const char *section_name, const char *section_type, int section_idx,
								 const char           *method_name,
								 GVariant           *inargs,
								 GVariant           **outargs)
{
	const gchar *intf, *key, *val;
	GVariant *value;
	char cmd[128] = {0};

	if (g_strcmp0(method_name, "Get") == 0) {

		char line[64] = {0};
		int len = 0;
		FILE *fp = NULL;
		
		g_variant_get(inargs, "(&s&s)", &intf, &key);

		if (section_name)
			sprintf(cmd, "uci get nets.%s.%s", section_name, key);
		else if (section_type)
			sprintf(cmd, "uci get nets.@%s[%d].%s", section_type, section_idx, key);
		fp = popen(cmd, "r");
		if (fp) {
			fgets(line, 64, fp);
			len = strlen(line);
			if (line[len - 1] == '\n')
				line[len - 1] = '\0';	
			pclose(fp);
		}

		*outargs = g_variant_new("(v)", g_variant_new("s", line));
	} else if (g_strcmp0(method_name, "Set") == 0) {

		g_variant_get(inargs, "(&s&sv)", &intf, &key, &value);
		g_variant_get(value, "&s", &val);

		if (section_name)
			sprintf(cmd, "uci set nets.%s.%s=%s", section_name, key, val);
		else if (section_type)
			sprintf(cmd, "uci set nets.@%s[%d].%s=%s", section_type, section_idx, key, val);
			
		system(cmd);	
		system("uci commit demoapp");
	}

	return 0;
}

static int
config_global_hmcall(const char           *object_path,
								 const char           *interface_name,
								 const char           *method_name,
								 GVariant           *inargs,
								 GVariant           **outargs,
								 void *user_data)
{
	_config_handle_method_call("global", NULL, 0, method_name, inargs, outargs);
	return 0;
}

static int config_global_propget(const char    *path,
									   const char    *prop,
									   GVariant     **value,
									   void          *userdata)
{
	char line[64] = {0};
	int len = 0;
	FILE *fp = NULL;
	char cmd[128] = {0};
	//g_print("%s(%d): get %s\n", __func__, __LINE__, prop);

	sprintf(cmd, "/sbin/uci get nets.global.%s", prop);
	fp = popen(cmd, "r");

	if (fp) {
		fgets(line, 64, fp);
		len = strlen(line);

		if (line[len - 1] == '\n')
			line[len - 1] = '\0';

		pclose(fp);
	}

	*value =g_variant_new("s", line);
	return 0;
}

static CtSgwGDBusInterfaceVTable_t interface_vtable = { handle_method_call, NULL, NULL, NULL, NULL };

static CtSgwGDBusInterfaceVTable_t interface_global_vtable = { config_global_hmcall, NULL, NULL, NULL, NULL };

static int xbnets_stop(void)
{
	running = 0;
	return 0;
}

static int xbnets_reload(void)
{
	CtSgwLog(LOG_NOTICE, "reloaded\n");
	return 0;
}

struct NetworkPort networkPort;
void updateNetworkConfigure(void);
int handle_postmsg(struct NetworkPort *port, const char *msg,int len);
char* data_back = NULL;

static char *xbnets_postmsg(char *msg)
{
	char *data; 
	g_print("xbnets postmsg ptr %p\n", msg);

	if(msg == NULL)
		return "{Result:-1}";

	CtSgwLog(LOG_NOTICE, "postmsg %s\n", msg);
	g_print("xbnets postmsg %s\n", msg);

	//return "{Result:0}";

	if(data_back != NULL){
		g_free(data_back);
		data_back = NULL;
	}

	updateNetworkConfigure();  //temp solution for IP address empty.

    networkPort.sendback = handle_postmsg;
    networkPort.socket = -1;
    networkPort.packetFlag = 0;
    if(route_message(&networkPort, msg,strlen(msg)) == -100)
   {
     notify_reconnect();
   }

   if(data_back != NULL){
   	  return data_back;
   }

	return "{Result:0}";
}

int handle_postmsg(struct NetworkPort *port, const char *msg,int len)
{
	char * data = NULL;
	if(len <= 0)
		return len;

	data = g_malloc(len+1);
	data[len] = '\0';
	strncpy(data, msg, len);

	data_back = data;
	return len;
}

static void test_register_methods(CtSgwDBusNodeInfo_t *intro, CtSgwDBusNodeInfo_t *config_g)
{
	guint registration_id;

	CtSgwLog(LOG_NOTICE, "%s: registering dbus object.", __func__);
	
	registration_id = CtSgwDBusRegisterObject("/com/xbsafe/nets1",
					  intro->interfaces[0],
					  &interface_vtable,
					  NULL);
	g_assert(registration_id > 0);

	registration_id = CtSgwDBusRegisterObject("/com/xbsafe/nets1/Config/global/global",
					  config_g->interfaces[0],
					  &interface_global_vtable,
					  NULL);
	g_assert(registration_id > 0);

	//setup response for method: GetManagedObjects
	GDBusObjectManagerServer *manager = NULL;
	GDBusObjectSkeleton *skeleton;

	manager = g_dbus_object_manager_server_new("/com/xbsafe/nets1/Config");

	skeleton = g_dbus_object_skeleton_new("/com/xbsafe/nets1/Config/global/global");
	g_dbus_object_manager_server_export(manager, skeleton);

	//TODO: add api func to return connection
	g_dbus_object_manager_server_set_connection(manager, sgw_dbus_service_context->connection);

	int fd;

	unlink(uds_file);
	fd = usock(USOCK_UNIX | USOCK_UDP | USOCK_SERVER | USOCK_NONBLOCK, uds_file, NULL);
	if (fd < 0) {
		fprintf(stderr,"Failed to open %s, %s\n", uds_file, strerror(errno));
		return;
	}
	chmod(uds_file, 0666);
	ustream_fd_init(&uds, fd);

}

static int  _dns_resolver(char *domain, char *ip_list, int max_len_ip)
{
	char *ipstr = ip_list;
	int i = 0;
	struct addrinfo *result, *rp;
	struct addrinfo hints = {
		.ai_family = AF_UNSPEC,
		.ai_socktype = SOCK_STREAM,
	};
	CtSgwLog(LOG_ERR, "*** %s, domain %s", __func__, domain);

	if (getaddrinfo(domain, NULL, &hints, &result)) {
		CtSgwLog(LOG_ERR, "failed to getaddrinfo %s", domain);
		return -1;
	}

	for (rp = result; rp != NULL; rp = rp->ai_next) {
		i++;
		if (rp->ai_family == AF_INET)
			inet_ntop(rp->ai_family, &(((struct sockaddr_in *)rp->ai_addr)->sin_addr), ipstr, max_len_ip);
		else if (rp->ai_family == AF_INET6)
			inet_ntop(rp->ai_family, &(((struct sockaddr_in6 *)rp->ai_addr)->sin6_addr), ipstr, max_len_ip);
		CtSgwLog(LOG_DEBUG, "**** [%d] %s", i, ipstr);
		ipstr += max_len_ip;
	}	

	freeaddrinfo(result);

	return i;
}

void print_dir(void)
{
    char curdir[1024];
    getcwd(curdir, 1024);
    CtSgwLog(LOG_NOTICE, "cwd: %s\n", curdir);
}


int main(int argc, char *argv[])
{
	CtSgwDBusNodeInfo_t *mm_data = NULL;
	CtSgwDBusNodeInfo_t *intro_data = NULL;
	CtSgwDBusNodeInfo_t *config_g_data = NULL;
	int ret = 0;

	CtSgwAppMgtCallbacks_t app_cbs = { 
		.stop = xbnets_stop,
		.reload = xbnets_reload,
		.restore_factory = NULL,
		.post_msg = xbnets_postmsg
	};

	CtSgwLogOpen(LOG_USER, "xbnets");

	CtSgwLog(LOG_NOTICE, "start\n");
	print_dir();
	
	ret = CtSgwDBusStartService("com.xbsafe.nets1");
	if (ret < 0) {
		CtSgwLog(LOG_NOTICE, "failed to start dbus service\n");
		CtSgwLogClose();	
		return ret;
	}

	intro_data = CtSgwDBusNodeParseXml(intro_xml);
	config_g_data = CtSgwDBusNodeParseXml(config_global_xml);

	test_register_methods(intro_data, config_g_data);

	mm_data = CtSgwAppRegisterMgtFuncs(&app_cbs);

	char ipaddrs[5][46];
	//system("cat /tmp/resolv.conf >/data/xbnets.dns");
	system("cat /etc/resolv.conf >/data/nets.dns");
	//system("ping www.qq.com -c 4 > /data/nets.ping");
	//_dns_resolver("www.qq.com", ipaddrs[0], 46);

	CtSgwDeviceInfo_t dev_info;

	memset(&dev_info, 0, sizeof(CtSgwDeviceInfo_t));
	ret = CtSgwGetDeviceInfo(&dev_info);
	g_print("ret = %d\n", ret);

	//ret = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
	//CtSgwLog(LOG_NOTICE, "socket ret %d, %s\n", ret, strerror(errno));

	g_print("xbnets Running\n");

	//starting xbnets specific code
    memset(&xb_conn, 0, sizeof(xb_conn));
    strcpy(xb_conn.host, "qoe.xbsafe.cn"); //222.222.104.254 220.165.83.34 qoe.xbsafe.cn
    xb_conn.port = 61000;

    xb_conn.retry_interval = 10;
    xb_conn.policy = 0; 

    loadconfig_file("/etc/config/default_xbnets.ini");

	xb_main(argc, argv);
	
	while (running) {
		sleep(1);
	}
	g_print("xbnets Exiting\n");

	CtSgwDBusNodeInfoUnref(intro_data);
	CtSgwDBusNodeInfoUnref(config_g_data);
	CtSgwDBusNodeInfoUnref(mm_data);
	CtSgwDBusStopService();

	CtSgwLog(LOG_NOTICE, "exit\n");
	CtSgwLogClose();	

	return 0;
}

