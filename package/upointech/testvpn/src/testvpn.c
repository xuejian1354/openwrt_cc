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

#include <gio/gio.h>

#include "capi-app-mgr.h"

#include "libubox/ustream.h"
#include "libubox/uloop.h"
#include "libubox/usock.h"

static GDBusNodeInfo *introspection_data = NULL;
static GDBusNodeInfo *introspection_data2 = NULL;
static GDBusNodeInfo *introspection_config_data = NULL;
static gchar *_global_title = NULL;
GMainLoop *loop;

char *uds_file = "/uds/testvpn/testvpn.uds";

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
static const gchar introspection_xml[] =
	"<node>"
	"  <interface name='com.jarvis.testvpn1.intf1'>"
	"    <method name='Stop'>"
	"	 <arg type='u' name='response' direction='out'/>"
	"    </method>"
	"    <method name='Reload'>"
	"	 <arg type='u' name='response' direction='out'/>"
	"    </method>"
	"  </interface>"
	"</node>";

static const gchar introspection_xml2[] =
	"<node>"
	"  <interface name='com.jarvis.testvpn1.intf2'>"
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
	"	 <property type='s' name='Title' access='readwrite'/>"
	"	 <property type='s' name='ReadingAlwaysThrowsError' access='read'/>"
	"	 <property type='s' name='WritingAlwaysThrowsError' access='readwrite'/>"
	"  </interface>"
	"</node>";

static const gchar introspection_config_xml[] =
	"<node>"
	"  <interface name='org.freedesktop.DBus.Properties'>"
	"    <method name='Get' >"
	"      <arg type='s' name='key' direction='in'/>"
	"      <arg type='s' name='value' direction='out'/>"
	"    </method>"
	"    <method name='Set' >"
	"      <arg type='s' name='key' direction='in'/>"
	"      <arg type='s' name='value' direction='in'/>"
	"      <arg type='i' name='response' direction='out'/>"
	"    </method>"
	"    <property type='s' name='key1' access='readwrite'/>"
	"    <property type='s' name='key3' access='readwrite'/>"
	"    <property type='s' name='hello' access='readwrite'/>"
	"  </interface>"
	"</node>";


static void
handle_method_call(GDBusConnection       *connection,
				   const gchar           *sender,
				   const gchar           *object_path,
				   const gchar           *interface_name,
				   const gchar           *method_name,
				   GVariant              *parameters,
				   GDBusMethodInvocation *invocation,
				   gpointer               user_data)
{
	g_print("%s,%d: call %s\n", __func__, __LINE__, method_name);
	if (g_strcmp0(method_name, "Stop") == 0) {
		guint ret = 0;

		//TODO: Add necessary cleanup functions

		g_main_loop_quit(loop);

		g_dbus_method_invocation_return_value (invocation, g_variant_new ("(u)", ret));
	} else if (g_strcmp0(method_name, "Reload") == 0) {
		guint ret = 0;

		//TODO

		g_dbus_method_invocation_return_value(invocation, g_variant_new("(u)", ret));
	} else if (g_strcmp0(method_name, "Func1") == 0) {
		const gchar *name = NULL;

		g_variant_get (parameters, "(&s)", &name);

		g_dbus_method_invocation_return_value(invocation, g_variant_new("(s)", name));
	} else if (g_strcmp0(method_name, "Func2") == 0) {
		gchar *response = "Byebye2";
		g_dbus_method_invocation_return_value(invocation, g_variant_new("(s)", response));
	}
}

static GVariant *
handle_get_property(GDBusConnection  *connection,
					const gchar      *sender,
					const gchar      *object_path,
					const gchar      *interface_name,
					const gchar      *property_name,
					GError          **error,
					gpointer          user_data)
{
	GVariant *ret;

	ret = NULL;

	if (g_strcmp0(property_name, "Title") == 0) {
		if (_global_title == NULL)
			_global_title = g_strdup("Back To C!");

		ret = g_variant_new_string(_global_title);
	} else if (g_strcmp0(property_name, "ReadingAlwaysThrowsError") == 0) {
		g_set_error(error,
					G_IO_ERROR,
					G_IO_ERROR_FAILED,
					"Hello %s. I thought I said reading this property "
					"always results in an error. kthxbye",
					sender);
	} else if (g_strcmp0(property_name, "WritingAlwaysThrowsError") == 0) {
		ret = g_variant_new_string("There's no home like home");
	}

	return ret;
}

static gboolean
handle_set_property(GDBusConnection  *connection,
					const gchar      *sender,
					const gchar      *object_path,
					const gchar      *interface_name,
					const gchar      *property_name,
					GVariant         *value,
					GError          **error,
					gpointer          user_data)
{
	if (g_strcmp0(property_name, "Title") == 0) {
		if (g_strcmp0(_global_title, g_variant_get_string(value, NULL)) != 0) {
			GVariantBuilder *builder;
			GError *local_error;

			g_free(_global_title);
			_global_title = g_variant_dup_string(value, NULL);

			local_error = NULL;
			builder = g_variant_builder_new(G_VARIANT_TYPE_ARRAY);
			g_variant_builder_add(builder,
								  "{sv}",
								  "Title",
								  g_variant_new_string(_global_title));
			g_dbus_connection_emit_signal(connection,
										  NULL,
										  object_path,
										  "org.freedesktop.DBus.Properties",
										  "PropertiesChanged",
										  g_variant_new("(sa{sv}as)",
												  interface_name,
												  builder,
												  NULL),
										  &local_error);
			g_assert_no_error(local_error);
		}
	} else if (g_strcmp0(property_name, "ReadingAlwaysThrowsError") == 0) {
		/* do nothing - they can't read it after all! */
	} else if (g_strcmp0(property_name, "WritingAlwaysThrowsError") == 0) {
		g_set_error(error,
					G_IO_ERROR,
					G_IO_ERROR_FAILED,
					"Hello AGAIN %s. I thought I said writing this property "
					"always results in an error. kthxbye",
					sender);
	}

	return *error == NULL;
}


static const GDBusInterfaceVTable interface_vtable = {
	handle_method_call,
	handle_get_property,
	handle_set_property,
};

static void
config_global_handle_method_call(GDBusConnection       *connection,
								 const gchar           *sender,
								 const gchar           *object_path,
								 const gchar           *interface_name,
								 const gchar           *method_name,
								 GVariant              *parameters,
								 GDBusMethodInvocation *invocation,
								 gpointer               user_data)
{
	const gchar *key;
	GDBusProxy *proxy;
	GError *error;
	GVariant *result;

	error = NULL;
	proxy = g_dbus_proxy_new_sync(connection,
								  G_DBUS_PROXY_FLAGS_NONE,
								  NULL,					  /* GDBusInterfaceInfo */
								  "com.ctc.appframework1", /* name */
								  "/com/ctc/appframework1", /* object path */
								  "com.ctc.appframework1.UCI",		  /* interface */
								  NULL, /* GCancellable */
								  &error);
	g_assert_no_error(error);

	g_print("%s,%d: call %s\n", __func__, __LINE__, method_name);
	if (g_strcmp0(method_name, "Get") == 0) {

		const gchar *str = NULL;

		g_variant_get(parameters, "(&s)", &key);

		result = g_dbus_proxy_call_sync(proxy,
										"ReadOption",
										g_variant_new("(sss)", "testvpn", "global", key),
										G_DBUS_CALL_FLAGS_NONE,
										-1,
										NULL,
										&error);
		g_assert_no_error(error);
		g_assert(result != NULL);
		g_variant_get(result, "(&s)", &str);

		g_dbus_method_invocation_return_value(invocation, g_variant_new("(s)", str));
	} else if (g_strcmp0(method_name, "Set") == 0) {

		const gchar *value;
		gint ret = 0;

		g_variant_get(parameters, "(&s&s)", &key, &value);

		result = g_dbus_proxy_call_sync(proxy,
										"WriteOption",
										g_variant_new("(ssss)", "testvpn", "global", key, value),
										G_DBUS_CALL_FLAGS_NONE,
										-1,
										NULL,
										&error);
		g_assert_no_error(error);
		g_assert(result != NULL);
		g_variant_get(result, "(i)", &ret);

		g_dbus_method_invocation_return_value(invocation, g_variant_new("(i)", ret));
	}
}


static const GDBusInterfaceVTable interface_global_vtable = {config_global_handle_method_call, NULL, NULL};


static void
on_bus_acquired(GDBusConnection *connection,
				const gchar     *name,
				gpointer         user_data)
{
	guint registration_id;

	CtSgwLog(LOG_NOTICE, "%s: registering dbus object.", __func__);
	
	registration_id = g_dbus_connection_register_object(connection,
					  "/com/jarvis/testvpn1",
					  introspection_data->interfaces[0],
					  &interface_vtable,
					  NULL,  /* user_data */
					  NULL,  /* user_data_free_func */
					  NULL); /* GError** */

	g_assert(registration_id > 0);

	registration_id = g_dbus_connection_register_object(connection,
					  "/com/jarvis/testvpn1",
					  introspection_data2->interfaces[0],
					  &interface_vtable,
					  NULL,	/* user_data */
					  NULL,	/* user_data_free_func */
					  NULL); /* GError** */

	g_assert(registration_id > 0);

	registration_id = g_dbus_connection_register_object(connection,
					  "/com/jarvis/testvpn1/Config/global/global",
					  introspection_config_data->interfaces[0],
					  &interface_global_vtable, /* vtable */
					  NULL, /* user_data */
					  NULL, /* user_data_free_func */
					  NULL); /* GError** */

	g_assert(registration_id > 0);

	registration_id = g_dbus_connection_register_object(connection,
					  "/com/jarvis/testvpn1/Config/section/1",
					  introspection_config_data->interfaces[0],
					  &interface_global_vtable, /* vtable */
					  NULL, /* user_data */
					  NULL, /* user_data_free_func */
					  NULL); /* GError** */

	g_assert(registration_id > 0);
	registration_id = g_dbus_connection_register_object(connection,
					  "/com/jarvis/testvpn1/Config/section/2",
					  introspection_config_data->interfaces[0],
					  &interface_global_vtable, /* vtable */
					  NULL, /* user_data */
					  NULL, /* user_data_free_func */
					  NULL); /* GError** */

	g_assert(registration_id > 0);

#if 1 //setup response for method: GetManagedObjects
	GDBusObjectManagerServer *manager = NULL;
	GDBusObjectSkeleton *skeleton;

	manager = g_dbus_object_manager_server_new("/com/jarvis/testvpn1/Config");

	skeleton = g_dbus_object_skeleton_new("/com/jarvis/testvpn1/Config/global/global");
	g_dbus_object_manager_server_export(manager, skeleton);
	skeleton = g_dbus_object_skeleton_new("/com/jarvis/testvpn1/Config/section/1");
	g_dbus_object_manager_server_export(manager, skeleton);
	skeleton = g_dbus_object_skeleton_new("/com/jarvis/testvpn1/Config/section/2");

	g_dbus_object_manager_server_export(manager, skeleton);
	g_dbus_object_manager_server_set_connection(manager, connection);
#endif

	int fd;

	unlink(uds_file);
	fd = usock(USOCK_UNIX | USOCK_UDP | USOCK_SERVER | USOCK_NONBLOCK, uds_file, NULL);
	if (fd < 0) {
		fprintf(stderr,"Failed to open %s\n", uds_file);
		return;
	}
	chmod(uds_file, 0666);
	ustream_fd_init(&uds, fd);

}

static void
on_name_lost(GDBusConnection *connection,
			 const gchar     *name,
			 gpointer         user_data)
{
	CtSgwLog(LOG_NOTICE, "%s\n", __func__);
	exit(1);
}

#if 0
static int testvpn_stop(void)
{
	return 0;
}

static int testvpn_reload(void)
{
	return 0;
}
#endif

static void testvpn()
{
	int ret;

	CtSgwStrArray_t domains[2];
	memset(domains, 0, sizeof(CtSgwStrArray_t)*2);
	strcpy(domains[0].str, "www.qq.com");
	strcpy(domains[1].str, "www.163.com");

	CtSgwStrArray_t ips[2];
	memset(ips, 0, sizeof(CtSgwStrArray_t)*2);
	strcpy(ips[0].str, "140.205.230.13-140.205.230.13");
	strcpy(ips[1].str, "180.163.150.161-180.163.150.165");

	CtSgwVPNConn_t vpn =
	{
		.path.str = "",
		.vpn_type = "l2tp",
		.tunnel_name = "vpn1",
		.user_id = "111111",
		.vpn_enable = TRUE,
		.vpn_mode = "steady",
		.vpn_priority = "5",
		.vpn_idletime = "3600",
		.account_proxy = "ip.addr_to_get_vpn_account.com",
		.vpn_addr = "118.89.112.188",
		.vpn_account = "nj_ct_a",
		.vpn_pwd = "UMn3mqm96KWw26jw",
		.vpn_port = "1701",
		.attach_mode = "1",
		.domain_num = 2,
		.domains = domains,
		.ip_num = 2,
		.ips = ips,
		.mac_num = 0,
		.terminal_mac = NULL
	};

	/*ret = CtSgwDelVPNConnByName("vpntest1");
	if (ret == CTSGW_OK)
	{
		CtSgwLog(LOG_NOTICE, "CtSgwDelVPNConnection success.");
	}
	else
	{
		CtSgwLog(LOG_NOTICE, "CtSgwDelVPNConnection failed.");
	}*/

	ret = CtSgwAddVPNConnection(&vpn);
	if (ret == CTSGW_OK)
	{
		CtSgwLog(LOG_NOTICE, "CtSgwAddVPNConnection success.");
	}
	else
	{
		CtSgwLog(LOG_NOTICE, "CtSgwAddVPNConnection failed.");
	}

	/*CtSgwDetachVPNConnection ("vpntest1",
		"61.129.7.47;115.231.171.70",
		"www.sogou.com",
		NULL);*/

	CtSgwAttachVPNConnection ("vpn1",
		"8.8.4.4-8.8.8.8;47.95.164.112-47.95.164.113",
		"www.baidu.com",
		NULL);
}

int main(int argc, char *argv[])
{
	guint owner_id;

	//CtSgwAppMgtCallbacks_t app_cbs = { 
	//	.stop = testvpn_stop,
	//	.reload = testvpn_reload
	//};

	CtSgwLogOpen(LOG_USER, "testvpn");

	CtSgwLog(LOG_NOTICE, "start\n");

	CtSgwSetDbusEnv();

	//CtSgwAppRegisterMgtFuncs(&app_cbs);

	introspection_data = g_dbus_node_info_new_for_xml(introspection_xml, NULL);
	g_assert(introspection_data != NULL);
	introspection_data2 = g_dbus_node_info_new_for_xml(introspection_xml2, NULL);
	g_assert(introspection_data2 != NULL);

	introspection_config_data = g_dbus_node_info_new_for_xml(introspection_config_xml, NULL);
	g_assert(introspection_config_data != NULL);

	owner_id = g_bus_own_name(DBUS_TYPE,
							  "com.jarvis.testvpn1",
							  G_BUS_NAME_OWNER_FLAGS_ALLOW_REPLACEMENT,
							  on_bus_acquired,
							  NULL,
							  on_name_lost,
							  NULL,
							  NULL);
	g_assert(owner_id != 0);

	CtSgwPPPoE_t pppoe;
	memset(&pppoe, 0, sizeof(pppoe));
	if (CtSgwGetPPPoEConfig(&pppoe) == CTSGW_OK)
	{
		char excmd[64] = {0};
		sprintf(excmd,"echo %s > /data/pppuser.dat", pppoe.PPPoEUserName);
		system(excmd);
	}

	testvpn();

	loop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(loop);

	CtSgwLog(LOG_NOTICE, "exit\n");

	g_bus_unown_name(owner_id);
	g_dbus_node_info_unref(introspection_data);
	g_dbus_node_info_unref(introspection_data2);
	g_dbus_node_info_unref(introspection_config_data);

	CtSgwLogClose();	

	return 0;
}
