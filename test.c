#include <tizen.h>
#include <app.h>
#include <Evas.h>
#include <Elementary.h>
#include <assert.h>
#include <dlog.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <account.h>
#include <account-types.h>
#include <account-error.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <bundle.h>
#include <sync_manager.h>
#include <sync-error.h>
#include "sync-adapter-app.h"


#define SYNC_ADAPTER_SERVICE_APP_ID "org.tizen.syncadapterapp.service"
#define PKG_NAME "org.tizen.syncadapterapp"
#define TIME_FOR_POPUP 2
#define NUM_OF_ITEMS 4
#define NUM_OF_CAPABILITY 6
#define MAX_NUM 10
#define MAX_SIZE 50
#define LOG_TAG "syncadapterapp"

account_h account;
bool is_accountless = false;
int account_id = -1;
int cnt_sync_jobs = 0;

char displayed_job[MAX_SIZE] = "On Demand Sync";
char displayed_interval[MAX_SIZE] = "every 30 minutes";
char displayed_capability[MAX_SIZE] = "Calendar";
char displayed_option[MAX_SIZE] = "Option None";

char popup_loc[MAX_SIZE];

sync_period_e sync_interval = SYNC_PERIOD_INTERVAL_30MIN;
char *sync_capability = SYNC_SUPPORTS_CAPABILITY_CALENDAR;
sync_option_e sync_option = SYNC_OPTION_NONE;

int on_demand_sync_job_id = -1;
int periodic_sync_job_id = -1;
int data_change_sync_job_id = -1;
int data_change_id[NUM_OF_CAPABILITY] = {-1, };

char list_of_sync_jobs[MAX_NUM][MAX_SIZE];
bool ini = false;
bool is_all_checked = false;
int remove_sync_job[MAX_NUM] = {-1, };
bool list_of_remove_sync_job[MAX_NUM] = {false, };

static Evas_Object *ctxpopup = NULL;

typedef struct appdata {
	Evas_Object *win;
	Evas_Object *layout;
	Evas_Object *conform;
	Evas_Object *nf;
} appdata_s;

appdata_s *appData;

typedef struct viewdata {
	Evas_Object *win;
	Evas_Object *nf;
	Evas_Object *layout;
	Evas_Object *list;
	Evas_Object *syncBtn;
	Evas_Object *selectAllBtn;
	Evas_Object *removeBtn;
	int account_id;
	const char *capability;
	long frequency;
} viewdata_s;

typedef struct list_item_data {
	int index;
	int type;
	Elm_Object_Item *item;
	Eina_Bool expanded;
} list_item_data_s;

list_item_data_s *main_id;
list_item_data_s *setting_id;
list_item_data_s *data_id;
list_item_data_s *foreach_id;

struct MemoryStruct {
	char *memory;
	size_t size;
};

char *pageContents;
Evas_Object *list = NULL;
Evas_Object *NF;
int clear_flag = 0;

bool SYNCFLAG = false;

static Evas_Object *manualObject;


static void
win_delete_request_cb(void *data , Evas_Object *obj , void *event_info)
{
	ui_app_exit();
}


bool query_account_cb(account_h account, void *user_data)
{
	account_get_account_id(account, &account_id);
	return false;
}


static Eina_Bool
sync_job_naviframe_pop_cb(void *data, Elm_Object_Item *it)
{
	dlog_print(DLOG_INFO, LOG_TAG, "Return to previous view");

	if (ctxpopup != NULL) {
		evas_object_del(ctxpopup);
		ctxpopup = NULL;
	}

	return EINA_TRUE;
}


static Eina_Bool
foreach_naviframe_pop_cb(void *data, Elm_Object_Item *it)
{
	memset(list_of_sync_jobs, '\0', sizeof(list_of_sync_jobs));
	cnt_sync_jobs = 0;

	if (ctxpopup != NULL) {
		evas_object_del(ctxpopup);
		ctxpopup = NULL;
	}

	return EINA_TRUE;
}


static Eina_Bool
naviframe_pop_cb(void *data, Elm_Object_Item *it)
{
	if (ctxpopup != NULL) {
		evas_object_del(ctxpopup);
		ctxpopup = NULL;
	}

	ui_app_exit();
	return EINA_FALSE;
}


static void
app_get_resource(const char *edj_file_in, char *edj_path_out, int edj_path_max)
{
	char *res_path = app_get_resource_path();
	if (res_path) {
		snprintf(edj_path_out, edj_path_max, "%s%s", res_path, edj_file_in);
		free(res_path);
	}
}


static void
genlist_selected_cb(void *data, Evas_Object *obj, void *event_info)
{
	Elm_Object_Item *it = event_info;
	Eina_Bool expanded = EINA_FALSE;

	elm_genlist_item_selected_set(it, EINA_FALSE);

	expanded = elm_genlist_item_expanded_get(it);
	elm_genlist_item_expanded_set(it, !expanded);
}


static char*
get_text_main_menu(void *data, Evas_Object *obj, const char *part)
{
	main_id = (list_item_data_s *)data;

	switch (main_id->index) {
	case 0:
		if (!strcmp("elm.text", part))
			return strdup("Create an account");
		else
			return NULL;
	case 1:
		if (!strcmp("elm.text", part))
			return strdup("Set as accountless");
		else
			return NULL;
	case 2:
		if (!strcmp("elm.text", part))
			return strdup("Start sync with settings");
		else
			return NULL;
	case 3:
		if (!strcmp("elm.text", part))
			return strdup("Manage sync jobs");
		else
			return NULL;
	}

	return NULL;
}


static char*
get_text_data_change(void *data, Evas_Object *obj, const char *part)
{
	data_id = (list_item_data_s *)data;

	switch (data_id->index) {
	case 0:
		if (!strcmp("elm.text", part))
			return strdup("1. Press home button");
		else
			return NULL;
	case 1:
		if (!strcmp("elm.text", part))
			return strdup("2. Change data in your device");
		else
			return NULL;
	case 2:
		if (!strcmp("elm.text", part))
			return strdup("3. Pop up will come after change");
		else
			return NULL;
	case 3:
		if (!strcmp("elm.text", part))
			return strdup("4. Check the pop up message");
		else
			return NULL;
	}

	return NULL;
}


static char*
get_text_setting_menu(void *data, Evas_Object *obj, const char *part)
{
	setting_id = (list_item_data_s *)data;

	char buf[1024];

	switch (setting_id->index) {
	case 0:
		if (!strcmp("elm.text", part))
			return strdup("Sync types");
		else if (!strcmp("elm.text.multiline", part)) {
			char *sync_job_desc = "It is for selecting kind of sync job";
			snprintf(buf, sizeof(buf), "<font color=#3DB9CCFF>%s</font><br>%s", displayed_job, sync_job_desc);
			return strdup(buf);
		}
		break;
	case 1:
		if (!strcmp("elm.text", part))
			return strdup("Period interval types");
		else if (!strcmp("elm.text.multiline", part)) {
			if (!strcmp(displayed_job, "Periodic Sync")) {
				char *sync_period_desc = "It is for selecting kind of sync interval";
				snprintf(buf, sizeof(buf), "<font color=#3DB9CCFF>%s</font><br>%s", displayed_interval, sync_period_desc);
			} else {
				char *sync_period_desc = "It can be selected only for Periodic Sync";
				snprintf(buf, sizeof(buf), "<font color=#CCCCCCFF>%s</font><br>%s", displayed_interval, sync_period_desc);
			}
			return strdup(buf);
		}
		break;
	case 2:
		if (!strcmp("elm.text", part))
			return strdup("Capability types");
		else if (!strcmp("elm.text.multiline", part)) {
			if (!strcmp(displayed_job, "Data Change Sync")) {
				char *sync_capability_desc = "It is for selecting kind of sync capability";
				snprintf(buf, sizeof(buf), "<font color=#3DB9CCFF>%s</font><br>%s", displayed_capability, sync_capability_desc);
			} else {
				char *sync_capability_desc = "It can be selected only for Data Change Sync";
				snprintf(buf, sizeof(buf), "<font color=#CCCCCCFF>%s</font><br>%s", displayed_capability, sync_capability_desc);
			}
			return strdup(buf);
		}
		break;
	case 3:
		if (!strcmp("elm.text", part))
			return strdup("Option types");
		else if (!strcmp("elm.text.multiline", part)) {
			char *sync_option_desc = "It is for selecting kind of sync option";
			snprintf(buf, sizeof(buf), "<font color=#3DB9CCFF>%s</font><br>%s", displayed_option, sync_option_desc);
			return strdup(buf);
		}
		break;
	}

	return NULL;
}


static void
gl_del_cb(void *data, Evas_Object *obj)
{
	list_item_data_s *lid = data;
	free(lid);
}


void
notify_by_using_popup(void *data, Evas_Object *obj, void *event_info)
{
	dlog_print(DLOG_INFO, LOG_TAG, "Sync Adapter App: select account mode first - create an account or use accountless mode");

	Evas_Object *nf = data, *popup;
	Evas_Object *win = nf;

	popup = elm_popup_add(win);
	elm_popup_align_set(popup, ELM_NOTIFY_ALIGN_FILL, 1.0);
	eext_object_event_callback_add(popup, EEXT_CALLBACK_BACK, eext_popup_back_cb, NULL);
	evas_object_size_hint_weight_set(popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

	elm_object_text_set(popup, "Select account mode first");

	elm_popup_timeout_set(popup, TIME_FOR_POPUP);
	evas_object_show(popup);
}


static void
cb_add_on_demand_sync(void *pData, Evas_Object *pObj, void *pEvent_info)
{
	dlog_print(DLOG_INFO, LOG_TAG, "Sync Adapter App: Request manual sync");

	viewdata_s* viewData = (viewdata_s *)pData;

	int ret = SYNC_ERROR_NONE;
	if (is_accountless) {
		dlog_print(DLOG_INFO, LOG_TAG, "Sync Adapter App: request accountless On Demand Sync");
		ret = sync_manager_on_demand_sync_job(NULL, "OnDemand", sync_option, NULL, &on_demand_sync_job_id);
	} else {
		dlog_print(DLOG_INFO, LOG_TAG, "Sync Adapter App: request On Demand Sync with an account");
		account_query_account_by_user_name(query_account_cb, "dummy_user", NULL);

		account_h account;
		account_create(&account);
		account_query_account_by_account_id(account_id, &account);

		viewData->account_id = account_id;

		ret = sync_manager_on_demand_sync_job(account, "OnDemand", sync_option, NULL, &on_demand_sync_job_id);

		account_destroy(account);
	}

	if (ret != SYNC_ERROR_NONE)
		dlog_print(DLOG_INFO, LOG_TAG, "Sync manager failed with error code %d", ret);
	else
		dlog_print(DLOG_INFO, LOG_TAG, "sync manager added on demand sync id %d", on_demand_sync_job_id);

	dlog_print(DLOG_INFO, LOG_TAG, "Sync Adapter App: Exit the cb_add_on_demand_sync()");
}


static void
cb_add_periodic_sync(void *pData, Evas_Object *pObj, void *pEvent_info)
{
	dlog_print(DLOG_INFO, LOG_TAG, "Sync Adapter App: Request periodic sync");

	viewdata_s* viewData = (viewdata_s *)pData;
	int ret = SYNC_ERROR_NONE;

	if (is_accountless) {
		dlog_print(DLOG_INFO, LOG_TAG, "Sync Adapter App: request accountless Periodic Sync");
		ret = sync_manager_add_periodic_sync_job(NULL, "Periodic", sync_interval, sync_option, NULL, &periodic_sync_job_id);
	} else {
		dlog_print(DLOG_INFO, LOG_TAG, "Sync Adapter App: request Periodic Sync with an account");
		account_query_account_by_user_name(query_account_cb, "dummy_user", NULL);

		account_h account;
		account_create(&account);
		account_query_account_by_account_id(account_id, &account);

		viewData->account_id = account_id;

		ret = sync_manager_add_periodic_sync_job(account, "Periodic", sync_interval, sync_option, NULL, &periodic_sync_job_id);

		account_destroy(account);
	}

	if ((sync_option == SYNC_OPTION_NONE) | (sync_option == SYNC_OPTION_NO_RETRY)) {
		Evas_Object *popup;
		Evas_Object *win = NF;

		popup = elm_popup_add(win);
		elm_popup_align_set(popup, ELM_NOTIFY_ALIGN_FILL, 1.0);
		eext_object_event_callback_add(popup, EEXT_CALLBACK_BACK, eext_popup_back_cb, NULL);
		evas_object_size_hint_weight_set(popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

		if (sync_interval == SYNC_PERIOD_INTERVAL_30MIN)
			elm_object_text_set(popup, "It is expected in 30mins");
		else if (sync_interval == SYNC_PERIOD_INTERVAL_1H)
			elm_object_text_set(popup, "It is expected in an hour");
		else if (sync_interval == SYNC_PERIOD_INTERVAL_2H)
			elm_object_text_set(popup, "It is expected in 2 hours");
		else if (sync_interval == SYNC_PERIOD_INTERVAL_3H)
			elm_object_text_set(popup, "It is expected in 3 hours");
		else if (sync_interval == SYNC_PERIOD_INTERVAL_6H)
			elm_object_text_set(popup, "It is expected in 6 hours");
		else if (sync_interval == SYNC_PERIOD_INTERVAL_12H)
			elm_object_text_set(popup, "It is expected in 12 hours");
		else
			elm_object_text_set(popup, "It is expected in a day");

		elm_popup_timeout_set(popup, TIME_FOR_POPUP);
		evas_object_show(popup);
	}

	if (ret != SYNC_ERROR_NONE)
		dlog_print(DLOG_INFO, LOG_TAG, "Sync manager failed with error code %d", ret);
	else
		dlog_print(DLOG_INFO, LOG_TAG, "sync manager added periodic sync id %d", periodic_sync_job_id);

	dlog_print(DLOG_INFO, LOG_TAG, "Sync Adapter App: Exit the cb_add_periodic_sync()");
}


static void
cb_add_data_change_sync(void *pData, Evas_Object *pObj, void *pEvent_info)
{
	dlog_print(DLOG_INFO, LOG_TAG, "Sync Adapter App: Request data change sync");

	viewdata_s* viewData = (viewdata_s *)pData;
	int ret = SYNC_ERROR_NONE;
	int idx = 0;

	if (is_accountless) {
		dlog_print(DLOG_INFO, LOG_TAG, "Sync Adapter App: request accountless Data Change Sync");

		for (idx = 0; idx < NUM_OF_CAPABILITY; idx++) {
			if (data_change_id[idx] == -1) {
				ret = sync_manager_add_data_change_sync_job(NULL, sync_capability, sync_option, NULL, &data_change_sync_job_id);
				data_change_id[idx] = data_change_sync_job_id;
				dlog_print(DLOG_INFO, LOG_TAG, "[accountless] restored data_change_id[%d] = %d", idx, data_change_id[idx]);
				break;
			} else {
				if (idx == NUM_OF_CAPABILITY-1) {
					dlog_print(DLOG_INFO, LOG_TAG, "data_change_id[idx] is full");
					break;
				}
				continue;
			}
		}
	} else {
		dlog_print(DLOG_INFO, LOG_TAG, "Sync Adapter App: request Data Change Sync with an account");
		account_query_account_by_user_name(query_account_cb, "dummy_user", NULL);

		account_h account;
		account_create(&account);
		account_query_account_by_account_id(account_id, &account);

		viewData->account_id = account_id;

		for (idx = 0; idx < NUM_OF_CAPABILITY; idx++) {
			if (data_change_id[idx] == -1) {
				ret = sync_manager_add_data_change_sync_job(account, sync_capability, sync_option, NULL, &data_change_sync_job_id);
				data_change_id[idx] = data_change_sync_job_id;
				dlog_print(DLOG_INFO, LOG_TAG, "[account] restored data_change_id[%d] = %d", idx, data_change_id[idx]);
				break;
			} else {
				if (idx == NUM_OF_CAPABILITY-1) {
					dlog_print(DLOG_INFO, LOG_TAG, "data_change_id[idx] is full");
					break;
				}
				continue;
			}
		}
		account_destroy(account);
	}

	if (ret != SYNC_ERROR_NONE)
		dlog_print(DLOG_INFO, LOG_TAG, "Sync manager failed with error code %d", ret);
	else {
		if (data_change_sync_job_id != -1)
			dlog_print(DLOG_INFO, LOG_TAG, "sync manager added data change sync id %d", data_change_sync_job_id);
	}

	dlog_print(DLOG_INFO, LOG_TAG, "Sync Adapter App: Exit the cb_add_data_change_sync()");
}


static void
cb_start_sync(void *data, Evas_Object *obj, void *event_info)
{
	viewdata_s* viewData = (viewdata_s *)malloc(sizeof(viewdata_s));

	if (!strcmp(displayed_job, "On Demand Sync")) {
		cb_add_on_demand_sync(viewData, NULL, NULL);
	} else if (!strcmp(displayed_job, "Periodic Sync")) {
		cb_add_periodic_sync(viewData, NULL, NULL);
	} else {
		Elm_Object_Item *nf_it;
		char edj_path[PATH_MAX] = {0, };
		Evas_Object *nf = data, *layout, *win;
		viewData->nf = nf;
		layout = elm_layout_add(nf);
		app_get_resource(EDJ_FILE, edj_path, (int)PATH_MAX);
		elm_layout_file_set(layout, edj_path, GRP_MAIN);

		viewData->layout = layout;
		win = nf;

		clear_flag = 1;

		viewData->frequency = -1;
		manualObject = layout;

		Evas_Object *genlist;
		Elm_Object_Item *it;
		Elm_Genlist_Item_Class *itc;
		int idx;

		itc = elm_genlist_item_class_new();
		itc->item_style = "type1";
		itc->func.text_get = get_text_data_change;
		itc->func.del = gl_del_cb;

		genlist = elm_genlist_add(nf);
		elm_genlist_block_count_set(genlist, 14);
		elm_genlist_mode_set(genlist, ELM_LIST_COMPRESS);
		elm_genlist_homogeneous_set(genlist, EINA_TRUE);

		for (idx = 0; idx < NUM_OF_ITEMS; idx++) {
			data_id = calloc(sizeof(list_item_data_s), 1);
			data_id->index = idx;

			it = elm_genlist_item_append(genlist, itc, data_id, NULL, ELM_GENLIST_ITEM_NONE, NULL, nf);

			data_id->item = it;
		}

		evas_object_smart_callback_add(genlist, "selected", genlist_selected_cb, NULL);
		elm_genlist_item_class_free(itc);

		NF = nf;

		nf_it = elm_naviframe_item_push(nf, "Data Change Sync", NULL, NULL, genlist, NULL);
		cb_add_data_change_sync(viewData, NULL, NULL);

		elm_naviframe_item_pop_cb_set(nf_it, sync_job_naviframe_pop_cb, viewData);
	}
}


static void
create_account()
{
	dlog_print(DLOG_INFO, LOG_TAG, "Creating dummy account");

	account_id = 10;

	int ret = account_create(&account);
	dlog_print(DLOG_INFO, LOG_TAG, "account_create = %d", ret);

	ret = account_set_user_name(account, "dummy_user");
	dlog_print(DLOG_INFO, LOG_TAG, "account_set_user_name = %d", ret);

	ret = account_set_email_address(account, "dummy_user@syncadapterapp.com");
	dlog_print(DLOG_INFO, LOG_TAG, "account_set_email_address = %d", ret);

	ret = account_set_capability(account, ACCOUNT_SUPPORTS_CAPABILITY_CALENDAR, ACCOUNT_CAPABILITY_ENABLED);
	dlog_print(DLOG_INFO, LOG_TAG, "account_set_capability = %d", ret);

	ret = account_set_capability(account, ACCOUNT_SUPPORTS_CAPABILITY_CONTACT, ACCOUNT_CAPABILITY_ENABLED);
	dlog_print(DLOG_INFO, LOG_TAG, "account_set_capability = %d", ret);

	ret = account_set_sync_support(account, ACCOUNT_SYNC_STATUS_IDLE);
	dlog_print(DLOG_INFO, LOG_TAG, "account_set_sync_support = %d", ret);

	ret = account_insert_to_db(account, &account_id);
	dlog_print(DLOG_INFO, LOG_TAG, "Account id is = %d", account_id);

	is_accountless = false;
}


void
on_create_account_sync_cb(void *data, Evas_Object *obj, void *event_info)
{
	dlog_print(DLOG_INFO, LOG_TAG, "Enter the create_account");

	Evas_Object *nf = data, *popup;
	Evas_Object *win = nf;

	popup = elm_popup_add(win);
	elm_popup_align_set(popup, ELM_NOTIFY_ALIGN_FILL, 1.0);
	eext_object_event_callback_add(popup, EEXT_CALLBACK_BACK, eext_popup_back_cb, NULL);
	evas_object_size_hint_weight_set(popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

	if (account_id == -1) {
		create_account();
		elm_object_text_set(popup, "Account creation completed");
	} else
		elm_object_text_set(popup, "Account is already created");

	elm_popup_timeout_set(popup, TIME_FOR_POPUP);
	evas_object_show(popup);

	dlog_print(DLOG_INFO, LOG_TAG, "created account Id [%d]", account_id);
}


static void
set_accountless()
{
	dlog_print(DLOG_INFO, LOG_TAG, "Set accountless");

	int ret = ACCOUNT_ERROR_NONE;

	ret = account_delete_from_db_by_package_name(PKG_NAME);
	if (ret == ACCOUNT_ERROR_NONE) {
		dlog_print(DLOG_INFO, LOG_TAG, "account delete success");
		if (account) {
			ret = account_destroy(account);
			if (ret == ACCOUNT_ERROR_NONE)
			dlog_print(DLOG_INFO, LOG_TAG, "account handle is removed successfully");
		}
	}

	account_id = -1;
	is_accountless = true;

	dlog_print(DLOG_INFO, LOG_TAG, "Dummy account is set again as = %d", account_id);
}


void
on_select_accountless_sync_cb(void *data, Evas_Object *obj, void *event_info)
{
	dlog_print(DLOG_INFO, LOG_TAG, "Enter the select_accountless");

	Evas_Object *nf = data, *popup;
	Evas_Object *win = nf;

	popup = elm_popup_add(win);
	elm_popup_align_set(popup, ELM_NOTIFY_ALIGN_FILL, 1.0);
	eext_object_event_callback_add(popup, EEXT_CALLBACK_BACK, eext_popup_back_cb, NULL);
	evas_object_size_hint_weight_set(popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

	if (!is_accountless) {
		set_accountless();
		elm_object_text_set(popup, "Setting accountless completed");
	} else
		elm_object_text_set(popup, "Accountless is already set");

	elm_popup_timeout_set(popup, TIME_FOR_POPUP);
	evas_object_show(popup);

	dlog_print(DLOG_INFO, LOG_TAG, "set accountless complete");
}


void on_select_settings_sync_cb(void *data, Evas_Object *obj, void *event_info);

static void
locate_ctxpopup(Evas_Object *ctxpopup, Evas_Object *obj)
{
	dlog_print(DLOG_INFO, LOG_TAG, "Enter locate_ctxpopup()");

	Evas_Coord x, y, w, h;
	evas_object_geometry_get(obj, NULL, NULL, &w, &h);

	if (!strcmp(popup_loc, "sync job"))
		x = 0, y = 100;
	else if (!strcmp(popup_loc, "sync interval"))
		x = 175, y = 150;
	else if (!strcmp(popup_loc, "sync capability"))
		x = 135, y = 200;
	else
		x = 0, y = 500;

	evas_object_move(ctxpopup, x + (w / 2), y + (h / 2));
}


static void
ctxpopup_dismissed_cb(void *data, Evas_Object *obj, void *event_info)
{
	dlog_print(DLOG_INFO, LOG_TAG, "ctxpopup_dismissed_cb");
	evas_object_del(ctxpopup);
	ctxpopup = NULL;
}


static void
select_job_cb(void *data, Evas_Object *obj, void *event_info)
{
	Evas_Object *nf = (Evas_Object *)data;

	const char *txt = elm_object_item_text_get(event_info);
	if (txt)
		dlog_print(DLOG_INFO, LOG_TAG, "text(%s) is clicked\n", txt);

	snprintf(displayed_job, 50, "%s", txt);

	elm_naviframe_item_pop(nf);
	on_select_settings_sync_cb(data, obj, event_info);
}

static void
select_period_cb(void *data, Evas_Object *obj, void *event_info)
{
	Evas_Object *nf = (Evas_Object *)data;

	const char *txt = elm_object_item_text_get(event_info);
	if (txt)
		dlog_print(DLOG_INFO, LOG_TAG, "text(%s) is clicked\n", txt);

	snprintf(displayed_interval, 50, "%s", txt);
	if (!strcmp("every 30 minutes", displayed_interval))
		sync_interval = SYNC_PERIOD_INTERVAL_30MIN;
	else if (!strcmp("every an hour", displayed_interval))
		sync_interval = SYNC_PERIOD_INTERVAL_1H;
	else if (!strcmp("every 2 hours", displayed_interval))
		sync_interval = SYNC_PERIOD_INTERVAL_2H;
	else if (!strcmp("every 3 hours", displayed_interval))
		sync_interval = SYNC_PERIOD_INTERVAL_3H;
	else if (!strcmp("every 6 hours", displayed_interval))
		sync_interval = SYNC_PERIOD_INTERVAL_6H;
	else if (!strcmp("every 12 hours", displayed_interval))
		sync_interval = SYNC_PERIOD_INTERVAL_12H;
	else
		sync_interval = SYNC_PERIOD_INTERVAL_1DAY;

	elm_naviframe_item_pop(nf);
	on_select_settings_sync_cb(data, obj, event_info);
}


static void
select_capability_cb(void *data, Evas_Object *obj, void *event_info)
{
	Evas_Object *nf = (Evas_Object *)data;

	const char *txt = elm_object_item_text_get(event_info);
	if (txt)
		dlog_print(DLOG_INFO, LOG_TAG, "text(%s) is clicked\n", txt);

	snprintf(displayed_capability, 50, "%s", txt);
	if (!strcmp("Calendar", displayed_capability))
		sync_capability = SYNC_SUPPORTS_CAPABILITY_CALENDAR;
	else if (!strcmp("Contact", displayed_capability))
		sync_capability = SYNC_SUPPORTS_CAPABILITY_CONTACT;
	else if (!strcmp("Image", displayed_capability))
		sync_capability = SYNC_SUPPORTS_CAPABILITY_IMAGE;
	else if (!strcmp("Music", displayed_capability))
		sync_capability = SYNC_SUPPORTS_CAPABILITY_MUSIC;
	else if (!strcmp("Sound", displayed_capability))
		sync_capability = SYNC_SUPPORTS_CAPABILITY_SOUND;
	else
		sync_capability = SYNC_SUPPORTS_CAPABILITY_VIDEO;

	elm_naviframe_item_pop(nf);
	on_select_settings_sync_cb(data, obj, event_info);
}


static void
select_option_cb(void *data, Evas_Object *obj, void *event_info)
{
	Evas_Object *nf = (Evas_Object *)data;

	const char *txt = elm_object_item_text_get(event_info);
	if (txt)
		dlog_print(DLOG_INFO, LOG_TAG, "text(%s) is clicked\n", txt);

	snprintf(displayed_option, 50, "%s", txt);
	if (!strcmp("Option None", displayed_option))
		sync_option = SYNC_OPTION_NONE;
	else if (!strcmp("Sync with Priority", displayed_option))
		sync_option = SYNC_OPTION_EXPEDITED;
	else if (!strcmp("Sync Once", displayed_option))
		sync_option = SYNC_OPTION_NO_RETRY;
	else
		sync_option = SYNC_OPTION_EXPEDITED | SYNC_OPTION_NO_RETRY;

	elm_naviframe_item_pop(nf);
	on_select_settings_sync_cb(data, obj, event_info);
}


void
on_select_sync_jobs_cb(void *data, Evas_Object *obj, void *event_info)
{
	dlog_print(DLOG_INFO, LOG_TAG, "Sync job");

	Evas_Object *nf = data;
	evas_object_del(ctxpopup);

	ctxpopup = elm_ctxpopup_add(nf);
	eext_object_event_callback_add(ctxpopup, EEXT_CALLBACK_BACK, eext_ctxpopup_back_cb, NULL);
	evas_object_smart_callback_add(ctxpopup, "dismissed", ctxpopup_dismissed_cb, NULL);

	elm_ctxpopup_item_append(ctxpopup, "On Demand Sync", NULL, select_job_cb, nf);
	elm_ctxpopup_item_append(ctxpopup, "Periodic Sync", NULL, select_job_cb, nf);
	elm_ctxpopup_item_append(ctxpopup, "Data Change Sync", NULL, select_job_cb, nf);

	snprintf(popup_loc, MAX_SIZE, "sync job");

	locate_ctxpopup(ctxpopup, obj);
	evas_object_show(ctxpopup);
}


void
on_select_sync_period_cb(void *data, Evas_Object *obj, void *event_info)
{
	dlog_print(DLOG_INFO, LOG_TAG, "Sync period");

	if (!strcmp(displayed_job, "On Demand Sync") | !strcmp(displayed_job, "Data Change Sync")) {

	} else {
		Evas_Object *nf = data;
		evas_object_del(ctxpopup);

		ctxpopup = elm_ctxpopup_add(nf);
		eext_object_event_callback_add(ctxpopup, EEXT_CALLBACK_BACK, eext_ctxpopup_back_cb, NULL);
		evas_object_smart_callback_add(ctxpopup, "dismissed", ctxpopup_dismissed_cb, NULL);

		elm_ctxpopup_item_append(ctxpopup, "every 30 minutes", NULL, select_period_cb, nf);
		elm_ctxpopup_item_append(ctxpopup, "every an hour", NULL, select_period_cb, nf);
		elm_ctxpopup_item_append(ctxpopup, "every 2 hours", NULL, select_period_cb, nf);
		elm_ctxpopup_item_append(ctxpopup, "every 3 hours", NULL, select_period_cb, nf);
		elm_ctxpopup_item_append(ctxpopup, "every 6 hours", NULL, select_period_cb, nf);
		elm_ctxpopup_item_append(ctxpopup, "every 12 hours", NULL, select_period_cb, nf);
		elm_ctxpopup_item_append(ctxpopup, "every a day", NULL, select_period_cb, nf);

		snprintf(popup_loc, MAX_SIZE, "sync interval");

		locate_ctxpopup(ctxpopup, obj);
		evas_object_show(ctxpopup);
	}
}


void
on_select_sync_capability_cb(void *data, Evas_Object *obj, void *event_info)
{
	dlog_print(DLOG_INFO, LOG_TAG, "Sync capa");

	if (!strcmp(displayed_job, "On Demand Sync") | !strcmp(displayed_job, "Periodic Sync")) {

	} else {
		Evas_Object *nf = data;
		evas_object_del(ctxpopup);

		ctxpopup = elm_ctxpopup_add(nf);
		eext_object_event_callback_add(ctxpopup, EEXT_CALLBACK_BACK, eext_ctxpopup_back_cb, NULL);
		evas_object_smart_callback_add(ctxpopup, "dismissed", ctxpopup_dismissed_cb, NULL);

		elm_ctxpopup_item_append(ctxpopup, "Calendar", NULL, select_capability_cb, nf);
		elm_ctxpopup_item_append(ctxpopup, "Contact", NULL, select_capability_cb, nf);
		elm_ctxpopup_item_append(ctxpopup, "Image", NULL, select_capability_cb, nf);
		elm_ctxpopup_item_append(ctxpopup, "Music", NULL, select_capability_cb, nf);
		elm_ctxpopup_item_append(ctxpopup, "Sound", NULL, select_capability_cb, nf);
		elm_ctxpopup_item_append(ctxpopup, "Video", NULL, select_capability_cb, nf);

		snprintf(popup_loc, MAX_SIZE, "sync capability");

		locate_ctxpopup(ctxpopup, obj);
		evas_object_show(ctxpopup);
	}
}


void
on_select_sync_option_cb(void *data, Evas_Object *obj, void *event_info)
{
	dlog_print(DLOG_INFO, LOG_TAG, "Sync option");

	Evas_Object *nf = data;
	evas_object_del(ctxpopup);

	ctxpopup = elm_ctxpopup_add(nf);
	eext_object_event_callback_add(ctxpopup, EEXT_CALLBACK_BACK, eext_ctxpopup_back_cb, NULL);
	evas_object_smart_callback_add(ctxpopup, "dismissed", ctxpopup_dismissed_cb, NULL);

	elm_ctxpopup_item_append(ctxpopup, "Option None", NULL, select_option_cb, nf);
	elm_ctxpopup_item_append(ctxpopup, "Sync with Priority", NULL, select_option_cb, nf);
	elm_ctxpopup_item_append(ctxpopup, "Sync Once", NULL, select_option_cb, nf);
	elm_ctxpopup_item_append(ctxpopup, "Sync Once with Priority", NULL, select_option_cb, nf);

	snprintf(popup_loc, MAX_SIZE, "sync option");

	locate_ctxpopup(ctxpopup, obj);
	evas_object_show(ctxpopup);
}


void
on_select_settings_sync_cb(void *data, Evas_Object *obj, void *event_info)
{
	dlog_print(DLOG_INFO, LOG_TAG, "Enter the Start sync with settings");

	Elm_Object_Item *nf_it, *it;
	viewdata_s* viewData = (viewdata_s *)malloc(sizeof(viewdata_s));
	Evas_Object *nf = data, *layout, *win, *syncBtn, *genlist;
	viewData->nf = nf;
	Elm_Genlist_Item_Class *itc;
	layout = elm_layout_add(nf);
	char edj_path[PATH_MAX] = {0, };
	app_get_resource(EDJ_FILE, edj_path, (int)PATH_MAX);
	elm_layout_file_set(layout, edj_path, GRP_MAIN);

	viewData->layout = layout;
	win = nf;

	genlist = elm_genlist_add(nf);
	elm_genlist_block_count_set(genlist, 14);
	elm_genlist_mode_set(genlist, ELM_LIST_COMPRESS);
	elm_genlist_homogeneous_set(genlist, EINA_TRUE);
	elm_object_part_content_set(layout, "genlist_tiny1", genlist);

	itc = elm_genlist_item_class_new();
	itc->item_style = "multiline";
	itc->func.text_get = get_text_setting_menu;
	itc->func.del = gl_del_cb;

	int idx;

	for (idx = 0; idx < NUM_OF_ITEMS; idx++) {
		setting_id = calloc(sizeof(list_item_data_s), 1);
		setting_id->type = 3;
		setting_id->index = idx;

		switch (idx) {
		case 0:
			it = elm_genlist_item_append(genlist, itc, setting_id, NULL, ELM_GENLIST_ITEM_NONE, on_select_sync_jobs_cb, nf);
			break;
		case 1:
			it = elm_genlist_item_append(genlist, itc, setting_id, NULL, ELM_GENLIST_ITEM_NONE, on_select_sync_period_cb, nf);
			break;
		case 2:
			it = elm_genlist_item_append(genlist, itc, setting_id, NULL, ELM_GENLIST_ITEM_NONE, on_select_sync_capability_cb, nf);
			break;
		case 3:
			it = elm_genlist_item_append(genlist, itc, setting_id, NULL, ELM_GENLIST_ITEM_NONE, on_select_sync_option_cb, nf);
			break;
		default:
			break;
		}

		setting_id->item = it;
	}

	evas_object_smart_callback_add(genlist, "selected", genlist_selected_cb, NULL);
	elm_genlist_item_class_free(itc);

	syncBtn = elm_button_add(win);
	elm_object_text_set(syncBtn, "Start Sync");
	elm_object_part_content_set(layout, "sync_btn", syncBtn);

	if (is_accountless || (account_id != -1))
		evas_object_smart_callback_add(syncBtn, "clicked", cb_start_sync, nf);
	else
		evas_object_smart_callback_add(syncBtn, "clicked", notify_by_using_popup, nf);

	evas_object_show(syncBtn);
	viewData->syncBtn = syncBtn;

	clear_flag = 1;

	viewData->frequency = -1;
	manualObject = layout;

	nf_it = elm_naviframe_item_push(nf, "Start sync with settings", NULL, NULL, layout, NULL);
	elm_naviframe_item_pop_cb_set(nf_it, sync_job_naviframe_pop_cb, viewData);
}


bool
sync_adapter_sample_foreach_sync_job_cb(account_h account, const char *sync_job_name, const char *sync_capability, int sync_job_id, bundle* sync_job_user_data, void *user_data)
{
	char sync_job_info[MAX_SIZE];
	memset(sync_job_info, 0, sizeof(sync_job_info));

	if (sync_job_name) {
		if (!strcmp(sync_job_name, "OnDemand")) {
			on_demand_sync_job_id = sync_job_id;
			sprintf(sync_job_info, "[%d] %s %s", sync_job_id, strdup(sync_job_name), strdup(displayed_option));
			dlog_print(DLOG_INFO, LOG_TAG, "[%d] %s %s", sync_job_id, sync_job_name, displayed_option);
		} else if (!strcmp(sync_job_name, "Periodic")) {
			periodic_sync_job_id = sync_job_id;
			sprintf(sync_job_info, "[%d] %s %s", sync_job_id, strdup(sync_job_name), strdup(displayed_interval));
			dlog_print(DLOG_INFO, LOG_TAG, "[%d] %s %s", sync_job_id, sync_job_name, displayed_interval);
		}
	} else if (sync_capability) {
		if (!strcmp(sync_capability, SYNC_SUPPORTS_CAPABILITY_CALENDAR))
			sprintf(sync_job_info, "[%d] Data Change for %s", sync_job_id, strdup("Calendar"));
		else if (!strcmp(sync_capability, SYNC_SUPPORTS_CAPABILITY_CONTACT))
			sprintf(sync_job_info, "[%d] Data Change for %s", sync_job_id, strdup("Contact"));
		else if (!strcmp(sync_capability, SYNC_SUPPORTS_CAPABILITY_IMAGE))
			sprintf(sync_job_info, "[%d] Data Change for %s", sync_job_id, strdup("Image"));
		else if (!strcmp(sync_capability, SYNC_SUPPORTS_CAPABILITY_MUSIC))
			sprintf(sync_job_info, "[%d] Data Change for %s", sync_job_id, strdup("Music"));
		else if (!strcmp(sync_capability, SYNC_SUPPORTS_CAPABILITY_SOUND))
			sprintf(sync_job_info, "[%d] Data Change for %s", sync_job_id, strdup("Sound"));
		else if (!strcmp(sync_capability, SYNC_SUPPORTS_CAPABILITY_VIDEO))
			sprintf(sync_job_info, "[%d] Data Change for %s", sync_job_id, strdup("Video"));

		dlog_print(DLOG_INFO, LOG_TAG, "[%d] DataChange", sync_job_id);
	} else if (cnt_sync_jobs == 0) {
		return true;
	}

	int idx, temp_idx = 0;

	for (idx = 0; idx < MAX_NUM; idx++) {
		if (list_of_sync_jobs[idx][0] == '\0') {
			remove_sync_job[idx] = sync_job_id;
			temp_idx = idx;
			break;
		}
	}
	strcpy(list_of_sync_jobs[temp_idx], sync_job_info);
	dlog_print(DLOG_INFO, LOG_TAG, "copy to list_of_sync_jobs[%d] : %s", temp_idx, list_of_sync_jobs[temp_idx]);

	cnt_sync_jobs++;

	return true;
}


static void
check_changed_cb(void *data, Evas_Object *obj, void *event_info)
{
	int idx = (int)data;

	Eina_Bool state = elm_check_state_get(obj);
	dlog_print(DLOG_INFO, LOG_TAG, "sync_job_id: [%d], State: [%d]", remove_sync_job[idx], state);
	if (state)
		list_of_remove_sync_job[idx] = true;
	else
		list_of_remove_sync_job[idx] = false;
}


static Evas_Object*
create_check(Evas_Object *parent, list_item_data_s *foreach_id)
{
	Evas_Object *check;
	check = elm_check_add(parent);
	if (is_all_checked)
		elm_check_state_set(check, EINA_TRUE);
	else
		elm_check_state_set(check, EINA_FALSE);
	evas_object_smart_callback_add(check, "changed", check_changed_cb, (void *)foreach_id->index);
	return check;
}


static Evas_Object*
get_content_registered_sync_jobs(void *data, Evas_Object *obj, const char *part)
{
	foreach_id = (list_item_data_s *)data;

	if ((foreach_id->type / 3) % 4 == 1 && !strcmp("elm.swallow.icon", part))
		return NULL;
	else if ((foreach_id->type / 3) % 4 == 2 && !strcmp("elm.swallow.end", part))
		return create_check(obj, foreach_id);
	else if ((foreach_id->type / 3) % 4 == 3) {
		if (!strcmp("elm.swallow.icon", part))
			return NULL;
		else if (!strcmp("elm.swallow.end", part))
			return create_check(obj, foreach_id);
	}

	switch (foreach_id->type % 3) {
	case 0:
		return NULL;
	case 1:
		if (!strcmp("elm.swallow.icon.1", part))
			return NULL;
		else
			return NULL;
	case 2:
		return NULL;
	}

	return NULL;
}


static char*
get_text_registered_sync_jobs(void *data, Evas_Object *obj, const char *part)
{
	foreach_id = (list_item_data_s *)data;

	char buf[50];

	snprintf(buf, sizeof(buf), "%s", list_of_sync_jobs[foreach_id->index]);

	switch (foreach_id->type % 3) {
	case 0:
		if (!strcmp("elm.text", part))
			return strdup(buf);
		else return NULL;
	case 1:
		if (!strcmp("elm.text", part))
			return strdup(buf);
		else return NULL;
	case 2:
		if (!strcmp("elm.text", part))
			return strdup(buf);
		else if (!strcmp("elm.text.end", part))
			return strdup("Sub text");
		else return NULL;
	}

	return NULL;
}


void on_manage_sync_jobs_cb(void *data, Evas_Object *obj, void *event_info);

void
on_select_all_sync_jobs_cb(void *data, Evas_Object *obj, void *event_info)
{
	dlog_print(DLOG_INFO, LOG_TAG, "Enter select all sync jobs");

	if (!is_all_checked) {
		is_all_checked = true;
		memset(list_of_remove_sync_job, true, sizeof(list_of_remove_sync_job));
	} else {
		is_all_checked = false;
		memset(list_of_remove_sync_job, false, sizeof(list_of_remove_sync_job));
	}

	Evas_Object *nf = data;
	elm_naviframe_item_pop(nf);
	on_manage_sync_jobs_cb(data, obj, event_info);
}


void
on_remove_selected_sync_jobs_cb(void *data, Evas_Object *obj, void *event_info)
{
	dlog_print(DLOG_INFO, LOG_TAG, "Enter remove selected sync jobs");

	int idx, idx2;

	for (idx = 0; idx < cnt_sync_jobs; idx++) {
		if (list_of_remove_sync_job[idx]) {
			sync_manager_remove_sync_job(remove_sync_job[idx]);
			dlog_print(DLOG_INFO, LOG_TAG, "removed sync job: [%d]", remove_sync_job[idx]);
			list_of_remove_sync_job[idx] = false;
			for (idx2 = 0; idx2 < NUM_OF_CAPABILITY; idx2++) {
				if (data_change_id[idx2] == remove_sync_job[idx]) {
					data_change_id[idx2] = -1;
					break;
				}
			}
			remove_sync_job[idx] = -1;
		}
	}

	on_demand_sync_job_id = -1;
	periodic_sync_job_id = -1;

	if (is_all_checked)
		memset(data_change_id, -1, sizeof(data_change_id));

	Evas_Object *nf = data;
	elm_naviframe_item_pop(nf);
	on_manage_sync_jobs_cb(data, obj, event_info);
}


void
on_manage_sync_jobs_cb(void *data, Evas_Object *obj, void *event_info)
{
	Elm_Object_Item *nf_it, *it;
	viewdata_s* viewData = (viewdata_s *)malloc(sizeof(viewdata_s));
	Evas_Object *nf = data, *layout, *win, *removeBtn, *selectAllBtn, *genlist;
	viewData->nf = nf;
	Elm_Genlist_Item_Class *itc;
	layout = elm_layout_add(nf);
	char edj_path[PATH_MAX] = {0, };
	app_get_resource(EDJ_FILE, edj_path, (int)PATH_MAX);
	elm_layout_file_set(layout, edj_path, GRP_MAIN);

	viewData->layout = layout;
	win = nf;
	manualObject = layout;

	int idx, n_items;

	genlist = elm_genlist_add(nf);
	elm_genlist_block_count_set(genlist, 14);
	elm_genlist_mode_set(genlist, ELM_LIST_COMPRESS);
	elm_genlist_homogeneous_set(genlist, EINA_TRUE);
	elm_object_part_content_set(layout, "genlist_tiny2", genlist);

	memset(remove_sync_job, -1, sizeof(remove_sync_job));

	int ret = sync_manager_foreach_sync_job(sync_adapter_sample_foreach_sync_job_cb, NULL);

	for (idx = 0; idx < MAX_NUM; idx++) {
		if (list_of_sync_jobs[idx][0] != '\0')
			dlog_print(DLOG_INFO, LOG_TAG, "after copying to list_of_sync_jobs[%d] : %s", idx, list_of_sync_jobs[idx]);
	}

	itc = elm_genlist_item_class_new();
	itc->item_style = "type1";
	itc->func.content_get = get_content_registered_sync_jobs;
	itc->func.text_get = get_text_registered_sync_jobs;
	itc->func.del = gl_del_cb;

	n_items = cnt_sync_jobs;

	for (idx = 0; idx < n_items; idx++) {
		foreach_id = calloc(sizeof(list_item_data_s), 1);
		foreach_id->type = 6;
		foreach_id->index = idx;

		it = elm_genlist_item_append(genlist, itc, foreach_id, NULL, ELM_GENLIST_ITEM_TREE, NULL, nf);

		foreach_id->item = it;
	}

	evas_object_smart_callback_add(genlist, "selected", genlist_selected_cb, NULL);
	elm_genlist_item_class_free(itc);

	NF = nf;

	if (ret == SYNC_ERROR_NONE)
		evas_object_show(genlist);
	else
		dlog_print(DLOG_INFO, LOG_TAG, "Error %d", ret);

	selectAllBtn = elm_button_add(nf);
	elm_object_style_set(selectAllBtn, "naviframe/title_right");
	elm_object_text_set(selectAllBtn, "select all");
	evas_object_smart_callback_add(selectAllBtn, "clicked", on_select_all_sync_jobs_cb, nf);
	evas_object_show(selectAllBtn);
	viewData->selectAllBtn = selectAllBtn;

	removeBtn = elm_button_add(win);
	elm_object_text_set(removeBtn, "Remove");
	elm_object_part_content_set(layout, "remove_btn", removeBtn);
	evas_object_smart_callback_add(removeBtn, "clicked", on_remove_selected_sync_jobs_cb, nf);
	evas_object_show(removeBtn);
	viewData->removeBtn = removeBtn;

	nf_it = elm_naviframe_item_push(nf, "Manage sync jobs", NULL, selectAllBtn, layout, NULL);
	elm_object_item_part_content_set(nf_it, "title_right_btn", selectAllBtn);
	elm_naviframe_item_pop_cb_set(nf_it, foreach_naviframe_pop_cb, viewData);
}


static void
on_select_manage_sync_jobs_cb(void *data, Evas_Object *obj, void *event_info)
{
	is_all_checked = false;
	memset(list_of_remove_sync_job, false, sizeof(list_of_remove_sync_job));

	on_manage_sync_jobs_cb(data, obj, event_info);
}


static void
create_sync_main_menu(appdata_s *ad)
{
	app_control_h app_control;
	int ret = app_control_create(&app_control);
	if (ret != APP_CONTROL_ERROR_NONE) {
		dlog_print(DLOG_INFO, LOG_TAG, "Creating app_control handle is failed [%s]", get_error_message(ret));
		return;
	}

	ret = app_control_set_app_id(app_control, SYNC_ADAPTER_SERVICE_APP_ID);
	if (ret != APP_CONTROL_ERROR_NONE) {
		dlog_print(DLOG_INFO, LOG_TAG, "Setting app id is failed [%s]", get_error_message(ret));
		return;
	}

	ret = app_control_send_launch_request(app_control, NULL, NULL);
	if (ret == APP_CONTROL_ERROR_NONE)
		dlog_print(DLOG_INFO, LOG_TAG, "Launching sync service app successfully [%s]", SYNC_ADAPTER_SERVICE_APP_ID);
	else {
		dlog_print(DLOG_INFO, LOG_TAG, "Launching sync service app is failed [%s]", get_error_message(ret));
		return;
	}

	memset(list_of_sync_jobs, '\0', sizeof(list_of_sync_jobs));

	Evas_Object *nf = ad->nf, *genlist;
	Elm_Object_Item *nf_it, *it;
	Elm_Genlist_Item_Class *itc;
	int idx;

	itc = elm_genlist_item_class_new();
	itc->item_style = "type1";
	itc->func.text_get = get_text_main_menu;
	itc->func.del = gl_del_cb;

	genlist = elm_genlist_add(nf);
	elm_genlist_block_count_set(genlist, 14);
	elm_genlist_mode_set(genlist, ELM_LIST_COMPRESS);
	elm_genlist_homogeneous_set(genlist, EINA_TRUE);

	memset(data_change_id, -1, sizeof(data_change_id));

	for (idx = 0; idx < NUM_OF_ITEMS; idx++) {
		main_id = calloc(sizeof(list_item_data_s), 1);
		main_id->index = idx;

		switch (idx) {
		case 0:
			it = elm_genlist_item_append(genlist, itc, main_id, NULL, ELM_GENLIST_ITEM_NONE, on_create_account_sync_cb, nf);
			break;
		case 1:
			it = elm_genlist_item_append(genlist, itc, main_id, NULL, ELM_GENLIST_ITEM_NONE, on_select_accountless_sync_cb, nf);
			break;
		case 2:
			it = elm_genlist_item_append(genlist, itc, main_id, NULL, ELM_GENLIST_ITEM_NONE, on_select_settings_sync_cb, nf);
			break;
		case 3:
			it = elm_genlist_item_append(genlist, itc, main_id, NULL, ELM_GENLIST_ITEM_NONE, on_select_manage_sync_jobs_cb, nf);
			break;
		default:
			break;
		}

		main_id->item = it;
	}
	evas_object_smart_callback_add(genlist, "selected", genlist_selected_cb, NULL);
	elm_genlist_item_class_free(itc);

	NF = nf;

	nf_it = elm_naviframe_item_push(nf, "Sync Adapter App", NULL, NULL, genlist, NULL);
	elm_naviframe_item_pop_cb_set(nf_it, naviframe_pop_cb, ad->win);
}


static void
create_base_gui(appdata_s *ad)
{
	/* Window */
	ad->win = elm_win_util_standard_add(PACKAGE, PACKAGE);
	elm_win_conformant_set(ad->win, EINA_TRUE);
	elm_win_autodel_set(ad->win, EINA_TRUE);

	if (elm_win_wm_rotation_supported_get(ad->win)) {
		int rots[4] = { 0, 90, 180, 270 };
		elm_win_wm_rotation_available_rotations_set(ad->win, (const int *)(&rots), 4);
	}

	evas_object_smart_callback_add(ad->win, "delete,request", win_delete_request_cb, NULL);

	/* Conformant */
	ad->conform = elm_conformant_add(ad->win);
	elm_win_indicator_mode_set(ad->win, ELM_WIN_INDICATOR_SHOW);
	elm_win_indicator_opacity_set(ad->win, ELM_WIN_INDICATOR_OPAQUE);
	evas_object_size_hint_weight_set(ad->conform, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_win_resize_object_add(ad->win, ad->conform);
	evas_object_show(ad->conform);

	/* Base Layout */
	ad->layout = elm_layout_add(ad->conform);
	evas_object_size_hint_weight_set(ad->layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_layout_theme_set(ad->layout, "layout", "application", "default");
	evas_object_show(ad->layout);
	elm_object_content_set(ad->conform, ad->layout);

	/* Naviframe */
	ad->nf = elm_naviframe_add(ad->layout);
	elm_object_part_content_set(ad->layout, "elm.swallow.content", ad->nf);
	elm_naviframe_event_enabled_set(ad->nf, EINA_TRUE);
	eext_object_event_callback_add(ad->nf, EEXT_CALLBACK_BACK, eext_naviframe_back_cb, NULL);
	eext_object_event_callback_add(ad->nf, EEXT_CALLBACK_MORE, eext_naviframe_more_cb, NULL);

	/* Add sync main menu*/
	create_sync_main_menu(ad);

	/* Show window after base gui is set up */
	evas_object_show(ad->win);
}


static bool
app_create(void *data)
{
	/*
	 * Hook to take necessary actions before main event loop starts
	 * Initialize UI resources and application's data
	 * If this function returns true, the main loop of application starts
	 * If this function returns false, the application is terminated
	 */

	appdata_s *ad = data;
	create_base_gui(ad);

	return true;
}


static void
app_control(app_control_h app_control, void *data)
{
	char *operation;
	int ret = app_control_get_operation(app_control, &operation);
	if (ret != APP_CONTROL_ERROR_NONE) {
		dlog_print(DLOG_INFO, LOG_TAG, "failed to get operation");
		return;
	} else
		dlog_print(DLOG_INFO, LOG_TAG, "get operation: [%s]", operation);

	dlog_print(DLOG_INFO, LOG_TAG, "Sync Job completed by sync-service");

	if (strcmp(operation, "http://tizen.org/appcontrol/operation/default")) {
		Evas_Object *popup;
		Evas_Object *win = NF;

		popup = elm_popup_add(win);
		elm_popup_align_set(popup, ELM_NOTIFY_ALIGN_FILL, 1.0);
		eext_object_event_callback_add(popup, EEXT_CALLBACK_BACK, eext_popup_back_cb, NULL);
		evas_object_size_hint_weight_set(popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

		if (operation && !strcmp(operation, "http://tizen.org/appcontrol/operation/on_demand_sync_complete")) {
			dlog_print(DLOG_INFO, LOG_TAG, "On Demand Sync is completed");
			elm_object_text_set(popup, "On Demand Sync is completed");
		} else if (operation && !strcmp(operation, "http://tizen.org/appcontrol/operation/periodic_sync_complete")) {
			dlog_print(DLOG_INFO, LOG_TAG, "Periodic Sync is completed");
			elm_object_text_set(popup, "Periodic Sync is completed");
		} else if (operation && !strcmp(operation, "http://tizen.org/appcontrol/operation/data_change_sync_complete")) {
			dlog_print(DLOG_INFO, LOG_TAG, "Data Change Sync is completed");
			elm_object_text_set(popup, "Data Change Sync is completed");
		}

		elm_popup_timeout_set(popup, TIME_FOR_POPUP);
		evas_object_show(popup);
	}

	return;
}


static void
app_pause(void *data)
{
	/* Take necessary actions when application becomes invisible. */
}


static void
app_resume(void *data)
{
	/* Take necessary actions when application becomes visible. */
}


static void
app_terminate(void *data)
{
	/* Release all resources. */
}


static void
ui_app_low_battery(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_LOW_BATTERY*/
}


static void
ui_app_low_memory(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_LOW_MEMORY*/
}


static void
ui_app_orient_changed(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_DEVICE_ORIENTATION_CHANGED*/
	return;
}


static void
ui_app_lang_changed(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_LANGUAGE_CHANGED*/
	char *locale = NULL;
	system_settings_get_value_string(SYSTEM_SETTINGS_KEY_LOCALE_LANGUAGE, &locale);
	elm_language_set(locale);
	free(locale);
	return;
}


static void
ui_app_region_changed(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_REGION_FORMAT_CHANGED*/
}


int
main(int argc, char *argv[])
{
	appdata_s ad = {0,};
	int ret = 0;

	ui_app_lifecycle_callback_s event_callback = {0,};
	app_event_handler_h handlers[5] = {NULL, };

	event_callback.create = app_create;
	event_callback.terminate = app_terminate;
	event_callback.pause = app_pause;
	event_callback.resume = app_resume;
	event_callback.app_control = app_control;

	ui_app_add_event_handler(&handlers[APP_EVENT_LOW_BATTERY], APP_EVENT_LOW_BATTERY, ui_app_low_battery, &ad);
	ui_app_add_event_handler(&handlers[APP_EVENT_LOW_MEMORY], APP_EVENT_LOW_MEMORY, ui_app_low_memory, &ad);
	ui_app_add_event_handler(&handlers[APP_EVENT_DEVICE_ORIENTATION_CHANGED], APP_EVENT_DEVICE_ORIENTATION_CHANGED, ui_app_orient_changed, &ad);
	ui_app_add_event_handler(&handlers[APP_EVENT_LANGUAGE_CHANGED], APP_EVENT_LANGUAGE_CHANGED, ui_app_lang_changed, &ad);
	ui_app_add_event_handler(&handlers[APP_EVENT_REGION_FORMAT_CHANGED], APP_EVENT_REGION_FORMAT_CHANGED, ui_app_region_changed, &ad);
	ui_app_remove_event_handler(handlers[APP_EVENT_LOW_MEMORY]);

	ret = ui_app_main(argc, argv, &event_callback, &ad);
	if (ret != APP_ERROR_NONE)
		dlog_print(DLOG_ERROR, LOG_TAG, "ui_app_main() is failed. err = %d", ret);

	return ret;
}
