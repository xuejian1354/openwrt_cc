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
#include <netdb.h>
#include <arpa/inet.h>

#include <gio/gio.h>

#include "capi.h"
#include "demolib.h"

int running = 1;

typedef struct {
	GDBusInterfaceSkeleton parent;
	gint number;
} DemoInterface;

typedef struct {
	GDBusInterfaceSkeletonClass parent_class;
} DemoInterfaceClass;

static GType demo_interface_get_type(void);
G_DEFINE_TYPE(DemoInterface, demo_interface, G_TYPE_DBUS_INTERFACE_SKELETON);

static void
demo_interface_init(DemoInterface *self)
{
	//do nothing
}

static GDBusInterfaceInfo *
demo_interface_get_info(GDBusInterfaceSkeleton *skeleton)
{
	static GDBusPropertyInfo key1_info = {
		-1,
		"key1",
		"s",
		G_DBUS_PROPERTY_INFO_FLAGS_READABLE | G_DBUS_PROPERTY_INFO_FLAGS_WRITABLE,
		NULL,
	};

	static GDBusPropertyInfo key2_info = {
		-1,
		"key2",
		"s",
		G_DBUS_PROPERTY_INFO_FLAGS_READABLE | G_DBUS_PROPERTY_INFO_FLAGS_WRITABLE,
		NULL,
	};
	static GDBusPropertyInfo key3_info = {
		-1,
		"key3",
		"s",
		G_DBUS_PROPERTY_INFO_FLAGS_READABLE | G_DBUS_PROPERTY_INFO_FLAGS_WRITABLE,
		NULL,
	};

	static GDBusPropertyInfo *property_info[] = {
		&key1_info, &key2_info, &key3_info,
		NULL
	};

	static GDBusInterfaceInfo interface_info = {
		-1,
		(gchar *) "com.upointech.demoapp1.Config.section",
		NULL,
		NULL,
		(GDBusPropertyInfo **) &property_info,
		NULL
	};

	return &interface_info;
}

static void _get_uci_section_val(const char *section_type, int idx, const char *opt_name, char *opt_val)
{
	char line[64] = {0};
	int len = 0;
	FILE *fp = NULL;
	char cmd[128] = {0};

	sprintf(cmd, "/sbin/uci get demoapp.@section[%d].%s", idx, opt_name);
	fp = popen(cmd, "r");

	if (fp) {
		fgets(line, 64, fp);
		len = strlen(line);

		if (line[len - 1] == '\n')
			line[len - 1] = '\0';

		pclose(fp);
	}

	g_strlcpy(opt_val, line, 32);
	return;
}

static GVariant *
demo_interface_get_property(GDBusConnection *connection,
							const gchar *sender,
							const gchar *object_path,
							const gchar *interface_name,
							const gchar *property_name,
							GError **error,
							gpointer user_data)
{
	DemoInterface *self = user_data;

	if (g_strstr_len(property_name, -1, "key")) {
		char val[32] = {0};
		_get_uci_section_val("section", self->number, property_name, val);
		return g_variant_new_string(val);
	} else
		return NULL;
}

static gboolean demo_interface_set_property(GDBusConnection       *connection,
		const gchar           *sender,
		const gchar           *object_path,
		const gchar           *interface_name,
		const gchar           *property_name,
		GVariant              *value,
		GError               **error,
		gpointer               user_data)
{
	DemoInterface *self = user_data;
	char cmd[128] = {0};
	gchar *val;

	if (g_strstr_len(property_name, -1, "key")) {
		g_variant_get(value, "&s", &val);

		sprintf(cmd, "/sbin/uci set demoapp.@section[%d].%s=%s", self->number, property_name, val);
		system(cmd);
		system("/sbin/uci commit demoapp");
	}

	return TRUE;
}

static GDBusInterfaceVTable *
demo_interface_get_vtable(GDBusInterfaceSkeleton *interface)
{
	static GDBusInterfaceVTable vtable = {
		NULL,
		demo_interface_get_property,
		demo_interface_set_property,
	};

	return &vtable;
}

static GVariant *
demo_interface_get_properties(GDBusInterfaceSkeleton *interface)
{
	GVariantBuilder builder;
	GDBusInterfaceInfo *info;
	GDBusInterfaceVTable *vtable;
	guint n;

	info = g_dbus_interface_skeleton_get_info(interface);
	vtable = g_dbus_interface_skeleton_get_vtable(interface);

	g_variant_builder_init(&builder, G_VARIANT_TYPE("a{sv}"));

	for (n = 0; info->properties[n] != NULL; n++) {
		if (info->properties[n]->flags & G_DBUS_PROPERTY_INFO_FLAGS_READABLE) {
			GVariant *value;
			g_return_val_if_fail(vtable->get_property != NULL, NULL);
			value = (vtable->get_property)(g_dbus_interface_skeleton_get_connection(interface), NULL,
										   g_dbus_interface_skeleton_get_object_path(interface),
										   info->name, info->properties[n]->name,
										   NULL, interface);

			if (value != NULL) {
				g_variant_take_ref(value);
				g_variant_builder_add(&builder, "{sv}", info->properties[n]->name, value);
				g_variant_unref(value);
			}
		}
	}

	return g_variant_builder_end(&builder);
}

static void
demo_interface_flush(GDBusInterfaceSkeleton *skeleton)
{
	//do nothing
}

static void
demo_interface_class_init(DemoInterfaceClass *klass)
{
	GDBusInterfaceSkeletonClass *skeleton_class = G_DBUS_INTERFACE_SKELETON_CLASS(klass);
	skeleton_class->get_info = demo_interface_get_info;
	skeleton_class->get_properties = demo_interface_get_properties;
	skeleton_class->flush = demo_interface_flush;
	skeleton_class->get_vtable = demo_interface_get_vtable;
}


static const unsigned char intro_xml[] =
	"<node>"
	"  <interface name='com.upointech.demoapp1.service'>"
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


static int
handle_method_call(const char           *object_path,
				   const char           *interface_name,
				   const char           *method_name,
				   GVariant            *inargs,
				   GVariant            **outargs,
				   void                      *user_data)
{
	//g_print("%s,%d: call %s\n", __func__, __LINE__, method_name);

	if (g_strcmp0(method_name, "Func1") == 0) {
		const gchar *name = NULL;
		char str[64] = {0};

		g_variant_get(inargs, "(&s)", &name);

		sprintf(str, "Hello %s", name);
		*outargs = g_variant_new("(s)", str);
	} else if (g_strcmp0(method_name, "Func2") == 0) {

		GDBusConnection *connection;
		GDBusProxy *proxy;
		GError *error = NULL;
		GVariant *result;
		const gchar *str = NULL;

		connection = g_bus_get_sync(DBUS_TYPE, NULL, &error);
		g_assert_no_error(error);

		proxy = g_dbus_proxy_new_sync(connection,
									  G_DBUS_PROXY_FLAGS_NONE,
									  NULL, 				  /* GDBusInterfaceInfo */
									  "com.upointech.testapp1", /* name */
									  "/com/upointech/testapp1", /* object path */
									  "com.upointech.testapp1.intf2",		  /* interface */
									  NULL, /* GCancellable */
									  &error);
		g_assert_no_error(error);

		result = g_dbus_proxy_call_sync(proxy,
										"Func2",
										NULL,
										G_DBUS_CALL_FLAGS_NONE,
										-1,
										NULL,
										&error);
		g_assert_no_error(error);
		g_assert(result != NULL);
		g_variant_get(result, "(&s)", &str);

		*outargs = g_variant_new("(s)", str);

		g_variant_unref(result);
		g_object_unref(proxy);
		g_object_unref(connection);
	}

	return 0;
}

static const unsigned char config_global_xml[] =
	"<node>"
	"  <interface name='com.upointech.demoapp1.Config.global'>"
	"	 <property type='s' name='360hi' access='readwrite'/>"
	"	 <property type='s' name='hello' access='readwrite'/>"
	"	 <property type='s' name='shit' access='readwrite'/>"
	"  </interface>"
	"</node>";

#if 0
static const unsigned char config_section_xml[] =
	"<node>"
	"  <interface name='com.upointech.demoapp1.Config.section'>"
	"	 <property type='s' name='key1' access='readwrite'/>"
	"	 <property type='s' name='key2' access='read'/>"
	"	 <property type='s' name='key3' access='readwrite'/>"
	"  </interface>"
	"</node>";
#endif

static int _config_handle_method_call(const char *section_name, const char *section_type, int section_idx,
									  const char           *method_name,
									  GVariant           *inargs,
									  GVariant           **outargs)
{
	const gchar *intf, *key, *val;
	GVariant *value;
	char cmd[128] = {0};
	
	//g_print("%s(%d): method %s\n", __func__, __LINE__, method_name);
	
#if 0 //use CtSgwDBusPropGetFunction, then I don't need to handle GetAll

	if (g_strcmp0(method_name, "Get") == 0) {

		char line[64] = {0};
		int len = 0;
		FILE *fp = NULL;

		g_variant_get(inargs, "(&s&s)", &intf, &key);

		if (section_name)
			sprintf(cmd, "uci get demoapp.%s.%s", section_name, key);
		else if (section_type)
			sprintf(cmd, "uci get demoapp.@%s[%d].%s", section_type, section_idx, key);

		fp = popen(cmd, "r");

		if (fp) {
			fgets(line, 64, fp);
			len = strlen(line);

			if (line[len - 1] == '\n')
				line[len - 1] = '\0';

			pclose(fp);
		}

		*outargs = g_variant_new("(v)", g_variant_new("s", line));
	} else
#endif
		if (g_strcmp0(method_name, "Set") == 0) {

			g_variant_get(inargs, "(&s&sv)", &intf, &key, &value);
			g_variant_get(value, "&s", &val);

			if (section_name)
				sprintf(cmd, "/sbin/uci set demoapp.%s.%s=%s", section_name, key, val);
			else if (section_type)
				sprintf(cmd, "/sbin/uci set demoapp.@%s[%d].%s=%s", section_type, section_idx, key, val);

			system(cmd);
			system("/sbin/uci commit demoapp");
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

#if 0
static int
config_section_hmcall(const char           *object_path,
					  const char           *interface_name,
					  const char           *method_name,
					  GVariant           *inargs,
					  GVariant           **outargs,
					  void *user_data)
{
	int i = 0;

	if (user_data)
		i = (int)user_data;

	//fprintf(stderr, "%s %d: %s, i = %d\n", __func__, __LINE__, method_name, i);
	_config_handle_method_call(NULL, "section", i, method_name, inargs, outargs);
	return 0;
}

static GVariant *config_section_propget(const char    *path,
										const char    *prop,
										GVariant     **value,
										void          *userdata)
{
	char line[64] = {0};
	int len = 0;
	FILE *fp = NULL;
	int idx = 0;
	char cmd[128] = {0};

	if (userdata)
		idx = (int)userdata;

	sprintf(cmd, "uci get demoapp.@section[%d].%s", idx, prop);
	fp = popen(cmd, "r");

	if (fp) {
		fgets(line, 64, fp);
		len = strlen(line);

		if (line[len - 1] == '\n')
			line[len - 1] = '\0';

		pclose(fp);
	}

	*value = g_variant_new("(v)", g_variant_new("s", line));
	return *value;
}

#endif

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

	sprintf(cmd, "/sbin/uci get demoapp.global.%s", prop);
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

static CtSgwGDBusInterfaceVTable_t interface_global_vtable = { config_global_hmcall, config_global_propget, NULL, NULL, NULL };
//static CtSgwGDBusInterfaceVTable_t interface_section_vtable = {config_section_hmcall, config_section_propget, NULL, NULL, NULL};

#if 0 //DDBBGG
static gboolean
on_timeout_cb(gpointer user_data)
{

	GDBusConnection *connection;
	GDBusProxy *proxy;
	GError *error = NULL;
	GVariant *result;
	const gchar *str = NULL;

	connection = g_bus_get_sync(DBUS_TYPE, NULL, &error);
	g_assert_no_error(error);

	//fprintf(stderr, "************ call com.upointech.testapp1.intf2.Func2\n");
	proxy = g_dbus_proxy_new_sync(connection,
								  G_DBUS_PROXY_FLAGS_NONE,
								  NULL, 				  /* GDBusInterfaceInfo */
								  "com.upointech.testapp1", /* name */
								  "/com/upointech/testapp1", /* object path */
								  "com.upointech.testapp1.intf2",		  /* interface */
								  NULL, /* GCancellable */
								  &error);
	g_assert_no_error(error);

	result = g_dbus_proxy_call_sync(proxy,
									"Func2",
									NULL,
									G_DBUS_CALL_FLAGS_NONE,
									-1,
									NULL,
									&error);
	g_assert_no_error(error);
	g_assert(result != NULL);
	g_variant_get(result, "(&s)", &str);

	//fprintf(stderr, "************ ret %s\n", str);

	g_variant_unref(result);
	g_object_unref(proxy);
	g_object_unref(connection);

	return TRUE;
}
#endif

static int demoapp_stop(void)
{
	running = 0;
	return 0;
}

static int demoapp_reload(void)
{
	CtSgwLog(LOG_NOTICE, "reloaded\n");
	return 0;
}

static int demoapp_restore(void)
{
	CtSgwLog(LOG_NOTICE, "restore factory\n");
	return 0;
}

static char *demoapp_postmsg(char *msg)
{
	//msg can be any format, as long as caller have the protocol with callee
	g_print("PostMsg: %s\n", msg);
	return "this is msg from demoapp";
}

//static void demo_register_methods(CtSgwDBusNodeInfo_t *intro, CtSgwDBusNodeInfo_t *config_g, CtSgwDBusNodeInfo_t *config_s)
static void demo_register_methods(CtSgwDBusNodeInfo_t *intro, CtSgwDBusNodeInfo_t *config_g)
{
	guint registration_id;
	char buf[128] = {0};

	FILE *fp = NULL;
	int cnt = 0, i = 0;

	fp = popen("/sbin/uci show demoapp | grep '=section' | wc -l", "r");

	if (fp) {
		fgets(buf, sizeof(buf), fp);
		cnt = atoi(buf);
		pclose(fp);
	}

	registration_id = CtSgwDBusRegisterObject("/com/upointech/demoapp1",
					  intro->interfaces[0],
					  &interface_vtable,
					  NULL);
	g_assert(registration_id > 0);

	registration_id = CtSgwDBusRegisterObject("/com/upointech/demoapp1/Config/global/global",
					  config_g->interfaces[0],
					  &interface_global_vtable,
					  NULL);
	g_assert(registration_id > 0);

	//setup response for method: GetManagedObjects
	GDBusObjectManagerServer *manager = NULL;
	GDBusObjectSkeleton *skeleton;
	DemoInterface *demo;

	manager = g_dbus_object_manager_server_new("/com/upointech/demoapp1/Config");

	skeleton = g_dbus_object_skeleton_new("/com/upointech/demoapp1/Config/global/global");
	g_dbus_object_manager_server_export(manager, skeleton);

	for (i = 0; i < cnt; i++) {
		sprintf(buf, "/com/upointech/demoapp1/Config/section/%d", i + 1);
#if 0
		registration_id = CtSgwDBusRegisterObject(buf,
						  config_s->interfaces[0],
						  &interface_section_vtable,
						  (void *)i);
		g_assert(registration_id > 0);
#endif
		skeleton = g_dbus_object_skeleton_new(buf);
		demo = g_object_new(demo_interface_get_type(), NULL);
		demo->number = i;
		g_dbus_object_skeleton_add_interface(skeleton, G_DBUS_INTERFACE_SKELETON(demo));
		g_dbus_object_manager_server_export(manager, skeleton);
	}

	g_dbus_object_manager_server_set_connection(manager, sgw_dbus_service_context->connection);

	//DDBBGG
	//g_timeout_add_seconds (120, on_timeout_cb, sgw_dbus_service_context->connection);
}

int main(int argc, char *argv[])
{
	CtSgwDBusNodeInfo_t *intro_data = NULL;
	CtSgwDBusNodeInfo_t *config_g_data = NULL;
	struct demo_list mylist;
	struct demo_list *it,*next;
	int ret = 0;

	CtSgwDBusNodeInfo_t *mm_data = NULL;
	CtSgwAppMgtCallbacks_t app_cbs = {
		.stop = demoapp_stop,
		.reload = demoapp_reload,
		.restore_factory = demoapp_restore,
		.post_msg = demoapp_postmsg
	};

	CtSgwLogOpen(LOG_USER, "demoapp");
	CtSgwLog(LOG_NOTICE, "start\n");

	//test if /opt/apps/demoapp/files/libdemo.so is available
	demo_list_init(&mylist);	
	//CtSgwLog(LOG_NOTICE, "after list init, close log\n");
	//CtSgwLogClose();
	//CtSgwLog(LOG_NOTICE, "You can not see this in logread\n");

	char *msg = NULL;

	msg = CtSgwPostMsg("testapp", "hello testapp");
	if (msg) {
		CtSgwLog(LOG_NOTICE, "testapp retmsg: %s", msg);
		g_free(msg);
	}

	demo_list_for_each_safe(it, &mylist, next) {
		demo_list_del(it);
		free(it->elem);
		free(it);
	}

	ret = CtSgwDBusStartService("com.upointech.demoapp1");
	if (ret < 0) {
		CtSgwLog(LOG_NOTICE, "failed to start dbus service\n");
		CtSgwLogClose();	
		return ret;
	}	

	intro_data = CtSgwDBusNodeParseXml(intro_xml);
	config_g_data = CtSgwDBusNodeParseXml(config_global_xml);

	demo_register_methods(intro_data, config_g_data);

	mm_data = CtSgwAppRegisterMgtFuncs(&app_cbs);

	//g_print("DEMOAPP Running\n");
	ret = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
	CtSgwLog(LOG_NOTICE, "socket ret %d, %s\n", ret, strerror(errno));

	while (running) {
		sleep(2);
	}

	//g_print("DEMOAPP Exiting\n");

	CtSgwDBusNodeInfoUnref(intro_data);
	CtSgwDBusNodeInfoUnref(config_g_data);
	CtSgwDBusNodeInfoUnref(mm_data);
	CtSgwDBusStopService();

	CtSgwLog(LOG_NOTICE, "exit\n");
	CtSgwLogClose();

	return 0;
}

